"""Reuse valid482progress and exact composed441target with existing coherent-capture loader."""
import hashlib,json
from pathlib import Path
from inspect_native_failure3d import _read_owned
from assess_surface_transaction470 import sha
R=Path(__file__).resolve().parent
assessment=R/'saved_production_surface482_serial_assessment.json'
a=json.loads(assessment.read_text());assert not a['final_mesh_accepted'] and a['shared_coordinates_and_transported_tensors_consistent']
source=a['shards'][0];result=source['result'];assert result['positive_cells']==result['sensor_cells']==a['output_cells']
case=Path(a['case']);assert json.loads((case/'terminal_handles_actual.json').read_text())['all_owned_handles_terminal']
files=source['files'];paths={}
for key in ('owned','parameters'):
    item=files[key];path=Path(item['path']);assert sha(path)==item['sha256'] and path.stat().st_size==item['bytes'];paths[key]=path
nodes={};cells=0
for cell in _read_owned(paths['owned'],a['output_cells'],2_000_000_000):
    assert cell['sensor_present'];cells+=1
    for i,node in enumerate(cell['nodes']):
        value=(tuple(node['xyz']),tuple(cell['sensor'][i*6:i*6+6]));assert nodes.setdefault(node['id'],value)==value
assert cells==303606 and len(nodes)==61464
receipt=json.loads((R/'saved_production_surface482_inputs.json').read_text())
for item in [receipt['target'],receipt['prepared_reference'],*receipt['all_rank_targets']]:assert sha(item['path'])==item['sha256']
adapter=R/'surface483_current_state';adapter.mkdir()
for key in ('owned','parameters'):
    link=adapter/f'trajectory_working_{key}.bin';link.symlink_to(paths[key].resolve())
    receipt[key]=dict(path=str(link.resolve()),adapter_path=str(link.absolute()),sha256=sha(link))
(adapter/'terminal_handles_actual.json').symlink_to((case/'terminal_handles_actual.json').resolve())
receipt.update(cells=cells,nodes=len(nodes),source_assessment=str(assessment.resolve()),source_assessment_sha256=sha(assessment),
    capture_adapter=str(adapter.resolve()),purpose='Continue preserved482valid construction progress through complete entry wall census; not a matching-input speed baseline.',
    provisional=True,exact_scheduling_resume=False,final_mesh_accepted=False)
(R/'saved_production_surface483_inputs.json').write_text(json.dumps(receipt,indent=2)+'\n')
print(json.dumps(dict(cells=cells,nodes=len(nodes),adapter=str(adapter),stage_height=receipt['stage_height'])))
