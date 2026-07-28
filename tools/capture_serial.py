#!/usr/bin/env python3
"""
Capture the board's serial output to stdout for a fixed number of seconds.

`pio device monitor` needs a real TTY, so it can't be scripted or redirected to
a file. This does the same job non-interactively - used to record the benchmark
logs from bench_aead / bench_tls.

Usage:  python3 tools/capture_serial.py [SECONDS] [PORT] [BAUD]
"""
import sys, time

try:
    import serial
except ImportError:
    sys.exit("need pyserial: ~/.platformio/penv/bin/python -m pip install pyserial")

seconds = int(sys.argv[1]) if len(sys.argv) > 1 else 60
port = sys.argv[2] if len(sys.argv) > 2 else "/dev/ttyACM1"
baud = int(sys.argv[3]) if len(sys.argv) > 3 else 115200

with serial.Serial(port, baud, timeout=1) as ser:
    end = time.time() + seconds
    while time.time() < end:
        line = ser.readline()
        if line:
            sys.stdout.write(line.decode("utf-8", "replace"))
            sys.stdout.flush()
