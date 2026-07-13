#!/usr/bin/env python3
"""
Jetson Network Diagnostics Tool
Version: 1.0.0

This tool diagnoses network issues when connecting to the GG ProDino board
from Jetson devices. It tests various potential bottlenecks:

1. Network interface type (Ethernet vs USB adapter)
2. TCP socket timing and buffer sizes
3. HTTP connection pooling behavior
4. Request/response timing breakdown
5. Connection reuse vs new connections

Usage: python3 jetson_diagnostics.py [--host IP]
"""

import socket
import time
import json
import subprocess
import sys
import argparse
import statistics
from typing import List, Dict, Tuple, Optional
from datetime import datetime

try:
    import requests
except ImportError:
    print("[ERROR] requests module not installed. Run: pip3 install requests")
    sys.exit(1)


# =============================================================================
# CONFIGURATION
# =============================================================================

VERSION = "1.0.0"
DEFAULT_HOST = "192.168.1.198"


# =============================================================================
# DIAGNOSTIC TESTS
# =============================================================================

def print_header(title: str):
    print()
    print("=" * 70)
    print(f"  {title}")
    print("=" * 70)


def print_section(title: str):
    print()
    print(f"[{title}]")
    print("-" * 50)


def test_network_interface() -> Dict:
    """Test network interface configuration"""
    print_section("Network Interface Analysis")
    
    result = {
        "type": "unknown",
        "name": "unknown",
        "speed": "unknown",
        "mtu": "unknown",
        "ip": "unknown"
    }
    
    try:
        # Get default route interface
        output = subprocess.check_output(["ip", "route", "show", "default"], text=True)
        if "dev" in output:
            iface = output.split("dev ")[1].split()[0]
            result["name"] = iface
            
            # Check if USB adapter
            if "usb" in iface.lower() or "enx" in iface or "eth" not in iface:
                result["type"] = "USB Ethernet Adapter"
                print(f"   Interface:       {iface} (USB ADAPTER - may cause latency)")
            else:
                result["type"] = "Built-in Ethernet"
                print(f"   Interface:       {iface} (Native)")
            
            # Get interface details
            try:
                ip_output = subprocess.check_output(["ip", "addr", "show", iface], text=True)
                for line in ip_output.split("\n"):
                    if "inet " in line and "inet6" not in line:
                        result["ip"] = line.split()[1].split("/")[0]
                        print(f"   IP Address:      {result['ip']}")
                    if "mtu" in line.lower():
                        mtu = line.split("mtu ")[1].split()[0]
                        result["mtu"] = mtu
                        print(f"   MTU:             {mtu}")
            except:
                pass
            
            # Try to get link speed
            try:
                ethtool_output = subprocess.check_output(["ethtool", iface], text=True, stderr=subprocess.DEVNULL)
                for line in ethtool_output.split("\n"):
                    if "Speed:" in line:
                        result["speed"] = line.split(":")[1].strip()
                        print(f"   Link Speed:      {result['speed']}")
            except:
                print(f"   Link Speed:      (unable to determine)")
                
    except Exception as e:
        print(f"   [ERROR] Could not analyze network interface: {e}")
    
    return result


def test_tcp_connection(host: str, port: int = 80, count: int = 10) -> Dict:
    """Test raw TCP connection timing"""
    print_section("TCP Connection Timing")
    
    latencies = []
    errors = 0
    
    print(f"   Testing {count} TCP connections to {host}:{port}...")
    
    for i in range(count):
        start = time.time()
        try:
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.settimeout(5)
            sock.connect((host, port))
            latency = (time.time() - start) * 1000
            latencies.append(latency)
            sock.close()
        except Exception as e:
            errors += 1
            print(f"   Connection {i+1}: ERROR - {e}")
    
    result = {
        "attempts": count,
        "success": len(latencies),
        "errors": errors,
        "avg_ms": statistics.mean(latencies) if latencies else 0,
        "min_ms": min(latencies) if latencies else 0,
        "max_ms": max(latencies) if latencies else 0,
        "stdev_ms": statistics.stdev(latencies) if len(latencies) > 1 else 0
    }
    
    print(f"   Success:         {result['success']}/{count}")
    print(f"   Errors:          {errors}")
    print(f"   Avg Latency:     {result['avg_ms']:.1f}ms")
    print(f"   Min Latency:     {result['min_ms']:.1f}ms")
    print(f"   Max Latency:     {result['max_ms']:.1f}ms")
    print(f"   Std Dev:         {result['stdev_ms']:.1f}ms")
    
    if result['max_ms'] > 100:
        print(f"   [WARN] High max latency detected - possible network congestion")
    if result['stdev_ms'] > 20:
        print(f"   [WARN] High latency variance - unstable connection")
    
    return result


