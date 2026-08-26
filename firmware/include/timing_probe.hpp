#ifndef TIMING_PROBE_HPP
#define TIMING_PROBE_HPP

/**
 * @file timing_probe.hpp
 * @brief Bench instrumentation for the V1.5.1.1 dt stress test.
 *
 * Answers one question: what is the real interval between two consecutive
 * orientation-filter updates, at the extremes of controller load?
 *
 * That interval matters because ALPHA in calculations.cpp is a FIXED PER-STEP
 * weight, so the complementary filter's time constant is about 49 x dt - 0.49 s
 * at the intended 10 ms cadence, but 4.9 s if dt ever reaches the 0.1 s clamp
 * in status_manager.cpp. The filter's behaviour therefore depends on how busy
 * the loop is, and nothing in V1.5.1 has ever measured that.
 *
 * Two deliberate choices:
 *
 *  - It measures with micros(), NOT the millis() delta the filter itself uses.
 *    millis() quantises to 1 ms, which on a nominal 10 ms period is a 10%
 *    error - the reported dt could only ever be "10 or 11". The filter's own
 *    dt computation is left untouched, so instrumenting does not alter the
 *    thing being measured.
 *
 *  - It records the RAW interval, before status_manager applies its 0.1 s
 *    clamp. A stall longer than that is invisible in the value the filter
 *    consumes, and those are exactly the events worth knowing about, so the
 *    clamp is counted separately instead of being allowed to hide them.
 *
 * The whole file compiles to nothing unless TIMING_PROBE is defined, so
 * `pio run -e main` still produces the untouched V1.5.1 firmware and only
 * `pio run -e timing` carries the instrument.
 */

#ifndef TIMING_PROBE
#define TIMING_PROBE 0
#endif

#if TIMING_PROBE

#include <stdint.h>
#include <ArduinoJson.h>

/**
 * @brief Sections timed individually, so a slow interval can be ATTRIBUTED to
 *        a caller rather than merely observed.
 *
 * Without this a report says "max dt was 90 ms" and the next question - which
 * is always "caused by what?" - has no answer. Keep TP_BLOCK_COUNT last.
 */
enum TimingBlock : uint8_t {
    TP_SENSORS = 0,  ///< the six I2C reads at the top of statusUpdate()
    TP_FILTER,       ///< the orientation filter call itself
    TP_GPSREAD,      ///< _gg_hal.get_gps_data() - the read alone
    TP_POWER,        ///< the INA219 bus voltage + current read
    TP_SANITY,       ///< the checkSane() block at the end
    TP_H_READ,       ///< pulling the request off the socket
    TP_H_PARSE,      ///< deserializeJson on the body
    TP_H_HANDLE,     ///< the command itself, building the response document
    TP_H_SEND,       ///< serialising and writing the response
    TP_H_SER,        ///< just the JSON -> bytes conversion
    TP_H_WRITE,      ///< just the socket write
    TP_H_CLOSE,      ///< client.stop()
    TP_STATUS,       ///< all of statusUpdate()
    TP_HTTP,         ///< httpServerLoop() - one client's request lands here
    TP_OCU,          ///< ocuMonitorUpdate() - the ARP probe, a known stall
    TP_SERIAL,       ///< statusWriteToSerial() - the 1 Hz ~55 line dump
    TP_LOOP,         ///< one whole loop() iteration
    TP_BLOCK_COUNT
};

/** Histogram bucket upper edges in microseconds; the last bucket catches
 *  everything above the final edge. Chosen to straddle the intended 10 ms
 *  period and the 100 ms clamp, so the shape of the distribution is visible
 *  and not just its endpoints. */
constexpr uint8_t TP_BUCKET_COUNT = 8;

/**
 * @brief Start a measurement window. Clears all stats and discards the first
 *        interval (which would otherwise include the arming HTTP request).
 * @param windowSeconds Window length; clamped to 1..600.
 */
void timingProbeArm(uint32_t windowSeconds);

/** @brief Call once per loop(). Freezes the window when it expires, so the
 *         readout request cannot contaminate what was measured. */
void timingProbeService();

/** @brief Record one filter-to-filter interval, raw and pre-clamp. */
void timingProbeInterval(uint32_t rawDtUs);

/** @brief Record one block duration. */
void timingProbeBlock(uint8_t block, uint32_t us);

/** @brief Fill a get_timing response. */
void timingProbeToJson(JsonDocument &resp);

#define TP_ARM(s)          timingProbeArm(s)
#define TP_SERVICE()       timingProbeService()
#define TP_INTERVAL(us)    timingProbeInterval(us)
#define TP_BEGIN(id)       uint32_t tp_t0_##id = micros()
#define TP_END(id)         timingProbeBlock((id), (uint32_t)(micros() - tp_t0_##id))

#else  /* !TIMING_PROBE - production build, every hook vanishes */

#define TP_ARM(s)          ((void)0)
#define TP_SERVICE()       ((void)0)
#define TP_INTERVAL(us)    ((void)0)
#define TP_BEGIN(id)       ((void)0)
#define TP_END(id)         ((void)0)

#endif /* TIMING_PROBE */

#endif /* TIMING_PROBE_HPP */
