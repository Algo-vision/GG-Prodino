# Telemetry transport benchmark: board-side TLS vs. application-layer AEAD

**Date:** 2026-07-28 · **Hardware:** KMP ProDino MKR Zero (SAMD21 Cortex-M0+, 48 MHz,
32 KB RAM, 256 KB flash, W5500 Ethernet) · **Branch:** `aead-telemetry`

## Why this test exists

The board currently reaches the cloud with mutual TLS to AWS IoT. That works, but
it costs ~10 s of blocked CPU per connection and leaves under 1 KB of free RAM,
which is what makes the HTTP API stall and occasionally hard-fault. The question
was whether a cheaper construction could carry the same security properties on
this specific chip.

Rather than argue it, both designs were measured on the real board.

## Method

Two **standalone** firmwares, each running nothing but the telemetry path — no
HTTP server, no sensors, no config manager, no OCU. Every millisecond and every
byte reported therefore belongs to telemetry and nothing else.

| | firmware | server |
|---|---|---|
| old | [`src/bench_tls.cpp`](../firmware/src/bench_tls.cpp) — mutual TLS + MQTT | AWS IoT Core |
| new | [`src/bench_aead.cpp`](../firmware/src/bench_aead.cpp) — ChaCha20-Poly1305 + HTTP POST | [`tools/ingest_test_server.py`](../tools/ingest_test_server.py) |

Both compile the same measurement code ([`src/bench_common.hpp`](../firmware/src/bench_common.hpp))
and build the same ~590 B status payload, so the two logs compare line for line.
One cycle = connect → send one payload → disconnect, in both.

Run with:

```bash
pio run -e bench_aead -t upload      # then: python3 tools/ingest_test_server.py
pio run -e bench_tls  -t upload
python3 tools/capture_serial.py 120  # pio device monitor needs a TTY; this doesn't
```

## Results

Ten sends each, both **100 % successful** — this is not a case of one design
working and the other not. TLS works; it is simply expensive on this chip.

```
[SUMMARY] mode=TLS   runtime=148 s        [SUMMARY] mode=AEAD  runtime=45 s
  sends=10 fails=0 success=100%             sends=10 fails=0 success=100%
  total  ms: min=10371 avg=10393            total  ms: min=34 avg=100 max=255
             max=10407
  crypto ms: min=9771 avg=9789 max=9814     crypto ms: min=9 avg=9 max=9
  net    ms: avg=138                        net    ms: avg=91
  MAX LOOP BLOCK = 10409 ms                 MAX LOOP BLOCK = 256 ms
  freeRam now=8527  MINIMUM EVER=7239       freeRam now=27491  MINIMUM EVER=26003
```

| Metric | TLS → AWS IoT | AEAD → our server | Ratio |
|---|---|---|---|
| **CPU per message** | **9,789 ms** | **9.6 ms** | **1,020×** |
| Max loop block (board unresponsive) | 10,409 ms | 256 ms | 41× |
| Free RAM, minimum ever | 7,239 B | 26,003 B | 3.6× |
| Static RAM | 20,752 B (63.3 %) | 3,824 B (11.7 %) | 5.4× |
| Static flash | 120,012 B (45.8 %) | 51,764 B (19.7 %) | 2.3× |
| Free RAM at boot | 9,471 B | 28,495 B | 3.0× |
| Practical send interval | ≥ 15 s | 5 s, and lower is fine | |
| Delivery success | 10/10 | 10/10 | |

Put in terms of the board's time: across 148 s of runtime the TLS firmware spent
**98 seconds — two thirds of its life — doing handshake arithmetic** to deliver
ten 546-byte messages. The AEAD firmware spent 96 milliseconds to deliver ten.

### Why the gap is this large

The expensive part of TLS is the *asymmetric* crypto in the handshake — RSA-2048
in this configuration. Symmetric crypto is not expensive here at all. The
benchmark isolates exactly that: 9,789 ms of handshake versus 9.6 ms of
ChaCha20-Poly1305 over the same payload.

The handshake cost is also **unavoidable per message**, because AWS IoT does not
support TLS session resumption. The ten measured handshakes were 9771–9814 ms —
a 43 ms spread across ten runs. There is no warm-up, no amortisation, no caching.

### Two honest caveats

- **TLS on a *held* session publishes in 185 ms, not 10 s.** Keeping the session
  open is exactly what the `board-only-tls` "Plan B" design does. The catch is
  that holding it costs ~1.6 KB of heap permanently, and that is precisely the
  RAM collision that hard-faults the board when an HTTP request arrives.
- **AEAD's 256 ms maximum came from send #1 only** — ARP plus the first TCP
  connect on a cold path. Sends 2–16 ran 32–130 ms. The crypto never moved off
  9.6 ms. 256 ms is a fair worst case to quote, but it is network, not design.

## Verification beyond timing

The receiver authenticated and decrypted every packet, and the sequence numbers
increase strictly (so anti-replay is live, not just implemented):

```
OK #1  SN2003 epoch=19952 seq=22 592B  fw=bench-aead pitch=1.25 ip=192.168.1.198
OK #2  SN2003 epoch=19952 seq=23 592B  fw=bench-aead pitch=1.26 ip=192.168.1.198
OK #3  SN2003 epoch=19952 seq=24 592B  fw=bench-aead pitch=1.27 ip=192.168.1.198
```

## What the new design does and does not protect against

**Protects:** eavesdropping (ChaCha20), tampering and forgery (Poly1305 — one
flipped bit fails authentication), replay (`boot_epoch || msg_seq`, strictly
increasing per board), and device spoofing (no key ⇒ no valid packet). Keys are
**per board**, so a compromised board affects only itself and revocation is a
one-line registry edit.

**Does not protect:** no forward secrecy — anyone who later extracts a board's key
*and* recorded its traffic can decrypt that board's past messages; no defence
against key extraction from a physically opened board (this hardware has no
secure element — the same exposure the embedded TLS private key already has); and
the board does not authenticate the server, which is acceptable for send-only
telemetry but **must be revisited if cloud→board commands are ever added**.

## A note on the test environment

Early runs showed every POST failing with `connect FAILED`. The board diagnosed
it itself once a probe was added:

```
[PROBE] 192.168.1.1:80      OK      2 ms     <- router
[PROBE] 192.168.1.169:8088  FAIL 2001 ms     <- dev laptop
[PROBE] 192.168.1.169:80    FAIL 2001 ms     <- dev laptop, any port
```

The board's outbound TCP was fine; `ufw` on the development laptop was dropping
inbound LAN connections. Worked around by running the receiver in a container
(Docker's iptables rules bypass ufw). On the host directly it would need
`sudo ufw allow from 192.168.1.0/24 to any port 8088 proto tcp`. Nothing to do
with the firmware — recorded here so the same hour is not lost twice.
