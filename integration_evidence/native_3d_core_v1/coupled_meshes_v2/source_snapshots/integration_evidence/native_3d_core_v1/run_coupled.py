"""Small sequential full-mesh native3D geometry/metric probes and independent compact-grid audit."""
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import time
root=Path(__file__).resolve().parents[2]
base=root/'integration_evidence/native_3d_core_v1'
objects=Path(sys.argv[1]).resolve();folder=Path(sys.argv[2]).resolve()
print('Reused validated object folder:',objects,flush=True)
print('Case working and retained folder:',folder,flush=True)
folder.mkdir(exist_ok=False)
env=dict(os.environ,OMP_NUM_THREADS='1',OPENBLAS_NUM_THREADS='1',MKL_NUM_THREADS='1')
receipt=json.loads((objects/'validation.json').read_text());records=[];status='FAIL'
source_paths=['Common/include/adaptation/CNativeMesh3D.hpp','Common/src/adaptation/CNativePredicates3D.cpp',
 'Common/include/adaptation/CNativeTopology3D.hpp','Common/src/adaptation/CNativeTopology3D.cpp',
 'Common/include/adaptation/CNativeCavity3D.hpp','Common/src/adaptation/CNativeCavity3D.cpp',
 'Common/include/adaptation/CNativeField3D.hpp','Common/src/adaptation/CNativeField3D.cpp',
 'Common/include/adaptation/CNativeBoundary3D.hpp','Common/src/adaptation/CNativeBoundary3D.cpp',
 'Common/include/adaptation/CNativeMetric3D.hpp','Common/src/adaptation/CNativeMetric3D.cpp',
 'Common/src/adt/CADTBaseClass.cpp','Common/src/adt/CADTElemClass.cpp','Common/include/parallelization/mpi_structure.cpp',
 str((base/'coupled_probe.cpp').relative_to(root)),str((base/'audit_coupled.py').relative_to(root)),str((base/'audit_cavities.py').relative_to(root)),str(Path(__file__).resolve().relative_to(root))]
linked=['predicates.o','topology.o','cavity.o','field.o','boundary.o','metric.o','CADTBaseClass.o','CADTElemClass.o','mpi_structure.o']
initial={p:hashlib.sha256((root/p).read_bytes()).hexdigest() for p in source_paths}
def run(command,name):
    start=time.monotonic();p=subprocess.run(command,cwd=root,env=env,text=True,capture_output=True)
    (folder/(name+'.stdout')).write_text(p.stdout);(folder/(name+'.stderr')).write_text(p.stderr)
    records.append(dict(stage=name,command=list(map(str,command)),exit_code=p.returncode,seconds=time.monotonic()-start))
    assert p.returncode==0,name+' failed; evidence retained in '+str(folder)
    print('Completed',name,'seconds',records[-1]['seconds'],flush=True)
try:
    assert receipt['status']=='PASS' and not receipt.get('changed_during_run')
    for p in source_paths[:15]:assert initial[p]==receipt['source_sha256'][p],p
    for p in linked:assert hashlib.sha256((objects/p).read_bytes()).hexdigest()==receipt['artifact_sha256'][p],p
    (folder/'validated_objects_receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
    for path in source_paths[15:]:
        dest=folder/'source_snapshots'/path;dest.parent.mkdir(parents=True,exist_ok=True);dest.write_bytes((root/path).read_bytes())
    run(['nice','-n','19','g++','-std=c++17','-O2','-fno-fast-math','-fno-unsafe-math-optimizations','-ffp-contract=off',
         '-Wall','-Wextra','-Werror','-I.',str(base/'coupled_probe.cpp')]+[str(objects/p) for p in linked]+['-o',str(folder/'coupled_probe')],'compile_probe')
    run(['nice','-n','19',str(folder/'coupled_probe'),str(folder)],'probe')
    run(['nice','-n','19',sys.executable,str(base/'audit_coupled.py'),str(folder)],'independent_audit')
    status='PASS'
finally:
    final={p:hashlib.sha256((root/p).read_bytes()).hexdigest() for p in source_paths};changed=[p for p in source_paths if final[p]!=initial[p]]
    if changed:status='FAIL'
    result=dict(status=status,stages=records,source_sha256=initial,final_source_sha256=final,changed_during_run=changed,
      reused_object_sha256={p:hashlib.sha256((objects/p).read_bytes()).hexdigest() for p in linked if (objects/p).exists()},
      scope='Bounded serial whole-mesh edge refinement/inverse coarsening on five manufactured original-P1 fields with adaptable planar wall triangles. Independent geometry/facet/marker/metric audit and SU2 exports; no SU2 CFD, solution/history transfer, MPI, CGNS or performance scaling certificate. Phase observations include toy selection, geometry, metric, commit, fields/final/output; nested field timers are not added.')
    result['artifact_sha256']={str(p.relative_to(folder)):hashlib.sha256(p.read_bytes()).hexdigest() for p in folder.rglob('*') if p.is_file()}
    (folder/'validation.json').write_text(json.dumps(result,indent=2)+'\n');print(status,folder,flush=True)
    if changed:raise RuntimeError('Source changed during validation: '+', '.join(changed))
