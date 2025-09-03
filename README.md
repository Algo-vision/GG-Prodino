# GG-Prodino PlatformIO Project

## Overview

This project provides firmware and API for the ProDino MKR Zero device, supporting:
- Over-the-Air (OTA) firmware updates (in technician mode)
- REST API for relay and LED control
- IMU (accelerometer) and GPS data acquisition
- Status reporting
- LED status indication (blinking/solid, color-coded)

## Device Features
- 4 controllable relays
- IO LED (multi-color: OFF, GREEN, RED, ORANGE)
- Internal LED
- Button1 for technician mode
- IMU (LSM6DS3) and GPS (u-blox) via I2C

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
**Response:**
```json
{
  "type": "status",
  "relays_status": [0,0,0,0],
  "imuX": 0.0,
  "imuY": 0.0,
  "imuZ": 0.0,
  "imuValid": true,
  "gpsLat": 0.0,
  "gpsLng": 0.0,
  "gpsAlt": 0.0,
  "gpsValid": true,
  "GPSConnected": true,
  "ledInternal": true,
  "ledIo": "OFF",
  "button1": false
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

The status response fields correspond to the following structure in firmware:
```cpp
struct DeviceStatus {
  bool relays_status[4];
  float imuX, imuY, imuZ;
  bool imuValid;
  double gpsLat, gpsLng, gpsAlt;
  bool gpsValid;
  bool gpsConnected;
  bool ledInternal;
  LED_STATES ledIo; // "OFF", "GREEN", "RED", "ORANGE"
  bool button1;
};
```

## LED Status Indication
- **Solid GREEN:** All devices OK, user connected
- **Blinking GREEN:** All devices OK, no user connected
- **Solid RED:** Sensor error, user connected
- **Blinking RED:** Sensor error, no user connected
- **ORANGE:** Technician mode (OTA enabled)

## Python API Tester
See `gg_api_tester.py` for example usage of the API from Python.

## Notes
- Default login: user `admin`, password `1234`
- Device IP: `192.168.1.198` (change in firmware if needed)
- OTA only available in technician mode

---
For more details, see code comments and `HTTP_API_README.md`.
