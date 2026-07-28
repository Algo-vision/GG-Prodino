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

// ============================================================================
// CONSTANTS
// ============================================================================

/** Maximum number of whitelisted IPs */
constexpr int MAX_WHITELIST_IPS = 10;

/** Maximum length of serial number string */
constexpr int MAX_SERIAL_NUMBER_LENGTH = 16;

/** Validation marker for valid configuration */
constexpr uint32_t CONFIG_VALID_MARKER = 0xCAFECAFE;

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

    // IMU Mount Orientation
    uint8_t imu_mount_orientation;            ///< ImuMountOrientation enum value (see i2c_imu_gps.hpp)

    // Secure telemetry (per-board key). The nonce's boot id is NOT stored here:
    // FlashStorage lives inside the sketch image, so a firmware upload erases it
    // and the board would restart its counter - see secure_telemetry.hpp.
    uint8_t device_key[32];                   ///< Per-board ChaCha20-Poly1305 key (WRITE-ONLY: never returned by any endpoint)
    bool device_key_set;                      ///< True once a key has been provisioned

    /**
     * @brief Constructor - initializes with default values
     */
    Config() : whitelist_count(0), serial_number_set(false),
               validation_marker(0), motor_work_seconds(0), burned_hours_seconds(0),
               imu_mount_orientation(0), device_key_set(false) {
        memset(device_key, 0, sizeof(device_key));
        // Default controller IP: 192.168.1.198
        controller_ip_bytes[0] = 192;
        controller_ip_bytes[1] = 168;
        controller_ip_bytes[2] = 1;
        controller_ip_bytes[3] = 198;
        
        // Default whitelist IPs: 192.168.1.20, 192.168.1.169, 192.168.1.33,
        // 192.168.1.152 (the in-HLC gateway / Jetson - fetches AWS certs from us)
        whitelist_ip_bytes[0][0] = 192; whitelist_ip_bytes[0][1] = 168;
        whitelist_ip_bytes[0][2] = 1;   whitelist_ip_bytes[0][3] = 20;

        whitelist_ip_bytes[1][0] = 192; whitelist_ip_bytes[1][1] = 168;
        whitelist_ip_bytes[1][2] = 1;   whitelist_ip_bytes[1][3] = 169;

        whitelist_ip_bytes[2][0] = 192; whitelist_ip_bytes[2][1] = 168;
        whitelist_ip_bytes[2][2] = 1;   whitelist_ip_bytes[2][3] = 33;

        whitelist_ip_bytes[3][0] = 192; whitelist_ip_bytes[3][1] = 168;
        whitelist_ip_bytes[3][2] = 1;   whitelist_ip_bytes[3][3] = 152;
        whitelist_count = 4;
        
        // Default router/MQTT broker IP: 192.168.1.1 (Teltonika default)
        router_ip_bytes[0] = 192;
        router_ip_bytes[1] = 168;
        router_ip_bytes[2] = 1;
        router_ip_bytes[3] = 1;
        
        // Default serial number (unconfigured)
        memset(serial_number, 0, sizeof(serial_number));
        strcpy(serial_number, "UNCONFIGURED");
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

/** Router/MQTT broker IP address */
extern IPAddress g_routerIP;

/** Device serial number string */
extern String g_serialNumber;

/** Current motor work seconds counter */
extern uint32_t g_motorWorkSeconds;

/** Seconds of operation since the serial number was burned */
extern uint32_t g_burnedHoursSeconds;

/** Current IMU mount orientation (ImuMountOrientation enum value, see i2c_imu_gps.hpp) */
extern uint8_t g_imuMountOrientation;

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
 * @brief Set IMU mount orientation
 * @param orientation ImuMountOrientation enum value (see i2c_imu_gps.hpp)
 */
void configSetImuMountOrientation(uint8_t orientation);

// ---------------------------------------------------------------------------
// Secure-telemetry key + replay counter
// ---------------------------------------------------------------------------

/**
 * @brief Burn the per-board telemetry key to flash (technician action).
 * @param key32 pointer to exactly 32 key bytes
 * @return true on success
 * @note WRITE-ONLY by design: there is deliberately no getter that returns the
 *       key off-board. Only the telemetry module reads it, in-place.
 */
bool configSetDeviceKey(const uint8_t* key32);

/** @return true if a device key has been provisioned. */
bool configHasDeviceKey();

/** @brief Copy the device key into out32 (internal use by the telemetry module). */
bool configGetDeviceKey(uint8_t* out32);

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

#endif // CONFIG_MANAGER_HPP
