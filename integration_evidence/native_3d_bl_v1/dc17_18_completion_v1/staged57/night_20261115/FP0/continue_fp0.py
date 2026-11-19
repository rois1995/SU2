"""Continue FP0 measurements after the isolated build is terminal."""

import sys
import time

from run_fp0 import ITEM, cut, run
from seal_build import seal


if __name__ == '__main__':
    label = int(sys.argv[1]) if len(sys.argv) == 2 else 592
    launch = cut.read(ITEM / ('build_controller_launch.json' if label == 592
                              else f'build{label}_controller_launch.json'))
    while not cut.terminal(launch):
        time.sleep(5)
    if not (ITEM / f'build{label}.json').is_file():
        raise RuntimeError('The isolated compile stopped before producing a test binary receipt.')
    if label == 592:
        seal()
    else:
        assert cut.read(ITEM / f'build{label}.json')['passed']
    sys.argv = [str(ITEM / 'run_fp0.py'), 'ordered']
    run()
