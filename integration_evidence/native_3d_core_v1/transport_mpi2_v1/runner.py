"""Actual low-priority two-rank native3D regression with independent M=N/M<N bridge audit."""
from pathlib import Path
import hashlib,json,os,re,subprocess,sys,time
root=Path('/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated');where=root/'integration_evidence/native_3d_core_v1'
folder=Path(__file__).resolve().parent;build=root/'integration_evidence/build-integrated-v2';binary=build/'UnitTests/test_driver'
sha=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest()
receipt_path=where/'transport_build_v1/validation.json';b=json.loads(receipt_path.read_text());assert b['status']=='PASS'
sys.path[:0]=[str(where),str(root/'integration_evidence/native_cluster_3d_core_v1')]
from run import command,SELECTION
from audit_lifecycle import parse,transport_counters
toolnames=('audit_geometry_bridge.py','audit_lifecycle.py','audit_coupled.py','audit_cavities.py')
toolpins={n:sha(where/n) for n in toolnames}
env=dict(os.environ,TMPDIR=str(folder),OMP_NUM_THREADS='1',OPENBLAS_NUM_THREADS='1',MKL_NUM_THREADS='1',OMPI_MCA_mpi_yield_when_idle='1',PYTHONDONTWRITEBYTECODE='1')
stages=[];status='FAIL';reason=''
def run(args,label):
 v=command(list(map(str,args)),folder,label,env,180);stages.append(v)
 if v['exit_code'] or v.get('timeout'):raise RuntimeError(label+' failed; original logs retained.')
 return (folder/(label+'.stdout')).read_text()
try:
 for args,label in [(['uptime'],'host_load'),(['free','-m'],'host_memory'),(['ps','-eo','pid,ppid,ni,pcpu,comm','--sort=-pcpu'],'host_processes')]:run(args,label)
 read=lambda:list(map(int,Path('/proc/stat').read_text().splitlines()[0].split()[1:]))
 samples=[]
 for _ in range(3):
  a=read();time.sleep(2);d=[y-x for x,y in zip(a,read())];samples.append((d[3]+d[4])/sum(d))
 gate=dict(cpus=os.cpu_count(),idle_samples=samples,load1=os.getloadavg()[0],desired_ranks=2)
 (folder/'host_gate.json').write_text(json.dumps(gate,indent=2)+'\n')
 if min(samples)<.30 or gate['load1']>=gate['cpus']-1:raise RuntimeError('Resource gate refused before MPI launch.')
 assert sha(binary)==b['binary_sha256'] and all(sha(root/n)==h for n,h in b['source_sha256'].items())
 out=run(['nice','-n','19','mpirun','--bind-to','none','-np','2',binary,SELECTION,'--use-colour','no'],'case')
 assert len(re.findall(r'All tests passed \(\d+ assertions in 69 test cases\)',out))==2
 run(['nice','-n','19',sys.executable,where/'audit_geometry_bridge.py',folder,'2',folder/'case.stdout'],'independent_bridge')
 audited=json.loads((folder/'independent_audit.json').read_text());assert audited['status']=='PASS'
 rows=[parse(s) for s in out.splitlines() if s.startswith('NATIVE3D ')]
 assert len(rows)==4 and all(transport_counters(r) for r in rows)
 for rank in (0,1):assert [r['complete'] for r in rows if r['rank']==rank]==[0,1]
 (folder/'transport_rows.json').write_text(json.dumps(rows,indent=2)+'\n')
 status='PASS'
except Exception as e:
 reason=str(e);print(reason,flush=True)
finally:
 source_drift=[n for n,h in b['source_sha256'].items() if sha(root/n)!=h]
 object_drift=[n for n,h in b['actual_objects_sha256'].items() if sha(build/n)!=h]
 archive_drift=[n for n,h in {**b['library_sha256'],**b['thin_archive_member_sha256']}.items() if sha(n)!=h]
 tool_drift=[n for n,h in toolpins.items() if sha(where/n)!=h]
 if source_drift or object_drift or archive_drift or tool_drift or sha(binary)!=b['binary_sha256']:status='FAIL'
 v=dict(status=status,reason=reason,stages=stages,binary_sha256=sha(binary),build_validation_sha256=sha(receipt_path),
 source_drift=source_drift,object_drift=object_drift,archive_drift=archive_drift,tool_drift=tool_drift,tools_sha256=toolpins,
 scope='Full configured68-object driver; actual MPI2 selected69 cases per rank, originalP1/private-query/surface/complete-dependency/transaction/refusal controls, default accepted/rejected transport closure. Independent exact bridge geometry/markers/metric/current+BDF/SU2CGNS checks at N2M1 and N2M2. Reuses source/binary/774 archive members from serial transport build. Single contended correctness run, not MPI4/n4/persisted-restart/repeated timing/scaling/fullGoal1.')
 v['artifact_sha256']={str(p.relative_to(folder)):sha(p) for p in folder.rglob('*') if p.is_file()}
 (folder/'validation.json').write_text(json.dumps(v,indent=2)+'\n');print(status,folder,flush=True)
raise SystemExit(status!='PASS')
