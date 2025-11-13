# GG-Prodino PlatformIO Project

## Overview

This project provides firmware and API for the ProDino MKR Zero device, Key functionalities include:
- **Advanced Sensor Data:** Retrieval of a wide range of IMU (accelerometer and gyroscope) and GPS data points.
- **Real-time Calculated Parameters:** Derivation of Pitch, Roll, and Yaw from sensor inputs.
- **LED Status Indication:** Detailed, color-coded LED feedback reflecting system safety, sensor connectivity, and operational modes.
- **Over-the-Air (OTA) Firmware Updates:** Available in a dedicated technician mode for convenient maintenance.
- **REST API:** For controlling relays, managing LEDs, retrieving detailed device status, and configuring network settings.
- **Enhanced GUI:** User-friendly interface with features like manual LED override, dynamic IP configuration, and robust error handling for communication loss.

## Device Features
- 4 controllable relays
- IO LED (multi-color: OFF, GREEN, RED, ORANGE)
- Internal LED
- Button1 for technician mode toggling
- **IMU (LSM6DS3) Data:** Linear acceleration (X, Y, Z), Angular velocity (X, Y, Z).
- **GPS (u-blox) Data:** Longitude, Latitude, Altitude, Time, Linear velocity (North, East, Down), Horizontal velocity (Ground speed), Absolute heading.
- **Calculated Outputs:** Pitch, Roll, Yaw.

## Installation Instructions

### Prerequisites
- [PlatformIO](https://platformio.org/) installed (VSCode recommended)
- Python 3.x (for OTA uploader)

### Setup
1. Clone this repository:
   ```sh
   git clone <repo-url>
   cd GG-Prodino
   ```
2. Install PlatformIO dependencies:
   - Open the project in VSCode with PlatformIO extension.
   - PlatformIO will auto-install libraries from `platformio.ini`.
3. Connect your ProDino MKR Zero device to the network.
4. Build and upload firmware:
   - For direct upload: Use USB and PlatformIO's upload button.
   - For OTA: Hold Button1 during power-up to enter technician mode, then use the custom OTA uploader:
     ```sh
     pio run -t upload
     ```

## Technician Mode & OTA
- Hold Button1 for 5 seconds during startup to enter technician mode.
- In technician mode, OTA updates are enabled and the IO LED is set to ORANGE.


## IP Whitelist

**Only clients from the following IP addresses can communicate with the device:**

- `192.168.1.10`
- `192.168.1.15`

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
  "type": "status",                   // Always "status"
  "firmwareVersion": "1.0.0",         // Firmware version
  "relays_status": [false, false, false, false], // Array of 0/1 (off/on) for each relay
  "optoin_status": [false, false, false, false], // Array of 0/1 (off/on) for each optocoupler input
  "imuX": 0.01,                       // float, IMU Linear acceleration X-axis (g)
  "imuY": -0.02,                      // float, IMU Linear acceleration Y-axis (g)
  "imuZ": 0.98,                       // float, IMU Linear acceleration Z-axis (g)
  "imuGx": 1.5,                       // float, IMU Angular velocity X-axis (º/s)
  "imuGy": -0.8,                      // float, IMU Angular velocity Y-axis (º/s)
  "imuGz": 0.2,                       // float, IMU Angular velocity Z-axis (º/s)
  "pitch": 5.2,                       // float, Calculated Pitch (º)
  "roll": -3.1,                       // float, Calculated Roll (º)
  "yaw": 45.7,                        // float, Calculated Yaw (º)
  "imuValid": true,                   // boolean, true if IMU data is valid
  "gpsLat": 32.0853,                  // float, GPS Latitude (º)
  "gpsLng": 34.7818,                  // float, GPS Longitude (º)
  "gpsAlt": 150.2,                    // float, GPS Altitude (meters)
  "gpsTime": "2025-11-06 10:30:45",   // string, GPS Time (YYYY-MM-DD hh:mm:ss)
  "gpsSpeedNorth": 10.5,              // float, Linear velocity North (km/hr)
  "gpsSpeedEast": 2.7,              // float, Linear velocity East (km/hr)
  "gpsSpeedDown": 0.1,                // float, Linear velocity Down (km/hr) - IMU derived
  "gpsGroundSpeed": 10.8,             // float, Horizontal velocity (Ground speed) (km/hr)
  "gpsHeading": 75.3,                 // float, Absolute heading (º)
  "gpsValid": true,                   // boolean, true if GPS fix is valid
  "GPSConnected": true,               // boolean, true if GPS module is communicating
  "ledInternal": true,                // boolean, internal LED state
  "ledIo": "GREEN",                   // string: "OFF", "GREEN", "RED", or "ORANGE"
  "button1": false                    // boolean, true if button1 is pressed
}
```

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

### 4. Set IO LED
**Request:**
```json
{
  "type": "set_io_led",
  "token": "<token>",
  "color": "OFF"|"GREEN"|"RED"|"ORANGE"
}
```
**Response:**
- Returns updated status

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
  float imuGx, imuGy, imuGz;           // Angular Velocities
  float pitch, roll, yaw;             // Calculated Orientations
  bool imuValid;
  double gpsLat, gpsLng, gpsAlt;      // GPS Coordinates & Altitude
  char gpsTime[20];                   // "YYYY-MM-DD hh:mm:ss"
  float gpsSpeedNorth;                // Linear Velocity North
  float gpsSpeedEast;                 // Linear Velocity East
  float gpsSpeedDown;                 // Linear Velocity Down (IMU derived)
  float gpsGroundSpeed;               // Horizontal Velocity (Ground Speed)
  float gpsHeading;                   // Absolute Heading
  bool gpsValid;
  bool gpsConnected;
  bool ledInternal;
  LED_STATES ledIo;                   // "OFF", "GREEN", "RED", "ORANGE"
  bool button1;
};
```

