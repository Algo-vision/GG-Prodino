/**
 * @file ocu_monitor.hpp
 * @brief OCU (Operator Control Unit) Heartbeat Monitor
 *
 * Tracks whether the OCU is reachable by exchanging a small UDP heartbeat
 * with the whitelisted client IP whose last octet is 169.
 *
 * Wire protocol (defined here - the OCU side needs a matching responder,
 * which is separate work not covered by this firmware):
 * - Every OCU_HEARTBEAT_INTERVAL_MS, the board sends the fixed payload
 *   "GRK_HB" via UDP to the .169 whitelist IP on OCU_HEARTBEAT_PORT.
 * - The OCU is expected to reply with any UDP packet on the same port.
 * - Receiving any reply within OCU_HEARTBEAT_TIMEOUT_MS of the last
 *   successful reply marks the OCU as connected.
 */

#ifndef OCU_MONITOR_HPP
#define OCU_MONITOR_HPP

#include <Arduino.h>

// ============================================================================
// CONFIGURATION
// ============================================================================

/** UDP port used for the OCU heartbeat exchange */
constexpr unsigned int OCU_HEARTBEAT_PORT = 5001;

/** How often the board sends a heartbeat ping */
constexpr unsigned long OCU_HEARTBEAT_INTERVAL_MS = 1000;

/** Connection is considered lost if no reply arrives within this window
 *  (allows for a couple of missed beats before declaring disconnected) */
constexpr unsigned long OCU_HEARTBEAT_TIMEOUT_MS = 2500;

// ============================================================================
// OCU MONITOR FUNCTIONS
// ============================================================================

/**
 * @brief Initialize the OCU heartbeat monitor
 *
 * Call in setup() after the whitelist has been loaded.
 */
void ocuMonitorInit();

/**
 * @brief Send heartbeat pings and check for replies
 *
 * Call in loop(). Sends a ping every OCU_HEARTBEAT_INTERVAL_MS to the
 * whitelisted IP ending in .169, and processes any incoming replies.
 */
void ocuMonitorUpdate();

/**
 * @brief Check whether the OCU is currently connected
 * @return true if a heartbeat reply was received within the timeout window
 */
bool ocuMonitorIsConnected();

/**
 * @brief Get how long the OCU has been continuously disconnected
 * @return Milliseconds since the OCU was last connected, or 0 if currently connected
 */
unsigned long ocuMonitorDisconnectedDurationMs();

#endif // OCU_MONITOR_HPP
