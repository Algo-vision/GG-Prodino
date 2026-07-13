#include "calculations.hpp"
#include <math.h>

// Complementary filter weight: how much to trust the gyro's short-term
// integration vs. the accelerometer's long-term gravity-vector reference.
static const float ALPHA = 0.98f;

void calculateOrientation_1(float &pitch, float &roll, float &yaw,
                          float ax, float ay, float az,
                          float gx, float gy, float gz, float dt,
                          float ax_offset, float ay_offset) {
    if (dt <= 0) {
        return;
    }

    ax -= ax_offset;
    ay -= ay_offset;

    float pitch_acc = atan2(-ay, az) * 180.0 / M_PI;
    float roll_acc  = atan2(-ax, sqrt(ay * ay + az * az)) * 180.0 / M_PI;

    pitch = ALPHA * (pitch - gx * dt) + (1.0f - ALPHA) * pitch_acc;
    roll  = ALPHA * (roll  + gy * dt) + (1.0f - ALPHA) * roll_acc;

    // Yaw (rotation about the vertical/gravity axis) has no accelerometer
    // reference, so it's pure gyro integration and will drift over time.
    yaw += gz * dt;
    if (yaw > 180.0) {
        yaw -= 360.0;
    }
    if (yaw < -180.0) {
        yaw += 360.0;
    }
}

void calculateOrientation_2(float &pitch, float &roll, float &yaw,
                          float ax, float ay, float az,
                          float gx, float gy, float gz, float dt,
                          float ax_offset, float ay_offset) {
    if (dt <= 0) {
        return;
    }

    ax -= ax_offset;
    ay -= ay_offset;

    // IMU2 is mounted 180 deg rotated from IMU1 in the X/Y plane, so its
    // accelerometer/gyro X and Y readings are the negative of IMU1's for the
    // same physical motion - the signs here are flipped relative to
    // calculateOrientation_1 to compensate.
    float pitch_acc = atan2(ay, az) * 180.0 / M_PI;
    float roll_acc  = atan2(ax, sqrt(ay * ay + az * az)) * 180.0 / M_PI;

    pitch = ALPHA * (pitch + gx * dt) + (1.0f - ALPHA) * pitch_acc;
    roll  = ALPHA * (roll  - gy * dt) + (1.0f - ALPHA) * roll_acc;

    // Z/yaw is unaffected by the in-plane 180 deg mount rotation, so gz keeps
    // the same sign as IMU1.
    yaw += gz * dt;
    if (yaw > 180.0) {
        yaw -= 360.0;
    }
    if (yaw < -180.0) {
        yaw += 360.0;
    }
}

void calculateMergedOrientation(float &pitch, float &roll, float &yaw,
                                float ax1, float ay1, float az1,
                                float gx1, float gy1, float gz1,
                                float ax2, float ay2, float az2,
                                float gx2, float gy2, float gz2,
                                float dt,
                                float ax1_offset, float ay1_offset,
                                float ax2_offset, float ay2_offset) {
    if (dt <= 0) {
        return;
    }

    ax1 -= ax1_offset;
    ay1 -= ay1_offset;
    ax2 -= ax2_offset;
    ay2 -= ay2_offset;

    // Average the two IMUs' sign-corrected accelerometer angles (IMU2's
    // formulas mirror calculateOrientation_2's flipped signs) into a common
    // frame before filtering.
    float pitch_acc = (atan2(-ay1, az1) + atan2(ay2, az2)) * 0.5f * 180.0 / M_PI;
    float roll_acc  = (atan2(-ax1, sqrt(ay1 * ay1 + az1 * az1)) +
                       atan2(ax2, sqrt(ay2 * ay2 + az2 * az2))) * 0.5f * 180.0 / M_PI;

    pitch = ALPHA * (pitch + ((-gx1) + gx2) * 0.5f * dt) + (1.0f - ALPHA) * pitch_acc;
    roll  = ALPHA * (roll  + (gy1 + (-gy2)) * 0.5f * dt) + (1.0f - ALPHA) * roll_acc;

    // gz is the same sign on both IMUs (unaffected by the in-plane rotation).
    yaw += (gz1 + gz2) * 0.5f * dt;
    if (yaw > 180.0) {
        yaw -= 360.0;
    }
    if (yaw < -180.0) {
        yaw += 360.0;
    }
}
