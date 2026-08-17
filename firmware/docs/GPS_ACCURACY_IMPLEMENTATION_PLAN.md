# Add GPS Position Accuracy and Height Above Ellipsoid

Keep TinyGPSPlus for all existing GPS data. Add SparkFun u-blox GNSS library only for `hAcc`, `vAcc`, and `altEllipsoid` from UBX NAV-PVT.

> [!NOTE]
> **Height above Ellipsoid vs Altitude (hMSL):** Current `gpsAlt` = Mean Sea Level altitude. New `gpsAltEllipsoid` = raw GPS measurement vs WGS84 ellipsoid. Difference: 20-100m depending on location. Ellipsoid height is better for precision positioning.

> [!IMPORTANT]
> Both libraries share the same u-blox module on I2C `0x42`. TinyGPSPlus reads NMEA, SparkFun reads UBX binary — they coexist without conflict.

## New JSON Fields

| Field | Type | Unit | Description |
|:------|:----:|:----:|:------------|
| `gpsHAcc` | float | mm | Horizontal position accuracy estimate |
| `gpsVAcc` | float | mm | Vertical position accuracy estimate |
| `gpsAltEllipsoid` | double | mm | Height above WGS84 ellipsoid |

---

## Proposed Changes

### 1. Firmware — GPS Low-Level Layer

#### [MODIFY] [i2c_imu_gps.hpp](file:///home/haim/repos/GG/GG-GRK/lib/GG/src/i2c_imu_gps.hpp)
- Add `#include <SparkFun_u-blox_GNSS_Arduino_Library.h>`
- Add `float hAcc`, `float vAcc`, `double altEllipsoid` to `gps_data` struct
- Declare `extern SFE_UBLOX_GNSS myGNSS;` and `void initUbloxGNSS();`

#### [MODIFY] [i2c_imu_gps.cpp](file:///home/haim/repos/GG/GG-GRK/lib/GG/src/i2c_imu_gps.cpp)
- Create `SFE_UBLOX_GNSS myGNSS` instance + `initUbloxGNSS()` function
- In `readGPSCoords()`: call `myGNSS.getHorizontalAccEst()`, `getVerticalAccEst()`, `getAltitude()`

---

### 2. Firmware — Status & API Layer

#### [MODIFY] [status_manager.hpp](file:///home/haim/repos/GG/GG-GRK/include/status_manager.hpp)
- Add `float gpsHAcc`, `float gpsVAcc`, `double gpsAltEllipsoid` to `DeviceStatus` struct

#### [MODIFY] [status_manager.cpp](file:///home/haim/repos/GG/GG-GRK/src/status_manager.cpp)
- Copy new fields from `currentGpsData` to `g_status`
- Add `resp["gpsHAcc"]`, `resp["gpsVAcc"]`, `resp["gpsAltEllipsoid"]` to JSON output

---

### 3. Firmware — MQTT & Main

#### [MODIFY] [mqtt_handler.hpp](file:///home/haim/repos/GG/GG-GRK/include/mqtt_handler.hpp)
- Add `float hAcc, float vAcc, double altEllipsoid` params to `publishGPS()`
- Publish new MQTT topic `grk/{sn}/gps/accuracy` → `{"hAcc": ..., "vAcc": ..., "altEllipsoid": ...}`

#### [MODIFY] [main.cpp](file:///home/haim/repos/GG/GG-GRK/src/main.cpp)
- In `setup()`: call `initUbloxGNSS()` after `_gg_hal.init()`
- Pass new fields to `publishGPS()`

---

### 4. Python GUI (PyQt5)

#### [MODIFY] [main_widget.py](file:///home/haim/repos/GG/GG-GRK/test/main_widget.py)
- Add `"gpsHAcc"`, `"gpsVAcc"`, `"gpsAltEllipsoid"` to the `fields` list (line 132)
- These auto-display via the existing generic `status_labels` loop — no extra UI code needed

---

### 5. Python API Tester

#### [MODIFY] [gg_api_tester.py](file:///home/haim/repos/GG/GG-GRK/test/gg_api_tester.py)
- In `format_status()` (around line 288), add lines for the new fields:
  ```
  GPS Accuracy:  H={gpsHAcc}mm V={gpsVAcc}mm
  GPS Alt (Ellipsoid): {gpsAltEllipsoid}mm
  ```

