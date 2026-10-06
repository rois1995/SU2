"""Validate the MPI warm-start halo-seed fix with original ILU settings."""
from pathlib import Path
import hashlib, json, os, shutil, signal, subprocess, time
source=Path(__file__).resolve().parent.parent
root=source/'integration_evidence';state=root/'ad_repair_controls_v14.json'
if state.exists():raise SystemExit('Preserve previous evidence; choose a fresh version.')
status={'runner_pid':os.getpid(),'phase':'waiting_for_build','started':time.time(),'steps':[]}
env=dict(os.environ,OMP_NUM_THREADS='1',OPENBLAS_NUM_THREADS='1',PYTHONDONTWRITEBYTECODE='1')
def save():state.write_text(json.dumps(status,indent=2)+'\n')
def sha(path):
 h=hashlib.sha256()
 with Path(path).open('rb') as f:
  for b in iter(lambda:f.read(1024*1024),b''):h.update(b)
 return h.hexdigest()
def quiet():
 since=None
 while True:
  busy=[]
  for line in subprocess.check_output(['ps','-eo','pid,stat,comm'],text=True).splitlines()[1:]:
   pid,flags,name=line.split(maxsplit=2)
   if 'Z' not in flags and pid!='918696' and (name in ('ninja','cc1plus','test_driver','test_driver_AD','test_memory') or name.startswith('SU2_CFD')):busy.append(int(pid))
  status.update(phase='waiting_for_machine',busy=busy,checked=time.time());save()
  if busy:since=None
  elif since is None:since=time.monotonic()
  elif time.monotonic()-since>=15:return
  time.sleep(5)
save()
try:
 quiet()
 if '[2/2] Linking target UnitTests/test_driver_AD' not in (root/'ad_final_fixture_rebuild_v14.log').read_text():raise RuntimeError('Final fixture build not verified')
 archive=root/'ad_repair_archive_v14';archive.mkdir()
 build=root/'build-integrated-ad-v1'
 status.update(source_revision=subprocess.check_output(['git','rev-parse','HEAD'],cwd=source,text=True).strip(),source_status=subprocess.check_output(['git','status','--short'],cwd=source,text=True),binary_sha256={})
 for name,rel in [('SU2_CFD_AD','SU2_CFD/src/SU2_CFD_AD'),('test_driver_AD','UnitTests/test_driver_AD')]:
  p=build/rel;shutil.copy2(p,archive/name);status['binary_sha256'][name]=sha(p)
 status['source_sha256']={}
 for rel in subprocess.check_output(['git','ls-files','-z','Common','SU2_CFD','UnitTests','meson.build','meson_options.txt'],cwd=source,text=True).split('\0'):
  if rel and (source/rel).is_file():status['source_sha256'][rel]=sha(source/rel)
 save()
 for ranks in (1,2,4):
  quiet();wd=root/f'integrated_ad_controls_np{ranks}_v14';wd.mkdir()
  command=['mpiexec','-n',str(ranks),str(build/'UnitTests/test_driver_AD'),'[GoalSwap]','--use-colour','no'];log=root/f'ad_controls_np{ranks}_v14.log';begun=time.monotonic()
  with log.open('x') as out:
   child=subprocess.Popen(command,cwd=wd,env=env,stdout=out,stderr=subprocess.STDOUT,start_new_session=True)
   status.update(phase='running',label=f'ad_controls_np{ranks}_v14',child_pid=child.pid);save()
   try:code=child.wait(timeout=600)
   except subprocess.TimeoutExpired:
    os.killpg(child.pid,signal.SIGTERM)
    try:child.wait(timeout=10)
    except subprocess.TimeoutExpired:os.killpg(child.pid,signal.SIGKILL);child.wait()
    code=124
  text=log.read_text();row={'ranks':ranks,'exit':code,'elapsed_seconds':time.monotonic()-begun,'command':command,'verified':code==0 and text.count('All tests passed')==ranks,'log':str(log)}
  status['steps'].append(row);status.pop('child_pid',None);save()
  if not row['verified']:raise RuntimeError(f'AD controls at{ranks} ranks failed; preserve and diagnose')
 status.update(phase='terminal',exit=0,ended=time.time());save()
except BaseException as error:
 status.update(phase='terminal',exit=1,reason=str(error),ended=time.time());status.pop('child_pid',None);save();raise
