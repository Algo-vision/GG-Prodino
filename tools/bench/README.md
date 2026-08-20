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

## `imu_agreement_test.py`

P3 item 2 — the two-IMU agreement check.

```sh
./imu_agreement_test.py           # ~2.5 minutes
./imu_agreement_test.py --quick   # shorter windows
```

**It tests for false positives, not for detection.** Both sensors sit on one
board reading one gravity vector, so a genuine disagreement cannot be provoked
without physically moving one relative to the other. That path is covered by
`firmware/test/test_imu_agreement` — eight cases including a dead sensor reading
zero, an inverted axis, and one reading 2.5x the other.

What can bite in the field is the opposite. When the check trips it clears
**both** `imu1Sane` and `imu2Sane`, and those flags select which IMU drives the
orientation filter — so a spurious trip freezes pitch, roll and yaw at their
last values. On a moving machine that is worse than the fault being looked for.

| step | you do | it proves |
|:--|:--|:--|
| 1 | flat and still | a healthy pair agrees, at least one axis is compared |
| 2 | nothing | it does not trip while nothing is happening |
| 3 | **tilt to ~45 deg** | coverage RISES with tilt — two axes compared, not zero |
| 4 | **shake it hard** | no false trip under vibration. The one that matters |
| 5 | rotate through many attitudes | coverage never drops to zero axes |
| 6 | flat again | the flags recover and the angles are live |

It reads `imuDisagreeCount` rather than polling `imusAgree`: the check runs at
loop rate (~95 Hz) and this polls at about 30, so a brief trip would freeze the
angles and never land in a sample. A latched counter cannot miss one.

Every phase also reports the **worst per-axis ratio** it saw against the x2
limit, so the margin is visible rather than assumed. On the bench, level, that
figure is 1.03.

## What it cannot check here

The rest thresholds are bench numbers - a stationary board never exceeded
0.098 g or 4.35 dps across 66271 samples. A machine idling with its engine
running will vibrate considerably more, and these want confirming on the vehicle
before anyone relies on them.
