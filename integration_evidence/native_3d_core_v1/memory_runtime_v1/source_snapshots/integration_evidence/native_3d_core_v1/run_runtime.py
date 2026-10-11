"""Sequential actual-Meson native3D driver checks. No standalone kernel overrides."""
from pathlib import Path
import hashlib,json,os,shlex,subprocess,sys,time
root=Path(__file__).resolve().parents[2];where=Path(__file__).resolve().parent
folder=Path(sys.argv[1]).resolve();build=root/'integration_evidence/build-integrated-v2'
print('Case working and retained folder:',folder,flush=True);folder.mkdir(exist_ok=False)
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
paths=subprocess.check_output(['git','ls-files','Common','SU2_CFD','UnitTests','meson.build','meson_options.txt'],cwd=root,text=True).splitlines()
extras=['Common/include/adaptation/CNativeReferenceIO3D.hpp','Common/include/adaptation/CNativeRemesher3D.hpp','Common/src/adaptation/CNativeReferenceIO3D.cpp','Common/src/adaptation/CNativeRemesher3D.cpp',str(Path(__file__).resolve().relative_to(root)),'integration_evidence/native_3d_core_v1/audit_runtime.py','integration_evidence/native_3d_core_v1/audit_geometry_bridge.py','integration_evidence/native_3d_core_v1/audit_coupled.py','integration_evidence/native_3d_core_v1/audit_cavities.py','UnitTests/SU2_CFD/adaptation/CNativeRemesher3D_tests.cpp','integration_evidence/native_3d_core_v1/audit_defaults.py']
paths+=extras
pins={p:sha(root/p) for p in paths if (root/p).is_file() and (p.endswith(('.cpp','.hpp','.h','.txt','.py')) or p.endswith('meson.build'))}
owned=subprocess.check_output(['git','diff','--name-only'],cwd=root,text=True).splitlines()+extras
for p in set(owned):
 if (root/p).is_file():q=folder/'source_snapshots'/p;q.parent.mkdir(parents=True,exist_ok=True);q.write_bytes((root/p).read_bytes())
lines=(build/'build.ninja').read_text().splitlines();i=next(i for i,l in enumerate(lines) if l.startswith('build UnitTests/test_driver:'))
args=shlex.split(next(l.split('=',1)[1].strip() for l in lines[i+1:i+8] if l.strip().startswith('LINK_ARGS =')))
args=[str(build/p) if p.endswith('.a') and not p.startswith('/') else p for p in args if not p.startswith('-Wl,-rpath')]
targets=['UnitTests/test_driver.p/SU2_CFD_adaptation_CNativeImport3D_tests.cpp.o','UnitTests/test_driver.p/test_driver.cpp.o','UnitTests/test_driver.p/SU2_CFD_adaptation_CNativeRemesher_tests.cpp.o','UnitTests/test_driver.p/Common_adaptation_CMMGInterface_tests.cpp.o','UnitTests/test_driver.p/SU2_CFD_adaptation_AdaptationMPI_tests.cpp.o','UnitTests/test_driver.p/SU2_CFD_adaptation_CNativeRemesher3D_tests.cpp.o']
env=dict(os.environ,PATH='/tmp/native-build-tools:'+os.environ['PATH'],OMP_NUM_THREADS='1',OPENBLAS_NUM_THREADS='1',MKL_NUM_THREADS='1',OMPI_MCA_mpi_yield_when_idle='1',OMPI_MCA_hwloc_base_binding_policy='none',PYTHONDONTWRITEBYTECODE='1')
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
 run(['nice','-n','19','mpicxx','-pthread']+[build/p for p in targets]+args+['-o',binary],'link_actual_archives')
 for n in (1,2,4):
  work=folder/('budget_mpi'+str(n));work.mkdir();print('Case working and retained folder:',work,flush=True)
  run(['uptime'],'before_budget_mpi'+str(n)+'_load')
  out=run(['nice','-n','19','mpirun','--oversubscribe','--bind-to','none','-np',str(n),binary,'[NativeBudget3D]','--use-colour','no'],'budget_mpi'+str(n),work)
  assert out.count('All tests passed')==n
 for n in (1,2,4):
  work=folder/('mpi'+str(n));work.mkdir();print('Case working and retained folder:',work,flush=True)
  for cmd,label in [(['uptime'],'load'),(['free','-m'],'memory'),(['ps','-eo','pid,ppid,ni,pcpu,etime,comm','--sort=-pcpu'],'processes')]:run(cmd,'before_mpi'+str(n)+'_'+label)
  out=run(['nice','-n','19','mpirun','--oversubscribe','--bind-to','none','-np',str(n),binary,'[NativeRuntime3D],[NativeReference3D],[NativeBridge2D]','--use-colour','no'],'mpi'+str(n),work)
  assert out.count('All tests passed')==n
  run(['nice','-n','19',sys.executable,where/'audit_runtime.py',work,str(n),folder/('mpi'+str(n)+'.stdout')],'audit_mpi'+str(n))
 # Relevant actual native2D driver and MMG interface checks use the same rebuilt libraries.
 for n in (1,2,4):
  work=folder/('regression_mpi'+str(n));work.mkdir();print('Case working and retained folder:',work,flush=True)
  run(['uptime'],'before_regression_mpi'+str(n)+'_load')
  out=run(['nice','-n','19','mpirun','--oversubscribe','--bind-to','none','-np',str(n),binary,'[NativeRemesher],[NativeDefault3D],MPI adaptation: extracted and remeshed mesh independent of the ranks','--use-colour','no'],'regression_mpi'+str(n),work)
  assert out.count('All tests passed')==n
 for n in (1,2,4):
  for suffix in ('.su2','.su2.native_ref'):
   source=folder/('regression_mpi'+str(n))/('default_native3d'+suffix)
   destination=folder/('mpi'+str(n))/source.name
   destination.write_bytes(source.read_bytes())
 run(['nice','-n','19',sys.executable,where/'audit_defaults.py',folder,folder],'audit_defaults')
 status='PASS'
finally:
 changed=[p for p,h in pins.items() if sha(root/p)!=h]
 archive_changes=[p for p,h in {**libs,**members}.items() if sha(Path(p))!=h]
 object_changes=[p for p,h in objects.items() if sha(build/p)!=h]
 if changed or archive_changes or object_changes:status='FAIL'
 result=dict(status=status,source_sha256=pins,changed_during_run=changed,archive_changes=archive_changes,object_changes=object_changes,library_sha256=libs,thin_archive_member_sha256=members,actual_test_object_sha256=objects,build_ninja_sha256=sha(build/'build.ninja'),stages=stages,
             scope='Actual configured Meson test objects and complete current SU2 archives, opt-in 3D factory and ReplaceMesh/reference notification, BDF2 affine states and original-P1 transported sensors, one-rank budget veto, SU2/CGNS output and independent strict audits, timer OFF/ON fixed-work controls. Native2D driver and MMG round-trip regressions. No CFD iterations or complete lifecycle/performance qualification.')
 if binary.exists():result['binary_sha256']=sha(binary)
 result['artifact_sha256']={str(p.relative_to(folder)):sha(p) for p in folder.rglob('*') if p.is_file() and p.suffix not in ('.o','.pyc') and p.name!='test_runtime'}
 (folder/'validation.json').write_text(json.dumps(result,indent=2)+'\n');print(status,folder,flush=True)
