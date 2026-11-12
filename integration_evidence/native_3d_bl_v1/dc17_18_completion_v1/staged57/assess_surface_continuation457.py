"""Assess bounded complete-support compound surface refinements on saved M6 cavities."""
import hashlib,json,time,struct
from pathlib import Path
R=Path(__file__).resolve().parent
sha=lambda p:hashlib.file_digest(Path(p).open('rb'),'sha256').hexdigest()
def main():
    case=max((R/'cases').glob('curved457_surface_continuation_serial_*'),key=lambda p:p.stat().st_mtime)
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
    assert rounds and len(rounds)<=4
    assert [x['round'] for x in rounds]==list(range(len(rounds)))
    net=dict(q=0,J=0,geometry=0)
    for row in rounds:
        assert row['old_cells']<=64 and row['new_cells']<=128 and not row['mesh_accepted']
        if row['private_published']:
            assert all(row[k] for k in ('constructed','geometry_progress','metric_admission','parameters','contact_protocol','contact','preserved_heights','provisional_height'))
            for key in net:net[key]+=row['new_'+key+'_bad']-row['old_'+key+'_bad']
        for key in net:assert row['net_'+key+'_bad']==net[key]
    useful=[x for x in rounds if x['private_published']]
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
        records=records,private_published_steps=useful,net_changed_bad_cells=net,
        owned_records_byte_identical_to_saved451=matched,owned_export_sha256=sha(saved),script_sha256=sha(__file__),
        limits=['Serial actual capability comparison, not MPI production qualification.',
            'Private centroid q/J and SurfacePlanning guidance use transported current nodal metrics; not final full-target metric certification.',
            'Q/J observed during validity-first construction; final gates remain unchanged.',
            'No accepted full BL or RANS workflow completion.'],
        production_changed=False,goal_complete=False)
    (R/'surface_continuation457_assessment.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(dict(rounds=rounds,net_changed_bad_cells=net),indent=2))
if __name__=='__main__':main()
