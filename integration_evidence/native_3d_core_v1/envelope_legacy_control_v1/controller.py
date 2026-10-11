from pathlib import Path
import hashlib,json,os,subprocess,sys,time
root=Path('/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated');where=root/'integration_evidence/native_3d_core_v1'
folder=where/'envelope_legacy_control_v1';folder.mkdir(exist_ok=False)
print('Case working and retained folder:',folder,flush=True)
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
qualified=where/'envelope_mpi2_m2_v1';pin=json.loads((qualified/'validation.json').read_text());assert pin['status']=='PASS'
binary=qualified/'test_runtime';assert sha(binary)==pin['binary_sha256']
old=json.loads((where/'reader_skew_v1/validation.json').read_text())
assert pin['library_sha256']==old['library_sha256'] and pin['thin_archive_member_sha256']==old['thin_archive_member_sha256']
(folder/'controller.py').write_bytes(Path(__file__).read_bytes())
env=dict(os.environ,OMP_NUM_THREADS='1',OPENBLAS_NUM_THREADS='1',MKL_NUM_THREADS='1',OMPI_MCA_mpi_yield_when_idle='1',PYTHONDONTWRITEBYTECODE='1')
stages=[];status='FAIL'
def run(cmd,label,cwd=folder):
 print('Starting',label,flush=True);start=time.monotonic()
 p=subprocess.run(['timeout','--signal=TERM','--kill-after=10s','900s']+list(map(str,cmd)),cwd=cwd,env=env,capture_output=True,text=True)
 (folder/(label+'.stdout')).write_text(p.stdout);(folder/(label+'.stderr')).write_text(p.stderr)
 stages.append(dict(stage=label,command=list(map(str,cmd)),seconds=time.monotonic()-start,exit_code=p.returncode));assert p.returncode==0,label
 return p.stdout
try:
 for cmd,label in [(['uptime'],'host_load'),(['free','-m'],'host_memory'),(['ps','-eo','pid,ni,pcpu,comm','--sort=-pcpu'],'host_processes')]:run(cmd,label)
 out=run(['nice','-n','19','mpirun','--oversubscribe','--bind-to','none','-np','1',binary,'[NativeRuntime3D],[NativeBridge2D],[MMGRoundTrip]','--use-colour','no'],'case')
 assert out.count('All tests passed')==1
 run(['nice','-n','19',sys.executable,where/'audit_runtime.py',folder,'1',folder/'case.stdout'],'independent_audit')
 status='PASS'
finally:
 changed=[p for p,h in pin['source_sha256'].items() if sha(root/p)!=h]
 libraries={**pin['library_sha256'],**pin['thin_archive_member_sha256']}
 changes=[p for p,h in libraries.items() if sha(Path(p))!=h]
 if changed or changes:status='FAIL'
 result=dict(status=status,qualified_binary_receipt_sha256=sha(qualified/'validation.json'),binary_sha256=sha(binary),
  source_changes=changed,production_changes=changes,production_equal_to_published_reader=True,stages=stages,
  scope='Actual compiled default output-helper control and existing native2D transfer/MMG roundtrip tests at MPI1. Original-P1 affine current/BDF and native SU2/CGNS independent audits. No production source change or new full 2D scaling qualification.')
 result['artifact_sha256']={str(p.relative_to(folder)):sha(p) for p in folder.rglob('*') if p.is_file() and p.suffix!='.pyc'}
 (folder/'validation.json').write_text(json.dumps(result,indent=2)+'\n');print(status,folder,flush=True)
