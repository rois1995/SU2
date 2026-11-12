"""Qualify bounded existing-memo diagnostics; do not infer production speedup."""
import hashlib,json,time,struct
from pathlib import Path
R=Path(__file__).resolve().parent
sha=lambda p:hashlib.file_digest(Path(p).open('rb'),'sha256').hexdigest()
def main():
    case=max((R/'cases').glob('curved446_surface_guidance_reuse_serial_*'),key=lambda p:p.stat().st_mtime)
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
    comparisons=[x for x in records if x.get('stage')=='guidance_reuse_comparison']
    assert comparisons,'No constructed candidate: comparison is vacuous.'
    expected={'recorded_geometry_refusal','recorded_visibility_obstruction','recorded_visibility_obstruction_b','unchanged_wall_control','leading_edge'}
    assert {x['context'] for x in comparisons}==expected
    for row in comparisons:
        assert row['scores_identical'] and not row['published']
        candidates=[x for x in records if x.get('context')==row['context'] and x.get('operator')==row['operator'] and 'private_feasible' in x]
        assert len(candidates)==1
        candidate=candidates[0]
        row['candidate_private_feasible']=candidate['private_feasible']
        row['fresh_geometry_excess']=candidate['new_geometry_excess']
        row['old_geometry_excess']=candidate['old_geometry_excess']
        row['production_coarsening_reaches_old_side']=candidate['new_geometry_excess']==0
        row['work_difference']=row['uncached_work']-row['memo_work']
    original=R/'cases/curved441_ramp5_saved_target_surface_mpi6_20261112T132736_1419501'
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
        comparisons=comparisons,actual_reused_faces=sum(x['reused_faces'] for x in comparisons),
        owned_records_byte_identical_to_saved441=matched,owned_export_sha256=sha(saved),script_sha256=sha(__file__),
        limits=['Full-score comparisons, not production early-reject coarsening; zero-new-excess rows alone reach old-side evaluation.',
            'Fresh-first memo replay follows old-first baseline; warm geometry caches and ordering confound wall-time comparisons.',
            'Enclosing candidate work/time includes additional diagnostic calls and cannot be compared directly with443.',
            'Representative private cavities, not surface readiness, generated BL, MPI qualification or final acceptance.'],
        production_changed=False,goal_complete=False)
    (R/'guidance_reuse446_assessment.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(comparisons,indent=2))
if __name__=='__main__':main()
