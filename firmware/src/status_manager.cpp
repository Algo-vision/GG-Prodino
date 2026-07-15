/**
 * @file status_manager.cpp
 * @brief Device Status Management Implementation
 */

#include "status_manager.hpp"
#include "config_manager.hpp"
#include "KMPProDinoMKRZero.h"
#include "KMPCommon.h"
#include <i2c_imu_gps.hpp>
#include <imu_mount_orientation.hpp>
#include <sanity_check.hpp>
#include "calculations.hpp"
#include <Ethernet.h>
#include <Adafruit_INA219.h>

// ============================================================================
// FIRMWARE VERSION (configurable constant)
// ============================================================================

#ifndef FIRMWARE_VERSION
#define FIRMWARE_VERSION "1.5.0"
#endif

// ============================================================================
// GLOBAL STATUS INSTANCE
// ============================================================================

DeviceStatus g_status;

// ============================================================================
// IMU CALIBRATION OFFSETS
// ============================================================================

float g_imuXOffset = 0.0f;
float g_imuYOffset = 0.0f;
float g_gyroXOffset = 0.0f;
float g_gyroYOffset = 0.0f;
float g_gyroZOffset = 0.0f;

float g_imu2XOffset = 0.0f;
float g_imu2YOffset = 0.0f;
float g_gyro2XOffset = 0.0f;
float g_gyro2YOffset = 0.0f;
float g_gyro2ZOffset = 0.0f;

// ============================================================================
// INTERNAL STATE
// ============================================================================

/** Last update timestamp for dt calculation */
static unsigned long s_lastUpdateTime = 0;

/** Previous GPS altitude for vertical speed calculation */
static double s_previousGpsAltitude = 0.0;

/** Previous GPS time for vertical speed calculation */
static unsigned long s_previousGpsTime = 0;

/** Filtered GPS vertical speed (low-pass filtered) */
static float s_filteredGpsSpeedDown = 0.0f;

/** Smoothing factor for vertical speed filter (0.0 to 1.0) */
static const float SPEED_DOWN_FILTER_ALPHA = 0.2f;

/** Flag indicating all devices are connected */
static bool s_allDevicesConnected = false;

/** Power monitor instance */
static Adafruit_INA219 s_powerMonitor;

/** Power monitor connection status */
static bool s_powerMonitorConnected = false;

// ============================================================================
// SANITY-CHECK STATE ("not all zero, not stuck" - see statusUpdate())
// ============================================================================

/** Consecutive identical-reading counts, per sensor group */
static uint8_t s_powerStuckCount = 0;
static uint8_t s_imu1StuckCount = 0;
static uint8_t s_imu2StuckCount = 0;
static uint8_t s_angleStuckCount = 0;
static uint8_t s_gpsStuckCount = 0;

/** Previous readings, per sensor group, for stuck-value comparison */
static float s_prevPower[2] = {0, 0};
static float s_prevImu1[6] = {0, 0, 0, 0, 0, 0};
static float s_prevImu2[6] = {0, 0, 0, 0, 0, 0};
static float s_prevAngle[3] = {0, 0, 0};
static double s_prevGps[3] = {0, 0, 0};

/** N consecutive identical readings are treated as a frozen/stuck sensor */
static const uint8_t SANITY_STUCK_THRESHOLD = 3;

/** Timestamp of the last safe->unsafe transition (0 while safe) */
static unsigned long s_unsafeSinceMs = 0;

// External reference to technician_mode (defined in main.cpp)
extern bool technician_mode;

// External reference to GG_HAL instance (defined in main.cpp)
extern GG_HAL _gg_hal;

// ============================================================================
// IMPLEMENTATION
// ============================================================================

const char* statusGetFirmwareVersion() {
    return FIRMWARE_VERSION;
}

