
#include "i2c_imu_gps.hpp"
#include <cmath>
#include <TinyGPSPlus.h> // Include TinyGPSPlus header
#include <SparkFun_u-blox_GNSS_v3.h>

TinyGPSPlus gps; // Declare the TinyGPSPlus object globally within this file
char gpsBuffer[GPS_BUFFER_LEN]; // Declare gpsBuffer globally within this file

#if defined(TIMING_PROBE) && TIMING_PROBE
volatile uint32_t g_gpsI2CAddrFail = 0;
volatile uint32_t g_gpsI2CAvailFail = 0;
volatile uint32_t g_gpsPvtFresh = 0;
#define TP_COUNT(c) ((c)++)
#else
#define TP_COUNT(c) ((void)0)
#endif
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
        // UBX only. NMEA was the overwhelming majority of the bytes this
        // module put on the bus - measured at ~3 kB/s with both protocols
        // enabled, against ~500 B/s for the NAV-PVT messages alone - and
        // every one of those bytes had to be clocked across a 100 kHz bus
        // by the main loop. NAV-PVT already carries position, velocity,
        // heading, satellite count, accuracy and time, so the NMEA stream
        // was duplicating what we were already receiving in binary.
        //
        // Turning it off also ends a race: the manual I2C read fed
        // TinyGPSPlus while getPVT() drained the remainder into the UBX
        // parser, so each parser saw an arbitrary fraction of the stream.
        // There is now one reader and one parser.
        myGNSS.setI2COutput(COM_TYPE_UBX);
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

/** Decode one 14-byte OUT_TEMP_L..OUTZ_H_XL burst: temp (2), gyro (6), accel (6). */
static void decodeImuBurst(const uint8_t *raw,
                           float &ax, float &ay, float &az,
                           float &gx, float &gy, float &gz, float &tempC)
{
    int16_t t = (int16_t)(raw[1] << 8 | raw[0]);
    tempC = LSM6DS3_TEMP_REFERENCE_C + (t / LSM6DS3_TEMP_SENSITIVITY_LSB_PER_C);

    // ±500 dps: 17.50 mdps/LSB
    gx = (int16_t)(raw[3] << 8 | raw[2]) * 0.0175;
    gy = (int16_t)(raw[5] << 8 | raw[4]) * 0.0175;
    gz = (int16_t)(raw[7] << 8 | raw[6]) * 0.0175;

    // ±8g: 0.244 mg/LSB
    ax = (int16_t)(raw[9]  << 8 | raw[8])  * 0.000244;
    ay = (int16_t)(raw[11] << 8 | raw[10]) * 0.000244;
    az = (int16_t)(raw[13] << 8 | raw[12]) * 0.000244;
}

/**
 * @brief Temperature, gyro and accel in ONE bus transaction.
 *
 * OUT_TEMP_L (0x20) through OUTZ_H_XL (0x2D) are contiguous on the LSM6DS3,
 * and IF_INC is set at init, so a single auto-incremented 14-byte read
 * returns all three. Read separately they cost three transactions - each
 * with its own START, two address bytes and a register byte - which at
 * 100 kHz is ~2.2 ms per IMU per update, ~45% of the controller's entire
 * second across both IMUs. The burst is ~1.5 ms for the same registers.
 *
 * The three sensors are also sampled from the SAME instant this way, where
 * the split reads gave the filter an accel and a gyro from readings up to
 * a millisecond apart.
 */
