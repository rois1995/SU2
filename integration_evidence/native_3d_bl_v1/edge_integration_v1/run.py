#!/usr/bin/env python3
"""Focused, resumable kernel checks; no CFD campaign or scaling claim."""
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
    'Common/include/adaptation/CNativeField3D.hpp', 'Common/src/adaptation/CNativeField3D.cpp',
    'Common/include/adaptation/CNativeEngine3D.hpp', 'Common/src/adaptation/CNativeEngine3D.cpp',
    'Common/src/adaptation/CNativeRemesher3D.cpp',
    'SU2_CFD/include/adaptation/CBoundaryLayerMetric.hpp', 'SU2_CFD/src/adaptation/CBoundaryLayerMetric.cpp',
    'UnitTests/Common/adaptation/CNativeField3D_tests.cpp',
    'UnitTests/Common/adaptation/CNativeMetric3D_tests.cpp',
    'UnitTests/SU2_CFD/adaptation/BoundaryLayerMetric_tests.cpp',
]

def digest(path):
    h = hashlib.sha256()
    with path.open('rb') as stream:
        for data in iter(lambda: stream.read(1024 * 1024), b''):
            h.update(data)
    return h.hexdigest()

def main():
    provenance = {name: digest(ROOT / name) for name in SOURCES}
    provenance['binary'] = digest(BINARY)
    provenance['helper'] = digest(Path(__file__).resolve())
    compile_commands = ROOT / 'integration_evidence/build-integrated-v2/compile_commands.json'
    provenance['compile_commands'] = digest(compile_commands)
    for name in ['Common/src/libSU2AccurateSum.a.p/adaptation_CNativeField3D.cpp.o',
                 'Common/src/libSU2AccurateSum.a.p/adaptation_CNativeEngine3D.cpp.o',
                 'SU2_CFD/src/libSU2core.a.p/adaptation_CBoundaryLayerMetric.cpp.o',
                 'UnitTests/test_driver.p/SU2_CFD_adaptation_BoundaryLayerMetric_tests.cpp.o',
                 'UnitTests/test_driver.p/Common_adaptation_CNativeField3D_tests.cpp.o']:
        provenance['object:' + name] = digest(ROOT / 'integration_evidence/build-integrated-v2' / name)
    names = []
    for file in SOURCES[-3:-1]:
        names += re.findall(r'TEST_CASE\(\s*"([^"]+)"', (ROOT / file).read_text())
    stages = [
        ('BL_kernels', '[NativeComposite3D],[NativeComposite2D],Boundary-layer metric*'),
        ('field_and_acceptance', ','.join(n.replace(',', '\\,') for n in names)),
    ]
    cpu = int((HERE / 'selected_cpu.txt').read_text())
    environment = dict(os.environ, OMP_NUM_THREADS='1', OPENBLAS_NUM_THREADS='1')
    receipts = []
    for name, selection in stages:
        record = HERE / (name + '.json')
        command = ['taskset', '-c', str(cpu), 'nice', '-n', '19', str(BINARY), selection, '--reporter', 'compact']
        contract = {'provenance': provenance, 'command': command, 'MPI_ranks': 1,
                    'environment': {'OMP_NUM_THREADS': '1', 'OPENBLAS_NUM_THREADS': '1'}}
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
                                      stderr=subprocess.STDOUT, timeout=120).returncode
            except subprocess.TimeoutExpired:
                code = 124
        text = log.read_text()
        passed = code == 0 and 'Passed all' in text and 'test cases' in text
        receipt = {'stage': name, 'status': 'PASS' if passed else 'FAIL', 'returncode': code,
                   'contract': contract, 'log': log.name, 'log_sha256': digest(log),
                   'elapsed_seconds': time.monotonic() - start, 'scope': 'serial kernel correctness only'}
        record.write_text(json.dumps(receipt, indent=2) + '\n')
        receipts.append(receipt)
        print(name + ': ' + receipt['status'], flush=True)
    status = 'PASS' if all(r['status'] == 'PASS' for r in receipts) else 'FAIL'
    (HERE / 'validation.json').write_text(json.dumps({'status': status, 'stages': receipts,
        'limitations': ['No native BL remesh, first-height/volume/analytic authority qualification',
                        'No CFD, MPI consistency, AD runtime or performance qualification']}, indent=2) + '\n')
    return 0 if status == 'PASS' else 1

if __name__ == '__main__':
    sys.exit(main())
