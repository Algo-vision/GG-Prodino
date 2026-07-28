/**
 * @file telemetry_bench.hpp
 * @brief Shared measurement harness for the TLS-vs-AEAD telemetry benchmark.
 *
 * Compiled into BOTH benchmark builds (main_tls / main_aead) so the printed
 * numbers are directly comparable - same code, same units, same format.
 *
 * Metrics:
 *   - time of one complete send
 *   - MAX single blocking stretch of loop() (the number that decides whether
 *     the HTTP server can survive alongside the telemetry path)
 *   - free RAM now AND the minimum-ever watermark (the watermark is what
 *     actually hard-faults the board, not the steady-state value)
 *   - send successes / failures
 */
#ifndef TELEMETRY_BENCH_HPP
#define TELEMETRY_BENCH_HPP

#include <Arduino.h>

extern "C" char* sbrk(int);

namespace bench {

inline int freeRam() {
    char top;
    return &top - reinterpret_cast<char*>(sbrk(0));
}

// --- state -----------------------------------------------------------------
inline uint32_t g_sends      = 0;   ///< successful sends
inline uint32_t g_fails      = 0;   ///< failed sends
inline uint32_t g_lastSendMs = 0;   ///< duration of the most recent send
inline uint32_t g_maxSendMs  = 0;   ///< worst send duration
inline uint32_t g_maxBlockMs = 0;   ///< longest single loop() iteration
inline int      g_minFreeRam = 99999;
inline uint32_t s_lastLoopMs = 0;

/** Call once at the top of every loop() iteration. */
inline void loopTick() {
    uint32_t now = millis();
    if (s_lastLoopMs != 0) {
        uint32_t d = now - s_lastLoopMs;
        if (d > g_maxBlockMs) g_maxBlockMs = d;
    }
    s_lastLoopMs = now;
    int fr = freeRam();
    if (fr < g_minFreeRam) g_minFreeRam = fr;
}

/** Bracket a send: begin() ... end(ok). */
inline uint32_t sendBegin() { return millis(); }

inline void sendEnd(uint32_t t0, bool ok) {
    uint32_t dt = millis() - t0;
    g_lastSendMs = dt;
    if (dt > g_maxSendMs) g_maxSendMs = dt;
    if (ok) g_sends++; else g_fails++;
    Serial.print(F("[BENCH] send#")); Serial.print(g_sends + g_fails);
    Serial.print(ok ? F(" OK   ") : F(" FAIL "));
    Serial.print(F("took ")); Serial.print(dt); Serial.print(F(" ms"));
    Serial.print(F("  freeRam=")); Serial.print(freeRam());
    Serial.print(F("  min=")); Serial.println(g_minFreeRam);
}

/** Periodic one-line summary (call every few seconds). */
inline void report(const char* mode) {
    Serial.print(F("[BENCH] mode=")); Serial.print(mode);
    Serial.print(F("  sends=")); Serial.print(g_sends);
    Serial.print(F(" fails=")); Serial.print(g_fails);
    Serial.print(F("  lastSend=")); Serial.print(g_lastSendMs);
    Serial.print(F("ms maxSend=")); Serial.print(g_maxSendMs);
    Serial.print(F("ms  maxBlock=")); Serial.print(g_maxBlockMs);
    Serial.print(F("ms  freeRam=")); Serial.print(freeRam());
    Serial.print(F(" min=")); Serial.println(g_minFreeRam);
}

}  // namespace bench

#endif  // TELEMETRY_BENCH_HPP
