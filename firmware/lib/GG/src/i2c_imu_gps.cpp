
#include "i2c_imu_gps.hpp"
#include <cmath>
#include <TinyGPSPlus.h> // Include TinyGPSPlus header
#include <SparkFun_u-blox_GNSS_v3.h>

TinyGPSPlus gps; // Declare the TinyGPSPlus object globally within this file
char gpsBuffer[GPS_BUFFER_LEN]; // Declare gpsBuffer globally within this file
SFE_UBLOX_GNSS myGNSS; // SparkFun u-blox GNSS for UBX protocol (hAcc/vAcc)

bool imu_initialized = false;
bool imu2_initialized = false;
bool gps_conncted = false;
bool gnss_initialized = false;

// Rate-limit IMU (re)initialization. initIMU() does a software reset with a
// blocking delay; without this, a missing or intermittently-failing IMU would
// re-run that reset on *every* read (each failed read clears the init flag),
// costing ~20ms per read and stalling statusUpdate/HTTP. With a real IMU
// present, init succeeds once and this never triggers again.
static const unsigned long IMU_INIT_RETRY_MS = 2000;
static unsigned long s_lastImu1InitAttempt = 0;
static unsigned long s_lastImu2InitAttempt = 0;

// Ensure an IMU is initialized before a read, but only retry init at most once
// per IMU_INIT_RETRY_MS. Returns false if init is not (yet) done and it's too
// soon to retry - in which case the caller should skip the read.
static bool ensureImuInit(bool &initFlag, unsigned long &lastAttempt, void (*initFn)())
{
    if (initFlag) return true;
    unsigned long now = millis();
    if (lastAttempt != 0 && (now - lastAttempt) < IMU_INIT_RETRY_MS) {
        return false; // too soon to retry
    }
    lastAttempt = (now == 0) ? 1 : now; // avoid the 0 "never attempted" sentinel
    initFn();                            // sets initFlag = true
    return true;
}

void initUbloxGNSS()
{
    if (myGNSS.begin(Wire, GPS_ADDR)) {
        // Disable NMEA output on I2C to avoid conflicts with TinyGPSPlus
        // We only want UBX NAV-PVT for accuracy data
        myGNSS.setI2COutput(COM_TYPE_UBX | COM_TYPE_NMEA);
        myGNSS.setNavigationFrequency(5); // 5Hz to match our update rate
        myGNSS.setAutoPVT(true); // Enable automatic NAV-PVT messages
        gnss_initialized = true;
        Serial.println("u-blox GNSS (UBX) initialized for accuracy data");
    } else {
        gnss_initialized = false;
        Serial.println("WARNING: u-blox GNSS (UBX) init failed - accuracy data unavailable");
    }
}
// ---- Helper functions for IMU1 (0x6A) ----
void imuWriteByte(uint8_t reg, uint8_t value)
{
    Wire.beginTransmission(IMU_ADDR);
    Wire.write(reg);
    Wire.write(value);
    Wire.endTransmission();
}
void initIMU()
{
    // Software reset to ensure clean state
    imuWriteByte(LSM6DS3_CTRL3_C, 0x01); // SW_RESET bit
    delay(20); // Wait for reset to complete

    // CTRL3_C: BDU=1 (block data update, avoids reading torn samples), IF_INC=1 (auto-increment)
    imuWriteByte(LSM6DS3_CTRL3_C, 0x04);

    // Accel: 104 Hz, ±8g - harsh-terrain tuning (matches reference V1.3.2.1/V1.3.6)
    imuWriteByte(LSM6DS3_CTRL1_XL, 0x4C);

    // Enable hardware LPF2 anti-aliasing filter, cutoff at ODR/9 = 11.55 Hz
    imuWriteByte(LSM6DS3_CTRL8_XL, 0xC0);

    // Gyro: 104 Hz, ±500 dps
    imuWriteByte(LSM6DS3_CTRL2_G, 0x44);
    imu_initialized = true;
}

