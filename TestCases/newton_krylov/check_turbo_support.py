"""Check that common Newton rejects unsupported turbomachinery boundary averages."""
import argparse
import json
import os
from pathlib import Path
import subprocess

from report_state import sha


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
    common = '''SOLVER= RANS
KIND_TURB_MODEL= SST
MACH_NUMBER= 0.2
REYNOLDS_NUMBER= 1e6
CONV_NUM_METHOD_FLOW= ROE
CONV_NUM_METHOD_TURB= SCALAR_UPWIND
MGLEVEL= 0
MESH_FORMAT= RECTANGLE
MESH_BOX_SIZE= 5,5,5
ITER= 1
OUTPUT_FILES= NONE
'''
    turbo = '''MARKER_TURBOMACHINERY= (inlet,outlet)
TURBOMACHINERY_KIND= AXIAL
TURBO_PERF_KIND= TURBINE
'''
    markers = dict(
        turbo=turbo + 'MARKER_RIEMANN= (inlet,TOTAL_CONDITIONS_PT,101325,300,1,0,0, '
                      'outlet,STATIC_PRESSURE,101000,0,0,0,0)\n',
        giles=turbo + 'MARKER_GILES= (inlet,TOTAL_CONDITIONS_PT,101325,300,1,0,0,1,1, '
                      'outlet,STATIC_PRESSURE,101000,0,0,0,0,1,1)\n')
    expected = 'NEWTON_EQUATIONS currently excludes turbomachinery boundary averaging'
    records = []
    for kind, marker in markers.items():
        for selection in ['NONE', '(FLOW)', '(TURB)', '(FLOW,TURB)']:
            name = kind + '_' + selection.replace('(', '').replace(')', '').replace(',', '_')
            directory = output / name
            directory.mkdir()
            config = common + marker + 'NEWTON_EQUATIONS= ' + selection + '\n'
            if selection == '(FLOW,TURB)':
                config += 'NEWTON_COUPLED= YES\n'
            (directory / 'run.cfg').write_text(config)
            command = ['mpirun', '--bind-to', 'none', '-n', str(args.ranks), str(binary), 'run.cfg', '--dryrun']
            with (directory / 'log.txt').open('x') as log:
                process = subprocess.run(command, cwd=directory,
                    env=dict(os.environ, OMP_NUM_THREADS=str(args.threads), OMP_WAIT_POLICY='PASSIVE'),
                    stdout=log, stderr=subprocess.STDOUT, timeout=30)
            text = (directory / 'log.txt').read_text()
            assert (expected in text) == (selection != 'NONE'), text
            if selection == 'NONE':
                assert process.returncode == 0 and 'DRY_RUN option enabled' in text, text
            else:
                assert process.returncode != 0, 'Common Newton must reject the unsupported configuration'
            assert not list(directory.glob('restart*')), 'A rejected setup must not create a restart'
            records.append(dict(kind=kind, selection=selection, command=command, exit_code=process.returncode,
                                config_sha256=sha(directory / 'run.cfg')))
    (output / 'results.json').write_text(json.dumps(dict(binary_sha256=sha(binary), ranks=args.ranks,
        threads=args.threads, trials=records), indent=2) + '\n')
    print('All eight native/common turbomachinery capability checks passed.')


if __name__ == '__main__':
    main()
