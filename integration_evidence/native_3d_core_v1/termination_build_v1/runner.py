"""Actual configured native3D termination-diagnostic correctness build; memory-gated, one CPU, nice19/j1."""
from pathlib import Path
import hashlib,json,os,shlex,shutil,subprocess,time
root=Path('/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated')
folder=Path(__file__).resolve().parent;build=root/'integration_evidence/build-integrated-v2'
sha=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest()
names=subprocess.check_output(['git','ls-files','Common','SU2_CFD','UnitTests','meson.build','meson_options.txt'],cwd=root,text=True).splitlines()
pins={p:sha(root/p) for p in names if (root/p).is_file() and (p.endswith(('.cpp','.hpp','.h','.inl','.tpp','.txt')) or p=='meson.build')}
changes=subprocess.check_output(['git','diff','--name-only','--ignore-submodules=all'],cwd=root,text=True).splitlines()
for p in changes:
 if p.startswith(('Common/','SU2_CFD/','UnitTests/')):
  target=folder/'source_snapshots'/p;target.parent.mkdir(parents=True,exist_ok=True);target.write_bytes((root/p).read_bytes())
scratch=folder/'compiler_scratch';scratch.mkdir(exist_ok=False)
env=dict(os.environ,PATH='/tmp/native-build-tools:'+os.environ['PATH'],TMPDIR=str(scratch),OMP_NUM_THREADS='1',OPENBLAS_NUM_THREADS='1',MKL_NUM_THREADS='1',PYTHONDONTWRITEBYTECODE='1')
stages=[];objects={};libraries={};members={};status='FAIL';reason='';binary=None
def run(cmd,label,cwd=root):
 started=time.monotonic()
 with (folder/(label+'.stdout')).open('w') as out,(folder/(label+'.stderr')).open('w') as err:
  p=subprocess.Popen(list(map(str,cmd)),cwd=cwd,env=env,stdout=out,stderr=err)
  print(label,'PID',p.pid,flush=True);code=p.wait()
 stages.append(dict(stage=label,pid=p.pid,command=list(map(str,cmd)),cwd=str(cwd),seconds=time.monotonic()-started,exit_code=code))
 if code:raise RuntimeError(label+' failed')
try:
 for cmd,label in [(['uptime'],'host_load'),(['free','-m'],'host_memory'),(['ps','-eo','pid,ppid,ni,pcpu,comm','--sort=-pcpu'],'host_processes')]:run(cmd,label)
 read=lambda:list(map(int,Path('/proc/stat').read_text().splitlines()[0].split()[1:]))
 a=read();time.sleep(2);b=read();delta=[y-x for x,y in zip(a,b)]
 idle=(delta[3]+delta[4])/sum(delta);load=os.getloadavg()[0];cpus=os.cpu_count()
 mem={s.split(':')[0]:int(s.split()[1])*1024 for s in Path('/proc/meminfo').read_text().splitlines() if s.startswith(('MemAvailable:','SwapFree:'))}
 gate=dict(cpus=cpus,idle_fraction=idle,load1=load,available_memory=mem['MemAvailable'],disk_free=shutil.disk_usage(root).free)
 (folder/'host_gate.json').write_text(json.dumps(gate,indent=2)+'\n')
 if mem['MemAvailable']<10*1024**3 or gate['disk_free']<5*1024**3:
  raise RuntimeError('Resource gate refused before compiler launch.')
 selected=7 if 7 in os.sched_getaffinity(0) else min(os.sched_getaffinity(0))
 gate.update(affinity=[selected],contention_authorized=True,owned_maximum_compute_cpus=1,nice=19,threads=1)
 (folder/'host_gate.json').write_text(json.dumps(gate,indent=2)+'\n')
 os.sched_setaffinity(0,{selected})
 run(['/tmp/native-build-tools/ninja','-C',build,'-n','UnitTests/test_driver'],'ninja_preflight')
 run(['timeout','--signal=TERM','--kill-after=10s','1800s','nice','-n','19','/tmp/native-build-tools/ninja','-C',build,'-j1','UnitTests/test_driver'],'compile')
 line=next(s for s in (build/'build.ninja').read_text().splitlines() if s.startswith('build UnitTests/test_driver:'))
 objects={p:sha(build/p) for p in shlex.split(line) if p.endswith('.o')}
 assert len(objects)==68
 for name in shlex.split(line):
  if not name.endswith('.a'):continue
  p=Path(name);p=p if p.is_absolute() else build/p;libraries[str(p)]=sha(p)
  with p.open('rb') as f:thin=f.read(8)==b'!<thin>\n'
  if thin:
   for name in subprocess.check_output(['ar','t',p],cwd=root,text=True).splitlines():
    q=Path(name);q=q if q.is_absolute() else root/q;members[str(q)]=sha(q)
 binary=sha(build/'UnitTests/test_driver');status='PASS'
except Exception as e:
 reason=str(e);print(reason,flush=True)
finally:
 drift=[p for p,h in pins.items() if sha(root/p)!=h]
 if drift:status='FAIL'
 v=dict(status=status,reason=reason,stages=stages,source_sha256=pins,source_drift=drift,
        actual_objects_sha256=objects,library_sha256=libraries,thin_archive_member_sha256=members,
        binary_sha256=binary,build_ninja_sha256=sha(build/'build.ninja'),
        branch=subprocess.check_output(['git','branch','--show-current'],cwd=root,text=True).strip(),
        base_sha=subprocess.check_output(['git','rev-parse','HEAD'],cwd=root,text=True).strip(),
        scope='Full configured68-object native3D termination-diagnostic build only. User-authorized contended correctness build on one CPU at nice19/j1/thread1, memory/disk admission retained. No numerical/MPI PASS or timing claim.')
 v['artifact_sha256']={str(p.relative_to(folder)):sha(p) for p in folder.rglob('*') if p.is_file() and 'compiler_scratch' not in p.parts}
 (folder/'validation.json').write_text(json.dumps(v,indent=2)+'\n');print(status,folder,flush=True)
raise SystemExit(status!='PASS')
