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
#include "ocu_monitor.hpp"
#include <zero_calibration.hpp>
#include <rest_detector.hpp>
#include <gyro_bias.hpp>

// ============================================================================
// FIRMWARE VERSION (configurable constant)
// ============================================================================

#ifndef FIRMWARE_VERSION
#define FIRMWARE_VERSION "1.5.1.1"
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

/** Mounting transforms, rebuilt whenever a calibration is burned. Identity
 *  until one is - an uncalibrated controller reports the sensor frame rather
 *  than a guess. */
static float s_R1[9];
static float s_R2[9];

static RestDetector s_rest;
static GyroBias s_bias1;
static GyroBias s_bias2;

/** Most recent bias-corrected, mount-corrected accelerometer vectors. The
 *  calibration commands need the same numbers the filter just used, and
 *  re-reading the sensor from an HTTP handler would both cost I2C time and
 *  break the rule that only loop() touches the sensors. */
static float s_a1[3] = {0.0f, 0.0f, 0.0f};
static float s_a2[3] = {0.0f, 0.0f, 0.0f};
static bool  s_a1Valid = false;
static bool  s_a2Valid = false;

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

/** Minimum ground speed (km/hr) for the GPS course to be a usable heading.
 *  Below this the vehicle is effectively stationary and course-over-ground is
 *  noise, not a bearing. */
static const float GPS_HEADING_MIN_SPEED_KMH = 3.0f;

/** True once yaw has been snapped to an absolute GPS bearing at least once */
static bool s_yawAlignedToGps = false;

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

/** Rebuild both mounting transforms from whatever is currently burned. */
static void rebuildZeroCal() {
    if (g_zeroCalValid) {
        if (!zeroCalBuildRotation(g_imu1ZeroG, s_R1)) zeroCalIdentity(s_R1);
        if (!zeroCalBuildRotation(g_imu2ZeroG, s_R2)) zeroCalIdentity(s_R2);
    } else {
        zeroCalIdentity(s_R1);
        zeroCalIdentity(s_R2);
    }
}

