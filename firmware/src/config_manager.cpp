/**
 * @file config_manager.cpp
 * @brief Configuration Management Module Implementation
 * 
 * Implements persistent storage of device configuration to FlashStorage.
 */

#include "config_manager.hpp"
#include <Arduino_DebugUtils.h>  // For NVIC_SystemReset()

// ============================================================================
// FLASH STORAGE DECLARATION
// ============================================================================

/** FlashStorage object for persistent configuration */
FlashStorage(g_configStore, Config);

// ============================================================================
// GLOBAL CONFIGURATION STATE
// ============================================================================

IPAddress g_controllerIP;
IPAddress g_whitelist[MAX_WHITELIST_IPS];
int g_whitelistCount = 0;
IPAddress g_routerIP;
String g_serialNumber = "UNCONFIGURED";
uint32_t g_motorWorkSeconds = 0;
uint32_t g_burnedHoursSeconds = 0;
uint8_t g_imuMountOrientation = 0;

// Reboot scheduling globals
bool g_rebootPending = false;
unsigned long g_rebootTimeMs = 0;

// ============================================================================
// INTERNAL STATE FOR WORK HOURS TRACKING
// ============================================================================

/** Last time we updated the work hours counter (millis) */
static unsigned long s_workHoursLastUpdateMs = 0;

/** Last time we saved work hours to flash (millis) */
static unsigned long s_workHoursLastSaveMs = 0;

/** Last time we updated the burned-hours counter (millis) */
static unsigned long s_burnedHoursLastUpdateMs = 0;

/** Last time we saved burned hours to flash (millis) */
static unsigned long s_burnedHoursLastSaveMs = 0;

// ============================================================================
// CONFIGURATION MANAGEMENT IMPLEMENTATION
// ============================================================================

void configSave() {
    Config configData;
    
    // Save controller IP
    configData.controller_ip_bytes[0] = g_controllerIP[0];
    configData.controller_ip_bytes[1] = g_controllerIP[1];
    configData.controller_ip_bytes[2] = g_controllerIP[2];
    configData.controller_ip_bytes[3] = g_controllerIP[3];
    
    // Save whitelist IPs
    configData.whitelist_count = g_whitelistCount;
    for (int i = 0; i < g_whitelistCount; ++i) {
        configData.whitelist_ip_bytes[i][0] = g_whitelist[i][0];
        configData.whitelist_ip_bytes[i][1] = g_whitelist[i][1];
        configData.whitelist_ip_bytes[i][2] = g_whitelist[i][2];
        configData.whitelist_ip_bytes[i][3] = g_whitelist[i][3];
    }
    
    // Save router IP
    configData.router_ip_bytes[0] = g_routerIP[0];
    configData.router_ip_bytes[1] = g_routerIP[1];
    configData.router_ip_bytes[2] = g_routerIP[2];
    configData.router_ip_bytes[3] = g_routerIP[3];
    
    // Save work hours
    configData.motor_work_seconds = g_motorWorkSeconds;

    // Save burned hours
    configData.burned_hours_seconds = g_burnedHoursSeconds;

    // Save IMU mount orientation
    configData.imu_mount_orientation = g_imuMountOrientation;

    // Preserve serial number from existing flash data
    Config existingData = g_configStore.read();
    if (existingData.serial_number_set && existingData.validation_marker == CONFIG_VALID_MARKER) {
        strncpy(configData.serial_number, existingData.serial_number, MAX_SERIAL_NUMBER_LENGTH - 1);
        configData.serial_number[MAX_SERIAL_NUMBER_LENGTH - 1] = '\0';
        configData.serial_number_set = true;
        configData.validation_marker = CONFIG_VALID_MARKER;
    }

    // Preserve the telemetry key too. configSave() builds a FRESH Config, so
    // without this any save (an IP change, or the 5-minute work-hours autosave)
    // would silently wipe the provisioned key.
    if (existingData.device_key_set && existingData.validation_marker == CONFIG_VALID_MARKER) {
        memcpy(configData.device_key, existingData.device_key, sizeof(configData.device_key));
        configData.device_key_set = true;
        configData.validation_marker = CONFIG_VALID_MARKER;
    }

    g_configStore.write(configData);
    Serial.println("Configuration saved to FlashStorage.");
}

