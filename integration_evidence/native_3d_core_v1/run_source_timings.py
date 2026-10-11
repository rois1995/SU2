"""Three alternating pinned MPI1 source-probe pairs, with identical query/mesh/edit work."""
import hashlib,json,os,statistics,subprocess,sys,time
from pathlib import Path
root=Path(__file__).resolve().parents[2]
base,current,folder=map(lambda x:Path(x).resolve(),sys.argv[1:4])
print('Case working and retained folder:',folder,flush=True);folder.mkdir(exist_ok=False)
receipts={k:json.loads((p/'validation.json').read_text()) for k,p in [('baseline',base),('candidate',current)]}
control='integration_evidence/native_3d_core_v1/large_source_probe.cpp'
assert receipts['baseline']['source_sha256'][control]==receipts['candidate']['source_sha256'][control]
for k,p in [('baseline',base),('candidate',current)]:
 r=receipts[k];assert r['status']=='PASS' and not r['changed_during_run']
 assert hashlib.sha256((p/'large_source_probe').read_bytes()).hexdigest()==r['binary_sha256']
env=dict(os.environ,OMP_NUM_THREADS='1',OPENBLAS_NUM_THREADS='1',MKL_NUM_THREADS='1',PYTHONDONTWRITEBYTECODE='1',OMPI_MCA_mpi_yield_when_idle='1')
records=[];status='FAIL'
try:
 for trial in range(3):
  for name in (['baseline','candidate'] if trial%2==0 else ['candidate','baseline']):
   case=folder/(str(trial)+'_'+name);case.mkdir();mesh=case/'meshes';mesh.mkdir()
   print('Case working and retained folder:',case,flush=True)
   for cmd,label in [(['uptime'],'load'),(['free','-m'],'memory'),(['ps','-eo','pid,ppid,ni,pcpu,args','--sort=-pcpu'],'processes')]:
    p=subprocess.run(cmd,capture_output=True,text=True);(case/(label+'.txt')).write_text(p.stdout if label!='processes' else '\n'.join(p.stdout.splitlines()[:12])+'\n')
   binary=(base if name=='baseline' else current)/'large_source_probe'
   cmd=['nice','-n','19','mpirun','--oversubscribe','--bind-to','none','-np','1',str(binary),str(mesh)]
   start=time.monotonic();p=subprocess.run(cmd,cwd=root,env=env,capture_output=True,text=True)
   (case/'stdout').write_text(p.stdout);(case/'stderr').write_text(p.stderr)
   assert p.returncode==0 and 'PASS large-source' in p.stdout
   observations=[];work=[];events=[]
   for line in p.stdout.splitlines():
    kind=line.split()[0] if line.split() else ''
    if kind in ('LARGE_IMPORT','LARGE_QUERY','LARGE_SUPPORT','LARGE_SUPPORT_QUERY','LARGE_SUPPORT_LIMIT'):observations.append(line)
    if kind=='LARGE_ENGINE':
     v=dict(x.split('=',1) for x in line.split()[1:]);work.append(tuple(v[k] for k in ['case','valid','source','initial','final_local','rounds','accepted','rejected','batches','retries','query_import_retries','candidates','filtered','payloads','max_patch']))
    if kind=='LARGE_EVENT':events.append(dict(x.split('=',1) for x in line.split()[1:]))
   meshes={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in mesh.glob('*.json')};assert len(meshes)==8
   records.append(dict(trial=trial,checkpoint=name,seconds=time.monotonic()-start,observations=observations,work=work,events=events,meshes=meshes,command=cmd))
 for trial in range(3):
  a,b=[r for r in records if r['trial']==trial]
  assert all(a[k]==b[k] for k in ('observations','work','meshes')),'Changed actual query, mesh or edit work'
 summary=[]
 for label in ('coarse_m1','wedge_m1','dense_m1','dense_wedge_m1'):
  for name in ('baseline','candidate'):
   values=[float(e['parent_s']) for r in records if r['checkpoint']==name for e in r['events'] if e['case']==label]
   assert len(values)==3;summary.append(dict(case=label,checkpoint=name,median=statistics.median(values),minimum=min(values),maximum=max(values)))
 status='PASS'
finally:
 result=dict(status=status,records=records,summary=summary if status=='PASS' else [],runner_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
 receipts={k:dict(path=str(p),sha256=hashlib.sha256((p/'validation.json').read_bytes()).hexdigest(),binary_sha256=receipts[k]['binary_sha256']) for k,p in [('baseline',base),('candidate',current)]},
 scope='Three alternating MPI1 process pairs; pinned identical source probe and identical query/mesh/edit work. Frozen source preparation/output/audit outside event timers. No CFD, scaling or affordability claim.')
 result['artifact_sha256']={str(p.relative_to(folder)):hashlib.sha256(p.read_bytes()).hexdigest() for p in folder.rglob('*') if p.is_file()}
 (folder/'validation.json').write_text(json.dumps(result,indent=2)+'\n');print(status,folder,flush=True)
