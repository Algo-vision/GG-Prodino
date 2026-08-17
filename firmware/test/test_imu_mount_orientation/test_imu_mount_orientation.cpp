/**
 * Native unit tests for the IMU axis map (remap + validation + string conversion).
 * Run with: pio test -e native (from firmware/)
 */
#include <unity.h>
#include <string.h>
#include "../../lib/GG/src/imu_mount_orientation.cpp"

static const float EPS = 0.0001f;

static ImuAxisMap makeMap(uint8_t pitch, uint8_t roll, uint8_t yaw,
                          uint8_t pitchInv = 0, uint8_t rollInv = 0, uint8_t yawInv = 0) {
    ImuAxisMap map = {pitch, roll, yaw, pitchInv, rollInv, yawInv};
    return map;
}

// The default (pitch=X, roll=Y, yaw=Z) must leave readings untouched, so an
// unconfigured board behaves exactly as it did before the axis map existed.
void test_default_map_is_a_no_op(void) {
    float x = 1.0f, y = 2.0f, z = 3.0f;
    applyImuAxisMap(x, y, z, imuAxisMapDefault());
    TEST_ASSERT_FLOAT_WITHIN(EPS, 1.0f, x);
    TEST_ASSERT_FLOAT_WITHIN(EPS, 2.0f, y);
    TEST_ASSERT_FLOAT_WITHIN(EPS, 3.0f, z);
}

void test_default_map_axes(void) {
    ImuAxisMap map = imuAxisMapDefault();
    TEST_ASSERT_EQUAL_UINT8(IMU_AXIS_X, map.pitchAxis);
    TEST_ASSERT_EQUAL_UINT8(IMU_AXIS_Y, map.rollAxis);
    TEST_ASSERT_EQUAL_UINT8(IMU_AXIS_Z, map.yawAxis);
    TEST_ASSERT_EQUAL_UINT8(0, map.pitchInvert);
    TEST_ASSERT_EQUAL_UINT8(0, map.rollInvert);
    TEST_ASSERT_EQUAL_UINT8(0, map.yawInvert);
}

// ---------------------------------------------------------------------------
// Invert flags
// ---------------------------------------------------------------------------

void test_invert_negates_only_the_flagged_angle(void) {
    float x = 1.0f, y = 2.0f, z = 3.0f;
    applyImuAxisMap(x, y, z, makeMap(IMU_AXIS_X, IMU_AXIS_Y, IMU_AXIS_Z, 1, 0, 0));
    TEST_ASSERT_FLOAT_WITHIN(EPS, -1.0f, x);
    TEST_ASSERT_FLOAT_WITHIN(EPS, 2.0f, y);
    TEST_ASSERT_FLOAT_WITHIN(EPS, 3.0f, z);
}

void test_all_three_inverted(void) {
    float x = 1.0f, y = 2.0f, z = 3.0f;
    applyImuAxisMap(x, y, z, makeMap(IMU_AXIS_X, IMU_AXIS_Y, IMU_AXIS_Z, 1, 1, 1));
    TEST_ASSERT_FLOAT_WITHIN(EPS, -1.0f, x);
    TEST_ASSERT_FLOAT_WITHIN(EPS, -2.0f, y);
    TEST_ASSERT_FLOAT_WITHIN(EPS, -3.0f, z);
}

// Invert is applied AFTER the axis is selected, so it negates the remapped
// component - not the same-named raw one.
void test_invert_applies_after_reordering(void) {
    float x = 1.0f, y = 2.0f, z = 3.0f;
    applyImuAxisMap(x, y, z, makeMap(IMU_AXIS_Z, IMU_AXIS_X, IMU_AXIS_Y, 1, 0, 1));
    TEST_ASSERT_FLOAT_WITHIN(EPS, -3.0f, x);  // pitch reads sensor Z, negated
    TEST_ASSERT_FLOAT_WITHIN(EPS, 1.0f, y);   // roll  reads sensor X
    TEST_ASSERT_FLOAT_WITHIN(EPS, -2.0f, z);  // yaw   reads sensor Y, negated
}

