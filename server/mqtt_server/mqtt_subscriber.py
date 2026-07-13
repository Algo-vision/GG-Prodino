#!/usr/bin/env python3
"""
MQTT Subscriber Server for Prodino IoT Device

This server subscribes to MQTT topics published by the Prodino device,
stores the latest sensor data, and exposes it via REST API and WebSocket.
"""

import json
import time
from datetime import datetime
from threading import Lock
import paho.mqtt.client as mqtt
from flask import Flask, jsonify, request
from flask_socketio import SocketIO, emit
from flask_cors import CORS

import config

# Initialize Flask app
app = Flask(__name__)
CORS(app)
socketio = SocketIO(app, cors_allowed_origins="*")

# Data storage (thread-safe)
data_lock = Lock()
latest_data = {
    "status": None,
    "gps": {"position": None, "velocity": None, "heading": None, "accuracy": None},
    "imu": {"accel": None, "gyro": None, "orientation": None},
    "relays": None,
    "leds": {"internal": None, "io": None},
    "sensors": {"optos": None, "button_tech": None},
    "validity": {"gps": None, "imu": None},
    "last_update": None,
    "connected": False
}

# Historical data
data_history = []

# MQTT Client
mqtt_client = None


# ============ MQTT Callbacks ============

def on_connect(client, userdata, flags, rc):
    """Callback when connected to MQTT broker"""
    if rc == 0:
        print(f"✓ Connected to MQTT broker at {config.MQTT_BROKER}:{config.MQTT_PORT}")
        
        # Subscribe to all prodino topics
        for topic in config.SUBSCRIBE_TOPICS:
            client.subscribe(topic)
            print(f"✓ Subscribed to topic: {topic}")
        
        with data_lock:
            latest_data["connected"] = True
            
    else:
        print(f"✗ Failed to connect to MQTT broker. Return code: {rc}")
        error_messages = {
            1: "Incorrect protocol version",
            2: "Invalid client identifier",
            3: "Server unavailable",
            4: "Bad username or password",
            5: "Not authorized"
        }
        print(f"  Error: {error_messages.get(rc, 'Unknown error')}")


def on_disconnect(client, userdata, rc):
    """Callback when disconnected from MQTT broker"""
    print(f"✗ Disconnected from MQTT broker (rc={rc})")
    with data_lock:
        latest_data["connected"] = False
    
    if rc != 0:
        print("  Unexpected disconnection. Will attempt to reconnect...")


def on_message(client, userdata, msg):
    """Callback when a message is received"""
    topic = msg.topic
    payload = msg.payload.decode('utf-8')
    
    try:
        # Try to parse as JSON
        data = json.loads(payload)
    except json.JSONDecodeError:
        # If not JSON, store as string
        data = payload
    
    print(f"← {topic}: {payload[:100]}...")  # Print first 100 chars
    
    # Update data storage based on topic
    with data_lock:
        latest_data["last_update"] = datetime.now().isoformat()
        
        if topic == "prodino/status":
            latest_data["status"] = data
            
        elif topic == "prodino/gps/position":
            latest_data["gps"]["position"] = data
            
        elif topic == "prodino/gps/velocity":
            latest_data["gps"]["velocity"] = data
            
        elif topic == "prodino/gps/heading":
            latest_data["gps"]["heading"] = data
            
        elif topic == "prodino/imu/accel":
            latest_data["imu"]["accel"] = data
            
        elif topic == "prodino/imu/gyro":
            latest_data["imu"]["gyro"] = data
            
        elif topic == "prodino/imu/orientation":
            latest_data["imu"]["orientation"] = data
            
        elif topic == "prodino/relays/state":
            latest_data["relays"] = data
            
        elif topic == "prodino/leds/internal":
            latest_data["leds"]["internal"] = data
            
        elif topic == "prodino/leds/io":
            latest_data["leds"]["io"] = data
            
        elif topic == "prodino/sensors/optos":
            latest_data["sensors"]["optos"] = data
            
        elif topic == "prodino/sensors/button_tech":
            latest_data["sensors"]["button_tech"] = data
            
        elif topic == "prodino/validity/gps":
            latest_data["validity"]["gps"] = data
            
        elif topic == "prodino/validity/imu":
            latest_data["validity"]["imu"] = data
        
        elif topic == "prodino/gps/accuracy":
            latest_data["gps"]["accuracy"] = data
        
        # Add to history
        if len(data_history) >= config.MAX_HISTORY_SIZE:
            data_history.pop(0)
        data_history.append({
            "timestamp": latest_data["last_update"],
            "topic": topic,
            "data": data
        })
    
    # Broadcast update to WebSocket clients
    socketio.emit('prodino_update', {
        "topic": topic,
        "data": data,
        "timestamp": latest_data["last_update"]
    })