void statusInit() {
    s_lastUpdateTime = millis();

    rebuildZeroCal();
    restDetectorReset(s_rest);

    // Seed the bias from flash rather than measuring it here. setup() cannot
    // know whether the machine is standing still - see gyro_bias.hpp.
    gyroBiasReset(s_bias1);
    gyroBiasReset(s_bias2);
    if (g_gyroBiasValid) {
        s_bias1.x = g_imu1GyroBias[0]; s_bias1.y = g_imu1GyroBias[1]; s_bias1.z = g_imu1GyroBias[2];
        s_bias2.x = g_imu2GyroBias[0]; s_bias2.y = g_imu2GyroBias[1]; s_bias2.z = g_imu2GyroBias[2];
        s_bias1.valid = s_bias2.valid = true;
    }
    s_previousGpsAltitude = 0.0;
    s_previousGpsTime = 0;
    s_filteredGpsSpeedDown = 0.0f;
    s_allDevicesConnected = false;
    s_yawAlignedToGps = false;
    
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
    applyImuAxisMap(ax, ay, az, g_imuAxisMap);
    g_status.imuX = ax;
    g_status.imuY = ay;
    g_status.imuZ = az;

    // Read IMU1 Gyroscope
    float gx, gy, gz;
    _gg_hal.get_gyro_data(gx, gy, gz);
    applyImuAxisMap(gx, gy, gz, g_imuAxisMap);
    g_status.imuGx = gx - s_bias1.x;
    g_status.imuGy = gy - s_bias1.y;
    g_status.imuGz = gz - s_bias1.z;

    g_status.imuValid = imuValid;

    // Read IMU2 Accelerometer
    float ax2, ay2, az2;
    bool imu2Valid = readAccelerometer_2(ax2, ay2, az2);
    applyImuAxisMap(ax2, ay2, az2, g_imuAxisMap);
    g_status.imu2X = ax2;
    g_status.imu2Y = ay2;
    g_status.imu2Z = az2;

    // Read IMU2 Gyroscope
    float gx2, gy2, gz2;
    _gg_hal.get_gyro_data_2(gx2, gy2, gz2);
    applyImuAxisMap(gx2, gy2, gz2, g_imuAxisMap);
    g_status.imu2Gx = gx2 - s_bias2.x;
    g_status.imu2Gy = gy2 - s_bias2.y;
    g_status.imu2Gz = gz2 - s_bias2.z;

    g_status.imu2Valid = imu2Valid;

    // Read on-chip die temperature, averaged over whichever IMUs answer. Used
    // to interpret gyro bias drift (0.05 dps/degC) - see imuLimitations.md.
    float temp1 = 0.0f, temp2 = 0.0f;
    bool temp1Ok = readImuTemperature(temp1);
    bool temp2Ok = readImuTemperature_2(temp2);
    if (temp1Ok && temp2Ok) {
        g_status.imuTemp = (temp1 + temp2) * 0.5f;
    } else if (temp1Ok) {
        g_status.imuTemp = temp1;
    } else if (temp2Ok) {
        g_status.imuTemp = temp2;
    }
    // else: neither readable - retain the last known value


    // Calculate time delta, clamped so a stalled main loop cannot integrate one
    // instantaneous gyro sample across a long gap (see STATUS_UPDATE_MAX_DT_S).
    unsigned long currentTime = millis();
    float dt = (currentTime - s_lastUpdateTime) / 1000.0f;
    s_lastUpdateTime = currentTime;
    if (dt > STATUS_UPDATE_MAX_DT_S) {
        dt = STATUS_UPDATE_MAX_DT_S;
    }

    // ------------------------------------------------------------------
    // Is the machine standing still, and if it has been for long enough,
    // what does the gyro read when it should read nothing?
    //
    // Fed from whichever IMU is answering. Accelerometer MAGNITUDE is what
    // the test uses and rotation cannot change a magnitude, so the mounting
    // transform is irrelevant here and the sensor-frame values are used.
    // ------------------------------------------------------------------
    if (imuValid) {
        restDetectorUpdate(s_rest, dt,
                           g_status.imuX, g_status.imuY, g_status.imuZ,
                           g_status.imuGx, g_status.imuGy, g_status.imuGz);
    } else if (imu2Valid) {
        restDetectorUpdate(s_rest, dt,
                           g_status.imu2X, g_status.imu2Y, g_status.imu2Z,
                           g_status.imu2Gx, g_status.imu2Gy, g_status.imu2Gz);
    } else {
        restDetectorReset(s_rest);
    }

    if (s_rest.stillSeconds >= REST_SECONDS_FOR_BIAS) {
        // RAW rates, deliberately. Feeding back the corrected ones would
        // drive the estimate to zero and undo the very correction it exists
        // to provide.
        bool moved = false;
        if (imuValid)  moved |= gyroBiasUpdate(s_bias1, gx,  gy,  gz,  dt);
        if (imu2Valid) moved |= gyroBiasUpdate(s_bias2, gx2, gy2, gz2, dt);
        if (moved) {
            // Mirrored into the legacy globals so anything still reading them
            // sees the live value, and handed to config so the periodic save
            // carries it across the next power cycle.
            g_gyroXOffset = s_bias1.x; g_gyroYOffset = s_bias1.y; g_gyroZOffset = s_bias1.z;
            g_gyro2XOffset = s_bias2.x; g_gyro2YOffset = s_bias2.y; g_gyro2ZOffset = s_bias2.z;
            const float b1[3] = {s_bias1.x, s_bias1.y, s_bias1.z};
            const float b2[3] = {s_bias2.x, s_bias2.y, s_bias2.z};
            configStoreGyroBias(b1, b2);
        }
    }

    // ------------------------------------------------------------------
    // Decide whether the GPS heading can be used to correct yaw this cycle.
    //
    // These are LAST cycle's GPS values - this cycle's fix is read further
    // down, since that block needs the dt computed just above. That lag is
    // harmless: the GPS produces a new fix a few times a second while
    // statusUpdate() runs far more often, so the heading is inherently older
    // than the gyro sample regardless.
    //
    // Course over ground is only meaningful while actually moving. Standing
    // still it is noise, and letting it correct yaw then would make a
    // stationary board slowly rotate to a random bearing.
    // ------------------------------------------------------------------
    bool gpsHeadingValid = g_status.gpsValid &&
                           (g_status.gpsGroundSpeed >= GPS_HEADING_MIN_SPEED_KMH);
    float gpsHeading = g_status.gpsHeading;

    // On the first usable heading (boot, or after a long fix loss) snap yaw
    // straight to it instead of letting the filter walk there over ~15s -
    // the gyro's starting yaw is an arbitrary reference, not an estimate
    // worth preserving.
    if (gpsHeadingValid && !s_yawAlignedToGps) {
        g_status.yaw = wrapTo180(gpsHeading);
        s_yawAlignedToGps = true;
    }

    // ------------------------------------------------------------------
    // Per-IMU sanity, computed BEFORE the orientation filter because it
    // decides which IMU(s) feed it.
    //
    // This is a LONG-RUN health signal only: an IMU is unsane when it stops
    // responding, reads all-zero, or freezes at an identical value for
    // SANITY_STUCK_THRESHOLD consecutive samples. It is deliberately NOT
    // affected by transient vehicle dynamics, so a hard manoeuvre can never
    // momentarily flip it false.
    // ------------------------------------------------------------------
    float imu1Now[6] = {g_status.imuX, g_status.imuY, g_status.imuZ,
                        g_status.imuGx, g_status.imuGy, g_status.imuGz};
    bool imu1Health = imuValid && checkSane(imu1Now, s_prevImu1, 6, s_imu1StuckCount,
                                            SANITY_STUCK_THRESHOLD);

    float imu2Now[6] = {g_status.imu2X, g_status.imu2Y, g_status.imu2Z,
                        g_status.imu2Gx, g_status.imu2Gy, g_status.imu2Gz};
    bool imu2Health = imu2Valid && checkSane(imu2Now, s_prevImu2, 6, s_imu2StuckCount,
                                             SANITY_STUCK_THRESHOLD);

    g_status.imu1Sane = imu1Health;
    g_status.imu2Sane = imu2Health;

    // Select which IMU(s) drive the orientation filter, by sanity.
    bool useImu1 = g_status.imu1Sane;
    bool useImu2 = g_status.imu2Sane;

    // ------------------------------------------------------------------
    // Into the machine's frame before anything is computed from it.
    //
    // Both vectors are rotated, not just gravity: a sensor tilted on its
    // bracket has its rotation axes tilted by exactly the same amount, so a
    // gyro rate about the sensor's X is not a rate about the machine's.
    //
    // The offsets passed to the filters below are now zero. The correction
    // has already been applied here, and correctly - see zero_calibration.hpp
    // for why subtracting a constant from ax/ay was only ever right at the
    // attitude it was captured in.
    // ------------------------------------------------------------------
    float a1x = g_status.imuX,  a1y = g_status.imuY,  a1z = g_status.imuZ;
    float w1x = g_status.imuGx, w1y = g_status.imuGy, w1z = g_status.imuGz;
    zeroCalApply(s_R1, a1x, a1y, a1z);
    zeroCalApply(s_R1, w1x, w1y, w1z);

    float a2x = g_status.imu2X,  a2y = g_status.imu2Y,  a2z = g_status.imu2Z;
    float w2x = g_status.imu2Gx, w2y = g_status.imu2Gy, w2z = g_status.imu2Gz;
    zeroCalApply(s_R2, a2x, a2y, a2z);
    zeroCalApply(s_R2, w2x, w2y, w2z);

    s_a1[0] = a1x; s_a1[1] = a1y; s_a1[2] = a1z; s_a1Valid = imuValid;
    s_a2[0] = a2x; s_a2[1] = a2y; s_a2[2] = a2z; s_a2Valid = imu2Valid;

    if (useImu1 && useImu2) {
        // Both sane - fuse for a less noisy estimate
        calculateMergedOrientation(g_status.pitch, g_status.roll, g_status.yaw,
                          a1x, a1y, a1z, w1x, w1y, w1z,
                          a2x, a2y, a2z, w2x, w2y, w2z, dt,
                          0.0f, 0.0f, 0.0f, 0.0f,
                          gpsHeadingValid, gpsHeading);
    } else if (useImu1) {
        // Only IMU1 usable
        calculateOrientation_1(g_status.pitch, g_status.roll, g_status.yaw,
                          a1x, a1y, a1z, w1x, w1y, w1z, dt,
                          0.0f, 0.0f,
                          gpsHeadingValid, gpsHeading);
    } else if (useImu2) {
        // Only IMU2 usable
        calculateOrientation_2(g_status.pitch, g_status.roll, g_status.yaw,
                          a2x, a2y, a2z, w2x, w2y, w2z, dt,
                          0.0f, 0.0f,
                          gpsHeadingValid, gpsHeading);
    }
    // else: neither IMU healthy - leave pitch/roll/yaw at their last known values

    // Read GPS data. Timed on its own: scenarios A and B showed the
    // unattributed part of statusUpdate GROWING per call as the loop
    // slowed (20.68 -> 33.57 ms), which is what draining a queue looks
    // like, and this is the only queue in here.
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
        g_status.systemVoltage = s_powerMonitor.getBusVoltage_V();
        // The INA219 library reports milliamps; the API exposes amps.
        g_status.systemCurrent_A = s_powerMonitor.getCurrent_mA() / 1000.0f;
    } else {
        g_status.systemVoltage = -1.0f;  // Indicate error
        g_status.systemCurrent_A = -1.0f;
    }

    // ------------------------------------------------------------------
    // Sanity checks: not all zero, not stuck at the same value for
    // several consecutive reads. Power monitor additionally requires no
    // negative values (voltage/current are never legitimately negative
    // on this hardware) - that check is skipped for IMU/angle/GPS since
    // negative values are physically normal for those signed quantities.
    // ------------------------------------------------------------------


    // Power monitor
    float powerNow[2] = {g_status.systemVoltage, g_status.systemCurrent_A};
    bool powerSaneCheck = checkSane(powerNow, s_prevPower, 2, s_powerStuckCount,
                                     SANITY_STUCK_THRESHOLD, /*requireNonNegative=*/true);
    g_status.powerSane = s_powerMonitorConnected && powerSaneCheck;

    // NOTE: imu1Sane / imu2Sane are computed earlier in this function, before
    // the orientation filter, because they now select which IMU(s) feed it.

    // Calculated angle data (pitch/roll/yaw)
    float angleNow[3] = {g_status.pitch, g_status.roll, g_status.yaw};
    bool angleSaneCheck = checkSane(angleNow, s_prevAngle, 3, s_angleStuckCount, SANITY_STUCK_THRESHOLD);
    g_status.angleSane = (g_status.imu1Sane || g_status.imu2Sane) && angleSaneCheck;

    // GPS position (double precision - lat/lng need it)
    double gpsNow[3] = {g_status.gpsLat, g_status.gpsLng, g_status.gpsAlt};
    bool gpsSaneCheck = checkSane(gpsNow, s_prevGps, 3, s_gpsStuckCount, SANITY_STUCK_THRESHOLD);
    g_status.gpsSane = g_status.gpsValid && gpsSaneCheck;

}