// Negation flips sign but not length, so magnitude still survives.
void test_inverts_preserve_magnitude(void) {
    float x = 1.0f, y = 2.0f, z = 3.0f;
    float originalMagSq = x * x + y * y + z * z;
    applyImuAxisMap(x, y, z, makeMap(IMU_AXIS_Y, IMU_AXIS_Z, IMU_AXIS_X, 1, 0, 1));
    TEST_ASSERT_FLOAT_WITHIN(EPS, originalMagSq, x * x + y * y + z * z);
}

// Bytes read back from flash written by a pre-invert firmware are junk; only
// a clean 0/1 is acceptable.
void test_out_of_range_invert_flags_are_rejected(void) {
    TEST_ASSERT_FALSE(imuAxisMapIsValid(makeMap(IMU_AXIS_X, IMU_AXIS_Y, IMU_AXIS_Z, 2, 0, 0)));
    TEST_ASSERT_FALSE(imuAxisMapIsValid(makeMap(IMU_AXIS_X, IMU_AXIS_Y, IMU_AXIS_Z, 0, 255, 0)));
    TEST_ASSERT_FALSE(imuAxisMapIsValid(makeMap(IMU_AXIS_X, IMU_AXIS_Y, IMU_AXIS_Z, 0, 0, 7)));
}

void test_valid_invert_flags_are_accepted(void) {
    TEST_ASSERT_TRUE(imuAxisMapIsValid(makeMap(IMU_AXIS_X, IMU_AXIS_Y, IMU_AXIS_Z, 1, 1, 1)));
    TEST_ASSERT_TRUE(imuAxisMapIsValid(makeMap(IMU_AXIS_Z, IMU_AXIS_X, IMU_AXIS_Y, 0, 1, 0)));
}

// Swapping pitch and roll swaps the first two output components.
void test_swapped_pitch_and_roll(void) {
    float x = 1.0f, y = 2.0f, z = 3.0f;
    applyImuAxisMap(x, y, z, makeMap(IMU_AXIS_Y, IMU_AXIS_X, IMU_AXIS_Z));
    TEST_ASSERT_FLOAT_WITHIN(EPS, 2.0f, x);
    TEST_ASSERT_FLOAT_WITHIN(EPS, 1.0f, y);
    TEST_ASSERT_FLOAT_WITHIN(EPS, 3.0f, z);
}

// Sensor mounted on its side: yaw now rides the sensor's X axis.
void test_yaw_on_x_axis(void) {
    float x = 1.0f, y = 2.0f, z = 3.0f;
    applyImuAxisMap(x, y, z, makeMap(IMU_AXIS_Z, IMU_AXIS_Y, IMU_AXIS_X));
    TEST_ASSERT_FLOAT_WITHIN(EPS, 3.0f, x);  // pitch reads sensor Z
    TEST_ASSERT_FLOAT_WITHIN(EPS, 2.0f, y);  // roll  reads sensor Y
    TEST_ASSERT_FLOAT_WITHIN(EPS, 1.0f, z);  // yaw   reads sensor X
}

// Every permutation must preserve vector magnitude - it only reorders
// components, so no scaling or distortion can be introduced.
void test_all_permutations_preserve_magnitude(void) {
    const uint8_t perms[6][3] = {
        {0, 1, 2}, {0, 2, 1}, {1, 0, 2}, {1, 2, 0}, {2, 0, 1}, {2, 1, 0}
    };
    for (int i = 0; i < 6; i++) {
        float x = 1.0f, y = 2.0f, z = 3.0f;
        float originalMagSq = x * x + y * y + z * z;
        applyImuAxisMap(x, y, z, makeMap(perms[i][0], perms[i][1], perms[i][2]));
        float newMagSq = x * x + y * y + z * z;
        TEST_ASSERT_FLOAT_WITHIN(EPS, originalMagSq, newMagSq);
    }
}

