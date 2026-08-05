/**
 * @file bench_send.cpp
 * @brief STANDALONE: how long does ONE telemetry send actually block the loop?
 *        Repeats it 1000 times and reports the whole distribution.
 *
 * The production firmware only reports min/avg/max, which cannot tell "every
 * send is 18 ms except one bad one" apart from "sends are all over the place".
 * This runs the identical send path - same packet, same persistent connection,
 * same deferred reply - with nothing else on the board, and prints a histogram.
 *
 * Blocking time is measured exactly as production measures it: from the start
 * of the send to the moment it hands control back to loop(). Waiting for the
 * server's reply is NOT included, because that happens in the pump and blocks
 * nothing - the same reason it is excluded in the real firmware.
 *
 * Build/flash:  pio run -e bench_send -t upload
 * Watch:        python3 tools/capture_serial.py 400
 */
#include <KMPProDinoMKRZero.h>
#include <Ethernet.h>
#include <ArduinoJson.h>
#include <bearssl.h>

#include "bench_common.hpp"
#include "telemetry_key.h"      // gitignored: #define TELEM_KEY_HEX "<64 hex>"

#ifndef BENCH_HOST
#define BENCH_HOST "16.171.11.151"
#endif
#ifndef BENCH_PORT
#define BENCH_PORT 5555
#endif
#ifndef BENCH_SENDS
#define BENCH_SENDS 1000
#endif
#ifndef BENCH_GAP_MS
#define BENCH_GAP_MS 200        // gap between sends; keeps a 1000-run under 4 min
#endif

static const char* SERIAL_NUMBER = "SN2003";

static constexpr uint8_t VERSION    = 0x02;
static constexpr size_t  SERIAL_LEN = 16;
static constexpr size_t  HEADER_LEN = 29;
static constexpr size_t  TAG_LEN    = 16;

static uint8_t  s_key[32];
static uint8_t  s_bootId[8];
static uint32_t s_msgSeq = 0;

static EthernetClient s_client;
static bool     s_pending = false;
static uint32_t s_pendingStart = 0;

// 1 ms-resolution histogram. Exact percentiles and exact counts come straight
// out of it, and it costs ~1 KB instead of storing 1000 samples.
static constexpr uint16_t HIST_MAX = 512;
static uint16_t s_hist[HIST_MAX + 1];      // [HIST_MAX] = everything >= 512 ms
static uint32_t s_n = 0, s_fails = 0, s_reconnects = 0;
static uint32_t s_sum = 0, s_min = 0xFFFFFFFF, s_max = 0;
static uint32_t s_maxBuild = 0, s_maxReconn = 0, s_maxWrite = 0;
static bool     s_done = false;

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

/// Value at percentile q (0..1), read straight off the histogram.
static uint32_t pct(float q) {
    uint32_t target = (uint32_t)(s_n * q);
    uint32_t seen = 0;
    for (uint16_t i = 0; i <= HIST_MAX; i++) {
        seen += s_hist[i];
        if (seen >= target) return i;
    }
    return HIST_MAX;
}

static uint32_t countRange(uint16_t lo, uint16_t hi) {
    uint32_t c = 0;
    for (uint16_t i = lo; i <= hi && i <= HIST_MAX; i++) c += s_hist[i];
    return c;
}

static void printBar(const char* label, uint32_t c) {
    Serial.print(F("   ")); Serial.print(label);
    Serial.print(F(" : ")); Serial.print(c);
    Serial.print(F("  ("));
    Serial.print(s_n ? (c * 1000UL / s_n) / 10.0f : 0.0f, 1);
    Serial.print(F("%)  "));
    uint32_t bars = s_n ? (c * 40UL / s_n) : 0;
    for (uint32_t i = 0; i < bars; i++) Serial.print('#');
    Serial.println();
}

