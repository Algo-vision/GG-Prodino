#ifndef REST_DETECTOR_HPP
#define REST_DETECTOR_HPP

/**
 * @file rest_detector.hpp
 * @brief Decides whether the machine is standing still.
 *
 * Two things depend on this answer, and both are things V1.5.1 simply assumed:
 *
 *  - Gyro bias may only be learned while nothing is rotating, otherwise real
 *    motion gets absorbed into the bias and the board slowly learns to lie.
 *  - The initiated calibration is only meaningful at rest, because it trusts
 *    the accelerometer to be reading gravity alone.
 *
 * The test is the pair that must BOTH hold: the acceleration vector has the
 * length of gravity and nothing more, and the gyro reads nothing. A machine
 * accelerating in a straight line fails the first; one turning fails the
 * second.
 *
 * It reports for HOW LONG the condition has held rather than a bare yes/no, so
 * each caller can demand the confidence it needs - a brief still moment is
 * enough to nudge a bias estimate, and nowhere near enough to burn a
 * calibration into flash.
 *
 * THRESHOLDS ARE MEASURED, NOT GUESSED. Over 66271 samples across two ten
 * minute soaks, a stationary board on a bench never exceeded 0.098 g of
 * deviation or 4.35 dps. The limits below sit above those with margin.
 *
 * They are bench numbers. A real machine standing still with its engine
 * running will vibrate considerably more, so these want confirming on the
 * vehicle before anyone relies on them.
 *
 * KNOWN BLIND SPOT - and it is fundamental, not an oversight.
 *
 * An accelerometer cannot tell gravity from steady acceleration; they are the
 * same measurement. Acceleration ACROSS gravity barely lengthens the vector at
 * all - 0.5 g sideways gives |a| = sqrt(1 + 0.25) = 1.118, a deviation of only
 * 0.118 g - so anything under about 0.57 g of sustained, perfectly straight,
 * non-rotating acceleration reads as rest here. Rotation is caught by the gyro
 * and changing acceleration breaks the streak, so what survives is the narrow
 * case of a machine holding a hard constant acceleration in a straight line
 * for seconds at a time.
 *
 * No 6-axis sensor can close that gap. It is why the operator asking for the
 * calibration remains the real guard, exactly as specified - this check exists
 * to catch the obvious mistakes, not to replace that judgement.
 */

/** |a| may differ from 1 g by at most this. Worst bench observation: 0.098 g. */
constexpr float REST_ACCEL_TOLERANCE_G = 0.15f;

/** Total rotation rate ceiling. Worst bench observation: 4.35 dps. */
constexpr float REST_GYRO_LIMIT_DPS = 8.0f;

/** Enough stillness to trust a bias nudge. */
constexpr float REST_SECONDS_FOR_BIAS = 1.0f;

/** Enough stillness to accept a calibration request. Deliberately longer:
 *  this one is written to flash, or snaps the reported angle. */
constexpr float REST_SECONDS_FOR_CALIBRATION = 2.0f;

struct RestDetector {
    float stillSeconds;   ///< unbroken time the still condition has held
};

void restDetectorReset(RestDetector &d);

/**
 * @brief Feed one sample.
 * @return seconds of unbroken stillness; 0 the moment anything moves.
 *
 * Accumulates TIME rather than counting samples, because dt on this controller
 * is not constant - measured between 9 and 190 ms depending on load - so a
 * sample count would mean a different real duration under different load.
 */
float restDetectorUpdate(RestDetector &d, float dtSeconds,
                         float ax, float ay, float az,
                         float gx, float gy, float gz);

#endif /* REST_DETECTOR_HPP */
