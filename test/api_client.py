import requests
import json

class ApiClient:
    def __init__(self, base_ip):
        self.base_ip = base_ip
        self.base_url = f"http://{base_ip}/"
        self.token = None

    def login(self, username, password):
        payload = {"type": "login", "user": username, "pass": password}
        print(f"ApiClient.login: Sending payload: {json.dumps(payload)} to {self.base_url}") # Added print
        try:
            response = requests.post(self.base_url, data=json.dumps(payload), timeout=5) # Added timeout
            print(f"ApiClient.login: Received response status: {response.status_code}") # Added print
            if response.status_code == 200:
                data = response.json()
                print(f"ApiClient.login: Received response JSON: {data}") # Added print
                if data.get("success"):
                    self.token = data.get("token")
                    return True, data.get("token")
                else:
                    return False, data.get("token") # This will be an error message from the server
            else:
                return False, f"HTTP Error: {response.status_code}"
        except requests.exceptions.ConnectionError:
            return False, "Connection Error"
        except requests.exceptions.Timeout:
            return False, "Connection Timeout"
        except Exception as e:
            return False, f"An unexpected error occurred: {e}"

    def get_status(self):
        payload = {"type": "get_status", "token": self.token}
        try:
            response = requests.post(self.base_url, data=json.dumps(payload), timeout=5)
            if response.status_code == 200:
                return response.json()
            return None
        except (requests.exceptions.ConnectionError, requests.exceptions.Timeout):
            return None # Indicate communication loss
        except Exception as e:
            print(f"Error getting status: {e}")
            return None

    def set_relay(self, relay_id, state):
        payload = {"type": "set_relay", "token": self.token, "relay_id": relay_id, "state": state}
        try:
            response = requests.post(self.base_url, data=json.dumps(payload), timeout=5)
            return response.json() if response.status_code == 200 else None
        except (requests.exceptions.ConnectionError, requests.exceptions.Timeout):
            return None
        except Exception as e:
            print(f"Error setting relay: {e}")
            return None

    def set_led(self, color):
        payload = {"type": "set_io_led", "token": self.token, "color": color}
        try:
            response = requests.post(self.base_url, data=json.dumps(payload), timeout=5)
            return response.json() if response.status_code == 200 else None
        except (requests.exceptions.ConnectionError, requests.exceptions.Timeout):
            return None
        except Exception as e:
            print(f"Error setting LED: {e}")
            return None

    def set_internal_led(self, state):
        payload = {"type": "set_internal_led", "token": self.token, "state": state}
        try:
            response = requests.post(self.base_url, data=json.dumps(payload), timeout=5)
            return response.json() if response.status_code == 200 else None
        except (requests.exceptions.ConnectionError, requests.exceptions.Timeout):
            return None
        except Exception as e:
            print(f"Error setting internal LED: {e}")
            return None

    def set_ip_config(self, controller_ip, whitelist_ips):
        print(f"ApiClient.set_ip_config: Sending request to {self.base_url}")
        payload = {"type": "set_ip_config", "token": self.token, "controller_ip": controller_ip, "whitelist_ips": whitelist_ips}
        try:
            response = requests.post(self.base_url, data=json.dumps(payload), timeout=5)
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
