"""Publish the N20 report after the terminal build and campaign have been verified."""

from datetime import date
from pathlib import Path
import re
import subprocess
import sys

from checkpoint_a import SAFEGUARDS
from run_a import FIXTURES, N20, WORK, cut
from run_de import checkpoint, git
from run_w import BRANCH, G5, SCOPED
from seal_f import archive, identity_rows


def gate_roots():
    return (N20, FIXTURES, WORK, SAFEGUARDS, SCOPED, BRANCH / 'safeguards', G5,
            *(cut.PHASE / f'build{number}' for number in (584, 585, 586, 587) if
              (cut.PHASE / f'build{number}').is_dir()))


def verify_terminal(active_build):
    receipts = set()
    for root in gate_roots():
        if root.is_dir():
            receipts.update(root.rglob('*_launch.json'))
            receipts.update(root.rglob('*_wait.json'))
            receipts.update(root.rglob('*terminal*receipt.json'))
            receipts.update(root.rglob('terminal_handles.json'))
            receipts.update(root.rglob('safeguard_process_receipt.json'))
            receipts.update(root.rglob('g4_case_paths.json'))
            receipts.update(root.rglob('post_terminal_handles.json'))
            receipts.update(root.rglob('status.json'))
    identities = {}
    for path in receipts:
        for row in identity_rows(cut.read(path)):
            identities[row['pid'], row['start_ticks']] = row
    assert identities and all(cut.terminal(row) for row in identities.values())
    build = cut.PHASE / f'build{active_build}'
    pins = cut.read(build / 'source_pins.json')
    assert all(cut.sha(path) == digest for path, digest in pins.items())
    status = cut.read(build / 'build_status.json')
    assert status['pins_verified'] and status['exit_code'] == 0
    assert all(cut.sha(status[name]) == status[f'{name}_sha256'] for name in ('binary', 'test_binary'))
    host = N20 / 'final_host_preflight.json'
    subprocess.run([cut.PYTHON, str(cut.PREFLIGHT), str(host)], check=True)
    assert not cut.read(host)['active_OWNED_heavy_processes']
    out = N20 / 'terminal_audit.json'
    cut.save(out, {'build': active_build, 'source_pins_verified': len(pins),
                   'identities': sorted(identities.values(), key=lambda row: (row['pid'], row['start_ticks'])),
                   'all_owned_handles_terminal': True, 'binary_sha256': status['binary_sha256'],
                   'test_binary_sha256': status['test_binary_sha256'], 'host_preflight': str(host)})
    return out, host


def cases_to_archive(root):
    yield root
    yield from (case for case in (root / 'cases').glob('*') if case.is_dir())
    if root == G5:
        yield from (case for case in root.iterdir() if case.is_dir() and
                    case.name.startswith(('full', 'disk_restart_full')))
    pairs = root / 'g4_case_paths.json'
    if pairs.is_file():
        yield from (Path(row['case']) for row in cut.read(pairs)['new_cases'])


def evidence():
    roots = [N20, SCOPED, BRANCH / 'safeguards', G5, cut.PHASE / 'build585', cut.PHASE / 'build586',
             cut.PHASE / 'build587', cut.PHASE / 'round_8_5s_A/G5']
    roots = [root for root in roots if root.is_dir()]
    artifacts, archives = set(), {}
    for root in roots:
        for case in cases_to_archive(root):
            artifacts.update(path for path in case.iterdir() if path.is_file() and
                             path.suffix in ('.json', '.py', '.cfg', '.md'))
            logs = [path for path in case.glob('*.log') if path.is_file()]
            logs += [case / name for name in ('host_resources.jsonl', 'resources.jsonl',
                                             'history.csv', 'continued508_history.csv') if (case / name).is_file()]
            logs += [path for path in case.glob('rank_output/**/rank.*/*') if path.name in ('stdout', 'stderr')]
            for path in logs:
                packed, row = archive(path)
                artifacts.add(packed)
                archives[str(packed)] = row
    manifest = N20 / 'log_archives.json'
    cut.save(manifest, archives)
    artifacts.add(manifest)
    artifacts.update((N20 / 'reports').glob('*'))
    artifacts.update(path for path in (cut.PHASE / 'build585_aliases.py',) if path.is_file())
    return artifacts


def update_docs(data, audit):
    progress = cut.NIGHT / 'progress.md'
    text = progress.read_text()
    replaced, count = re.subn(r'## Current item — N20, step 8\.5s\n\n.*?\n\n(?=## Current state)',
                              '## Current state — N20\n\n' + data['progress_current'] +
                              ' Wait for the next user order.\n\n', text, count=1, flags=re.S)
    assert count == 1
    replaced = re.sub(r'(?m)^Updated \d{4}-\d{2}-\d{2}\.', f'Updated {date.today()}.', replaced, count=1)
    progress.write_text(replaced + f'\nN20. Final audit verified {len(audit["identities"])} terminal owned '
                        'identities and the active source and binaries.\n')
    handoff = cut.REPO / 'HANDOFF_Codex.md'
    heading, n19, rest = handoff.read_text().split('\n\n', 2)
    n19 = n19.replace('Default build583 is active. ', '')
    handoff.write_text(f'{heading}\n\n{data["handoff_current"]}\n\n{n19}\n\n' +
                       rest.replace('N19 ends with its measurement report.', 'N20 ends with its report.'))
    how = cut.REPO / 'HowToProceed.md'
    heading, old, rest = how.read_text().split('\n\n', 2)
    old = old.replace('and build579 is active', 'with build579 restored at that checkpoint')
    how.write_text(f'{heading}\n\n{data["proceed_current"]}\n\n{old}\n\n{rest}')
    files = [progress, handoff, how]
    if 'timings_section' in data:
        timings = cut.REPO / 'Handbook/ADAPTATION_TIMINGS.md'
        with timings.open('a') as out:
            out.write('\n\n' + data['timings_section'].rstrip() + '\n')
        files.append(timings)
    return files


def main():
    assert Path('/proc/1/comm').read_text().strip() == 'systemd'
    disposition = cut.read(N20 / 'disposition.json')
    active = disposition['active_build']
    assert disposition['active_commit'] == git('rev-parse', 'HEAD')
    audit_path, host = verify_terminal(active)
    options = ['--build', str(disposition['build']), '--active-build', str(active)]
    for key in ('a_status', 'w_status', 'a_commit', 'w_commit', 'revert', 'active_commit', 'diagnosis', 'a_full'):
        if disposition.get(key):
            options += ['--' + key.replace('_', '-'), str(disposition[key])]
    draft = N20 / 'docs_draft.json'
    subprocess.run([cut.PYTHON, str(N20 / 'report_85s.py'), 'write',
                    *options, '--docs-draft', str(draft)], check=True)
    docs = update_docs(cut.read(draft), cut.read(audit_path))
    questions = cut.NIGHT / 'QUESTIONS_FOR_REVIEWER.md'
    with questions.open('a') as out:
        out.write(f'\nN20 A implementation question commit: `{disposition["a_commit"]}`.\n')
    head = checkpoint('Report N20 fine repair and wide-round disposition',
                      [*evidence(), *docs, questions, audit_path, host])
    print(head, flush=True)


if __name__ == '__main__':
    main()
