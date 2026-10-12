"""Qualified current-binary aspect2 turning-direction wedge correctness; no timing claim."""
from pathlib import Path
import hashlib,json,os,re,subprocess,sys,time
root=Path('/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated');where=root/'integration_evidence/native_3d_core_v1'
folder=Path(__file__).resolve().parent;build=root/'integration_evidence/build-integrated-v2';binary=build/'UnitTests/test_driver'
sha=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest()
receipt_path=where/'query_routing_build_v1/validation.json';b=json.loads(receipt_path.read_text());assert b['status']=='PASS'
sys.path[:0]=[str(where),str(root/'integration_evidence/native_cluster_3d_core_v1')]
from run import command,SELECTION
from audit_lifecycle import parse,transport_counters
toolnames=('audit_envelope.py','audit_geometry_bridge.py','audit_lifecycle.py','audit_coupled.py','audit_cavities.py','audit_restart.py','export_envelope_vtu.py')
toolpins={n:sha(where/n) for n in toolnames}
package=root/'integration_evidence/native_cluster_3d_core_v1'
packagepins={n:sha(package/n) for n in ('run.py','mpi_profile.py','profile_summary.py')}
env=dict(os.environ,TMPDIR=str(folder),OMP_NUM_THREADS='1',OPENBLAS_NUM_THREADS='1',MKL_NUM_THREADS='1',OMPI_MCA_mpi_yield_when_idle='1',PYTHONDONTWRITEBYTECODE='1')
parameters=dict(ranks=1,workers=1,size=4,anisotropy=2,varying=1,direction=1,height_power=0,wedge=True,aligned_markers=True,diagnostic_rounds=512,repeat=0,detail=1,restart_format='none')
(folder/'parameters.json').write_text(json.dumps(parameters,indent=2)+'\n')
for key in tuple(env):
 if key.startswith('SU2_NATIVE3D_ENVELOPE_'):del env[key]
env.update({f'SU2_NATIVE3D_ENVELOPE_{k.upper()}':str(v) for k,v in dict(size=4,workers=1,anisotropy=2,varying=1,direction=1,height_power=0,wedge=1,aligned=1,rounds=512,repeat=0,detail=1,restart=0).items()})
(folder/'case_environment.json').write_text(json.dumps({k:v for k,v in env.items() if k.startswith('SU2_NATIVE3D_ENVELOPE_') or k in ('OMP_NUM_THREADS','OPENBLAS_NUM_THREADS','MKL_NUM_THREADS')},indent=2)+'\n')
source_deltas={n:sha(root/n) for n,h in b['source_sha256'].items() if sha(root/n)!=h}
assert not source_deltas,source_deltas
sourcepins={n:sha(root/n) for n in b['source_sha256']}
(folder/'source_boundary.json').write_text(json.dumps(dict(qualified_build_source_sha256=b['source_sha256'],current_source_sha256=sourcepins,source_changes_since_qualified_build=source_deltas,scope='Uses the newly qualified query_routing_build_v1 complete configured binary; current production/test source pins match exactly. Includes complete changed-star/facet/orientation filtering and bounded narrow actual-query donor routing. Frozen acceptance, target and round budget remain unchanged.'),indent=2)+'\n')
stages=[];status='FAIL';reason=''
def run(args,label):
 v=command(list(map(str,args)),folder,label,env,900);stages.append(v)
 if v['exit_code'] or v.get('timeout'):raise RuntimeError(label+' failed; original logs retained.')
 return (folder/(label+'.stdout')).read_text()
try:
 for args,label in [(['uptime'],'host_load'),(['free','-m'],'host_memory'),(['ps','-eo','pid,ppid,ni,pcpu,comm','--sort=-pcpu'],'host_processes')]:run(args,label)
 assert sha(binary)==b['binary_sha256'] and all(sha(root/n)==h for n,h in sourcepins.items())
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
           contention_authorized=True,scope='User explicitly requests correctness runs even under CPU contention; one actual MPI rank on one CPU, no timing/scaling claim')
 (folder/'host_gate.json').write_text(json.dumps(gate,indent=2)+'\n')
 memory={k:int(v.split()[0]) for k,v in (line.split(':',1) for line in Path('/proc/meminfo').read_text().splitlines())}
 if memory['MemAvailable']<2*1024*1024:raise RuntimeError('Insufficient available memory before MPI correctness launch.')
 assert parameters['diagnostic_rounds']==512 and parameters['height_power']==0 and parameters['anisotropy']==2
 assert env['SU2_NATIVE3D_ENVELOPE_HEIGHT_POWER']==str(parameters['height_power'])
 out=run(['taskset','-c',str(selected),'nice','-n','19','mpirun','--oversubscribe','--bind-to','none','-np','1',binary,'[NativeEnvelope3D]','--use-colour','no'],'case')
 assert len(re.findall(r'All tests passed \(\d+ assertions in 1 test case\)',out))==1
 run(['taskset','-c',str(selected),'nice','-n','19',sys.executable,where/'audit_envelope.py',folder],'independent_envelope')
 assert json.loads((folder/'independent_audit.json').read_text())['status']=='PASS'
 run(['taskset','-c',str(selected),'nice','-n','19',sys.executable,where/'export_envelope_vtu.py',folder],'vtu_export')
 status='PASS'
except Exception as e:
 reason=str(e);print(reason,flush=True)
finally:
 input_scope=dict(parameters=parameters,scope='NEW stronger metric correctness fixture: aspect2, spatial strength/turning direction, size4 wedge; q/J/L/noise/production512gates unchanged.')
 (folder/'input_scope.json').write_text(json.dumps(input_scope,indent=2)+'\n')
 source_drift=[n for n,h in sourcepins.items() if sha(root/n)!=h]
 object_drift=[n for n,h in b['actual_objects_sha256'].items() if sha(build/n)!=h]
 archive_drift=[n for n,h in {**b['library_sha256'],**b['thin_archive_member_sha256']}.items() if sha(n)!=h]
 tool_drift=[n for n,h in toolpins.items() if sha(where/n)!=h]+[str(package/n) for n,h in packagepins.items() if sha(package/n)!=h]
 if source_drift or object_drift or archive_drift or tool_drift or sha(binary)!=b['binary_sha256']:status='FAIL'
 v=dict(status=status,reason=reason,stages=stages,binary_sha256=sha(binary),build_validation_sha256=sha(receipt_path),
 source_drift=source_drift,object_drift=object_drift,archive_drift=archive_drift,tool_drift=tool_drift,tools_sha256=toolpins,package_tools_sha256=packagepins,source_boundary_sha256=sha(folder/'source_boundary.json'),
 scope='Actual newly qualified query_routing_build_v1 full68-object binary; whole aspect2 directional n4 wedge N1M1 production512 correctness, aspect2 rotated spatial original-P1 metric, three refine/coarsen/refine events, q>=.20 J>=.05 L<=1.8 rounds512 noise0. Independent exact geometry/markers/metric/current+BDF/SU2CGNS audits. One-CPU contended correctness only; no sustained speedup/scaling/allocator calibration or full Goal1 certificate. Includes complete changed-star/facet/orientation filtering and bounded narrow actual-query donor routing; frozen acceptance/target/round budget unchanged. Changed selection trajectory requires independent auditing.')
 v['artifact_sha256']={str(p.relative_to(folder)):sha(p) for p in folder.rglob('*') if p.is_file()}
 (folder/'validation.json').write_text(json.dumps(v,indent=2)+'\n');print(status,folder,flush=True)
raise SystemExit(status!='PASS')
