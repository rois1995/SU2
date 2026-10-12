"""Isolated configured-directory variant and one audited N4/M2 mesh correctness case."""
from pathlib import Path
import hashlib,json,os,re,shlex,shutil,subprocess,sys,time
folder=Path(__file__).resolve().parent;root=folder.parents[2];base=folder.parent;build=root/'integration_evidence/build-integrated-v2'
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
print('Working and retained case folder:',folder,flush=True)
assert not (folder/'validation.json').exists(),'Keep terminal evidence unchanged.'
qualified_path=base/'completion_diagnostic_build_v1/validation.json';qualified=json.loads(qualified_path.read_text());assert qualified['status']=='PASS'
assert all(sha(root/n)==h for n,h in qualified['source_sha256'].items())
assert all(sha(build/n)==h for n,h in qualified['actual_objects_sha256'].items())
libs={**qualified['library_sha256'],**qualified['thin_archive_member_sha256']}
assert all(sha(Path(n))==h for n,h in libs.items())
source=root/'Common/src/adaptation/CNativeDistributed3D.cpp';variant=folder/'directory_rotating.cpp'
entry=next(r for r in json.loads((build/'compile_commands.json').read_text()) if r['file'].endswith('/adaptation/CNativeDistributed3D.cpp'))
args=shlex.split(entry['command']);compile=[];i=0
while i<len(args):
 if args[i] in ('-o','-MQ','-MF','-c'):i+=2;continue
 if args[i]=='-MD':i+=1;continue
 compile.append(args[i]);i+=1
obj=folder/'directory_rotating.o';binary=folder/'test_driver_rotating'
compile+=['-o',str(obj),'-c',str(variant)]
lines=(build/'build.ninja').read_text().splitlines();start=next(i for i,s in enumerate(lines) if s.startswith('build UnitTests/test_driver: cpp_LINKER '))
objects=shlex.split(lines[start].split('cpp_LINKER ',1)[1].split(' | ',1)[0]);assert len(objects)==68
linkargs=shlex.split(next(s.split('=',1)[1] for s in lines[start+1:] if s.startswith(' LINK_ARGS =')))
link=[args[0],str(obj),*objects,'-o',str(binary),'-Wl,-Map,'+str(folder/'link.map'),*linkargs]
baseline=base/'box_filter_n4_n4_m2_v1'
for name in ('parameters.json','case_environment.json'):shutil.copyfile(baseline/name,folder/name)
parameters=json.loads((folder/'parameters.json').read_text());assert (parameters['ranks'],parameters['workers'],parameters['diagnostic_rounds'])==(4,2,512)
env=dict(os.environ,OMP_NUM_THREADS='1',OPENBLAS_NUM_THREADS='1',MKL_NUM_THREADS='1',NUMEXPR_NUM_THREADS='1',PYTHONDONTWRITEBYTECODE='1',OMPI_MCA_mpi_yield_when_idle='1')
for key in tuple(env):
 if key.startswith('SU2_NATIVE3D_ENVELOPE_'):del env[key]
env.update(json.loads((folder/'case_environment.json').read_text()))
memory=int(next(s.split()[1] for s in Path('/proc/meminfo').read_text().splitlines() if s.startswith('MemAvailable:')))*1024
assert memory>=3*1024**3 and shutil.disk_usage(folder).free>=3*1024**3
(folder/'host.txt').write_text(subprocess.check_output(['uptime'],text=True)+subprocess.check_output(['free','-h'],text=True)+'\n'+'\n'.join(subprocess.check_output(['ps','-eo','pid,ppid,ni,pcpu,pmem,comm','--sort=-pcpu'],text=True).splitlines()[:13])+'\n')
def ticks():return {line.split()[0]:list(map(int,line.split()[1:])) for line in Path('/proc/stat').read_text().splitlines() if line.startswith('cpu')}
a=ticks();time.sleep(1);b=ticks();idle={}
for cpu in os.sched_getaffinity(0):
 d=[y-x for x,y in zip(a['cpu'+str(cpu)],b['cpu'+str(cpu)])];idle[cpu]=(d[3]+d[4])/sum(d) if sum(d) else 0
