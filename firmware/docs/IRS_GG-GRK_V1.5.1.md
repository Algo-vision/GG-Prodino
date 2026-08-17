# Interface Requirements Specification (IRS)

## GG-GRK High-Level Controller — Firmware V1.5.1

| Field | Value |
| :-- | :-- |
| Document type | Interface Requirements Specification |
| System | GG-GRK Generic Robotic Kit — High-Level Controller (HLC) |
| Configuration item | Firmware V1.5.1 |
| Hardware platform | KMP ProDino MKR Zero (Ethernet), Microchip SAMD21 |
| Document status | Issued for client review |
| Prepared from | Firmware source, as-built |

---

## 1. Scope

### 1.1 Identification

This document specifies the external interfaces of the GG-GRK High-Level Controller (HLC) running firmware version **1.5.1**. It defines the protocols, data elements, timing and electrical characteristics that any external system must comply with in order to interoperate with the HLC.

### 1.2 System overview

The HLC is an Ethernet-connected embedded controller providing:

- Remote control of 4 relay outputs and 2 status LEDs
- Acquisition and reporting of dual-IMU inertial data and GNSS position/velocity
- Derived vehicle orientation (pitch, roll, yaw)
- Monitoring of 4 opto-isolated safety inputs
- System power monitoring (voltage and current)
- Operator Control Unit (OCU) reachability monitoring with LED annunciation
- Over-the-air firmware update in a protected technician mode

### 1.3 Document overview

Section 3 defines each interface and its requirements. Requirements are uniquely identified as **[IRS-xxx-nnn]** and are individually verifiable. Section 4 specifies performance and timing. Section 5 states constraints and known limitations that affect interface users. Section 6 provides the verification matrix.

### 1.4 Requirement conventions

- **shall** — a binding requirement
- **should** — a recommendation
- **may** — an option

---

## 2. Referenced Documents

| Ref | Document |
| :-- | :-- |
| [R1] | `README.md` — GG-GRK Firmware V1.5.1 user and API documentation |
| [R2] | `imuLimitations.md` — IMU angle accuracy analysis and limitations |
| [R3] | `docs/LSM6DS3TR.md` — ST LSM6DS3TR IMU datasheet |
| [R4] | RFC 2616 — Hypertext Transfer Protocol HTTP/1.1 |
| [R5] | RFC 8259 — The JavaScript Object Notation (JSON) Data Interchange Format |
| [R6] | RFC 826 — An Ethernet Address Resolution Protocol (ARP) |

---

## 3. Interface Requirements

### 3.1 Interface identification

```
                          +---------------------------+
   Operator Control Unit  |                           |
   (OCU, x.x.x.169) <---->|  IF-2  HTTP/JSON API      |
                          |        TCP port 80        |
   Monitoring clients <-->|                           |
                          |  IF-3  OCU reachability   |
                          |        UDP port 5001      |<---> OCU host stack
                          |                           |      (ARP responder)
   Maintenance PC   <---->|  IF-6  OTA update         |
                          |        TCP port 65280     |
                          |                           |
   Technician       <---->|  IF-5  Serial console     |
                          |        USB, 115200 Bd     |
                          |                           |
                          |  IF-4  Discrete I/O       |<---> Vehicle harness
                          |        Relays / Opto / LED|      (J16, EPC)
                          +---------------------------+
                                       |
                                    IF-1 Ethernet 10/100
```

| ID | Interface | Type | Direction |
| :-- | :-- | :-- | :-- |
| IF-1 | Ethernet / IP network | Physical + network | Bidirectional |
| IF-2 | HTTP/JSON control and telemetry | Application | Client → HLC (request/response) |
| IF-3 | OCU reachability | Network (ARP-based) | HLC → OCU |
| IF-4 | Discrete I/O | Electrical | Bidirectional |
| IF-5 | Serial diagnostic console | Application | HLC → Technician |
| IF-6 | OTA firmware update | Application | Maintenance PC → HLC |

