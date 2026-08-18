# Bench tests

Hardware acceptance tests. They talk to a real controller over the network, so
the machine running them must be in its IP whitelist.

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
