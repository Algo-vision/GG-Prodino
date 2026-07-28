/**
 * @file bench_tls.cpp
 * @brief STANDALONE benchmark firmware #2: telemetry over mutual TLS to AWS IoT.
 *        Nothing else runs. Twin of bench_aead.cpp - identical reporting.
 *
 * This is the OLD/current method. Like its twin it is a pure software test: no
 * HTTP server, no config manager, no sensors - so the numbers belong entirely to
 * the telemetry path.
 *
 * Server: AWS IoT Core (the endpoint in aws_certs_store.h).
 *
 * One cycle mirrors exactly what bench_aead does per cycle - connect, send one
 * payload, disconnect - so "total ms" means the same thing in both logs. The
 * handshake is reported separately in the crypto column because it IS the cost:
 * AWS IoT does not support TLS session resumption, so every reconnect pays it in
 * full. A second, WARM publish on the still-open session is also timed, to show
 * the best case a persistent session could ever give.
 *
 * Build/flash:  pio run -e bench_tls -t upload
 * Watch:        pio device monitor -b 115200
 */
#include <KMPProDinoMKRZero.h>
#include <Ethernet.h>
#include <SSLClient.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

#include "bench_common.hpp"
#include "aws_certs_store.h"    // AWS_ENDPOINT, AWS_PORT, AWS_DEV_CERT, AWS_DEV_KEY
#include "aws_root_ca.h"        // TAs, TAs_NUM

static const char* SERIAL_NUMBER = "SN2003";

static EthernetClient s_eth;
static SSLClient      s_ssl(s_eth, TAs, TAs_NUM, A5);

/// PubSubClient calls flush() after every packet, but SSLClient::flush() waits
/// for BR_SSL_RECVAPP - data AWS never sends back for a QoS-0 PUBLISH - so it
/// blocks ~30 s and then latches a write error. Pumping available() instead
/// encrypts and ships the record without waiting. This wrapper is the fix that
/// made board-side TLS work at all; do NOT call SSLClient::flush() here.
class PacketClient : public Client {
    SSLClient& c;
public:
    PacketClient(SSLClient& s) : c(s) {}
    int connect(IPAddress ip, uint16_t p) override { return c.connect(ip, p); }
    int connect(const char* h, uint16_t p) override { return c.connect(h, p); }
    size_t write(uint8_t b) override { return c.write(b); }
    size_t write(const uint8_t* b, size_t s) override {
        size_t n = c.write(b, s);
        c.available();
        return n;
    }
    int available() override { return c.available(); }
    int read() override { return c.read(); }
    int read(uint8_t* b, size_t s) override { return c.read(b, s); }
    int peek() override { return c.peek(); }
    void flush() override { c.available(); }
    void stop() override { c.stop(); }
    uint8_t connected() override { return c.connected(); }
    operator bool() override { return (bool)c; }
};
static PacketClient s_packet(s_ssl);
static PubSubClient s_mqtt(s_packet);
static SSLClientParameters s_mTLS =
    SSLClientParameters::fromPEM(AWS_DEV_CERT, strlen(AWS_DEV_CERT),
                                 AWS_DEV_KEY,  strlen(AWS_DEV_KEY));

static bench::Stats s_stats;
static uint32_t s_seq = 0;
static uint32_t s_connAttempt = 0;

// ---------------------------------------------------------------------------

/// Publish one payload; returns the wall time in ms, or 0 on failure.
static uint32_t publishOne(char* buf, size_t cap, size_t* outLen) {
    char topic[64];
    snprintf(topic, sizeof(topic), "grk/%s/status", SERIAL_NUMBER);
    size_t len = bench::buildPayload(buf, cap, SERIAL_NUMBER, "bench-tls", ++s_seq);
    *outLen = len;
    uint32_t t = millis();
    // streamed publish: no String, no 1 KB copy inside PubSubClient
    if (!s_mqtt.beginPublish(topic, len, false)) return 0;
    s_mqtt.write((const uint8_t*)buf, len);
    if (!s_mqtt.endPublish()) return 0;
    uint32_t dt = millis() - t;
    return dt == 0 ? 1 : dt;      // 0 is our failure sentinel
}