void statusInit() {
    s_lastUpdateTime = millis();
    s_previousGpsAltitude = 0.0;
    s_previousGpsTime = 0;
    s_filteredGpsSpeedDown = 0.0f;
    s_allDevicesConnected = false;
    
    // Initialize power monitor
    s_powerMonitorConnected = s_powerMonitor.begin();
    if (s_powerMonitorConnected) {
        Serial.println("Power monitor initialized");
    } else {
        Serial.println("Power monitor not found - power monitoring disabled");
    }
    
    Serial.println("Status manager initialized");
}

void statusUpdate() {
    // Read IMU1 Accelerometer
    float ax, ay, az;
    bool imuValid = readAccelerometer(ax, ay, az);
    applyMountOrientationRemap(ax, ay, az, g_imuMountOrientation);
    g_status.imuX = ax;
    g_status.imuY = ay;
    g_status.imuZ = az;

    // Read IMU1 Gyroscope
    float gx, gy, gz;
    _gg_hal.get_gyro_data(gx, gy, gz);
    applyMountOrientationRemap(gx, gy, gz, g_imuMountOrientation);
    g_status.imuGx = gx - g_gyroXOffset;
    g_status.imuGy = gy - g_gyroYOffset;
    g_status.imuGz = gz - g_gyroZOffset;

    g_status.imuValid = imuValid;

    // Read IMU2 Accelerometer
    float ax2, ay2, az2;
    bool imu2Valid = readAccelerometer_2(ax2, ay2, az2);
    applyMountOrientationRemap(ax2, ay2, az2, g_imuMountOrientation);
    g_status.imu2X = ax2;
    g_status.imu2Y = ay2;
    g_status.imu2Z = az2;

    // Read IMU2 Gyroscope
    float gx2, gy2, gz2;
    _gg_hal.get_gyro_data_2(gx2, gy2, gz2);
    applyMountOrientationRemap(gx2, gy2, gz2, g_imuMountOrientation);
    g_status.imu2Gx = gx2 - g_gyro2XOffset;
    g_status.imu2Gy = gy2 - g_gyro2YOffset;
    g_status.imu2Gz = gz2 - g_gyro2ZOffset;

    g_status.imu2Valid = imu2Valid;

    // Calculate time delta
    unsigned long currentTime = millis();
    float dt = (currentTime - s_lastUpdateTime) / 1000.0f;
    s_lastUpdateTime = currentTime;

    // Calculate orientation from whichever IMU(s) are valid
    if (g_status.imuValid && g_status.imu2Valid) {
        // Both valid - fuse for a less noisy estimate
        calculateMergedOrientation(g_status.pitch, g_status.roll, g_status.yaw,
                          g_status.imuX, g_status.imuY, g_status.imuZ,
                          g_status.imuGx, g_status.imuGy, g_status.imuGz,
                          g_status.imu2X, g_status.imu2Y, g_status.imu2Z,
                          g_status.imu2Gx, g_status.imu2Gy, g_status.imu2Gz, dt,
                          g_imuXOffset, g_imuYOffset,
                          g_imu2XOffset, g_imu2YOffset);
    } else if (g_status.imuValid) {
        // Only IMU1 valid
        calculateOrientation_1(g_status.pitch, g_status.roll, g_status.yaw,
                          g_status.imuX, g_status.imuY, g_status.imuZ,
                          g_status.imuGx, g_status.imuGy, g_status.imuGz, dt,
                          g_imuXOffset, g_imuYOffset);
    } else if (g_status.imu2Valid) {
        // Only IMU2 valid
        calculateOrientation_2(g_status.pitch, g_status.roll, g_status.yaw,
                          g_status.imu2X, g_status.imu2Y, g_status.imu2Z,
                          g_status.imu2Gx, g_status.imu2Gy, g_status.imu2Gz, dt,
                          g_imu2XOffset, g_imu2YOffset);
    }
    // else: neither IMU valid - leave pitch/roll/yaw at their last known values

    // Read GPS data
    gps_data currentGpsData;
    _gg_hal.get_gps_data(currentGpsData);
    g_status.gpsValid = currentGpsData.valid;
    g_status.gpsConnected = gps_conncted;  // Global from i2c_imu_gps.cpp
    g_status.gpsSatellites = currentGpsData.satellites;
    g_status.gpsHAcc = currentGpsData.hAcc;
    g_status.gpsVAcc = currentGpsData.vAcc;
    g_status.gpsAltEllipsoid = currentGpsData.altEllipsoid;
    
    if (g_status.gpsValid) {
        g_status.gpsLat = currentGpsData.latitude;
        g_status.gpsLng = currentGpsData.longitude;
        g_status.gpsAlt = currentGpsData.altitude;

        // Retain last known-good position separately - gpsLat/Lng/Alt get
        // cleared below when the fix is lost, these don't.
        g_status.lastGpsLat = currentGpsData.latitude;
        g_status.lastGpsLng = currentGpsData.longitude;
        g_status.lastGpsAlt = currentGpsData.altitude;


        // Calculate vertical speed from GPS altitude changes
        if (s_previousGpsTime > 0 && dt > 0.0f) {
            float verticalSpeedMs = (s_previousGpsAltitude - currentGpsData.altitude) / dt;
            float currentSpeedDown = verticalSpeedMs * 3.6f;  // Convert m/s to km/hr
            
            // Apply EMA filter
            s_filteredGpsSpeedDown = (SPEED_DOWN_FILTER_ALPHA * currentSpeedDown) + 
                                     ((1.0f - SPEED_DOWN_FILTER_ALPHA) * s_filteredGpsSpeedDown);
            g_status.gpsSpeedDown = s_filteredGpsSpeedDown;
        } else {
            g_status.gpsSpeedDown = 0.0f;
            s_filteredGpsSpeedDown = 0.0f;
        }
        
        s_previousGpsAltitude = currentGpsData.altitude;
        s_previousGpsTime = currentTime;
        
        strcpy(g_status.gpsTime, currentGpsData.time_str);
        g_status.gpsSpeedNorth = currentGpsData.speed_north;
        g_status.gpsSpeedEast = currentGpsData.speed_east;
        g_status.gpsGroundSpeed = currentGpsData.ground_speed;
        g_status.gpsHeading = currentGpsData.heading;
    } else {
        // Clear GPS data if not valid
        g_status.gpsLat = 0;
        g_status.gpsLng = 0;
        g_status.gpsAlt = 0;
        strcpy(g_status.gpsTime, "");
        g_status.gpsSpeedNorth = 0;
        g_status.gpsSpeedEast = 0;
        g_status.gpsSpeedDown = 0;
        g_status.gpsGroundSpeed = 0;
        g_status.gpsHeading = 0;
        g_status.gpsHAcc = 0;
        g_status.gpsVAcc = 0;
        g_status.gpsAltEllipsoid = 0;
        s_previousGpsAltitude = 0.0;
        s_previousGpsTime = 0;
    }
    
    // Read button and LED states
    g_status.button_tech = _gg_hal.get_button_tech_state();
    g_status.ledIo = _gg_hal.get_indicator_led_state();

    // Update all devices connected flag
    s_allDevicesConnected = gps_conncted && imuValid;

    // Read relay states
    for (uint8_t i = 0; i < RELAY_COUNT; i++) {
        g_status.relays_status[i] = KMPProDinoMKRZero.GetRelayState(i);
    }

    // Read opto-isolator inputs
    for (uint8_t i = 0; i < OPTOIN_COUNT; i++) {
        g_status.optos_status[i] = _gg_hal.get_optoin_state(i);
    }

    // Safety mode: both optocoupler safety inputs must be active
    g_status.safetyMode = g_status.optos_status[0] && g_status.optos_status[1];
    if (g_status.safetyMode) {
        s_unsafeSinceMs = 0;
    } else if (s_unsafeSinceMs == 0) {
        s_unsafeSinceMs = currentTime;
    }
    g_status.safetyModeUnsafeDurationMs = (s_unsafeSinceMs == 0) ? 0 : (currentTime - s_unsafeSinceMs);

    // Update technician mode flag
    if (technician_mode) {
        g_status.technicianMode = true;
    }

    // Read power monitor
    g_status.powerConnected = s_powerMonitorConnected;
    if (s_powerMonitorConnected) {
        g_status.busVoltage = s_powerMonitor.getBusVoltage_V();
        g_status.busCurrent_mA = s_powerMonitor.getCurrent_mA();
    } else {
        g_status.busVoltage = -1.0f;  // Indicate error
        g_status.busCurrent_mA = -1.0f;
    }

    // ------------------------------------------------------------------
    // Sanity checks: not all zero, not stuck at the same value for
    // several consecutive reads. Power monitor additionally requires no
    // negative values (voltage/current are never legitimately negative
    // on this hardware) - that check is skipped for IMU/angle/GPS since
    // negative values are physically normal for those signed quantities.
    // ------------------------------------------------------------------

    // Power monitor
    float powerNow[2] = {g_status.busVoltage, g_status.busCurrent_mA};
    bool powerSaneCheck = checkSane(powerNow, s_prevPower, 2, s_powerStuckCount,
                                     SANITY_STUCK_THRESHOLD, /*requireNonNegative=*/true);
    g_status.powerSane = s_powerMonitorConnected && powerSaneCheck;

    // IMU1 raw readings (accel + gyro)
    float imu1Now[6] = {g_status.imuX, g_status.imuY, g_status.imuZ,
                         g_status.imuGx, g_status.imuGy, g_status.imuGz};
    bool imu1SaneCheck = checkSane(imu1Now, s_prevImu1, 6, s_imu1StuckCount, SANITY_STUCK_THRESHOLD);
    g_status.imu1Sane = g_status.imuValid && imu1SaneCheck;

    // IMU2 raw readings (accel + gyro)
    float imu2Now[6] = {g_status.imu2X, g_status.imu2Y, g_status.imu2Z,
                         g_status.imu2Gx, g_status.imu2Gy, g_status.imu2Gz};
    bool imu2SaneCheck = checkSane(imu2Now, s_prevImu2, 6, s_imu2StuckCount, SANITY_STUCK_THRESHOLD);
    g_status.imu2Sane = g_status.imu2Valid && imu2SaneCheck;

    // Calculated angle data (pitch/roll/yaw)
    float angleNow[3] = {g_status.pitch, g_status.roll, g_status.yaw};
    bool angleSaneCheck = checkSane(angleNow, s_prevAngle, 3, s_angleStuckCount, SANITY_STUCK_THRESHOLD);
    g_status.angleSane = (g_status.imuValid || g_status.imu2Valid) && angleSaneCheck;

    // GPS position (double precision - lat/lng need it)
    double gpsNow[3] = {g_status.gpsLat, g_status.gpsLng, g_status.gpsAlt};
    bool gpsSaneCheck = checkSane(gpsNow, s_prevGps, 3, s_gpsStuckCount, SANITY_STUCK_THRESHOLD);
    g_status.gpsSane = g_status.gpsValid && gpsSaneCheck;
}

