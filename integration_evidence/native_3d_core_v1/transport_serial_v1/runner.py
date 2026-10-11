"""Actual one-rank native3D transport regression and independent bridge audit."""
from pathlib import Path
import hashlib,json,os,re,subprocess,sys,time
root=Path('/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated');where=root/'integration_evidence/native_3d_core_v1'
folder=Path(__file__).resolve().parent;build=root/'integration_evidence/build-integrated-v2';binary=build/'UnitTests/test_driver'
sha=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest()
receipt_path=where/'transport_build_v1/validation.json';receipt=json.loads(receipt_path.read_text());assert receipt['status']=='PASS'
sys.path.insert(0,str(root/'integration_evidence/native_cluster_3d_core_v1'))
from run import command
env=dict(os.environ,TMPDIR=str(folder),OMP_NUM_THREADS='1',OPENBLAS_NUM_THREADS='1',MKL_NUM_THREADS='1',OMPI_MCA_mpi_yield_when_idle='1',PYTHONDONTWRITEBYTECODE='1')
toolnames=('audit_geometry_bridge.py','audit_coupled.py','audit_cavities.py','audit_lifecycle.py')
toolpins={n:sha(where/n) for n in toolnames}
stages=[];status='FAIL';reason='';raw_identity={}
def execute(cmd,label,cwd):
 result=command(list(map(str,cmd)),cwd,label,env,300);stages.append(dict(stage=label,**result))
 if result['exit_code']!=0 or result.get('timeout'):raise RuntimeError(label+' failed; retained logs in '+str(cwd))
 return (cwd/(label+'.stdout')).read_text()
try:
 for cmd,label in [(['uptime'],'host_load'),(['free','-m'],'host_memory'),(['ps','-eo','pid,ppid,ni,pcpu,comm','--sort=-pcpu'],'host_processes')]:execute(cmd,label,folder)
 read=lambda:list(map(int,Path('/proc/stat').read_text().splitlines()[0].split()[1:]))
 a=read();time.sleep(2);b=read();d=[y-x for x,y in zip(a,b)]
 gate=dict(idle_fraction=(d[3]+d[4])/sum(d),load1=os.getloadavg()[0],cpus=os.cpu_count())
 (folder/'host_gate.json').write_text(json.dumps(gate,indent=2)+'\n')
 if gate['idle_fraction']<.25 or gate['load1']>=gate['cpus']:raise RuntimeError('Resource gate refused before MPI launch.')
 assert sha(binary)==receipt['binary_sha256']
 native=folder/'native_serial';native.mkdir(exist_ok=False)
 selection='[NativeReference3D],[NativeMesh3D],[NativeDonorReuse3D],[NativeCollapseFilter3D],[NativeCoplanar3D],[NativeInsertion3D],[NativeAutomaticRepair3D],[NativeBudget3D],[NativeDefault3D],[NativeBridge2D],[NativeBridge3D]'
 out=execute(['nice','-n','19','mpirun','--bind-to','none','-np','1',binary,selection,'--use-colour','no'],'native_serial',native)
 assert len(re.findall(r'All tests passed \(\d+ assertions in 69 test cases\)',out))==1
 mmg=folder/'mmg_serial';mmg.mkdir(exist_ok=False)
 out=execute(['nice','-n','19','mpirun','--bind-to','none','-np','1',binary,'MMG interface: 2D round trip,MMG interface: 3D round trip','--use-colour','no'],'mmg_serial',mmg)
 assert 'All tests passed (48 assertions in 2 test cases)' in out
 execute(['nice','-n','19',sys.executable,where/'audit_geometry_bridge.py',native,'1',native/'native_serial.stdout'],'independent_bridge',native)
 from audit_lifecycle import parse,transport_counters
 rows=[parse(line) for line in (native/'native_serial.stdout').read_text().splitlines() if line.startswith('NATIVE3D ')]
 assert len(rows)==2 and all(transport_counters(r) for r in rows)
 assert [int(r['complete']) for r in rows]==[0,1]
 (folder/'transport_rows.json').write_text(json.dumps(rows,indent=2)+'\n')
 baseline=where/'reference_admission_serial_v1/native_serial'
 files=sorted(p.name for p in baseline.iterdir() if p.is_file())
 raw_identity={name:dict(baseline_sha256=sha(baseline/name),current_sha256=sha(native/name)) for name in files}
 for name,pins in raw_identity.items():
  if not name.endswith('.cgns'):assert pins['baseline_sha256']==pins['current_sha256'],name+' numerical artifact changed'
 status='PASS'
except Exception as e:
 reason=str(e);print(reason,flush=True)
finally:
 drift=[name for name,h in receipt['source_sha256'].items() if sha(root/name)!=h]
 object_drift=[name for name,h in receipt['actual_objects_sha256'].items() if sha(build/name)!=h]
 archive_drift=[name for name,h in {**receipt['library_sha256'],**receipt['thin_archive_member_sha256']}.items() if sha(name)!=h]
 tool_drift=[n for n,h in toolpins.items() if sha(where/n)!=h]
 if drift or object_drift or archive_drift or tool_drift or sha(binary)!=receipt['binary_sha256']:status='FAIL'
 v=dict(status=status,reason=reason,stages=stages,binary_sha256=sha(binary),build_validation_sha256=sha(receipt_path),
        source_drift=drift,object_drift=object_drift,archive_drift=archive_drift,tool_drift=tool_drift,tools_sha256=toolpins,
        raw_identity=raw_identity,scope='Actual configured68-object driver; MPI1 selected69 regression including success/rejection/reused-World transport controls, serialMMG48 and independent geometry/markers/P1/affine-current+BDF/SU2CGNS bridge audit. Native rows include one rejection and one acceptance. Not MPI2/4, n4 envelope/persisted restart, repeated timing/scaling, complete traffic/wait or fullGoal1.')
 v['artifact_sha256']={str(p.relative_to(folder)):sha(p) for p in folder.rglob('*') if p.is_file()}
 (folder/'validation.json').write_text(json.dumps(v,indent=2)+'\n');print(status,folder,flush=True)
raise SystemExit(status!='PASS')
