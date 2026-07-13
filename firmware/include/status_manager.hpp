/**
 * @file status_manager.hpp
 * @brief Device Status Management Module
 * 
 * Handles:
 * - DeviceStatus structure definition
 * - Hardware status reading and updates
 * - Status JSON message generation
 * - Serial status output
 */

#ifndef STATUS_MANAGER_HPP
#define STATUS_MANAGER_HPP

#include <Arduino.h>
#include <ArduinoJson.h>
#include "gg_hal.hpp"

// ============================================================================
// LED STATE ENUMERATION
// ============================================================================

// LED_STATES is defined in gg_hal.hpp, so we just include it

// ============================================================================
// DEVICE STATUS STRUCTURE
// ============================================================================

/**
 * @brief Complete device status structure
 * 
 * Contains all sensor readings, I/O states, and device telemetry.
 */
struct DeviceStatus {
    // Relay states (4 relays)
    bool relays_status[4] = {false, false, false, false};
    
    // Opto-isolator input states (4 inputs)
    bool optos_status[4] = {false, false, false, false};
    
    // IMU Accelerometer (g)
    float imuX = 0;
    float imuY = 0;
    float imuZ = 0;
    
    // IMU Gyroscope (deg/s)
    float imuGx = 0;
    float imuGy = 0;
    float imuGz = 0;
    
    // Calculated orientation (degrees)
    float pitch = 0;
    float roll = 0;
    float yaw = 0;
    
    // IMU validity
    bool imuValid = false;
    
    // GPS Position
    double gpsLat = 0;
    double gpsLng = 0;
    double gpsAlt = 0;
    
    // GPS Time (YYYY-MM-DD hh:mm:ss)
    char gpsTime[20] = "";
    
    // GPS Velocity (km/hr)
    float gpsSpeedNorth = 0;
    float gpsSpeedEast = 0;
    float gpsSpeedDown = 0;
    float gpsGroundSpeed = 0;
    
    // GPS Heading (degrees)
    float gpsHeading = 0;
    
    // GPS Satellite count
    uint8_t gpsSatellites = 0;
    
    // GPS Accuracy (from UBX NAV-PVT)
    float gpsHAcc = 0;         // Horizontal accuracy estimate (mm)
    float gpsVAcc = 0;         // Vertical accuracy estimate (mm)
    double gpsAltEllipsoid = 0; // Height above WGS84 ellipsoid (mm)
    
    // GPS validity flags
    bool gpsValid = false;
    bool gpsConnected = false;
    
    // LED states
    bool ledInternal = false;
    LED_STATES ledIo = OFF;
    
    // Button state
    bool button_tech = false;
    
    // Technician mode flag
    bool technicianMode = false;
    
    // Power Monitor (voltage/current sensor)
    bool powerConnected = false;
    float busVoltage = 0.0;
};

// ============================================================================
// GLOBAL STATUS INSTANCE
// ============================================================================

/** Global device status (updated by statusUpdate()) */
extern DeviceStatus g_status;

// ============================================================================
// IMU CALIBRATION OFFSETS
// ============================================================================

/** IMU X-axis offset (set during calibration) */
extern float g_imuXOffset;

/** IMU Y-axis offset */
extern float g_imuYOffset;

/** Gyroscope X-axis offset */
extern float g_gyroXOffset;

/** Gyroscope Y-axis offset */
extern float g_gyroYOffset;

/** Gyroscope Z-axis offset */
extern float g_gyroZOffset;

// ============================================================================
// STATUS MANAGEMENT FUNCTIONS
// ============================================================================

/**
 * @brief Initialize status manager
 * 
 * Call in setup() after HAL initialization.
 */
void statusInit();

/**
 * @brief Update hardware status
 * 
 * Reads all sensors and updates g_status.
 * Call this periodically in loop() or before generating status response.
 */
void statusUpdate();

/**
 * @brief Generate status JSON document
 * @param requestDoc Optional request document (for including request metadata)
 * @return JsonDocument containing complete device status
 */
JsonDocument statusGenerateJson(JsonDocument* requestDoc = nullptr);

/**
 * @brief Generate status JSON without request document
 * @return JsonDocument containing complete device status
 * 
 * Overload for UDP broadcast and other cases where no request is provided.
 */
JsonDocument statusGenerateJsonSimple();

/**
 * @brief Write formatted status to Serial
 * 
 * Outputs human-readable status for debugging.
 */
void statusWriteToSerial();

/**
 * @brief Check if all devices are connected
 * @return true if GPS and IMU are both valid
 */
bool statusAllDevicesConnected();

#endif // STATUS_MANAGER_HPP
