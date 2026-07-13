/**
 * @file led_controller.cpp
 * @brief LED Status Indicator Controller Implementation
 */

#include "led_controller.hpp"
#include "status_manager.hpp"
#include "ocu_monitor.hpp"
#include "gg_hal.hpp"

// ============================================================================
// INTERNAL STATE
// ============================================================================

/** Last LED change time for blink timing */
static unsigned long s_ledLastChangeTime = 0;

/** Timestamp of ledControllerInit(), for the boot-up grace period */
static unsigned long s_bootStartTime = 0;

/** Manual LED control flag */
static bool s_manualControlActive = false;

// External references
extern bool technician_mode;
extern GG_HAL _gg_hal;

// ============================================================================
// IMPLEMENTATION
// ============================================================================

/** Toggle the given color on/off at LED_BLINK_INTERVAL_MS */
static void blinkColor(LED_STATES color) {
    if ((millis() - s_ledLastChangeTime) > LED_BLINK_INTERVAL_MS) {
        if (_gg_hal.get_indicator_led_state() == OFF) {
            _gg_hal.set_indicator_led(color);
        } else {
            _gg_hal.set_indicator_led(OFF);
        }
        s_ledLastChangeTime = millis();
    }
}

void ledControllerInit() {
    s_ledLastChangeTime = millis();
    s_bootStartTime = millis();
    s_manualControlActive = false;
    Serial.println("LED controller initialized");
}

void ledControllerUpdate() {
    // Skip if manual control is active
    if (s_manualControlActive) {
        return;
    }

    bool stillBooting = (millis() - s_bootStartTime) < LED_BOOT_GRACE_MS;

    if (technician_mode || stillBooting) {
        // Solid Orange: technician mode, or still within the boot-up grace period
        _gg_hal.set_indicator_led(ORANGE);
        return;
    }

    bool ocuConnected = ocuMonitorIsConnected();
    bool isSafeState = g_status.safetyMode;

    if (ocuConnected) {
        if (isSafeState) {
            // Solid Green: OCU connected, safety mode active
            _gg_hal.set_indicator_led(GREEN);
        } else {
            // Blinking Green: OCU connected, safety mode not active
            blinkColor(GREEN);
        }
    } else {
        if (isSafeState) {
            // Blinking Red: OCU disconnected, safety mode active
            blinkColor(RED);
        } else {
            // Solid Red: OCU disconnected, safety mode not active
            _gg_hal.set_indicator_led(RED);
        }
    }
}

void ledControllerSetManualMode(bool enabled) {
    s_manualControlActive = enabled;
    if (enabled) {
        Serial.println("LED manual control enabled");
    } else {
        Serial.println("LED automatic control enabled");
        s_ledLastChangeTime = millis();  // Reset blink timer
    }
}

bool ledControllerIsManualMode() {
    return s_manualControlActive;
}
