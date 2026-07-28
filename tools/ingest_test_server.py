#!/usr/bin/env python3
"""
Local receiver for the board's encrypted telemetry (Phase-0 benchmark).

Implements the exact verify/decrypt/replay logic that will later go into the EC2
Express route, so it doubles as the reference implementation to validate the
board's crypto against.

Wire format (see firmware/include/secure_telemetry.hpp):
    off  size  field
    0    1     version = 0x02
    1    16    serial (ASCII, NUL-padded)
    17   12    nonce = boot_id(8, random per boot) || msg_seq(4 BE)
    29   N     ciphertext (ChaCha20)
    29+N 16    Poly1305 tag
    AAD = bytes 0..28

Usage:
    python3 tools/ingest_test_server.py [PORT]     # default 8088
Keys are read from tools/device_keys.json  {"SN2003": "<64 hex>"}
"""
import json, os, sys, struct
from http.server import BaseHTTPRequestHandler, HTTPServer

try:
    from cryptography.hazmat.primitives.ciphers.aead import ChaCha20Poly1305
except ImportError:
    sys.exit("need: pip install cryptography")

PORT = int(sys.argv[1]) if len(sys.argv) > 1 else 8088
KEYS_FILE = os.path.join(os.path.dirname(__file__), "device_keys.json")

HEADER_LEN, TAG_LEN = 29, 16

def load_keys():
    if not os.path.exists(KEYS_FILE):
        sys.exit(f"missing {KEYS_FILE} - run tools/gen_device_key.py <SERIAL>")
    with open(KEYS_FILE) as f:
        return {k: bytes.fromhex(v) for k, v in json.load(f).items()}

KEYS = load_keys()
# serial -> {boot_id_hex: highest msg_seq}. Boot ids are random per boot, so an
# unseen one is a genuine restart; only a repeat within a boot is a replay.
SEEN = {}
STATS = {"ok": 0, "bad_auth": 0, "replay": 0, "unknown": 0}

class Handler(BaseHTTPRequestHandler):
    def log_message(self, *a):
        pass                                   # keep the console clean

    def do_POST(self):
        if self.path != "/api/ingest":
            self.send_response(404); self.end_headers(); return
        n = int(self.headers.get("Content-Length", 0))
        pkt = self.rfile.read(n)

        if len(pkt) < HEADER_LEN + TAG_LEN:
            return self._reply(400, "short")

        header = pkt[:HEADER_LEN]                       # AAD
        version = header[0]
        serial = header[1:17].split(b"\x00")[0].decode("ascii", "replace")
        nonce = header[17:29]
        boot_id = nonce[:8].hex()
        (msg_seq,) = struct.unpack(">I", nonce[8:])
        ct_and_tag = pkt[HEADER_LEN:]

        key = KEYS.get(serial)
        if version != 2 or key is None:
            STATS["unknown"] += 1
            print(f"  UNKNOWN serial={serial!r} version={version}")
            return self._reply(401, "no")

        # authenticity + confidentiality (raises if tampered)
        try:
            plaintext = ChaCha20Poly1305(key).decrypt(nonce, ct_and_tag, header)
        except Exception:
            STATS["bad_auth"] += 1
            print(f"  BAD AUTH serial={serial} boot={boot_id} seq={msg_seq}")
            return self._reply(401, "no")

        # replay: the sequence must advance within this boot
        boots = SEEN.setdefault(serial, {})
        if msg_seq <= boots.get(boot_id, 0):
            STATS["replay"] += 1
            print(f"  REPLAY   serial={serial} boot={boot_id} seq={msg_seq}")
            return self._reply(409, "old")
        boots[boot_id] = msg_seq

        STATS["ok"] += 1
        try:
            data = json.loads(plaintext)
            summary = (f"fw={data.get('firmwareVersion')} "
                       f"pitch={data.get('pitch'):.2f} roll={data.get('roll'):.2f} "
                       f"ip={data.get('controllerIp')} motorS={data.get('motorWorkSeconds')}")
        except Exception:
            summary = f"{len(plaintext)}B (not JSON)"
        print(f"OK #{STATS['ok']:<4} {serial} boot={boot_id[:8]} seq={msg_seq} "
              f"{len(pkt)}B  {summary}")
        self._reply(204, "")

    def _reply(self, code, _msg):
        self.send_response(code)
        self.send_header("Content-Length", "0")
        self.end_headers()

if __name__ == "__main__":
    print(f"ingest receiver on 0.0.0.0:{PORT}/api/ingest   keys: {list(KEYS)}")
    print("(Ctrl+C to stop)\n")
    try:
        HTTPServer(("0.0.0.0", PORT), Handler).serve_forever()
    except KeyboardInterrupt:
        print(f"\nstats: {STATS}")
