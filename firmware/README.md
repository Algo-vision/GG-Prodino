# GG-GRK Firmware - V1.5.1.1

V1.5.1 with Priority 2 on top, and nothing else. Level is burned once by a
technician instead of being guessed at every power-on, the gyro's zero is
learned whenever the machine is genuinely at rest, and a technician can
re-sync the filter to gravity on demand. See
[Calibration](#calibration) and the three commands 13-15 below.

Priority 1 - the `dt` stress test that measured this build - is in
[`docs/P1_dt_stress_test.md`](docs/P1_dt_stress_test.md); its probe compiles
out of `env:main` entirely.

## Overview

This is the firmware and HTTP API for the G.G. Controller. Key functionality includes:
- **Advanced Sensor Data:** Retrieval of a wide range of IMU (accelerometer and gyroscope) and GPS data points, from two independent IMUs.
- **Real-time Calculated Parameters:** Derivation of Pitch, Roll, and Yaw from sensor inputs, with yaw drift corrected against the GPS heading.
- **Sensor Sanity Checks:** Every sensor group is continuously validated ("not all zero, not stuck").
- **OCU Reachability Monitoring:** Link-level detection of whether the Operator Control Unit is on the network.
- **LED Status Indication:** Detailed, color-coded LED feedback reflecting system safety, sensor connectivity, and operational modes.
- **Over-the-Air (OTA) Firmware Updates:** Available in a dedicated technician mode for convenient maintenance.
- **REST API:** For controlling relays, managing LEDs, retrieving detailed device status, and configuring network settings.
- **Multi-Session Support:** Up to 5 simultaneous authenticated sessions for multiple concurrent users.

## Device Features
- 4 controllable relays
  - **Relays 0 & 1:** Auto-reset to OFF after 5 seconds when turned ON (safety feature)
  - **Relays 2 & 3:** Standard operation (remain in set state)
- IO LED (multi-color: OFF, GREEN, RED, ORANGE, AUTO)
- Internal LED
- Button for enabling technician mode
- **IMU (LSM6DS3) Data:** Linear acceleration (X, Y, Z), Angular velocity (X, Y, Z). Two IMUs are read independently.
- **GPS (u-blox) Data:** Longitude, Latitude, Altitude, Time, Linear velocity (North, East, Down), Horizontal velocity (Ground speed), Absolute heading.
- **Power Monitor (INA219):** System voltage [V] and current [A].
- **Calculated Outputs:** Pitch, Roll, Yaw.

## Third-Party Dependencies

Two vendor libraries are used as-is and are out of scope for any internal renaming:
- **`lib/ProDinoMKRZero/`** - KMP Electronics' library for the physical "ProDino MKR Zero" board this firmware runs on. Its class names and headers (e.g. `KMPProDinoMKRZero`) reflect the real board model and are left unmodified.
- **`Adafruit_INA219`** (declared in `platformio.ini` as `adafruit/Adafruit INA219@^1.2.3`) - Adafruit's library for the power-monitor chip. Our own code only refers to it generically as "power monitor" everywhere else.

## Installation Instructions

### Prerequisites
- [PlatformIO](https://platformio.org/) installed (VSCode recommended)
- Python 3.x (for the OTA uploader and the desktop GUI)

### Setup
1. Clone the repository and open this folder as the PlatformIO project root (it contains `platformio.ini`, `src/`, `include/`, and `lib/`).
2. Install PlatformIO dependencies:
   - Open the folder in VSCode with the PlatformIO extension.
   - PlatformIO will auto-install libraries from `platformio.ini`.
3. Connect your G.G. Controller to the network.
4. Build and upload firmware:
   - For direct upload: Use USB and PlatformIO's upload button.
   - For OTA: Hold the button during power-up to enter technician mode, then use the custom OTA uploader:
     ```sh
     pio run -t upload
     ```

### Building and Testing

```sh
pio run -e main        # build the embedded firmware (the default env)
pio test -e native     # host-native unit tests (requires gcc/g++ on PATH)
pio run -e timing      # bench only: env:main plus the dt probe
```

`env:timing` is `env:main` with `-D TIMING_PROBE=1`, which is the only thing
that compiles the probe in. Everywhere else it expands to nothing, so
`pio run -e main` produces a byte-identical binary with the probe sources
present or absent (verified: md5 `6021299ed8caa82c7a1c6e598c4df0e2` both ways).
**Never ship `env:timing`.**

The `native` environment is test-only and covers the hardware-free logic: `calculations`, `imu_mount_orientation`, `sanity_check`, `ocu_connection_state`, `zero_calibration`, and the rest detector and gyro-bias estimator. It is never built by a plain `pio run`.

## Technician Mode & OTA
- Hold the button for 5 seconds during startup to enter technician mode.
- In technician mode, OTA updates are enabled and the IO LED is set to solid ORANGE.

## Multi-Session Support

The device supports up to **5 concurrent authenticated sessions** (`MAX_SESSIONS`), allowing multiple users to connect simultaneously:
- Each successful login receives a unique session token
- A token expires after 5 minutes without use (`TOKEN_TIMEOUT_MS`); each authenticated request refreshes it
- When all slots are in use, the oldest session is replaced
- All sessions can retrieve status and send commands independently

## IP Whitelist

**Only clients from whitelisted IP addresses can communicate with the device.** The factory defaults are:

- `192.168.1.20`
- `192.168.1.169` &nbsp;(by convention the OCU - see [LED Status Indication](#led-status-indication))
- `192.168.1.33`

Up to `MAX_WHITELIST_IPS` (**3**) entries can be stored; the list is configurable via the GUI and the `set_ip_config` API.

Any request from a non-whitelisted IP will be rejected with HTTP 403 and:

```json
{
  "type": "error",
  "message": "IP not allowed"
}
```

## API Reference

All API requests are HTTP POST to the device IP (default: `http://192.168.1.198/`).
Payloads are JSON objects. Responses are JSON. Every request except `login` must include a valid `token`.

| # | `type` | Purpose |
| :-- | :-- | :-- |
| 1 | `login` | Authenticate, obtain a token |
| 2 | `get_status` | Everything, grouped into `config` / `overview` / `imu` / `gps` |
| 3 | `get_overview` | Power, relays, opto inputs, safety, OCU |
| 4 | `get_imu` | Both IMUs and the calculated orientation |
| 5 | `get_gps` | GPS fix, velocity, heading |
| 6 | `get_config` | Firmware, network, technician state, IMU axis map |
| 7 | `set_relay` | Set one relay |
| 8 | `set_io_led` | Set the IO LED color, or return it to AUTO |
| 9 | `set_internal_led` | Set the MCU's internal LED |
| 10 | `reset_led_control` | Force the IO LED back to automatic control, OFF |
| 11 | `set_ip_config` | Set controller IP and whitelist (reboots) |
| 12 | `set_imu_axis_map` | Set which sensor axis each angle rotates about |
| 13 | `burn_zero_calibration` | Record the machine's current attitude as level, in flash |
| 14 | `calibrate_now` | Discard accumulated drift; re-sync pitch/roll to gravity, zero yaw |
| 15 | `get_calibration` | Read calibration state and whether the machine is at rest |

### 1. Login
**Request:**
```json
{
  "type": "login",
  "user": "<username>",
  "pass": "<password>"
}
```
**Response:**
```json
{
  "type": "login_result",
  "success": true,
  "token": "<token>"
}
```

**Failed Login Response:**
If the login fails, `success` will be `false` and a `message` field will describe the error (`"Invalid username"` or `"Invalid password"`).

### 2. Get Status
Returns everything the board knows, as the union of the four `get_*` commands below. The four category objects mirror those commands field-for-field, so a client can use `get_status` for a single round trip or the individual commands for a smaller payload.

**Request:**
```json
{
  "type": "get_status",
  "token": "<token>"
}
```
**Response (all values are examples):**
```json
{
  "type": "status",
  "ledInternal": false,
  "ledIo": "GREEN",
  "button_tech": false,
  "motorWorkHours": 12.5,
  "motorWorkSeconds": 45000,

  "config": {
    "firmwareVersion": "1.5.1",
    "controllerIp": "192.168.1.198",
    "whitelistIps": ["192.168.1.20", "192.168.1.169", "192.168.1.33"],
    "technicianMode": false,
    "burnedHours": 12.5,
    "sessionHours": 0.34,
    "techLedColor": "GREEN",
    "imuPitchAxis": "X",
    "imuRollAxis": "Y",
    "imuYawAxis": "Z",
    "imuPitchInvert": false,
    "imuRollInvert": false,
    "imuYawInvert": false
  },

  "overview": {
    "powerConnected": true,
    "powerSane": true,
    "systemVoltage": 13.8,
    "systemCurrent_A": 0.25,
    "relays_status": [false, false, false, false],
    "optoin_status": [true, true, false, false],
    "safetyMode": true,
    "safetyModeDurationMs": 0,
    "ocuConnected": true,
    "ocuDisconnectedDurationMs": 0
  },

  "imu": {
    "angleSane": true,
    "pitch": 5.2,
    "roll": -3.1,
    "yaw": 45.7,
    "imuValid": true,
    "imu1Sane": true,
    "imuX": 0.01, "imuY": -0.02, "imuZ": 0.98,
    "imuGx": 1.5, "imuGy": -0.8, "imuGz": 0.2,
    "imu2Valid": true,
    "imu2Sane": true,
    "imu2X": -0.01, "imu2Y": 0.02, "imu2Z": -0.98,
    "imu2Gx": -1.5, "imu2Gy": 0.8, "imu2Gz": -0.2,
    "imuTemp": 31.5
  },

  "gps": {
    "gpsConnected": true,
    "gpsSane": true,
    "gpsSatellites": 8,
    "gpsLat": 32.0853,
    "gpsLng": 34.7818,
    "gpsAlt": 150.2,
    "gpsHeading": 75.3,
    "gpsGroundSpeed": 10.8,
    "gpsSpeedNorth": 10.5,
    "gpsSpeedEast": 2.7,
    "gpsSpeedDown": 0.1,
    "gpsTime": "2025-11-06 10:30:45",
    "lastGpsLat": 32.0853,
    "lastGpsLng": 34.7818,
    "lastGpsAlt": 150.2
  }
}
```

**Every field appears exactly once.** `firmwareVersion`, `controllerIp`, `whitelistIps` and `technicianMode` live under `config` only - they are no longer repeated at the top level.

### 3. Get Overview
**Request:** `{"type": "get_overview", "token": "<token>"}`

**Response:** `type` is `"overview"`, followed by the fields of the `overview` object above.

### 4. Get IMU
**Request:** `{"type": "get_imu", "token": "<token>"}`

**Response:** `type` is `"imu"`, followed by the fields of the `imu` object above.

### 5. Get GPS
**Request:** `{"type": "get_gps", "token": "<token>"}`

**Response:** `type` is `"gps"`, followed by the fields of the `gps` object above.

### 6. Get Config
**Request:** `{"type": "get_config", "token": "<token>"}`

**Response:** `type` is `"config"`, followed by the fields of the `config` object above.

### 7. Set Relay
**Request:**
```json
{
  "type": "set_relay",
  "token": "<token>",
  "relay_id": 0,
  "state": true
}
```
**Response:** Returns updated status (see Get Status).

**Auto-Reset Feature:**
- **Relays 0 & 1:** When set to `true`, these relays automatically reset to `false` after 5 seconds. This is a safety feature to prevent accidental prolonged power cuts.
- **Relays 2 & 3:** Standard operation - remain in the set state until manually changed.

### 8. Set IO LED
**Request:**
```json
{
  "type": "set_io_led",
  "token": "<token>",
  "color": "OFF"
}
```
`color` is one of `"OFF"`, `"GREEN"`, `"RED"`, `"ORANGE"`, `"AUTO"`.

**Response:** Returns updated status.

**LED Control Modes:**
- **Manual Colors (OFF/GREEN/RED/ORANGE):** Sets the LED to a specific color and activates manual control mode, overriding automatic LED logic.
- **AUTO:** Deactivates manual control and returns the LED to automatic mode, where it reflects OCU connectivity and safety state (see LED Status Indication section).

### 9. Set Internal LED
**Request:**
```json
{
  "type": "set_internal_led",
  "token": "<token>",
  "state": true
}
```
**Response:** Returns updated status.

### 10. Reset LED Control
Cancels manual LED override and forces the IO LED OFF; automatic control resumes on the next update.

**Request:** `{"type": "reset_led_control", "token": "<token>"}`

**Response:** Returns updated status.

### 11. Set IP Config
**Request:**
```json
{
  "type": "set_ip_config",
  "token": "<token>",
  "controller_ip": "192.168.1.198",
  "whitelist_ips": ["192.168.1.20", "192.168.1.169", "192.168.1.33"]
}
```
**Response:**
```json
{
  "success": true,
  "message": "IP configuration updated. Board will reboot."
}
```
The response is sent first, then **the board reboots after 2 seconds**. Reconnect at the new IP.

### 12. Set IMU Axis Map
Declares which physical sensor axis each angle rotates about, so the angles still read correctly when the sensor is mounted in a different orientation. See [IMU Axis Mapping](#imu-axis-mapping) for the details.

**Request:**
```json
{
  "type": "set_imu_axis_map",
  "token": "<token>",
  "pitch_axis": "X",
  "roll_axis": "Y",
  "yaw_axis": "Z",
  "pitch_invert": false,
  "roll_invert": false,
  "yaw_invert": false
}
```
Each axis is `"X"`, `"Y"` or `"Z"` (case-sensitive). All three must be **different** - the axis assignment is a permutation. The three `*_invert` flags are optional (default `false`) and negate that angle after the axis is selected.

**Response:**
```json
{
  "success": true,
  "message": "IMU axis map set to: pitch=X roll=Y yaw=Z",
  "reboot_required": true
}
```
Reboot the device, then re-burn the zero calibration (command 13) - the stored one was measured through the old map.

### 13. Burn Zero Calibration
Records the attitude the machine is standing in **right now** as this installation's level reference, and writes it to flash. It is never done automatically: a controller that has never been calibrated reports the sensor frame and says so, rather than assuming whatever slope it was switched on over is flat.

**Request:**
```json
{ "type": "burn_zero_calibration", "token": "<token>" }
```
**Response:**
```json
{
  "type": "zero_calibration_result",
  "success": true,
  "restSeconds": 7.4,
  "mountPitch": -2.31,
  "mountRoll": 0.88
}
```
`mountPitch` / `mountRoll` are the angles the sensor was sitting at when level was declared - i.e. how the bracket holds it - so a technician can sanity-check the burn before trusting it.

Refused (**HTTP 409**, `code` `E-300`) unless the machine has been at rest for `REST_SECONDS_FOR_CALIBRATION` (2 s) **and both IMUs are sane**. Both, not either: a zero burned from one IMU leaves the other uncorrected, and the angle would jump the moment the filter fell back to it. A refusal leaves the previous calibration untouched.

### 14. Calibrate Now
Initiated calibration: throws away accumulated drift. At rest the accelerometer alone gives the true pitch and roll, so the filter is snapped onto them and yaw is zeroed - gravity says nothing about heading, so there is nothing to correct yaw against while stationary.

Unlike command 13 this **stores nothing**, does not redefine level, and is valid at **any** attitude, level or not. It is the answer to error accumulating over a long run, and to angles left wrong by a hard enough shock to clip a reading.

**Request:**
```json
{ "type": "calibrate_now", "token": "<token>" }
```
**Response:**
```json
{
  "type": "calibrate_now_result",
  "success": true,
  "restSeconds": 3.1,
  "pitch": 12.44,
  "roll": -0.62,
  "yaw": 0.0
}
```
Refused (**HTTP 409**, `code` `E-301`) unless the machine has been at rest for 2 s. **It is the operator's responsibility to send this when they know the machine is standing still** - the rest check is a guard, not a guarantee.

### 15. Get Calibration
**Request:**
```json
{ "type": "get_calibration", "token": "<token>" }
```
**Response:**
```json
{
  "type": "calibration",
  "zeroCalValid": true,
  "mountPitch": -2.31,
  "mountRoll": 0.88,
  "gyroBiasValid": true,
  "gyroBias1": [0.041, -0.017, 0.006],
  "gyroBias2": [0.038, -0.021, 0.009],
  "restSeconds": 7.4,
  "restRequiredS": 2.0,
  "atRest": true
}
```
`restSeconds` / `restRequiredS` / `atRest` exist so a GUI can grey out a calibrate button **and say why**, instead of letting the request fail. `zeroCalValid`, `mountPitch`, `mountRoll`, `restSeconds` and `atRest` also ride along in `get_status` and `get_imu`, because without a burned zero the reported angles are still perfectly plausible - they are just measured from the enclosure instead of from the machine - and a client has no other way to tell a calibrated board from an uncalibrated one.

### Error Response
```json
{
  "type": "error",
  "message": "<error description>"
}
```

## Field Reference

Units: acceleration `g`, angular velocity `°/s`, orientation `°`, GPS coordinates `°`, altitude `m`, velocity `km/hr`, heading `°`, voltage `V`, current `A`, durations `ms`.

**Top level**
- **type:** Message type.
- **firmwareVersion:** Firmware version.
- **ledInternal:** State of the LED on the MCU.
- **ledIo:** State of the indication LED on the HLC (`"OFF"`, `"GREEN"`, `"RED"`, `"ORANGE"`).
- **button_tech:** True if the button on the HLC is pressed.
- **technicianMode:** True if the device is in technician mode.
- **controllerIp:** Current controller IP address.
- **whitelistIps:** Array of allowed client IP addresses.
- **motorWorkHours / motorWorkSeconds:** Total motor operational time since first boot.

**`config`**
- **burnedHours:** Operational time accumulated since the burned-hours counter was last reset [hours].
- **sessionHours:** Uptime since the last boot [hours].
- **techLedColor:** Current IO LED color (same value as `ledIo`).
- **imuPitchAxis / imuRollAxis / imuYawAxis:** Which sensor axis each angle rotates about, `"X"` / `"Y"` / `"Z"` (see command 14).
- **imuPitchInvert / imuRollInvert / imuYawInvert:** Whether that axis is negated.

**`overview`**
- **powerConnected:** True if the INA219 power monitor is connected.
- **powerSane:** Power readings pass the sanity check (see below). Additionally requires non-negative values.
- **systemVoltage / systemCurrent_A:** Readings from the power monitor, in **volts** and **amps**. `-1` for both indicates the monitor is absent. (The INA219 reports milliamps; the firmware converts on read.)
- **relays_status[0]:** Cut-off power switch for the internal Ethernet switch. Used to hard-reset it. True cuts power. **Automatically returns to False after 5 seconds.**
- **relays_status[1]:** Cut-off power switch for the internal computer. True cuts power. **Automatically returns to False after 5 seconds.**
- **relays_status[2]:** Enables/disables power (13.8V/GND) through J16, pin 1.
- **relays_status[3]:** Enables/disables power (13.8V/GND) through J16, pin 2.
- **optoin_status[0], [1]:** Indicate whether the corresponding EPC channel is in safety mode.
- **optoin_status[2], [3]:** Reserved.
- **safetyMode:** True when **both** `optoin_status[0]` and `[1]` are true.
- **safetyModeDurationMs:** How long the system has been continuously unsafe; 0 while safe.
- **ocuConnected:** True if the OCU is reachable (see LED Status Indication).
- **ocuDisconnectedDurationMs:** How long the OCU has been continuously unreachable; 0 while connected.

**`imu`**
- **pitch / roll:** Calculated orientation, self-correcting against gravity. Fused from both IMUs when both are valid; otherwise derived from whichever one is. Retains its last value if neither is valid.
- **yaw:** Calculated heading. Gyro-integrated and corrected against `gpsHeading` while moving - see [Orientation & Yaw Correction](#orientation--yaw-correction). Free-running (drifting) with no fix or below 3 km/hr.
- **imuValid / imu2Valid:** True if that IMU is communicating (I2C read succeeded).
- **imu1Sane / imu2Sane:** That IMU is healthy **and** its accelerometer is currently usable as a horizon. **These select which IMU(s) drive the orientation filter** - see [Sanity Checks](#sanity-checks).
- **angleSane:** The calculated angles pass the sanity check.
- **imuTemp:** On-chip die temperature [°C], averaged over whichever IMUs are readable. Reads above ambient due to self-heating; absolute accuracy is poor (datasheet `Toff` = ±15 °C) but the *change since boot* is accurate and is what drives gyro bias drift.
- **imuX/Y/Z, imu2X/Y/Z:** Linear acceleration per axis.
- **imuGx/Gy/Gz, imu2Gx/Gy/Gz:** Angular velocity per axis, with the learned gyro bias removed (see [Calibration](#calibration)).

**`gps`**
- **gpsConnected:** True if the GPS module is communicating.
- **gpsSane:** GPS position passes the sanity check (see below).
- **gpsSatellites:** Number of satellites used for the position fix.
- **gpsLat / gpsLng / gpsAlt:** Current position. **Cleared to 0 when the fix is lost.**
- **lastGpsLat / lastGpsLng / lastGpsAlt:** Last known-good position; *not* cleared when the fix is lost.
- **gpsHeading:** Absolute heading ("azimuth"). Also used as the absolute reference that corrects `yaw`.
- **gpsGroundSpeed:** Horizontal velocity ("speedometer").
- **gpsSpeedNorth / gpsSpeedEast:** Linear velocity components.
- **gpsSpeedDown:** Vertical velocity, derived from altitude change and low-pass filtered.
- **gpsTime:** GPS time, `"YYYY-MM-DD hh:mm:ss"`. Empty string when there is no fix.

## IMU Axis Mapping

The IMU can be bolted to the chassis in any orientation. Rather than enumerating named mounting presets, the installer states directly **which sensor axis carries each angle**:

| Angle | Default axis | Default invert |
| :-- | :-- | :-- |
| Pitch | `X` | no |
| Roll  | `Y` | no |
| Yaw   | `Z` | no |

Each angle also has an **Invert** flag that negates it, for a sensor mounted flipped end-for-end along that axis. Set it all from the desktop GUI ("IMU Axis Mapping", with the `test/tools/MSB_Axis.png` diagram shown in the Config panel for reference) or via the `set_imu_axis_map` API. It persists in flash and applies to **both** IMUs, to the accelerometer and gyroscope alike.

The defaults above are the identity mapping and reproduce exactly the behaviour of firmware that predates this setting. They follow the orientation math in `lib/GG/src/calculations.cpp`, which derives pitch from the accelerometer's Y/Z components and integrates `gx`, and derives roll from X and integrates `gy`. **If your convention is "X = roll, Y = pitch", set Pitch=`Y` and Roll=`X`** - that is exactly what this feature is for.

Implementation notes:
- `applyImuAxisMap()` (`lib/GG/src/imu_mount_orientation.hpp`) reorders each raw reading from sensor-native axes into the fixed logical frame the orientation math expects, so `calculations.cpp` itself never changes when the mounting does.
- The map must be a **permutation** - two angles cannot share an axis, which would drop one sensor axis and double-count another. The API rejects it, the GUI blocks it, and an invalid map read back from flash falls back to the default.
- Inversion is applied **after** the axis is chosen, so it negates the remapped component, not the same-named raw one.
- Invert flags are stored as bytes and must be exactly 0 or 1; that is how junk from a pre-invert firmware's flash is rejected, falling back to the default map.
- Re-burn the zero calibration after changing it: the stored gravity direction was measured through the old map, and is meaningless under a new one.

## Calibration

Three separate things, often confused. Two of them are new in V1.5.1.1.

| | what it corrects | when | stored? |
| :-- | :-- | :-- | :-- |
| **Axis map** | how the sensor is bolted on, to the nearest 90° | once, at install | yes |
| **Zero calibration** | the residual tilt of the bracket, to a fraction of a degree | once, by a technician on flat ground | yes, flash |
| **Gyro bias** | the rate the gyro reports when it is not turning | continuously, whenever the machine is at rest | yes, as a seed |
| **Calibrate now** | drift the filter has accumulated since | on demand, machine stationary | no |

### What V1.5.1 did, and why it changed

`setup()` averaged 100 accelerometer and 100 gyro samples per IMU - four seconds of blocking boot - and treated the result as both "level" and "zero rotation". Both assume the machine is standing still **and** on flat ground at the instant it is switched on, and neither can be promised. Power up on a slope and the slope becomes level. Power up on a running vehicle and a real rotation rate becomes the gyro's zero, after which yaw drifts for the whole session.

V1.5.1.1 does neither at boot. `main.cpp` has no `calibrateIMU()`.

### Zero calibration

A technician who can see the machine is flat sends `burn_zero_calibration` (command 13). What is stored is the **gravity direction** each IMU reads at that moment, not an offset - and that distinction is the whole point. Subtracting a constant from `ax`/`ay` is only correct at the attitude it was captured in; a rotation is correct at every attitude. Both the accelerometer and the gyro vectors are rotated through it, because a sensor tilted on its bracket has its rotation axes tilted by exactly the same amount.

Until one is burned the transform is the identity: the controller reports the sensor frame, uncorrected, and says so via `zeroCalValid`. That is the honest default - it is not a guess about how the unit is mounted.

Both markers are validated independently in flash (`ZERO_CAL_MARKER`, `GYRO_BIAS_MARKER`), so a record written by firmware that predates these fields reads as "uncalibrated" rather than installing whatever bytes happened to be there.

> **A USB flash erases the whole chip.** `bossac --erase` takes the stored configuration with it, including the zero calibration - re-burn it after any USB update. The gyro bias does not need re-burning; it re-learns at the first rest.

### Gyro bias

Learned, never assumed. The rest detector (`lib/GG/src/rest_detector.hpp`) watches for `|a|` within 0.15 g of 1 g and every gyro axis under 8 °/s; after 1 s of that the bias estimator (`lib/GG/src/gyro_bias.hpp`) pulls toward the raw rates with a 30 s time constant. It is fed the **raw** rates deliberately - feeding back the corrected ones would drive the estimate to zero and undo the correction it exists to provide.

It rides to flash on the periodic `configSave()` rather than writing on every update, which would erase a flash page every few seconds. On the next power cycle it is loaded as a starting point, not as truth.

> The rest thresholds are **bench numbers** - a stationary board never exceeded 0.098 g or 4.35 °/s across 66271 samples. A machine idling with its engine running will vibrate considerably more. Confirm them on the vehicle before relying on them.

### Calibrate now

`calibrate_now` (command 14) corrects the estimate without redefining anything. It exists for two failure modes: error accumulating over a long run, and angles left wrong after a shock hard enough to clip a reading. Pitch and roll are recomputed from gravity alone; yaw is zeroed, because at rest there is no reference to correct it against with this hardware. The GPS gate needs real motion before it touches yaw again, so the zero stands until the machine moves.

## Orientation & Yaw Correction

Pitch and roll are **self-correcting**: the accelerometer's gravity vector is an absolute long-term reference, so a complementary filter (`ALPHA = 0.98`) holds them steady no matter how long the board runs.

Yaw has no such reference — gravity says nothing about rotation about the vertical axis — so on its own it is pure gyro integration and drifts without bound. The firmware therefore corrects yaw against the **GPS course over ground** (`gpsHeading`), which plays the same role for yaw that gravity plays for pitch/roll. This applies identically whether orientation comes from IMU1 alone, IMU2 alone, or both fused.

**When the correction is applied.** `gpsHeading` is only used while there is a valid fix **and** ground speed is at least `GPS_HEADING_MIN_SPEED_KMH` (3 km/hr). Course over ground is meaningless at a standstill — it is noise, and correcting yaw with it would make a stationary board slowly rotate to a random bearing. Below the threshold or with no fix, yaw free-runs on the gyro exactly as before.

**First fix snaps.** The gyro's starting yaw is an arbitrary reference, not an estimate worth preserving, so the first usable heading after boot (or after a long fix loss) sets yaw directly rather than letting the filter walk there.

**Rate-independent gain.** The correction is expressed as a time constant (`YAW_GPS_TAU_S = 5s`), not a fixed per-step weight like pitch/roll use: `gain = dt / (TAU + dt)`. Convergence therefore depends on elapsed time only, so it rides out a stalled or jittery main loop unchanged — something the pitch/roll form cannot do, which is why `statusUpdate()` is pinned to a fixed cadence (see [Sensor sampling](#notes)).

**Wrap-safe.** The correction takes the shortest signed path to the target bearing, so a heading change across the ±180° seam turns the short way instead of spinning the long way round.

**Limits worth knowing:**
- The correction **bounds** drift rather than eliminating it. A constant residual gyro bias leaves a steady-state offset of roughly `bias × TAU` — a 2°/s bias settles about 10° off. (Simulated: uncorrected, that same bias drifts 120° in 60 s.) The rest-time bias estimator removes most of the bias; eliminating the remainder entirely would need an integral bias estimator.
- Yaw becomes an **absolute bearing** once corrected, not a relative angle from power-on.
- **Convention:** blending assumes yaw and `gpsHeading` share a sense of rotation. If they turn opposite ways the filter pulls yaw toward the mirrored bearing instead of converging — invert the yaw axis in the [IMU axis map](#imu-axis-mapping), which flips the sign of the gyro rate feeding yaw.

### Sanity Checks

Each `*Sane` flag is the result of a continuous "not all zero, not stuck" check (`lib/GG/src/sanity_check.hpp`): the reading fails if every value in the group is zero, or if the whole group has been byte-identical for 3 consecutive updates (a frozen sensor). The power monitor additionally fails on negative voltage or current, which this hardware never legitimately produces; that extra rule is deliberately *not* applied to IMU/angle/GPS values, which are legitimately signed. A `*Sane` flag is also false whenever the corresponding sensor is not valid/connected.

**`imu1Sane` / `imu2Sane` carry a second condition and have a second job.**

They additionally require the accelerometer to be usable as a horizon: `|‖a‖ − 1 g| ≤ 0.05 g` (`ACCEL_TRUST_TOLERANCE_G`). An accelerometer cannot distinguish vehicle acceleration from gravity, so when the measured magnitude departs from 1 g we know extra acceleration is present and the "gravity" direction is not trustworthy.

These two flags then **select which IMU(s) feed the orientation filter**:

| State | Behaviour |
| :-- | :-- |
| both sane | Fused (`calculateMergedOrientation`) |
| one sane | That IMU alone |
| neither sane, at least one healthy | That IMU's gyro alone — **coasting**, accelerometer term dropped |
| neither healthy | Angles frozen at their last values |

A misbehaving-but-responding IMU is therefore excluded from the fusion rather than silently averaged in at full weight, which is what the previous `imuValid`-based selection did.

The "coast" fallback matters: if a sane-check failure caused the IMU to be dropped outright, then every time the vehicle accelerated hard both IMUs would drop and the angles would freeze. Instead the gyro keeps integrating and only the accelerometer correction is suspended — accurate short-term, drifting slowly, and far better than following a false horizon. See [imuLimitations.md](imuLimitations.md) for what this gate does and does not catch.

## Error Reference

The GRK board communicates errors through a combination of HTTP status codes and specific messages within the JSON response payload.

### Error codes

Every error response carries a stable `code` field alongside `message`:

```json
{ "type": "error", "code": "E-201", "message": "Invalid relay number" }
```

| Code | HTTP | Message | Cause |
| :-- | :-- | :-- | :-- |
| `E-100` | 400 | `Request timeout` | Body not received within 100 ms |
| `E-101` | 401 | `Token required.` | `token` field absent |
| `E-102` | 401 | `Invalid or expired token.` | Token unknown or timed out |
| `E-103` | 403 | `IP not allowed` | Source IP not whitelisted |
| `E-110` | 200 | `Invalid username` | Login: unknown user (in `login_result`) |
| `E-111` | 200 | `Invalid password` | Login: wrong password (in `login_result`) |
| `E-200` | 200 | `Unknown request type` | Unrecognised `type` |
| `E-201` | 200 | `Invalid relay number` | `relay_id` outside 0-3 |
| `E-202` | 200 | `Invalid LED color` | Colour not in permitted set |
| `E-203` | 200 | `Invalid IP address or whitelist entry provided.` | Malformed address, or more than 3 whitelist entries |
| `E-204` | 200 | `Invalid IMU axis (expected X, Y or Z)` | Bad axis token |
| `E-205` | 200 | `Pitch, roll and yaw must each use a different axis` | Axis map not a permutation |
| `E-300` | 409 | `machine is not standing still` / `both IMUs must be healthy to burn a zero calibration` / `measured gravity vector was unusable - nothing was changed` | `burn_zero_calibration` refused |
| `E-301` | 409 | `machine is not standing still` / `no healthy IMU to calibrate from` | `calibrate_now` refused |

Codes are stable across releases; `message` text is not guaranteed to be. Clients should branch on `code`.

Note the split: transport and authentication failures use HTTP status codes; application-level rejections return **HTTP 200** with an error body. The two calibration refusals are the exception - they are 409 Conflict, because "not right now" is a state conflict rather than a malformed request.

### OTA Update Errors

Errors during Over-the-Air (OTA) firmware updates (only available in technician mode) are printed to the device's serial console and are not transmitted over the network as HTTP responses. These messages follow the format:

*   `OTA Error[<error_code>]: <error_message>`

## DeviceStatus Structure

The status response fields correspond to the following structure in firmware (`include/status_manager.hpp`):

```cpp
struct DeviceStatus {
  bool relays_status[4];
  bool optos_status[4];
  float imuX, imuY, imuZ;             // IMU1 Linear Accelerations
  float imuGx, imuGy, imuGz;          // IMU1 Angular Velocities
  float pitch, roll, yaw;             // Calculated Orientations
  bool imuValid;                      // True if IMU1 is communicating
  float imu2X, imu2Y, imu2Z;          // IMU2 Linear Accelerations
  float imu2Gx, imu2Gy, imu2Gz;       // IMU2 Angular Velocities
  bool imu2Valid;                     // True if IMU2 is communicating
  bool imu1Sane, imu2Sane, angleSane; // Sanity checks
  double gpsLat, gpsLng, gpsAlt;      // GPS Coordinates & Altitude
  double lastGpsLat, lastGpsLng, lastGpsAlt; // Last known-good GPS position
  char gpsTime[20];                   // "YYYY-MM-DD hh:mm:ss"
  float gpsSpeedNorth, gpsSpeedEast, gpsSpeedDown;
  float gpsGroundSpeed, gpsHeading;
  uint8_t gpsSatellites;
  float gpsHAcc, gpsVAcc;             // Accuracy estimates (mm), not currently in the API
  double gpsAltEllipsoid;             // Height above WGS84 ellipsoid (mm), not currently in the API
  bool gpsValid, gpsConnected, gpsSane;
  bool safetyMode;
  unsigned long safetyModeUnsafeDurationMs;
  bool ledInternal;
  LED_STATES ledIo;                   // "OFF", "GREEN", "RED", "ORANGE"
  bool button_tech, technicianMode;
  bool powerConnected, powerSane;
  float systemVoltage, systemCurrent_A;  // Volts / Amps
};
```

## Serial Output

Once per second the board prints the complete `get_status` document to the serial console at **115200 baud**. The dump is skipped entirely when no USB host has opened the port, so an unattended board spends no time formatting output nobody reads. `statusWriteToSerial()` renders that JSON document directly - top-level scalars first, then one block per category - so the serial log can never drift out of sync with the API:

```
========== DEVICE STATUS ==========
  localIp: 192.168.1.198
  type: "status"
  firmwareVersion: "1.5.1"
  ...
[config]
  imuPitchAxis: "X"
  imuRollAxis: "Y"
  imuYawAxis: "Z"
  imuPitchInvert: false
  ...
[overview]
  relays_status: [false,false,false,false]
  ocuConnected: true
  ...
[imu]
  pitch: 5.2
  ...
[gps]
  gpsSatellites: 8
  ...
===================================
```

`localIp` is the only line not taken from the JSON: it is the board's actual link address, whereas `controllerIp` is the *configured* one.

## LED Status Indication

The IO LED provides critical system status feedback based on OCU (Operator Control Unit) connectivity and safety mode:

| LED State          | Condition                                          | Description                                                                                     |
| :----------------- | :-------------------------------------------------- | :------------------------------------------------------------------------------------------------ |
| **Solid Orange**    | Technician mode, or within the boot-up grace period | Device is in technician mode, or still within `LED_BOOT_GRACE_MS` (60s) of startup while sensors/network/OCU comm settle. |
| **Solid Green**     | OCU connected & safety mode active                  | The OCU was confirmed reachable within the timeout window. Both optocoupler safety inputs (`optoin_status[0]` and `[1]`) are TRUE. |
| **Blinking Green**  | OCU connected & safety mode not active              | OCU connection is healthy, but at least one optocoupler safety input is FALSE.                  |
| **Blinking Red**    | OCU disconnected & safety mode active               | The OCU was not confirmed reachable within the timeout window, but the optocoupler safety inputs are both TRUE. |
| **Solid Red**       | OCU disconnected & safety mode not active           | The OCU is unreachable, and at least one optocoupler safety input is FALSE.                     |

### OCU Reachability

See [`include/ocu_monitor.hpp`](include/ocu_monitor.hpp) for the implementation. `ocuConnected` tracks **link-level reachability** of the OCU - the whitelisted IP whose last octet is **169** - so nothing has to be running on the OCU, and the state is independent of whether the desktop GUI is open. Three signals, cheapest first:

1. **PHY link** (`Ethernet.linkStatus()`). Cable unplugged → disconnected immediately, no probe attempted.
2. **Traffic already arriving from the OCU.** Free, and it suppresses the probe below.
3. **ARP probe.** A one-byte UDP datagram is sent to the OCU; the W5500 must resolve its MAC by ARP before it can transmit, so a successful send proves the OCU answered ARP - it is powered on and on the network. Nothing needs to listen at the far end.

It goes false after `OCU_ACTIVITY_TIMEOUT_MS` (5s) without a confirmation.

A literal ICMP `ping` cannot be used as the signal: the W5500 answers echo requests in hardware, so pings *to* the board never reach the firmware, and the Ethernet library has no ICMP sender. The ARP probe tests the same thing one layer lower.

**Blocking cost:** probing a host that is present resolves in ~1 ms. Probing an *absent* host blocks the main loop for the ARP retry window, so probes back off to one per 10s once the OCU is considered gone, and `ocuMonitorInit()` lowers the W5500 retry count (`RCR`) from 8 to 3 - 4 attempts ≈ 800 ms instead of ~1.8 s. `RCR` is a global register and also governs TCP retransmission for the HTTP server; raise `OCU_ARP_RETRY_COUNT` if this board is ever run over a lossy link.

## OTA Firmware Updates
Firmware can be updated Over-The-Air (OTA) through the desktop GUI (see [`test/tools/`](test/tools/)) in Technician Mode. Select a `.bin` file and initiate upload.

## Notes
- **Default Login:** User `admin`, password `1234`.
- **Default Device IP:** `192.168.1.198` (configurable via GUI and API).
- **Default Whitelist IPs:** `192.168.1.20`, `192.168.1.169`, `192.168.1.33` (configurable via GUI and API; maximum 3 entries).
- **Serial Console:** 115200 baud.
- **OTA Updates:** Only available in technician mode (hold the technician button for 5 seconds during startup), and can be initiated via the GUI or a separate uploader tool.
- **IMU Calibration:** Nothing is calibrated at startup. Burn the zero calibration once, with the machine on flat ground (command 13), and re-burn it after changing the IMU axis map. See [Calibration](#calibration).
- **Connection model:** The HTTP server uses one connection per request (`Connection: close`), which is intentional for the 8-socket W5500.
- **Sensor sampling:** `statusUpdate()` runs **only** from `loop()`, on a fixed 10 ms cadence (`STATUS_UPDATE_INTERVAL_MS`). API responses report the most recent sample and never trigger a read, so values can be up to 10 ms old. This is deliberate: the pitch/roll filter uses a fixed per-step weight, so sampling per HTTP request made the angles depend on the polling rate. See [imuLimitations.md](imuLimitations.md) section 4.1.
