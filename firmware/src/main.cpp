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
 *
 * @version 1.5.1
 */

// ============================================================================
// INCLUDES
// ============================================================================

// Board and hardware
#include "KMPCommon.h"
#include "KMPProDinoMKRZero.h"
#include <ArduinoJson.h>
#include <ArduinoOTA.h>
#include <Ethernet.h>
#include <EthernetUdp.h>

// HAL and sensors
#include "calculations.hpp"
#include "gg_hal.hpp"
#include <i2c_imu_gps.hpp>
#include <imu_mount_orientation.hpp>

// Application modules
#include "auth_manager.hpp"
#include "config_manager.hpp"
#include "http_server.hpp"
#include "led_controller.hpp"
#include "ocu_monitor.hpp"
#include "relay_controller.hpp"
#include "status_manager.hpp"

// ============================================================================
// FIRMWARE VERSION
// ============================================================================

#define FIRMWARE_VERSION "1.5.1"

// ============================================================================
// NETWORK CONFIGURATION
// ============================================================================

/** MAC address for Ethernet controller */
byte _mac[] = {0x00, 0x08, 0xDC, 0x53, 0x09, 0x72};

/** HTTP server port */
constexpr uint16_t LOCAL_PORT = 80;

// ============================================================================
// GLOBAL INSTANCES
// ============================================================================

/** Ethernet HTTP server */
EthernetServer _server(LOCAL_PORT);

/** Hardware Abstraction Layer */
GG_HAL _gg_hal;

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

  // Initialize board hardware
  KMPProDinoMKRZero.init(ProDino_MKR_Zero_Ethernet);

  // Configure network
  IPAddress subnet(255, 255, 0, 0);
  IPAddress gateway(192, 168, g_controllerIP[2], 1);
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
    ArduinoOTA.onError([](int error, const char *msg) {
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
}

// ============================================================================
// MAIN LOOP
// ============================================================================

void loop() {
  // Handle OTA updates in technician mode
  if (technician_mode) {
    ArduinoOTA.handle();
  }

  // Sample the sensors and run the orientation filter on a FIXED cadence.
  // This is the only place statusUpdate() may be called - the HTTP handlers
  // deliberately do not refresh, so the filter's behaviour cannot depend on
  // how hard the board is being polled. See status_manager.hpp.
  static unsigned long lastStatusUpdate = 0;
  if (millis() - lastStatusUpdate >= STATUS_UPDATE_INTERVAL_MS) {
    lastStatusUpdate = millis();
    statusUpdate();
  }


  // Update motor work hours counter (saves to flash every 5 minutes)
  configUpdateWorkHours();

  // Update burned-hours counter (saves to flash every 5 minutes)
  configUpdateBurnedHours();

  // Send/check OCU heartbeat
  ocuMonitorUpdate();

  // 2. Handle HTTP requests
  httpServerLoop();

  // 3. Regular maintenance tasks
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
  int ax_count = 0;

  for (int i = 0; i < 100; i++) {
    float ax, ay, az;
    if (readAccelerometer(ax, ay, az)) {
      applyImuAxisMap(ax, ay, az, g_imuAxisMap);
      ax_sum += ax;
      ay_sum += ay;
      az_sum += az;
      ax_count++;
    }
    delay(10);
  }

  // Divide by the number of reads that actually SUCCEEDED. Dividing by the
  // loop count would scale the offset down by the failure rate on a flaky
  // sensor, silently biasing that IMU's pitch/roll for the whole session.
  if (ax_count > 0) {
    g_imuXOffset = ax_sum / ax_count;
    g_imuYOffset = ay_sum / ax_count;
  } else {
    g_imuXOffset = 0.0f;
    g_imuYOffset = 0.0f;
    Serial.println("WARNING: IMU1 accelerometer never responded - offsets left at zero");
  }
  // Note: Z offset not used in calculations (gravity component)

  Serial.println("Accelerometer calibration complete.");
  Serial.print("Accel Offsets: X=");
  Serial.print(g_imuXOffset);
  Serial.print(", Y=");
  Serial.println(g_imuYOffset);

  // Calibrate Gyroscope (100 readings)
  Serial.println("Calibrating Gyroscope... Keep the device flat and still.");
  float gx_sum = 0.0f, gy_sum = 0.0f, gz_sum = 0.0f;

  for (int i = 0; i < 100; i++) {
    float gx, gy, gz;
    _gg_hal.get_gyro_data(gx, gy, gz);
    applyImuAxisMap(gx, gy, gz, g_imuAxisMap);
    gx_sum += gx;
    gy_sum += gy;
    gz_sum += gz;
    delay(10);
  }

  g_gyroXOffset = gx_sum / 100.0f;
  g_gyroYOffset = gy_sum / 100.0f;
  g_gyroZOffset = gz_sum / 100.0f;

  Serial.println("Gyroscope calibration complete.");
  Serial.print("Gyro Offsets: X=");
  Serial.print(g_gyroXOffset);
  Serial.print(", Y=");
  Serial.print(g_gyroYOffset);
  Serial.print(", Z=");
  Serial.println(g_gyroZOffset);

  // Calibrate IMU2 Accelerometer (100 readings)
  Serial.println("Calibrating IMU2... Keep the device flat and still.");
  float ax2_sum = 0.0f, ay2_sum = 0.0f, az2_sum = 0.0f;
  int ax2_count = 0;

  for (int i = 0; i < 100; i++) {
    float ax2, ay2, az2;
    if (readAccelerometer_2(ax2, ay2, az2)) {
      applyImuAxisMap(ax2, ay2, az2, g_imuAxisMap);
      ax2_sum += ax2;
      ay2_sum += ay2;
      az2_sum += az2;
      ax2_count++;
    }
    delay(10);
  }

  // See the IMU1 note above - divide by successful reads, not the loop count.
  if (ax2_count > 0) {
    g_imu2XOffset = ax2_sum / ax2_count;
    g_imu2YOffset = ay2_sum / ax2_count;
  } else {
    g_imu2XOffset = 0.0f;
    g_imu2YOffset = 0.0f;
    Serial.println("WARNING: IMU2 accelerometer never responded - offsets left at zero");
  }
  // Note: Z offset not used in calculations (gravity component)

  Serial.println("IMU2 accelerometer calibration complete.");
  Serial.print("IMU2 Accel Offsets: X=");
  Serial.print(g_imu2XOffset);
  Serial.print(", Y=");
  Serial.println(g_imu2YOffset);

  // Calibrate IMU2 Gyroscope (100 readings)
  Serial.println(
      "Calibrating IMU2 gyroscope... Keep the device flat and still.");
  float gx2_sum = 0.0f, gy2_sum = 0.0f, gz2_sum = 0.0f;

  for (int i = 0; i < 100; i++) {
    float gx2, gy2, gz2;
    _gg_hal.get_gyro_data_2(gx2, gy2, gz2);
    applyImuAxisMap(gx2, gy2, gz2, g_imuAxisMap);
    gx2_sum += gx2;
    gy2_sum += gy2;
    gz2_sum += gz2;
    delay(10);
  }

  g_gyro2XOffset = gx2_sum / 100.0f;
  g_gyro2YOffset = gy2_sum / 100.0f;
  g_gyro2ZOffset = gz2_sum / 100.0f;

  Serial.println("IMU2 gyroscope calibration complete.");
  Serial.print("IMU2 Gyro Offsets: X=");
  Serial.print(g_gyro2XOffset);
  Serial.print(", Y=");
  Serial.print(g_gyro2YOffset);
  Serial.print(", Z=");
  Serial.println(g_gyro2ZOffset);
}
