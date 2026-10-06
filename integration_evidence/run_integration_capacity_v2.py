"""Sequential per-rank size limits and MPI controls for successful serial demands.
Wait for the independent production-output matrix to finish. Preserve any
unexpected failure for diagnosis; never equate a process failure to infeasibility.
"""
from pathlib import Path
import csv, hashlib, json, math, os, shutil, signal, subprocess, sys, time
source=Path(__file__).resolve().parent.parent
root=source/'integration_evidence';state=root/'integration_capacity_v2.json'
if state.exists():raise SystemExit('Preserve earlier evidence; choose a fresh version.')
status={'runner_pid':os.getpid(),'phase':'waiting_for_production','started':time.time(),'steps':[]}
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
def run(label,command,cwd=source,extra=None,timeout=1800,allow_timeout=False):
 quiet();log=root/(label+'.log');begun=time.monotonic();command=list(map(str,command))
 with log.open('x') as out:
  child=subprocess.Popen(command,cwd=cwd,env=dict(env,**(extra or {})),stdout=out,stderr=subprocess.STDOUT,start_new_session=True)
  status.update(phase='running',label=label,child_pid=child.pid);save()
  try:code=child.wait(timeout=timeout)
  except subprocess.TimeoutExpired:
   os.killpg(child.pid,signal.SIGTERM)
   try:child.wait(timeout=10)
   except subprocess.TimeoutExpired:os.killpg(child.pid,signal.SIGKILL);child.wait()
   code=124
 row=dict(label=label,exit=code,command=command,environment=extra or {},elapsed_seconds=time.monotonic()-begun,log=str(log))
 status['steps'].append(row);status.pop('child_pid',None);save()
 if code and not (allow_timeout and code==124):raise RuntimeError(label+' failed; classify preserved evidence before proceeding')
 return row
save()
try:
 prior=root/'native_production_chain_v3.json'
 while True:
  try:p=json.loads(prior.read_text())
  except (FileNotFoundError,json.JSONDecodeError):time.sleep(5);continue
  if p.get('phase')=='terminal':
   if p.get('exit'):raise RuntimeError('Production prerequisite failed; diagnose before continuing')
   break
  try:os.kill(p['runner_pid'],0)
  except ProcessLookupError:raise RuntimeError('Prerequisite supervisor missing; no assumed completion')
  time.sleep(5)
 directory=root/'integration_capacity_v2';directory.mkdir()
 status.update(runner_sha256=sha(__file__), source_revision=subprocess.check_output(['git','rev-parse','HEAD'],cwd=source,text=True).strip())
 baseline=json.loads((root/'robustness_chain_v8.json').read_text())
 binary=root/'robustness_campaign_v8/test_driver'
 if sha(binary)!=baseline['binary_sha256']:raise RuntimeError('Archived engine executable mismatch')
 status['engine_binary_sha256']=sha(binary);save()
 # A serial cost boundary does not prove the same boundary at higher rank counts.
 for ranks in (2,4):
  for tiles in (1024,2048,4096):
   group=directory/f'larger_np{ranks}_t{tiles}';group.mkdir()
   wd=group/f't{tiles}_p{ranks}';wd.mkdir()
   row=run('capacity_v2_'+wd.name,['mpiexec','-n',ranks,binary,'[NativeScaling2D]','--use-colour','no'],cwd=wd,
      extra={'SU2_NATIVE_SCALING_TILES':str(tiles),'SU2_NATIVE_SCALING_LAYOUT':'3'},timeout=240,allow_timeout=True)
   if row['exit']==124:
    row['outcome']='time_budget';save();break
   if Path(row['log']).read_text().count('All tests passed')!=ranks:raise RuntimeError('Missing Catch rank reports')
   row['measurement']=json.loads((wd/'native_scaling.json').read_text());save()
   # Fresh audit output per case, including incomplete contracts.
   run('audit_capacity_v2_'+wd.name,[sys.executable,root/'audit_engine_scaling.py',group],timeout=600)
   if not row['measurement']['complete']:
    row['outcome']='incomplete_contract';save();break
   if row['measurement']['adapt_seconds']>=120:
    row['outcome']='declared_cost_stop';save();break
 status['demand_controls_skipped']=[];save()
 # Follow protocol step7: control every successful serial demand at2/4 ranks.
 if sha(root/'build-integrated-v2/UnitTests/test_driver')!=baseline['binary_sha256']:
  raise RuntimeError('Current airfoil test executable differs from the measured baseline')
 for name,height in [('complexity12000',.0002),('height1e4',.0001),('height5e5',.00005)]:
  previous=root/f'integrated_airfoil_{name}_v8/evidence.json'
  if not previous.exists():
   status['demand_controls_skipped'].append(dict(name=name,reason='serial axis not executed'));save();continue
  result=json.loads(previous.read_text())
  if not result.get('runs') or not all(r['verified'] for r in result['runs']):
   status['demand_controls_skipped'].append(dict(name=name,reason='serial lifecycle failed; successful-serial control rule'));save();continue
  for ranks in (2,4):
   label=f'integrated_airfoil_{name}_p{ranks}_capacity_v2'
   run(label,[sys.executable,root/'run_native_driver_checks.py','--label',label,'--build','build-integrated-v2','--timeout','900',
       '--filter','[NativeAirfoil2D]','--save-audit','--airfoil-config',root/'airfoil_inputs'/(name+'.cfg'),'--ranks',ranks],timeout=1500)
   run(label+'_audit',[sys.executable,root/'run_native_airfoil_audits.py',root/label,'--label','independent_v1','--height',height,'--ranks',ranks],timeout=1800)
 # Outstanding actual4-rank goal/interruption controls remain a separate required gate.
 status['scope']='Native higher-rank capacities and successful serial demand controls only; no AD lifecycle claim'
 status['remaining_ad_controls']=['warm/cold4','interruption primal/adjoint1/2','native-adjoint rejection']
 status.update(phase='terminal',exit=0,ended=time.time());save()
except BaseException as error:
 status.update(phase='terminal',exit=1,reason=str(error),ended=time.time());status.pop('child_pid',None);save();raise
