"""Seal demonstrated elapsed-budget overrun; stop only unchanged verified owned MPI once."""
from pathlib import Path
import hashlib,json,os,signal,time
R=Path(__file__).resolve().parent;A=R/'m6_runs/euler_build267_coupled_geometry_repair_mach_only_real_cfd50k'
sha=lambda p:hashlib.file_digest(Path(p).open('rb'),'sha256').hexdigest()
def save(p,d):p.write_text(json.dumps(d,indent=2)+'\n')
assert not (A/'owned_stop_once.json').exists() and not (A/'capture_ready.json').exists()
assert not list(A.glob('adapted_mesh_inspection_rank*_manifest.json'))
now=time.time();prefixes={};rows=[]
for rank in range(6):
 p=A/f'rank_output/1/rank.{rank}/stdout';raw=p.read_bytes();assert raw.endswith(b'\n')
 lines=[s for s in raw.decode().splitlines() if 'NATIVE3D_STAGE rank=' in s];d=dict(v.split('=',1) for v in lines[-1].split() if '=' in v)
 assert d['stage']=='quality_entry' and d['rounds']=='25011'
 estimate=float(d['elapsed'])+now-p.stat().st_mtime
 assert estimate>5700,(rank,estimate)
 rows.append({'rank':rank,'last_stage':d,'file_mtime':p.stat().st_mtime,'estimated_elapsed_lower_bound':estimate})
 prefixes[str(p)]={'bytes':len(raw),'sha256':hashlib.sha256(raw).hexdigest()}
log=(A/'run.log').read_bytes();assert b'Native3D rejected event:' not in log and b'Native3D accepted event:' not in log
pins=json.loads((A/'pins.json').read_text());changed=[p for p,h in pins.items() if sha(p)!=h];assert not changed,changed
E=A/'deadline_overrun_evidence';assert not E.exists();E.mkdir()
for rank in range(6):
 p=A/f'rank_output/1/rank.{rank}/stdout';raw=p.read_bytes();assert hashlib.sha256(raw).hexdigest()==prefixes[str(p)]['sha256']
 (E/f'rank{rank}_stdout.log').write_bytes(raw)
(E/'run.log').write_bytes(log)
resource=(A/'resources.jsonl').read_bytes();resource=resource[:resource.rfind(b'\n')+1];(E/'resources.jsonl').write_bytes(resource)
files={str(p):{'bytes':p.stat().st_size,'sha256':sha(p)} for p in E.iterdir()}
startup=json.loads((A/'owned_startup.json').read_text());target=next(x for x in startup['owned_tree'] if x['pid']==3159077);assert target['start_ticks']==17805236
proc=Path('/proc')/str(target['pid']);f=(proc/'stat').read_text().rsplit(')',1)[1].split();assert int(f[19])==target['start_ticks']
assert (proc/'cwd').resolve()==A.resolve() and Path((proc/'exe').resolve()).name=='orterun'
record={'time':time.time(),'attempts':1,'signal_sent':False,'target':target,'authorized_condition':'Demonstrated non-yielding computational/coordination interval exceeds configured native elapsed budget; no claim of exact stack or unchanged mesh.','configured_maximum_seconds':5400,'last_census_rows':rows,'all_pins_verified_before_stop':True,'pins_checked':len(pins),'pins_manifest_sha256':sha(A/'pins.json'),'sealed_files':files,'last_complete_native_totals':{'cells':734908,'q_bad':6,'J_bad':5,'geometry_bad':2,'size_bad':0},'current_state_unknown':True,'native_event_refusal_observed':False,'current_mesh_captured':False,'stop_helper_sha256':sha(__file__)}
save(A/'deadline_overrun_sealed.json',record)
assert all(sha(p)==v['sha256'] and Path(p).stat().st_size==v['bytes'] for p,v in files.items())
assert (A/'run.log').read_bytes()==log,'Output advanced; reconsider stop without signalling.'
for p,v in prefixes.items():assert Path(p).stat().st_size==v['bytes'] and sha(p)==v['sha256'],'Census/output advanced; reconsider stop.'
fd=os.pidfd_open(target['pid'])
try:
 assert int((proc/'stat').read_text().rsplit(')',1)[1].split()[19])==target['start_ticks']
 assert (A/'run.log').read_bytes()==log
 with (A/'owned_stop_once.json').open('x') as out:out.write(json.dumps(record,indent=2)+'\n')
 try:signal.pidfd_send_signal(fd,signal.SIGTERM);record.update(signal_sent=True,state='one_pidfd_SIGTERM_sent_for_preserved_overrun')
 except ProcessLookupError:record['state']='target_exited_before_signal'
finally:os.close(fd)
save(A/'owned_stop_once.json',record)
print(json.dumps({'signal_sent':record['signal_sent'],'state':record['state'],'pins_checked':len(pins),'minimum_elapsed_lower_bound':min(x['estimated_elapsed_lower_bound'] for x in rows),'current_mesh_captured':False,'sealed_files':len(files)}))
