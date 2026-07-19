#!/bin/bash
# GRK Jetson CPU-temp publisher. Publishes under the HLC serial namespace so the
# gateway's data sits alongside the board's:  grk/<serial>/jetson/cpu_temp
# (a "system" = board + Jetson, both keyed by the same HLC serial).
#
# The serial is fetched FROM the board by grk-fetch-certs and written to
# SERIAL_FILE. Re-read every loop so a re-burn (picked up on the next fetch) is
# reflected without restarting this service. Falls back to UNCONFIGURED.
SERIAL_FILE="/etc/grk/hlc_serial"
ZONE="/sys/class/thermal/thermal_zone0/temp"   # cpu-thermal
while true; do
  serial="$(cat "$SERIAL_FILE" 2>/dev/null)"; serial="${serial:-UNCONFIGURED}"
  topic="grk/${serial}/jetson/cpu_temp"
  raw="$(cat "$ZONE" 2>/dev/null || echo -1000)"
  c="$(awk -v r="$raw" 'BEGIN{printf "%.1f", r/1000}')"
  mosquitto_pub -h localhost -p 1883 -t "$topic" \
    -m "{\"cpu_temp\":$c,\"unit\":\"C\"}" 2>/dev/null
  sleep 5
done
