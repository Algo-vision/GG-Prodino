import requests
import json

class ApiClient:
    def __init__(self, base_url):
        self.base_url = base_url
        self.token = None

    def login(self, username, password):
        payload = {"type": "login", "user": username, "pass": password}
        response = requests.post(self.base_url, data=json.dumps(payload))
        if response.status_code == 200:
            data = response.json()
            if data.get("success"):
                self.token = data.get("token")
                return True, data.get("token")
            else:
                return False, data.get("token")
        return False, None

    def get_status(self):
        payload = {"type": "get_status", "token": self.token}
        response = requests.post(self.base_url, data=json.dumps(payload))
        if response.status_code == 200:
            return response.json()
        return None

    def set_relay(self, relay_id, state):
        payload = {"type": "set_relay", "token": self.token, "relay_id": relay_id, "state": state}
        response = requests.post(self.base_url, data=json.dumps(payload))
        return response.json() if response.status_code == 200 else None

    def set_led(self, color):
        payload = {"type": "set_io_led", "token": self.token, "color": color}
        response = requests.post(self.base_url, data=json.dumps(payload))
        return response.json() if response.status_code == 200 else None

    def set_internal_led(self, state):
        payload = {"type": "set_internal_led", "token": self.token, "state": state}
        response = requests.post(self.base_url, data=json.dumps(payload))
        return response.json() if response.status_code == 200 else None
