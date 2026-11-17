from pathlib import Path
import json,hashlib,time
b=Path('integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/N4/section6_3_after_N11/build556');read=lambda f:json.loads(Path(f).read_text())
def sha(f):
 with Path(f).open('rb') as s:return hashlib.file_digest(s,'sha256').hexdigest()
x=read(b/'build_status.json');assert x['state']=='terminal' and x['exit_code']==0 and x['pins_verified'] and not x['changed'];pins=read(b/'source_pins.json');assert all(sha(f)==h for f,h in pins.items());assert sha(x['binary'])==x['binary_sha256'] and sha(x['test_binary'])==x['test_binary_sha256'];g=read(b/'generated_build_graph_after.json');assert g['build_status_sha256']==sha(b/'build_status.json') and g['source_pins_sha256']==sha(b/'source_pins.json');assert all(sha(f)==h for f,h in g['after'].items());t=read(b/'terminal_handles.json');assert t['all_owned_handles_terminal'];rows=t['identities']+[dict(read(b/'primary_launch_identity.json')['controller'],name='controller')]
for row in rows:
 try:
  f=(Path('/proc')/str(row['pid'])/'stat').read_text().rsplit(')',1)[1].split();alive=int(f[19])==row['start_ticks'] and f[0] not in {'Z','X'}
 except FileNotFoundError:alive=False
 row['alive']=alive;assert not alive,row
out=b/'primary_terminal_receipt.json';assert not out.exists();out.write_text(json.dumps(dict(actual_host_proc=True,unix=time.time(),exec_session=48969,exec_exit_code=0,seconds=x['seconds'],identities=rows,all_owned_handles_terminal=True,source_pins_verified=len(pins),binary_sha256=x['binary_sha256'],test_binary_sha256=x['test_binary_sha256'],generated_graph_verified=True,passed=True),indent=2)+'\n');print(json.dumps(dict(passed=True,seconds=x['seconds'],pins=len(pins),terminal_ids=len(rows),CFD=x['binary_sha256'],test=x['test_binary_sha256'])))