static void report() {
    Serial.println(F("\n================================================================"));
    Serial.print(F("  BLOCKING TIME PER SEND, n=")); Serial.print(s_n);
    Serial.print(F("   failures=")); Serial.print(s_fails);
    Serial.print(F("   reconnects=")); Serial.println(s_reconnects);
    Serial.println(F("================================================================"));

    Serial.print(F("  min=")); Serial.print(s_min);
    Serial.print(F("  p50=")); Serial.print(pct(0.50f));
    Serial.print(F("  p90=")); Serial.print(pct(0.90f));
    Serial.print(F("  p95=")); Serial.print(pct(0.95f));
    Serial.print(F("  p99=")); Serial.print(pct(0.99f));
    Serial.print(F("  max=")); Serial.print(s_max);
    Serial.print(F("  mean=")); Serial.print(s_n ? (float)s_sum / s_n : 0.0f, 1);
    Serial.println(F(" ms"));

    Serial.println(F("\n  distribution:"));
    printBar("<= 18 ms  ", countRange(0, 18));
    printBar("19 - 20   ", countRange(19, 20));
    printBar("21 - 30   ", countRange(21, 30));
    printBar("31 - 50   ", countRange(31, 50));
    printBar("51 - 100  ", countRange(51, 100));
    printBar("101 - 300 ", countRange(101, 300));
    printBar("> 300     ", countRange(301, HIST_MAX));

    Serial.println(F("\n  exact counts, every value that occurred:"));
    for (uint16_t i = 0; i <= HIST_MAX; i++) {
        if (!s_hist[i]) continue;
        Serial.print(F("   "));
        if (i == HIST_MAX) Serial.print(F(">=512")); else Serial.print(i);
        Serial.print(F(" ms x ")); Serial.println(s_hist[i]);
    }

    Serial.print(F("\n  worst phase: build=")); Serial.print(s_maxBuild);
    Serial.print(F("  reconnect=")); Serial.print(s_maxReconn);
    Serial.print(F("  write=")); Serial.print(s_maxWrite);
    Serial.println(F(" ms"));
    Serial.print(F("  freeRam=")); Serial.println(bench::freeRam());
    Serial.println(F("================================================================"));
}

// ---------------------------------------------------------------------------

static void doSend() {
    uint32_t t0 = millis();

    static constexpr size_t BODY_MAX = 1200;
    uint8_t pkt[HEADER_LEN + BODY_MAX + TAG_LEN];

    pkt[0] = VERSION;
    memset(pkt + 1, 0, SERIAL_LEN);
    strncpy((char*)(pkt + 1), SERIAL_NUMBER, SERIAL_LEN);
    uint32_t seq = ++s_msgSeq;
    uint8_t* nonce = pkt + 1 + SERIAL_LEN;
    memcpy(nonce, s_bootId, sizeof(s_bootId));
    for (int i = 0; i < 4; i++) nonce[8 + i] = (uint8_t)(seq >> (24 - 8 * i));

    size_t bodyLen = bench::buildPayload((char*)(pkt + HEADER_LEN), BODY_MAX,
                                         SERIAL_NUMBER, "bench-send", seq);
    uint8_t* tag = pkt + HEADER_LEN + bodyLen;
    br_poly1305_ctmul32_run(s_key, nonce, pkt + HEADER_LEN, bodyLen,
                            pkt, HEADER_LEN, tag, &br_chacha20_ct_run, 1);
    size_t pktLen = HEADER_LEN + bodyLen + TAG_LEN;

    uint32_t tBuild = millis();
    if (tBuild - t0 > s_maxBuild) s_maxBuild = tBuild - t0;

    // persistent connection, exactly as the production firmware does it
    uint32_t tConn = tBuild;
    if (!s_client.connected()) {
        s_reconnects++;
        s_client.stop();
        s_client.setConnectionTimeout(2000);
        if (!s_client.connect(BENCH_HOST, BENCH_PORT)) {
            s_client.stop();
            s_fails++;
            return;
        }
        tConn = millis();
        if (tConn - tBuild > s_maxReconn) s_maxReconn = tConn - tBuild;
    }

    uint32_t tWr = millis();
    char hdr[160];
    int hlen = snprintf(hdr, sizeof(hdr),
        "POST /api/ingest HTTP/1.1\r\nHost: " BENCH_HOST "\r\n"
        "Content-Type: application/octet-stream\r\n"
        "Content-Length: %u\r\nConnection: keep-alive\r\n\r\n", (unsigned)pktLen);
    s_client.write((const uint8_t*)hdr, hlen);
    s_client.write(pkt, pktLen);
    uint32_t wr = millis() - tWr;
    if (wr > s_maxWrite) s_maxWrite = wr;

    s_pending = true;
    s_pendingStart = millis();

    // THIS is the number: everything above blocked loop(); the reply does not.
    uint32_t blk = millis() - t0;
    s_hist[blk < HIST_MAX ? blk : HIST_MAX]++;
    s_sum += blk; s_n++;
    if (blk < s_min) s_min = blk;
    if (blk > s_max) s_max = blk;

    if (s_n % 100 == 0) {
        Serial.print(F("[BENCH] ")); Serial.print(s_n);
        Serial.print(F("/")); Serial.print((uint32_t)BENCH_SENDS);
        Serial.print(F("  last=")); Serial.print(blk);
        Serial.print(F(" ms  running max=")); Serial.print(s_max);
        Serial.print(F("  reconnects=")); Serial.println(s_reconnects);
    }
}

