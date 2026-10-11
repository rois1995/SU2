"""Qualify retained configured automatic-repair records independently; never reruns adaptation."""
from pathlib import Path
import hashlib,json,os,subprocess,sys,time
where=Path(__file__).resolve().parent
source=Path(sys.argv[1]).resolve();folder=Path(sys.argv[2]).resolve();folder.mkdir(exist_ok=False)
print('Case working and retained folder:',folder,flush=True)
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
receipt=json.loads((source/'validation.json').read_text());assert receipt['status']=='PASS'
assert not receipt['source_drift'] and not receipt['archive_drift'] and not receipt['object_drift']
status='FAIL';stages=[]
env=dict(os.environ,PYTHONDONTWRITEBYTECODE='1',OMP_NUM_THREADS='1',OPENBLAS_NUM_THREADS='1',MKL_NUM_THREADS='1')
pins={str(p):sha(p) for p in [source/'validation.json',Path(__file__).resolve()]+list(where.glob('audit*.py'))}
try:
 for ranks in (1,2,4):
  log=source/f'mpi{ranks}.stdout';assert receipt['artifact_sha256'][log.name]==sha(log)
  command=['timeout','--signal=TERM','--kill-after=10s','300s','nice','-n','19',sys.executable,where/'audit_engine_repairs.py',log,folder/f'mpi{ranks}',str(ranks)]
  start=time.monotonic();p=subprocess.run(list(map(str,command)),env=env,capture_output=True,text=True)
  (folder/f'mpi{ranks}.stdout').write_text(p.stdout);(folder/f'mpi{ranks}.stderr').write_text(p.stderr)
  stages.append(dict(ranks=ranks,command=list(map(str,command)),seconds=time.monotonic()-start,exit_code=p.returncode,source_log_sha256=sha(log)))
  print('mpi',ranks,p.returncode,p.stdout.strip(),flush=True);assert p.returncode==0
 assert all(sha(Path(p))==h for p,h in pins.items())
 status='PASS'
finally:
 result=dict(status=status,source_folder=str(source),pins=pins,stages=stages,
             scope='Independent exact positivity/embedding/incidence/facet/marker/volume/original-P1 and strict final residual audit of retained automatic Repair/Refine/Coarsen unit/thin/sheared fixtures with M=N and M<N, including missing/tampered source/final metric negative controls. No adaptation rerun, actual CFD, full scaling or full Goal1 completion.')
 result['artifact_sha256']={str(p.relative_to(folder)):sha(p) for p in folder.rglob('*') if p.is_file()}
 (folder/'validation.json').write_text(json.dumps(result,indent=2)+'\n');print(status,folder,flush=True)
