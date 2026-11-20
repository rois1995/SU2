"""Enforce the existing lab wall limit when SU2 only acknowledges SIGTERM."""

import json
import os
from pathlib import Path
import signal
import sys
import time

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parent))
from run_g1 import cut

PROC = Path('/proc')
PAIRS = ('LAB2_V1', 'LAB2_V0')


def details(pid):
    path = PROC / str(pid)
    fields = (path / 'stat').read_text().rsplit(')', 1)[1].split()
    command = (path / 'cmdline').read_bytes().rstrip(b'\0').split(b'\0')
    return {'pid': pid, 'start_ticks': int(fields[19]), 'ppid': int(fields[1]), 'pgid': int(fields[2]),
            'argv': [arg.decode() for arg in command]}


def sample(case, launch, elapsed):
    recipe = cut.read(case / 'command.json')
    controller = launch['controller']
    if cut.terminal(controller):
        return
    parent = details(controller['pid'])
    assert parent['start_ticks'] == controller['start_ticks']
    parent_path = PROC / str(parent['pid'])
    children = (parent_path / 'task' / str(parent['pid']) / 'children').read_text().split()
    if not children:
        return
    assert len(children) == 1
    group = details(int(children[0]))
    expected = recipe['argv'][recipe['argv'].index('timeout'):]
    assert recipe['ranks'] == 1 and recipe['seconds_limit'] == 1200
    assert expected[:2] == ['timeout', '1200'] and expected[-1] == '[.][NativeLocalCoreLab]'
    assert group['argv'] == expected and group['ppid'] == parent['pid']
    assert group['pgid'] == group['pid'] != parent['pgid']
    assert (PROC / str(group['pid']) / 'cwd').resolve() == case
    if (case / 'test_wait.json').exists() or cut.terminal(controller) or cut.terminal(group):
        return
    stop = case / 'hard_wall_stop.json'
    event = {'dataset': case.name, 'controller': parent, 'timeout': group, 'seconds_limit': 1200,
             'elapsed_before_hard_stop': elapsed, 'hard_stop_unix': time.time(), 'signal': 'SIGKILL',
             'reason': 'SU2 can continue its core sweep after timeout sends SIGTERM.'}
    try:
        os.killpg(group['pgid'], signal.SIGKILL)
    except ProcessLookupError:
        return
    cut.save(stop, event)
    print(f"{case.name}: enforced the 1200 s wall limit.", flush=True)


def run():
    assert (PROC / '1/comm').read_text().strip() == 'systemd'
    root = HERE / 'lab'
    launch = cut.read(root / 'LAB1_V1/test_launch.json')
    supervisor = launch['supervisor']
    while not cut.terminal(supervisor):
        delay = 2.
        for name in PAIRS:
            case = root / name
            launch_path = case / 'test_launch.json'
            if not launch_path.is_file() or any((case / filename).is_file()
                                                for filename in ('test_wait.json', 'hard_wall_stop.json')):
                continue
            launch = cut.read(launch_path)
            assert launch['supervisor'] == supervisor
            elapsed = time.time() - launch['unix']
            if elapsed >= 1200:
                try:
                    sample(case, launch, elapsed)
                except (FileNotFoundError, ProcessLookupError):
                    continue
            else:
                delay = min(delay, max(.05, 1200 - elapsed))
        time.sleep(delay)


if __name__ == '__main__':
    run()
