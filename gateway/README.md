# Gateway (in-HLC edge device: Jetson / RPi CM4)

Deployable artifacts for the local gateway that sits between the GRK board and
AWS IoT (Option B). The gateway runs Mosquitto with a TLS bridge to AWS and
forwards the board's plain local MQTT to the cloud. The board holds the per-unit
identity (AWS certs + serial); the gateway fetches it, so this OS image is generic.

The end-to-end data path (board → gateway → AWS → web) is in
[`../docs/BOARD_TO_WEBSERVER_DATAFLOW.md`](../docs/BOARD_TO_WEBSERVER_DATAFLOW.md).

## Contents

| Path | What it is |
|---|---|
| `bin/grk-fetch-certs.py` | Fetches the per-unit identity FROM the board over HTTP: AWS cert/key/ca + endpoint (→ `/etc/mosquitto/certs/`), the HLC serial (→ `/etc/grk/hlc_serial`), and stamps `remote_clientid grk-bridge-<serial>` into the bridge config. Runs before Mosquitto on boot. |
| `bin/grk-jetson-temp.sh` | Publishes gateway CPU temp to `grk/<serial>/jetson/cpu_temp` every 5 s (serial from `/etc/grk/hlc_serial`); the bridge forwards it to AWS. |
| `mosquitto/grk.conf` | Mosquitto config: local `:1883` listener for the board + TLS bridge to AWS IoT (`grk/# out`). `remote_clientid` is auto-stamped per unit. → `/etc/mosquitto/conf.d/` |
| `systemd/grk-fetch-certs.service` | oneshot, `Before=mosquitto` — runs the fetch on boot. → `/etc/systemd/system/` |
| `systemd/grk-jetson-temp.service` | CPU-temp publisher unit (`Restart=always`). → `/etc/systemd/system/` |
| `systemd/mosquitto.service.d/10-grk.conf` | drop-in so Mosquitto waits for the fetch. → `/etc/systemd/system/mosquitto.service.d/` |

**Topic namespace:** the gateway publishes under the board's HLC serial so a
"system" (board + gateway) shares one tree — `grk/<serial>/jetson/cpu_temp` sits
alongside `grk/<serial>/status`, `grk/<serial>/imu/...`, etc.

**Shared cert, whole fleet:** one AWS device cert is baked into every board and
served to its gateway; uniqueness at AWS comes from the per-unit
`remote_clientid grk-bridge-<serial>` (AWS IoT drops duplicate client ids). The
end customer never touches AWS.

## Full install (fresh gateway)

```bash
# scripts
sudo install -m 755 bin/grk-fetch-certs.py  /usr/local/bin/grk-fetch-certs.py
sudo install -m 755 bin/grk-jetson-temp.sh  /usr/local/bin/grk-jetson-temp.sh

# mosquitto broker + bridge
sudo apt install -y mosquitto mosquitto-clients
sudo cp mosquitto/grk.conf /etc/mosquitto/conf.d/grk.conf

# systemd units
sudo cp systemd/grk-fetch-certs.service  /etc/systemd/system/
sudo cp systemd/grk-jetson-temp.service  /etc/systemd/system/
sudo mkdir -p /etc/systemd/system/mosquitto.service.d
sudo cp systemd/mosquitto.service.d/10-grk.conf /etc/systemd/system/mosquitto.service.d/
sudo systemctl daemon-reload
sudo systemctl enable grk-fetch-certs.service grk-jetson-temp.service mosquitto

# first run (or reboot): fetch identity from the board, then start the bridge
sudo /usr/local/bin/grk-fetch-certs.py
sudo systemctl restart mosquitto
sudo systemctl start grk-jetson-temp.service
```

Set `GRK_BOARD_IP` (default `192.168.1.198`) as an env var if the board differs.

## Verify

```bash
systemctl is-active mosquitto grk-jetson-temp
grep remote_clientid /etc/mosquitto/conf.d/grk.conf     # grk-bridge-<serial>
mosquitto_sub -h localhost -p 1883 -t 'grk/#' -v        # board + jetson topics
```

## Notes

- Thermal zone: `/sys/class/thermal/thermal_zone0/temp` (cpu-thermal on Tegra).
  On other hardware confirm with
  `for z in /sys/class/thermal/thermal_zone*; do echo "$z $(cat $z/type)"; done`.
- The bridge certs come from the board via `grk-fetch-certs`; they are never
  stored in this repo.
