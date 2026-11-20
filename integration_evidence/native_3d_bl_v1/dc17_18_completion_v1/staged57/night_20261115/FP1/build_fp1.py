"""Seal an FP1 build using the active incremental graph and qualified baseline pins."""

from argparse import ArgumentParser
from pathlib import Path
import re
import subprocess

from run_g1 import ITEM, cut, sha

BUILD = cut.REPO / 'integration_evidence/build-integrated-v3-p5-20261114'
TOOLS = cut.REPO / 'integration_evidence/build-integrated-v2/tools'
TMP = TOOLS.parent / 'compile-tmp'
BASE = 'e40d7e3795'


def source_pins():
    pins = cut.read(cut.PHASE / 'build591/source_pins.json')
    areas = ['AGENTS.md', 'Common', 'SU2_CFD', 'UnitTests']
    changed = subprocess.check_output(['git', 'diff', '--name-only', BASE, '--', *areas],
                                      cwd=cut.REPO, text=True).splitlines()
    added = [
        'Common/include/adaptation/CNativeLocalCore3D.hpp',
        *(str(path.relative_to(cut.REPO)) for path in
          (cut.REPO / 'Common/src/adaptation/native_local').glob('*.cpp')),
        *(str(path.relative_to(cut.REPO)) for path in
          (cut.REPO / 'UnitTests/Common/adaptation').glob('CNativeLocalCore3D*_tests.cpp')),
        *(str(path.relative_to(cut.REPO)) for path in
          (cut.REPO / 'UnitTests/SU2_CFD/adaptation').glob('CNativeLocalCore*_tests.cpp')),
    ]
    for path in set(changed + added):
        path = cut.REPO / path
        pins[str(path)] = sha(path)
    verify(pins)
    return pins


def verify(pins):
    for path, digest in pins.items():
        if sha(Path(path)) != digest:
            raise RuntimeError(f'Unexpected build input: {path}')


def build(work, label, solver):
    if Path('/proc/1/comm').read_text().strip() != 'systemd':
        raise RuntimeError('FP1 builds require actual host resource receipts.')
    work.mkdir(parents=True, exist_ok=False)
    audit = cut.read(ITEM.parent / 'FP0/final_terminal_audit.json')
    assert audit['all_owned_handles_terminal'] and all(cut.terminal(row) for row in audit['identities'])
    pins = source_pins()
    manifest = work / 'source_pins.json'
    cut.save(manifest, pins)
    active_solver = BUILD / 'SU2_CFD/src/SU2_CFD'
    test_binary = BUILD / 'UnitTests/test_driver'
    targets = ['UnitTests/test_driver']
    if solver:
        targets.append('SU2_CFD/src/SU2_CFD')
    solver_before = sha(active_solver)
    env = ['env', 'OMP_NUM_THREADS=1', 'OPENBLAS_NUM_THREADS=1', 'MKL_NUM_THREADS=1',
           f'NINJA={TOOLS / "ninja"}', f'TMPDIR={TMP}']
    argv = [*env, 'nice', '-n', '19', '/usr/bin/time', '-v', '-o', str(work / 'build.time'),
            str(TOOLS / 'ninja'), '-C', str(BUILD), '-j', '4', *targets]
    graph = lambda: {name: sha(BUILD / name) for name in ('build.ninja', 'compile_commands.json')}
    cut.save(work / 'build_plan.json', {'argv': argv, 'graph_before': graph(),
             'source_pins_sha256': sha(manifest), 'solver_before': solver_before})
    try:
        wait = cut.run_gate(work, 'build', argv)
    except RuntimeError:
        wait = cut.read(work / 'build_wait.json')
    assert wait['terminal'] and cut.terminal(wait['controller'])
    verify(pins)
    warning_pattern = re.compile(r'(?i)\bwarning:')
    lines = (work / 'build_outer.log').read_text(errors='replace').splitlines()
    warnings = [line for line in lines if warning_pattern.search(line)]
    baseline = cut.read(ITEM.parent / 'FP0/build592_initial_warning_receipt.json')['warnings']
    new_warnings = [line for line in warnings if line not in baseline]
    assert solver or sha(active_solver) == solver_before
    ok = wait['returncode'] == 0 and not new_warnings
    cut.save(work / 'build_status.json', {
        'state': 'terminal', 'passed': ok, 'build': label, 'exit_code': wait['returncode'],
        'pins_verified': True, 'changed': [], 'source_pins_sha256': sha(manifest), 'graph_after': graph(),
        'binary': str(active_solver), 'binary_sha256': sha(active_solver),
        'test_binary': str(test_binary), 'test_binary_sha256': sha(test_binary),
        'warnings': warnings, 'new_warnings': new_warnings, 'wait': wait,
    })
    return ok


if __name__ == '__main__':
    parser = ArgumentParser(description=__doc__)
    parser.add_argument('work', type=Path)
    parser.add_argument('label', type=int)
    parser.add_argument('--solver', action='store_true')
    args = parser.parse_args()
    raise SystemExit(not build(args.work.resolve(), args.label, args.solver))
