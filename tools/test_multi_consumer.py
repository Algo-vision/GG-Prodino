#!/usr/bin/env python3
"""
Several real-time consumers polling the board at a fixed rate, all at once.

Unlike http_load_test.py (one client, as fast as it can), this models what
actually happens in the field: N independent consumers - a Jetson, a technician
GUI, a logger - each polling at its own steady frequency while the board also
sends telemetry to the cloud.

It answers:
  - what fail rate does each consumer see?
  - what is the longest window where NO consumer got an answer? (the board was
    busy or wedged - this is the number a real-time consumer actually feels)
  - do the consumers interfere with each other? (the W5500 has only 8 sockets
    and the board opens one per request)

Usage:
    python3 tools/test_multi_consumer.py [BOARD_IP] [SECONDS] [CONSUMERS] [HZ] [ENDPOINT]
    python3 tools/test_multi_consumer.py 192.168.1.198 900 3 20 get_status
"""
import json, statistics, sys, threading, time, urllib.request, urllib.error

BOARD    = sys.argv[1] if len(sys.argv) > 1 else "192.168.1.198"
DURATION = int(sys.argv[2]) if len(sys.argv) > 2 else 900
NCONS    = int(sys.argv[3]) if len(sys.argv) > 3 else 3
HZ       = float(sys.argv[4]) if len(sys.argv) > 4 else 20.0
ENDPOINT = sys.argv[5] if len(sys.argv) > 5 else "get_status"
USER, PW = "admin", "1234"
PERIOD   = 1.0 / HZ

results = {}
lock = threading.Lock()
last_any_ok = [None]          # wall time of the last successful response, any consumer
worst_global = [0.0, 0.0]     # [gap_seconds, when]


def post(payload, timeout=5):
    r = urllib.request.Request(f"http://{BOARD}/", data=json.dumps(payload).encode(),
                               headers={"Content-Type": "application/json"})
    with urllib.request.urlopen(r, timeout=timeout) as resp:
        return resp.read()


def consumer(idx, stop_at):
    stats = {"sent": 0, "ok": 0, "fail": 0, "lat": [], "errors": {},
             "max_own_gap": 0.0, "late": 0}
    try:
        tok = json.loads(post({"type": "login", "user": USER, "pass": PW}, timeout=5)).get("token")
    except Exception as e:
        tok = None
        stats["errors"]["login"] = str(e)[:40]

    last_ok = time.time()
    nxt = time.time()
    while time.time() < stop_at:
        now = time.time()
        if now < nxt:
            time.sleep(min(nxt - now, 0.02))
            continue
        # A consumer that falls behind its cadence skips ahead rather than
        # queueing up a backlog - that is what a real 20 Hz loop does.
        if now - nxt > PERIOD:
            stats["late"] += 1
            nxt = now
        nxt += PERIOD

        t0 = time.time()
        stats["sent"] += 1
        try:
            post({"type": ENDPOINT, "token": tok}, timeout=5)
            dt = (time.time() - t0) * 1000
            stats["ok"] += 1
            stats["lat"].append(dt)
            gap = time.time() - last_ok
            if gap > stats["max_own_gap"]:
                stats["max_own_gap"] = gap
            last_ok = time.time()
            with lock:
                if last_any_ok[0] is not None:
                    g = time.time() - last_any_ok[0]
                    if g > worst_global[0]:
                        worst_global[0] = g
                        worst_global[1] = time.time()
                last_any_ok[0] = time.time()
        except Exception as e:
            stats["fail"] += 1
            k = type(e).__name__
            if isinstance(e, urllib.error.URLError):
                k = str(e.reason)[:44]
            stats["errors"][k] = stats["errors"].get(k, 0) + 1

    with lock:
        results[idx] = stats


print(f"{NCONS} consumers x {HZ:g} Hz on http://{BOARD}/  endpoint={ENDPOINT}")
print(f"target {NCONS * HZ:g} req/s total, for {DURATION}s ({DURATION/60:.0f} min)\n")

start = time.time()
last_any_ok[0] = start
stop_at = start + DURATION
threads = [threading.Thread(target=consumer, args=(i, stop_at), daemon=True)
           for i in range(NCONS)]
for t in threads:
    t.start()

# progress, so a 15 minute run is not a black box
while any(t.is_alive() for t in threads):
    time.sleep(30)
    el = time.time() - start
    with lock:
        wg = worst_global[0]
    print(f"  [{el/60:5.1f} min] worst all-consumer gap so far: {wg*1000:.0f} ms", flush=True)
for t in threads:
    t.join()

elapsed = time.time() - start
print("\n" + "=" * 66)
print(f"{NCONS} consumers x {HZ:g} Hz   duration {elapsed:.0f}s")
print("=" * 66)

tot_sent = tot_ok = tot_fail = 0
all_lat = []
for i in sorted(results):
    s = results[i]
    tot_sent += s["sent"]; tot_ok += s["ok"]; tot_fail += s["fail"]
    all_lat += s["lat"]
    lat = sorted(s["lat"])
    p = lambda q: lat[int(len(lat) * q)] if lat else 0
    rate = s["ok"] / elapsed
    print(f"\nconsumer #{i+1}")
    print(f"   sent {s['sent']}   ok {s['ok']}   failed {s['fail']}"
          f"   fail rate {100*s['fail']/max(s['sent'],1):.2f}%")
    print(f"   achieved {rate:.1f} req/s   latency min={min(lat) if lat else 0:.0f}"
          f" p50={p(.5):.0f} p95={p(.95):.0f} p99={p(.99):.0f} max={max(lat) if lat else 0:.0f} ms")
    print(f"   longest gap between its own answers: {s['max_own_gap']*1000:.0f} ms")
    if s["errors"]:
        print(f"   errors: {s['errors']}")

lat = sorted(all_lat)
p = lambda q: lat[int(len(lat) * q)] if lat else 0
print("\n" + "-" * 66)
print(f"TOTAL      sent {tot_sent}   ok {tot_ok}   failed {tot_fail}"
      f"   fail rate {100*tot_fail/max(tot_sent,1):.2f}%")
print(f"           {tot_ok/elapsed:.1f} req/s answered across all consumers")
print(f"           latency p50={p(.5):.0f} p95={p(.95):.0f} p99={p(.99):.0f} "
      f"max={max(lat) if lat else 0:.0f} ms")
print(f"WORST GAP  {worst_global[0]*1000:.0f} ms with NO consumer getting an answer"
      f"  (at t+{worst_global[1]-start:.0f}s)")
print("-" * 66)
