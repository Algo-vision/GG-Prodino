#pragma once

#include <stdint.h>

// ============================================================================
// IMU MOUNT ORIENTATION
// ============================================================================
//
// The board's default mount orientation is standing upright. If it's
// mounted lying down instead, the physical axis that was "up" (gravity/yaw)
// is now one of the horizontal axes - applyMountOrientationRemap() remaps a
// raw (x,y,z) reading (works for both accelerometer and gyroscope vectors)
// from sensor-native axes into the standing-equivalent logical axes the
// rest of the firmware (calibration, orientation math) expects. Applied
// identically to both IMUs, independent of their fixed 180-degree
// relationship to each other.
//
// This module is deliberately free of any Arduino/hardware dependency so
// it can be unit-tested natively (see firmware/test/test_imu_mount_orientation.cpp).

enum ImuMountOrientation : uint8_t {
    MOUNT_STANDING = 0,       // Default, as pictured
    MOUNT_TILT_FORWARD = 1,
    MOUNT_TILT_BACKWARD = 2,
    MOUNT_TILT_LEFT = 3,
    MOUNT_TILT_RIGHT = 4,
};

// Note: the persisted mount-orientation setting is declared as
// g_imuMountOrientation in config_manager.hpp (alongside the other
// persisted config globals) - this library takes it as an explicit
// parameter rather than reading the global directly, since lib/GG
// cannot include headers from the main project's include/ directory.

/** Remap a raw (x,y,z) vector into logical standing-equivalent axes */
void applyMountOrientationRemap(float &x, float &y, float &z, uint8_t orientation);

/** Convert an ImuMountOrientation enum value to its wire string form */
const char* imuMountOrientationToString(uint8_t orientation);

/** Parse a wire string into an ImuMountOrientation value, or -1 if invalid */
int imuMountOrientationFromString(const char* orientation);
