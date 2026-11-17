from pathlib import Path
import json,hashlib,time
v=Path('integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/N14/section6_12/validation');c=v/'disk_restart_full552_event2'
def read(p):return json.loads(Path(p).read_text())
def sha(p):return hashlib.file_digest(Path(p).open('rb'),'sha256').hexdigest()
s=read(c/'status.json');post=read(c/'post_terminal_handles.json');valid=read(c/'disk_restart_validation.json');assert s['state']=='terminal' and s['exit_code']==0 and s['pins_verified'] and s['resource_gate']['passed'] and post['all_owned_handles_terminal'];assert valid['disk_restart_verified'] and valid['all_inputs_and_pins_unchanged'] and valid['coordinate_maximum_error']==0 and valid['all_output_fields_finite'];rows=[x['identity'] for x in post['owned_identity_checks']]+[post['controller_identity'],read(v/'restart_retry1_primary_launch_identity.json')['controller']];rows=list({(x['pid'],x['start_ticks']):x for x in rows}.values())
for row in rows:
 try:
  f=(Path('/proc')/str(row['pid'])/'stat').read_text().rsplit(')',1)[1].split();alive=int(f[19])==row['start_ticks'] and f[0] not in {'Z','X'}
 except FileNotFoundError:alive=False
 row['alive']=alive;assert not alive,row
pins=read(c/'pins.json');extra=read(v/'restart_retry1_primary_validation_pins.json');assert all(sha(p)==h for m in [pins,extra] for p,h in m.items());assert all(sha(c/name)==h for name,h in valid['artifact_sha256'].items());out=v/'restart_primary_terminal_receipt.json';assert not out.exists();out.write_text(json.dumps(dict(actual_host_proc=True,unix=time.time(),exec_session=1603,exec_exit_code=0,validator_session=8245,validator_exit_code=0,passed=True,all_owned_handles_terminal=True,identities=rows,case=str(c),seconds=s['seconds'],case_pins_verified=len(pins),extra_pins_verified=len(extra),disk_restart_validated=True,authoritative_validation_receipt=str(c/'disk_restart_validation.json'),validation_sha256=sha(c/'disk_restart_validation.json'),resource_gate=s['resource_gate']),indent=2)+'\n');print(json.dumps(dict(passed=True,seconds=s['seconds'],terminal_handles=len(rows),case_pins=len(pins),extra_pins=len(extra))))
