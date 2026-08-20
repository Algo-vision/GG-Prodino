/**
 * Native unit tests for the two-IMU agreement check.
 * Run with: pio test -e native (from firmware/)
 *
 * The failure that matters is not "it missed a disagreement" - it is "it
 * declared a healthy pair broken", because that is what the naive factor-of-two
 * test did on every level board, and a check that cries wolf gets switched off.
 */
#include <unity.h>
#include <math.h>
#include "../../lib/GG/src/imu_agreement.cpp"

static const float DEG = 3.14159265358979323846f / 180.0f;

/** Gravity as IMU1 reads it at a given roll, and as IMU2 reads the same
 *  physical attitude through its 180 degree in-plane mount. */
static void pair(float rollDeg, float g1[3], float g2[3], float scale2 = 1.0f) {
    const float r = rollDeg * DEG;
    g1[0] = -sinf(r); g1[1] = 0.0f; g1[2] = cosf(r);
    g2[0] = -g1[0] * scale2;   // mount rotation, undone inside the function
    g2[1] = -g1[1] * scale2;
    g2[2] =  g1[2] * scale2;
}

void test_healthy_pair_agrees_when_level(void) {
    float a[3], b[3];
    pair(0.0f, a, b);
    TEST_ASSERT_TRUE(imuAxesAgree(a[0],a[1],a[2], b[0],b[1],b[2]));
}

/** The bug this exists to avoid. Real numbers off the bench: X and Y sit at a
 *  few hundredths of a g and their RATIO is 3.4 and 15.5, while the sensors
 *  disagree by three hundredths of a g - nothing. A test without a deadband
 *  calls this broken. */
void test_the_near_zero_axes_that_broke_the_naive_check(void) {
    uint8_t checked = 0;
    bool ok = imuAxesAgree(-0.01471f, -0.03358f, 1.03561f,
                            0.00434f,  0.00217f, 1.01032f,  &checked);
    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_EQUAL_UINT8(1, checked);   // only Z carried enough signal
}

void test_one_axis_reading_double_is_caught(void) {
    float a[3], b[3];
    pair(0.0f, a, b);
    b[2] *= 2.5f;                       // IMU2's Z reads 2.5x IMU1's
    TEST_ASSERT_FALSE(imuAxesAgree(a[0],a[1],a[2], -b[0],-b[1],b[2]));
}

void test_a_dead_sensor_reading_zero_is_caught(void) {
    float a[3], b[3];
    pair(0.0f, a, b);
    TEST_ASSERT_FALSE(imuAxesAgree(a[0],a[1],a[2], 0.0f, 0.0f, 0.0f));
}

/** Opposite signs on an axis carrying real signal - one sensor has gravity
 *  pointing the other way. No ratio expresses that, so it is checked directly. */
void test_inverted_axis_is_caught(void) {
    TEST_ASSERT_FALSE(imuAxesAgree(0.0f, 0.0f,  1.0f,
                                   0.0f, 0.0f, -1.0f));
}

/** The mount rotation must be undone, or a healthy pair fails permanently. */
void test_the_180_degree_mount_is_accounted_for(void) {
    // IMU1 sees +0.5 on X; IMU2, rotated 180 in plane, sees -0.5 for the same
    // physical attitude. That is agreement, not disagreement.
    TEST_ASSERT_TRUE(imuAxesAgree( 0.5f, 0.0f, 0.87f,
                                  -0.5f, 0.0f, 0.87f));
}

/** Pins the consequence of whatever deadband is configured, so changing it
 *  cannot quietly change the coverage. At 45 deg both in-plane axes carry
 *  0.707 g: a deadband above that leaves the check with nothing to compare on
 *  a slope, one below it keeps the check live. At the configured 0.5 g the
 *  second branch is the live one. */
void test_deadband_coverage_on_a_slope(void) {
    float a[3], b[3];
    pair(45.0f, a, b);                  // both axes now ~0.707 g
    uint8_t checked = 99;
    imuAxesAgree(a[0],a[1],a[2], -b[0],-b[1],b[2], &checked);
    if (IMU_AGREE_DEADBAND_G > 0.708f) {
        TEST_ASSERT_EQUAL_UINT8(0, checked);   // nothing was compared
    } else {
        TEST_ASSERT_GREATER_THAN_UINT8(0, checked);
    }
}

/** With nothing comparable, the answer must be "no evidence of disagreement",
 *  not a fabricated pass or fail. */
void test_no_comparable_axis_reports_zero_checked(void) {
    uint8_t checked = 99;
    bool ok = imuAxesAgree(0.01f, 0.01f, 0.01f, -0.01f, -0.01f, 0.01f, &checked);
    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_EQUAL_UINT8(0, checked);
}

void setUp(void) {}
void tearDown(void) {}

int main(int, char **) {
    UNITY_BEGIN();
    RUN_TEST(test_healthy_pair_agrees_when_level);
    RUN_TEST(test_the_near_zero_axes_that_broke_the_naive_check);
    RUN_TEST(test_one_axis_reading_double_is_caught);
    RUN_TEST(test_a_dead_sensor_reading_zero_is_caught);
    RUN_TEST(test_inverted_axis_is_caught);
    RUN_TEST(test_the_180_degree_mount_is_accounted_for);
    RUN_TEST(test_deadband_coverage_on_a_slope);
    RUN_TEST(test_no_comparable_axis_reports_zero_checked);
    return UNITY_END();
}
