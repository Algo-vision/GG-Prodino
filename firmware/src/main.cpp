/**
 * @file main.cpp
 * @brief GRK Controller Main Application
 * 
 * This is the main entry point for the GRK controller.
 * All functionality has been modularized into separate components:
 *
 * - config_manager: Configuration persistence (IPs, serial number, work hours)
 * - auth_manager: Authentication and session management
 * - status_manager: Device status tracking and JSON generation
 * - http_server: HTTP API request handling
 * - led_controller: Status LED blink patterns
 * - relay_controller: Relay auto-reset functionality
 * - mqtt_handler: MQTT communication
 *
 * @version 1.5.0
 */

// ============================================================================
// INCLUDES
// ============================================================================

// Board and hardware
#include "KMPProDinoMKRZero.h"
#include "KMPCommon.h"
#include <Ethernet.h>
#include <EthernetUdp.h>
#include <ArduinoOTA.h>
#include <ArduinoJson.h>

// HAL and sensors
#include "gg_hal.hpp"
#include <i2c_imu_gps.hpp>
#include <imu_mount_orientation.hpp>
#include "calculations.hpp"

// Application modules
#include "config_manager.hpp"
#include "auth_manager.hpp"
#include "status_manager.hpp"
#include "http_server.hpp"
#include "led_controller.hpp"
#include "relay_controller.hpp"
#include "ocu_monitor.hpp"
#include "mqtt_handler.hpp"

// ============================================================================
// FIRMWARE VERSION
// ============================================================================

#define FIRMWARE_VERSION "1.5.0"

// ============================================================================
// NETWORK CONFIGURATION
// ============================================================================

/** MAC address for Ethernet controller */
byte _mac[] = {0x00, 0x08, 0xDC, 0x53, 0x09, 0x72};

/** HTTP server port */
constexpr uint16_t LOCAL_PORT = 80;

/** UDP broadcast port */
constexpr unsigned int UDP_PORT = 5000;

/** UDP broadcast interval (ms) */
constexpr unsigned long UDP_BROADCAST_INTERVAL = 200;

// MQTT_PUBLISH_INTERVAL is defined in mqtt_handler.hpp

// ============================================================================
// GLOBAL INSTANCES
// ============================================================================

/** Ethernet HTTP server */
EthernetServer _server(LOCAL_PORT);

/** UDP socket for broadcasting */
EthernetUDP udp;

/** Hardware Abstraction Layer */
GG_HAL _gg_hal;

/** MQTT Handler */
MQTTHandler mqttHandler;

// ============================================================================
// OPERATING MODE FLAGS
// ============================================================================

/** Technician mode flag (enables OTA, serial number programming) */
bool technician_mode = false;

/** OTA update in progress flag */
bool ota_in_progress = false;

/** User currently connected flag */
bool user_connected = false;

/** Last user connection timestamp */
unsigned long last_user_connected_time = 0;

// ============================================================================
// LED CONTROL STATE (accessed by http_server)
// ============================================================================

/** Manual LED control override flag */
bool manual_led_control_active = false;

/** Manual LED state value */
LED_STATES manual_led_state = OFF;

// ============================================================================
// FUNCTION PROTOTYPES
// ============================================================================

void calibrateIMU();
void handleSerialCommands();

// ============================================================================
// SETUP
// ============================================================================

