#!/usr/bin/env python3
"""GRK gateway: fetch the per-unit identity FROM the board and install it for the
Mosquitto bridge. The board holds the identity (A5) so this OS image stays
generic. Fetched over HTTP:
  - the AWS device credentials (cert/key/ca) + endpoint, ONE PART AT A TIME as
    raw text (the board can't build a single big multi-KB response), and
  - the HLC serial number, written to /etc/grk/hlc_serial so every gateway-side
    publisher (e.g. the CPU-temp service) can use the SAME grk/<serial>/... topic
    namespace as the board. A "system" = board + gateway under one serial.
Hardware-agnostic: runs unchanged on any Linux gateway (Jetson, RPi, ...) - it
only shells out to standard tools and writes standard paths.
Runs before mosquitto on boot.
"""
import os, re, sys, time, json, subprocess, urllib.request

BOARD       = os.environ.get("GRK_BOARD_IP", "192.168.1.198")
USER        = os.environ.get("GRK_USER", "admin")
PASSWD      = os.environ.get("GRK_PASS", "1234")
CERT_DIR    = "/etc/mosquitto/certs"
SERIAL_FILE = "/etc/grk/hlc_serial"
BRIDGE_CONF = "/etc/mosquitto/conf.d/grk.conf"

def req(payload, raw=False, timeout=10):
    r = urllib.request.Request(f"http://{BOARD}/", data=json.dumps(payload).encode(),
                               headers={"Content-Type": "application/json"})
    with urllib.request.urlopen(r, timeout=timeout) as resp:
        body = resp.read().decode()
        return body if raw else json.loads(body)

def login():
    j = req({"type": "login", "user": USER, "pass": PASSWD})
    if not j.get("token"):
        raise RuntimeError(f"login rejected: {j}")
    return j["token"]

# Retry the whole flow until the board is up and serves all parts.
last = None
for attempt in range(1, 13):
    try:
        tok = login()
        parts = {}
        for name in ("cert", "key", "ca", "endpoint"):
            parts[name] = req({"type": "get_aws_certs", "token": tok, "part": name}, raw=True)
            if not parts[name] or "-----BEGIN" not in parts[name] and name != "endpoint":
                raise RuntimeError(f"bad {name} response: {parts[name][:60]!r}")
        break
    except Exception as e:
        last = str(e)
        print(f"  attempt {attempt}/12: not ready ({last}); retry in 5s")
        time.sleep(5)
else:
    print(f"FAILED to fetch certs from board {BOARD}: {last}"); sys.exit(1)

os.makedirs(CERT_DIR, exist_ok=True)
for name, fn in (("cert", "device.crt"), ("key", "device.key"), ("ca", "AmazonRootCA1.pem")):
    with open(f"{CERT_DIR}/{fn}", "w") as f:
        f.write(parts[name] if parts[name].endswith("\n") else parts[name] + "\n")
subprocess.run(["chown", "mosquitto:mosquitto", f"{CERT_DIR}/device.crt",
                f"{CERT_DIR}/device.key", f"{CERT_DIR}/AmazonRootCA1.pem"], check=False)
subprocess.run(["chmod", "640", f"{CERT_DIR}/device.crt", f"{CERT_DIR}/device.key",
                f"{CERT_DIR}/AmazonRootCA1.pem"], check=False)
print(f"OK: fetched AWS certs from board {BOARD}; endpoint={parts['endpoint'].strip()}")

# Also fetch the HLC serial so gateway-side publishers share the board's topic
# namespace (grk/<serial>/...). Best-effort - never fail the cert install for it.
try:
    sj = req({"type": "get_serial_number", "token": tok})
    serial = (sj.get("serial_number") or "UNCONFIGURED").strip() or "UNCONFIGURED"
except Exception as e:
    serial = "UNCONFIGURED"
    print(f"  warn: could not fetch serial ({e}); using {serial}")
os.makedirs(os.path.dirname(SERIAL_FILE), exist_ok=True)
with open(SERIAL_FILE, "w") as f:
    f.write(serial + "\n")
print(f"OK: HLC serial = {serial} (written to {SERIAL_FILE})")

# Stamp the serial into the bridge's remote_clientid so every gateway connects
# to AWS IoT with a UNIQUE client id. AWS kicks duplicate client ids, so a
# shared device cert across many gateways only works if the client id differs
# per unit. Shared cert + unique client id = a whole fleet on one certificate.
try:
    conf = open(BRIDGE_CONF).read()
    new_id = f"grk-bridge-{serial}"
    if re.search(r"(?m)^\s*remote_clientid\s+.*$", conf):
        conf = re.sub(r"(?m)^\s*remote_clientid\s+.*$", f"remote_clientid {new_id}", conf)
    else:
        conf = conf.rstrip("\n") + f"\nremote_clientid {new_id}\n"
    open(BRIDGE_CONF, "w").write(conf)
    print(f"OK: bridge remote_clientid = {new_id}")
except Exception as e:
    print(f"  warn: could not set remote_clientid in {BRIDGE_CONF} ({e})")