def test_http_request_timing(host: str, count: int = 10) -> Dict:
    """Test HTTP request timing breakdown"""
    print_section("HTTP Request Timing Breakdown")
    
    url = f"http://{host}/"
    payload = {"type": "login", "user": "admin", "pass": "1234"}
    
    # First get a token
    token = None
    try:
        response = requests.post(url, data=json.dumps(payload), timeout=5)
        if response.status_code == 200:
            data = response.json()
            if data.get("success"):
                token = data.get("token")
                print(f"   [OK] Obtained token for testing")
            else:
                print(f"   [WARN] Login failed: {data.get('message')}")
        else:
            print(f"   [WARN] Login HTTP {response.status_code}")
    except Exception as e:
        print(f"   [ERROR] Could not login: {e}")
        return {"error": str(e)}
    
    if not token:
        print(f"   [ERROR] Cannot proceed without token")
        return {"error": "No token"}
    
    # Test get_status timing
    payload = {"type": "get_status", "token": token}
    
    dns_times = []
    connect_times = []
    send_times = []
    wait_times = []
    receive_times = []
    total_times = []
    errors = 0
    
    print(f"   Testing {count} HTTP requests...")
    
    for i in range(count):
        try:
            # We'll measure total time and breakdown isn't easily available
            # without more complex instrumentation
            start = time.time()
            response = requests.post(url, data=json.dumps(payload), timeout=5)
            total = (time.time() - start) * 1000
            
            if response.status_code == 200:
                total_times.append(total)
            else:
                errors += 1
                print(f"   Request {i+1}: HTTP {response.status_code}")
        except requests.exceptions.ConnectionError as e:
            errors += 1
            print(f"   Request {i+1}: CONNECTION ERROR")
        except requests.exceptions.Timeout:
            errors += 1
            print(f"   Request {i+1}: TIMEOUT")
        except Exception as e:
            errors += 1
            print(f"   Request {i+1}: {type(e).__name__}")
    
    result = {
        "attempts": count,
        "success": len(total_times),
        "errors": errors,
        "success_rate": len(total_times) / count * 100 if count > 0 else 0,
        "avg_ms": statistics.mean(total_times) if total_times else 0,
        "min_ms": min(total_times) if total_times else 0,
        "max_ms": max(total_times) if total_times else 0,
        "stdev_ms": statistics.stdev(total_times) if len(total_times) > 1 else 0
    }
    
    print(f"   Success:         {result['success']}/{count} ({result['success_rate']:.1f}%)")
    print(f"   Errors:          {errors}")
    print(f"   Avg Latency:     {result['avg_ms']:.1f}ms")
    print(f"   Min Latency:     {result['min_ms']:.1f}ms")
    print(f"   Max Latency:     {result['max_ms']:.1f}ms")
    print(f"   Std Dev:         {result['stdev_ms']:.1f}ms")
    
    return result


