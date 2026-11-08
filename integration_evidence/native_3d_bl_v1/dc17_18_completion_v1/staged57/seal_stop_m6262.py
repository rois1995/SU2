"""Seal actual event-one refusal, then signal only its verified owned MPI once."""
from pathlib import Path
import hashlib,json,os,signal,time
R=Path(__file__).resolve().parent
A=R/'m6_runs/euler_build262_compact_capture_mach_only_real_cfd50k'
sha=lambda p:hashlib.file_digest(Path(p).open('rb'),'sha256').hexdigest()
def save(p,v):p.write_text(json.dumps(v,indent=2)+'\n')
assert not (A/'owned_stop_once.json').exists() and not (A/'capture_ready.json').exists()
log=(A/'run.log').read_bytes();needle=b'Native3D rejected event:';at=log.find(needle);assert at>=0
end=log.find(b'\n',at);assert end>at;message=log[at:end].decode();assert 'elapsed-time budget' in message
paths=sorted(A.glob('adapted_mesh_inspection_rank*_manifest.json'));assert len(paths)==6
manifests=[json.loads(p.read_text()) for p in paths];assert {m['rank'] for m in manifests}==set(range(6))
files={}
for p,m in zip(paths,manifests):
 assert m['schema']=='native_failure3d_v2' and m['metrics_encoding']=='compact-array-v1' and m['event']==1 and m['ranks']==6
 assert m['cap_bytes']==134217728 and m['complete_shard'] and m['owned_complete'] and m['written_rows']==m['working_rows']
 assert not m['final_mesh_accepted'] and p.stat().st_size<=1024**2
 for role,q in [('manifest',p)]+[(k,A/v) for k,v in m['files'].items()]:
  assert q.resolve().parent==A.resolve() and q.is_file();size=q.stat().st_size
  if role!='manifest':assert size<=m['cap_bytes']
  if role=='owned':assert size==8+416*m['written_rows']
  files[str(q)]={'sha256':sha(q),'bytes':size,'role':role,'rank':m['rank']}
assert len(files)==30 and len({m['round'] for m in manifests})==1
pins=json.loads((A/'pins.json').read_text());changed=[p for p,h in pins.items() if sha(p)!=h];assert not changed
assert all(sha(p)==v['sha256'] and Path(p).stat().st_size==v['bytes'] for p,v in files.items())
shape=all(m['shape_complete'] for m in manifests)
totals={'cells':sum(m['working_rows'] for m in manifests),'q_bad':sum(m['progress']['q_bad'] for m in manifests) if shape else None,'j_bad':sum(m['progress']['j_bad'] for m in manifests) if shape else None,'q_bad_known':sum(m['progress']['q_bad'] for m in manifests),'j_bad_known':sum(m['progress']['j_bad'] for m in manifests),'geometry_bad':sum(m['progress']['geometry_bad'] for m in manifests),'size_bad':sum(m['progress']['size_bad'] for m in manifests)}
ready={'schema':json.loads((R/'m6_runs/euler_build260_paired_geometry_priority_mach_only_real_cfd50k/capture_ready.json').read_text())['schema'],'time':time.time(),'event':1,'scope':'All30actualrawfilesclosed/stable andwithinunchangedcaps;ALLownedrowscaptured. Terminalshapeincomplete isNOTa format/rowcoveragefailure anddoesNOTprove0globalq/J.','files':files,'manifests':[str(p) for p in paths],'owned_rows':sum(m['written_rows'] for m in manifests),'expected_owned_rows':totals['cells'],'metric_census_complete':all(m['metrics_complete'] for m in manifests),'config_sha256':sha(A/'run.cfg'),'pins_checked':len(pins),'all_pins_match':True,'log_prefix':{'bytes':end+1,'sha256':hashlib.sha256(log[:end+1]).hexdigest(),'refusal_message':message},'native_census_complete':all(m['census_complete'] for m in manifests),'native_shape_complete':shape,'native_geometry_complete':all(m['geometry_complete'] for m in manifests),'native_totals':totals,'stop_helper_sha256':sha(__file__)}
save(A/'capture_ready.json',ready)
save(A/'pin_check_before_owned_stop.json',{'time':time.time(),'all_pins_match':True,'pins_checked':len(pins),'changed':changed,'pins_manifest_sha256':sha(A/'pins.json'),'capture_ready_sha256':sha(A/'capture_ready.json')})
startup=json.loads((A/'owned_startup.json').read_text());target=next(v for v in startup['owned_tree'] if v['pid']==2598241);assert target['start_ticks']==16555245
proc=Path('/proc')/str(target['pid']);stat=proc/'stat'
record={'time':time.time(),'attempts':1,'signal_sent':False,'target':target,'capture_ready_sha256':sha(A/'capture_ready.json'),'pins_verified_before_stop':True,'reason':'Explicitfirsteventrefusal preventsTWOaccepted events;all30filessealed andALL1520pinsverifiedbeforestop.','stop_helper_sha256':sha(__file__)}
with (A/'owned_stop_once.json').open('x') as f:f.write(json.dumps(record,indent=2)+'\n')
if not stat.exists():record['state']='target_already_absent_no_signal'
else:
 fields=stat.read_text().rsplit(')',1)[1].split();assert int(fields[19])==target['start_ticks']
 assert (proc/'cwd').resolve()==A.resolve() and Path((proc/'exe').resolve()).name=='orterun'
 fd=os.pidfd_open(target['pid'])
 try:
  assert int(stat.read_text().rsplit(')',1)[1].split()[19])==target['start_ticks']
  signal.pidfd_send_signal(fd,signal.SIGTERM);record.update(signal_sent=True,state='one_pidfd_SIGTERM_sent')
 except ProcessLookupError:record['state']='target_exited_before_signal'
 finally:os.close(fd)
save(A/'owned_stop_once.json',record)
print(json.dumps({'owned_rows':ready['owned_rows'],'expected_owned_rows':ready['expected_owned_rows'],'metric_census_complete':ready['metric_census_complete'],'native_totals':totals,'sealed_files':len(files),'pins_verified':len(pins),'stop_state':record['state'],'signal_sent':record['signal_sent']}))
