from pathlib import Path
import hashlib,json,os,re,shlex,subprocess,sys,time
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2];BUILD=ROOT/'integration_evidence/build-integrated-v2'
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
control=json.loads((HERE.parent/'curved_runtime_regression_v2/validation.json').read_text());assert control['status']=='PASS'
changed={'UnitTests/SU2_CFD/adaptation/CNativeImport3D_tests.cpp'}
assert all(sha(ROOT/n)==h for n,h in control['source_sha256'].items() if n.startswith(('Common/','SU2_CFD/','UnitTests/')) and n not in changed)
pins={n:sha(ROOT/n) for n in control['source_sha256'] if n.startswith(('Common/','SU2_CFD/','UnitTests/'))}
pins[str(Path(__file__).resolve().relative_to(ROOT))]=sha(Path(__file__).resolve())
assert not (HERE/'validation.json').exists()
(HERE/'host.txt').write_text(subprocess.check_output(['uptime'],text=True)+subprocess.check_output(['free','-m'],text=True))
assert int(next(l.split()[1] for l in Path('/proc/meminfo').read_text().splitlines() if l.startswith('MemAvailable:')))*1024>=3*1024**3
def ticks():return {l.split()[0]:list(map(int,l.split()[1:])) for l in Path('/proc/stat').read_text().splitlines() if l.startswith('cpu')}
a=ticks();time.sleep(1);b=ticks();idle={}
for c in os.sched_getaffinity(0):
 d=[y-x for x,y in zip(a['cpu'+str(c)],b['cpu'+str(c)])];idle[c]=(d[3]+d[4])/sum(d) if sum(d) else 0
cpu=max(idle,key=idle.get);prefix=['taskset','-c',str(cpu),'nice','-n','19']
env=dict(os.environ,OMP_NUM_THREADS='1',OPENBLAS_NUM_THREADS='1',MKL_NUM_THREADS='1',PYTHONDONTWRITEBYTECODE='1',OMPI_MCA_mpi_yield_when_idle='1',SU2_NATIVE_CURVED_EVIDENCE='1')
sys.path.insert(0,str(ROOT/'integration_evidence/native_cluster_3d_core_v1'));from run import command
stages=[];objects={};libraries={};members={};counts=[];status='FAIL';reason='';binary=BUILD/'UnitTests/test_driver'
def run(args,label,work,timeout=1800):
 print('CASE_FOLDER='+str(work),flush=True);row=command(prefix+list(map(str,args)),work,label,env,timeout);stages.append(row)
 if row['exit_code'] or row.get('timeout'):raise RuntimeError(label+' failed; source and logs retained')
 return (work/(label+'.stdout')).read_text()
try:
 assert sha(binary)==control['binary_sha256']
 run(['/tmp/native-build-tools/ninja','-C',BUILD,'-j1','UnitTests/test_driver'],'build',HERE,3600)
 line=next(l for l in (BUILD/'build.ninja').read_text().splitlines() if l.startswith('build UnitTests/test_driver: cpp_LINKER '))
 objects={n:sha(BUILD/n) for n in shlex.split(line.split('cpp_LINKER ',1)[1].split(' | ',1)[0]) if n.endswith('.o')};assert len(objects)==68
 for n in control['library_sha256']:libraries[n]=sha(Path(n))
 for n in control['thin_archive_member_sha256']:members[n]=sha(Path(n))
 binary_pin=sha(binary)
 for n in (1,2,4):
  work=HERE/('mpi'+str(n));work.mkdir()
  out=run(['mpirun','--oversubscribe','--bind-to','none','-np',n,binary,'[NativeCurvedLifecycle3D],[NativeSolvedEuler3D]','--use-colour','no'],'case',work)
  found=re.findall(r'All tests passed \((\d+) assertions in (\d+) test cases\)',out)
  assert len(found)==n and all(int(c)==2 for a,c in found)
  counts.append({'ranks':n,'cases_per_rank':2,'assertions':[int(a) for a,c in found]})
 assert sha(binary)==binary_pin;status='PASS'
except Exception as e:reason=str(e);print(reason,flush=True)
finally:
 drift=[n for n,h in pins.items() if sha(ROOT/n)!=h]
 if drift:status='FAIL'
 receipt=dict(status=status,reason=reason,source_sha256=pins,source_drift=drift,actual_objects_sha256=objects,library_sha256=libraries,thin_archive_member_sha256=members,stages=stages,counts=counts,scope='Focused curved lifecycle: manufactured nonuniform conservative growth/shrink accounting, repeated refine/coarsen, actual CGNS/BDF restart and SU2/CGNS readback, plus actual solved subsonic Euler slip-wall pressure-Hessian metrics, repeated native events/restart/solver continuation. No shock/convergence/full Goal2 or performance/cluster qualification. One owned CPU/nice19/threads1/build-j1/MPI<=4.')
 if binary.exists():receipt['binary_sha256']=sha(binary)
 receipt['artifact_sha256']={str(p.relative_to(HERE)):sha(p) for p in HERE.rglob('*') if p.is_file() and p.name!='validation.json'}
 (HERE/'validation.json').write_text(json.dumps(receipt,indent=2)+'\n');print(status,flush=True)
raise SystemExit(status!='PASS')
