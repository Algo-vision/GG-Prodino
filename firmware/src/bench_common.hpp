/**
 * @file bench_common.hpp
 * @brief Shared measurement + reporting for the two standalone benchmark
 *        firmwares (bench_aead.cpp and bench_tls.cpp).
 *
 * Compiled into BOTH, so every printed number means exactly the same thing in
 * both runs and the two logs can be compared line for line.
 *
 * What is measured, and why each one matters:
 *   total   - wall time of one complete telemetry send (build+crypto+network)
 *   crypto  - the CPU-bound part: the cost the board pays even on a perfect network
 *   net     - the transport part, for reference
 *   maxLoop - LONGEST single blocking stretch of loop(). THE decisive number:
 *             anything else the board must do (HTTP API, relays) is dead for
 *             exactly this long.
 *   freeRam - live and minimum-ever. The minimum is what actually crashes a
 *             32 KB board, not the average.
 */
#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>

#ifndef BENCH_INTERVAL_MS
#define BENCH_INTERVAL_MS 5000UL
#endif

namespace bench {

extern "C" char* sbrk(int);
inline int freeRam() { char top; return &top - reinterpret_cast<char*>(sbrk(0)); }

/// The payload BOTH firmwares send: same shape and size as the real status JSON
/// (~700-900 B), so the crypto and the network see identical amounts of data.
inline size_t buildPayload(char* buf, size_t cap, const char* serial,
                           const char* fw, uint32_t seq) {
    JsonDocument doc;
    doc["serialNumber"]     = serial;
    doc["firmwareVersion"]  = fw;
    doc["uptimeMs"]         = millis();
    doc["seq"]              = seq;
    doc["pitch"]            = 1.23 + (seq % 10) * 0.01;
    doc["roll"]             = -0.45;
    doc["yaw"]              = 187.9;
    doc["imuValid"]         = true;
    doc["imu2Valid"]        = true;
    doc["accelX"] = 0.01; doc["accelY"] = -0.02; doc["accelZ"] = 9.79;
    doc["gyroX"]  = 0.00; doc["gyroY"]  = 0.01;  doc["gyroZ"]  = -0.01;
    doc["accel2X"] = 0.02; doc["accel2Y"] = -0.01; doc["accel2Z"] = 9.81;
    doc["gyro2X"]  = 0.01; doc["gyro2Y"]  = 0.00;  doc["gyro2Z"]  = 0.00;
    doc["gpsFix"]           = false;
    doc["latitude"]         = 32.0853;
    doc["longitude"]        = 34.7818;
    doc["altitude"]         = 34.0;
    doc["satellites"]       = 0;
    doc["controllerIp"]     = "192.168.1.198";
    doc["motorWorkSeconds"] = 12345;
    doc["busVoltage"]       = 24.1;
    doc["current_mA"]       = 310.5;
    doc["techMode"]         = false;
    JsonArray relays = doc["relays"].to<JsonArray>();
    for (int i = 0; i < 4; i++) relays.add(i % 2);
    JsonArray opto = doc["optoIn"].to<JsonArray>();
    for (int i = 0; i < 4; i++) opto.add(0);
    return serializeJson(doc, buf, cap);
}

struct Stats {
    uint32_t sends = 0, fails = 0;
    uint32_t totMin = 0xFFFFFFFF, totMax = 0, totSum = 0;
    uint32_t cryMin = 0xFFFFFFFF, cryMax = 0, crySum = 0;
    uint32_t netSum = 0;
    uint32_t maxLoopMs = 0;
    int      minFree = 0x7FFFFFFF;
    uint32_t lastLoopMs = 0;
    uint32_t t0 = 0;

    void begin() { t0 = millis(); lastLoopMs = millis(); minFree = freeRam(); }

    /// Call at the top of loop(): the gap since the previous iteration IS the
    /// time the board was unable to do anything else.
    void loopTick() {
        uint32_t now = millis();
        uint32_t gap = now - lastLoopMs;
        if (gap > maxLoopMs) maxLoopMs = gap;
        lastLoopMs = now;
        int fr = freeRam();
        if (fr < minFree) minFree = fr;
    }

    void record(uint32_t totalMs, uint32_t cryptoMs, uint32_t netMs, size_t, bool ok) {
        sends++;
        if (!ok) fails++;
        totSum += totalMs; crySum += cryptoMs; netSum += netMs;
        if (totalMs < totMin) totMin = totalMs;
        if (totalMs > totMax) totMax = totalMs;
        if (cryptoMs < cryMin) cryMin = cryptoMs;
        if (cryptoMs > cryMax) cryMax = cryptoMs;
        int fr = freeRam();
        if (fr < minFree) minFree = fr;
    }

    void printSend(const char* mode, uint32_t totalMs, uint32_t cryptoUs,
                   uint32_t netMs, size_t bytes, bool ok) const {
        Serial.print('['); Serial.print(mode); Serial.print(F("] send#"));
        Serial.print(sends);
        Serial.print(ok ? F("  OK  ") : F("  FAIL "));
        Serial.print(F(" total=")); Serial.print(totalMs); Serial.print(F(" ms"));
        Serial.print(F("  crypto=")); Serial.print(cryptoUs / 1000);
        Serial.print('.'); Serial.print((cryptoUs / 100) % 10); Serial.print(F(" ms"));
        Serial.print(F("  net=")); Serial.print(netMs); Serial.print(F(" ms"));
        Serial.print(F("  bytes=")); Serial.print((unsigned)bytes);
        Serial.print(F("  freeRam=")); Serial.print(freeRam());
        Serial.print(F(" min=")); Serial.println(minFree);
    }

    void printSummary(const char* mode) const {
        uint32_t n = sends ? sends : 1;
        Serial.println();
        Serial.print(F("[SUMMARY] mode=")); Serial.print(mode);
        Serial.print(F("  runtime=")); Serial.print((millis() - t0) / 1000); Serial.println(F(" s"));
        Serial.print(F("[SUMMARY]   sends=")); Serial.print(sends);
        Serial.print(F("  fails=")); Serial.print(fails);
        Serial.print(F("  success=")); Serial.print(100 - (fails * 100 / n)); Serial.println('%');
        Serial.print(F("[SUMMARY]   total  ms: min=")); Serial.print(totMin);
        Serial.print(F(" avg=")); Serial.print(totSum / n);
        Serial.print(F(" max=")); Serial.println(totMax);
        Serial.print(F("[SUMMARY]   crypto ms: min=")); Serial.print(cryMin);
        Serial.print(F(" avg=")); Serial.print(crySum / n);
        Serial.print(F(" max=")); Serial.println(cryMax);
        Serial.print(F("[SUMMARY]   net    ms: avg=")); Serial.println(netSum / n);
        Serial.print(F("[SUMMARY]   MAX LOOP BLOCK = ")); Serial.print(maxLoopMs);
        Serial.println(F(" ms   <-- how long the board is unresponsive"));
        Serial.print(F("[SUMMARY]   freeRam now=")); Serial.print(freeRam());
        Serial.print(F("  MINIMUM EVER=")); Serial.println(minFree);
        Serial.println();
    }
};

} // namespace bench
