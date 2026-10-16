#!/usr/bin/env python3
"""Replay the three recorded M6 fields without CFD; retain resumable per-case receipts."""
import fcntl
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import time

ROOT = Path(__file__).resolve().parents[3]
HERE = Path(__file__).resolve().parent
BUILD = ROOT / 'integration_evidence/build-integrated-v2'

def digest(path):
    value = hashlib.sha256()
    with path.open('rb') as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b''):
            value.update(block)
    return value.hexdigest()

def main():
    label = sys.argv[1] if len(sys.argv) == 2 else ''
    if not label or any(c not in 'abcdefghijklmnopqrstuvwxyz0123456789_-' for c in label):
        raise SystemExit('usage: run_captured_replays.py lowercase_checkpoint_label')
    paths = [Path(__file__).resolve(),
             ROOT/'SU2_CFD/src/adaptation/CBoundaryLayerMetric.cpp',
             ROOT/'SU2_CFD/include/adaptation/CBoundaryLayerMetric.hpp',
             ROOT/'SU2_CFD/src/solvers/CSolver.cpp',
             ROOT/'UnitTests/SU2_CFD/adaptation/BoundaryLayerMetric_tests.cpp',
             BUILD/'UnitTests/test_driver',
             BUILD/'SU2_CFD/src/libSU2core.a.p/adaptation_CBoundaryLayerMetric.cpp.o',
             BUILD/'build.ninja',BUILD/'meson-info/intro-buildoptions.json']
    common = {str(p.relative_to(ROOT)): digest(p) for p in paths}
    output = HERE/'captured_cells_v1'/label
    output.mkdir(exist_ok=True)
    lock = (output/".lock").open("w")
    fcntl.flock(lock, fcntl.LOCK_EX | fcntl.LOCK_NB)
    failed = False
    for cell in (329, 157, 88):
        source = HERE/'captured_cells_v1'/('cell%d' % cell)
        case = output/('cell%d' % cell)
        case.mkdir(exist_ok=True)
        fixture = source/'replay.dat'
        identities = dict(common)
        for p in (fixture, source/'input.json'):
            identities[str(p.relative_to(ROOT))] = digest(p)
        provenance = case/'provenance.json'
        result_path = case/'result.json'
        if provenance.exists():
            if json.loads(provenance.read_text())['sha256'] != identities:
                raise RuntimeError('Checkpoint identity changed; use a new label: %s' % case)
            if result_path.exists():
                result = json.loads(result_path.read_text())
                for name, expected in result['artifact_sha256'].items():
                    assert digest(case/name) == expected, name
                print('REUSE', case, 'exit', result['exit_code'], flush=True)
                failed = failed or result['exit_code'] != 0
                continue
        env = dict(os.environ, OMP_NUM_THREADS='1', OPENBLAS_NUM_THREADS='1', MKL_NUM_THREADS='1',
                   TMPDIR=str(BUILD/'compile-tmp'),SU2_NATIVE_BL_REPLAY=str(fixture))
        command = ['taskset','-c','0','nice','-n','19','timeout','--kill-after=5s','90s',
                   str(BUILD/'UnitTests/test_driver'),'[NativeFacetedVolumeReplay3D]','--abort','--reporter','compact']
        provenance.write_text(json.dumps({'state':'PRE_EXECUTION_IDENTITY','sha256':identities,
                                         'command':command,'input':str(fixture.relative_to(ROOT)),
                                         'scope':'Exact recorded first-trial field; no CFD, no accepted-mesh or independent integral proof.'},indent=2)+'\n')
        print('RUN',case,flush=True)
        start = time.monotonic()
        with (case/'run.log').open('w') as log:
            result = subprocess.run(command,cwd=str(case),env=env,stdout=log,stderr=subprocess.STDOUT)
        artifacts = {p.name:digest(p) for p in case.iterdir() if p.is_file() and p.name not in ('provenance.json','result.json')}
        data = {'exit_code':result.returncode,'seconds':time.monotonic()-start,'artifact_sha256':artifacts,
                'scope':'Fixture finite/SPD, positive rule, original barycentrics and volume/first/second moments; numerical density accuracy separately required.'}
        result_path.write_text(json.dumps(data,indent=2)+'\n')
        print('DONE',case,'exit',result.returncode,'seconds',data['seconds'],flush=True)
        print((case/'run.log').read_text()[-1600:],flush=True)
        failed = failed or result.returncode != 0
    return int(failed)

if __name__ == '__main__':
    sys.exit(main())
