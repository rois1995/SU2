"""Build both SC1 binaries and authenticate them against the frozen DEF1 source pins."""

from argparse import ArgumentParser
from pathlib import Path
import runpy
import subprocess
import sys

HERE = Path(__file__).resolve().parent
DEF1 = HERE.parent / 'DEF1'
BASE = '862717ac8f'
ROOTS = ('Common', 'SU2_CFD', 'UnitTests', 'TestCases', 'config_template.cfg')
ORDER = HERE / 'inputs/SC1_GEOMETRY_CERTIFICATES.md'
ORDER_SHA = '10280d0d70d04ed4bb7d4721d295055035bdceb2b8a3fe4149be1847704e5eb2'
SOURCE_CHECK = HERE / 'reports/source_check.md'
PATCH = HERE / 'production.diff'
BASE_BUILD = DEF1 / 'build681'

inherited = runpy.run_path(str(DEF1 / 'build_fp3.py'))
BUILD, cut, verify = (inherited[name] for name in ('BUILD', 'cut', 'verify'))
inherited['terminal_build'].__globals__['BASE'] = BASE
TARGETS = ('SU2_CFD/src/SU2_CFD', 'UnitTests/test_driver')


def git(*args):
    return subprocess.check_output(['git', *args], cwd=cut.REPO)


def from_base(path):
    """Authenticate committed DEF1 evidence without needing the old binaries."""
    assert path.read_bytes() == git('show', f'{BASE}:{path.relative_to(cut.REPO)}'), path
    return path


def verify_order():
    assert cut.sha(ORDER) == ORDER_SHA
    assert ORDER.with_suffix('.sha256').read_text().split() == [ORDER_SHA, ORDER.name]


def verify_production():
    assert not git('status', '--porcelain', '-uno', '--', *ROOTS)
    assert git('diff', '--no-ext-diff', BASE, '--', *ROOTS) == PATCH.read_bytes()
    assert (cut.REPO / 'AGENTS.md').read_bytes() == git('show', f'{BASE}:AGENTS.md')
    changed = git('diff', '--name-only', BASE, '--', *ROOTS).decode().splitlines()
    assert changed and all((cut.REPO / name).is_file() for name in changed), changed
    return sorted(changed)


def baseline():
    receipt = cut.read(from_base(BASE_BUILD / 'build_status.json'))
    manifest = from_base(BASE_BUILD / 'source_pins.json')
    assert receipt['passed'] and receipt['build'] == 681
    assert cut.sha(manifest) == receipt['source_pins_sha256']
    return receipt, cut.read(manifest)


def source_pins(changed):
    verify_order()
    _, pins = baseline()
    order = cut.REPO / 'AGENTS.md'
    changed = {str(cut.REPO / name) for name in changed} | {str(order)}
    verify({name: digest for name, digest in pins.items() if name not in changed})
    evidence = [ORDER, ORDER.with_suffix('.sha256'), SOURCE_CHECK, PATCH, HERE / 'build_fp3.py',
                *(from_base(DEF1 / name) for name in ('build_fp3.py', 'run_controls.py', 'legacy_config.py'))]
    pins.update({str(path): cut.sha(path) for path in [*map(Path, changed), *evidence]})
    return pins


def graph():
    return {name: cut.sha(BUILD / name) for name in ('build.ninja', 'compile_commands.json')}


def build(label):
    changed = verify_production()
    pins = source_pins(changed)
    previous, _ = baseline()
    before = graph()
    out = HERE / f'build{label}'
    out.mkdir()
    manifest = out / 'source_pins.json'
    cut.save(manifest, pins)
    wait = inherited['terminal_build'](out, BUILD, TARGETS, 'build')
    verify(pins)
    assert verify_production() == changed
    plan = cut.read(out / 'build_plan.json')
    solver, driver = (BUILD / target for target in TARGETS)
    binary_sha = cut.sha(solver) if solver.is_file() else None
    test_sha = cut.sha(driver) if driver.is_file() else None
    warnings = inherited['warnings'](out / 'build_outer.log')
    passed = (wait['returncode'] == 0 and binary_sha and test_sha
              and binary_sha != previous['binary_sha256'] and test_sha != previous['test_binary_sha256'])
    receipt = {
        'state': 'terminal', 'passed': bool(passed), 'build': label, 'exit_code': wait['returncode'],
        'base': BASE, 'baseline_build': 681, 'source_commit': plan['source_commit'],
        'pins_verified': True, 'changed': changed, 'source_pins_sha256': cut.sha(manifest),
        'production_diff_sha256': cut.sha(PATCH), 'graph_before': before, 'graph_after': graph(),
        'binary': str(solver), 'binary_sha256': binary_sha, 'test_binary': str(driver),
        'test_binary_sha256': test_sha, 'warnings': warnings, 'wait': wait,
    }
    cut.save(out / 'build_status.json', receipt)
    return receipt['passed']


def verify_build(label):
    out = HERE / f'build{label}'
    receipt = cut.read(out / 'build_status.json')
    manifest = out / 'source_pins.json'
    assert receipt['passed'] and receipt['build'] == label and receipt['base'] == BASE
    assert cut.sha(manifest) == receipt['source_pins_sha256']
    assert verify_production() == receipt['changed']
    assert not git('diff', '--name-only', receipt['source_commit'], '--', *ROOTS)
    verify(cut.read(manifest))
    for key in ('binary', 'test_binary'):
        assert cut.sha(Path(receipt[key])) == receipt[key + '_sha256']
    assert graph() == receipt['graph_after']
    assert cut.terminal(receipt['wait']['controller'])
    return receipt


if __name__ == '__main__':
    parser = ArgumentParser(description=__doc__)
    parser.add_argument('label', type=int)
    raise SystemExit(not build(parser.parse_args().label))
