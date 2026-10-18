#!/usr/bin/env python3
"""Summarize pinned accepted lifecycles; correctness receipts, never a scaling benchmark."""
import hashlib,json,re
from pathlib import Path
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2];BASE=HERE.with_name('planar_symmetry_reference_v1')
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
def fields(line):
    values=dict(re.findall(r'(\w+)=([^ ]+)',line));result={}
    for k,v in values.items():
        try:result[k]=float(v) if any(c in v for c in '.eE') else int(v)
        except ValueError:result[k]=v
    return result
def main():
    reports=[]
    for name in ('annulus_v10_n1','annulus_v10_n2','annulus_v10_n4','annulus_all_v10_n4'):
        folder=BASE/name;proof=json.loads((folder/'provenance.json').read_text());result=json.loads((folder/'result.json').read_text())
        assert result['exit_code']==0 and sha(folder/'provenance.json')==result['provenance_sha256'] and sha(folder/'run.log')==result['log_sha256']
        for file,digest in result['artifacts'].items():assert sha(folder/file)==digest
        audit=json.loads((folder/'independent_annulus.json').read_text());assert audit['status']=='INDEPENDENT_ANNULAR_SYMMETRY_LIFECYCLE_PASS'
        record=audit['cases'][0];assert record['accepted'] and record['flow_restart_residual']==record['SA_restart_residual']==0
        native=[];metric={};detail=[];replace=[];before_native=True
        for line in (folder/'run.log').read_text().splitlines():
            if line.startswith('NATIVE3D '):
                data=fields(line)
                if data['complete']==1:native.append(data)
                before_native=False
            elif line.startswith('NATIVE3D_STAGE stage=metric_build') and before_native:
                data=fields(line);metric[data['rank']]=data
            elif line.startswith('NATIVE3D_STAGE stage=replace'):replace.append(fields(line))
            elif line.startswith('NATIVE3D_DETAIL stage='):
                data=fields(line)
                if data.get('stop')=='complete':detail.append(data)
        assert len(native)==proof['ranks'] and len(replace)==proof['ranks'] and len(metric)==proof['ranks']
        assert all(data['workers']==record['workers'] for data in native)
        work=sum(max(d['global_work'] for d in detail if d['stage']==phase) for phase in ('refine','coarsen'))
        reports.append(dict(folder=name,CFD_ranks=proof['ranks'],workers=record['workers'],independent=record,shared_work=work,
            fixture_elapsed=result['elapsed_seconds'],metric_build_max=max(d['total'] for d in metric.values()),
            native_event_max=max(d['total'] for d in native),replace_max=max(d['total'] for d in replace),
            native_per_rank=native,metric_build_per_rank=list(metric.values()),replace_per_rank=replace,
            phase_work_and_cost=[{k:d[k] for k in ('stage','rank','workers','global_work','accepted','total','reconstruction','metric','donor','dependency','reservation','publication','update','other','process_cpu','requests','hits','misses','compositions','edge_evaluations','edge_reused','tracked_bytes','tracked_calls','native_bound')} for d in detail],
            hashes={str(p.relative_to(ROOT)):sha(p) for p in (folder/'provenance.json',folder/'result.json',folder/'run.log',folder/'independent_annulus.json')}))
    checks={}
    for n in (1,2):
        stem=f'symmetry_annulus_bl_budget_20_m{n}_frozen_profile.json'
        checks[f'unchanged_frozen_profile_N{n}']=sha(BASE/f'annulus_v9_n{n}'/stem)==sha(BASE/f'annulus_v10_n{n}'/stem)
    checks['byte_identical_accepted_serial_SU2']=sha(BASE/'annulus_v9_n1/symmetry_annulus_bl_budget_20_m1_written_SU2.su2')==sha(BASE/'annulus_v10_n1/symmetry_annulus_bl_budget_20_m1_written_SU2.su2')
    assert all(checks.values())
    output=dict(status='FOUR_ACCEPTED_ANNULAR_LIFECYCLES_INDEPENDENTLY_PASS',cases=reports,compatibility=checks,
        coordinate_admission=dict(candidates_per_leaf=27,leaves_per_proposal=4,maximum_proposals=4,extra_density_or_MPI_queries=0,
            work_scope='Up to432single-face geometry/height checks times imported wall-face count; included in reconstruction timing, not separately timed'),
        source_sha256=sha(Path(__file__)),limits=['Contended host and distinct valid edit sequences; no scaling/speedup or unsteady-affordability conclusion',
            'Tracked bytes/calls measure collective traffic, not peak memory. Process peak RSS is per process, not aggregate whole-job peak',
            'Metric initialization precedes native event; return CFD geometry is nested in replacement. Never sum nested clocks twice',
            'Full nodal composed gradation remains uncertified; sensor fixed point alone is insufficient',
            'Heuristic integration indicators, no proven error bound or monotone refined-partition accuracy',
            'Controlled Cartesian single-hole initialized RANS/SA, no physical viscous convergence or general aerodynamic readiness'])
    (HERE/'lifecycle_summary.json').write_text(json.dumps(output,indent=2)+'\n')
    for r in reports:print(r['folder'],r['independent']['cells'],r['shared_work'],r['metric_build_max'],r['native_event_max'],r['replace_max'],r['fixture_elapsed'],flush=True)
if __name__=='__main__':main()