def test_connection_reuse(host: str, count: int = 20) -> Dict:
    """Test if connection reuse helps"""
    print_section("Connection Reuse Test")
    
    url = f"http://{host}/"
    
    # Login first
    payload = {"type": "login", "user": "admin", "pass": "1234"}
    session = requests.Session()
    
    try:
        response = session.post(url, data=json.dumps(payload), timeout=5)
        if response.status_code != 200:
            print(f"   [ERROR] Login failed: HTTP {response.status_code}")
            return {"error": "login failed"}
        data = response.json()
        if not data.get("success"):
            print(f"   [ERROR] Login failed: {data.get('message')}")
            return {"error": "login failed"}
        token = data.get("token")
    except Exception as e:
        print(f"   [ERROR] {e}")
        return {"error": str(e)}
    
    # Test with session (connection reuse)
    payload = {"type": "get_status", "token": token}
    session_times = []
    session_errors = 0
    
    print(f"   Testing {count} requests WITH session (connection reuse)...")
    for i in range(count):
        try:
            start = time.time()
            response = session.post(url, data=json.dumps(payload), timeout=5)
            total = (time.time() - start) * 1000
            if response.status_code == 200:
                session_times.append(total)
            else:
                session_errors += 1
        except:
            session_errors += 1
    
    # Test without session (new connection each time)
    no_session_times = []
    no_session_errors = 0
    
    print(f"   Testing {count} requests WITHOUT session (new connection each time)...")
    for i in range(count):
        try:
            start = time.time()
            response = requests.post(url, data=json.dumps(payload), timeout=5)
            total = (time.time() - start) * 1000
            if response.status_code == 200:
                no_session_times.append(total)
            else:
                no_session_errors += 1
        except:
            no_session_errors += 1
    
    session.close()
    
    result = {
        "with_session": {
            "success": len(session_times),
            "errors": session_errors,
            "avg_ms": statistics.mean(session_times) if session_times else 0,
        },
        "without_session": {
            "success": len(no_session_times),
            "errors": no_session_errors,
            "avg_ms": statistics.mean(no_session_times) if no_session_times else 0,
        }
    }
    
    print()
    print(f"   WITH Session (reuse):")
    print(f"     Success:       {result['with_session']['success']}/{count}")
    print(f"     Errors:        {result['with_session']['errors']}")
    print(f"     Avg Latency:   {result['with_session']['avg_ms']:.1f}ms")
    print()
    print(f"   WITHOUT Session (new connection):")
    print(f"     Success:       {result['without_session']['success']}/{count}")
    print(f"     Errors:        {result['without_session']['errors']}")
    print(f"     Avg Latency:   {result['without_session']['avg_ms']:.1f}ms")
    
    if result['with_session']['avg_ms'] < result['without_session']['avg_ms'] * 0.8:
        print()
        print(f"   [TIP] Connection reuse provides significant speedup!")
        print(f"         Consider using requests.Session() in your code.")
    
    return result


def test_socket_options() -> Dict:
    """Show socket configuration"""
    print_section("Socket Configuration")
    
    result = {}
    
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        
        # Get various socket options
        rcvbuf = sock.getsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF)
        sndbuf = sock.getsockopt(socket.SOL_SOCKET, socket.SO_SNDBUF)
        
        result = {
            "rcvbuf": rcvbuf,
            "sndbuf": sndbuf,
        }
        
        print(f"   Receive Buffer:  {rcvbuf} bytes")
        print(f"   Send Buffer:     {sndbuf} bytes")
        
        sock.close()
        
        # Check TCP settings from system
        try:
            with open("/proc/sys/net/ipv4/tcp_fin_timeout") as f:
                print(f"   TCP FIN Timeout: {f.read().strip()}s")
            with open("/proc/sys/net/core/somaxconn") as f:
                print(f"   Max Connections: {f.read().strip()}")
        except:
            pass
            
    except Exception as e:
        print(f"   [ERROR] {e}")
    
    return result


