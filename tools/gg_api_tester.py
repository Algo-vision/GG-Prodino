#!/usr/bin/env python3
"""
GG GRK API Tester - Professional Client Tool
Version: 1.0.1

This tool tests the HTTP API of the GG GRK controller board.
It provides comprehensive diagnostics for connection, authentication,
and data retrieval issues.

Usage:
    python3 gg_api_tester.py [options]
    
Options:
    --host IP        Board IP address (default: 192.168.1.198)
    --user USER      Username (default: admin)
    --pass PASSWORD  Password (default: 1234)
    --count N        Number of status polls (default: 20)
    --interval MS    Interval between polls in ms (default: 500)
    --timeout SEC    Request timeout in seconds (default: 5)
    --help           Show this help message
"""

import requests
import time
import json
import sys
import argparse
import socket
from datetime import datetime
from enum import Enum
from typing import Optional, Dict, Any, Tuple


# =============================================================================
# CONFIGURATION
# =============================================================================

VERSION = "1.0.1"

class LED_STATES(Enum):
    OFF = "OFF"
    GREEN = "GREEN"
    RED = "RED"
    ORANGE = "ORANGE"


class ErrorType(Enum):
    """Categorized error types for clear diagnostics"""
    NONE = "none"
    CONNECTION_REFUSED = "connection_refused"
    TIMEOUT = "timeout"
    DNS_ERROR = "dns_error"
    NETWORK_UNREACHABLE = "network_unreachable"
    HTTP_ERROR = "http_error"
    JSON_PARSE_ERROR = "json_parse_error"
    AUTH_FAILED = "auth_failed"
    IP_NOT_WHITELISTED = "ip_not_whitelisted"
    TOKEN_INVALID = "token_invalid"
    UNKNOWN = "unknown"


# =============================================================================
# API CLIENT
# =============================================================================

