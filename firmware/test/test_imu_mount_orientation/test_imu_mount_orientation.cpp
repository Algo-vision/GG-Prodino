/**
 * Native unit tests for IMU mount orientation remap + string conversion.
 * Run with: pio test -e native (from firmware/)
 */
#include <unity.h>
#include <string.h>
#include "../../lib/GG/src/imu_mount_orientation.cpp"

static const float EPS = 0.0001f;

void test_standing_is_a_no_op(void) {
    float x = 1.0f, y = 2.0f, z = 3.0f;
    applyMountOrientationRemap(x, y, z, MOUNT_STANDING);
    TEST_ASSERT_FLOAT_WITHIN(EPS, 1.0f, x);
    TEST_ASSERT_FLOAT_WITHIN(EPS, 2.0f, y);
    TEST_ASSERT_FLOAT_WITHIN(EPS, 3.0f, z);
}

// Verify each tilt transform is a proper rotation (preserves vector
// magnitude - no scaling/distortion introduced).
static void assert_preserves_magnitude(uint8_t orientation) {
    float x = 1.0f, y = 2.0f, z = 3.0f;
    float originalMagSq = x * x + y * y + z * z;
    applyMountOrientationRemap(x, y, z, orientation);
    float newMagSq = x * x + y * y + z * z;
    TEST_ASSERT_FLOAT_WITHIN(EPS, originalMagSq, newMagSq);
}

void test_tilt_forward_preserves_magnitude(void) { assert_preserves_magnitude(MOUNT_TILT_FORWARD); }
void test_tilt_backward_preserves_magnitude(void) { assert_preserves_magnitude(MOUNT_TILT_BACKWARD); }
void test_tilt_left_preserves_magnitude(void) { assert_preserves_magnitude(MOUNT_TILT_LEFT); }
void test_tilt_right_preserves_magnitude(void) { assert_preserves_magnitude(MOUNT_TILT_RIGHT); }

// Forward and Backward should be inverse rotations of each other (applying
// one then the other returns to the original vector).
void test_forward_and_backward_are_inverses(void) {
    float x = 1.0f, y = 2.0f, z = 3.0f;
    applyMountOrientationRemap(x, y, z, MOUNT_TILT_FORWARD);
    applyMountOrientationRemap(x, y, z, MOUNT_TILT_BACKWARD);
    TEST_ASSERT_FLOAT_WITHIN(EPS, 1.0f, x);
    TEST_ASSERT_FLOAT_WITHIN(EPS, 2.0f, y);
    TEST_ASSERT_FLOAT_WITHIN(EPS, 3.0f, z);
}

// Left and Right should likewise be inverse rotations of each other.
void test_left_and_right_are_inverses(void) {
    float x = 1.0f, y = 2.0f, z = 3.0f;
    applyMountOrientationRemap(x, y, z, MOUNT_TILT_LEFT);
    applyMountOrientationRemap(x, y, z, MOUNT_TILT_RIGHT);
    TEST_ASSERT_FLOAT_WITHIN(EPS, 1.0f, x);
    TEST_ASSERT_FLOAT_WITHIN(EPS, 2.0f, y);
    TEST_ASSERT_FLOAT_WITHIN(EPS, 3.0f, z);
}

// Known exact values: gravity-down vector (0,0,1) under each tilt.
void test_tilt_forward_known_vector(void) {
    float x = 0.0f, y = 0.0f, z = 1.0f;
    applyMountOrientationRemap(x, y, z, MOUNT_TILT_FORWARD);
    // y'=z, z'=-y -> (0, 1, 0)
    TEST_ASSERT_FLOAT_WITHIN(EPS, 0.0f, x);
    TEST_ASSERT_FLOAT_WITHIN(EPS, 1.0f, y);
    TEST_ASSERT_FLOAT_WITHIN(EPS, 0.0f, z);
}

void test_tilt_left_known_vector(void) {
    float x = 0.0f, y = 0.0f, z = 1.0f;
    applyMountOrientationRemap(x, y, z, MOUNT_TILT_LEFT);
    // x'=z, z'=-x -> (1, 0, 0)
    TEST_ASSERT_FLOAT_WITHIN(EPS, 1.0f, x);
    TEST_ASSERT_FLOAT_WITHIN(EPS, 0.0f, y);
    TEST_ASSERT_FLOAT_WITHIN(EPS, 0.0f, z);
}

void test_string_round_trip_all_orientations(void) {
    uint8_t orientations[] = {MOUNT_STANDING, MOUNT_TILT_FORWARD, MOUNT_TILT_BACKWARD,
                              MOUNT_TILT_LEFT, MOUNT_TILT_RIGHT};
    for (int i = 0; i < 5; i++) {
        const char* str = imuMountOrientationToString(orientations[i]);
        int parsed = imuMountOrientationFromString(str);
        TEST_ASSERT_EQUAL_INT(orientations[i], parsed);
    }
}

void test_string_values_match_expected(void) {
    TEST_ASSERT_EQUAL_STRING("STANDING", imuMountOrientationToString(MOUNT_STANDING));
    TEST_ASSERT_EQUAL_STRING("TILT_FORWARD", imuMountOrientationToString(MOUNT_TILT_FORWARD));
    TEST_ASSERT_EQUAL_STRING("TILT_BACKWARD", imuMountOrientationToString(MOUNT_TILT_BACKWARD));
    TEST_ASSERT_EQUAL_STRING("TILT_LEFT", imuMountOrientationToString(MOUNT_TILT_LEFT));
    TEST_ASSERT_EQUAL_STRING("TILT_RIGHT", imuMountOrientationToString(MOUNT_TILT_RIGHT));
}

void test_invalid_string_returns_negative_one(void) {
    TEST_ASSERT_EQUAL_INT(-1, imuMountOrientationFromString("GARBAGE"));
    TEST_ASSERT_EQUAL_INT(-1, imuMountOrientationFromString(""));
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_standing_is_a_no_op);
    RUN_TEST(test_tilt_forward_preserves_magnitude);
    RUN_TEST(test_tilt_backward_preserves_magnitude);
    RUN_TEST(test_tilt_left_preserves_magnitude);
    RUN_TEST(test_tilt_right_preserves_magnitude);
    RUN_TEST(test_forward_and_backward_are_inverses);
    RUN_TEST(test_left_and_right_are_inverses);
    RUN_TEST(test_tilt_forward_known_vector);
    RUN_TEST(test_tilt_left_known_vector);
    RUN_TEST(test_string_round_trip_all_orientations);
    RUN_TEST(test_string_values_match_expected);
    RUN_TEST(test_invalid_string_returns_negative_one);
    return UNITY_END();
}
