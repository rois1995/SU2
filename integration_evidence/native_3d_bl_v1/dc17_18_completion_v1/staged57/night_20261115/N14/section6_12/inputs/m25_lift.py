#!/usr/bin/env python3
"""Is the lift of the split point a lower bound of the certified face bound? (M25 table, read only)"""
import csv, sys
TOL = 1e-4
rows = list(csv.DictReader(open(sys.argv[1], newline='')))
def f(r, k):
    try: return float(r[k])
    except ValueError: return float('nan')
def q(v, p):
    v = sorted(v); return v[int(p*(len(v)-1))]
ratio = []; viol = 0; n = 0; fail_but_pass = 0; lift_fail = 0
for r in rows:
    lift = f(r, 'midpoint_distance')
    for i in (0, 1):
        p = f(r, f'parent{i}')
        if p != p or lift != lift: continue
        n += 1; ratio.append(lift/p if p > 0 else 0.0)
        viol += lift > p*(1+1e-9)
        if lift > TOL:
            lift_fail += 1; fail_but_pass += p <= TOL
print(f"parent faces={n} lift>bound: {viol}  lift/bound q5/50/95/max={q(ratio,.05):.3f}/{q(ratio,.5):.3f}/{q(ratio,.95):.3f}/{max(ratio):.3f}")
print(f"faces on an edge with lift>tol: {lift_fail}; of them with bound<=tol: {fail_but_pass}")
# Second generation proxy: for lift-required edges, is the larger child kept by another lift-required edge of the list?
lift = {frozenset((r['edge_a'], r['edge_b'])): f(r, 'midpoint_distance') for r in rows}
known = miss = req = 0
for r in rows:
    if not f(r, 'midpoint_distance') > TOL: continue
    e = frozenset((r['edge_a'], r['edge_b']))
    for i in (0, 1):
        p = f(r, f'parent{i}'); big = max(f(r, f'child{i}0'), f(r, f'child{i}1'))
        if not (p > TOL and big > TOL): continue
        others = [frozenset(r[f'law_f{i}_e{j}_edge'].split(':')) for j in range(3)]
        others = [o for o in others if o != e]
        vals = [lift.get(o) for o in others]
        if any(v is None for v in vals): miss += 1; continue
        known += 1; req += max(vals) > TOL
print(f"failing larger children after a lift-required split: other old edge lift known={known} (not in list={miss}); another old edge has lift>tol: {req}")
# Split position: bound point against the chord midpoint, relative to the edge length.
rel = [f(r,'midpoint_distance')/f(r,'edge_length') for r in rows if f(r,'edge_length') > 0]
print(f"lift/length q50/95/max={q(rel,.5):.4f}/{q(rel,.95):.4f}/{max(rel):.4f}")
