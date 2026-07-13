#!/usr/bin/env python3
"""
Multi-Client Test from DIFFERENT SOURCE IPs
Each client binds to a different IP alias on the local machine.
This simulates real-world multi-client scenarios.

Prerequisites:
  sudo ip addr add 192.168.1.169/24 dev enp7s0
  sudo ip addr add 192.168.1.33/24 dev enp7s0
"""

import requests
import json
import time
import threading
import socket
from datetime import datetime
from requests.adapters import HTTPAdapter
from urllib3.util.connection import create_connection

BOARD_IP = "192.168.1.198"
BOARD_URL = f"http://{BOARD_IP}/"
USERNAME = "admin"
PASSWORD = "1234"

# Source IPs (must be in whitelist and aliased on local interface)
SOURCE_IPS = [
    "192.168.1.20",   # Client 1 - original IP
    "192.168.1.169",  # Client 2 - alias
    "192.168.1.33",   # Client 3 - alias
]


class SourceIPAdapter(HTTPAdapter):
    """HTTP Adapter that binds to a specific source IP address"""
    
    def __init__(self, source_ip, *args, **kwargs):
        self.source_ip = source_ip
        super().__init__(*args, **kwargs)
    
    def init_poolmanager(self, *args, **kwargs):
        # Create a custom socket_options to bind to specific IP
        import urllib3
        
        # Store the source IP for later use
        self._source_ip = self.source_ip
        
        super().init_poolmanager(*args, **kwargs)
    
    def send(self, request, *args, **kwargs):
        # Monkey-patch the socket creation for this adapter
        original_create_connection = socket.create_connection
        source_ip = self.source_ip
        
        def bound_create_connection(address, *args, **kwargs):
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.bind((source_ip, 0))  # Bind to source IP with any port
            sock.connect(address)
            return sock
        
        socket.create_connection = bound_create_connection
        try:
            response = super().send(request, *args, **kwargs)
        finally:
            socket.create_connection = original_create_connection
        
        return response


class MultiIPBoardClient:
    """Board client that binds to a specific source IP"""
    
    def __init__(self, client_id, board_url, username, password, source_ip):
        self.client_id = client_id
        self.board_url = board_url
        self.username = username
        self.password = password
        self.source_ip = source_ip
        self.token = None
        self.is_running = False
        self.request_count = 0
        self.error_count = 0
        
        # Create session with custom adapter for source IP binding
        self.session = requests.Session()
        adapter = SourceIPAdapter(source_ip)
        self.session.mount('http://', adapter)
        self.session.mount('https://', adapter)
        
    def login(self):
        """Login and obtain authentication token"""
        payload = {"type": "login", "user": self.username, "pass": self.password}
        
        try:
            response = self.session.post(self.board_url, data=json.dumps(payload), timeout=5)
            if response.status_code == 200:
                data = response.json()
                if data.get("success"):
                    self.token = data.get("token")
                    print(f"✅ Client {self.client_id} ({self.source_ip}): Login successful (Token: {self.token[:8]}...)")
                    return True
                else:
                    print(f"❌ Client {self.client_id} ({self.source_ip}): Login failed - not whitelisted?")
            else:
                print(f"❌ Client {self.client_id} ({self.source_ip}): HTTP {response.status_code}")
            return False
        except Exception as e:
            print(f"❌ Client {self.client_id} ({self.source_ip}): Login error - {e}")
            return False
    
    def get_status(self):
        """Fetch device status"""
        if not self.token:
            return None
        
        payload = {"type": "get_status", "token": self.token}
        
        try:
            response = self.session.post(self.board_url, data=json.dumps(payload), timeout=5)
            if response.status_code == 200:
                self.request_count += 1
                return response.json()
            else:
                self.error_count += 1
                return None
        except Exception as e:
            self.error_count += 1
            return None
    
    def poll_status(self, duration_seconds=20, interval_seconds=3, offset_ms=0):
        """Poll status with initial offset"""
        if offset_ms > 0:
            time.sleep(offset_ms / 1000.0)
        
        print(f"🔄 Client {self.client_id} ({self.source_ip}): Starting polling")
        self.is_running = True
        start_time = time.time()
        
        while self.is_running and (time.time() - start_time) < duration_seconds:
            status = self.get_status()
            if status:
                gps_valid = status.get('gpsValid', False)
                imu_valid = status.get('imuValid', False)
                firmware = status.get('firmwareVersion', 'Unknown')
                timestamp = datetime.now().strftime("%H:%M:%S.%f")[:-3]
                
                success_rate = (self.request_count / (self.request_count + self.error_count) * 100) if (self.request_count + self.error_count) > 0 else 0
                
                print(f"📊 [{timestamp}] Client {self.client_id} ({self.source_ip}): "
                      f"FW={firmware}, GPS={'✓' if gps_valid else '✗'}, IMU={'✓' if imu_valid else '✗'} | "
                      f"Requests={self.request_count}, Errors={self.error_count} ({success_rate:.0f}%)")
            
            time.sleep(interval_seconds)
        
        self.is_running = False
        success_rate = (self.request_count / (self.request_count + self.error_count) * 100) if (self.request_count + self.error_count) > 0 else 0
        print(f"⏹️  Client {self.client_id} ({self.source_ip}): Stopped. "
              f"Total: {self.request_count} OK, {self.error_count} errors ({success_rate:.0f}%)")


