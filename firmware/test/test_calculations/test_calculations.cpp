/**
 * Native unit tests for the dual-IMU orientation fusion math.
 * Run with: pio test -e native (from firmware/)
 *
 * Includes the real source directly (rather than linking through
 * PlatformIO's library resolution) so this test always exercises the
 * actual production code, with zero Arduino/hardware dependency pulled in.
 */
#include <unity.h>
#include <math.h>
#include "../../lib/GG/src/calculations.cpp"

static const float TOLERANCE_DEG = 0.5f;

void test_level_flat_gives_zero_pitch_roll(void) {
    float pitch = 0, roll = 0, yaw = 0;
    // Flat and level: gravity purely along Z, no rotation rate
    calculateOrientation_1(pitch, roll, yaw, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.1f, 0.0f, 0.0f);
    TEST_ASSERT_FLOAT_WITHIN(TOLERANCE_DEG, 0.0f, pitch);
    TEST_ASSERT_FLOAT_WITHIN(TOLERANCE_DEG, 0.0f, roll);
    TEST_ASSERT_FLOAT_WITHIN(TOLERANCE_DEG, 0.0f, yaw);
}

// Repeatedly feed the same stationary-tilt accelerometer reading (zero
// gyro rate) and confirm the complementary filter converges to the
// accelerometer-derived angle (alpha=0.98 converges to <0.01% of the way
// remaining after ~500 iterations, per calculations.cpp's own filter math).
void test_pitch_converges_to_known_tilt(void) {
    float pitch = 0, roll = 0, yaw = 0;
    // 30 degree pitch tilt: ay = -sin(30deg), az = cos(30deg)
    float ay = -sinf(30.0f * M_PI / 180.0f);
    float az = cosf(30.0f * M_PI / 180.0f);
    for (int i = 0; i < 500; i++) {
        calculateOrientation_1(pitch, roll, yaw, 0.0f, ay, az, 0.0f, 0.0f, 0.0f, 0.01f, 0.0f, 0.0f);
    }
    TEST_ASSERT_FLOAT_WITHIN(TOLERANCE_DEG, 30.0f, pitch);
    TEST_ASSERT_FLOAT_WITHIN(TOLERANCE_DEG, 0.0f, roll);
}

void test_roll_converges_to_known_tilt(void) {
    float pitch = 0, roll = 0, yaw = 0;
    // 20 degree roll tilt: ax = -sin(20deg), az = cos(20deg)
    float ax = -sinf(20.0f * M_PI / 180.0f);
    float az = cosf(20.0f * M_PI / 180.0f);
    for (int i = 0; i < 500; i++) {
        calculateOrientation_1(pitch, roll, yaw, ax, 0.0f, az, 0.0f, 0.0f, 0.0f, 0.01f, 0.0f, 0.0f);
    }
    TEST_ASSERT_FLOAT_WITHIN(TOLERANCE_DEG, 0.0f, pitch);
    TEST_ASSERT_FLOAT_WITHIN(TOLERANCE_DEG, 20.0f, roll);
}

void test_calibration_offsets_are_applied(void) {
    float pitch = 0, roll = 0, yaw = 0;
    // Feed a biased ax reading with a matching offset - should converge to
    // the same as an unbiased reading with no offset (roll near 0).
    float az = 1.0f;
    float ax_bias = 0.2f;
    for (int i = 0; i < 500; i++) {
        calculateOrientation_1(pitch, roll, yaw, ax_bias, 0.0f, az, 0.0f, 0.0f, 0.0f, 0.01f, ax_bias, 0.0f);
    }
    TEST_ASSERT_FLOAT_WITHIN(TOLERANCE_DEG, 0.0f, roll);
}