class GGApiClient:
    """Professional API client for GG GRK board"""
    
    def __init__(self, host: str, user: str = "admin", password: str = "1234", 
                 timeout: float = 5.0):
        self.host = host
        self.base_url = f"http://{host}/"
        self.user = user
        self.password = password
        self.timeout = timeout
        self.token: Optional[str] = None
        
        # Statistics
        self.stats = {
            "requests": 0,
            "success": 0,
            "errors": 0,
            "errors_by_type": {},
            "latencies": [],
        }
    
    def _classify_error(self, exception: Exception, response: Optional[requests.Response] = None) -> Tuple[ErrorType, str]:
        """Classify an error into a specific type with description"""
        
        if response is not None:
            if response.status_code == 403:
                return ErrorType.IP_NOT_WHITELISTED, f"Your IP address is not in the board's whitelist (HTTP 403)"
            elif response.status_code == 401:
                return ErrorType.TOKEN_INVALID, f"Token is invalid or expired (HTTP 401)"
            elif response.status_code >= 400:
                return ErrorType.HTTP_ERROR, f"HTTP error {response.status_code}"
        
        error_str = str(exception).lower()
        
        if isinstance(exception, requests.exceptions.ConnectionError):
            if "connection refused" in error_str:
                return ErrorType.CONNECTION_REFUSED, "Board is not accepting connections (port 80 closed or board busy)"
            elif "network is unreachable" in error_str:
                return ErrorType.NETWORK_UNREACHABLE, "Network is unreachable - check Ethernet/WiFi connection"
            elif "name or service not known" in error_str or "nodename nor servname" in error_str:
                return ErrorType.DNS_ERROR, "Could not resolve hostname"
            else:
                return ErrorType.CONNECTION_REFUSED, f"Connection failed: {exception}"
        
        elif isinstance(exception, requests.exceptions.Timeout):
            return ErrorType.TIMEOUT, f"Request timed out after {self.timeout}s - board may be overloaded"
        
        elif isinstance(exception, json.JSONDecodeError):
            return ErrorType.JSON_PARSE_ERROR, "Invalid JSON response from board"
        
        else:
            return ErrorType.UNKNOWN, str(exception)
    
    def _record_error(self, error_type: ErrorType):
        """Record an error in statistics"""
        self.stats["errors"] += 1
        if error_type.value not in self.stats["errors_by_type"]:
            self.stats["errors_by_type"][error_type.value] = 0
        self.stats["errors_by_type"][error_type.value] += 1
    
    def _make_request(self, payload: Dict[str, Any]) -> Tuple[Optional[Dict], ErrorType, str, float]:
        """
        Make an HTTP request to the board.
        Returns: (data, error_type, error_message, latency_ms)
        """
        self.stats["requests"] += 1
        start_time = time.time()
        
        try:
            response = requests.post(
                self.base_url, 
                data=json.dumps(payload), 
                timeout=self.timeout,
                headers={"Content-Type": "application/json"}
            )
            latency = (time.time() - start_time) * 1000
            
            # Check HTTP status
            if response.status_code != 200:
                error_type, error_msg = self._classify_error(Exception(), response)
                self._record_error(error_type)
                return None, error_type, error_msg, latency
            
            # Parse JSON
            try:
                data = response.json()
            except json.JSONDecodeError as e:
                self._record_error(ErrorType.JSON_PARSE_ERROR)
                return None, ErrorType.JSON_PARSE_ERROR, f"Invalid JSON: {e}", latency
            
            # Check for error response
            if data.get("type") == "error":
                msg = data.get("message", "Unknown error")
                if "token" in msg.lower():
                    self._record_error(ErrorType.TOKEN_INVALID)
                    return None, ErrorType.TOKEN_INVALID, msg, latency
                elif "ip" in msg.lower() or "whitelist" in msg.lower():
                    self._record_error(ErrorType.IP_NOT_WHITELISTED)
                    return None, ErrorType.IP_NOT_WHITELISTED, msg, latency
                else:
                    self._record_error(ErrorType.AUTH_FAILED)
                    return None, ErrorType.AUTH_FAILED, msg, latency
            
            self.stats["success"] += 1
            self.stats["latencies"].append(latency)
            return data, ErrorType.NONE, "", latency
            
        except Exception as e:
            latency = (time.time() - start_time) * 1000
            error_type, error_msg = self._classify_error(e)
            self._record_error(error_type)
            return None, error_type, error_msg, latency
    
    def login(self, max_retries: int = 3) -> bool:
        """
        Authenticate with the board.
        Returns True if successful, False otherwise.
        """
        payload = {"type": "login", "user": self.user, "pass": self.password}
        
        for attempt in range(max_retries):
            data, error_type, error_msg, latency = self._make_request(payload)
            
            if data is not None:
                if data.get("success"):
                    self.token = data.get("token")
                    print(f"  [OK] Login successful (latency: {latency:.0f}ms)")
                    print(f"       Token: {self.token}")
                    return True
                else:
                    print(f"  [FAIL] Login rejected: {data.get('message', 'Unknown reason')}")
                    return False
            else:
                if attempt < max_retries - 1:
                    print(f"  [WARN] Attempt {attempt + 1}/{max_retries}: {error_msg}")
                    time.sleep(1)
                else:
                    print(f"  [FAIL] {error_msg}")
        
        return False
    
    def get_status(self) -> Tuple[Optional[Dict], float]:
        """
        Get device status.
        Returns: (status_dict, latency_ms) or (None, latency_ms) on error
        """
        if not self.token:
            return None, 0
        
        payload = {"type": "get_status", "token": self.token}
        data, error_type, error_msg, latency = self._make_request(payload)
        
        if data is not None:
            return data, latency
        else:
            return None, latency
    
    def set_relay(self, relay_id: int, state: bool) -> Optional[Dict]:
        """Set relay state"""
        if not self.token:
            return None
        payload = {"type": "set_relay", "token": self.token, "relay_id": relay_id, "state": state}
        data, _, _, _ = self._make_request(payload)
        return data
    
    def set_led(self, color: LED_STATES) -> Optional[Dict]:
        """Set IO LED color"""
        if not self.token:
            return None
        payload = {"type": "set_io_led", "token": self.token, "color": color.value}
        data, _, _, _ = self._make_request(payload)
        return data
    
    def get_statistics(self) -> Dict:
        """Get request statistics"""
        latencies = self.stats["latencies"]
        return {
            "total_requests": self.stats["requests"],
            "successful": self.stats["success"],
            "failed": self.stats["errors"],
            "success_rate": (self.stats["success"] / self.stats["requests"] * 100) if self.stats["requests"] > 0 else 0,
            "avg_latency_ms": sum(latencies) / len(latencies) if latencies else 0,
            "min_latency_ms": min(latencies) if latencies else 0,
            "max_latency_ms": max(latencies) if latencies else 0,
            "errors_by_type": self.stats["errors_by_type"],
        }


# =============================================================================
# MAIN TEST ROUTINE
# =============================================================================

