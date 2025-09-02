
#include "i2c_imu_gps.hpp"
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

    Serial.print("Raw Accel: ");
    Serial.print(ax_raw);
    Serial.print(", ");
    Serial.print(ay_raw);
    Serial.print(", ");
    Serial.println(az_raw);

    // Convert raw to g (±2g)
    ax = ax_raw * 0.000061;
    ay = ay_raw * 0.000061;
    az = az_raw * 0.000061;
    return true;
}


bool readGPSCoords(double &lat, double &lng, double &alt)
{
    Wire.beginTransmission(GPS_ADDR);
    byte error = Wire.endTransmission();
    if (error != 0)
    {
        gps_conncted = false;
        lat = lng = alt = 0.0;
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
    if (gps.location.isUpdated())
    {
        lat = gps.location.lat();
        lng = gps.location.lng();
        alt = gps.altitude.meters();
        return true;
    }
    return false;
}
