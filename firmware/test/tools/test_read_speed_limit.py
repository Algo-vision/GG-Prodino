#!/usr/bin/env python3
"""
Read Speed Limit Test
Tests how fast data can be read from the board until it hits its limit.
Progressively increases request frequency and measures response times.

Usage:
    python test_read_speed_limit.py [board_ip]
    
Example:
    python test_read_speed_limit.py 192.168.1.198
"""

import requests
import json
import time
import threading
import statistics
import argparse
from datetime import datetime
from dataclasses import dataclass, field
from typing import List, Optional
from concurrent.futures import ThreadPoolExecutor, as_completed


@dataclass
class RequestResult:
    """Result of a single request"""
    success: bool
    response_time_ms: float
    timestamp: float
    error: Optional[str] = None


@dataclass
class SpeedTestResult:
    """Result of a speed test at a given rate"""
    requests_per_second: float
    total_requests: int
    successful_requests: int
    failed_requests: int
    avg_response_ms: float
    min_response_ms: float
    max_response_ms: float
    p50_response_ms: float
    p95_response_ms: float
    p99_response_ms: float
    success_rate: float
    actual_rps: float  # Actual achieved requests per second
    
    def is_healthy(self, min_success_rate: float = 95.0, max_avg_response_ms: float = 500) -> bool:
        """Check if this test result is within acceptable bounds"""
        return self.success_rate >= min_success_rate and self.avg_response_ms <= max_avg_response_ms


