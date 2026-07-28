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
static constexpr uint16_t TELEM_ATTEMPT_TIMEOUT_MS = 300;

static uint8_t  s_key[32];
static bool     s_haveKey   = false;
static uint32_t s_bootEpoch = 0;
static uint64_t s_msgSeq    = 0;

void telemetryInit() {
    s_haveKey = configGetDeviceKey(s_key);
    if (!s_haveKey) {
        Serial.println(F("[TELEM] no device key provisioned - use SET_KEY:<64 hex> in technician mode"));
        return;
    }
    // ONE flash write per boot; the epoch is the high half of the nonce so a
    // (epoch, seq) pair can never repeat even across power cycles.
    s_bootEpoch = configBumpTelemetryBootEpoch();
    s_msgSeq = 0;
    Serial.print(F("[TELEM] ready. bootEpoch=")); Serial.println(s_bootEpoch);
}

bool telemetryHasKey() { return s_haveKey; }

bool telemetrySendStatus() {
    if (!s_haveKey) return false;

    uint32_t t0 = bench::sendBegin();

    // ---- 1. build the packet on the STACK (no heap: proven wedge cause) ----
    // [header 29][ciphertext N][tag 16]
    static constexpr size_t BODY_MAX = 1200;
    uint8_t pkt[TELEM_HEADER_LEN + BODY_MAX + TELEM_TAG_LEN];

    // header: version | serial(16, NUL-padded) | nonce(12)
    pkt[0] = TELEM_VERSION;
    memset(pkt + 1, 0, TELEM_SERIAL_LEN);
    strncpy((char*)(pkt + 1), g_serialNumber.c_str(), TELEM_SERIAL_LEN);

    uint64_t seq = ++s_msgSeq;
    uint8_t* nonce = pkt + 1 + TELEM_SERIAL_LEN;
    nonce[0] = (uint8_t)(s_bootEpoch >> 24); nonce[1] = (uint8_t)(s_bootEpoch >> 16);
    nonce[2] = (uint8_t)(s_bootEpoch >> 8);  nonce[3] = (uint8_t)(s_bootEpoch);
    for (int i = 0; i < 8; i++) nonce[4 + i] = (uint8_t)(seq >> (56 - 8 * i));

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
    EthernetClient c;
    c.setConnectionTimeout(TELEM_ATTEMPT_TIMEOUT_MS);
    if (!c.connect(TELEM_HOST, TELEM_PORT)) {
        c.stop();
        bench::sendEnd(t0, false);
        return false;
    }

    char hdr[160];
    int hlen = snprintf(hdr, sizeof(hdr),
        "POST " TELEM_PATH " HTTP/1.1\r\nHost: " TELEM_HOST "\r\n"
        "Content-Type: application/octet-stream\r\n"
        "Content-Length: %u\r\nConnection: close\r\n\r\n", (unsigned)pktLen);
    c.write((const uint8_t*)hdr, hlen);
    c.write(pkt, pktLen);

    // read just enough of the reply to learn the status code
    bool ok = false;
    uint32_t waitStart = millis();
    while (c.connected() && !c.available() && millis() - waitStart < TELEM_ATTEMPT_TIMEOUT_MS) {
        /* spin briefly; capped */
    }
    if (c.available()) {
        char resp[16] = {0};
        int n = c.read((uint8_t*)resp, sizeof(resp) - 1);
        if (n > 12) ok = (resp[9] == '2');   // "HTTP/1.1 2xx"
    }
    c.stop();

    bench::sendEnd(t0, ok);
    return ok;
}
