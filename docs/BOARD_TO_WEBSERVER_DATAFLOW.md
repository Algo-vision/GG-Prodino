# Board → Web Server: how the whole connection works

How data gets from the GRK board (in the HLC) all the way to the cloud web
dashboard, and — separately — to local consumers. There are **two independent
data planes**; understanding them separately is the key to the whole system.

```
                      ┌──────────────────────── THE HLC (field unit) ────────────────────────┐
                      │                                                                        │
   Local consumers    │   ┌───────────┐   plain MQTT 1 Hz    ┌──────────────────────────┐     │
   (GUI, tools) ◀────HTTP─┤ GRK board │────(grk/<SN>/...)───▶│  Gateway (Jetson / RPi)  │     │
   request/response  │   │  SAMD21   │   :1883               │  Mosquitto broker        │     │
   :80 (as fast as   │   │  +W5500   │                       │  + TLS bridge            │     │
    the board can)   │   │           │◀──HTTP: fetch certs───┤  grk-fetch-certs         │     │
                      │   └───────────┘   + serial (once)    └────────────┬─────────────┘     │
                      │    holds AWS certs                                 │                   │
                      └────────────────────────────────────────────────── │ ──────────────────┘
                                                                           │ TLS 8883 (mutual)
                                                                           ▼
                                                              ┌────────────────────────┐
                                                              │   AWS IoT Core          │
                                                              │   (broker, grk/#)       │
                                                              └────────────┬───────────┘
                                                                           │ MQTT-TLS (backend cert)
                                                                           ▼
                                                              ┌────────────────────────┐
   Browser ◀───── Socket.io / HTTP :5555 ──────────────────  │  Web backend (Node)     │
   (dashboard)    (live device_update events)                │  EC2, prodino_web_ui    │
                                                              │  subscribes grk/#       │
                                                              └────────────────────────┘
```

## Plane 1 — HTTP (local, fast, request/response)

**Who:** local consumers on the HLC LAN — the desktop GUI, `tools/`, the
orientation viewer. **Not** the cloud.

- **Transport:** plain HTTP on **port 80**, board is the server.
- **Shape:** one JSON request → one JSON response, e.g.
  `POST / {"type":"get_imu","token":"..."}`. Endpoints: `login`, `get_config`,
  `get_overview`, `get_imu`, `get_gps`, `get_status`, `set_*`, `get_aws_certs`,
  `get_serial_number`.
- **Auth:** login returns a token; every call carries it. **Source IP must be in
  the board's whitelist** or the board returns `403` before checking the token.
- **Speed:** "as fast as the board can" (requirement B2). Measured capacity and
  its limits are in the **Capacity** section below.

## Plane 2 — MQTT (to the cloud dashboard, throttled to 1 Hz)

**Who:** the web dashboard. **Path has four hops:**

