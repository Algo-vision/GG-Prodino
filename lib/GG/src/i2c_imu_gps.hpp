#pragma once

#include <Wire.h>
#include <TinyGPSPlus.h>

// IMU (LSM6DS3) I2C address
#define IMU_ADDR 0x6A
#define LSM6DS3_CTRL1_XL 0x10
#define LSM6DS3_CTRL2_G 0x11
#define LSM6DS3_OUTX_L_XL 0x28
#define LSM6DS3_OUTX_L_G 0x22

// GPS (u-blox) I2C address
#define GPS_ADDR 0x42
#define GPS_BUFFER_LEN 128

// Struct to hold GPS data
struct gps_data {
  double latitude;
  double longitude;
  double altitude;
  char time_str[20]; // YYYY-MM-DD hh:mm:ss\0
  float speed_north;
  float speed_east;
  float speed_down;
  float ground_speed;
  float heading;
  bool valid;
  uint8_t satellites;  // Number of satellites used for position fix
};

// Externally defined TinyGPSPlus object and buffer
extern TinyGPSPlus gps;
extern char gpsBuffer[GPS_BUFFER_LEN];
extern bool gps_conncted;
// IMU helper functions
void imuWriteByte(uint8_t reg, uint8_t value);
bool imuReadBytes(uint8_t reg, uint8_t *data, uint8_t len);
bool readAccelerometer(float &ax, float &ay, float &az);
bool readGyroscope(float &gx, float &gy, float &gz);
void initIMU();

// GPS helper function
bool readGPSCoords(gps_data &data);
