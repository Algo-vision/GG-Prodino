/**
 * @file led_controller.hpp
 * @brief LED Status Indicator Controller
 * 
 * Manages the bi-color status LED with blink patterns based on:
 * - Technician mode state
 * - Optocoupler safety state (voltage present)
 * - Sensor connectivity (GPS + IMU)
 * 
 * LED States:
 * - Solid Green: Normal mode, all sensors connected, safe state
 * - Blink Green: Normal mode, all sensors connected, unsafe state
 * - Solid Red: Normal mode, sensor disconnected, safe state
 * - Blink Red: Normal mode, sensor disconnected, unsafe state
 * - Solid Orange: Technician mode, safe state
 * - Blink Orange: Technician mode, unsafe state
 */

#ifndef LED_CONTROLLER_HPP
#define LED_CONTROLLER_HPP

#include <Arduino.h>

// ============================================================================
// CONFIGURATION
// ============================================================================

/** LED blink interval in milliseconds */
constexpr unsigned long LED_BLINK_INTERVAL_MS = 500;

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
