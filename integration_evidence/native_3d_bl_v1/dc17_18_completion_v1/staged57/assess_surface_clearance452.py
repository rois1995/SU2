"""Assess bounded actual surface-clearance diagnostics without treating witnesses as certificates."""
import hashlib,json,time,struct
from pathlib import Path
R=Path(__file__).resolve().parent
sha=lambda p:hashlib.file_digest(Path(p).open('rb'),'sha256').hexdigest()
def main():
    case=max((R/'cases').glob('curved452_surface_clearance_serial_*'),key=lambda p:p.stat().st_mtime)
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
    expected={'recorded_edge_r5','r4_adjacent_candidate_0','r4_adjacent_candidate_1','smooth_control','leading_edge_control'}
    assert {x['context'] for x in records}==expected
    operators={'SplitEdge','InsertEdgeCavity','RefitWallSplit','ReconstructWallSplit','ReconstructSplitPatch'}
    for context in expected:
        tests=[x for x in records if x['context']==context and x['stage'] in ('operator','support_refused')]
        assert len(tests)==5 and {x['operator'] for x in tests}==operators
        initial=[x for x in records if x['context']==context and x['stage']=='height' and x['operator']=='input']
        assert len(initial)==1
    assert all(x['edge']==[7907,7908] for x in records if x['context']=='recorded_edge_r5' and 'edge' in x)
    refinements=[x for x in records if x['stage']=='clearance_refinement']
    assert all(not x['candidate_is_sign_certificate'] for x in refinements)
    summary=[]
    for context in sorted(expected):
        for operation in sorted(operators|{'input'}):
            samples=[x for x in refinements if x['context']==context and x['operator']==operation]
            if samples:summary.append(dict(context=context,operator=operation,refinements=samples,
                positive_clearance_proved_by_tightening=any(x['clearance_lower_over_h']>0 for x in samples),
                candidate_positive_only=any(x['signed_candidate_over_h']>0 and x['clearance_lower_over_h']<=0 for x in samples)))
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
        records=records,clearance_summary=summary,
        owned_records_byte_identical_to_saved451=matched,owned_export_sha256=sha(saved),script_sha256=sha(__file__),
        limits=['Reproduced recorded R5 edge on terminal451state, not an unavailable exact private trial payload.',
            'R4log retains only first endpoint23829: its two actual adjacent edges are candidates, not identity-qualified reproductions.',
            'Signed returned-candidate values are local witnesses, not sign certificates or proof of body penetration.',
            'SurfacePlanning metrics and fitting guidance are not final composed-edge or geometry acceptance.',
            'No q/J, transfer, generated BL or MPI production qualification is claimed by this serial diagnostic.'],
        production_changed=False,goal_complete=False)
    (R/'surface_clearance452_assessment.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(summary,indent=2))
if __name__=='__main__':main()
