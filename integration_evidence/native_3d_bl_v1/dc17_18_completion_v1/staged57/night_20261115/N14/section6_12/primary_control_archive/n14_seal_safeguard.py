from pathlib import Path
import json,hashlib,time,sys
v=Path('integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/N14/section6_12/validation');tag=sys.argv[1];case=Path(sys.argv[2]).resolve(strict=True);session=int(sys.argv[3])
def read(p):return json.loads(Path(p).read_text())
s=read(case/'status.json');assert s['state']=='terminal' and s['exit_code']==0 and s['pins_verified'] and not s['changed'];r=read(case/'safeguard_process_receipt.json');assert r['state']=='terminal' and r['exit_code']==0 and not r['live_owned_descendants'];rows=r['identities']+[dict(read(v/f'{tag}_primary_launch_identity.json')['controller'],name='controller')]
f=case/'terminal_handles.json'
if f.exists():
 handles=read(f);assert handles['all_owned_handles_terminal'];rows+=handles.get('sampled_identities',handles.get('identities',[]))
unique={(row['pid'],row['start_ticks']):row for row in rows};rows=list(unique.values())
for row in rows:
 try:
  f=(Path('/proc')/str(row['pid'])/'stat').read_text().rsplit(')',1)[1].split();alive=int(f[19])==row['start_ticks'] and f[0] not in {'Z','X'}
 except FileNotFoundError:alive=False
 row['alive']=alive;assert not alive,row
pins=read(case/'pins.json');extra=read(v/f'{tag}_primary_validation_pins.json');assert all(hashlib.sha256(Path(p).read_bytes()).hexdigest()==h for d in [pins,extra] for p,h in d.items());resources=[json.loads(line) for line in (case/'resources.jsonl').read_text().splitlines()];peak=max(row['sampled_process_tree_rss_bytes'] for row in resources);assert peak<=19999490048
out=v/f'{tag}_primary_terminal_receipt.json';assert not out.exists();out.write_text(json.dumps(dict(actual_host_proc=True,unix=time.time(),exec_session=session,exec_exit_code=0,identities=rows,all_owned_handles_terminal=True,case=str(case),seconds=s['seconds'],case_pins_verified=len(pins),extra_pins_verified=len(extra),observed_case_tree_rss_peak_bytes=peak,not_full_P8_peak=True,passed=True),indent=2)+'\n');print(json.dumps(dict(tag=tag,seconds=s['seconds'],terminal_handles=len(rows),case_pins=len(pins),observed_tree_peak=peak)))
