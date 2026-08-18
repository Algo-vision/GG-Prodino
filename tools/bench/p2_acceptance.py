#!/usr/bin/env python3
"""
P2 hardware acceptance tests - zero calibration, rest detection, gyro bias.

Interactive: it tells you what to do to the board, you press Enter, it checks.
Run from the repo root:  ./p2_test.py

Nothing here is destructive except the zero calibrations, and those are
re-burnable - step 9 puts a flat one back before finishing.
"""
import argparse, json, http.client, sys, time

HOST, PORT = "192.168.1.198", 80
USER, PASS = "admin", "1234"

PASS_N, FAIL_N, SKIP_N = 0, 0, 0
FAILURES = []

def rpc(payload, timeout=10):
    c = http.client.HTTPConnection(HOST, PORT, timeout=timeout)
    try:
        b = json.dumps(payload)
        c.request("POST", "/", b, {"Content-Type": "application/json",
                                   "Content-Length": str(len(b))})
        r = c.getresponse()
        return r.status, json.loads(r.read().decode("utf-8", "replace"))
    finally:
        c.close()

def token():
    return rpc({"type": "login", "user": USER, "pass": PASS})[1]["token"]

def cal(t):   return rpc({"type": "get_calibration", "token": t})[1]
def imu(t):   return rpc({"type": "get_imu", "token": t})[1]

def check(name, ok, detail=""):
    global PASS_N, FAIL_N
    if ok:
        PASS_N += 1; print("   \033[32mPASS\033[0m  %s %s" % (name, detail))
    else:
        FAIL_N += 1; FAILURES.append(name)
        print("   \033[31mFAIL\033[0m  %s %s" % (name, detail))

def skip(name, why):
    global SKIP_N
    SKIP_N += 1
    print("   \033[33mSKIP\033[0m  %s (%s)" % (name, why))

def ask(msg):
    input("\n>>> %s  [Enter when done] " % msg)

def hold_still(t, seconds=4.0):
    """Wait until the board reports enough continuous rest."""
    print("    waiting for the board to settle ...", end="", flush=True)
    for _ in range(40):
        r = cal(t)
        if r["restSeconds"] >= seconds:
            print(" still for %.1fs" % r["restSeconds"]); return True
        time.sleep(1)
    print(" gave up (restSeconds=%.1f)" % r["restSeconds"]); return False

def settle(seconds=3.0):
    """Let the complementary filter converge - tau is about 0.65 s at 100 kHz."""
    time.sleep(seconds)

def reboot(t):
    """Reset via set_ip_config with the values it already has."""
    # Re-login first. The token dies after 5 minutes and step 11 deliberately
    # waits 6, so the one we were handed may be long gone.
    t = token()
    cfg = rpc({"type": "get_config", "token": t})[1]
    ip, wl = cfg["controllerIp"], cfg["whitelistIps"]
    if "192.168.1.169" not in wl:
        sys.exit("refusing to reboot: our IP is not in the whitelist we would write")
    try:
        rpc({"type": "set_ip_config", "token": t,
             "controller_ip": ip, "whitelist_ips": wl})
    except Exception:
        pass
    print("    rebooting", end="", flush=True)
    for _ in range(40):
        time.sleep(2); print(".", end="", flush=True)
        try:
            t2 = token(); print(" back"); return t2
        except Exception:
            pass
    sys.exit("board did not come back")

# ---------------------------------------------------------------------------

