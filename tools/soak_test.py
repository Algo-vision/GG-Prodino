#!/usr/bin/env python3
"""
Long-duration soak test: HTTP consumers against the board, plus the board's
encrypted telemetry reaching the cloud server.

Writes one summary block per window (default 15 min) to a log file AND to the
console, then a cumulative summary at the end. Safe to leave running for hours:
latencies are kept as a histogram, not a list, so memory does not grow.

    python3 tools/soak_test.py                       # 10 h, 1 consumer @ 20 Hz
    python3 tools/soak_test.py --hours 10 --consumers 2 --hz 20
    python3 tools/soak_test.py --hours 0.1 --window 2 # quick shakedown

Telemetry is checked by reading the sequence number the server last accepted
from this board: the increase across a window is how many packets it received.
That needs SSH access to the server; pass --no-telemetry to skip it, and the
HTTP side still runs.

Stop early with Ctrl+C - it prints the cumulative summary before exiting.
"""
import argparse, json, os, re, statistics, subprocess, sys, threading, time
import urllib.request, urllib.error
from datetime import datetime

ap = argparse.ArgumentParser(description="Board soak test: HTTP consumers + cloud telemetry")
ap.add_argument("--board", default="192.168.1.198", help="board IP")
ap.add_argument("--hours", type=float, default=10.0, help="total duration in hours")
ap.add_argument("--window", type=float, default=15.0, help="minutes per summary block")
ap.add_argument("--consumers", type=int, default=1, help="parallel HTTP consumers")
ap.add_argument("--hz", type=float, default=20.0, help="poll rate per consumer")
ap.add_argument("--endpoint", default="get_status", help="board API command to poll")
ap.add_argument("--user", default="admin")
ap.add_argument("--password", default="1234")
ap.add_argument("--log", default=None, help="log file (default soak_<timestamp>.log)")
ap.add_argument("--ec2", default="16.171.11.151", help="cloud server host")
ap.add_argument("--ssh-key", default="keys/instance_gg_key.pem")
ap.add_argument("--ssh-user", default="ubuntu")
ap.add_argument("--serial", default="SN2003", help="board serial, for the telemetry check")
ap.add_argument("--telemetry-interval", type=float, default=60.0,
                help="seconds between telemetry sends, to compute how many were expected")
ap.add_argument("--no-telemetry", action="store_true", help="skip the cloud check")
args = ap.parse_args()

LOG = args.log or f"soak_{datetime.now():%Y%m%d_%H%M}.log"
PERIOD = 1.0 / args.hz
STOP = threading.Event()
LK = threading.Lock()

# ---- shared counters, reset each window ------------------------------------
def new_bucket():
    return {"sent": 0, "ok": 0, "fail": 0, "hist": {}, "errors": {},
            "lat_min": None, "lat_max": 0.0, "lat_sum": 0.0,
            "worst_gap": 0.0}

win = new_bucket()      # current window
tot = new_bucket()      # whole run
last_ok_time = [time.time()]


def record(latency_ms, ok, err=None):
    with LK:
        for b in (win, tot):
            b["sent"] += 1
            if ok:
                b["ok"] += 1
                ms = int(latency_ms)
                b["hist"][ms] = b["hist"].get(ms, 0) + 1
                b["lat_sum"] += latency_ms
                if b["lat_min"] is None or latency_ms < b["lat_min"]: b["lat_min"] = latency_ms
                if latency_ms > b["lat_max"]: b["lat_max"] = latency_ms
            else:
                b["fail"] += 1
                if err: b["errors"][err] = b["errors"].get(err, 0) + 1
        if ok:
            now = time.time()
            gap = now - last_ok_time[0]
            for b in (win, tot):
                if gap > b["worst_gap"]: b["worst_gap"] = gap
            last_ok_time[0] = now


def pct(hist, q):
    """Percentile straight out of the histogram - no list of samples kept."""
    n = sum(hist.values())
    if not n: return 0
    target, seen = n * q, 0
    for ms in sorted(hist):
        seen += hist[ms]
        if seen >= target: return ms
    return max(hist)


# ---- the consumers ---------------------------------------------------------
def post(payload, timeout=5):
    r = urllib.request.Request(f"http://{args.board}/", data=json.dumps(payload).encode(),
                               headers={"Content-Type": "application/json"})
    with urllib.request.urlopen(r, timeout=timeout) as resp:
        return resp.read()


def consumer():
    tok = None
    for _ in range(40):                       # the board may refuse while busy
        if STOP.is_set(): return
        try:
            tok = json.loads(post({"type": "login", "user": args.user,
                                   "pass": args.password}, timeout=5)).get("token")
            if tok: break
        except Exception:
            pass
        time.sleep(0.25)

    nxt = time.time()
    while not STOP.is_set():
        now = time.time()
        if now < nxt:
            time.sleep(min(nxt - now, 0.02)); continue
        if now - nxt > PERIOD: nxt = now       # fell behind: skip, don't backlog
        nxt += PERIOD

        t0 = time.time()
        try:
            post({"type": args.endpoint, "token": tok})
            record((time.time() - t0) * 1000, True)
        except Exception as e:
            reason = str(getattr(e, "reason", e))[:44] or type(e).__name__
            record(0, False, reason)
            if "Unauthorized" in reason or "401" in reason:   # session expired
                try:
                    tok = json.loads(post({"type": "login", "user": args.user,
                                           "pass": args.password})).get("token")
                except Exception:
                    pass


