/**
 * @file bench_aead.cpp
 * @brief STANDALONE benchmark firmware #1: telemetry protected with
 *        ChaCha20-Poly1305 over plain HTTP. Nothing else runs.
 *
 * This is the NEW method. It is deliberately a pure software test: no HTTP
 * server, no config manager, no status manager, no OCU, no sensors - so every
 * millisecond and every byte of RAM reported here belongs to the telemetry path
 * and nothing else. Its twin is bench_tls.cpp, which measures the OLD method
 * (mutual TLS to AWS IoT) with the exact same reporting format.
 *
 * Server: tools/ingest_test_server.py on the laptop (BENCH_HOST:BENCH_PORT).
 *
 * Build/flash:  pio run -e bench_aead -t upload
 * Watch:        pio device monitor -b 115200
 */
#include <KMPProDinoMKRZero.h>
#include <Ethernet.h>
#include <ArduinoJson.h>
#include <bearssl.h>

#include "bench_common.hpp"
#include "telemetry_key.h"      // gitignored: #define TELEM_KEY_HEX "<64 hex>"

#ifndef BENCH_HOST
#define BENCH_HOST "192.168.1.169"
#endif
#ifndef BENCH_PORT
#define BENCH_PORT 8088
#endif

static const char* SERIAL_NUMBER = "SN2003";

// Wire format (must match tools/ingest_test_server.py):
//   [0]      version = 0x02
//   [1..16]  serial (ASCII, NUL-padded)
//   [17..28] nonce = boot_id(8, random per boot) || msg_seq(4 BE)
//   [29..]   ciphertext, then the 16-byte Poly1305 tag
//   AAD      = bytes 0..28
static constexpr uint8_t VERSION    = 0x02;
static constexpr size_t  SERIAL_LEN = 16;
static constexpr size_t  HEADER_LEN = 29;
static constexpr size_t  TAG_LEN    = 16;

static uint8_t  s_key[32];
static uint8_t  s_bootId[8];          // random per boot, exactly like the firmware
static uint32_t s_msgSeq = 0;

static bench::Stats s_stats;

// ---------------------------------------------------------------------------

static bool hexToKey(const char* hex, uint8_t out[32]) {
    for (int i = 0; i < 32; i++) {
        int v = 0;
        for (int h = 0; h < 2; h++) {
            char ch = hex[i * 2 + h];
            int n = (ch >= '0' && ch <= '9') ? ch - '0'
                  : (ch >= 'a' && ch <= 'f') ? ch - 'a' + 10
                  : (ch >= 'A' && ch <= 'F') ? ch - 'A' + 10 : -1;
            if (n < 0) return false;
            v = (v << 4) | n;
        }
        out[i] = (uint8_t)v;
    }
    return true;
}

static void doSend() {
    static constexpr size_t BODY_MAX = 1200;
    uint8_t pkt[HEADER_LEN + BODY_MAX + TAG_LEN];      // STACK - zero heap

    // ---- header ----
    pkt[0] = VERSION;
    memset(pkt + 1, 0, SERIAL_LEN);
    strncpy((char*)(pkt + 1), SERIAL_NUMBER, SERIAL_LEN);
    uint32_t seq = ++s_msgSeq;
    uint8_t* nonce = pkt + 1 + SERIAL_LEN;
    memcpy(nonce, s_bootId, sizeof(s_bootId));
    for (int i = 0; i < 4; i++) nonce[8 + i] = (uint8_t)(seq >> (24 - 8 * i));

    uint32_t tTotal = millis();

    // ---- 1. build + encrypt (the CPU cost we are measuring) ----
    uint32_t tCrypto = micros();
    size_t bodyLen = bench::buildPayload((char*)(pkt + HEADER_LEN), BODY_MAX,
                                         SERIAL_NUMBER, "bench-aead", (uint32_t)seq);
    uint8_t* tag = pkt + HEADER_LEN + bodyLen;
    br_poly1305_ctmul32_run(s_key, nonce,
                            pkt + HEADER_LEN, bodyLen,     // encrypted in place
                            pkt, HEADER_LEN,               // AAD
                            tag, &br_chacha20_ct_run, 1);
    uint32_t cryptoUs = micros() - tCrypto;
    size_t pktLen = HEADER_LEN + bodyLen + TAG_LEN;

    // ---- 2. POST it ----
    uint32_t tNet = millis();
    bool ok = false;
    EthernetClient c;
    c.setConnectionTimeout(1000);
    if (c.connect(BENCH_HOST, BENCH_PORT)) {
        char hdr[160];
        int hlen = snprintf(hdr, sizeof(hdr),
            "POST /api/ingest HTTP/1.1\r\nHost: " BENCH_HOST "\r\n"
            "Content-Type: application/octet-stream\r\n"
            "Content-Length: %u\r\nConnection: close\r\n\r\n", (unsigned)pktLen);
        c.write((const uint8_t*)hdr, hlen);
        c.write(pkt, pktLen);

        uint32_t wait = millis();
        while (c.connected() && !c.available() && millis() - wait < 1000) { }
        if (c.available()) {
            char resp[20] = {0};
            int n = c.read((uint8_t*)resp, sizeof(resp) - 1);
            if (n > 12) {
                ok = (resp[9] == '2');
                if (!ok) { Serial.print(F("[AEAD] server said: ")); Serial.println(&resp[9]); }
            }
        } else {
            Serial.println(F("[AEAD] no reply within 1000 ms"));
        }
    } else {
        Serial.println(F("[AEAD] TCP connect FAILED - is ingest_test_server.py running?"));
    }
    c.stop();
    uint32_t netMs = millis() - tNet;
    uint32_t totalMs = millis() - tTotal;

    s_stats.record(totalMs, cryptoUs / 1000, netMs, pktLen, ok);
    s_stats.printSend("AEAD", totalMs, cryptoUs, netMs, pktLen, ok);
}