bool imuReadBytes(uint8_t reg, uint8_t *data, uint8_t len)
{
    byte error;
    Wire.beginTransmission(IMU_ADDR);
    Wire.write(reg);
    error = Wire.endTransmission(false);
    Wire.requestFrom(IMU_ADDR, len);
    if (error != 0) {
        return false; // Error in communication
    }
    for (uint8_t i = 0; i < len; i++)
    {
        if (Wire.available())
            data[i] = Wire.read();
    }
    return true;
}

bool readAccelerometer(float &ax, float &ay, float &az)
{
    if (!ensureImuInit(imu_initialized, s_lastImu1InitAttempt, initIMU))
    {
        ax = ay = az = 0.0;
        return false; // not initialized and too soon to retry
    }

    uint8_t rawData[6]={0,0,0,0,0,0};
    bool valid = imuReadBytes(LSM6DS3_OUTX_L_XL, rawData, 6);
    if (!valid) {
        ax = ay = az = 0.0;
        imu_initialized = false;
        return valid; // Error reading data
    }
    int16_t ax_raw = (int16_t)(rawData[1] << 8 | rawData[0]);
    int16_t ay_raw = (int16_t)(rawData[3] << 8 | rawData[2]);
    int16_t az_raw = (int16_t)(rawData[5] << 8 | rawData[4]);

    // Convert raw to g (±8g sensitivity: 0.244 mg/LSB = 0.000244 g/LSB)
    ax = ax_raw * 0.000244;
    ay = ay_raw * 0.000244;
    az = az_raw * 0.000244;
    return true;
}

bool readGyroscope(float &gx, float &gy, float &gz)
{
    if (!ensureImuInit(imu_initialized, s_lastImu1InitAttempt, initIMU))
    {
        gx = gy = gz = 0.0;
        return false; // not initialized and too soon to retry
    }

    uint8_t rawData[6]={0,0,0,0,0,0};
    bool valid = imuReadBytes(LSM6DS3_OUTX_L_G, rawData, 6);
    if (!valid) {
        gx = gy = gz = 0.0;
        imu_initialized = false;
        return valid; // Error reading data
    }
    int16_t gx_raw = (int16_t)(rawData[1] << 8 | rawData[0]);
    int16_t gy_raw = (int16_t)(rawData[3] << 8 | rawData[2]);
    int16_t gz_raw = (int16_t)(rawData[5] << 8 | rawData[4]);

    // Convert raw to dps (±500 dps sensitivity: 17.50 mdps/LSB = 0.0175 dps/LSB)
    gx = gx_raw * 0.0175;
    gy = gy_raw * 0.0175;
    gz = gz_raw * 0.0175;
    return true;
}

bool readImuTemperature(float &tempC)
{
    if (!ensureImuInit(imu_initialized, s_lastImu1InitAttempt, initIMU))
    {
        tempC = 0.0f;
        return false;
    }

    uint8_t rawData[2] = {0, 0};
    if (!imuReadBytes(LSM6DS3_OUT_TEMP_L, rawData, 2)) {
        tempC = 0.0f;
        imu_initialized = false;
        return false;
    }

    int16_t raw = (int16_t)(rawData[1] << 8 | rawData[0]);
    tempC = LSM6DS3_TEMP_REFERENCE_C + (raw / LSM6DS3_TEMP_SENSITIVITY_LSB_PER_C);
    return true;
}

// ---- Helper functions for IMU2 (0x6B) - mounted 180 deg rotated from IMU1 ----
void imu2WriteByte(uint8_t reg, uint8_t value)
{
    Wire.beginTransmission(IMU_2_ADDR);
    Wire.write(reg);
    Wire.write(value);
    Wire.endTransmission();
}
void initIMU_2()
{
    // Software reset to ensure clean state
    imu2WriteByte(LSM6DS3_CTRL3_C, 0x01); // SW_RESET bit
    delay(20); // Wait for reset to complete

    // CTRL3_C: BDU=1 (block data update, avoids reading torn samples), IF_INC=1 (auto-increment)
    imu2WriteByte(LSM6DS3_CTRL3_C, 0x04);

    // Accel: 104 Hz, ±8g - harsh-terrain tuning (matches reference V1.3.2.1/V1.3.6)
    imu2WriteByte(LSM6DS3_CTRL1_XL, 0x4C);

    // Enable hardware LPF2 anti-aliasing filter, cutoff at ODR/9 = 11.55 Hz
    imu2WriteByte(LSM6DS3_CTRL8_XL, 0xC0);

    // Gyro: 104 Hz, ±500 dps
    imu2WriteByte(LSM6DS3_CTRL2_G, 0x44);
    imu2_initialized = true;
}

