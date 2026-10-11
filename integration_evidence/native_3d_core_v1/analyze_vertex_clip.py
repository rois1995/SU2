"""Qualify exact-vertex clipping against matched production and isolated evidence."""
from collections import defaultdict
from pathlib import Path
import csv,json,re,statistics
from analyze_insertion import HERE,qualify,sha,parse,closes,ENGINE
from analyze_donor_reuse import FILES

def main():
    matrix=json.loads((HERE/'VERTEX_CLIP_MATRIX.json').read_text())
    larger=json.loads((HERE/'LARGER_WEDGE_VALIDATION.json').read_text());assert larger['status']=='PASS'
    assert larger['analyzer_sha256']==sha(HERE/'analyze_larger_wedge.py')
    assert larger['matrix_sha256']==sha(HERE/'LARGER_WEDGE_MATRIX.json')
    assert larger['csv_sha256']==sha(HERE/'larger_wedge_rank_phases.csv')
    reg=qualify('vertex_clip_regression_v1',('source_drift','object_drift','archive_drift'))
    assert len(reg['actual_objects_sha256'])==68
    for ranks in (1,2,4):
        out=(HERE/'vertex_clip_regression_v1'/f'mpi{ranks}.stdout').read_text()
        assert 'No test cases matched' not in out
        assert len(re.findall(r'All tests passed \(\d+ assertions in 66 test cases\)',out))==ranks
    assert 'All tests passed (48 assertions in 2 test cases)' in (HERE/'vertex_clip_regression_v1/mmg_serial.stdout').read_text()
    probe=qualify('vertex_clip_probe_v1',());assert probe['baseline_revision']==matrix['base']
    for path,digest in probe['input_sha256'].items():assert sha(Path(path))==digest
    assert sha(HERE/'probe_vertex_clip.py')==probe['pins']['integration_evidence/native_3d_core_v1/probe_vertex_clip.py']
    predicate='Common/src/adaptation/CNativePredicates3D.cpp'
    assert sha(HERE.parents[1]/predicate)==sha(HERE/'vertex_clip_probe_v1/candidate_original.cpp')
    assert sha(HERE/'vertex_clip_probe_v1/baseline_original.cpp')==probe['pins'][predicate]
    syntax=qualify('vertex_clip_syntax_v1',())
    assert syntax['source_sha256']==sha(HERE.parents[1]/predicate)
    assert len(syntax['stages'])==3 and [s['mode'] for s in syntax['stages']]==['scalar','DD-macro','AD-macro']
    kernels=[];samples=defaultdict(list)
    for line in (HERE/'vertex_clip_probe_v1/probe.stdout').read_text().splitlines():
        row=parse(line)
        if line.startswith('EQUALITY '):kernels.append(row)
        elif line.startswith('TIMING '):samples[(int(row['cycle']),int(row['candidate']))].append(row)
        else:raise AssertionError(line)
    assert len(kernels)==3
    for row,setup in zip(kernels,probe['sets']):
        cycle=int(row['cycle']);assert setup['cycle']==cycle and setup['chords']==row['chords'] and setup['over256_candidate_chords_skipped']==0
        assert row['candidate_exact']<row['baseline_exact']
        for c in (0,1):assert sorted(int(r['repeat']) for r in samples[(cycle,c)])==[0,1,2]
        old=[r['seconds'] for r in samples[(cycle,0)]];new=[r['seconds'] for r in samples[(cycle,1)]]
        row.update(baseline_seconds=old,candidate_seconds=new,median_reduction_fraction=1-statistics.median(new)/statistics.median(old))
    rows=[];cases=[];production=None;binary=None
    files=FILES+('envelope_s2_written_SU2_reload_target.json','envelope_s2_written_CGNS_reload_target.json')
    for entry in matrix['cases']:
        name=entry['folder'];receipt=qualify(name,('changed_during_run','archive_changes','object_changes'))
        old=qualify(entry['baseline'],('changed_during_run','archive_changes','object_changes'))
        assert set(receipt['source_sha256'])==set(old['source_sha256'])
        changed=[k for k,v in old['source_sha256'].items() if receipt['source_sha256'][k]!=v]
        assert set(changed)=={predicate,'UnitTests/Common/adaptation/CNativeMesh3D_tests.cpp'}
        assert receipt['actual_test_object_sha256']==old['actual_test_object_sha256']
        assert set(receipt['thin_archive_member_sha256'])==set(old['thin_archive_member_sha256'])
        members=[k for k,v in old['thin_archive_member_sha256'].items() if receipt['thin_archive_member_sha256'][k]!=v]
        assert len(members)==1 and members[0].endswith('/adaptation_CNativePredicates3D.cpp.o')
        assert set(receipt['library_sha256'])==set(old['library_sha256'])
        assert all(k.endswith('/libSU2AccurateSum.a') for k,v in old['library_sha256'].items() if receipt['library_sha256'][k]!=v)
        pins=(receipt['library_sha256'],receipt['thin_archive_member_sha256'])
        if production is None:production=pins;binary=receipt['binary_sha256']
        else:assert pins==production and receipt['binary_sha256']==binary
        assert all(reg['source_sha256'][k]==v for k,v in receipt['source_sha256'].items() if k in reg['source_sha256'])
        assert all(reg['library_sha256'].get(k)==v for k,v in pins[0].items()) and reg['thin_archive_member_sha256']==pins[1]
        assert all(reg['actual_objects_sha256'].get(k)==v for k,v in receipt['actual_test_object_sha256'].items())
        unchanged={f:sha(HERE/name/f) for f in files if (HERE/name/f).read_bytes()==(HERE/entry['baseline']/f).read_bytes()}
        assert len(unchanged)==13,(name,'fixed-work data changed')
        a=json.loads((HERE/name/'independent_audit.json').read_text());o=json.loads((HERE/entry['baseline']/'independent_audit.json').read_text())
        assert a['status']=='PASS' and a['adversaries_rejected']==6
        assert {k:v for k,v in a['parameters'].items() if k not in ('folder','binary_from')}=={k:v for k,v in o['parameters'].items() if k not in ('folder','binary_from')}
        assert all(a['parameters'][k]==entry[k] for k in ('ranks','workers'))
        assert a['frames']==o['frames'] and a['states']==o['states'] and a['reload_metrics']==o['reload_metrics']
        details=[]
        for line in (HERE/name/'case.stdout').read_text().splitlines():
            if not line.startswith('NATIVE3D_DETAIL '):continue
            row=parse(line)
            if row['stage'] in ('refine','coarsen'):closes(row,ENGINE)
            elif row['stage'] in ('import','weights','return'):closes(row,('points','cells','faces','source','query','reader'))
            elif row['stage']=='partition':closes(row,('graph','parmetis','migration'))
            else:assert row['stage']=='source'
            details.append(row)
        rows.extend(dict(case=name,record='exclusive',**r) for r in a['rank_stages'])
        rows.extend(dict(case=name,record='nested',**r) for r in details)
        cycles=[]
        for cycle in range(3):
            parent=[r for r in a['rank_rows'] if r['cycle']==cycle];previous=[r for r in o['rank_rows'] if r['cycle']==cycle]
            native=[[r for r in a['native_rows'] if r['rank']==rank][cycle] for rank in range(entry['ranks'])]
            old_native=[[r for r in o['native_rows'] if r['rank']==rank][cycle] for rank in range(entry['ranks'])]
            ops=sum(r['refine_accepted']+r['coarsen_accepted'] for r in native)
            assert [(r['initial'],r['final'],r['refine_accepted'],r['coarsen_accepted']) for r in native]==[(r['initial'],r['final'],r['refine_accepted'],r['coarsen_accepted']) for r in old_native]
            wall=max(r['parent_wall'] for r in parent)
            cycles.append(dict(cycle=cycle,parent_max_seconds=wall,baseline_parent_max_seconds=max(r['parent_wall'] for r in previous),
                accepted_edits=ops,accepted_edits_per_parent_second=ops/wall,parent_max_over_mean=wall/statistics.mean(r['parent_wall'] for r in parent),
                native_bound=max(r['native_bound'] for r in native),lifetime_rss=max(r['process_peak_rss'] for r in native)))
        cases.append(dict(folder=name,baseline=entry['baseline'],unchanged=unchanged,cycles=cycles,
            production_member_changed=members[0],validation_sha256=sha(HERE/name/'validation.json'),audit_sha256=sha(HERE/name/'independent_audit.json')))
    keys=['case','record','stage','rank','ranks']+sorted({k for r in rows for k in r}-{'case','record','stage','rank','ranks'})
    with (HERE/'vertex_clip_rank_phases.csv').open('w',newline='') as f:
        writer=csv.DictWriter(f,fieldnames=keys,lineterminator='\n');writer.writeheader();writer.writerows(rows)
    out=dict(status='PASS',analyzer_sha256=sha(Path(__file__)),dependencies_sha256={n:sha(HERE/n) for n in ('analyze_insertion.py','analyze_donor_reuse.py','audit_lifecycle.py')},
        matrix_sha256=sha(HERE/'VERTEX_CLIP_MATRIX.json'),larger_validation_sha256=sha(HERE/'LARGER_WEDGE_VALIDATION.json'),
        regression_sha256=sha(HERE/'vertex_clip_regression_v1/validation.json'),syntax_sha256=sha(HERE/'vertex_clip_syntax_v1/validation.json'),
        probe_sha256=sha(HERE/'vertex_clip_probe_v1/validation.json'),kernels=kernels,cases=cases,accounting_rows=len(rows),csv_sha256=sha(HERE/'vertex_clip_rank_phases.csv'),
        scope='Exact-vertex numerator reuse only. Four fixed-work MPI1/2/4 M=N/M<N cases:52 raw geometry/source/state/readback files byte-identical, one production archive member changed, full68-object configured native/2D+3D bridge and serialMMG regression. Passive TU scalar/DD-macro/AD-macro syntax only, no active AD solver link/run claim. Three alternating isolated kernel samples; single contended actual pilots establish no full-cycle speedup/scaling. Global reference256-face limit, communication/private-wait/memory calibration, detail overhead and SGE repetitions remain open; fullGoal1 ACTIVE.')
    (HERE/'VERTEX_CLIP_VALIDATION.json').write_text(json.dumps(out,indent=2)+'\n')
    print('PASS',len(cases),'fixed-work cases;',len(cases)*len(files),'byte-identical files;',len(rows),'closed phase rows')
if __name__=='__main__':main()
