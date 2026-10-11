from pathlib import Path
import hashlib,json,os,subprocess,time
root=Path('/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated');build=root/'integration_evidence/build-integrated-v2'
folder=root/'integration_evidence/native_3d_core_v1/geometry_build_v1';folder.mkdir(exist_ok=False)
print('Case working and retained folder:',folder,flush=True)
(folder/'runner_snapshot.py').write_bytes(Path(__file__).read_bytes())
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
tracked=subprocess.check_output(['git','ls-files','Common','SU2_CFD','meson.build','meson_options.txt'],cwd=root,text=True).splitlines()
tracked+=['Common/include/adaptation/CNativeImport3D.hpp','Common/src/adaptation/CNativeImport3D.cpp']
pins={p:sha(root/p) for p in tracked if (root/p).is_file() and (p.endswith(('.cpp','.hpp','.build','.txt')))}
(folder/'source_sha256.json').write_text(json.dumps(pins,indent=2)+'\n')
for p in ['Common/include/adaptation/CNativeImport3D.hpp','Common/src/adaptation/CNativeImport3D.cpp','Common/src/adaptation/meson.build']:
 q=folder/'source_snapshots'/p;q.parent.mkdir(parents=True,exist_ok=True);q.write_bytes((root/p).read_bytes())
(folder/'planned_build.stdout').write_bytes(Path('/tmp/native-geometry-bridge-build-plan.stdout').read_bytes())
env=dict(os.environ,PATH='/tmp/native-build-tools:'+os.environ['PATH'],OMP_NUM_THREADS='1',OPENBLAS_NUM_THREADS='1',MKL_NUM_THREADS='1')
targets=['Common/src/libSU2Common.a','SU2_CFD/src/libSU2core.a','Common/src/libSU2AccurateSum.a']
(folder/'active.json').write_text(json.dumps(dict(pid=os.getpid(),targets=targets,started=time.time()),indent=2)+'\n')
for cmd,name in [(['uptime'],'host_load'),(['free','-m'],'host_memory'),(['df','-h',str(root)],'host_disk'),(['ps','-eo','pid,ppid,ni,pcpu,etime,comm','--sort=-pcpu'],'host_processes')]:
 (folder/(name+'.stdout')).write_text(subprocess.check_output(cmd,text=True))
commands=subprocess.check_output(['/tmp/native-build-tools/ninja','-C',str(build),'-t','commands']+targets,text=True,env=env)
(folder/'actual_build_commands.txt').write_text(commands)
start=time.monotonic();status='FAIL';code=-1
command=['nice','-n','19','/tmp/native-build-tools/ninja','-C',str(build),'-j1']+targets
try:
 with (folder/'build.stdout').open('w') as out,(folder/'build.stderr').open('w') as err:
  p=subprocess.Popen(command,cwd=root,env=env,stdout=out,stderr=err)
  (folder/'active.json').write_text(json.dumps(dict(pid=os.getpid(),child_pid=p.pid,targets=targets,started=time.time()),indent=2)+'\n')
  code=p.wait()
 if code==0:status='PASS'
finally:
 changed=[p for p,h in pins.items() if sha(root/p)!=h]
 if changed:status='FAIL'
 result=dict(status=status,exit_code=code,command=command,seconds=time.monotonic()-start,source_sha256=pins,changed_during_build=changed,scope='Actual current-ABI primal MPI/CGNS/MMG SU2 Common, strict and CFD core archives. Compilation only; not a 3D runtime/correctness/affordability certificate.')
 (folder/'validation.json').write_text(json.dumps(result,indent=2)+'\n')
 print(status,folder,flush=True)