JsonDocument statusGenerateJson(JsonDocument* requestDoc) {
    // Update hardware status first
    statusUpdate();

    JsonDocument resp;
    resp["type"] = "status";
    resp["firmwareVersion"] = FIRMWARE_VERSION;
    
    // Relay states array
    JsonArray relaysArray = resp["relays_status"].to<JsonArray>();
    for (uint8_t i = 0; i < RELAY_COUNT; i++) {
        relaysArray.add(KMPProDinoMKRZero.GetRelayState(i));
    }
    
    // IMU data
    resp["imuX"] = g_status.imuX;
    resp["imuY"] = g_status.imuY;
    resp["imuZ"] = g_status.imuZ;
    resp["imuGx"] = g_status.imuGx;
    resp["imuGy"] = g_status.imuGy;
    resp["imuGz"] = g_status.imuGz;
    resp["pitch"] = g_status.pitch;
    resp["roll"] = g_status.roll;
    resp["yaw"] = g_status.yaw;

    // IMU2 data (mounted 180 deg rotated from IMU1)
    resp["imu2X"] = g_status.imu2X;
    resp["imu2Y"] = g_status.imu2Y;
    resp["imu2Z"] = g_status.imu2Z;
    resp["imu2Gx"] = g_status.imu2Gx;
    resp["imu2Gy"] = g_status.imu2Gy;
    resp["imu2Gz"] = g_status.imu2Gz;
    resp["imu2Valid"] = g_status.imu2Valid;

    // GPS data
    resp["gpsLat"] = g_status.gpsLat;
    resp["gpsLng"] = g_status.gpsLng;
    resp["gpsAlt"] = g_status.gpsAlt;
    resp["gpsTime"] = g_status.gpsTime;
    resp["gpsSpeedNorth"] = g_status.gpsSpeedNorth;
    resp["gpsSpeedEast"] = g_status.gpsSpeedEast;
    resp["gpsSpeedDown"] = g_status.gpsSpeedDown;
    resp["gpsGroundSpeed"] = g_status.gpsGroundSpeed;
    resp["gpsHeading"] = g_status.gpsHeading;
    
    // LED state
    resp["ledInternal"] = g_status.ledInternal;
    switch (g_status.ledIo) {
        case OFF:    resp["ledIo"] = "OFF";    break;
        case GREEN:  resp["ledIo"] = "GREEN";  break;
        case RED:    resp["ledIo"] = "RED";    break;
        case ORANGE: resp["ledIo"] = "ORANGE"; break;
        default:     resp["ledIo"] = "OFF";    break;
    }
    
    // Validity and connectivity
    resp["gpsValid"] = g_status.gpsValid;
    resp["button_tech"] = g_status.button_tech;
    resp["imuValid"] = g_status.imuValid;
    resp["GPSConnected"] = g_status.gpsConnected;
    resp["gpsSatellites"] = g_status.gpsSatellites;
    resp["gpsHAcc"] = g_status.gpsHAcc;
    resp["gpsVAcc"] = g_status.gpsVAcc;
    resp["gpsAltEllipsoid"] = g_status.gpsAltEllipsoid;
    resp["technicianMode"] = g_status.technicianMode;
    
    // Opto inputs array
    JsonArray optosArray = resp["optoin_status"].to<JsonArray>();
    for (uint8_t i = 0; i < OPTOIN_COUNT; i++) {
        optosArray.add(g_status.optos_status[i]);
    }
    
    // IP configuration
    resp["controllerIp"] = g_controllerIP.toString();
    resp["routerIp"] = g_routerIP.toString();
    
    JsonArray whitelistArray = resp["whitelistIps"].to<JsonArray>();
    for (int i = 0; i < g_whitelistCount; ++i) {
        whitelistArray.add(g_whitelist[i].toString());
    }
    
    // Motor work hours
    resp["motorWorkHours"] = round(g_motorWorkSeconds / 3600.0 * 100) / 100.0;  // 2 decimal places
    resp["motorWorkSeconds"] = g_motorWorkSeconds;
    
    // Power monitor
    resp["powerConnected"] = g_status.powerConnected;
    resp["busVoltage"] = g_status.busVoltage;
    
    return resp;
}

