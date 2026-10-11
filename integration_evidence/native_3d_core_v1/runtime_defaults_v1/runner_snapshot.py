from pathlib import Path
import hashlib,json,os,shlex,subprocess,time
root=Path('/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated');where=root/'integration_evidence/native_3d_core_v1';base=where/'runtime_regression_v1';folder=where/'runtime_defaults_v1';build=root/'integration_evidence/build-integrated-v2'
folder.mkdir(exist_ok=False);print('Case working and retained folder:',folder,flush=True);(folder/'runner_snapshot.py').write_bytes(Path(__file__).read_bytes())
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
receipt=json.loads((base/'validation.json').read_text());assert receipt['status']=='PASS';pins=receipt['source_sha256'].copy();pins.pop('UnitTests/meson.build')
assert all(sha(root/p)==h for p,h in pins.items())
for p in ['UnitTests/meson.build','UnitTests/SU2_CFD/adaptation/CNativeRemesher3D_tests.cpp']:
 pins[p]=sha(root/p);q=folder/'source_snapshots'/p;q.parent.mkdir(parents=True,exist_ok=True);q.write_bytes((root/p).read_bytes())
libs=receipt['library_sha256'];members=receipt['thin_archive_member_sha256'];assert all(sha(Path(p))==h for p,h in {**libs,**members}.items())
objects={};status='FAIL';stages=[];binary=folder/'test_runtime_defaults'
env=dict(os.environ,PATH='/tmp/native-build-tools:'+os.environ['PATH'],OMP_NUM_THREADS='1',OPENBLAS_NUM_THREADS='1',MKL_NUM_THREADS='1',OMPI_MCA_mpi_yield_when_idle='1',OMPI_MCA_hwloc_base_binding_policy='none',PYTHONDONTWRITEBYTECODE='1')
def run(command,label,cwd=root):
 command=['timeout','--signal=TERM','--kill-after=10s','900s']+list(map(str,command));print('Starting',label,flush=True);start=time.monotonic();p=subprocess.run(command,cwd=cwd,env=env,capture_output=True,text=True)
 (folder/(label+'.stdout')).write_text(p.stdout);(folder/(label+'.stderr')).write_text(p.stderr);stages.append(dict(stage=label,command=command,cwd=str(cwd),seconds=time.monotonic()-start,exit_code=p.returncode))
 if p.returncode:raise RuntimeError(label+' failed')
 return p.stdout
try:
 run(['uptime'],'host_load');run(['free','-m'],'host_memory');run(['ps','-eo','pid,ppid,ni,pcpu,etime,comm','--sort=-pcpu'],'host_processes')
 targets=['UnitTests/test_driver.p/SU2_CFD_adaptation_CNativeRemesher3D_tests.cpp.o','UnitTests/test_driver.p/test_driver.cpp.o']
 run(['nice','-n','19','/tmp/native-build-tools/ninja','-C',build,'-j1']+targets,'compile_actual_meson_tests')
 objects={p:sha(build/p) for p in targets};lines=(build/'build.ninja').read_text().splitlines();i=next(i for i,l in enumerate(lines) if l.startswith('build UnitTests/test_driver:'))
 args=shlex.split(next(l.split('=',1)[1].strip() for l in lines[i+1:i+8] if l.strip().startswith('LINK_ARGS =')))
 args=[str(build/p) if p.endswith('.a') and not p.startswith('/') else p for p in args if not p.startswith('-Wl,-rpath')]
 run(['nice','-n','19','mpicxx','-pthread']+[build/p for p in targets]+args+['-o',binary],'link_actual_archives')
 for n in (1,2,4):
  work=folder/('mpi'+str(n));work.mkdir();print('Case working and retained folder:',work,flush=True);run(['uptime'],'before_mpi'+str(n)+'_load')
  out=run(['nice','-n','19','mpirun','--oversubscribe','--bind-to','none','-np',str(n),binary,'[NativeDefault3D]','--use-colour','no'],'mpi'+str(n),work);assert out.count('All tests passed')==n
 status='PASS'
finally:
 changed=[p for p,h in pins.items() if sha(root/p)!=h];archive_changes=[p for p,h in {**libs,**members}.items() if sha(Path(p))!=h]
 if changed or archive_changes:status='FAIL'
 result=dict(status=status,source_sha256=pins,changed_during_run=changed,archive_changes=archive_changes,library_sha256=libs,thin_archive_member_sha256=members,actual_test_object_sha256=objects,stages=stages,scope='Same production libraries as passing runtime/regression; adds actual-Meson tests of default N-worker CFD ownership without native repartitioning, rejection after private accepted edits, unchanged accepted geometry/solution/sensor and reference, retry, reader-return transport and idempotent publication/sidecar. No actual driver replacement or full lifecycle/performance claim added.')
 if binary.exists():result['binary_sha256']=sha(binary)
 result['artifact_sha256']={str(p.relative_to(folder)):sha(p) for p in folder.rglob('*') if p.is_file() and p.suffix not in ('.o','.pyc') and p.name!='test_runtime_defaults'}
 (folder/'validation.json').write_text(json.dumps(result,indent=2)+'\n');print(status,folder,flush=True)
