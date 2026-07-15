/**
 * @file ocu_monitor.hpp
 * @brief OCU (Operator Control Unit) Heartbeat Monitor
 *
 * Tracks whether the OCU is reachable via a UDP heartbeat.
 *
 * The board is a PASSIVE RESPONDER - it never initiates traffic to the OCU.
 * This is deliberate: actively sending to a fixed OCU address when the OCU is
 * absent forces the W5500 to do a blocking ARP resolution (~1.6s of retries)
 * on every send, which stalls the HTTP server. By only ever replying to a
 * packet the OCU just sent us, the destination MAC is already ARP-cached, so
 * the reply never blocks - even when the OCU is offline (we simply stay quiet).
 *
 * Wire protocol (the OCU side needs a matching sender, which is separate work
 * not covered by this firmware):
 * - The OCU - the whitelisted IP whose last octet is 169 - sends a UDP packet
 *   (payload "GRK_HB", though any payload works) to the board's
 *   OCU_HEARTBEAT_PORT roughly once per second.
 * - On receipt from that .169 address, the board marks the OCU connected and
 *   replies "GRK_HB" to the sender (so the OCU can also confirm the board is
 *   alive). Packets from any other source are ignored, so a stray packet can't
 *   falsely report OCU connectivity.
 * - If no packet arrives within OCU_HEARTBEAT_TIMEOUT_MS, the OCU is considered
 *   disconnected.
 */

#ifndef OCU_MONITOR_HPP
#define OCU_MONITOR_HPP

#include <Arduino.h>

// ============================================================================
// CONFIGURATION
// ============================================================================

/** UDP port the board listens on for OCU heartbeats (and replies from) */
constexpr unsigned int OCU_HEARTBEAT_PORT = 5001;

/** Connection is considered lost if no heartbeat arrives within this window
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
 * @brief Process incoming OCU heartbeats and reply to them
 *
 * Call in loop(). Drains any received heartbeat packets, marks the OCU
 * connected, and replies to the sender. Never initiates traffic on its own,
 * so it can never block on ARP resolution.
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
