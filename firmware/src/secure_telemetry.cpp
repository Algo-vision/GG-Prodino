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
#include <EthernetUdp.h>
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

// Transport. UDP is the better fit and is much cheaper on the board: a TCP POST
// costs three ~80 ms round trips (connect, reply, close), and the connect alone
// blocks loop() for 78 ms of the 97 ms total. UDP has no handshake, so a send is
// just the crypto plus the write - about 19 ms.
//
// Nothing is lost by dropping TCP: the packet is already self-contained and
// authenticated, so it needs neither ordering nor a stream, and a lost datagram
// only costs one status update (the server sees the gap in msg_seq).
//
// Default is still TCP because UDP must be opened in the EC2 security group
// first; flip this to 1 once inbound UDP on TELEM_PORT is allowed.
#ifndef TELEM_USE_UDP
#define TELEM_USE_UDP 0
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
#if TELEM_USE_UDP
static EthernetUDP s_udp;
static IPAddress   s_serverIp;
#else
static EthernetClient s_client;
#endif
static bool     s_pending      = false;
static uint32_t s_pendingStart = 0;
static uint32_t s_pendingT0    = 0;
static uint32_t s_profCrypto   = 0;
static uint32_t s_profConnect  = 0;

// Per-send blocking time, accumulated. The [PROFILE] line is printed per send,
// but a load test has to run with no serial monitor attached (an attached
// monitor blocks on USB CDC and wrecks the very HTTP throughput being measured),
// so those lines go nowhere. These survive to be read afterwards, and answer
// "was every send cheap, or only most of them?" rather than just reporting the
// worst one.
static uint32_t s_blkMin = 0xFFFFFFFF, s_blkMax = 0, s_blkSum = 0, s_blkN = 0;

// Worst case of each phase of a send, so a slow one can be attributed rather
// than guessed at. Under a saturating HTTP load the send competes with the API
// for the same W5500 - these say exactly where that shows up.
static uint32_t s_maxBuild   = 0;   // status JSON + ChaCha20-Poly1305
static uint32_t s_maxReconn  = 0;   // stop() + connect(), 0 while the link holds
static uint32_t s_maxWrite   = 0;   // handing the bytes to the W5500
static uint32_t s_reconnects = 0;

// The last N blocking times, so a short run can be read send-by-send instead of
// inferred from min/avg/max - "how many were above 18 ms" should be countable,
// not deduced.
static constexpr uint8_t RECENT_N = 32;
static uint16_t s_recent[RECENT_N];
static uint8_t  s_recentIdx = 0;
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
#if TELEM_USE_UDP
    // One local port for the whole run - no per-send socket setup, and the
    // server's reply comes back here.
    s_udp.begin(TELEM_PORT);
    s_serverIp.fromString(TELEM_HOST);   // dotted-quad: no DNS round trip
    Serial.print(F("[TELEM] transport: UDP -> ")); Serial.print(s_serverIp);
    Serial.print(':'); Serial.println(TELEM_PORT);
#else
    Serial.print(F("[TELEM] transport: TCP -> ")); Serial.print(F(TELEM_HOST));
    Serial.print(':'); Serial.println(TELEM_PORT);
#endif

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

void telemetryPrintBlockStats() {
    Serial.print(F("[TELEM] blocking per send: n=")); Serial.print(s_blkN);
    if (s_blkN) {
        Serial.print(F("  min=")); Serial.print(s_blkMin);
        Serial.print(F(" avg=")); Serial.print(s_blkSum / s_blkN);
        Serial.print(F(" max=")); Serial.print(s_blkMax);
    }
    Serial.println(F(" ms"));
    Serial.print(F("[TELEM]   worst phase: build=")); Serial.print(s_maxBuild);
    Serial.print(F(" reconnect=")); Serial.print(s_maxReconn);
    Serial.print(F(" write=")); Serial.print(s_maxWrite);
    Serial.print(F(" ms   reconnects=")); Serial.println(s_reconnects);

    uint8_t shown = s_recentIdx < RECENT_N ? s_recentIdx : RECENT_N;
    Serial.print(F("[TELEM]   last ")); Serial.print(shown);
    Serial.print(F(" sends (ms):"));
    uint8_t over = 0;
    for (uint8_t i = 0; i < shown; i++) {
        uint16_t v = s_recent[(s_recentIdx >= RECENT_N)
                              ? (uint8_t)((s_recentIdx + i) % RECENT_N) : i];
        Serial.print(' '); Serial.print(v);
        if (v > 18) over++;
    }
    Serial.print(F("   >18 ms: ")); Serial.print(over);
    Serial.print('/'); Serial.println(shown);
}