def format_status(status: Dict) -> str:
    """Format status dict as a readable string showing all fields"""
    lines = []
    
    # Core info
    lines.append(f"  Firmware:       {status.get('firmwareVersion', 'N/A')}")
    lines.append(f"  Controller IP:  {status.get('controllerIp', 'N/A')}")
    lines.append(f"  Router IP:      {status.get('routerIp', 'N/A')}")
    lines.append(f"  Whitelist:      {status.get('whitelistIps', [])}")
    
    # Hardware status
    relays = status.get('relays_status', [0,0,0,0])
    optoin = status.get('optoin_status', [0,0,0,0])
    lines.append(f"  Relays:         {relays}")
    lines.append(f"  OptoIn:         {optoin}")
    
    # IMU
    imu_valid = status.get('imuValid', False)
    lines.append(f"  IMU Valid:      {imu_valid}")
    if imu_valid:
        lines.append(f"  IMU Accel:      X={status.get('imuX', 0):.3f} Y={status.get('imuY', 0):.3f} Z={status.get('imuZ', 0):.3f}")
        lines.append(f"  IMU Gyro:       X={status.get('imuGx', 0):.3f} Y={status.get('imuGy', 0):.3f} Z={status.get('imuGz', 0):.3f}")
        lines.append(f"  Orientation:    Pitch={status.get('pitch', 0):.2f} Roll={status.get('roll', 0):.2f} Yaw={status.get('yaw', 0):.2f}")
    
    # GPS
    gps_connected = status.get('GPSConnected', False)
    gps_valid = status.get('gpsValid', False)
    lines.append(f"  GPS Connected:  {gps_connected}")
    lines.append(f"  GPS Valid:      {gps_valid}")
    if gps_valid:
        lines.append(f"  GPS Position:   Lat={status.get('gpsLat', 0):.6f} Lng={status.get('gpsLng', 0):.6f} Alt={status.get('gpsAlt', 0):.1f}m")
        lines.append(f"  GPS Speed:      Ground={status.get('gpsGroundSpeed', 0):.1f}km/h Heading={status.get('gpsHeading', 0):.1f}deg")
        lines.append(f"  GPS Satellites: {status.get('gpsSatellites', 0)}")
        lines.append(f"  GPS Accuracy:   H={status.get('gpsHAcc', 0):.0f}mm V={status.get('gpsVAcc', 0):.0f}mm")
        lines.append(f"  GPS Alt (Ellipsoid): {status.get('gpsAltEllipsoid', 0):.0f}mm")
        lines.append(f"  GPS Time:       {status.get('gpsTime', 'N/A')}")
    
    # LEDs and buttons
    lines.append(f"  LED Internal:   {status.get('ledInternal', False)}")
    lines.append(f"  LED IO:         {status.get('ledIo', 'OFF')}")
    lines.append(f"  Button Tech:    {status.get('button_tech', False)}")
    lines.append(f"  Technician Mode:{status.get('technicianMode', False)}")
    
    # Motor hours
    lines.append(f"  Motor Hours:    {status.get('motorWorkHours', 0):.2f}h ({status.get('motorWorkSeconds', 0)}s)")
    
    # INA219
    ina_connected = status.get('inaConnected', False)
    lines.append(f"  INA219:         {'Connected' if ina_connected else 'Not Connected'}")
    if ina_connected:
        lines.append(f"  Bus Voltage:    {status.get('busVoltage', 0):.2f}V")
    
    return "\n".join(lines)


