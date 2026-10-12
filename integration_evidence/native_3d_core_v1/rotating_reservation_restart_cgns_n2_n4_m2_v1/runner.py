"""Isolated rotating reservation whole-case correctness with fresh independent mesh/state/output audits."""
from pathlib import Path
import hashlib,json,os,re,subprocess,sys,time
root=Path('/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated');where=root/'integration_evidence/native_3d_core_v1'
folder=Path(__file__).resolve().parent;build=root/'integration_evidence/build-integrated-v2';binary=build/'UnitTests/test_driver'
sha=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest()
assert not (folder/'validation.json').exists(),'Keep terminal evidence unchanged.'
receipt_path=where/'completion_diagnostic_build_v1/validation.json';b=json.loads(receipt_path.read_text());assert b['status']=='PASS'
production_binary=binary;production_sha=b['binary_sha256']
variant_folder=where/'rotating_reservation_whole_build_v1'
variant_path=variant_folder/'validation.json';variant_receipt=json.loads(variant_path.read_text());assert variant_receipt['status']=='PASS'
variant_pins={variant_path:sha(variant_path),variant_folder/'directory_rotating.cpp':sha(variant_folder/'directory_rotating.cpp')}
assert variant_pins[variant_folder/'directory_rotating.cpp']==sha(where/'rotating_reservation_probe_v1/directory_rotating.cpp')
b=dict(b,binary_sha256=variant_receipt['binary_sha256']);binary=variant_folder/'test_driver_rotating'
assert sha(production_binary)==production_sha

sys.path[:0]=[str(where),str(root/'integration_evidence/native_cluster_3d_core_v1')]
from run import command,SELECTION
from audit_lifecycle import parse,transport_counters
toolnames=('audit_envelope.py','audit_geometry_bridge.py','audit_lifecycle.py','audit_coupled.py','audit_cavities.py','audit_restart.py','export_envelope_vtu.py')
toolpins={n:sha(where/n) for n in toolnames}
package=root/'integration_evidence/native_cluster_3d_core_v1'
packagepins={n:sha(package/n) for n in ('run.py','mpi_profile.py','profile_summary.py')}
env=dict(os.environ,TMPDIR=str(folder),OMP_NUM_THREADS='1',OPENBLAS_NUM_THREADS='1',MKL_NUM_THREADS='1',OMPI_MCA_mpi_yield_when_idle='1',PYTHONDONTWRITEBYTECODE='1')
parameters=dict(ranks=4,workers=2,size=2,anisotropy=1,varying=1,direction=1,height_power=0,wedge=True,aligned_markers=True,diagnostic_rounds=512,repeat=0,detail=1,restart_format='cgns')
(folder/'parameters.json').write_text(json.dumps(parameters,indent=2)+'\n')
for key in tuple(env):
 if key.startswith('SU2_NATIVE3D_ENVELOPE_'):del env[key]
