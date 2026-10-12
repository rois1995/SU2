"""One sequential focused integrated-batch correctness check; retain failures and exact build pins."""
from pathlib import Path
import hashlib,json,os,re,shlex,shutil,subprocess,sys,time
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2];BASE=HERE.parent;BUILD=ROOT/'integration_evidence/build-integrated-v2'
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
assert not (HERE/'validation.json').exists()
prior=json.loads((BASE/'STATIC_ASSESSMENT_FIXES_VALIDATION.json').read_text())
pins={n:sha(ROOT/n) for n in prior['production_source_sha256']}
pins[str(Path(__file__).resolve().relative_to(ROOT))]=sha(Path(__file__).resolve())
for name in ('audit_geometry_bridge.py','audit_coupled.py','audit_cavities.py','audit_distributed.py','audit_donors.py','audit_anisotropic.py','audit_cycles.py','audit_repairs.py','audit_engine_repairs.py','audit_coplanar.py','audit_lifecycle.py'):
 pins[str((BASE/name).relative_to(ROOT))]=sha(BASE/name)
pins['integration_evidence/native_cluster_3d_core_v1/run.py']=sha(ROOT/'integration_evidence/native_cluster_3d_core_v1/run.py')
assert int(next(l.split()[1] for l in Path('/proc/meminfo').read_text().splitlines() if l.startswith('MemAvailable:')))*1024>=3*1024**3
assert shutil.disk_usage(HERE).free>=3*1024**3
(HERE/'host.txt').write_text(subprocess.check_output(['uptime'],text=True)+subprocess.check_output(['free','-m'],text=True)+'\n'.join(subprocess.check_output(['ps','-eo','pid,ppid,ni,pcpu,pmem,comm','--sort=-pcpu'],text=True).splitlines()[:15])+'\n')
def ticks():return {l.split()[0]:list(map(int,l.split()[1:])) for l in Path('/proc/stat').read_text().splitlines() if l.startswith('cpu')}
a=ticks();time.sleep(1);b=ticks();idle={}
for cpu in os.sched_getaffinity(0):
 d=[y-x for x,y in zip(a['cpu'+str(cpu)],b['cpu'+str(cpu)])];idle[cpu]=(d[3]+d[4])/sum(d) if sum(d) else 0
cpu=max(idle,key=idle.get);prefix=['taskset','-c',str(cpu),'nice','-n','19']
(HERE/'host_gate.json').write_text(json.dumps(dict(cpu=cpu,idle=idle,nice=19,maximum_compute_cpus=1,threads=1,scope='User-authorized contended correctness only, no performance campaign'),indent=2)+'\n')
env=dict(os.environ,OMP_NUM_THREADS='1',OPENBLAS_NUM_THREADS='1',MKL_NUM_THREADS='1',NUMEXPR_NUM_THREADS='1',PYTHONDONTWRITEBYTECODE='1',OMPI_MCA_mpi_yield_when_idle='1')
sys.path.insert(0,str(ROOT/'integration_evidence/native_cluster_3d_core_v1'));from run import command,distributed_audits
stages=[];status='FAIL';reason='';objects={};libraries={};members={};binary=BUILD/'UnitTests/test_driver'
def run(args,label,cwd,timeout=1800):
 print(label,flush=True);row=command(prefix+list(map(str,args)),cwd,label,env,timeout);stages.append(row)
 if cwd!=HERE:
  for suffix in ('stdout','stderr'):shutil.move(cwd/(label+'.'+suffix),HERE/(label+'.'+suffix))
 if row['exit_code'] or row.get('timeout'):raise RuntimeError(label+' failed; logs retained')
 return (HERE/(label+'.stdout')).read_text()
try:
 run(['/tmp/native-build-tools/ninja','-C',BUILD,'-j1','UnitTests/test_driver'],'empty_connectivity_focus_build',ROOT)
 lines=(BUILD/'build.ninja').read_text().splitlines();link=next(l for l in lines if l.startswith('build UnitTests/test_driver: cpp_LINKER '))
 objects={n:sha(BUILD/n) for n in shlex.split(link.split('cpp_LINKER ',1)[1].split(' | ',1)[0]) if n.endswith('.o')};assert len(objects)==68
 args=shlex.split(next(l.split('=',1)[1] for l in lines[lines.index(link)+1:] if l.startswith(' LINK_ARGS =')))
 for n in args:
  if not n.endswith('.a'):continue
  p=Path(n);p=p if p.is_absolute() else BUILD/p;libraries[str(p)]=sha(p)
  with p.open('rb') as f:thin=f.read(8)==b'!<thin>\n'
  if thin:
   for n in subprocess.check_output(['ar','t',p],cwd=ROOT,text=True).splitlines():
    q=Path(n);q=q if q.is_absolute() else ROOT/q;members[str(q)]=sha(q)
 binary_pin=sha(binary)
 for n in (1,2,4):
  work=HERE/('mpi'+str(n));work.mkdir();print('CASE_FOLDER='+str(work),flush=True)
  out=run(['mpirun','--oversubscribe','--bind-to','none','-np',n,binary,'[NativeBridge3D]','--use-colour','no'],'mpi'+str(n),work)
  assert len(re.findall(r'All tests passed \(\d+ assertions in 3 test cases\)',out))==n
 assert sha(binary)==binary_pin;status='PASS'
except Exception as e:reason=str(e);print(reason,flush=True)
finally:
 drift=[n for n,h in pins.items() if sha(ROOT/n)!=h];objdrift=[n for n,h in objects.items() if sha(BUILD/n)!=h];libdrift=[n for n,h in {**libraries,**members}.items() if sha(Path(n))!=h]
 if drift or objdrift or libdrift:status='FAIL'
 r=dict(status=status,reason=reason,source_sha256=pins,source_drift=drift,object_drift=objdrift,library_drift=libdrift,actual_objects_sha256=objects,library_sha256=libraries,thin_archive_member_sha256=members,stages=stages,scope='Focused actual CFD bridge and empty-connectivity source checks on sequential MPI1/2/4; no repeated driver or performance campaign.')
 if binary.exists():r['binary_sha256']=sha(binary)
 r['artifact_sha256']={str(p.relative_to(HERE)):sha(p) for p in HERE.rglob('*') if p.is_file() and p.name!='validation.json' and p.suffix!='.pyc'}
 (HERE/'validation.json').write_text(json.dumps(r,indent=2)+'\n');print(status,flush=True)
sys.exit(status!='PASS')
