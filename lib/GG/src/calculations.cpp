#include "calculations.hpp"
#include <math.h>

void calculatePitchRoll(float &pitch, float &roll, float ax, float ay, float az, float gx, float gy, float dt, float ax_offset, float ay_offset) {
    if (dt <= 0) {
        return;
    }

    // Apply calibration offsets
    ax -= ax_offset;
    ay -= ay_offset;

    // Calculate pitch and roll from accelerometer data (in degrees)
    // This gives the absolute orientation based on the gravity vector,
    // but it's sensitive to external accelerations.
    float pitch_acc = atan2(-ax, sqrt(ay * ay + az * az)) * 180.0 / M_PI;
    float roll_acc = atan2(ay, az) * 180.0 / M_PI;

    // Complementary filter combines the accelerometer (long-term) and gyroscope (short-term) data.
    // Gyroscope data is integrated to get the change in angle.
    // A weighted average is then taken.
    const float alpha = 0.98; // Weight for gyroscope data (trust in short-term changes)
    pitch = alpha * (pitch + gx * dt) + (1.0 - alpha) * pitch_acc;
    roll = alpha * (roll + gy * dt) + (1.0 - alpha) * roll_acc;
}

void calculateYaw(float &yaw, float gz, float dt) {
    if (dt <= 0) {
        return;
    }
    // Integrate gyroscope Z-axis data to get yaw.
    // This is a basic integration and will drift over time without a magnetometer for correction.
    yaw += gz * dt;

    // Keep yaw within -180 to 180 degrees
    if (yaw > 180.0) {
        yaw -= 360.0;
    }
    if (yaw < -180.0) {
        yaw += 360.0;
    }
}
