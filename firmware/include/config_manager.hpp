/**
 * @file config_manager.hpp
 * @brief Configuration Management Module
 * 
 * Handles persistent storage of device configuration including:
 * - Controller IP address
 * - IP whitelist for authentication
 * - Router/MQTT broker IP
 * - Serial number
 * - Motor work hours counter
 * 
 * Configuration is stored in FlashStorage and persists across power cycles.
 */

#ifndef CONFIG_MANAGER_HPP
#define CONFIG_MANAGER_HPP

#include <Arduino.h>
#include <IPAddress.h>
#include <FlashStorage.h>
#include <imu_mount_orientation.hpp>

// ============================================================================
// CONSTANTS
// ============================================================================

/** Maximum number of whitelisted IPs */
constexpr int MAX_WHITELIST_IPS = 3;

/** Maximum length of serial number string */
constexpr int MAX_SERIAL_NUMBER_LENGTH = 16;

/** Validation marker for valid configuration */
constexpr uint32_t CONFIG_VALID_MARKER = 0xCAFECAFE;

/** Markers for the two blocks appended below, each validated separately.
 *
 *  Flash written by an earlier firmware has arbitrary bytes where those
 *  blocks now sit. A 0/1 flag would read as "calibrated" about half the time
 *  and install whatever junk followed it as a mounting angle; matching a
 *  specific 32-bit value by accident is not a real risk. */
constexpr uint32_t ZERO_CAL_MARKER  = 0x5A3C0001;
constexpr uint32_t GYRO_BIAS_MARKER = 0x5A3C0002;

/** Interval between work hours flash saves (5 minutes) */
constexpr unsigned long WORK_HOURS_SAVE_INTERVAL_MS = 300000;

// ============================================================================
// CONFIGURATION STRUCTURE
// ============================================================================

/**
 * @brief Persistent configuration data structure
 * 
 * This struct is stored in FlashStorage and contains all configuration
 * that needs to persist across power cycles.
 */
struct Config {
    // Network Configuration
    byte controller_ip_bytes[4];              ///< Controller's static IP address
    byte whitelist_ip_bytes[MAX_WHITELIST_IPS][4];  ///< Whitelisted client IPs
    int whitelist_count;                      ///< Number of valid whitelist entries
    
    // Router/MQTT Broker Configuration
    byte router_ip_bytes[4];                  ///< Teltonika router IP (MQTT broker)
    
    // Serial Number Configuration
    char serial_number[MAX_SERIAL_NUMBER_LENGTH];  ///< Device serial number
    bool serial_number_set;                   ///< True if serial number has been programmed
    uint32_t validation_marker;               ///< Set to CONFIG_VALID_MARKER when valid
    
    // Motor Work Hours Counter
    uint32_t motor_work_seconds;              ///< Total operational seconds since first boot

    // Burned Hours Counter
    uint32_t burned_hours_seconds;            ///< Total operational seconds since serial number burn

    // IMU Axis Mapping - which sensor axis each angle rotates about.
    // Kept last in the struct so that growing it from the single
    // imu_mount_orientation byte it replaced does not shift any field above.
    uint8_t imu_pitch_axis;                   ///< ImuAxis value (see imu_mount_orientation.hpp)
    uint8_t imu_roll_axis;                    ///< ImuAxis value
    uint8_t imu_yaw_axis;                     ///< ImuAxis value
    uint8_t imu_pitch_invert;                 ///< 0 or 1 - negate the pitch axis
    uint8_t imu_roll_invert;                  ///< 0 or 1 - negate the roll axis
    uint8_t imu_yaw_invert;                   ///< 0 or 1 - negate the yaw axis

    // Zero calibration - the gravity direction each IMU reads with the machine
    // on level ground. See zero_calibration.hpp for why a direction and not an
    // offset. Written only when a technician burns one; never inferred at boot.
    uint32_t zero_cal_marker;
    float imu1_zero_g[3];
    float imu2_zero_g[3];

    // Last learned gyro zero-rate offset, carried across power cycles as a
    // starting point. See gyro_bias.hpp - boot no longer measures this,
    // because it cannot know whether the machine is standing still.
    uint32_t gyro_bias_marker;
    float imu1_gyro_bias[3];
    float imu2_gyro_bias[3];

    /**
     * @brief Constructor - initializes with default values
     */
    Config() : whitelist_count(0), serial_number_set(false),
               validation_marker(0), motor_work_seconds(0), burned_hours_seconds(0),
               imu_pitch_axis(IMU_AXIS_X), imu_roll_axis(IMU_AXIS_Y),
               imu_yaw_axis(IMU_AXIS_Z), imu_pitch_invert(0),
               imu_roll_invert(0), imu_yaw_invert(0),
               zero_cal_marker(0), gyro_bias_marker(0) {
        // Uncalibrated is the honest default: report the sensor frame as-is
        // rather than invent a correction nobody asked for.
        for (int i = 0; i < 3; ++i) {
            imu1_zero_g[i] = 0.0f;    imu2_zero_g[i] = 0.0f;
            imu1_gyro_bias[i] = 0.0f; imu2_gyro_bias[i] = 0.0f;
        }
        // Default controller IP: 192.168.1.198
        controller_ip_bytes[0] = 192;
        controller_ip_bytes[1] = 168;
        controller_ip_bytes[2] = 1;
        controller_ip_bytes[3] = 198;
        
        // Default whitelist IPs: 192.168.1.20, 192.168.1.169, 192.168.1.33
        whitelist_ip_bytes[0][0] = 192; whitelist_ip_bytes[0][1] = 168;
        whitelist_ip_bytes[0][2] = 1;   whitelist_ip_bytes[0][3] = 20;

        whitelist_ip_bytes[1][0] = 192; whitelist_ip_bytes[1][1] = 168;
        whitelist_ip_bytes[1][2] = 1;   whitelist_ip_bytes[1][3] = 169;

        whitelist_ip_bytes[2][0] = 192; whitelist_ip_bytes[2][1] = 168;
        whitelist_ip_bytes[2][2] = 1;   whitelist_ip_bytes[2][3] = 33;

        whitelist_count = 3;
    }
};

