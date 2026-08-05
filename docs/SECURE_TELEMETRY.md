# Secure telemetry: how it works, and how to key a board

Every 5 minutes each board sends its status to the dashboard server. The message
is **encrypted**, **authenticated** and **replay-protected** — but it is *not*
sent over TLS/HTTPS, because this board cannot afford TLS. The security lives in
the message itself instead of in the transport.

If you only need to put a new board into service, jump to
[Keying a board](#keying-a-board-2-steps) — it is two commands.

---

## 1. Why not just use HTTPS?

We measured it on the actual hardware (full write-up:
[TELEMETRY_BENCHMARK.md](TELEMETRY_BENCHMARK.md)). On this SAMD21 at 48 MHz:

| | TLS to AWS IoT | What we do now |
|---|---|---|
| CPU blocked per message | **9,789 ms** | **16 ms** |
| Free RAM while running | 7.2 KB | 26 KB |
| Effect on the HTTP API | froze it ~15 s, sometimes crashed the board | none measurable |

The expensive part of TLS is the *handshake* — the public-key maths used to agree
on a session key with a stranger. It costs ~10 seconds here, every single time,
because AWS IoT does not support session resumption.

But we are not talking to a stranger. **We control both ends**, so we can put a
shared secret key on the board in advance and skip the entire negotiation. The
symmetric crypto that TLS would use *after* its handshake costs 16 ms.

That is the whole idea: keep the encryption, drop the handshake.

---

## 2. What protects the message

Each board has its **own 32-byte secret key**, known only to that board and the
server. The message is protected with **ChaCha20-Poly1305**, a standard
authenticated-encryption algorithm (the same one used by TLS 1.3, WireGuard and
SSH):

- **ChaCha20** encrypts the status JSON — an eavesdropper sees only random bytes.
- **Poly1305** adds a 16-byte authentication tag over the whole packet. Change
  one bit anywhere and the server rejects it. Forging a packet without the key is
  not feasible.
- A **nonce** makes every message unique, which also lets the server reject
  replays (see below).

### The packet

```
offset  size  field
0       1     version = 0x02
1       16    serial number, ASCII, NUL-padded   e.g. "SN2003"
17      12    nonce = boot_id(8) || msg_seq(4)
29      N     ciphertext (the status JSON, encrypted)
29+N    16    Poly1305 authentication tag
```

Sent as `POST /api/ingest` with `Content-Type: application/octet-stream`.
Typical size ~880 bytes.

The 29-byte header is **authenticated but not encrypted** — the server must read
the serial number in clear to know *which* key to try. It is covered by the tag,
so it still cannot be tampered with.

### The nonce, and why it matters

ChaCha20-Poly1305 has one hard rule: **never reuse a nonce with the same key.**
Breaking it leaks the XOR of two messages and undermines the tag. So the nonce is
built to make repetition structurally impossible:

- `boot_id` — 8 random bytes, drawn fresh every time the board powers up.
- `msg_seq` — a counter starting at 1, incremented per message within that boot.

The board draws `boot_id` by hashing (SHA-256) three independent entropy sources:
ADC noise on a floating pin, `micros()` jitter, and how long the boot took (which
varies by milliseconds every start because IMU calibration and DHCP never take
exactly as long). The device key is mixed in too, so two boards cannot collide
even given identical entropy.

> **Why random instead of a stored counter?** We tried a counter in flash first.
> `FlashStorage` on this chip keeps its data *inside the sketch image*, so every
> firmware upload erased it — the board restarted at 1, the server saw a number it
> had already accepted, and correctly rejected the board as a replay. Technicians
> reflash boards often, so that was unworkable. Random needs no storage: there is
> nothing for an upload to erase.

### Replay protection

The server remembers, per board, which `boot_id`s it has seen (the last 512) and
the highest `msg_seq` within each. A packet is accepted only if its boot id is
new, or its sequence advances that boot's counter. Capturing a packet and sending
it again later gets `409`.

### Server responses

| code | meaning |
|---|---|
| `204` | accepted, dashboard updated |
| `401` | unknown serial, or authentication failed (tampered / wrong key) |
| `409` | replay — this packet was already seen |
| `400` | malformed packet |

The server deliberately does not say *which* check failed.

---

## 3. Keying a board (2 steps)

Every board needs its own key. Generating one and burning it is two commands.

### Step 1 — generate the key (on your laptop)

```bash
python3 tools/gen_device_key.py SN2003
```

Replace `SN2003` with the board's serial number. This prints:

```
registered SN2003 in tools/device_keys.json (mode 600)

Provision the board (technician mode) - paste into its serial console:

    SET_KEY:512f53d3384048b4a44093f7078cee3ca726fe02092d558b4212d91ee0b58a3f

Then power-cycle the board. Verify with:  GET_KEY_STATUS
```

The key is saved to `tools/device_keys.json`. **That file is a secret** — it is
gitignored, kept mode 600, and must never be committed or emailed.

### Step 2 — burn it into the board

Connect the board by USB and open its serial console:

```bash
cd firmware
pio device monitor -b 115200 -f send_on_enter
```

Paste the `SET_KEY:...` line from step 1 and press **Enter**. You should see:

```
SUCCESS: device telemetry key burned (32 bytes)
Reboot to start using the new key
```

Power-cycle the board. To confirm, type `GET_KEY_STATUS` — it should answer
`Device key: SET`. The boot log will also show:

```
[TELEM] key source: flash (SET_KEY)
[TELEM] ready. bootId=3369F653A21B04E7
```

That's it. The board is keyed for life; the key survives reboots and power cuts.

> The key is **write-only**. No endpoint, command or API will ever read it back
> out — not even in technician mode. If you lose it, generate a new one and
> re-burn (step 1 warns you before overwriting).

### Step 3 — tell the server about the key

The server needs the same key to decrypt. Copy the registry across and restart:

```bash
scp -i keys/instance_gg_key.pem tools/device_keys.json \
    ubuntu@16.171.11.151:~/prodino_web_ui/device_keys.json
ssh -i keys/instance_gg_key.pem ubuntu@16.171.11.151 \
    "chmod 600 ~/prodino_web_ui/device_keys.json && pm2 restart grk-backend"
```

The log should then show `🔑 Loaded telemetry keys for: SN2003, ...`.

### Revoking or rotating a board

- **Revoke:** delete that board's entry from the server's `device_keys.json` and
  restart the backend. That board is rejected immediately; no other board is
  affected.
- **Rotate:** run step 1 again (it asks before overwriting), then repeat steps
  2 and 3.

---

## 4. Checking that it works

**On the board** (serial console) — one line per send:

```
[PROFILE] blocking: crypto=16 connect=78 write=3 ms (reply+close deferred)
[PROFILE] total=175 ms (blocking was crypto=16 + connect=78)
[BENCH] mode=AEAD  sends=12 fails=0 ...
```

**On the server:**

```bash
ssh -i keys/instance_gg_key.pem ubuntu@16.171.11.151 "pm2 logs grk-backend --lines 20 --nostream"
# 🔐 [SN2003] ingest boot=3369f653 seq=7 881B pitch=0 roll=0 ip=192.168.1.198
```

**Without a board** — verify the server's crypto, including the attacks it must
reject:

```bash
python3 tools/test_ingest.py http://16.171.11.151:5555
```

```
  PASS  valid packet                 got 204, expected 204
  PASS  tampered ciphertext          got 401, expected 401
  PASS  replayed packet              got 409, expected 409
  PASS  out-of-order (older seq)     got 409, expected 409
  PASS  unknown serial               got 401, expected 401
  PASS  next valid packet            got 204, expected 204
  PASS  reflashed board (new boot)   got 204, expected 204
```

It uses the throwaway serial `SNTEST`, never a real board, and `SNTEST` is hidden
from the dashboard.

---

## 5. Cost on the board

The board's own HTTP API is the real-time path — local consumers (e.g. a Jetson)
poll GPS and IMU at 20 Hz — so what matters is not how long a send *takes*, but
how long it stops `loop()` from serving them. Three things were done about it:

1. **Waiting for the reply is deferred.** `telemetrySendStatus()` returns as soon
   as the request is on the wire; `telemetryPump()` collects the response from
   `loop()` over later iterations. Saves ~80 ms of blocking.
2. **The connection is held open.** The ~78 ms TCP handshake is paid once at
   startup, not on every send. Measured against EC2 from the same router the
   board is on: an idle connection survives at least a 300 s gap, so a 5-minute
   interval reuses it. If it does drop, the next send transparently reopens it.
3. **No sensor read in the send path.** The payload uses the snapshot the 1 Hz
   loop already refreshed (`statusGenerateJsonNoRefresh()`), so the send does not
   pay for blocking I2C reads.

| | measured (per-send connect) | with the persistent connection |
|---|---|---|
| Blocks `loop()` per send | 97 ms (16 crypto + 78 connect + 3 write) | **~15 ms** (crypto + write) |
| HTTP API during a send | 100% at 25 req/s, worst stall 240 ms | worst stall expected ~50 ms |

> The 97 ms column is measured on hardware. The persistent-connection column is
> the design target — the keep-alive behaviour it depends on is measured, but the
> board-side figure still needs confirming on the bench.

The interval is a product decision, not a limit: telemetry at 5 minutes costs
about 0.005% of the board's time.

**UDP alternative.** Build with `-D TELEM_USE_UDP=1` and the send becomes a
single datagram — no handshake, no teardown, no connection to keep alive. The
packet is already self-contained and authenticated, so nothing is lost but
delivery confirmation, and telemetry is best-effort anyway. It is not the default
only because inbound UDP must first be opened in the EC2 security group.

---

## 6. What this does and does not protect against

**Protects against**

- **Eavesdropping** — the payload is encrypted; the network sees random bytes.
- **Tampering / forgery** — Poly1305 rejects any modified packet, and a packet
  cannot be created without the board's key.
- **Replay** — a captured packet resent later is rejected.
- **Spoofing** — no key, no valid packet. Keys are per board, so one compromised
  board affects only itself and is revoked with a one-line registry edit.

**Does not protect against**

- **No forward secrecy.** Someone who later extracts a board's key *and* recorded
  its past traffic can decrypt that board's past messages. TLS with ephemeral
  keys would prevent this; it costs 9.8 s per message on this chip.
- **Key extraction from an opened board.** There is no secure element on this
  hardware. This is the same exposure an embedded TLS private key would have had.
- **The board does not authenticate the server.** Fine for send-only telemetry —
  an attacker impersonating the server learns nothing (the traffic is encrypted to
  a key they don't have) and cannot send commands. **This must be revisited if
  cloud→board commands are ever added**; those would need their own signed-command
  design.

---

## 7. Files

| what | where |
|---|---|
| Board: build, encrypt, send | `firmware/src/secure_telemetry.cpp` |
| Board: packet format + nonce rules | `firmware/include/secure_telemetry.hpp` |
| Board: key storage (write-only) | `firmware/src/config_manager.cpp` |
| Server: decrypt, verify, replay-check | `server/web_ui/server.js` (`/api/ingest`) |
| Key generator | `tools/gen_device_key.py` |
| Security tests | `tools/test_ingest.py` |
| Reference receiver (offline testing) | `tools/ingest_test_server.py` |
| Crypto library (BearSSL, vendored) | `firmware/lib/BearSSL/` |
| Secrets — never commit | `tools/device_keys.json`, `server/web_ui/device_keys.json` |
