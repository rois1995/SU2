"""Sequential actual-Meson native3D driver checks. No standalone kernel overrides."""
from pathlib import Path
import hashlib,json,os,shlex,subprocess,sys,time
root=Path(__file__).resolve().parents[2];where=Path(__file__).resolve().parent
import argparse,shutil
parser=argparse.ArgumentParser(description='One sequential manufactured native3D envelope case using actual configured archives.')
parser.add_argument('folder');parser.add_argument('--ranks',type=int,choices=(1,2,4),default=1)
parser.add_argument('--binary-from',type=Path);parser.add_argument('--workers',type=int,default=1);parser.add_argument('--size',type=int,choices=(1,2,3,4),default=2)
parser.add_argument('--anisotropy',type=int,choices=(0,1,2),default=1);parser.add_argument('--varying',type=int,choices=(0,1),default=0)
parser.add_argument('--repeat',type=int,choices=(0,1,2),default=0);parser.add_argument('--detail',type=int,choices=(0,1),default=1)
options=parser.parse_args();assert 1<=options.workers<=options.ranks
folder=Path(options.folder).resolve();build=root/'integration_evidence/build-integrated-v2'
print('Case working and retained folder:',folder,flush=True);folder.mkdir(exist_ok=False)
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
paths=subprocess.check_output(['git','ls-files','Common','SU2_CFD','UnitTests','meson.build','meson_options.txt'],cwd=root,text=True).splitlines()
extras=['Common/include/adaptation/CNativeRuntimeProfile3D.hpp','integration_evidence/native_3d_core_v1/audit_lifecycle.py','Common/include/adaptation/CNativeReferenceIO3D.hpp','Common/include/adaptation/CNativeRemesher3D.hpp','Common/src/adaptation/CNativeReferenceIO3D.cpp','Common/src/adaptation/CNativeRemesher3D.cpp',str(Path(__file__).resolve().relative_to(root)),'integration_evidence/native_3d_core_v1/audit_runtime.py','integration_evidence/native_3d_core_v1/audit_geometry_bridge.py','integration_evidence/native_3d_core_v1/audit_coupled.py','integration_evidence/native_3d_core_v1/audit_cavities.py','UnitTests/SU2_CFD/adaptation/CNativeRemesher3D_tests.cpp','integration_evidence/native_3d_core_v1/audit_defaults.py']
extras+=['integration_evidence/native_3d_core_v1/audit_envelope.py','integration_evidence/native_3d_core_v1/export_envelope_vtu.py'];paths+=extras
pins={p:sha(root/p) for p in paths if (root/p).is_file() and (p.endswith(('.cpp','.hpp','.h','.txt','.py')) or p.endswith('meson.build'))}
owned=subprocess.check_output(['git','diff','--name-only'],cwd=root,text=True).splitlines()+extras
for p in set(owned):
 if (root/p).is_file():q=folder/'source_snapshots'/p;q.parent.mkdir(parents=True,exist_ok=True);q.write_bytes((root/p).read_bytes())
lines=(build/'build.ninja').read_text().splitlines();i=next(i for i,l in enumerate(lines) if l.startswith('build UnitTests/test_driver:'))
args=shlex.split(next(l.split('=',1)[1].strip() for l in lines[i+1:i+8] if l.strip().startswith('LINK_ARGS =')))
args=[str(build/p) if p.endswith('.a') and not p.startswith('/') else p for p in args if not p.startswith('-Wl,-rpath')]
targets=['UnitTests/test_driver.p/SU2_CFD_adaptation_CNativeImport3D_tests.cpp.o','UnitTests/test_driver.p/test_driver.cpp.o','UnitTests/test_driver.p/SU2_CFD_adaptation_CNativeRemesher_tests.cpp.o','UnitTests/test_driver.p/Common_adaptation_CMMGInterface_tests.cpp.o','UnitTests/test_driver.p/SU2_CFD_adaptation_AdaptationMPI_tests.cpp.o','UnitTests/test_driver.p/SU2_CFD_adaptation_CNativeRemesher3D_tests.cpp.o']
env=dict(os.environ,PATH='/tmp/native-build-tools:'+os.environ['PATH'],OMP_NUM_THREADS='1',OPENBLAS_NUM_THREADS='1',MKL_NUM_THREADS='1',OMPI_MCA_mpi_yield_when_idle='1',OMPI_MCA_hwloc_base_binding_policy='none',PYTHONDONTWRITEBYTECODE='1')
env.update({f'SU2_NATIVE3D_ENVELOPE_{key.upper()}':str(getattr(options,key)) for key in ('size','anisotropy','varying','workers','repeat','detail')})
(folder/'parameters.json').write_text(json.dumps(vars(options),indent=2,default=str)+'\n')
status='FAIL';stages=[];libs={};members={};objects={};binary=folder/'test_runtime'
def run(command,label,cwd=root):
 command=['timeout','--signal=TERM','--kill-after=10s','900s']+list(map(str,command));print('Starting',label,flush=True);start=time.monotonic();p=subprocess.run(command,cwd=cwd,env=env,capture_output=True,text=True)
 (folder/(label+'.stdout')).write_text(p.stdout);(folder/(label+'.stderr')).write_text(p.stderr)
 stages.append(dict(stage=label,command=command,cwd=str(cwd),seconds=time.monotonic()-start,exit_code=p.returncode))
 if p.returncode:raise RuntimeError(label+' failed; inspect '+str(folder))
 return p.stdout
