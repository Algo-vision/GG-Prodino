#include "zero_calibration.hpp"
#include <math.h>

void zeroCalIdentity(float R[9]) {
    R[0] = 1.0f; R[1] = 0.0f; R[2] = 0.0f;
    R[3] = 0.0f; R[4] = 1.0f; R[5] = 0.0f;
    R[6] = 0.0f; R[7] = 0.0f; R[8] = 1.0f;
}

bool zeroCalBuildRotation(const float g0[3], float R[9]) {
    zeroCalIdentity(R);

    float norm = sqrtf(g0[0]*g0[0] + g0[1]*g0[1] + g0[2]*g0[2]);
    // A vector this short is a dead sensor or a failed read, not an attitude.
    if (norm < 1e-4f) {
        return false;
    }

    // Source: measured gravity direction. Target: +Z.
    const float ax = g0[0] / norm;
    const float ay = g0[1] / norm;
    const float az = g0[2] / norm;

    // Rodrigues' rotation from a to b, with a x b as the axis and a . b as the
    // cosine. Since b is exactly +Z both reduce to single components.
    const float vx =  ay;   // (a x b).x = ay*bz - az*by = ay*1 - az*0
    const float vy = -ax;   // (a x b).y = az*bx - ax*bz = 0 - ax*1
    const float vz =  0.0f; // (a x b).z = ax*by - ay*bx = 0
    const float c  =  az;   // a . b

    // Antiparallel: the axis vanishes and the formula below divides by zero.
    // Any perpendicular axis is a valid 180 deg rotation; X is as good as any.
    // Physically this is the board mounted upside down.
    if (c < -1.0f + 1e-6f) {
        R[0] = 1.0f; R[1] =  0.0f; R[2] =  0.0f;
        R[3] = 0.0f; R[4] = -1.0f; R[5] =  0.0f;
        R[6] = 0.0f; R[7] =  0.0f; R[8] = -1.0f;
        return true;
    }

    // R = I + [v]x + [v]x^2 * 1/(1+c)
    const float k = 1.0f / (1.0f + c);

    R[0] = 1.0f + k * (-vz*vz - vy*vy);
    R[1] = -vz  + k * (vx*vy);
    R[2] =  vy  + k * (vx*vz);

    R[3] =  vz  + k * (vx*vy);
    R[4] = 1.0f + k * (-vz*vz - vx*vx);
    R[5] = -vx  + k * (vy*vz);

    R[6] = -vy  + k * (vx*vz);
    R[7] =  vx  + k * (vy*vz);
    R[8] = 1.0f + k * (-vy*vy - vx*vx);

    return true;
}

void zeroCalApply(const float R[9], float &x, float &y, float &z) {
    const float rx = R[0]*x + R[1]*y + R[2]*z;
    const float ry = R[3]*x + R[4]*y + R[5]*z;
    const float rz = R[6]*x + R[7]*y + R[8]*z;
    x = rx; y = ry; z = rz;
}
