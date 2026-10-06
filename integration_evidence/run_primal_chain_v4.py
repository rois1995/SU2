"""Single sequential integration build/control/pilot chain; live state and provenance on disk."""
from pathlib import Path
import hashlib,json,os,subprocess,sys,time,signal
source=Path(__file__).resolve().parent.parent
root=source/'integration_evidence'
state=root/'primal_chain_v4.json'
if state.exists():raise SystemExit('Use a new version; preserve evidence.')
status={'runner_pid':os.getpid(),'started':time.time(),'steps':[],'source_revision':subprocess.check_output(['git','rev-parse','HEAD'],cwd=source,text=True).strip()}
env=dict(os.environ,OMP_NUM_THREADS='1',OPENBLAS_NUM_THREADS='1',PYTHONDONTWRITEBYTECODE='1')
env['PATH']='/tmp/claude-1000/-media-rausa-4TB-SU2-Versions-SU2-AdapNoExt/bae33fc3-5b05-48ef-a3e8-4d222dffb2fe/scratchpad/ninjabin:'+env['PATH']
def save():state.write_text(json.dumps(status,indent=2)+'\n')
def quiet():
 since=None
 while True:
  busy=[]
  for row in subprocess.check_output(['ps','-eo','pid,stat,comm'],text=True).splitlines()[1:]:
   pid,flags,name=row.split(maxsplit=2)
   if 'Z' in flags or pid=='918696':continue
   if name in ('ninja','cc1plus','test_driver','test_driver_AD','test_memory') or name.startswith('SU2_CFD'):busy.append({'pid':int(pid),'name':name})
  status.update(phase='waiting_for_machine',observed_busy=busy,checked=time.time());save()
  if busy:since=None
  elif since is None:since=time.monotonic()
  elif time.monotonic()-since>=15:return
  time.sleep(5)
def run(label,command,cwd=source,extra=None,timeout=None):
 quiet()
 actual=dict(env,**(extra or {}));start=time.monotonic()
 log=root/(label+'.log')
 with log.open('x') as output:
  child=subprocess.Popen(command,cwd=cwd,env=actual,stdout=output,stderr=subprocess.STDOUT,start_new_session=True)
  status.update(phase='running',label=label,child_pid=child.pid,command=list(map(str,command)),checked=time.time());save()
  try:code=child.wait(timeout=timeout)
  except subprocess.TimeoutExpired:
   os.killpg(child.pid,signal.SIGTERM)
   try:child.wait(timeout=10)
   except subprocess.TimeoutExpired:os.killpg(child.pid,signal.SIGKILL);child.wait()
   code=124
 row={'label':label,'command':list(map(str,command)),'exit':code,'elapsed_seconds':time.monotonic()-start,'log':str(log)}
 status['steps'].append(row);status.pop('child_pid',None);save()
 if code:raise RuntimeError(f'{label} exited {code}; stop before further jobs')
 return row
try:
 build=root/'build-integrated-v2'
 run('primal_build_v4',['ninja','-C',build,'-j2','SU2_CFD/src/SU2_CFD','UnitTests/test_driver','UnitTests/test_memory'])
 tags='[NativeReader],[NativeImport2D],[NativeMesh2D],[NativeField2D],[NativeDistributed2D],[NativeEngine2D],[NativeGeometry],[NativeCollision],[NativeReferenceIO],[NativeFixedAdaptive2D],[NativeRemesher],[NativeAirfoilSlices2D],[NativeCGNS2D],[NativeSupport2D],[NativeBL2D],[NativeProducedBL2D],[MeshOrder],Mesh output from memory*,[Output]'
 run('primal_controls_runner_v4',[sys.executable,root/'run_native_driver_checks.py','--label','integrated_primal_controls_v4','--build','build-integrated-v2','--timeout','600','--filter',tags,'--save-audit'])
 run('primal_memory_runner_v4',[sys.executable,root/'run_native_driver_checks.py','--label','integrated_primal_memory_v4','--build','build-integrated-v2','--timeout','240','--binary','test_memory','--filter','[NativeMemory]'])
 run('primal_mmg_runner_v4',[sys.executable,root/'run_native_driver_checks.py','--label','integrated_mmg_default_v4','--build','build-integrated-v2','--timeout','240','--filter','[MMGDefault2D]','--save-audit'])
 run('primal_output_audits_v4',[sys.executable,root/'run_native_final_output_audits.py',root/'integrated_primal_controls_v4','--label','independent_outputs_v4'])
 pilot=root/'scaling_pilot_v4';pilot.mkdir()
 binary=build/'UnitTests/test_driver'
 status['pilot_binary_sha256']=hashlib.sha256(binary.read_bytes()).hexdigest();save()
 for tiles in (1,4,16,64):
  for ranks in (1,2,4):
   wd=pilot/f't{tiles}_p{ranks}_layout1_ar10';wd.mkdir()
   row=run(wd.name,['mpiexec','-n',str(ranks),binary,'[NativeScaling2D]','--use-colour','no'],cwd=wd,extra={'SU2_NATIVE_SCALING_TILES':str(tiles),'SU2_NATIVE_SCALING_LAYOUT':'1','SU2_NATIVE_SCALING_AR':'10'},timeout=240)
   if not (wd/'native_scaling.json').is_file():raise RuntimeError('No scaling outcome artifact')
   row['measurement']=json.loads((wd/'native_scaling.json').read_text());save()
 status.update(phase='terminal',exit=0,ended=time.time());save()
except BaseException as error:
 status.update(phase='terminal',exit=1,reason=str(error),ended=time.time());status.pop('child_pid',None);save();raise
