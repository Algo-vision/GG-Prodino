# GG-GRK Firmware - V1.5

## Overview

This is the firmware and HTTP/MQTT API for the G.G. Controller. Key functionality includes:
- **Advanced Sensor Data:** Retrieval of a wide range of IMU (accelerometer and gyroscope) and GPS data points.
- **Real-time Calculated Parameters:** Derivation of Pitch, Roll, and Yaw from sensor inputs.
- **LED Status Indication:** Detailed, color-coded LED feedback reflecting system safety, sensor connectivity, and operational modes.
- **Over-the-Air (OTA) Firmware Updates:** Available in a dedicated technician mode for convenient maintenance.
- **REST API:** For controlling relays, managing LEDs, retrieving detailed device status, and configuring network settings.
- **UDP Status Broadcast:** Real-time status updates broadcast to all clients for efficient multi-client monitoring.
- **Multi-Session Support:** Up to 5 simultaneous authenticated sessions for multiple concurrent users.

## Device Features
- 4 controllable relays
  - **Relays 0 & 1:** Auto-reset to OFF after 5 seconds when turned ON (safety feature)
  - **Relays 2 & 3:** Standard operation (remain in set state)
- IO LED (multi-color: OFF, GREEN, RED, ORANGE, AUTO)
- Internal LED
- Button for enabling technician mode
- **IMU (LSM6DS3) Data:** Linear acceleration (X, Y, Z), Angular velocity (X, Y, Z).
- **GPS (u-blox) Data:** Longitude, Latitude, Altitude, Time, Linear velocity (North, East, Down), Horizontal velocity (Ground speed), Absolute heading, Horizontal accuracy (hAcc), Vertical accuracy (vAcc), Height above ellipsoid.
- **Calculated Outputs:** Pitch, Roll, Yaw.

## Third-Party Dependencies

Two vendor libraries are used as-is and are out of scope for any internal renaming:
- **`lib/ProDinoMKRZero/`** - KMP Electronics' library for the physical "ProDino MKR Zero" board this firmware runs on. Its class names and headers (e.g. `KMPProDinoMKRZero`) reflect the real board model and are left unmodified.
- **`Adafruit_INA219`** (declared in `platformio.ini` as `adafruit/Adafruit INA219@^1.2.3`) - Adafruit's library for the power-monitor chip. Our own code only refers to it generically as "power monitor" everywhere else.

## Installation Instructions

