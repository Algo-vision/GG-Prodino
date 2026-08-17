# Constant-Temperature and Vibration Errors

## Pitch, Roll and Yaw — GG-GRK HLC Firmware V1.5.1

**Companion to** [`imuLimitations.md`](../imuLimitations.md), which covers the two dominant error mechanisms: temperature-driven gyro bias, and the accelerometer's inability to separate gravity from vehicle acceleration.

This document covers what those leave out:

1. **What remains when the temperature does not change** — the error floor you cannot remove by thermal control or recalibration.
2. **What rough terrain actually does** — vibration and shock, which produce *systematic* offsets, not merely noise.

Numbers marked **[derived]** follow from the LSM6DS3TR datasheet and the firmware's own filter constants. Numbers marked **[requires measurement]** cannot be obtained from the datasheet and are given as formulas plus a procedure in section 6.

---

## 1. Sensor chain as configured

| Stage | Accelerometer | Gyroscope |
| :-- | :-- | :-- |
| ODR | 104 Hz | 104 Hz |
| Full scale | ±8 g | ±500 dps |
| Analog filter | 50 Hz (Table 48, ODR ≤ 104 Hz) | ODR-determined; not separately tabulated |
| Digital LPF | **LPF2 enabled, ODR/9 = 11.55 Hz** (`CTRL8_XL = 0xC0`) | **none configured** |
| Sampled by firmware at | 100 Hz (`STATUS_UPDATE_INTERVAL_MS = 10`) | 100 Hz |

The firmware writes only `CTRL1_XL`, `CTRL8_XL` and `CTRL2_G`. On this part `CTRL6_C` carries no gyro low-pass selection, and `CTRL7_G`'s filter is a **high**-pass (unused here). **The gyro therefore has no user-configured anti-alias filter** — see section 5.4.

---

## 2. Why constant temperature is not "no drift"

`imuLimitations.md` attributes residual gyro bias to `G_OffDr = 0.05 °/s/°C × ΔT`. Setting ΔT = 0 does **not** give zero bias. Two terms survive:

- **Calibration residual** — the startup average is itself a noisy estimate of the true bias. **[derived]**
- **In-run bias instability** — the bias wanders randomly even at fixed temperature. **[requires measurement]**

A third term, angle random walk, is present but negligible here.

---

## 3. Constant-temperature error budget

### 3.1 Calibration residual bias **[derived]**

`calibrateIMU()` averages 100 gyro samples. With rate noise density `Rn = 7 mdps/√Hz` and a noise bandwidth of ODR/2 = 52 Hz:

```
per-sample noise  σ = 0.007 × √52          = 0.0505 °/s
after 100 averages σ = 0.0505 / √100       = 0.0051 °/s   ← residual bias
```

This is a **fixed offset for the whole session** — recalibration is the only thing that changes it.

| Elapsed | Uncorrected yaw error (`b × t`) |
| :-- | :-- |
| 1 minute | **0.30°** |
| 10 minutes | **3.0°** |
| 1 hour | **18°** |

Contribution to pitch/roll (bounded by the complementary filter, τ = 0.49 s): `0.0051 × 0.49` = **0.0025°** — negligible.

> This assumes the board is genuinely still during the 1-second calibration. Movement or vibration during that window inflates the residual directly and without limit — see section 5.5.

### 3.2 In-run bias instability **[requires measurement]**

The gyro bias wanders at constant temperature. **The LSM6DS3TR datasheet does not specify this parameter** — there is no bias-instability or Allan-variance figure to cite. It must be measured (section 6, Test 2).

Formula: `yaw error = b_inrun × t`.

Typical range for consumer MEMS gyros of this class, **for planning only until measured**:

| `b_inrun` | per minute | per 10 minutes | per hour |
| :-- | :-- | :-- | :-- |
| 5 °/hr (0.0014 °/s) | 0.08° | 0.8° | 5° |
| 10 °/hr (0.0028 °/s) | 0.17° | 1.7° | 10° |
| 30 °/hr (0.0083 °/s) | 0.50° | 5.0° | 30° |

Contribution to pitch/roll: `b_inrun × 0.49 s` ≤ 0.004° — negligible.

### 3.3 Angle random walk **[derived]**

Integrating the gyro's white noise gives `σ_θ = Rn × √t = 0.007 × √t` degrees:

| Elapsed | 1 s | 10 s | 1 min | 10 min | 1 hour |
| :-- | :-- | :-- | :-- | :-- | :-- |
| σ | 0.007° | 0.022° | 0.054° | 0.171° | **0.420°** |

Negligible against the bias terms — 0.42° after an hour versus 18°+ from bias.

Where a filter bounds it (`σ ≈ Rn × √(τ/2)`):

- pitch/roll, τ = 0.49 s → **0.0035°**
- yaw with GPS correction, τ = 5 s → **0.011°**

### 3.4 Accelerometer noise **[derived]**

`An = 90 µg/√Hz`; LPF2 at 11.55 Hz gives an equivalent noise bandwidth of `(π/2) × 11.55` = 18.1 Hz:

```
σ_a  = 90 µg/√Hz × √18.1 Hz = 0.383 mg
tilt = atan(0.000383)        = 0.022°   (raw)
```

Through the complementary filter the accelerometer carries weight `(1 − α) = 0.02`, attenuating noise by `√(0.02/1.98)` = 0.10:

**Contribution to pitch/roll ≈ 0.002°.**

> `An` is specified at FS = ±2 g; the board runs ±8 g, where noise density is typically equal or slightly higher. Even a 4× error here leaves this term negligible.

### 3.5 Constant-temperature summary

**At rest, at constant temperature:**

| Term | Pitch / Roll | Yaw (uncorrected) | Yaw (GPS-corrected) |
| :-- | :-- | :-- | :-- |
| Calibration residual bias | 0.0025° | **0.30°/min** | 0.026° |
| In-run bias instability | ≤0.004° | **0.08–0.50°/min** | 0.007–0.042° |
| Angle random walk | 0.0035° | 0.054°/min (√t) | 0.011° |
| Accelerometer noise | 0.002° | — | — |
| **Total** | **≈0.01°** | **≈0.4–0.8°/min, unbounded** | **≈0.05°** |

Two conclusions worth stating plainly:

- **Pitch and roll have a noise floor around 0.01°.** At rest and at constant temperature they are effectively noiseless. Any real-world error you see is the boot-attitude reference (`imuLimitations.md` section 4.3), mechanical flex of the mount, or vehicle motion — **not** the sensor.
- **Yaw still drifts 20–50°/hour with the temperature perfectly stable.** Thermal control alone does not fix yaw. Only the GPS correction bounds it, and that requires ≥3 km/hr of ground speed.

---

## 4. What rough terrain actually does

`imuLimitations.md` currently says rough terrain gives "a noisier reading rather than a systematic offset". **That is too reassuring.** Four mechanisms produce genuine systematic offsets.

### 4.1 Vibration rectification **[formula derived, magnitude requires measurement]**

Pitch is computed as `atan2(−a_y, a_z)`, which is **non-linear**. A symmetric oscillation of the gravity vector therefore does *not* average to zero.

Writing `a_y = δy` and `a_z = g + δz` and expanding to second order:

```
θ ≈ −δy/(g + δz) ≈ −δy/g + (δy·δz)/g²
```

Taking the time average, with zero-mean vibration `E[δy] = 0`:

```
                Cov(a_lateral , a_vertical)
  E[θ]  ≈  ─────────────────────────────────   radians
                          g²
```

**The rectified DC error is driven by the correlation between lateral and vertical acceleration.** This is exactly what a wheel striking a bump produces: a vertical jolt coupled to a lateral or pitching reaction. Uncorrelated vibration rectifies to nearly zero; correlated vibration does not.

Worked values (`error [°] = 57.3 × Cov / g²`):

| Lateral RMS | Vertical RMS | Correlation ρ | Cov | **Systematic tilt error** |
| :-- | :-- | :-- | :-- | :-- |
| 0.1 g | 0.1 g | 0.5 | 0.005 g² | **0.3°** |
| 0.3 g | 0.3 g | 0.5 | 0.045 g² | **2.6°** |
| 0.3 g | 0.3 g | 1.0 | 0.090 g² | **5.2°** |
| 0.5 g | 0.5 g | 0.5 | 0.125 g² | **7.2°** |

This is a **steady offset for as long as the vibration lasts**, not a fluctuation that averages out. It is invisible to every health flag in the API.