void setup() {
    Serial.begin(115200);
    
    // Initialize LED pins
    // LED_GREEN_LEG and LED_RED_LEG are defined in gg_hal.hpp
    pinMode(LED_GREEN_LEG, OUTPUT);
    pinMode(LED_RED_LEG, OUTPUT);
    // The boot-up solid-orange indicator and grace period are handled by
    // led_controller (LED_BOOT_GRACE_MS), once ledControllerInit() runs below.

    // Load configuration from flash
    configLoad();
    serialNumberLoad();
    
    // Initialize board hardware
    KMPProDinoMKRZero.init(ProDino_MKR_Zero_Ethernet);
    
    // Configure network
    IPAddress subnet(255, 255, 0, 0);
    IPAddress gateway(192, 168, 100, 1);
    IPAddress dns(8, 8, 8, 8);
    Ethernet.begin(_mac, g_controllerIP, dns, gateway, subnet);
    
    // Start servers
    _server.begin();
    _gg_hal.init();
    
    // Initialize u-blox GNSS for UBX protocol (hAcc/vAcc/altEllipsoid)
    initUbloxGNSS();
    
    // Check for technician mode (button held for 5 seconds during startup)
    unsigned long techStart = millis();
    bool techButtonHeld = false;
    Serial.println("Hold button 1 to enter technician mode...");
    Serial.print("Button state is: ");
    Serial.println(_gg_hal.get_button_tech_state() ? "PRESSED" : "RELEASED");
    
    while ((millis() - techStart) < 5000) {
        bool buttonState = _gg_hal.get_button_tech_state();
        Serial.print("Button state is: ");
        Serial.println(buttonState ? "PRESSED" : "RELEASED");
        
        if (buttonState) {
            techButtonHeld = true;
        } else {
            techButtonHeld = false;
            break;
        }
        delay(10);
    }
    
    if (techButtonHeld) {
        technician_mode = true;
        
        // Initialize OTA in technician mode
        ArduinoOTA.onStart([]() {
            ota_in_progress = true;
            Serial.println("OTA update started.");
        });
        ArduinoOTA.onError([](int error, const char* msg) {
            ota_in_progress = false;
            Serial.print("OTA Error[");
            Serial.print(error);
            Serial.print("]: ");
            Serial.println(msg);
        });
        ArduinoOTA.begin(Ethernet.localIP(), "grk", "", InternalStorage);
        Serial.println("OTA update enabled. Use Arduino IDE or compatible tool.");
    }
    
    Serial.println(technician_mode ? "Technician mode enabled." : "Normal mode.");
    
    // Calibrate IMU sensors
    calibrateIMU();
    
    // Initialize application modules
    authInit();
    statusInit();
    httpServerInit(_server);
    ledControllerInit();
    relayControllerInit();
    ocuMonitorInit();

    // Start UDP broadcast
    udp.begin(UDP_PORT);
    Serial.println("UDP Broadcast started on port " + String(UDP_PORT));
    
    // Initialize MQTT Handler with device serial number and router IP
    mqttHandler.begin(g_serialNumber, g_routerIP);
    Serial.println("MQTT handler initialized. Will attempt connection in loop()...");
    
    // Print startup info
    Serial.println("Starting up...");
    Serial.println("The example WebRelay is started.");
    Serial.println("IPs:");
    Serial.print("Local IP: ");
    Serial.println(Ethernet.localIP());
    Serial.print("Gateway IP: ");
    Serial.println(Ethernet.gatewayIP());
    Serial.print("Subnet Mask: ");
    Serial.println(Ethernet.subnetMask());
    Serial.print("MQTT Broker (Router) IP: ");
    Serial.println(g_routerIP);
}

// ============================================================================
// MAIN LOOP
// ============================================================================

