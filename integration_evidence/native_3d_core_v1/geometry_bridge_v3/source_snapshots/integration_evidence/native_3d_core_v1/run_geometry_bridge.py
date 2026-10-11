"""Small sequential actual-SU2 geometry/solution bridge checks; no CFD iteration or full-driver claim."""
from pathlib import Path
import hashlib,json,os,shlex,subprocess,sys,time
root=Path(__file__).resolve().parents[2];folder=Path(sys.argv[1]).resolve()
print('Case working and retained folder:',folder,flush=True);folder.mkdir(exist_ok=False)
base=root/'integration_evidence/native_3d_core_v1/coplanar_regression_v2';build=root/'integration_evidence/build-integrated-v2'
receipt=json.loads((base/'validation.json').read_text());assert receipt['status']=='PASS'
core=[p for p in receipt['source_sha256'] if p.endswith(('.cpp','.hpp')) and not p.startswith('UnitTests/')]
new=['Common/src/adaptation/CNativeImport3D.cpp','Common/include/adaptation/CNativeImport3D.hpp',
 'Common/src/adaptation/CNativeBoundary3D.cpp','Common/include/adaptation/CNativeBoundary3D.hpp',
 'Common/src/adaptation/meson.build','UnitTests/meson.build','UnitTests/SU2_CFD/adaptation/CNativeImport3D_tests.cpp',
 'UnitTests/SU2_CFD/adaptation/TransferTestCase.hpp','UnitTests/Common/adaptation/SimplexMeshTestCase.hpp','UnitTests/test_driver.cpp',
 'integration_evidence/native_3d_core_v1/audit_coupled.py','integration_evidence/native_3d_core_v1/audit_cavities.py',
 'Common/src/adaptation/CReaderSlices.cpp','Common/src/adaptation/CNativePartition3D.cpp','Common/include/adaptation/CNativePartition3D.hpp',
 'integration_evidence/native_3d_core_v1/run_geometry_bridge.py']
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
for p in core:
 if p not in new:assert sha(root/p)==receipt['source_sha256'][p],p
pins={p:sha(root/p) for p in set(core+new)}
for p in new:
 q=folder/'source_snapshots'/p;q.parent.mkdir(parents=True,exist_ok=True);q.write_bytes((root/p).read_bytes())
units=['Common/src/adaptation/CNative'+name+'3D.cpp' for name in ['Predicates','Topology','Cavity','Field','Metric','Distributed','Donor','Engine']]
house=['CADTBaseClass','CADTElemClass','mpi_structure','CPassiveComm','CDistributedSearch','CTransferMemory']
objects=[base/(Path(p).stem+'.o') for p in units]+[base/(p+'.o') for p in house]
partition=root/'integration_evidence/native_3d_core_v1/partition_controls_v2/CNativePartition3D.o';objects.append(partition)
partition_receipt=json.loads((partition.parent/'validation.json').read_text())
for p in ['Common/src/adaptation/CNativePartition3D.cpp','Common/include/adaptation/CNativePartition3D.hpp']:assert pins[p]==partition_receipt['source_sha256'][p]
object_pins={str(p):sha(p) for p in objects}
lines=(build/'build.ninja').read_text().splitlines();i=next(i for i,l in enumerate(lines) if l.startswith('build UnitTests/test_driver:'))
arguments=next(l.split('=',1)[1].strip() for l in lines[i+1:i+8] if l.strip().startswith('LINK_ARGS ='))
libs=shlex.split(arguments);libs=[str(build/p) if p.endswith('.a') and not p.startswith('/') else p for p in libs if not p.startswith('-Wl,-rpath')]
library_pins={p:sha(Path(p)) for p in set(libs) if p.endswith('.a') and Path(p).exists()}
member_pins={}
for library in library_pins:
 with Path(library).open('rb') as archive:thin=archive.read(8)==b'!<thin>\n'
 if thin:
  for member in subprocess.check_output(['ar','t',library],cwd=root,text=True).splitlines():
   path=Path(member);path=path if path.is_absolute() else root/path
   member_pins[str(path)]=sha(path)
