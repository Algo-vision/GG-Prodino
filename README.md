# GG-GRK — V1.5.1.1

Firmware for the GRK Pro Dino controller, plus the tools used to configure and
verify it on the bench.

This branch is **Ron's V1.5.1 delivery, unmodified, with Priority 2 on top and
the Priority 1 measurement** — and nothing else. The P1 probe compiles out of
the production build entirely, so `pio run -e main` is byte-identical with it
present or absent. The GPS, HTTP and I2C performance work that measurement led
to is *not* here; it lives on `perf-experiments`, pending review.

---

## What is here

```
firmware/                 the controller firmware (PlatformIO, SAMD21)
tools/
  technician_gui/         PyQt desktop tool for a board on the bench
  bench/                  hardware acceptance tests
temp/                     scratch - notes, task inputs, logs. Ignored.
GG-GRK_V1.5.1/            the reference build as delivered, for diffing. Ignored.
```

Each directory has its own README with the detail; this one says how they fit
together.

### `firmware/`

Reads two IMUs and a GPS over I2C, fuses them into pitch / roll / yaw, drives
four relays and the status LEDs, watches the OCU, and answers a JSON API over
Ethernet.

```sh
cd firmware
pio run -e main        # build
pio test -e native     # host-side unit tests, no hardware needed
pio run -e main -t upload
```

What V1.5.1.1 adds over V1.5.1 is the calibration work (Priority 2): level is
burned once by a technician instead of being guessed at every power-on, and the
gyro's zero is learned whenever the machine is genuinely at rest rather than
measured during `setup()`. See `firmware/README.md`.

Priority 1 — what `dt` in the angle formula actually is — is answered in
[firmware/docs/P1_dt_stress_test.md](firmware/docs/P1_dt_stress_test.md):
**20.0 ms minimum, 89.5 ms maximum idle; 221.3 ms maximum under one HTTP
consumer**, where 42.9% of updates exceed the filter's 100 ms clamp. The probe
that measured it is behind `-D TIMING_PROBE=1` (`pio run -e timing`), bench
only.

### `tools/technician_gui/`

```sh
cd tools/technician_gui && python3 gui_main.py
```

Live status, relays, LEDs, IP configuration, IMU axis map, and the two
calibrations. This is **V1.5.1's own GUI** with the Calibration box added and
nothing else changed - deliberately not the v1.5.2 build of it, which sends six
commands V1.5.1 does not implement. 39 unit tests, no hardware needed.

### `tools/bench/`

`dt_stress_test.py` - the P1 measurement: arms the on-board probe, applies a
chosen load, reads the frozen result.

`p2_acceptance.py` - 22 interactive checks on real hardware, including booting
the board while shaking it, which is the case that motivated the calibration
change.

---

## The rest of the system, which is not on this branch

The controller is one part of a larger system. The others live on `v1.5.2`:

| component | where | what it is |
|:----------|:------|:-----------|
| **Web server / dashboard** | `v1.5.2:server/web_ui` | Node.js dashboard: board list, live status, users, per-board telemetry keys |
| **MQTT server** | `v1.5.2:server/mqtt_server` | Mosquitto config, bridge and subscriber for the local-broker path |
| **Packaged GUI** | `v1.5.2:dist/` | Built technician GUI for machines without Python |

```sh
git worktree add ../GG-Prodino-v1.5.2 v1.5.2      # to look at them
```

**They are deliberately absent here, not removed.** The V1.5.1 build Ron
delivered is a standalone PlatformIO project with no server component at all,
and this branch is that build plus P2. More importantly the v1.5.2 dashboard
expects the encrypted-telemetry firmware: boards authenticate with a per-board
key and the server rejects anything it cannot verify. V1.5.1 has no such path -
its only external interface is the local JSON API - so the dashboard has nothing
to talk to and the technician GUI's telemetry-key controls were removed here for
the same reason.

If you have a `server/` directory in your working tree, it is the ignored
remains of an earlier `v1.5.2` checkout - `node_modules`, certificates, `.env`.
Git removed the tracked files when you switched branches and left those behind.
**It contains private keys**; treat it accordingly.

---

## Branches

| branch | what it is |
|:-------|:-----------|
| `v1.5.1` | Ron's V1.5.1 build imported verbatim - the reference to diff against |
| **`v1.5.1.1`** | **this one:** V1.5.1 + Priority 2 |
| `perf-experiments` | GPS, HTTP and I2C performance work, with the measurements. Not reviewed |
| `v1.5.2` | the wider system: encrypted telemetry, web dashboard, MQTT |

---

## Bench notes worth knowing

**A USB flash erases the whole chip.** `bossac --erase` takes the stored
configuration with it, including the zero calibration - re-burn from the GUI
after any USB update. The gyro bias survives, because it re-learns itself at the
first rest.

**PlatformIO lives in `.venv/`.** There is an unrelated PyPI package called
`pio` that shadows the real one on `PATH`; use `source .venv/bin/activate` or
call `.venv/bin/pio` directly.

**Run `pio` from `firmware/`,** not from the repo root.
