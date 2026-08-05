#!/usr/bin/env python3
"""
Does ONE TCP connection to the ingest server survive long idle gaps?

That is the only real risk in the board holding a persistent connection: the
board pays the ~78 ms TCP handshake once instead of on every send, but if the
router's NAT table or the server drops an idle connection, the next send falls
into a dead socket.

This runs from the laptop, which sits behind the SAME router as the board, so
it exercises the same NAT path. It opens one socket, then sends a real encrypted
telemetry packet after each idle gap - on that same socket - and reports whether
the connection was still alive.

Usage:
    python3 tools/test_keepalive.py http://16.171.11.151:5555 [gap_seconds ...]

    # default: probe after 30 s, 60 s, 120 s, 300 s of idle
    python3 tools/test_keepalive.py http://16.171.11.151:5555
    # just check a 5-minute gap
    python3 tools/test_keepalive.py http://16.171.11.151:5555 300
"""
import json, os, secrets, socket, struct, sys, time

try:
    from cryptography.hazmat.primitives.ciphers.aead import ChaCha20Poly1305
except ImportError:
    sys.exit("need: pip install cryptography")

BASE = sys.argv[1].rstrip("/") if len(sys.argv) > 1 else "http://localhost:5555"
GAPS = [int(a) for a in sys.argv[2:]] or [30, 60, 120, 300]
SERIAL = "SNTEST"
KEYS_FILE = os.path.join(os.path.dirname(__file__), "device_keys.json")

HOST = BASE.split("//", 1)[1].split(":")[0]
PORT = int(BASE.rsplit(":", 1)[1])

with open(KEYS_FILE) as f:
    KEY = bytes.fromhex(json.load(f)[SERIAL])

BOOT_ID = secrets.token_bytes(8)


def build(seq):
    header = bytearray(29)
    header[0] = 2
    header[1:1 + len(SERIAL)] = SERIAL.encode()
    nonce = BOOT_ID + struct.pack(">I", seq)
    header[17:29] = nonce
    body = json.dumps({"type": "status", "firmwareVersion": "keepalive-test",
                       "pitch": 1.5, "roll": -0.25, "seq": seq}).encode()
    return bytes(header) + ChaCha20Poly1305(KEY).encrypt(nonce, body, bytes(header))


def send_on(sock, seq):
    """Send one packet with Connection: keep-alive; return (status, elapsed_ms)."""
    pkt = build(seq)
    req = (f"POST /api/ingest HTTP/1.1\r\nHost: {HOST}\r\n"
           f"Content-Type: application/octet-stream\r\n"
           f"Content-Length: {len(pkt)}\r\nConnection: keep-alive\r\n\r\n").encode() + pkt
    t0 = time.time()
    sock.sendall(req)
    resp = sock.recv(256)
    ms = (time.time() - t0) * 1000
    if not resp:
        raise ConnectionError("peer closed")
    return resp.split(b"\r\n", 1)[0].decode(errors="replace"), ms


print(f"keep-alive probe: {HOST}:{PORT}   idle gaps: {GAPS}\n")

t0 = time.time()
sock = socket.create_connection((HOST, PORT), timeout=20)
handshake_ms = (time.time() - t0) * 1000
print(f"  TCP connect (the cost the board pays ONCE): {handshake_ms:.0f} ms\n")

seq = 0
results = []
try:
    seq += 1
    status, ms = send_on(sock, seq)
    print(f"  t=0s        send #{seq}  -> {status}  ({ms:.0f} ms)")

    for gap in GAPS:
        print(f"  ... idling {gap}s ...", flush=True)
        time.sleep(gap)
        seq += 1
        try:
            status, ms = send_on(sock, seq)
            alive = "204" in status
            print(f"  after {gap:>4}s idle  send #{seq}  -> {status}  ({ms:.0f} ms)"
                  f"   {'CONNECTION ALIVE' if alive else 'unexpected status'}")
            results.append((gap, alive, ms))
        except Exception as e:
            print(f"  after {gap:>4}s idle  send #{seq}  -> DEAD: {type(e).__name__}: {e}")
            results.append((gap, False, None))
            break
finally:
    sock.close()

print("\n--- verdict ---")
if results and all(a for _, a, _ in results):
    longest = max(g for g, _, _ in results)
    reuse = [m for _, a, m in results if a and m]
    print(f"  connection survived every gap up to {longest}s")
    if reuse:
        print(f"  send on the open connection: {min(reuse):.0f}-{max(reuse):.0f} ms "
              f"(vs {handshake_ms:.0f} ms just to open a new one)")
    print("  => the board can hold one connection and skip the handshake per send")
else:
    died = next((g for g, a, _ in results if not a), None)
    print(f"  connection DIED after a {died}s idle gap")
    print("  => either send more often than that, or use UDP (delivery is best-effort anyway)")
