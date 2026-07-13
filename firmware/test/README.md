# Firmware Native Tests

Unit tests for the hardware-free pure logic in this firmware, run natively on
the host (no ARM cross-compile, no real I2C/EEPROM/UDP/HTTP hardware).

## Running

```sh
cd firmware
pio test -e native
```

Plain `pio run` (no `-e` flag) only builds the real embedded firmware
(`env:main`) - the `native` environment is test-only.

## What's covered

- `test_calculations/` - dual-IMU orientation fusion math (`calculations.cpp`)
- `test_imu_mount_orientation/` - mount-orientation axis remap + string
  conversion (`imu_mount_orientation.cpp`)
- `test_sanity_check/` - the shared "not all zero / not stuck" check
  (`sanity_check.hpp`)
- `test_ocu_state/` - OCU heartbeat connected/disconnected state transitions
  (`ocu_connection_state.hpp`)

Each test file includes the real production source directly (e.g.
`#include "../../lib/GG/src/calculations.cpp"`) rather than linking through
PlatformIO's library resolution, so it always exercises the actual code, not
a copy - while staying free of any Arduino/hardware dependency.

## What's NOT covered (and why)

This is embedded Arduino firmware - most of it is hardware I/O, not testable
without either real hardware or a much larger mocking investment:

- I2C register reads/writes (`readAccelerometer`, `readGyroscope`, `initIMU`, etc.)
- EEPROM/FlashStorage persistence (`config_manager`'s save/load)
- Actual UDP send/receive (the I/O half of `ocu_monitor.cpp`)
- HTTP request routing/dispatch (`http_server.cpp`'s `httpServerLoop`)

These need validation on real hardware.
