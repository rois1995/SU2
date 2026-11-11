#!/usr/bin/env python3
"""Offline reconstruction of build389's interior G7/K15 edge-panel trace."""
import argparse
import csv
import hashlib
import json
from decimal import Decimal, getcontext
from pathlib import Path

getcontext().prec = 50
HERE = Path(__file__).resolve().parent
D = Decimal
N = [D(x) for x in ('.991455371120812639206854697526329', '.949107912342758524526189684047851', '.864864423359769072789712788640926', '.741531185599394439863864773280788', '.586087235467691130294144838258730', '.405845151377397166906606412076961', '.207784955007898467600689403773245')]
K = [D(x) for x in ('.022935322010529224963732008058970', '.063092092629978553290700663189204', '.104790010322250183839876322541518', '.140653259715525918745189590510238', '.169004726639267902826583426598550', '.190350578064785409913256402421014', '.204432940075298892414161999234649', '.209482141084727828012999174891714')]
G = [D(x) for x in ('.129484966168869693270611432679082', '.279705391489276667901467771423780', '.381830050505118944950369775488975', '.417959183673469387755102040816327')]
Z = sorted([-x for x in N] + [D(0)] + N)
TOL = D("1e-12")


def panels_from_rows(rows):
    """A panel is recognized by its unordered 15-node parameter multiset."""
    out, i = [], 0
    while i + 15 <= len(rows):
        block = rows[i:i+15]
        xs = [r[0] for r in block]
        xm = sum(xs) / 15
        den = sum(z*z for z in Z)
        half = sum(z*(x-xm) for z, x in zip(Z, sorted(xs))) / den
        # Sorting samples against the canonical symmetric node set removes pair evaluation order.
        err = max(abs(x - (xm + half*z)) for x, z in zip(sorted(xs), Z))
        err=max(err,abs(xs[0]-xm))
        for j,node in enumerate(N):
            err=max(err,*(abs(x-y) for x,y in zip(sorted(xs[1+2*j:3+2*j]),[xm-half*node,xm+half*node])))
        if half <= 0 or err > D("2e-18") * max(abs(xm), abs(half), D("1e-30")):
            raise ValueError(f"invalid complete G7/K15 node protocol at sample {i}: half={half}, residual={err}")
        lo, hi = xm-half, xm+half
        # Node weights in the source: Gauss nodes are center and K15 j=1,3,5 pairs.
        kval = K[7]*block[0][1]; gval = G[3]*block[0][1]
        for j, node in enumerate(N):
            pair = block[1+2*j][1]+block[2+2*j][1]
            kval += K[j]*pair
            if j % 2: gval += G[j//2]*pair
        out.append(dict(lo=lo, hi=hi, value=half*kval, error=half*abs(kval-gval), first=i, rows=15))
        i += 15
    return out, len(rows)-i


def analyze(rows, cuts):
    if len(rows) > 65536: raise ValueError(f"trace has {len(rows)} rows, exceeds 65536 evaluation cap")
    if not cuts or cuts[0] != 0 or cuts[-1] != 1 or any(a >= b for a,b in zip(cuts,cuts[1:])):
        raise ValueError(f"invalid geometric coverage cuts: {cuts}")
    panels, partial = panels_from_rows(rows)
    intervals = list(zip(cuts, cuts[1:]))
    def tol(a,b,scale): return D("2e-18") * max(abs(a),abs(b),abs(scale),D("1e-30"))
    def near(a,b,scale): return abs(a-b) <= tol(a,b,scale)
    initial=[]
    for idx,(a,b) in enumerate(intervals):
        if idx == len(panels): break
        p=panels[idx]
        if not (near(p['lo'],a,b-a) and near(p['hi'],b,b-a)):
            raise ValueError(f"initial coverage/order mismatch interval {idx} [{a},{b}], observed [{p['lo']},{p['hi']}]; {len(panels)} complete panels, {partial} trailing samples")
        initial.append(p)
    coverage_complete=len(initial)==len(intervals)
    roots=list(initial); leaves=list(roots); split_count=0
    rest=panels[len(initial):]
    pairs=rest[:len(rest)//2*2]; orphan=rest[-1] if len(rest)%2 else None
    for i in range(0,len(pairs),2):
        left,right=pairs[i:i+2]
        parents=[p for p in leaves if near(p['lo'],left['lo'],p['hi']-p['lo']) and near(p['hi'],right['hi'],p['hi']-p['lo']) and near((p['lo']+p['hi'])/2,left['hi'],p['hi']-p['lo']) and near(left['hi'],right['lo'],p['hi']-p['lo'])]
        if len(parents)!=1: raise ValueError(f"source-ordered refinement pair does not match one active parent: left=[{left['lo']},{left['hi']}] right=[{right['lo']},{right['hi']}]")
        leaves.remove(parents[0]); leaves.extend((left,right)); split_count+=1
    # A complete lone child cannot replace its parent until its sibling also completes.
    if orphan:
        if not partial: raise ValueError(f"odd complete refinement panel count without a trailing partial panel: [{orphan['lo']},{orphan['hi']}]")
        candidates=[p for p in leaves if (near(orphan['lo'],p['lo'],p['hi']-p['lo']) and near(orphan['hi'],(p['lo']+p['hi'])/2,p['hi']-p['lo'])) or
                    (near(orphan['lo'],(p['lo']+p['hi'])/2,p['hi']-p['lo']) and near(orphan['hi'],p['hi'],p['hi']-p['lo']))]
        if len(candidates)!=1: raise ValueError(f"unpaired complete refinement panel [{orphan['lo']},{orphan['hi']}]")
    def depth(p):
        n=0; cur=p
        while cur not in roots:
            par=[q for q in panels if q['lo']<=cur['lo']+tol(q['lo'],cur['lo'],q['hi']-q['lo']) and q['hi']>=cur['hi']-tol(q['hi'],cur['hi'],q['hi']-q['lo']) and q['hi']-q['lo']>cur['hi']-cur['lo']]
            if not par: break
            cur=min(par,key=lambda q:q['hi']-q['lo']); n+=1
        return n
    # The first cumulative counter includes earlier preparation; attribute only row-to-row deltas.
    for p in panels:
        start=p['first']; end=start+14
        p['work']=max(D(0),rows[end][5]-rows[start-1][5]) if start else max(D(0),rows[end][5]-rows[start][5])
        p['work_attributed']=bool(start)
    per=[]
    for i,(a,b) in enumerate(intervals):
        group=[p for p in leaves if p['lo']>=a-tol(a,p['lo'],b-a) and p['hi']<=b+tol(b,p['hi'],b-a)]
        measured=[p for p in panels if p['lo']>=a-tol(a,p['lo'],b-a) and p['hi']<=b+tol(b,p['hi'],b-a)]
        per.append(dict(cut_interval=[str(a),str(b)],active_leaves=len(group),panels=len(measured),initial_panels=1 if measured else 0,
            refinement_panels=max(0,len(measured)-1),evaluations=15*len(measured),estimated_integral=str(sum((p['value'] for p in group),D(0))),
            summed_G7_K15_difference=str(sum((p['error'] for p in group),D(0))),
            geometry_work_delta=str(sum((p['work'] for p in measured),D(0))),
            max_depth=max((depth(p) for p in group),default=0)))
    return dict(reconstructed_complete_panels=len(panels),partial_panel_count=1 if partial else 0,
        trailing_partial_evaluations=partial,expected_initial_panel_count=len(intervals),observed_initial_panel_count=len(initial),refinement_panel_count=max(0,len(panels)-len(initial)),
        initial_coverage_complete=coverage_complete,unresolved_initial_intervals=[[str(a),str(b)] for a,b in intervals[len(initial):]],
        total_evaluations=15*len(panels)+partial,estimated_integral=str(sum((p['value'] for p in leaves),D(0))) if coverage_complete and partial==0 else None,
        summed_G7_K15_difference=str(sum((p['error'] for p in leaves),D(0))) if coverage_complete and partial==0 else None,
        last_complete_partition_estimate=str(sum((p['value'] for p in leaves),D(0))) if coverage_complete else None,
        last_complete_partition_error=str(sum((p['error'] for p in leaves),D(0))) if coverage_complete else None,
        last_complete_partition_allowance=str(D('1e-9')*abs(sum((p['value'] for p in leaves),D(0)))+D('1e-12')) if coverage_complete else None,
        active_leaf_count=len(leaves),complete_split_parent_count=split_count,max_depth=max((depth(p) for p in leaves),default=0),per_initial_cut=per,
        initial_panel_evaluations=15*len(initial),refinement_panel_evaluations=15*(len(panels)-len(initial)),
        initial_geometry_work=str(sum((p['work'] for p in initial),D(0))),refinement_geometry_work=str(sum((p['work'] for p in panels[len(initial):]),D(0))),
        first_query_work_unassigned=str(rows[0][5]) if rows else None,
        normalized_integrand_variation=str((max(r[1] for r in rows)-min(r[1] for r in rows))/(sum((r[1] for r in rows),D(0))/len(rows))) if rows and sum((r[1] for r in rows),D(0)) else None,
        top_remaining_error_leaves=[dict(interval=[str(p['lo']),str(p['hi'])],estimated_error=str(p['error']),depth=depth(p)) for p in sorted(leaves,key=lambda p:p['error'],reverse=True)[:10]],
        interpretation="Reconstructed numerical G7/K15 estimates from sampled integrands; differences are not conservative bounds and do not certify or accept the edge.")


def read_rows(path):
    rows=[]
    with path.open(newline='') as f:
        for n,row in enumerate(csv.reader(f,delimiter='\t'),1):
            if not row or row[0].lstrip().startswith('#'): continue
            try:
                if len(row)!=6: raise ValueError(f"expected 6 columns, got {len(row)}")
                value=tuple(D(x.strip()) for x in row)
                if not all(x.is_finite() for x in value) or not (0<=value[0]<=1) or value[1]<=0 or value[5]<0 or value[5]!=value[5].to_integral_value():
                    raise ValueError("nonfinite/invalid parameter, speed or work")
                if rows and value[5]<rows[-1][5]:raise ValueError("cumulative work decreased")
                rows.append(value)
            except Exception as e: raise ValueError(f"bad sample row {n}: {e}") from e
    return rows


def self_test():
    def sample(a,b,f,swap=False,rounded=False):
        m=(a+b)/2; h=(b-a)/2; xs=[m]+[m+s*h*x for x in N for s in (-1,1)]
        # Intentionally reverse each symmetric pair's evaluation order in alternate panels.
        if swap: xs=[m]+[m+s*h*x for x in N for s in (1,-1)]
        if rounded: xs=[D(format(x,'.21g')) for x in xs]
        return [(x,f(x),0,0,0,D(i)) for i,x in enumerate(xs)]
    rows=sample(D(0),D('.5'),lambda x:D(2))+sample(D('.5'),D(1),lambda x:D(2),True)
    res=analyze(rows,[D(0),D('.5'),D(1)])
    assert abs(D(res['estimated_integral'])-D(2))<D('1e-22') and res['observed_initial_panel_count']==2
    # One initial interval refined into its exact binary children; linear integrand integrates exactly.
    rows=sample(D(0),D(1),lambda x:D(1)+x)+sample(D(0),D('.5'),lambda x:D(1)+x,True)+sample(D('.5'),D(1),lambda x:D(1)+x)
    res=analyze(rows,[D(0),D(1)])
    assert abs(D(res['estimated_integral'])-D('1.5'))<D('1e-22') and res['active_leaf_count']==2
    p,partial=panels_from_rows(sample(D(0),D(1),lambda x:D(1))[:-1]); assert len(p)==0 and partial==14
    incomplete=analyze(sample(D(0),D(1),lambda x:D(1))+sample(D(0),D('.5'),lambda x:D(1))[:-1],[D(0),D(1)])
    assert incomplete['partial_panel_count']==1 and incomplete['estimated_integral'] is None and incomplete['active_leaf_count']==1
    try: analyze(sample(D(0),D('.4'),lambda x:D(1)),[D(0),D(1)])
    except ValueError as e: assert 'initial coverage/order mismatch' in str(e)
    else: raise AssertionError('mismatched initial coverage accepted')
    assert analyze([], [D(0),D(1)])['estimated_integral'] is None
    broken=sample(D(0),D(1),lambda x:D(1));broken[3]=(D('.42'),*broken[3][1:])
    try:panels_from_rows(broken)
    except ValueError:pass
    else:raise AssertionError('malformed complete node protocol accepted')
    switched=sample(D(0),D(1),lambda x:D(1));switched[1:3],switched[3:5]=switched[3:5],switched[1:3]
    try:panels_from_rows(switched)
    except ValueError:pass
    else:raise AssertionError('wrong source node-pair order accepted')
    rounded=sample(D(0),D('.0003'),lambda x:1+x)
    rounded=[(x+D('1e-19')*D('.0003')*((-1)**j),*r[1:]) for j,r in enumerate(rounded) for x in [r[0]]]
    assert len(panels_from_rows(rounded)[0])==1
    tinylo=D('.5'); tinyhi=D('.50000001')
    tinyrows=sample(D(0),tinylo,lambda x:D(3))+sample(tinylo,tinyhi,lambda x:D(3),rounded=True)+sample(tinyhi,D(1),lambda x:D(3))
    tiny=analyze(tinyrows,[D(0),tinylo,tinyhi,D(1)])
    assert abs(D(tiny['estimated_integral'])-D(3)) < D('1e-19')


def main():
    ap=argparse.ArgumentParser(); ap.add_argument('--samples',type=Path,default=HERE/'actual_bl_edge_samples.tsv'); ap.add_argument('--cuts',type=Path,default=HERE/'actual_bl_edge_cuts.jsonl'); ap.add_argument('--self-test',action='store_true'); args=ap.parse_args()
    if args.self_test: self_test(); print('self-test passed'); return
    if not args.samples.exists() or not args.cuts.exists(): raise SystemExit(f"missing actual trace inputs: samples={args.samples.exists()} cuts={args.cuts.exists()}")
    cutsrows=[json.loads(x,parse_float=D) for x in args.cuts.read_text().splitlines() if x.strip()]
    if len(cutsrows)!=1: raise SystemExit(f"expected one traced edge context, found {len(cutsrows)}")
    c=cutsrows[0]
    try: result=analyze(read_rows(args.samples),[D(x) for x in c['cuts']])
    except Exception as e: raise SystemExit(f"trace reconstruction failed: {e}") from e
    case_dir=args.samples.resolve().parent
    result.update(schema='native3d_target_panels389_reconstruction_v1',edge_a=c['a'],edge_b=c['b'],
        input_sha256={str(args.samples.resolve()):hashlib.sha256(args.samples.read_bytes()).hexdigest(),str(args.cuts.resolve()):hashlib.sha256(args.cuts.read_bytes()).hexdigest()},
        terminal_status={str(p):json.loads(p.read_text()) for p in [case_dir/'status.json',case_dir/'terminal_handles.json',HERE/'build389_status.json',HERE/'build389_terminal_handles.json'] if p.exists()},
        input_case_pins=(json.loads((case_dir/'pins.json').read_text()) if (case_dir/'pins.json').exists() else None),
        source_pins=(json.loads((HERE/'build389_source_pins.json').read_text()) if (HERE/'build389_source_pins.json').exists() else None),
        binary_pins=(json.loads((HERE/'build389_status.json').read_text()).get('binaries') if (HERE/'build389_status.json').exists() else None))
    out=HERE/'target_panels389_assessment.json'; out.write_text(json.dumps(result,indent=2,default=str)+'\n'); print(out)

if __name__=='__main__': main()
