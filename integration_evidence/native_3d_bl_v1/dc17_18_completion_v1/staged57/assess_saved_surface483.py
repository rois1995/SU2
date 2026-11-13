"""Verify current-state surface captures independently; no new geometry evaluation."""
import hashlib, json, struct, sys
from pathlib import Path
from assess_surface_transaction470 import sha
from inspect_native_failure3d import _read_owned
R=Path(__file__).resolve().parent
case=Path(sys.argv[1])
status=json.loads((case/'status.json').read_text())
assert status['state']=='terminal' and status['exit_code']==0 and status['pins_verified']
pins=json.loads((case/'pins.json').read_text())
assert all(sha(p)==h for p,h in pins.items())
handles=json.loads((case/'terminal_handles_actual.json').read_text())
assert handles['actual_host_proc'] and handles['all_owned_handles_terminal']
inputs=json.loads((R/'saved_production_surface483_inputs.json').read_text())
source=Path(inputs['owned']['path']);assert sha(source)==inputs['owned']['sha256']
def rows(path,width):
    with path.open('rb') as f:
        n=struct.unpack('<Q',f.read(8))[0]
        assert path.stat().st_size==8+n*width
        for _ in range(n):
            row=f.read(width);assert len(row)==width;yield row
        assert not f.read(1)
expected={}
for row in rows(source,416):
    identity=struct.unpack_from('<Q',row,8)[0];assert identity not in expected
    expected[identity]=hashlib.sha256(row[8:144]+row[224:416]+b'\xff'*16).digest()
assert len(expected)==inputs['cells']
donor_ids=set();cell_ids=set();nodes={};shards=[]
for manifest_path in sorted(case.glob('stage_resumed_surface_rank*_manifest.json')):
    m=json.loads(manifest_path.read_text());rank=m['rank']
    result=json.loads((case/f'actual_saved_surface_stage_rank{rank}.json').read_text())
    assert m['complete_shard'] and m['private_capture'] and not m['final_mesh_accepted']
    assert result['same_target_policy'] and result['input_parameters_unchanged'] and result['captured']
    assert result['positive_cells']==result['sensor_cells']==m['working_rows']==result['output_cells']
    assert result['stage_height']==inputs['stage_height'] and not result['engine_valid']
    files={k:case/v for k,v in m['files'].items()}
    assert files['parameters'].stat().st_size==m['parameters_payload_bytes']>0
    assert m['owned_record_bytes']==416 and m['donor_record_bytes']==344
    donors=0
    for row in rows(files['donors'],344):
        identity=struct.unpack_from('<Q',row)[0];assert identity not in donor_ids
        assert expected[identity]==hashlib.sha256(row).digest();donor_ids.add(identity);donors+=1
    assert donors==m['donor_rows']==result['donor_cells']
    for cell in _read_owned(files['owned'],m['working_rows'],12884901888):
        assert cell['id'] not in cell_ids and cell['sensor_present'];cell_ids.add(cell['id'])
        for i,node in enumerate(cell['nodes']):
            value=(node['xyz'],cell['sensor'][6*i:6*i+6])
            if node['id'] in nodes:assert nodes[node['id']]==value
            else:nodes[node['id']]=value
    shards.append(dict(result=result,manifest=str(manifest_path),files={k:dict(path=str(p),sha256=sha(p),bytes=p.stat().st_size) for k,p in files.items()}))
assert len(shards) in (1,6) and len(shards)==shards[0]['result']['ranks']
assert donor_ids==set(expected)
assert all(s['result']['global_output_cells']==len(cell_ids) for s in shards)
report=dict(case=str(case),status=status,assessment_script_sha256=sha(__file__),pins_verified=len(pins),actual_handles_terminal=True,
    transported_input_donors_exact=len(donor_ids),output_cells=len(cell_ids),output_nodes=len(nodes),
    shared_coordinates_and_transported_tensors_consistent=True,shards=shards,
    target_reconstruction_inputs=inputs,surface_ready=all(s['result']['surface_prepared'] for s in shards),
    final_mesh_accepted=False,goal_complete=False,
    limitations=['Provisional stage output, not a CFD restart.',
    'Full target/authority/config plus current tensors and associations required; capture alone is incomplete.',
    'Independent payload byte/readability checks do not establish final geometry, q/J, heights or exact scheduling resume.',
    'Complete wall census required before interpreting zero known geometry failures as closure.'])
output=R/('saved_production_surface483_'+('serial' if len(shards)==1 else 'mpi6')+'_assessment.json')
output.write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps({k:report[k] for k in ('pins_verified','transported_input_donors_exact','output_cells','output_nodes','surface_ready')},indent=2))