## LED Status Indication

The IO LED provides critical system status feedback based on the following logic:

| LED State          | Condition                                                              | Description                                                                                             |
| :----------------- | :--------------------------------------------------------------------- | :------------------------------------------------------------------------------------------------------ |
| **Blinking Orange**| Technician Mode active & System Unsafe                                 | Device is in technician mode. Optocouplers `optos_status[0]` OR `optos_status[1]` are FALSE.            |
| **Blinking Green** | Normal Mode & System Unsafe & All Sensors Connected                    | Device in normal operation. Optocouplers `optos_status[0]` OR `optos_status[1]` are FALSE. Both GPS (`gpsValid` and `GPSConnected`) and IMU (`imuValid`) are TRUE. |
| **Blinking Red**   | Normal Mode & System Unsafe & One or More Sensors Disconnected        | Device in normal operation. Optocouplers `optos_status[0]` OR `optos_status[1]` are FALSE. Either GPS (`gpsValid` or `GPSConnected`) or IMU (`imuValid`) is FALSE. |
| **Solid Orange**   | Technician Mode active & System Safe                                   | Device is in technician mode. Both optocouplers `optos_status[0]` AND `optos_status[1]` are TRUE.       |
| **Solid Green**    | Normal Mode & System Safe & All Sensors Connected                      | Device in normal operation. Both optocouplers `optos_status[0]` AND `optos_status[1]` are TRUE. Both GPS (`gpsValid` and `GPSConnected`) and IMU (`imuValid`) are TRUE. |
| **Solid Red**      | Normal Mode & System Safe & One or More Sensors Disconnected          | Device in normal operation. Both optocouplers `optos_status[0]` AND `optos_status[1]` are TRUE. Either GPS (`gpsValid` or `GPSConnected`) or IMU (`imuValid`) is FALSE. |

## GUI Features (Python PyQt5)

A desktop GUI application (`test/gui_main.py`) is provided for easy interaction with the Prodino device.
It includes the following enhancements:

-   **LED Override Checkbox:** Allows manual control of the IO LED color directly from the GUI, bypassing automatic firmware logic for testing purposes.
-   **IP Configuration Fields:** Dedicated fields to modify the controller's IP address and two whitelisted IP addresses.
    -   Changing the controller's IP will trigger a popup notification and redirect to the Login screen, requiring reconnection at the new IP.
-   **Firmware and GUI Version Display:** Displays the firmware version from the controller on the main screen and the GUI application version on the login screen.
-   **Communication Loss Handling:**
        - If communication with the controller is lost while on the main screen, the GUI automatically returns to the Login screen.
        - Attempting to log in without controller communication will display a popup notification.
    
    ### Building the GUI Executable
    
    To create a standalone executable for the GUI application, follow these steps:
    
    1.  **Install PyInstaller:**
        ```sh
        pip install pyinstaller
        ```
    2.  **Navigate to the `test` directory:**
        ```sh
        cd test
        ```
    3.  **Build the executable:**
        ```sh
        pyinstaller --onefile --name "GG-Prodino-GUI" gui_main.py
        ```
        (Note: The `--distpath` argument can be used to specify an output directory, e.g., `--distpath ../dist` to place it in the project's root `dist` folder.)
    4.  **Find the executable:**
        The executable will be located in the `dist/` directory (e.g., `dist/GG-Prodino-GUI` on Linux or `dist/GG-Prodino-GUI.exe` on Windows).
    
    ## Python API Tester
    See `gg_api_tester.py` for example usage of the API from Python.

## GUI Version
The GUI version is displayed on the Login screen.''' + '''
## OTA Firmware Updates
Firmware can be updated Over-The-Air (OTA) through the GUI in Technician Mode. Select a `.bin` file and initiate upload.

## Notes
- **Default Login:** User `admin`, password `1234`.
- **Default Device IP:** `192.168.1.198` (configurable via GUI and API).
- **Default Whitelist IPs:** `192.168.1.20`, `192.168.1.169` (configurable via GUI and API).
- **OTA Updates:** Only available in technician mode (hold Button1 for 5 seconds during startup), and can be initiated via the GUI or a separate uploader tool.
- **IMU Calibration:** The IMU performs a self-calibration on startup; keep the device still and flat during this process.

---
