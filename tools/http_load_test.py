#!/usr/bin/env python3
"""
HTTP load test for the GRK board.
Hammers the board's HTTP API as fast as possible and reports:
  - average request rate + OK response rate
  - latency stats (min/avg/p50/p95/max)
  - the LONGEST "stuck" period (max time with NO response -> board frozen/blocked)
  - a timeline of every stuck / slow / failed request (with timestamps)

Usage:
    python3 http_load_test.py [BOARD_IP] [SECONDS] [ENDPOINT]
    e.g.  python3 http_load_test.py 192.168.1.198 30 get_imu

Notes:
  * Your laptop must be on the SAME LAN as the board (192.168.1.x) and its IP
    must be in the board's whitelist, or every request returns 403.
  * get_imu is the lightest endpoint; get_status is the heaviest (~1 KB response).
"""
import sys, time, json, statistics, urllib.request

BOARD    = sys.argv[1] if len(sys.argv) > 1 else "192.168.1.198"
DURATION = int(sys.argv[2]) if len(sys.argv) > 2 else 30
ENDPOINT = sys.argv[3] if len(sys.argv) > 3 else "get_imu"
USER, PW = "admin", "1234"
STUCK_MS = 500     # a gap longer than this (no response) counts as "stuck"

def post(payload, timeout=5):
    r = urllib.request.Request(f"http://{BOARD}/", data=json.dumps(payload).encode(),
                               headers={"Content-Type": "application/json"})
    with urllib.request.urlopen(r, timeout=timeout) as resp:
        return resp.read()

print(f"Target: http://{BOARD}/   endpoint={ENDPOINT}   duration={DURATION}s")
# Best-effort login. If it fails (board stuck), we still run and just hammer
# login requests - a timeout on those is itself the measurement ("stuck").
tok = None
try:
    tok = json.loads(post({"type": "login", "user": USER, "pass": PW}, timeout=3)).get("token")
    print("logged in OK; hammering", ENDPOINT)
except Exception as e:
    print(f"(initial login failed: {e}) -> hammering login requests and measuring stuck-time")
print()

def one_request():
    if tok:
        return post({"type": ENDPOINT, "token": tok}, timeout=3)
    return post({"type": "login", "user": USER, "pass": PW}, timeout=3)

lat, fails, events = [], 0, []
t_start = time.time()
last_ok = t_start
max_gap = 0.0
n = 0
while time.time() - t_start < DURATION:
    t0 = time.time()
    try:
        one_request()
        ms = (time.time() - t0) * 1000.0
        lat.append(ms)
        gap = time.time() - last_ok
        if gap > max_gap: max_gap = gap
        if gap * 1000 > STUCK_MS:
            events.append((t0 - t_start, "STUCK", f"{gap*1000:.0f} ms with NO response"))
        if ms > STUCK_MS:
            events.append((t0 - t_start, "SLOW", f"{ms:.0f} ms response"))
        last_ok = time.time()
    except Exception as e:
        fails += 1
        events.append((time.time() - t_start, "FAIL", type(e).__name__ + ": " + str(e)[:40]))
    n += 1

wall = time.time() - t_start
ok = len(lat)
print("=" * 64)
print(f"duration           : {wall:6.1f} s")
print(f"requests sent      : {n}")
print(f"OK responses       : {ok}   ({100*ok/max(n,1):.1f}%)")
print(f"failed / timeout   : {fails}   ({100*fails/max(n,1):.1f}%)")
print(f"avg request rate   : {n/wall:6.1f} req/s")
print(f"OK response rate   : {ok/wall:6.1f} resp/s")
if lat:
    lat.sort()
    print(f"latency (ms)       : min={lat[0]:.0f}  avg={statistics.mean(lat):.0f}  "
          f"p50={lat[len(lat)//2]:.0f}  p95={lat[int(len(lat)*0.95)]:.0f}  max={lat[-1]:.0f}")
print(f"LONGEST STUCK      : {max_gap*1000:.0f} ms with NO response  <-- board blocked/frozen here")
print("=" * 64)
print(f"stuck/slow/fail events (>{STUCK_MS} ms), time since start:")
for t, kind, d in events[:50]:
    print(f"  t={t:7.2f}s  {kind:5}  {d}")
if len(events) > 50:
    print(f"  ... and {len(events)-50} more events")
if not events:
    print("  (none - board stayed responsive the whole time)")