JsonDocument statusGenerateJson(JsonDocument* requestDoc) {
    // NOTE: this deliberately does NOT call statusUpdate(). Sensors are sampled
    // on a fixed cadence from loop() only (STATUS_UPDATE_INTERVAL_MS); calling
    // it per HTTP request made the orientation filter's behaviour depend on how
    // hard the board was being polled - see the header for the full reason.
    // Values here are therefore up to one update interval old.
    JsonDocument resp;
    resp["type"] = "status";

    // Top level carries only what belongs to no category. Anything also
    // reported inside "config" (firmwareVersion, controllerIp, whitelistIps,
    // technicianMode) is deliberately NOT repeated here - every field appears
    // exactly once in the document.
    resp["ledInternal"] = g_status.ledInternal;
    switch (g_status.ledIo) {
        case OFF:    resp["ledIo"] = "OFF";    break;
        case GREEN:  resp["ledIo"] = "GREEN";  break;
        case RED:    resp["ledIo"] = "RED";    break;
        case ORANGE: resp["ledIo"] = "ORANGE"; break;
        default:     resp["ledIo"] = "OFF";    break;
    }
    resp["button_tech"] = g_status.button_tech;
    resp["motorWorkHours"] = round(g_motorWorkSeconds / 3600.0 * 100) / 100.0;
    resp["motorWorkSeconds"] = g_motorWorkSeconds;

    // The four groups below mirror the get_overview / get_imu / get_gps /
    // get_config handlers in http_server.cpp field-for-field - get_status is
    // just all four in one response. Keep them in sync when a field is added
    // there. Every field appears exactly once in the document.

    // Config fields (see "get_config" in http_server.cpp)
    JsonObject config = resp["config"].to<JsonObject>();
    config["firmwareVersion"] = FIRMWARE_VERSION;
    config["controllerIp"] = g_controllerIP.toString();
    JsonArray configWhitelistArray = config["whitelistIps"].to<JsonArray>();
    for (int i = 0; i < g_whitelistCount; ++i) {
        configWhitelistArray.add(g_whitelist[i].toString());
    }
    config["technicianMode"] = technician_mode;
    config["burnedHours"] = round(configGetBurnedHoursFloat() * 100) / 100.0;
    config["sessionHours"] = round((millis() / 3600000.0) * 100) / 100.0;
    switch (g_status.ledIo) {
        case OFF:    config["techLedColor"] = "OFF";    break;
        case GREEN:  config["techLedColor"] = "GREEN";  break;
        case RED:    config["techLedColor"] = "RED";    break;
        case ORANGE: config["techLedColor"] = "ORANGE"; break;
        default:     config["techLedColor"] = "OFF";    break;
    }
    config["imuPitchAxis"]   = imuAxisToString(g_imuAxisMap.pitchAxis);
    config["imuRollAxis"]    = imuAxisToString(g_imuAxisMap.rollAxis);
    config["imuYawAxis"]     = imuAxisToString(g_imuAxisMap.yawAxis);
    config["imuPitchInvert"] = (bool)g_imuAxisMap.pitchInvert;
    config["imuRollInvert"]  = (bool)g_imuAxisMap.rollInvert;
    config["imuYawInvert"]   = (bool)g_imuAxisMap.yawInvert;

    // Overview fields (see "get_overview" in http_server.cpp)
    JsonObject overview = resp["overview"].to<JsonObject>();
    overview["powerConnected"] = g_status.powerConnected;
    overview["powerSane"] = g_status.powerSane;
    overview["systemVoltage"] = g_status.systemVoltage;
    overview["systemCurrent_A"] = g_status.systemCurrent_A;
    JsonArray relaysArray = overview["relays_status"].to<JsonArray>();
    for (uint8_t i = 0; i < RELAY_COUNT; i++) {
        relaysArray.add(KMPProDinoMKRZero.GetRelayState(i));
    }
    JsonArray optosArray = overview["optoin_status"].to<JsonArray>();
    for (uint8_t i = 0; i < OPTOIN_COUNT; i++) {
        optosArray.add(g_status.optos_status[i]);
    }
    overview["safetyMode"] = g_status.safetyMode;
    overview["safetyModeDurationMs"] = g_status.safetyModeUnsafeDurationMs;
    overview["ocuConnected"] = ocuMonitorIsConnected();
    overview["ocuDisconnectedDurationMs"] = ocuMonitorDisconnectedDurationMs();

    // IMU fields (see "get_imu" in http_server.cpp)
    JsonObject imu = resp["imu"].to<JsonObject>();
    imu["angleSane"] = g_status.angleSane;
    imu["pitch"] = g_status.pitch;
    imu["roll"] = g_status.roll;
    imu["yaw"] = g_status.yaw;
    imu["imuValid"] = g_status.imuValid;
    imu["imu1Sane"] = g_status.imu1Sane;
    imu["imuX"] = g_status.imuX;
    imu["imuY"] = g_status.imuY;
    imu["imuZ"] = g_status.imuZ;
    imu["imuGx"] = g_status.imuGx;
    imu["imuGy"] = g_status.imuGy;
    imu["imuGz"] = g_status.imuGz;
    imu["imu2Valid"] = g_status.imu2Valid;
    imu["imu2Sane"] = g_status.imu2Sane;
    imu["imu2X"] = g_status.imu2X;
    imu["imu2Y"] = g_status.imu2Y;
    imu["imu2Z"] = g_status.imu2Z;
    imu["imu2Gx"] = g_status.imu2Gx;
    imu["imu2Gy"] = g_status.imu2Gy;
    imu["imu2Gz"] = g_status.imu2Gz;

    // Calibration state travels with the status, not only in get_calibration.
    // Without a burned zero the angles below are still perfectly plausible -
    // they are just measured from the enclosure instead of from the machine -
    // so a client has no way to tell a calibrated board from an uncalibrated
    // one by looking at them. Anything polling get_status can now say so.
    imu["zeroCalValid"] = g_zeroCalValid;
    {
        float mp = 0.0f, mr = 0.0f;
        statusZeroCalAngles(mp, mr);
        imu["mountPitch"] = mp;
        imu["mountRoll"]  = mr;
    }
    // So a UI can grey out a calibrate button and say WHY, rather than
    // letting the request fail.
    imu["restSeconds"] = statusRestSeconds();
    imu["atRest"] = statusRestSeconds() >= REST_SECONDS_FOR_CALIBRATION;
    imu["imuTemp"] = g_status.imuTemp;

    // GPS fields (see "get_gps" in http_server.cpp)
    JsonObject gps = resp["gps"].to<JsonObject>();
    gps["gpsConnected"] = g_status.gpsConnected;
    gps["gpsSane"] = g_status.gpsSane;
    gps["gpsSatellites"] = g_status.gpsSatellites;
    gps["gpsLat"] = g_status.gpsLat;
    gps["gpsLng"] = g_status.gpsLng;
    gps["gpsAlt"] = g_status.gpsAlt;
    gps["gpsHeading"] = g_status.gpsHeading;
    gps["gpsGroundSpeed"] = g_status.gpsGroundSpeed;
    gps["gpsSpeedNorth"] = g_status.gpsSpeedNorth;
    gps["gpsSpeedEast"] = g_status.gpsSpeedEast;
    gps["gpsSpeedDown"] = g_status.gpsSpeedDown;
    gps["gpsTime"] = g_status.gpsTime;
    gps["lastGpsLat"] = g_status.lastGpsLat;
    gps["lastGpsLng"] = g_status.lastGpsLng;
    gps["lastGpsAlt"] = g_status.lastGpsAlt;

    return resp;
}

