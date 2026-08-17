/**
 * Native unit tests for the OCU heartbeat connection-state logic.
 * Run with: pio test -e native (from firmware/)
 */
#include <unity.h>
#include "../../lib/GG/src/ocu_connection_state.hpp"

static const unsigned long TIMEOUT_MS = 2500;

void test_never_replied_is_not_connected(void) {
    TEST_ASSERT_FALSE(ocuIsConnected(0, 10000, TIMEOUT_MS));
}

void test_recent_reply_is_connected(void) {
    // Reply 100ms ago, well within the 2500ms timeout
    TEST_ASSERT_TRUE(ocuIsConnected(1000, 1100, TIMEOUT_MS));
}

void test_reply_exactly_at_timeout_boundary_is_disconnected(void) {
    // now - lastReplyMs == timeoutMs -> not "< timeoutMs", so disconnected
    TEST_ASSERT_FALSE(ocuIsConnected(1000, 1000 + TIMEOUT_MS, TIMEOUT_MS));
}

void test_reply_just_under_timeout_is_connected(void) {
    TEST_ASSERT_TRUE(ocuIsConnected(1000, 1000 + TIMEOUT_MS - 1, TIMEOUT_MS));
}

void test_reply_past_timeout_is_disconnected(void) {
    TEST_ASSERT_FALSE(ocuIsConnected(1000, 1000 + TIMEOUT_MS + 500, TIMEOUT_MS));
}

void test_disconnected_since_set_on_transition_to_disconnected(void) {
    unsigned long disconnectedSinceMs = 0; // starts "connected" (0 = no transition yet)
    ocuUpdateDisconnectedSince(/*connected=*/false, /*now=*/5000, disconnectedSinceMs);
    TEST_ASSERT_EQUAL_UINT32(5000, disconnectedSinceMs);
}

void test_disconnected_since_does_not_change_while_still_disconnected(void) {
    unsigned long disconnectedSinceMs = 5000;
    ocuUpdateDisconnectedSince(false, 8000, disconnectedSinceMs);
    // Should still be the ORIGINAL disconnect time, not overwritten
    TEST_ASSERT_EQUAL_UINT32(5000, disconnectedSinceMs);
}

void test_disconnected_since_resets_to_zero_on_reconnect(void) {
    unsigned long disconnectedSinceMs = 5000;
    ocuUpdateDisconnectedSince(/*connected=*/true, 9000, disconnectedSinceMs);
    TEST_ASSERT_EQUAL_UINT32(0, disconnectedSinceMs);
}

void test_duration_is_zero_while_connected(void) {
    TEST_ASSERT_EQUAL_UINT32(0, ocuDisconnectedDuration(true, 10000, 5000));
}

void test_duration_is_zero_when_never_disconnected(void) {
    TEST_ASSERT_EQUAL_UINT32(0, ocuDisconnectedDuration(false, 10000, 0));
}

void test_duration_counts_up_while_disconnected(void) {
    TEST_ASSERT_EQUAL_UINT32(3000, ocuDisconnectedDuration(false, 8000, 5000));
}

// End-to-end: simulate a heartbeat sequence over "time" and confirm the
// full connected/duration story is consistent at each step.
void test_full_sequence_connect_disconnect_reconnect(void) {
    unsigned long disconnectedSinceMs = 0;
    unsigned long lastReplyMs = 0;

    // t=0: boot, no reply yet -> disconnected
    bool connected = ocuIsConnected(lastReplyMs, 0, TIMEOUT_MS);
    ocuUpdateDisconnectedSince(connected, 0, disconnectedSinceMs);
    TEST_ASSERT_FALSE(connected);
    TEST_ASSERT_EQUAL_UINT32(0, disconnectedSinceMs); // ocuMonitorInit sets this separately in production

    // t=1000: reply arrives -> connected
    lastReplyMs = 1000;
    connected = ocuIsConnected(lastReplyMs, 1000, TIMEOUT_MS);
    ocuUpdateDisconnectedSince(connected, 1000, disconnectedSinceMs);
    TEST_ASSERT_TRUE(connected);
    TEST_ASSERT_EQUAL_UINT32(0, disconnectedSinceMs);

    // t=5000: no reply since t=1000, timeout (2500ms) exceeded -> disconnected
    connected = ocuIsConnected(lastReplyMs, 5000, TIMEOUT_MS);
    ocuUpdateDisconnectedSince(connected, 5000, disconnectedSinceMs);
    TEST_ASSERT_FALSE(connected);
    TEST_ASSERT_EQUAL_UINT32(5000, disconnectedSinceMs);
    TEST_ASSERT_EQUAL_UINT32(0, ocuDisconnectedDuration(connected, 5000, disconnectedSinceMs));

    // t=7000: still disconnected, 2000ms into the disconnected streak
    connected = ocuIsConnected(lastReplyMs, 7000, TIMEOUT_MS);
    ocuUpdateDisconnectedSince(connected, 7000, disconnectedSinceMs);
    TEST_ASSERT_FALSE(connected);
    TEST_ASSERT_EQUAL_UINT32(2000, ocuDisconnectedDuration(connected, 7000, disconnectedSinceMs));

    // t=7500: reply arrives again -> reconnected, duration resets
    lastReplyMs = 7500;
    connected = ocuIsConnected(lastReplyMs, 7500, TIMEOUT_MS);
    ocuUpdateDisconnectedSince(connected, 7500, disconnectedSinceMs);
    TEST_ASSERT_TRUE(connected);
    TEST_ASSERT_EQUAL_UINT32(0, ocuDisconnectedDuration(connected, 7500, disconnectedSinceMs));
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_never_replied_is_not_connected);
    RUN_TEST(test_recent_reply_is_connected);
    RUN_TEST(test_reply_exactly_at_timeout_boundary_is_disconnected);
    RUN_TEST(test_reply_just_under_timeout_is_connected);
    RUN_TEST(test_reply_past_timeout_is_disconnected);
    RUN_TEST(test_disconnected_since_set_on_transition_to_disconnected);
    RUN_TEST(test_disconnected_since_does_not_change_while_still_disconnected);
    RUN_TEST(test_disconnected_since_resets_to_zero_on_reconnect);
    RUN_TEST(test_duration_is_zero_while_connected);
    RUN_TEST(test_duration_is_zero_when_never_disconnected);
    RUN_TEST(test_duration_counts_up_while_disconnected);
    RUN_TEST(test_full_sequence_connect_disconnect_reconnect);
    return UNITY_END();
}
