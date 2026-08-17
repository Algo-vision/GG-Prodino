# ota_uploader.py

import sys
import os
import base64
import requests

def upload(ip, firmware_path, password="arduino"):
    url = f"http://{ip}:65280/sketch"
    size = os.path.getsize(firmware_path)
    print(f"Uploading to {url}, firmware size={size}")

   
    headers = {
        "Content-Type": "application/octet-stream",
        "Content-Length": str(size)
    }

    with open(firmware_path, "rb") as f:
        try:
            response = requests.post(url, headers=headers, data=f, timeout=60)
            print(f"HTTP {response.status_code}: {response.text}")
            if response.status_code == 200:
                print("Upload complete!")
            else:
                print("Upload failed!")
        except Exception as e:
            print(f"Error: {e}")

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: ota_uploader.py <firmware.bin> [password] [ip]")
        sys.exit(1)

    firmware = sys.argv[1]
    password = sys.argv[2] if len(sys.argv) > 2 else "arduino"
    ip = sys.argv[3] if len(sys.argv) > 3 else os.getenv("UPLOAD_PORT", "192.168.1.198")

    upload(ip, firmware, password)
