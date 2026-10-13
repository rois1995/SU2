from pathlib import Path
import os,time,subprocess,hashlib,json,sys
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2];CASE=HERE.parent/'resolved_vortex_v3'
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
native=json.loads((CASE/'validation.json').read_text());assert native['status']=='PASS'
assert not (HERE/'validation.json').exists(), 'Preserve terminal evidence; use a new folder after a failed run'
plan=[(n,m,e) for n in (1,2,4) for m in (1,2,4) if m<=n for e in range(2)]
assert len(plan)==12 and len(set(plan))==12

def ticks():return {l.split()[0]:list(map(int,l.split()[1:])) for l in Path('/proc/stat').read_text().splitlines() if l.startswith('cpu')}
a=ticks();time.sleep(1);b=ticks();idle={}
for c in os.sched_getaffinity(0):
 d=[y-x for x,y in zip(a['cpu'+str(c)],b['cpu'+str(c)])];idle[c]=(d[3]+d[4])/sum(d) if sum(d) else 0
cpu=max(idle,key=idle.get);env=dict(os.environ,PYTHONDONTWRITEBYTECODE='1',OMP_NUM_THREADS='1',OPENBLAS_NUM_THREADS='1');stages=[];status='PASS'

def certificate(choice):
 p=HERE/('event_n%d_m%d_s%d.json'%choice)
 if not p.exists():return None
 d=json.loads(p.read_text());assert d['status']=='PASS_RESOLVED_VORTEX_EVENT' and tuple(d['selection'])==choice
 assert d['script_sha256']==sha(HERE/'audit_lifecycle.py')
 for n,h in d['inputs_sha256'].items():assert sha(ROOT/n)==h, 'Completed audit input drift'
 for n,h in d['helper_sha256'].items():assert sha(ROOT/'integration_evidence/native_3d_core_v1'/n)==h
 assert d['inputs_sha256'][str((CASE/'validation.json').relative_to(ROOT))]==sha(CASE/'validation.json')
 return d

jobs=[('source_index_selfcheck', 'check_index.py', [], None)]+[('n%d_m%d_s%d'%c,'audit_lifecycle.py',['resolved_vortex_v3',*map(str,c)],c) for c in plan]
for label,script,args,choice in jobs:
 if choice and certificate(choice):
  stages.append({'label':label,'exit_code':0,'reused_verified_certificate':True});continue
 print('CASE_FOLDER='+str(HERE)+' '+label,flush=True)
 started=time.monotonic()
 with (HERE/(label+'.stdout')).open('w') as out,(HERE/(label+'.stderr')).open('w') as err:
  r=subprocess.run(['taskset','-c',str(cpu),'nice','-n','19','timeout','1800s',sys.executable,str(HERE/script),*args],stdout=out,stderr=err,env=env)
 stages.append({'label':label,'exit_code':r.returncode,'seconds':time.monotonic()-started})
 (HERE/'progress.json').write_text(json.dumps({'status':'IN_PROGRESS' if not r.returncode else 'FAIL','planned_events':plan,'stages':stages},indent=2)+'\n')
 if r.returncode:status='FAIL';break
 if choice:assert certificate(choice)
if status=='PASS':
 results={};inputs={}
 for n,m,e in plan:
  d=certificate((n,m,e));assert d
  rows=results.setdefault(str(n),{}).setdefault('resolved_vortex_m'+str(m),[]);assert len(rows)==e;rows.append(d['result'])
  for p,h in d['inputs_sha256'].items():
   assert p not in inputs or inputs[p]==h;inputs[p]=h
 assert sum(len(events) for rows in results.values() for events in rows.values())==12
 (HERE/'independent_lifecycle_validation.json').write_text(json.dumps({'status':'PASS_RESOLVED_VORTEX_LIFECYCLES','results':results,'inputs_sha256':inputs,'script_sha256':sha(HERE/'audit_lifecycle.py'),'scope':'All six N/M variants and twelve full events, identical exact audit predicates to v1; per-event persisted certificates. No partial-event acceptance.'},indent=2)+'\n')
receipt={'status':status,'cpu':cpu,'planned_events':plan,'stages':stages,'native_receipt_sha256':sha(CASE/'validation.json'),'artifact_sha256':{str(p.relative_to(HERE)):sha(p) for p in HERE.rglob('*') if p.is_file() and p.name!='validation.json'},'scope':'Sequential per-event independent whole-lifecycle audit on unchanged terminal native evidence. Completed certificates are reused only after checking every input/helper/script hash. Same 1800s bound per event; one CPU/nice19/threads1; no solver rerun or relaxed gate.'}
(HERE/'validation.json').write_text(json.dumps(receipt,indent=2)+'\n');print(status,flush=True);raise SystemExit(status!='PASS')
