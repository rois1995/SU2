"""Render the requested <=80-line report from saved counter/load evidence."""
import json
from pathlib import Path
ROOT=Path(__file__).resolve().parent;REPO=ROOT.parents[4]
def read(name):return json.loads((ROOT/name).read_text())
d=read('comparison_summary.json');load=read('load_summary.json');status=read('run/status.json');outcomes=read('outcomes.json');running=status['state']!='terminal'
ref=d['reference'];cur=d['current'];global_counts=read('first_collapse_global_counters.json')
labels=[('collapse_setup','C* collapse'),('surface_setup','S* surface'),('interior','I interior'),('repartition_setup','R* repartition/setup'),('quality','Q* quality/return'),('alternation_setup','A* alternation/re-entry'),('finish','F finish')]
def current(i):return cur[i] if i<len(cur) else None
def inner(i,profile=False):
    if profile:return [5344.540316,5324.459194][i]
    return current(i)['ranks'][0]['native_inner_seconds'] if current(i) else None
def seconds(v):return f'{v:.6f}' if v is not None else 'pending'
def mesh(i):
    row=outcomes.get(str(i+1))
    if not row:return 'pending'
    return f"{row['points']}/{row['tets']} ({'accepted' if row['accepted'] else 'retained'})"
def result(i):
    row=outcomes.get(str(i+1));return ('accepted' if row['accepted'] else 'rejected, '+row['reason']) if row else 'pending'
def phase_text(event,key):
    if not event or key not in event['phases']:return 'not separately emitted'
    v=event['phases'][key];mean=v['world_election_mean_us_rank0'];return f"{v['max_rank_boundary_seconds']:.2f}; {int(v['global_accepted_edits'])}; {int(v['world_election_calls_common_per_rank'])}; {mean:.1f}" if mean is not None else f"{v['max_rank_boundary_seconds']:.2f}; {int(v['global_accepted_edits'])}; 0; —"
def pair(old,new,key,other=None):
    def text(row):
        if row is None:return 'pending'
        x=row[key]
        if other:return f"{x:.2f}/{row[other]}"
        return f'{x:.2f}'
    return text(old)+' → '+text(new)
def geom(old,new):
    return f"{old['geometry_trials']}/{old['geometry_rejected']} → "+(f"{new['geometry_trials']}/{new['geometry_rejected']}" if new else 'pending')
def load_text(i,key):
    events={e['invocation']:e for e in load['events']};v=events.get(i,{}).get('phases',{}).get(key,{})
    return f"{v['n']}: {v['min']:.2f}/{v['median']:.2f}/{v['max']:.2f}" if v.get('n') else 'no samples/window'
lines=['# M6 build-494 no-BL unprofiled check — 2026-11-14','', '**IN PROGRESS: second configured adaptation remains active.**' if running else f"**Driver exit {status['exit_code']}; the single prescribed unprofiled run is terminal.**",'We could not wait for the quiet-load condition: after 24.5 minutes of failed rechecks, the user instructed launch at load 2.72 (required <1.0). The CPU-use gate passed; the load condition was explicitly waived.','No native source/build/config/gate/budget/limit changes; no perf, gdb or rank-stopping observer. nice 19, six ranks, single-thread libraries; /proc receipts every ~10 seconds.','Evidence: `integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/nobl_clean_check_20261114`; exact hashes/argv/environment: `run/provenance.json`; program SHA 0311b5a2a4f871bd… and config SHA 5ad2dc1d9a1cd8bf…. All 1539 build-494 source pins matched.','', '## T1 — outcomes (first-adaptation clocks/counts are rank 0)','','| Quantity | Build 293 | Build 494 profiled/busy | This unprofiled run |','|---|---|---|---|','| Collapse exit seconds | 194.723054 | 1423.509701 | 1410.686670 |','| Accepted collapses | 480 | 480 | 480 |','| First split seconds/splits | 241.14/5047 | 1609.19/3520 | 1593.361908/3520 |','| Second split seconds/splits | 274.10/9949 | 1737.87/8442 | 1722.372641/8442 |',f"| Adaptation 1 inner seconds/result | {ref[0]['ranks'][0]['native_inner_seconds']:.6f}, accepted | {inner(0,True):.6f}, rejected | {seconds(inner(0))}, {result(0)} |",f'| Adaptation 1 points/tets | 149051/736323 accepted | 61330/303359 retained | {mesh(0)} |',f"| Adaptation 2 inner seconds/result | {ref[1]['ranks'][0]['native_inner_seconds']:.6f}, accepted | {inner(1,True):.6f}, rejected | {seconds(inner(1))}, {result(1)} |",f'| Adaptation 2 points/tets | 172001/870422 accepted | 61330/303359 retained | {mesh(1)} |','Second inputs diverge after the first result: current CFD retained the original mesh; reference CFD continued on its first accepted adaptation. Complexity 50,000 is nominal, not a point-count gate.','', '## T2 — first-adaptation boundary bins','','Each cell=max-rank boundary seconds; global accepted edits; common per-rank World election calls; rank-0 mean election µs. World counts are not summed over six ranks.','| Phase | Build 293 | This run |','|---|---|---|']
for key,label in labels:lines.append(f'| {label} | {phase_text(ref[0],key)} | {phase_text(current(0),key)} |')
a=ref[0]['ranks'];b=cur[0]['ranks']
lines += [f"C/S/R/A* include setup/re-entry; Q* includes quality through return, and the current duration is budget-censored. Finish has no separate emitted boundary. Named S/I lane maxima: 293 = {max(r['surface_lane_seconds'] for r in a):.2f}/{max(r['interior_lane_seconds'] for r in a):.2f}s; current = {max(r['surface_lane_seconds'] for r in b):.2f}/{max(r['interior_lane_seconds'] for r in b):.2f} s; these scopes differ from the bins.",'','## T3 — per-rank exit counters','','Every cell is build 293 → build 494. Invocation/rank identifies rows. U=upper seconds/calls; E=World election seconds/calls; update/other are seconds; G=geometry trials/rejections. Timers overlap; do not add them. Directory elections are separate and retained in `comparison_summary.json`.','| Inv/rank | U | E | Update | G | Other |','|---|---|---|---|---|---|']
for i in range(2):
    for rank in range(6):
        old=ref[i]['ranks'][rank];new=current(i)['ranks'][rank] if current(i) else None
        lines.append(f"| {i+1}/{rank} | {pair(old,new,'upper_seconds','upper_calls')} | {pair(old,new,'world_election_seconds','world_election_calls')} | {pair(old,new,'update_seconds')} | {geom(old,new)} | {pair(old,new,'other_seconds')} |")
