/**
 * @file ocu_monitor.hpp
 * @brief OCU (Operator Control Unit) Connection Monitor
 *
 * Tracks whether the OCU - the whitelisted IP whose last octet is 169 - is
 * reachable at the LINK level. Nothing has to be running on the OCU: the
 * check succeeds as long as its network stack is up and answering ARP, so
 * ocuConnected stays true with the desktop GUI closed.
 *
 * How reachability is determined, cheapest signal first:
 *
 * 1. PHY link (Ethernet.linkStatus()). Cable unplugged -> instantly
 *    disconnected, and no probe is attempted (it could only time out).
 * 2. Traffic already arriving from the OCU (ocuMonitorNotifyActivity()).
 *    Free, and it suppresses the probe below while the OCU is talking to us.
 * 3. An ARP probe. A one-byte UDP datagram is sent to the OCU; the W5500 must
 *    resolve its MAC by ARP before it can transmit, so endPacket() succeeding
 *    proves the OCU answered an ARP request - i.e. it is powered on and on the
 *    network. Nothing needs to listen on OCU_PROBE_PORT; the datagram itself
 *    is irrelevant and the OCU is free to discard it.
 *
 * Note on ICMP: a literal `ping` cannot be used. The W5500 answers incoming
 * echo requests in hardware, so pings TO the board never reach the firmware,
 * and the Ethernet library exposes no ICMP sender. The ARP probe above tests
 * the same thing - is the host alive on the LAN - one layer lower.
 *
 * COST / BLOCKING: an ARP probe to a host that IS present resolves in about a
 * millisecond. To a host that is ABSENT it blocks the main loop for the full
 * ARP retry window (RTR x (RCR+1)), which is why probes back off to
 * OCU_PROBE_INTERVAL_UNREACHABLE_MS once the OCU is considered gone, and why
 * ocuMonitorInit() lowers the retry count - see OCU_ARP_RETRY_COUNT.
 */

#ifndef OCU_MONITOR_HPP
#define OCU_MONITOR_HPP

#include <Arduino.h>
#include <IPAddress.h>

// ============================================================================
// CONFIGURATION
// ============================================================================

/** Connection is considered lost if the OCU has not been confirmed reachable
 *  within this window (allows a couple of missed probes before flapping) */
constexpr unsigned long OCU_ACTIVITY_TIMEOUT_MS = 5000;

/** Probe cadence while the OCU is reachable. Cheap - ARP resolves in ~1ms. */
constexpr unsigned long OCU_PROBE_INTERVAL_CONNECTED_MS = 1000;

/** Probe cadence once the OCU is unreachable. Much slower: every failed probe
 *  blocks the main loop for the ARP retry window, so this bounds the cost of
 *  an absent OCU (and an absent OCU is already a degraded state). */
constexpr unsigned long OCU_PROBE_INTERVAL_UNREACHABLE_MS = 10000;

/** Local UDP port used to source the ARP probe */
constexpr unsigned int OCU_PROBE_LOCAL_PORT = 5001;

/** Destination port of the ARP probe. Nothing listens there - only the ARP
 *  resolution that precedes the send is of interest. */
constexpr unsigned int OCU_PROBE_PORT = 5001;

/** W5500 ARP/TCP retry count set at init (register RCR).
 *  Chip default is 8, i.e. 9 attempts x 200ms = ~1.8s of blocking per failed
 *  probe. 3 gives 4 attempts (~800ms), still generous for a wired LAN where
 *  RTT is under a millisecond.
 *  NOTE: RCR is global - it also governs TCP retransmission for the HTTP
 *  server. Raise it if this board is ever run over a lossy link. */
constexpr uint8_t OCU_ARP_RETRY_COUNT = 3;

// ============================================================================
// OCU MONITOR FUNCTIONS
// ============================================================================

/**
 * @brief Initialize the OCU connection monitor
 *
 * Call in setup() after Ethernet.begin() and after the whitelist is loaded.
 * Opens the probe socket and applies OCU_ARP_RETRY_COUNT.
 */
void ocuMonitorInit();

/**
 * @brief Report that a packet arrived from a client
 *
 * Optional fast path: call for every inbound request with the sender's
 * address. Traffic from the OCU confirms reachability for free and suppresses
 * the next ARP probe. Any other address is ignored, so another client can
 * never falsely report OCU connectivity.
 *
 * @param senderIp Address the request came from
 */
void ocuMonitorNotifyActivity(const IPAddress& senderIp);

/**
 * @brief Check the link and probe the OCU when due
 *
 * Call in loop(). See the blocking note in the file header.
 */
void ocuMonitorUpdate();

/**
 * @brief Check whether the OCU is currently reachable
 * @return true if the OCU was confirmed reachable within the timeout window
 */
bool ocuMonitorIsConnected();

/**
 * @brief Get how long the OCU has been continuously unreachable
 * @return Milliseconds since the OCU was last reachable, or 0 if reachable now
 */
unsigned long ocuMonitorDisconnectedDurationMs();

#endif // OCU_MONITOR_HPP
