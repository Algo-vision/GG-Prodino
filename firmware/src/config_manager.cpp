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

float g_imu1ZeroG[3]     = {0.0f, 0.0f, 0.0f};
float g_imu2ZeroG[3]     = {0.0f, 0.0f, 0.0f};
bool  g_zeroCalValid     = false;
float g_imu1GyroBias[3]  = {0.0f, 0.0f, 0.0f};
float g_imu2GyroBias[3]  = {0.0f, 0.0f, 0.0f};
bool  g_gyroBiasValid    = false;

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
ImuAxisMap g_imuAxisMap = {IMU_AXIS_X, IMU_AXIS_Y, IMU_AXIS_Z};

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
    
    // Save router/MQTT broker IP
    configData.router_ip_bytes[0] = g_routerIP[0];
    configData.router_ip_bytes[1] = g_routerIP[1];
    configData.router_ip_bytes[2] = g_routerIP[2];
    configData.router_ip_bytes[3] = g_routerIP[3];
    
    // Save work hours
    configData.motor_work_seconds = g_motorWorkSeconds;

    // Save burned hours
    configData.burned_hours_seconds = g_burnedHoursSeconds;

    // Save IMU mount orientation
    configData.imu_pitch_axis   = g_imuAxisMap.pitchAxis;
    configData.imu_roll_axis    = g_imuAxisMap.rollAxis;
    configData.imu_yaw_axis     = g_imuAxisMap.yawAxis;
    configData.imu_pitch_invert = g_imuAxisMap.pitchInvert;
    configData.imu_roll_invert  = g_imuAxisMap.rollInvert;
    configData.imu_yaw_invert   = g_imuAxisMap.yawInvert;

    // Zero calibration and gyro bias.
    //
    // These MUST be written on every save. configSave() builds a fresh Config
    // from globals, so a field omitted here is a field erased - and the
    // work-hours counter calls this every five minutes, which would quietly
    // wipe a technician's calibration a few minutes after it was burned.
    if (g_zeroCalValid) {
        configData.zero_cal_marker = ZERO_CAL_MARKER;
        for (int i = 0; i < 3; ++i) {
            configData.imu1_zero_g[i] = g_imu1ZeroG[i];
            configData.imu2_zero_g[i] = g_imu2ZeroG[i];
        }
    }
    if (g_gyroBiasValid) {
        configData.gyro_bias_marker = GYRO_BIAS_MARKER;
        for (int i = 0; i < 3; ++i) {
            configData.imu1_gyro_bias[i] = g_imu1GyroBias[i];
            configData.imu2_gyro_bias[i] = g_imu2GyroBias[i];
        }
    }

    // Preserve serial number from existing flash data
    Config existingData = g_configStore.read();
    if (existingData.serial_number_set && existingData.validation_marker == CONFIG_VALID_MARKER) {
        strncpy(configData.serial_number, existingData.serial_number, MAX_SERIAL_NUMBER_LENGTH - 1);
        configData.serial_number[MAX_SERIAL_NUMBER_LENGTH - 1] = '\0';
        configData.serial_number_set = true;
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
        g_imuAxisMap = imuAxisMapDefault();

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
        
        // Load router/MQTT broker IP
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

        // Load IMU axis mapping. Flash written by a firmware predating the
        // axis map holds junk in these bytes, so fall back to the default
        // unless what we read is a valid permutation.
        ImuAxisMap storedMap = {configData.imu_pitch_axis,
                                configData.imu_roll_axis,
                                configData.imu_yaw_axis,
                                configData.imu_pitch_invert,
                                configData.imu_roll_invert,
                                configData.imu_yaw_invert};
        g_imuAxisMap = imuAxisMapIsValid(storedMap) ? storedMap : imuAxisMapDefault();
    }

    // Initialize work hours timing
    s_workHoursLastUpdateMs = millis();
    s_workHoursLastSaveMs = millis();

    // Initialize burned hours timing
    s_burnedHoursLastUpdateMs = millis();
    s_burnedHoursLastSaveMs = millis();
    
    // Log loaded configuration
    // Zero calibration and gyro bias, each accepted only on its own marker.
    // Anything else - erased flash, or a record written before these fields
    // existed - leaves the controller uncalibrated, which is the safe read.
    if (configData.zero_cal_marker == ZERO_CAL_MARKER) {
        for (int i = 0; i < 3; ++i) {
            g_imu1ZeroG[i] = configData.imu1_zero_g[i];
            g_imu2ZeroG[i] = configData.imu2_zero_g[i];
        }
        g_zeroCalValid = true;
        Serial.println("Zero calibration loaded from flash.");
    } else {
        g_zeroCalValid = false;
        Serial.println("NO zero calibration burned - angles are reported in the");
        Serial.println("sensor frame, uncorrected for how the unit is mounted.");
    }

    if (configData.gyro_bias_marker == GYRO_BIAS_MARKER) {
        for (int i = 0; i < 3; ++i) {
            g_imu1GyroBias[i] = configData.imu1_gyro_bias[i];
            g_imu2GyroBias[i] = configData.imu2_gyro_bias[i];
        }
        g_gyroBiasValid = true;
        Serial.println("Gyro bias seeded from flash.");
    } else {
        g_gyroBiasValid = false;
        Serial.println("No stored gyro bias - it will be learned at the first rest.");
    }

    Serial.println("Configuration loaded from FlashStorage.");
    Serial.print("Controller IP: ");
    Serial.println(g_controllerIP.toString());
    Serial.print("Router/MQTT IP: ");
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

void configSetImuAxisMap(const ImuAxisMap& map) {
    g_imuAxisMap = map;
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

bool configBurnZeroCal(const float imu1G[3], const float imu2G[3]) {
    // Length is the only thing that can be checked here - direction is
    // whatever the bracket gives it. A zero-length vector means a dead or
    // unread sensor, and installing that would rotate every later reading by
    // garbage, so refuse and keep whatever was already burned.
    float n1 = sqrt(imu1G[0]*imu1G[0] + imu1G[1]*imu1G[1] + imu1G[2]*imu1G[2]);
    float n2 = sqrt(imu2G[0]*imu2G[0] + imu2G[1]*imu2G[1] + imu2G[2]*imu2G[2]);
    if (n1 < 0.5f || n2 < 0.5f) {
        return false;
    }

    for (int i = 0; i < 3; ++i) {
        g_imu1ZeroG[i] = imu1G[i];
        g_imu2ZeroG[i] = imu2G[i];
    }
    g_zeroCalValid = true;
    configSave();
    return true;
}

void configStoreGyroBias(const float bias1[3], const float bias2[3]) {
    for (int i = 0; i < 3; ++i) {
        g_imu1GyroBias[i] = bias1[i];
        g_imu2GyroBias[i] = bias2[i];
    }
    g_gyroBiasValid = true;
    // Deliberately no configSave() - the periodic one carries it. Writing on
    // every estimate change would erase a flash page every few seconds.
}
