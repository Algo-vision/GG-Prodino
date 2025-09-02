import requests
import time
import json
import enum

class LED_STATES(enum.Enum):
    OFF = "OFF"
    GREEN = "GREEN"
    RED = "RED"
    ORANGE = "ORANGE"

BASE_URL = "http://192.168.1.198/"


token = None
def login():
    url = f"{BASE_URL}"
    payload = {"type":"login","user": "admin", "pass": "1234"}
    response = requests.post(url, data=json.dumps(payload))
    assert response.status_code == 200 
    data = response.json()
    token = data.get("token")
    print(data)
    return token


def get_status(token):
    url = f"{BASE_URL}"
    payload = {"type":"get_status","token": token}
    response = requests.post(url, data=json.dumps(payload))
    assert response.status_code == 200 
    data = response.json()
    print(data)
    return data
def set_relay(token, relay_id, state):
    url = f"{BASE_URL}"
    payload = {"type":"set_relay","token": token, "relay_id": relay_id, "state": state}
    response = requests.post(url, data=json.dumps(payload)) 
    assert response.status_code == 200 
    data = response.json()
    print(data)
    return data
def set_led(token, color: LED_STATES):
    url = f"{BASE_URL}"
    payload = {"type":"set_io_led","token": token, "color": color.value}
    response = requests.post(url, data=json.dumps(payload)) 
    assert response.status_code == 200 
    data = response.json()
    print(data)
    return data

token = login()
status = get_status(token)

for i in range(20):
    status = get_status(token)
    time.sleep(0.5)
{
    "type": "login",
    "user":"<username>",
    "pass":"<password>"
}
{
    "type": "get_status",
    "token":"<token>"
}
{
    "type": "set_relay",
    "token":"<token>",
    "relay_id": "<id>",
    "state": "<ON|OFF>"
}
{
    "type": "set_io_led",
    "token":"<token>",
    "color": "<OFF|GREEN|RED|ORANGE>"
}
{
    "type": "set_internal_led",
    "token":"<token>",
    "state": "<ON|OFF>"
}
{
  "type": "status",                # always "status"
  "relays_status": [0,0,0,0],      # array of 0/1 (off/on) for each relay
  "imuX": 0.0,                     # float, IMU X axis
  "imuY": 0.0,                     # float, IMU Y axis
  "imuZ": 0.0,                     # float, IMU Z axis
  "gpsLat": 0.0,                   # float, GPS latitude
  "gpsLng": 0.0,                   # float, GPS longitude
  "gpsAlt": 0.0,                   # float, GPS altitude
  "ledInternal": "<True|False>",            # boolean, internal LED state
  "ledIo": "OFF",                  # string: "OFF", "GREEN", "RED", or "ORANGE"
  "gpsValid": "<True|False>",               # boolean, true if GPS fix is valid
  "button1": "<True|False>",             # boolean, true if button1 is pressed
"imuValid": "<True|False>"                 # boolean, true if IMU data is valid
}