/**
 * @file secure_telemetry.cpp
 * @brief ChaCha20-Poly1305 protected telemetry over plain HTTP POST.
 *        See secure_telemetry.hpp for the wire format and the nonce-safety rule.
 */
#include "secure_telemetry.hpp"
#include "config_manager.hpp"
#include "status_manager.hpp"
#include "telemetry_bench.hpp"

#include <Ethernet.h>
#include <ArduinoJson.h>
#include <bearssl.h>

// Optional compiled-in key for bench builds. telemetry_key.h is GITIGNORED and
// holds a single line:  #define TELEM_KEY_HEX "<64 hex chars>"
// Generate it with:  python3 tools/gen_device_key.py SN2003 --embed
// Flash provisioning (SET_KEY:) always wins over this - the embedded key is only
// a fallback so a board can be benchmarked without a technician step. Production
// firmware ships WITHOUT this header, so every board needs its own SET_KEY.
#if defined(__has_include)
#  if __has_include("telemetry_key.h")
#    include "telemetry_key.h"
#  endif
#endif

/// Decode 64 hex chars into 32 bytes. Returns false on any non-hex character.
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

// Where the encrypted telemetry goes. During the Phase-0 benchmark this is the
// laptop running tools/ingest_test_server.py; in production it is the EC2
// dashboard server.
#ifndef TELEM_HOST
#define TELEM_HOST "192.168.1.33"
#endif
#ifndef TELEM_PORT
#define TELEM_PORT 8088
#endif
#ifndef TELEM_PATH
#define TELEM_PATH "/api/ingest"
#endif

/// Whole-attempt cap. Telemetry is LOWER priority than the HTTP API: if the
/// server is slow or gone we give up quickly rather than stalling loop().
#ifndef TELEM_TIMEOUT_MS
#define TELEM_TIMEOUT_MS 300
#endif
static constexpr uint16_t TELEM_ATTEMPT_TIMEOUT_MS = TELEM_TIMEOUT_MS;

static uint8_t  s_key[32];
static bool     s_haveKey = false;

// A send is split in two: the blocking head (build + encrypt + connect + write)
// runs in telemetrySendStatus(), and the tail (wait for the reply, close the
// socket) is pumped from loop() so two ~80 ms round trips never block the board.
static EthernetClient s_client;
static bool     s_pending      = false;
static uint32_t s_pendingStart = 0;
static uint32_t s_pendingT0    = 0;
static uint32_t s_profCrypto   = 0;
static uint32_t s_profConnect  = 0;
static uint8_t  s_bootId[TELEM_BOOT_ID_LEN];
static uint32_t s_msgSeq  = 0;

/**
 * Draw a fresh 64-bit boot id. This replaces the flash-stored boot counter,
 * which every firmware upload erased (FlashStorage lives inside the sketch
 * image) - see the header for why that broke reflashed boards.
 *
 * Entropy is hashed from three independent sources so that a weak one cannot
 * sink the result:
 *   - ADC noise on a floating pin (the low bits are genuinely noisy),
 *   - micros() sampled inside the loop (ADC conversion jitter),
 *   - the elapsed time to reach this point, which varies by milliseconds every
 *     boot because IMU calibration and DHCP never take exactly as long.
 * The device key is mixed in too, so two boards can never draw the same id even
 * if they somehow saw identical entropy.
 */
static void drawBootId(uint8_t out[TELEM_BOOT_ID_LEN]) {
    br_sha256_context ctx;
    br_sha256_init(&ctx);
    br_sha256_update(&ctx, s_key, sizeof(s_key));

    for (int i = 0; i < 128; i++) {
        uint16_t adc = (uint16_t)analogRead(A5);
        uint32_t t   = micros();
        br_sha256_update(&ctx, &adc, sizeof(adc));
        br_sha256_update(&ctx, &t, sizeof(t));
    }

    uint32_t upMicros = micros(), upMillis = millis();
    br_sha256_update(&ctx, &upMicros, sizeof(upMicros));
    br_sha256_update(&ctx, &upMillis, sizeof(upMillis));

    uint8_t digest[32];
    br_sha256_out(&ctx, digest);
    memcpy(out, digest, TELEM_BOOT_ID_LEN);
}

void telemetryInit() {
    s_haveKey = configGetDeviceKey(s_key);
    if (s_haveKey) {
        Serial.println(F("[TELEM] key source: flash (SET_KEY)"));
    }
#ifdef TELEM_KEY_HEX
    else if (hexToKey(TELEM_KEY_HEX, s_key)) {
        s_haveKey = true;
        Serial.println(F("[TELEM] key source: COMPILED-IN (bench build - not for production)"));
    }
#endif
    if (!s_haveKey) {
        Serial.println(F("[TELEM] no device key provisioned - use SET_KEY:<64 hex> in technician mode"));
        return;
    }
    // No flash write, nothing for a firmware upload to erase.
    drawBootId(s_bootId);
    s_msgSeq = 0;
    Serial.print(F("[TELEM] ready. bootId="));
    for (size_t i = 0; i < TELEM_BOOT_ID_LEN; i++) {
        if (s_bootId[i] < 0x10) Serial.print('0');
        Serial.print(s_bootId[i], HEX);
    }
    Serial.println();
}

bool telemetryHasKey() { return s_haveKey; }

