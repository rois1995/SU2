"""Assess bounded complete-support compound surface refinements on saved M6 cavities."""
import hashlib,json,time,struct
from pathlib import Path
R=Path(__file__).resolve().parent
sha=lambda p:hashlib.file_digest(Path(p).open('rb'),'sha256').hexdigest()
def main():
    case=max((R/'cases').glob('curved466_surface_patch_serial_*'),key=lambda p:p.stat().st_mtime)
    status=json.loads((case/'status.json').read_text())
    assert status['state']=='terminal' and status['exit_code']==0 and status['pins_verified']
    pins=json.loads((case/'pins.json').read_text());assert all(sha(p)==h for p,h in pins.items())
    receipt=json.loads((case/'terminal_handles.json').read_text())
    identities=receipt['sampled_identities']+[receipt['wrapper_excluded']]
    checked=[]
    for row in identities:
        try:
            v=Path('/proc',str(row['pid']),'stat').read_text().rsplit(')',1)[1].split()
            terminal=int(v[19])!=row['start_ticks'] or v[0]=='Z';state=v[0]
        except FileNotFoundError:terminal=True;state='absent'
        assert terminal,row
        checked.append(dict(**row,terminal=terminal,state=state))
    terminal=dict(actual_host_proc=True,all_owned_handles_terminal=True,wrapper_verified_terminal=True,identities=checked,checked_unix=time.time())
    (case/'terminal_handles_actual.json').write_text(json.dumps(terminal,indent=2)+'\n')
    records=[json.loads(x) for x in (case/'actual_bl_cavity_planning.jsonl').read_text().splitlines()]
    rounds=[x for x in records if x['stage']=='split_continuation']
    assert len(rounds)==1 and rounds[0]['private_published'] and rounds[0]['new_cells']==19
    inputs=[x for x in records if x['stage']=='trajectory_input'];assert len(inputs)==1 and inputs[0]['native_identity_topology_pass'] and inputs[0]['complete_reservation']
    states=[x for x in records if x['stage']=='trajectory_state']
    operations=[x for x in records if x['stage']=='trajectory_operation']
    results=[x for x in records if x['stage']=='trajectory_result']
    assert len(results)==1 and states and states[0]['cells']==36
    result=results[0];assert result['old_cells']==36 and result['new_cells']<=128
    assert result['attempts']==len(operations)<=24
    assert result['cuts']==sum(x['admitted'] for x in operations)
    assert not result['published'] and not result['mesh_accepted'] and not result['surface_geometry_certified']
    assert result['mesh_to_model_guidance_closed']==(result['geometry_bad']==0)
    for x in operations:
        if x['admitted']:assert all(x[k] for k in ('constructed','metric','parameters','contact_protocol','contact','preserved_heights','provisional_height'))
    assert [x['pass'] for x in states]==list(range(len(states)))
    assert states[-1]['geometry_bad']==result['geometry_bad']
    initial_faces=[x for x in records if x['stage']=='residual_face']
    admitted_edges={tuple(sorted((f['vertices'][k][0],f['vertices'][(k+1)%3][0]))) for f in initial_faces for k in range(3)}
    first_new=max(v[0] for f in initial_faces for v in f['vertices'])+1
    attribution=[]
    for state in states:
        faces=[x for x in records if x['stage']=='trajectory_face' and x['pass']==state['pass']]
        bad=[x for x in faces if x['bound']>1e-4]
        unsupported=set()
        for f in bad:
            for k in range(3):
                edge=tuple(sorted((f['vertices'][k],f['vertices'][(k+1)%3])))
                if edge not in admitted_edges and max(edge)<first_new:unsupported.add(edge)
        attribution.append(dict(pass_index=state['pass'],faces=len(faces),bound_bad_faces=len(bad),maximum_face_bound=max(x['bound'] for x in faces),
            bad_face_edges_without_original_star_admission=sorted(unsupported)))
    original=R/'cases/curved451_strip_reuse_saved_target_surface_mpi6_20261112T185700_120424'
    # Existing serial fixture asserts all owned records unchanged and exports them for independent identity checking.
    saved=case/'correction_unchanged_owned.bin'
    assert saved.exists()
    def rows(path):
        with path.open('rb') as stream:
            count=struct.unpack('<Q',stream.read(8))[0]
            for _ in range(count):
                row=stream.read(416);assert len(row)==416
                yield struct.unpack_from('<Q',row,8)[0],hashlib.sha256(row).digest()
            assert not stream.read(1)
    expected_rows={}
    for rank in range(6):
        for identity,digest in rows(original/f'stage_surface_rank{rank}_owned.bin'):
            assert identity not in expected_rows;expected_rows[identity]=digest
    matched=0
    for identity,digest in rows(saved):
        assert expected_rows.pop(identity)==digest;matched+=1
    assert not expected_rows
    report=dict(case=str(case),status=status,all_source_runtime_input_pins_verified=len(pins),all_actual_handles_terminal=True,
        states=states,operations=operations,result=result,records=records,attribution=attribution,
        tangential_suitability_established=False,usable_stage_checkpoint=False,
        owned_records_byte_identical_to_saved451=matched,owned_export_sha256=sha(saved),script_sha256=sha(__file__),
        limits=['Serial private trajectory only; no production/MPI qualification.',
            'Complete face guidance is not final bidirectional geometry certification.',
            'Q/J observed with current transported nodal field during validity-first construction.',
            'No accepted BL/RANS workflow completion; first-row height bits alone do not prove alignment/coverage or transition quality.',
            'geometry_prerequisite_closed is not tangential qualification; target_first_height_coverage is not usable BL readiness.'],
        production_changed=False,goal_complete=False)
    (R/'surface_patch466_assessment.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(dict(states=len(states),operations=len(operations),result=result),indent=2))
if __name__=='__main__':main()
