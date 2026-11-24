"""Build the FP2-M1 test driver on the build664 production baseline."""

from argparse import ArgumentParser
from pathlib import Path
import re
import subprocess
import sys

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parent / 'FP1'))
from build_fp1 import BUILD, TMP, TOOLS, cut, verify  # noqa: E402

BASE = 'af5844543d'
BASE_BUILD = HERE.parent / 'FP2_C3/build664'
SOURCES = (
    'UnitTests/SU2_CFD/adaptation/CNativeLocalCoreLab_tests.cpp',
    'UnitTests/SU2_CFD/adaptation/fixtures/native_volume_handover.inc',
    'UnitTests/SU2_CFD/adaptation/fixtures/native_m1_observer.inc',
    'UnitTests/SU2_CFD/adaptation/fixtures/native_m1_support_fixture.inc',
)


def build(label):
    if Path('/proc/1/comm').read_text().strip() != 'systemd':
        raise RuntimeError('FP2-M1 build requires actual host process receipts.')
    subprocess.run(['git', 'diff', '--exit-code', BASE, '--', 'Common', 'SU2_CFD'], cwd=cut.REPO, check=True)
    receipt = cut.read(BASE_BUILD / 'build_status.json')
    old_manifest = BASE_BUILD / 'source_pins.json'
    if not receipt['passed'] or cut.sha(old_manifest) != receipt['source_pins_sha256']:
        raise RuntimeError('Build664 baseline manifest differs from the sealed build.')
    solver, test_driver = Path(receipt['binary']), Path(receipt['test_binary'])
    if cut.sha(solver) != receipt['binary_sha256']:
        raise RuntimeError('Solver binary differs from build664.')
    pins = cut.read(old_manifest)
    allowed = {str(cut.REPO / path) for path in SOURCES}
    verify({path: digest for path, digest in pins.items() if path not in allowed})
    extra = sorted(HERE.glob('*.py')) + sorted((HERE / 'inputs').iterdir()) + [HERE / 'source_check.md']
    pins.update({str(path): cut.sha(path) for path in [*map(Path, allowed), *extra]})
    out = HERE / f'build{label}'
    out.mkdir()
    manifest = out / 'source_pins.json'
    cut.save(manifest, pins)
    argv = ['env', 'OMP_NUM_THREADS=1', 'OPENBLAS_NUM_THREADS=1', 'MKL_NUM_THREADS=1',
            f'NINJA={TOOLS / "ninja"}', f'TMPDIR={TMP}', 'nice', '-n', '19', '/usr/bin/time',
            '-v', '-o', str(out / 'build.time'), str(TOOLS / 'ninja'), '-C', str(BUILD),
            '-j', '4', 'UnitTests/test_driver']
    cut.save(out / 'build_plan.json', {'argv': argv, 'base': BASE, 'baseline_build': 664,
                                     'baseline_pins_sha256': cut.sha(old_manifest),
                                     'solver_before': receipt['binary_sha256']})
    try:
        wait = cut.run_gate(out, 'build', argv)
    except RuntimeError:
        wait = cut.read(out / 'build_wait.json')
    verify(pins)
    warnings = [line for line in (out / 'build_outer.log').read_text(errors='replace').splitlines()
                if re.search(r'(?i)\bwarning:', line)]
    solver_sha = cut.sha(solver)
    passed = (wait['returncode'] == 0 and wait['terminal'] and cut.terminal(wait['controller'])
              and not warnings and solver_sha == receipt['binary_sha256'])
    cut.save(out / 'build_status.json', {
        'state': 'terminal', 'passed': passed, 'build': label, 'exit_code': wait['returncode'],
        'pins_verified': True, 'changed': [], 'source_pins_sha256': cut.sha(manifest),
        'graph_after': {name: cut.sha(BUILD / name) for name in ('build.ninja', 'compile_commands.json')},
        'binary': str(solver), 'binary_sha256': solver_sha, 'test_binary': str(test_driver),
        'test_binary_sha256': cut.sha(test_driver), 'warnings': warnings, 'new_warnings': warnings, 'wait': wait,
    })
    return passed


if __name__ == '__main__':
    parser = ArgumentParser(description=__doc__)
    parser.add_argument('label', type=int)
    raise SystemExit(not build(parser.parse_args().label))