JsonDocument statusGenerateJsonSimple() {
    return statusGenerateJson(nullptr);
}

/**
 * @brief Print one "name: value" line, indented by two spaces.
 *
 * serializeJson() is used for the value so every JSON type renders correctly
 * (booleans as true/false, arrays as [a,b,c], doubles at full precision)
 * without this function needing to know the field's type.
 */
static void printJsonField(const char* name, JsonVariantConst value) {
    Serial.print("  ");
    Serial.print(name);
    Serial.print(": ");
    serializeJson(value, Serial);
    Serial.println();
}

void statusWriteToSerial() {
    // Skip entirely when no USB host has opened the port. Without this the
    // board still spends the time formatting ~55 lines every second and
    // pushing them into a buffer nobody drains.
    if (!Serial) {
        return;
    }

    // Dump exactly what get_status returns, so the serial log and the API can
    // never disagree. Adding a field to statusGenerateJson() above makes it
    // show up here automatically - nothing to update in this function.
    // Reports the latest sampled state; it does not itself trigger a read.
    JsonDocument doc = statusGenerateJson(nullptr);
    JsonObjectConst root = doc.as<JsonObjectConst>();

    Serial.println();
    Serial.println("========== DEVICE STATUS ==========");

    // The actual link IP, which is not part of the JSON (controllerIp under
    // [config] is the *configured* address - they differ if DHCP is in play).
    Serial.print("  localIp: ");
    Serial.println(Ethernet.localIP());

    // Pass 1: the top-level scalars (type, firmwareVersion, ledIo, ...).
    for (JsonPairConst kv : root) {
        if (kv.value().is<JsonObjectConst>()) continue;
        printJsonField(kv.key().c_str(), kv.value());
    }

    // Pass 2: one block per category (overview / imu / gps / config).
    for (JsonPairConst kv : root) {
        if (!kv.value().is<JsonObjectConst>()) continue;
        Serial.print("[");
        Serial.print(kv.key().c_str());
        Serial.println("]");
        for (JsonPairConst field : kv.value().as<JsonObjectConst>()) {
            printJsonField(field.key().c_str(), field.value());
        }
    }

    Serial.println("===================================");
    Serial.println();
}