---

### 3.2 IF-1 — Ethernet / IP Network Interface

| ID | Requirement |
| :-- | :-- |
| **[IRS-NET-010]** | The HLC shall connect via 10/100BASE-T Ethernet (Wiznet W5500 controller). |
| **[IRS-NET-020]** | The HLC shall use a **static IPv4 address**. DHCP is not supported. |
| **[IRS-NET-030]** | The default IP address shall be `192.168.1.198`, configurable via IF-2 (`set_ip_config`). |
| **[IRS-NET-040]** | The subnet mask shall be `255.255.0.0` (/16). |
| **[IRS-NET-050]** | The default gateway shall be derived as `192.168.<third octet of controller IP>.1`. |
| **[IRS-NET-060]** | The DNS server shall be `8.8.8.8`. (Not used in normal operation.) |
| **[IRS-NET-070]** | The HLC shall use MAC address `00:08:DC:53:09:72`. |
| **[IRS-NET-080]** | The HLC shall accept requests **only** from IPv4 addresses present in its whitelist. All other requests shall be rejected per [IRS-API-160]. |
| **[IRS-NET-090]** | The whitelist shall hold up to **3** entries; the factory default is `192.168.1.20`, `192.168.1.169`, `192.168.1.33`. |
| **[IRS-NET-100]** | The HLC shall provide a maximum of **8** concurrent TCP sockets (W5500 hardware limit), of which one is reserved for IF-3. |

> ⚠ **Integration constraint — fixed MAC.** [IRS-NET-070] specifies a **hard-coded** MAC address. Two or more HLC units on the same Layer-2 segment will therefore present duplicate MAC addresses, causing unpredictable switch forwarding and loss of communication. Multi-unit installations require either separate Layer-2 segments or a firmware change to derive the MAC from the serial number. See section 5.2.

---

### 3.3 IF-2 — HTTP/JSON Control and Telemetry Interface

#### 3.3.1 Transport and framing

| ID | Requirement |
| :-- | :-- |
| **[IRS-API-010]** | The HLC shall expose an HTTP/1.1 server on **TCP port 80**. |
| **[IRS-API-020]** | All API requests shall use the **POST** method to resource path `/`. |
| **[IRS-API-030]** | Request bodies shall be UTF-8 encoded JSON objects per [R5]. |
| **[IRS-API-040]** | Requests shall include a valid `Content-Length` header. |
| **[IRS-API-050]** | Responses shall carry `Content-Type: application/json` unless otherwise stated. |
| **[IRS-API-060]** | The HLC shall close the TCP connection after each response (`Connection: close`). Clients shall open a **new connection per request**; connection reuse is not supported. |
| **[IRS-API-070]** | Every request shall contain a `type` field identifying the command. |
| **[IRS-API-080]** | Every request except `login` shall contain a valid `token` field. |
| **[IRS-API-090]** | The HLC shall service up to **4** client connections per main-loop iteration. |
| **[IRS-API-100]** | If no request body is received within **100 ms** of the request line, the HLC shall respond HTTP 400 with message `"Request timeout"`. |

#### 3.3.2 Authentication and session management

| ID | Requirement |
| :-- | :-- |
| **[IRS-API-110]** | The client shall authenticate via the `login` command, supplying `user` and `pass`. |
| **[IRS-API-120]** | On success the HLC shall return a session **token** of exactly **16 characters** drawn from the set `A`–`Z`. |
| **[IRS-API-130]** | The HLC shall maintain up to **5** concurrent sessions. |
| **[IRS-API-140]** | A token shall expire after **300 000 ms (5 minutes)** without use. Each successful authenticated request refreshes its token. |
| **[IRS-API-150]** | When all 5 session slots are occupied, a new login shall displace the least-recently-used session. |
| **[IRS-API-160]** | Requests from a non-whitelisted source IP shall receive **HTTP 403** with body `{"type":"error","code":"E-103","message":"IP not allowed"}`. |
| **[IRS-API-170]** | Requests with a missing or invalid token shall receive **HTTP 401**. |

