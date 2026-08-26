#include "timing_probe.hpp"

#if TIMING_PROBE

#include <Arduino.h>
#include "status_manager.hpp"   // STATUS_UPDATE_MAX_DT_S
#include <i2c_imu_gps.hpp>      // g_gpsPvtFresh

namespace {

/** Bucket upper edges in microseconds. Anything above the last edge falls into
 *  the final catch-all bucket. 100000 is the clamp in status_manager.cpp, so
 *  the top two buckets are precisely the region where the filter's dt stops
 *  tracking reality. */
const uint32_t kBucketEdgeUs[TP_BUCKET_COUNT] = {
    8000, 12000, 16000, 25000, 40000, 60000, 100000, 0xFFFFFFFFu
};

/** The clamp, in microseconds, so the count here means exactly "times the
 *  filter under-integrated" rather than something merely close to it. */
const uint32_t kClampUs = (uint32_t)(STATUS_UPDATE_MAX_DT_S * 1000000.0f);

enum ProbeState : uint8_t { PROBE_IDLE = 0, PROBE_RUNNING, PROBE_FROZEN };

struct IntervalStats {
    uint32_t count;
    uint32_t minUs;
    uint32_t maxUs;
    uint64_t sumUs;
    uint32_t clampHits;
    uint32_t buckets[TP_BUCKET_COUNT];
};

struct BlockStats {
    uint32_t count;
    uint32_t maxUs;
    uint64_t sumUs;
};

/** NAV-PVT solutions parsed during the window. Without this a GPS read that
 *  became cheap because it STOPPED WORKING is indistinguishable from one
 *  that became cheap because it got efficient - and the first would look
 *  like a win. Free-running counter, so snapshot it at arm.
 */
uint32_t      s_gpsPvtAtArm = 0;

ProbeState    s_state = PROBE_IDLE;
uint32_t      s_windowMs = 0;
uint32_t      s_startMs = 0;
uint32_t      s_elapsedMs = 0;
bool          s_skipNext = false;
IntervalStats s_dt;
BlockStats    s_blocks[TP_BLOCK_COUNT];

// NO explicit array size. With one, sizeof(kBlockNames)/sizeof(...) equals
// TP_BLOCK_COUNT whatever is written here - the array just pads with
// nullptr - and the assert below becomes tautologically true. Unsized, the
// assert actually counts the names.
const char* const kBlockNames[] = {
    "sensors", "filter", "gps_read", "power", "sanity",
    "h_read", "h_parse", "h_handle", "h_send", "h_ser", "h_write", "h_close",
    "status", "http", "ocu", "serial", "loop"
};
static_assert(sizeof(kBlockNames) / sizeof(kBlockNames[0]) == TP_BLOCK_COUNT,
              "kBlockNames must have exactly one entry per TimingBlock");

void clearStats() {
    s_dt.count = 0;
    s_dt.minUs = 0xFFFFFFFFu;
    s_dt.maxUs = 0;
    s_dt.sumUs = 0;
    s_dt.clampHits = 0;
    for (uint8_t i = 0; i < TP_BUCKET_COUNT; i++) {
        s_dt.buckets[i] = 0;
    }
    for (uint8_t b = 0; b < TP_BLOCK_COUNT; b++) {
        s_blocks[b].count = 0;
        s_blocks[b].maxUs = 0;
        s_blocks[b].sumUs = 0;
    }

    s_gpsPvtAtArm = g_gpsPvtFresh;
}

/** Mean as a float, or 0 with no samples - so a JSON reader never has to
 *  special-case an empty window. */
float meanUs(uint64_t sum, uint32_t count) {
    return count ? (float)((double)sum / (double)count) : 0.0f;
}

}  // namespace

void timingProbeArm(uint32_t windowSeconds) {
    if (windowSeconds < 1)   windowSeconds = 1;
    if (windowSeconds > 600) windowSeconds = 600;

    clearStats();
    s_windowMs  = windowSeconds * 1000UL;
    s_startMs   = millis();
    s_elapsedMs = 0;
    // The interval spanning the arming request would carry that request's own
    // cost into a window meant to exclude it. Drop it.
    s_skipNext  = true;
    s_state     = PROBE_RUNNING;
}

