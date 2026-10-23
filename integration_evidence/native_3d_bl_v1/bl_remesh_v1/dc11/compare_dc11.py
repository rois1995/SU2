#!/usr/bin/env python3
"""DC-11 vs DC-10 per-case table. Usage: compare_dc11.py <dc11 root> [<dc11 root> ...]; DC-10 reference = dc10/diag2 (single)
and dc10/runs (fan). Columns: R2 rowed/unrowed, cells, bad, long, height failures, V outcome/work/completed sweeps,
total seconds, split-policy counters, D-floor witness."""
import json, sys, os
H = os.path.dirname(os.path.abspath(__file__))
CASES = ['annulus', 'm6_le', 'm6_root_te', 'm6_tip_te', 'tube_root_x', 'two_tube_y']
def row(d):
    f = d['final']; r = d['R2_counts']; v = d.get('V11', {})
    return (f"rowed/unrowed {r['rowed']}/{r['unrowed']} cells {f['cells']} bad {f['bad_shape']} long {f['long_edges']} h {f['height_failures']} "
            f"complete {f['complete']} V {d['V']['outcome']} work {d['V']['work']} sweeps {v.get('completed_sweeps', d['V_counts']['sweeps'])} t {d['total_seconds']:.0f}s")
for root in sys.argv[1:]:
    print(f"== {root}")
    tot = [0, 0]
    for c in CASES:
        p = os.path.join(root, f'dc11_{c}.json')
        if not os.path.exists(p): print(f"  {c}: missing"); continue
        d = json.load(open(p)); lift = d['limits']['lift']
        ref = os.path.join(H, '..', 'dc10', 'diag2' if lift == 'single' else 'runs', f'dc10_{c}.json')
        b = json.load(open(ref))
        print(f"  {c:12s} DC11 {row(d)}\n  {'':12s} DC10 {row(b)}")
        print(f"  {'':12s} policy {d.get('split_policy_after_V')} accepted {d.get('V11', {}).get('accepted_by')}")
        print(f"  {'':12s} floor {d.get('V11', {}).get('floor_refusals')} grow {d.get('V11', {}).get('grow')}")
        tot[0] += d['final']['bad_shape'] + d['final']['long_edges'] + d['final']['height_failures']
        tot[1] += b['final']['bad_shape'] + b['final']['long_edges'] + b['final']['height_failures']
    print(f"  total gate failures (bad+long+height) DC11 {tot[0]} vs DC10 {tot[1]}")
