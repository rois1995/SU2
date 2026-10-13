#!/usr/bin/env python3
"""Resumable first-layer engine checks; no completed BL adaptation or scaling claim."""
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
 'Common/include/adaptation/CNativeEngine3D.hpp','Common/src/adaptation/CNativeEngine3D.cpp',
 'Common/include/adaptation/CNativeBoundary3D.hpp','Common/src/adaptation/CNativeBoundary3D.cpp',
 'Common/src/adaptation/CNativeDistributed3D.cpp','Common/src/adaptation/CNativeRemesher3D.cpp',
 'UnitTests/Common/adaptation/CNativeDistributed3D_tests.cpp',
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
    for name in ['Common/src/libSU2AccurateSum.a.p/adaptation_CNativeEngine3D.cpp.o',
                 'Common/src/libSU2AccurateSum.a.p/adaptation_CNativeDistributed3D.cpp.o',
                 'UnitTests/test_driver.p/Common_adaptation_CNativeDistributed3D_tests.cpp.o']:
        provenance['object:' + name] = digest(ROOT / 'integration_evidence/build-integrated-v2' / name)
    stages = [('mpi1',1),('mpi2',2),('mpi4',4)]
    cpus = (HERE / 'selected_cpus.txt').read_text().strip()
    environment = dict(os.environ, OMP_NUM_THREADS='1', OPENBLAS_NUM_THREADS='1', OMPI_MCA_mpi_yield_when_idle='1')
    receipts = []
    for name, ranks in stages:
        record = HERE / (name + '.json')
        command = ['taskset','-c',cpus,'nice','-n','19','mpirun','--oversubscribe','--bind-to','none','-np',str(ranks),str(BINARY),'[NativeFirstLayerEngine3D]','--use-colour','no']
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
        matches = re.findall(r'All tests passed \((\d+) assertions in (\d+) test cases?\)',text)
        passed = code == 0 and len(matches) == ranks and all(int(c)==2 for a,c in matches)
        receipt = {'stage': name, 'status': 'PASS' if passed else 'FAIL', 'returncode': code,
                   'contract': contract, 'log': log.name, 'log_sha256': digest(log),
                   'elapsed_seconds': time.monotonic() - start, 'scope': 'MPI first-row initialization/protection and worker-subset correctness; whole remesh not qualified'}
        record.write_text(json.dumps(receipt, indent=2) + '\n')
        receipts.append(receipt)
        print(name + ': ' + receipt['status'], flush=True)
    status = 'PASS' if all(r['status'] == 'PASS' for r in receipts) else 'FAIL'
    aggregate = HERE / 'validation.json'
    if aggregate.exists():
        aggregate.rename(HERE / ('validation_previous_' + str(time.time_ns()) + '.json'))
    (HERE / 'validation.json').write_text(json.dumps({'status': status, 'stages': receipts,
        'limitations': ['No completed production BL remesh, composed BL metric, transfer or RANS qualification',
                        'No CFD timestep, AD runtime or performance qualification']}, indent=2) + '\n')
    return 0 if status == 'PASS' else 1

if __name__ == '__main__':
    sys.exit(main())
