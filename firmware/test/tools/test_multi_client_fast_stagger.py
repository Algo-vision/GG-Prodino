#!/usr/bin/env python3
"""
Multi-Client Test with STAGGERED polling to avoid connection issues
This demonstrates the recommended approach for 3 simultaneous clients
"""

import requests
import json
import time
import threading
from datetime import datetime

BOARD_IP = "192.168.1.198"
BOARD_URL = f"http://{BOARD_IP}/"
USERNAME = "admin"
PASSWORD = "1234"
NUM_CLIENTS = 3

class BoardClient:
    """Board client with improved connection handling"""
    
    def __init__(self, client_id, board_url, username, password):
        self.client_id = client_id
        self.board_url = board_url
        self.username = username
        self.password = password
        self.token = None
        self.is_running = False
        self.request_count = 0
        self.error_count = 0
        
    def login(self):
        """Login and obtain authentication token"""
        payload = {"type": "login", "user": self.username, "pass": self.password}
        
        try:
            response = requests.post(self.board_url, data=json.dumps(payload), timeout=5)
            if response.status_code == 200:
                data = response.json()
                if data.get("success"):
                    self.token = data.get("token")
                    print(f"✅ Client {self.client_id}: Login successful (Token: {self.token[:8]}...)")
                    return True
            return False
        except Exception as e:
            print(f"❌ Client {self.client_id}: Login error - {e}")
            return False
    
    def get_status(self):
        """Fetch device status with retry logic"""
        if not self.token:
            return None
        
        payload = {"type": "get_status", "token": self.token}
        
        # Retry logic for connection refused errors
        for attempt in range(2):
            try:
                response = requests.post(self.board_url, data=json.dumps(payload), timeout=5)
                if response.status_code == 200:
                    self.request_count += 1
                    return response.json()
                elif response.status_code == 401:
                    self.error_count += 1
                    return None
                else:
                    if attempt == 0:
                        time.sleep(0.1)  # Brief pause before retry
                        continue
                    self.error_count += 1
                    return None
            except requests.exceptions.ConnectionError:
                if attempt == 0:
                    time.sleep(0.1)  # Brief pause before retry
                    continue
                self.error_count += 1
                return None
            except Exception as e:
                self.error_count += 1
                return None
        
        return None
    
    def poll_status_staggered(self, duration_seconds=20, interval_seconds=2, offset_ms=0):
        """Poll status with initial offset and longer interval"""
        # Initial offset to stagger requests
        if offset_ms > 0:
            time.sleep(offset_ms / 1000.0)
        
        print(f"🔄 Client {self.client_id}: Starting polling (offset={offset_ms}ms, interval={interval_seconds}s)")
        self.is_running = True
        start_time = time.time()
        
        while self.is_running and (time.time() - start_time) < duration_seconds:
            status = self.get_status()
            if status:
                gps_valid = status.get('gpsValid', False)
                imu_valid = status.get('imuValid', False)
                firmware = (status.get('config') or {}).get(
                    'firmwareVersion', status.get('firmwareVersion', 'Unknown'))
                timestamp = datetime.now().strftime("%H:%M:%S.%f")[:-3]
                
                success_rate = (self.request_count / (self.request_count + self.error_count) * 100) if (self.request_count + self.error_count) > 0 else 0
                
                print(f"📊 [{timestamp}] Client {self.client_id}: "
                      f"FW={firmware}, GPS={'✓' if gps_valid else '✗'}, IMU={'✓' if imu_valid else '✗'} | "
                      f"Requests={self.request_count}, Errors={self.error_count} ({success_rate:.0f}% success)")
            
            time.sleep(interval_seconds)
        
        self.is_running = False
        success_rate = (self.request_count / (self.request_count + self.error_count) * 100) if (self.request_count + self.error_count) > 0 else 0
        print(f"⏹️  Client {self.client_id}: Polling stopped. "
              f"Total: {self.request_count} requests, {self.error_count} errors ({success_rate:.0f}% success)")


def test_staggered_polling():
    """Test with recommended staggered polling approach"""
    print("\n" + "======================================================================\n"
          "  FAST STAGGER TEST (AGGRESSIVE)\n"
          "======================================================================\n"
          f"\n🎯 Target Board: {BOARD_URL}\n"
          f"👥 Number of clients: {NUM_CLIENTS}\n"
          f"⏱️  Polling interval: 3 seconds\n"
          f"🔀 Stagger offset: 200ms (High Concurrency)\n"
          f"📅 Test started: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}\n")
    
    # Create and login clients
    clients = []
    print("🔐 Logging in all clients...")
    for i in range(NUM_CLIENTS):
        client = BoardClient(i+1, BOARD_URL, USERNAME, PASSWORD)
        if client.login():
            clients.append(client)
        time.sleep(0.3)
    
    print(f"\n✅ {len(clients)}/{NUM_CLIENTS} clients logged in successfully\n")
    
    if len(clients) == 0:
        print("❌ No clients logged in. Check board connection.")
        return
    
    # Start staggered polling
    print("📡 Starting FAST STAGGERED polling from all clients for 20 seconds...")
    print("    Client 1: starts at T+0ms")
    print("    Client 2: starts at T+200ms")
    print("    Client 3: starts at T+400ms")
    print("    Polling interval: 3 seconds\n")
    
    threads = []
    for i, client in enumerate(clients):
        offset = i * 200  # Stagger by 200ms (Aggressive!)
        thread = threading.Thread(
            target=lambda c=client, o=offset: c.poll_status_staggered(20, 3, o)
        )
        thread.start()
        threads.append(thread)
    
    # Wait for completion
    for thread in threads:
        thread.join()
    
    # Report results
    print("\n" + "="*70)
    print("  TEST RESULTS")
    print("="*70)
    
    total_requests = sum(c.request_count for c in clients)
    total_errors = sum(c.error_count for c in clients)
    overall_success = (total_requests / (total_requests + total_errors) * 100) if (total_requests + total_errors) > 0 else 0
    
    print(f"\n📊 Summary:")
    for client in clients:
        success_rate = (client.request_count / (client.request_count + client.error_count) * 100) if (client.request_count + client.error_count) > 0 else 0
        print(f"  Client {client.client_id}: {client.request_count} successful, {client.error_count} errors ({success_rate:.1f}% success)")
    
    print(f"\n📈 Overall: {total_requests} successful, {total_errors} errors ({overall_success:.1f}% success)")
    
    if overall_success >= 95:
        print("✅ EXCELLENT: >95% success rate - system is stable!")
    elif overall_success >= 80:
        print("✅ GOOD: >80% success rate - acceptable for production")
    elif overall_success >= 50:
        print("⚠️  FAIR: 50-80% success rate - consider further optimization")
    else:
        print("❌ POOR: <50% success rate - board may be overloaded or network issues")
    
    print("\n💡 Recommendation:")
    if overall_success >= 80:
        print("   This configuration (3s polling, 1000ms offset) works well for 3 clients.")
        print("   You can deploy this to production.")
    else:
        print("   Try increasing polling interval to 4 seconds or reducing number of clients.")


if __name__ == "__main__":
    test_staggered_polling()