def test_multi_ip_clients():
    """Test with clients from different source IPs"""
    print("\n" + "="*70)
    print("  MULTI-IP CLIENT TEST")
    print("="*70)
    print(f"\n🎯 Target Board: {BOARD_URL}")
    print(f"👥 Clients: {len(SOURCE_IPS)}")
    print(f"📌 Source IPs: {', '.join(SOURCE_IPS)}")
    print(f"⏱️  Polling: 3 seconds")
    print(f"📅 Started: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}\n")
    
    # Create and login clients
    clients = []
    print("🔐 Logging in all clients from different IPs...\n")
    
    for i, source_ip in enumerate(SOURCE_IPS):
        client = MultiIPBoardClient(i+1, BOARD_URL, USERNAME, PASSWORD, source_ip)
        if client.login():
            clients.append(client)
        time.sleep(0.3)
    
    print(f"\n✅ {len(clients)}/{len(SOURCE_IPS)} clients logged in\n")
    
    if len(clients) == 0:
        print("❌ No clients logged in. Check whitelist and IP aliases.")
        return
    
    # Start polling
    print("📡 Starting polling from all clients for 20 seconds...\n")
    
    threads = []
    for i, client in enumerate(clients):
        offset = i * 200  # Stagger by 200ms
        thread = threading.Thread(
            target=lambda c=client, o=offset: c.poll_status(20, 3, o)
        )
        thread.start()
        threads.append(thread)
    
    for thread in threads:
        thread.join()
    
    # Report
    print("\n" + "="*70)
    print("  TEST RESULTS")
    print("="*70)
    
    total_requests = sum(c.request_count for c in clients)
    total_errors = sum(c.error_count for c in clients)
    overall_success = (total_requests / (total_requests + total_errors) * 100) if (total_requests + total_errors) > 0 else 0
    
    print(f"\n📊 Per-Client Summary:")
    for client in clients:
        success_rate = (client.request_count / (client.request_count + client.error_count) * 100) if (client.request_count + client.error_count) > 0 else 0
        print(f"  Client {client.client_id} ({client.source_ip}): {client.request_count} OK, {client.error_count} errors ({success_rate:.1f}%)")
    
    print(f"\n📈 Overall: {total_requests} successful, {total_errors} errors ({overall_success:.1f}% success)")
    
    if overall_success >= 95:
        print("✅ EXCELLENT: >95% success rate!")
    elif overall_success >= 80:
        print("✅ GOOD: >80% success rate")
    else:
        print("⚠️  Some issues detected. Check board logs.")


if __name__ == "__main__":
    test_multi_ip_clients()
