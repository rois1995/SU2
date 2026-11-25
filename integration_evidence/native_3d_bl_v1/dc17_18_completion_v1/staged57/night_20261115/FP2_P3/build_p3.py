"""Build both FP2-P3 binaries against the frozen witness closure."""

from argparse import ArgumentParser
from pathlib import Path
import re
import subprocess
import sys

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parent / 'FP2_M1'))
from build_m1 import BUILD, TMP, TOOLS, cut, verify  # noqa: E402

BASE = '8f1325ef35'
BASE_BUILD = HERE.parent / 'FP2_P2/build672'
SOURCE = 'Common/src/adaptation/CNativeAffineReference.cpp'
CHANGED = {SOURCE}
PATCH = HERE / 'production.diff'


def verify_production():
    roots = ('Common', 'SU2_CFD', 'UnitTests')
    changed = set(subprocess.check_output(
        ['git', 'diff', '--name-only', BASE, '--', *roots], cwd=cut.REPO, text=True).splitlines())
    assert changed == CHANGED, changed
    patch = subprocess.check_output(['git', 'diff', '--no-ext-diff', BASE, '--', SOURCE], cwd=cut.REPO)
    assert patch == PATCH.read_bytes(), PATCH


def build(label):
    assert Path('/proc/1/comm').read_text().strip() == 'systemd'
    verify_production()
    baseline = cut.read(BASE_BUILD / 'build_status.json')
    prior = BASE_BUILD / 'source_pins.json'
    assert baseline['passed'] and cut.sha(prior) == baseline['source_pins_sha256']
    pins = cut.read(prior)
    paths = [cut.REPO / name for name in sorted(CHANGED)]
    allowed = set(map(str, paths))
    verify({name: digest for name, digest in pins.items() if name not in allowed})
    paths += sorted((HERE / 'inputs').iterdir()) + [Path(__file__).resolve(), HERE / 'reports/source_check.md', PATCH]
    pins.update({str(path): cut.sha(path) for path in paths})
    out = HERE / f'build{label}'
    out.mkdir()
    manifest = out / 'source_pins.json'
    cut.save(manifest, pins)
    targets = ['SU2_CFD/src/SU2_CFD', 'UnitTests/test_driver']
    argv = ['env', 'OMP_NUM_THREADS=1', 'OPENBLAS_NUM_THREADS=1', 'MKL_NUM_THREADS=1',
            f'NINJA={TOOLS / "ninja"}', f'TMPDIR={TMP}', 'nice', '-n', '19', '/usr/bin/time',
            '-v', '-o', str(out / 'build.time'), str(TOOLS / 'ninja'), '-C', str(BUILD), '-j', '4', *targets]
    plan = {'argv': argv, 'base': BASE, 'baseline_build': 672, 'baseline_pins_sha256': cut.sha(prior),
            'production_diff_sha256': cut.sha(PATCH), 'source_pins_sha256': cut.sha(manifest)}
    cut.save(out / 'build_plan.json', plan)
    try:
        wait = cut.run_gate(out, 'build', argv)
    except RuntimeError:
        wait = cut.read(out / 'build_wait.json')
    verify(pins)
    verify_production()
    warnings = [line for line in (out / 'build_outer.log').read_text(errors='replace').splitlines()
                if re.search(r'(?i)\bwarning:', line)]
    solver, test_driver = (BUILD / target for target in targets)
    solver_sha, test_sha = cut.sha(solver), cut.sha(test_driver)
    passed = (wait['returncode'] == 0 and wait['terminal'] and cut.terminal(wait['controller'])
              and not warnings and solver_sha != baseline['binary_sha256'] and test_sha != baseline['test_binary_sha256'])
    cut.save(out / 'build_status.json', {
        'state': 'terminal', 'passed': passed, 'build': label, 'exit_code': wait['returncode'],
        'pins_verified': True, 'changed': sorted(CHANGED), 'source_pins_sha256': plan['source_pins_sha256'],
        'production_diff_sha256': plan['production_diff_sha256'],
        'graph_after': {name: cut.sha(BUILD / name) for name in ('build.ninja', 'compile_commands.json')},
        'binary': str(solver), 'binary_sha256': solver_sha, 'test_binary': str(test_driver),
        'test_binary_sha256': test_sha, 'warnings': warnings, 'new_warnings': warnings, 'wait': wait,
    })
    return passed


def verify_build(label):
    out = HERE / f'build{label}'
    receipt = cut.read(out / 'build_status.json')
    manifest = out / 'source_pins.json'
    assert receipt['passed'] and receipt['build'] == label and cut.sha(manifest) == receipt['source_pins_sha256']
    verify_production()
    verify(cut.read(manifest))
    for path, digest in (('binary', 'binary_sha256'), ('test_binary', 'test_binary_sha256')):
        assert cut.sha(Path(receipt[path])) == receipt[digest]
    assert all(cut.sha(BUILD / name) == digest for name, digest in receipt['graph_after'].items())
    assert cut.terminal(receipt['wait']['controller'])
    return receipt


if __name__ == '__main__':
    parser = ArgumentParser(description=__doc__)
    parser.add_argument('label', type=int)
    raise SystemExit(not build(parser.parse_args().label))
