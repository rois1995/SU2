"""Qualify the completed N6 phase and install the next work order amendment."""
from pathlib import Path
import subprocess
import sys
import time

N18 = Path(__file__).resolve().parent
sys.path.insert(0, str(N18.parent / 'N6'))
from run_cut import N6, cut


def run(path, *args):
    subprocess.run([cut.PYTHON, str(path), *args], cwd=cut.REPO, check=True)


def main():
    assert Path('/proc/1/comm').read_text().strip() == 'systemd'
    handle = Path(sys.argv[1]) if len(sys.argv) == 2 else N6 / 'continue_campaign_launch.json'
    active = cut.read(handle)
    while not cut.terminal(active):
        time.sleep(15)
    phase = cut.PHASE / 'phase_N6'
    assert cut.read(phase / 'G5/acceptance_summary.json')['passed']
    assert cut.read(phase / 'safeguards/acceptance_summary.json')['passed']
    run(phase / 'seal_phase.py')
    run(phase / 'write_phase_report.py')
    run(phase / 'finish_work_order.py', 'N6')
    run(N18 / 'apply_order.py')
    cut.save(N18 / 'ready_after_n6.json', {'n6_passed': True, 'amendment_applied': True,
             'next': 'Read the pinned I8R2 completely and do the whole-item N18 source check.'})


if __name__ == '__main__':
    main()