> ⚠ **Security constraint.** Credentials and tokens are transmitted in **cleartext** (no TLS), and token generation uses a non-cryptographic, unseeded pseudo-random source. See section 5.1 — this interface is specified for use only on a trusted, physically protected network segment.

#### 3.3.3 Command summary

| # | `type` | Function | Reboots |
| :-- | :-- | :-- | :-- |
| 1 | `login` | Authenticate, obtain session token | No |
| 2 | `get_status` | All telemetry, grouped into 4 categories | No |
| 3 | `get_overview` | Power, relays, opto inputs, safety, OCU | No |
| 4 | `get_imu` | Dual-IMU data, orientation, IMU temperature | No |
| 5 | `get_gps` | GNSS fix, velocity, heading | No |
| 6 | `get_config` | Firmware, network, technician state, axis map | No |
| 7 | `set_relay` | Command one relay output | No |
| 8 | `set_io_led` | Set IO LED colour or return to AUTO | No |
| 9 | `set_internal_led` | Set MCU internal LED | No |
| 10 | `reset_led_control` | Return IO LED to automatic control | No |
| 11 | `set_ip_config` | Set controller IP and whitelist | **Yes** |
| 12 | `set_serial_number` | Burn serial number (technician mode) | **Yes** |
| 13 | `get_serial_number` | Read serial number | No |
| 14 | `set_imu_axis_map` | Configure IMU axis assignment and inversion | Recommended |

| ID | Requirement |
| :-- | :-- |
| **[IRS-API-180]** | The HLC shall implement exactly the 12 commands listed above. An unrecognised `type` shall return `{"type":"error","message":"Unknown request type"}` with HTTP 200. |
| **[IRS-API-190]** | `get_status` shall return the union of the `get_overview`, `get_imu`, `get_gps` and `get_config` payloads, nested under keys `overview`, `imu`, `gps`, `config` respectively. |
| **[IRS-API-200]** | Each nested category object in `get_status` shall contain the same data elements as its corresponding standalone command. |
| **[IRS-API-210]** | Command 11 shall transmit its response **before** initiating the reboot. |

Full request/response examples for every command are given in [R1] API Reference and are not duplicated here.

#### 3.3.4 Data element dictionary

All numeric values are JSON numbers; booleans are JSON `true`/`false`; text is a JSON string.

**Top level (`get_status` only)**

| Element | Type | Unit / Range | Description |
| :-- | :-- | :-- | :-- |
| `type` | string | `"status"` | Message type |
| `firmwareVersion` | string | e.g. `"1.5.1"` | Firmware version |
| `ledInternal` | bool | — | MCU internal LED state |
| `ledIo` | string | `OFF`\|`GREEN`\|`RED`\|`ORANGE` | IO LED state |
| `button_tech` | bool | — | Technician button pressed |
| `technicianMode` | bool | — | Technician mode active |
| `controllerIp` | string | dotted quad | Configured controller IP |
| `whitelistIps` | string[ ] | 0–3 entries | Permitted client IPs |
| `motorWorkHours` | number | hours, 2 dp | Cumulative motor operating time |
| `motorWorkSeconds` | number | seconds | Cumulative motor operating time |

**Category `config`**

