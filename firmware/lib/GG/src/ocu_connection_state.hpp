#pragma once

// ============================================================================
// OCU CONNECTION STATE
// ============================================================================
//
// Pure state-transition logic for the OCU heartbeat, decoupled from the
// actual UDP I/O and from calling millis() directly (time is passed in
// explicitly) so it can be unit-tested natively (see
// firmware/test/test_ocu_state.cpp). Used by ocu_monitor.cpp, which owns
// the actual socket send/receive and supplies real millis() values.

/** Is the OCU connected, given when the last heartbeat reply arrived? */
inline bool ocuIsConnected(unsigned long lastReplyMs, unsigned long now, unsigned long timeoutMs) {
    return (lastReplyMs != 0) && (now - lastReplyMs < timeoutMs);
}

/**
 * @brief Update the connected->disconnected transition timestamp.
 * @param connected Current connection state (from ocuIsConnected()).
 * @param now Current time.
 * @param disconnectedSinceMs In/out - 0 while connected; set to `now` on the
 *   moment of transitioning to disconnected; left unchanged while already
 *   disconnected; reset to 0 on transitioning back to connected.
 */
inline void ocuUpdateDisconnectedSince(bool connected, unsigned long now, unsigned long &disconnectedSinceMs) {
    if (connected) {
        disconnectedSinceMs = 0;
    } else if (disconnectedSinceMs == 0) {
        disconnectedSinceMs = now;
    }
}

/** How long has it been continuously disconnected (0 if currently connected) */
inline unsigned long ocuDisconnectedDuration(bool connected, unsigned long now, unsigned long disconnectedSinceMs) {
    if (connected || disconnectedSinceMs == 0) {
        return 0;
    }
    return now - disconnectedSinceMs;
}
