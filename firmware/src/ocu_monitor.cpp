/**
 * @file ocu_monitor.cpp
 * @brief OCU Heartbeat Monitor Implementation (passive responder)
 *
 * The board only ever REPLIES to heartbeats the OCU sends - it never initiates
 * traffic to a fixed address. This avoids the W5500 ARP-resolution stall (~1.6s
 * of blocking retries) that occurs when sending to an absent host, which would
 * otherwise freeze the HTTP server whenever the OCU is offline. See
 * ocu_monitor.hpp for the wire protocol.
 */

#include "ocu_monitor.hpp"
#include "ocu_connection_state.hpp"
#include "config_manager.hpp"
#include <Ethernet.h>
#include <EthernetUdp.h>

// ============================================================================
// INTERNAL STATE
// ============================================================================

static EthernetUDP s_udp;
static bool s_udpStarted = false;

/** Last time a heartbeat packet was received from the OCU */
static unsigned long s_lastReplyMs = 0;

/** Timestamp of the last connected->disconnected transition (0 while connected) */
static unsigned long s_disconnectedSinceMs = 0;

/** Small buffer to drain incoming heartbeat packets */
static uint8_t s_rxBuffer[32];

/** Reply payload sent back to the OCU on each received heartbeat */
static const char OCU_REPLY_PAYLOAD[] = "GRK_HB";

// ============================================================================
// IMPLEMENTATION
// ============================================================================

/** Find the whitelisted IP whose last octet is 169 (the OCU). Comparison only -
 *  we never send to it unsolicited, so no ARP resolution is triggered. */
static bool findOcuAddress(IPAddress &outAddress) {
    for (int i = 0; i < g_whitelistCount; ++i) {
        if (g_whitelist[i][3] == 169) {
            outAddress = g_whitelist[i];
            return true;
        }
    }
    return false;
}

void ocuMonitorInit() {
    s_udpStarted = s_udp.begin(OCU_HEARTBEAT_PORT);
    s_lastReplyMs = 0;
    s_disconnectedSinceMs = millis();  // no heartbeat yet, so start "disconnected"

    if (s_udpStarted) {
        Serial.println("OCU heartbeat monitor (passive) listening on port " + String(OCU_HEARTBEAT_PORT));
    } else {
        Serial.println("WARNING: OCU heartbeat UDP socket failed to start");
    }
}

void ocuMonitorUpdate() {
    if (!s_udpStarted) {
        return;
    }

    unsigned long now = millis();

    IPAddress ocuAddress;
    bool haveOcu = findOcuAddress(ocuAddress);

    // Drain any received heartbeat packets. Only packets from the OCU (the .169
    // whitelist IP) count as "alive" - a stray packet from anywhere else must
    // not falsely report OCU connectivity.
    int packetSize = s_udp.parsePacket();
    while (packetSize > 0) {
        // Capture the sender before reading (remoteIP/Port are valid after parsePacket)
        IPAddress senderIp = s_udp.remoteIP();
        uint16_t senderPort = s_udp.remotePort();

        s_udp.read(s_rxBuffer, min((size_t)packetSize, sizeof(s_rxBuffer)));

        if (haveOcu && senderIp == ocuAddress) {
            s_lastReplyMs = now;

            // Reply to the OCU. Its MAC is already ARP-cached (the packet just
            // arrived from it), so this send can never block on ARP resolution.
            s_udp.beginPacket(senderIp, senderPort);
            s_udp.write((const uint8_t*)OCU_REPLY_PAYLOAD, sizeof(OCU_REPLY_PAYLOAD) - 1);
            s_udp.endPacket();
        }

        packetSize = s_udp.parsePacket();
    }

    // Update the connected/disconnected-since state from the fresh receive time
    bool connectedNow = ocuIsConnected(s_lastReplyMs, now, OCU_HEARTBEAT_TIMEOUT_MS);
    ocuUpdateDisconnectedSince(connectedNow, now, s_disconnectedSinceMs);
}

bool ocuMonitorIsConnected() {
    return ocuIsConnected(s_lastReplyMs, millis(), OCU_HEARTBEAT_TIMEOUT_MS);
}

unsigned long ocuMonitorDisconnectedDurationMs() {
    unsigned long now = millis();
    bool connected = ocuIsConnected(s_lastReplyMs, now, OCU_HEARTBEAT_TIMEOUT_MS);
    return ocuDisconnectedDuration(connected, now, s_disconnectedSinceMs);
}
