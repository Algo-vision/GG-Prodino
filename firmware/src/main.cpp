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
 * - secure_telemetry: encrypted status uplink to the dashboard server
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
#include <utility/w5100.h>   // [DIAG] socket table dump
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
#include "telemetry_bench.hpp"   // shared [BENCH] harness (both builds)
#include "secure_telemetry.hpp"  // encrypted telemetry to the dashboard server

// ============================================================================
// TELEMETRY TRANSPORT
// ============================================================================
// The board sends its status as a ChaCha20-Poly1305 packet over plain HTTP to
// our own server: ~16 ms of CPU, no handshake, no heap. It does NOT speak TLS.
// Board-side TLS to AWS IoT was measured at 9,789 ms of blocked CPU per message
// on this chip - see docs/TELEMETRY_BENCHMARK.md - and was removed.
// How the crypto works and how to provision a key: docs/SECURE_TELEMETRY.md
// ============================================================================

extern "C" char* sbrk(int);
static int freeRam() { char t; return &t - reinterpret_cast<char*>(sbrk(0)); }

// ============================================================================
// 50 ms REAL-TIME ISR (TC4) - keeps time-critical GPIO tasks alive even while
// the main loop is busy (e.g. inside a telemetry send).
// RULES: ISR tasks must be GPIO/state-only. NO SPI, NO I2C, NO heap - the main
// loop owns the W5500/SPI bus; touching it from here would corrupt transfers.
// (ledControllerUpdate/relayControllerUpdate end in digitalWrite - verified.)
// ============================================================================
static uint32_t s_maxStatusMs = 0;   // [DIAG] how long the 1 Hz I2C refresh blocks

volatile bool     g_isrTasksEnabled = false;
volatile uint32_t g_isrTicks = 0;
volatile uint32_t g_isrLastMs = 0;
volatile uint32_t g_isrMaxGapMs = 0;

extern "C" void TC4_Handler() {
    if (TC4->COUNT16.INTFLAG.bit.MC0) {
        TC4->COUNT16.INTFLAG.reg = TC_INTFLAG_MC0;
        uint32_t now = millis();
        if (g_isrLastMs) {
            uint32_t gap = now - g_isrLastMs;
            if (gap > g_isrMaxGapMs) g_isrMaxGapMs = gap;
        }
        g_isrLastMs = now;
        g_isrTicks++;
        if (g_isrTasksEnabled) {
            ledControllerUpdate();      // GPIO-only: technician LED state machine
            relayControllerUpdate();    // GPIO-only: relay auto-reset timers
        }
    }
}

static void startRtIsr50ms() {
    GCLK->CLKCTRL.reg = GCLK_CLKCTRL_CLKEN | GCLK_CLKCTRL_GEN_GCLK0 | GCLK_CLKCTRL_ID_TC4_TC5;
    while (GCLK->STATUS.bit.SYNCBUSY);
    TC4->COUNT16.CTRLA.reg = TC_CTRLA_SWRST;
    while (TC4->COUNT16.CTRLA.bit.SWRST);
    // 48 MHz / 1024 = 46875 Hz; MFRQ resets on CC0 -> CC0 = 2343 -> 50 ms
    TC4->COUNT16.CTRLA.reg = TC_CTRLA_MODE_COUNT16 | TC_CTRLA_WAVEGEN_MFRQ | TC_CTRLA_PRESCALER_DIV1024;
    TC4->COUNT16.CC[0].reg = 2343;
    while (TC4->COUNT16.STATUS.bit.SYNCBUSY);
    TC4->COUNT16.INTENSET.reg = TC_INTENSET_MC0;
    NVIC_SetPriority(TC4_IRQn, 3);      // low priority: never preempt SysTick/USB
    NVIC_EnableIRQ(TC4_IRQn);
    TC4->COUNT16.CTRLA.bit.ENABLE = 1;
    while (TC4->COUNT16.STATUS.bit.SYNCBUSY);
}

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

/**
 * How often loop() re-reads the sensors into the shared snapshot.
 *
 * The HTTP API serves that snapshot rather than reading the sensors per request:
 * the read blocks for ~35-41 ms on I2C, which a 20 Hz consumer would otherwise
 * pay on every single poll. 100 ms keeps the data fresh enough (a consumer never
 * sees data older than one refresh) while leaving the loop free to answer.
 */
