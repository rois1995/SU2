"""One diagnostic-only replay of the frozen n8 failure using actual configured test objects."""
import hashlib,json,os,shlex,subprocess,sys,time
from pathlib import Path
folder=Path(__file__).resolve().parent;root=folder.parents[2];base=folder.parent;build=root/'integration_evidence/build-integrated-v2'
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
print('WORKING_FOLDER='+str(folder),flush=True)
assert not (folder/'validation.json').exists(),'Preserve terminal evidence.'
qualified=json.loads((base/'coarsen_changes_build_v1/validation.json').read_text());assert qualified['status']=='PASS'
assert all(sha(root/n)==h for n,h in qualified['source_sha256'].items())
assert all(sha(build/n)==h for n,h in qualified['actual_objects_sha256'].items())
assert all(sha(Path(n))==h for n,h in {**qualified['library_sha256'],**qualified['thin_archive_member_sha256']}.items())
engine=root/'Common/src/adaptation/CNativeEngine3D.cpp';s=engine.read_text()
old='#include "../../include/adaptation/CNativeEngine3D.hpp"'
assert s.count(old)==1
s=s.replace(old,'#include "../../../Common/include/adaptation/CNativeEngine3D.hpp"\n#include "trace.hpp"')
old='      if (chosen && !approved) ++stats.rejected;';assert s.count(old)==1
s=s.replace(old,'      TraceAttempt(intent,round,world.rank,chosen,approved,repairing,choice.edge,imported,fresh,result.reason);\n'+old)
old='    stats.final_owned_cells=working.size();';assert s.count(old)==1
s=s.replace(old,'    TraceFinish(intent,world.rank,result.valid,working,geometry);\n'+old)
(folder/'engine_trace.cpp').write_text(s)
entry=next(r for r in json.loads((build/'compile_commands.json').read_text()) if r['file'].endswith('/adaptation/CNativeEngine3D.cpp'))
args=shlex.split(entry['command']);compile=[];i=0
while i<len(args):
 if args[i] in ('-o','-MQ','-MF','-c'):i+=2;continue
 if args[i]=='-MD':i+=1;continue
 compile.append(args[i]);i+=1
obj=folder/'engine_trace.o';binary=folder/'test_driver_trace'
compile+=['-o',str(obj),'-c',str(folder/'engine_trace.cpp')]
lines=(build/'build.ninja').read_text().splitlines();start=next(i for i,s in enumerate(lines) if s.startswith('build UnitTests/test_driver: cpp_LINKER '))
objects=shlex.split(lines[start].split('cpp_LINKER ',1)[1].split(' | ',1)[0]);assert len(objects)==68
linkargs=shlex.split(next(s.split('=',1)[1] for s in lines[start+1:] if s.startswith(' LINK_ARGS =')))
link=[args[0],str(obj)]+objects+['-o',str(binary)]+linkargs
oldcase=base/'coarsen_changes_n8_n1_m1_v1'
for name in ('parameters.json','case_environment.json'):(folder/name).write_bytes((oldcase/name).read_bytes())
env=dict(os.environ,OMP_NUM_THREADS='1',OPENBLAS_NUM_THREADS='1',MKL_NUM_THREADS='1',NUMEXPR_NUM_THREADS='1',PYTHONDONTWRITEBYTECODE='1',OMPI_MCA_mpi_yield_when_idle='1')
for key in tuple(env):
 if key.startswith('SU2_NATIVE3D_ENVELOPE_'):del env[key]
env.update(json.loads((folder/'case_environment.json').read_text()))
available=int(next(s.split()[1] for s in Path('/proc/meminfo').read_text().splitlines() if s.startswith('MemAvailable:')))*1024
assert available>=2*1024**3
import shutil
assert shutil.disk_usage(folder).free>=3*1024**3,'Insufficient actual scratch space for isolated diagnostic link.'
def read_cpu():
 return {line.split()[0]:list(map(int,line.split()[1:])) for line in Path('/proc/stat').read_text().splitlines() if line.startswith('cpu')}
a=read_cpu();time.sleep(2);b=read_cpu();idles={}
for cpu in os.sched_getaffinity(0):
 d=[y-x for x,y in zip(a['cpu'+str(cpu)],b['cpu'+str(cpu)])];idles[cpu]=(d[3]+d[4])/sum(d) if sum(d) else 0
cpu=max(idles,key=idles.get);prefix=['nice','-n','19','taskset','-c',str(cpu)]
(folder/'host.txt').write_text(subprocess.check_output(['uptime'],text=True)+subprocess.check_output(['free','-h'],text=True)+'\n'+'\n'.join(subprocess.check_output(['ps','-eo','pid,ppid,ni,pcpu,pmem,comm','--sort=-pcpu'],text=True).splitlines()[:13])+'\n'+str(dict(cpu=cpu,available_bytes=available))+'\n')
pins={str(p.relative_to(root)):sha(p) for p in [engine,folder/'trace.hpp',folder/'engine_trace.cpp',folder/'run.py',build/'compile_commands.json',build/'build.ninja']}
stages=[]
for label,cmd,cwd in [('compile',prefix+compile,build),('link',prefix+link,build),('case',prefix+['mpirun','--oversubscribe','--bind-to','none','-np','1',str(binary),'[NativeEnvelope3D]','--use-colour','no'],folder)]:
 print(label,flush=True);start=time.monotonic()
 with (folder/(label+'.stdout')).open('w') as out,(folder/(label+'.stderr')).open('w') as err:
  p=subprocess.run(cmd,cwd=cwd,env=env,stdout=out,stderr=err,timeout=600)
 stages.append(dict(label=label,argv=cmd,exit_code=p.returncode,seconds=time.monotonic()-start))
 if p.returncode:break
result=dict(status='PASS' if len(stages)==3 and [s['exit_code'] for s in stages]==[0,0,1] else 'FAIL',adaptation_status='FAIL',stages=stages,source_sha256=pins,source_drift=[n for n,h in pins.items() if sha(root/n)!=h],scope='Diagnostic-source variant of the actual complete configured 68-object test driver. Exactly three engine instrumentation insertions; same dependency-correct selection/orientation filter/operators/acceptance/metric policy and frozen n8 input as the qualified current driver. Records separate coarsening epochs and distinct private exports. Preserves expected rejected event; no accepted-grid, MPI scalability, or timing claim.',qualified_build_receipt_sha256=sha(base/'coarsen_changes_build_v1/validation.json'))
if binary.exists():result['binary_sha256']=sha(binary)
result['original_objects_sha256']=qualified['actual_objects_sha256'];result['original_library_sha256']={**qualified['library_sha256'],**qualified['thin_archive_member_sha256']}
result['object_drift']=[n for n,h in qualified['actual_objects_sha256'].items() if sha(build/n)!=h]
result['library_drift']=[n for n,h in result['original_library_sha256'].items() if sha(Path(n))!=h]
result['artifact_sha256']={p.name:sha(p) for p in folder.iterdir() if p.is_file() and p.name not in ('validation.json','test_driver_trace','engine_trace.o')}
(folder/'validation.json').write_text(json.dumps(result,indent=2)+'\n');print(result['status'],flush=True)
sys.exit(result['status']!='PASS' or bool(result['source_drift']+result['object_drift']+result['library_drift']))