env=dict(os.environ,OMP_NUM_THREADS='1',OPENBLAS_NUM_THREADS='1',MKL_NUM_THREADS='1',OMPI_MCA_mpi_yield_when_idle='1',OMPI_MCA_hwloc_base_binding_policy='none',PYTHONDONTWRITEBYTECODE='1')
stages=[];status='FAIL'
def run(command,label,cwd=root):
 print('Starting',label,flush=True);start=time.monotonic();p=subprocess.run(list(map(str,command)),cwd=cwd,env=env,text=True,capture_output=True)
 (folder/(label+'.stdout')).write_text(p.stdout);(folder/(label+'.stderr')).write_text(p.stderr)
 stages.append(dict(stage=label,command=list(map(str,command)),cwd=str(cwd),seconds=time.monotonic()-start,exit_code=p.returncode))
 if p.returncode:raise RuntimeError(label+' failed; inspect '+str(folder))
 return p.stdout
try:
 for cmd,name in [(['uptime'],'host_load'),(['free','-m'],'host_memory'),(['ps','-eo','pid,ppid,ni,pcpu,etime,comm','--sort=-pcpu'],'host_processes')]:run(cmd,name)
 compiler=['nice','-n','19','mpicxx','-std=c++17','-DHAVE_MPI','-DHAVE_CGNS','-DHAVE_METIS','-DHAVE_PARMETIS','-DHAVE_MMG','-DOMPI_SKIP_MPICXX=1','-Iexternals/eigen','-Iexternals/metis/include','-Iexternals/parmetis/include','-fno-fast-math','-fno-unsafe-math-optimizations','-ffp-contract=off','-ffunction-sections','-fdata-sections']
 for unit in ['Common/src/adaptation/CNativeBoundary3D.cpp','Common/src/adaptation/CNativeImport3D.cpp']:
  obj=folder/(Path(unit).stem+'.o');run(compiler+['-O2','-Wall','-Wextra','-Werror','-Wno-unused-parameter','-Wno-deprecated-declarations','-c',unit,'-o',obj],'compile_'+Path(unit).stem);objects.append(obj)
 binary=folder/'test_geometry_bridge';run(compiler+['-O0','-Iexternals/catch2','UnitTests/test_driver.cpp',new[6]]+objects+libs+['-Wl,--gc-sections','-o',binary],'link')
 for n in (1,2,4):
  work=folder/('mpi'+str(n));work.mkdir();print('Case working and retained folder:',work,flush=True)
  run(['uptime'],'before_mpi'+str(n)+'_load');run(['free','-m'],'before_mpi'+str(n)+'_memory');run(['ps','-eo','pid,ppid,ni,pcpu,etime,comm','--sort=-pcpu'],'before_mpi'+str(n)+'_processes')
  out=run(['nice','-n','19','mpirun','--oversubscribe','--bind-to','none','-np',str(n),binary,'[NativeBridge3D]','--use-colour','no'],'mpi'+str(n),work)
  assert out.count('All tests passed')==n
  command=['nice','-n','19',sys.executable,'-c',"import sys,json;from pathlib import Path;sys.path.insert(0,sys.argv[1]);from audit_coupled import frame;p=Path(sys.argv[2]);r=[frame(f,True)[0] for f in sorted(p.glob('bridge*.json'))];assert r;print(json.dumps(r,indent=2))",str(root/'integration_evidence/native_3d_core_v1'),str(work)]
  run(command,'audit_mpi'+str(n))
 status='PASS'
finally:
 changed=[p for p,h in pins.items() if sha(root/p)!=h]
 if changed or any(sha(Path(p))!=h for p,h in object_pins.items()) or any(sha(Path(p))!=h for p,h in library_pins.items()) or any(sha(Path(p))!=h for p,h in member_pins.items()):status='FAIL'
 result=dict(status=status,source_sha256=pins,changed_during_run=changed,object_sha256=object_pins,library_sha256=library_pins,thin_archive_member_sha256=member_pins,stages=stages,
  scope='Actual SU2 owned tetrahedral geometry/sensor import, weighted M<=N worker adaptation, distributed original-P1 reader/metric return and actual geometry/solver affine solution plus BDF2 transfer. Reuses existing pinned core/house objects and SU2 library archives; no configured driver factory, CFD iterations, CGNS or performance qualification.')
 if (folder/'test_geometry_bridge').exists():result['binary_sha256']=sha(folder/'test_geometry_bridge')
 result['artifact_sha256']={str(p.relative_to(folder)):sha(p) for p in folder.rglob('*') if p.is_file() and p.suffix not in ('.o','.pyc') and p.name!='test_geometry_bridge'}
 (folder/'validation.json').write_text(json.dumps(result,indent=2)+'\n');print(status,folder,flush=True)