constexpr unsigned long STATUS_REFRESH_INTERVAL_MS = 100;

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
    
    Serial.print("[RAM] freeRam at boot = "); Serial.println(freeRam());

    // Configure network (the router is the path to the dashboard server)
    IPAddress subnet(255, 255, 255, 0);
    IPAddress gateway(192, 168, 1, 1);
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
    
    techButtonHeld = false;   // *** TEST: force normal mode (button floats PRESSED,
                              // and OTA eats ~1.5KB + a socket we need)
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
    
    Serial.print("[RAM] freeRam after Ethernet+servers = "); Serial.println(freeRam());

    // Initialize application modules
    authInit();
    statusInit();
    httpServerInit(_server);
    ledControllerInit();
    relayControllerInit();
    ocuMonitorInit();

    Serial.print("[RAM] freeRam after all modules init = "); Serial.println(freeRam());

    g_serialNumber = "SN2003";     // *** TEST: hardcoded; burn the real one with SET_SN

    telemetryInit();     // loads the per-board key + draws a fresh boot id
    Serial.print("[RAM] freeRam after telemetry setup = "); Serial.println(freeRam());
    
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
    Serial.print("Router IP: ");
    Serial.println(g_routerIP);

    // Start the 50 ms real-time ISR: LED + relay tasks run from here so they
    // keep their timing regardless of what the main loop is doing.
    startRtIsr50ms();
    g_isrTasksEnabled = true;
    Serial.println("[ISR] 50ms real-time tick started (LED + relay tasks)");
}

// ============================================================================
// MAIN LOOP
// ============================================================================

