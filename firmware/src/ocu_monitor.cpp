/**
 * @file ocu_monitor.cpp
 * @brief OCU Heartbeat Monitor Implementation
 */

#include "ocu_monitor.hpp"
#include "config_manager.hpp"
#include <Ethernet.h>
#include <EthernetUdp.h>

// ============================================================================
// INTERNAL STATE
// ============================================================================

static EthernetUDP s_udp;
static bool s_udpStarted = false;

/** Last time a heartbeat ping was sent */
static unsigned long s_lastPingMs = 0;

/** Last time a heartbeat reply was received */
static unsigned long s_lastReplyMs = 0;

/** Timestamp of the last connected->disconnected transition (0 while connected) */
static unsigned long s_disconnectedSinceMs = 0;

/** Small buffer to drain incoming heartbeat reply packets */
static uint8_t s_rxBuffer[32];

// ============================================================================
// IMPLEMENTATION
// ============================================================================

/** Find the whitelisted IP whose last octet is 169, or return false if none configured */
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
    s_lastPingMs = 0;
    s_lastReplyMs = 0;
    s_disconnectedSinceMs = millis();  // no reply yet, so start "disconnected"

    if (s_udpStarted) {
        Serial.println("OCU heartbeat monitor started on port " + String(OCU_HEARTBEAT_PORT));
    } else {
        Serial.println("WARNING: OCU heartbeat UDP socket failed to start");
    }
}

void ocuMonitorUpdate() {
    if (!s_udpStarted) {
        return;
    }

    unsigned long now = millis();

    // Send a heartbeat ping at the configured interval
    if (now - s_lastPingMs >= OCU_HEARTBEAT_INTERVAL_MS) {
        s_lastPingMs = now;

        IPAddress ocuAddress;
        if (findOcuAddress(ocuAddress)) {
            s_udp.beginPacket(ocuAddress, OCU_HEARTBEAT_PORT);
            s_udp.write((const uint8_t*)"GRK_HB", 6);
            s_udp.endPacket();
        }
    }

    // Check for a reply (any packet on this port counts as alive)
    int packetSize = s_udp.parsePacket();
    if (packetSize > 0) {
        s_udp.read(s_rxBuffer, min((size_t)packetSize, sizeof(s_rxBuffer)));

        bool wasDisconnected = (s_disconnectedSinceMs != 0);
        s_lastReplyMs = now;
        if (wasDisconnected) {
            s_disconnectedSinceMs = 0;  // transitioned back to connected
        }
    }

    // Detect a connected->disconnected transition
    bool connectedNow = (s_lastReplyMs != 0) && (now - s_lastReplyMs < OCU_HEARTBEAT_TIMEOUT_MS);
    if (!connectedNow && s_disconnectedSinceMs == 0) {
        s_disconnectedSinceMs = now;
    }
}

bool ocuMonitorIsConnected() {
    if (s_lastReplyMs == 0) {
        return false;
    }
    return (millis() - s_lastReplyMs) < OCU_HEARTBEAT_TIMEOUT_MS;
}

unsigned long ocuMonitorDisconnectedDurationMs() {
    if (ocuMonitorIsConnected() || s_disconnectedSinceMs == 0) {
        return 0;
    }
    return millis() - s_disconnectedSinceMs;
}
