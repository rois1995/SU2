import hashlib,json,os,subprocess,time
from pathlib import Path
root=Path('/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated')
folder=root/'integration_evidence/native_3d_core_v1/partition_meson_v1';folder.mkdir(exist_ok=False)
print('Case working and retained folder:',folder,flush=True)
(folder/'runner_snapshot.py').write_bytes(Path(__file__).read_bytes())
base=root/'integration_evidence/native_3d_core_v1/coplanar_regression_v2'
validated=root/'integration_evidence/native_3d_core_v1/partition_controls_v2/validation.json'
receipt=json.loads(validated.read_text());assert receipt['status']=='PASS'
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
pins=dict(receipt['source_sha256']);pins['UnitTests/Common/adaptation/CNativeMesh2D_tests.cpp']=sha(root/'UnitTests/Common/adaptation/CNativeMesh2D_tests.cpp')
for p,h in pins.items():assert sha(root/p)==h,p
build=root/'integration_evidence/build-integrated-v2'
house=['CADTBaseClass','CADTElemClass','mpi_structure','CPassiveComm','CDistributedSearch','CTransferMemory']
objects=[base/(name+'.o') for name in house]
object_pins={str(p):sha(p) for p in objects}
libs=[build/'externals/parmetis/libparmetis.a',build/'externals/metis/libmetis.a'];library_pins={str(p):sha(p) for p in libs}
env=dict(os.environ,PATH='/tmp/native-build-tools:'+os.environ['PATH'],OMP_NUM_THREADS='1',OPENBLAS_NUM_THREADS='1',MKL_NUM_THREADS='1',OMPI_MCA_mpi_yield_when_idle='1',OMPI_MCA_hwloc_base_binding_policy='none',PYTHONDONTWRITEBYTECODE='1')
stages=[];status='FAIL'
def run(command,label):
 print('Starting',label,flush=True);started=time.monotonic();p=subprocess.run(list(map(str,command)),cwd=root,env=env,text=True,capture_output=True)
 (folder/(label+'.stdout')).write_text(p.stdout);(folder/(label+'.stderr')).write_text(p.stderr)
 stages.append(dict(stage=label,command=list(map(str,command)),exit_code=p.returncode,seconds=time.monotonic()-started))
 if p.returncode:raise RuntimeError(label+' failed; inspect '+str(folder))
 return p.stdout
try:
 for c,label in [(['uptime'],'host_load'),(['free','-m'],'host_memory'),(['ps','-eo','pid,ppid,ni,pcpu,etime,comm','--sort=-pcpu'],'host_processes')]:run(c,label)
 targets=['Common/src/libSU2AccurateSum.a','UnitTests/test_driver.p/Common_adaptation_CNativePartition3D_tests.cpp.o','UnitTests/test_driver.p/Common_adaptation_CNativeMesh2D_tests.cpp.o']
 run(['nice','-n','19','/tmp/native-build-tools/ninja','-C',build,'-j1']+targets,'meson_build')
 commands=json.loads((build/'compile_commands.json').read_text())
 relevant=[c for c in commands if 'libSU2AccurateSum.a.p/' in c.get('output','') or any(p in c['file'] for p in ['CNativePartition3D_tests.cpp','CNativeMesh2D_tests.cpp'])]
 for c in relevant:
  if 'libSU2AccurateSum.a.p/' in c.get('output',''):
   command=c['command'];assert command.rfind('-fno-fast-math')>command.rfind(' -ffast-math') and '-ffp-contract=off' in command
 assert any('CNativePartition3D.cpp' in c['file'] for c in relevant)
 (folder/'actual_compile_commands.json').write_text(json.dumps(relevant,indent=2)+'\n')
 archive=build/'Common/src/libSU2AccurateSum.a'
 binary=folder/'test_actual_meson'
 run(['nice','-n','19','mpicxx','-std=c++17','-O0','-DHAVE_MPI','-DHAVE_PARMETIS','-DOMPI_SKIP_MPICXX=1','-Iexternals/catch2','-Iexternals/eigen','-fno-fast-math','-fno-unsafe-math-optimizations','-ffp-contract=off','UnitTests/test_driver.cpp']+[build/t for t in targets[1:]]+[archive]+objects+libs+['-Wl,--gc-sections','-o',binary],'link_actual_meson')
 for n in (1,2,4):
  label='mpi'+str(n);text=run(['nice','-n','19','mpirun','--oversubscribe','--bind-to','none','-np',str(n),binary,'[NativePartition3D],[NativeMesh2D]','--use-colour','no'],label)
  assert text.count('All tests passed')==n
  run(['nice','-n','19','python3','integration_evidence/native_3d_core_v1/audit_partition.py',folder/(label+'.stdout'),folder/(label+'_meshes'),str(n)],'audit_'+label)
 status='PASS'
finally:
 changed=[p for p,h in pins.items() if sha(root/p)!=h]
 if changed or any(sha(Path(p))!=h for p,h in object_pins.items()) or any(sha(Path(p))!=h for p,h in library_pins.items()):status='FAIL'
 result=dict(status=status,source_sha256=pins,changed_during_run=changed,object_sha256=object_pins,library_sha256=library_pins,partition_validation_sha256=sha(validated),stages=stages,
  scope='Actual SU2 Meson strict archive and actual Meson 3D partition/2D geometry test objects; link/run/audit MPI1/2/4. No full CFD lifecycle, native3D CGNS or runtime support qualification.')
 for p in [build/'Common/src/libSU2AccurateSum.a',folder/'test_actual_meson']:
  if p.exists():result[p.name+'_sha256']=sha(p)
 result['artifact_sha256']={str(p.relative_to(folder)):sha(p) for p in folder.rglob('*') if p.is_file() and p.name!='test_actual_meson'}
 (folder/'validation.json').write_text(json.dumps(result,indent=2)+'\n');print(status,folder,flush=True)