class BoardSpeedTester:
    """Tests read speed limits of the board"""
    
    def __init__(self, board_ip: str, username: str = "admin", password: str = "1234"):
        self.board_url = f"http://{board_ip}/"
        self.username = username
        self.password = password
        self.token = None
        self.session = requests.Session()
        
    def login(self) -> bool:
        """Login and obtain authentication token"""
        payload = {"type": "login", "user": self.username, "pass": self.password}
        
        try:
            response = self.session.post(self.board_url, data=json.dumps(payload), timeout=5)
            if response.status_code == 200:
                data = response.json()
                if data.get("success"):
                    self.token = data.get("token")
                    print(f"✅ Login successful (Token: {self.token[:8]}...)")
                    return True
                else:
                    print(f"❌ Login failed: {data.get('error', 'Unknown error')}")
            else:
                print(f"❌ HTTP {response.status_code}")
            return False
        except Exception as e:
            print(f"❌ Login error: {e}")
            return False
    
    def single_request(self) -> RequestResult:
        """Make a single status request and measure response time"""
        if not self.token:
            return RequestResult(False, 0, time.time(), "No token")
        
        payload = {"type": "get_status", "token": self.token}
        start_time = time.time()
        
        try:
            response = self.session.post(self.board_url, data=json.dumps(payload), timeout=5)
            end_time = time.time()
            response_time_ms = (end_time - start_time) * 1000
            
            if response.status_code == 200:
                data = response.json()
                if "error" in data:
                    return RequestResult(False, response_time_ms, start_time, data.get("error"))
                return RequestResult(True, response_time_ms, start_time)
            else:
                return RequestResult(False, response_time_ms, start_time, f"HTTP {response.status_code}")
        except requests.exceptions.Timeout:
            return RequestResult(False, 5000, start_time, "Timeout")
        except Exception as e:
            return RequestResult(False, 0, start_time, str(e))
    
    def run_speed_test(self, target_rps: float, duration_seconds: float = 5.0) -> SpeedTestResult:
        """Run a speed test at a target requests per second rate"""
        interval = 1.0 / target_rps
        results: List[RequestResult] = []
        
        start_time = time.time()
        next_request_time = start_time
        
        while (time.time() - start_time) < duration_seconds:
            # Wait until next scheduled request
            now = time.time()
            if now < next_request_time:
                time.sleep(max(0, next_request_time - now))
            
            # Make request
            result = self.single_request()
            results.append(result)
            
            # Schedule next request
            next_request_time += interval
        
        end_time = time.time()
        actual_duration = end_time - start_time
        
        return self._calculate_results(results, target_rps, actual_duration)
    
    def run_concurrent_speed_test(self, target_rps: float, duration_seconds: float = 5.0, 
                                   max_concurrent: int = 10) -> SpeedTestResult:
        """Run a speed test with concurrent requests"""
        results: List[RequestResult] = []
        results_lock = threading.Lock()
        stop_event = threading.Event()
        
        # Calculate workers needed and per-worker delay
        workers_needed = min(max_concurrent, max(1, int(target_rps / 5) + 1))
        per_worker_rps = target_rps / workers_needed
        per_worker_delay = 1.0 / per_worker_rps if per_worker_rps > 0 else 1.0
        
        def worker(worker_delay: float):
            while not stop_event.is_set():
                result = self.single_request()
                with results_lock:
                    results.append(result)
                # Each worker contributes its share of the total RPS
                time.sleep(max(0.01, worker_delay - (result.response_time_ms / 1000)))
        
        # Start workers
        threads = []
        
        start_time = time.time()
        for _ in range(workers_needed):
            t = threading.Thread(target=worker, args=(per_worker_delay,), daemon=True)
            t.start()
            threads.append(t)
        
        # Wait for duration
        time.sleep(duration_seconds)
        stop_event.set()
        
        # Wait for threads to finish
        for t in threads:
            t.join(timeout=1.0)
        
        end_time = time.time()
        actual_duration = end_time - start_time
        
        return self._calculate_results(results, target_rps, actual_duration)
    
    def _calculate_results(self, results: List[RequestResult], target_rps: float, 
                           actual_duration: float) -> SpeedTestResult:
        """Calculate statistics from results"""
        if not results:
            return SpeedTestResult(
                requests_per_second=target_rps,
                total_requests=0,
                successful_requests=0,
                failed_requests=0,
                avg_response_ms=0,
                min_response_ms=0,
                max_response_ms=0,
                p50_response_ms=0,
                p95_response_ms=0,
                p99_response_ms=0,
                success_rate=0,
                actual_rps=0
            )
        
        successful = [r for r in results if r.success]
        failed = [r for r in results if not r.success]
        
        response_times = [r.response_time_ms for r in successful] if successful else [0]
        response_times_sorted = sorted(response_times)
        
        def percentile(data: List[float], p: float) -> float:
            if not data:
                return 0
            k = (len(data) - 1) * (p / 100)
            f = int(k)
            c = f + 1 if f + 1 < len(data) else f
            return data[f] + (data[c] - data[f]) * (k - f)
        
        return SpeedTestResult(
            requests_per_second=target_rps,
            total_requests=len(results),
            successful_requests=len(successful),
            failed_requests=len(failed),
            avg_response_ms=statistics.mean(response_times) if response_times else 0,
            min_response_ms=min(response_times) if response_times else 0,
            max_response_ms=max(response_times) if response_times else 0,
            p50_response_ms=percentile(response_times_sorted, 50),
            p95_response_ms=percentile(response_times_sorted, 95),
            p99_response_ms=percentile(response_times_sorted, 99),
            success_rate=(len(successful) / len(results) * 100) if results else 0,
            actual_rps=len(results) / actual_duration if actual_duration > 0 else 0
        )
    
    def find_speed_limit(self, 
                         start_rps: float = 1.0,
                         max_rps: float = 100.0,
                         step_multiplier: float = 1.5,
                         test_duration: float = 5.0,
                         min_success_rate: float = 95.0,
                         max_avg_response_ms: float = 500.0,
                         use_concurrent: bool = True) -> dict:
        """
        Find the maximum sustainable request rate.
        Increases rate until success rate drops or response times become too high.
        """
        print("\n" + "="*70)
        print("  READ SPEED LIMIT TEST")
        print("="*70)
        print(f"\n🎯 Target Board: {self.board_url}")
        print(f"⏱️  Test duration per rate: {test_duration}s")
        print(f"📊 Success threshold: ≥{min_success_rate}%")
        print(f"⚡ Max avg response: ≤{max_avg_response_ms}ms")
        print(f"📅 Started: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}\n")
        
        all_results: List[SpeedTestResult] = []
        current_rps = start_rps
        last_healthy_result: Optional[SpeedTestResult] = None
        
        while current_rps <= max_rps:
            print(f"\n{'─'*60}")
            print(f"🔄 Testing at {current_rps:.1f} requests/second...")
            
            if use_concurrent and current_rps > 5:
                result = self.run_concurrent_speed_test(current_rps, test_duration)
            else:
                result = self.run_speed_test(current_rps, test_duration)
            
            all_results.append(result)
            
            # Print results
            status = "✅" if result.is_healthy(min_success_rate, max_avg_response_ms) else "⚠️ "
            print(f"{status} Target: {current_rps:.1f} RPS | Actual: {result.actual_rps:.1f} RPS")
            print(f"   📈 Requests: {result.successful_requests}/{result.total_requests} "
                  f"({result.success_rate:.1f}% success)")
            print(f"   ⏱️  Response times: avg={result.avg_response_ms:.1f}ms, "
                  f"p50={result.p50_response_ms:.1f}ms, p95={result.p95_response_ms:.1f}ms, "
                  f"p99={result.p99_response_ms:.1f}ms")
            print(f"   📉 Min/Max: {result.min_response_ms:.1f}ms / {result.max_response_ms:.1f}ms")
            
            if result.failed_requests > 0:
                print(f"   ❌ Failures: {result.failed_requests}")
            
            if result.is_healthy(min_success_rate, max_avg_response_ms):
                last_healthy_result = result
                current_rps *= step_multiplier
            else:
                print(f"\n⚠️  Performance degraded at {current_rps:.1f} RPS")
                break
        
        # Final report
        print("\n" + "="*70)
        print("  FINAL RESULTS")
        print("="*70)
        
        if last_healthy_result:
            print(f"\n🏆 Maximum Sustainable Rate: {last_healthy_result.actual_rps:.1f} RPS")
            print(f"   Target was: {last_healthy_result.requests_per_second:.1f} RPS")
            print(f"   Success rate: {last_healthy_result.success_rate:.1f}%")
            print(f"   Avg response: {last_healthy_result.avg_response_ms:.1f}ms")
            print(f"   P95 response: {last_healthy_result.p95_response_ms:.1f}ms")
        else:
            print("\n❌ No healthy rate found. Board may be overloaded or unreachable.")
        
        # Summary table
        print(f"\n📊 All Test Results:")
        print(f"{'─'*70}")
        print(f"{'Target RPS':>12} | {'Actual RPS':>10} | {'Success':>8} | {'Avg (ms)':>9} | {'P95 (ms)':>9}")
        print(f"{'─'*70}")
        for r in all_results:
            marker = "→" if (last_healthy_result and r.requests_per_second == last_healthy_result.requests_per_second) else " "
            print(f"{marker}{r.requests_per_second:>11.1f} | {r.actual_rps:>10.1f} | "
                  f"{r.success_rate:>7.1f}% | {r.avg_response_ms:>9.1f} | {r.p95_response_ms:>9.1f}")
        
        return {
            "max_sustainable_rps": last_healthy_result.actual_rps if last_healthy_result else 0,
            "max_target_rps": last_healthy_result.requests_per_second if last_healthy_result else 0,
            "all_results": all_results,
            "last_healthy": last_healthy_result
        }


