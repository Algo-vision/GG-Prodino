#pragma once

#include <Wire.h>
#include <TinyGPSPlus.h>

// IMU (LSM6DS3) I2C address
#define IMU_ADDR 0x6A
#define LSM6DS3_CTRL1_XL 0x10
#define LSM6DS3_CTRL2_G 0x11
#define LSM6DS3_OUTX_L_XL 0x28

// GPS (u-blox) I2C address
#define GPS_ADDR 0x42
#define GPS_BUFFER_LEN 128

// Externally defined TinyGPSPlus object and buffer
extern TinyGPSPlus gps;
extern char gpsBuffer[GPS_BUFFER_LEN];
extern bool gps_conncted;
// IMU helper functions
void imuWriteByte(uint8_t reg, uint8_t value);
bool imuReadBytes(uint8_t reg, uint8_t *data, uint8_t len);
bool readAccelerometer(float &ax, float &ay, float &az);
void initIMU();

// GPS helper function
bool readGPSCoords(double &lat, double &lng,double &alt);
