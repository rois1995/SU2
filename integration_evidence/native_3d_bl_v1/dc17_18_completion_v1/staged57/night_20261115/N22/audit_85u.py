"""Verify live host identities and the build left active after N22."""

from argparse import ArgumentParser
from pathlib import Path
import subprocess

from gq_baseline import ITEM, WORK, check_build, cut
from seal_f import identity_rows


def audit(build):
    assert Path('/proc/1/comm').read_text().strip() == 'systemd'
    builds = [cut.PHASE / f'build{label}' for label in {589, 590, 591, build}]
    scopes = [cut.PHASE / f'N22{part}/validation' for part in ('M', 'C')]
    workflows = [cut.PHASE / f'round_8_5u_{part}' for part in ('M', 'C')]
    roots = [ITEM, WORK, *builds, *scopes, *workflows]
    for scope in scopes:
        receipt = scope / 'g4_case_paths.json'
        if receipt.is_file():
            roots.extend(Path(row['case']) for row in cut.read(receipt)['new_cases'])
    documents = {path for root in roots if root.is_dir() for pattern in
                 ('*_launch.json', '*_wait.json', '*terminal*.json', 'status.json', 'g4_case_paths.json',
                  'safeguard_process_receipt.json') for path in root.rglob(pattern)}
    identities = {(row['pid'], row['start_ticks']): row
                  for path in documents for row in identity_rows(cut.read(path))}
    live = [row for row in identities.values() if not cut.terminal(row)]
    assert identities and not live, live
    active = cut.PHASE / f'build{build}'
    status = check_build(active)
    pins = cut.read(active / 'source_pins.json')
    assert all(cut.sha(path) == digest for path, digest in pins.items())
    for name in ('binary', 'test_binary'):
        assert cut.sha(status[name]) == status[f'{name}_sha256']
    preflight = ITEM / 'final_host_preflight.json'
    subprocess.run([cut.PYTHON, str(cut.PREFLIGHT), str(preflight)], check=True)
    assert not cut.read(preflight)['active_OWNED_heavy_processes']
    result = ITEM / 'final_terminal_audit.json'
    cut.save(result, {'build': build, 'identities': sorted(identities.values(), key=lambda row: row['pid']),
                      'all_owned_handles_terminal': True, 'source_pins_verified': len(pins),
                      'source_pins_sha256': cut.sha(active / 'source_pins.json'),
                      'binary_sha256': status['binary_sha256'], 'test_binary_sha256': status['test_binary_sha256'],
                      'host_preflight': str(preflight)})
    return result


if __name__ == '__main__':
    parser = ArgumentParser(description=__doc__)
    parser.add_argument('build', type=int)
    print(audit(parser.parse_args().build))
