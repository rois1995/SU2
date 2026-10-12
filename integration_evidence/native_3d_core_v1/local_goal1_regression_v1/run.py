from pathlib import Path
import hashlib,json,os,subprocess,time,re,shlex,sys
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2];BUILD=ROOT/'integration_evidence/build-integrated-v2'
def sha(p):
 h=hashlib.sha256()
 with p.open('rb') as f:
  for b in iter(lambda:f.read(1048576),b''):h.update(b)
 return h.hexdigest()
pins={str(p.relative_to(ROOT)):sha(p) for d in ('Common','SU2_CFD','UnitTests') for p in (ROOT/d).rglob('*') if p.is_file() and p.suffix in ('.cpp','.hpp','.h','.inl','.tpp')}
pins.update({str(p.relative_to(ROOT)):sha(p) for p in (HERE.parent).glob('audit_*.py')});pins['config_template.cfg']=sha(ROOT/'config_template.cfg');pins[str(Path(__file__).resolve().relative_to(ROOT))]=sha(Path(__file__).resolve())
assert not (HERE/'validation.json').exists()
assert int(next(l.split()[1] for l in Path('/proc/meminfo').read_text().splitlines() if l.startswith('MemAvailable:')))*1024>=3*1024**3
assert __import__('shutil').disk_usage(HERE).free>=3*1024**3
(HERE/'host.txt').write_text(subprocess.check_output(['uptime'],text=True)+subprocess.check_output(['free','-m'],text=True)+'\n'.join(subprocess.check_output(['ps','-eo','pid,ppid,ni,pcpu,pmem,comm','--sort=-pcpu'],text=True).splitlines()[:15])+'\n')
def ticks():return {l.split()[0]:list(map(int,l.split()[1:])) for l in Path('/proc/stat').read_text().splitlines() if l.startswith('cpu')}
a=ticks();time.sleep(1);b=ticks();idle={}
for cpu in os.sched_getaffinity(0):
 d=[y-x for x,y in zip(a['cpu'+str(cpu)],b['cpu'+str(cpu)])];idle[cpu]=(d[3]+d[4])/sum(d) if sum(d) else 0
cpu=max(idle,key=idle.get);prefix=['taskset','-c',str(cpu),'nice','-n','19']
(HERE/'host_gate.json').write_text(json.dumps(dict(cpu=cpu,idle=idle,nice=19,maximum_compute_cpus=1,threads=1,scope='User-authorized correctness only, no performance campaign'),indent=2)+'\n')
env=dict(os.environ,OMP_NUM_THREADS='1',OPENBLAS_NUM_THREADS='1',MKL_NUM_THREADS='1',PYTHONDONTWRITEBYTECODE='1',OMPI_MCA_mpi_yield_when_idle='1')
sys.path.insert(0,str(ROOT/'integration_evidence/native_cluster_3d_core_v1'));from run import command
stages=[];status='FAIL';reason='';objects={};libraries={};members={};binary=BUILD/'UnitTests/test_driver'
def run(args,label,cwd,timeout=18000):
 print(label,flush=True);row=command(prefix+list(map(str,args)),cwd,label,env,timeout);stages.append(row)
 if cwd!=HERE:
  for ext in ('stdout','stderr'):__import__('shutil').move(cwd/(label+'.'+ext),HERE/(label+'.'+ext))
 if row['exit_code'] or row.get('timeout'):raise RuntimeError(label+' failed; logs retained')
 return (HERE/(label+'.stdout')).read_text()
