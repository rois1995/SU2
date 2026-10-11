"""Sequential actual (Par)METIS native3D controls reusing unchanged, pinned native core objects."""
import hashlib,json,os,subprocess,sys,time
from pathlib import Path
root=Path(__file__).resolve().parents[2]
base=Path(sys.argv[1]).resolve();folder=Path(sys.argv[2]).resolve()
print('Case working and retained folder:',folder,flush=True);folder.mkdir(exist_ok=False)
receipt=json.loads((base/'validation.json').read_text());assert receipt['status']=='PASS' and not receipt['changed_during_run']
units=['Common/src/adaptation/CNative'+name+'3D.cpp' for name in ['Predicates','Topology','Cavity','Boundary','Field','Metric','Distributed','Donor','Engine']]
house=['Common/src/adt/CADTBaseClass.cpp','Common/src/adt/CADTElemClass.cpp','Common/include/parallelization/mpi_structure.cpp',
       'Common/src/parallelization/CPassiveComm.cpp','Common/src/adaptation/CDistributedSearch.cpp','Common/src/adaptation/CTransferMemory.cpp']
core=[p for p in receipt['source_sha256'] if p.endswith(('.cpp','.hpp')) and not p.startswith('UnitTests/')]
new=['Common/src/adaptation/CNativePartition3D.cpp','Common/include/adaptation/CNativePartition3D.hpp',
     'UnitTests/Common/adaptation/CNativePartition3D_tests.cpp','Common/src/meson.build','UnitTests/meson.build',
     'integration_evidence/native_3d_core_v1/audit_partition.py','integration_evidence/native_3d_core_v1/run_partition.py',
     'integration_evidence/native_3d_core_v1/audit_coupled.py','integration_evidence/native_3d_core_v1/audit_cavities.py','UnitTests/test_driver.cpp']
libs=[root/'integration_evidence/build-integrated-v2/externals/parmetis/libparmetis.a',root/'integration_evidence/build-integrated-v2/externals/metis/libmetis.a']
headers=['externals/metis/include/metis.h','externals/parmetis/include/parmetis.h']
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
for p in core:assert sha(root/p)==receipt['source_sha256'][p],p
pins={p:sha(root/p) for p in core+new+headers};objects=[base/(Path(p).stem+'.o') for p in units+house]
serial_base=base/'actual_nonmpi';serial_receipt=json.loads((serial_base/'validation.json').read_text())
assert serial_receipt['status']=='PASS' and not serial_receipt['changed_during_run']
for p in core:assert sha(root/p)==serial_receipt['source_sha256'][p],p
serial_objects=[serial_base/(Path(p).stem+'.o') for p in units+house]
object_pins={str(p):sha(p) for p in objects+serial_objects};library_pins={str(p):sha(p) for p in libs}
for p in new:
 q=folder/'source_snapshots'/p;q.parent.mkdir(parents=True,exist_ok=True);q.write_bytes((root/p).read_bytes())
env=dict(os.environ,OMP_NUM_THREADS='1',OPENBLAS_NUM_THREADS='1',MKL_NUM_THREADS='1',OMPI_MCA_mpi_yield_when_idle='1',OMPI_MCA_hwloc_base_binding_policy='none',PYTHONDONTWRITEBYTECODE='1')
stages=[];status='FAIL'
def run(command,label):
 print('Starting',label,flush=True);started=time.monotonic()
 p=subprocess.run(command,cwd=root,env=env,text=True,capture_output=True)
 (folder/(label+'.stdout')).write_text(p.stdout);(folder/(label+'.stderr')).write_text(p.stderr)
 stages.append(dict(stage=label,command=list(map(str,command)),exit_code=p.returncode,seconds=time.monotonic()-started))
 if p.returncode:raise RuntimeError(label+' failed; evidence retained '+str(folder))
 return p.stdout
def host(label):
 for c,s in [(['uptime'],'load'),(['free','-m'],'memory'),(['ps','-eo','pid,ppid,ni,pcpu,comm','--sort=-pcpu'],'processes')]:
  p=subprocess.run(c,text=True,capture_output=True);(folder/(label+'_'+s+'.txt')).write_text(p.stdout if s!='processes' else '\n'.join(p.stdout.splitlines()[:16])+'\n')
