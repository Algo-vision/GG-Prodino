/**
 * Native unit tests for the at-rest detector and the gyro bias estimator.
 * Run with: pio test -e native (from firmware/)
 *
 * These two decide when the controller is allowed to believe things about
 * itself - when a calibration may be burned, and when a bias may be learned.
 * The failure that matters is not "it missed a still moment", it is "it called
 * a moving machine still", so most of these push on that direction.
 */
#include <unity.h>
#include <math.h>
#include "../../lib/GG/src/rest_detector.cpp"
#include "../../lib/GG/src/gyro_bias.cpp"

static const float DT = 0.011f;   // the measured 10.8 ms mean

/** Feed the detector the same sample for a while. */
static float feedFor(RestDetector &d, float seconds,
                     float ax, float ay, float az,
                     float gx, float gy, float gz) {
    float out = 0.0f;
    for (float t = 0.0f; t < seconds; t += DT) {
        out = restDetectorUpdate(d, DT, ax, ay, az, gx, gy, gz);
    }
    return out;
}

// ---------------------------------------------------------------- rest ----

void test_still_board_accumulates_time(void) {
    RestDetector d; restDetectorReset(d);
    float s = feedFor(d, 3.0f, 0.0f, 0.0f, 1.0f, 0.05f, 0.05f, 0.05f);
    TEST_ASSERT_TRUE(s >= REST_SECONDS_FOR_CALIBRATION);
}

/** The worst readings a stationary board actually produced over two ten
 *  minute soaks must still count as still, or the feature never fires. */
void test_worst_observed_bench_noise_still_counts_as_still(void) {
    RestDetector d; restDetectorReset(d);
    // 0.098 g deviation and 4.35 dps - the measured extremes.
    float s = feedFor(d, 3.0f, 0.0f, 0.0f, 1.098f, 2.5f, 2.5f, 2.5f);
    TEST_ASSERT_TRUE(s >= REST_SECONDS_FOR_CALIBRATION);
}

void test_rotation_alone_defeats_it(void) {
    RestDetector d; restDetectorReset(d);
    // Gravity looks perfect; the machine is turning.
    float s = feedFor(d, 3.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 30.0f);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, s);
}

void test_linear_acceleration_alone_defeats_it(void) {
    RestDetector d; restDetectorReset(d);
    // 0.8 g of push across gravity: |a| = sqrt(1 + 0.64) = 1.28, which is
    // 0.28 g out and comfortably past the tolerance.
    float s = feedFor(d, 3.0f, 0.8f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, s);
}

/** Records the documented blind spot rather than pretending it is not there.
 *  0.5 g sideways only stretches |a| to 1.118, so it reads as rest. An
 *  accelerometer cannot separate gravity from steady acceleration - if this
 *  test ever starts failing, something has changed the detector's contract
 *  and the header's warning needs revisiting. */
void test_gentle_constant_sideways_acceleration_is_a_known_blind_spot(void) {
    RestDetector d; restDetectorReset(d);
    float s = feedFor(d, 3.0f, 0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f);
    TEST_ASSERT_TRUE(s >= REST_SECONDS_FOR_CALIBRATION);
}

/** Free fall has zero gyro AND a consistent reading - and is the last moment
 *  anyone should be calibrating. Caught by the magnitude test, not the gyro. */
void test_free_fall_is_not_rest(void) {
    RestDetector d; restDetectorReset(d);
    float s = feedFor(d, 3.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, s);
}

void test_a_single_bad_sample_resets_the_clock(void) {
    RestDetector d; restDetectorReset(d);
    feedFor(d, 5.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f);
    float s = restDetectorUpdate(d, DT, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 50.0f);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, s);
}

/** Tilted but stationary is still stationary - the initiated calibration is
 *  specified to run in a non-level attitude, so this must hold. */
void test_stationary_on_a_slope_is_rest(void) {
    RestDetector d; restDetectorReset(d);
    const float p = 35.0f * 3.14159265f / 180.0f;
    float s = feedFor(d, 3.0f, 0.0f, -sinf(p), cosf(p), 0.0f, 0.0f, 0.0f);
    TEST_ASSERT_TRUE(s >= REST_SECONDS_FOR_CALIBRATION);
}