# ============ Web Routes ============

@app.route('/')
def index():
    """Serve the dashboard"""
    return app.send_static_file('index.html')


# ============ REST API Endpoints ============

@app.route('/api/status', methods=['GET'])
def get_status():
    """Get complete status from all topics"""
    with data_lock:
        return jsonify(latest_data)


@app.route('/api/gps', methods=['GET'])
def get_gps():
    """Get GPS data only"""
    with data_lock:
        return jsonify({
            "gps": latest_data["gps"],
            "validity": latest_data["validity"]["gps"],
            "last_update": latest_data["last_update"]
        })


@app.route('/api/imu', methods=['GET'])
def get_imu():
    """Get IMU data only"""
    with data_lock:
        return jsonify({
            "imu": latest_data["imu"],
            "validity": latest_data["validity"]["imu"],
            "last_update": latest_data["last_update"]
        })


@app.route('/api/relays', methods=['GET'])
def get_relays():
    """Get relay states"""
    with data_lock:
        return jsonify({
            "relays": latest_data["relays"],
            "last_update": latest_data["last_update"]
        })


@app.route('/api/leds', methods=['GET'])
def get_leds():
    """Get LED states"""
    with data_lock:
        return jsonify({
            "leds": latest_data["leds"],
            "last_update": latest_data["last_update"]
        })


@app.route('/api/history', methods=['GET'])
def get_history():
    """Get historical data"""
    limit = request.args.get('limit', 100, type=int)
    with data_lock:
        return jsonify({
            "history": data_history[-limit:],
            "count": len(data_history)
        })


@app.route('/api/health', methods=['GET'])
def health_check():
    """Health check endpoint"""
    with data_lock:
        return jsonify({
            "status": "healthy",
            "mqtt_connected": latest_data["connected"],
            "last_update": latest_data["last_update"]
        })


# ============ WebSocket Events ============

@socketio.on('connect')
def handle_connect():
    """Handle WebSocket connection"""
    print(f"✓ WebSocket client connected")
    with data_lock:
        emit('initial_data', latest_data)


@socketio.on('disconnect')
def handle_disconnect():
    """Handle WebSocket disconnection"""
    print(f"✗ WebSocket client disconnected")


# ============ MQTT Setup ============

def setup_mqtt():
    """Initialize and connect MQTT client"""
    global mqtt_client
    
    mqtt_client = mqtt.Client(client_id=config.MQTT_CLIENT_ID)
    
    # Set callbacks
    mqtt_client.on_connect = on_connect
    mqtt_client.on_disconnect = on_disconnect
    mqtt_client.on_message = on_message
    
    # Set authentication if configured
    if config.MQTT_USERNAME and config.MQTT_PASSWORD:
        mqtt_client.username_pw_set(config.MQTT_USERNAME, config.MQTT_PASSWORD)
    
    # Connect to broker
    try:
        print(f"Connecting to MQTT broker at {config.MQTT_BROKER}:{config.MQTT_PORT}...")
        mqtt_client.connect(config.MQTT_BROKER, config.MQTT_PORT, config.MQTT_KEEPALIVE)
        mqtt_client.loop_start()  # Start network loop in background thread
        return True
    except Exception as e:
        print(f"✗ Failed to connect to MQTT broker: {e}")
        return False


# ============ Main ============

if __name__ == '__main__':
    print("=" * 60)
    print("Prodino MQTT Subscriber Server")
    print("=" * 60)
    
    # Setup MQTT
    if not setup_mqtt():
        print("\n⚠ Warning: MQTT broker connection failed. Server will start but won't receive data.")
        print("  Make sure Mosquitto is running: systemctl status mosquitto")
    
    # Start Flask server
    print(f"\nStarting Flask server on {config.WEB_SERVER_HOST}:{config.WEB_SERVER_PORT}...")
    print(f"API Base URL: http://localhost:{config.WEB_SERVER_PORT}/api")
    print("=" * 60)
    print("\nAPI Endpoints:")
    print(f"  GET /api/status   - Complete status")
    print(f"  GET /api/gps      - GPS data")
    print(f"  GET /api/imu      - IMU data")
    print(f"  GET /api/relays   - Relay states")
    print(f"  GET /api/leds     - LED states")
    print(f"  GET /api/history  - Historical data")
    print(f"  GET /api/health   - Health check")
    print(f"\nWebSocket: ws://localhost:{config.WEB_SERVER_PORT}")
    print("=" * 60)
    print("\nPress Ctrl+C to stop\n")
    
    socketio.run(
        app,
        host=config.WEB_SERVER_HOST,
        port=config.WEB_SERVER_PORT,
        debug=config.DEBUG,
        allow_unsafe_werkzeug=True
    )