// IMU2 is mounted 180 deg rotated from IMU1 in-plane: for the SAME physical
// tilt, IMU2's ax/ay/gx/gy readings are the negative of IMU1's (Z/gz
// unchanged). calculateOrientation_2 should converge to the SAME angle as
// calculateOrientation_1 for that shared physical motion - this is the
// actual thing that makes fusing the two sensors valid.
void test_imu2_sign_flip_matches_imu1_for_same_physical_tilt(void) {
    float pitch1 = 0, roll1 = 0, yaw1 = 0;
    float pitch2 = 0, roll2 = 0, yaw2 = 0;

    float ay1 = -sinf(15.0f * M_PI / 180.0f);
    float az1 = cosf(15.0f * M_PI / 180.0f);
    // IMU2 sees the same physical tilt with X/Y negated
    float ay2 = -ay1;
    float az2 = az1;

    for (int i = 0; i < 500; i++) {
        calculateOrientation_1(pitch1, roll1, yaw1, 0.0f, ay1, az1, 0.0f, 0.0f, 0.0f, 0.01f, 0.0f, 0.0f);
        calculateOrientation_2(pitch2, roll2, yaw2, 0.0f, ay2, az2, 0.0f, 0.0f, 0.0f, 0.01f, 0.0f, 0.0f);
    }
    TEST_ASSERT_FLOAT_WITHIN(TOLERANCE_DEG, pitch1, pitch2);
    TEST_ASSERT_FLOAT_WITHIN(TOLERANCE_DEG, roll1, roll2);
}

void test_merged_orientation_converges_to_same_tilt_as_single_imu(void) {
    float pitch = 0, roll = 0, yaw = 0;

    float ay1 = -sinf(25.0f * M_PI / 180.0f);
    float az1 = cosf(25.0f * M_PI / 180.0f);
    float ay2 = -ay1; // IMU2 sign-flipped for the same physical tilt
    float az2 = az1;

    for (int i = 0; i < 500; i++) {
        calculateMergedOrientation(pitch, roll, yaw,
                                    0.0f, ay1, az1, 0.0f, 0.0f, 0.0f,
                                    0.0f, ay2, az2, 0.0f, 0.0f, 0.0f,
                                    0.01f, 0.0f, 0.0f, 0.0f, 0.0f);
    }
    TEST_ASSERT_FLOAT_WITHIN(TOLERANCE_DEG, 25.0f, pitch);
    TEST_ASSERT_FLOAT_WITHIN(TOLERANCE_DEG, 0.0f, roll);
}

void test_yaw_wraps_above_180(void) {
    float pitch = 0, roll = 0, yaw = 179.0f;
    // gz=10 deg/s for 1s -> raw yaw would be 189, should wrap to -171
    calculateOrientation_1(pitch, roll, yaw, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 10.0f, 1.0f, 0.0f, 0.0f);
    TEST_ASSERT_FLOAT_WITHIN(TOLERANCE_DEG, -171.0f, yaw);
}

void test_yaw_wraps_below_negative_180(void) {
    float pitch = 0, roll = 0, yaw = -179.0f;
    // gz=-10 deg/s for 1s -> raw yaw would be -189, should wrap to 171
    calculateOrientation_1(pitch, roll, yaw, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, -10.0f, 1.0f, 0.0f, 0.0f);
    TEST_ASSERT_FLOAT_WITHIN(TOLERANCE_DEG, 171.0f, yaw);
}

void test_zero_dt_is_a_no_op(void) {
    float pitch = 12.3f, roll = -4.5f, yaw = 67.8f;
    calculateOrientation_1(pitch, roll, yaw, 1.0f, 1.0f, 1.0f, 5.0f, 5.0f, 5.0f, 0.0f, 0.0f, 0.0f);
    TEST_ASSERT_EQUAL_FLOAT(12.3f, pitch);
    TEST_ASSERT_EQUAL_FLOAT(-4.5f, roll);
    TEST_ASSERT_EQUAL_FLOAT(67.8f, yaw);
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_level_flat_gives_zero_pitch_roll);
    RUN_TEST(test_pitch_converges_to_known_tilt);
    RUN_TEST(test_roll_converges_to_known_tilt);
    RUN_TEST(test_calibration_offsets_are_applied);
    RUN_TEST(test_imu2_sign_flip_matches_imu1_for_same_physical_tilt);
    RUN_TEST(test_merged_orientation_converges_to_same_tilt_as_single_imu);
    RUN_TEST(test_yaw_wraps_above_180);
    RUN_TEST(test_yaw_wraps_below_negative_180);
    RUN_TEST(test_zero_dt_is_a_no_op);
    return UNITY_END();
}
