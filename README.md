# GG-GRK - V1.5.2

GRK Controller: an embedded IoT device with dual IMU, GPS, relay control, a local
HTTP API for real-time consumers, encrypted telemetry to the cloud, and a
real-time web dashboard.

## Repository Layout

- **[`firmware/`](firmware/)** - Board firmware (PlatformIO/Arduino), HTTP API reference, hardware/LED behavior. See [`firmware/README.md`](firmware/README.md).
- **[`server/web_ui/`](server/web_ui/)** - GRK Mission Control, a real-time Node.js/Socket.io web dashboard with live GPS tracking. Also receives the boards' encrypted telemetry.
- **[`tools/`](tools/)** - Desktop GUI client, key generator, API and load-test scripts, OTA uploader. See [`tools/README.md`](tools/README.md).
- **[`docs/`](docs/)**
  - [**Secure telemetry**](docs/SECURE_TELEMETRY.md) - how the encryption works and how to key a board
  - [Telemetry benchmark](docs/TELEMETRY_BENCHMARK.md) - why it is built this way, measured on the hardware
  - [Board ↔ web server data flow](docs/BOARD_TO_WEBSERVER_DATAFLOW.md)
  - [Local Development Setup](docs/local_setup/LOCAL_DEVELOPMENT_SETUP.md)
  - [Cloud Setup](docs/cloud_setup/) - EC2 dashboard and Google sign-in

## How the board talks to the outside world

Two independent paths, which do not interfere with each other:

**1. Local HTTP API — for real-time consumers.** Anything on the same network
(a Jetson, the technician GUI) polls the board directly over JSON-over-HTTP for
GPS, IMU, relays and so on. Measured: ~18 requests/second sustained, 44 ms
median. Access is restricted by an IP whitelist plus a login token.

**2. Encrypted telemetry — for the cloud dashboard.** Once a minute the board
sends its status to the EC2 server. This is the path described below.

## Encrypted telemetry, in short

The board sends its status over **plain HTTP, but the message itself is
encrypted** — the security is in the message, not in the transport. There is no
HTTPS/TLS involved.

**Why not just HTTPS?** We measured it on this hardware. A TLS handshake costs
this 48 MHz chip **9,789 ms of blocked CPU per message** and leaves under 1 KB of
free RAM — it froze the local API for ~15 seconds at a time and sometimes crashed
the board. The expensive part is the *handshake*: the public-key maths two
strangers use to agree on a key. But we are not strangers — we own both ends. So
we put a shared secret key on the board in advance and skip the negotiation
entirely. What remains costs **~16 ms**. Full numbers:
[TELEMETRY_BENCHMARK.md](docs/TELEMETRY_BENCHMARK.md).

**What protects a message.** Each board has its **own 32-byte secret key**, known
only to that board and the server. Every message is protected with
**ChaCha20-Poly1305** — the same algorithm TLS 1.3 and WireGuard use:

| | |
|---|---|
| **Encrypted** | an eavesdropper sees only random bytes |
| **Authenticated** | change one bit and the server rejects it; forging a packet without the key is not feasible |
| **Replay-protected** | capture a packet and re-send it later and the server rejects it |
| **Per board** | one compromised board affects only itself, and revoking it is deleting one line |

The packet is `version | serial | nonce | ciphertext | tag`, POSTed to
`/api/ingest`. The serial is in clear so the server knows which key to try; it is
still covered by the authentication tag, so it cannot be tampered with.

### Where the keys live

```
   your laptop                    the board                    the EC2 server
┌────────────────┐          ┌──────────────────┐          ┌──────────────────┐
│ gen_device_key │  SET_KEY │  flash memory    │          │ device_keys.json │
│      .py       │─────────►│  WRITE-ONLY:     │          │  {"SN2003":...}  │
│                │  by USB  │  never readable  │          │                  │
│ device_keys    │          └──────────────────┘          └──────────────────┘
│    .json       │───────────────── scp ────────────────────────────►
└────────────────┘
      the only file that holds every key - gitignored, mode 600
```

The key generator writes `tools/device_keys.json`. That one file is the master
registry; the board gets its key burned in over USB and **can never be asked for
it again** (no command or endpoint returns it), and the server gets a copy so it
can decrypt. Both copies are **gitignored and never committed**.

### Putting a new board into service

All of it is done from the two GUIs - no SSH, no file copying:

1. **Give the board its serial number.** In the technician GUI (`tools/`),
   Technician Mode -> Set SN. This is what the server uses to pick the key. A
   serial can only be burned once; changing it means erasing flash first, which
   a firmware upload does.
2. **Create the key.** In the dashboard, Admin -> Board Telemetry Keys -> enter
   that serial -> Create Key. This registers it with the server at the same
   time. Press Copy. **The key is shown once and cannot be retrieved.**
3. **Burn it into the board.** In the technician GUI, Telemetry Key -> paste ->
   Burn Key to Board. The panel then shows `Key on this board: SET`.

The command line still works if you prefer it:

```bash
python3 tools/gen_device_key.py SN2003        # generate + register locally
# paste the SET_KEY: line into the board's USB console, then copy the registry:
scp -i keys/instance_gg_key.pem tools/device_keys.json \
    ubuntu@16.171.11.151:~/prodino_web_ui/device_keys.json
ssh -i keys/instance_gg_key.pem ubuntu@16.171.11.151 \
    "chmod 600 ~/prodino_web_ui/device_keys.json && pm2 restart grk-backend"
```

**Revoking a board:** Admin -> Board Telemetry Keys -> Revoke. That board is
rejected immediately; no others are affected.
**Rotating a key:** create a key for the same serial (it warns first), then burn
the new one into the board — it stops reporting until you do.

### Checking it works

```bash
# the security tests - no board needed
python3 tools/test_ingest.py http://16.171.11.151:5555
#   valid -> 204,  tampered -> 401,  replayed -> 409,  unknown serial -> 401

# what the server is receiving from real boards: Admin -> Board Telemetry Keys
# shows each board's status, last seen, firmware and last message. Or by SSH:
ssh -i keys/instance_gg_key.pem ubuntu@16.171.11.151 \
    "pm2 logs grk-backend --lines 20 --nostream"
#   🔐 [SN2003] ingest/tcp boot=9e47ab84 seq=1 1375B pitch=-0.004 roll=0.010
```

Full detail, including what this does **not** protect against:
[docs/SECURE_TELEMETRY.md](docs/SECURE_TELEMETRY.md).

## Getting Started

- Build/flash the board: [`firmware/README.md`](firmware/README.md#installation-instructions).
- Run the dashboard locally: [`docs/local_setup/LOCAL_DEVELOPMENT_SETUP.md`](docs/local_setup/LOCAL_DEVELOPMENT_SETUP.md).
- Deploy the dashboard to EC2: [`docs/cloud_setup/`](docs/cloud_setup/).
- Put a board into service: the three steps above.
