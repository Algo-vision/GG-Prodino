
#include "i2c_imu_gps.hpp"
#include <cmath>
#include <TinyGPSPlus.h> // Include TinyGPSPlus header

TinyGPSPlus gps; // Declare the TinyGPSPlus object globally within this file
char gpsBuffer[GPS_BUFFER_LEN]; // Declare gpsBuffer globally within this file

bool imu_initialized = false;
bool gps_conncted = false;
// ---- Helper functions for IMU ----
void imuWriteByte(uint8_t reg, uint8_t value)
{
    Wire.beginTransmission(IMU_ADDR);
    Wire.write(reg);
    Wire.write(value);
    Wire.endTransmission();
}
void initIMU()
{
    // Initialize IMU: 104 Hz, ±2g, 100 Hz filter
    imuWriteByte(LSM6DS3_CTRL1_XL, 0x60);
    // Initialize Gyro: 104 Hz, ±245 dps (not used now)
    imuWriteByte(LSM6DS3_CTRL2_G, 0x60);
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
    if (!imu_initialized)
    {
        initIMU();
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


    // Convert raw to g (±2g)
    ax = ax_raw * 0.000061;
    ay = ay_raw * 0.000061;
    az = az_raw * 0.000061;
    return true;
}

bool readGyroscope(float &gx, float &gy, float &gz)
{
    if (!imu_initialized)
    {
        initIMU();
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

    // Convert raw to dps (±245 dps)
    // Sensitivity for ±245 dps is 8.75 mdps/LSB = 0.00875 dps/LSB
    gx = gx_raw * 0.00875;
    gy = gy_raw * 0.00875;
    gz = gz_raw * 0.00875;
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
    return data.valid;
}
