/**
 * @file relay_controller.cpp
 * @brief Relay Auto-Reset Controller Implementation
 */

#include "relay_controller.hpp"
#include "KMPProDinoMKRZero.h"

// ============================================================================
// INTERNAL STATE
// ============================================================================

/** Auto-reset target times for each relay */
static unsigned long s_autoResetTime[RELAY_COUNT_MAX] = {0, 0, 0, 0};

/** Auto-reset active flags for each relay */
static bool s_autoResetActive[RELAY_COUNT_MAX] = {false, false, false, false};

// ============================================================================
// IMPLEMENTATION
// ============================================================================

void relayControllerInit() {
    for (int i = 0; i < RELAY_COUNT_MAX; i++) {
        s_autoResetTime[i] = 0;
        s_autoResetActive[i] = false;
    }
    Serial.println("Relay controller initialized");
}

void relayControllerUpdate() {
    unsigned long now = millis();
    
    for (int i = 0; i < RELAY_COUNT_MAX; i++) {
        if (s_autoResetActive[i] && now >= s_autoResetTime[i]) {
            KMPProDinoMKRZero.SetRelayState(i, false);
            s_autoResetActive[i] = false;
            Serial.print("Relay ");
            Serial.print(i);
            Serial.println(" auto-reset to OFF");
        }
    }
}

bool relayControllerSet(int relayIndex, bool state, unsigned long autoResetMs) {
    if (relayIndex < 0 || relayIndex >= RELAY_COUNT_MAX) {
        return false;
    }
    
    // Set the relay state
    KMPProDinoMKRZero.SetRelayState(relayIndex, state);
    
    // Setup auto-reset if requested and relay is being turned ON
    if (state && autoResetMs > 0) {
        s_autoResetTime[relayIndex] = millis() + autoResetMs;
        s_autoResetActive[relayIndex] = true;
        Serial.print("Relay ");
        Serial.print(relayIndex);
        Serial.print(" set to ON with auto-reset in ");
        Serial.print(autoResetMs);
        Serial.println("ms");
    } else {
        // Cancel any pending auto-reset when manually setting state
        s_autoResetActive[relayIndex] = false;
    }
    
    return true;
}

void relayControllerCancelAutoReset(int relayIndex) {
    if (relayIndex >= 0 && relayIndex < RELAY_COUNT_MAX) {
        s_autoResetActive[relayIndex] = false;
    }
}

bool relayControllerIsAutoResetPending(int relayIndex) {
    if (relayIndex >= 0 && relayIndex < RELAY_COUNT_MAX) {
        return s_autoResetActive[relayIndex];
    }
    return false;
}

bool relayControllerGetState(int relayIndex) {
    if (relayIndex >= 0 && relayIndex < RELAY_COUNT_MAX) {
        return KMPProDinoMKRZero.GetRelayState(relayIndex);
    }
    return false;
}
