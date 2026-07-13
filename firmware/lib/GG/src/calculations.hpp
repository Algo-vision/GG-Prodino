#pragma once

// Calculates pitch, roll, and yaw from a single IMU using a complementary filter.
// Pitch/roll are corrected against the accelerometer's gravity vector (long-term
// reference); yaw is pure gyro integration (no accelerometer reference exists for
// rotation about the vertical/gravity axis) and will drift over time.
// @param pitch, roll, yaw: (in/out) Current orientation values, updated by the function.
// @param ax, ay, az: Accelerometer readings in g's.
// @param gx, gy, gz: Gyroscope readings in degrees/sec.
// @param dt: Time delta in seconds.
// @param ax_offset, ay_offset: Accelerometer offsets for calibration.
void calculateOrientation_1(float &pitch, float &roll, float &yaw,
                          float ax, float ay, float az,
                          float gx, float gy, float gz, float dt,
                          float ax_offset, float ay_offset);

// Same as calculateOrientation_1, but for IMU2, which is mounted 180 degrees
// rotated from IMU1 in the X/Y plane (the accelerometer/gyro X and Y signs are
// flipped relative to IMU1; Z/yaw is unaffected since the rotation is about
// the vertical axis).
void calculateOrientation_2(float &pitch, float &roll, float &yaw,
                          float ax, float ay, float az,
                          float gx, float gy, float gz, float dt,
                          float ax_offset, float ay_offset);

// Fuses both IMUs by averaging their sign-corrected accelerometer angles and
// gyro rates into a common frame before applying a single complementary
// filter. Used when both IMUs report valid data, for a less noisy estimate
// than either alone.
void calculateMergedOrientation(float &pitch, float &roll, float &yaw,
                                float ax1, float ay1, float az1,
                                float gx1, float gy1, float gz1,
                                float ax2, float ay2, float az2,
                                float gx2, float gy2, float gz2,
                                float dt,
                                float ax1_offset, float ay1_offset,
                                float ax2_offset, float ay2_offset);
