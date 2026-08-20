#!/usr/bin/env python3
"""
Hardware acceptance test for the two-IMU agreement check (P3 item 2).

    ./imu_agreement_test.py

WHAT THIS CAN AND CANNOT PROVE

It cannot provoke a real disagreement. Both sensors are on one board reading
one gravity vector; nothing short of physically moving one relative to the
other makes them differ, and the failure path is covered by the unit tests in
firmware/test/test_imu_agreement instead.

What it tests is the failure that would actually bite in the field: a FALSE
POSITIVE. When the check trips, both imu1Sane and imu2Sane are cleared, and
those flags select which IMU drives the orientation filter - so a spurious trip
freezes pitch, roll and yaw at their last values. On a moving machine that is
worse than the fault it is looking for.

So every phase here asks the same question: does a healthy pair still agree
while still, while tilted, while shaken, and through a slow sweep of attitudes?

It reads imuDisagreeCount rather than polling imusAgree, because the check runs
at loop rate (~95 Hz) and this polls at perhaps 20 - a brief trip would freeze
the angles and never appear in a sample. The counter cannot miss one.
"""

import argparse, json, http.client, math, sys, time

HOST = "192.168.1.198"
PASS_N = FAIL_N = 0
FAILURES = []

def rpc(payload, timeout=8):
    c = http.client.HTTPConnection(HOST, 80, timeout=timeout)
    try:
        b = json.dumps(payload)
        c.request("POST", "/", b, {"Content-Type": "application/json",
                                   "Content-Length": str(len(b))})
        return json.loads(c.getresponse().read().decode("utf-8", "replace"))
    finally:
        c.close()

def token():
    r = rpc({"type": "login", "user": "admin", "pass": "1234"})
    if not r.get("success"):
        sys.exit("login failed: %s" % r.get("message"))
    return r["token"]

def imu(t):
    return rpc({"type": "get_imu", "token": t})

def check(name, ok, detail=""):
    global PASS_N, FAIL_N
    if ok:
        PASS_N += 1; print("   \033[32mPASS\033[0m  %s %s" % (name, detail))
    else:
        FAIL_N += 1; FAILURES.append(name)
        print("   \033[31mFAIL\033[0m  %s %s" % (name, detail))

def ask(msg):
    input("\n>>> %s  [Enter when done] " % msg)

def ratios(s):
    """Per-axis IMU1-vs-IMU2 ratio, with IMU2's 180 deg in-plane mount undone.

    Returns only axes above the deadband, because a ratio between two numbers
    that should both be zero is meaningless - that is the whole reason the
    deadband exists."""
    a = (s["imuX"], s["imuY"], s["imuZ"])
    b = (-s["imu2X"], -s["imu2Y"], s["imu2Z"])
    out = {}
    for i, n in enumerate("XYZ"):
        m1, m2 = abs(a[i]), abs(b[i])
        if max(m1, m2) < 0.5:          # IMU_AGREE_DEADBAND_G
            continue
        lo, hi = min(m1, m2), max(m1, m2)
        out[n] = hi / lo if lo > 1e-9 else float("inf")
    return out

def watch(t, seconds, label):
    """Sample for a while. Returns (disagreements, worst ratio, axis coverage)."""
    start = imu(t)["imuDisagreeCount"]
    worst, axes_seen, n = 1.0, set(), 0
    t0 = time.time()
    while time.time() - t0 < seconds:
        s = imu(t); n += 1
        axes_seen.add(s["imuAgreeAxes"])
        for v in ratios(s).values():
            worst = max(worst, v)
    end = imu(t)
    trips = end["imuDisagreeCount"] - start
    print("      %s: %d samples, worst ratio %.2f, axes compared %s, trips %d"
          % (label, n, worst, sorted(axes_seen), trips))
    return trips, worst, axes_seen