bool telemetrySendStatus() {
    if (!s_haveKey) return false;
    if (s_pending) {                  // previous send not finished - skip this tick
        Serial.println(F("[TELEM] previous send still in flight, skipping"));
        return false;
    }

    uint32_t t0 = bench::sendBegin();

    // ---- 1. build the packet on the STACK (no heap: proven wedge cause) ----
    // [header 29][ciphertext N][tag 16]
    static constexpr size_t BODY_MAX = 1200;
    uint8_t pkt[TELEM_HEADER_LEN + BODY_MAX + TELEM_TAG_LEN];

    // header: version | serial(16, NUL-padded) | nonce(12)
    pkt[0] = TELEM_VERSION;
    memset(pkt + 1, 0, TELEM_SERIAL_LEN);
    strncpy((char*)(pkt + 1), g_serialNumber.c_str(), TELEM_SERIAL_LEN);

    uint32_t seq = ++s_msgSeq;
    uint8_t* nonce = pkt + 1 + TELEM_SERIAL_LEN;
    memcpy(nonce, s_bootId, TELEM_BOOT_ID_LEN);
    for (int i = 0; i < 4; i++) nonce[TELEM_BOOT_ID_LEN + i] = (uint8_t)(seq >> (24 - 8 * i));

    // payload: the same status JSON the HTTP API serves, serialized straight
    // into the packet buffer (no String, no intermediate copy)
    JsonDocument doc = statusGenerateJsonSimple();
    size_t bodyLen = serializeJson(doc, (char*)(pkt + TELEM_HEADER_LEN), BODY_MAX);
    if (bodyLen == 0 || bodyLen >= BODY_MAX) {
        bench::sendEnd(t0, false);
        return false;
    }

    // ---- 2. encrypt + authenticate in ONE call (AAD = the header) ----
    uint8_t* tag = pkt + TELEM_HEADER_LEN + bodyLen;
    br_poly1305_ctmul32_run(s_key, nonce,
                            pkt + TELEM_HEADER_LEN, bodyLen,   // data (in place)
                            pkt, TELEM_HEADER_LEN,             // AAD
                            tag, &br_chacha20_ct_run, 1 /*encrypt*/);
    size_t pktLen = TELEM_HEADER_LEN + bodyLen + TELEM_TAG_LEN;

    // ---- 3. POST it (short timeout, single attempt, never blocks long) ----
    uint32_t tCrypto = millis();          // [PROFILE] build+encrypt done here
    EthernetClient& c = s_client;
    c.setConnectionTimeout(TELEM_ATTEMPT_TIMEOUT_MS);
    if (!c.connect(TELEM_HOST, TELEM_PORT)) {
        c.stop();
        Serial.println(F("[TELEM] connect FAILED (receiver down / wrong host / firewall)"));
        bench::sendEnd(t0, false);
        return false;
    }

    uint32_t tConnect = millis();         // [PROFILE] TCP handshake done here

    char hdr[160];
    int hlen = snprintf(hdr, sizeof(hdr),
        "POST " TELEM_PATH " HTTP/1.1\r\nHost: " TELEM_HOST "\r\n"
        "Content-Type: application/octet-stream\r\n"
        "Content-Length: %u\r\nConnection: close\r\n\r\n", (unsigned)pktLen);
    c.write((const uint8_t*)hdr, hlen);
    c.write(pkt, pktLen);

    // The request is on the wire. Waiting for the reply and for the socket to
    // close costs two more round trips (~80 ms each) that the board would just
    // spin through - so hand them to telemetryPump() and return to loop() now.
    s_pending = true;
    s_pendingStart = millis();
    s_pendingT0 = t0;
    s_profConnect = tConnect - tCrypto;
    s_profCrypto = tCrypto - t0;

    Serial.print(F("[PROFILE] blocking: crypto=")); Serial.print(s_profCrypto);
    Serial.print(F(" connect=")); Serial.print(s_profConnect);
    Serial.print(F(" write=")); Serial.print(millis() - tConnect);
    Serial.println(F(" ms (reply+close deferred)"));
    return true;
}

void telemetryPump() {
    if (!s_pending) return;

    bool done = false, ok = false;

    if (s_client.available()) {
        char resp[16] = {0};
        int n = s_client.read((uint8_t*)resp, sizeof(resp) - 1);
        if (n > 12) {
            ok = (resp[9] == '2');           // "HTTP/1.1 2xx"
            if (!ok) {
                Serial.print(F("[TELEM] server rejected: "));
                Serial.println(&resp[9]);    // 401 = bad auth, 409 = replay
            }
        }
        done = true;
    } else if (!s_client.connected()) {
        Serial.println(F("[TELEM] connection closed with no reply"));
        done = true;
    } else if (millis() - s_pendingStart > TELEM_ATTEMPT_TIMEOUT_MS) {
        Serial.println(F("[TELEM] no reply within timeout"));
        done = true;
    }

    if (!done) return;                       // still waiting; costs ~0 per loop

    // stop() waits up to its connection timeout for the FIN handshake to
    // complete - another ~80 ms round trip, which showed up as a stall even out
    // here in the pump. The request is already delivered and answered, so drop
    // the timeout to 1 ms: stop() then force-closes the socket immediately.
    s_client.setConnectionTimeout(1);
    s_client.stop();
    s_pending = false;

    Serial.print(F("[PROFILE] total=")); Serial.print(millis() - s_pendingT0);
    Serial.print(F(" ms (blocking was crypto=")); Serial.print(s_profCrypto);
    Serial.print(F(" + connect=")); Serial.print(s_profConnect);
    Serial.println(F(")"));

    bench::sendEnd(s_pendingT0, ok);
}