bool telemetrySendStatus() {
    if (!s_haveKey) return false;
    if (s_pending) {                  // previous send not finished - skip this tick
        Serial.println(F("[TELEM] previous send still in flight, skipping"));
        return false;
    }

    uint32_t t0 = bench::sendBegin();

    // ---- 1. build the packet on the STACK (no heap: proven wedge cause) ----
    // [header 29][ciphertext N][tag 16]
    // V1.5.1's status document is nested (config/overview/imu/gps) and
    // measures ~1350 B, so the old 1200 B cap silently failed every send at the
    // build step. Sized with headroom; the buffer is on the STACK, so it costs
    // nothing when no send is in flight.
    static constexpr size_t BODY_MAX = 1800;
    uint8_t pkt[TELEM_HEADER_LEN + BODY_MAX + TELEM_TAG_LEN];

    // header: version | serial(16, NUL-padded) | nonce(12)
    pkt[0] = TELEM_VERSION;
    memset(pkt + 1, 0, TELEM_SERIAL_LEN);
    strncpy((char*)(pkt + 1), serialNumberGet().c_str(), TELEM_SERIAL_LEN);

    uint32_t seq = ++s_msgSeq;
    uint8_t* nonce = pkt + 1 + TELEM_SERIAL_LEN;
    memcpy(nonce, s_bootId, TELEM_BOOT_ID_LEN);
    for (int i = 0; i < 4; i++) nonce[TELEM_BOOT_ID_LEN + i] = (uint8_t)(seq >> (24 - 8 * i));

    // payload: the same status JSON the HTTP API serves, serialized straight
    // into the packet buffer (no String, no intermediate copy)
    // statusGenerateJsonSimple() does NOT sample the sensors - loop() owns that
    // cadence - so a send never pays for a blocking I2C read on top.
    JsonDocument doc = statusGenerateJsonSimple();
    size_t bodyLen = serializeJson(doc, (char*)(pkt + TELEM_HEADER_LEN), BODY_MAX);
    if (bodyLen == 0 || bodyLen >= BODY_MAX) {
        Serial.print(F("[TELEM] payload does not fit: "));
        Serial.print((unsigned)bodyLen); Serial.print('/');
        Serial.println((unsigned)BODY_MAX);
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

    // ---- 3. ship it ----
    uint32_t tCrypto = millis();          // [PROFILE] build+encrypt done here

#if TELEM_USE_UDP
    // No handshake, no teardown: one datagram, then straight back to loop().
    s_udp.beginPacket(s_serverIp, TELEM_PORT);
    s_udp.write(pkt, pktLen);
    bool sent = (s_udp.endPacket() == 1);
    if (!sent) {
        Serial.println(F("[TELEM] UDP send failed (no route / ARP)"));
        bench::sendEnd(t0, false);
        return false;
    }
    s_pending = true;
    s_pendingStart = millis();
    s_pendingT0 = t0;
    s_profCrypto = tCrypto - t0;
    s_profConnect = 0;

    Serial.print(F("[PROFILE] blocking: crypto=")); Serial.print(s_profCrypto);
    Serial.print(F(" send=")); Serial.print(millis() - tCrypto);
    Serial.println(F(" ms (UDP - no connect, no close)"));
    return true;
#else
    // PERSISTENT connection: the ~78 ms TCP handshake is paid ONCE, not per
    // send. That handshake used to be 78 of the 97 ms this function blocked
    // for, and every one of those milliseconds was a millisecond the 20 Hz
    // HTTP API could not be served.
    EthernetClient& c = s_client;
    if (tCrypto - t0 > s_maxBuild) s_maxBuild = tCrypto - t0;
    uint32_t tConnect = millis();
    if (!c.connected()) {
        s_reconnects++;
        c.stop();
        c.setConnectionTimeout(TELEM_ATTEMPT_TIMEOUT_MS);
        Serial.println(F("[TELEM] opening persistent connection..."));
        if (!c.connect(TELEM_HOST, TELEM_PORT)) {
            c.stop();
            Serial.println(F("[TELEM] connect FAILED (server down / wrong host / firewall)"));
            bench::sendEnd(t0, false);
            return false;
        }
        tConnect = millis();
        if (tConnect - tCrypto > s_maxReconn) s_maxReconn = tConnect - tCrypto;
    }

    uint32_t tWriteStart = millis();
    char hdr[160];
    int hlen = snprintf(hdr, sizeof(hdr),
        "POST " TELEM_PATH " HTTP/1.1\r\nHost: " TELEM_HOST "\r\n"
        "Content-Type: application/octet-stream\r\n"
        "Content-Length: %u\r\nConnection: keep-alive\r\n\r\n", (unsigned)pktLen);
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

    uint32_t wr = millis() - tWriteStart;
    if (wr > s_maxWrite) s_maxWrite = wr;

    uint32_t blk = millis() - t0;
    s_recent[s_recentIdx++ % RECENT_N] = (uint16_t)(blk > 65535 ? 65535 : blk);
    s_blkSum += blk; s_blkN++;
    if (blk < s_blkMin) s_blkMin = blk;
    if (blk > s_blkMax) s_blkMax = blk;

    Serial.print(F("[PROFILE] blocking: crypto=")); Serial.print(s_profCrypto);
    Serial.print(F(" connect=")); Serial.print(s_profConnect);
    Serial.print(F(" write=")); Serial.print(millis() - tConnect);
    Serial.println(F(" ms (reply read later; connection stays open)"));
    return true;
#endif
}

void telemetryPump() {
    if (!s_pending) return;

    bool done = false, ok = false;

#if TELEM_USE_UDP
    // The server answers with a single byte: '2' accepted, '4' rejected,
    // '9' replay. Reading it costs nothing, and a lost reply is not an error
    // worth blocking for - the next send is only minutes away.
    int sz = s_udp.parsePacket();
    if (sz > 0) {
        char verdict = 0;
        s_udp.read(&verdict, 1);
        ok = (verdict == '2');
        if (!ok) {
            Serial.print(F("[TELEM] server rejected, code ")); Serial.println(verdict);
        }
        done = true;
    } else if (millis() - s_pendingStart > TELEM_ATTEMPT_TIMEOUT_MS) {
        Serial.println(F("[TELEM] no UDP reply within timeout (packet may still have landed)"));
        done = true;
    }
    if (!done) return;
    s_pending = false;
#else
    if (s_client.available()) {
        // Drain the WHOLE response: on a keep-alive connection anything left
        // behind would be misread as the head of the next one. Responses are
        // small (a 204 and a few headers), so this is a handful of reads.
        char resp[64] = {0};
        int n = s_client.read((uint8_t*)resp, sizeof(resp) - 1);
        if (n > 12) {
            ok = (resp[9] == '2');           // "HTTP/1.1 2xx"
            if (!ok) {
                Serial.print(F("[TELEM] server rejected: "));
                Serial.println(&resp[9]);    // 401 = bad auth, 409 = replay
            }
        }
        uint8_t sink[64];
        while (s_client.available()) s_client.read(sink, sizeof(sink));
        done = true;
    } else if (!s_client.connected()) {
        // The server or a NAT box dropped the connection. Not an error worth
        // blocking for - the next send reopens it.
        Serial.println(F("[TELEM] connection dropped; will reopen on next send"));
        s_client.setConnectionTimeout(1);
        s_client.stop();
        done = true;
    } else if (millis() - s_pendingStart > TELEM_ATTEMPT_TIMEOUT_MS) {
        // Telemetry is best-effort: don't tear the connection down over a slow
        // or lost reply, just stop waiting for it.
        Serial.println(F("[TELEM] no reply within timeout (packet may still have landed)"));
        done = true;
    }

    if (!done) return;                       // still waiting; costs ~0 per loop
    s_pending = false;
#endif

    Serial.print(F("[PROFILE] total=")); Serial.print(millis() - s_pendingT0);
    Serial.print(F(" ms (blocking was crypto=")); Serial.print(s_profCrypto);
    Serial.print(F(" + connect=")); Serial.print(s_profConnect);
    Serial.println(F(")"));

    bench::sendEnd(s_pendingT0, ok);
}