### 4.2 Clipping **[thresholds derived, occurrence requires measurement]**

**Accelerometer, ±8 g.** Gravity already occupies 1 g on one axis, leaving 7 g of headroom on that side and 9 g on the other. Sustained Gaussian vibration would need roughly ≥1 g RMS to clip; **shock** from a hard impact reaches 8 g far more easily. Because the clipping is asymmetric (gravity offsets the axis), truncation shifts the *mean* — a second rectification path on top of section 4.1.

**Gyroscope, ±500 dps.** This is the more damaging one. Each clipped sample loses `(ω_true − 500) × dt` of rotation, and for yaw that loss is **permanent** — there is no absolute reference to recover it unless GPS correction is active.

| True peak rate | Angle lost per sample (dt = 10 ms) | 100 ms burst (10 samples) |
| :-- | :-- | :-- |
| 600 °/s | 1.0° | **10°** |
| 800 °/s | 3.0° | **30°** |
| 1200 °/s | 7.0° | **70°** |

A single hard impact that spins the chassis past 500 °/s for a tenth of a second can therefore put yaw tens of degrees out, instantly and silently. Pitch and roll partially recover through gravity; yaw does not.

### 4.3 Aliasing **[exposure identified, margin requires confirmation]**

Sampling at 100 Hz sets Nyquist at 50 Hz. Content above that folds down into the signal band and is indistinguishable from real motion.

- **Accelerometer — protected.** The 50 Hz analog filter plus LPF2 at 11.55 Hz attenuate well before Nyquist.
- **Gyroscope — not explicitly protected.** The firmware configures no gyro low-pass (section 1). Vibration above ~50 Hz can alias into the integrated angle, and structural resonances on a tracked or wheeled chassis commonly sit in the 50–500 Hz band.

Aliased energy appears as a **bias-like offset** in the gyro, so it acts exactly like extra drift — and being motion-driven, it appears only when the vehicle is working.

### 4.4 Calibration corruption **[procedure exists to detect]**

`calibrateIMU()` assumes the board is still and level for 1 second at boot. If the engine is running or the vehicle is being loaded during that window, the captured offsets absorb the vibration mean and are wrong **for the entire session** — a fixed error on every subsequent reading, with no indication in any flag.

This is the single cheapest failure to avoid: calibrate before starting the engine.

---

## 5. What can and cannot be bounded today

| Error | Status | Value |
| :-- | :-- | :-- |
| Calibration residual bias | **[derived]** | 0.0051 °/s ⇒ 0.30°/min yaw |
| Angle random walk | **[derived]** | 0.007 × √t degrees |
| Accelerometer noise → tilt | **[derived]** | 0.002° filtered |
| Clipping thresholds and cost | **[derived]** | ±8 g / ±500 dps; table in 4.2 |
| In-run bias instability | **[requires measurement]** | not in datasheet; `b × t` |
| Vibration rectification | **[requires measurement]** | `57.3 × Cov(a_lat,a_vert)/g²` degrees |
| Clipping occurrence rate | **[requires measurement]** | needs peak logging |
| Gyro aliasing margin | **[requires confirmation]** | gyro bandwidth not configured |

---

## 6. Measurement procedures

> ### ⚠ Instrumentation limit — read first
>
> **The existing interfaces cannot characterise vibration.** The HTTP API sustains roughly 20 requests/second, giving a Nyquist limit near **10 Hz**, and the serial console emits only 1 Hz. Sensor data is sampled internally at 100 Hz but only the latest snapshot is exposed.
>
> Tests 1 and 2 work over the existing API. **Tests 3–5 need on-board capture at the full 100 Hz** — either a burst-capture command (e.g. "record N samples of `imuX/Y/Z`, `imuGx/Gy/Gz` and return them") or running the vehicle with a laptop on the serial port with a temporary high-rate raw dump. Attempting Test 3 at 20 Hz will alias the vibration and understate it.

### Test 1 — Constant-temperature yaw drift (residual bias)

**Closes:** section 3.1 verification.

1. Power the board, let it calibrate, and leave it **completely still** on a solid surface.
2. Poll `get_imu` at 1 Hz for **30–60 minutes**, logging `yaw` and `imuTemp`.
3. Discard the first 10 minutes (thermal settling).
4. Over a window where `imuTemp` changes by less than 1 °C, fit a straight line to `yaw` vs time.

