#!/usr/bin/env python3
"""
P1 dt stress test driver for GG-GRK V1.5.1.1 (firmware env:timing).

Answers Ron's Priority 1 question: what is dt in the angle formula at the
controller's lightest and heaviest load.

Arms the on-board probe, applies a chosen load for the window, then reads the
FROZEN result - the board stops accumulating when the window expires, so the
readout request is never part of what was measured.

  Scenario A (floor):    ./dt_stress_test.py --load idle
  Scenario B (loaded):   ./dt_stress_test.py --load poll

Needs a board flashed with `pio run -e timing -t upload`; env:main has no
probe and answers "unknown request type".

Close the USB serial monitor before running either. statusWriteToSerial()
skips itself when no host has the port open, so an unopened port already means
"no serial printing" - but an OPEN monitor silently adds ~55 lines/s of
formatting to every result.

The board closes the TCP connection after each request, so every poll is a
fresh connection - that is what a real consumer costs it, not just the JSON.
"""

import argparse
import http.client
import json
import sys
import time


def rpc(host, port, payload, timeout=5.0):
    conn = http.client.HTTPConnection(host, port, timeout=timeout)
    try:
        body = json.dumps(payload)
        conn.request("POST", "/", body,
                     {"Content-Type": "application/json",
                      "Content-Length": str(len(body))})
        resp = conn.getresponse()
        return json.loads(resp.read().decode("utf-8", "replace"))
    finally:
        conn.close()


def login(host, port, user, password):
    r = rpc(host, port, {"type": "login", "user": user, "pass": password})
    if not r.get("success"):
        sys.exit("login failed: %s %s" % (r.get("code"), r.get("message")))
    return r["token"]


def fmt_us(us):
    return "%8.2f ms" % (us / 1000.0)


def report(r, polls, elapsed):
    dt = r["dt"]
    print()
    print("=" * 62)
    print("  state=%s  window=%ss  elapsed=%sms" %
          (r["state"], r["windowS"], r["elapsedMs"]))
    print("=" * 62)
    print("  filter updates : %d" % dt["count"])
    print("  effective rate : %.2f Hz      (scheduler asks 100, IMUs run 104)" % dt["hz"])
    print("  dt min         : %s" % fmt_us(dt["minUs"]))
    print("  dt mean        : %s" % fmt_us(dt["meanUs"]))
    print("  dt max         : %s" % fmt_us(dt["maxUs"]))
    print("  clamp hits     : %d      (dt over 100ms - filter under-integrates)"
          % dt["clampHits"])
    if polls is not None:
        print("  client polls   : %d in %.1fs = %.1f req/s" %
              (polls, elapsed, polls / elapsed if elapsed else 0))

    print()
    print("  dt distribution")
    edges, buckets = dt["edgesUs"], dt["buckets"]
    total = max(sum(buckets), 1)
    lo = 0
    for e, n in zip(edges, buckets):
        label = ("  >= %5.1f ms       " % (lo / 1000.0) if e == 0
                 else "  %5.1f - %5.1f ms  " % (lo / 1000.0, e / 1000.0))
        print("%s %7d  %5.1f%%  %s" %
              (label, n, 100.0 * n / total, "#" * int(40.0 * n / total)))
        lo = e

    print()
    print("  where the time goes        calls      mean        max     total")
    print("  " + "-" * 58)
    for name, b in sorted(r["blocks"].items(), key=lambda kv: -kv[1]["maxUs"]):
        print("  %-22s %7d  %s  %s  %6d ms" %
              (name, b["count"], fmt_us(b["meanUs"]),
               fmt_us(b["maxUs"]), b["totalMs"]))
    print()


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--host", default="192.168.1.198")
    ap.add_argument("--port", type=int, default=80)
    ap.add_argument("--user", default="admin")
    ap.add_argument("--password", default="1234")
    ap.add_argument("--window", type=int, default=60, help="seconds")
    ap.add_argument("--load", choices=["idle", "poll"], default="idle")
    ap.add_argument("--request", default="get_status",
                    help="command the polling consumer sends")
    args = ap.parse_args()

    print("logging in to %s:%d ..." % (args.host, args.port))
    token = login(args.host, args.port, args.user, args.password)

    print("arming probe for %ds (load=%s)" % (args.window, args.load))
    armed = rpc(args.host, args.port,
                {"type": "reset_timing", "token": token, "window_s": args.window})
    # env:main compiles the probe out entirely, so it answers E-200 here. Say so
    # rather than running a full window and failing at the readout.
    if armed.get("type") != "timing":
        sys.exit("board has no timing probe (%s) - flash `pio run -e timing`"
                 % armed.get("message", armed))

    polls, t0 = None, time.time()
    if args.load == "poll":
        polls = 0
        errors = 0
        while time.time() - t0 < args.window:
            try:
                rpc(args.host, args.port, {"type": args.request, "token": token})
                polls += 1
            except Exception:
                errors += 1
                if errors > 50:
                    sys.exit("too many request failures - is this IP whitelisted?")
            if polls and polls % 100 == 0:
                print("  %d polls, %.0fs elapsed" % (polls, time.time() - t0))
        if errors:
            print("  (%d failed requests)" % errors)
    else:
        while time.time() - t0 < args.window:
            time.sleep(1)
            print("  %ds ..." % int(time.time() - t0), end="\r", flush=True)

    elapsed = time.time() - t0
    time.sleep(1.5)   # let the board's own service() latch the freeze

    # Re-login before reading. TOKEN_TIMEOUT_MS is 5 minutes and an idle window
    # sends nothing, so on any window past that the original token is dead by
    # now. Refreshing it DURING the window is not an option - that is the very
    # load the idle scenario exists to exclude - so we get a fresh one here,
    # after the probe has already frozen.
    token = login(args.host, args.port, args.user, args.password)
    r = rpc(args.host, args.port, {"type": "get_timing", "token": token})
    if "dt" not in r:
        sys.exit("readout failed: %s" % r)
    if r.get("state") != "frozen":
        print("WARNING: probe state is %r, expected 'frozen'" % r.get("state"))
    report(r, polls, elapsed)


if __name__ == "__main__":
    main()
