# Technician GUI

PyQt5 desktop tool for a controller on the bench: live status, relays, LEDs, IP
configuration, IMU axis map, and the zero / drift calibrations.

```sh
cd tools/technician_gui
python3 gui_main.py
```

Requires `PyQt5` and `requests`. Log in with the controller's credentials; the
machine running this must be in the controller's IP whitelist or every request
comes back 403.

## It is matched to this firmware, not to v1.5.2

The v1.5.2 build of this GUI sends six commands V1.5.1 does not implement, so
those controls could only ever return "unknown request type". Removed here:

| removed | why |
|:--------|:----|
| Telemetry Key | `set_device_key` - the v1.5.2 encrypted-telemetry work |
| Router Configuration | `get_router_ip` / `set_router_ip` |
| Serial number | `get_serial_number` / `set_serial_number`, added after V1.5.1 |

The "IMU Mount Orientation" dropdown sent `set_imu_mount_orientation`, which
V1.5.1 replaced with an explicit per-axis map; it is now three axis choices and
three invert flags matching `set_imu_axis_map`.

The firmware uploader stays - OTA does exist in V1.5.1.

## Calibration

Two buttons that are easy to confuse, and the difference matters:

| | redefines level | written to flash | at a 10 deg tilt reports |
|:--|:--|:--|:--|
| **Burn Zero Calibration** | yes | yes | **0 deg** |
| **Re-sync to Gravity** | no | no | **10 deg** |

**Burn Zero Calibration** declares the machine's current attitude to be level.
Press it only with the machine standing on flat ground. It is re-burnable.

**Re-sync to Gravity** takes pitch and roll straight from the accelerometer and
zeroes yaw. It does *not* level anything - on a slope it reports the slope. Its
job is removing error accumulated since gravity was last consulted, and at rest
the filter has usually converged there already, so it often changes nothing. The
dialog reports the correction it actually applied, so "nothing happened" is
visible rather than ambiguous.

Both are refused unless the machine has been still long enough; the buttons
disable themselves and the reason is on screen.

## The calibration status line is the thing to look at

Without a burned zero the angles are still perfectly plausible - they are simply
measured from the enclosure instead of from the machine - so nothing about the
numbers reveals an uncalibrated board. The status line says so in red.

**A USB firmware flash erases the whole chip** (`bossac --erase`), which takes
the zero calibration with it. Re-burn after any USB update. The gyro bias does
survive, because it re-learns itself at the first rest.
