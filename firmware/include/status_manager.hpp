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

    // Sanity ("not all zero, not stuck", plus a usable gravity magnitude) -
    // these SELECT which IMU(s) feed the orientation filter, see statusUpdate()
    bool imu1Sane = false;
    bool imu2Sane = false;
    bool angleSane = false;

    /** Do the two accelerometers tell the same story? See imu_agreement.hpp.
     *  When they disagree, BOTH imu1Sane and imu2Sane are cleared, per the
     *  V1.5.1.1 specification: nothing identifies which of the two is the
     *  wrong one, so neither can be trusted. Reported separately so a client
     *  can tell this apart from a sensor that stopped responding. */
    bool imusAgree = true;

    /** How many axes carried enough signal to be worth comparing. ZERO means
     *  imusAgree is 'no evidence', not 'verified' - the deadband can leave
     *  the check with nothing to look at. See IMU_AGREE_DEADBAND_G. */
    uint8_t imuAgreeAxes = 0;

    /** Times the agreement check has failed since boot. Latched, because the
     *  check runs at loop rate (~95 Hz) while anything watching over HTTP
     *  polls at perhaps 20 - a brief false positive during vibration would
     *  freeze the angles for a moment and never be seen. A counter cannot
     *  miss it. */
    uint32_t imuDisagreeCount = 0;

    // On-chip die temperature, averaged over whichever IMUs are readable.
    // Reads above ambient because of self-heating. Absolute accuracy is poor
    // (datasheet Toff = +/-15 degC); the change since boot is the useful part.
    float imuTemp = 0;

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
    float systemVoltage = 0.0;    // Volts
    float systemCurrent_A = 0.0;  // Amps (the INA219 reports mA - converted on read)
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
/**
 * @brief Burn the current attitude as this installation's level reference.
 *
 * Requires the machine to have been at rest for REST_SECONDS_FOR_CALIBRATION,
 * because it writes to flash and a bad one is worse than none.
 *
 * @param err Out: why it was refused, when it returns false.
 * @return true if measured, stored and applied.
 */
bool statusBurnZeroCalibration(const char *&err);

/**
 * @brief Initiated calibration: discard accumulated drift.
 *
 * At rest the accelerometer alone gives the true pitch and roll, so the
 * filter is snapped onto them and yaw is zeroed. Unlike the zero calibration
 * this stores nothing - it corrects the estimate, it does not redefine level,
 * and it is valid in ANY attitude, level or not.
 */
bool statusInitiatedCalibration(const char *&err);

/** @brief Unbroken seconds the machine has been observed at rest; 0 if moving. */
float statusRestSeconds();

/** @brief Mounting angles implied by the burned calibration, for the GUI. */
void statusZeroCalAngles(float &pitchDeg, float &rollDeg);

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
 * @return Firmware version (e.g. "1.5.1")
 */
const char* statusGetFirmwareVersion();

/**
 * @brief Initialize status manager
 *
 * Call in setup() after HAL initialization.
 */
void statusInit();

/** Fixed sampling interval for statusUpdate(), in milliseconds.
 *
 *  10 ms (100 Hz) closely matches the IMUs' 104 Hz output data rate, so each
 *  call gets approximately one fresh sample, and it puts the pitch/roll
 *  complementary filter's time constant at a sane dt x 49 = 0.49 s. */
constexpr unsigned long STATUS_UPDATE_INTERVAL_MS = 10;

/** Upper bound applied to the dt used by the orientation filter.
 *
 *  If something blocks the main loop (a failed OCU ARP probe can cost several
 *  hundred ms), the next dt would otherwise integrate a single instantaneous
 *  gyro sample across that whole gap as if the rate had been constant.
 *  Clamping under-integrates instead, which is the lesser error. */
constexpr float STATUS_UPDATE_MAX_DT_S = 0.1f;

/**
 * @brief Update hardware status
 *
 * Reads all sensors, runs the orientation filter, and updates g_status.
 *
 * @warning Call ONLY from loop(), on the fixed STATUS_UPDATE_INTERVAL_MS
 *          cadence. It must NOT be called per HTTP request: the pitch/roll
 *          complementary filter uses a fixed per-step weight, so its effective
 *          time constant is dt x 49 and the accelerometer gains a fixed 2%
 *          weight on every call regardless of elapsed time. Extra calls from
 *          request handlers therefore shortened the time constant and sped up
 *          accelerometer tracking - making the angles depend on how hard the
 *          board was being polled, and increasing susceptibility to the
 *          linear-acceleration error. See imuLimitations.md.
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
 * Outputs human-readable status for debugging: every field of the
 * statusGenerateJson() response, one per line, grouped under its category
 * header ([overview] / [imu] / [gps] / [config]). Because it renders that
 * document directly, the serial log always matches the get_status API.
 *
 * @note Calls statusGenerateJson(), which performs a fresh statusUpdate()
 *       (sensor read) - it does not just print the cached g_status.
 */
void statusWriteToSerial();

/**
 * @brief Check if all devices are connected
 * @return true if GPS and IMU are both valid
 */
bool statusAllDevicesConnected();

#endif // STATUS_MANAGER_HPP
