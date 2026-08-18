#include "rest_detector.hpp"
#include <math.h>

void restDetectorReset(RestDetector &d) {
    d.stillSeconds = 0.0f;
}

float restDetectorUpdate(RestDetector &d, float dtSeconds,
                         float ax, float ay, float az,
                         float gx, float gy, float gz) {
    // A stalled or absurd dt must not be able to accumulate credit toward
    // "still for long enough" - see STATUS_UPDATE_MAX_DT_S for how far dt can
    // stretch when the loop is busy.
    if (dtSeconds <= 0.0f || dtSeconds > 1.0f) {
        d.stillSeconds = 0.0f;
        return 0.0f;
    }

    const float accelMag = sqrtf(ax*ax + ay*ay + az*az);
    const float gyroMag  = sqrtf(gx*gx + gy*gy + gz*gz);

    const bool still = (fabsf(accelMag - 1.0f) <= REST_ACCEL_TOLERANCE_G) &&
                       (gyroMag <= REST_GYRO_LIMIT_DPS);

    // Any single disqualifying sample resets the clock rather than decaying it.
    // Motion that only shows up intermittently is still motion, and the cost of
    // being wrong here is a bad calibration burned into flash.
    d.stillSeconds = still ? (d.stillSeconds + dtSeconds) : 0.0f;
    return d.stillSeconds;
}
