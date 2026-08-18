#include "gyro_bias.hpp"
#include <math.h>

void gyroBiasReset(GyroBias &b) {
    b.x = b.y = b.z = 0.0f;
    b.valid = false;
}

bool gyroBiasUpdate(GyroBias &b, float rawGx, float rawGy, float rawGz,
                    float dtSeconds) {
    if (dtSeconds <= 0.0f || dtSeconds > 1.0f) {
        return false;
    }

    if (fabsf(rawGx) > GYRO_BIAS_SANE_LIMIT_DPS ||
        fabsf(rawGy) > GYRO_BIAS_SANE_LIMIT_DPS ||
        fabsf(rawGz) > GYRO_BIAS_SANE_LIMIT_DPS) {
        return false;
    }

    // With nothing to build on, take the first at-rest reading whole rather
    // than crawling toward it from zero over the next minute.
    if (!b.valid) {
        b.x = rawGx; b.y = rawGy; b.z = rawGz;
        b.valid = true;
        return true;
    }

    const float gain = dtSeconds / (GYRO_BIAS_TAU_S + dtSeconds);
    b.x += gain * (rawGx - b.x);
    b.y += gain * (rawGy - b.y);
    b.z += gain * (rawGz - b.z);
    return true;
}
