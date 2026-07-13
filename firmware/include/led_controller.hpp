/**
 * @file led_controller.hpp
 * @brief LED Status Indicator Controller
 *
 * Manages the bi-color status LED based on:
 * - Technician mode / system boot-up
 * - OCU heartbeat connectivity (see ocu_monitor.hpp)
 * - Safety mode (both optocoupler safety inputs active)
 *
 * LED States:
 * - Solid Orange:   Technician mode, or still within the boot-up grace period
 * - Solid Green:    OCU connected, safety mode active
 * - Blink Green:    OCU connected, safety mode not active
 * - Solid Red:      OCU disconnected, safety mode not active
 * - Blink Red:      OCU disconnected, safety mode active
 */

#ifndef LED_CONTROLLER_HPP
#define LED_CONTROLLER_HPP

#include <Arduino.h>

// ============================================================================
// CONFIGURATION
// ============================================================================

/** LED blink interval in milliseconds */
constexpr unsigned long LED_BLINK_INTERVAL_MS = 500;

/** Boot-up grace period: LED shows solid orange for this long after start,
 *  regardless of OCU/safety state, while sensors/network/OCU comm settle */
constexpr unsigned long LED_BOOT_GRACE_MS = 60000;

// ============================================================================
// LED CONTROLLER FUNCTIONS
// ============================================================================

/**
 * @brief Initialize LED controller
 * 
 * Call in setup() after HAL initialization.
 */
void ledControllerInit();

/**
 * @brief Update LED state based on system status
 * 
 * Call in loop() to update LED blink patterns.
 * Respects manual LED control flag - does nothing if manual control is active.
 */
void ledControllerUpdate();

/**
 * @brief Enable/disable manual LED control
 * @param enabled true to enable manual control, false for automatic
 * 
 * When manual control is enabled, ledControllerUpdate() does nothing,
 * allowing external code to directly set LED state.
 */
void ledControllerSetManualMode(bool enabled);

/**
 * @brief Check if manual LED control is active
 * @return true if manual control is enabled
 */
bool ledControllerIsManualMode();

#endif // LED_CONTROLLER_HPP
