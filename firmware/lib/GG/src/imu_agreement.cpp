#include "imu_agreement.hpp"
#include <math.h>

bool imuAxesAgree(float ax1, float ay1, float az1,
                  float ax2, float ay2, float az2,
                  uint8_t *axesChecked) {
    // IMU2's 180 degree in-plane mount, undone. Z is unaffected by a rotation
    // about Z, so it keeps its sign - the same convention
    // calculateMergedOrientation() uses.
    const float a[3] = { ax1, ay1, az1 };
    const float b[3] = { -ax2, -ay2, az2 };

    uint8_t checked = 0;
    bool agree = true;

    for (uint8_t i = 0; i < 3; i++) {
        const float m1 = fabsf(a[i]);
        const float m2 = fabsf(b[i]);

        // Neither sensor sees enough on this axis for a ratio to mean anything.
        if (m1 < IMU_AGREE_DEADBAND_G && m2 < IMU_AGREE_DEADBAND_G) {
            continue;
        }
        checked++;

        // Opposite signs on an axis that is carrying real signal is a
        // disagreement no ratio can express - one of them has the vector
        // pointing the other way.
        if ((a[i] > 0.0f) != (b[i] > 0.0f)) {
            agree = false;
            continue;
        }

        // One reading large while the other is ~0 is the clearest possible
        // disagreement, and dividing by it would not be defined.
        const float lo = (m1 < m2) ? m1 : m2;
        const float hi = (m1 < m2) ? m2 : m1;
        if (lo < 1e-6f) {
            agree = false;
            continue;
        }
        if ((hi / lo) > IMU_AGREE_RATIO) {
            agree = false;
        }
    }

    if (axesChecked) {
        *axesChecked = checked;
    }
    return agree;
}
