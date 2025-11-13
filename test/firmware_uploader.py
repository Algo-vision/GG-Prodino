import base64
import requests
import os

def upload_firmware(ip, firmware_path, password=""):
    print(f"upload_firmware: Starting firmware upload to {ip}")
    url = f"http://{ip}:65280/sketch"
    size = os.path.getsize(firmware_path)
    auth_str = f"Basic {base64.b64encode(f'arduino:{password}'.encode()).decode()}"
    headers = {
        "Content-Type": "application/octet-stream",
        "Content-Length": str(size),
        "Authorization": auth_str
    }
    print(f"upload_firmware: URL: {url}")
    print(f"upload_firmware: Headers: {headers}")
    print(f"upload_firmware: Firmware size: {size}")

    with open(firmware_path, "rb") as f:
        try:
            print("upload_firmware: Sending POST request...")
            response = requests.post(url, headers=headers, data=f, timeout=60)
            print(f"upload_firmware: Response status code: {response.status_code}")
            print(f"upload_firmware: Response text: {response.text}")
            return response.status_code == 200, response.text
        except Exception as e:
            print(f"upload_firmware: An exception occurred: {e}")
            return False, str(e)
