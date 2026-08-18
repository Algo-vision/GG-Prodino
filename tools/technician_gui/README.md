# Technician GUI

PyQt5 desktop tool for a controller on the bench: live status, relays, LEDs, IP
configuration, IMU axis map, and the two calibrations.

```sh
cd tools/technician_gui
python3 gui_main.py
```

Requires `PyQt5` and `requests` (`pip install -r requirements.txt`). Log in with
the controller's credentials; the machine running this must be in the
controller's IP whitelist or every request comes back 403.

## Lineage

This is **V1.5.1's own GUI** (`GG-GRK_V1.5.1/test/tools/`), with the Calibration
box added and nothing else changed — same status sections, same axis-map
controls, same `MSB_Axis.png` diagram in the Config panel. Line endings were
normalised from CRLF to LF; `GUI_VERSION` went 1.4.0 → 1.5.0 for the new
controls.

It is deliberately **not** the v1.5.2 build of the same GUI, which sends
commands V1.5.1 does not implement (`set_device_key`, `get_router_ip` /
`set_router_ip`, `get_serial_number` / `set_serial_number`) and carries
compatibility shims for firmware that predates V1.5.1's sectioned `get_status`.

## Calibration

Two buttons that are easy to confuse, and the difference matters:

| | redefines level | written to flash | at a 10 deg tilt reports |
|:--|:--|:--|:--|
| **Burn Zero Calibration** | yes | yes | **0 deg** |
| **Re-sync to Gravity** | no | no | **10 deg** |

**Burn Zero Calibration** declares the machine's current attitude to be level.
Press it only with the machine standing on flat ground — it asks first, because
a zero burned on a slope makes that slope the new zero. It is re-burnable, and a
refusal leaves the previous calibration intact. The dialog reports the mounting
angles it measured, so a burn that looks nothing like the installation is
visible rather than silent.

**Re-sync to Gravity** takes pitch and roll straight from the accelerometer and
zeroes yaw. It does *not* level anything — on a slope it reports the slope. Its
job is removing error accumulated since gravity was last consulted, and at rest
the filter has usually converged there already, so it often changes nothing. The
dialog reports the correction it actually applied, so "nothing happened" is
visible rather than ambiguous.

Both are refused unless the machine has been still for 2 s; the buttons disable
themselves and the reason is on screen.

## The calibration status line is the thing to look at

Without a burned zero the angles are still perfectly plausible — they are simply
measured from the enclosure instead of from the machine — so nothing about the
numbers reveals an uncalibrated board. The status line says so in red.

**A USB firmware flash erases the whole chip** (`bossac --erase`), which takes
the zero calibration with it. Re-burn after any USB update. The gyro bias does
survive, because it re-learns itself at the first rest.

Against plain V1.5.1 the line reads "this firmware has no calibration commands"
and both buttons stay disabled, rather than offering controls that can only
fail.

## Tests

```sh
cd tools/technician_gui/tests && python3 -m pytest
```

39 tests, no hardware and no display needed (`conftest.py` forces Qt offscreen).

`test_api_client.py` and `test_main_widget.py` come from V1.5.1 and needed
repair: they were written before the axis-map invert flags existed, so six of
them failed on Ron's delivery as shipped — `IMU_ANGLES` unpacked as a 4-tuple
when it is a 5-tuple, and the expected `set_imu_axis_map` payload was missing
the three `*_invert` keys. Fixed here; `firmware/test/tools/` still holds the
originals, untouched.

`test_calibration.py` is new and covers the P2 controls.
