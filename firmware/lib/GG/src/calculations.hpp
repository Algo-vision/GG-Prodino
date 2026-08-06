#pragma once

// ============================================================================
// ORIENTATION CALCULATION
// ============================================================================
//
// Pitch and roll are self-correcting: the accelerometer's gravity vector is an
// absolute long-term reference for them, so a complementary filter keeps them
// from drifting no matter how long the board runs.
//
// Yaw has no such reference - gravity says nothing about rotation about the
// vertical axis - so on its own it is pure gyro integration and drifts without
// bound. GPS course-over-ground (gpsHeading) is the absolute reference for
// yaw, and these functions blend it in the same way the accelerometer is
// blended into pitch/roll.
//
// ACCELEROMETER TRUST: an accelerometer measures gravity PLUS whatever linear
// acceleration the vehicle is undergoing, and cannot tell them apart. While
// accelerating, braking or cornering, its "gravity" vector is tilted and would
// drag pitch/roll toward a false horizon. When the caller reports
// accelTrusted = false, the accelerometer term is dropped entirely and
// pitch/roll COAST on the gyro alone - accurate short-term, drifting slowly,
// which is far better than following a wrong horizon.
//
// The caller decides whether the heading is usable and passes that in via
// gpsHeadingValid. GPS course is only meaningful while the vehicle is actually
// MOVING - standing still it is noise, and letting it correct yaw then would
// make a stationary board slowly rotate to a random bearing.
//
// CONVENTION: gpsHeading is an absolute compass bearing (0-360 deg, clockwise
// from north) and yaw is normalised to (-180, 180]. Blending assumes the two
// share a sense of rotation. If they turn opposite ways, yaw is pulled toward
// the mirrored bearing instead of converging - invert the yaw axis in the IMU
// axis map (see imu_mount_orientation.hpp), which flips the sign of the gyro
// rate feeding yaw.

/** Time constant (seconds) for pulling yaw toward the GPS heading. Larger =
 *  slower, smoother correction that leans more on the gyro in between fixes. */
extern const float YAW_GPS_TAU_S;

/** Normalise an angle in degrees into (-180, 180] */
float wrapTo180(float degrees);

// Calculates pitch, roll, and yaw from a single IMU using a complementary filter.
// Pitch/roll are corrected against the accelerometer's gravity vector (long-term
// reference); yaw is gyro integration corrected against the GPS heading when the
// caller reports one is usable, and free-running (drifting) when it is not.
// @param pitch, roll, yaw: (in/out) Current orientation values, updated by the function.
// @param ax, ay, az: Accelerometer readings in g's.
// @param gx, gy, gz: Gyroscope readings in degrees/sec.
// @param dt: Time delta in seconds.
// @param ax_offset, ay_offset: Accelerometer offsets for calibration.
// @param gpsHeadingValid: true if gpsHeading is a usable absolute reference.
// @param gpsHeading: GPS course over ground in degrees (0-360).
void calculateOrientation_1(float &pitch, float &roll, float &yaw,
                          float ax, float ay, float az,
                          float gx, float gy, float gz, float dt,
                          float ax_offset, float ay_offset,
                          bool gpsHeadingValid = false, float gpsHeading = 0.0f,
                          bool accelTrusted = true);

// Same as calculateOrientation_1, but for IMU2, which is mounted 180 degrees
// rotated from IMU1 in the X/Y plane (the accelerometer/gyro X and Y signs are
// flipped relative to IMU1; Z/yaw is unaffected since the rotation is about
// the vertical axis).
void calculateOrientation_2(float &pitch, float &roll, float &yaw,
                          float ax, float ay, float az,
                          float gx, float gy, float gz, float dt,
                          float ax_offset, float ay_offset,
                          bool gpsHeadingValid = false, float gpsHeading = 0.0f,
                          bool accelTrusted = true);

// Fuses both IMUs by averaging their sign-corrected accelerometer angles and
// gyro rates into a common frame before applying a single complementary
// filter. Used when both IMUs report valid data, for a less noisy estimate
// than either alone. Yaw is corrected against the GPS heading exactly as in
// the single-IMU case, using the averaged gyro rate.
void calculateMergedOrientation(float &pitch, float &roll, float &yaw,
                                float ax1, float ay1, float az1,
                                float gx1, float gy1, float gz1,
                                float ax2, float ay2, float az2,
                                float gx2, float gy2, float gz2,
                                float dt,
                                float ax1_offset, float ay1_offset,
                                float ax2_offset, float ay2_offset,
                                bool gpsHeadingValid = false, float gpsHeading = 0.0f,
                                bool accelTrusted = true);