def run_test(args):
    """Run the API test"""
    
    print("=" * 70)
    print(f"  GG GRK API Tester v{VERSION}")
    print("=" * 70)
    print()
    
    # System info
    print("[CONFIG] Test Configuration:")
    print(f"   Target Board:    {args.host}")
    print(f"   Username:        {args.user}")
    print(f"   Poll Count:      {args.count}")
    print(f"   Poll Interval:   {args.interval}ms")
    print(f"   Timeout:         {args.timeout}s")
    print()
    
    # Get local IP
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        s.connect((args.host, 80))
        local_ip = s.getsockname()[0]
        s.close()
        print(f"   Your IP:         {local_ip}")
    except:
        print(f"   Your IP:         (could not determine)")
    print()
    
    # Connectivity check
    print("[STEP 1] Connectivity Check")
    print(f"   Testing port 80 on {args.host}...", end=" ", flush=True)
    
    ping_ok = False
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.settimeout(2)
        result = sock.connect_ex((args.host, 80))
        sock.close()
        if result == 0:
            print("[OK] Port 80 open")
            ping_ok = True
        else:
            print(f"[FAIL] Port 80 closed (error code: {result})")
    except Exception as e:
        print(f"[FAIL] {e}")
    
    if not ping_ok:
        print("\n[ERROR] Cannot reach the board. Please check:")
        print("   1. Is the board powered on?")
        print("   2. Is the Ethernet cable connected?")
        print("   3. Is the IP address correct?")
        print(f"   4. Try: ping {args.host}")
        return
    print()
    
    # Create client
    client = GGApiClient(
        host=args.host,
        user=args.user,
        password=args.password,
        timeout=args.timeout
    )
    
    # Login
    print("[STEP 2] Authentication")
    if not client.login():
        print("\n[ERROR] Authentication failed. Please check:")
        print("   1. Is your IP in the whitelist?")
        print("   2. Are the credentials correct?")
        print("   3. Is another client currently connected (may invalidate tokens)?")
        return
    print()
    
    # Poll status
    print(f"[STEP 3] Polling Status ({args.count} iterations)")
    print("-" * 70)
    
    for i in range(args.count):
        status, latency = client.get_status()
        
        timestamp = datetime.now().strftime("%H:%M:%S.%f")[:-3]
        
        if status:
            print(f"\n[{timestamp}] Request #{i+1} - Latency: {latency:.0f}ms - [OK]")
            print(format_status(status))
        else:
            stats = client.get_statistics()
            last_error = list(stats["errors_by_type"].keys())[-1] if stats["errors_by_type"] else "unknown"
            print(f"\n[{timestamp}] Request #{i+1} - Latency: {latency:.0f}ms - [ERROR: {last_error}]")
        
        if i < args.count - 1:
            time.sleep(args.interval / 1000.0)
    
    print()
    print("-" * 70)
    print()
    
    # Results
    stats = client.get_statistics()
    
    print("=" * 70)
    print("  TEST RESULTS")
    print("=" * 70)
    print()
    print("[STATS] Request Statistics:")
    print(f"   Total Requests:    {stats['total_requests']}")
    print(f"   Successful:        {stats['successful']}")
    print(f"   Failed:            {stats['failed']}")
    print(f"   Success Rate:      {stats['success_rate']:.1f}%")
    print()
    print("[TIMING] Latency:")
    print(f"   Average:           {stats['avg_latency_ms']:.0f}ms")
    print(f"   Minimum:           {stats['min_latency_ms']:.0f}ms")
    print(f"   Maximum:           {stats['max_latency_ms']:.0f}ms")
    
    if stats["errors_by_type"]:
        print()
        print("[ERRORS] Errors by Type:")
        for error_type, count in stats["errors_by_type"].items():
            print(f"   {error_type}: {count}")
    
    print()
    
    # Final verdict
    if stats['success_rate'] >= 99:
        print("[RESULT] EXCELLENT: API is working perfectly!")
    elif stats['success_rate'] >= 95:
        print("[RESULT] GOOD: API is working well with minor issues.")
    elif stats['success_rate'] >= 80:
        print("[RESULT] WARNING: Some reliability issues detected.")
        print("    Possible causes:")
        print("    - Network congestion")
        print("    - Board under heavy load")
        print("    - Multiple clients competing for resources")
    else:
        print("[RESULT] PROBLEM: Significant reliability issues.")
        print("    Please check:")
        print("    - Network connectivity")
        print("    - Board health")
        print("    - Whitelist configuration")
    
    print()
    print("=" * 70)


def main():
    parser = argparse.ArgumentParser(
        description="GG GRK API Tester - Professional testing tool for the GG GRK HTTP API",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  python3 gg_api_tester.py                     # Run with defaults
  python3 gg_api_tester.py --host 192.168.0.50 # Use different IP
  python3 gg_api_tester.py --count 100         # Run 100 polls
  python3 gg_api_tester.py --interval 200      # Poll every 200ms
        """
    )
    
    parser.add_argument("--host", default="192.168.1.198", help="Board IP address")
    parser.add_argument("--user", default="admin", help="Username")
    parser.add_argument("--password", dest="password", default="1234", help="Password")
    parser.add_argument("--count", type=int, default=20, help="Number of status polls")
    parser.add_argument("--interval", type=int, default=50, help="Interval between polls (ms)")
    parser.add_argument("--timeout", type=float, default=5.0, help="Request timeout (seconds)")
    parser.add_argument("--version", action="version", version=f"%(prog)s {VERSION}")
    
    args = parser.parse_args()
    
    try:
        run_test(args)
    except KeyboardInterrupt:
        print("\n\n[WARN] Test interrupted by user")
        sys.exit(1)


if __name__ == "__main__":
    main()