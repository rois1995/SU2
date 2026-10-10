"""Sequential low-priority MPI 1/2/4 directory/atomic-publication controls; no CFD build."""
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import time

root = Path(__file__).resolve().parents[2]
folder = Path(sys.argv[1]).resolve() if len(sys.argv) > 1 else root/'integration_evidence/native_3d_core_v1/distributed_controls_v1'
print('Case working and retained folder:', folder, flush=True)
folder.mkdir(exist_ok=False)
env = dict(os.environ, OMP_NUM_THREADS='1', OPENBLAS_NUM_THREADS='1', MKL_NUM_THREADS='1',
           OMPI_MCA_mpi_yield_when_idle='1', OMPI_MCA_hwloc_base_binding_policy='none')
records = []
def run(command, name):
    print('Starting', name, flush=True)
    start = time.monotonic()
    p = subprocess.run(command, cwd=root, env=env, text=True, capture_output=True)
    (folder/(name+'.stdout')).write_text(p.stdout)
    (folder/(name+'.stderr')).write_text(p.stderr)
    records.append(dict(stage=name, command=list(map(str, command)), exit_code=p.returncode,
                        seconds=time.monotonic()-start))
    if p.returncode: raise RuntimeError(name+' failed; evidence retained in '+str(folder))
    return p.stdout

def host(name):
    for command, suffix in [(['uptime'], 'load'), (['free', '-m'], 'memory'),
                            (['ps', '-eo', 'pid,ppid,ni,pcpu,args', '--sort=-pcpu'], 'processes')]:
        output = subprocess.run(command, text=True, capture_output=True)
        (folder/(name+'_'+suffix+'.txt')).write_text(output.stdout if suffix != 'processes' else '\n'.join(output.stdout.splitlines()[:16])+'\n')

units = ['Common/src/adaptation/CNative'+name+'3D.cpp' for name in ['Predicates', 'Topology', 'Cavity', 'Boundary', 'Field', 'Metric', 'Distributed']]
house = ['Common/src/adt/CADTBaseClass.cpp', 'Common/src/adt/CADTElemClass.cpp',
         'Common/include/parallelization/mpi_structure.cpp', 'Common/src/parallelization/CPassiveComm.cpp',
         'Common/src/adaptation/CDistributedSearch.cpp']
tests = ['UnitTests/Common/adaptation/CNative'+name+'3D_tests.cpp' for name in ['Mesh', 'Topology', 'Cavity', 'Boundary', 'Field', 'Metric', 'Distributed']]
headers = ['Common/include/adaptation/CNative'+name+'3D.hpp' for name in ['Mesh', 'Topology', 'Cavity', 'Boundary', 'Field', 'Metric', 'Distributed']]
files = units+house+tests+headers+['Common/src/meson.build', 'UnitTests/meson.build', 'UnitTests/test_driver.cpp',
    'Common/include/adaptation/CNativeDistributed2D.hpp', 'Common/include/adaptation/CTransferMemory.hpp',
    'Common/include/adaptation/CDistributedSearch.hpp', 'Common/include/parallelization/CPassiveComm.hpp',
    'Common/include/parallelization/mpi_structure.hpp', 'integration_evidence/native_3d_core_v1/audit_distributed.py',
    'integration_evidence/native_3d_core_v1/audit_coupled.py', 'integration_evidence/native_3d_core_v1/audit_cavities.py',
    str(Path(__file__).resolve().relative_to(root))]
initial = {p: hashlib.sha256((root/p).read_bytes()).hexdigest() for p in files}
git = shutil.which('git') is not None and (root/'.git').exists()
base = subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=root, text=True).strip() if git else None
snapshots = {}
for path in files:
    data = (root/path).read_bytes()
    original = subprocess.run(['git', 'show', 'HEAD:'+path], cwd=root, capture_output=True) if git else None
    if original is None or original.returncode or original.stdout != data:
        saved = folder/'source_snapshots'/path
        saved.parent.mkdir(parents=True, exist_ok=True)
        saved.write_bytes(data)
        snapshots[str(saved.relative_to(folder))] = hashlib.sha256(data).hexdigest()