env.update({f'SU2_NATIVE3D_ENVELOPE_{k.upper()}':str(v) for k,v in dict(size=2,workers=2,anisotropy=1,varying=1,direction=1,height_power=0,wedge=1,aligned=1,rounds=512,repeat=0,detail=1,restart=2).items()})
(folder/'case_environment.json').write_text(json.dumps({k:v for k,v in env.items() if k.startswith('SU2_NATIVE3D_ENVELOPE_') or k in ('OMP_NUM_THREADS','OPENBLAS_NUM_THREADS','MKL_NUM_THREADS')},indent=2)+'\n')
baseline=root/'integration_evidence/native_3d_core_v1/reference_admission_restart_cgns_n2_n4_m2_v1'
source_deltas={n:sha(root/n) for n,h in b['source_sha256'].items() if sha(root/n)!=h}
assert not source_deltas,source_deltas
sourcepins={n:sha(root/n) for n in b['source_sha256']}
(folder/'source_boundary.json').write_text(json.dumps(dict(qualified_build_source_sha256=b['source_sha256'],current_source_sha256=sourcepins,source_changes_since_qualified_build=source_deltas,scope='Normal completion-diagnostic source/object/archive pins are unchanged. Isolated directory rotates reservation priority with a checked phase word in existing admission votes. Frozen targets and acceptance/512round budget unchanged.'),indent=2)+'\n')
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
 gate=dict(cpus=os.cpu_count(),allowed=allowed,idle_samples=samples,load1=os.getloadavg()[0],ranks=4,
           affinity=[selected],owned_maximum_compute_cpus=1,nice=19,threads=1,
           contention_authorized=True,scope='User explicitly requests correctness runs even under CPU contention; 4 actual MPI ranks on one CPU, no timing/scaling claim')
 (folder/'host_gate.json').write_text(json.dumps(gate,indent=2)+'\n')
 memory={k:int(v.split()[0]) for k,v in (line.split(':',1) for line in Path('/proc/meminfo').read_text().splitlines())}
 if memory['MemAvailable']<2*1024*1024:raise RuntimeError('Insufficient available memory before MPI correctness launch.')
 assert all(value==json.loads((baseline/'parameters.json').read_text())[key] for key,value in parameters.items())
 out=run(['taskset','-c',str(selected),'nice','-n','19','mpirun','--oversubscribe','--bind-to','none','-np','4',binary,'[NativeEnvelope3D]','--use-colour','no'],'case')
 assert len(re.findall(r'All tests passed \(\d+ assertions in 1 test case\)',out))==4
 run(['taskset','-c',str(selected),'nice','-n','19',sys.executable,where/'audit_envelope.py',folder],'independent_envelope')
 assert json.loads((folder/'independent_audit.json').read_text())['status']=='PASS'
 run(['taskset','-c',str(selected),'nice','-n','19',sys.executable,where/'export_envelope_vtu.py',folder],'vtu_export')
 status='PASS'
except Exception as e:
 reason=str(e);print(reason,flush=True)
finally:
 baseline_inputs={}
 for name in ('envelope.cfg','envelope_input.su2','envelope_s0_initial.json'):
  prior=baseline/name; actual=folder/name
  if prior.is_file() and actual.is_file():baseline_inputs[name]=dict(baseline_sha256=sha(prior),current_sha256=sha(actual),equal=sha(prior)==sha(actual))
 if baseline_inputs and not all(row['equal'] for row in baseline_inputs.values()):status='FAIL';reason+=' Frozen baseline input drift.'
 (folder/'baseline_comparison.json').write_text(json.dumps(baseline_inputs,indent=2)+'\n')
 source_drift=[n for n,h in sourcepins.items() if sha(root/n)!=h]
 object_drift=[n for n,h in b['actual_objects_sha256'].items() if sha(build/n)!=h]
 archive_drift=[n for n,h in {**b['library_sha256'],**b['thin_archive_member_sha256']}.items() if sha(n)!=h]
 tool_drift=[n for n,h in toolpins.items() if sha(where/n)!=h]+[str(package/n) for n,h in packagepins.items() if sha(package/n)!=h]
 variant_drift=[str(p) for p,h in variant_pins.items() if sha(p)!=h]
 if source_drift or object_drift or archive_drift or tool_drift or variant_drift or sha(binary)!=b['binary_sha256'] or sha(production_binary)!=production_sha:status='FAIL'
 v=dict(status=status,reason=reason,stages=stages,binary_sha256=sha(binary),build_validation_sha256=sha(receipt_path),isolated_variant_receipt_sha256=sha(variant_path),isolated_variant_source_sha256=sha(variant_folder/'directory_rotating.cpp'),variant_drift=variant_drift,production_binary_sha256=sha(production_binary),
 source_drift=source_drift,object_drift=object_drift,archive_drift=archive_drift,tool_drift=tool_drift,tools_sha256=toolpins,package_tools_sha256=packagepins,source_boundary_sha256=sha(folder/'source_boundary.json'),
 scope='Isolated rotating-priority complete68-object driver on qualified normal completion-diagnostic objects. Actual three refine/coarsen/refine events with frozen original-P1 sensors, q>=.20 J>=.05 L<=1.8 rounds512 noise0. Fresh exact geometry/markers/metric/current+BDF/SU2CGNS audits. Changed mesh trajectory, not fixed-work speed evidence; one-CPU contended correctness only. Production source/binary/objects/archives unchanged; no fullGoal1/scaling/allocator qualification.')
 v['artifact_sha256']={str(p.relative_to(folder)):sha(p) for p in folder.rglob('*') if p.is_file()}
 (folder/'validation.json').write_text(json.dumps(v,indent=2)+'\n');print(status,folder,flush=True)
raise SystemExit(status!='PASS')
