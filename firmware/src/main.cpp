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
#include "mqtt_handler.hpp"
#include "telemetry_bench.hpp"   // shared [BENCH] harness (both builds)
#include "secure_telemetry.hpp"  // AEAD telemetry (used when TELEMETRY_MODE_AEAD)
#include <SSLClient.h>
#include "aws_certs_store.h"   // AWS_ENDPOINT, AWS_PORT, AWS_DEV_CERT, AWS_DEV_KEY
#include "aws_root_ca.h"       // TAs, TAs_NUM (AWS server trust anchor)

// ============================================================================
// MQTT TRANSPORT - TLS DIRECT TO AWS IoT  *** TEST BUILD (do not commit) ***
// ============================================================================
// RAM/CPU stress test: the FULL production firmware + the FULL TLS stack, to see
// if board-direct AWS IoT fits on this 32KB SAMD21. Publishes to AWS_ENDPOINT.

/** Plain TCP -> wrapped in SSLClient(BearSSL) -> AWS IoT over mutual TLS */
static EthernetClient s_mqttTcp;
static SSLClient      s_ssl(s_mqttTcp, TAs, TAs_NUM, A5);

// Packet-granular SEND PUMP. SSLClient buffers writes; the send must be driven
// by the caller. CRITICAL: SSLClient::flush() is broken for QoS-0 MQTT - it
// waits for BR_SSL_RECVAPP (application data FROM the server) after sending,
// but a QoS-0 publish gets no reply -> flush() blocks the full 30 s timeout,
// latches a write error and kills the connection (root cause of every drop we
// saw). SSLClient::available() instead pumps the engine ONCE, non-blocking:
// it encrypts + sends the buffered record and returns. So: after each complete
// MQTT packet (one bulk write), pump with available() - never call flush().
class PacketClient : public Client {
    SSLClient& c;
public:
    PacketClient(SSLClient& s) : c(s) {}
    int connect(IPAddress ip, uint16_t p) override { return c.connect(ip, p); }
    int connect(const char* h, uint16_t p) override { return c.connect(h, p); }
    size_t write(uint8_t b) override { return c.write(b); }        // buffer only
    size_t write(const uint8_t* b, size_t s) override {
        size_t n = c.write(b, s);
        c.available();                    // pump: encrypt + send, non-blocking
        return n;
    }
    int available() override { return c.available(); }
    int read() override { return c.read(); }
    int read(uint8_t* b, size_t s) override { return c.read(b, s); }
    int peek() override { return c.peek(); }
    void flush() override { c.available(); }   // never SSLClient::flush() (see above)
    void stop() override { c.stop(); }
    uint8_t connected() override { return c.connected(); }
    operator bool() override { return (bool)c; }
};
static PacketClient s_flushing(s_ssl);
static SSLClientParameters s_mTLS =
    SSLClientParameters::fromPEM(AWS_DEV_CERT, strlen(AWS_DEV_CERT),
                                 AWS_DEV_KEY,  strlen(AWS_DEV_KEY));

extern "C" char* sbrk(int);
static int freeRam() { char t; return &t - reinterpret_cast<char*>(sbrk(0)); }

// ============================================================================
// 50 ms REAL-TIME ISR (TC4) - keeps time-critical GPIO tasks alive even while
// the main loop is blocked for ~16 s inside the TLS publish window.
// RULES: ISR tasks must be GPIO/state-only. NO SPI, NO I2C, NO heap - the main
// loop owns the W5500/SPI bus; touching it from here would corrupt transfers.
// (ledControllerUpdate/relayControllerUpdate end in digitalWrite - verified.)
// ============================================================================
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

// MQTT_PUBLISH_INTERVAL is defined in mqtt_handler.hpp

// ============================================================================
// GLOBAL INSTANCES
// ============================================================================

/** Ethernet HTTP server */
EthernetServer _server(LOCAL_PORT);

/** Hardware Abstraction Layer */
GG_HAL _gg_hal;

/** MQTT Handler (over the TLS transport, to AWS IoT) */
MQTTHandler mqttHandler(s_flushing);

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

    // Configure network (home router = internet path to AWS when online)
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
                              // and OTA eats ~1.5KB + a socket we need for the TLS test)
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

    // TLS DIRECT to AWS IoT (TEST): full firmware + full TLS stack.
    g_serialNumber = "SN2003";     // *** TEST: hardcode serial so AWS/webserver recognizes it
    s_ssl.setMutualAuthParams(s_mTLS);
    mqttHandler.begin(g_serialNumber, AWS_ENDPOINT, AWS_PORT);
    Serial.print("[RAM] freeRam after TLS/MQTT setup = "); Serial.println(freeRam());
    Serial.print("[RAM] target = "); Serial.print(AWS_ENDPOINT); Serial.print(":"); Serial.println(AWS_PORT);
    Serial.println("MQTT handler initialized. Will connect to AWS over TLS in loop()...");