void test_absurd_dt_earns_no_credit(void) {
    RestDetector d; restDetectorReset(d);
    float s = restDetectorUpdate(d, 5.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, s);
}

// ---------------------------------------------------------------- bias ----

void test_first_reading_is_adopted_whole(void) {
    GyroBias b; gyroBiasReset(b);
    TEST_ASSERT_TRUE(gyroBiasUpdate(b, 1.5f, -2.0f, 0.5f, DT));
    TEST_ASSERT_TRUE(b.valid);
    TEST_ASSERT_FLOAT_WITHIN(1e-4f, 1.5f, b.x);
    TEST_ASSERT_FLOAT_WITHIN(1e-4f, -2.0f, b.y);
}

void test_converges_toward_a_changed_bias(void) {
    GyroBias b; gyroBiasReset(b);
    gyroBiasUpdate(b, 0.0f, 0.0f, 0.0f, DT);      // seeded at zero
    // Sensor has drifted to 1.0 dps; feed that for three time constants.
    for (float t = 0.0f; t < 3.0f * GYRO_BIAS_TAU_S; t += DT) {
        gyroBiasUpdate(b, 1.0f, 0.0f, 0.0f, DT);
    }
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 1.0f, b.x);
}

/** The point of the long time constant: a brief window of false "rest" must
 *  not be able to drag the estimate anywhere meaningful. */
void test_a_brief_false_rest_barely_moves_it(void) {
    GyroBias b; gyroBiasReset(b);
    gyroBiasUpdate(b, 0.0f, 0.0f, 0.0f, DT);
    for (float t = 0.0f; t < 1.0f; t += DT) {     // one second of 5 dps
        gyroBiasUpdate(b, 5.0f, 0.0f, 0.0f, DT);
    }
    TEST_ASSERT_TRUE(b.x < 0.25f);
}

void test_implausible_rate_is_refused(void) {
    GyroBias b; gyroBiasReset(b);
    TEST_ASSERT_FALSE(gyroBiasUpdate(b, 100.0f, 0.0f, 0.0f, DT));
    TEST_ASSERT_FALSE(b.valid);
}

/** Convergence must depend on elapsed time, not on call count - the exact trap
 *  the pitch/roll filter's fixed per-step ALPHA falls into. */
void test_convergence_is_rate_independent(void) {
    GyroBias fast; gyroBiasReset(fast);
    GyroBias slow; gyroBiasReset(slow);
    gyroBiasUpdate(fast, 0.0f, 0.0f, 0.0f, 0.010f);
    gyroBiasUpdate(slow, 0.0f, 0.0f, 0.0f, 0.100f);

    for (float t = 0.0f; t < 20.0f; t += 0.010f) gyroBiasUpdate(fast, 1.0f, 0, 0, 0.010f);
    for (float t = 0.0f; t < 20.0f; t += 0.100f) gyroBiasUpdate(slow, 1.0f, 0, 0, 0.100f);

    TEST_ASSERT_FLOAT_WITHIN(0.02f, fast.x, slow.x);
}

void setUp(void) {}
void tearDown(void) {}

int main(int, char **) {
    UNITY_BEGIN();
    RUN_TEST(test_still_board_accumulates_time);
    RUN_TEST(test_worst_observed_bench_noise_still_counts_as_still);
    RUN_TEST(test_rotation_alone_defeats_it);
    RUN_TEST(test_linear_acceleration_alone_defeats_it);
    RUN_TEST(test_gentle_constant_sideways_acceleration_is_a_known_blind_spot);
    RUN_TEST(test_free_fall_is_not_rest);
    RUN_TEST(test_a_single_bad_sample_resets_the_clock);
    RUN_TEST(test_stationary_on_a_slope_is_rest);
    RUN_TEST(test_absurd_dt_earns_no_credit);
    RUN_TEST(test_first_reading_is_adopted_whole);
    RUN_TEST(test_converges_toward_a_changed_bias);
    RUN_TEST(test_a_brief_false_rest_barely_moves_it);
    RUN_TEST(test_implausible_rate_is_refused);
    RUN_TEST(test_convergence_is_rate_independent);
    return UNITY_END();
}
