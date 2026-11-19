"""Seal the FP0 process audit and confirm the active production solver is unchanged."""

from pathlib import Path
import json
import subprocess

from reuse_objects import ACTIVE, TEST_FILES
from run_fp0 import ITEM, REPO, cut, progress, sha, verified_build
from seal_f import identity_rows


def audit():
    binary, build = verified_build()
    paths = {path for pattern in ('*_launch.json', '*_wait.json', 'status.json') for path in ITEM.glob(pattern)}
    for root in (ITEM / 'cases', ITEM / 'G1', ITEM / 'recoveries'):
        for pattern in ('*_launch.json', '*_wait.json', 'status.json'):
            paths.update(root.rglob(pattern))
    identities = {(row['pid'], row['start_ticks']): row
                  for path in paths for row in identity_rows(cut.read(path))}
    observations = ITEM / 'actual_runtime_observations.jsonl'
    if observations.is_file():
        for line in observations.read_text().splitlines():
            for row in identity_rows(json.loads(line)):
                identities[row['pid'], row['start_ticks']] = row
    assert identities and all(cut.terminal(row) for row in identities.values()), identities
    n22 = cut.read(ITEM.parent / 'N22/final_terminal_audit.json')
    active = sha(ACTIVE / 'SU2_CFD/src/SU2_CFD')
    assert active == n22['binary_sha256'] == build['main_solver_sha256']
    modified = subprocess.check_output(['git', 'diff', '--name-only', '03323fa621', '--', 'UnitTests'],
                                       cwd=REPO, text=True).splitlines()
    additional = subprocess.check_output(['git', 'ls-files', '--others', '--exclude-standard', '--', 'UnitTests'],
                                         cwd=REPO, text=True).splitlines()
    expected_tests = set(map(str, TEST_FILES))
    assert set(modified) | set(additional) == expected_tests
    production = ['Common', 'SU2_CFD', 'SU2_DEF', 'SU2_DOT', 'SU2_GEO', 'SU2_SOL',
                  'SU2_PY', 'meson.build', 'meson_options.txt']
    subprocess.run(['git', 'diff', '--exit-code', '03323fa621', '--', *production],
                   cwd=REPO, check=True, stdout=subprocess.DEVNULL)
    preflight = ITEM / 'final_host_preflight.json'
    subprocess.run([cut.PYTHON, str(cut.PREFLIGHT), str(preflight)], check=True)
    assert not cut.read(preflight)['active_OWNED_heavy_processes']
    pins = Path(build.get('source_pins', ITEM / 'test_source_pins.json'))
    result = ITEM / 'final_terminal_audit.json'
    cut.save(result, {'build': build['build'], 'all_owned_handles_terminal': True,
             'identities': sorted(identities.values(), key=lambda row: (row['pid'], row['start_ticks'])),
             'source_pins_verified': len(cut.read(pins)), 'source_pins_sha256': sha(pins),
             'test_binary_sha256': sha(binary), 'active_production_solver_sha256': active,
             'host_preflight': str(preflight), 'test_only_paths': sorted(expected_tests)})
    progress(f'{len(identities)} owned process identities verified terminal. The active N22 solver hash is unchanged.')
    return result


if __name__ == '__main__':
    print(audit())
