"""Resource-gated actual one-rank lifecycle/serialized-transport correctness, not local performance certification."""
from pathlib import Path
import hashlib,json,os,subprocess,sys,time
root=Path('/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated');where=root/'integration_evidence/native_3d_core_v1'
folder=Path(__file__).resolve().parent;build=root/'integration_evidence/build-integrated-v2';binary=build/'UnitTests/test_driver'
sha=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest()
receipt_path=where/'box_filter_build_v2/validation.json';b=json.loads(receipt_path.read_text());assert b['status']=='PASS'
sys.path[:0]=[str(where),str(root/'integration_evidence/native_cluster_3d_core_v1')]
from run import command
from audit_lifecycle import parse,nested_stage,transport_counters,termination_counters,coarsening_counters
from profile_summary import profiles
toolnames=['audit_lifecycle.py','audit_geometry_bridge.py','audit_coupled.py','audit_cavities.py']
toolpins={n:sha(where/n) for n in toolnames};profile_pin=sha(root/'integration_evidence/native_cluster_3d_core_v1/profile_summary.py')
env=dict(os.environ,TMPDIR=str(folder),OMP_NUM_THREADS='1',OPENBLAS_NUM_THREADS='1',MKL_NUM_THREADS='1',OMPI_MCA_mpi_yield_when_idle='1',PYTHONDONTWRITEBYTECODE='1')
stages=[];status='FAIL';reason=''
def run(args,label):
 v=command(list(map(str,args)),folder,label,env,180);stages.append(v)
 if v['exit_code'] or v.get('timeout'):raise RuntimeError(label+' failed; logs retained.')
 return (folder/(label+'.stdout')).read_text()
try:
 for args,label in [(['uptime'],'host_load'),(['free','-m'],'host_memory'),(['ps','-eo','pid,ppid,ni,pcpu,comm','--sort=-pcpu'],'host_processes')]:run(args,label)
 read=lambda:list(map(int,Path('/proc/stat').read_text().splitlines()[0].split()[1:]))
 a=read();time.sleep(2);d=[y-x for x,y in zip(a,read())]
 gate=dict(cpus=os.cpu_count(),idle_fraction=(d[3]+d[4])/sum(d),load1=os.getloadavg()[0])
 (folder/'host_gate.json').write_text(json.dumps(gate,indent=2)+'\n')
 memory={k:int(v.split()[0]) for k,v in (line.split(':',1) for line in Path('/proc/meminfo').read_text().splitlines())}
 if memory['MemAvailable']<2*1024*1024:raise RuntimeError('Insufficient available memory before correctness launch.')
 selected=max(os.sched_getaffinity(0),key=lambda cpu:next((float(r['idle_samples'][-1]['cpu'+str(cpu)]) for r in [json.loads((where/'query_routing_mpi4_v1/host_gate.json').read_text())]),0))
 os.sched_setaffinity(0,{selected})
 gate.update(available_memory_kib=memory['MemAvailable'],affinity=[selected],contention_authorized=True,owned_maximum_compute_cpus=1,nice=19,threads=1)
 (folder/'host_gate.json').write_text(json.dumps(gate,indent=2)+'\n')
 assert sha(binary)==b['binary_sha256'] and all(sha(root/n)==h for n,h in b['source_sha256'].items())
 out=run(['nice','-n','19','mpirun','--bind-to','none','-np','1',binary,'[NativeLifecycle3D]','--use-colour','no'],'case')
 assert out.count('All tests passed')==1 and 'No test cases matched' not in out
 run(['nice','-n','19',sys.executable,where/'audit_lifecycle.py',folder,'1',folder/'case.stdout'],'independent_lifecycle')
 audited=json.loads((folder/'lifecycle_audit.json').read_text());assert audited['status']=='PASS'
 native=[parse(s) for s in out.splitlines() if s.startswith('NATIVE3D ')]
 details=[parse(s) for s in out.splitlines() if s.startswith('NATIVE3D_DETAIL ')]
 assert len(native)==6 and all(transport_counters(r) for r in native)
 engines=[r for r in details if r['stage'] in ('refine','coarsen')]
 assert len(engines)==6 and all(termination_counters(r) and coarsening_counters(r) and r['complete']==1 for r in engines)
 assert all(transport_counters(r) for r in details if r['stage'] in ('refine','coarsen','import','weights','return'))
 summary=profiles(native,audited['rank_stages'],details)
 (folder/'rank_profiles.json').write_text(json.dumps(summary,indent=2)+'\n')
 status='PASS'
except Exception as e:
 reason=str(e);print(reason,flush=True)
finally:
 source_drift=[n for n,h in b['source_sha256'].items() if sha(root/n)!=h]
 object_drift=[n for n,h in b['actual_objects_sha256'].items() if sha(build/n)!=h]
 archive_drift=[n for n,h in {**b['library_sha256'],**b['thin_archive_member_sha256']}.items() if sha(n)!=h]
 tool_drift=[n for n,h in toolpins.items() if sha(where/n)!=h]
 if source_drift or object_drift or archive_drift or tool_drift or sha(binary)!=b['binary_sha256'] or sha(root/'integration_evidence/native_cluster_3d_core_v1/profile_summary.py')!=profile_pin:status='FAIL'
 v=dict(status=status,reason=reason,stages=stages,binary_sha256=sha(binary),build_validation_sha256=sha(receipt_path),
 source_drift=source_drift,object_drift=object_drift,archive_drift=archive_drift,tool_drift=tool_drift,
 tools_sha256=toolpins,profile_summary_sha256=profile_pin,
 scope='One MPI rank, existing forced-size faceted lifecycle fixture: six fresh OFF/ON controls use actual metric/Hessian construction, weighted native remesh and CFD geometry/solver/BDF replacement, mesh output and independent audits. Validates complete termination and changed-star/facet/orientation accounting on six successful engine rows, serialized wrapper totals and closed wall/CPU/rank/query profiles. Contended correctness only: repetitions are built into the fixture and do not certify local timing/scaling/affordability, MPI2/4, n4/persisted restart or fullGoal1.')
 v['artifact_sha256']={str(p.relative_to(folder)):sha(p) for p in folder.rglob('*') if p.is_file()}
 (folder/'validation.json').write_text(json.dumps(v,indent=2)+'\n');print(status,folder,flush=True)
raise SystemExit(status!='PASS')
