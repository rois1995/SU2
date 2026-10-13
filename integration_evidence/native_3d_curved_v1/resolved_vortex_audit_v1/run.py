from pathlib import Path
import os,time,subprocess,hashlib,json,sys
HERE=Path(__file__).resolve().parent;CASE=HERE.parent/'resolved_vortex_v3'
assert json.loads((CASE/'validation.json').read_text())['status']=='PASS','Wait for terminal native MPI qualification'
assert not (HERE/'validation.json').exists()
def ticks():return {l.split()[0]:list(map(int,l.split()[1:])) for l in Path('/proc/stat').read_text().splitlines() if l.startswith('cpu')}
a=ticks();time.sleep(1);b=ticks();idle={}
for c in os.sched_getaffinity(0):
 d=[y-x for x,y in zip(a['cpu'+str(c)],b['cpu'+str(c)])];idle[c]=(d[3]+d[4])/sum(d) if sum(d) else 0
cpu=max(idle,key=idle.get);env=dict(os.environ,PYTHONDONTWRITEBYTECODE='1',OMP_NUM_THREADS='1',OPENBLAS_NUM_THREADS='1');stages=[];status='PASS'
for name,args in [('check_index.py',[]),('audit_physics.py',['resolved_vortex_v3']),('export_flow.py',['resolved_vortex_v3']),('audit_lifecycle.py',['resolved_vortex_v3'])]:
 print('CASE_FOLDER='+str(HERE)+' '+name,flush=True)
 with (HERE/(name+'.stdout')).open('w') as out,(HERE/(name+'.stderr')).open('w') as err:
  r=subprocess.run(['taskset','-c',str(cpu),'nice','-n','19','timeout','1800s',sys.executable,str(HERE/name),*args],stdout=out,stderr=err,env=env)
 stages.append({'script':name,'exit_code':r.returncode})
 if r.returncode:status='FAIL';break
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
receipt={'status':status,'cpu':cpu,'stages':stages,'native_receipt_sha256':sha(CASE/'validation.json'),'artifact_sha256':{str(p.relative_to(HERE)):sha(p) for p in HERE.rglob('*') if p.is_file() and p.name!='validation.json'},'scope':'Independent source-box selfcheck, analytic/evolved resolved-vortex physics, inspectable actual flow exports and exact geometry/P1-or-extension/metric/transport/conservation/current+BDF/restart audits. One job/CPU/nice19/threads1. No convergence/shock/full Goal2 claim.'}
(HERE/'validation.json').write_text(json.dumps(receipt,indent=2)+'\n');print(status,flush=True);raise SystemExit(status!='PASS')