JsonDocument statusGenerateJsonSimple() {
    return statusGenerateJson(nullptr);
}

void statusWriteToSerial() {
    Serial.print("Relays: ");
    for (uint8_t i = 0; i < RELAY_COUNT; i++) {
        Serial.print(KMPProDinoMKRZero.GetRelayState(i) ? "1" : "0");
        if (i < RELAY_COUNT - 1) Serial.print(", ");
    }
    
    Serial.print("OptoIn: ");
    for (uint8_t i = 0; i < RELAY_COUNT; i++) {
        Serial.print(g_status.optos_status[i] ? "1" : "0");
        if (i < RELAY_COUNT - 1) Serial.print(", ");
    }
    
    Serial.print(" | IMU Accel: ");
    Serial.print(g_status.imuX, 2);
    Serial.print(", ");
    Serial.print(g_status.imuY, 2);
    Serial.print(", ");
    Serial.print(g_status.imuZ, 2);
    
    Serial.print(" | IMU Gyro: ");
    Serial.print(g_status.imuGx, 2);
    Serial.print(", ");
    Serial.print(g_status.imuGy, 2);
    Serial.print(", ");
    Serial.print(g_status.imuGz, 2);
    
    Serial.print(" | Pitch: ");
    Serial.print(g_status.pitch, 2);
    Serial.print(" | Roll: ");
    Serial.print(g_status.roll, 2);
    Serial.print(" | Yaw: ");
    Serial.print(g_status.yaw, 2);
    
    Serial.print(" | IMU Valid: ");
    Serial.print(g_status.imuValid ? "Yes" : "No");

    Serial.print(" | IMU2 Accel: ");
    Serial.print(g_status.imu2X, 2);
    Serial.print(", ");
    Serial.print(g_status.imu2Y, 2);
    Serial.print(", ");
    Serial.print(g_status.imu2Z, 2);

    Serial.print(" | IMU2 Gyro: ");
    Serial.print(g_status.imu2Gx, 2);
    Serial.print(", ");
    Serial.print(g_status.imu2Gy, 2);
    Serial.print(", ");
    Serial.print(g_status.imu2Gz, 2);

    Serial.print(" | IMU2 Valid: ");
    Serial.print(g_status.imu2Valid ? "Yes" : "No");

    Serial.print(" | GPS: ");
    if (g_status.gpsValid) {
        Serial.print(g_status.gpsLat, 6);
        Serial.print(", ");
        Serial.print(g_status.gpsLng, 6);
        Serial.print(", ");
        Serial.print(g_status.gpsAlt, 2);
        Serial.print(" | Time: ");
        Serial.print(g_status.gpsTime);
        Serial.print(" | Spd N/E/D: ");
        Serial.print(g_status.gpsSpeedNorth, 2);
        Serial.print(", ");
        Serial.print(g_status.gpsSpeedEast, 2);
        Serial.print(", ");
        Serial.print(g_status.gpsSpeedDown, 2);
        Serial.print(" | Gnd Spd: ");
        Serial.print(g_status.gpsGroundSpeed, 2);
        Serial.print(" | Heading: ");
        Serial.print(g_status.gpsHeading, 2);
    } else {
        Serial.print("No fix");
    }
    
    Serial.print(" | GPS Connected: ");
    Serial.print(g_status.gpsConnected ? "Yes" : "No");
    Serial.print(" | button_tech: ");
    Serial.print(g_status.button_tech ? "Pressed" : "Released");
    Serial.print(" | LED Internal: ");
    Serial.print(g_status.ledInternal ? "ON" : "OFF");
    Serial.print(" | LED IO: ");
    
    switch (g_status.ledIo) {
        case OFF:    Serial.print("OFF");    break;
        case GREEN:  Serial.print("GREEN");  break;
        case RED:    Serial.print("RED");    break;
        case ORANGE: Serial.print("ORANGE"); break;
        default:     Serial.print("OFF");    break;
    }
    
    Serial.print(" | IP: ");
    Serial.print(Ethernet.localIP());
    Serial.print(" | Power: ");
    Serial.print(g_status.powerConnected ? "Yes" : "No");
    if (g_status.powerConnected) {
        Serial.print(" | Bus V: ");
        Serial.print(g_status.busVoltage, 2);
        Serial.print("V");
    }
    Serial.println();
    Serial.println();
}

bool statusAllDevicesConnected() {
    return s_allDevicesConnected;
}
