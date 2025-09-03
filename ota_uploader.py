# ota_uploader.py
import socket
import sys
import os
import struct
import time

PORT = 65280   # ArduinoOTA port

def upload(ip, firmware_path):
    size = os.path.getsize(firmware_path)
    print(f"Connecting to {ip}:{PORT}, firmware size={size}")

    # open TCP socket
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.settimeout(10)
    s.connect((ip, PORT))

    # handshake: send size
    header = struct.pack("<II", 0, size)  # 2x uint32: flash addr=0, size
    s.sendall(header)

    # send binary in chunks
    with open(firmware_path, "rb") as f:
        sent = 0
        while True:
            chunk = f.read(1024)
            if not chunk:
                break
            s.sendall(chunk)
            sent += len(chunk)
            percent = int(sent * 100 / size)
            print(f"\rProgress: {percent}%", end="")
    print()

    s.close()
    print("Upload complete!")


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: ota_uploader.py <firmware.bin>")
        sys.exit(1)

    firmware = sys.argv[1]
    ip = os.getenv("UPLOAD_PORT", "192.168.1.198")

    upload(ip, firmware)
