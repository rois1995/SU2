"""Qualify the frozen stronger-anisotropy actual-driver cases and their costs."""
from pathlib import Path
import json
from analyze_insertion import HERE,qualify,sha,parse,closes,ENGINE
CASES=('robustness_a2_n2_n1_v1','robustness_a2_n2_n4_m2_v1')
def main():
    baseline=qualify('donor_reuse_regression_v4',('source_drift','object_drift','archive_drift'))
    cases=[]
    for name in CASES:
        receipt=qualify(name,('changed_during_run','archive_changes','object_changes'))
        assert all(baseline['library_sha256'].get(k)==v for k,v in receipt['library_sha256'].items())
        assert receipt['thin_archive_member_sha256']==baseline['thin_archive_member_sha256']
        assert all(baseline['source_sha256'][k]==v for k,v in receipt['source_sha256'].items() if k in baseline['source_sha256'])
        a=json.loads((HERE/name/'independent_audit.json').read_text());assert a['status']=='PASS'
        p=a['parameters'];assert (p['size'],p['anisotropy'],p['varying'],p['diagnostic_rounds'],p['aligned_markers'])==(2,2,1,512,True)
        assert (p['ranks'],p['workers'])==((1,1) if name.endswith('n1_v1') else (4,2))
        for line in (HERE/name/'case.stdout').read_text().splitlines():
            if not line.startswith('NATIVE3D_DETAIL '):continue
            r=parse(line)
            if r['stage'] in ('refine','coarsen'):closes(r,ENGINE)
            elif r['stage'] in ('import','weights','return'):closes(r,('points','cells','faces','source','query','reader'))
            elif r['stage']=='partition':closes(r,('graph','parmetis','migration'))
            else:assert r['stage']=='source'
        frames=[r for r in a['frames'] if r['file'] in ('envelope_s0.json','envelope_s1.json','envelope_s2.json')]
        assert len(frames)==len(a['states'])==3
        cycles=[]
        for c in range(3):
            rows=[r for r in a['rank_rows'] if r['cycle']==c];wall=max(r['parent_wall'] for r in rows)
            native=[[r for r in a['native_rows'] if r['rank']==rank][c] for rank in range(p['ranks'])]
            edits=sum(r['refine_accepted']+r['coarsen_accepted'] for r in native)
            cycles.append(dict(cycle=c,parent_max_seconds=wall,remesh_max_seconds=max(r['total'] for r in native),accepted_edits=edits,
                accepted_edits_per_parent_second=edits/wall,native_bound=max(r['native_bound'] for r in native),lifetime_rss=max(r['process_peak_rss'] for r in native)))
        cases.append(dict(folder=name,cells=[r['cells'] for r in frames],minimum_q=min(r['minimum_mean_ratio'] for r in frames),
            minimum_J=min(r['minimum_scaled_jacobian'] for r in frames),maximum_L=max(r['maximum_length'] for r in frames),
            state_error=max(r['maximum_state_relative_error'] for r in a['states']),tensor_error=max(r['maximum_transported_tensor_absolute_error'] for r in a['states']),
            validation_sha256=sha(HERE/name/'validation.json'),audit_sha256=sha(HERE/name/'independent_audit.json'),cycles=cycles))
    result=dict(status='PASS',matrix_sha256=sha(HERE/'ROBUSTNESS_EXPANSION_MATRIX.json'),analyzer_sha256=sha(Path(__file__)),
        dependencies_sha256={n:sha(HERE/n) for n in ('analyze_insertion.py','audit_lifecycle.py')},regression_validation_sha256=sha(HERE/'donor_reuse_regression_v4/validation.json'),
        cases=cases,scope='Two actual default512 manufactured box lifecycles, aspect2 constant rotated eigenframe with spatial strength variation; MPI1/M1 and MPI4/M2, independent geometry/original-P1/current+BDF2/SU2CGNS audits and closed accounting. Single contended pilots, no spatial direction-change, thin/wedge/restart/fullGoal1/scaling/CFD affordability claim.')
    (HERE/'ROBUSTNESS_STRENGTH_VALIDATION.json').write_text(json.dumps(result,indent=2)+'\n')
    print('PASS',len(cases),'stronger-anisotropy driver cases')
if __name__=='__main__':main()
