"""Actual contention-authorized one-CPU low-priority 1-rank native3D regression with independent M=N/M<N bridge audit."""
from pathlib import Path
import hashlib,json,os,re,subprocess,sys,time
root=Path('/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated');where=root/'integration_evidence/native_3d_core_v1'
folder=Path(__file__).resolve().parent;build=root/'integration_evidence/build-integrated-v2';binary=build/'UnitTests/test_driver'
sha=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest()
receipt_path=where/'completion_diagnostic_build_v1/validation.json';b=json.loads(receipt_path.read_text());assert b['status']=='PASS'
sys.path[:0]=[str(where),str(root/'integration_evidence/native_cluster_3d_core_v1')]
from run import command,SELECTION,distributed_audits
from audit_lifecycle import parse,transport_counters
toolnames=('audit_geometry_bridge.py','audit_lifecycle.py','audit_coupled.py','audit_cavities.py','audit_distributed.py','audit_donors.py','audit_anisotropic.py','audit_cycles.py','audit_repairs.py','audit_engine_repairs.py','audit_coplanar.py')
toolpins={n:sha(where/n) for n in toolnames}
package=root/'integration_evidence/native_cluster_3d_core_v1'
packagepins={n:sha(package/n) for n in ('run.py','mpi_profile.py','profile_summary.py')}
env=dict(os.environ,TMPDIR=str(folder),OMP_NUM_THREADS='1',OPENBLAS_NUM_THREADS='1',MKL_NUM_THREADS='1',OMPI_MCA_mpi_yield_when_idle='1',PYTHONDONTWRITEBYTECODE='1')
stages=[];status='FAIL';reason=''
def run(args,label):
 v=command(list(map(str,args)),folder,label,env,900);stages.append(v)
 if v['exit_code'] or v.get('timeout'):raise RuntimeError(label+' failed; original logs retained.')
 return (folder/(label+'.stdout')).read_text()
try:
 for args,label in [(['uptime'],'host_load'),(['free','-m'],'host_memory'),(['ps','-eo','pid,ppid,ni,pcpu,comm','--sort=-pcpu'],'host_processes')]:run(args,label)
 assert sha(binary)==b['binary_sha256'] and all(sha(root/n)==h for n,h in b['source_sha256'].items())
 assert all(sha(build/n)==h for n,h in b['actual_objects_sha256'].items())
 assert all(sha(n)==h for n,h in {**b['library_sha256'],**b['thin_archive_member_sha256']}.items())
 allowed=sorted(os.sched_getaffinity(0))
 def read():
  return {line.split()[0]:list(map(int,line.split()[1:])) for line in Path('/proc/stat').read_text().splitlines() if line.startswith('cpu')}
 samples=[]
 for _ in range(3):
  a=read();time.sleep(2);now=read();row={}
  for key in a:
   d=[y-x for x,y in zip(a[key],now[key])];row[key]=(d[3]+d[4])/sum(d) if sum(d) else 0
  samples.append(row)
 selected=max(allowed,key=lambda cpu:min(row['cpu'+str(cpu)] for row in samples))
 gate=dict(cpus=os.cpu_count(),allowed=allowed,idle_samples=samples,load1=os.getloadavg()[0],ranks=1,
           affinity=[selected],owned_maximum_compute_cpus=1,nice=19,threads=1,
           contention_authorized=True,scope='User explicitly requests correctness runs even under CPU contention; 1 actual MPI ranks on one CPU, no timing/scaling claim')
 (folder/'host_gate.json').write_text(json.dumps(gate,indent=2)+'\n')
 memory={k:int(v.split()[0]) for k,v in (line.split(':',1) for line in Path('/proc/meminfo').read_text().splitlines())}
 if memory['MemAvailable']<2*1024*1024:raise RuntimeError('Insufficient available memory before MPI correctness launch.')
 out=run(['taskset','-c',str(selected),'nice','-n','19','mpirun','--oversubscribe','--bind-to','none','-np','1',binary,SELECTION,'--use-colour','no'],'case')
 assert len(re.findall(r'All tests passed \(\d+ assertions in 97 test cases\)',out))==1
 run(['taskset','-c',str(selected),'nice','-n','19',sys.executable,where/'audit_geometry_bridge.py',folder,'1',folder/'case.stdout'],'independent_bridge')
 audited=json.loads((folder/'independent_audit.json').read_text());assert audited['status']=='PASS'
 os.sched_setaffinity(0,{selected});os.nice(19)
 stages.extend(distributed_audits(folder,1,where,env,900))
 assert json.loads((folder/'distributed_audit.json').read_text())['status']=='PASS'
 rows=[parse(s) for s in out.splitlines() if s.startswith('NATIVE3D ')]
 assert len(rows)==2 and all(transport_counters(r) for r in rows)
 for rank in range(1):assert [r['complete'] for r in rows if r['rank']==rank]==[0,1]
 (folder/'transport_rows.json').write_text(json.dumps(rows,indent=2)+'\n')
 mmg=run(['taskset','-c',str(selected),'nice','-n','19','mpirun','--oversubscribe','--bind-to','none','-np','1',binary,'MMG interface: 2D round trip,MMG interface: 3D round trip','--use-colour','no'],'mmg_serial')
 assert 'All tests passed (48 assertions in 2 test cases)' in mmg
 status='PASS'
except Exception as e:
 reason=str(e);print(reason,flush=True)
finally:
 source_drift=[n for n,h in b['source_sha256'].items() if sha(root/n)!=h]
 object_drift=[n for n,h in b['actual_objects_sha256'].items() if sha(build/n)!=h]
 archive_drift=[n for n,h in {**b['library_sha256'],**b['thin_archive_member_sha256']}.items() if sha(n)!=h]
 tool_drift=[n for n,h in toolpins.items() if sha(where/n)!=h]+[str(package/n) for n,h in packagepins.items() if sha(package/n)!=h]
 if source_drift or object_drift or archive_drift or tool_drift or sha(binary)!=b['binary_sha256']:status='FAIL'
 v=dict(status=status,reason=reason,stages=stages,binary_sha256=sha(binary),build_validation_sha256=sha(receipt_path),
 source_drift=source_drift,object_drift=object_drift,archive_drift=archive_drift,tool_drift=tool_drift,tools_sha256=toolpins,package_tools_sha256=packagepins,
 scope='Full configured68-object driver with dependency-correct coarsening notifications and full local-star exact facet/orientation filtering and bounded grouped actual-query donor requests and conservative projected-box rejection; actual MPI1 selected97 cases per rank, originalP1/private-query/surface/complete-dependency/transaction/refusal controls, default accepted/rejected transport closure. Independent exact bridge geometry/markers/metric/current+BDF/SU2CGNS plus seven complete distributed-record suites over the supported worker subsets. All29 donor/engine/directory and3 partition declarations covered;24 were outside the prior73-case selection. Exhaustive grouped query request checks include non-dyadic endpoints, rotated thin/empty owners and one request per cell/owner. Uses newly configured completion_diagnostic_build_v1 binary/source/object/archive pins. User-authorized one-CPU contended correctness run with taskset/nice19/yield/thread1; MPI rank count is 1 but CPU allocation one. No instrumentation overhead/scaling claim, not larger n4/persisted-restart/repeated timing/scaling/fullGoal1.')
 v['artifact_sha256']={str(p.relative_to(folder)):sha(p) for p in folder.rglob('*') if p.is_file()}
 (folder/'validation.json').write_text(json.dumps(v,indent=2)+'\n');print(status,folder,flush=True)
raise SystemExit(status!='PASS')
