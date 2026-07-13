#!/usr/bin/env python3
"""
Quick test script for MQTT integration
Tests Mosquitto → Python Server data flow before GRK is ready
"""

import time
import subprocess
import json

print("=" * 60)
print("MQTT Integration Test Script")
print("=" * 60)

# Test topics to publish
test_data = {
    "grk/gps/position": json.dumps({"lat": 32.0853, "lng": 34.7818, "alt": 150.2}),
    "grk/gps/velocity": json.dumps({"north": 10.5, "east": 2.7, "down": 0.1, "ground": 10.8}),
    "grk/gps/heading": "75.3",
    "grk/imu/accel": json.dumps({"x": 0.01, "y": -0.02, "z": 0.98}),
    "grk/imu/gyro": json.dumps({"gx": 1.5, "gy": -0.8, "gz": 0.2}),
    "grk/imu/orientation": json.dumps({"pitch": 5.2, "roll": -3.1, "yaw": 45.7}),
    "grk/relays/state": json.dumps([False, False, False, False]),
    "grk/leds/internal": "true",
    "grk/leds/io": "GREEN",
    "grk/sensors/optos": json.dumps([False, False, False, False]),
    "grk/sensors/button_tech": "false",
    "grk/validity/gps": json.dumps({"valid": True, "connected": True}),
    "grk/validity/imu": "true"
}

print("\nPublishing test MQTT messages...")
print("Make sure Python MQTT server is running: python mqtt_subscriber.py\n")

for topic, message in test_data.items():
    print(f"Publishing to {topic}...")
    cmd = [
        "mosquitto_pub",
        "-h", "localhost",
        "-t", topic,
        "-m", message
    ]
    
    try:
        subprocess.run(cmd, check=True, capture_output=True)
        print(f"  ✓ Published: {message[:50]}...")
    except subprocess.CalledProcessError as e:
        print(f"  ✗ Failed: {e}")
    except FileNotFoundError:
        print("  ✗ Error: mosquitto_pub not found. Install with: sudo apt install mosquitto-clients")
        break
    
    time.sleep(0.2)

print("\n" + "=" * 60)
print("Test complete! Check Python server output for received messages.")
print("\nTo verify:")
print("1. Check Python server terminal for incoming messages")
print("2. Visit http://localhost:5000/api/status")
print("3. Check browser console for WebSocket updates")
print("=" * 60)