void test_valid_maps_are_accepted(void) {
    TEST_ASSERT_TRUE(imuAxisMapIsValid(imuAxisMapDefault()));
    TEST_ASSERT_TRUE(imuAxisMapIsValid(makeMap(IMU_AXIS_Z, IMU_AXIS_X, IMU_AXIS_Y)));
    TEST_ASSERT_TRUE(imuAxisMapIsValid(makeMap(IMU_AXIS_Y, IMU_AXIS_Z, IMU_AXIS_X)));
}

// Two angles sharing an axis would drop one sensor axis and double-count
// another - it must be rejected, not silently applied.
void test_duplicate_axes_are_rejected(void) {
    TEST_ASSERT_FALSE(imuAxisMapIsValid(makeMap(IMU_AXIS_X, IMU_AXIS_X, IMU_AXIS_Z)));
    TEST_ASSERT_FALSE(imuAxisMapIsValid(makeMap(IMU_AXIS_X, IMU_AXIS_Y, IMU_AXIS_Y)));
    TEST_ASSERT_FALSE(imuAxisMapIsValid(makeMap(IMU_AXIS_Z, IMU_AXIS_Z, IMU_AXIS_Z)));
}

void test_out_of_range_axes_are_rejected(void) {
    TEST_ASSERT_FALSE(imuAxisMapIsValid(makeMap(3, IMU_AXIS_Y, IMU_AXIS_Z)));
    TEST_ASSERT_FALSE(imuAxisMapIsValid(makeMap(IMU_AXIS_X, 200, IMU_AXIS_Z)));
}

// An invalid map must leave the reading untouched rather than corrupting it
// (e.g. flash bytes from a firmware that predates the axis map).
void test_invalid_map_leaves_reading_untouched(void) {
    float x = 1.0f, y = 2.0f, z = 3.0f;
    applyImuAxisMap(x, y, z, makeMap(IMU_AXIS_X, IMU_AXIS_X, IMU_AXIS_X));
    TEST_ASSERT_FLOAT_WITHIN(EPS, 1.0f, x);
    TEST_ASSERT_FLOAT_WITHIN(EPS, 2.0f, y);
    TEST_ASSERT_FLOAT_WITHIN(EPS, 3.0f, z);
}

void test_string_round_trip_all_axes(void) {
    uint8_t axes[] = {IMU_AXIS_X, IMU_AXIS_Y, IMU_AXIS_Z};
    for (int i = 0; i < 3; i++) {
        const char* str = imuAxisToString(axes[i]);
        int parsed = imuAxisFromString(str);
        TEST_ASSERT_EQUAL_INT(axes[i], parsed);
    }
}

void test_string_values_match_expected(void) {
    TEST_ASSERT_EQUAL_STRING("X", imuAxisToString(IMU_AXIS_X));
    TEST_ASSERT_EQUAL_STRING("Y", imuAxisToString(IMU_AXIS_Y));
    TEST_ASSERT_EQUAL_STRING("Z", imuAxisToString(IMU_AXIS_Z));
}

void test_invalid_string_returns_negative_one(void) {
    TEST_ASSERT_EQUAL_INT(-1, imuAxisFromString("GARBAGE"));
    TEST_ASSERT_EQUAL_INT(-1, imuAxisFromString(""));
    TEST_ASSERT_EQUAL_INT(-1, imuAxisFromString("x"));  // case-sensitive
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_default_map_is_a_no_op);
    RUN_TEST(test_default_map_axes);
    RUN_TEST(test_swapped_pitch_and_roll);
    RUN_TEST(test_yaw_on_x_axis);
    RUN_TEST(test_all_permutations_preserve_magnitude);
    RUN_TEST(test_valid_maps_are_accepted);
    RUN_TEST(test_duplicate_axes_are_rejected);
    RUN_TEST(test_out_of_range_axes_are_rejected);
    RUN_TEST(test_invalid_map_leaves_reading_untouched);
    RUN_TEST(test_string_round_trip_all_axes);
    RUN_TEST(test_string_values_match_expected);
    RUN_TEST(test_invalid_string_returns_negative_one);
    return UNITY_END();
}