def test_rapid_fire(host: str, count: int = 50, delay_ms: int = 50) -> Dict:
    """Test rapid-fire requests to simulate real usage"""
    print_section(f"Rapid Fire Test ({count} requests, {delay_ms}ms delay)")
    
    url = f"http://{host}/"
    session = requests.Session()
    
    # Login
    payload = {"type": "login", "user": "admin", "pass": "1234"}
    try:
        response = session.post(url, data=json.dumps(payload), timeout=5)
        data = response.json()
        if not data.get("success"):
            print(f"   [ERROR] Login failed")
            return {"error": "login failed"}
        token = data.get("token")
    except Exception as e:
        print(f"   [ERROR] {e}")
        return {"error": str(e)}
    
    # Rapid fire
    payload = {"type": "get_status", "token": token}
    latencies = []
    errors = []
    
    for i in range(count):
        try:
            start = time.time()
            response = session.post(url, data=json.dumps(payload), timeout=5)
            latency = (time.time() - start) * 1000
            
            if response.status_code == 200:
                latencies.append(latency)
            else:
                errors.append((i, "HTTP " + str(response.status_code)))
        except requests.exceptions.ConnectionError:
            errors.append((i, "connection_refused"))
        except requests.exceptions.Timeout:
            errors.append((i, "timeout"))
        except Exception as e:
            errors.append((i, str(type(e).__name__)))
        
        time.sleep(delay_ms / 1000.0)
    
    session.close()
    
    result = {
        "total": count,
        "success": len(latencies),
        "errors": len(errors),
        "success_rate": len(latencies) / count * 100 if count > 0 else 0,
        "avg_ms": statistics.mean(latencies) if latencies else 0,
        "min_ms": min(latencies) if latencies else 0,
        "max_ms": max(latencies) if latencies else 0,
        "stdev_ms": statistics.stdev(latencies) if len(latencies) > 1 else 0,
        "error_details": errors
    }
    
    print(f"   Success:         {result['success']}/{count} ({result['success_rate']:.1f}%)")
    print(f"   Errors:          {result['errors']}")
    print(f"   Avg Latency:     {result['avg_ms']:.1f}ms")
    print(f"   Min Latency:     {result['min_ms']:.1f}ms")
    print(f"   Max Latency:     {result['max_ms']:.1f}ms")
    print(f"   Std Dev:         {result['stdev_ms']:.1f}ms")
    
    if errors:
        print()
        print(f"   Error breakdown:")
        error_types = {}
        for idx, err in errors:
            error_types[err] = error_types.get(err, 0) + 1
        for err_type, cnt in error_types.items():
            print(f"     {err_type}: {cnt}")
    
    return result


# =============================================================================
# MAIN
# =============================================================================

def main():
    parser = argparse.ArgumentParser(description="Jetson Network Diagnostics for GG ProDino")
    parser.add_argument("--host", default=DEFAULT_HOST, help="Board IP address")
    args = parser.parse_args()
    
    print_header(f"Jetson Network Diagnostics v{VERSION}")
    print(f"   Target:          {args.host}")
    print(f"   Timestamp:       {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
    
    # Run all tests
    results = {}
    
    results["network_interface"] = test_network_interface()
    results["socket_options"] = test_socket_options()
    results["tcp_connection"] = test_tcp_connection(args.host)
    results["http_timing"] = test_http_request_timing(args.host)
    results["connection_reuse"] = test_connection_reuse(args.host)
    results["rapid_fire"] = test_rapid_fire(args.host)
    
    # Summary
    print_header("SUMMARY & RECOMMENDATIONS")
    
    print("\n[FINDINGS]")
    
    # Analyze results
    issues = []
    tips = []
    
    if results.get("network_interface", {}).get("type") == "USB Ethernet Adapter":
        issues.append("USB Ethernet adapter detected - may have higher latency than native")
        tips.append("Consider using a native Ethernet port if available")
    
    tcp_success = results.get("tcp_connection", {}).get("success", 0)
    tcp_attempts = results.get("tcp_connection", {}).get("attempts", 10)
    if tcp_success < tcp_attempts:
        issues.append(f"TCP connection failures: {tcp_attempts - tcp_success}/{tcp_attempts}")
    
    http_success_rate = results.get("http_timing", {}).get("success_rate", 0)
    if http_success_rate < 99:
        issues.append(f"HTTP success rate: {http_success_rate:.1f}%")
    
    rapid_success_rate = results.get("rapid_fire", {}).get("success_rate", 0)
    if rapid_success_rate < 95:
        issues.append(f"Rapid fire success rate: {rapid_success_rate:.1f}%")
        tips.append("Consider adding retry logic with exponential backoff")
    
    with_session_err = results.get("connection_reuse", {}).get("with_session", {}).get("errors", 0)
    without_session_err = results.get("connection_reuse", {}).get("without_session", {}).get("errors", 0)
    if without_session_err > with_session_err:
        tips.append("Use requests.Session() to reuse connections and reduce errors")
    
    if issues:
        for issue in issues:
            print(f"   [!] {issue}")
    else:
        print("   No significant issues detected")
    
    if tips:
        print("\n[RECOMMENDATIONS]")
        for tip in tips:
            print(f"   * {tip}")
    
    print()
    print("=" * 70)


if __name__ == "__main__":
    main()
