"""Reuse byte-identical qualified binary fixture with exact archived producer bytes."""
from pathlib import Path
import hashlib,json,subprocess,time
R=Path(__file__).resolve().parent
old=R/'m6_runs/euler_build244_mach_only_real_cfd50k/placement246_inputs_v1'
out=R/'m6_runs/euler_build262_compact_capture_mach_only_real_cfd50k/placement267_reuse_v1'
assert not out.exists();out.mkdir()
sha=lambda p:hashlib.file_digest(Path(p).open('rb'),'sha256').hexdigest()
receipt=old/'witness_preparation.json';prior=json.loads(receipt.read_text())
assert prior['input_hashes_before']==prior['input_hashes_after']
inputs=dict(prior['input_hashes_before']);reader=(R/'inspect_native_failure3d.py').resolve()
expected=inputs.pop(str(reader));repo=R.parents[3]
relative=reader.relative_to(repo)
data=subprocess.check_output(['git','show','b240f86b35:'+str(relative)],cwd=repo)
assert hashlib.sha256(data).hexdigest()==expected
snapshot=out/'inspect_native_failure3d_028b7184ff.py.txt';snapshot.write_bytes(data)
inputs[str(snapshot.resolve())]=expected
inputs[str(receipt.resolve())]=sha(receipt)
inputs[str(Path(__file__).resolve())]=sha(__file__)
for name,digest in inputs.items():assert sha(name)==digest,name
for name,item in prior['outputs'].items():
 p=old/name;assert p.stat().st_size==item['bytes'] and sha(p)==item['sha256'],name
 (out/name).symlink_to(p.resolve())
current=dict(prior);current.update(input_hashes_before=inputs,input_hashes_after=inputs,
 scope='Unchanged qualified binary MPI cavity payloads. Original reader archived exactly; no reexecution or attribution to changed reader. Current runner/source/runtime pinned independently.',
 reuse267={'time':time.time(),'original_receipt':str(receipt.resolve()),'original_receipt_sha256':sha(receipt),
 'archived_reader_sha256':expected,'archived_reader_git_commit':'b240f86b35','all_output_payloads_byte_identical':True,
 'current_reader_sha256':sha(reader),'current_reader_use':'Not used by C++ binary fixture; prior actual reader/capture qualification preserved.'})
(out/'witness_preparation.json').write_text(json.dumps(current,indent=2)+'\n')
print(out)