void loop() {
    // Handle OTA updates in technician mode
    if (technician_mode) {
        ArduinoOTA.handle();
    }
    
    // Check for scheduled reboot
    if (g_rebootPending && millis() >= g_rebootTimeMs) {
        Serial.println("Executing scheduled reboot...");
        delay(100);
        NVIC_SystemReset();
    }
    
    // Handle serial console commands (SET_SN, GET_SN)
    handleSerialCommands();
    
    // Update motor work hours counter (saves to flash every 5 minutes)
    configUpdateWorkHours();

    // Update burned-hours counter (saves to flash every 5 minutes)
    configUpdateBurnedHours();

    // Send/check OCU heartbeat
    ocuMonitorUpdate();


    // 1. Handle MQTT connection maintenance
    mqttHandler.loop();  // Re-enabled
    
    // 2. Handle HTTP requests
    httpServerLoop();
    
    // 3. Update status and publish MQTT data (1Hz)
    static unsigned long lastMQTTPublish = 0;
    if (millis() - lastMQTTPublish > MQTT_PUBLISH_INTERVAL) {
        // Always update hardware status (ensures Serial output has fresh data)
        statusUpdate();
        
        if (mqttHandler.isConnected()) {
            
            // Publish complete status
            JsonDocument statusDoc = statusGenerateJsonSimple();
            mqttHandler.publishStatus(statusDoc);
            
            // Publish GPS data
            mqttHandler.publishGPS(
                g_status.gpsLat, g_status.gpsLng, g_status.gpsAlt,
                g_status.gpsSpeedNorth, g_status.gpsSpeedEast, 
                g_status.gpsSpeedDown, g_status.gpsGroundSpeed,
                g_status.gpsHeading,
                g_status.gpsValid, g_status.gpsConnected,
                g_status.gpsTime, g_status.gpsSatellites,
                g_status.gpsHAcc, g_status.gpsVAcc, g_status.gpsAltEllipsoid
            );
            
            // Publish IMU data
            mqttHandler.publishIMU(
                g_status.imuX, g_status.imuY, g_status.imuZ,
                g_status.imuGx, g_status.imuGy, g_status.imuGz,
                g_status.pitch, g_status.roll, g_status.yaw,
                g_status.imuValid
            );
            
            // Publish relay states
            mqttHandler.publishRelays(
                g_status.relays_status[0], g_status.relays_status[1],
                g_status.relays_status[2], g_status.relays_status[3]
            );
            
            // Publish LED states
            const char* ledIoStr = "OFF";
            if (g_status.ledIo == GREEN) ledIoStr = "GREEN";
            else if (g_status.ledIo == RED) ledIoStr = "RED";
            else if (g_status.ledIo == ORANGE) ledIoStr = "ORANGE";
            mqttHandler.publishLEDs(g_status.ledInternal, ledIoStr);
            
            // Publish sensor states
            mqttHandler.publishSensors(
                g_status.optos_status[0], g_status.optos_status[1],
                g_status.optos_status[2], g_status.optos_status[3],
                g_status.button_tech
            );
            
            // Publish power monitoring
            mqttHandler.publishPower(g_status.powerConnected, g_status.busVoltage);
        }
        lastMQTTPublish = millis();
    }
    
    // 4. Regular maintenance tasks
    static unsigned long lastSerialOutput = 0;
    if (millis() - lastSerialOutput > 1000) {
        statusWriteToSerial();
        lastSerialOutput = millis();
    }
    relayControllerUpdate();
    
    // Update user connection status
    if (millis() - httpGetLastUserConnectedTime() > 5000) {
        user_connected = false;
    }
    
    // Update LED status
    ledControllerUpdate();
}

// ============================================================================
// IMU CALIBRATION
// ============================================================================

