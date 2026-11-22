"""Build FP2-B on the qualified incremental graph and pin the included test source."""

from argparse import ArgumentParser
from pathlib import Path
import sys

sys.dont_write_bytecode = True
HERE = Path(__file__).resolve().parent
sys.path[:0] = [str(HERE.parent / folder) for folder in ('FP1', 'FP2_B0')]
import build_fp1 as build
import run_b0 as b0


def main():
    parser = ArgumentParser(description=__doc__)
    parser.add_argument('label', type=int)
    parser.add_argument('--solver', action='store_true')
    args = parser.parse_args()
    cut = build.cut
    b0.host()
    b0.verify_terminal()
    for launch in (*HERE.glob('build*/build_launch.json'), *HERE.glob('cases/build*/*/test_launch.json')):
        start = cut.read(launch)
        assert cut.terminal(start['supervisor']), launch
        wait = launch.with_name(launch.name.replace('_launch.json', '_wait.json'))
        row = cut.read(wait)
        assert row['terminal'] and row['controller'] == start['controller'] and cut.terminal(row['controller']), wait
    for processes in HERE.glob('cases/build*/*/owned_processes.json'):
        assert all(cut.terminal(row) for row in cut.read(processes)['identities']), processes
    original_pins = build.source_pins

    def source_pins():
        pins = original_pins()
        path = cut.REPO / 'UnitTests/SU2_CFD/adaptation/fixtures/native_volume_handover.inc'
        if path.is_file():
            pins[str(path)] = build.sha(path)
        return pins

    build.source_pins = source_pins
    return not build.build(HERE / f'build{args.label}', args.label, args.solver)


if __name__ == '__main__':
    raise SystemExit(main())
