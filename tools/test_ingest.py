#!/usr/bin/env python3
"""
Exercise a /api/ingest endpoint the way the board does, plus the attacks it is
supposed to reject. Use it to verify the server after a deploy without waiting
for a board.

    python3 tools/test_ingest.py http://16.171.11.151:5555

Expects:  valid -> 204,  tampered -> 401,  replayed -> 409,  unknown serial -> 401

Defaults to the throwaway serial SNTEST, never a real board.
"""
import json, os, secrets, struct, sys, urllib.request, urllib.error

try:
    from cryptography.hazmat.primitives.ciphers.aead import ChaCha20Poly1305
except ImportError:
    sys.exit("need: pip install cryptography")

BASE = sys.argv[1].rstrip("/") if len(sys.argv) > 1 else "http://localhost:5555"
SERIAL = (sys.argv[2] if len(sys.argv) > 2 else "SNTEST").upper()
KEYS_FILE = os.path.join(os.path.dirname(__file__), "device_keys.json")

with open(KEYS_FILE) as f:
    keys = json.load(f)
if SERIAL not in keys:
    sys.exit(f"no key for {SERIAL} in {KEYS_FILE}")
KEY = bytes.fromhex(keys[SERIAL])

# A fresh boot id per run, exactly as the board draws one at every boot, so
# repeated runs never collide with their own history.
BOOT_ID = secrets.token_bytes(8)


def build(seq, boot_id=None, serial=SERIAL, payload=None):
    header = bytearray(29)
    header[0] = 2
    header[1:1 + len(serial.encode())] = serial.encode()
    nonce = (boot_id or BOOT_ID) + struct.pack(">I", seq)
    header[17:29] = nonce
    body = json.dumps(payload or {
        "type": "status", "firmwareVersion": "test-ingest",
        "pitch": 1.5, "roll": -0.25, "yaw": 90.0,
        "imuValid": True, "gpsValid": False, "controllerIp": "192.168.1.198",
        "motorWorkHours": 3.42, "busVoltage": 24.1, "powerConnected": True,
        "relays_status": [False, True, False, False],
        "optoin_status": [False, False, False, False],
    }).encode()
    ct = ChaCha20Poly1305(KEY).encrypt(nonce, body, bytes(header))
    return bytes(header) + ct


def post(pkt):
    req = urllib.request.Request(f"{BASE}/api/ingest", data=pkt,
                                 headers={"Content-Type": "application/octet-stream"})
    try:
        with urllib.request.urlopen(req, timeout=10) as r:
            return r.status
    except urllib.error.HTTPError as e:
        return e.code
    except Exception as e:
        return f"ERROR {e}"


def check(name, got, want):
    ok = got == want
    print(f"  {'PASS' if ok else 'FAIL'}  {name:<28} got {got}, expected {want}")
    return ok


print(f"testing {BASE}/api/ingest as {SERIAL} (boot {BOOT_ID.hex()})\n")
results = []

good = build(seq=1)
results.append(check("valid packet", post(good), 204))

# flip one ciphertext byte -> Poly1305 must reject it
bad = bytearray(build(seq=2))
bad[40] ^= 0x01
results.append(check("tampered ciphertext", post(bytes(bad)), 401))

# the exact packet again -> replay counter must reject it
results.append(check("replayed packet", post(good), 409))

# an older sequence number -> also a replay
results.append(check("out-of-order (older seq)", post(build(seq=0)), 409))

# a serial with no registered key
results.append(check("unknown serial", post(build(seq=3, serial="SN9999")), 401))

# a fresh higher sequence still works after all that
results.append(check("next valid packet", post(build(seq=10)), 204))

# REGRESSION: a reflashed board draws a NEW boot id and restarts at seq=1. Under
# the old stored-counter scheme the server saw a lower number and rejected the
# board until the backend was restarted. It must be accepted.
results.append(check("reflashed board (new boot)",
                     post(build(seq=1, boot_id=secrets.token_bytes(8))), 204))

print(f"\n{sum(results)}/{len(results)} passed")
sys.exit(0 if all(results) else 1)