try:
 host('before_build')
 compiler=['nice','-n','19','mpicxx','-std=c++17','-DHAVE_MPI','-DHAVE_PARMETIS','-DOMPI_SKIP_MPICXX=1',
           '-Iexternals/eigen','-Iexternals/parmetis/include','-Iexternals/metis/include','-fno-fast-math',
           '-fno-unsafe-math-optimizations','-ffp-contract=off','-ffunction-sections','-fdata-sections']
 obj=folder/'CNativePartition3D.o'
 run(compiler+['-O2','-Wall','-Wextra','-Werror','-Wno-unused-parameter','-c',new[0],'-o',str(obj)],'compile_partition')
 binary=folder/'test_partition'
 run(compiler+['-O0','-Iexternals/catch2','UnitTests/test_driver.cpp',new[2],str(obj)]+list(map(str,objects+libs))+
     ['-lm','-Wl,--gc-sections','-o',str(binary)],'link')
 for ranks in (1,2,4):
  for repeat in (1,2,3):
   label='mpi%d_repeat%d'%(ranks,repeat);host('before_'+label)
   output=run(['nice','-n','19','mpirun','--oversubscribe','--bind-to','none','-np',str(ranks),str(binary),'[NativePartition3D]','--use-colour','no'],label)
   assert output.count('All tests passed')==ranks
   run(['nice','-n','19',sys.executable,new[5],str(folder/(label+'.stdout')),str(folder/(label+'_meshes')),str(ranks)],'audit_'+label)
 host('before_nonmpi')
 passive=['nice','-n','19','g++','-std=c++17','-Iexternals/eigen','-fno-fast-math','-fno-unsafe-math-optimizations','-ffp-contract=off','-ffunction-sections','-fdata-sections']
 serial_obj=folder/'CNativePartition3D_nonmpi.o';serial_binary=folder/'test_partition_nonmpi'
 run(passive+['-O2','-Wall','-Wextra','-Werror','-Wno-unused-parameter','-c',new[0],'-o',str(serial_obj)],'compile_nonmpi')
 run(passive+['-O0','-Iexternals/catch2','UnitTests/test_driver.cpp',new[2],str(serial_obj)]+list(map(str,serial_objects))+['-Wl,--gc-sections','-o',str(serial_binary)],'link_nonmpi')
 output=run(['nice','-n','19',str(serial_binary),'[NativePartition3D]','--use-colour','no'],'nonmpi')
 assert output.count('All tests passed')==1
 run(['nice','-n','19',sys.executable,new[5],str(folder/'nonmpi.stdout'),str(folder/'nonmpi_meshes'),'1','nonmpi'],'audit_nonmpi')
 status='PASS'
finally:
 changed=[p for p,h in pins.items() if sha(root/p)!=h]
 if changed or any(sha(Path(p))!=h for p,h in object_pins.items()) or any(sha(Path(p))!=h for p,h in library_pins.items()):status='FAIL'
 result=dict(status=status,base_commit=subprocess.check_output(['git','rev-parse','HEAD'],cwd=root,text=True).strip(),
  source_validation_sha256=sha(base/'validation.json'),serial_source_validation_sha256=sha(serial_base/'validation.json'),source_sha256=pins,object_sha256=object_pins,library_sha256=library_pins,
  changed_during_run=changed,stages=stages,
  scope='Actual new native3D weight/dual-graph/atomic (Par)METIS migration and empty-rank/explicit subset controls, reusing unchanged source-pinned core objects. Three sequential MPI1/2/4 worker migration/adaptation/return repeats with unchanged original sensor records, and an actual nonMPI partition control. Imported auditor sources pinned; four independent auditor adversaries. Source/binary/library pins; no production CFD/CGNS or performance scaling certificate.')
 for name in ('test_partition','test_partition_nonmpi'):
  if (folder/name).exists():result[name+'_sha256']=sha(folder/name)
 result['artifact_sha256']={str(p.relative_to(folder)):sha(p) for p in folder.rglob('*') if p.is_file() and p.suffix!='.o' and p.name not in ('test_partition','test_partition_nonmpi')}
 (folder/'validation.json').write_text(json.dumps(result,indent=2)+'\n');print(status,folder,flush=True)
