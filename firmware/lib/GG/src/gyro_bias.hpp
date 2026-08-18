#ifndef GYRO_BIAS_HPP
#define GYRO_BIAS_HPP

/**
 * @file gyro_bias.hpp
 * @brief Gyro zero-rate offset, learned while the machine stands still.
 *
 * V1.5.1 measured this once in setup(), averaging 100 samples over four
 * seconds, and used the result for the whole session. That assumes the machine
 * is stationary at the instant it is powered on - which cannot be promised.
 * Power it up on a running vehicle and a real rotation rate is captured as
 * "zero", after which yaw drifts for as long as the controller stays on.
 *
 * Instead: seed from flash at boot, and refine whenever the machine is
 * genuinely observed to be at rest. This assumes nothing about the moment of
 * power-on, and it tracks the sensor's thermal drift (0.05 dps per degree C on
 * an LSM6DS3TR) throughout the day rather than freezing one temperature's
 * answer at boot.
 *
 * Expressed as a TIME CONSTANT rather than a per-sample weight, so how fast it
 * converges depends on elapsed time and not on how often the loop happens to
 * call it - the same reason applyGpsYawCorrection() in calculations.cpp is
 * written that way, and the trap that pitch/roll's fixed ALPHA falls into.
 */

/** Slow on purpose. A brief false "at rest" - a machine idling smoothly enough
 *  to pass the test while genuinely creeping - can then only move the estimate
 *  a little before the detector drops out again. */
constexpr float GYRO_BIAS_TAU_S = 30.0f;

/** Beyond this a "bias" is not an offset, it is a fault or real motion that
 *  slipped through. Refuse rather than learn it: the LSM6DS3TR's zero-rate
 *  offset is specified in single digits of dps. */
constexpr float GYRO_BIAS_SANE_LIMIT_DPS = 20.0f;

struct GyroBias {
    float x, y, z;
    bool  valid;   ///< false until seeded from flash or learned at rest
};

void gyroBiasReset(GyroBias &b);

/**
 * @brief Pull the estimate toward the currently observed rate.
 *
 * Call ONLY with the machine confirmed at rest, and with RAW gyro readings -
 * feeding it bias-corrected values would drive the estimate to zero and undo
 * the correction.
 *
 * @return true if the estimate was updated.
 */
bool gyroBiasUpdate(GyroBias &b, float rawGx, float rawGy, float rawGz,
                    float dtSeconds);

#endif /* GYRO_BIAS_HPP */