try:
 for cmd,label in [(['uptime'],'host_load'),(['free','-m'],'host_memory'),(['ps','-eo','pid,ppid,ni,pcpu,etime,comm','--sort=-pcpu'],'host_processes')]:run(cmd,label)
 run(['nice','-n','19','/tmp/native-build-tools/ninja','-C',build,'-j1','Common/src/libSU2Common.a','SU2_CFD/src/libSU2core.a','Common/src/libSU2AccurateSum.a','SU2_CFD/src/SU2_CFD']+targets,'compile_actual_meson_tests')
 libs={p:sha(Path(p)) for p in args if p.endswith('.a') and Path(p).exists()}
 for library in libs:
  with Path(library).open('rb') as f:thin=f.read(8)==b'!<thin>\n'
  if thin:
   for member in subprocess.check_output(['ar','t',library],cwd=root,text=True).splitlines():
    p=Path(member);p=p if p.is_absolute() else root/p;members[str(p)]=sha(p)
 objects={p:sha(build/p) for p in targets}
 if options.binary_from:
  start=time.monotonic();previous=options.binary_from.resolve();qualified=json.loads((previous/'validation.json').read_text())
  if qualified['status']!='PASS':
   recovery=json.loads((previous/'AUDIT_RECOVERY.json').read_text())
   assert recovery['status']=='PASS' and recovery['original_validation_sha256']==sha(previous/'validation.json')
   assert recovery['independent_audit_sha256']==sha(previous/'independent_audit.json')
  assert all(x['exit_code']==0 for x in qualified['stages'] if x['stage']!='independent_audit')
  assert not qualified['changed_during_run'] and not qualified['archive_changes'] and not qualified['object_changes']
  assert qualified['library_sha256']==libs and qualified['thin_archive_member_sha256']==members and qualified['actual_test_object_sha256']==objects
  assert qualified['build_ninja_sha256']==sha(build/'build.ninja')
  assert sha(previous/'test_runtime')==qualified['binary_sha256']
  try:os.link(previous/'test_runtime',binary)
  except OSError:shutil.copy2(previous/'test_runtime',binary)
  assert sha(binary)==qualified['binary_sha256']
  stages.append(dict(stage='reuse_qualified_binary',seconds=time.monotonic()-start,exit_code=0,from_folder=str(previous),validation_sha256=sha(previous/'validation.json'),binary_sha256=sha(binary)))
 else:
  run(['nice','-n','19','mpicxx','-pthread']+[build/p for p in targets]+args+['-o',binary],'link_actual_archives')
 run(['uptime'],'before_case_load')
 out=run(['nice','-n','19','mpirun','--oversubscribe','--bind-to','none','-np',str(options.ranks),binary,'[NativeEnvelope3D]','--use-colour','no'],'case',folder)
 assert out.count('All tests passed')==options.ranks
 run(['nice','-n','19',sys.executable,where/'audit_envelope.py',folder],'independent_audit',folder)
 run(['nice','-n','19',sys.executable,where/'export_envelope_vtu.py',folder],'vtu_export',folder)
 status='PASS'
finally:
 changed=[p for p,h in pins.items() if sha(root/p)!=h]
 archive_changes=[p for p,h in {**libs,**members}.items() if sha(Path(p))!=h]
 object_changes=[p for p,h in objects.items() if sha(build/p)!=h]
 if changed or archive_changes or object_changes:status='FAIL'
 result=dict(status=status,source_sha256=pins,changed_during_run=changed,archive_changes=archive_changes,object_changes=object_changes,library_sha256=libs,thin_archive_member_sha256=members,actual_test_object_sha256=objects,build_ninja_sha256=sha(build/'build.ninja'),stages=stages,
             scope='Actual configured complete-archive native3D manufactured anisotropic driver envelope: three refine/coarsen/refine events and original-P1/BDF2 audits at one fixed N/M/size/metric/detail/repeat. Manufactured metric preparation; no CFD iteration, measured scaling or full Goal1 completion.')
 if binary.exists():result['binary_sha256']=sha(binary)
 result['artifact_sha256']={str(p.relative_to(folder)):sha(p) for p in folder.rglob('*') if p.is_file() and p.suffix not in ('.o','.pyc') and p.name!='test_runtime'}
 (folder/'validation.json').write_text(json.dumps(result,indent=2)+'\n');print(status,folder,flush=True)
