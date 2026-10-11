"""Complete actual-archive runtime regressions without repeating already-passing 3D work."""
from pathlib import Path
import hashlib,json,os,shlex,subprocess,sys,time
root=Path(__file__).resolve().parents[2];where=Path(__file__).resolve().parent
folder=Path(sys.argv[1]).resolve();base=Path(sys.argv[2]).resolve();folder.mkdir(exist_ok=False)
print('Case working and retained folder:',folder,flush=True)
receipt=json.loads((base/'validation.json').read_text());assert receipt['status']=='FAIL' and not receipt['changed_during_run'] and not receipt['archive_changes'] and not receipt['object_changes']
assert receipt['stages'][-1]['stage']=='regression_mpi2'
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
# Only the orchestration changed after that failed serial-MMG test selection.
pins=receipt['source_sha256'].copy();pins.pop('integration_evidence/native_3d_core_v1/run_runtime.py');assert all(sha(root/p)==h for p,h in pins.items())
for n in (1,2,4):assert json.loads((base/('mpi'+str(n))/'independent_audit.json').read_text())['status']=='PASS'
libs=receipt['library_sha256'];members=receipt['thin_archive_member_sha256'];assert all(sha(Path(p))==h for p,h in {**libs,**members}.items())
pins['integration_evidence/native_3d_core_v1/run_runtime_regression.py']=sha(Path(__file__).resolve())
(folder/'runner_snapshot.py').write_bytes(Path(__file__).read_bytes())
build=root/'integration_evidence/build-integrated-v2';lines=(build/'build.ninja').read_text().splitlines();i=next(i for i,l in enumerate(lines) if l.startswith('build UnitTests/test_driver:'))
args=shlex.split(next(l.split('=',1)[1].strip() for l in lines[i+1:i+8] if l.strip().startswith('LINK_ARGS =')))
args=[str(build/p) if p.endswith('.a') and not p.startswith('/') else p for p in args if not p.startswith('-Wl,-rpath')]
targets=list(receipt['actual_test_object_sha256'])+['UnitTests/test_driver.p/SU2_CFD_adaptation_AdaptationMPI_tests.cpp.o']
env=dict(os.environ,PATH='/tmp/native-build-tools:'+os.environ['PATH'],OMP_NUM_THREADS='1',OPENBLAS_NUM_THREADS='1',MKL_NUM_THREADS='1',OMPI_MCA_mpi_yield_when_idle='1',OMPI_MCA_hwloc_base_binding_policy='none',PYTHONDONTWRITEBYTECODE='1')
status='FAIL';stages=[];objects={};binary=folder/'test_runtime_regression'
def run(command,label,cwd=root):
 print('Starting',label,flush=True);command=['timeout','--signal=TERM','--kill-after=10s','900s']+list(map(str,command));started=time.monotonic();p=subprocess.run(command,cwd=cwd,env=env,capture_output=True,text=True)
 (folder/(label+'.stdout')).write_text(p.stdout);(folder/(label+'.stderr')).write_text(p.stderr);stages.append(dict(stage=label,command=command,cwd=str(cwd),seconds=time.monotonic()-started,exit_code=p.returncode))
 if p.returncode:raise RuntimeError(label+' failed')
 return p.stdout
try:
 for cmd,label in [(['uptime'],'host_load'),(['free','-m'],'host_memory'),(['ps','-eo','pid,ppid,ni,pcpu,etime,comm','--sort=-pcpu'],'host_processes')]:run(cmd,label)
 run(['nice','-n','19','/tmp/native-build-tools/ninja','-C',build,'-j1']+targets,'compile_actual_meson_tests')
 objects={p:sha(build/p) for p in targets}
 run(['nice','-n','19','mpicxx','-pthread']+[build/p for p in targets]+args+['-o',binary],'link_actual_archives')
 for n in (1,2,4):
  work=folder/('mpi'+str(n));work.mkdir();print('Case working and retained folder:',work,flush=True)
  run(['uptime'],'before_mpi'+str(n)+'_load');run(['free','-m'],'before_mpi'+str(n)+'_memory')
  out=run(['nice','-n','19','mpirun','--oversubscribe','--bind-to','none','-np',str(n),binary,'[NativeRemesher],MPI adaptation: extracted and remeshed mesh independent of the ranks','--use-colour','no'],'mpi'+str(n),work)
  assert out.count('All tests passed')==n
 status='PASS'
finally:
 changed=[p for p,h in pins.items() if sha(root/p)!=h];archive_changes=[p for p,h in {**libs,**members}.items() if sha(Path(p))!=h];object_changes=[p for p,h in objects.items() if sha(build/p)!=h]
 if changed or archive_changes or object_changes:status='FAIL'
 result=dict(status=status,base_3d_receipt=str(base/'validation.json'),base_3d_receipt_sha256=sha(base/'validation.json'),base_3d_audit_sha256={str(base/('mpi'+str(n))/'independent_audit.json'):sha(base/('mpi'+str(n))/'independent_audit.json') for n in (1,2,4)},source_sha256=pins,changed_during_run=changed,archive_changes=archive_changes,object_changes=object_changes,library_sha256=libs,thin_archive_member_sha256=members,actual_test_object_sha256=objects,stages=stages,
             scope='Complete unchanged actual native2D driver repeated-remesh/affine state checks and existing MPI-aware MMG serial-versus-distributed gathered/remeshed bitwise comparisons in 2D/3D. Passing 3D runtime evidence is linked from the unchanged source/library baseline; original combined receipt remains FAIL.')
 if binary.exists():result['binary_sha256']=sha(binary)
 result['artifact_sha256']={str(p.relative_to(folder)):sha(p) for p in folder.rglob('*') if p.is_file() and p.suffix not in ('.o','.pyc') and p.name!='test_runtime_regression'}
 (folder/'validation.json').write_text(json.dumps(result,indent=2)+'\n');print(status,folder,flush=True)
