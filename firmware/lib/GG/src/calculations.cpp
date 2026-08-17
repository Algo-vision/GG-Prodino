#include "calculations.hpp"
#include <math.h>

// Complementary filter weight: how much to trust the gyro's short-term
// integration vs. the accelerometer's long-term gravity-vector reference.
static const float ALPHA = 0.98f;

// Time constant for pulling yaw toward the GPS heading. 5s is slow enough to
// ride out GPS course noise and brief dropouts, fast enough to pull out
// accumulated gyro drift well within a minute.
const float YAW_GPS_TAU_S = 5.0f;

float wrapTo180(float degrees) {
    while (degrees > 180.0f)   degrees -= 360.0f;
    while (degrees <= -180.0f) degrees += 360.0f;
    return degrees;
}

/**
 * @brief Blend the freshly integrated gyro yaw toward the GPS heading.
 *
 * Expressed as a TIME CONSTANT rather than the fixed per-step weight used for
 * pitch/roll above, so the convergence rate depends on elapsed time only and
 * not on the call rate. statusUpdate() is now called on a fixed cadence, but
 * this form also rides out a stalled or jittery loop without changing how fast
 * yaw converges - which the pitch/roll form cannot do.
 *
 * With no usable heading this is just the wrap, leaving yaw free-running on
 * the gyro (and drifting) exactly as before.
 */
static float applyGpsYawCorrection(float yawGyro, float dt,
                                   bool gpsHeadingValid, float gpsHeading) {
    if (!gpsHeadingValid) {
        return wrapTo180(yawGyro);
    }

    // Shortest signed path to the target bearing, so a correction across the
    // +/-180 seam takes the short way round instead of spinning the long way.
    float error = wrapTo180(gpsHeading - yawGyro);
    float gain = dt / (YAW_GPS_TAU_S + dt);
    return wrapTo180(yawGyro + gain * error);
}

void calculateOrientation_1(float &pitch, float &roll, float &yaw,
                          float ax, float ay, float az,
                          float gx, float gy, float gz, float dt,
                          float ax_offset, float ay_offset,
                          bool gpsHeadingValid, float gpsHeading,
                          bool accelTrusted) {
    if (dt <= 0) {
        return;
    }

    ax -= ax_offset;
    ay -= ay_offset;

    if (accelTrusted) {
        float pitch_acc = atan2(-ay, az) * 180.0 / M_PI;
        float roll_acc  = atan2(-ax, sqrt(ay * ay + az * az)) * 180.0 / M_PI;

        pitch = ALPHA * (pitch - gx * dt) + (1.0f - ALPHA) * pitch_acc;
        roll  = ALPHA * (roll  + gy * dt) + (1.0f - ALPHA) * roll_acc;
    } else {
        // Coast: the accelerometer is measuring vehicle acceleration on top of
        // gravity right now, so integrate the gyro alone rather than follow a
        // false horizon.
        pitch -= gx * dt;
        roll  += gy * dt;
    }

    // Yaw (rotation about the vertical/gravity axis) has no accelerometer
    // reference, so it is gyro integration corrected against the GPS heading.
    yaw = applyGpsYawCorrection(yaw + gz * dt, dt, gpsHeadingValid, gpsHeading);
}

void calculateOrientation_2(float &pitch, float &roll, float &yaw,
                          float ax, float ay, float az,
                          float gx, float gy, float gz, float dt,
                          float ax_offset, float ay_offset,
                          bool gpsHeadingValid, float gpsHeading,
                          bool accelTrusted) {
    if (dt <= 0) {
        return;
    }

    ax -= ax_offset;
    ay -= ay_offset;

    // IMU2 is mounted 180 deg rotated from IMU1 in the X/Y plane, so its
    // accelerometer/gyro X and Y readings are the negative of IMU1's for the
    // same physical motion - the signs here are flipped relative to
    // calculateOrientation_1 to compensate.
    if (accelTrusted) {
        float pitch_acc = atan2(ay, az) * 180.0 / M_PI;
        float roll_acc  = atan2(ax, sqrt(ay * ay + az * az)) * 180.0 / M_PI;

        pitch = ALPHA * (pitch + gx * dt) + (1.0f - ALPHA) * pitch_acc;
        roll  = ALPHA * (roll  - gy * dt) + (1.0f - ALPHA) * roll_acc;
    } else {
        // Coast on the gyro - see calculateOrientation_1().
        pitch += gx * dt;
        roll  -= gy * dt;
    }

    // Z/yaw is unaffected by the in-plane 180 deg mount rotation, so gz keeps
    // the same sign as IMU1 - and so does the GPS correction.
    yaw = applyGpsYawCorrection(yaw + gz * dt, dt, gpsHeadingValid, gpsHeading);
}

void calculateMergedOrientation(float &pitch, float &roll, float &yaw,
                                float ax1, float ay1, float az1,
                                float gx1, float gy1, float gz1,
                                float ax2, float ay2, float az2,
                                float gx2, float gy2, float gz2,
                                float dt,
                                float ax1_offset, float ay1_offset,
                                float ax2_offset, float ay2_offset,
                                bool gpsHeadingValid, float gpsHeading,
                                bool accelTrusted) {
    if (dt <= 0) {
        return;
    }

    ax1 -= ax1_offset;
    ay1 -= ay1_offset;
    ax2 -= ax2_offset;
    ay2 -= ay2_offset;

    if (accelTrusted) {
        // Average the two IMUs' sign-corrected accelerometer angles (IMU2's
        // formulas mirror calculateOrientation_2's flipped signs) into a common
        // frame before filtering.
        float pitch_acc = (atan2(-ay1, az1) + atan2(ay2, az2)) * 0.5f * 180.0 / M_PI;
        float roll_acc  = (atan2(-ax1, sqrt(ay1 * ay1 + az1 * az1)) +
                           atan2(ax2, sqrt(ay2 * ay2 + az2 * az2))) * 0.5f * 180.0 / M_PI;

        pitch = ALPHA * (pitch + ((-gx1) + gx2) * 0.5f * dt) + (1.0f - ALPHA) * pitch_acc;
        roll  = ALPHA * (roll  + (gy1 + (-gy2)) * 0.5f * dt) + (1.0f - ALPHA) * roll_acc;
    } else {
        // Coast on the averaged gyro rates - see calculateOrientation_1().
        pitch += ((-gx1) + gx2) * 0.5f * dt;
        roll  += (gy1 + (-gy2)) * 0.5f * dt;
    }

    // gz is the same sign on both IMUs (unaffected by the in-plane rotation),
    // so the averaged rate feeds the same GPS-corrected yaw filter.
    yaw = applyGpsYawCorrection(yaw + (gz1 + gz2) * 0.5f * dt, dt,
                                gpsHeadingValid, gpsHeading);
}
