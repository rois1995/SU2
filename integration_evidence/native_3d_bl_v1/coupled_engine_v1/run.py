#!/usr/bin/env python3
"""Resumable coupled native BL correctness; no CFD or scaling claim."""
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
 'Common/include/adaptation/CNativeField3D.hpp','Common/src/adaptation/CNativeField3D.cpp',
 'Common/include/adaptation/CNativeEngine3D.hpp','Common/src/adaptation/CNativeEngine3D.cpp',
 'Common/include/adaptation/CNativeBoundary3D.hpp','Common/src/adaptation/CNativeBoundary3D.cpp',
 'Common/src/adaptation/CNativeDistributed3D.cpp','Common/src/adaptation/CNativeMetric3D.cpp',
 'UnitTests/Common/adaptation/fixtures/native_bl_corner.inc','UnitTests/Common/adaptation/fixtures/native_joint_obstruction.inc',
 'Common/include/adaptation/CNativeMetric3D.hpp','UnitTests/Common/adaptation/CNativeMetric3D_tests.cpp',
 'UnitTests/Common/adaptation/CNativeBoundary3D_tests.cpp',
 'SU2_CFD/include/adaptation/CBoundaryLayerMetric.hpp','SU2_CFD/src/adaptation/CBoundaryLayerMetric.cpp',
 'UnitTests/Common/adaptation/CNativeDistributed3D_tests.cpp','UnitTests/Common/adaptation/CNativeField3D_tests.cpp',
 'UnitTests/SU2_CFD/adaptation/BoundaryLayerMetric_tests.cpp','UnitTests/Common/adaptation/SimplexMeshTestCase.hpp',
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
    for name in ['Common/src/libSU2AccurateSum.a.p/adaptation_CNativeField3D.cpp.o',
                 'Common/src/libSU2AccurateSum.a.p/adaptation_CNativeEngine3D.cpp.o',
                 'Common/src/libSU2AccurateSum.a.p/adaptation_CNativeBoundary3D.cpp.o',
                 'Common/src/libSU2AccurateSum.a.p/adaptation_CNativeMetric3D.cpp.o',
                 'UnitTests/test_driver.p/Common_adaptation_CNativeBoundary3D_tests.cpp.o',
                 'UnitTests/test_driver.p/Common_adaptation_CNativeMetric3D_tests.cpp.o',
                 'SU2_CFD/src/libSU2core.a.p/adaptation_CBoundaryLayerMetric.cpp.o',
                 'UnitTests/test_driver.p/Common_adaptation_CNativeDistributed3D_tests.cpp.o',
                 'UnitTests/test_driver.p/Common_adaptation_CNativeField3D_tests.cpp.o',
                 'UnitTests/test_driver.p/SU2_CFD_adaptation_BoundaryLayerMetric_tests.cpp.o']:
        provenance['object:' + name] = digest(ROOT / 'integration_evidence/build-integrated-v2' / name)
    stages = [
      ('balanced_analytic','[NativeBalancedSplit3D]',1),
      ('joint_witness','[NativeBLCorner3D],[NativeBLCornerGrouped3D],[NativePlanarStrip3D],[NativeSurfaceStrip3D]',1),
      ('grouped_coupled_mpi1','[NativeCoupledBLGrouped3D]',1),
      ('metric_movement','[NativeMetricMove3D]',1),
      ('microscopic_balance','Native 3D composed edges resolve a microscopic BL without spreading donor tensors',1),
      ('coupled_mpi1','[NativeCoupledBL3D]',1),
      ('height_engine_mpi1','[NativeFirstLayerEngine3D]',1),
      ('height_engine_mpi2','[NativeFirstLayerEngine3D]',2),
      ('height_engine_mpi4','[NativeFirstLayerEngine3D]',4),
      ('composed_startup_mpi1','[NativeCoupledBLBudget3D]',1),
      ('composed_startup_mpi2','[NativeCoupledBLBudget3D]',2),
      ('composed_startup_mpi4','[NativeCoupledBLBudget3D]',4),
      ('coupled_mpi2','[NativeCoupledBL3D]',2),
      ('coupled_mpi4','[NativeCoupledBL3D]',4),
    ]
    requested=sys.argv[1:]
    if requested:
        unknown=set(requested)-{stage[0] for stage in stages}
        if unknown:raise ValueError('Unknown stages: '+','.join(sorted(unknown)))
        stages=[stage for stage in stages if stage[0] in requested]
    cpu = int((HERE / 'selected_cpu.txt').read_text())
    environment = dict(os.environ, OMP_NUM_THREADS='1', OPENBLAS_NUM_THREADS='1', OMPI_MCA_mpi_yield_when_idle='1', SU2_NATIVE_CURVED_EVIDENCE='1')
    receipts = []
    for name, selection, ranks in stages:
        record = HERE / (name + '.json')
        command = ['taskset', '-c', str(cpu), 'nice', '-n', '19']
        if ranks>1: command += ['mpirun','--oversubscribe','--bind-to','none','-np',str(ranks)]
        command += [str(BINARY), selection, '--reporter', 'compact']
        contract = {'provenance': provenance, 'command': command, 'MPI_ranks': ranks,
                    'environment': {'OMP_NUM_THREADS': '1', 'OPENBLAS_NUM_THREADS': '1', 'OMPI_MCA_mpi_yield_when_idle': '1','SU2_NATIVE_CURVED_EVIDENCE':'1'}}
        if name in ('coupled_mpi2','coupled_mpi4') and not any(r['stage']=='coupled_mpi1' and r['status']=='PASS' for r in receipts):
            receipt={'stage':name,'status':'NOT_RUN','reason':'Serial coupled construction has not passed; do not repeat known failure on more ranks.'}
            receipts.append(receipt)
            print(name+': NOT_RUN',flush=True)
            continue
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
        matches = re.findall(r'Passed (?:(?:all|both) )?(\d+) test cases? with (\d+) assertions?', text)
        expected = 4 if name=='joint_witness' else (2 if name.startswith('height_engine_') or name in ('metric_movement','balanced_analytic') else 1)
        passed = code == 0 and len(matches) == ranks and all(int(c)==expected for c,a in matches)
        receipt = {'stage': name, 'status': 'PASS' if passed else 'FAIL', 'returncode': code,
                   'contract': contract, 'log': log.name, 'log_sha256': digest(log),
                   'elapsed_seconds': time.monotonic() - start, 'scope': 'frozen-target split placement/first-height engine and complete coupled construction/removal correctness; not CFD/transfer/scaling'}
        record.write_text(json.dumps(receipt, indent=2) + '\n')
        receipts.append(receipt)
        print(name + ': ' + receipt['status'], flush=True)
    status = 'PASS' if all(r['status'] == 'PASS' for r in receipts) else 'FAIL'
    aggregate = HERE / ('validation_focused.json' if requested else 'validation.json')
    if aggregate.exists():
        aggregate.rename(HERE / ('validation_previous_' + str(time.time_ns()) + '.json'))
    (HERE / ('validation_focused.json' if requested else 'validation.json')).write_text(json.dumps({'status': status, 'stages': receipts,
        'limitations': ['No production wrapper, CFD, transfer, curved/interacting-wall BL or performance qualification',
                        'Whole campaign passes only when complete coupled construction/removal passes every requested rank']}, indent=2) + '\n')
    return 0 if status == 'PASS' else 1

if __name__ == '__main__':
    sys.exit(main())
