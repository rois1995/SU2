"""Qualify canonical physical binding against exhaustive tests and fixed-work meshes."""
from pathlib import Path
import csv,json,re,statistics
from analyze_insertion import HERE,qualify,sha,parse,closes,ENGINE
from analyze_donor_reuse import FILES

def main():
    matrix=json.loads((HERE/'REFERENCE_LOOKUP_MATRIX.json').read_text())
    reg=qualify(matrix['regression'],('source_drift','object_drift','archive_drift'))
    assert len(reg['actual_objects_sha256'])==68
    for ranks in (1,2,4):
        out=(HERE/matrix['regression']/f'mpi{ranks}.stdout').read_text()
        assert 'No test cases matched' not in out
        assert len(re.findall(r'All tests passed \(\d+ assertions in 67 test cases\)',out))==ranks
    assert 'All tests passed (48 assertions in 2 test cases)' in (HERE/matrix['regression']/'mmg_serial.stdout').read_text()
    files=FILES+('envelope_s2_written_SU2_reload_target.json','envelope_s2_written_CGNS_reload_target.json')
    production=None;binary=None;cases=[];rows=[]
    for entry in matrix['cases']:
        name=entry['folder'];receipt=qualify(name,('changed_during_run','archive_changes','object_changes'))
        old=qualify(entry['baseline'],('changed_during_run','archive_changes','object_changes'))
        assert set(receipt['source_sha256'])==set(old['source_sha256'])
        changed=[k for k,v in old['source_sha256'].items() if receipt['source_sha256'][k]!=v]
        assert set(changed)=={'Common/include/adaptation/CNativeBoundary3D.hpp','Common/src/adaptation/CNativeBoundary3D.cpp','Common/src/adaptation/CNativeImport3D.cpp','UnitTests/Common/adaptation/CNativeBoundary3D_tests.cpp'}
        assert set(receipt['thin_archive_member_sha256'])==set(old['thin_archive_member_sha256'])
        members=[k for k,v in old['thin_archive_member_sha256'].items() if receipt['thin_archive_member_sha256'][k]!=v]
        assert members and all('Native' in Path(k).name and '3D' in Path(k).name or Path(k).name=='drivers_CSinglezoneDriver.cpp.o' for k in members),members
        assert set(receipt['library_sha256'])==set(old['library_sha256'])
        libs=[k for k,v in old['library_sha256'].items() if receipt['library_sha256'][k]!=v]
        assert all(Path(k).name in ('libSU2core.a','libSU2Common.a','libSU2AccurateSum.a') for k in libs),libs
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
            production_members_changed=members,validation_sha256=sha(HERE/name/'validation.json'),audit_sha256=sha(HERE/name/'independent_audit.json')))
    keys=['case','record','stage','rank','ranks']+sorted({k for r in rows for k in r}-{'case','record','stage','rank','ranks'})
    with (HERE/'reference_lookup_rank_phases.csv').open('w',newline='') as f:
        writer=csv.DictWriter(f,fieldnames=keys,lineterminator='\n');writer.writeheader();writer.writerows(rows)
    out=dict(status='PASS',analyzer_sha256=sha(Path(__file__)),dependencies_sha256={n:sha(HERE/n) for n in ('analyze_insertion.py','analyze_donor_reuse.py','audit_lifecycle.py')},
        matrix_sha256=sha(HERE/'REFERENCE_LOOKUP_MATRIX.json'),regression_sha256=sha(HERE/matrix['regression']/'validation.json'),cases=cases,
        accounting_rows=len(rows),csv_sha256=sha(HERE/'reference_lookup_rank_phases.csv'),
        scope='Canonical finite oriented physical binding only. Same-marker singleton direct lookup; multiple patches use existing local ADT conservative boxes and exact final checks. Exhaustive equivalence/gap/marker/orientation/nextafter/nonfinite/rejection and conservative storage controls in actual full68-object configured MPI1/2/4 and serialMMG regression. Two fixed-work actual cases preserve26 raw geometry/source/state/SU2CGNS readback files and accepted rank-local edits. Single contended timing pilots; no full-cycle speedup, exact allocator calibration or global-size completion claim. Global256-facet and1MiB-sidecar limits still enforced; scalable certification/count admission, complete profiling/overhead/SGE campaigns remain open. FullGoal1 ACTIVE.')
    (HERE/'REFERENCE_LOOKUP_VALIDATION.json').write_text(json.dumps(out,indent=2)+'\n')
    print('PASS',len(cases),'fixed-work cases;',len(cases)*len(files),'byte-identical files;',len(rows),'closed phase rows')
if __name__=='__main__':main()
