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

    // IMU2 Accelerometer (g) - mounted 180 deg rotated from IMU1
    float imu2X = 0;
    float imu2Y = 0;
    float imu2Z = 0;

    // IMU2 Gyroscope (deg/s)
    float imu2Gx = 0;
    float imu2Gy = 0;
    float imu2Gz = 0;

    // IMU2 validity
    bool imu2Valid = false;

    // Sanity checks ("not all zero, not stuck") - IMU1/IMU2 raw readings and
    // the calculated angle data
    bool imu1Sane = false;
    bool imu2Sane = false;
    bool angleSane = false;

    // GPS Position
    double gpsLat = 0;
    double gpsLng = 0;
    double gpsAlt = 0;

    // Last known-good GPS position (retained when the current fix is lost)
    double lastGpsLat = 0;
    double lastGpsLng = 0;
    double lastGpsAlt = 0;
    
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
    bool gpsSane = false;

    // Safety mode: true when both optocoupler safety inputs are active
    // (optos_status[0] && optos_status[1])
    bool safetyMode = false;

    // How long the system has been continuously unsafe (0 while safe;
    // resets to 0 as soon as safetyMode becomes true again, and naturally
    // resets on reboot since it's millis()-based)
    unsigned long safetyModeUnsafeDurationMs = 0;
    
    // LED states
    bool ledInternal = false;
    LED_STATES ledIo = OFF;
    
    // Button state
    bool button_tech = false;
    
    // Technician mode flag
    bool technicianMode = false;
    
    // Power Monitor (voltage/current sensor)
    bool powerConnected = false;
    bool powerSane = false;
    float busVoltage = 0.0;
    float busCurrent_mA = 0.0;
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

/** IMU2 X-axis offset (set during calibration) */
extern float g_imu2XOffset;

/** IMU2 Y-axis offset */
extern float g_imu2YOffset;

/** IMU2 Gyroscope X-axis offset */
extern float g_gyro2XOffset;

/** IMU2 Gyroscope Y-axis offset */
extern float g_gyro2YOffset;

/** IMU2 Gyroscope Z-axis offset */
extern float g_gyro2ZOffset;

// ============================================================================
// STATUS MANAGEMENT FUNCTIONS
// ============================================================================

/**
 * @brief Get the firmware version string
 * @return Firmware version (e.g. "1.5.0")
 */
const char* statusGetFirmwareVersion();

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
/**
 * @brief Milliseconds since statusUpdate() last read the sensors.
 *
 * Every get_status request refreshes the sensors itself, so while a consumer is
 * polling (the Jetson polls at 20 Hz) the periodic refresh in loop() is pure
 * duplicated I2C - and it blocks for up to ~41 ms, which showed up as the worst
 * stall those same consumers saw. Use this to skip it when it is not needed.
 */
unsigned long statusMsSinceRefresh();

JsonDocument statusGenerateJsonSimple();

/**
 * @brief Same document as statusGenerateJsonSimple(), but WITHOUT re-reading the
 *        sensors first.
 *
 * statusUpdate() does blocking I2C reads. The main loop already runs it once a
 * second, so the telemetry path can reuse that snapshot instead of paying for
 * its own - which keeps the telemetry send from delaying the 20 Hz HTTP API
 * that local consumers poll.
 */
JsonDocument statusGenerateJsonNoRefresh();

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