status = 'FAIL'
try:
    host('before_build')
    compiler = ['nice', '-n', '19', 'mpicxx', '-std=c++17', '-DHAVE_MPI', '-DOMPI_SKIP_MPICXX=1',
                '-ffunction-sections', '-fdata-sections', '-Iexternals/eigen', '-fno-fast-math',
                '-fno-unsafe-math-optimizations', '-ffp-contract=off']
    objects = []
    for source in units+house:
        obj = folder/(Path(source).stem+'.o')
        flags = ['-O2', '-Wall', '-Wextra', '-Werror', '-Wno-unused-parameter'] if source in units else ['-O2']
        run(compiler+flags+['-c', source, '-o', str(obj)], 'compile_'+Path(source).stem)
        objects.append(str(obj))
    run(compiler+['-O0', '-Iexternals/catch2', 'UnitTests/test_driver.cpp']+tests+objects+
        ['-Wl,--gc-sections', '-o', str(folder/'test_distributed')], 'compile_tests')
    for ranks in [1, 2, 4]:
        host('before_mpi'+str(ranks))
        tag = '[NativeDistributed3D],[NativeMesh3D]' if ranks == 1 else '[NativeDistributed3D]'
        output = run(['nice', '-n', '19', 'mpirun', '--oversubscribe', '--bind-to', 'none', '-np', str(ranks),
                      str(folder/'test_distributed'), tag, '--use-colour', 'no'], 'mpi'+str(ranks))
        assert output.count('All tests passed') == ranks
        reports = []
        for line in output.splitlines():
            if line.startswith('DIRECTORY ') or line.startswith('SUBSET_RETURN '): reports.append(line)
        (folder/('mpi'+str(ranks)+'_profiles.json')).write_text(json.dumps(reports, indent=2)+'\n')
        run(['nice', '-n', '19', sys.executable, 'integration_evidence/native_3d_core_v1/audit_distributed.py',
             str(folder/('mpi'+str(ranks)+'.stdout')), str(folder/('mpi'+str(ranks)+'_meshes'))], 'audit_mpi'+str(ranks))
    status = 'PASS'
finally:
    final = {p: hashlib.sha256((root/p).read_bytes()).hexdigest() for p in files}
    changed = [p for p in files if initial[p] != final[p]]
    if changed: status = 'FAIL'
    result = dict(status=status, stages=records, base_commit=base, source_sha256=initial,
        final_source_sha256=final, changed_during_run=changed, source_snapshot_sha256=snapshots,
        scope='Actual SU2 passive MPI transport/failure election, complete bounded tetrahedral vertex/edge/face imports, coupled one-step refinement/inverse, version/storage rollback, subset mesh-record migration/return. MPI1 includes existing 3D kernel controls. No production remesher/runtime, ParMETIS, CFD solution/history transfer, CGNS output, full adaptation/runtime embedding or scaling qualification. Actual MPI one-step mesh records undergo an independent exact embedding/marker/P1 audit and produce inspectable SU2 files; refined grids remain oversized private progress.')
    result['artifact_sha256'] = {str(p.relative_to(folder)): hashlib.sha256(p.read_bytes()).hexdigest()
        for p in folder.rglob('*') if p.is_file() and p.suffix != '.o' and p.name != 'test_distributed'}
    if (folder/'test_distributed').exists(): result['binary_sha256'] = hashlib.sha256((folder/'test_distributed').read_bytes()).hexdigest()
    (folder/'validation.json').write_text(json.dumps(result, indent=2)+'\n')
    print(status, folder, flush=True)
    if changed: raise RuntimeError('Source changed during validation: '+', '.join(changed))
