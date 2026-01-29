import requests
import json
import time

class ApiClient:
    def __init__(self, base_ip):
        self.base_ip = base_ip
        self.base_url = f"http://{base_ip}/"
        self.token = None
        # Store credentials for auto re-login
        self._username = None
        self._password = None
        # Use session for connection pooling (reuses TCP connections)
        self.session = requests.Session()
        # Timing debug
        self.last_call_time = None

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
        
        # Timing debug - time since last call
        now = time.time()
        if self.last_call_time:
            time_since_last = (now - self.last_call_time) * 1000  # ms
            print(f"[TIMING] Time since last get_status call: {time_since_last:.1f}ms")
        self.last_call_time = now
        
        try:
            start_time = time.time()
            response = self.session.post(self.base_url, data=json.dumps(payload), timeout=5)
            end_time = time.time()
            request_duration = (end_time - start_time) * 1000  # ms
            print(f"[TIMING] get_status request took: {request_duration:.1f}ms (status: {response.status_code})")
            
            if response.status_code == 200:
                return response.json()
            if response.status_code == 401:
                # Try to re-login automatically
                if self._try_relogin():
                    # Retry the request with new token
                    payload["token"] = self.token
                    response = self.session.post(self.base_url, data=json.dumps(payload), timeout=5)
                    if response.status_code == 200:
                        print("[ApiClient] Auto re-login successful, resumed operation")
                        return response.json()
                return {"error": "AUTH_ERROR"}
            return None
        except (requests.exceptions.ConnectionError, requests.exceptions.Timeout) as e:
            print(f"[TIMING] get_status FAILED: {e}")
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
