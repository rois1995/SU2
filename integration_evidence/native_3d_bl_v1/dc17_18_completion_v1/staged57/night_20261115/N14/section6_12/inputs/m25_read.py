#!/usr/bin/env python3
"""Read-only analysis of the agent's M25 outcome table (work order 6.10, item N13)."""
import csv, sys
from statistics import median
TOL = 1e-4
rows = list(csv.DictReader(open(sys.argv[1], newline='')))
def f(r, k):
    try: return float(r[k])
    except ValueError: return float('nan')
def q(v, p):
    v = sorted(v); return v[int(p*(len(v)-1))] if v else float('nan')
def hinge2(x): return max(0.0, x-TOL)**2

def faces(r):
    """For each parent face: (parent bound, child bounds, estimates of the edge and of the two others)."""
    out = []
    edge = {r['edge_a'], r['edge_b']}
    for i in (0, 1):
        p = f(r, f'parent{i}'); c = (f(r, f'child{i}0'), f(r, f'child{i}1'))
        est = {}
        for j in range(3):
            est[frozenset(r[f'law_f{i}_e{j}_edge'].split(':'))] = (f(r, f'law_f{i}_e{j}_value'), r[f'law_f{i}_e{j}_supported'] == '1')
        mine = est[frozenset(edge)]
        others = [v for k, v in est.items() if k != frozenset(edge)]
        out.append(dict(parent=p, child=c, mine=mine, others=others))
    return out

def describe(name, sel):
    n = len(sel)
    if not n: print(f"{name}: 0"); return
    verdict = {k: sum(r['n11_first_verdict'] == k for r in sel) for k in ('accepted', 'guidance', 'none')}
    lift = [f(r, 'midpoint_distance')/TOL for r in sel]
    big, small, ratio, worse, second = [], [], [], 0, []
    for r in sel:
        fs = [x for x in faces(r) if x['parent'] > TOL]
        b = sum(hinge2(x['parent']) for x in fs); a = sum(hinge2(c) for x in fs for c in x['child'])
        if b > 0: ratio.append(a/b)
        for x in fs:
            big.append(max(x['child'])/x['parent']); small.append(min(x['child'])/x['parent'])
            worse += max(x['child']) > x['parent']*1.2
            if x['mine'][0] >= max(o[0] for o in x['others']):
                second.append(max(o[0] for o in x['others'])/x['mine'][0])
    print(f"{name}: n={n} n11 acc/guid/none={verdict['accepted']}/{verdict['guidance']}/{verdict['none']}"
          f" | lift/tol q10/50/90={q(lift,.1):.2f}/{q(lift,.5):.2f}/{q(lift,.9):.2f} lift>tol={sum(x>1 for x in lift)/n:.2f}"
          f" | own-sum after/before q50={q(ratio,.5):.2f} <1:{sum(x<1 for x in ratio)/max(1,len(ratio)):.2f}"
          f" | big child/parent q50/90/max={q(big,.5):.2f}/{q(big,.9):.2f}/{max(big):.2f} >1.2:{worse}/{len(big)}"
          f" | small q50={q(small,.5):.2f} | 2nd/1st est q50={q(second,.5):.2f}")

def law_required(r):
    """N14 text: some failing parent face where the edge has the largest estimate, all three supported, need."""
    if r['need'] != '1': return False
    for x in faces(r):
        if x['parent'] > TOL and x['mine'][1] and all(o[1] for o in x['others']) and x['mine'][0] >= max(o[0] for o in x['others']):
            return True
    return False
def in_failing(r): return any(x['parent'] > TOL for x in faces(r))
def lift_required(r): return in_failing(r) and f(r, 'midpoint_distance') > TOL

print("rows", len(rows), "in a failing face", sum(in_failing(r) for r in rows))
describe("ALL in failing face", [r for r in rows if in_failing(r)])
describe("LAW required (N14 text)", [r for r in rows if law_required(r)])
describe("  law & n11 guidance", [r for r in rows if law_required(r) and r['n11_first_verdict'] == 'guidance'])
describe("  law & n11 accepted", [r for r in rows if law_required(r) and r['n11_first_verdict'] == 'accepted'])
describe("  law & lift<=tol", [r for r in rows if law_required(r) and not lift_required(r)])
describe("LIFT required (lift>tol)", [r for r in rows if lift_required(r)])
describe("  lift & not law", [r for r in rows if lift_required(r) and not law_required(r)])
describe("  lift & n11 guidance", [r for r in rows if lift_required(r) and r['n11_first_verdict'] == 'guidance'])
describe("NOT law, failing, n11 guidance", [r for r in rows if in_failing(r) and not law_required(r) and r['n11_first_verdict'] == 'guidance'])
describe("NOT law, failing, n11 accepted", [r for r in rows if in_failing(r) and not law_required(r) and r['n11_first_verdict'] == 'accepted'])
# Estimate against the true lift.
er = [f(r, 'estimate')/f(r, 'midpoint_distance') for r in rows if f(r, 'midpoint_distance') > 0]
print("estimate / lift q5/25/50/75/95:", *(f"{q(er,p):.2f}" for p in (.05,.25,.5,.75,.95)))
# Which child keeps the error: child with the larger other-edge estimate?