1. **Board → gateway (plain MQTT, :1883).** The board publishes once per second
   to `grk/<serial>/...` (status, imu/*, gps/*, relays, leds, power, sensors) on
   the gateway's local Mosquitto. Plain TCP — no TLS on the MCU (RAM-limited).
   Client id `grk_<serial>`.
2. **Gateway → AWS IoT (TLS bridge, :8883).** Mosquitto bridges `grk/# out` to
   AWS IoT Core over mutual TLS. The gateway also publishes
   `grk/<serial>/jetson/cpu_temp`. Bridge client id `grk-bridge-<serial>`
   (unique per unit → a shared device cert works fleet-wide).
3. **AWS IoT Core.** Broker; routes `grk/#`. Account `259459697584`, region
   `eu-north-1`, endpoint `a21aufrn5hgzfe-ats.iot.eu-north-1.amazonaws.com`.
4. **AWS → web backend (Node).** `server.js` connects to AWS IoT with its own
   `gg_backend` cert, subscribes `grk/#`, parses the serial out of each topic,
   keeps a per-device state map, and pushes `device_update` events to browsers
   over **Socket.io** (served on **:5555**, behind Google OAuth login).

### Identity flow (why the client never touches AWS)

The **board holds the AWS device cert + key + the HLC serial**. On boot the
gateway runs `grk-fetch-certs`, which pulls them from the board over HTTP
(`get_aws_certs` per-part, `get_serial_number`), installs the certs for the
bridge, writes `/etc/grk/hlc_serial`, and stamps the unique bridge client id.
So the gateway OS image is generic and the same firmware image (one shared cert)
goes on every board — see [`../gateway/README.md`](../gateway/README.md) for the
gateway setup and install steps.

### Why two planes?

HTTP is request/response and can't push; it's perfect for a local operator tool
that pulls fresh data on demand as fast as possible. MQTT is publish/subscribe
and survives the WAN, NAT, and reconnects — right for a cloud dashboard, but
throttled to 1 Hz to keep the MCU's radio/CPU budget free for the HTTP plane.

---

## Capacity — how much HTTP the board can serve, and why

Measured from a whitelisted client, board reachable at ~3 ms ping RTT
(so the LAN is **not** the bottleneck):

| Endpoint | Response | Throughput | Latency p50 |
|---|---|---|---|
| `get_config` | 293 B | **~14.5 req/s** | 46 ms |
| `get_imu` | 352 B | ~13.6 req/s | 58 ms |
| `get_status` | 948 B | ~10.4 req/s | 106 ms |

Concurrency does **not** help — 2–4 parallel clients stay at ~14 req/s and most
requests **error out** (the board effectively serves one connection at a time).

### Why ~14 req/s is the ceiling (in order of impact)

1. **Nagle + delayed-ACK (~40 ms/req) — the dominant cost, and it's SOFTWARE.**
   The ~43 ms latency floor with only ~3 ms RTT is the classic signature. The
   board writes each response as ~10 small TCP segments (status line, each
   header, body — `httpSendResponse`). With Nagle on (W5500 default) and the
   client's delayed-ACK timer (~40 ms), the board stalls waiting for an ACK
   before sending the rest. **This is not a hardware limit** — coalescing the
   response into a single `client.write()` (or enabling TCP_NODELAY) would
   remove most of the 40 ms and raise throughput several-fold. *(Not changed
   yet — flagged as the #1 optimization.)*
2. **New TCP connection per request (`Connection: close`).** No keep-alive, so
   every request pays a full handshake + teardown (`client.stop()`) on the
   W5500. Reusing the connection would cut per-request overhead.
3. **Single-threaded, one effective connection.** SAMD21 Cortex-M0+ @ 48 MHz
   processes a request fully before the next; the request loop reads byte-by-byte
   building an Arduino `String`. Parallel clients contend for the few W5500
   sockets and mostly time out — hence the concurrency failures.
4. **Response size → SPI time (a real but secondary hardware cost).** Bigger
   payloads take longer to shuttle to the W5500 over SPI, which is why
   `get_status` (948 B) runs slower than `get_config` (293 B). This is the one
   genuinely hardware-bound factor here, and it's minor next to #1.

### Hard hardware ceilings (context, not today's bottleneck)

- **RAM (32 KB).** Caps response size: ~1 KB responses are fine; a single ~4.5 KB
  response hard-faults the board (why `get_aws_certs` serves one part per call).
- **W5500 sockets (max ~8).** Limits real concurrency even if the software were
  multi-threaded.
- **SAMD21 @ 48 MHz, W5500 over SPI.** The raw byte throughput ceiling is far
  above 14 req/s — today we're nowhere near it; we're pinned by #1–#3.

### Bottom line

**The current ~14 req/s is set by software (Nagle/delayed-ACK + per-request
connection setup + single-threading), not by the CPU, the network, or the SPI
bus.** The single highest-value change is sending each HTTP response as one write.
For the intended use (a local operator tool polling a few endpoints), ~14 req/s
with ~50 ms latency is already comfortably "as fast as the board can" for one
consumer; it only becomes a limit if many consumers poll hard in parallel.
