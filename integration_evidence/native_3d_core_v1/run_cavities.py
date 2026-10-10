"""Sequential low-priority standalone native cavity probes and independent small-mesh export/audit."""
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import time
root=Path(__file__).resolve().parents[2]
base=root/'integration_evidence/native_3d_core_v1'
folder=Path(sys.argv[1]).resolve()
print('Case working and retained folder:',folder,flush=True)
folder.mkdir(exist_ok=False)
env=dict(os.environ,OMP_NUM_THREADS='1',OPENBLAS_NUM_THREADS='1',MKL_NUM_THREADS='1')
records=[]
sources=['Common/include/adaptation/CNativeMesh3D.hpp','Common/src/adaptation/CNativePredicates3D.cpp',
         'Common/include/adaptation/CNativeTopology3D.hpp','Common/src/adaptation/CNativeTopology3D.cpp',
         'Common/include/adaptation/CNativeCavity3D.hpp','Common/src/adaptation/CNativeCavity3D.cpp',
         'integration_evidence/native_3d_core_v1/cavity_probe.cpp',
         'integration_evidence/native_3d_core_v1/audit_cavities.py',str(Path(__file__).resolve().relative_to(root))]
def run(command,name):
    start=time.monotonic()
    p=subprocess.run(command,cwd=root,env=env,text=True,capture_output=True)
    (folder/(name+'.stdout')).write_text(p.stdout);(folder/(name+'.stderr')).write_text(p.stderr)
    records.append(dict(stage=name,command=list(map(str,command)),exit_code=p.returncode,seconds=time.monotonic()-start))
    assert p.returncode==0,name+' failed; evidence retained in '+str(folder)
status='FAIL'
try:
    run(['nice','-n','19','g++','-std=c++17','-O2','-fno-fast-math','-fno-unsafe-math-optimizations',
         '-ffp-contract=off','-Wall','-Wextra','-Werror','-I.',
         'Common/src/adaptation/CNativePredicates3D.cpp','Common/src/adaptation/CNativeTopology3D.cpp',
         'Common/src/adaptation/CNativeCavity3D.cpp',str(base/'cavity_probe.cpp'),'-o',str(folder/'cavity_probe')],'compile_probe')
    run(['nice','-n','19',str(folder/'cavity_probe'),str(folder)],'probe')
    run(['nice','-n','19',sys.executable,str(base/'audit_cavities.py'),str(folder)],'independent_audit')
    status='PASS'
finally:
    receipt=dict(status=status,stages=records,source_sha256={p:hashlib.sha256((root/p).read_bytes()).hexdigest() for p in sources},
        scope='Eight tiny geometric private-cavity controls (five metric-gated, three geometry-only); one sequential low-priority process. Observed times are not a performance/scaling benchmark; no CFD, physical-surface edits, spatial donor queries or MPI.')
    receipt['artifact_sha256']={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in folder.iterdir() if p.is_file()}
    (folder/'validation.json').write_text(json.dumps(receipt,indent=2)+'\n')
    print(status,folder,flush=True)
