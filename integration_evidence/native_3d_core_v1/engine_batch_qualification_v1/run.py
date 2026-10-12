"""One sequential integrated-engine milestone qualification using the complete configured production driver."""
from pathlib import Path
import hashlib,json,os,re,shlex,shutil,subprocess,sys,time
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2];BASE=HERE.parent;BUILD=ROOT/'integration_evidence/build-integrated-v2'
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
assert not (HERE/'validation.json').exists()
prior=json.loads((BASE/'STATIC_ASSESSMENT_FIXES_VALIDATION.json').read_text())
pins={n:sha(ROOT/n) for n in prior['production_source_sha256']}
pins[str(Path(__file__).resolve().relative_to(ROOT))]=sha(Path(__file__).resolve())
for name in ('audit_geometry_bridge.py','audit_coupled.py','audit_cavities.py','audit_distributed.py','audit_donors.py','audit_anisotropic.py','audit_cycles.py','audit_repairs.py','audit_engine_repairs.py','audit_coplanar.py','audit_lifecycle.py','audit_batch_engine.py','audit_envelope.py','audit_restart.py','export_envelope_vtu.py','audit_query_routing_equivalence.py'):
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
 focused=json.loads((BASE/'empty_connectivity_focus_v1/validation.json').read_text())
 assert focused['status']=='PASS' and not focused['source_drift'] and not focused['object_drift'] and not focused['library_drift']
 assert all(sha(ROOT/n)==h for n,h in focused['source_sha256'].items() if n.startswith(('Common/','SU2_CFD/','UnitTests/')))
 assert sha(binary)==focused['binary_sha256']
 completed=BASE/'engine_batch_milestone_v5';master=json.loads((completed/'validation.json').read_text())
 assert master['status']=='FAIL' and master['reason'].endswith("/lifecycle_mpi1/independent_audit.json'")
 assert not master['source_drift'] and not master['object_drift'] and not master['library_drift']
 assert all(sha(ROOT/n)==h for n,h in master['source_sha256'].items())
 assert all(sha(completed/n)==h for n,h in master['artifact_sha256'].items())
 assert all(s['exit_code']==0 for s in master['stages'])
 assert sha(binary)==master['binary_sha256']
 cases=json.loads((completed/'reused_driver_cases.json').read_text());assert cases['status']=='PASS'
 for case in cases['cases']:
  work=ROOT/case['folder'];assert sha(work/'independent_audit.json')==case['certificate_sha256']
  assert json.loads((work/'independent_audit.json').read_text())['status']=='PASS'
  assert all(sha(work/n)==h for n,h in case['numerical_artifacts_sha256'].items())
 for n in (1,2,4):
  raw=(completed/('mpi'+str(n)+'.stdout')).read_text()
  assert len(re.findall(r'All tests passed \(\d+ assertions in 104 test cases\)',raw))==n
  work=completed/('mpi'+str(n))
  assert json.loads((work/'independent_bridge_audit.json').read_text())['status']=='PASS'
  assert json.loads((work/'distributed_audit.json').read_text())['status']=='PASS'
  assert len([s for s in master['stages'] if s['label']=='audit_batch_engine'])==3
 lifecycle=completed/'lifecycle_mpi1/lifecycle_audit.json';life=json.loads(lifecycle.read_text())
 assert life['status']=='PASS' and len(life['frames'])==6
 (HERE/'reused_regression.json').write_text(json.dumps(dict(status='PASS',original_receipt_sha256=sha(completed/'validation.json'),
   original_status=master['status'],original_collector_failure=master['reason'],verified_original_artifact_count=len(master['artifact_sha256']),
   mixed_cases_per_rank=104,ranks=[1,2,4],completed_stages=master['stages'],reused_driver_cases=cases,
   lifecycle_audit_sha256=sha(lifecycle),lifecycle_frames=len(life['frames']),
   scope='Every current CPP/auditor stage exited0. Collector-only expected lifecycle certificate basename was incorrect; actual lifecycle_audit.json is PASS. No numerical rerun or promotion of the original failed receipt.'),indent=2)+'\n')
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
 assert objects==focused['actual_objects_sha256']
 assert libraries==focused['library_sha256'] and members==focused['thin_archive_member_sha256']
 binary_pin=sha(binary)
 mmg=HERE/'mmg_serial';mmg.mkdir()
 print('CASE_FOLDER='+str(mmg),flush=True)
 out=run(['mpirun','--bind-to','none','-np',1,binary,'MMG interface: 2D round trip,MMG interface: 3D round trip','--use-colour','no'],'mmg_serial',mmg)
 assert 'All tests passed (48 assertions in 2 test cases)' in out
 assert sha(binary)==binary_pin;status='PASS'
except Exception as e:reason=str(e);print(reason,flush=True)
finally:
 drift=[n for n,h in pins.items() if sha(ROOT/n)!=h];objdrift=[n for n,h in objects.items() if sha(BUILD/n)!=h];libdrift=[n for n,h in {**libraries,**members}.items() if sha(Path(n))!=h]
 if drift or objdrift or libdrift:status='FAIL'
 r=dict(status=status,reason=reason,source_sha256=pins,source_drift=drift,object_drift=objdrift,library_drift=libdrift,actual_objects_sha256=objects,library_sha256=libraries,thin_archive_member_sha256=members,stages=stages,scope='Complete integrated bounded refinement/coarsening-batch milestone with current full104case MPI1/2/4 and independent/lifecycle results reused only after complete artifact and source/binary/object/archive pin verification: actual full configured68-object production-archive driver including empty-CFD-rank rebuild fix; current default512 size8 trajectory, N4M2 rotated varying strong/thin CGNS-restart envelope,104mixed cases MPI1/2/4 with bridge/eight distributed audits; completed current driver outputs reused with verified artifact/source/binary pins; N8 exact equivalence and fresh actual output/runtime checks, N4 fresh full geometry/metric/current-BDF/CGNS audit, actual Hessian/CFD/BDF lifecycle and serial MMG. Several disjoint complete stars share lookup/fetch/reservation; each candidate preserves strict frozen-target validation and atomic publication. Epochs and actual edits are separate. No repeated timing/scaling/affordability or fullGoal1 completion claim.')
 if binary.exists():r['binary_sha256']=sha(binary)
 r['artifact_sha256']={str(p.relative_to(HERE)):sha(p) for p in HERE.rglob('*') if p.is_file() and p.name!='validation.json' and p.suffix!='.pyc'}
 (HERE/'validation.json').write_text(json.dumps(r,indent=2)+'\n');print(status,flush=True)
sys.exit(status!='PASS')
