"""Reuse current237 serializer for two actual LE contexts; no production changes."""
from pathlib import Path
import hashlib,json
HERE=Path(__file__).resolve().parent;R=HERE.parent
source=R/'prepare_current237_witnesses.py';code=source.read_text()
changes=[('out = run / "current_witnesses_v1"','out = run / "leading_edge253_witnesses_v1"'),('    assert any(row["physical_faces"] == 0 for row in chosen)\n',''),('    assert any(row["physical_faces"] > 0 and not row["geometry_bad"] for row in chosen)\n','')]
for old,new in changes:
 assert code.count(old)==1;code=code.replace(old,new)
namespace={'__file__':str(source.resolve()),'__name__':'bounded_le_serializer'};exec(compile(code,str(source),'exec'),namespace)
run=R/'m6_runs/euler_build247_canonical_mach_only_real_cfd50k';selection=HERE/'le_cavity_selection.json'
namespace['prepare'](run,selection)
out=run/'leading_edge253_witnesses_v1';p=out/'witness_preparation.json';receipt=json.loads(p.read_text())
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
for path in [Path(__file__).resolve(),selection.resolve()]:receipt['input_hashes_before'][str(path)]=receipt['input_hashes_after'][str(path)]=sha(path)
receipt['bounded_transform']={'base_serializer':str(source),'source_sha256':sha(source),'transformed_sha256':hashlib.sha256(code.encode()).hexdigest(),'changes':changes,'reason':'Two user-requested actual smooth-LE contexts only; omit unrelated interior/clean control-presence assertions. Original-star controls remain in unchanged compiled fixture; output directory unique.'}
p.write_text(json.dumps(receipt,indent=2)+'\n')
print(out)