---

### 6. Web App — Mission Control (Node.js)

#### [MODIFY] [server.js](file:///home/haim/repos/GG/GG-GRK/web_ui/server.js)
- In `createDefaultDeviceState()` (line 105): add `hAcc: 0, vAcc: 0, altEllipsoid: 0` to `gps` object
- Add `case 'gps/accuracy':` handler in MQTT message switch (line 256) to update `device.gps.hAcc`, `vAcc`, `altEllipsoid`
- In `status` case (line 306): extract `gpsHAcc`, `gpsVAcc`, `gpsAltEllipsoid` from full status message

#### [MODIFY] [index.html](file:///home/haim/repos/GG/GG-GRK/web_ui/public/index.html)
- Add 3 new `data-row` entries in the GPS card (after Satellites, around line 122):
  ```html
  <div class="data-row">
      <span class="data-label">H. Accuracy</span>
      <span id="gps-hacc" class="data-value">-- mm</span>
  </div>
  <div class="data-row">
      <span class="data-label">V. Accuracy</span>
      <span id="gps-vacc" class="data-value">-- mm</span>
  </div>
  <div class="data-row">
      <span class="data-label">Alt (Ellipsoid)</span>
      <span id="gps-alt-ellipsoid" class="data-value">0.0 m</span>
  </div>
  ```

#### [MODIFY] [app.js](file:///home/haim/repos/GG/GG-GRK/web_ui/public/app.js)
- Add `const` references for the 3 new DOM elements
- In the update function: set their `textContent` from `state.gps.hAcc`, etc.

---

### 7. MQTT Debug Server

#### [MODIFY] [mqtt_subscriber.py](file:///home/haim/repos/GG/GG-GRK/mqtt_server/mqtt_subscriber.py)
- Add `"accuracy"` key to `latest_data["gps"]` (line 29)
- Add `elif topic == "grk/gps/accuracy":` handler
- Include accuracy data in `/api/gps` endpoint response

---

### 8. Build Configuration

#### [MODIFY] [platformio.ini](file:///home/haim/repos/GG/GG-GRK/platformio.ini)
- Add `sparkfun/SparkFun u-blox GNSS v3@^3.1.1` to `lib_deps`

---

### 9. Documentation

#### [MODIFY] [README.md](file:///home/haim/repos/GG/GG-GRK/README.md)
- Add new fields to the API status response example (line 210)
- Add descriptions for `gpsHAcc`, `gpsVAcc`, `gpsAltEllipsoid`
- Update `DeviceStatus` struct documentation
- Add `grk/gps/accuracy` MQTT topic

---

## Summary of All Files

| # | File | Layer | Change |
|:-:|:-----|:------|:-------|
| 1 | `i2c_imu_gps.hpp` | Firmware | Struct + includes |
| 2 | `i2c_imu_gps.cpp` | Firmware | UBX read logic |
| 3 | `status_manager.hpp` | Firmware | DeviceStatus struct |
| 4 | `status_manager.cpp` | Firmware | Status update + JSON |
| 5 | `mqtt_handler.hpp` | Firmware | MQTT publish |
| 6 | `main.cpp` | Firmware | Init + wire-up |
| 7 | `platformio.ini` | Build | New lib dependency |
| 8 | `main_widget.py` | Python GUI | Fields list |
| 9 | `gg_api_tester.py` | Python Tester | Display format |
| 10 | `server.js` | Web Backend | MQTT → state |
| 11 | `index.html` | Web Frontend | HTML elements |
| 12 | `app.js` | Web Frontend | JS data binding |
| 13 | `mqtt_subscriber.py` | MQTT Debug | Topic handler |
| 14 | `README.md` | Documentation | API docs update |

## Verification Plan

### Automated Tests
- `pio run` — verify firmware compiles with both GPS libraries

### Manual Verification
1. Flash firmware → call `get_status` API → verify new JSON fields
2. Check MQTT topic `grk/{sn}/gps/accuracy`
3. Open Python GUI → confirm new fields appear in status table
4. Open Web UI → confirm new rows in GPS card show live data
5. Run `gg_api_tester.py` → confirm accuracy data in output
6. Verify all existing GPS fields still work correctly
