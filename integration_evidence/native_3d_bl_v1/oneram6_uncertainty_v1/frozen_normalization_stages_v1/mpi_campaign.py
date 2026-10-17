"""Continue the connected campaign without repeating its sealed serial geometry audit."""
import json
import campaign as c
folder=c.HERE/'attempt2_pilot_runtime_np1'
manifest=c.HERE/'validated_pilot_manifest.json'
files={p.name:c.run.sha(p) for p in folder.iterdir() if p.is_file()}
proof={'files':files,'independent_result':c.run.sha(folder/'independent_normalization.json'),
       'pilot_summary':c.run.sha(c.HERE/'pilot_summary.json'),'wrapper':c.run.sha(c.Path(__file__))}
if manifest.exists():assert json.loads(manifest.read_text())==proof,'Sealed serial evidence changed'
else:
 assert json.loads((folder/'result.json').read_text())['exit_code']==0
 assert json.loads((folder/'independent_normalization.json').read_text())==json.loads((c.HERE/'pilot_summary.json').read_text())['runtime_np1']
 c.save(manifest,proof)
original=c.runtime
def reuse_or_audit(path):
 if path!=folder:return original(path)
 assert {p.name:c.run.sha(p) for p in folder.iterdir() if p.is_file()}==proof['files']
 print('REUSE sealed independent serial audit',path,flush=True)
 return json.loads((folder/'independent_normalization.json').read_text())
c.runtime=reuse_or_audit
c.main()
result=json.loads((c.HERE/'summary.json').read_text());result['serial_audit_reuse_manifest_sha256']=c.run.sha(manifest)
result['MPI_wrapper_sha256']=c.run.sha(c.Path(__file__));c.save(c.HERE/'summary.json',result)
