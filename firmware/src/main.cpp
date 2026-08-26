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
 * @version 1.5.1.1
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
#include "timing_probe.hpp"

// ============================================================================
// FIRMWARE VERSION
// ============================================================================

#define FIRMWARE_VERSION "1.5.1.1"

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

  // NO IMU calibration here, deliberately.
  //
  // V1.5.1 averaged 100 accelerometer and 100 gyro samples per IMU at this
  // point - four seconds of blocking boot - and treated the result as both
  // "level" and "zero rotation". Both assume the machine is standing still
  // and on flat ground at the instant it is switched on, and neither can be
  // promised: power up on a slope and the slope becomes level, power up on a
  // running vehicle and a real rotation rate becomes zero.
  //
  // Level is now burned once by a technician who can see the machine is flat
  // (config_manager.hpp), and the gyro's zero is learned whenever the machine
  // is actually observed to be at rest (gyro_bias.hpp). statusInit() loads
  // both from flash below.

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
  TP_BEGIN(TP_LOOP);
  // Closes the measurement window the moment it expires, so the HTTP
  // request that reads the results cannot land inside them.
  TP_SERVICE();

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
  { TP_BEGIN(TP_OCU); ocuMonitorUpdate(); TP_END(TP_OCU); }

  // 2. Handle HTTP requests. The probe records only iterations that served a
  // client - the empty socket scan runs thousands of times a second and would
  // otherwise drown the per-request statistics in ~30us samples.
  {
    TP_BEGIN(TP_HTTP);
    int served = httpServerLoop();
    if (served > 0) { TP_END(TP_HTTP); }
    (void)served;
  }

  // 3. Regular maintenance tasks
  static unsigned long lastSerialOutput = 0;
  if (millis() - lastSerialOutput > 1000) {
    { TP_BEGIN(TP_SERIAL); statusWriteToSerial(); TP_END(TP_SERIAL); }
    lastSerialOutput = millis();
  }
  relayControllerUpdate();

  // Update user connection status
  if (millis() - httpGetLastUserConnectedTime() > 5000) {
    user_connected = false;
  }

  // Update LED status
  ledControllerUpdate();

  TP_END(TP_LOOP);
}
