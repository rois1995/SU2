"""Sequential current-source static-fix qualification on the real configured driver."""
from pathlib import Path
import hashlib,json,os,re,shlex,subprocess,sys,time
HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[2]
BASE=HERE.parent
BUILD=ROOT/'integration_evidence/build-integrated-v2'
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
assert not (HERE/'validation.json').exists(), 'Keep terminal receipts immutable'
prior=json.loads((BASE/'completion_diagnostic_build_v1/validation.json').read_text())
pins={name:sha(ROOT/name) for name in prior['source_sha256']}
pins[str(Path(__file__).resolve().relative_to(ROOT))]=sha(Path(__file__).resolve())
env=dict(os.environ,OMP_NUM_THREADS='1',OPENBLAS_NUM_THREADS='1',MKL_NUM_THREADS='1',NUMEXPR_NUM_THREADS='1',PYTHONDONTWRITEBYTECODE='1',OMPI_MCA_mpi_yield_when_idle='1')
for k in tuple(env):
 if k.startswith('SU2_NATIVE3D_ENVELOPE_'):del env[k]
assert int(next(l.split()[1] for l in Path('/proc/meminfo').read_text().splitlines() if l.startswith('MemAvailable:')))*1024>=3*1024**3
import shutil
assert shutil.disk_usage(HERE).free>=3*1024**3
(HERE/'host.txt').write_text(subprocess.check_output(['uptime'],text=True)+subprocess.check_output(['free','-m'],text=True)+'\n'.join(subprocess.check_output(['ps','-eo','pid,ppid,ni,pcpu,pmem,comm','--sort=-pcpu'],text=True).splitlines()[:15])+'\n')
def ticks():return {l.split()[0]:list(map(int,l.split()[1:])) for l in Path('/proc/stat').read_text().splitlines() if l.startswith('cpu')}
a=ticks();time.sleep(1);b=ticks();idle={}
for cpu in os.sched_getaffinity(0):
 d=[y-x for x,y in zip(a['cpu'+str(cpu)],b['cpu'+str(cpu)])];idle[cpu]=(d[3]+d[4])/sum(d) if sum(d) else 0
cpu=max(idle,key=idle.get);prefix=['taskset','-c',str(cpu),'nice','-n','19']
(HERE/'host_gate.json').write_text(json.dumps(dict(cpu=cpu,idle=idle,nice=19,maximum_compute_cpus=1,threads=1,scope='Contention-authorized correctness only; no timing/scaling claim'),indent=2)+'\n')
sys.path.insert(0,str(ROOT/'integration_evidence/native_cluster_3d_core_v1'))
from run import command
stages=[];status='FAIL';reason='';objects={};libraries={};members={};binary=BUILD/'UnitTests/test_driver'
def run(args,label,cwd,timeout=900):
 print(label,flush=True)
 row=command(prefix+list(map(str,args)),cwd,label,env,timeout);stages.append(row)
 if cwd!=HERE:
  for suffix in ('stdout','stderr'):shutil.move(cwd/(label+'.'+suffix),HERE/(label+'.'+suffix))
 if row['exit_code'] or row.get('timeout'):raise RuntimeError(label+' failed; logs retained')
 return (HERE/(label+'.stdout')).read_text()
try:
 run(['/tmp/native-build-tools/ninja','-C',BUILD,'-j1','UnitTests/test_driver'],'build',ROOT,1800)
 lines=(BUILD/'build.ninja').read_text().splitlines()
 link=next(l for l in lines if l.startswith('build UnitTests/test_driver: cpp_LINKER '))
 for name in shlex.split(link.split('cpp_LINKER ',1)[1].split(' | ',1)[0]):
  if name.endswith('.o'):objects[name]=sha(BUILD/name)
 assert len(objects)==68
 linkargs=shlex.split(next(l.split('=',1)[1] for l in lines[lines.index(link)+1:] if l.startswith(' LINK_ARGS =')))
 for name in linkargs:
  if not name.endswith('.a'):continue
  p=Path(name);p=p if p.is_absolute() else BUILD/p
  libraries[str(p)]=sha(p)
  with p.open('rb') as f:thin=f.read(8)==b'!<thin>\n'
  if thin:
   for name in subprocess.check_output(['ar','t',p],cwd=ROOT,text=True).splitlines():
    q=Path(name);q=q if q.is_absolute() else ROOT/q;members[str(q)]=sha(q)
 binary_pin=sha(binary)
 selection='[NativeMesh3D],[NativeDistributed3D],[NativeReference3D],[NativeBudget3D],[NativeDefault3D],[NativeBridge2D],[NativeBridge3D],[NativeRuntime3D]'
 for n in (1,2,4):
  work=HERE/('mpi'+str(n));work.mkdir()
  print('CASE_FOLDER='+str(work),flush=True)
  out=run(['mpirun','--oversubscribe','--bind-to','none','-np',n,binary,selection,'--use-colour','no'],'mpi'+str(n),work,1800)
  assert 'No test cases matched' not in out
  assert len(re.findall(r'All tests passed \(\d+ assertions in \d+ test cases\)',out))==n
  for tool in ('audit_coupled.py','audit_cavities.py','audit_geometry_bridge.py','audit_runtime.py'):
   run([sys.executable,BASE/tool,work],tool.removesuffix('.py')+'_mpi'+str(n),work,1800)
 mmg=HERE/'mmg_serial';mmg.mkdir()
 out=run(['mpirun','--bind-to','none','-np',1,binary,'MMG interface: 2D round trip,MMG interface: 3D round trip','--use-colour','no'],'mmg_serial',mmg)
 assert 'All tests passed' in out
 assert sha(binary)==binary_pin
 status='PASS'
except Exception as e:reason=str(e);print(reason,flush=True)
finally:
 drift=[name for name,digest in pins.items() if sha(ROOT/name)!=digest]
 objdrift=[name for name,digest in objects.items() if sha(BUILD/name)!=digest]
 libdrift=[name for name,digest in {**libraries,**members}.items() if sha(Path(name))!=digest]
 if drift or objdrift or libdrift:status='FAIL'
 record=dict(status=status,reason=reason,source_sha256=pins,source_drift=drift,object_drift=objdrift,library_drift=libdrift,actual_objects_sha256=objects,library_sha256=libraries,thin_archive_member_sha256=members,stages=stages,scope='Current configured full68-object driver; static-fix component/MPI1/2/4 and real bridge/default/rejection/runtime checks plus fresh independent audits and serial MMG controls. Local correctness only, no performance/scaling/high-rank or fullGoal1 claim.')
 if binary.exists():record['binary_sha256']=sha(binary)
 record['artifact_sha256']={str(p.relative_to(HERE)):sha(p) for p in HERE.rglob('*') if p.is_file() and p.name!='validation.json' and p.suffix!='.pyc'}
 (HERE/'validation.json').write_text(json.dumps(record,indent=2)+'\n');print(status,flush=True)
sys.exit(status!='PASS')
