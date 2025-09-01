
#include "../include/i2c_imu_gps.hpp"

// ---- Helper functions for IMU ----
void imuWriteByte(uint8_t reg, uint8_t value)
{
    Wire.beginTransmission(IMU_ADDR);
    Wire.write(reg);
    Wire.write(value);
    Wire.endTransmission();
}

void imuReadBytes(uint8_t reg, uint8_t *data, uint8_t len)
{
    Wire.beginTransmission(IMU_ADDR);
    Wire.write(reg);
    Wire.endTransmission(false);
    Wire.requestFrom(IMU_ADDR, len);
    for (uint8_t i = 0; i < len; i++)
    {
        if (Wire.available())
            data[i] = Wire.read();
    }
}

void readAccelerometer(float &ax, float &ay, float &az)
{
    uint8_t rawData[6];
    imuReadBytes(LSM6DS3_OUTX_L_XL, rawData, 6);

    int16_t ax_raw = (int16_t)(rawData[1] << 8 | rawData[0]);
    int16_t ay_raw = (int16_t)(rawData[3] << 8 | rawData[2]);
    int16_t az_raw = (int16_t)(rawData[5] << 8 | rawData[4]);

    // Convert raw to g (±2g)
    ax = ax_raw * 0.000061;
    ay = ay_raw * 0.000061;
    az = az_raw * 0.000061;
}


void initIMU()
{
    // Initialize IMU: 104 Hz, ±2g, 100 Hz filter
    imuWriteByte(LSM6DS3_CTRL1_XL, 0x60);
    // Initialize Gyro: 104 Hz, ±245 dps (not used now)
    imuWriteByte(LSM6DS3_CTRL2_G, 0x60);
}

bool readGPSCoords(double &lat, double &lng)
{

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
        return true;
    }
    return false;
}