def main():
    global HOST
    ap = argparse.ArgumentParser()
    ap.add_argument("--host", default=HOST)
    ap.add_argument("--quick", action="store_true", help="shorter watch windows")
    a = ap.parse_args(); HOST = a.host
    W = 10 if a.quick else 25

    t = token()
    print("=" * 70)
    print(" IMU agreement (P3 item 2) - %s" % HOST)
    print("=" * 70)

    # ---- 1 -----------------------------------------------------------
    print("\n[1] Baseline - board level and still")
    ask("Put the board FLAT and leave it alone")
    s = imu(t)
    print("      raw  IMU1 %+.4f %+.4f %+.4f" % (s["imuX"], s["imuY"], s["imuZ"]))
    print("           IMU2 %+.4f %+.4f %+.4f" % (s["imu2X"], s["imu2Y"], s["imu2Z"]))
    r = ratios(s)
    print("      axes above the deadband: %s" % (
        ", ".join("%s=%.2f" % (k, v) for k, v in r.items()) or "none"))
    check("firmware reports the agreement state", "imusAgree" in s)
    check("a healthy pair agrees", s["imusAgree"] is True)
    check("at least one axis is being compared", s["imuAgreeAxes"] >= 1,
          "(%d)" % s["imuAgreeAxes"])
    check("both sane flags intact", s["imu1Sane"] and s["imu2Sane"])
    base_trips = s["imuDisagreeCount"]
    print("      disagreements since boot: %d" % base_trips)

    # ---- 2 -----------------------------------------------------------
    print("\n[2] Sustained - it must not trip while nothing is happening")
    trips, worst, _ = watch(t, W, "still")
    check("no trips while still", trips == 0, "(%d)" % trips)
    check("margin to the x2 limit is comfortable", worst < 1.5,
          "(worst ratio %.2f, limit 2.00)" % worst)

    # ---- 3 -----------------------------------------------------------
    print("\n[3] Tilted - the deadband must still find axes to compare")
    print("    At 0.5 g, two in-plane axes reach 0.707 g at 45 deg, so coverage")
    print("    should RISE with tilt rather than fall away.")
    ask("Tilt the board to roughly 45 deg and HOLD it steady")
    trips, worst, axes = watch(t, W, "tilted")
    check("no trips while tilted", trips == 0, "(%d)" % trips)
    check("more than one axis compared when tilted", max(axes) >= 2,
          "(axes seen: %s)" % sorted(axes))
    check("margin holds when tilted", worst < 1.5, "(worst ratio %.2f)" % worst)

    # ---- 4 -----------------------------------------------------------
    print("\n[4] Shaken - THE test that matters")
    print("    A false trip here clears both sane flags, which stops the")
    print("    orientation filter and freezes the reported angles.")
    ask("SHAKE the board hard and keep shaking for the whole window")
    trips, worst, _ = watch(t, W, "shaken")
    check("no trips under vibration", trips == 0, "(%d)" % trips)
    print("      worst ratio while shaken: %.2f (limit 2.00)" % worst)

    # ---- 5 -----------------------------------------------------------
    print("\n[5] Swept - through many attitudes")
    ask("Slowly rotate the board through as many orientations as you can")
    trips, worst, axes = watch(t, W, "swept")
    check("no trips through a full sweep", trips == 0, "(%d)" % trips)
    check("coverage never dropped to zero axes", 0 not in axes,
          "(axes seen: %s)" % sorted(axes))

    # ---- 6 -----------------------------------------------------------
    print("\n[6] Settled - the angles are still live afterwards")
    ask("Put the board FLAT again and leave it")
    time.sleep(3)
    s = imu(t)
    check("agrees again", s["imusAgree"] is True)
    check("both sane flags intact", s["imu1Sane"] and s["imu2Sane"])
    check("angleSane recovered", s["angleSane"] is True)
    total = s["imuDisagreeCount"] - base_trips
    check("no disagreement anywhere in this run", total == 0, "(%d)" % total)
    print("      angles now: pitch %.3f roll %.3f" % (s["pitch"], s["roll"]))

    print("\n" + "=" * 70)
    print(" %d passed, %d failed" % (PASS_N, FAIL_N))
    for f in FAILURES:
        print("   - %s" % f)
    print("""
 NOT covered here: a genuine disagreement. Two sensors on one board see one
 gravity vector, so it cannot be provoked without physically moving one. That
 path is covered by firmware/test/test_imu_agreement - eight cases including a
 dead sensor reading zero, an inverted axis, and one reading 2.5x the other.""")
    print("=" * 70)
    return 1 if FAIL_N else 0

if __name__ == "__main__":
    sys.exit(main())