| Element | Type | Unit / Range | Description |
| :-- | :-- | :-- | :-- |
| `firmwareVersion` | string | — | Firmware version |
| `controllerIp` | string | dotted quad | Configured controller IP |
| `whitelistIps` | string[ ] | 0–3 | Permitted client IPs |
| `technicianMode` | bool | — | Technician mode active |
| `burnedHours` | number | hours, 2 dp | Operating time since serial number burn |
| `sessionHours` | number | hours, 2 dp | Operating time since last boot |
| `techLedColor` | string | `OFF`\|`GREEN`\|`RED`\|`ORANGE` | Current IO LED colour |
| `imuPitchAxis` | string | `X`\|`Y`\|`Z` | Sensor axis carrying pitch |
| `imuRollAxis` | string | `X`\|`Y`\|`Z` | Sensor axis carrying roll |
| `imuYawAxis` | string | `X`\|`Y`\|`Z` | Sensor axis carrying yaw |
| `imuPitchInvert` | bool | — | Negate pitch axis |
| `imuRollInvert` | bool | — | Negate roll axis |
| `imuYawInvert` | bool | — | Negate yaw axis |

**Category `overview`**

| Element | Type | Unit / Range | Description |
| :-- | :-- | :-- | :-- |
| `powerConnected` | bool | — | INA219 power monitor present |
| `powerSane` | bool | — | Power readings pass sanity check |
| `systemVoltage` | number | V, `-1` = monitor absent | System bus voltage |
| `systemCurrent_A` | number | A, `-1` = monitor absent | System bus current |
| `relays_status` | bool[4] | — | Relay output states, index 0–3 |
| `optoin_status` | bool[4] | — | Opto-isolated input states, index 0–3 |
| `safetyMode` | bool | — | `optoin_status[0] AND [1]` |
| `safetyModeDurationMs` | number | ms, 0 while safe | Continuous unsafe duration |
| `ocuConnected` | bool | — | OCU reachable (see IF-3) |
| `ocuDisconnectedDurationMs` | number | ms, 0 while connected | Continuous unreachable duration |

**Category `imu`**

| Element | Type | Unit / Range | Description |
| :-- | :-- | :-- | :-- |
| `pitch`, `roll`, `yaw` | number | degrees, (−180, 180] | Calculated orientation |
| `angleSane` | bool | — | Orientation passes sanity check |
| `imuValid`, `imu2Valid` | bool | — | IMU communicating (I2C read OK) |
| `imu1Sane`, `imu2Sane` | bool | — | IMU healthy **and** accelerometer usable as horizon |
| `imuX/Y/Z`, `imu2X/Y/Z` | number | g | Linear acceleration per axis |
| `imuGx/Gy/Gz`, `imu2Gx/Gy/Gz` | number | °/s | Angular rate per axis, calibration offset removed |
| `imuTemp` | number | °C | IMU die temperature, averaged over readable units |

**Category `gps`**

| Element | Type | Unit / Range | Description |
| :-- | :-- | :-- | :-- |
| `gpsConnected` | bool | — | GNSS receiver communicating |
| `gpsSane` | bool | — | Position passes sanity check |
| `gpsSatellites` | number | count | Satellites used in fix |
| `gpsLat`, `gpsLng` | number | degrees, **0 when fix lost** | Current position |
| `gpsAlt` | number | m, **0 when fix lost** | Current altitude |
| `lastGpsLat/Lng/Alt` | number | degrees / m | Last known-good position, retained on fix loss |
| `gpsHeading` | number | degrees, 0–360 | Course over ground |
| `gpsGroundSpeed` | number | km/hr | Horizontal speed |
| `gpsSpeedNorth/East/Down` | number | km/hr | Velocity components |
| `gpsTime` | string | `YYYY-MM-DD hh:mm:ss`, empty on no fix | GNSS UTC time |

| ID | Requirement |
| :-- | :-- |
| **[IRS-API-220]** | `gpsLat`, `gpsLng` and `gpsAlt` shall be set to 0 when the GNSS fix is invalid. Clients requiring last-known position shall use `lastGpsLat/Lng/Alt`. |
| **[IRS-API-230]** | `systemVoltage` and `systemCurrent_A` shall both report `-1` when the power monitor is absent. |
| **[IRS-API-240]** | Reported telemetry shall be no more than **10 ms** older than the most recent sensor sample (see [IRS-PERF-030]). |