try:
 qualified=HERE.parent/'execution_policy_whole_v2'
 control=json.loads((qualified/'validation.json').read_text())
 assert control['status']=='PASS' and not control['source_drift'] and not control['object_drift'] and not control['library_drift']
 assert all(sha(ROOT/n)==h for n,h in control['source_sha256'].items() if n.startswith(('Common/','SU2_CFD/','UnitTests/')) or n=='config_template.cfg')
 assert sha(binary)==control['binary_sha256']
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
 assert objects==control['actual_objects_sha256']
 assert libraries==control['library_sha256'] and members==control['thin_archive_member_sha256']
 selection='[NativeMesh3D],[NativeDistributed3D],[NativeReference3D],[NativeBudget3D],[NativeDefault3D],[NativeBridge2D],[NativeBridge3D]'
 # Registration is read from the actual configured driver, rather than inferred from source grep.
 row=command(prefix+[str(binary),selection,'--list-test-names-only'],HERE,'registered_cases',env,60);stages.append(row)
 names=(HERE/'registered_cases.stdout').read_text().splitlines()
 assert len(names)==106 and row['exit_code'] in (0,106) and not row.get('timeout'), 'Incomplete actual registration'
 for n in (1,2,4):
  work=HERE/('mpi'+str(n));work.mkdir();print('CASE_FOLDER='+str(work),flush=True)
  out=run(['mpirun','--oversubscribe','--bind-to','none','-np',n,binary,selection,'--use-colour','no'],'mpi'+str(n),work,1800)
  assert len(re.findall(r'All tests passed \(\d+ assertions in 106 test cases\)',out))==n
  run(['python3',HERE.parent/'audit_geometry_bridge.py',work,n,HERE/('mpi'+str(n)+'.stdout')],'bridge_mpi'+str(n),work,1800)
  certificate=work/'independent_audit.json';assert json.loads(certificate.read_text())['status']=='PASS'
  certificate.rename(work/'independent_bridge_audit.json')
  import shutil
  shutil.copyfile(HERE/('mpi'+str(n)+'.stdout'),work/'case.stdout')
  from run import distributed_audits
  os.sched_setaffinity(0,{cpu})
  if os.nice(0)<19:os.nice(19-os.nice(0))
  stages.extend(distributed_audits(work,n,HERE.parent,env,1800))
  assert json.loads((work/'distributed_audit.json').read_text())['status']=='PASS'
 for n in (1,2,4):
  work=HERE/('native2d_mpi'+str(n));work.mkdir();print('CASE_FOLDER='+str(work),flush=True)
  out=run(['mpirun','--oversubscribe','--bind-to','none','-np',n,binary,'[NativeRemesher],[NativeRejectedOutput],[NativeEngine2D],[NativeMesh2D]','--use-colour','no'],'native2d_mpi'+str(n),work,1800)
  totals=re.findall(r'All tests passed \(\d+ assertions in (\d+) test cases\)',out)
  assert len(totals)==n and len(set(totals))==1 and int(totals[0])>=25
 mmg=HERE/'mmg_serial';mmg.mkdir();print('CASE_FOLDER='+str(mmg),flush=True)
 out=run(['mpirun','--bind-to','none','-np',1,binary,'MMG interface: 2D round trip,MMG interface: 3D round trip','--use-colour','no'],'mmg_serial',mmg,900)
 assert 'All tests passed (48 assertions in 2 test cases)' in out
 assert sha(binary)==binary_pin;status='PASS'
except Exception as e:reason=str(e);print(reason,flush=True)
finally:
 drift=[n for n,h in pins.items() if sha(ROOT/n)!=h];object_drift=[n for n,h in objects.items() if sha(BUILD/n)!=h];library_drift=[n for n,h in {**libraries,**members}.items() if sha(Path(n))!=h]
 if drift or object_drift or library_drift:status='FAIL'
 receipt=dict(status=status,reason=reason,source_sha256=pins,source_drift=drift,object_drift=object_drift,library_drift=library_drift,actual_objects_sha256=objects,library_sha256=libraries,thin_archive_member_sha256=members,stages=stages,scope='Local-only integrated current-source correctness: actual106registered cases MPI1/2/4, separate exact bridge and eight distributed mesh/metric/state auditors per MPI size, actual2D native operators/engine/eight replacements/rejected-output and serialMMG2D/3D. Full configured68-object driver/archives/members/source pins. No profiling/timing/scaling campaigns or cluster work. Actual metric-Hessian lifecycle still requires its separate correctness receipt.')
 if binary.exists():receipt['binary_sha256']=sha(binary)
 receipt['artifact_sha256']={str(p.relative_to(HERE)):sha(p) for p in HERE.rglob('*') if p.is_file() and p.name!='validation.json'}
 (HERE/'validation.json').write_text(json.dumps(receipt,indent=2)+'\n');print(status,flush=True)
raise SystemExit(status!='PASS')
