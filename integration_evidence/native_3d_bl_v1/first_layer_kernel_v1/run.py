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
 'Common/include/adaptation/CNativeBoundary3D.hpp','Common/src/adaptation/CNativeBoundary3D.cpp',
 'Common/include/adaptation/CNativeCavity3D.hpp','Common/src/adaptation/CNativeCavity3D.cpp',
 'UnitTests/Common/adaptation/CNativeBoundary3D_tests.cpp',
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
    for name in ['Common/src/libSU2AccurateSum.a.p/adaptation_CNativeBoundary3D.cpp.o',
                 'Common/src/libSU2AccurateSum.a.p/adaptation_CNativeCavity3D.cpp.o',
                 'UnitTests/test_driver.p/Common_adaptation_CNativeBoundary3D_tests.cpp.o']:
        provenance['object:' + name] = digest(ROOT / 'integration_evidence/build-integrated-v2' / name)
    stages = [
      ('joint_layer','Native3D joint first-height construction rebuilds wall children and transition'),
      ('atomic_rejection','Native3D first-layer obstruction rejects atomically without height relaxation'),
      ('tilt_diagnosis','Native3D first-height certificate reports tangential tilt independently'),
      ('junction_and_clearance','Native3D first layer handles wall junctions and distinguishes local clearance failure'),
    ]
    cpu = int((HERE / 'selected_cpu.txt').read_text())
    environment = dict(os.environ, OMP_NUM_THREADS='1', OPENBLAS_NUM_THREADS='1', SU2_NATIVE_CURVED_EVIDENCE='1')
    receipts = []
    for name, selection in stages:
        record = HERE / (name + '.json')
        command = ['taskset', '-c', str(cpu), 'nice', '-n', '19', str(BINARY), selection, '--reporter', 'compact']
        contract = {'provenance': provenance, 'command': command, 'MPI_ranks': 1,
                    'environment': {'OMP_NUM_THREADS': '1', 'OPENBLAS_NUM_THREADS': '1', 'SU2_NATIVE_CURVED_EVIDENCE': '1'}}
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
        match = re.search(r'Passed (?:all )?(\d+) test cases? with (\d+) assertions?', text)
        expected = 1
        passed = code == 0 and match is not None and int(match.group(1)) == expected
        receipt = {'stage': name, 'status': 'PASS' if passed else 'FAIL', 'returncode': code,
                   'contract': contract, 'log': log.name, 'log_sha256': digest(log),
                   'elapsed_seconds': time.monotonic() - start, 'scope': 'serial first-height geometry kernel only; metric/MPI engine not qualified'}
        record.write_text(json.dumps(receipt, indent=2) + '\n')
        receipts.append(receipt)
        print(name + ': ' + receipt['status'], flush=True)
    status = 'PASS' if all(r['status'] == 'PASS' for r in receipts) else 'FAIL'
    aggregate = HERE / 'validation.json'
    if aggregate.exists():
        aggregate.rename(HERE / ('validation_previous_' + str(time.time_ns()) + '.json'))
    (HERE / 'validation.json').write_text(json.dumps({'status': status, 'stages': receipts,
        'limitations': ['No production BL remesh, target metric, remote clearance or MPI transaction qualification',
                        'No CFD, MPI consistency, AD runtime or performance qualification']}, indent=2) + '\n')
    return 0 if status == 'PASS' else 1

if __name__ == '__main__':
    sys.exit(main())
