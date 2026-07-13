#pragma once

#include <stdint.h>

// ============================================================================
// SANITY CHECK
// ============================================================================
//
// Shared "is this sensor group's data reasonable" check: not all zero, and
// not stuck at the same value for several consecutive reads (a strong sign
// of a frozen/disconnected sensor, since real sensor noise means genuine
// live readings essentially never repeat exactly). Optionally also rejects
// negative values, for groups where negative readings are never physically
// legitimate (e.g. voltage/current) - skipped for signed quantities like
// accelerometer/gyro/GPS/angle data where negative values are normal.
//
// Templated on the value type so it works for both float (power/IMU/angle)
// and double (GPS lat/lng/alt, where float's precision loss could distort
// stuck-value comparisons). Header-only (template) and free of any
// Arduino/hardware dependency, so it can be unit-tested natively (see
// firmware/test/test_sanity_check.cpp).

/**
 * @brief Check whether a group of readings is sane.
 *
 * @param current Current readings for this group.
 * @param previous Previous readings for this group (in/out - updated to
 *   `current` on return, so the caller just needs to keep one persistent
 *   array per group across calls).
 * @param count Number of values in the group.
 * @param stuckCounter In/out consecutive-identical-reading counter for this
 *   group (caller keeps one persistent counter per group across calls).
 * @param threshold Number of consecutive identical readings that counts as
 *   "stuck".
 * @param requireNonNegative If true, any negative value fails the check.
 * @return true if the group is sane (not all zero, not stuck, and - if
 *   requested - no negative values).
 */
template <typename T>
bool checkSane(const T* current, T* previous, uint8_t count,
               uint8_t &stuckCounter, uint8_t threshold,
               bool requireNonNegative = false) {
    bool allZero = true;
    bool stuck = true;
    bool hasNegative = false;

    for (uint8_t i = 0; i < count; i++) {
        if (current[i] != 0) allZero = false;
        if (current[i] != previous[i]) stuck = false;
        if (current[i] < 0) hasNegative = true;
        previous[i] = current[i];
    }

    stuckCounter = stuck ? (stuckCounter + 1) : 0;

    bool sane = !allZero && (stuckCounter < threshold);
    if (requireNonNegative) {
        sane = sane && !hasNegative;
    }
    return sane;
}
