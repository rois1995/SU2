"""Assess bounded actual boundary-planning components; separate invocations are not additive."""
import hashlib,json,time,struct
from pathlib import Path
R=Path(__file__).resolve().parent
sha=lambda p:hashlib.file_digest(Path(p).open('rb'),'sha256').hexdigest()
def main():
    case=max((R/'cases').glob('curved451_boundary_strip_reuse_serial_*'),key=lambda p:p.stat().st_mtime)
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
    expected={'recorded_split_vertex_a','recorded_split_vertex_b','unchanged_wall_control','leading_edge'}
    components={'storage_accounting','mapped_surface_differential','associated_wall_sample','intrinsic_composed_guidance','complete_boundary_guidance','boundary_guidance_before_strip_reuse'}
    assert len(records)==48 and {x['context'] for x in records}==expected
    for context in expected:
        for repeat in (0,1):
            window=[x for x in records if x['context']==context and x['repeat']==repeat]
            assert len(window)==6 and {x['component'] for x in window}==components
            for row in window:
                assert row['calls']>0 and row['curved_sides']>0 and row['samples']>0
                assert row['separate_nested_invocations'] and not row['mesh_accepted']
                assert row['wall_seconds']>=0 and row['cpu_seconds']>=0
    paired=[]
    for context in sorted(expected):
        window=[x for x in records if x['context']==context]
        old=[x for x in window if x['component']=='boundary_guidance_before_strip_reuse']
        new=[x for x in window if x['component']=='complete_boundary_guidance']
        assert len(old)==len(new)==2
        old_wall=sum(x['wall_seconds'] for x in old);new_wall=sum(x['wall_seconds'] for x in new)
        paired.append(dict(context=context,old_wall_seconds=old_wall,new_wall_seconds=new_wall,
            old_cpu_seconds=sum(x['cpu_seconds'] for x in old),new_cpu_seconds=sum(x['cpu_seconds'] for x in new),
            wall_ratio=old_wall/new_wall,strip_pairs=new[0]['strip_pairs'],
            avoided_strip_predicate_calls=sum((x['curved_sides']-x['calls'])*x['strip_pairs'] for x in new),
            geometry_work_identical=[x['geometry_work'] for x in old]==[x['geometry_work'] for x in new]))
    original=R/'cases/curved449_idle_census_saved_target_surface_mpi6_20261112T171923_2017200'
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
        component_measurements=records,paired_comparisons=paired,
        owned_records_byte_identical_to_saved449=matched,owned_export_sha256=sha(saved),script_sha256=sha(__file__),
        limits=['Separate calls into nested functions; do not add component times or subtract them as exclusive production costs.',
            'Reverse node/full-component order on replay; within each node the old implementation runs first. Cache/order and contention limit timing attribution.',
            'Vertex stars of recorded refused split vertices, not reconstructed exact failing edge cavities.',
            'No MPI polling in single-rank diagnostic; serial wall changes still subject to contention.',
            'In-call strip reuse only; no generated BL, surface readiness or final acceptance established.'],
        production_changed=True,goal_complete=False)
    (R/'strip_reuse451_assessment.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(records,indent=2))
if __name__=='__main__':main()