bool readImuAll(float &ax, float &ay, float &az,
                float &gx, float &gy, float &gz, float &tempC)
{
    if (!ensureImuInit(imu_initialized, s_lastImu1InitAttempt, initIMU))
    {
        ax = ay = az = gx = gy = gz = tempC = 0.0f;
        return false; // not initialized and too soon to retry
    }

    uint8_t rawData[14] = {0};
    if (!imuReadBytes(LSM6DS3_OUT_TEMP_L, rawData, 14)) {
        ax = ay = az = gx = gy = gz = tempC = 0.0f;
        imu_initialized = false;
        return false;
    }
    decodeImuBurst(rawData, ax, ay, az, gx, gy, gz, tempC);
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

/** See readImuAll() - the same single-transaction read, for IMU2. */
bool readImuAll_2(float &ax, float &ay, float &az,
                  float &gx, float &gy, float &gz, float &tempC)
{
    if (!ensureImuInit(imu2_initialized, s_lastImu2InitAttempt, initIMU_2))
    {
        ax = ay = az = gx = gy = gz = tempC = 0.0f;
        return false; // not initialized and too soon to retry
    }

    uint8_t rawData[14] = {0};
    if (!imu2ReadBytes(LSM6DS3_OUT_TEMP_L, rawData, 14)) {
        ax = ay = az = gx = gy = gz = tempC = 0.0f;
        imu2_initialized = false;
        return false;
    }
    decodeImuBurst(rawData, ax, ay, az, gx, gy, gz, tempC);
    return true;
}



/** Last complete solution. getPVT() only reports fresh data when the module
 *  has actually produced a new one - roughly 5 times a second - while this is
 *  called on every loop pass, so the caller needs something to read in
 *  between. The struct it hands us is a fresh local each time. */
static gps_data s_lastFix;
static bool s_lastFixInit = false;

bool readGPSCoords(gps_data &data)
{
    if (!s_lastFixInit) {
        memset(&s_lastFix, 0, sizeof(s_lastFix));
        s_lastFixInit = true;
    }

    // Does the module still answer at all? One address byte, and it is the
    // only way to tell 'no new fix yet' apart from 'the module is gone'.
    Wire.beginTransmission(GPS_ADDR);
    if (Wire.endTransmission() != 0) {
        TP_COUNT(g_gpsI2CAddrFail);
        gps_conncted = false;
        memset(&s_lastFix, 0, sizeof(s_lastFix));
        data = s_lastFix;
        return false;
    }
    gps_conncted = true;

    if (!gnss_initialized) {
        data = s_lastFix;
        return data.valid;
    }

    // With setAutoPVT the module pushes NAV-PVT on its own and this does not
    // block: it reads whatever is waiting, parses it, and returns true only
    // when a new solution actually arrived. Nothing is left in the module's
    // buffer, which is what the library requires - stopping early loses the
    // bytes that were already fetched.
    if (myGNSS.getPVT()) {
        TP_COUNT(g_gpsPvtFresh);
        s_lastFix.latitude     = myGNSS.getLatitude()  / 10000000.0;
        s_lastFix.longitude    = myGNSS.getLongitude() / 10000000.0;
        s_lastFix.altitude     = myGNSS.getAltitudeMSL() / 1000.0;   // mm -> m
        s_lastFix.altEllipsoid = (double)myGNSS.getAltitude();       // mm
        s_lastFix.satellites   = myGNSS.getSIV();
        s_lastFix.hAcc         = (float)myGNSS.getHorizontalAccEst();
        s_lastFix.vAcc         = (float)myGNSS.getVerticalAccEst();

        // getGroundSpeed is mm/s; the API reports km/h.
        s_lastFix.ground_speed = myGNSS.getGroundSpeed() * 0.0036f;

        // getHeading is degrees x 1e-5, heading OF MOTION - course over
        // ground, not where the machine points. Below walking pace it is
        // noise, so it is suppressed exactly as the previous code did.
        float course_deg = myGNSS.getHeading() / 100000.0f;
        if (course_deg < 0.0f || course_deg > 360.0f) {
            course_deg = 0.0f;
        }
        if (s_lastFix.ground_speed < 1.0f) {
            course_deg = 0.0f;
        }
        s_lastFix.heading     = course_deg;
        float heading_rad     = course_deg * 3.14159265358979323846f / 180.0f;
        s_lastFix.speed_north = s_lastFix.ground_speed * cos(heading_rad);
        s_lastFix.speed_east  = s_lastFix.ground_speed * sin(heading_rad);
        s_lastFix.speed_down  = 0.0f;  // Placeholder

        uint16_t year = myGNSS.getYear();
        if (year > 2020) {
            sprintf(s_lastFix.time_str, "%04d-%02d-%02d %02d:%02d:%02d",
                    year, myGNSS.getMonth(), myGNSS.getDay(),
                    myGNSS.getHour(), myGNSS.getMinute(), myGNSS.getSecond());
        } else {
            s_lastFix.time_str[0] = '\0';
        }

        // Same three conditions the NMEA path used: a real position, at least
        // one satellite, and a date that is not cold-start junk. getGnssFixOk
        // is the module's own verdict and is stricter than any of them.
        bool locationValid = myGNSS.getGnssFixOk() &&
                             (fabs(s_lastFix.latitude) > 0.0001 ||
                              fabs(s_lastFix.longitude) > 0.0001);
        s_lastFix.valid = locationValid && (s_lastFix.satellites >= 1) && (year > 2020);
    }

    data = s_lastFix;
    return data.valid;
}
