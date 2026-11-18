"""Archive the passed phase-D workflow and verify its terminal identities."""
import gzip
import hashlib
from pathlib import Path
import shutil
import subprocess

from run_g5 import ROOT, WORKSPACE
from run_identity import cut


def audit(full, restart, safeguards):
    main = cut.read(full / 'status.json')
    disk = cut.read(restart / 'post_terminal_handles.json')
    assert main['all_owned_handles_terminal'] and disk['all_owned_handles_terminal']
    identities = main['owned_identities'] + [row['identity'] for row in disk['owned_identity_checks']]
    identities.extend([disk['controller_identity'], disk['controller_wrapper_identity']])
    for filename in ('phase_D_continuation_launch.json', 'G5_campaign_launch.json',
                     'D1_campaign_launch.json', 'D2_campaign_launch.json',
                     'standing_safeguards_campaign_launch.json'):
        identities.append(cut.read(ROOT / filename))
    for workspace in (WORKSPACE, ROOT / 'safeguards'):
        identities.extend(cut.read(path)['controller'] for path in workspace.glob('*_wait.json'))
    for item in safeguards['safeguards'].values():
        receipt = cut.read(Path(item['case']) / 'safeguard_process_receipt.json')
        assert receipt['state'] == 'terminal' and not receipt['live_owned_descendants']
        identities += receipt['identities']
    unique = {(row['pid'], row['start_ticks']): row for row in identities}
    assert all(cut.terminal(row) for row in unique.values())
    manifest = cut.PHASE / 'build572/source_pins.json'
    pins = cut.read(manifest)
    assert all(cut.sha(path) == digest for path, digest in pins.items())
    subprocess.run([cut.PYTHON, str(cut.PREFLIGHT), str(ROOT / 'final_host_preflight.json')],
                   cwd=cut.REPO, check=True)
    result = {'actual_host_proc': True, 'all_owned_handles_terminal': True,
              'source_pins_verified': len(pins), 'source_pins_sha256': cut.sha(manifest),
              'identities': [dict(row, terminal=True) for row in unique.values()]}
    cut.save(ROOT / 'phase_D_terminal_audit.json', result)
    return result


def archive_logs(full, restart, safeguards, accepted):
    logs = [full / name for name in ('run.log', 'host_resources.jsonl')]
    logs.extend(full / name for name in accepted['rank_stdout_sha256'])
    logs.extend(restart / name for name in ('run.log', 'resources.jsonl'))
    for item in safeguards['safeguards'].values():
        logs.extend(Path(item['case']) / name for name in ('run.log', 'resources.jsonl'))
    records = {}
    for path in logs:
        archive = cut.pack(path)
        with gzip.open(archive, 'rb') as stream:
            digest = hashlib.file_digest(stream, 'sha256').hexdigest()
        assert digest == cut.sha(path)
        records[str(archive.relative_to(ROOT))] = {'archive_sha256': cut.sha(archive), 'raw_sha256': digest}
    cut.save(ROOT / 'log_archives.json', records)


def preserve_binaries():
    build = cut.read(cut.PHASE / 'build572/build_status.json')
    target = ROOT / 'qualified_build572'
    target.mkdir()
    binaries = {}
    for key in ('binary', 'test_binary'):
        original = Path(build[key])
        digest = build[f'{key}_sha256']
        assert cut.sha(original) == digest
        copy = target / original.name
        shutil.copyfile(original, copy)
        copy.chmod(0o555)
        assert cut.sha(copy) == digest
        binaries[key] = {'original': str(original), 'local_copy': str(copy), 'sha256': digest}
    cut.save(target / 'manifest.json', {'build': 572, 'git_payloads': False, 'binaries': binaries})


def main():
    assert Path('/proc/1/comm').read_text().strip() == 'systemd'
    g5 = cut.read(WORKSPACE / 'acceptance_summary.json')
    safeguards = cut.read(ROOT / 'safeguards/acceptance_summary.json')
    assert g5['passed'] and safeguards['passed'] and safeguards['build'] == 572
    full = WORKSPACE / 'full572'
    restart = WORKSPACE / 'disk_restart_full572_event2'
    accepted = cut.read(full / 'n8_payload_identity.json')
    assert accepted['passed'] and all(cut.sha(full / path) == digest for path, digest in accepted['hashes'].items())
    result = audit(full, restart, safeguards)
    archive_logs(full, restart, safeguards, accepted)
    preserve_binaries()
    cut.save(ROOT / 'acceptance_summary.json', {'passed': True, 'build': 572,
             'D1': cut.read(ROOT / 'D1_checkpoint.json'), 'D2': cut.read(ROOT / 'D2_checkpoint.json'),
             'G5_seconds': g5['seconds_total'], 'isolated_restart_seconds': g5['isolated_restart']['seconds'],
             'source_pins_verified': result['source_pins_verified'],
             'terminal_identities': len(result['identities'])})


if __name__ == '__main__':
    main()
