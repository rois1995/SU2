from pathlib import Path
import json,hashlib,time,sys
p=Path('integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/N4/section6_3_after_N11');v=p/'end_B_preparation';read=lambda f:json.loads(Path(f).read_text());sha=lambda f:hashlib.file_digest(Path(f).open('rb'),'sha256').hexdigest();r=read(v/'g4_case_paths.json');assert r['passed'] and len(r['new_cases'])==3;rows=[];cases=[]
for record in r['new_cases']:
 case=Path(record['case']);status=read(case/'status.json');assert status['state']=='terminal' and status['exit_code']==0 and status['pins_verified'] and not status['changed'];pins=read(case/'pins.json');assert all(sha(f)==h for f,h in pins.items());rows+=record['actual_host_terminal_handles'];cases.append(dict(case=str(case),seconds=status['seconds'],pins_verified=len(pins),status_sha256=sha(case/'status.json')))
rows+=[dict(read(v/'g4_primary_launch_identity.json')['controller'],name='primary-controller')]
for row in rows:
 try:
  f=(Path('/proc')/str(row['pid'])/'stat').read_text().rsplit(')',1)[1].split();alive=int(f[19])==row['start_ticks'] and f[0] not in {'Z','X'}
 except FileNotFoundError:alive=False
 row['alive']=alive;assert not alive,row
out=v/'g4_primary_terminal_receipt.json';assert not out.exists();receipt=dict(actual_host_proc=True,unix=time.time(),exec_session=int(sys.argv[1]),exec_exit_code=0,passed=True,all_owned_handles_terminal=True,identities=rows,cases=cases,case_paths_receipt_sha256=sha(v/'g4_case_paths.json'));out.write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps({k:x for k,x in receipt.items() if k!='identities'}))