#### 3.3.5 Error responses

| ID | Requirement |
| :-- | :-- |
| **[IRS-API-250]** | Error responses shall take the form `{"type":"error","code":"<id>","message":"<description>"}`. The `code` shall be stable across releases; `message` text is not guaranteed stable. |
| **[IRS-API-260]** | Transport- and authentication-level errors shall use HTTP status codes 400, 401 and 403. |
| **[IRS-API-270]** | Application-level errors shall be returned with **HTTP 200** and an error body. |

| Code | HTTP | Message | Cause |
| :-- | :-- | :-- | :-- |
| `E-100` | 400 | `Request timeout` | Body not received within 100 ms |
| `E-101` | 401 | `Token required.` | `token` field absent |
| `E-102` | 401 | `Invalid or expired token.` | Token unknown or timed out |
| `E-103` | 403 | `IP not allowed` | Source IP not whitelisted |
| `E-110` | 200 | `Invalid username` | Login rejected |
| `E-111` | 200 | `Invalid password` | Login rejected |
| `E-200` | 200 | `Unknown request type` | Unrecognised `type` |
| `E-201` | 200 | `Invalid relay number` | `relay_id` outside 0-3 |
| `E-202` | 200 | `Invalid LED color` | Colour not in permitted set |
| `E-203` | 200 | `Invalid IP address or whitelist entry provided.` | Malformed address, or more than 3 entries |
| `E-204` | 200 | `Invalid IMU axis (expected X, Y or Z)` | Bad axis token |
| `E-205` | 200 | `Pitch, roll and yaw must each use a different axis` | Not a permutation |

---

### 3.4 IF-3 — OCU Reachability Interface

| ID | Requirement |
| :-- | :-- |
| **[IRS-OCU-010]** | The HLC shall identify the OCU as the whitelisted IPv4 address whose **final octet is 169**. |
| **[IRS-OCU-020]** | The HLC shall determine OCU reachability at the **link layer**, requiring no application software on the OCU. |
| **[IRS-OCU-030]** | The HLC shall transmit a 1-byte UDP datagram from local port **5001** to OCU port **5001** as a reachability probe. The OCU is **not** required to listen on, or respond to, this port. |
| **[IRS-OCU-040]** | Reachability shall be deemed confirmed when the probe's ARP resolution succeeds, i.e. the OCU answered an ARP request per [R6]. |
| **[IRS-OCU-050]** | Any traffic received from the OCU address shall also confirm reachability and suppress the next probe. |
| **[IRS-OCU-060]** | The HLC shall probe every **1 000 ms** while reachable and every **10 000 ms** while unreachable. |
| **[IRS-OCU-070]** | `ocuConnected` shall become false after **5 000 ms** without confirmation. |
| **[IRS-OCU-080]** | Loss of Ethernet link shall set `ocuConnected` false immediately, without probing. |
| **[IRS-OCU-090]** | The HLC shall never initiate unsolicited traffic to any other address. |

> **Note.** ICMP echo (`ping`) cannot be used as the reachability signal: the W5500 answers echo requests in hardware, so they are not visible to the firmware. The ARP probe tests the same property one layer lower.

---

### 3.5 IF-4 — Discrete I/O Interface

#### 3.5.1 Relay outputs

| ID | Requirement |
| :-- | :-- |
| **[IRS-DIO-010]** | The HLC shall provide **4** relay outputs, indices 0–3, commanded via `set_relay`. |
| **[IRS-DIO-020]** | Relays 0 and 1 shall **automatically return to OFF 5 000 ms** after being commanded ON. |
| **[IRS-DIO-030]** | Relays 2 and 3 shall latch in the commanded state until changed. |