**Result:** slope = residual bias in °/s. Compare with the derived 0.0051 °/s. A significantly larger value indicates the board was not still during calibration.

Repeat per IMU by temporarily forcing single-IMU operation, since the fused yaw averages both.

### Test 2 — In-run bias instability (Allan deviation)

**Closes:** section 3.2, the largest unquantified constant-temperature term.

1. Use the same log as Test 1, but sample at **10 Hz or faster** and run for **at least 2 hours** in a thermally stable room.
2. Compute the Allan deviation of the *gyro rate* (`imuGz`) versus averaging time τ.
3. The **minimum** of the curve is the in-run bias instability; the τ at which it occurs is the optimal recalibration interval.

**Result:** substitute into the section 3.2 table. If the minimum sits at τ = 100 s, recalibrating more often than every 100 s gains nothing.

### Test 3 — Vibration spectrum and rectification

**Closes:** section 4.1, the dominant rough-terrain error.

1. Capture `imuX`, `imuY`, `imuZ` at the full **100 Hz** while driving representative terrain at representative speed.
2. Compute, over 10-second windows:
   - RMS of the lateral component (perpendicular to gravity)
   - RMS of the vertical component
   - **`Cov(a_lateral, a_vertical)`** — the quantity that actually matters
3. Evaluate `error [°] = 57.3 × Cov / g²`.

**Result:** the systematic tilt offset for that terrain. Cross-check by comparing reported `pitch`/`roll` on a *level* rough section against the true (zero) attitude — the mean offset should match the prediction.

### Test 4 — Clipping census

**Closes:** section 4.2.

1. From the same 100 Hz capture, count samples where any accelerometer axis is at ±8 g or any gyro axis is at ±500 dps.
2. For each gyro clip, the lost angle is at least `(500 − |ω_measured|) × dt` — a lower bound only, since the true rate is unknown once clipped.

**Result:** if gyro clipping occurs at all, either raise the full scale to ±1000 or ±2000 dps (costing sensitivity) or accept that yaw needs a GPS fix to survive rough terrain. **A single clip event can cost tens of degrees.**

### Test 5 — End-to-end angle error on terrain

**Closes:** the practical question — what ± should be expected.

1. Park on a surveyed slope of known pitch and roll. Record the reported values (this establishes the boot-attitude reference offset).
2. Drive a representative rough section and return to the same surveyed spot.
3. Compare reported pitch/roll to truth on arrival, and yaw to a known bearing.

**Result:** the total real-world error envelope, including everything in this document plus mounting effects. Run it three times to separate repeatable bias from random variation.

---

## 7. Recommendations

Ordered by benefit relative to effort:

1. **Calibrate before starting the engine.** Zero cost, removes section 4.4 entirely.
2. **Run Test 1 and Test 2.** They need only the existing API and answer the largest constant-temperature unknown.
3. **Add a burst-capture command** returning N raw samples at 100 Hz. Without it, Tests 3–5 cannot be done properly, and the vibration error stays unquantified.
4. **Add gyro clip counters** to the telemetry — a simple saturating count of samples at ±500 dps would turn section 4.2 from invisible into observable, and is far cheaper than full capture.
5. **Confirm the gyro anti-alias margin** (section 4.3). If the internal bandwidth is not comfortably below 50 Hz, either raise the ODR (giving more margin before decimating to 100 Hz) or mechanically isolate the mount.
6. **Consider mechanical isolation.** Vibration rectification (section 4.1) scales with the *square* of vibration amplitude — halving the vibration cuts the tilt error by four.

---

## 8. Open items

| # | Item | Blocking |
| :-- | :-- | :-- |
| 1 | In-run bias instability, per unit | Test 2 |
| 2 | Vibration RMS and lateral/vertical covariance on representative terrain | Test 3 (needs 100 Hz capture) |
| 3 | Gyro clipping occurrence | Test 4 (needs 100 Hz capture or clip counters) |
| 4 | Gyro internal bandwidth / anti-alias margin | ST confirmation or bench sweep |
| 5 | Gyro g-sensitivity (linear acceleration coupling into rate) | Not in datasheet; bench test on a shaker |
| 6 | Whether calibration is ever performed with the engine running | Operational procedure review |