def main():
    global HOST
    ap = argparse.ArgumentParser()
    ap.add_argument("--host", default=HOST)
    ap.add_argument("--tilt", type=float, default=None,
                    help="reference angle for step 5, from an inclinometer")
    a = ap.parse_args()
    HOST = a.host

    t = token()
    print("=" * 70)
    print(" P2 acceptance - %s" % HOST)
    print("=" * 70)

    # ---- 1 ------------------------------------------------------------
    print("\n[1] Starting state")
    c = cal(t)
    print("    zeroCalValid=%s  mount=%.3f/%.3f  gyroBiasValid=%s  restSeconds=%.1f"
          % (c["zeroCalValid"], c["mountPitch"], c["mountRoll"],
             c["gyroBiasValid"], c["restSeconds"]))
    check("controller reports its calibration state", "zeroCalValid" in c)

    # ---- 2 ------------------------------------------------------------
    print("\n[2] Rest detector reacts to movement")
    ask("SHAKE the table / the board, and keep shaking")
    r = cal(t)
    check("restSeconds collapses while moving", r["restSeconds"] < 1.0,
          "(restSeconds=%.2f)" % r["restSeconds"])
    check("atRest is false while moving", not r["atRest"])

    # ---- 3 ------------------------------------------------------------
    print("\n[3] Both calibrations are refused while moving")
    ask("KEEP SHAKING through this step")
    s, r = rpc({"type": "burn_zero_calibration", "token": t})
    check("burn_zero_calibration refused", (not r.get("success")) and s == 409,
          "(http %d, %s)" % (s, r.get("message")))
    s, r = rpc({"type": "calibrate_now", "token": t})
    check("calibrate_now refused", (not r.get("success")) and s == 409,
          "(http %d, %s)" % (s, r.get("message")))

    # ---- 4 ------------------------------------------------------------
    print("\n[4] Rest re-establishes, and burning works on the flat")
    ask("STOP. Put the board FLAT and leave it alone")
    check("rest re-establishes after movement stops", hold_still(t))
    s, r = rpc({"type": "burn_zero_calibration", "token": t})
    check("burn accepted when still", r.get("success"),
          "(mount %.3f/%.3f)" % (r.get("mountPitch", 0), r.get("mountRoll", 0)))
    settle()
    i = imu(t)
    check("reports level right after burning",
          abs(i["pitch"]) < 0.5 and abs(i["roll"]) < 0.5,
          "(pitch=%.3f roll=%.3f)" % (i["pitch"], i["roll"]))

    # ---- 5 ------------------------------------------------------------
    print("\n[5] Tilt accuracy - the rotation math on real hardware")
    if a.tilt is None:
        skip("tilt accuracy", "re-run with --tilt <degrees> and an inclinometer")
    else:
        ask("Tilt the board to %.1f deg in PITCH and hold it steady" % a.tilt)
        hold_still(t, 2.5); settle()
        i = imu(t)
        err = abs(abs(i["pitch"]) - a.tilt)
        check("reported pitch matches the reference within 1.5 deg", err < 1.5,
              "(reported %.2f, reference %.1f, error %.2f)" % (i["pitch"], a.tilt, err))
        check("roll stays near zero during a pure pitch tilt", abs(i["roll"]) < 3.0,
              "(roll=%.2f)" % i["roll"])

    # ---- 6 ------------------------------------------------------------
    print("\n[6] Burning at a TILTED attitude redefines level there")
    ask("Tilt the board to any clear angle (20-45 deg) and HOLD it steady")
    if not hold_still(t, 2.5):
        skip("tilted burn", "board never settled")
    else:
        settle()
        tilted = imu(t)
        print("    before burning, at this attitude: pitch=%.2f roll=%.2f"
              % (tilted["pitch"], tilted["roll"]))
        # Without a real tilt every assertion below is satisfied by zero, and
        # the rotation is never exercised at an angle where it could be wrong.
        magnitude = max(abs(tilted["pitch"]), abs(tilted["roll"]))
        if magnitude < 10.0:
            check("board was actually tilted before burning", False,
                  "(only %.2f deg - TILT IT and re-run, this test proves nothing flat)"
                  % magnitude)
            print("    skipping the rest of step 6 - there is no tilt to check")
            raise SystemExit(1)
        check("board was actually tilted before burning", True,
              "(%.1f deg)" % magnitude)
        s, r = rpc({"type": "burn_zero_calibration", "token": t})
        check("burn accepted at a tilted attitude", r.get("success"),
              "(mount %.2f/%.2f)" % (r.get("mountPitch", 0), r.get("mountRoll", 0)))
        settle()
        i = imu(t)
        check("this attitude now reads as level",
              abs(i["pitch"]) < 0.5 and abs(i["roll"]) < 0.5,
              "(pitch=%.3f roll=%.3f)" % (i["pitch"], i["roll"]))

        ask("Now put the board back FLAT and leave it")
        hold_still(t, 2.5); settle()
        back = imu(t)
        expect_p, expect_r = -tilted["pitch"], -tilted["roll"]
        check("flat now reads the negative of the burned tilt",
              abs(back["pitch"] - expect_p) < 2.0 and abs(back["roll"] - expect_r) < 2.0,
              "(got %.2f/%.2f, expected about %.2f/%.2f)"
              % (back["pitch"], back["roll"], expect_p, expect_r))

    # ---- 7 ------------------------------------------------------------
    print("\n[7] Re-burnable - put level back where it belongs")
    hold_still(t, 2.5)
    s, r = rpc({"type": "burn_zero_calibration", "token": t})
    check("re-burn accepted (calibration is not one-shot)", r.get("success"))
    settle()
    i = imu(t)
    check("flat reads level again", abs(i["pitch"]) < 0.5 and abs(i["roll"]) < 0.5,
          "(pitch=%.3f roll=%.3f)" % (i["pitch"], i["roll"]))

    # ---- 8 ------------------------------------------------------------
    print("\n[8] Initiated calibration on the flat")
    print("    Diagnostic: if pitch/roll barely move, the zero calibration is")
    print("    good and the filter had not drifted. If they JUMP, the zero")
    print("    calibration is wrong - see the P2 notes.")
    before = imu(t)
    s, r = rpc({"type": "calibrate_now", "token": t})
    check("calibrate_now accepted when still", r.get("success"))
    if r.get("success"):
        dp = abs(r["pitch"] - before["pitch"]); dr = abs(r["roll"] - before["roll"])
        check("pitch/roll barely move (filter had already converged)",
              dp < 1.0 and dr < 1.0, "(moved %.3f / %.3f deg)" % (dp, dr))

    # ---- 9 ------------------------------------------------------------
    print("\n[9] Survives a reboot")
    pre = cal(t)
    t = reboot(t)
    post = cal(t)
    check("zero calibration survived", post["zeroCalValid"] and
          abs(post["mountPitch"] - pre["mountPitch"]) < 0.01 and
          abs(post["mountRoll"] - pre["mountRoll"]) < 0.01,
          "(%.3f/%.3f -> %.3f/%.3f)" % (pre["mountPitch"], pre["mountRoll"],
                                        post["mountPitch"], post["mountRoll"]))
    check("gyro bias came back from flash", post["gyroBiasValid"],
          "(%s)" % [round(v, 4) for v in post["gyroBias1"]])
    settle(4)
    i = imu(t)
    check("comes up level with NO boot calibration",
          abs(i["pitch"]) < 1.0 and abs(i["roll"]) < 1.0,
          "(pitch=%.3f roll=%.3f)" % (i["pitch"], i["roll"]))

    # ---- 10 -----------------------------------------------------------
    print("\n[10] Booting while shaken - the whole reason boot calibration went")
    print("     V1.5.1 would bake a real rotation rate in as 'zero' here.")
    ask("SHAKE the board hard and KEEP SHAKING until told to stop")
    t = reboot(t)
    print("    (it rebooted while you were shaking)")
    ask("NOW STOP and put it FLAT")
    hold_still(t, 3.0); settle(4)
    i = imu(t); c = cal(t)
    check("still level after booting while shaken",
          abs(i["pitch"]) < 1.0 and abs(i["roll"]) < 1.0,
          "(pitch=%.3f roll=%.3f)" % (i["pitch"], i["roll"]))
    check("gyro bias is sane after a shaken boot",
          all(abs(v) < 5.0 for v in c["gyroBias1"]),
          "(%s)" % [round(v, 4) for v in c["gyroBias1"]])

    # ---- 11 -----------------------------------------------------------
    print("\n[11] The five-minute flash write does not erase the calibration")
    print("     configSave() rebuilds the whole record from globals and the")
    print("     work-hours counter calls it every 5 minutes.")
    print("     This waits 6 minutes, then reboots and re-checks.")
    if input("     run it? [y/N] ").strip().lower() != "y":
        skip("periodic-save survival", "not run")
    else:
        pre = cal(t)
        for m in range(6):
            time.sleep(60); print("     %d/6 min" % (m + 1))
        t = reboot(t)
        post = cal(t)
        check("calibration survived 6 minutes of periodic saves",
              post["zeroCalValid"] and
              abs(post["mountPitch"] - pre["mountPitch"]) < 0.01,
              "(%.3f -> %.3f)" % (pre["mountPitch"], post["mountPitch"]))

    # ---- summary ------------------------------------------------------
    print("\n" + "=" * 70)
    print(" %d passed, %d failed, %d skipped" % (PASS_N, FAIL_N, SKIP_N))
    if FAILURES:
        print(" failed:")
        for f in FAILURES: print("   - %s" % f)
    print("=" * 70)
    return 1 if FAIL_N else 0

if __name__ == "__main__":
    sys.exit(main())