| Index | Function | Behaviour |
| :-- | :-- | :-- |
| 0 | Power cut-off, internal Ethernet switch (hard reset) | Auto-reset, 5 s |
| 1 | Power cut-off, internal computer (hard reset) | Auto-reset, 5 s |
| 2 | Power enable 13.8 V/GND via J16 pin 1 | Latching |
| 3 | Power enable 13.8 V/GND via J16 pin 2 | Latching |

#### 3.5.2 Opto-isolated inputs

| ID | Requirement |
| :-- | :-- |
| **[IRS-DIO-040]** | The HLC shall provide **4** opto-isolated inputs, indices 0–3, reported in `optoin_status`. |
| **[IRS-DIO-050]** | `safetyMode` shall be asserted only when inputs 0 **and** 1 are both TRUE. |
| **[IRS-DIO-060]** | The HLC shall report the continuous unsafe duration in `safetyModeDurationMs`, reset to 0 on return to the safe state and on reboot. |

| Index | Function |
| :-- | :-- |
| 0 | EPC channel 1 safety state |
| 1 | EPC channel 2 safety state |
| 2 | Reserved |
| 3 | Reserved |

#### 3.5.3 IO LED annunciation

| ID | Requirement |
| :-- | :-- |
| **[IRS-DIO-070]** | The IO LED shall indicate OCU connectivity and safety state per the table below when under automatic control. |
| **[IRS-DIO-080]** | The blink period shall be **500 ms** per state change. |
| **[IRS-DIO-090]** | For the first **60 000 ms** after boot the LED shall show solid ORANGE while sensors, network and OCU communication settle. |
| **[IRS-DIO-100]** | Manual colour commands via `set_io_led` shall override automatic control until `AUTO` or `reset_led_control` is issued. |

| LED state | OCU | Safety mode | Meaning |
| :-- | :-- | :-- | :-- |
| Solid Orange | — | — | Technician mode, or within boot grace period |
| Solid Green | Connected | Active | Normal operation, safe |
| Blinking Green | Connected | Not active | OCU healthy, safety input(s) not asserted |
| Blinking Red | Disconnected | Active | OCU unreachable, safety inputs asserted |
| Solid Red | Disconnected | Not active | OCU unreachable and not in safe state |

#### 3.5.4 Technician button

| ID | Requirement |
| :-- | :-- |
| **[IRS-DIO-110]** | Holding the technician button continuously for **5 000 ms** during power-up shall enter technician mode. |
| **[IRS-DIO-120]** | Technician mode shall enable IF-6 (OTA). |
| **[IRS-DIO-130]** | Technician mode shall persist until the next reboot and shall be reported in `technicianMode`. |

---

### 3.6 IF-5 — Serial Diagnostic Interface

| ID | Requirement |
| :-- | :-- |
| **[IRS-SER-010]** | The HLC shall provide a diagnostic console over USB CDC at **115 200 baud, 8-N-1**. |
| **[IRS-SER-020]** | The HLC shall emit the complete `get_status` document once per **1 000 ms**, rendered as `name: value` lines grouped under `[config]`, `[overview]`, `[imu]`, `[gps]` headers. |
| **[IRS-SER-030]** | The console shall additionally report the board's actual link IP as `localIp`, distinct from the configured `controllerIp`. |
| **[IRS-SER-040]** | The interface shall be **output only**; the HLC shall not accept commands over serial. |
| **[IRS-SER-050]** | OTA errors shall be reported on this interface in the form `OTA Error[<code>]: <message>` and shall not be reported over IF-2. |

---

### 3.7 IF-6 — OTA Firmware Update Interface

| ID | Requirement |
| :-- | :-- |
| **[IRS-OTA-010]** | The HLC shall accept over-the-air firmware updates **only** while in technician mode. |
| **[IRS-OTA-020]** | The update server shall listen on **TCP port 65280**. |
| **[IRS-OTA-030]** | Firmware shall be uploaded by HTTP **POST** to `/sketch` with `Content-Type: application/octet-stream` and a correct `Content-Length`. |
| **[IRS-OTA-040]** | The request shall carry HTTP Basic authentication with username `arduino` and an **empty password**. |
| **[IRS-OTA-050]** | The payload shall be a raw `.bin` image built for the SAMD21 target. |
| **[IRS-OTA-060]** | HTTP 200 shall indicate a successful transfer; the HLC then reboots into the new image. |
| **[IRS-OTA-070]** | The OTA hostname shall be `grk`. |

