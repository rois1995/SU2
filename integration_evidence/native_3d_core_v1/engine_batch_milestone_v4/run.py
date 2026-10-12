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
 old=BASE/'engine_batch_milestone_v2';master=json.loads((old/'validation.json').read_text())
 assert not master['source_drift'] and not master['object_drift'] and not master['library_drift']
 # Preserve the failed all-case receipt; these two individually complete driver cases are independently certified.
 references={}
 for name in ('n8_n1_m1_default512','n4_n4_m2_thin_strong_cgns'):
  source=old/name;reference=HERE/('reference_'+name);shutil.copytree(source,reference)
  audit=json.loads((reference/'independent_audit.json').read_text());assert audit['status']=='PASS'
  assert all(sha(old/n)==h for n,h in master['artifact_sha256'].items() if n.startswith(name+'/'))
  tools={n:master['source_sha256'][str((BASE/n).relative_to(ROOT))] for n in ('audit_envelope.py','audit_geometry_bridge.py','audit_coupled.py','audit_cavities.py','audit_lifecycle.py','audit_restart.py')}
  assert all(sha(BASE/n)==h for n,h in tools.items())
  receipt=dict(status='PASS',scope='Single actual v2 driver case: CPP and complete independent geometry/metric/state/output audit passed. Aggregate v2 remains FAIL from its separate batch auditor; no whole-suite promotion.',
     aggregate_validation_sha256=sha(old/'validation.json'),aggregate_status=master['status'],tools_sha256=tools,
     artifact_sha256={str(p.relative_to(reference)):sha(p) for p in reference.rglob('*') if p.is_file()})
  (reference/'validation.json').write_text(json.dumps(receipt,indent=2)+'\n');references[name]=reference
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
 # Actual production driver envelopes use the same complete configured binary, with no copied/relinked test executable.
 for name,p in [
  ('n8_n1_m1_default512',dict(ranks=1,workers=1,size=8,anisotropy=1,varying=1,direction=1,height_power=0,wedge=True,aligned_markers=True,diagnostic_rounds=512,repeat=0,detail=1,restart_format='none',candidate_batch=8)),
  ('n4_n4_m2_thin_strong_cgns',dict(ranks=4,workers=2,size=4,anisotropy=2,varying=1,direction=1,height_power=20,wedge=True,aligned_markers=True,diagnostic_rounds=512,repeat=0,detail=1,restart_format='cgns',candidate_batch=8))]:
  work=HERE/name;work.mkdir();print('CASE_FOLDER='+str(work),flush=True)
  (work/'parameters.json').write_text(json.dumps(p,indent=2)+'\n')
  settings=dict(size=p['size'],workers=p['workers'],anisotropy=p['anisotropy'],varying=1,direction=1,height_power=p['height_power'],wedge=1,aligned=1,rounds=512,repeat=0,detail=1,restart=2 if p['restart_format']=='cgns' else 0,batch=8)
  local=dict(env,**{'SU2_NATIVE3D_ENVELOPE_'+k.upper():str(v) for k,v in settings.items()})
  args=prefix+['mpirun','--oversubscribe','--bind-to','none','-np',str(p['ranks']),str(binary),'[NativeEnvelope3D]','--use-colour','no']
  row=command(args,work,'case',local,1800);stages.append(row)
  if row['exit_code'] or row.get('timeout'):raise RuntimeError(name+' failed; full driver evidence retained')
  assert len(re.findall(r'All tests passed \(\d+ assertions in 1 test case\)',(work/'case.stdout').read_text()))==p['ranks']
  run([sys.executable,BASE/'audit_query_routing_equivalence.py',work,references[name]],'audit_'+name,work)
  assert json.loads((work/'independent_audit.json').read_text())['status']=='PASS'
  run([sys.executable,BASE/'export_envelope_vtu.py',work],'vtu_'+name,work)
 for n in (1,2,4):
  work=HERE/('mpi'+str(n));work.mkdir();print('CASE_FOLDER='+str(work),flush=True)
  out=run(['mpirun','--oversubscribe','--bind-to','none','-np',n,binary,'[NativeMesh3D],[NativeDistributed3D],[NativeReference3D],[NativeBudget3D],[NativeDefault3D],[NativeBridge2D],[NativeBridge3D],[NativeRuntime3D]','--use-colour','no'],'mpi'+str(n),work)
  assert len(re.findall(r'All tests passed \(\d+ assertions in 104 test cases\)',out))==n
  run([sys.executable,BASE/'audit_geometry_bridge.py',work,n,HERE/('mpi'+str(n)+'.stdout')],'bridge_mpi'+str(n),work)
  certificate=work/'independent_audit.json';assert json.loads(certificate.read_text())['status']=='PASS'
  certificate.rename(work/'independent_bridge_audit.json')
  shutil.copyfile(HERE/('mpi'+str(n)+'.stdout'),work/'case.stdout')
  os.sched_setaffinity(0,{cpu})
  if os.nice(0)<19:os.nice(19-os.nice(0))
  stages.extend(distributed_audits(work,n,BASE,env,1800))
  assert json.loads((work/'distributed_audit.json').read_text())['status']=='PASS'
 lifecycle=HERE/'lifecycle_mpi1';lifecycle.mkdir();print('CASE_FOLDER='+str(lifecycle),flush=True)
 out=run(['mpirun','--bind-to','none','-np',1,binary,'[NativeLifecycle3D]','--use-colour','no'],'lifecycle',lifecycle)
 shutil.copyfile(HERE/'lifecycle.stdout',lifecycle/'case.stdout')
 run([sys.executable,BASE/'audit_lifecycle.py',lifecycle,1,lifecycle/'case.stdout'],'audit_lifecycle',lifecycle)
 assert json.loads((lifecycle/'independent_audit.json').read_text())['status']=='PASS'
 mmg=HERE/'mmg_serial';mmg.mkdir()
 print('CASE_FOLDER='+str(mmg),flush=True)
 out=run(['mpirun','--bind-to','none','-np',1,binary,'MMG interface: 2D round trip,MMG interface: 3D round trip','--use-colour','no'],'mmg_serial',mmg)
 assert 'All tests passed (48 assertions in 2 test cases)' in out
 assert sha(binary)==binary_pin;status='PASS'