void configLoad() {
    Config configData = g_configStore.read();
    
    // Check if flash is uninitialized or corrupted
    bool isInvalid = (configData.controller_ip_bytes[0] == 0 &&
                      configData.controller_ip_bytes[1] == 0 &&
                      configData.controller_ip_bytes[2] == 0 &&
                      configData.controller_ip_bytes[3] == 0);
    
    bool hasEmptyWhitelist = (configData.whitelist_count == 0);
    
    if (isInvalid || hasEmptyWhitelist) {
        Serial.println("FlashStorage uninitialized, corrupted, or whitelist empty.");
        Serial.println("Setting default configuration...");
        
        // Use constructor defaults
        Config defaultConfig;
        configData = defaultConfig;
        
        // Populate globals from default config
        g_controllerIP = IPAddress(
            defaultConfig.controller_ip_bytes[0],
            defaultConfig.controller_ip_bytes[1],
            defaultConfig.controller_ip_bytes[2],
            defaultConfig.controller_ip_bytes[3]
        );
        
        g_whitelistCount = defaultConfig.whitelist_count;
        for (int i = 0; i < g_whitelistCount; ++i) {
            g_whitelist[i] = IPAddress(
                defaultConfig.whitelist_ip_bytes[i][0],
                defaultConfig.whitelist_ip_bytes[i][1],
                defaultConfig.whitelist_ip_bytes[i][2],
                defaultConfig.whitelist_ip_bytes[i][3]
            );
        }
        
        g_routerIP = IPAddress(
            defaultConfig.router_ip_bytes[0],
            defaultConfig.router_ip_bytes[1],
            defaultConfig.router_ip_bytes[2],
            defaultConfig.router_ip_bytes[3]
        );
        
        g_motorWorkSeconds = 0;
        g_burnedHoursSeconds = 0;
        g_imuMountOrientation = 0;

        // Save defaults to flash
        configSave();
    } else {
        // Load controller IP
        g_controllerIP = IPAddress(
            configData.controller_ip_bytes[0],
            configData.controller_ip_bytes[1],
            configData.controller_ip_bytes[2],
            configData.controller_ip_bytes[3]
        );
        
        // Load whitelist IPs
        g_whitelistCount = configData.whitelist_count;
        for (int i = 0; i < g_whitelistCount; ++i) {
            g_whitelist[i] = IPAddress(
                configData.whitelist_ip_bytes[i][0],
                configData.whitelist_ip_bytes[i][1],
                configData.whitelist_ip_bytes[i][2],
                configData.whitelist_ip_bytes[i][3]
            );
        }
        
        // Load router IP
        g_routerIP = IPAddress(
            configData.router_ip_bytes[0],
            configData.router_ip_bytes[1],
            configData.router_ip_bytes[2],
            configData.router_ip_bytes[3]
        );
        
        // Load motor work hours
        g_motorWorkSeconds = configData.motor_work_seconds;

        // Load burned hours
        g_burnedHoursSeconds = configData.burned_hours_seconds;

        // Load IMU mount orientation
        g_imuMountOrientation = configData.imu_mount_orientation;
    }

    // Initialize work hours timing
    s_workHoursLastUpdateMs = millis();
    s_workHoursLastSaveMs = millis();

    // Initialize burned hours timing
    s_burnedHoursLastUpdateMs = millis();
    s_burnedHoursLastSaveMs = millis();
    
    // Log loaded configuration
    Serial.println("Configuration loaded from FlashStorage.");
    Serial.print("Controller IP: ");
    Serial.println(g_controllerIP.toString());
    Serial.print("Router IP: ");
    Serial.println(g_routerIP.toString());
    Serial.print("Motor Work Hours: ");
    Serial.print(g_motorWorkSeconds / 3600);
    Serial.print("h ");
    Serial.print((g_motorWorkSeconds % 3600) / 60);
    Serial.println("m");
}

void configUpdateWorkHours() {
    unsigned long now = millis();
    
    // Calculate elapsed seconds since last update
    unsigned long elapsedMs = now - s_workHoursLastUpdateMs;
    unsigned long elapsedSeconds = elapsedMs / 1000;
    
    if (elapsedSeconds > 0) {
        g_motorWorkSeconds += elapsedSeconds;
        // Keep remainder for accuracy
        s_workHoursLastUpdateMs = now - (elapsedMs % 1000);
    }
    
    // Save to flash periodically to preserve flash life
    if (now - s_workHoursLastSaveMs >= WORK_HOURS_SAVE_INTERVAL_MS) {
        Config configData = g_configStore.read();
        configData.motor_work_seconds = g_motorWorkSeconds;
        g_configStore.write(configData);
        s_workHoursLastSaveMs = now;
        
        Serial.print("Work hours saved to flash: ");
        Serial.print(g_motorWorkSeconds / 3600);
        Serial.print("h ");
        Serial.print((g_motorWorkSeconds % 3600) / 60);
        Serial.println("m");
    }
}

float configGetWorkHoursFloat() {
    return g_motorWorkSeconds / 3600.0f;
}

void configUpdateBurnedHours() {
    unsigned long now = millis();

    // Calculate elapsed seconds since last update
    unsigned long elapsedMs = now - s_burnedHoursLastUpdateMs;
    unsigned long elapsedSeconds = elapsedMs / 1000;

    if (elapsedSeconds > 0) {
        g_burnedHoursSeconds += elapsedSeconds;
        // Keep remainder for accuracy
        s_burnedHoursLastUpdateMs = now - (elapsedMs % 1000);
    }

    // Save to flash periodically to preserve flash life
    if (now - s_burnedHoursLastSaveMs >= WORK_HOURS_SAVE_INTERVAL_MS) {
        Config configData = g_configStore.read();
        configData.burned_hours_seconds = g_burnedHoursSeconds;
        g_configStore.write(configData);
        s_burnedHoursLastSaveMs = now;
    }
}

