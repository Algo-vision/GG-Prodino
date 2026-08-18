# Bench tests

Hardware acceptance tests. They talk to a real controller over the network, so
the machine running them must be in its IP whitelist.

## `dt_stress_test.py`

Priority 1: what `dt` in the angle formula actually is, at the controller's
lightest and heaviest load. Needs a board flashed with the instrumented build —
`env:main` compiles the probe out entirely and answers "unknown request type".

```sh
cd ../../firmware && pio run -e timing -t upload
cd ../tools/bench
./dt_stress_test.py --load idle      # scenario A: no consumer, no serial monitor
./dt_stress_test.py --load poll      # scenario B: one consumer polling flat out
```

It arms the probe, applies the load for the window, then reads the **frozen**
result — the board stops accumulating when the window expires, so the request
that reads the numbers is never inside them. That matters most for the idle run,
where one HTTP request is the entire load being excluded.

**Close the USB serial monitor first.** `statusWriteToSerial()` skips itself
when no host has the port open, so an unopened port already means "no serial
printing" — but an open monitor silently adds ~55 lines/s to every result.

Results and analysis: [`firmware/docs/P1_dt_stress_test.md`](../../firmware/docs/P1_dt_stress_test.md).

## `p2_acceptance.py`

Interactive: it tells you what to do to the board and checks the result.

```sh
./p2_acceptance.py --tilt 30      # with an inclinometer reference
./p2_acceptance.py                # without; the tilt-accuracy step is skipped
```

22 checks across 11 groups. What each is really for:

| step | you do | it proves |
|:--|:--|:--|
| 2-3 | shake the board | rest detection collapses, and both calibrations are refused |
| 4 | stop, flat | rest re-establishes; burning works and reads level |
| 5 | tilt to a known angle | the rotation maths, against a real reference |
| 6 | **tilt 20-45 deg and burn there** | that attitude becomes level, and flat then reads **minus** the tilt |
| 7 | flat, burn again | the calibration is not one-shot |
| 8 | nothing | diagnostic - see below |
| 9 | nothing | it all survives a reboot, and comes up level with **no boot calibration** |
| 10 | **shake it while it reboots** | the case that motivated removing the boot calibration |
| 11 | wait 6 minutes | the periodic flash write does not erase the calibration |

**Step 6 is the important one.** Burn at 30 degrees, return to flat, and it must
read about -30. Anything else means the rotation is wrong. It refuses to pass if
the board was not actually tilted, because every assertion in it is satisfied by
zero and the test would otherwise prove nothing.

**Step 8 is a diagnostic, not a pass/fail.** Run "Re-sync to Gravity" on flat
ground: if pitch/roll barely move, the zero calibration is good and the filter
had not drifted. If they jump, the zero calibration itself is wrong.

**Step 10 is the point of P2.** V1.5.1 measured the gyro's zero during setup(),
so powering up on a running machine captured a real rotation rate as "zero" and
yaw drifted for the whole session.

## What it cannot check here

The rest thresholds are bench numbers - a stationary board never exceeded
0.098 g or 4.35 dps across 66271 samples. A machine idling with its engine
running will vibrate considerably more, and these want confirming on the vehicle
before anyone relies on them.
