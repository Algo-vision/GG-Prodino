# P1 — dt stress test

**Question (Ron, 17/08, Priority 1):** what is the value of `dt` in the angle
formula — its minimum when the controller is at its lightest, and its maximum
when it is at its busiest.

**Board:** GG-GRK controller, branch `v1.5.1.1`, `pio run -e timing`
**Date:** 2026-08-18 · **Windows:** 60 s · **GPS antenna: DISCONNECTED**

> **Scope.** This measures **V1.5.1 exactly as delivered**. The instrumentation
> is compiled out of `env:main`, so nothing here changes shipped behaviour. The
> remediation this measurement led to — GPS read pattern, HTTP cost, I2C clock —
> is **not on this branch**; it lives on `perf-experiments` with its own
> measurements, pending review. See [§5](#5-what-to-do-about-it).

---

## 1. Answer

| | idle (lightest) | one HTTP consumer (heaviest) |
|---|---|---|
| **dt min** | **19.98 ms** | 20.00 ms |
| dt mean | 26.27 ms | 83.24 ms |
| **dt max** | **89.52 ms** | **221.26 ms** |
| effective update rate | 38.1 Hz | 12.0 Hz |
| updates above the 0.1 s clamp | 0 | **309 of 749 — 42.9%** |

Raw runs: [`measurements/A_idle_60s.txt`](measurements/A_idle_60s.txt),
[`measurements/B_poll_60s.txt`](measurements/B_poll_60s.txt).

Two findings matter more than those numbers.

**The filter has never run at its design point.** `ALPHA` in `calculations.cpp`
is a fixed per-step weight, so the complementary filter's time constant is
`≈49 × dt`. The code assumes a 10 ms step, i.e. 0.49 s. Measured, it is **1.29 s
idle and 4.08 s under load** — 2.6× to 8.3× slower than intended, and varying
with how busy the controller happens to be.

**Under load, two of every five updates silently understate motion.** 42.9% of
updates had a real interval above the 0.1 s clamp in `status_manager.cpp`. On
those the filter integrates the gyro with a `dt` smaller than the truth — at the
221 ms maximum it uses 100 ms, i.e. **52% of the rotation that actually
occurred**. That is an accuracy defect at ordinary load, not a performance note.

---

## 2. Method

A probe was added under a `TIMING_PROBE` build flag
([`include/timing_probe.hpp`](../include/timing_probe.hpp), `env:timing` in
`platformio.ini`). The production `env:main` build compiles to a `firmware.bin`
**byte-identical** to the reference V1.5.1 binary
(md5 `15df868f0cb2cc405dff777f7c352946`), so the instrument costs shipped
behaviour nothing.

Three decisions the measurement would have been worthless without:

- **Measured with `micros()`, not the `millis()` delta the filter uses.**
  `millis()` quantises to 1 ms, a 10% error on a 10 ms period — `dt` could only
  ever report "10 or 11". The filter's own `dt` computation is untouched, so
  instrumenting does not perturb what it measures.
- **Recorded raw, before the 0.1 s clamp**, with clamp events counted
  separately. Past that point the filter's `dt` stops tracking reality, and
  those are exactly the events worth seeing.
- **Arm / freeze / read.** `reset_timing` opens a window that auto-freezes when
  it expires; `get_timing` reads the frozen snapshot. The readout request is
  therefore never inside the measurement — which matters most for the idle run,
  where a single HTTP request is the entire load being excluded.

Ten blocks are timed individually (`sensors`, `filter`, `gps_read`, `power`,
`sanity`, `status`, `http`, `ocu`, `serial`, `loop`) so a slow interval can be
attributed rather than merely observed.

**Scenario A** — no HTTP consumer, no serial monitor. **Scenario B** — one
consumer polling `get_status` as fast as it can, new TCP connection per request
(the board calls `client.stop()` after each one).

To reproduce, from [`tools/bench/`](../../tools/bench/):

```sh
cd firmware && pio run -e timing -t upload
cd ../tools/bench
./dt_stress_test.py --load idle          # scenario A
./dt_stress_test.py --load poll          # scenario B
```

---

## 3. Where the time goes

`statusUpdate()` was **98.6%** of all loop time, and `gps_read` was **73.1%** of
that. Two independent costs, both verified against the source:

**A blind 128-byte I2C read on every call.** `readGPSCoords()` issues
`Wire.requestFrom(GPS_ADDR, 128)` unconditionally, never asking the module how
much data is waiting. `Wire.begin()` is called with no `setClock()`, so the bus
runs at 100 kHz — about 90 µs per byte. That is **11.5 ms per call**, paid
whether the module had 500 bytes queued or nothing but `0xFF` filler. At loop
rate: **438 ms of every second, 44% of wall clock**, mostly clocking out
padding.

**Draining the module's output stream.** `getPVT()` with `setAutoPVT(true)`
routes into `checkUbloxI2C()`, which reads the bytes-available register and then
loops `while (bytesAvailable)` until the module is empty. The module is
configured for `COM_TYPE_UBX | COM_TYPE_NMEA` — both protocols — at 5 Hz,
producing a measured **~3 kB/s**. At 90 µs/byte that is **~270 ms/s, 27%**.

A cost model built from only those two numbers reproduces both runs exactly:

| | model | measured |
|---|---|---|
| idle | 721.5 ms/s | 721.5 ms/s |
| loaded | 393.1 ms/s | 393.1 ms/s |

The drain rate came out at 3146 B/s idle and 2830 B/s loaded — **the same
regardless of how often we read**, because it is the module's output rate, not a
function of our call count. That also explains why `gps_read`'s **mean** grew 73%
under load while its **max** did not move (74.81 → 74.38 ms): the backlog is
proportional to elapsed time. Hence a feedback loop — load slows the loop, a
slower loop makes each GPS read more expensive, which slows the loop further.

Every other block was constant to the hundredth of a millisecond across a 3×
change in loop rate (`sensors` 4.72 ms, `power` 1.43 ms, `filter` 0.65 ms,
`sanity` 0.05 ms). Exactly one block moved.

---

## 4. Caveats

- **The GPS antenna was disconnected for every run.** The module still streams
  PVT, so the costs are real, but with a live fix it emits more per epoch —
  these numbers are plausibly a floor, not a worst case. **All of this should be
  re-run with an antenna attached before it is treated as final.**
- **`statusWriteToSerial()` costs a fixed 10.01 ms once per second even with no
  monitor attached** — mean exactly equal to max in all runs, which is a timeout
  being hit, not work being done. The `if (!Serial)` guard does not early-return
  on this board; the USB CDC port evaluates as open. ~1% of the window, but it
  is a real defect.
- **Scenario B saturates the board by construction** — the client polls as fast
  as it can, so a faster board simply receives more requests. Throughput
  comparisons are sound; `dt` comparisons are between saturated states, not
  equal-load states.
- The probe's own `micros()` calls (~20 per iteration across 10 blocks) are
  inside these numbers. Negligible against a 19 ms GPS read, but not zero.

---

## 5. What to do about it

**The GPS fix is now on this branch**, reviewed and accepted. The HTTP and I2C
work is not, and stays on `perf-experiments`.

The module was configured for `COM_TYPE_UBX | COM_TYPE_NMEA` — both protocols —
emitting ~3 kB/s against ~500 B/s for the NAV-PVT messages alone, with NMEA
duplicating information the binary already carried. It is now UBX only, and the
hand-written 128-byte read is gone: the SparkFun library is the sole reader and
asks the module how much is waiting before taking it. **`setNavigationFrequency(5)`
is untouched** — the module still produces five solutions a second; it simply
stops transmitting each one twice.

Measured on one board, one network, two runs per case, before and after:

| idle | before | after |
|:--|--:|--:|
| filter update rate | 36.39 Hz | **94.76 Hz** |
| dt mean | 27.48 ms | **10.55 ms** |
| dt maximum | 91.6 – 125.2 ms | **29.8 ms** |
| GPS read, mean | 19.77 ms | **0.85 ms** |
| GPS share of every second | **71.9%** | **8.1%** |
| filter time constant | 1.35 s | **0.52 s** |

| one consumer | before | after |
|:--|--:|--:|
| filter update rate | 11.98 Hz | **17.3 – 19.3 Hz** |
| dt mean | 83.45 ms | **52.0 – 57.9 ms** |
| **updates above the 0.1 s clamp** | **42.5%** | **6.6 – 8.6%** |
| GPS read, mean | 33.01 ms | **3.55 – 3.92 ms** |
| client requests served | 10.7 /s | 16.3 /s |

Verified the path is delivering and not merely cheap: **5.13 Hz of NAV-PVT
parsed**, matching the module's configured rate. A GPS read that became cheap by
breaking would otherwise look like a win.

An intermediate version that only fixed the blind read and left NMEA enabled
reached 75 Hz idle but barely moved loaded clamping — 42.9% to 36.7%. Under load
the cost is dominated by the volume of data the module emits, not by how it is
requested, which is why turning NMEA off is what actually closes that case.

Raising I2C to 400 kHz was tested and is **not** recommended — the gain was
real, but SparkFun recommend 100 kHz for u-blox parts and the case for leaving
the vendor's recommendation was not made.

One recommendation is worth raising here because it bears directly on Priority 3:

**Make `ALPHA` dt-independent.** Even with the loop fully fixed, `dt` varies and
a fixed per-step weight makes the filter's time constant vary with it. The yaw
path in `calculations.cpp` already does this correctly (`gain = dt / (TAU + dt)`);
pitch and roll do not. That removes the entire class of problem rather than one
instance of it, and is arguably a better answer to the accumulated-error concern
than the Priority-3 formula change.
