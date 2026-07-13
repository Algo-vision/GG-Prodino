#pragma once

#include <Wire.h>
#include <TinyGPSPlus.h>
#include <SparkFun_u-blox_GNSS_v3.h>

// IMU (LSM6DS3) I2C addresses - two IMUs on the MSB, IMU2 mounted 180 deg
// rotated from IMU1 in-plane (X/Y flip, Z/yaw unchanged) to cancel drift
// when fused. IMU1 = U8, IMU2 = U7 on the MSB silkscreen.
#define IMU_ADDR 0x6A
#define IMU_2_ADDR 0x6B
#define LSM6DS3_CTRL1_XL 0x10
#define LSM6DS3_CTRL2_G 0x11
#define LSM6DS3_CTRL3_C 0x12
#define LSM6DS3_CTRL8_XL 0x17
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
  float hAcc;          // Horizontal accuracy estimate (mm)
  float vAcc;          // Vertical accuracy estimate (mm)
  double altEllipsoid; // Height above WGS84 ellipsoid (mm)
};

// Externally defined TinyGPSPlus object and buffer
extern TinyGPSPlus gps;
extern char gpsBuffer[GPS_BUFFER_LEN];
extern bool gps_conncted;
extern SFE_UBLOX_GNSS myGNSS;

// u-blox GNSS initialization (for UBX protocol - hAcc/vAcc)
void initUbloxGNSS();
// IMU helper functions
void imuWriteByte(uint8_t reg, uint8_t value);
bool imuReadBytes(uint8_t reg, uint8_t *data, uint8_t len);
bool readAccelerometer(float &ax, float &ay, float &az);
bool readGyroscope(float &gx, float &gy, float &gz);
void initIMU();

// IMU2 helper functions
bool readAccelerometer_2(float &ax, float &ay, float &az);
bool readGyroscope_2(float &gx, float &gy, float &gz);
void initIMU_2();

// GPS helper function
bool readGPSCoords(gps_data &data);

// Note: IMU mount orientation (ImuMountOrientation enum + remap math) lives
// in imu_mount_orientation.hpp - a separate, hardware-free module so it can
// be unit-tested natively. Include that header directly where needed.
