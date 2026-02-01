#!/usr/bin/env python3
"""
GG API Tester - Tests HTTP API with robust error handling
"""
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
TIMEOUT = 5  # seconds


def login(max_retries=3):
    """Login with retry logic"""
    url = f"{BASE_URL}"
    payload = {"type": "login", "user": "admin", "pass": "1234"}
    
    for attempt in range(max_retries):
        try:
            response = requests.post(url, data=json.dumps(payload), timeout=TIMEOUT)
            if response.status_code == 200:
                data = response.json()
                if data.get("success"):
                    print(f"✅ Login successful: {data.get('token')[:8]}...")
                    return data.get("token")
                else:
                    print(f"❌ Login failed: {data.get('message')}")
                    return None
            elif response.status_code == 403:
                print(f"❌ Login failed: IP not whitelisted (403)")
                return None
            else:
                print(f"⚠️  Login HTTP {response.status_code}")
        except requests.exceptions.ConnectionError:
            print(f"⚠️  Connection failed (attempt {attempt + 1}/{max_retries})")
            time.sleep(1)
        except requests.exceptions.Timeout:
            print(f"⚠️  Timeout (attempt {attempt + 1}/{max_retries})")
            time.sleep(1)
        except Exception as e:
            print(f"❌ Login error: {e}")
            return None
    
    print(f"❌ Login failed after {max_retries} attempts")
    return None


def get_status(token):
    """Get status with error handling"""
    url = f"{BASE_URL}"
    payload = {"type": "get_status", "token": token}
    
    try:
        response = requests.post(url, data=json.dumps(payload), timeout=TIMEOUT)
        if response.status_code == 200:
            data = response.json()
            if data.get("type") == "error":
                print(f"⚠️  Status error: {data.get('message')}")
                return None
            return data
        else:
            print(f"⚠️  Status HTTP {response.status_code}")
            return None
    except requests.exceptions.ConnectionError:
        print(f"⚠️  Connection refused - skipping")
        return None
    except requests.exceptions.Timeout:
        print(f"⚠️  Timeout - skipping")
        return None
    except Exception as e:
        print(f"⚠️  Status error: {e}")
        return None


def set_relay(token, relay_id, state):
    """Set relay with error handling"""
    url = f"{BASE_URL}"
    payload = {"type": "set_relay", "token": token, "relay_id": relay_id, "state": state}
    
    try:
        response = requests.post(url, data=json.dumps(payload), timeout=TIMEOUT)
        if response.status_code == 200:
            return response.json()
        else:
            print(f"⚠️  Set relay HTTP {response.status_code}")
            return None
    except Exception as e:
        print(f"⚠️  Set relay error: {e}")
        return None


def set_led(token, color: LED_STATES):
    """Set LED with error handling"""
    url = f"{BASE_URL}"
    payload = {"type": "set_io_led", "token": token, "color": color.value}
    
    try:
        response = requests.post(url, data=json.dumps(payload), timeout=TIMEOUT)
        if response.status_code == 200:
            return response.json()
        else:
            print(f"⚠️  Set LED HTTP {response.status_code}")
            return None
    except Exception as e:
        print(f"⚠️  Set LED error: {e}")
        return None


def main():
    print("=" * 60)
    print("  GG API TESTER")
    print("=" * 60)
    print(f"Target: {BASE_URL}")
    print()
    
    # Login
    token = login()
    if not token:
        print("\n❌ Could not login. Check:")
        print("   - Is the board reachable? ping 192.168.1.198")
        print("   - Is your IP in the whitelist?")
        return
    
    # Get status in a loop
    success_count = 0
    error_count = 0
    
    print("\n📊 Polling status for 20 iterations...\n")
    
    for i in range(20):
        status = get_status(token)
        if status:
            success_count += 1
            imu = status.get('imuValid', False)
            gps = status.get('gpsValid', False)
            pitch = status.get('pitch', 0)
            roll = status.get('roll', 0)
            print(f"[{i+1:2d}] IMU: {'✓' if imu else '✗'} | GPS: {'✓' if gps else '✗'} | "
                  f"Pitch: {pitch:6.2f}° | Roll: {roll:6.2f}°")
        else:
            error_count += 1
        
        time.sleep(0.5)
    
    # Summary
    print("\n" + "=" * 60)
    success_rate = success_count / (success_count + error_count) * 100 if (success_count + error_count) > 0 else 0
    print(f"📊 Results: {success_count} success, {error_count} errors ({success_rate:.1f}%)")
    
    if success_rate >= 95:
        print("✅ EXCELLENT!")
    elif success_rate >= 80:
        print("✅ GOOD")
    else:
        print("⚠️  Some issues detected")


if __name__ == "__main__":
    main()