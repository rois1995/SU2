"""Sequential closure of remaining BL/phase/partial-audit/support gates."""
from pathlib import Path
import hashlib,json,os,signal,subprocess,sys,time
root=Path(__file__).resolve().parent;source=root.parent
state=root/'native_validation_closure_v1.json'
if state.exists():raise SystemExit('Preserve prior evidence; use a fresh version')
status=dict(runner_pid=os.getpid(),phase='starting',steps=[])
env=dict(os.environ,OMP_NUM_THREADS='1',OPENBLAS_NUM_THREADS='1',PYTHONDONTWRITEBYTECODE='1')
def sha(path):
 h=hashlib.sha256()
 with Path(path).open('rb') as f:
  for b in iter(lambda:f.read(1024*1024),b''):h.update(b)
 return h.hexdigest()
def save():state.write_text(json.dumps(status,indent=2)+'\n')
def quiet():
 since=None
 while True:
  busy=[]
  for row in subprocess.check_output(['ps','-eo','pid,stat,comm'],text=True).splitlines()[1:]:
   pid,flags,name=row.split(maxsplit=2)
   if pid!='918696' and 'Z' not in flags and (name in ('ninja','cc1plus','test_driver','test_driver_AD','test_memory') or name.startswith('SU2_CFD')):busy.append(int(pid))
  status.update(phase='waiting_for_machine',busy=busy);save()
  if busy:since=None
  elif since is None:since=time.monotonic()
  elif time.monotonic()-since>=15:return
  time.sleep(5)
def run(label,command,cwd=source,timeout=900,diagnostic=None):
 command=list(map(str,command))
 if not any('run_native_driver_checks.py' in arg for arg in command):quiet()
 log=root/(label+'.log');begun=time.monotonic()
 with log.open('x') as out:
  child=subprocess.Popen(command,cwd=cwd,env=env,stdout=out,stderr=subprocess.STDOUT,start_new_session=True)
  status.update(phase='running',label=label,child_pid=child.pid,working_directory=str(cwd));save()
  try:code=child.wait(timeout=timeout)
  except subprocess.TimeoutExpired:
   os.killpg(child.pid,signal.SIGTERM)
   try:child.wait(timeout=10)
   except subprocess.TimeoutExpired:os.killpg(child.pid,signal.SIGKILL);child.wait()
   code=124
 text=log.read_text()
 passed=(0<code<128 and code!=124 and diagnostic in text and 'Mesh Adaptation Cycle' not in text) if diagnostic else code==0
 row=dict(label=label,command=command,working_directory=str(cwd),exit=code,elapsed_seconds=time.monotonic()-begun,verified=passed,expected_diagnostic=diagnostic,log=str(log))
 status['steps'].append(row);status.pop('child_pid',None);save()
 if not passed:raise RuntimeError(label+' failed; preserve and classify before continuing')
 return row
save()
try:
 build=json.loads((root/'native_phase_step_build_v1.json').read_text())
 if build['exit'] or sha(root/'build-integrated-v2/UnitTests/test_driver')!=build['binary_sha256']:raise RuntimeError('Rebuilt executable mismatch')
 for p,digest in build['source_sha256'].items():
  if sha(source/p)!=digest:raise RuntimeError('Rebuilt test source changed')
 status.update(binary_sha256=build['binary_sha256'],source_sha256=build['source_sha256'],runner_sha256=sha(__file__))
 tools=['run_native_driver_checks.py','run_native_bl_audits.py','run_native_airfoil_audits.py','audit_native_airfoil.py','audit_native_bl.py','frozen_field_audit.py','airfoil_reference_audit.py','check_airfoil_phase_timing.py']
 status['tool_sha256']={p:sha(root/p) for p in tools};save()
 for label,tag in [('integrated_bl_smooth_v1','[NativeBL2D]'),('integrated_bl_step_v1','[NativeBLStep2D]')]:
  run(label,[sys.executable,root/'run_native_driver_checks.py','--label',label,'--build','build-integrated-v2','--filter',tag,'--save-audit','--timeout','240'],timeout=1000)
  run(label+'_audit',[sys.executable,root/'run_native_bl_audits.py',root/label,'--label','independent_v1'],timeout=600)
 for label,tag,diagnostic in [('native_unsupported_derivative_v1','[NativeUnsupportedDerivative]','Native adaptation does not support continuous or discrete adjoint configurations.'),('native_missing_restart_reference_v1','[NativeMissingRestartReference]','Native solution restart requires the original geometry sidecar:')]:
  run(label,[sys.executable,root/'run_native_driver_checks.py','--label',label,'--build','build-integrated-v2','--filter',tag,'--timeout','60','--expect-diagnostic',diagnostic],timeout=400)
 label='integrated_airfoil_phase_timing_v1'
 run(label,[sys.executable,root/'run_native_driver_checks.py','--label',label,'--build','build-integrated-v2','--filter','[NativeAirfoil2D]','--save-audit','--timeout','900','--airfoil-config',root/'integrated_airfoil_baseline_v8/airfoil_input.cfg'],timeout=3600)
 run(label+'_audit',[sys.executable,root/'run_native_airfoil_audits.py',root/label,'--label','independent_v1','--height','.0002'],timeout=1800)
 run(label+'_timing',[sys.executable,root/'check_airfoil_phase_timing.py',root/label,'--output',root/label/'timing_audit_v1.json'],timeout=120)
 for name,cycle,height in [('complexity12000',1,.0002),('height1e4',0,.0001)]:
  case=root/f'integrated_airfoil_{name}_v8'
  run('partial_'+name+'_audit_v1',[sys.executable,root/'audit_native_airfoil.py',case/'audit_np1','--cycle',cycle,'--height',height,'--suffix','adapted','--reference',case/'airfoil_input.su2','--output',case/'partial_target_audit_v1.json'],timeout=900)
 # Actual application parsing: native + TWO_PASS must stop before the first CFD cycle.
 directory=root/'native_twopass_rejection_v1';directory.mkdir()
 cfg=(root/'integrated_native_production_v2/run_template.cfg').read_text().replace('MESH_FILENAME= input.su2','MESH_FILENAME= '+str(source/'QuickStart/mesh_NACA0012_inv.su2'))+'\nADAP_BL_METHOD= TWO_PASS\n'
 binary=root/'integrated_native_production_v1/SU2_CFD'
 for ranks in (1,2,4):
  wd=directory/f'np{ranks}';wd.mkdir();(wd/'run.cfg').write_text(cfg)
  row=run('native_twopass_p'+str(ranks)+'_v1',['mpiexec','-n',ranks,binary,'run.cfg'],cwd=wd,timeout=60,diagnostic='ADAP_BL_METHOD= TWO_PASS requires ADAP_REMESHER= MMG; native cavities build the BL with METRIC.')
  row['config_sha256']=sha(wd/'run.cfg');row['binary_sha256']=sha(binary);save()
 for p,digest in status['tool_sha256'].items():
  if sha(root/p)!=digest:raise RuntimeError('Closure tool changed during execution')
 status.update(phase='terminal',exit=0,ended=time.time());save()
except BaseException as error:
 status.update(phase='terminal',exit=1,reason=str(error),ended=time.time());status.pop('child_pid',None);save();raise
