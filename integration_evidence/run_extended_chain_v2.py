"""Wait for the sole primal chain, then audit pilot meshes and validate merged auxiliary/AD paths."""
from pathlib import Path
import json,os,subprocess,sys,time,signal
source=Path(__file__).resolve().parent.parent
root=source/'integration_evidence';state=root/'extended_chain_v2.json';prereq=root/'primal_chain_v3.json'
if state.exists():raise SystemExit('Preserve previous evidence; choose a fresh version.')
status={'runner_pid':os.getpid(),'phase':'waiting_for_primal','started':time.time(),'steps':[]}
env=dict(os.environ,OMP_NUM_THREADS='1',OPENBLAS_NUM_THREADS='1',PYTHONDONTWRITEBYTECODE='1')
env['PATH']='/tmp/claude-1000/-media-rausa-4TB-SU2-Versions-SU2-AdapNoExt/bae33fc3-5b05-48ef-a3e8-4d222dffb2fe/scratchpad/ninjabin:'+env['PATH']
def save():state.write_text(json.dumps(status,indent=2)+'\n')
def quiet():
 since=None
 while True:
  busy=[]
  for line in subprocess.check_output(['ps','-eo','pid,stat,comm'],text=True).splitlines()[1:]:
   pid,flags,name=line.split(maxsplit=2)
   if 'Z' in flags or pid=='918696':continue
   if name in ('ninja','cc1plus','test_driver','test_driver_AD','test_memory') or name.startswith('SU2_CFD'):busy.append(int(pid))
  status.update(phase='waiting_for_machine',busy=busy,checked=time.time());save()
  if busy:since=None
  elif since is None:since=time.monotonic()
  elif time.monotonic()-since>=15:return
  time.sleep(5)
def run(label,command,cwd=source,timeout=None):
 quiet();started=time.monotonic()
 with (root/(label+'.log')).open('x') as output:
  child=subprocess.Popen(command,cwd=cwd,env=env,stdout=output,stderr=subprocess.STDOUT,start_new_session=True)
  status.update(phase='running',child_pid=child.pid,label=label,command=list(map(str,command)));save()
  try:code=child.wait(timeout=timeout)
  except subprocess.TimeoutExpired:
   os.killpg(child.pid,signal.SIGTERM)
   try:child.wait(timeout=10)
   except subprocess.TimeoutExpired:os.killpg(child.pid,signal.SIGKILL);child.wait()
   code=124
 row={'label':label,'exit':code,'elapsed_seconds':time.monotonic()-started,'command':list(map(str,command))}
 status['steps'].append(row);status.pop('child_pid',None);save()
 if code:raise RuntimeError(f'{label} exited {code}; stop before additional work')
 return row
save()
try:
 while True:
  try:previous=json.loads(prereq.read_text())
  except (FileNotFoundError,json.JSONDecodeError):time.sleep(5);continue
  if previous.get('phase')=='terminal':
   if previous.get('exit'):raise RuntimeError('Primal chain failed; await diagnosis before proceeding')
   break
  try:os.kill(previous['runner_pid'],0)
  except ProcessLookupError:raise RuntimeError('Primal supervisor disappeared; do not assume completion')
  time.sleep(5)
 status['source_revision']=subprocess.check_output(['git','rev-parse','HEAD'],cwd=source,text=True).strip();save()
 run('engine_pilot_audit_v2',[sys.executable,root/'audit_engine_scaling.py',root/'scaling_pilot_v3'])
 tags='[GoalMetric],[CustomSensors],[AdjointTransfer],Reference wall*,Wall size rule*,Two-pass*,Gate G1*,Gates G3*,Corner rule*,Pass-B donor*,Log-Euclidean mean*'
 run('auxiliary_runner_v2',[sys.executable,root/'run_native_driver_checks.py','--label','integrated_auxiliary_v2','--build','build-integrated-v2','--timeout','600','--filter',tags])
 build=root/'build-integrated-ad-v1'
 run('ad_setup_v2',[sys.executable,source/'externals/meson/meson.py','setup',build,source,'-Dwith-mpi=enabled','-Dwith-omp=false','-Denable-normal=false','-Denable-autodiff=true','-Denable-cgns=true','-Denable-tecio=false','-Denable-tests=true','-Denable-mmg=true','-Dmmg_scotch_root=/home/rausa/Software/scotch','-Dbuildtype=debugoptimized','-Db_ndebug=false'])
 run('ad_build_v2',['ninja','-C',build,'-j2','SU2_CFD/src/SU2_CFD_AD','UnitTests/test_driver_AD'])
 for ranks in (1,2,4):
  wd=root/f'integrated_ad_controls_np{ranks}_v2';wd.mkdir()
  row=run(f'ad_controls_np{ranks}_v2',['mpiexec','-n',str(ranks),build/'UnitTests/test_driver_AD','[GoalSwap]','--use-colour','no'],cwd=wd,timeout=600)
  text=(root/f'ad_controls_np{ranks}_v2.log').read_text()
  if text.count('All tests passed')!=ranks:raise RuntimeError('AD Catch result missing or no selected tests')
 status.update(phase='terminal',exit=0,ended=time.time());save()
except BaseException as error:
 status.update(phase='terminal',exit=1,reason=str(error),ended=time.time());status.pop('child_pid',None);save();raise
