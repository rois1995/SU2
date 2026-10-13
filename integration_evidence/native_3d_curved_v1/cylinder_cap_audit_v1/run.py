from pathlib import Path
import os,time,subprocess,json,hashlib,sys
HERE=Path(__file__).resolve().parent
assert not (HERE/'validation.json').exists()
def ticks():return {l.split()[0]:list(map(int,l.split()[1:])) for l in Path('/proc/stat').read_text().splitlines() if l.startswith('cpu')}
a=ticks();time.sleep(1);b=ticks();idle={}
for c in os.sched_getaffinity(0):
 d=[y-x for x,y in zip(a['cpu'+str(c)],b['cpu'+str(c)])];idle[c]=(d[3]+d[4])/sum(d) if sum(d) else 0
cpu=max(idle,key=idle.get);env=dict(os.environ,PYTHONDONTWRITEBYTECODE='1',OMP_NUM_THREADS='1',OPENBLAS_NUM_THREADS='1')
rows=[];status='PASS'
for name in ('audit.py','audit_primitives.py'):
 print('CASE_FOLDER='+str(HERE)+' '+name,flush=True)
 with (HERE/(name+'.stdout')).open('w') as out,(HERE/(name+'.stderr')).open('w') as err:
  r=subprocess.run(['taskset','-c',str(cpu),'nice','-n','19','timeout','900s',sys.executable,str(HERE/name),'cylinder_cap_coverage_v2'],stdout=out,stderr=err,env=env)
 rows.append({'script':name,'exit_code':r.returncode})
 if r.returncode:status='FAIL';break
receipt={'status':status,'cpu':cpu,'stages':rows,'scope':'Independent fresh-collar plus accepted primitive geometry/field/actual conserved current+BDF and restart/output audits; not full Goal2 or efficiency. One owned job/CPU/nice19/threads1.','artifact_sha256':{p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in HERE.iterdir() if p.is_file() and p.name!='validation.json'}}
(HERE/'validation.json').write_text(json.dumps(receipt,indent=2)+'\n');print(status,flush=True)
raise SystemExit(status!='PASS')