void timingProbeService() {
    if (s_state != PROBE_RUNNING) {
        return;
    }
    uint32_t elapsed = millis() - s_startMs;   // wrap-safe on unsigned
    if (elapsed >= s_windowMs) {
        s_elapsedMs = elapsed;
        s_state = PROBE_FROZEN;
    }
}

void timingProbeInterval(uint32_t rawDtUs) {
    if (s_state != PROBE_RUNNING) {
        return;
    }
    if (s_skipNext) {
        s_skipNext = false;
        return;
    }

    s_dt.count++;
    s_dt.sumUs += rawDtUs;
    if (rawDtUs < s_dt.minUs) s_dt.minUs = rawDtUs;
    if (rawDtUs > s_dt.maxUs) s_dt.maxUs = rawDtUs;
    if (rawDtUs > kClampUs)   s_dt.clampHits++;

    for (uint8_t i = 0; i < TP_BUCKET_COUNT; i++) {
        if (rawDtUs < kBucketEdgeUs[i]) {
            s_dt.buckets[i]++;
            break;
        }
    }
}

void timingProbeBlock(uint8_t block, uint32_t us) {
    if (s_state != PROBE_RUNNING || block >= TP_BLOCK_COUNT) {
        return;
    }
    BlockStats &b = s_blocks[block];
    b.count++;
    b.sumUs += us;
    if (us > b.maxUs) b.maxUs = us;
}

void timingProbeToJson(JsonDocument &resp) {
    resp["type"] = "timing";
    resp["state"] = (s_state == PROBE_IDLE)    ? "idle"
                  : (s_state == PROBE_RUNNING) ? "running"
                                               : "frozen";
    resp["windowS"] = s_windowMs / 1000UL;
    resp["elapsedMs"] = (s_state == PROBE_RUNNING) ? (millis() - s_startMs)
                                                   : s_elapsedMs;

    JsonObject dt = resp["dt"].to<JsonObject>();
    dt["count"]     = s_dt.count;
    dt["minUs"]     = s_dt.count ? s_dt.minUs : 0;
    dt["maxUs"]     = s_dt.maxUs;
    dt["meanUs"]    = meanUs(s_dt.sumUs, s_dt.count);
    dt["clampHits"] = s_dt.clampHits;
    // Effective sample rate over the window - the number to compare against
    // the 100 Hz the scheduler asks for and the 104 Hz the IMUs produce.
    dt["hz"] = (s_dt.count && s_dt.sumUs)
                   ? (float)(1000000.0 * (double)s_dt.count / (double)s_dt.sumUs)
                   : 0.0f;

    JsonArray edges = dt["edgesUs"].to<JsonArray>();
    JsonArray hist  = dt["buckets"].to<JsonArray>();
    for (uint8_t i = 0; i < TP_BUCKET_COUNT; i++) {
        edges.add(kBucketEdgeUs[i] == 0xFFFFFFFFu ? 0 : kBucketEdgeUs[i]);
        hist.add(s_dt.buckets[i]);
    }

    resp["gpsPvtFresh"] = (uint32_t)(g_gpsPvtFresh - s_gpsPvtAtArm);

    JsonObject blocks = resp["blocks"].to<JsonObject>();
    for (uint8_t b = 0; b < TP_BLOCK_COUNT; b++) {
        JsonObject o = blocks[kBlockNames[b]].to<JsonObject>();
        o["count"]  = s_blocks[b].count;
        o["maxUs"]  = s_blocks[b].maxUs;
        o["meanUs"] = meanUs(s_blocks[b].sumUs, s_blocks[b].count);
        o["totalMs"] = (uint32_t)(s_blocks[b].sumUs / 1000ULL);
    }
}

#endif /* TIMING_PROBE */
