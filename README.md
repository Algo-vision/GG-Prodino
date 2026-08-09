# GG-GRK — V1.5.2

Control and monitoring system for the GRK machine: a board on the vehicle reads
its sensors, drives its relays, serves that data live to equipment on the same
network, and reports an encrypted status to a cloud dashboard once a minute.

Three pieces:

| | what it is | where it runs |
|---|---|---|
| **Controller** | Reads the sensors, drives the relays, serves the local API, sends telemetry | KMP ProDino MKR Zero (SAMD21, 48 MHz, 32 KB RAM) on the vehicle |
| **Dashboard** | Live fleet view, map, user access, board key management | Node.js on EC2 |
| **Technician GUI** | Bench tool: read live values, drive relays/LEDs, provision a board, upload firmware | Desktop (PyQt5) |

---

## What the controller does

### What it reads

| Source | Values |
|---|---|
| **IMU ×2** (LSM6DS3) | Acceleration X/Y/Z and angular rate X/Y/Z, from both sensors, plus die temperature |
| **Orientation** (derived) | Pitch, roll, yaw — a complementary filter over both IMUs, sampled at 100 Hz |
| **GPS** (u-blox) | Latitude, longitude, altitude, time, velocity N/E/D, ground speed, heading, satellite count, and the last known-good fix |
| **Power monitor** (INA219) | System voltage and current |
| **Opto inputs ×4** | Isolated digital inputs; the first two are the safety pair |
| **Technician button** | Held during startup to enter technician mode |

Sensors are sampled on a **fixed 100 Hz cadence from the main loop** — never per
request — so the orientation filter behaves the same no matter how hard the board
is being polled. The GPS is the exception: it is read at 1 Hz, because its read
blocks for ~120 ms against ~2 ms for everything else.

### What it controls

- **4 relays.** Relays 0 and 1 auto-reset to OFF after a delay (a safety
  feature); 2 and 3 hold their state.
- **Bi-colour status LED**, driven by machine state rather than by hand:

  | LED | Meaning |
  |---|---|
  | Solid orange | Technician mode, or still in the boot grace period |
  | Solid green | OCU connected, safety mode active |
  | Blinking green | OCU connected, safety mode not active |
  | Solid red | OCU disconnected, safety mode not active |
  | Blinking red | OCU disconnected, safety mode active |

- **Internal LED**, under direct API control.

### What it checks

- **Safety mode** — true when both safety opto inputs are active. The board also
  reports how long it has been continuously unsafe.
- **Sanity flags** — every sensor group carries one (`imu1Sane`, `imu2Sane`,
  `angleSane`, `gpsSane`, `powerSane`). A reading that is all-zero or frozen for
  several samples is flagged, and the orientation filter uses the flags to decide
  which IMU(s) to trust rather than blindly averaging.
- **OCU presence** — whether the Operator Control Unit is reachable, determined
  by the controller alone (PHY link, traffic already arriving, then an ARP
  probe). Nothing has to be installed on the OCU.

### Timing, measured on the hardware

