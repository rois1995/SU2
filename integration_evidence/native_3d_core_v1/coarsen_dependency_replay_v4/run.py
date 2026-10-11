"""Build one source-matched local selector reproduction; no production mutation."""
import hashlib,json,os,shlex,subprocess,sys,time
from pathlib import Path
folder=Path(__file__).resolve().parent;root=folder.parents[2]
build=root/'integration_evidence/build-integrated-v2'
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
print('WORKING_FOLDER='+str(folder),flush=True)
assert not (folder/'validation.json').exists(),'Preserve prior terminal evidence; choose a fresh folder.'
entries=json.loads((build/'compile_commands.json').read_text())
entry=next(r for r in entries if r['file'].endswith('/adaptation/CNativeEngine3D.cpp'))
command=shlex.split(entry['command']);cooked=[];i=0
while i<len(command):
 arg=command[i]
 if arg in ('-o','-MQ','-MF','-c'):i+=2;continue
 if arg=='-MD':i+=1;continue
 cooked.append(arg);i+=1
obj=folder/'probe.o';exe=folder/'probe'
cooked+=['-o',str(obj),'-c',str(folder/'probe.cpp')]
lines=(build/'build.ninja').read_text().splitlines()
start=next(i for i,s in enumerate(lines) if s.startswith('build UnitTests/test_driver: cpp_LINKER '))
linkargs=shlex.split(next(s.split('=',1)[1] for s in lines[start+1:] if s.startswith(' LINK_ARGS =')))
link=[command[0],str(obj),'-o',str(exe)]+linkargs
available=int(next(s.split()[1] for s in Path('/proc/meminfo').read_text().splitlines() if s.startswith('MemAvailable:')))*1024
assert available>=2*1024**3,'Insufficient actual available host memory.'
cpu=min(os.sched_getaffinity(0));prefix=['nice','-n','19','taskset','-c',str(cpu)]
env=dict(os.environ,OMP_NUM_THREADS='1',OPENBLAS_NUM_THREADS='1',MKL_NUM_THREADS='1',NUMEXPR_NUM_THREADS='1')
sourcepaths=[root/'Common/src/adaptation/CNativeEngine3D.cpp',folder/'probe.cpp',folder/'run.py',build/'compile_commands.json',build/'build.ninja']
pins={str(p.relative_to(root)):sha(p) for p in sourcepaths}
(folder/'host.txt').write_text(subprocess.check_output(['uptime'],text=True)+subprocess.check_output(['free','-h'],text=True)+'\n'+'\n'.join(subprocess.check_output(['ps','-eo','pid,ppid,ni,pcpu,pmem,comm','--sort=-pcpu'],text=True).splitlines()[:13])+'\n'+str(dict(available_bytes=available,cpu=cpu))+'\n')
stages=[]
for label,cmd,cwd in [('compile',prefix+cooked,build),('link',prefix+link,build),('probe',prefix+[str(exe)],folder)]:
 print(label,flush=True)
 start=time.monotonic()
 with (folder/(label+'.stdout')).open('w') as out,(folder/(label+'.stderr')).open('w') as err:
  p=subprocess.run(cmd,cwd=cwd,env=env,stdout=out,stderr=err)
 stages.append(dict(label=label,argv=cmd,exit_code=p.returncode,seconds=time.monotonic()-start))
 if p.returncode:break
result=dict(status='PASS' if len(stages)==3 and all(s['exit_code']==0 for s in stages) else 'FAIL',stages=stages,source_sha256=pins,source_drift=[n for n,h in pins.items() if sha(root/n)!=h],scope='Source-matched local selection/kernel reproduction only; not an engine campaign, MPI test, fix, or proof of the n8 failure cause.')
if exe.exists():result['binary_sha256']=sha(exe)
result['artifact_sha256']={p.name:sha(p) for p in folder.iterdir() if p.is_file() and p.name not in ('validation.json','probe','probe.o')}
(folder/'validation.json').write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps(result,indent=2),flush=True)
sys.exit(result['status']!='PASS' or bool(result['source_drift']))
