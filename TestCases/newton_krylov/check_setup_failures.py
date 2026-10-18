"""Check that common Newton setup failures use the normal collective SU2 error path."""
import argparse
import json
import os
from pathlib import Path
import subprocess

from report_state import option, sha


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--binary', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--ranks', type=int, default=1)
    parser.add_argument('--threads', type=int, default=1)
    args = parser.parse_args()
    if args.ranks < 1 or args.threads < 1:
        parser.error('ranks and threads must be positive')
    binary = args.binary.resolve(strict=True)
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    config = '''SOLVER= NAVIER_STOKES
MACH_NUMBER= 0.2
REYNOLDS_NUMBER= 1e6
MESH_FORMAT= RECTANGLE
MESH_BOX_SIZE= 5,5,5
MARKER_HEATFLUX= (y_minus,0)
MARKER_FAR= (x_minus,x_plus,y_plus)
CONV_NUM_METHOD_FLOW= ROE
MGLEVEL= 0
ITER= 1
OUTPUT_FILES= NONE
NEWTON_KRYLOV= YES
NEWTON_EQUATIONS= (FLOW)
NEWTON_KRYLOV_IPARAM= (0,0,1,0)
NEWTON_KRYLOV_DPARAM= (0,0.1,0,0,1)
'''
    actuator = option(config, 'NEWTON_KRYLOV_DPARAM', '(0,0.1,0,1e-6,1)') + '''
ACTDISK_TYPE= BC_THRUST
MARKER_ACTDISK= (inlet,outlet,1,0,1000,1,0,1000)
'''
    trials = [('finite_difference', config, 'Invalid legacy Newton finite-difference'),
              ('actuator_disk', actuator, 'NEWTON_EQUATIONS currently excludes actuator-disk feedback')]
    results = []
    for name, settings, expected in trials:
        directory = output / name
        directory.mkdir()
        (directory / 'run.cfg').write_text(settings)
        command = ['mpirun', '--bind-to', 'none', '-n', str(args.ranks), str(binary), 'run.cfg']
        with (directory / 'log.txt').open('x') as log:
            process = subprocess.run(command, cwd=directory,
                env=dict(os.environ, OMP_NUM_THREADS=str(args.threads), OMP_WAIT_POLICY='PASSIVE'),
                stdout=log, stderr=subprocess.STDOUT, timeout=30)
        text = (directory / 'log.txt').read_text()
        assert process.returncode != 0 and expected in text, text
        assert 'Error in' in text and 'terminate called' not in text, text
        assert not list(directory.glob('restart*')), 'Rejected setup must not write a restart'
        results.append(dict(name=name, command=command, exit_code=process.returncode,
                            config_sha256=sha(directory / 'run.cfg')))
    (output / 'results.json').write_text(json.dumps(dict(trials=results,
        binary_sha256=sha(binary)), indent=2) + '\n')
    print('Collective Newton setup failure checks passed.')


if __name__ == '__main__':
    main()
