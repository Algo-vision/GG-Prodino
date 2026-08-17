#pragma once

#include <stdint.h>

// ============================================================================
// IMU AXIS MAPPING
// ============================================================================
//
// Which physical sensor axis each angle rotates about. The IMU can be bolted
// to the chassis in any orientation; rather than enumerating named mounting
// presets, the installer states directly which sensor axis carries pitch,
// which carries roll, and which carries yaw.
//
// applyImuAxisMap() reorders a raw (x,y,z) reading - it works for both
// accelerometer and gyroscope vectors - from sensor-native axes into the
// logical axes the rest of the firmware (calibration, orientation math)
// expects. The orientation math in calculations.cpp is written against a
// fixed logical frame:
//
//     logical X -> the pitch axis    (pitch_acc from Y/Z, pitch integrates gx)
//     logical Y -> the roll axis     (roll_acc from X,    roll  integrates gy)
//     logical Z -> the yaw axis      (yaw integrates gz)
//
// so remapping here is all that is needed - calculations.cpp is untouched by
// a mounting change. The map is applied identically to both IMUs, independent
// of their fixed 180-degree relationship to each other.
//
// The axis assignment is a PERMUTATION: the three angles must use three
// different axes. Each angle additionally carries an INVERT flag, which
// negates that component after reordering - needed when the sensor is mounted
// flipped end-for-end along an axis, where the angle would otherwise run
// backwards.
//
// This module is deliberately free of any Arduino/hardware dependency so it
// can be unit-tested natively (see test/test_imu_mount_orientation/).

enum ImuAxis : uint8_t {
    IMU_AXIS_X = 0,
    IMU_AXIS_Y = 1,
    IMU_AXIS_Z = 2,
};

/** Which sensor axis each angle rotates about, and whether to negate it.
 *  The invert flags are uint8_t rather than bool so that a byte read back
 *  from flash written by an older firmware can be range-checked (0 or 1)
 *  instead of silently becoming "true". */
struct ImuAxisMap {
    uint8_t pitchAxis;
    uint8_t rollAxis;
    uint8_t yawAxis;
    uint8_t pitchInvert;
    uint8_t rollInvert;
    uint8_t yawInvert;
};

// Note: the persisted setting is declared as g_imuAxisMap in
// config_manager.hpp (alongside the other persisted config globals) - the
// functions here take the map as an explicit parameter rather than reading
// the global directly, since lib/GG cannot include headers from the main
// project's include/ directory.

/** Firmware default: pitch about X, roll about Y, yaw about Z, none inverted
 *  (identity remap) */
ImuAxisMap imuAxisMapDefault();

/** True if every axis is in range, no two angles share an axis, and every
 *  invert flag is exactly 0 or 1 */
bool imuAxisMapIsValid(const ImuAxisMap &map);

/** Reorder (and optionally negate) a raw (x,y,z) sensor vector into the
 *  logical pitch/roll/yaw frame */
void applyImuAxisMap(float &x, float &y, float &z, const ImuAxisMap &map);

/** Convert an ImuAxis value to its wire string form ("X" / "Y" / "Z") */
const char* imuAxisToString(uint8_t axis);

/** Parse a wire string ("X" / "Y" / "Z") into an ImuAxis value, or -1 if invalid */
int imuAxisFromString(const char* axis);
