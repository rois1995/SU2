"""Resume FP3-A Step 1 with the P3 scoped evaluator bound by path."""

from argparse import ArgumentParser
from pathlib import Path
import runpy
from types import SimpleNamespace

import run_controls as controls

SELF = Path(__file__).resolve()
SCOPED = controls.P3 / 'run_scoped.py'
original_support = controls.support


def support(build):
    return (*original_support(build), SELF, SCOPED)


if __name__ == '__main__':
    parser = ArgumentParser(description=__doc__)
    parser.add_argument('build', type=int)
    parser.add_argument('control', choices=controls.CHECKS, nargs='?')
    parser.add_argument('--finalize-measurement-failure', action='store_true')
    args = parser.parse_args()
    # Older fixture imports use the same module name and leave it in sys.modules.
    controls.scoped = SimpleNamespace(**runpy.run_path(str(SCOPED)))
    controls.support = support
    names = (args.control,) if args.control else controls.CHECKS
    raise SystemExit(controls.EXIT[controls.run(args.build, names, args.finalize_measurement_failure)])