> ⚠ **Security constraint.** IF-6 has an empty password and no image signature verification. Any host able to reach port 65280 while the unit is in technician mode can install arbitrary firmware. See section 5.1.

---

## 4. Performance and Timing Requirements

| ID | Requirement |
| :-- | :-- |
| **[IRS-PERF-010]** | The HLC shall sample all sensors and update derived orientation on a fixed **10 ms** cadence. |
| **[IRS-PERF-020]** | Sensor sampling shall be independent of API request rate; API requests shall not trigger sensor reads. |
| **[IRS-PERF-030]** | Telemetry returned via IF-2 shall be no more than one sample interval (10 ms) old. |
| **[IRS-PERF-040]** | The HLC shall sustain a sustained request rate of at least **20 requests/second** aggregated across clients. |
| **[IRS-PERF-050]** | The HLC shall support at least **5** concurrent authenticated sessions ([IRS-API-130]). |
| **[IRS-PERF-060]** | An active client IP shall remain registered for UDP/telemetry purposes for **30 000 ms** after its last request. |

**Latency characteristics (informative).** The connection-per-request model imposes a floor of approximately **40 ms** per transaction (TCP setup plus teardown). A previously recorded `get_status` median was ~61 ms. These figures are design-time observations carried forward from the source and **have not been re-measured for this release**; see section 6.

---

## 5. Constraints and Known Limitations

These are stated explicitly because they materially affect how the interfaces may be used.

### 5.1 Security

| Item | Statement |
| :-- | :-- |
| Transport | All interfaces are **unencrypted**. Credentials, tokens and telemetry are transmitted in cleartext. |
| Credentials | Default login is user `admin`, password `1234`, compiled into the firmware. |
| Token strength | 16 characters from a 26-symbol alphabet, produced by a **non-cryptographic** PRNG that is **never seeded**. The token sequence following a reset is therefore reproducible. |
| Access control | The IP whitelist is the primary control. It is enforced on source IP only and offers no protection against address spoofing on the local segment. |
| OTA | Empty password, no image authentication ([IRS-OTA-040]). |

**Consequence:** these interfaces are specified for use on a **trusted, physically protected, isolated network segment only**. They are not suitable for exposure to a shared corporate network or the public Internet without an external security boundary.

### 5.2 Multi-unit deployment

The fixed MAC address ([IRS-NET-070]) prevents more than one HLC operating on a single Layer-2 segment. See the warning in section 3.2.

### 5.3 Measurement accuracy

Reported orientation is subject to significant, well-characterised error that varies with operating condition. Clients shall consult **[R2] `imuLimitations.md`** before using `pitch`, `roll` or `yaw` for any control or safety function. In summary:

| Condition | Pitch / Roll | Yaw |
| :-- | :-- | :-- |
| Stationary | ≈0.8–1.1° bounded | **Unbounded drift** (≈60°/min at +20 °C) |
| Moving, GNSS fix | Up to ≈17° during 0.3 g manoeuvres | ≈5° bounded, **180° error when reversing** |
| Moving, no fix | As above | **Unbounded drift** |

Specific interface-relevant consequences:

- `yaw` is **not corrected while stationary or below 3 km/hr** and shall not be treated as a valid heading in those conditions.
- `yaw` follows GNSS **course over ground**, not vehicle heading; it is 180° in error while the vehicle reverses.
- `pitch`/`roll` are referenced to the **attitude at power-on**, not to true level, unless the unit is guaranteed level at boot.
- If both IMUs fail, `pitch`/`roll`/`yaw` **retain their last values indefinitely** and remain present in the response. Clients shall monitor `imuValid`, `imu2Valid`, `imu1Sane` and `imu2Sane` to detect this condition.

