#include "imu_mount_orientation.hpp"
#include <string.h>

ImuAxisMap imuAxisMapDefault() {
    ImuAxisMap map;
    map.pitchAxis   = IMU_AXIS_X;
    map.rollAxis    = IMU_AXIS_Y;
    map.yawAxis     = IMU_AXIS_Z;
    map.pitchInvert = 0;
    map.rollInvert  = 0;
    map.yawInvert   = 0;
    return map;
}

bool imuAxisMapIsValid(const ImuAxisMap &map) {
    if (map.pitchAxis > IMU_AXIS_Z || map.rollAxis > IMU_AXIS_Z || map.yawAxis > IMU_AXIS_Z) {
        return false;
    }
    // Invert flags are stored as bytes, so reject anything that is not a clean
    // 0/1 - that is how junk from a pre-invert firmware's flash is caught.
    if (map.pitchInvert > 1 || map.rollInvert > 1 || map.yawInvert > 1) {
        return false;
    }
    // The axes must be a permutation - two angles sharing an axis would drop
    // one sensor axis entirely and double-count another.
    return (map.pitchAxis != map.rollAxis) &&
           (map.pitchAxis != map.yawAxis) &&
           (map.rollAxis  != map.yawAxis);
}

void applyImuAxisMap(float &x, float &y, float &z, const ImuAxisMap &map) {
    if (!imuAxisMapIsValid(map)) {
        return;  // leave the reading untouched rather than corrupting it
    }

    const float raw[3] = {x, y, z};
    x = map.pitchInvert ? -raw[map.pitchAxis] : raw[map.pitchAxis];
    y = map.rollInvert  ? -raw[map.rollAxis]  : raw[map.rollAxis];
    z = map.yawInvert   ? -raw[map.yawAxis]   : raw[map.yawAxis];
}

const char* imuAxisToString(uint8_t axis) {
    switch (axis) {
        case IMU_AXIS_Y: return "Y";
        case IMU_AXIS_Z: return "Z";
        case IMU_AXIS_X:
        default:         return "X";
    }
}

int imuAxisFromString(const char* axis) {
    if (axis == 0) return -1;
    if (strcmp(axis, "X") == 0) return IMU_AXIS_X;
    if (strcmp(axis, "Y") == 0) return IMU_AXIS_Y;
    if (strcmp(axis, "Z") == 0) return IMU_AXIS_Z;
    return -1;
}
