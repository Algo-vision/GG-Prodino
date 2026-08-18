#ifndef ZERO_CALIBRATION_HPP
#define ZERO_CALIBRATION_HPP

/**
 * @file zero_calibration.hpp
 * @brief "Zero calibration" - what level means for one installed controller.
 *
 * The IMU is bolted to the machine at whatever angle the bracket happens to
 * give it, and each chip additionally has its own zero-g offset (tens of mg on
 * an LSM6DS3TR). Both show up the same way: with the machine standing on level
 * ground, the measured gravity vector is not straight down the sensor's Z axis.
 *
 * The zero calibration records that vector once, and every later reading is
 * rotated by the transform that puts it back where it belongs.
 *
 * WHY A ROTATION AND NOT AN OFFSET
 *
 * V1.5.1 subtracted a constant from ax/ay before the atan2. That cancels the
 * error only at the attitude where it was captured - calibrate at 10 deg and
 * the stored offset is sin(10 deg) = 0.174 g, which is simply the wrong
 * correction at 40 deg. It also changes the LENGTH of the acceleration vector,
 * which breaks the |a| ~ 1 g test that the at-rest detector depends on.
 *
 * Subtracting calibration ANGLES instead is closer, but rotations do not
 * commute, so it drifts once both pitch and roll offsets are non-trivial.
 *
 * Rotating is exact at every attitude, for any mounting angle, and costs nine
 * multiply-adds per vector - nothing next to the ~1.5 ms the I2C read takes.
 *
 * WHAT IT DELIBERATELY DOES NOT FIX
 *
 * Gravity says nothing about rotation ABOUT gravity, so the mounting's heading
 * component is unobservable here. This uses the minimal rotation - the one that
 * aligns the vectors and adds no spin about the vertical - leaving heading to
 * the GPS reference, which is how the system already works.
 *
 * The same rotation is applied to the gyro, since a tilted sensor's rotation
 * axes are tilted by exactly the same amount.
 */

/** Identity, for an uncalibrated controller: reports the sensor frame as-is
 *  rather than silently inventing a correction. */
void zeroCalIdentity(float R[9]);

/**
 * @brief Build the rotation that carries a measured gravity vector to vertical.
 *
 * @param g0 Gravity as measured with the machine on level ground. Need not be
 *           normalised; only its direction is used.
 * @param R  Out: row-major 3x3.
 * @return false if g0 has no usable direction (zero length), leaving R as
 *         identity - a failed calibration must not be able to install a
 *         garbage transform.
 */
bool zeroCalBuildRotation(const float g0[3], float R[9]);

/** @brief Apply R in place to one vector (accelerometer or gyro). */
void zeroCalApply(const float R[9], float &x, float &y, float &z);

#endif /* ZERO_CALIBRATION_HPP */
