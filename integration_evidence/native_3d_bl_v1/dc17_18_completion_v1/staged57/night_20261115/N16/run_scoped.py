"""Bind step 8.5r to the frozen build and qualification adapters."""
from pathlib import Path
import os
import re
import subprocess
import sys

N16 = Path(__file__).resolve().parent
sys.path.insert(0, str(N16.parent / 'N4/section6_3_after_N11/phase_D'))
from run_identity import cut

TEST_SOURCE = 'UnitTests/Common/adaptation/CNativeDistributed3D_tests.cpp'


def prepare_build(previous, build, changes):
    prior = cut.PHASE / f'build{previous}'
    target = cut.PHASE / f'build{build}'
    pins = cut.read(prior / 'source_pins.json')
    for name in changes:
        path = cut.REPO / name
        assert str(path) in pins, path
        pins[str(path)] = cut.sha(path)
    changed = [path for path, digest in pins.items() if cut.sha(path) != digest]
    assert not changed, changed
    target.mkdir()
    cut.save(target / 'source_pins.json', pins)
    script = (prior / 'build.py').read_text().replace(str(previous), str(build))
    (target / 'build.py').write_text(script)
    return target


def check_build(target):
    status = cut.read(target / 'build_status.json')
    handles = cut.read(target / 'terminal_handles.json')
    assert status['pins_verified'] and status['exit_code'] == 0
    assert handles['all_owned_handles_terminal'] and all(cut.terminal(row) for row in handles['identities'])
    log = target / 'build.log'
    warnings = [line for line in log.read_text(errors='replace').splitlines() if re.search(r'\bwarning:', line, re.I)]
    assert not warnings, warnings
    return status


def test_command(binary, selector):
    command = ['env', 'OMP_NUM_THREADS=1', 'OPENBLAS_NUM_THREADS=1', 'MKL_NUM_THREADS=1']
    return [*command, 'nice', '-n', '19', 'timeout', '180', str(binary), selector]


def part_a():
    workspace = N16 / 'A'
    workspace.mkdir()
    build = 573
    changes = ['Common/src/adaptation/CNativeDistributed3D.cpp', TEST_SOURCE]
    target = prepare_build(572, build, changes)
    cut.run_gate(target, 'build', [cut.PYTHON, str(target / 'build.py')])
    status = check_build(target)
    wait = cut.run_gate(workspace, 'A4', test_command(status['test_binary'], '[NativeQualityRoundBounds3D]'))
    assert 'All tests passed' in (workspace / 'A4_outer.log').read_text()
    cut.save(workspace / 'acceptance_summary.json', {'passed': True, 'build': build, 'changes': changes,
             'checked_peer_pairs': 2, 'fixture_seconds': wait['elapsed_seconds'], 'binary_sha256': status['test_binary_sha256']})


if __name__ == '__main__':
    assert Path('/proc/1/comm').read_text().strip() == 'systemd'
    assert sys.argv[1:] == ['A']
    part_a()