### 5.4 GNSS data

`gpsLat`, `gpsLng` and `gpsAlt` are zeroed on fix loss ([IRS-API-220]) — a valid coordinate of exactly zero is indistinguishable from "no fix". Clients shall use `gpsConnected`, `gpsSane` and `gpsSatellites` to qualify the data.

### 5.5 Reserved and fixed-value elements

`jetsonCpuTemp` is always `-1`; no communication channel to a Jetson-class computer exists in this configuration. `optoin_status[2]` and `[3]` are reserved and unassigned.

---

## 6. Qualification Provisions

| Method | Code | Definition |
| :-- | :-- | :-- |
| Inspection | I | Examination of source, configuration or documentation |
| Analysis | A | Derivation from established theory or datasheet values |
| Demonstration | D | Observation of operation without instrumentation |
| Test | T | Instrumented measurement against a quantified criterion |

| Requirement group | Method | Verification approach |
| :-- | :-- | :-- |
| IRS-NET-010 … 100 | I / D | Inspect configuration; demonstrate connectivity and whitelist rejection from a non-listed IP |
| IRS-API-010 … 100 | T | Automated protocol conformance suite against a live unit |
| IRS-API-110 … 170 | T | Session lifecycle test: token issue, expiry at 5 min, 6th concurrent login |
| IRS-API-180 … 270 | T | Schema and error-path validation for all 14 commands |
| IRS-OCU-010 … 090 | T | Disconnect OCU and cable; verify state transitions within stated timeouts |
| IRS-DIO-010 … 130 | D / T | Command each relay and observe; time relay 0/1 auto-reset; exercise LED matrix |
| IRS-SER-010 … 050 | D | Capture console output and compare with the concurrent `get_status` response |
| IRS-OTA-010 … 070 | D | Upload a signed test image in and out of technician mode |
| IRS-PERF-010 … 060 | T | Instrument sample cadence; sustained load test at 20 req/s |
| section 5.3 accuracy | A / T | Analysis per [R2]; confirmation against a ground-truth reference |

**Verification status.** The requirements in this document are derived by inspection of the as-built firmware source and are believed to reflect it accurately. **No formal qualification campaign has been executed against this document**, and the latency figures in section 4 are carried forward from design-time observations rather than measured for this release. The items listed in [R2] section 8 remain open.

---

## 7. Notes

### 7.1 Acronyms

| Term | Definition |
| :-- | :-- |
| ARP | Address Resolution Protocol |
| EPC | Electrical Power Controller |
| GNSS | Global Navigation Satellite System |
| HLC | High-Level Controller |
| IMU | Inertial Measurement Unit |
| IRS | Interface Requirements Specification |
| MSB | Main Safety Board |
| OCU | Operator Control Unit |
| ODR | Output Data Rate |
| OTA | Over-The-Air (firmware update) |
| PRNG | Pseudo-Random Number Generator |

### 7.2 Default configuration summary

| Parameter | Default |
| :-- | :-- |
| IP address | `192.168.1.198` |
| Subnet mask | `255.255.0.0` |
| MAC address | `00:08:DC:53:09:72` |
| API port | TCP 80 |
| OCU probe port | UDP 5001 |
| OTA port | TCP 65280 |
| Serial console | 115 200 baud |
| Login | `admin` / `1234` |
| Whitelist | `192.168.1.20`, `192.168.1.169`, `192.168.1.33` |
| IMU axis map | Pitch=X, Roll=Y, Yaw=Z, no inversion |

### 7.3 Document maintenance

This IRS describes firmware **V1.5.1** as built. Any change to a data element name, unit, port, timeout or default listed herein constitutes an interface change and requires reissue of this document with an incremented revision.