#if defined(TELEMETRY_MODE_AEAD)
    telemetryInit();     // loads the per-board key + bumps the boot epoch
    Serial.println("[TEST] telemetry mode = AEAD (ChaCha20-Poly1305 over plain HTTP)");
#else
    Serial.println("[TEST] telemetry mode = TLS (AWS IoT publish window)");
#endif
    
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

    // Start the 50 ms real-time ISR (LED + relay tasks live there now, so they
    // keep running even while the TLS publish window blocks the main loop).
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


    // ========================================================================
    // *** PERSISTENT TLS CONNECTION (test) ***
    // Handshake ONCE (mqttHandler.loop() reconnects only if the link drops),
    // ========================================================================
    // PLAN B: PUBLISH WINDOW every interval - connect -> publish -> DISCONNECT.
    // Measured fact on this 32KB chip: a LIVE TLS session holds ~1.6KB of heap,
    // leaving ~719B free -> an HTTP request (~1.5KB of allocations) hard-faults.
    // With the session CLOSED, HTTP has ~2.3KB and runs at 99.8% / 24ms.
    // So TLS and HTTP take turns: brief publish window, then HTTP gets all the
    // RAM back. Each piece was measured working today (connect 10s, publish
    // 42ms, HTTP 99.8% between windows).
    // ========================================================================
    // BENCHMARK: the send interval is the same for both builds so the runs are
    // directly comparable (override with -D TELEM_INTERVAL_MS=...).
#ifndef TELEM_INTERVAL_MS
#define TELEM_INTERVAL_MS 60000UL
#endif
    const unsigned long PUBLISH_EVERY_MS = TELEM_INTERVAL_MS;
    static unsigned long lastPubWindow = 0;
    static bool firstWindowDone = false;
    unsigned long windowDue = firstWindowDone ? PUBLISH_EVERY_MS : 15000;
    if (millis() - lastPubWindow >= windowDue) {
        lastPubWindow = millis();
        firstWindowDone = true;

#if defined(TELEMETRY_MODE_AEAD)
        // ---- NEW: ChaCha20-Poly1305 packet over plain HTTP (no handshake) ----
        telemetrySendStatus();          // instrumented inside (bench::sendEnd)
#else
        // ---- OLD: TLS publish window - connect, publish, disconnect ----
        uint32_t tb = bench::sendBegin();
        Serial.print("\n[PUB] window OPEN (HTTP pauses). freeRam=");
        Serial.println(freeRam());
        bool pubOk = false;
        if (mqttHandler.connectToMQTTBroker()) {
            // no statusUpdate() here - the 1 Hz loop keeps g_status fresh, and
            // skipping it removes I2C time from the window
            JsonDocument doc = statusGenerateJsonSimple();
            mqttHandler.publishStatus(doc);
            // pump the SSL engine so the record is fully on the wire before close
            for (int i = 0; i < 5; i++) { s_flushing.available(); delay(10); }
            pubOk = true;
        }
        mqttHandler.forceDisconnect();   // release the TLS heap -> HTTP gets it back
        Serial.print("[PUB] window CLOSED (HTTP resumes). freeRam=");
        Serial.println(freeRam());
        bench::sendEnd(tb, pubOk);
#endif
    }

    // periodic benchmark summary (identical in both builds)
    static unsigned long lastBenchReport = 0;
    if (millis() - lastBenchReport >= 10000) {
        lastBenchReport = millis();
#if defined(TELEMETRY_MODE_AEAD)
        bench::report("AEAD");
#else
        bench::report("TLS");
#endif
    }

    httpServerLoop();            // HTTP served every iteration between windows

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
        Serial.print("  mqtt="); Serial.println(mqttHandler.isConnected() ? "CONNECTED" : "down");
        // ISR health: maxGap should stay ~50-51 ms even THROUGH a publish window
        uint32_t ticks = g_isrTicks, maxGap = g_isrMaxGapMs;
        g_isrMaxGapMs = 0;
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

    static unsigned long lastStat = 0;
    if (millis() - lastStat > MQTT_PUBLISH_INTERVAL) {
        lastStat = millis();
        statusUpdate();          // keep local status fresh (HTTP consumers)
    }

    // 4. Regular maintenance tasks
    static unsigned long lastSerialOutput = 0;
    if (millis() - lastSerialOutput > 1000) {
        statusWriteToSerial();
        lastSerialOutput = millis();
    }
    // NOTE: relayControllerUpdate() + ledControllerUpdate() moved to the 50 ms
    // TC4 ISR - they keep running even while the TLS window blocks this loop.

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
                Serial.println("Reboot to apply new serial number to MQTT topics");
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
