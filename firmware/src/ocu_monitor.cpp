/**
 * @file ocu_monitor.cpp
 * @brief OCU Connection Monitor Implementation (link-level reachability)
 *
 * Determines whether the OCU is up on the network without requiring anything
 * to be running on it - see ocu_monitor.hpp for the three signals used and
 * for the blocking cost of a probe to an absent host.
 */

#include "ocu_monitor.hpp"
#include "ocu_connection_state.hpp"
#include "config_manager.hpp"
#include <Ethernet.h>
#include <EthernetUdp.h>

// ============================================================================
// INTERNAL STATE
// ============================================================================

/** Socket used to source the ARP probe */
static EthernetUDP s_udp;
static bool s_udpStarted = false;

/** Last time the OCU was confirmed reachable (0 = never) */
static unsigned long s_lastSeenMs = 0;

/** Timestamp of the last reachable->unreachable transition (0 while reachable) */
static unsigned long s_disconnectedSinceMs = 0;

/** Last time a probe was attempted */
static unsigned long s_lastProbeMs = 0;

// ============================================================================
// IMPLEMENTATION
// ============================================================================

/** Find the whitelisted IP whose last octet is 169 (the OCU). */
static bool findOcuAddress(IPAddress &outAddress) {
    for (int i = 0; i < g_whitelistCount; ++i) {
        if (g_whitelist[i][3] == 169) {
            outAddress = g_whitelist[i];
            return true;
        }
    }
    return false;
}

/**
 * @brief Send a one-byte datagram to the OCU and report whether ARP resolved.
 *
 * The payload is meaningless and nothing needs to be listening at the far end
 * - endPacket() returns false only when the W5500 gave up resolving the OCU's
 * MAC address, which is exactly "the OCU is not on the network".
 */
static bool probeOcu() {
    if (!s_udpStarted) {
        return false;
    }

    IPAddress ocuAddress;
    if (!findOcuAddress(ocuAddress)) {
        return false;
    }

    if (!s_udp.beginPacket(ocuAddress, OCU_PROBE_PORT)) {
        return false;
    }
    s_udp.write((uint8_t)0);
    return s_udp.endPacket() != 0;
}

void ocuMonitorInit() {
    s_lastSeenMs = 0;
    s_disconnectedSinceMs = millis();  // nothing confirmed yet, so start "disconnected"
    s_lastProbeMs = 0;

    // Bound how long a probe to an absent OCU can block the main loop.
    // Global setting - also applies to the HTTP server's TCP retransmits.
    Ethernet.setRetransmissionCount(OCU_ARP_RETRY_COUNT);

    s_udpStarted = s_udp.begin(OCU_PROBE_LOCAL_PORT);
    if (!s_udpStarted) {
        Serial.println("WARNING: OCU probe UDP socket failed to start");
    }

    IPAddress ocuAddress;
    if (findOcuAddress(ocuAddress)) {
        Serial.println("OCU monitor probing " + ocuAddress.toString() + " for link-level reachability");
    } else {
        Serial.println("WARNING: no whitelisted IP ends in .169 - OCU can never report connected");
    }
}

void ocuMonitorNotifyActivity(const IPAddress& senderIp) {
    IPAddress ocuAddress;
    if (findOcuAddress(ocuAddress) && senderIp == ocuAddress) {
        s_lastSeenMs = millis();
    }
}

void ocuMonitorUpdate() {
    unsigned long now = millis();

    // 1. No cable, no reachability. Report it immediately rather than waiting
    //    out OCU_ACTIVITY_TIMEOUT_MS, and skip the probe - with the link down
    //    it could only burn the full ARP retry window.
    if (Ethernet.linkStatus() == LinkOFF) {
        s_lastSeenMs = 0;
        ocuUpdateDisconnectedSince(false, now, s_disconnectedSinceMs);
        return;
    }

    bool connected = ocuIsConnected(s_lastSeenMs, now, OCU_ACTIVITY_TIMEOUT_MS);
    unsigned long interval = connected ? OCU_PROBE_INTERVAL_CONNECTED_MS
                                       : OCU_PROBE_INTERVAL_UNREACHABLE_MS;

    // 2. Skip the probe if the OCU has already proved itself within this
    //    interval by sending us something (see ocuMonitorNotifyActivity).
    bool heardRecently = (s_lastSeenMs != 0) && ((now - s_lastSeenMs) < interval);

    // 3. Probe when due.
    if (!heardRecently && (now - s_lastProbeMs) >= interval) {
        s_lastProbeMs = now;
        if (probeOcu()) {
            s_lastSeenMs = millis();
        }
        now = millis();  // a failed probe blocks for the ARP retry window
    }

    connected = ocuIsConnected(s_lastSeenMs, now, OCU_ACTIVITY_TIMEOUT_MS);
    ocuUpdateDisconnectedSince(connected, now, s_disconnectedSinceMs);
}

bool ocuMonitorIsConnected() {
    return ocuIsConnected(s_lastSeenMs, millis(), OCU_ACTIVITY_TIMEOUT_MS);
}

unsigned long ocuMonitorDisconnectedDurationMs() {
    unsigned long now = millis();
    bool connected = ocuIsConnected(s_lastSeenMs, now, OCU_ACTIVITY_TIMEOUT_MS);
    return ocuDisconnectedDuration(connected, now, s_disconnectedSinceMs);
}
