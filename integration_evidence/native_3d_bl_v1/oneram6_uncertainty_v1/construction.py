#!/usr/bin/env python3
"""One bounded production-backend replay of the first provisional M6 target. Never advance CFD."""
import json, os, subprocess, sys, time
from pathlib import Path
from run import HERE, ROOT, BUILD, BINARY, pins, sha


def main():
    source = HERE / 'construction_first_trial_v1'
    label = sys.argv[1] if len(sys.argv) > 1 else 'construction_interior_edge_v1'
    assert '/' not in label and label not in ('.', '..')
    folder = HERE / label
    folder.mkdir(exist_ok=True)
    saved = json.loads((source / 'provenance.json').read_text())['identity']
    for name in ('Coarse.su2', 'Coarse.su2.native_ref', 'donor_flow.dat', 'sensor_original.bin'):
        origin = source / name
        assert sha(origin) == saved[str(origin.relative_to(ROOT))]
        dest = folder / name
        if not dest.exists():
            dest.symlink_to(Path(os.path.relpath(origin.resolve(), folder)))
        assert dest.resolve().is_relative_to(ROOT) if hasattr(Path, 'is_relative_to') else ROOT in dest.resolve().parents
        assert sha(dest) == sha(origin)
    config = folder / 'run.cfg'
    if config.exists():
        assert config.read_bytes() == (source / 'run.cfg').read_bytes()
    else:
        config.write_bytes((source / 'run.cfg').read_bytes())
    identity = dict(pins())
    paths = [Path(__file__).resolve(), config] + [folder / name for name in ('Coarse.su2', 'Coarse.su2.native_ref', 'donor_flow.dat', 'sensor_original.bin')]
    paths += [ROOT / p for p in ('Common/src/adaptation/CNativeRemesher3D.cpp', 'SU2_CFD/src/drivers/CSinglezoneDriver.cpp',
                                'Common/src/adaptation/CNativeField3D.cpp', 'UnitTests/SU2_CFD/adaptation/CNativeRemesher3D_tests.cpp')]
    identity.update({str(path.relative_to(ROOT)): sha(path) for path in paths})
    proof = folder / 'provenance.json'
    if proof.exists():
        assert json.loads(proof.read_text()) == identity, 'Changed provenance; preserve the previous experiment.'
    else:
        proof.write_text(json.dumps(identity, indent=2) + '\n')
    result = folder / 'result.json'
    if result.exists():
        report = json.loads(result.read_text())
        assert report['provenance_sha256'] == sha(proof) and report['log_sha256'] == sha(folder / 'run.log')
        print('REUSE', json.dumps(report), flush=True)
        return
    assert not (folder / 'live.json').exists(), 'Check the recorded process before restarting an unfinished job.'
    command = ['taskset', '-c', '0-3', 'nice', '-n', '19', '/usr/bin/time', '-v', '-o', 'resources.txt',
               'timeout', '--kill-after=10s', '600s', 'mpirun', '-np', '4', '--bind-to', 'none',
               str(BINARY), '[NativeFrozenVolume3D]', '--abort', '--reporter', 'compact']
    env = dict(os.environ, OMP_NUM_THREADS='1', OPENBLAS_NUM_THREADS='1', MKL_NUM_THREADS='1',
               TMPDIR=str(BUILD / 'compile-tmp'), SU2_NATIVE_FROZEN_CONFIG=str(config),
               SU2_NATIVE_FROZEN_SENSOR3D=str(folder / 'sensor_original.bin'))
    start = time.monotonic()
    with (folder / 'run.log').open('w') as log:
        child = subprocess.Popen(command, cwd=folder, env=env, stdout=log, stderr=subprocess.STDOUT)
        (folder / 'live.json').write_text(json.dumps({'pid': child.pid, 'command': command, 'started': time.time()}, indent=2) + '\n')
        code = child.wait()
    report = {'exit_code': code, 'elapsed_seconds': time.monotonic() - start, 'command': command,
              'provenance_sha256': sha(proof), 'log_sha256': sha(folder / 'run.log'),
              'resource_sha256': sha(folder / 'resources.txt'), 'CFD_steps': 0,
              'normalization_accuracy_established': False, 'solution_transfer_performed': False}
    output = folder / 'construction_result.json'
    if output.exists():
        report['construction'] = json.loads(output.read_text())
        report['construction_sha256'] = sha(output)
    result.write_text(json.dumps(report, indent=2) + '\n')
    (folder / 'live.json').rename(folder / 'terminal_process.json')
    print(json.dumps(report), flush=True)
    assert code == 0, 'Inspect saved rejection/resource/timeout evidence; do not infer accepted construction.'


if __name__ == '__main__':
    main()
