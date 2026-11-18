"""Verify terminal identities and seal the N6 numerical evidence."""
import gzip
import hashlib
from pathlib import Path
import subprocess

from run_g5 import ROOT, WORKSPACE, cut

N6 = cut.NIGHT / 'N6'
BUILDS = {'Remesh': 576, 'Optimizer': 577, 'Selection': 578}


def identities_from(value):
    if isinstance(value, dict):
        if isinstance(value.get('pid'), int) and isinstance(value.get('start_ticks'), int):
            yield {'pid': value['pid'], 'start_ticks': value['start_ticks']}
        for member in value.values():
            yield from identities_from(member)
    elif isinstance(value, list):
        for member in value:
            yield from identities_from(member)


def scope_receipts():
    receipts = []
    for name, build in BUILDS.items():
        directory = cut.PHASE / f'N6{name}/validation'
        summary = cut.read(directory / 'acceptance_summary.json')
        assert summary['state'] == 'PASSED' and summary['build'] == build
        assert [row['records'] for row in summary['G1'].values()] == [375, 114, 228]
        assert summary['G2']['work'] == 90_809_869 and summary['G3']['passed'] and summary['GQ']['passed']
        receipts.extend(directory.glob('*_wait.json'))
        receipts.append(cut.PHASE / f'build{build}/terminal_handles.json')
        for cohort in ('serial', 'mpi2', 'mpi4'):
            row = cut.read(directory / f'G1_candidate_{cohort}_summary.json')
            assert row['identity_pass']
            receipts.append(Path(row['candidate_attempt']) / 'n11_terminal_receipt.json')
        for gate in ('G2', 'GQ'):
            receipts.append(Path(summary[gate]['comparison']).parent / 'status.json')
        receipts.append(Path(summary['G3']['case']) / 'g3_terminal_receipt.json')
        if name == 'Selection':
            assert summary['G4']['passed']
            receipts.append(directory / 'g4_case_paths.json')
    return receipts


def archive_logs(full, restart, guards, numerical):
    logs = [full / name for name in ('run.log', 'host_resources.jsonl', 'history.csv')]
    logs.extend(full / name for name in numerical['rank_stdout_sha256'])
    logs.extend(restart / name for name in ('run.log', 'resources.jsonl', 'continued508_history.csv'))
    for row in guards['safeguards'].values():
        logs.extend(Path(row['case']) / name for name in ('run.log', 'resources.jsonl'))
    logs.extend(N6.glob('*_campaign.log'))
    records = {}
    for original in logs:
        archive = cut.pack(original)
        with gzip.open(archive, 'rb') as stream:
            digest = hashlib.file_digest(stream, 'sha256').hexdigest()
        assert digest == cut.sha(original), original
        records[str(archive)] = {'archive_sha256': cut.sha(archive), 'raw_sha256': digest}
    return records


def main():
    assert Path('/proc/1/comm').read_text().strip() == 'systemd'
    for launch in N6.glob('*_campaign_launch.json'):
        assert cut.terminal(cut.read(launch)), launch
    g5 = cut.read(WORKSPACE / 'acceptance_summary.json')
    guards = cut.read(ROOT / 'safeguards/acceptance_summary.json')
    assert g5['passed'] and guards['passed'] and guards['build'] == 578
    full = WORKSPACE / 'full578'
    restart = WORKSPACE / 'disk_restart_full578_event2'
    numerical = cut.read(full / 'n8_payload_identity.json')
    assert numerical['passed'] and cut.read(full / 'strict_full_identity.json')['passed']
    assert all(cut.sha(full / name) == digest for name, digest in numerical['hashes'].items())
    assert cut.read(restart / 'disk_restart_validation.json')['disk_restart_verified']
    receipts = [*scope_receipts(), full / 'status.json', restart / 'post_terminal_handles.json']
    receipts.extend(N6.glob('*_campaign_launch.json'))
    for workspace in (WORKSPACE, ROOT / 'safeguards'):
        receipts.extend(workspace.glob('*_wait.json'))
    for row in guards['safeguards'].values():
        receipts.append(Path(row['case']) / 'safeguard_process_receipt.json')
    identities = {}
    for path in receipts:
        for row in identities_from(cut.read(path)):
            identities[(row['pid'], row['start_ticks'])] = row
    assert identities and all(cut.terminal(row) for row in identities.values())
    pins = cut.PHASE / 'build578/source_pins.json'
    source = cut.read(pins)
    assert all(cut.sha(path) == digest for path, digest in source.items())
    subprocess.run([cut.PYTHON, str(cut.PREFLIGHT), str(ROOT / 'final_host_preflight.json')],
                   cwd=cut.REPO, check=True)
    audit = {'actual_host_proc': True, 'all_owned_handles_terminal': True,
             'source_pins_verified': len(source), 'source_pins_sha256': cut.sha(pins),
             'identities': [dict(row, terminal=True) for row in identities.values()]}
    cut.save(ROOT / 'terminal_audit.json', audit)
    status = cut.read(cut.PHASE / 'build578/build_status.json')
    binaries = {key: {'path': status[key], 'sha256': status[f'{key}_sha256']}
                for key in ('binary', 'test_binary')}
    assert all(cut.sha(row['path']) == row['sha256'] for row in binaries.values())
    cut.save(ROOT / 'binary_manifest.json', {'build': 578, 'binaries': binaries})
    cut.save(ROOT / 'log_archives.json', archive_logs(full, restart, guards, numerical))
    cut.save(ROOT / 'acceptance_summary.json', {'passed': True, 'build': 578,
             'substeps': {name: cut.read(N6 / f'{name}_checkpoint.json') for name in BUILDS},
             'G5_seconds': g5['seconds_total'], 'isolated_restart_seconds': g5['isolated_restart']['seconds'],
             'source_pins_verified': len(source), 'terminal_identities': len(identities)})


if __name__ == '__main__':
    main()
