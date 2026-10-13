#!/usr/bin/env python3
"""Resumable thin-volume/complexity checks; no CFD timestep, remesh or scaling claim."""
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys
import time

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
BINARY = ROOT / 'integration_evidence/build-integrated-v2/UnitTests/test_driver'
SOURCES = [
    'SU2_CFD/include/adaptation/CBoundaryLayerMetric.hpp', 'SU2_CFD/src/adaptation/CBoundaryLayerMetric.cpp',
    'SU2_CFD/include/adaptation/ConvexClipping.hpp', 'SU2_CFD/include/solvers/CSolver.hpp',
    'SU2_CFD/src/solvers/CSolver.cpp', 'SU2_CFD/src/drivers/CSinglezoneDriver.cpp',
    'Common/include/adaptation/CNativeRemesher3D.hpp', 'Common/src/adaptation/CNativeRemesher3D.cpp',
    'UnitTests/SU2_CFD/adaptation/BoundaryLayerMetric_tests.cpp',
]

def digest(path):
    h = hashlib.sha256()
    with path.open('rb') as stream:
        for data in iter(lambda: stream.read(1024 * 1024), b''):
            h.update(data)
    return h.hexdigest()

def main():
    build = ROOT / 'integration_evidence/build-integrated-v2'
    check = subprocess.run(['/tmp/native-build-tools/ninja', '-C', str(build), '-n', 'UnitTests/test_driver'],
                           stdout=subprocess.PIPE, stderr=subprocess.STDOUT, universal_newlines=True, timeout=30)
    if check.returncode != 0 or 'ninja: no work to do.' not in check.stdout:
        print('Refusing stale/incomplete build; finish the existing build before running cases.', flush=True)
        print(check.stdout, flush=True)
        return 2
    provenance = {name: digest(ROOT / name) for name in SOURCES}
    provenance['binary'] = digest(BINARY)
    provenance['helper'] = digest(Path(__file__).resolve())
    compile_commands = ROOT / 'integration_evidence/build-integrated-v2/compile_commands.json'
    provenance['compile_commands'] = digest(compile_commands)
    for name in ['SU2_CFD/src/libSU2core.a.p/solvers_CSolver.cpp.o',
                 'SU2_CFD/src/libSU2core.a.p/drivers_CSinglezoneDriver.cpp.o',
                 'Common/src/libSU2Common.a.p/adaptation_CNativeRemesher3D.cpp.o',
                 'SU2_CFD/src/libSU2core.a.p/adaptation_CBoundaryLayerMetric.cpp.o',
                 'UnitTests/test_driver.p/SU2_CFD_adaptation_BoundaryLayerMetric_tests.cpp.o']:
        provenance['object:' + name] = digest(ROOT / 'integration_evidence/build-integrated-v2' / name)
    stages = [
        ('microscopic_tetrahedron', 'Native geometric volume quadrature resolves microscopic BL in coarse tetrahedra', 1),
        ('production_complexity', 'Native3D complexity integrates the geometric BL with sensor-only donors', 1),
        ('original_reference', 'Native3D complexity reference does not rebase the wall on the adapted surface', 1),
        ('nearest_face_change', 'Native3D volume integration resolves a far vertex closest to another wall face', 1),
        ('hidden_support', 'Native3D volume support detects a thin finite wall missed by all cell vertices', 1),
        ('parallel_geometry', '[NativeParallelBL3D]', 1),
        ('existing_BL_kernels', '[NativeComposite3D],[NativeComposite2D],Boundary-layer metric*', 1),
        ('production_complexity_mpi2', 'Native3D complexity integrates the geometric BL with sensor-only donors', 2),
        ('production_complexity_mpi4', 'Native3D complexity integrates the geometric BL with sensor-only donors', 4),
    ]
    cpu = int((HERE / 'selected_cpu.txt').read_text())
    environment = dict(os.environ, OMP_NUM_THREADS='1', OPENBLAS_NUM_THREADS='1', OMPI_MCA_mpi_yield_when_idle='1')
    receipts = []
    for name, selection, ranks in stages:
        record = HERE / (name + '.json')
        command = ['taskset', '-c', str(cpu), 'nice', '-n', '19']
        if ranks>1: command += ['mpirun','--oversubscribe','--bind-to','none','-np',str(ranks)]
        command += [str(BINARY), selection, '--reporter', 'compact']
        contract = {'provenance': provenance, 'command': command, 'MPI_ranks': ranks,
                    'environment': {'OMP_NUM_THREADS': '1', 'OPENBLAS_NUM_THREADS': '1', 'OMPI_MCA_mpi_yield_when_idle': '1'}}
        if record.exists():
            old = json.loads(record.read_text())
            if old.get('status') == 'PASS' and old.get('contract') == contract and digest(HERE / old['log']) == old['log_sha256']:
                receipts.append(old)
                print('Reuse:', name, flush=True)
                continue
            record.rename(HERE / (name + '_previous_' + str(time.time_ns()) + '.json'))
        start = time.monotonic()
        log = HERE / (name + '_' + str(time.time_ns()) + '.log')
        with log.open('w') as output:
            try:
                code = subprocess.run(command, cwd=HERE, env=environment, stdout=output,
                                      stderr=subprocess.STDOUT, timeout=180).returncode
            except subprocess.TimeoutExpired:
                code = 124
        text = log.read_text()
        matches = re.findall(r'Passed (?:all )?(\d+) test cases? with (\d+) assertions?', text)
        expected = 19 if name == 'existing_BL_kernels' else 1
        passed = code == 0 and len(matches) == ranks and all(int(c)==expected for c,a in matches)
        receipt = {'stage': name, 'status': 'PASS' if passed else 'FAIL', 'returncode': code,
                   'contract': contract, 'log': log.name, 'log_sha256': digest(log),
                   'elapsed_seconds': time.monotonic() - start, 'scope': 'kernel/complexity correctness only; MPI case counts explicit in contract'}
        record.write_text(json.dumps(receipt, indent=2) + '\n')
        receipts.append(receipt)
        print(name + ': ' + receipt['status'], flush=True)
    status = 'PASS' if all(r['status'] == 'PASS' for r in receipts) else 'FAIL'
    aggregate = HERE / 'validation.json'
    if aggregate.exists():
        aggregate.rename(HERE / ('validation_previous_' + str(time.time_ns()) + '.json'))
    (HERE / 'validation.json').write_text(json.dumps({'status': status, 'stages': receipts,
        'limitations': ['No native BL remesh, first-height or analytic authority qualification',
                        'No CFD, full BL remesh/transfer MPI consistency, AD runtime or performance qualification']}, indent=2) + '\n')
    return 0 if status == 'PASS' else 1

if __name__ == '__main__':
    sys.exit(main())