### Prerequisites
- [PlatformIO](https://platformio.org/) installed (VSCode recommended)
- Python 3.x (for OTA uploader)

### Setup
1. Clone the repository and open the `firmware/` folder as the PlatformIO project root (it contains `platformio.ini`, `src/`, `include/`, and `lib/`).
2. Install PlatformIO dependencies:
   - Open `firmware/` in VSCode with the PlatformIO extension.
   - PlatformIO will auto-install libraries from `platformio.ini`.
3. Connect your G.G. Controller to the network.
4. Build and upload firmware:
   - For direct upload: Use USB and PlatformIO's upload button.
   - For OTA: Hold the button during power-up to enter technician mode, then use the custom OTA uploader:
     ```sh
     pio run -t upload
     ```

## Technician Mode & OTA
- Hold the button for 5 seconds during startup to enter technician mode.
- In technician mode, OTA updates are enabled and the IO LED is set to ORANGE.

## UDP Status Broadcast

**New in v1.4:** The device now broadcasts real-time status updates via UDP for efficient multi-client monitoring.

### How It Works
- **Broadcast Address:** `192.168.1.255:5000` (local subnet broadcast)
- **Frequency:** 5 Hz (every 200ms)
- **Protocol:** UDP (connectionless, low overhead)
- **Format:** JSON (same structure as HTTP `get_status` response)

### Benefits
- **Unlimited Monitoring Clients:** Any number of clients can listen simultaneously without additional load on the board
- **Real-time Updates:** 5 updates per second for near-instant responsiveness
- **Zero Collision:** Unlike HTTP polling, UDP broadcast has no collision issues
- **Reduced Network Load:** Board sends one packet regardless of how many clients are listening

**Note:** UDP is unreliable - occasional packet loss is normal and expected. Clients should handle this gracefully with a fallback HTTP poll.

## MQTT Integration

**New in v1.5:** The device supports MQTT for robust, asynchronous telemetry and cloud integration.
- **Broker:** Configurable in `include/mqtt_handler.hpp` (default: RUTX12 or local broker).
- **Topics:**
  - `grk/gps/position`: Latitude and Longitude.
  - `grk/gps/velocity`: Ground speed.
  - `grk/gps/heading`: Absolute heading.
  - `grk/imu/accel`: Raw accelerometer data.
  - `grk/imu/gyro`: Raw gyroscope data.
  - `grk/imu/orientation`: Pitch, Roll, and Yaw.
  - `grk/sensors/optos`: Optocoupler states.
  - `grk/sensors/button_tech`: Technician button state.
  - `grk/relays/state`: Current relay states.
  - `grk/gps/satellites`: Number of GPS satellites.
  - `grk/gps/accuracy`: Position accuracy estimates (`hAcc`, `vAcc` in mm) and height above ellipsoid (`altEllipsoid` in mm).

See [`server/mqtt_server/`](../server/mqtt_server/) for a debug subscriber and [`server/web_ui/`](../server/web_ui/) for the live dashboard that consume these topics.

## Multi-Session Support

The device supports up to **5 concurrent authenticated sessions**, allowing multiple users to connect simultaneously:
- Each successful login receives a unique session token
- Tokens are valid until overwritten by newer sessions (oldest-first replacement)
- All sessions can retrieve status and send commands independently
- Ideal for multi-user environments (e.g., 3 monitoring stations)

## IP Whitelist

**Only clients from the following IP addresses can communicate with the device:**

- `192.168.1.20`
- `192.168.1.169`
- `192.168.1.33`

Any request from a non-whitelisted IP will be rejected with an error response:

```json
{
  "type": "error",
  "message": "IP not allowed"
}
```

## API Reference

All API requests are HTTP POST to the device IP (default: `http://192.168.1.198/`).
Payloads are JSON objects. Responses are JSON.

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
If the login fails, `success` will be `false` and a `message` field will describe the error.
```json
{
  "type": "login_result",
  "success": false,
  "message": "Invalid username"
}
```
or
```json
{
  "type": "login_result",
  "success": false,
  "message": "Invalid password"
}
```

### 2. Get Status
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
  "type": "status",                               // string
  "firmwareVersion": "1.5.0",                     // string
  "relays_status": [false, false, false, false],  // Array of booleans
  "optoin_status": [false, false, false, false],  // Array of booleans
  "imuX": 0.01,                                   // float
  "imuY": -0.02,                                  // float
  "imuZ": 0.98,                                   // float
  "imuGx": 1.5,                                   // float
  "imuGy": -0.8,                                  // float
  "imuGz": 0.2,                                   // float
  "pitch": 5.2,                                   // float
  "roll": -3.1,                                   // float
  "yaw": 45.7,                                    // float
  "imuValid": true,                               // boolean
  "gpsLat": 32.0853,                              // float
  "gpsLng": 34.7818,                              // float
  "gpsAlt": 150.2,                                // float
  "gpsTime": "2025-11-06 10:30:45",               // string
  "gpsSpeedNorth": 10.5,                          // float
  "gpsSpeedEast": 2.7,                            // float
  "gpsSpeedDown": 0.1,                            // float
  "gpsGroundSpeed": 10.8,                         // float
  "gpsHeading": 75.3,                             // float
  "gpsValid": true,                               // boolean
  "GPSConnected": true,                           // boolean
  "ledInternal": true,                            // boolean
  "ledIo": "GREEN",                               // string
  "button_tech": false                            // boolean
}
```

**Description:**
- **type:** Message type.
- **firmwareVersion:** Firmware version.
- **relay_status[0]:** Cut-off power switch for the internal Ethernet switch. Used to hard-reset the Ethernet switch if needed. True to cut-off power.
**Automatically returns to False after 5 seconds.**
- **relay_status[1]:** Cut-off power switch for the internal Computer. Used to hard-reset the computer if needed. True to cut-off power.
**Automatically returns to False after 5 seconds.**
- **relay_status[2]:** Switch to enable or disable power (13.8V/GND) through J16, pin 1.
- **relay_status[3]:** Switch to enable or disable power (13.8V/GND) through J16, pin 2.
- **optoin_status[0]:** Indicates if the first channel of the EPC is in safety mode.
- **optoin_status[1]:** Indicates if the first channel of the EPC is in safety mode.
- **optoin_status[2]:** Reserved.
- **optoin_status[3]:** Reserved.
- **imuX:** IMU Linear acceleration in the X-axis [g].
- **imuY:** IMU Linear acceleration in the Y-axis [g].
- **imuZ:** IMU Linear acceleration in the Z-axis [g].
- **imuGx:** IMU Angular velocity in the X-axis [º/s].
- **imuGy:** IMU Angular velocity in the Y-axis [º/s].
- **imuGz:** IMU Angular velocity in the Z-axis [º/s].
- **pitch:** Calculated Pitch [º].
- **roll:** Calculated Roll [º].
- **yaw:** Calculated Yaw [º].
- **imuValid:** True if IMU module is communicating.
- **gpsLat:** GPS Latitude [º].
- **gpsLng:** GPS Longitude [º].
- **gpsAlt:** GPS Altitude [m].
- **gpsTime:** GPS time [YYYY-MM-DD hh:mm:ss].
- **gpsSpeedNorth:** GPS Linear velocity North [km/hr].
- **gpsSpeedEast:** GPS Linear velocity East [km/hr].
- **gpsSpeedDown:** GPS Linear velocity Down [km/hr].
- **gpsGroundSpeed:** GPS Horizontal velocity ("Speedometer") [km/hr].
- **gpsHeading:** GPS Absolute heading ("Azimuth") [º].
- **gpsValid:** True if GPS fix is valid.
- **gpsConnected:** True if GPS module is communicating.
- **gpsSatellites:** Number of satellites used for position fix.
- **gpsHAcc:** GPS horizontal position accuracy estimate [mm].
- **gpsVAcc:** GPS vertical position accuracy estimate [mm].
- **gpsAltEllipsoid:** GPS height above WGS84 ellipsoid [mm].
- **ledInternal:** Status of the LED on the MCU.
- **ledIo:** Status of the indication LED on the HLC.
- **button_tech:** True if the button on the HLC is pressed.


### 3. Set Relay
**Request:**
```json
{
  "type": "set_relay",
  "token": "<token>",
  "relay_id": <0-3>,
  "state": true
}
```
**Response:**
- Returns updated status (see Get Status)

**Auto-Reset Feature:**
- **Relays 0 & 1:** When set to `true`, these relays will automatically reset to `false` after 5 seconds. This is a safety feature to prevent accidental prolonged power cuts.
- **Relays 2 & 3:** Standard operation - remain in the set state until manually changed.

### 4. Set IO LED
**Request:**
```json
{
  "type": "set_io_led",
  "token": "<token>",
  "color": "OFF"|"GREEN"|"RED"|"ORANGE"|"AUTO"
}
```
**Response:**
- Returns updated status

**LED Control Modes:**
- **Manual Colors (OFF/GREEN/RED/ORANGE):** Sets the LED to a specific color and activates manual control mode, overriding automatic LED logic.
- **AUTO:** Deactivates manual control and returns the LED to automatic mode, where it reflects OCU connectivity and safety state (see LED Status Indication section).

### 5. Set Internal LED
**Request:**
```json
{
  "type": "set_internal_led",
  "token": "<token>",
  "state": true
}
```
**Response:**
- Returns updated status

### Error Response
```json
{
  "type": "error",
  "message": "<error description>"
}
```

## Error Reference

The GRK board device communicates errors through a combination of HTTP status codes and specific messages within the JSON response payload.

### HTTP Status Code Errors

*   **HTTP 401 Unauthorized**: Returned when an API request (other than `login`) fails authentication. The `message` field will provide more details.
    *   **JSON Payloads**:
        *   `{"type": "error", "message": "Authentication required. Please login first."}`: No user is currently logged in.
        *   `{"type": "error", "message": "Token required."}`: The `token` field is missing from the request.
        *   `{"type": "error", "message": "Invalid or expired token."}`: The provided token is incorrect or no longer valid.

*   **HTTP 403 Forbidden**: This status code is returned when a client attempts to connect from an IP address that is not present in the device's whitelist.
    *   **JSON Payload**:
        ```json
        {
          "type": "error",
          "message": "IP not allowed"
        }
        ```

### JSON Payload Errors (with HTTP 200 OK)

For some application-level errors, the device will return an HTTP 200 OK status, but the JSON response body will contain an error object with a `type` of "error" and a descriptive `message`.

*   **Invalid Relay Number**: Returned when a `set_relay` request specifies a `relay_id` that is outside the valid range (0-3).
    *   **JSON Payload**:
        ```json
        {
          "type": "error",
          "message": "Invalid relay number"
        }
        ```

*   **Invalid LED Color**: Occurs when a `set_io_led` request provides a `color` value that is not one of the accepted options ("OFF", "GREEN", "RED", "ORANGE", "AUTO").
    *   **JSON Payload**:
        ```json
        {
          "type": "error",
          "message": "Invalid LED color"
        }
        ```

*   **Invalid IP Address or Whitelist Entry**: Returned when a `set_ip_config` request contains an incorrectly formatted IP address for either the controller or any entry in the whitelist.
    *   **JSON Payload**:
        ```json
        {
          "type": "error",
          "message": "Invalid IP address or whitelist entry provided."
        }
        ```

*   **Unknown Request Type**: Occurs when the `type` field in a JSON request does not correspond to any recognized API command.
    *   **JSON Payload**:
        ```json
        {
          "type": "error",
          "message": "Unknown request type"
        }
        ```

### OTA Update Errors

Errors during Over-the-Air (OTA) firmware updates (only available in technician mode) are printed to the device's serial console and are not transmitted over the network as HTTP responses. These messages typically follow the format:

*   `OTA Error[<error_code>]: <error_message>`

## DeviceStatus Structure

The status response fields correspond to the following structure in firmware. Units are as follows:
- Acceleration: `g` (gravitational acceleration)
- Angular Velocity: `°/s` (degrees per second)
- Pitch, Roll, Yaw: `°` (degrees)
- GPS Coordinates: `°` (degrees)
- Altitude: `meters`
- Linear/Horizontal Velocity: `km/hr` (kilometers per hour)
- Heading: `°` (degrees)

```cpp
struct DeviceStatus {
  bool relays_status[4];
  bool optos_status[4];
  float imuX, imuY, imuZ;             // Linear Accelerations
  float imuGx, imuGy, imuGz;          // Angular Velocities
  float pitch, roll, yaw;             // Calculated Orientations
  bool imuValid;                      // True if IMU module is communicating
  double gpsLat, gpsLng, gpsAlt;      // GPS Coordinates & Altitude
  char gpsTime[20];                   // "YYYY-MM-DD hh:mm:ss"
  float gpsSpeedNorth;                // Linear Velocity North
  float gpsSpeedEast;                 // Linear Velocity East
  float gpsSpeedDown;                 // Linear Velocity Down (IMU derived)
  float gpsGroundSpeed;               // Horizontal Velocity (Ground Speed)
  float gpsHeading;                   // Absolute Heading
  bool gpsValid;                      // True if GPS fix is valid
  bool gpsConnected;                  // True if GPS module is communicating
  bool ledInternal;
  LED_STATES ledIo;                   // "OFF", "GREEN", "RED", "ORANGE"
  bool button_tech;                       // True if the button on the HLC is pressed
};
```

## LED Status Indication

The IO LED provides critical system status feedback based on OCU (Operator Control Unit) heartbeat connectivity and safety mode:

| LED State          | Condition                                          | Description                                                                                     |
| :----------------- | :-------------------------------------------------- | :------------------------------------------------------------------------------------------------ |
| **Solid Orange**    | Technician mode, or within the boot-up grace period | Device is in technician mode, or still within `LED_BOOT_GRACE_MS` (60s) of startup while sensors/network/OCU comm settle. |
| **Solid Green**     | OCU connected & safety mode active                  | Heartbeat reply received from the OCU within the timeout window. Both optocoupler safety inputs (`optos_status[0]` and `[1]`) are TRUE. |
| **Blinking Green**  | OCU connected & safety mode not active              | OCU heartbeat is healthy, but at least one optocoupler safety input is FALSE.                    |
| **Blinking Red**    | OCU disconnected & safety mode active               | No heartbeat reply from the OCU within the timeout window, but the optocoupler safety inputs are both TRUE. |
| **Solid Red**       | OCU disconnected & safety mode not active           | No heartbeat reply from the OCU, and at least one optocoupler safety input is FALSE.             |

See [`include/ocu_monitor.hpp`](include/ocu_monitor.hpp) for the OCU heartbeat protocol - note the OCU's own script needs a matching UDP responder for `ocuConnected` to ever read true.

## OTA Firmware Updates
Firmware can be updated Over-The-Air (OTA) through the desktop GUI (see [`tools/README.md`](../tools/README.md)) in Technician Mode. Select a `.bin` file and initiate upload.

## Notes
- **Default Login:** User `admin`, password `1234`.
- **Default Device IP:** `192.168.1.198` (configurable via GUI and API).
- **Default Whitelist IPs:** `192.168.1.20`, `192.168.1.169` (configurable via GUI and API).
- **OTA Updates:** Only available in technician mode (hold button_tech for 5 seconds during startup), and can be initiated via the GUI or a separate uploader tool.
- **IMU Calibration:** The IMU performs a self-calibration on startup; keep the device still and flat during this process.