cpu=max(idle,key=idle.get);prefix=['taskset','-c',str(cpu),'nice','-n','19']
(folder/'host_gate.json').write_text(json.dumps(dict(cpu=cpu,available_memory=memory,idle=idle,owned_maximum_compute_cpus=1,nice=19,threads=1,scope='Contention-authorized correctness-only isolated build/MPI4 case; no timing/scaling comparison.'),indent=2)+'\n')
tools=('audit_envelope.py','audit_geometry_bridge.py','audit_lifecycle.py','audit_coupled.py','audit_cavities.py','audit_restart.py','export_envelope_vtu.py')
pins={str(p.relative_to(root)):sha(p) for p in [source,variant,Path(__file__).resolve(),build/'compile_commands.json',build/'build.ninja',*[base/n for n in tools]]}
sys.path.insert(0,str(root/'integration_evidence/native_cluster_3d_core_v1'));from run import command
stages=[];status='FAIL';reason=''
try:
 for label,cmd,cwd in [('compile',prefix+compile,build),('link',prefix+link,build),
     ('case',prefix+['mpirun','--oversubscribe','--bind-to','none','-np','4',str(binary),'[NativeEnvelope3D]','--use-colour','no'],folder)]:
  print(label,flush=True);row=command(list(map(str,cmd)),cwd,label,env,900)
  if cwd!=folder:
   for ext in ('stdout','stderr'):shutil.move(cwd/(label+'.'+ext),folder/(label+'.'+ext))
  stages.append(row)
  if row['exit_code'] or row.get('timeout'):raise RuntimeError(label+' failed; original logs retained')
  if label=='link':
   text=(folder/'link.map').read_text()
   assert str(obj) in text and 'adaptation_CNativeDistributed3D.cpp.o' not in text,'Original directory member was unexpectedly extracted'
 out=(folder/'case.stdout').read_text();assert len(re.findall(r'All tests passed \(\d+ assertions in 1 test case\)',out))==4
 for label,tool in [('audit','audit_envelope.py'),('export','export_envelope_vtu.py')]:
  row=command(prefix+[sys.executable,str(base/tool),str(folder)],folder,label,env,900);stages.append(row)
  if row['exit_code'] or row.get('timeout'):raise RuntimeError(label+' failed; original files retained')
 assert json.loads((folder/'independent_audit.json').read_text())['status']=='PASS'
 status='PASS'
except Exception as e:reason=str(e);print('FAIL',reason,flush=True)
finally:
 drift=[n for n,h in pins.items() if sha(root/n)!=h]
 objects_drift=[n for n,h in qualified['actual_objects_sha256'].items() if sha(build/n)!=h]
 libs_drift=[n for n,h in libs.items() if sha(Path(n))!=h]
 if drift or objects_drift or libs_drift:status='FAIL'
 result=dict(status=status,reason=reason,stages=stages,qualified_build_receipt_sha256=sha(qualified_path),source_sha256=pins,source_drift=drift,object_drift=objects_drift,library_drift=libs_drift,original_objects_sha256=qualified['actual_objects_sha256'],original_library_sha256=libs,
     binary_sha256=sha(binary) if binary.exists() else None,object_sha256=sha(obj) if obj.exists() else None,
     scope='Isolated complete configured driver with only directory reservation priority rotated by matching call epoch; one validated epoch word per claim, no extra collective. Actual N4M2 three-event default512 wedge correctness and unchanged exact audits if PASS. Production binary/objects/archives untouched. Changed trajectory is not fixed-work speed evidence, general MPI/robustness/scaling certification or fullGoal1.')
 result['artifact_sha256']={p.name:sha(p) for p in folder.iterdir() if p.is_file() and p.name not in ('validation.json',binary.name,obj.name)}
 (folder/'validation.json').write_text(json.dumps(result,indent=2)+'\n');print(status,folder,flush=True)
raise SystemExit(status!='PASS')