// ---------------------------------------------------------------------------

void setup() {
    Serial.begin(115200);
    uint32_t t0 = millis();
    while (!Serial && millis() - t0 < 3000) { }

    Serial.println(F("\n================================================================"));
    Serial.println(F("  BENCH #1: AEAD (ChaCha20-Poly1305 over plain HTTP)"));
    Serial.println(F("  standalone - no HTTP server, no sensors, no config"));
    Serial.println(F("================================================================"));
    Serial.print(F("[AEAD] freeRam at boot: ")); Serial.println(bench::freeRam());

    KMPProDinoMKRZero.init(ProDino_MKR_Zero_Ethernet);

    byte mac[] = {0x00, 0x08, 0xDC, 0x53, 0x09, 0x72};
    Serial.println(F("[AEAD] DHCP..."));
    if (Ethernet.begin(mac) == 0) {
        Serial.println(F("[AEAD] no DHCP -> static 192.168.1.198"));
        Ethernet.begin(mac, IPAddress(192,168,1,198), IPAddress(8,8,8,8),
                       IPAddress(192,168,1,1), IPAddress(255,255,255,0));
    }
    delay(1000);
    Serial.print(F("[AEAD] IP: ")); Serial.println(Ethernet.localIP());
    Serial.print(F("[AEAD] server: ")); Serial.print(F(BENCH_HOST));
    Serial.print(':'); Serial.println(BENCH_PORT);

    // --- connectivity probe: which of host / port is the problem? ---
    struct { const char* host; uint16_t port; } probes[] = {
        { BENCH_HOST, BENCH_PORT },   // the receiver we actually want
        { "192.168.1.1", 80 },        // the router: proves outbound TCP works at all
        { BENCH_HOST, 22 },   { BENCH_HOST, 80 },   { BENCH_HOST, 1883 },
        { BENCH_HOST, 3000 }, { BENCH_HOST, 5173 }, { BENCH_HOST, 7070 },
        { BENCH_HOST, 8000 }, { BENCH_HOST, 1935 },
    };
    for (auto& p : probes) {
        EthernetClient probe;
        probe.setConnectionTimeout(2000);
        uint32_t t = millis();
        int r = probe.connect(p.host, p.port);
        Serial.print(F("[PROBE] ")); Serial.print(p.host); Serial.print(':');
        Serial.print(p.port); Serial.print(r ? F("  OK  ") : F("  FAIL "));
        Serial.print(millis() - t); Serial.println(F(" ms"));
        probe.stop();
    }

    if (!hexToKey(TELEM_KEY_HEX, s_key)) {
        Serial.println(F("[AEAD] FATAL: bad TELEM_KEY_HEX"));
        while (1) { }
    }
    for (size_t i = 0; i < sizeof(s_bootId); i++) {
        s_bootId[i] = (uint8_t)(analogRead(A5) ^ micros());   // bench-grade entropy
    }
    Serial.print(F("[AEAD] key loaded, bootId="));
    for (size_t i = 0; i < sizeof(s_bootId); i++) {
        if (s_bootId[i] < 0x10) Serial.print('0');
        Serial.print(s_bootId[i], HEX);
    }
    Serial.println();
    Serial.print(F("[AEAD] freeRam after init: ")); Serial.println(bench::freeRam());
    Serial.print(F("[AEAD] send interval: ")); Serial.print(BENCH_INTERVAL_MS);
    Serial.println(F(" ms\n"));
    s_stats.begin();
}

void loop() {
    s_stats.loopTick();          // measures the longest blocking stretch of loop()

    static uint32_t last = 0;
    if (millis() - last >= BENCH_INTERVAL_MS) {
        last = millis();
        doSend();
        if (s_stats.sends % 10 == 0) s_stats.printSummary("AEAD");
    }
}
