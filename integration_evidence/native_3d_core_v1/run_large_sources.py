"""One sequential native3D large-source pilot at a time; full source and failure receipts."""
import hashlib,json,os,subprocess,sys,time
from pathlib import Path
root=Path(__file__).resolve().parents[2]
folder=Path(sys.argv[1]).resolve()
print('Case working and retained folder:',folder,flush=True)
folder.mkdir(exist_ok=False)
env=dict(os.environ,OMP_NUM_THREADS='1',OPENBLAS_NUM_THREADS='1',MKL_NUM_THREADS='1',
 OMPI_MCA_mpi_yield_when_idle='1',OMPI_MCA_hwloc_base_binding_policy='none',PYTHONDONTWRITEBYTECODE='1')
units=['Common/src/adaptation/CNative'+name+'3D.cpp' for name in
 ['Predicates','Topology','Cavity','Boundary','Field','Metric','Distributed','Donor','Engine']]
house=['Common/src/adt/CADTBaseClass.cpp','Common/src/adt/CADTElemClass.cpp',
 'Common/include/parallelization/mpi_structure.cpp','Common/src/parallelization/CPassiveComm.cpp',
 'Common/src/adaptation/CDistributedSearch.cpp','Common/src/adaptation/CTransferMemory.cpp']
headers=['Common/include/adaptation/CNative'+name+'3D.hpp' for name in
 ['Mesh','Topology','Cavity','Boundary','Field','Metric','Distributed','Donor','Engine']]
control='integration_evidence/native_3d_core_v1/large_source_probe.cpp'
files=units+house+headers+[control,str(Path(__file__).resolve().relative_to(root)),
 'integration_evidence/native_3d_core_v1/audit_large_sources.py',
 'integration_evidence/native_3d_core_v1/audit_coupled.py','integration_evidence/native_3d_core_v1/audit_cavities.py',
 'Common/include/adaptation/CNativeDistributed2D.hpp','Common/include/adaptation/CTransferMemory.hpp',
 'Common/include/adaptation/CDistributedSearch.hpp','Common/include/parallelization/CPassiveComm.hpp',
 'Common/include/parallelization/mpi_structure.hpp']
def pins():return {p:hashlib.sha256((root/p).read_bytes()).hexdigest() for p in files}
initial=pins();records=[];status='FAIL'
for path in files:
 saved=folder/'source_snapshots'/path;saved.parent.mkdir(parents=True,exist_ok=True);saved.write_bytes((root/path).read_bytes())
def host(label):
 for cmd,suffix in [(['uptime'],'load'),(['free','-m'],'memory'),
  (['ps','-eo','pid,ppid,ni,pcpu,args','--sort=-pcpu'],'processes')]:
  p=subprocess.run(cmd,capture_output=True,text=True)
  (folder/(label+'_'+suffix+'.txt')).write_text(p.stdout if suffix!='processes' else '\n'.join(p.stdout.splitlines()[:16])+'\n')
def run(cmd,label):
 print('Starting',label,flush=True);start=time.monotonic()
 p=subprocess.run(cmd,cwd=root,env=env,capture_output=True,text=True)
 (folder/(label+'.stdout')).write_text(p.stdout);(folder/(label+'.stderr')).write_text(p.stderr)
 records.append(dict(stage=label,command=list(map(str,cmd)),exit_code=p.returncode,seconds=time.monotonic()-start))
 if p.returncode:raise RuntimeError(label+' failed; retained in '+str(folder))
 return p.stdout
try:
 host('before_build');compiler=['nice','-n','19','mpicxx','-std=c++17','-DHAVE_MPI','-DOMPI_SKIP_MPICXX=1',
 '-I.','-Iexternals/eigen','-fno-fast-math','-fno-unsafe-math-optimizations','-ffp-contract=off',
 '-ffunction-sections','-fdata-sections','-O2']
 objects=[]
 for source in units+house:
  obj=folder/(Path(source).stem+'.o');flags=['-Wall','-Wextra','-Werror','-Wno-unused-parameter'] if source in units else []
  run(compiler+flags+['-c',source,'-o',str(obj)],'compile_'+Path(source).stem);objects.append(str(obj))
 binary=folder/'large_source_probe'
 run(compiler+['-Wall','-Wextra','-Werror','-Wno-unused-parameter',control]+objects+
 ['-Wl,--gc-sections','-o',str(binary)],'link')
 for ranks in [1,2,4]:
  case=folder/('mpi'+str(ranks)+'_meshes');case.mkdir()
  print('Case working and retained folder:',case,flush=True)
  host('before_mpi'+str(ranks));out=run(['nice','-n','19','/usr/bin/time','-v','mpirun','--oversubscribe',
  '--bind-to','none','-np',str(ranks),str(binary),str(case)],'mpi'+str(ranks))
  assert 'PASS large-source' in out
  run(['nice','-n','19',sys.executable,'integration_evidence/native_3d_core_v1/audit_large_sources.py',str(folder),str(ranks)],'audit_mpi'+str(ranks))
 status='PASS'
finally:
 final=pins();changed=[p for p in files if initial[p]!=final[p]]
 if changed:status='FAIL'
 result=dict(status=status,source_sha256=initial,final_source_sha256=final,changed_during_run=changed,stages=records,
 scope='Large connected immutable-source import/engine correctness pilot. No runtime/CFD/CGNS/partition/scaling qualification.')
 result['artifact_sha256']={str(p.relative_to(folder)):hashlib.sha256(p.read_bytes()).hexdigest()
 for p in folder.rglob('*') if p.is_file() and p.suffix!='.o' and p.name!='large_source_probe'}
 if (folder/'large_source_probe').exists():result['binary_sha256']=hashlib.sha256((folder/'large_source_probe').read_bytes()).hexdigest()
 (folder/'validation.json').write_text(json.dumps(result,indent=2)+'\n');print(status,folder,flush=True)
 if changed:raise RuntimeError('Source changed during run')
