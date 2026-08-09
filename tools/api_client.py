import requests
import json

class ApiClient:
    # Firmware 1.5.1 changed get_status from one flat object into sections
    # (config / overview / imu / gps), and renamed two fields. Everything that
    # reads a status document expects the flat shape, so normalise here rather
    # than in every caller - and keep working with older firmware, which really
    # is flat.
    _SECTIONS = ("config", "overview", "imu", "gps")
    _RENAMED = (("busVoltage", "systemVoltage"), ("GPSConnected", "gpsConnected"))

    @classmethod
    def _flatten(cls, doc):
        if not isinstance(doc, dict):
            return doc
        flat = dict(doc)
        for section in cls._SECTIONS:
            sub = doc.get(section)
            if isinstance(sub, dict):
                for k, v in sub.items():
                    flat.setdefault(k, v)      # a top-level key always wins
        for old, new in cls._RENAMED:
            if old not in flat and new in flat:
                flat[old] = flat[new]
        return flat

    def __init__(self, base_ip):
        self.base_ip = base_ip
        self.base_url = f"http://{base_ip}/"
        self.token = None
        # Store credentials for auto re-login
        self._username = None
        self._password = None
        # Use session for connection pooling (reuses TCP connections)
        self.session = requests.Session()

    def login(self, username, password):
        # Store credentials for auto re-login
        self._username = username
        self._password = password
        
        payload = {"type": "login", "user": username, "pass": password}
        print(f"ApiClient.login: Sending payload: {json.dumps(payload)} to {self.base_url}")
        try:
            response = self.session.post(self.base_url, data=json.dumps(payload), timeout=5)
            print(f"ApiClient.login: Received response status: {response.status_code}")
            if response.status_code == 200:
                data = response.json()
                print(f"ApiClient.login: Received response JSON: {data}")
                if data.get("success"):
                    self.token = data.get("token")
                    return True, data.get("token")
                else:
                    # Assuming the server sends a 'message' on failed login
                    return False, data.get("message", "Invalid credentials")
            else:
                return False, f"HTTP Error: {response.status_code}"
        except requests.exceptions.ConnectionError:
            return False, "Connection Error"
        except requests.exceptions.Timeout:
            return False, "Connection Timeout"
        except Exception as e:
            return False, f"An unexpected error occurred: {e}"

    def _try_relogin(self):
        """Attempt to re-login using stored credentials after a 401 error"""
        if self._username and self._password:
            print("[ApiClient] Token expired, attempting auto re-login...")
            success, _ = self.login(self._username, self._password)
            return success
        return False

    def get_status(self):
        payload = {"type": "get_status", "token": self.token}

        try:
            response = self.session.post(self.base_url, data=json.dumps(payload), timeout=5)

            if response.status_code == 200:
                return self._flatten(response.json())
            if response.status_code == 401:
                # Try to re-login automatically
                if self._try_relogin():
                    # Retry the request with new token
                    payload["token"] = self.token
                    response = self.session.post(self.base_url, data=json.dumps(payload), timeout=5)
                    if response.status_code == 200:
                        print("[ApiClient] Auto re-login successful, resumed operation")
                        return self._flatten(response.json())
                return {"error": "AUTH_ERROR"}
            return None
        except (requests.exceptions.ConnectionError, requests.exceptions.Timeout) as e:
            return None # Indicate communication loss
        except Exception as e:
            print(f"Error getting status: {e}")
            return None

    def set_relay(self, relay_id, state):
        payload = {"type": "set_relay", "token": self.token, "relay_id": relay_id, "state": state}
        try:
            response = self.session.post(self.base_url, data=json.dumps(payload), timeout=5)
            if response.status_code == 200:
                return response.json()
            if response.status_code == 401:
                return {"error": "AUTH_ERROR"}
            return None
        except (requests.exceptions.ConnectionError, requests.exceptions.Timeout):
            return None
        except Exception as e:
            print(f"Error setting relay: {e}")
            return None

    def set_led(self, color):
        payload = {"type": "set_io_led", "token": self.token, "color": color}
        try:
            response = self.session.post(self.base_url, data=json.dumps(payload), timeout=5)
            if response.status_code == 200:
                return response.json()
            if response.status_code == 401:
                return {"error": "AUTH_ERROR"}
            return None
        except (requests.exceptions.ConnectionError, requests.exceptions.Timeout):
            return None
        except Exception as e:
            print(f"Error setting LED: {e}")
            return None

    def set_internal_led(self, state):
        payload = {"type": "set_internal_led", "token": self.token, "state": state}
        try:
            response = self.session.post(self.base_url, data=json.dumps(payload), timeout=5)
            if response.status_code == 200:
                return response.json()
            if response.status_code == 401:
                return {"error": "AUTH_ERROR"}
            return None
        except (requests.exceptions.ConnectionError, requests.exceptions.Timeout):
            return None
        except Exception as e:
            print(f"Error setting internal LED: {e}")
            return None

    def set_ip_config(self, controller_ip, whitelist_ips):
        print(f"ApiClient.set_ip_config: Sending request to {self.base_url}")
        payload = {"type": "set_ip_config", "token": self.token, "controller_ip": controller_ip, "whitelist_ips": whitelist_ips}
        try:
            response = self.session.post(self.base_url, data=json.dumps(payload), timeout=5)
            print(f"ApiClient.set_ip_config: Received response with status code {response.status_code}")
            if response.status_code == 200:
                data = response.json()
                print(f"ApiClient.set_ip_config: Response JSON: {data}")
                if data.get("success"):
                    self.base_ip = controller_ip # Update base_ip if successful
                    self.base_url = f"http://{controller_ip}/"
                    print(f"ApiClient.set_ip_config: IP configuration updated successfully on board. New base_ip: {self.base_ip}")
                    return True, "IP configuration updated successfully."
                else:
                    print(f"ApiClient.set_ip_config: Server reported error: {data.get('message', 'Unknown error')}")
                    return False, data.get("message", "Unknown error")
            elif response.status_code == 401:
                return False, "Authentication Error"
            else:
                print(f"ApiClient.set_ip_config: HTTP Error: {response.status_code}")
                return False, f"HTTP Error: {response.status_code}"
        except requests.exceptions.ConnectionError:
            print("ApiClient.set_ip_config: Connection Error")
            return False, "Connection Error"
        except requests.exceptions.Timeout:
            print("ApiClient.set_ip_config: Connection Timeout")
            return False, "Connection Timeout"
        except Exception as e:
            print(f"ApiClient.set_ip_config: An unexpected error occurred: {e}")
            return False, f"An unexpected error occurred: {e}"

    def set_serial_number(self, serial_number):
        payload = {"type": "set_serial_number", "token": self.token, "serial_number": str(serial_number)}
        try:
            response = self.session.post(self.base_url, data=json.dumps(payload), timeout=5)
            if response.status_code == 200:
                return response.json()
            if response.status_code == 401:
                return {"error": "AUTH_ERROR"}
            if response.status_code == 403:
                return {"error": "FORBIDDEN", "message": "Technician mode required"}
            return None
        except (requests.exceptions.ConnectionError, requests.exceptions.Timeout):
            return None
        except Exception as e:
            print(f"Error setting serial number: {e}")
            return None

    def set_device_key(self, key_hex):
        """Burn the per-board telemetry key (64 hex chars) into the board.

        Write-only by design: nothing ever reads a key back out of a board, so
        there is deliberately no get_device_key. Whether one is present shows up
        as deviceKeySet in get_config().
        """
        payload = {"type": "set_device_key", "token": self.token, "key": str(key_hex)}
        try:
            response = self.session.post(self.base_url, data=json.dumps(payload), timeout=8)
            if response.status_code == 200:
                return response.json()
            if response.status_code == 401:
                return {"error": "AUTH_ERROR"}
            if response.status_code == 403:
                return {"error": "FORBIDDEN", "message": "Not permitted"}
            return None
        except (requests.exceptions.ConnectionError, requests.exceptions.Timeout):
            return None
        except Exception as e:
            print(f"Error setting device key: {e}")
            return None

    def get_serial_number(self):
        payload = {"type": "get_serial_number", "token": self.token}
        try:
            response = self.session.post(self.base_url, data=json.dumps(payload), timeout=5)
            if response.status_code == 200:
                return response.json()
            if response.status_code == 401:
                return {"error": "AUTH_ERROR"}
            return None
        except (requests.exceptions.ConnectionError, requests.exceptions.Timeout):
            return None
        except Exception as e:
            print(f"Error getting serial number: {e}")
            return None

    def set_router_ip(self, router_ip):
        payload = {"type": "set_router_ip", "token": self.token, "router_ip": router_ip}
        try:
            response = self.session.post(self.base_url, data=json.dumps(payload), timeout=5)
            if response.status_code == 200:
                data = response.json()
                if data.get("success"):
                    return True, data.get("message", "Router IP updated")
                else:
                    return False, data.get("message", "Unknown error")
            if response.status_code == 401:
                return False, "Authentication Error"
            return False, f"HTTP Error: {response.status_code}"
        except (requests.exceptions.ConnectionError, requests.exceptions.Timeout):
            return False, "Connection Error"
        except Exception as e:
            print(f"Error setting router IP: {e}")
            return False, str(e)

    def get_router_ip(self):
        payload = {"type": "get_router_ip", "token": self.token}
        try:
            response = self.session.post(self.base_url, data=json.dumps(payload), timeout=5)
            if response.status_code == 200:
                return response.json()
            if response.status_code == 401:
                return {"error": "AUTH_ERROR"}
            return None
        except (requests.exceptions.ConnectionError, requests.exceptions.Timeout):
            return None
        except Exception as e:
            print(f"Error getting router IP: {e}")
            return None

    def set_imu_mount_orientation(self, orientation):
        payload = {"type": "set_imu_mount_orientation", "token": self.token, "orientation": orientation}
        try:
            response = self.session.post(self.base_url, data=json.dumps(payload), timeout=5)
            if response.status_code == 200:
                data = response.json()
                if data.get("success"):
                    return True, data.get("message", "IMU mount orientation updated")
                else:
                    return False, data.get("message", "Unknown error")
            if response.status_code == 401:
                return False, "Authentication Error"
            return False, f"HTTP Error: {response.status_code}"
        except (requests.exceptions.ConnectionError, requests.exceptions.Timeout):
            return False, "Connection Error"
        except Exception as e:
            print(f"Error setting IMU mount orientation: {e}")
            return False, str(e)

    def get_config(self):
        payload = {"type": "get_config", "token": self.token}
        try:
            response = self.session.post(self.base_url, data=json.dumps(payload), timeout=5)
            if response.status_code == 200:
                return self._flatten(response.json())
            if response.status_code == 401:
                return {"error": "AUTH_ERROR"}
            return None
        except (requests.exceptions.ConnectionError, requests.exceptions.Timeout):
            return None
        except Exception as e:
            print(f"Error getting config: {e}")
            return None

    def get_imu(self):
        payload = {"type": "get_imu", "token": self.token}
        try:
            response = self.session.post(self.base_url, data=json.dumps(payload), timeout=5)
            if response.status_code == 200:
                return self._flatten(response.json())
            if response.status_code == 401:
                return {"error": "AUTH_ERROR"}
            return None
        except (requests.exceptions.ConnectionError, requests.exceptions.Timeout):
            return None
        except Exception as e:
            print(f"Error getting IMU data: {e}")
            return None