void calibrateIMU() {
    // Calibrate Accelerometer (100 readings)
    Serial.println("Calibrating IMU... Keep the device flat and still.");
    float ax_sum = 0.0f, ay_sum = 0.0f, az_sum = 0.0f;
    
    for (int i = 0; i < 100; i++) {
        float ax, ay, az;
        if (readAccelerometer(ax, ay, az)) {
            applyMountOrientationRemap(ax, ay, az, g_imuMountOrientation);
            ax_sum += ax;
            ay_sum += ay;
            az_sum += az;
        }
        delay(10);
    }
    
    g_imuXOffset = ax_sum / 100.0f;
    g_imuYOffset = ay_sum / 100.0f;
    // Note: Z offset not used in calculations (gravity component)
    
    Serial.println("Accelerometer calibration complete.");
    Serial.print("Accel Offsets: X="); Serial.print(g_imuXOffset);
    Serial.print(", Y="); Serial.println(g_imuYOffset);
    
    // Calibrate Gyroscope (100 readings)
    Serial.println("Calibrating Gyroscope... Keep the device flat and still.");
    float gx_sum = 0.0f, gy_sum = 0.0f, gz_sum = 0.0f;
    
    for (int i = 0; i < 100; i++) {
        float gx, gy, gz;
        _gg_hal.get_gyro_data(gx, gy, gz);
        applyMountOrientationRemap(gx, gy, gz, g_imuMountOrientation);
        gx_sum += gx;
        gy_sum += gy;
        gz_sum += gz;
        delay(10);
    }
    
    g_gyroXOffset = gx_sum / 100.0f;
    g_gyroYOffset = gy_sum / 100.0f;
    g_gyroZOffset = gz_sum / 100.0f;

    Serial.println("Gyroscope calibration complete.");
    Serial.print("Gyro Offsets: X="); Serial.print(g_gyroXOffset);
    Serial.print(", Y="); Serial.print(g_gyroYOffset);
    Serial.print(", Z="); Serial.println(g_gyroZOffset);

    // Calibrate IMU2 Accelerometer (100 readings)
    Serial.println("Calibrating IMU2... Keep the device flat and still.");
    float ax2_sum = 0.0f, ay2_sum = 0.0f, az2_sum = 0.0f;

    for (int i = 0; i < 100; i++) {
        float ax2, ay2, az2;
        if (readAccelerometer_2(ax2, ay2, az2)) {
            applyMountOrientationRemap(ax2, ay2, az2, g_imuMountOrientation);
            ax2_sum += ax2;
            ay2_sum += ay2;
            az2_sum += az2;
        }
        delay(10);
    }

    g_imu2XOffset = ax2_sum / 100.0f;
    g_imu2YOffset = ay2_sum / 100.0f;
    // Note: Z offset not used in calculations (gravity component)

    Serial.println("IMU2 accelerometer calibration complete.");
    Serial.print("IMU2 Accel Offsets: X="); Serial.print(g_imu2XOffset);
    Serial.print(", Y="); Serial.println(g_imu2YOffset);

    // Calibrate IMU2 Gyroscope (100 readings)
    Serial.println("Calibrating IMU2 gyroscope... Keep the device flat and still.");
    float gx2_sum = 0.0f, gy2_sum = 0.0f, gz2_sum = 0.0f;

    for (int i = 0; i < 100; i++) {
        float gx2, gy2, gz2;
        _gg_hal.get_gyro_data_2(gx2, gy2, gz2);
        applyMountOrientationRemap(gx2, gy2, gz2, g_imuMountOrientation);
        gx2_sum += gx2;
        gy2_sum += gy2;
        gz2_sum += gz2;
        delay(10);
    }

    g_gyro2XOffset = gx2_sum / 100.0f;
    g_gyro2YOffset = gy2_sum / 100.0f;
    g_gyro2ZOffset = gz2_sum / 100.0f;

    Serial.println("IMU2 gyroscope calibration complete.");
    Serial.print("IMU2 Gyro Offsets: X="); Serial.print(g_gyro2XOffset);
    Serial.print(", Y="); Serial.print(g_gyro2YOffset);
    Serial.print(", Z="); Serial.println(g_gyro2ZOffset);
}

// ============================================================================
// SERIAL COMMAND HANDLER
// ============================================================================

void handleSerialCommands() {
    if (Serial.available()) {
        String cmd = Serial.readStringUntil('\n');
        cmd.trim();
        
        if (cmd.startsWith("SET_SN:") && technician_mode) {
            String newSN = cmd.substring(7);
            newSN.trim();
            if (serialNumberBurn(newSN.c_str())) {
                Serial.println("Reboot to apply new serial number to MQTT topics");
            }
        } else if (cmd == "GET_SN") {
            Serial.println("Serial Number: " + serialNumberGet());
        } else if (cmd.startsWith("SET_SN:") && !technician_mode) {
            Serial.println("ERROR: Technician mode required for serial number programming");
            Serial.println("Hold button during startup to enter technician mode");
        }
    }
}