static void doCycle() {
    uint32_t tTotal = millis();
    bool ok = false;
    size_t bytes = 0;
    uint32_t handshakeMs = 0, pubMs = 0, warmMs = 0;

    // ---- 1. connect: TCP + TLS handshake + MQTT CONNECT (the CPU cost) ----
    char cid[32];
    snprintf(cid, sizeof(cid), "grk-bench-%lu", (unsigned long)s_connAttempt++);
    Serial.print(F("[TLS] connecting ")); Serial.print(cid);
    Serial.print(F(" ... freeRam=")); Serial.println(bench::freeRam());

    uint32_t tHs = millis();
    bool connected = s_mqtt.connect(cid);
    handshakeMs = millis() - tHs;

    if (!connected) {
        Serial.print(F("[TLS] connect FAILED rc=")); Serial.print(s_mqtt.state());
        Serial.print(F(" after ")); Serial.print(handshakeMs); Serial.println(F(" ms"));
        s_ssl.stop();                      // clear SSLClient's latched write error
        uint32_t totalMs = millis() - tTotal;
        s_stats.record(totalMs, handshakeMs, 0, 0, false);
        s_stats.printSend("TLS", totalMs, handshakeMs * 1000, 0, 0, false);
        return;
    }
    Serial.print(F("[TLS] handshake+CONNECT = ")); Serial.print(handshakeMs);
    Serial.print(F(" ms  freeRam=")); Serial.println(bench::freeRam());

    // ---- 2. publish one payload (cold: first record on a fresh session) ----
    {
        char buf[1200];
        pubMs = publishOne(buf, sizeof(buf), &bytes);
        ok = (pubMs != 0);

        // ---- 3. a SECOND publish on the still-open session (warm best case) ----
        if (ok) {
            size_t dummy;
            warmMs = publishOne(buf, sizeof(buf), &dummy);
        }
    }

    // ---- 4. tear down, exactly like the AEAD twin closes its TCP socket ----
    s_mqtt.disconnect();
    s_ssl.stop();

    uint32_t totalMs = millis() - tTotal;
    s_stats.record(totalMs, handshakeMs, pubMs, bytes, ok);
    s_stats.printSend("TLS", totalMs, handshakeMs * 1000UL, pubMs, bytes, ok);
    Serial.print(F("[TLS]   (warm publish on the open session = "));
    Serial.print(warmMs); Serial.println(F(" ms - best case if the session is held)"));
}

// ---------------------------------------------------------------------------

void setup() {
    Serial.begin(115200);
    uint32_t t0 = millis();
    while (!Serial && millis() - t0 < 3000) { }

    Serial.println(F("\n================================================================"));
    Serial.println(F("  BENCH #2: TLS (mutual TLS + MQTT to AWS IoT)"));
    Serial.println(F("  standalone - no HTTP server, no sensors, no config"));
    Serial.println(F("================================================================"));
    Serial.print(F("[TLS] freeRam at boot: ")); Serial.println(bench::freeRam());

    KMPProDinoMKRZero.init(ProDino_MKR_Zero_Ethernet);

    byte mac[] = {0x00, 0x08, 0xDC, 0x53, 0x09, 0x72};
    Serial.println(F("[TLS] DHCP..."));
    if (Ethernet.begin(mac) == 0) {
        Serial.println(F("[TLS] no DHCP -> static 192.168.1.198"));
        Ethernet.begin(mac, IPAddress(192,168,1,198), IPAddress(8,8,8,8),
                       IPAddress(192,168,1,1), IPAddress(255,255,255,0));
    }
    delay(1000);
    Serial.print(F("[TLS] IP: ")); Serial.println(Ethernet.localIP());
    Serial.print(F("[TLS] server: ")); Serial.print(AWS_ENDPOINT);
    Serial.print(':'); Serial.println(AWS_PORT);

    s_ssl.setMutualAuthParams(s_mTLS);
    s_mqtt.setServer(AWS_ENDPOINT, AWS_PORT);
    s_mqtt.setBufferSize(256);
    s_mqtt.setKeepAlive(1200);
    s_mqtt.setSocketTimeout(15);

    Serial.print(F("[TLS] freeRam after init: ")); Serial.println(bench::freeRam());
    Serial.print(F("[TLS] send interval: ")); Serial.print(BENCH_INTERVAL_MS);
    Serial.println(F(" ms\n"));
    s_stats.begin();
}

void loop() {
    s_stats.loopTick();          // measures the longest blocking stretch of loop()

    static uint32_t last = 0;
    if (millis() - last >= BENCH_INTERVAL_MS) {
        last = millis();
        doCycle();
        if (s_stats.sends % 10 == 0) s_stats.printSummary("TLS");
    }
}
