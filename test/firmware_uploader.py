import base64
import requests
import os

def upload_firmware(ip, firmware_path, password=""):
    url = f"http://{ip}:65280/sketch"
    size = os.path.getsize(firmware_path)
    headers = {
        "Content-Type": "application/octet-stream",
        "Content-Length": str(size),
        "Authorization": f"Basic {base64.b64encode(f'arduino:{password}'.encode()).decode()}"
    }
    with open(firmware_path, "rb") as f:
        try:
            response = requests.post(url, headers=headers, data=f, timeout=60)
            return response.status_code == 200, response.text
        except Exception as e:
            return False, str(e)
