/**
 * Native unit tests for the shared sanity-check (not-all-zero, not-stuck)
 * logic. Run with: pio test -e native (from firmware/)
 */
#include <unity.h>
#include "../../lib/GG/src/sanity_check.hpp"

void test_all_zero_is_not_sane(void) {
    float current[3] = {0, 0, 0};
    float previous[3] = {0, 0, 0};
    uint8_t stuckCounter = 0;
    // All-zero on the very first call: previous starts equal to current
    // (both zero), so this is also "stuck" from sample 1 - but critically
    // it must fail on allZero regardless of the stuck counter.
    bool sane = checkSane(current, previous, 3, stuckCounter, 3);
    TEST_ASSERT_FALSE(sane);
}

void test_changing_values_are_sane(void) {
    float previous[3] = {1.0f, 2.0f, 3.0f};
    uint8_t stuckCounter = 0;
    for (int i = 0; i < 10; i++) {
        float current[3] = {1.0f + i, 2.0f + i * 0.5f, 3.0f - i * 0.1f};
        bool sane = checkSane(current, previous, 3, stuckCounter, 3);
        TEST_ASSERT_TRUE(sane);
    }
}

void test_stuck_value_detected_after_threshold(void) {
    float previous[3] = {5.0f, 5.0f, 5.0f};
    uint8_t stuckCounter = 0;
    float current[3] = {5.0f, 5.0f, 5.0f};

    // Threshold=3: first 2 repeats still count as sane (stuckCounter 1, 2),
    // the 3rd repeat trips it (stuckCounter reaches 3, == threshold).
    TEST_ASSERT_TRUE(checkSane(current, previous, 3, stuckCounter, 3));   // stuckCounter -> 1
    TEST_ASSERT_TRUE(checkSane(current, previous, 3, stuckCounter, 3));   // stuckCounter -> 2
    TEST_ASSERT_FALSE(checkSane(current, previous, 3, stuckCounter, 3));  // stuckCounter -> 3, >= threshold
}

void test_stuck_counter_resets_on_change(void) {
    float previous[3] = {5.0f, 5.0f, 5.0f};
    uint8_t stuckCounter = 0;
    float stuckValue[3] = {5.0f, 5.0f, 5.0f};
    float changedValue[3] = {5.0f, 5.0f, 6.0f};

    checkSane(stuckValue, previous, 3, stuckCounter, 3);  // stuckCounter -> 1
    checkSane(stuckValue, previous, 3, stuckCounter, 3);  // stuckCounter -> 2
    checkSane(changedValue, previous, 3, stuckCounter, 3); // changed -> resets to 0
    TEST_ASSERT_EQUAL_UINT8(0, stuckCounter);

    // Now stuck again on the new value - takes a fresh 3 in a row to trip
    TEST_ASSERT_TRUE(checkSane(changedValue, previous, 3, stuckCounter, 3));
    TEST_ASSERT_TRUE(checkSane(changedValue, previous, 3, stuckCounter, 3));
    TEST_ASSERT_FALSE(checkSane(changedValue, previous, 3, stuckCounter, 3));
}

void test_negative_check_only_applies_when_requested(void) {
    float previous[2] = {0, 0};
    uint8_t stuckCounter = 0;
    float current[2] = {-1.0f, 2.0f};

    // Without requireNonNegative, negative values are fine
    bool saneWithoutCheck = checkSane(current, previous, 2, stuckCounter, 3, false);
    TEST_ASSERT_TRUE(saneWithoutCheck);

    // Reset state and check again WITH requireNonNegative
    float previous2[2] = {0, 0};
    uint8_t stuckCounter2 = 0;
    bool saneWithCheck = checkSane(current, previous2, 2, stuckCounter2, 3, true);
    TEST_ASSERT_FALSE(saneWithCheck);
}

void test_double_precision_works_for_gps(void) {
    // GPS lat/lng need double precision - confirm the template works for it
    double previous[3] = {32.0853, 34.7818, 150.2};
    double current[3] = {32.0854, 34.7819, 150.3};
    uint8_t stuckCounter = 0;
    bool sane = checkSane(current, previous, 3, stuckCounter, 3);
    TEST_ASSERT_TRUE(sane);
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_all_zero_is_not_sane);
    RUN_TEST(test_changing_values_are_sane);
    RUN_TEST(test_stuck_value_detected_after_threshold);
    RUN_TEST(test_stuck_counter_resets_on_change);
    RUN_TEST(test_negative_check_only_applies_when_requested);
    RUN_TEST(test_double_precision_works_for_gps);
    return UNITY_END();
}
