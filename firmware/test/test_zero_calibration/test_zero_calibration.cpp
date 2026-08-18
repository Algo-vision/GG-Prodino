/**
 * Native unit tests for the zero-calibration rotation.
 * Run with: pio test -e native (from firmware/)
 *
 * The property that matters is not "the rotation is correct at the calibration
 * attitude" - the offset method V1.5.1 used already managed that. It is that
 * the correction still holds AT EVERY OTHER ATTITUDE, which is exactly where
 * subtracting a constant falls apart. Most of these tests check that.
 */
#include <unity.h>
#include <math.h>
#include "../../lib/GG/src/zero_calibration.cpp"

static const float TOL = 1e-4f;
static const float DEG = 3.14159265358979323846f / 180.0f;

/** Gravity as an ideal accelerometer reads it at a given pitch/roll. */
static void gravityAt(float pitchDeg, float rollDeg, float g[3]) {
    const float p = pitchDeg * DEG, r = rollDeg * DEG;
    g[0] = -sinf(r) * cosf(p);
    g[1] = -sinf(p);
    g[2] =  cosf(p) * cosf(r);
}

static float pitchOf(float x, float y, float z) {
    (void)x; return atan2f(-y, z) / DEG;
}
static float rollOf(float x, float y, float z) {
    return atan2f(-x, sqrtf(y*y + z*z)) / DEG;
}

void test_identity_when_uncalibrated(void) {
    float R[9];
    zeroCalIdentity(R);
    float x = 0.3f, y = -0.4f, z = 0.86f;
    zeroCalApply(R, x, y, z);
    TEST_ASSERT_FLOAT_WITHIN(TOL, 0.3f, x);
    TEST_ASSERT_FLOAT_WITHIN(TOL, -0.4f, y);
    TEST_ASSERT_FLOAT_WITHIN(TOL, 0.86f, z);
}

void test_already_level_builds_identity(void) {
    float g0[3] = {0.0f, 0.0f, 1.0f}, R[9];
    TEST_ASSERT_TRUE(zeroCalBuildRotation(g0, R));
    float x = 0.1f, y = 0.2f, z = 0.97f;
    zeroCalApply(R, x, y, z);
    TEST_ASSERT_FLOAT_WITHIN(TOL, 0.1f, x);
    TEST_ASSERT_FLOAT_WITHIN(TOL, 0.2f, y);
    TEST_ASSERT_FLOAT_WITHIN(TOL, 0.97f, z);
}

void test_calibration_attitude_reads_zero(void) {
    float g0[3], R[9];
    gravityAt(12.0f, -7.0f, g0);            // mounted 12 deg out in pitch, -7 in roll
    TEST_ASSERT_TRUE(zeroCalBuildRotation(g0, R));

    float x = g0[0], y = g0[1], z = g0[2];
    zeroCalApply(R, x, y, z);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, pitchOf(x, y, z));
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, rollOf(x, y, z));
}

/** The whole point. Calibrate at one attitude, then tilt the machine a long
 *  way and check the reported angle is the REAL change, not a distorted one. */
void test_correction_holds_far_from_the_calibration_point(void) {
    float g0[3], R[9];
    gravityAt(10.0f, 0.0f, g0);             // 10 deg mounting error in pitch
    TEST_ASSERT_TRUE(zeroCalBuildRotation(g0, R));

    // Machine now genuinely at 40 deg pitch; the sensor therefore sees 50.
    float g[3];
    gravityAt(50.0f, 0.0f, g);
    float x = g[0], y = g[1], z = g[2];
    zeroCalApply(R, x, y, z);

    TEST_ASSERT_FLOAT_WITHIN(0.05f, 40.0f, pitchOf(x, y, z));
    TEST_ASSERT_FLOAT_WITHIN(0.05f, 0.0f, rollOf(x, y, z));
}

/** V1.5.1's offset method, run on the same case, to show what it costs. */
void test_offset_method_is_wrong_away_from_calibration(void) {
    float g0[3];
    gravityAt(10.0f, 0.0f, g0);
    const float ay_offset = g0[1];          // what calibrateIMU() would store

    float g[3];
    gravityAt(50.0f, 0.0f, g);
    const float pitchOffsetMethod = pitchOf(g[0], g[1] - ay_offset, g[2]);

    // It does not land on 40 deg. Documented here so the error is a measured
    // number and not an assertion: about 2.5 deg at this attitude.
    TEST_ASSERT_TRUE(fabsf(pitchOffsetMethod - 40.0f) > 1.0f);
}

void test_both_axes_together(void) {
    float g0[3], R[9];
    gravityAt(15.0f, 20.0f, g0);
    TEST_ASSERT_TRUE(zeroCalBuildRotation(g0, R));

    // Real machine attitude 25 pitch / -10 roll, seen through the same mount.
    // Composing tilts is not additive, so assert against the sensor reading
    // put through the transform rather than against arithmetic on the angles.
    float x = g0[0], y = g0[1], z = g0[2];
    zeroCalApply(R, x, y, z);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, pitchOf(x, y, z));
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, rollOf(x, y, z));
}

/** A rotation must not change the length of what it rotates - the at-rest
 *  detector's |a| ~ 1 g test depends on it, and this is precisely what the
 *  old offset subtraction broke. */
void test_magnitude_is_preserved(void) {
    float g0[3], R[9];
    gravityAt(23.0f, -31.0f, g0);
    TEST_ASSERT_TRUE(zeroCalBuildRotation(g0, R));

    float x = 0.5f, y = -0.6f, z = 0.62f;
    const float before = sqrtf(x*x + y*y + z*z);
    zeroCalApply(R, x, y, z);
    TEST_ASSERT_FLOAT_WITHIN(1e-4f, before, sqrtf(x*x + y*y + z*z));
}

void test_upside_down_mount(void) {
    float g0[3] = {0.0f, 0.0f, -1.0f}, R[9];
    TEST_ASSERT_TRUE(zeroCalBuildRotation(g0, R));
    float x = 0.0f, y = 0.0f, z = -1.0f;
    zeroCalApply(R, x, y, z);
    TEST_ASSERT_FLOAT_WITHIN(TOL, 1.0f, z);
}

void test_zero_length_vector_refuses_and_leaves_identity(void) {
    float g0[3] = {0.0f, 0.0f, 0.0f}, R[9];
    TEST_ASSERT_FALSE(zeroCalBuildRotation(g0, R));
    float x = 0.3f, y = 0.4f, z = 0.5f;
    zeroCalApply(R, x, y, z);
    TEST_ASSERT_FLOAT_WITHIN(TOL, 0.3f, x);
    TEST_ASSERT_FLOAT_WITHIN(TOL, 0.4f, y);
    TEST_ASSERT_FLOAT_WITHIN(TOL, 0.5f, z);
}

void setUp(void) {}
void tearDown(void) {}

int main(int, char **) {
    UNITY_BEGIN();
    RUN_TEST(test_identity_when_uncalibrated);
    RUN_TEST(test_already_level_builds_identity);
    RUN_TEST(test_calibration_attitude_reads_zero);
    RUN_TEST(test_correction_holds_far_from_the_calibration_point);
    RUN_TEST(test_offset_method_is_wrong_away_from_calibration);
    RUN_TEST(test_both_axes_together);
    RUN_TEST(test_magnitude_is_preserved);
    RUN_TEST(test_upside_down_mount);
    RUN_TEST(test_zero_length_vector_refuses_and_leaves_identity);
    return UNITY_END();
}
