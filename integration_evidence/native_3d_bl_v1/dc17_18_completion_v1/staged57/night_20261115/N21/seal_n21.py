"""Seal N21 checkpoints and verify that the reported active binaries match the source."""

from pathlib import Path
import subprocess

from run_p_scoped import BUILD as P_BUILD, BRANCH as P_BRANCH, N21, WORK as P_WORK, checkpoint_paths, cut
from run_w_scoped import W_BUILD, BRANCH as W_BRANCH, WORK as W_WORK
from run_de import check_build, checkpoint, git
from seal_f import archive, identity_rows


def cases(workspace):
    yield workspace
    if (workspace / 'cases').is_dir():
        yield from (path for path in (workspace / 'cases').iterdir() if path.is_dir())
    if (workspace / 'g4_case_paths.json').is_file():
        yield from (Path(row['case']) for row in cut.read(workspace / 'g4_case_paths.json')['new_cases'])
    yield from (path for path in workspace.iterdir() if path.is_dir() and
                path.name.startswith(('full', 'disk_restart_full')))


def artifacts(workspaces, manifest):
    files = set()
    logs = set()
    for workspace in workspaces:
        if not workspace.is_dir():
            continue
        for case in cases(workspace):
            files.update(path for path in case.iterdir() if path.is_file() and
                         path.suffix in ('.json', '.py', '.cfg', '.md'))
            logs.update(case.glob('*.log'))
            logs.update(case.glob('*.jsonl'))
            logs.update(case / name for name in ('history.csv', 'continued508_history.csv') if (case / name).is_file())
            logs.update(path for path in case.glob('rank_output/**/rank.*/*') if path.name in ('stdout', 'stderr'))
    records = {}
    for path in sorted(logs):
        packed, row = archive(path)
        files.add(packed)
        records[str(packed)] = row
    cut.save(manifest, records)
    files.add(manifest)
    return files


def verify_terminal(workspaces, label, active_build):
    assert Path('/proc/1/comm').read_text().strip() == 'systemd'
    documents = set()
    for root in workspaces:
        if root.is_dir():
            for pattern in ('*_launch.json', '*_wait.json', '*terminal*.json', 'status.json', 'g4_case_paths.json',
                            'safeguard_process_receipt.json'):
                documents.update(root.rglob(pattern))
    identities = {}
    for path in documents:
        for row in identity_rows(cut.read(path)):
            identities[row['pid'], row['start_ticks']] = row
    assert identities and all(cut.terminal(row) for row in identities.values())
    build = cut.PHASE / f'build{active_build}'
    status = check_build(build)
    pins = cut.read(build / 'source_pins.json')
    assert all(cut.sha(path) == digest for path, digest in pins.items())
    for label_key in ('binary', 'test_binary'):
        assert cut.sha(status[label_key]) == status[f'{label_key}_sha256']
    preflight = N21 / f'{label}_host_preflight.json'
    subprocess.run([cut.PYTHON, str(cut.PREFLIGHT), str(preflight)], check=True)
    assert not cut.read(preflight)['active_OWNED_heavy_processes']
    audit = N21 / f'{label}_terminal_audit.json'
    cut.save(audit, {'build': active_build, 'all_owned_handles_terminal': True,
                    'identities': sorted(identities.values(), key=lambda row: (row['pid'], row['start_ticks'])),
                    'source_pins_verified': len(pins), 'source_pins_sha256': cut.sha(build / 'source_pins.json'),
                    'binary_sha256': status['binary_sha256'], 'test_binary_sha256': status['test_binary_sha256'],
                    'host_preflight': str(preflight)})
    return audit, preflight


def seal_p():
    g5 = P_BRANCH / 'G5'
    result = cut.read(g5 / 'acceptance_summary.json')
    assert result['passed'] and result['build'] == P_BUILD
    p_source = cut.read(P_WORK / 'candidate_source.json')
    scoped = cut.read(P_WORK / 'acceptance_summary.json')
    assert scoped['state'] == 'PASSED' and scoped['build'] == P_BUILD
    assert cut.read(N21 / 'fixtures/acceptance_summary.json')['passed']
    assert cut.read(P_BRANCH / 'safeguards/acceptance_summary.json')['passed']
    assert git('rev-parse', 'HEAD') == p_source['base_commit']
    build = cut.PHASE / f'build{P_BUILD}'
    roots = (N21, P_WORK, P_BRANCH / 'safeguards', g5, build)
    audit = verify_terminal(roots, 'P', P_BUILD)
    evidence = artifacts((N21, g5, build), N21 / 'P_log_archives.json')
    evidence.update(N21 / 'inputs' / name for name in ('IMPLEMENTATION_8_5T.md', 'SHA256SUMS'))
    aliases = cut.PHASE / f'build{P_BUILD}_aliases.py'
    if aliases.is_file():
        evidence.add(aliases)
    with (cut.NIGHT / 'progress.md').open('a') as out:
        out.write(f'\nN21 P. Build{P_BUILD} passed scoped identity, safeguards, the two-event G5, all six payload '
                  'identities and the 53.4 s certification ceiling. The P acceptance receipt records the T1 maxima. '
                  'Proceed with W.\n')
    evidence.update([*p_source['source_paths'], *checkpoint_paths(), *audit, cut.NIGHT / 'progress.md',
                     cut.NIGHT / 'QUESTIONS_FOR_REVIEWER.md'])
    commit = checkpoint('N21 P: share the final surface proof across ranks', evidence)
    cut.save(N21 / 'P_checkpoint.json', {'passed': True, 'build': P_BUILD, 'commit': commit,
                                       'g5': str(g5 / 'acceptance_summary.json')})
    print(commit, flush=True)


if __name__ == '__main__':
    seal_p()
