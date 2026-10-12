"""Build isolated rotating directory plus actual supplementary MPI protocol tests."""
from pathlib import Path
import hashlib,json,os,re,shlex,shutil,subprocess,sys,time
folder=Path(__file__).resolve().parent;root=folder.parents[2];base=folder.parent;build=root/'integration_evidence/build-integrated-v2';sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
assert not (folder/'validation.json').exists(),'Keep terminal evidence.'
qualified_path=base/'completion_diagnostic_build_v1/validation.json';b=json.loads(qualified_path.read_text());assert b['status']=='PASS'
assert all(sha(root/n)==h for n,h in b['source_sha256'].items())
assert all(sha(build/n)==h for n,h in b['actual_objects_sha256'].items())
libs={**b['library_sha256'],**b['thin_archive_member_sha256']};assert all(sha(Path(n))==h for n,h in libs.items())
print('Working and retained case folder:',folder,flush=True)
env=dict(os.environ,PYTHONDONTWRITEBYTECODE='1',OMP_NUM_THREADS='1',OPENBLAS_NUM_THREADS='1',MKL_NUM_THREADS='1',OMPI_MCA_mpi_yield_when_idle='1')
(folder/'host.txt').write_text(subprocess.check_output(['uptime'],text=True)+subprocess.check_output(['free','-h'],text=True)+'\n'+'\n'.join(subprocess.check_output(['ps','-eo','pid,ppid,ni,pcpu,pmem,comm','--sort=-pcpu'],text=True).splitlines()[:13])+'\n')
available=int(next(l.split()[1] for l in Path('/proc/meminfo').read_text().splitlines() if l.startswith('MemAvailable:')))*1024
assert available>=3*1024**3 and shutil.disk_usage(folder).free>=3*1024**3
sample=lambda:{l.split()[0]:list(map(int,l.split()[1:])) for l in Path('/proc/stat').read_text().splitlines() if l.startswith('cpu')}
a=sample();time.sleep(1);now=sample();idle={}
for cpu in os.sched_getaffinity(0):
 d=[y-x for x,y in zip(a['cpu'+str(cpu)],now['cpu'+str(cpu)])];idle[cpu]=(d[3]+d[4])/sum(d) if sum(d) else 0
cpu=max(idle,key=idle.get);prefix=['taskset','-c',str(cpu),'nice','-n','19']
(folder/'host_gate.json').write_text(json.dumps(dict(cpu=cpu,idle=idle,available_bytes=available,owned_maximum_compute_cpus=1,nice=19,threads=1,scope='Sequential isolated compile/link/MPI1/2/4 protocol correctness only.'),indent=2)+'\n')
commands=json.loads((build/'compile_commands.json').read_text())
def compile_command(suffix,source,obj):
 entry=next(r for r in commands if r['file'].endswith(suffix));args=shlex.split(entry['command']);cmd=[];i=0
 while i<len(args):
  if args[i] in ('-o','-MQ','-MF','-c'):i+=2;continue
  if args[i]=='-MD':i+=1;continue
  cmd.append(args[i]);i+=1
 return args[0],cmd+['-o',str(obj),'-c',str(source)]
dir_obj=folder/'directory_rotating.o';test_obj=folder/'protocol_tests.o';binary=folder/'test_driver_protocol'
compiler,dir_compile=compile_command('/adaptation/CNativeDistributed3D.cpp',folder/'directory_rotating.cpp',dir_obj)
_,test_compile=compile_command('/adaptation/CNativeDistributed3D_tests.cpp',folder/'protocol_tests.cpp',test_obj)
lines=(build/'build.ninja').read_text().splitlines();start=next(i for i,l in enumerate(lines) if l.startswith('build UnitTests/test_driver: cpp_LINKER '))
objects=shlex.split(lines[start].split('cpp_LINKER ',1)[1].split(' | ',1)[0]);assert len(objects)==68
old_test=[n for n in objects if n.endswith('Common_adaptation_CNativeDistributed3D_tests.cpp.o')];assert len(old_test)==1
objects=[n for n in objects if n not in old_test]
linkargs=shlex.split(next(l.split('=',1)[1] for l in lines[start+1:] if l.startswith(' LINK_ARGS =')))
link=[compiler,str(dir_obj),str(test_obj),*objects,'-o',str(binary),'-Wl,-Map,'+str(folder/'link.map'),*linkargs]
pins={str(p.relative_to(root)):sha(p) for p in [folder/'directory_rotating.cpp',folder/'protocol_tests.cpp',Path(__file__).resolve(),root/'Common/src/adaptation/CNativeDistributed3D.cpp',root/'UnitTests/Common/adaptation/CNativeDistributed3D_tests.cpp',build/'compile_commands.json',build/'build.ninja']}
stages=[];status='FAIL';reason=''
def run(cmd,label,cwd=folder):
 with (folder/(label+'.stdout')).open('w') as out,(folder/(label+'.stderr')).open('w') as err:
  begin=time.monotonic();p=subprocess.run(list(map(str,cmd)),cwd=cwd,env=env,stdout=out,stderr=err,timeout=900)
 stages.append(dict(label=label,command=list(map(str,cmd)),cwd=str(cwd),exit_code=p.returncode,seconds=time.monotonic()-begin))
 if p.returncode:raise RuntimeError(label+' failed; source and logs retained')
try:
 for cmd,label in [(dir_compile,'compile_directory'),(test_compile,'compile_tests'),(link,'link')]:
  print(label,flush=True);run(prefix+cmd,label,build)
 text=(folder/'link.map').read_text();assert 'adaptation_CNativeDistributed3D.cpp.o' not in text and str(dir_obj) in text
 for ranks in (1,2,4):
  print('MPI',ranks,flush=True);label='mpi%d'%ranks
  run(prefix+['mpirun','--oversubscribe','--bind-to','none','-np',str(ranks),str(binary),'[NativeRotatingProbe3D]','--use-colour','no'],label)
  out=(folder/(label+'.stdout')).read_text();assert len(re.findall(r'All tests passed \(\d+ assertions in 3 test cases\)',out))==ranks
 status='PASS'
except Exception as e:reason=str(e);print('FAIL',reason,flush=True)
finally:
 source_drift=[n for n,h in pins.items() if sha(root/n)!=h]
 object_drift=[n for n,h in b['actual_objects_sha256'].items() if sha(build/n)!=h]
 library_drift=[n for n,h in libs.items() if sha(Path(n))!=h]
 if source_drift or object_drift or library_drift:status='FAIL'
 v=dict(status=status,reason=reason,stages=stages,source_sha256=pins,source_drift=source_drift,object_drift=object_drift,library_drift=library_drift,qualified_build_receipt_sha256=sha(qualified_path),original_objects_sha256=b['actual_objects_sha256'],original_library_sha256=libs,
     disposable_sha256={p.name:sha(p) for p in (dir_obj,test_obj,binary) if p.exists()},scope='Isolated actual MPI1/2/4 protocol tests using exact audited rotating directory and qualified test helpers. Repeated shared/disjoint full stars, one-rank memory veto and test-only priority-phase corruption preserve both stores. Production binary/source/objects/archives untouched. No new mesh/PDE/performance/general qualification.')
 v['artifact_sha256']={p.name:sha(p) for p in folder.iterdir() if p.is_file() and p.name not in ('validation.json',dir_obj.name,test_obj.name,binary.name)}
 (folder/'validation.json').write_text(json.dumps(v,indent=2)+'\n');print(status,folder,flush=True)
raise SystemExit(status!='PASS')
