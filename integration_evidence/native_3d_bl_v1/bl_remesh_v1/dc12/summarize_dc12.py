#!/usr/bin/env python3
"""DC-12 campaign table: per case complete / bad / long / height failures / layers in 3-25 h0 / time per phase."""
import json, os, sys
root = sys.argv[1] if len(sys.argv) > 1 else os.path.join(os.path.dirname(os.path.abspath(__file__)), "runs")
cases = ["annulus", "m6_le", "m6_root_te", "m6_tip_te", "tube_root_x", "two_tube_y"]
print(f"{'case':12s} {'done':5s} {'cells':>7s} {'bad':>5s} {'long':>5s} {'h':>3s} {'unm':>4s} {'A2 frac':>7s} {'dev p10/med/p90':>16s} "
      f"{'G lifts':>7s} {'audit viol':>10s} {'stale':>5s} {'W':>5s} {'R1':>5s} {'R2':>5s} {'G':>5s} {'V':>6s} {'total':>6s} V-outcome")
for c in cases:
    p = os.path.join(root, f"dc12_{c}.json")
    if not os.path.exists(p): print(f"{c:12s} missing"); continue
    d = json.load(open(p)); f = d["final"]; L = d["layers_final"]; v = d["V12"]
    dev = "/".join(str(x) for x in L["deviation_p10_median_p90"])
    print(f"{c:12s} {('yes' if f['complete'] else 'no'):5s} {f['cells']:7d} {f['bad_shape']:5d} {f['long_edges']:5d} {f['height_failures']:3d} {f['unmeasurable_edges']:4d} "
          f"{L['fraction']:7.3f} {dev:>16s} {d['G']['lifted_points']:7d} {d['first_row_audit']['violations']:10d} {d['stale_apex_registrations']:5d} "
          f"{d['W']['seconds']:5.0f} {d['R1']['seconds']:5.0f} {d['R2']['seconds']:5.0f} {d['G']['seconds']:5.0f} {v['stage']['seconds']:6.0f} {d['total_seconds']:6.0f} {v['stage']['outcome']}")
for c in ["tube_root_x", "m6_root_te"]:
    p = os.path.join(root, f"volonly12_{c}.json")
    if not os.path.exists(p): print(f"volonly {c}: missing"); continue
    d = json.load(open(p)); f = d.get("final", {})
    print(f"volonly {c:12s} valid={d['valid']} stop={d['stop']} rounds={d['rounds']} cells={d['cells']} final long={f.get('long_edges')} bad={f.get('bad_shape')} "
          f"complete={f.get('complete')} t={d['seconds']:.0f} (refine {d['refine_seconds']:.0f}, optimize {d['optimize_seconds']:.0f})")
