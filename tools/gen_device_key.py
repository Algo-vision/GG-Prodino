#!/usr/bin/env python3
"""
Generate a per-board telemetry key.

Each board gets its OWN 32-byte key, so a compromised board never exposes any
other, and revoking one is a single registry deletion.

Usage:
    python3 tools/gen_device_key.py SN2003            # generate + register
    python3 tools/gen_device_key.py SN2003 --show     # print existing key line

Writes tools/device_keys.json (gitignored) and prints the provisioning command
to paste into the board's serial console while in technician mode.
"""
import json, os, secrets, sys

KEYS_FILE = os.path.join(os.path.dirname(__file__), "device_keys.json")

def load():
    if os.path.exists(KEYS_FILE):
        with open(KEYS_FILE) as f:
            return json.load(f)
    return {}

def save(keys):
    with open(KEYS_FILE, "w") as f:
        json.dump(keys, f, indent=2, sort_keys=True)
    os.chmod(KEYS_FILE, 0o600)          # secrets: owner-only

def main():
    if len(sys.argv) < 2:
        sys.exit(__doc__)
    serial = sys.argv[1].strip().upper()
    show_only = "--show" in sys.argv

    keys = load()
    if show_only:
        if serial not in keys:
            sys.exit(f"no key registered for {serial}")
        key_hex = keys[serial]
    else:
        if serial in keys:
            ans = input(f"{serial} already has a key - ROTATE it? "
                        f"(the board must be re-provisioned) [y/N] ")
            if ans.strip().lower() != "y":
                sys.exit("aborted")
        key_hex = secrets.token_bytes(32).hex()
        keys[serial] = key_hex
        save(keys)
        print(f"registered {serial} in {KEYS_FILE} (mode 600)\n")

    print("Provision the board (technician mode) - paste into its serial console:")
    print(f"\n    SET_KEY:{key_hex}\n")
    print("Then power-cycle the board. Verify with:  GET_KEY_STATUS")

if __name__ == "__main__":
    main()