static void pump() {
    if (!s_pending) return;
    if (s_client.available()) {
        uint8_t sink[64];
        while (s_client.available()) s_client.read(sink, sizeof(sink));
        s_pending = false;
    } else if (!s_client.connected()) {
        s_client.setConnectionTimeout(1);
        s_client.stop();
        s_pending = false;
    } else if (millis() - s_pendingStart > 2000) {
        s_pending = false;
    }
}

// ---------------------------------------------------------------------------

void setup() {
    Serial.begin(115200);
    uint32_t t0 = millis();
    while (!Serial && millis() - t0 < 3000) { }

    Serial.println(F("\n================================================================"));
    Serial.print(F("  bench_send: ")); Serial.print((uint32_t)BENCH_SENDS);
    Serial.println(F(" telemetry sends, nothing else running"));
    Serial.println(F("================================================================"));

    KMPProDinoMKRZero.init(ProDino_MKR_Zero_Ethernet);

    byte mac[] = {0x00, 0x08, 0xDC, 0x53, 0x09, 0x72};
    if (Ethernet.begin(mac) == 0) {
        Ethernet.begin(mac, IPAddress(192,168,1,198), IPAddress(8,8,8,8),
                       IPAddress(192,168,1,1), IPAddress(255,255,255,0));
    }
    delay(1000);
    Serial.print(F("[BENCH] IP: ")); Serial.print(Ethernet.localIP());
    Serial.print(F("  ->  ")); Serial.print(F(BENCH_HOST));
    Serial.print(':'); Serial.println(BENCH_PORT);

    if (!hexToKey(TELEM_KEY_HEX, s_key)) {
        Serial.println(F("[BENCH] FATAL: bad TELEM_KEY_HEX"));
        while (1) { }
    }
    for (size_t i = 0; i < sizeof(s_bootId); i++) {
        s_bootId[i] = (uint8_t)(analogRead(A5) ^ micros());
    }
    Serial.print(F("[BENCH] gap between sends: ")); Serial.print((uint32_t)BENCH_GAP_MS);
    Serial.println(F(" ms\n"));
}

void loop() {
    pump();

    if (s_done) return;

    static uint32_t last = 0;
    if (millis() - last >= (uint32_t)BENCH_GAP_MS) {
        last = millis();
        doSend();
        if (s_n >= (uint32_t)BENCH_SENDS) {
            s_done = true;
            report();
        }
    }
}