| | |
|---|---|
| Local API | ~18 requests/second sustained, 44 ms median, 94 ms p95 |
| Telemetry send | 20 ms of blocked loop time, once a minute (0.03% of the board's time) |
| Relay + LED timing | Driven by a 50 ms hardware timer interrupt, unaffected by anything above |
| RAM | 6.9 KB of 32 KB used; ~24 KB free at runtime |

---

## The two ways data leaves the board

**1. Local HTTP API — for real-time consumers.** Anything on the same network (a
Jetson, the technician GUI) polls the board directly with JSON over HTTP. Guarded
by an IP whitelist plus a login token. This is the path with a latency budget.

**2. Encrypted telemetry — for the cloud dashboard.** Once a minute the board
sends its status to the EC2 server. Best-effort by design: it must never delay
the API above.

The two do not interfere. A telemetry send blocks the loop for 20 ms; the API
answers ~18 requests a second straight through it.

### The local API

`POST /` with `{"type": "<command>", "token": "<token>"}`. Get a token with
`{"type":"login","user":...,"pass":...}`.

| Command | Purpose |
|---|---|
| `get_status` | Everything, in sections: `config`, `overview`, `imu`, `gps` |
| `get_overview` / `get_imu` / `get_gps` | The individual sections, for cheaper polling |
| `get_config` | IPs, whitelist, firmware version, axis map, technician mode, whether a key is set |
| `set_relay`, `set_io_led`, `set_internal_led`, `reset_led_control` | Actuators |
| `set_ip_config` | Controller IP and whitelist (reboots the board) |
| `set_imu_axis_map` | Which sensor axis each angle rotates about, and inversions |
| `set_serial_number` / `get_serial_number` | Provisioning — see below |
| `set_device_key` | Provisioning — see below |

Responses are served from the buffered sensor snapshot, so a request never
triggers a blocking sensor read.

---

## Encrypted telemetry

The board sends over **plain HTTP, but the message itself is encrypted** — the
security is in the message, not the transport. No TLS is involved.

**Why not HTTPS?** Measured on this hardware: a TLS handshake costs this 48 MHz
chip **9,789 ms of blocked CPU per message** and leaves under 1 KB of free RAM.
It froze the local API for ~15 seconds at a time and sometimes crashed the board.
The expensive part is the *handshake* — the public-key maths two strangers use to
agree on a key. We are not strangers; we own both ends. So the key is placed on
the board in advance and the negotiation is skipped. What remains costs ~16 ms.
Full numbers: [TELEMETRY_BENCHMARK.md](docs/TELEMETRY_BENCHMARK.md).

Each board has its **own 32-byte secret key**. Messages use **ChaCha20-Poly1305**
— the algorithm TLS 1.3 and WireGuard use:

| | |
|---|---|
| **Encrypted** | an eavesdropper sees only random bytes |
| **Authenticated** | change one bit and the server rejects it; forging without the key is not feasible |
| **Replay-protected** | a captured packet re-sent later is rejected |
| **Per board** | one compromised board affects only itself; revoking it is one click |

---

## Putting a board into service

Two things must be provisioned, in this order. **Both live in flash, and a
firmware upload erases flash — so after every upload the board must be
re-provisioned.**

### 1. Serial number — technician GUI

Technician Mode → **Set SN** → enter the digits → **Set SN**.

This identifies the board and is what the server uses to select its key. The
board reboots automatically. A serial can only be burned **once**; changing it
means erasing flash first (which a firmware upload does).

### 2. Key — created in the dashboard, burned with the GUI

> **The dashboard is the only place keys should be created.** It generates the
> key *and* registers it with the server in one step. Creating a key any other
> way leaves the server holding a different one, and the board's packets are
> rejected as `BAD AUTH` with no obvious cause.

1. **Dashboard → Admin → Board Telemetry Keys.** Enter the board's serial
   (e.g. `SN2010`) → **Create Key** → **Copy**.
   The key is shown **once** and cannot be retrieved afterwards.
2. **Technician GUI → Telemetry Key.** Paste it → **Burn Key to Board**.
   The panel then reads `Key on this board: SET`. It takes effect immediately —
   no reboot.

The board should appear online in the dashboard within a minute.

**Revoking:** Admin → Board Telemetry Keys → Revoke. That board is rejected
immediately; no others are affected.
**Rotating:** create a key for the same serial (it warns first), then burn the new
one — the board stops reporting until you do.

### Rebooting a board

This matters more than it looks:

| method | effect |
|---|---|
| Power cycle, or `set_ip_config` | Clean restart, flash intact |
| **Firmware upload** | **Erases flash** — serial and key are lost, re-provision after |
| **1200-baud serial touch** | Drops into the bootloader; the board stops serving HTTP while the Ethernet chip keeps answering pings, so it looks alive |

---

## Checking it works

```bash
# the server's crypto, including the attacks it must reject - no board needed
python3 tools/test_ingest.py http://16.171.11.151:5555
#   valid -> 204,  tampered -> 401,  replayed -> 409,  unknown serial -> 401

# what the board's API does under a realistic load
python3 tools/test_multi_consumer.py 192.168.1.198 300 1 20 get_status

# a long soak: HTTP consumers plus telemetry, summarised every 15 minutes
python3 tools/soak_test.py --hours 10 --window 15
```

Board status is visible in the dashboard under **Admin → Board Telemetry Keys**
(last seen, firmware, board IP, last message), which is also the quickest way to
tell whether a board is reporting.

---

## Repository Layout

- **[`firmware/`](firmware/)** — Board firmware (PlatformIO/Arduino). API reference and hardware behaviour: [`firmware/README.md`](firmware/README.md).
- **[`server/web_ui/`](server/web_ui/)** — Dashboard (Node.js/Socket.io): live fleet view, map, user management, board key management, and the telemetry ingest endpoint.
- **[`tools/`](tools/)** — Technician GUI, key generator, API and load-test scripts, OTA uploader. See [`tools/README.md`](tools/README.md).
- **[`docs/`](docs/)**
  - [**Secure telemetry**](docs/SECURE_TELEMETRY.md) — how the encryption works, and what it does *not* protect against
  - [Telemetry benchmark](docs/TELEMETRY_BENCHMARK.md) — TLS vs. this, measured on the board
  - [Board ↔ web server data flow](docs/BOARD_TO_WEBSERVER_DATAFLOW.md)
  - [Local Development Setup](docs/local_setup/LOCAL_DEVELOPMENT_SETUP.md)
  - [Cloud Setup](docs/cloud_setup/) — EC2 dashboard and Google sign-in

## Getting Started

```bash
# build and flash the board  (use ~/.platformio/penv/bin/pio if `pio` is shadowed)
cd firmware && pio run -e main -t upload

# technician GUI
cd tools && python3 gui_main.py

# dashboard, locally
cd server/web_ui && npm install && node server.js
```

Then provision the board with the two steps above — a freshly flashed board has
no serial number and no key, and will not appear in the dashboard until it does.