# ---- telemetry: how many packets did the server accept? --------------------
def server_seq():
    """Latest accepted sequence number for this board, or None."""
    if args.no_telemetry: return None
    cmd = ["ssh", "-o", "StrictHostKeyChecking=no", "-o", "ConnectTimeout=15",
           "-i", args.ssh_key, f"{args.ssh_user}@{args.ec2}",
           f"pm2 logs grk-backend --lines 400 --nostream 2>&1 | grep '{args.serial}' | grep -o 'seq=[0-9]*' | tail -1"]
    try:
        out = subprocess.run(cmd, capture_output=True, text=True, timeout=60).stdout.strip()
        m = re.search(r"seq=(\d+)", out)
        return int(m.group(1)) if m else None
    except Exception:
        return None


# ---- reporting -------------------------------------------------------------
def emit(line=""):
    print(line, flush=True)
    with open(LOG, "a") as f:
        f.write(line + "\n")


def summarise(b, label, seconds, tele):
    n = max(b["sent"], 1)
    rate = b["ok"] / seconds if seconds else 0
    emit(f"  requests     sent {b['sent']}  ok {b['ok']}  failed {b['fail']}"
         f"   fail rate {100*b['fail']/n:.2f}%")
    emit(f"  throughput   {rate:.1f} req/s answered"
         f"   (asked for {args.consumers * args.hz:g}/s)")
    if b["ok"]:
        emit(f"  latency ms   min={b['lat_min']:.0f} p50={pct(b['hist'],.5)}"
             f" p95={pct(b['hist'],.95)} p99={pct(b['hist'],.99)}"
             f" max={b['lat_max']:.0f} avg={b['lat_sum']/b['ok']:.0f}")
    emit(f"  worst gap    {b['worst_gap']*1000:.0f} ms with no consumer answered")
    if b["errors"]:
        emit(f"  errors       {b['errors']}")
    if tele is not None:
        got, expected = tele
        # one short is tolerable (a send can straddle the window boundary);
        # nothing arriving is never OK.
        mark = "OK" if got >= max(1, expected - 1) else "SHORT"
        emit(f"  telemetry    {got} packets accepted by the server"
             f" (expected ~{expected})   {mark}")
    elif not args.no_telemetry:
        emit("  telemetry    could not read the server (SSH failed) - HTTP results still valid")


# ---- run -------------------------------------------------------------------
started = datetime.now()
emit("=" * 72)
emit(f"SOAK TEST  started {started:%Y-%m-%d %H:%M:%S}")
emit(f"  board {args.board}   {args.consumers} consumer(s) x {args.hz:g} Hz on {args.endpoint}")
emit(f"  duration {args.hours} h   window {args.window} min   log {LOG}")
if not args.no_telemetry:
    emit(f"  telemetry via {args.ssh_user}@{args.ec2}, serial {args.serial},"
         f" expecting one send per {args.telemetry_interval:g} s")
emit("=" * 72)

threads = [threading.Thread(target=consumer, daemon=True) for _ in range(args.consumers)]
for t in threads: t.start()

t_start = time.time()
t_end = t_start + args.hours * 3600
win_secs = args.window * 60
seq_prev = server_seq()
tele_total = [0, 0]      # [accepted, expected] across the whole run
win_no = 0

try:
    while time.time() < t_end:
        target = min(time.time() + win_secs, t_end)
        while time.time() < target and not STOP.is_set():
            time.sleep(1)

        win_no += 1
        elapsed = time.time() - t_start
        seq_now = server_seq()
        tele = None
        if seq_now is not None and seq_prev is not None:
            got = max(seq_now - seq_prev, 0)
            tele = (got, int(win_secs / args.telemetry_interval))
            tele_total[0] += got
            tele_total[1] += int(win_secs / args.telemetry_interval)
        seq_prev = seq_now if seq_now is not None else seq_prev

        with LK:
            snapshot, win_reset = dict(win), new_bucket()
            snapshot["hist"] = dict(win["hist"]); snapshot["errors"] = dict(win["errors"])
            win.clear(); win.update(win_reset)

        emit("")
        emit(f"--- window {win_no}  [{datetime.now():%H:%M:%S}]"
             f"  t+{elapsed/3600:.2f} h ---")
        summarise(snapshot, "window", win_secs, tele)
except KeyboardInterrupt:
    emit("\n(interrupted - printing totals)")

STOP.set()
for t in threads: t.join(timeout=5)

total_secs = time.time() - t_start
emit("")
emit("=" * 72)
emit(f"CUMULATIVE over {total_secs/3600:.2f} h   finished {datetime.now():%Y-%m-%d %H:%M:%S}")
emit("=" * 72)
summarise(tot, "total", total_secs,
          tuple(tele_total) if tele_total[1] else None)
emit("")
emit(f"log written to {os.path.abspath(LOG)}")
