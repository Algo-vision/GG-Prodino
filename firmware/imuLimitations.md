# IMU Angle Limitations — Expected Errors in Pitch, Roll and Yaw

**Firmware:** GG-GRK V1.5.1
**Sensor:** ST LSM6DS3TR (2 units) — [`docs/LSM6DS3TR.md`](docs/LSM6DS3TR.md)
**Companion:** [`docs/ConstantTemperatureAndVibrationErrors.md`](docs/ConstantTemperatureAndVibrationErrors.md) — the error floor at constant temperature, and what vibration/rough terrain adds
**Scope:** the accuracy you can expect from `pitch`, `roll` and `yaw` as reported by `get_imu` / `get_status`, broken down by idle vs moving, with vs without GPS, and one IMU vs both.

> **Status of these numbers.** The formulas are exact and the sensor figures are the LSM6DS3TR datasheet's *typical* values. Two inputs remain unmeasured on your hardware: the **temperature rise** the board actually experiences after boot, and the **real `dt` distribution** under HTTP load. Both are called out in [What's Missing](#7-whats-missing-to-pin-these-numbers-down). Datasheet typicals are not guaranteed limits and vary unit to unit.

---

## 1. Sensor configuration and specifications

Both IMUs are configured identically in `lib/GG/src/i2c_imu_gps.cpp`:

| Setting | Value |
| :-- | :-- |
| Accelerometer ODR / full scale | 104 Hz / **±8 g** |
| Accelerometer anti-alias filter | hardware LPF2, cutoff ODR/9 = **11.55 Hz** |
| Gyroscope ODR / full scale | 104 Hz / **±500 dps** |

Relevant LSM6DS3TR datasheet figures (typical, 25 °C):

| Symbol | Parameter | Typical | Consequence here |
| :-- | :-- | :-- | :-- |
| `G_TyOff` | Gyro zero-rate level | **±10 dps** | Huge — this is why startup calibration is mandatory |
| `G_OffDr` | Gyro zero-rate change vs. temperature | **±0.05 dps/°C** | **Dominates residual bias after calibration** |
| `Rn` | Rate noise density | **7 mdps/√Hz** | ≈0.05 dps/sample; negligible vs. bias |
| `G_SoDr` | Gyro sensitivity change vs. temperature | **±1.5 %** | Scale error: a 90° turn can be off ≈1.35° |
| `LA_TyOff` | Accel zero-g offset accuracy | **±40 mg** | ≈2.3° of tilt if uncalibrated |
| `LA_OffDr` | Accel zero-g change vs. temperature | **±0.5 mg/°C** | Slow pitch/roll zero drift after calibration |
| `An` | Acceleration noise density | **90 µg/√Hz** | ≈0.4 mg ⇒ ≈0.02° of tilt noise; negligible |
| `LA_SoDr` | Accel sensitivity change vs. temperature | **±1 %** | Minor tilt scale error |

Two consequences of the configuration itself:

- **Gyro saturates above ±500 dps.** Rotation faster than 500 °/s (1.4 rev/s) clips, and the clipped portion is lost from the integration permanently — yaw jumps and never recovers without GPS. A skid-steer pivot or a hard spin can reach this.
- **Accelerometer saturates above ±8 g**, and its LPF2 cutoff of 11.55 Hz means genuine tilt motion faster than ~11 Hz is attenuated. Good for anti-aliasing, but the accelerometer reference lags fast transients.

---

## 2. How each angle is produced

All three angles come from `lib/GG/src/calculations.cpp`, in one of three functions depending on which IMUs are alive. They differ only in sign handling and averaging; **the error mechanisms are identical for all three**.

### Pitch and Roll — gyro + accelerometer

```
pitch = ALPHA * (pitch ± gx·dt) + (1 - ALPHA) * pitch_acc
roll  = ALPHA * (roll  ± gy·dt) + (1 - ALPHA) * roll_acc
```

with `ALPHA = 0.98`. The gyro gives fast smooth motion; the accelerometer's gravity vector is the absolute long-term reference that stops drift.

### Yaw — gyro + GPS course

```
yaw_gyro = yaw + gz·dt
error    = wrapTo180(gpsHeading - yaw_gyro)
yaw      = wrapTo180(yaw_gyro + (dt / (5s + dt)) · error)   // only when GPS heading is usable
```

Gravity says nothing about rotation about the vertical axis, so yaw has **no** equivalent of the accelerometer. Its only absolute reference is GPS course over ground, applied with a 5 s time constant (`YAW_GPS_TAU_S`), and **only while a valid fix exists and ground speed ≥ 3 km/hr** (`GPS_HEADING_MIN_SPEED_KMH`).

### Startup calibration

`calibrateIMU()` in `src/main.cpp` averages **100 samples at 10 ms intervals (≈1 s)** per sensor, per IMU, while the device must be **still and flat**. Gyro X/Y/Z offsets and accelerometer **X/Y** offsets are captured (Z is deliberately not corrected — it carries gravity).

Calibration is very effective *when it works*: averaging 100 samples reduces the gyro noise contribution to ≈0.005 dps and the accel contribution to ≈0.04 mg. It removes essentially all of the ±10 dps zero-rate level. What it cannot remove is everything that changes *afterwards* — which is why temperature is the dominant residual term.

---

## 3. The two error mechanisms that dominate

### Mechanism A — residual gyro bias, driven by temperature

Startup calibration removes the bias present during that one second. What remains is `G_OffDr = ±0.05 dps/°C` multiplied by the temperature change since boot. Calibration runs once and is never repeated.

| Temperature rise since calibration | Residual gyro bias `b` |
| :-- | :-- |
| +10 °C | 0.5 dps |
| +20 °C | 1.0 dps |
| +30 °C | 1.5 dps |
| +40 °C | 2.0 dps |

A board that boots cold and then self-heats inside an enclosure, or a vehicle that starts at dawn and runs into midday sun, easily sees +20 to +30 °C. **This is the single largest driver of yaw drift.**

> Note that ΔT = 0 does **not** mean zero drift. A calibration residual of ≈0.005 °/s plus in-run bias instability still costs yaw roughly 20-50°/hour with the temperature perfectly stable — see the companion document, section 3.

- In a **corrected** angle (pitch/roll always; yaw only when GPS-corrected), bias gives a **bounded** steady-state offset `≈ b × τ`.
- In an **uncorrected** angle (yaw with no usable GPS), bias integrates **without bound**: `error = b × t`.

The random-walk contribution from gyro noise is negligible by comparison: `7 mdps/√Hz` gives only ≈0.17° after 10 minutes, versus hundreds of degrees from bias.

### Mechanism B — the accelerometer cannot separate gravity from motion

This directly qualifies the common assumption that *"pitch and roll always correct themselves against gravity."*

**They self-correct only when the sole acceleration acting on the board is gravity.** An accelerometer measures *specific force* — gravity **plus** vehicle acceleration. Under acceleration, braking or cornering, the "gravity" vector the filter trusts is tilted:

| Sustained linear acceleration | Steady-state tilt error |
| :-- | :-- |
| 0.5 m/s² (0.05 g) | **2.9°** |
| 1.0 m/s² (0.10 g) | **5.8°** |
| 2.0 m/s² (0.20 g) | **11.5°** |
| 3.0 m/s² (0.31 g) | **17.0°** |
| 5.0 m/s² (0.51 g) | **27.0°** |

(`error = atan(a/g)`.) The complementary filter limits how *fast* this creeps in, but any acceleration sustained beyond the filter's time constant is followed almost fully.

**No mitigation is applied.** An accelerometer-magnitude gate was trialled and removed: because horizontal acceleration changes the magnitude only to second order (`sqrt(1 + (a/g)^2)`), a threshold loose enough to be safe against temperature drift of the zero-g offset only tripped above ~0.32 g, while a threshold tight enough to catch moderate cornering risked being exceeded permanently by that same drift - which would have disabled the gravity correction for good. Rejecting moderate sustained cornering requires the acceleration to be *subtracted* using GPS- or wheel-speed-derived acceleration, not detected from magnitude alone.


---

## 4. Pitch and Roll — detailed

### 4.1 Filter time constant is now fixed by the sampling cadence

`ALPHA` is a fixed per-step weight, so `τ = dt × ALPHA/(1−ALPHA) = dt × 49`. That makes the filter's behaviour a direct function of how often `statusUpdate()` is called:

| `dt` per call | τ (pitch/roll) | Error per 1 dps residual bias |
| :-- | :-- | :-- |
| 5 ms | 0.24 s | 0.24° |
| **10 ms (current)** | **0.49 s** | **0.49°** |
| 20 ms | 0.98 s | 0.98° |
| 50 ms | 2.45 s | 2.45° |
| 100 ms | 4.90 s | 4.90° |

**`statusUpdate()` is now called only from `loop()`, on a fixed 10 ms cadence** (`STATUS_UPDATE_INTERVAL_MS`), giving τ ≈ 0.49 s. 10 ms closely matches the IMUs' 104 Hz ODR, so each call gets roughly one fresh sample.

Previously it also ran on **every HTTP request**, which had two compounding effects: `dt` collapsed to 1–2 ms on those extra calls (shrinking τ by an order of magnitude), and — more subtly — the accelerometer received its fixed 2 % weight on *every* call regardless of elapsed time, so heavier polling meant faster accelerometer tracking and therefore **greater** susceptibility to the linear-acceleration error of Mechanism B. The angles literally depended on how hard the board was being polled. HTTP handlers now serve the most recent sample (at most 10 ms old) without triggering a read.

`dt` is additionally clamped to `STATUS_UPDATE_MAX_DT_S` (0.1 s), so if something stalls the loop — a failed OCU ARP probe can cost several hundred ms — a single instantaneous gyro sample is not integrated across the whole gap.

(Yaw's GPS correction was always immune: its gain is `dt/(τ+dt)`, which is rate-independent by construction.)

### 4.2 Error budget at rest

Combining the datasheet figures at the fixed `dt = 10 ms` (τ ≈ 0.49 s):

| Source | +10 °C | +20 °C | +30 °C |
| :-- | :-- | :-- | :-- |
| Gyro bias × τ (`0.05 dps/°C × ΔT × 0.49 s`) | 0.25° | 0.49° | 0.74° |
| Accel zero-g drift (`atan(0.5 mg/°C × ΔT)`) | 0.29° | 0.57° | 0.86° |
| Accel noise through the filter | ≈0.004° | ≈0.004° | ≈0.004° |
| **Approximate total (worst case, added)** | **≈0.5°** | **≈1.1°** | **≈1.6°** |

**Expected at rest: roughly 0.5–1.5°, stable and non-accumulating**, growing slowly as the board warms. This is the regime where the angles are most trustworthy.

### 4.3 Pitch/roll are relative to the boot attitude, not to true level

The accelerometer X/Y offsets captured at startup are subtracted from every later reading. If the board is **not level when it boots**, that tilt is absorbed into the offsets and the board reads ≈0° *at its boot attitude*. Permanent mounting tilt becomes an invisible zero point rather than a reported angle.

These angles answer *"how far has it tilted since power-on"*, **not** *"what is the absolute tilt relative to gravity"* — unless the boot attitude is guaranteed level.

### 4.4 Moving

Mechanism B dominates and dwarfs everything in section 4.2. Use the table in section 3.

- **Steady cruise, flat ground** — nearly as good as at rest.
- **Accelerating / braking** — *pitch* error in the direction of acceleration.
- **Cornering** — *roll* error toward the outside of the turn. Worst case, because lateral acceleration persists far longer than τ, so the error is fully absorbed rather than filtered out.
- **Rough terrain** — **not merely noise.** Vibration rectifies into a *systematic* tilt offset (the angle formula is non-linear, so symmetric shaking does not average out), accelerometer clipping shifts the mean, and a jolt past ±500 dps costs yaw permanently. See [`docs/ConstantTemperatureAndVibrationErrors.md`](docs/ConstantTemperatureAndVibrationErrors.md).

A 17° roll error while cornering at 0.3 g is expected behaviour for a gravity-referenced filter with no linear-acceleration rejection — not a defect.

### 4.5 GPS makes **no difference at all** to pitch and roll

GPS is not an input to pitch or roll in any code path, idle or moving, fix or no fix. The `gpsHeadingValid`/`gpsHeading` parameters affect only the yaw term. Every pitch/roll row below is therefore identical in its "with GPS" and "without GPS" columns.

---

## 5. Yaw — detailed

### 5.1 Idle — **GPS does not help, in either case**

The least obvious point in this document.

At a standstill the course gate (ground speed ≥ 3 km/hr) is **not** satisfied, so the correction is switched off — deliberately, because GPS course at zero speed is noise, not a bearing, and would make a parked vehicle slowly rotate to a random direction.

So **when idle, yaw is pure gyro integration whether or not you have a fix**, and its error grows without bound at `b × t`:

| Temp rise (⇒ bias) | After 1 min | After 10 min | After 1 hour |
| :-- | :-- | :-- | :-- |
| +10 °C (0.5 dps) | 30° | 300° | 1800° (5 turns) |
| +20 °C (1.0 dps) | 60° | 600° | 3600° |
| +30 °C (1.5 dps) | 90° | 900° | 5400° |

**A vehicle parked for more than a few minutes has an effectively unknown yaw**, and it is not corrected until it moves again at ≥3 km/hr — then converging with a 5 s time constant (~15 s for most of the error; ~26 s to come within 1° of a 170° error, by simulation).

### 5.2 Moving, with GPS — bounded, but course ≠ heading

With a fix and ≥3 km/hr, yaw error becomes **bounded**:

| Temp rise (⇒ bias) | Steady-state yaw offset (`b × 5 s`) |
| :-- | :-- |
| +10 °C (0.5 dps) | 2.5° |
| +20 °C (1.0 dps) | 5.0° |
| +30 °C (1.5 dps) | 7.5° |

Plus the GPS course error itself, which grows rapidly as speed falls toward the 3 km/hr gate.

**The structural limitation is bigger than any of this. GPS course over ground is the direction the vehicle is *travelling*, not the direction it is *pointing*** — and the firmware treats them as identical. Where they differ, yaw is wrong by that difference *and the filter actively drives it there*:

- **Driving in reverse** — course is **180° opposite** to heading. For a platform that reverses regularly this is the dominant yaw error, not gyro bias.
- **Skid-steer / crab / sideslip** — off by the slip angle.
- **Sliding on loose or icy ground** — same, transiently.
- **Pivoting in place** — speed stays under the gate, so the rotation is gyro-only and never corrected.

There is no reverse detection and no way to distinguish these cases. This needs an extra input (see section 7).

### 5.3 Moving, without GPS — unbounded

Identical to idle: pure integration, `b × t`. Use the table in section 5.1.

### 5.4 Yaw is an absolute bearing only after the first fix

The first usable heading snaps yaw directly to the GPS bearing. Before that, yaw is a **relative** angle from an arbitrary power-on reference. Consumers should not treat yaw as a compass bearing until the vehicle has moved at ≥3 km/hr at least once.

### 5.5 Sign-convention risk (unverified)

The blend assumes yaw and `gpsHeading` share a sense of rotation. `gpsHeading` is a compass bearing (clockwise from north); gyro yaw follows the right-hand rule about Z. **If they are opposite on this hardware the filter pulls yaw toward the mirrored bearing instead of converging.** Not verified on hardware. Fix is one click: tick **Invert** on Yaw in the IMU axis map, which flips the sign of the gyro rate feeding yaw.

---

## 6. One IMU vs. both

Two LSM6DS3TR units are fitted, IMU2 mounted **180° rotated from IMU1 in the X/Y plane**. Which code path runs is chosen in `statusUpdate()` purely on the validity flags:

| Condition | Function used | Offsets used |
| :-- | :-- | :-- |
| `imuValid && imu2Valid` | `calculateMergedOrientation()` | both sets |
| `imuValid` only | `calculateOrientation_1()` | `g_imuXOffset`, `g_imuYOffset` |
| `imu2Valid` only | `calculateOrientation_2()` (signs mirrored) | `g_imu2XOffset`, `g_imu2YOffset` |
| neither | *none* | — angles **frozen at last value** |

### 6.1 What merging actually buys you

Merging averages the two sensors' accelerometer-derived angles and their gyro rates. Because the two units' noise and their residual biases are **statistically independent**, averaging improves both by √2:

| Quantity | Single IMU | Both merged | Improvement |
| :-- | :-- | :-- | :-- |
| Random noise (gyro & accel) | σ | σ/√2 | **≈29 % lower** |
| Expected residual bias magnitude | σ_b | σ_b/√2 | **≈29 % lower** |
| Zero-g offset drift with temperature | σ_o | σ_o/√2 | **≈29 % lower** |

Applied to section 4.2, dual-IMU at rest is roughly **0.8°** where a single IMU gives **1.1°** (at +20 °C). For yaw at section 5.2, roughly **3.5°** instead of **5.0°**.

**This is a statistical expectation, not a guarantee.** Both chips sit on the same board at the same temperature; `G_OffDr` is specified as ±0.05 dps/°C, so the *sign* varies unit to unit. If both units happen to drift the same direction, averaging buys nothing. If they drift oppositely, it cancels almost entirely. You get √2 *on average across a fleet*, not on any given board. The reported `imuTemp` field tells you how far the board has moved from its calibration temperature, which is what sets the size of this term.

### 6.2 What merging does **not** buy you

- **Nothing against Mechanism B.** Both IMUs are rigidly attached to the same chassis and experience the *same* linear acceleration. Cornering error is a common-mode error and averaging two sensors does not reduce it at all. Since Mechanism B is the dominant error while driving, **merging barely improves moving accuracy.**
- **Nothing against GPS course-vs-heading error** (section 5.2), which is common-mode too.
- **Nothing against gyro saturation** — both clip at ±500 dps together.

### 6.3 Fault isolation (implemented)

Path selection is driven by `imu1Sane` / `imu2Sane`, **not** by the raw `imuValid` / `imu2Valid` I2C-success flags. An IMU that is reading but **wrong** — frozen at a constant value, or all-zero — is therefore excluded from the fusion rather than averaged in at full weight.

| State | Behaviour |
| :-- | :-- |
| both sane | fused |
| one sane | that IMU alone |
| neither healthy | angles frozen at last values |

`imuNSane` reflects sustained sensor health only - responding, not all-zero, not frozen. It deliberately does **not** react to vehicle dynamics, so a hard manoeuvre can never momentarily drop an IMU out of the fusion.

**Residual limitation:** `checkSane` detects *frozen* and *all-zero* data. It does **not** detect an IMU that is live and plausible but wrong — one that has come loose on its mount, or whose bias has drifted badly. Such a unit still passes and is still fused. Cross-checking the two IMUs against each other (flagging a disagreement beyond a few degrees) would catch that class, and is not currently done.

### 6.4 IMU1-only vs. IMU2-only

Functionally equivalent — same filter, same constants, same error magnitudes, with IMU2's signs mirrored to compensate for its 180° mounting. There is no accuracy reason to prefer one. The only asymmetry is that they use separately captured calibration offsets, so a bad calibration on one unit affects only that path.

### 6.5 Calibration divisor (fixed)

`calibrateIMU()` previously divided the accumulated sum by the loop count (100) regardless of how many reads actually succeeded, so an intermittent IMU — say 60 of 100 reads good — produced an offset at 60 % of its true value, permanently biasing that IMU's pitch/roll for the whole session.

It now divides by the number of **successful** reads, and warns on serial if an accelerometer never responded (leaving that IMU's offsets at zero rather than undefined).

**Remaining gap:** the *gyro* calibration loops call `get_gyro_data()` without checking a return value at all, so a failing gyro contributes zeros to the average rather than being skipped. A fully absent gyro yields a zero offset (harmless); an intermittent one still pulls its offset toward zero in proportion to the failure rate.

### 6.6 With neither IMU valid, angles freeze silently

If both fail, no orientation function runs and `pitch`/`roll`/`yaw` **retain their last values indefinitely**. The API keeps reporting them like live data. Only `imuValid` / `imu2Valid` (and the `*Sane` flags) reveal that nothing is updating — a consumer that reads only the angles cannot tell a frozen reading from a live one.

---

## 7. Summary

### 7.1 Summary table

Assumes the fixed `dt = 10 ms` (τ ≈ 0.49 s) and a **+20 °C** rise since calibration (⇒ `b ≈ 1.0 dps`). Single-IMU figures; divide by ≈1.4 for the merged case at rest.

| Condition | Pitch | Roll | Yaw |
| :-- | :-- | :-- | :-- |
| **Idle, with GPS** | ≈1.1°, bounded, non-accumulating. **Best case.** | Same as pitch | **Unbounded: 60°/min** — gate closed below 3 km/hr, GPS contributes nothing |
| **Idle, without GPS** | Identical — GPS is not an input | Identical | **Unbounded: 60°/min** — identical to "with GPS" |
| **Moving, with GPS** | Dominated by acceleration: ≈6° at 0.1 g, ≈17° at 0.3 g | Same, worst in sustained turns | **Bounded ≈5°**, *if* course = heading. **180° wrong in reverse**; off by slip angle when skidding |
| **Moving, without GPS** | Identical — GPS is not an input | Identical | **Unbounded: 60°/min** |

### 7.2 One IMU vs. both

| Aspect | Single IMU | Both merged |
| :-- | :-- | :-- |
| Noise & bias at rest | baseline (≈1.1°) | **≈29 % better** (≈0.8°), statistically |
| Accuracy while driving | baseline | **Essentially the same** — acceleration error is common-mode |
| Yaw with GPS | ≈5° | ≈3.5° |
| Fault isolation | fails visibly | A frozen/all-zero IMU is now **excluded** by `imuNSane`; a live-but-wrong one still pollutes |
| Near ±180° tilt | correct | arithmetic angle averaging breaks at the wrap (+179° and −179° average to 0°) |

### 7.3 The seven things to take away

1. **GPS never affects pitch or roll.** Its only role is correcting yaw.
2. **"Pitch and roll always self-correct against gravity" holds only at rest.** While accelerating or cornering the accelerometer cannot tell motion from gravity — ≈17° at 0.3 g. This is the largest error in the system during normal driving.
3. **Yaw is never corrected while stationary**, with or without GPS, and drifts ≈60°/min at a +20 °C rise. Treat yaw after any idle period as unknown.
4. **Yaw tracks direction of travel, not direction of pointing.** Reversing puts it 180° out.
5. **Temperature is the dominant residual-bias driver** (`0.05 dps/°C`), because calibration runs once at boot and is never repeated.
6. **Two IMUs help at rest (≈29 %) but barely while driving**, because the dominant driving error is common to both sensors.
7. **Merging now excludes a frozen or all-zero IMU** via `imu1Sane`/`imu2Sane`, but cannot detect a live-but-wrong one.

### 7.4 Quick reliability ranking

| Rank | Measurement | Trustworthiness |
| :-- | :-- | :-- |
| 1 | Pitch/roll at rest, both IMUs | Good — ≈0.8° bounded |
| 2 | Pitch/roll at rest, one IMU | Good — ≈1.1° bounded |
| 3 | Yaw moving forward with a good fix | Fair — ≈5°, only while genuinely moving forward |
| 4 | Pitch/roll while driving | Poor during acceleration/cornering; good in steady cruise |
| 5 | Yaw stationary or with no fix | **Not trustworthy** — ≈60°/min unbounded |
| 6 | Yaw while reversing or skidding | **Actively wrong** — up to 180° |

---

## 8. What's missing to pin these numbers down

The datasheet has closed the biggest gap. What remains:

### Still needed to sharpen the estimates

1. **The board's actual temperature rise after boot, and its ambient range.** Every bias figure above is `0.05 dps/°C × ΔT`, so this scales the entire yaw analysis. **This is now measurable**: the on-chip sensor is read and reported as `imuTemp` in `get_imu`. Log it across a cold-start-to-warm cycle and every "+20 °C" row becomes a real number for your installation. Note the absolute reading is only ±15 °C accurate (`Toff`) — use the *change* since boot, which is accurate.
2. ~~The real distribution of `dt`.~~ **Resolved** — `statusUpdate()` now runs only from `loop()` at a fixed 10 ms cadence, so τ ≈ 0.49 s deterministically. Worth confirming on hardware that the loop actually keeps up with 10 ms (the OCU probe and flash writes are the plausible stallers; `dt` is clamped at 0.1 s if not).
3. **Measured residual bias per unit.** Datasheet typicals are not per-unit guarantees. Directly measurable: keep the board still and powered, log `yaw` for 10–30 minutes, and the slope is `b`. Repeat cold and warm to confirm the 0.05 dps/°C figure on your units, and do it for each IMU separately.
4. **GPS course-accuracy vs. ground speed** from the u-blox datasheet or its `headAcc`/`sAcc` fields. Course error near the 3 km/hr gate may exceed the gyro contribution, making 3 km/hr the wrong threshold. The receiver already reports accuracy estimates the firmware ignores.
5. **Vibration spectrum while stationary but powered.** LPF2 at 11.55 Hz rejects much of it, but anything below that passes into pitch/roll — and vibration *during the 1 s calibration window* corrupts the offsets for the whole session.

### Needed about the vehicle and its use

6. **Does the vehicle reverse, skid-steer or crab?** Decides whether the course-vs-heading issue is a corner case or the dominant yaw error.
7. **Typical and maximum linear accelerations**, and how long they are sustained — converts section 3's table into an error envelope for your duty cycle.
8. **Peak rotation rate**, to confirm the ±500 dps gyro range is never clipped during pivots or impacts.
9. **Is the board guaranteed level at boot?** Determines whether pitch/roll are absolute or only relative (section 4.3).
10. **The required accuracy per angle.** Without a target there is no way to say whether ≈6° of roll error while cornering is acceptable. This decides whether any fix below is worth doing.

### Needs a hardware measurement

11. **The yaw/heading sign convention** (section 5.5) — one drive in a known direction settles it.
12. **A ground-truth reference** for real validation: a surveyed level plate for pitch/roll, and dual-antenna GNSS or an optical reference for true heading.

### Already implemented

- ✅ Fusion driven by `imu1Sane`/`imu2Sane` instead of `imuValid` (section 6.3).
- ✅ On-chip temperature read and reported as `imuTemp`.
- ✅ Calibration divides by successful reads (section 6.5).
- ✅ `statusUpdate()` moved to a fixed 10 ms cadence in `loop()` only, with a `dt` clamp (section 4.1).

### Highest-value improvements remaining, in order

1. **Compensate gyro bias from `imuTemp`** — the temperature is now measured but not yet *used*. Applying `−0.05 dps/°C × (T − T_calibration)` to each gyro axis, or simply recalibrating when the temperature has moved more than a few degrees, attacks the dominant residual-bias term directly.
2. **Subtract GPS/wheel-derived linear acceleration** from the accelerometer instead of relying on the magnitude gate — the only way to reject moderate sustained cornering (section 3).
3. **Reverse detection** for yaw, from drive command or wheel direction, to add 180° to the GPS course when reversing.
4. **Cross-check the two IMUs against each other** to catch a loose or badly-drifted unit that still passes `checkSane` (section 6.3).
5. **Validate the gyro calibration reads** the way the accelerometer loops now do (section 6.5).
6. **Weight the yaw correction by the GPS receiver's own heading-accuracy estimate** instead of the fixed 3 km/hr gate.
7. **A magnetometer**, if one can be fitted, is the only fix for stationary yaw (section 5.1) — no amount of tuning solves that one.