bool imu2ReadBytes(uint8_t reg, uint8_t *data, uint8_t len)
{
    byte error;
    Wire.beginTransmission(IMU_2_ADDR);
    Wire.write(reg);
    error = Wire.endTransmission(false);
    Wire.requestFrom(IMU_2_ADDR, len);
    if (error != 0) {
        return false; // Error in communication
    }
    for (uint8_t i = 0; i < len; i++)
    {
        if (Wire.available())
            data[i] = Wire.read();
    }
    return true;
}

bool readAccelerometer_2(float &ax, float &ay, float &az)
{
    if (!ensureImuInit(imu2_initialized, s_lastImu2InitAttempt, initIMU_2))
    {
        ax = ay = az = 0.0;
        return false; // not initialized and too soon to retry
    }

    uint8_t rawData[6]={0,0,0,0,0,0};
    bool valid = imu2ReadBytes(LSM6DS3_OUTX_L_XL, rawData, 6);
    if (!valid) {
        ax = ay = az = 0.0;
        imu2_initialized = false;
        return valid; // Error reading data
    }
    int16_t ax_raw = (int16_t)(rawData[1] << 8 | rawData[0]);
    int16_t ay_raw = (int16_t)(rawData[3] << 8 | rawData[2]);
    int16_t az_raw = (int16_t)(rawData[5] << 8 | rawData[4]);

    // Convert raw to g (±8g sensitivity: 0.244 mg/LSB = 0.000244 g/LSB)
    ax = ax_raw * 0.000244;
    ay = ay_raw * 0.000244;
    az = az_raw * 0.000244;
    return true;
}

bool readGyroscope_2(float &gx, float &gy, float &gz)
{
    if (!ensureImuInit(imu2_initialized, s_lastImu2InitAttempt, initIMU_2))
    {
        gx = gy = gz = 0.0;
        return false; // not initialized and too soon to retry
    }

    uint8_t rawData[6]={0,0,0,0,0,0};
    bool valid = imu2ReadBytes(LSM6DS3_OUTX_L_G, rawData, 6);
    if (!valid) {
        gx = gy = gz = 0.0;
        imu2_initialized = false;
        return valid; // Error reading data
    }
    int16_t gx_raw = (int16_t)(rawData[1] << 8 | rawData[0]);
    int16_t gy_raw = (int16_t)(rawData[3] << 8 | rawData[2]);
    int16_t gz_raw = (int16_t)(rawData[5] << 8 | rawData[4]);

    // Convert raw to dps (±500 dps sensitivity: 17.50 mdps/LSB = 0.0175 dps/LSB)
    gx = gx_raw * 0.0175;
    gy = gy_raw * 0.0175;
    gz = gz_raw * 0.0175;
    return true;
}

bool readImuTemperature_2(float &tempC)
{
    if (!ensureImuInit(imu2_initialized, s_lastImu2InitAttempt, initIMU_2))
    {
        tempC = 0.0f;
        return false;
    }

    uint8_t rawData[2] = {0, 0};
    if (!imu2ReadBytes(LSM6DS3_OUT_TEMP_L, rawData, 2)) {
        tempC = 0.0f;
        imu2_initialized = false;
        return false;
    }

    int16_t raw = (int16_t)(rawData[1] << 8 | rawData[0]);
    tempC = LSM6DS3_TEMP_REFERENCE_C + (raw / LSM6DS3_TEMP_SENSITIVITY_LSB_PER_C);
    return true;
}


