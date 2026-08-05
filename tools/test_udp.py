#!/usr/bin/env python3
"""
UDP telemetry transport check - the counterpart to test_keepalive.py.

test_keepalive.py answers "can the board hold ONE TCP connection open across
idle gaps?". This answers the equivalent questions for UDP, which is the other
option for getting the ~78 ms TCP handshake off the board's critical path:

  - does a packet get through, and is it accepted (crypto + replay verified)?
  - is anything lost across long idle gaps? (UDP has no connection to go stale,
    which is the whole point - there is nothing for a NAT box to drop)
  - what does a send actually cost?
  - is a full-size telemetry packet delivered without IP fragmentation?

Usage:
    python3 tools/test_udp.py http://HOST:PORT [gap_seconds ...]
    python3 tools/test_udp.py http://192.168.1.169:5599            # default gaps
    python3 tools/test_udp.py http://192.168.1.169:5599 30 60      # custom gaps

Note the server replies with ONE byte: '2' accepted, '4' rejected, '9' replay.
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


def build(seq, pad=0):
    """One telemetry packet. pad grows the payload to a realistic size."""
    header = bytearray(29)
    header[0] = 2
    header[1:1 + len(SERIAL)] = SERIAL.encode()
    nonce = BOOT_ID + struct.pack(">I", seq)
    header[17:29] = nonce
    doc = {"type": "status", "firmwareVersion": "udp-test", "seq": seq,
           "pitch": 1.5, "roll": -0.25, "yaw": 90.0, "controllerIp": "192.168.1.198"}
    if pad:
        doc["pad"] = "x" * pad
    body = json.dumps(doc).encode()
    return bytes(header) + ChaCha20Poly1305(KEY).encrypt(nonce, body, bytes(header))


def send(sock, pkt, timeout=5):
    """Returns (verdict_char_or_None, elapsed_ms). None = no reply."""
    sock.settimeout(timeout)
    t0 = time.time()
    sock.sendto(pkt, (HOST, PORT))
    try:
        reply, _ = sock.recvfrom(8)
        return chr(reply[0]), (time.time() - t0) * 1000
    except socket.timeout:
        return None, (time.time() - t0) * 1000


sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
print(f"UDP telemetry probe: {HOST}:{PORT}\n")
ok = True

# ---- 1. basic delivery + acceptance -----------------------------------------
seq = 0
seq += 1
verdict, ms = send(sock, build(seq))
if verdict is None:
    print("  FAIL  no reply at all - is the port open (EC2 security group) and the server up?")
    sys.exit(1)
print(f"  {'PASS' if verdict == '2' else 'FAIL'}  accepted            "
      f"verdict='{verdict}' ({ms:.1f} ms)")
ok &= verdict == "2"

# ---- 2. replay is still rejected over UDP -----------------------------------
verdict, ms = send(sock, build(seq))
print(f"  {'PASS' if verdict == '9' else 'FAIL'}  replay rejected     "
      f"verdict='{verdict}' ({ms:.1f} ms)")
ok &= verdict == "9"

# ---- 3. burst: loss + latency spread ----------------------------------------
lat, lost = [], 0
for _ in range(40):
    seq += 1
    verdict, ms = send(sock, build(seq), timeout=2)
    if verdict == "2":
        lat.append(ms)
    else:
        lost += 1
n = len(lat) + lost
print(f"  {'PASS' if lost == 0 else 'WARN'}  burst of {n:<3}          "
      f"delivered {len(lat)}/{n} ({100*len(lat)//n}%), "
      f"rtt min={min(lat):.1f} avg={sum(lat)/len(lat):.1f} max={max(lat):.1f} ms")

# ---- 4. full-size packet (must not fragment: ~880 B < 1500 MTU) -------------
seq += 1
big = build(seq, pad=700)
verdict, ms = send(sock, big)
print(f"  {'PASS' if verdict == '2' else 'FAIL'}  full-size {len(big)}B     "
      f"verdict='{verdict}' ({ms:.1f} ms)   MTU headroom: {1500 - 28 - len(big)} B")
ok &= verdict == "2"

# ---- 5. idle gaps: UDP keeps no state, so nothing can go stale --------------
print(f"\n  idle-gap check (the TCP equivalent is what test_keepalive.py measures):")
for gap in GAPS:
    print(f"  ... idling {gap}s ...", flush=True)
    time.sleep(gap)
    seq += 1
    verdict, ms = send(sock, build(seq))
    alive = verdict == "2"
    ok &= alive
    print(f"  {'PASS' if alive else 'FAIL'}  after {gap:>4}s idle    "
          f"verdict='{verdict}' ({ms:.1f} ms)")

sock.close()
print(f"\n--- verdict ---")
print("  UDP has no connection state, so an idle gap costs nothing and there is"
      "\n  nothing a NAT box or the server can drop between sends." if ok else
      "  something failed - see above")
sys.exit(0 if ok else 1)