bool statusAllDevicesConnected() {
    return s_allDevicesConnected;
}

float statusRestSeconds() {
    return s_rest.stillSeconds;
}

void statusZeroCalAngles(float &pitchDeg, float &rollDeg) {
    pitchDeg = rollDeg = 0.0f;
    if (!g_zeroCalValid) {
        return;
    }
    // The attitude the sensor was sitting at when level was declared - i.e.
    // how the bracket holds it. Useful to a technician deciding whether a
    // burn looks sane before trusting it.
    const float x = g_imu1ZeroG[0], y = g_imu1ZeroG[1], z = g_imu1ZeroG[2];
    pitchDeg = atan2(-y, z) * 180.0 / M_PI;
    rollDeg  = atan2(-x, sqrt(y * y + z * z)) * 180.0 / M_PI;
}

bool statusBurnZeroCalibration(const char *&err) {
    if (s_rest.stillSeconds < REST_SECONDS_FOR_CALIBRATION) {
        err = "machine is not standing still";
        return false;
    }
    if (!g_status.imu1Sane || !g_status.imu2Sane) {
        // Both, not either. A zero burned from one IMU would leave the other
        // uncorrected, and the angle would jump the moment the filter fell
        // back to it.
        err = "both IMUs must be healthy to burn a zero calibration";
        return false;
    }

    // Sensor-frame gravity, NOT the mount-corrected vectors - this is what
    // defines the correction, so it has to be measured before one is applied.
    const float g1[3] = {g_status.imuX,  g_status.imuY,  g_status.imuZ};
    const float g2[3] = {g_status.imu2X, g_status.imu2Y, g_status.imu2Z};

    if (!configBurnZeroCal(g1, g2)) {
        err = "measured gravity vector was unusable - nothing was changed";
        return false;
    }

    rebuildZeroCal();

    // The filter still holds angles computed through the OLD transform, and
    // would take its time constant to walk to the new ones. Level was just
    // declared, so say so immediately.
    g_status.pitch = 0.0f;
    g_status.roll  = 0.0f;

    err = nullptr;
    return true;
}

