"""Build and authenticate DEF1 with the pinned direct and reverse-AD trees."""

from argparse import ArgumentParser
import hashlib
from pathlib import Path
import re
import subprocess
import sys

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parent / 'FP2_M1'))
from build_m1 import BUILD, TMP, TOOLS, cut, verify  # noqa: E402

BASE = 'efbab5351e'
BASE_BUILD = HERE.parent / 'FP3_E/build680'
AD_BUILD = cut.REPO / 'integration_evidence/build-integrated-ad-v1'
PRODUCTION = (
    'Common/include/CConfig.hpp',
    'Common/include/adaptation/CAdaptationOptions.hpp',
    'Common/include/adaptation/CMMGInterface.hpp',
    'Common/src/CConfig.cpp',
    'Common/src/adaptation/CMMGInterface.cpp',
    'config_template.cfg',
)
FIXTURES = ('UnitTests', 'TestCases')


def git(*args):
    return subprocess.check_output(['git', *args], cwd=cut.REPO)


def source_diff(*paths):
    return git('diff', '--no-ext-diff', BASE, '--', *paths)


def verify_production(label):
    changed = set(git('diff', '--name-only', BASE, '--',
                      'Common', 'SU2_CFD', 'UnitTests', 'TestCases', 'config_template.cfg').decode().splitlines())
    assert {name for name in changed if not name.startswith(('UnitTests/', 'TestCases/'))} == set(PRODUCTION)
    assert source_diff(*PRODUCTION) == (HERE / 'production.diff').read_bytes()
    assert source_diff(*FIXTURES) == (HERE / f'fixture_{label}.diff').read_bytes()
    assert not git('status', '--porcelain', '-uno', '--', 'Common', 'SU2_CFD', 'UnitTests',
                   'TestCases', 'config_template.cfg')
    return sorted(changed)


def source_manifest(label, paths, out):
    baseline = cut.read(BASE_BUILD / 'build_status.json')
    prior = BASE_BUILD / 'source_pins.json'
    assert baseline['passed'] and cut.sha(prior) == baseline['source_pins_sha256']
    pins = cut.read(prior)
    order = cut.REPO / 'AGENTS.md'
    prior_order = cut.read(BASE_BUILD.parent / 'accepted_reference.json')['validation_work_order']
    assert pins[str(order)] == prior_order['sha256']
    assert hashlib.sha256(git('show', f'{prior_order["commit"]}:AGENTS.md')).hexdigest() == prior_order['sha256']
    assert order.read_bytes() == git('show', f'{BASE}:AGENTS.md')
    pins[str(order)] = cut.sha(order)
    changed = {str(cut.REPO / name) for name in paths}
    verify({name: digest for name, digest in pins.items() if name not in changed})
    evidence = [Path(__file__).resolve(), HERE / 'reports/source_check.md', HERE / 'production.diff',
                HERE / f'fixture_{label}.diff', *(HERE / 'inputs').iterdir()]
    pins.update({str(path): cut.sha(path) for path in [*map(Path, changed), *evidence]})
    cut.save(out / 'source_pins.json', pins)
    return pins


def warnings(path):
    return [line for line in path.read_text(errors='replace').splitlines()
            if re.search(r'(?i)\bwarning:', line)]


def terminal_build(out, folder, targets, kind):
    assert Path('/proc/1/comm').read_text().strip() == 'systemd'
    compiler = ['env', 'OMP_NUM_THREADS=1', 'OPENBLAS_NUM_THREADS=1', 'MKL_NUM_THREADS=1',
                f'NINJA={TOOLS / "ninja"}', f'TMPDIR={TMP}', 'nice', '-n', '19', '/usr/bin/time',
                '-v', '-o', str(out / f'{kind}.time'), str(TOOLS / 'ninja'), '-C', str(folder), '-j', '4', *targets]
    cut.save(out / f'{kind}_plan.json', {'argv': compiler, 'base': BASE,
                                        'source_commit': git('rev-parse', 'HEAD').decode().strip(),
                                        'source_pins_sha256': cut.sha(out / 'source_pins.json')})
    try:
        wait = cut.run_gate(out, kind, compiler)
    except RuntimeError:
        wait = cut.read(out / f'{kind}_wait.json')
    assert wait['terminal'] and cut.terminal(wait['controller'])
    return wait


def direct(label):
    changed = verify_production(label)
    out = HERE / f'build{label}'
    out.mkdir()
    pins = source_manifest(label, changed, out)
    targets = ['SU2_CFD/src/SU2_CFD', 'UnitTests/test_driver']
    wait = terminal_build(out, BUILD, targets, 'build')
    verify(pins)
    verify_production(label)
    solver, tests = (BUILD / name for name in targets)
    receipt = {'state': 'terminal', 'build': label, 'exit_code': wait['returncode'],
               'changed': changed, 'source_pins_sha256': cut.sha(out / 'source_pins.json'),
               'production_diff_sha256': cut.sha(HERE / 'production.diff'),
               'source_commit': cut.read(out / 'build_plan.json')['source_commit'], 'pins_verified': True,
               'graph_after': {name: cut.sha(BUILD / name) for name in ('build.ninja', 'compile_commands.json')},
               'warnings': warnings(out / 'build_outer.log'), 'wait': wait}
    receipt.update(passed=wait['returncode'] == 0 and solver.is_file() and tests.is_file(),
                   binary=str(solver), test_binary=str(tests),
                   binary_sha256=cut.sha(solver) if solver.is_file() else None,
                   test_binary_sha256=cut.sha(tests) if tests.is_file() else None)
    cut.save(out / 'build_status.json', receipt)
    return receipt['passed']


def verify_build(label):
    out = HERE / f'build{label}'
    receipt = cut.read(out / 'build_status.json')
    pins = out / 'source_pins.json'
    assert receipt['passed'] and receipt['build'] == label and cut.sha(pins) == receipt['source_pins_sha256']
    verify_production(label)
    verify(cut.read(pins))
    for key in ('binary', 'test_binary'):
        assert cut.sha(receipt[key]) == receipt[f'{key}_sha256']
    assert all(cut.sha(BUILD / name) == digest for name, digest in receipt['graph_after'].items())
    assert cut.terminal(receipt['wait']['controller'])
    return receipt


def reverse_ad(label):
    direct_receipt = verify_build(label)
    out = HERE / f'build{label}'
    options = {row['name']: row['value'] for row in cut.read(AD_BUILD / 'meson-info/intro-buildoptions.json')}
    assert options['enable-autodiff'] and options['enable-tests'] and options['enable-mmg']
    target = 'UnitTests/test_driver_AD'
    wait = terminal_build(out, AD_BUILD, [target], 'build_ad')
    verify_build(label)
    binary = AD_BUILD / target
    receipt = {'passed': wait['returncode'] == 0 and binary.is_file(), 'build': label,
               'source_commit': direct_receipt['source_commit'],
               'source_pins_sha256': direct_receipt['source_pins_sha256'], 'wait': wait,
               'binary': str(binary), 'binary_sha256': cut.sha(binary) if binary.is_file() else None,
               'graph_after': {name: cut.sha(AD_BUILD / name) for name in ('build.ninja', 'compile_commands.json')},
               'warnings': warnings(out / 'build_ad_outer.log')}
    cut.save(out / 'build_ad_status.json', receipt)
    return receipt['passed']


if __name__ == '__main__':
    parser = ArgumentParser(description=__doc__)
    parser.add_argument('label', type=int)
    parser.add_argument('--ad', action='store_true')
    args = parser.parse_args()
    raise SystemExit(not (reverse_ad if args.ad else direct)(args.label))
