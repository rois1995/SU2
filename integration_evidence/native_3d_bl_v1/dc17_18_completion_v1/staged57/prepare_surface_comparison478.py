"""Reuse476Owned/parameters and441embedded prepared authority; no upstream construction."""
import hashlib,importlib.util,json,struct
from pathlib import Path
R=Path(__file__).resolve().parent
sha=lambda p:hashlib.file_digest(Path(p).open('rb'),'sha256').hexdigest()
def main():
    a=json.loads((R/'current_trajectory476_assessment.json').read_text());case=Path(a['case'])
    owned=case/'trajectory_working_owned.bin';params=case/'trajectory_working_parameters.bin'
    assert sha(owned)==a['working_mesh_sha256'] and sha(params)==a['working_parameters_sha256']
    producer=json.loads((R/'target_checkpoint441_producer.json').read_text());target=Path(producer['producer'])/'target_rank0.bin'
    assert sha(target)==producer['files']['target_rank0.bin']['sha256']
    blob=target.read_bytes();magic,version,n=struct.unpack_from('<QQQ',blob);assert magic==0x4e424c5441524731 and version==2 and 0<n<len(blob)-8
    # Header reference_bytes and trailing checksum are native target-v2 fields; decoder checks authority byte equality.
    reference=R/'prepared_reference441.native_ref';reference.write_bytes(blob[-8-n:-8])
    spec=importlib.util.spec_from_file_location('native_inspect',R/'inspect_native_failure3d.py');mod=importlib.util.module_from_spec(spec);spec.loader.exec_module(mod)
    pairs=[('active_rank4',14983,24439),('active_rank5',67,68),('clean_control',6070,6072)]
    cells={label:[] for label,_,_ in pairs};count=0
    for row in mod._read_owned(owned,304521,2_000_000_000):
        count+=1;assert row['sensor_present'];ids=sorted(v['id'] for v in row['nodes'])
        for label,u,v in pairs:
            if u in ids or v in ids:cells[label].append(ids)
    assert count==304521
    lines=['3']
    for label,u,v in pairs:
        assert 0<len(cells[label])<=64;lines.append(f'{label} {u} {v} {len(cells[label])}');lines+=[' '.join(map(str,ids)) for ids in sorted(cells[label])]
    contexts=R/'surface_comparison478_contexts.dat';contexts.write_text('\n'.join(lines)+'\n')
    receipt=dict(scope='Current476coherent state and prepared reference extracted from SAME441target; no sensor/normalization/geometry re-preparation.',owned=dict(path=str(owned),sha256=sha(owned),rows=count),parameters=dict(path=str(params),sha256=sha(params)),target=dict(path=str(target),sha256=sha(target)),prepared_reference=dict(path=str(reference),sha256=sha(reference),bytes=n,offset=len(blob)-8-n,decoder_must_require_exact_target_reference_agreement=True),contexts=dict(path=str(contexts),sha256=sha(contexts),pairs=[dict(label=label,vertices=[u,v],union_cells=len(cells[label]),slots=8) for label,u,v in pairs]),limits=dict(total_slots=24,geometry_seconds=300,outer_seconds=600,old_cells=64,new_cells=128,bytes=12<<30))
    (R/'surface_comparison478_inputs.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt['contexts'],indent=2))
if __name__=='__main__':main()
