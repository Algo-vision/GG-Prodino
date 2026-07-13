# MQTT Server for GRK IoT Device

This directory contains the Python MQTT subscriber server that receives data from the GRK device via MQTT and exposes it through a REST API and WebSocket.

## Quick Start

### 1. Install Dependencies

```bash
cd mqtt_server
pip install -r requirements.txt
```

### 2. Start Mosquitto Broker (if not running)

```bash
# Check if running
systemctl status mosquitto

# If not running, start it
sudo systemctl start mosquitto

# Enable auto-start on boot
sudo systemctl enable mosquitto
```

### 3. Run the MQTT Subscriber Server

```bash
python mqtt_subscriber.py
```

The server will start on `http://localhost:5000`

## Testing Without GRK

### Test with mosquitto_pub

```bash
# Publish test GPS data
mosquitto_pub -h localhost -t "grk/gps/position" \
  -m '{"lat": 32.0853, "lng": 34.7818, "alt": 150.2}'

# Publish test IMU data
mosquitto_pub -h localhost -t "grk/imu/accel" \
  -m '{"x": 0.01, "y": -0.02, "z": 0.98}'

# Publish test status
mosquitto_pub -h localhost -t "grk/status" \
  -m '{"firmwareVersion": "1.4", "gpsLat": 32.0853, "gpsLng": 34.7818}'
```

### Test API Endpoints

```bash
# Get complete status
curl http://localhost:5000/api/status

# Get GPS data
curl http://localhost:5000/api/gps

# Get health check
curl http://localhost:5000/api/health
```

## API Documentation

### REST Endpoints

| Endpoint | Method | Description |
|----------|--------|-------------|
| `/api/status` | GET | Get complete status from all topics |
| `/api/gps` | GET | Get GPS data only |
| `/api/imu` | GET | Get IMU data only |
| `/api/relays` | GET | Get relay states |
| `/api/leds` | GET | Get LED states |
| `/api/history` | GET | Get historical data (query param: `limit`) |
| `/api/health` | GET | Health check endpoint |

### WebSocket

Connect to `ws://localhost:5000` to receive real-time updates.

**Events:**
- `connect` - Client connected
- `initial_data` - Receive current data on connection
- `grk_update` - Receive updates when new MQTT messages arrive

**Example (JavaScript):**
```javascript
const socket = io('http://localhost:5000');

socket.on('connect', () => {
    console.log('Connected to server');
});

socket.on('initial_data', (data) => {
    console.log('Initial data:', data);
});

socket.on('grk_update', (update) => {
    console.log('Update:', update.topic, update.data);
});
```

## Configuration

Edit `config.py` to change settings:

- `MQTT_BROKER`: MQTT broker address (default: "localhost")
- `MQTT_PORT`: MQTT broker port (default: 1883)
- `MQTT_USERNAME`: Username for authentication (None = anonymous)
- `MQTT_PASSWORD`: Password for authentication
- `WEB_SERVER_PORT`: Flask server port (default: 5000)

## MQTT Topic Structure

The server subscribes to `grk/#` which includes:

```
grk/
├── status              # Complete status JSON
├── gps/
│   ├── position       # {lat, lng, alt}
│   ├── velocity       # {north, east, down, ground}
│   └── heading        # degrees
├── imu/
│   ├── accel          # {x, y, z}
│   ├── gyro           # {gx, gy, gz}
│   └── orientation    # {pitch, roll, yaw}
├── relays/
│   └── state          # [true, false, false, true]
├── leds/
│   ├── internal       # true/false
│   └── io             # "OFF", "GREEN", "RED", "ORANGE"
├── sensors/
│   ├── optos          # [true, true, false, false]
│   └── button_tech    # true/false
└── validity/
    ├── gps            # {valid, connected}
    └── imu            # true
```

## Troubleshooting

### MQTT Connection Failed

1. **Check if Mosquitto is running:**
   ```bash
   systemctl status mosquitto
   ```

2. **Check Mosquitto logs:**
   ```bash
   sudo tail -f /var/log/mosquitto/mosquitto.log
   ```

3. **Test MQTT connection manually:**
   ```bash
   mosquitto_sub -h localhost -t "grk/#" -v
   ```

### Port Already in Use

If port 5000 is already in use, change `WEB_SERVER_PORT` in `config.py`.

### Authentication Errors

If you enable authentication in Mosquitto:
1. Update `config.py` with username/password
2. Make sure password file exists in Mosquitto config
3. Restart Mosquitto: `sudo systemctl restart mosquitto`

## Next Steps

Once this server is running and tested:
1. Update GRK firmware to publish MQTT messages
2. Connect GRK to same network as this server
3. Verify data flow: GRK → Mosquitto → Python Server
4. Build web app to consume the REST API/WebSocket