// ============================================================================
// GLOBAL CONFIGURATION STATE (extern declarations)
// ============================================================================

/** Current controller IP address (loaded from flash) */
extern IPAddress g_controllerIP;

/** Current whitelist of allowed client IPs */
extern IPAddress g_whitelist[MAX_WHITELIST_IPS];

/** Number of IPs in whitelist */
extern int g_whitelistCount;

/** Current motor work seconds counter */
extern uint32_t g_motorWorkSeconds;

/** Seconds of operation since the serial number was burned */
extern uint32_t g_burnedHoursSeconds;

/** Which sensor axis each angle rotates about (see imu_mount_orientation.hpp) */
extern ImuAxisMap g_imuAxisMap;

/** Flag to indicate a reboot is pending */
extern bool g_rebootPending;

/** Time (in millis) when the pending reboot should execute */
extern unsigned long g_rebootTimeMs;

// ============================================================================
// CONFIGURATION MANAGEMENT FUNCTIONS
// ============================================================================

/**
 * @brief Load configuration from FlashStorage
 * 
 * Reads stored configuration from flash. If flash is uninitialized or
 * corrupted, default values are written and used.
 * Also initializes work hours timing variables.
 */
void configLoad();

/**
 * @brief Save current configuration to FlashStorage
 * 
 * Writes all current configuration values (IPs, whitelist, etc.) to flash.
 * Does NOT save serial number - use serialNumberBurn() for that.
 */
void configSave();

/**
 * @brief Update motor work hours counter
 * 
 * Should be called periodically in loop() to track operational time.
 * Automatically saves to flash every WORK_HOURS_SAVE_INTERVAL_MS.
 */
void configUpdateWorkHours();

/**
 * @brief Get motor work hours as a float
 * @return Work hours with decimal precision
 */
float configGetWorkHoursFloat();

/**
 * @brief Update burned-hours counter
 *
 * Should be called periodically in loop() to track operational time
 * since the serial number was burned. Automatically saves to flash
 * every WORK_HOURS_SAVE_INTERVAL_MS.
 */
void configUpdateBurnedHours();

/**
 * @brief Get burned hours as a float
 * @return Hours since serial number burn, with decimal precision
 */
float configGetBurnedHoursFloat();

/**
 * @brief Set controller IP address
 * @param ip New controller IP address
 */
void configSetControllerIP(const IPAddress& ip);

/**
 * @brief Set router/MQTT broker IP address
 * @param ip New router IP address
 */
void configSetRouterIP(const IPAddress& ip);

/**
 * @brief Set which sensor axis each angle rotates about
 * @param map Axis map; must be a valid permutation (see imuAxisMapIsValid())
 */
void configSetImuAxisMap(const ImuAxisMap& map);

/**
 * @brief Set whitelist IPs
 * @param ips Array of IP addresses
 * @param count Number of IPs in array
 * @return true if successful, false if count exceeds MAX_WHITELIST_IPS
 */
bool configSetWhitelist(const IPAddress* ips, int count);

/**
 * @brief Check if an IP is in the whitelist
 * @param ip IP address to check
 * @return true if IP is whitelisted
 */
bool configIsIPWhitelisted(const IPAddress& ip);

// ============================================================================
// SERIAL NUMBER MANAGEMENT FUNCTIONS
// ============================================================================

/**
 * @brief Load serial number from flash
 * 
 * Should be called in setup() after configLoad().
 */
void serialNumberLoad();

/**
 * @brief Programs serial number to flash (one-time burn)
 * @param serialNumber The serial number to burn (e.g., "2042")
 * @return true if successful, false if already set or invalid
 * 
 * @note Once burned, the serial number cannot be changed.
 * @note Requires technician mode to be active.
 */
bool serialNumberBurn(const char* serialNumber);

/**
 * @brief Get current serial number
 * @return Serial number string (or "UNCONFIGURED" if not set)
 */
String serialNumberGet();

/**
 * @brief Check if serial number is modifiable
 * @return true if serial number can be burned (not yet set)
 */
bool serialNumberIsModifiable();

// ---------------------------------------------------------------------------
// Zero calibration and gyro bias
//
// Both live in the same flash record as everything else, so they ride along
// with the existing periodic configSave() instead of adding flash writes of
// their own - the work-hours counter already rewrites this record every five
// minutes, and that is quite enough erase cycles.
// ---------------------------------------------------------------------------

/** Gravity direction each IMU reads when level. Zeros until burned. */
extern float g_imu1ZeroG[3];
extern float g_imu2ZeroG[3];
extern bool  g_zeroCalValid;

/** Learned gyro zero-rate offsets, seeded from flash at boot. */
extern float g_imu1GyroBias[3];
extern float g_imu2GyroBias[3];
extern bool  g_gyroBiasValid;

/**
 * @brief Store a freshly measured zero calibration and persist it.
 * @return false if either vector has no usable direction, leaving the
 *         previous calibration untouched - a failed measurement must not be
 *         able to destroy a good one.
 */
bool configBurnZeroCal(const float imu1G[3], const float imu2G[3]);

/** @brief Persist the current gyro bias estimate. Cheap - it only updates the
 *         globals; the next configSave() writes them. */
void configStoreGyroBias(const float bias1[3], const float bias2[3]);

#endif // CONFIG_MANAGER_HPP
