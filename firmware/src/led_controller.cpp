/**
 * @file led_controller.cpp
 * @brief LED Status Indicator Controller Implementation
 */

#include "led_controller.hpp"
#include "status_manager.hpp"
#include "gg_hal.hpp"

// ============================================================================
// INTERNAL STATE
// ============================================================================

/** Last LED change time for blink timing */
static unsigned long s_ledLastChangeTime = 0;

/** Manual LED control flag */
static bool s_manualControlActive = false;

// External references
extern bool technician_mode;
extern GG_HAL _gg_hal;

// ============================================================================
// IMPLEMENTATION
// ============================================================================

void ledControllerInit() {
    s_ledLastChangeTime = millis();
    s_manualControlActive = false;
    Serial.println("LED controller initialized");
}

void ledControllerUpdate() {
    // Skip if manual control is active
    if (s_manualControlActive) {
        return;
    }
    
    // Get current states from global status
    bool isSafeState = g_status.safetyMode;
    bool imuConnected = g_status.imuValid;
    bool gpsConnected = g_status.gpsConnected;
    bool allSensorsConnected = imuConnected && gpsConnected;
    
    if (technician_mode) {
        // TECHNICIAN MODE LED PATTERNS
        if (isSafeState) {
            // Solid Orange: Technician mode, voltage to optocouplers (safe)
            _gg_hal.set_indicator_led(ORANGE);
        } else {
            // Blinking Orange: Technician mode, no voltage to optocouplers (unsafe)
            if ((millis() - s_ledLastChangeTime) > LED_BLINK_INTERVAL_MS) {
                if (_gg_hal.get_indicator_led_state() == OFF) {
                    _gg_hal.set_indicator_led(ORANGE);
                } else {
                    _gg_hal.set_indicator_led(OFF);
                }
                s_ledLastChangeTime = millis();
            }
        }
    } else {
        // NORMAL MODE LED PATTERNS
        if (isSafeState) {
            if (allSensorsConnected) {
                // Solid Green: GPS & IMU connected, voltage to optocouplers (safe)
                _gg_hal.set_indicator_led(GREEN);
            } else {
                // Solid Red: IMU disconnected (or GPS), voltage to optocouplers (safe)
                _gg_hal.set_indicator_led(RED);
            }
        } else {
            // Unsafe state (no voltage to optocouplers)
            if (allSensorsConnected) {
                // Blinking Green: GPS & IMU connected, no voltage to optocouplers (unsafe)
                if ((millis() - s_ledLastChangeTime) > LED_BLINK_INTERVAL_MS) {
                    if (_gg_hal.get_indicator_led_state() == OFF) {
                        _gg_hal.set_indicator_led(GREEN);
                    } else {
                        _gg_hal.set_indicator_led(OFF);
                    }
                    s_ledLastChangeTime = millis();
                }
            } else {
                // Blinking Red: IMU disconnected (or GPS), no voltage to optocouplers (unsafe)
                if ((millis() - s_ledLastChangeTime) > LED_BLINK_INTERVAL_MS) {
                    if (_gg_hal.get_indicator_led_state() == OFF) {
                        _gg_hal.set_indicator_led(RED);
                    } else {
                        _gg_hal.set_indicator_led(OFF);
                    }
                    s_ledLastChangeTime = millis();
                }
            }
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
