"""Reuse a complete independent certificate only for exact mesh/field/state data; recheck current files and accounting."""
from pathlib import Path
import argparse,copy,hashlib,json,re,sys
from audit_envelope import WEDGE_MARKERS,MARKERS,STAGES
from audit_geometry_bridge import signature,su2_signature,cgns_signature
from audit_lifecycle import parse,stage,nested_stage,closes,transport_counters
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()

def main(folder,reference,partial=False,work_reference=None):
    where=Path(__file__).resolve().parent
    prior=json.loads((reference/'validation.json').read_text())
    certificate=json.loads((reference/'independent_audit.json').read_text())
    assert prior['status']==certificate['status']=='PASS'
    assert sha(reference/'independent_audit.json')==prior['artifact_sha256']['independent_audit.json']
    assert all(sha(where/n)==h for n,h in prior['tools_sha256'].items()),'Reference auditor changed'
    p=json.loads((folder/'parameters.json').read_text());q=certificate['parameters']
    assert p==({**q,'diagnostic_rounds':512} if partial else q)
    cycles=range(1 if partial else 3)
    frames=[r for r in certificate['frames'] if not partial or r['file'] in ('envelope_s0_initial.json','envelope_s0.json')]
    states=[r for r in certificate['states'] if not partial or r['file']=='envelope_s0_states.json']
    names={r['file'] for r in frames+states}
    names.update(('envelope.cfg','envelope_input.su2'))
    if partial:names.add('envelope_s1_initial.json')
    for cycle in cycles:names.add(f'envelope_mesh_{cycle+1:05d}.su2.native_ref')
    if not partial:
        names.update(('envelope_s2_written_SU2_reload.json','envelope_s2_written_CGNS_reload.json'))
    matched={}
    for name in sorted(names):
        expected=prior['artifact_sha256'][name]
        assert sha(reference/name)==expected,'Reference artifact drift: '+name
        assert sha(folder/name)==expected,'Current numerical artifact differs: '+name
        matched[name]=expected
    # A deliberately changed field cannot inherit an otherwise matching geometry certificate.
    sample=(reference/'envelope_s0_states.json').read_bytes()
    assert hashlib.sha256(sample+b' ').hexdigest()!=matched['envelope_s0_states.json']
    markers=WEDGE_MARKERS if p['wedge'] else MARKERS
    outputs=[]
    for cycle in cycles:
        data=json.loads((folder/f'envelope_s{cycle}.json').read_text())
        actual=folder/f'envelope_mesh_{cycle+1:05d}.su2'
        assert su2_signature(actual,markers)==signature(data,markers)
        outputs.append(dict(file=actual.name,sha256=sha(actual)))
    if not partial:
        for fmt in ('SU2','CGNS'):
            actual=folder/('envelope_s2_written_'+fmt+('.cgns' if fmt=='CGNS' else '.su2'))
            assert (su2_signature(actual,markers) if fmt=='SU2' else cgns_signature(actual,markers))==signature(data,markers)
            outputs.append(dict(file=actual.name,sha256=sha(actual)))
    lines=(folder/'case.stdout').read_text().splitlines()
    native=[parse(s) for s in lines if s.startswith('NATIVE3D ')]
    rank_rows=[parse(s) for s in lines if s.startswith('ENVELOPE ')]
    details=[parse(s) for s in lines if s.startswith('NATIVE3D_DETAIL ')]
    stages=[parse(s) for s in lines if s.startswith('NATIVE3D_STAGE ')]
    assert len(native)==(2 if partial else 3)*p['ranks']
    assert len(rank_rows)==len(cycles)*p['ranks']
    if partial:assert [r['complete'] for r in native]==[1,0]
    else:
        assert all(r['complete']==1 for r in native)
        assert len(re.findall(r'All tests passed \(\d+ assertions in 1 test case\)', '\n'.join(lines)))==p['ranks']
    for row in native:
        assert row['ranks']==p['ranks'] and row['workers']==p['workers']
        closes(row,('support','reference','import','source','weights','partition','donor_migration','communicator','refine','coarsen','wait','return'))
        transport_counters(row)
    for row in details:nested_stage(row)
    for row in stages:stage(row)
    control=work_reference or reference
    old=[parse(s) for s in (control/'case.stdout').read_text().splitlines() if s.startswith('NATIVE3D_DETAIL ') and any('stage='+k+' ' in s for k in ('refine','coarsen'))]
    current=[r for r in details if r['stage'] in ('refine','coarsen')]
    keys=('rank','ranks','workers','complete','rounds','attempted','accepted','rejected','conflicts',
          'edge_evaluations','edge_reused','final_owned_cells','cached_measured_cells','cached_invalid_cells',
          'remaining_edges','remaining_repairs','moved','reconnected','inserted','donor_batches','private_query_retries','donor_payloads')
    assert len(old)==len(current)
    for a,b in zip(old,current):
        assert a['stage']==b['stage']
        assert all(a[k]==b[k] for k in keys),'Physical/query work changed at '+b['stage']
    result=copy.deepcopy(certificate)
    result.update(status='PASS',campaign_status='FAIL' if partial else 'PASS',parameters=p,frames=frames,states=states,
                  reload_metrics=[] if partial else certificate['reload_metrics'],rank_rows=rank_rows,rank_stages=stages,native_rows=native,
                  audit_mode='exact artifact certificate reuse plus fresh current output/runtime checks',cycles_audited=list(cycles),
                  reference_certificate_sha256=sha(reference/'independent_audit.json'),reference_validation_sha256=sha(reference/'validation.json'),
                  matched_artifacts_sha256=matched,actual_outputs=outputs,matched_physical_work_fields=list(keys),
                  work_reference_log_sha256=sha(control/'case.stdout'),current_log_sha256=sha(folder/'case.stdout'),
                  tools_sha256={n:sha(where/n) for n in prior['tools_sha256']},script_sha256=sha(Path(__file__).resolve()),
                  scope='Prior complete exact geometry/field/current+BDF certification reused only for byte-identical actual current frames and states. Current actual SU2/CGNS files and all runtime phases/counters are independently checked; physical/query work matches the declared control. No new full geometry audit, source-target relaxation or speed/scaling/fullGoal1 claim. '+('First event only; full campaign remains FAIL.' if partial else 'All three events match the fully certified reference.'))
    output=folder/('first_cycle_equivalence_audit.json' if partial else 'independent_audit.json')
    output.write_text(json.dumps(result,indent=2)+'\n')
    print('PASS',len(frames),'exact frames,',len(states),'state frames, fresh outputs and matched physical work; campaign',result['campaign_status'])

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('folder',type=Path);parser.add_argument('reference',type=Path)
    parser.add_argument('--partial',action='store_true');parser.add_argument('--work-reference',type=Path)
    a=parser.parse_args();main(a.folder.resolve(),a.reference.resolve(),a.partial,a.work_reference.resolve() if a.work_reference else None)
