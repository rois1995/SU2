"""Verify the transported stage input and every existing target's geometry authority before integration."""
import json,hashlib,struct,importlib.util
from pathlib import Path
R=Path(__file__).resolve().parent
sha=lambda p:hashlib.file_digest(Path(p).open('rb'),'sha256').hexdigest()
a=json.loads((R/'saved_connected_trajectory481_assessment.json').read_text());case=Path(a['case'])
owned=case/'trajectory_working_owned.bin';parameters=case/'trajectory_working_parameters.bin'
assert sha(owned)==a['working_owned_sha256'] and sha(parameters)==a['working_parameters_sha256']
base=json.loads((R/'surface_comparison478_inputs.json').read_text());reference=Path(base['prepared_reference']['path']);assert sha(reference)==base['prepared_reference']['sha256']
producer=Path(base['target']['path']).parent;targets=[]
for rank in range(6):
    p=producer/f'target_rank{rank}.bin'
    with p.open('rb') as f:
        h=f.read(24);magic,version,count=struct.unpack('<QQQ',h);assert version==2
        f.seek(-8-count,2);block=f.read(count);assert len(block)==count
    assert hashlib.sha256(block).hexdigest()==sha(reference)
    targets.append(dict(path=str(p),sha256=sha(p),reference_bytes=count,same_reference=True))
spec=importlib.util.spec_from_file_location('native_inspection',R/'inspect_native_failure3d.py');reader=importlib.util.module_from_spec(spec);spec.loader.exec_module(reader)
nodes={};cells=0
for row in reader._read_owned(owned,a['working_cells'],2_000_000_000):
    assert row['sensor_present'];cells+=1
    for k,n in enumerate(row['nodes']):
        value=(tuple(n['xyz']),tuple(row['sensor'][6*k:6*k+6]));assert nodes.setdefault(n['id'],value)==value
receipt=dict(owned=dict(path=str(owned),sha256=sha(owned)),parameters=dict(path=str(parameters),sha256=sha(parameters)),target=base['target'],prepared_reference=base['prepared_reference'],all_rank_targets=targets,cells=cells,nodes=len(nodes),shared_nodal_coordinates_and_tensors_consistent=True,profile='SAME saved441composed policy and no-CAD authority; current transported tensors, no originalP1 rebootstrap.',stage_height=6.4e-5,maximum_surface_seconds=388.8,maximum_surface_work=233220,source_assessment=str(R/'saved_connected_trajectory481_assessment.json'),source_assessment_sha256=sha(R/'saved_connected_trajectory481_assessment.json'),partition='Serial unchanged; MPI6 existing diagnostic id%5 layout leaves rank5empty at entry. Production automatic ownership unchanged.',purpose='Exercise existing production PrepareSurface sequence on preserved progress; diagnostic execution is not surface readiness or final acceptance.')
(R/'saved_production_surface482_inputs.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(dict(cells=cells,nodes=len(nodes),targets_verified=len(targets),reference_bytes=targets[0]['reference_bytes'])))