except Exception as e:reason=str(e);print(reason,flush=True)
finally:
 drift=[n for n,h in pins.items() if sha(ROOT/n)!=h];objdrift=[n for n,h in objects.items() if sha(BUILD/n)!=h];libdrift=[n for n,h in {**libraries,**members}.items() if sha(Path(n))!=h]
 if drift or objdrift or libdrift:status='FAIL'
 r=dict(status=status,reason=reason,source_sha256=pins,source_drift=drift,object_drift=objdrift,library_drift=libdrift,actual_objects_sha256=objects,library_sha256=libraries,thin_archive_member_sha256=members,stages=stages,scope='Complete integrated bounded refinement/coarsening-batch milestone: actual full configured68-object production-archive driver including empty-CFD-rank rebuild fix; current default512 size8 trajectory, N4M2 rotated varying strong/thin CGNS-restart envelope,104mixed cases MPI1/2/4 with bridge/eight distributed audits; driver mesh/field/state certificates reused only after exact current artifact equality and fresh actual output/runtime checks, actual Hessian/CFD/BDF lifecycle and serial MMG. Several disjoint complete stars share lookup/fetch/reservation; each candidate preserves strict frozen-target validation and atomic publication. Epochs and actual edits are separate. No repeated timing/scaling/affordability or fullGoal1 completion claim.')
 if binary.exists():r['binary_sha256']=sha(binary)
 r['artifact_sha256']={str(p.relative_to(HERE)):sha(p) for p in HERE.rglob('*') if p.is_file() and p.name!='validation.json' and p.suffix!='.pyc'}
 (HERE/'validation.json').write_text(json.dumps(r,indent=2)+'\n');print(status,flush=True)
sys.exit(status!='PASS')
