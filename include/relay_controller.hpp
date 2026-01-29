/**
 * @file relay_controller.hpp
 * @brief Relay Auto-Reset Controller
 * 
 * Manages timed relay operations with automatic reset functionality.
 * Relays can be turned ON with an auto-reset timer that will automatically
 * turn them OFF after a specified duration.
 */

#ifndef RELAY_CONTROLLER_HPP
#define RELAY_CONTROLLER_HPP

#include <Arduino.h>

// ============================================================================
// CONFIGURATION
// ============================================================================

/** Default auto-reset duration in milliseconds */
constexpr unsigned long RELAY_AUTO_RESET_DEFAULT_MS = 2000;

/** Number of relays */
constexpr int RELAY_COUNT_MAX = 4;

// ============================================================================
// RELAY CONTROLLER FUNCTIONS
// ============================================================================

/**
 * @brief Initialize relay controller
 * 
 * Call in setup() after board initialization.
 */
void relayControllerInit();

/**
 * @brief Check and process any pending auto-resets
 * 
 * Call this in loop() to process relay auto-reset timers.
 */
void relayControllerUpdate();

/**
 * @brief Set relay with optional auto-reset
 * @param relayIndex Relay number (0-3)
 * @param state Relay state (true=ON, false=OFF)
 * @param autoResetMs Auto-reset delay in ms (0 = no auto-reset)
 * @return true if relay was set successfully
 */
bool relayControllerSet(int relayIndex, bool state, unsigned long autoResetMs = 0);

/**
 * @brief Cancel pending auto-reset for a relay
 * @param relayIndex Relay number (0-3)
 */
void relayControllerCancelAutoReset(int relayIndex);

/**
 * @brief Check if relay has a pending auto-reset
 * @param relayIndex Relay number (0-3)
 * @return true if auto-reset is pending
 */
bool relayControllerIsAutoResetPending(int relayIndex);

/**
 * @brief Get relay state
 * @param relayIndex Relay number (0-3)
 * @return Current relay state
 */
bool relayControllerGetState(int relayIndex);

#endif // RELAY_CONTROLLER_HPP