def run_burst_test(tester: BoardSpeedTester, burst_size: int = 50) -> dict:
    """
    Run a burst test - send many requests as fast as possible.
    Measures raw throughput without rate limiting.
    """
    print("\n" + "="*70)
    print("  BURST TEST (Maximum Throughput)")
    print("="*70)
    print(f"\n🚀 Sending {burst_size} requests as fast as possible...\n")
    
    results: List[RequestResult] = []
    start_time = time.time()
    
    for i in range(burst_size):
        result = tester.single_request()
        results.append(result)
        
        # Progress indicator
        if (i + 1) % 10 == 0:
            elapsed = time.time() - start_time
            current_rps = (i + 1) / elapsed if elapsed > 0 else 0
            print(f"   Progress: {i+1}/{burst_size} ({current_rps:.1f} req/s)")
    
    end_time = time.time()
    duration = end_time - start_time
    
    successful = sum(1 for r in results if r.success)
    failed = sum(1 for r in results if not r.success)
    response_times = [r.response_time_ms for r in results if r.success]
    
    print(f"\n📊 Burst Test Results:")
    print(f"   Total requests: {burst_size}")
    print(f"   Successful: {successful} ({successful/burst_size*100:.1f}%)")
    print(f"   Failed: {failed}")
    print(f"   Duration: {duration:.2f}s")
    print(f"   Throughput: {burst_size/duration:.1f} requests/second")
    
    if response_times:
        print(f"   Avg response: {statistics.mean(response_times):.1f}ms")
        print(f"   Min response: {min(response_times):.1f}ms")
        print(f"   Max response: {max(response_times):.1f}ms")
    
    return {
        "total": burst_size,
        "successful": successful,
        "failed": failed,
        "duration": duration,
        "throughput_rps": burst_size / duration if duration > 0 else 0
    }