void loop() {
    bench::loopTick();   // measures max loop block + min free RAM (both builds)

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


    // ---- periodic telemetry: one encrypted packet to the dashboard server ----
    // Only the head of a send blocks loop() (~97 ms: crypto + TCP connect); the
    // reply and socket close are finished by telemetryPump() below.
#ifndef TELEM_INTERVAL_MS
#define TELEM_INTERVAL_MS 60000UL       // 1 minute
#endif
    static unsigned long lastTelemetry = 0;
    static bool firstSendDone = false;
    unsigned long sendDue = firstSendDone ? TELEM_INTERVAL_MS : 15000;
    if (millis() - lastTelemetry >= sendDue) {
        lastTelemetry = millis();
        firstSendDone = true;
        telemetrySendStatus();
    }

    static unsigned long lastBenchReport = 0;
    if (millis() - lastBenchReport >= 10000) {
        lastBenchReport = millis();
        bench::report("AEAD");
        telemetryPrintBlockStats();
    }

    telemetryPump();             // finishes an in-flight send; ~0 when idle

    httpServerLoop();            // HTTP served every loop iteration

    // FIX 2 (v2): ZOMBIE-LISTENER WATCHDOG. Measured fact: the W5500 can report
    // Sn_SR=LISTEN on :80 while its connection engine RSTs every SYN (zombie).
    // The register cannot be trusted, so every 10 s force-CLOSE the :80 socket;
    // httpServerLoop's server.available() then recreates a FRESH listener.
    // POLITE MODE: skip the refresh while HTTP traffic is actively being served
    // (a refresh mid-request caused a rare connection reset) - only refresh when
    // the server has been idle, which is exactly when a zombie needs curing.
    static unsigned long lastListenRecreate = 0;
    bool httpActive = (millis() - httpGetLastUserConnectedTime()) < 10000;
    if (!httpActive && millis() - lastListenRecreate >= 10000) {
        lastListenRecreate = millis();
        for (uint8_t s = 0; s < 8; s++) {
            SPI.beginTransaction(SPI_ETHERNET_SETTINGS);
            uint8_t st = W5100.readSnSR(s);
            uint16_t port = W5100.readSnPORT(s);
            SPI.endTransaction();
            if (port == 80 && st == 0x14 /*LISTEN*/) {
                SPI.beginTransaction(SPI_ETHERNET_SETTINGS);
                W5100.execCmdSn(s, Sock_CLOSE);   // kill the (possibly zombie) listener
                SPI.endTransaction();
                Serial.print("[HTTPWD] force-recreated :80 listener (was socket ");
                Serial.print(s); Serial.println(")");
                break;
            }
        }
        // httpServerLoop -> server.available() reopens the listener within ms
    }

    static unsigned long lastRamPrint = 0;
    if (millis() - lastRamPrint > 5000) {
        lastRamPrint = millis();
        Serial.print("[RAM] freeRam="); Serial.print(freeRam());
        Serial.print("  telemetry key="); Serial.println(telemetryHasKey() ? "ok" : "MISSING");
        // ISR health: maxGap should stay ~50-51 ms even THROUGH a publish window
        uint32_t ticks = g_isrTicks, maxGap = g_isrMaxGapMs;
        g_isrMaxGapMs = 0;
        Serial.print("[DIAG] statusUpdate max="); Serial.print(s_maxStatusMs);
        Serial.println(" ms  <- blocking I2C sensor read, every 1 s");
        Serial.print("[ISR] ticks="); Serial.print(ticks);
        Serial.print("  maxGap="); Serial.print(maxGap); Serial.println(" ms");
        // [DIAG] W5500 socket table: who owns every socket? (0x14=LISTEN,
        // 0x17=ESTABLISHED, 0x00=CLOSED, 0x22=UDP, 0x1C=CLOSE_WAIT)
        Serial.print("[SOCK] ");
        for (uint8_t s = 0; s < 8; s++) {
            SPI.beginTransaction(SPI_ETHERNET_SETTINGS);
            uint8_t st = W5100.readSnSR(s);
            uint16_t port = W5100.readSnPORT(s);
            SPI.endTransaction();
            Serial.print(s); Serial.print(":0x"); Serial.print(st, HEX);
            Serial.print("/"); Serial.print(port); Serial.print(" ");
        }
        Serial.println();
    }

    // Keep the snapshot fresh for consumers - but ONLY if serving HTTP requests
    // hasn't already done it. Each get_status refreshes the sensors itself, so
    // while anything is polling, this would be a duplicated ~41 ms I2C read, and
    // that duplicate was the longest stall those consumers saw.
    if (statusMsSinceRefresh() >= STATUS_REFRESH_INTERVAL_MS) {
        uint32_t ts = millis();
        statusUpdate();
        uint32_t d = millis() - ts;
        if (d > s_maxStatusMs) s_maxStatusMs = d;
    }

    // 4. Regular maintenance tasks
    static unsigned long lastSerialOutput = 0;
    if (millis() - lastSerialOutput > 1000) {
        statusWriteToSerial();
        lastSerialOutput = millis();
    }
    // NOTE: relayControllerUpdate() + ledControllerUpdate() moved to the 50 ms
    // TC4 ISR - they keep running regardless of what this loop is doing.

    // Update user connection status
    if (millis() - httpGetLastUserConnectedTime() > 5000) {
        user_connected = false;
    }
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
                Serial.println("Reboot to apply the new serial number");
            }
        } else if (cmd == "GET_SN") {
            Serial.println("Serial Number: " + serialNumberGet());
        } else if (cmd.startsWith("SET_KEY:")) {
            // Provision the per-board telemetry key: SET_KEY:<64 hex chars>
            // Write-only by design - no command ever reads the key back out.
            String hex = cmd.substring(8);
            hex.trim();
            if (hex.length() != 64) {
                Serial.println("ERROR: SET_KEY needs exactly 64 hex chars (32 bytes)");
            } else {
                uint8_t key[32];
                bool valid = true;
                for (int i = 0; i < 32 && valid; i++) {
                    char hi = hex[i * 2], lo = hex[i * 2 + 1];
                    auto nib = [&](char ch) -> int {
                        if (ch >= '0' && ch <= '9') return ch - '0';
                        if (ch >= 'a' && ch <= 'f') return ch - 'a' + 10;
                        if (ch >= 'A' && ch <= 'F') return ch - 'A' + 10;
                        return -1;
                    };
                    int h = nib(hi), l = nib(lo);
                    if (h < 0 || l < 0) valid = false;
                    else key[i] = (uint8_t)((h << 4) | l);
                }
                if (!valid) Serial.println("ERROR: SET_KEY contains non-hex characters");
                else if (configSetDeviceKey(key)) Serial.println("Reboot to start using the new key");
            }
        } else if (cmd == "GET_KEY_STATUS") {
            Serial.println(configHasDeviceKey() ? "Device key: SET" : "Device key: NOT SET");
        } else if (cmd.startsWith("SET_SN:") && !technician_mode) {
            Serial.println("ERROR: Technician mode required for serial number programming");
            Serial.println("Hold button during startup to enter technician mode");
        }
    }
}
