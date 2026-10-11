"""Sequential actual configured full-driver regressions for native3D recovery coordination."""
from pathlib import Path
import hashlib,json,os,subprocess,sys,time
root=Path(__file__).resolve().parents[2];where=Path(__file__).resolve().parent
folder=Path(sys.argv[1]).resolve();folder.mkdir(exist_ok=False)
print('Case working and retained folder:',folder,flush=True)
build=root/'integration_evidence/build-integrated-v2';sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
paths=subprocess.check_output(['git','ls-files','Common','SU2_CFD','UnitTests','meson.build','meson_options.txt'],cwd=root,text=True).splitlines()
paths.append(str(Path(__file__).resolve().relative_to(root)))
pins={p:sha(root/p) for p in paths if (root/p).is_file() and (p.endswith(('.cpp','.hpp','.h','.txt','.py')) or p.endswith('meson.build'))}
changed=subprocess.check_output(['git','diff','--name-only','--ignore-submodules=all'],cwd=root,text=True).splitlines()
for p in changed+[str(Path(__file__).resolve().relative_to(root))]:
 if (root/p).is_file():out=folder/'source_snapshots'/p;out.parent.mkdir(parents=True,exist_ok=True);out.write_bytes((root/p).read_bytes())
env=dict(os.environ,OMP_NUM_THREADS='1',OPENBLAS_NUM_THREADS='1',MKL_NUM_THREADS='1',OMPI_MCA_mpi_yield_when_idle='1',PYTHONDONTWRITEBYTECODE='1')
status='FAIL';stages=[];objects={};libraries={};members={}
def run(command,label,cwd=folder):
 print('Starting',label,flush=True);command=['timeout','--signal=TERM','--kill-after=10s',('1800s' if label=='compile_full_configured_driver' else '900s')]+list(map(str,command));start=time.monotonic()
 p=subprocess.run(command,cwd=cwd,env=env,capture_output=True,text=True)
 (folder/(label+'.stdout')).write_text(p.stdout);(folder/(label+'.stderr')).write_text(p.stderr)
 stages.append(dict(stage=label,command=command,cwd=str(cwd),seconds=time.monotonic()-start,exit_code=p.returncode))
 assert p.returncode==0,label+' failed in '+str(folder)
 return p.stdout
try:
 for cmd,label in [(['uptime'],'host_load'),(['free','-m'],'host_memory'),(['ps','-eo','pid,ni,pcpu,comm','--sort=-pcpu'],'host_processes')]:run(cmd,label)
 run(['nice','-n','19','/tmp/native-build-tools/ninja','-C',build,'-j1','UnitTests/test_driver'],'compile_full_configured_driver',root)
 line=next(l for l in (build/'build.ninja').read_text().splitlines() if l.startswith('build UnitTests/test_driver:'))
 objects={p:sha(build/p) for p in line.split() if p.endswith('.o')}
 for p in line.split():
  if not p.endswith('.a'):continue
  path=Path(p) if p.startswith('/') else build/p
  libraries[str(path)]=sha(path)
  with path.open('rb') as f:thin=f.read(8)==b'!<thin>\n'
  if thin:
   for member in subprocess.check_output(['ar','t',path],cwd=root,text=True).splitlines():
    q=Path(member);q=q if q.is_absolute() else root/q;members[str(q)]=sha(q)
 binary=build/'UnitTests/test_driver';binary_pin=sha(binary)
 # ExtractMesh gathers on rank0; legacy direct MMG round-trip fixtures are serial.
 mmg=folder/'mmg_serial';mmg.mkdir()
 out=run(['nice','-n','19','mpirun','--bind-to','none','-np','1',binary,
          'MMG interface: 2D round trip,MMG interface: 3D round trip','--use-colour','no'],'mmg_serial',mmg)
 assert 'No test cases matched' not in out and out.count('All tests passed')==1
 selection='[NativeMesh3D],[NativeDonorReuse3D],[NativeCollapseFilter3D],[NativeCoplanar3D],[NativeInsertion3D],[NativeAutomaticRepair3D],[NativeBudget3D],[NativeDefault3D],[NativeBridge2D],[NativeBridge3D]'
 for ranks in (1,2,4):
  work=folder/f'mpi{ranks}';work.mkdir()
  run(['uptime'],f'before_mpi{ranks}_load')
  out=run(['nice','-n','19','mpirun','--oversubscribe','--bind-to','none','-np',ranks,binary,selection,'--use-colour','no'],f'mpi{ranks}',work)
  assert 'No test cases matched' not in out, 'A regression selector matched no tests'
  assert out.count('All tests passed')==ranks
 assert sha(binary)==binary_pin
 status='PASS'
finally:
 drift=[p for p,h in pins.items() if sha(root/p)!=h]
 object_drift=[p for p,h in objects.items() if sha(build/p)!=h]
 archive_drift=[p for p,h in {**libraries,**members}.items() if sha(Path(p))!=h]
 if drift or object_drift or archive_drift:status='FAIL'
 result=dict(status=status,source_sha256=pins,source_drift=drift,object_drift=object_drift,archive_drift=archive_drift,
             actual_objects_sha256=objects,library_sha256=libraries,thin_archive_member_sha256=members,build_ninja_sha256=sha(build/'build.ninja'),stages=stages,
             scope='Configured complete68-object UnitTests/test_driver, actual production archives, native3D union donor reuse/invalid duplicate and all mesh arithmetic/segment/tensor/P1/gap/overlap/thin controls, exact immutable-facet coarsening filter, removable coplanar seams, certified interior/coupled planar insertion with thin/junction/incomplete-star/rollback controls, automatic Repair/Refine/Coarsen <=1recovery/strict-final/atomic input preservation, budget/refusal/retry/default rejection and native2D/native3D output/transfer bridge controls at MPI1/2/4; actual MMG 2D/3D round trips serially (legacy fixtures call master-only MMG). Test assertions, not an independent physical-flow/scaling or fullGoal1 certificate.')
 if 'binary_pin' in locals():result['binary_sha256']=binary_pin
 result['artifact_sha256']={str(p.relative_to(folder)):sha(p) for p in folder.rglob('*') if p.is_file() and p.suffix not in ('.o','.pyc')}
 (folder/'validation.json').write_text(json.dumps(result,indent=2)+'\n');print(status,folder,flush=True)