def run_sustained_test(tester: BoardSpeedTester, target_rps: float, duration: float = 30.0) -> dict:
    """
    Run a sustained load test at a fixed rate for a longer duration.
    Useful for stability testing.
    """
    print("\n" + "="*70)
    print("  SUSTAINED LOAD TEST")
    print("="*70)
    print(f"\n📊 Target: {target_rps} RPS for {duration} seconds\n")
    
    result = tester.run_concurrent_speed_test(target_rps, duration, max_concurrent=20)
    
    print(f"\n📊 Sustained Test Results:")
    print(f"   Duration: {duration}s at {target_rps} target RPS")
    print(f"   Actual RPS: {result.actual_rps:.1f}")
    print(f"   Total requests: {result.total_requests}")
    print(f"   Success rate: {result.success_rate:.1f}%")
    print(f"   Avg response: {result.avg_response_ms:.1f}ms")
    print(f"   P95 response: {result.p95_response_ms:.1f}ms")
    print(f"   P99 response: {result.p99_response_ms:.1f}ms")
    
    return {
        "target_rps": target_rps,
        "actual_rps": result.actual_rps,
        "success_rate": result.success_rate,
        "avg_response_ms": result.avg_response_ms
    }


def main():
    parser = argparse.ArgumentParser(description="Test board read speed limits")
    parser.add_argument("board_ip", nargs="?", default="192.168.1.198",
                        help="Board IP address (default: 192.168.1.198)")
    parser.add_argument("--user", "-u", default="admin", help="Username")
    parser.add_argument("--password", "-p", default="1234", help="Password")
    parser.add_argument("--start-rps", type=float, default=1.0, 
                        help="Starting requests per second (default: 1)")
    parser.add_argument("--max-rps", type=float, default=50.0,
                        help="Maximum requests per second to test (default: 50)")
    parser.add_argument("--duration", type=float, default=5.0,
                        help="Duration per test in seconds (default: 5)")
    parser.add_argument("--burst", action="store_true",
                        help="Also run burst test")
    parser.add_argument("--burst-size", type=int, default=100,
                        help="Number of requests in burst test (default: 100)")
    parser.add_argument("--sustained", type=float, default=0,
                        help="Run sustained test at this RPS (default: off)")
    parser.add_argument("--sustained-duration", type=float, default=30,
                        help="Duration of sustained test (default: 30s)")
    
    args = parser.parse_args()
    
    print("\n" + "="*70)
    print("  BOARD READ SPEED LIMIT TESTER")
    print("="*70)
    print(f"\n🎯 Board: http://{args.board_ip}/")
    print(f"👤 User: {args.user}")
    
    tester = BoardSpeedTester(args.board_ip, args.user, args.password)
    
    print("\n🔐 Logging in...")
    if not tester.login():
        print("❌ Failed to login. Exiting.")
        return
    
    # Run main speed limit test
    results = tester.find_speed_limit(
        start_rps=args.start_rps,
        max_rps=args.max_rps,
        test_duration=args.duration
    )
    
    # Optional burst test
    if args.burst:
        run_burst_test(tester, args.burst_size)
    
    # Optional sustained test
    if args.sustained > 0:
        run_sustained_test(tester, args.sustained, args.sustained_duration)
    
    print("\n" + "="*70)
    print("  TEST COMPLETE")
    print("="*70)
    
    max_rps = results.get("max_sustainable_rps", 0)
    if max_rps > 0:
        print(f"\n🏆 Maximum sustainable read rate: {max_rps:.1f} requests/second")
        print(f"   That's approximately {max_rps * 60:.0f} reads/minute")
        print(f"   Or {max_rps * 3600:.0f} reads/hour")
    else:
        print("\n⚠️  Could not determine maximum sustainable rate")
    
    print()


if __name__ == "__main__":
    main()
