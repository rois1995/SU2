"""Three alternating verified non-MPI checkpoint binary pairs; one process at a time."""
import hashlib,json,os,statistics,subprocess,sys,time
from pathlib import Path
base,candidate,folder=map(lambda x:Path(x).resolve(),sys.argv[1:4])
root=Path(__file__).resolve().parents[2]
print('Case working and retained folder:',folder,flush=True);folder.mkdir(exist_ok=False)
receipts={name:json.loads((path/'validation.json').read_text()) for name,path in [('baseline',base),('candidate',candidate)]}
for name,path in [('baseline',base),('candidate',candidate)]:
 r=receipts[name];assert r['status']=='PASS' and not r['changed_during_run']
 assert hashlib.sha256((path/'serial_engine').read_bytes()).hexdigest()==r['binary_sha256']
control='integration_evidence/native_3d_core_v1/serial_engine.cpp'
assert receipts['baseline']['source_sha256'][control]==receipts['candidate']['source_sha256'][control]
env=dict(os.environ,OMP_NUM_THREADS='1',OPENBLAS_NUM_THREADS='1',MKL_NUM_THREADS='1',PYTHONDONTWRITEBYTECODE='1')
records=[];status='FAIL'
try:
 for trial in range(3):
  for name in (['baseline','candidate'] if trial%2==0 else ['candidate','baseline']):
   path=base if name=='baseline' else candidate
   for cmd,label in [(['uptime'],'load'),(['free','-m'],'memory'),(['ps','-eo','pid,ppid,ni,pcpu,args','--sort=-pcpu'],'processes')]:
    p=subprocess.run(cmd,capture_output=True,text=True)
    (folder/(str(trial)+'_'+name+'_'+label+'.txt')).write_text(p.stdout if label!='processes' else '\n'.join(p.stdout.splitlines()[:12])+'\n')
   command=['nice','-n','19',str(path/'serial_engine')];start=time.monotonic()
   p=subprocess.run(command,cwd=root,env=env,capture_output=True,text=True)
   (folder/(str(trial)+'_'+name+'.stdout')).write_text(p.stdout)
   (folder/(str(trial)+'_'+name+'.stderr')).write_text(p.stderr)
   assert p.returncode==0 and 'PASS actual non-MPI engine' in p.stdout
   profiles=[]
   for line in p.stdout.splitlines():
    if line.startswith(('PROFILE ','COARSEN_PROFILE ','REPAIR_PROFILE ')):
     values=dict(v.split('=',1) for v in line.split()[1:]);values['kind']=line.split()[0];profiles.append(values)
   records.append(dict(trial=trial,checkpoint=name,command=command,seconds=time.monotonic()-start,profiles=profiles))
 for trial in range(3):
  pair=[x for x in records if x['trial']==trial]
  def work(row):return [(p['kind'],p['trial'],p['detail'],p['accepted'],p.get('rejected'),p['field_edges']) for p in row['profiles']]
  assert work(pair[0])==work(pair[1]),'Changed recorded accepted/query work'
 summary=[]
 for kind in ('PROFILE','COARSEN_PROFILE','REPAIR_PROFILE'):
  for name in ('baseline','candidate'):
   for detail in ('0','1'):
    values=[float(p['external_s']) for row in records if row['checkpoint']==name for p in row['profiles'] if p['kind']==kind and p['detail']==detail]
    summary.append(dict(kind=kind,checkpoint=name,detail=int(detail),count=len(values),median=statistics.median(values),minimum=min(values),maximum=max(values)))
 status='PASS'
finally:
 result=dict(status=status,stages=records,summary=summary if status=='PASS' else [],receipts={name:dict(path=str(path),sha256=hashlib.sha256((path/'validation.json').read_bytes()).hexdigest(),binary_sha256=receipts[name]['binary_sha256']) for name,path in [('baseline',base),('candidate',candidate)]},
 scope='Three alternating process pairs; each process includes three matched OFF/ON controls. Same control source, verified source/binary receipts and matching recorded accepted/edge work. Independent preceding MPI matrix confirms complete prior mesh identity. Local contention; no general performance, scaling or CFD affordability claim.')
 result['runner_sha256']=hashlib.sha256(Path(__file__).read_bytes()).hexdigest()
 result['artifact_sha256']={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in folder.iterdir() if p.is_file()}
 (folder/'validation.json').write_text(json.dumps(result,indent=2)+'\n');print(status,folder,flush=True)
