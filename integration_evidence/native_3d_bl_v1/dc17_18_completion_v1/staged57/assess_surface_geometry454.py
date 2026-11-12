"""Assess exact-support geometry bounds and complete original/fresh M6 patches."""
import hashlib,json,time,struct
from pathlib import Path
R=Path(__file__).resolve().parent
sha=lambda p:hashlib.file_digest(Path(p).open('rb'),'sha256').hexdigest()
def main():
    case=max((R/'cases').glob('curved454_surface_geometry_serial_*'),key=lambda p:p.stat().st_mtime)
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
    expected={'recorded_edge_r5','r4_adjacent_candidate','smooth_control','leading_edge_control'}
    assert {x['context'] for x in records}==expected
    paired=[]
    for context in sorted(expected):
        windows=[x for x in records if x['stage']=='complete_patch' and x['context']==context]
        assert len(windows)==2 and {x['side'] for x in windows}=={'old','fresh'}
        old=next(x for x in windows if x['side']=='old');new=next(x for x in windows if x['side']=='fresh')
        assert old['scores'][0]==old['production_excess'] and new['scores'][0]==new['production_excess']
        changed=[x for x in records if x['stage']=='face_bound' and x['context']==context]
        assert changed and all(x['strict_tolerance']==1e-4 and not x['final_certificate'] for x in changed)
        before={tuple(sorted(int(v[0]) for v in x['vertices'])):x for x in records if x['stage']=='cell' and x['context']==context and x['side']=='old'}
        after={tuple(sorted(int(v[0]) for v in x['vertices'])):x for x in records if x['stage']=='cell' and x['context']==context and x['side']=='fresh'}
        retired=before.keys()-after.keys();introduced=after.keys()-before.keys()
        paired.append(dict(context=context,old=old,fresh=new,query_thresholds=[1e-4,2e-4,5e-4,1e-3],
            improves=[b<a*(1-1e-12) for a,b in zip(old['scores'],new['scores'])],
            retired_cells=len(retired),introduced_cells=len(introduced),
            retired_q_bad=sum(before[k]['q']<.2 for k in retired),introduced_q_bad=sum(after[k]['q']<.2 for k in introduced),
            retired_J_bad=sum(before[k]['J']<.05 for k in retired),introduced_J_bad=sum(after[k]['J']<.05 for k in introduced)))
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
        records=records,paired_comparisons=paired,
        owned_records_byte_identical_to_saved451=matched,owned_export_sha256=sha(saved),script_sha256=sha(__file__),
        limits=['Alternate query thresholds are diagnostic only: strict physical geometry tolerance remains1e-4.',
            'Alternative complete score bounds reuse identical strict contributions for unchanged faces; not an exact hypothetical global replacement algorithm.',
            'All tensors/qJ are private centroid observations before nodal publication transport, not final acceptance.',
            'No production integration or MPI qualification; final BL/RANS objectives remain open.'],
        production_changed=False,goal_complete=False)
    (R/'surface_geometry454_assessment.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(paired,indent=2))
if __name__=='__main__':main()