float configGetBurnedHoursFloat() {
    return g_burnedHoursSeconds / 3600.0f;
}

void configSetControllerIP(const IPAddress& ip) {
    g_controllerIP = ip;
}

void configSetRouterIP(const IPAddress& ip) {
    g_routerIP = ip;
}

void configSetImuMountOrientation(uint8_t orientation) {
    g_imuMountOrientation = orientation;
}

bool configSetWhitelist(const IPAddress* ips, int count) {
    if (count > MAX_WHITELIST_IPS) {
        return false;
    }
    
    g_whitelistCount = count;
    for (int i = 0; i < count; ++i) {
        g_whitelist[i] = ips[i];
    }
    return true;
}

bool configIsIPWhitelisted(const IPAddress& ip) {
    for (int i = 0; i < g_whitelistCount; ++i) {
        if (g_whitelist[i] == ip) {
            return true;
        }
    }
    return false;
}

// ============================================================================
// SERIAL NUMBER MANAGEMENT IMPLEMENTATION
// ============================================================================

// Forward declaration - technician_mode is defined in main.cpp
extern bool technician_mode;

void serialNumberLoad() {
    Config configData = g_configStore.read();
    
    if (configData.serial_number_set && configData.validation_marker == CONFIG_VALID_MARKER) {
        g_serialNumber = String(configData.serial_number);
        Serial.println("Serial Number loaded: " + g_serialNumber);
    } else {
        g_serialNumber = "UNCONFIGURED";
        Serial.println("WARNING: Device serial number not configured!");
    }
}

bool serialNumberBurn(const char* input) {
    // Check if serial number is already set (prevent overwrite)
    Config configData = g_configStore.read();
    
    if (configData.serial_number_set && configData.validation_marker == CONFIG_VALID_MARKER) {
        Serial.println("ERROR: Serial number already set and cannot be changed");
        Serial.println("Current SN: " + String(configData.serial_number));
        return false;
    }
    
    // Check if input is a valid number between 2000 and 2999
    int snValue = atoi(input);
    
    if (snValue < 2000 || snValue > 2999) {
        Serial.println("ERROR: Serial number must be between 2000 and 2999");
        return false;
    }
    
    // Format serial number with "SN" prefix
    char formattedSN[MAX_SERIAL_NUMBER_LENGTH];
    sprintf(formattedSN, "SN%d", snValue);
    
    // Write to flash
    strncpy(configData.serial_number, formattedSN, MAX_SERIAL_NUMBER_LENGTH - 1);
    configData.serial_number[MAX_SERIAL_NUMBER_LENGTH - 1] = '\0';
    configData.serial_number_set = true;
    configData.validation_marker = CONFIG_VALID_MARKER;

    // Reset burned-hours counter - it tracks time since this exact burn event
    configData.burned_hours_seconds = 0;
    g_burnedHoursSeconds = 0;

    g_configStore.write(configData);

    // Update global
    g_serialNumber = String(formattedSN);
    
    Serial.println("SUCCESS: Serial number burned: " + g_serialNumber);
    Serial.println("Device will reboot in 2 seconds to apply new serial number...");
    
    // Schedule reboot instead of immediate reset
    g_rebootPending = true;
    g_rebootTimeMs = millis() + 2000;
    
    return true;
}

String serialNumberGet() {
    return g_serialNumber;
}

bool serialNumberIsModifiable() {
    Config configData = g_configStore.read();
    return !(configData.serial_number_set && configData.validation_marker == CONFIG_VALID_MARKER);
}

// ============================================================================
// SECURE-TELEMETRY KEY + REPLAY COUNTER
// ============================================================================

bool configSetDeviceKey(const uint8_t* key32) {
    if (key32 == nullptr) return false;
    Config configData = g_configStore.read();
    memcpy(configData.device_key, key32, 32);
    configData.device_key_set = true;
    configData.validation_marker = CONFIG_VALID_MARKER;
    g_configStore.write(configData);
    Serial.println("SUCCESS: device telemetry key burned (32 bytes)");
    return true;
}

bool configHasDeviceKey() {
    Config configData = g_configStore.read();
    return configData.device_key_set && configData.validation_marker == CONFIG_VALID_MARKER;
}

bool configGetDeviceKey(uint8_t* out32) {
    if (out32 == nullptr) return false;
    Config configData = g_configStore.read();
    if (!configData.device_key_set) return false;
    memcpy(out32, configData.device_key, 32);
    return true;
}

