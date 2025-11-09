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
    "type": "set_ip_config",
    "token":"<token>",
    "controller_ip": "192.168.1.XXX",
    "whitelist_ips": ["192.168.1.YYY", "192.168.1.ZZZ"]
}
{
  "type": "status",                   # always "status"
  "firmwareVersion": "1.0.0",         # Firmware version
  "relays_status": [0,0,0,0],         # array of 0/1 (off/on) for each relay
  "optoin_status": [0,0,0,0],         # array of 0/1 (off/on) for each optocoupler input
  "imuX": 0.0,                        # float, IMU Linear acceleration X-axis (g)
  "imuY": 0.0,                        # float, IMU Linear acceleration Y-axis (g)
  "imuZ": 0.0,                        # float, IMU Linear acceleration Z-axis (g)
  "imuGx": 0.0,                       # float, IMU Angular velocity X-axis (º/s)
  "imuGy": 0.0,                       # float, IMU Angular velocity Y-axis (º/s)
  "imuGz": 0.0,                       # float, IMU Angular velocity Z-axis (º/s)
  "pitch": 0.0,                       # float, Calculated Pitch (º)
  "roll": 0.0,                        # float, Calculated Roll (º)
  "yaw": 0.0,                         # float, Calculated Yaw (º)
  "imuValid": "<True|False>",         # boolean, true if IMU data is valid
  "gpsLat": 0.0,                      # float, GPS Latitude (º)
  "gpsLng": 0.0,                      # float, GPS Longitude (º)
  "gpsAlt": 0.0,                      # float, GPS Altitude (meters)
  "gpsTime": "YYYY-MM-DD hh:mm:ss",   # string, GPS Time
  "gpsSpeedNorth": 0.0,               # float, Linear velocity North (km/hr)
  "gpsSpeedEast": 0.0,                # float, Linear velocity East (km/hr)
  "imuSpeedDown": 0.0,                # float, Linear velocity Down (km/hr) - IMU derived
  "gpsGroundSpeed": 0.0,              # float, Horizontal velocity (Ground speed) (km/hr)
  "gpsHeading": 0.0,                  # float, Absolute heading (º)
  "gpsValid": "<True|False>",         # boolean, true if GPS fix is valid
  "GPSConnected": "<True|False>",     # boolean, true if GPS module is communicating
  "ledInternal": "<True|False>",      # boolean, internal LED state
  "ledIo": "OFF",                     # string: "OFF", "GREEN", "RED", or "ORANGE"
  "button1": "<True|False>"           # boolean, true if button1 is pressed
}