lines += ['', '## T4 — sampled host load','','Cells=N samples: min/median/max in timestamped marker windows. C* begins at the preceding input PARTITION log; no native epoch was estimated.','| Phase | Adaptation 1 | Adaptation 2 |','|---|---|---|']
for key,label in labels:lines.append(f'| {label} | {load_text(1,key)} | {load_text(2,key)} |')
lines += ['Poll cadence ~10s; MPI log times have 1s precision and unmeasured forwarding delay. Unsampled extrema and subsecond handoffs are unresolved.','', '## T5 — verdict','','**'+('Provisional ' if running else '')+'2 — the no-BL M6 path is slower on build 494 under the authorized launch conditions.** Collapse is 7.24× and the surface/setup bin is '+f"{cur[0]['phases']['surface_setup']['max_rank_boundary_seconds']/ref[0]['phases']['surface_setup']['max_rank_boundary_seconds']:.2f}× the historical wall time;"+' the first native attempt was rejected instead of accepted.',f"First-collapse global upper-work units {global_counts['build293']['upper_work']:,}→{global_counts['build494']['upper_work']:,} ({global_counts['work_ratio']:.2f}×), calls {global_counts['build293']['upper_calls']}→{global_counts['build494']['upper_calls']}, edits 2,495 → 2,496. Rank 3 proof 28.65→934.26 s and work 4.56M→830.25M, with 263 accepted collapses in both.",'The original strict quiet-host condition was not met. Host effects and the older test-driver versus current CFD execution context are not isolated; no timing correction or causal attribution is claimed.','', '## T6 — ranked read-only source candidates','','Exact old/new file contents were reverified against both pins (`source_comparison.json`, `source_primary_review.json`); matching per-file commits are not build IDs. Candidates were not tested.','','1. `76f10c8310`: `EntityFaceError`, `BoundPhysicalSurface`, `QuerySurfaceUnion*` use a separate ErrorMagnitude purpose above tolerance and require the covering upper/lower gap to close; strongest explanation for added queries/refinement.','2. `35e173b043`: `EntityFaceError` introduced a follow-up certificate query relative to 293; this is a call-path ancestor later modified by 76f10c8310, not an independent additive cause.','3. `54d2ca2bd4`: `QuerySurfaceUnionAtResolution` adds diameter/interval arithmetic and Lipschitz tightening; it may reduce refinement, so its net cost direction is unresolved. No fix/design/refactoring recommendation.']
assert len(lines)<=80,(len(lines),'line cap')
(REPO/'Handbook/ADAPTATION_M6_NOBL_CLEAN_CHECK_20261114.md').write_text('\n'.join(lines)+'\n');print('Report lines',len(lines))
