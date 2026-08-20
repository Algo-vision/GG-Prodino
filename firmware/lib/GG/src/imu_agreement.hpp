#ifndef IMU_AGREEMENT_HPP
#define IMU_AGREEMENT_HPP

#include <stdint.h>

/**
 * @file imu_agreement.hpp
 * @brief Do the two accelerometers tell the same story?
 *
 * Priority 3, item 2. Two sensors reading the same gravity vector should agree;
 * one that has drifted, come loose or failed will not, and that is worth
 * catching before the angle it produces is believed.
 *
 * IMU2 is mounted 180 degrees round in the X/Y plane, so its in-plane readings
 * are the negative of IMU1's for the same physical motion. That is undone here
 * before anything is compared - without it the check would compare +x against
 * -x and fail permanently.
 *
 * WHY THERE IS A DEADBAND
 *
 * The comparison is a RATIO, and a ratio between two numbers that should both
 * be zero means nothing. Measured on this board sitting level:
 *
 *     axis   IMU1        IMU2         |difference|   ratio
 *      X     -0.0147 g   -0.0043 g    0.0104 g       3.39
 *      Y     -0.0336 g   -0.0022 g    0.0314 g      15.50
 *      Z     +1.0356 g   +1.0103 g    0.0253 g       1.03
 *
 * X and Y "fail" a factor-of-two test by 3x and 15x while disagreeing by three
 * hundredths of a g - which is nothing. They are perpendicular to gravity, so
 * both sensors are reporting their own zero-g offset and the ratio is noise
 * divided by noise. Only axes carrying real signal are compared.
 */

/**
 * How much an axis must read before its ratio is worth testing.
 *
 * CHOOSING THIS MATTERS MORE THAN IT LOOKS. The whole acceleration vector is
 * about 1 g, shared between the three axes, so a high threshold means few axes
 * qualify:
 *
 *     1.000 g   no axis qualifies once the machine tilts past ~15 deg
 *     0.800 g   ...past ~39 deg
 *     0.598 g   at least one axis ALWAYS qualifies, at any attitude
 *     0.500 g   as above, with margin
 *
 * 0.598 g is 1/sqrt(3) of the measured vector length - the smallest any axis
 * can be, which happens when all three are equal. At or below it the check can
 * never go quiet, whatever attitude the machine is in.
 *
 * 0.5 g sits below that with room to spare, so at least one axis is always
 * compared. It is also far above the few hundredths of a g the perpendicular
 * axes carry at rest, which is the noise-against-noise region that made a
 * deadband necessary in the first place.
 */
constexpr float IMU_AGREE_DEADBAND_G = 0.5f;

/** How far apart the two may read before it counts as disagreement. A ratio,
 *  so 2.0 means either may be up to twice the other. */
constexpr float IMU_AGREE_RATIO = 2.0f;

/**
 * @brief Compare the two accelerometers axis by axis.
 *
 * @param axesChecked Optional out: how many axes carried enough signal to be
 *        worth comparing. **Zero means the function had nothing to judge** -
 *        the `true` it returns then is "no evidence of disagreement", not "the
 *        sensors agree". Worth reporting, because a check that silently stops
 *        looking is worse than no check.
 * @return false only when an axis that WAS compared disagreed.
 */
bool imuAxesAgree(float ax1, float ay1, float az1,
                  float ax2, float ay2, float az2,
                  uint8_t *axesChecked = nullptr);

#endif /* IMU_AGREEMENT_HPP */