bool statusInitiatedCalibration(const char *&err) {
    if (s_rest.stillSeconds < REST_SECONDS_FOR_CALIBRATION) {
        err = "machine is not standing still";
        return false;
    }

    // Average whichever IMUs are healthy, in the machine frame, exactly as
    // the filter does - so the correction lands on the same reference the
    // reported angle uses.
    float ax = 0.0f, ay = 0.0f, az = 0.0f;
    int n = 0;
    if (s_a1Valid && g_status.imu1Sane) {
        ax += s_a1[0]; ay += s_a1[1]; az += s_a1[2]; n++;
    }
    if (s_a2Valid && g_status.imu2Sane) {
        // IMU2 sits rotated 180 deg in X/Y, so its in-plane components are
        // negated into the common frame - the same convention
        // calculateMergedOrientation() uses.
        ax += -s_a2[0]; ay += -s_a2[1]; az += s_a2[2]; n++;
    }
    if (n == 0) {
        err = "no healthy IMU to calibrate from";
        return false;
    }
    ax /= n; ay /= n; az /= n;

    // At rest the accelerometer is reading gravity and nothing else, so this
    // IS the attitude - whatever the filter had drifted to is simply wrong.
    g_status.pitch = atan2(-ay, az) * 180.0 / M_PI;
    g_status.roll  = atan2(-ax, sqrt(ay * ay + az * az)) * 180.0 / M_PI;

    // Gravity says nothing about heading, so there is nothing to correct yaw
    // against while stationary - it is zeroed, as specified. The GPS gate
    // needs GPS_HEADING_MIN_SPEED_KMH before it touches yaw again, so this
    // stays put until the machine actually moves.
    g_status.yaw = 0.0f;
    s_yawAlignedToGps = false;

    err = nullptr;
    return true;
}