bool readGPSCoords(gps_data &data)
{
    Wire.beginTransmission(GPS_ADDR);
    byte error = Wire.endTransmission();
    if (error != 0)
    {
        gps_conncted = false;
        data.latitude = 0.0;
        data.longitude = 0.0;
        data.altitude = 0.0;
        data.time_str[0] = '\0'; // Clear time string
        data.speed_north = 0.0;
        data.speed_east = 0.0;
        data.speed_down = 0.0;
        data.ground_speed = 0.0;
        data.heading = 0.0;
        data.valid = false;
        data.satellites = 0;
        data.hAcc = 0.0;
        data.vAcc = 0.0;
        data.altEllipsoid = 0.0;
        return false; // GPS not connected
    }
    gps_conncted = true;
    // Read GPS data from I2C and feed TinyGPSPlus
    Wire.requestFrom(GPS_ADDR, (uint8_t)GPS_BUFFER_LEN);
    uint8_t i = 0;
    while (Wire.available() && i < GPS_BUFFER_LEN - 1)
    {
        char c = Wire.read();
        gpsBuffer[i++] = c;
        gps.encode(c); // feed TinyGPSPlus parser
    }
    gpsBuffer[i] = '\0'; // Null-terminate the buffer

    // Directly use the parsed data from TinyGPSPlus
    data.latitude = gps.location.lat();
    data.longitude = gps.location.lng();
    data.altitude = gps.altitude.meters();
    data.satellites = gps.satellites.isValid() ? gps.satellites.value() : 0;
    
    if (gps.date.isValid() && gps.time.isValid()) {
        sprintf(data.time_str, "%04d-%02d-%02d %02d:%02d:%02d",
                gps.date.year(), gps.date.month(), gps.date.day(),
                gps.time.hour(), gps.time.minute(), gps.time.second());
    } else {
        data.time_str[0] = '\0';
    }

    data.ground_speed = gps.speed.kmph(); // Ground speed in km/h
    if (gps.course.isValid()) {
        float course_deg = gps.course.deg();
        
        // Debugging for date/timestamp mixup issue
        Serial.print("DEBUG: Raw GPS Course: "); Serial.println(course_deg);
        Serial.print("DEBUG: GPS Date: "); Serial.println(gps.date.value());

        // Range validation: if not in [0, 360], reset to 0
        if (course_deg < 0.0 || course_deg > 360.0) {
            course_deg = 0.0;
        }

        // COG is only valid if moving. Filter out noise if speed is too low (< 1.0 km/h)
        if (data.ground_speed < 1.0) {
             course_deg = 0.0; // Or keep previous value, but 0 is safer for now
        }

        // Convert to radians ONLY for speed calculation (sin/cos expect radians)
        float heading_rad = course_deg * 3.14159265358979323846 / 180.0; 
        data.speed_north = data.ground_speed * cos(heading_rad);
        data.speed_east = data.ground_speed * sin(heading_rad);
        
        // Return the heading in degrees
        data.heading = course_deg;
    } else {
        data.speed_north = 0.0;
        data.speed_east = 0.0;
        data.heading = 0.0; 
    }
    data.speed_down = 0.0;  // Placeholder

    // Robust GPS validity check:
    // 1. Location must be valid AND not at 0,0 (ocean/default value)
    // 2. Must have at least 1 satellite
    // 3. Year must be > 2020 (sanity check - filters cold start junk dates like 1980/2000)
    bool locationValid = gps.location.isValid() && 
                         (fabs(data.latitude) > 0.0001 || fabs(data.longitude) > 0.0001);
    bool hasEnoughSatellites = data.satellites >= 1;
    bool dateReasonable = gps.date.isValid() && gps.date.year() > 2020;

    data.valid = locationValid && hasEnoughSatellites && dateReasonable;

    // Read accuracy data from UBX protocol (SparkFun library)
    if (gnss_initialized) {
        // getPVT() returns true if fresh NAV-PVT data is available
        if (myGNSS.getPVT()) {
            data.hAcc = (float)myGNSS.getHorizontalAccEst(); // mm
            data.vAcc = (float)myGNSS.getVerticalAccEst();   // mm
            data.altEllipsoid = (double)myGNSS.getAltitude(); // mm
        }
    } else {
        data.hAcc = 0.0;
        data.vAcc = 0.0;
        data.altEllipsoid = 0.0;
    }

    return data.valid;
}
