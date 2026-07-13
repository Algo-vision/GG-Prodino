#pragma once

// Calculates pitch and roll using a complementary filter.
// @param pitch: (in/out) The current pitch value, updated by the function.
// @param roll: (in/out) The current roll value, updated by the function.
// @param ax, ay, az: Accelerometer readings in g's.
// @param gx, gy: Gyroscope readings in degrees/sec.
// @param dt: Time delta in seconds.
// @param ax_offset, ay_offset: Accelerometer offsets for calibration.
void calculatePitchRoll(float &pitch, float &roll, float ax, float ay, float az, float gx, float gy, float dt, float ax_offset, float ay_offset);

// Calculates yaw by integrating gyroscope Z-axis data.
// @param yaw: (in/out) The current yaw value, updated by the function.
// @param gz: Gyroscope Z-axis reading in degrees/sec.
// @param dt: Time delta in seconds.
void calculateYaw(float &yaw, float gz, float dt);
