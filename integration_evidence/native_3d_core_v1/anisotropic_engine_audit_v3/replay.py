"""Canonical re-audit of immutable v1/v2 MPI observations; no native rerun."""
from pathlib import Path
import hashlib,json,os,subprocess,sys,time
root=Path.cwd();folder=Path(__file__).resolve().parent
print('Case working and retained folder:',folder,flush=True)
inputs=root/'integration_evidence/native_3d_core_v1'
files=['integration_evidence/native_3d_core_v1/audit_anisotropic.py','integration_evidence/native_3d_core_v1/audit_coupled.py','integration_evidence/native_3d_core_v1/audit_cavities.py']
def pins():return {p:hashlib.sha256((root/p).read_bytes()).hexdigest() for p in files}
initial=pins();records=[];status='FAIL';input_hashes={}
try:
    for variant in [1,2]:
        source=inputs/('anisotropic_engine_v%d'%variant)
        receipt=json.loads((source/'validation.json').read_text());assert receipt['status']=='PASS'
        for n in [1,2,4]:
            log=source/('mpi%d.stdout'%n);input_hashes[str(log.relative_to(root))]=hashlib.sha256(log.read_bytes()).hexdigest()
            label='v%d_mpi%d'%(variant,n);destination=folder/label
            print('Case working and retained folder:',destination,flush=True)
            command=['nice','-n','19',sys.executable,files[0],str(log),str(destination),str(n)]
            start=time.monotonic();p=subprocess.run(command,cwd=root,text=True,capture_output=True)
            (folder/(label+'.stdout')).write_text(p.stdout);(folder/(label+'.stderr')).write_text(p.stderr)
            records.append(dict(stage=label,command=command,exit_code=p.returncode,seconds=time.monotonic()-start))
            assert p.returncode==0,p.stderr
    matches=[]
    for n in [1,2,4]:
        a=folder/('v1_mpi%d'%n);b=folder/('v2_mpi%d'%n)
        for p in a.iterdir():
            if p.name=='independent_audit.json':continue
            assert p.read_bytes()==(b/p.name).read_bytes(),p.name
            matches.append(str((b/p.name).relative_to(folder)))
    assert len(matches)==90
    assert pins()==initial
    status='PASS'
finally:
    result=dict(status=status,source_sha256=initial,final_source_sha256=pins(),input_sha256=input_hashes,stages=records,
        scope='Canonical reserialization and independent full anisotropic transport/P1/mesh audits of unchanged actual MPI v1/v2 logs. Original donor ordering fixed only in audit output. No native rebuild or rerun.',
        same_mesh_artifacts=matches if status=='PASS' else [])
    result['artifact_sha256']={str(p.relative_to(folder)):hashlib.sha256(p.read_bytes()).hexdigest() for p in folder.rglob('*') if p.is_file()}
    (folder/'validation.json').write_text(json.dumps(result,indent=2)+'\n')
    print(status,folder,flush=True)
