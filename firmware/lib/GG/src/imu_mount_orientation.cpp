#include "imu_mount_orientation.hpp"
#include <string.h>

void applyMountOrientationRemap(float &x, float &y, float &z, uint8_t orientation) {
    float rx = x, ry = y, rz = z;
    switch (orientation) {
        case MOUNT_TILT_FORWARD:  // rotate -90 deg about X: y'=z, z'=-y
            y = rz;
            z = -ry;
            break;
        case MOUNT_TILT_BACKWARD: // rotate +90 deg about X: y'=-z, z'=y
            y = -rz;
            z = ry;
            break;
        case MOUNT_TILT_LEFT:     // rotate +90 deg about Y: x'=z, z'=-x
            x = rz;
            z = -rx;
            break;
        case MOUNT_TILT_RIGHT:    // rotate -90 deg about Y: x'=-z, z'=x
            x = -rz;
            z = rx;
            break;
        case MOUNT_STANDING:
        default:
            break; // no change
    }
}

const char* imuMountOrientationToString(uint8_t orientation) {
    switch (orientation) {
        case MOUNT_TILT_FORWARD:  return "TILT_FORWARD";
        case MOUNT_TILT_BACKWARD: return "TILT_BACKWARD";
        case MOUNT_TILT_LEFT:     return "TILT_LEFT";
        case MOUNT_TILT_RIGHT:    return "TILT_RIGHT";
        case MOUNT_STANDING:
        default:                  return "STANDING";
    }
}

int imuMountOrientationFromString(const char* orientation) {
    if (strcmp(orientation, "STANDING") == 0)       return MOUNT_STANDING;
    if (strcmp(orientation, "TILT_FORWARD") == 0)  return MOUNT_TILT_FORWARD;
    if (strcmp(orientation, "TILT_BACKWARD") == 0) return MOUNT_TILT_BACKWARD;
    if (strcmp(orientation, "TILT_LEFT") == 0)     return MOUNT_TILT_LEFT;
    if (strcmp(orientation, "TILT_RIGHT") == 0)    return MOUNT_TILT_RIGHT;
    return -1;
}
