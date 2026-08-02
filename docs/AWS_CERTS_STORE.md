# `firmware/src/aws_certs_store.h` — AWS device credentials store

## What it is

A **gitignored** C header (`.gitignore:27`) that embeds the AWS IoT device
credentials into the board firmware. It is the single place the credentials
live in the codebase, and it is deliberately absent from git because it
contains the device **private key**.

It defines exactly five symbols, all consumed by the `get_aws_certs` endpoint
in `firmware/src/http_server.cpp`:

| Symbol | Type | Content |
|---|---|---|
| `AWS_ENDPOINT` | `#define` string | `a21aufrn5hgzfe-ats.iot.eu-north-1.amazonaws.com` (IoT Core ATS endpoint, region `eu-north-1`) |
| `AWS_PORT` | `#define` int | `8883` (MQTT over TLS) |
| `AWS_DEV_CERT[]` | `const char[]` | Device certificate PEM (RSA, CN=`AWS IoT Certificate`, expires 2049-12-31) |
| `AWS_DEV_KEY[]` | `const char[]` | Device private key PEM (RSA-2048) |
| `AWS_ROOT_CA[]` | `const char[]` | Amazon Root CA 1 (public) |

The AWS IoT *thing* these credentials belong to is **`GRK_Board_Direct`**
(certificate ID `6a4746fb…`).

## Why the board carries the credentials (architecture)

Per the Option-B design (see `docs/BOARD_TO_WEBSERVER_DATAFLOW.md`): the board
publishes **plain MQTT** to the in-HLC gateway (Jetson/RPi); the *gateway's*
mosquitto bridges to AWS IoT over TLS. To keep the gateway OS image
hardware-agnostic and credential-free, **the board is the credential store**:

1. On boot, the gateway's `grk-fetch-certs.service` runs
   `gateway/bin/grk-fetch-certs.py`, which logs into the board and calls the
   authenticated HTTP endpoint:

   ```
   POST /  {"type":"get_aws_certs","token":<login>,"part":"cert|key|ca|endpoint"}
   ```

   One part per request, returned as raw `text/plain`. (Do **not** bundle all
   parts in one JSON response — the full ~4.5 KB payload overruns the MCU's
   memory/HTTP path and hard-faults the board.)

2. The fetcher writes the parts to `/etc/mosquitto/certs/` on the gateway,
   and mosquitto's bridge block uses them to connect to
   `AWS_ENDPOINT:AWS_PORT`.

The endpoint is token-authed and IP-whitelisted — the gateway IP must be in
the board's whitelist (`firmware/include/config_manager.hpp` default list).

## How to (re)generate it

Source material (all gitignored, transfer **out-of-band only** — never via git):

- `GRK_Board_Direct_keys/6a4746…-certificate.pem.crt` — device certificate
- `GRK_Board_Direct_keys/6a4746…-private.pem.key` — device private key
- `server/web_ui/certs/AmazonRootCA1.pem` — root CA (public; also at
  <https://www.amazontrust.com/repository/AmazonRootCA1.pem>)

Run the generator script (committed, contains no secrets):

```bash
python3 tools/generate_aws_certs_store.py
```

It locates the cert/key in `GRK_Board_Direct_keys/` automatically (errors out
unless exactly one pair is present), embeds the endpoint/port constants, and
writes `firmware/src/aws_certs_store.h`. The endpoint and port are defined at
the top of the script — update them there if the AWS account/region changes.

### Verify before flashing

```bash
# cert and key must be the same pair (identical modulus hashes):
openssl x509 -in GRK_Board_Direct_keys/*-certificate.pem.crt -noout -modulus | md5sum
openssl rsa  -in GRK_Board_Direct_keys/*-private.pem.key    -noout -modulus | md5sum

# confirm git still ignores the header:
git check-ignore firmware/src/aws_certs_store.h && echo OK
```

## If you don't have the keys (building without credentials)

The keys folder is intentionally not distributed with the repo. If you only
need the firmware to **compile** (e.g. a build machine that never talks to
AWS), create a stub `firmware/src/aws_certs_store.h` with the same five
symbols and dummy contents:

```c
#ifndef AWS_CERTS_STORE_H
#define AWS_CERTS_STORE_H
#define AWS_ENDPOINT "invalid.example.com"
#define AWS_PORT 8883
const char AWS_DEV_CERT[] = "STUB";
const char AWS_DEV_KEY[]  = "STUB";
const char AWS_ROOT_CA[]  = "STUB";
#endif
```

The board will run normally; only the gateway's cert fetch will get unusable
values, so the AWS bridge won't come up. For a real deployment, obtain
`GRK_Board_Direct_keys/` from Haim through a secure channel (not email/git).

## Security rules

- **Never commit**: `aws_certs_store.h`, `aws_client_certs.h`,
  `GRK_Board_Direct_keys/` (all gitignored). The device private key exists
  only in the keys folder, on flashed boards, and in AWS IoT.
- If the key is ever exposed, revoke certificate `6a4746fb…` in AWS IoT Core
  and issue a new one (attach the same policy/thing), then regenerate this
  header and re-flash.

## Related files / history

- `firmware/src/aws_client_certs.h` — **older, unrelated** gitignored header
  from the July 2026 throwaway on-board-TLS test (defines `AWS_CLIENT_CERT`
  etc.). Not included by any current source; do not confuse the two.
- `GRK_Board_Direct_keys/grk_ecc.csr` + `grk_ecc_private.pem.key` — pending
  ECC-certificate step for the Plan-B on-board-TLS experiment
  (`board-only-tls` branch). Once an ECC cert is issued via
  `register-certificate-without-ca`, it would be embedded the same way.
- `GATEWAY_SETUP.md` — full gateway provisioning (systemd units, mosquitto
  bridge config).
