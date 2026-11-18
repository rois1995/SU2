"""Verify and publish the M29 diagnostic and the restored default build."""

import gzip
import hashlib
from pathlib import Path
import re
import subprocess
import sys

from checkpoint_a import PART_A, A_BUILD
from report_m29 import check
from run_a import N19, SOURCES, cut
from run_b import F_PATHS
from run_de import checkpoint, git

RESTORED = cut.PHASE / 'build583'


def process_ids(value):
    if isinstance(value, dict):
        if {'pid', 'start_ticks'} <= value.keys():
            yield value['pid'], value['start_ticks']
        for item in value.values():
            yield from process_ids(item)
    elif isinstance(value, list):
        for item in value:
            yield from process_ids(item)


def terminal_audit(cases):
    receipts = [N19 / 'A_controller_launch.json', A_BUILD / 'terminal_handles.json',
                RESTORED / 'terminal_handles.json', RESTORED / 'build_wait.json']
    receipts.extend(PART_A.glob('*_wait.json'))
    receipts.extend(N19.glob('PartB*_launch.json'))
    receipts.extend(N19.glob('Restore*_launch.json'))
    receipts.extend((N19 / 'PartB/R3_prelaunch_controller_source').glob('*_wait.json'))
    receipts.extend(Path(path) for path in cut.read(PART_A / 'checkpoint_paths.json') if
                    Path(path).name in {'n11_terminal_receipt.json', 'primary_terminal_receipt.json',
                                        'status.json', 'terminal_handles.json'})
    for case in cases:
        receipts += [case / 'status.json', case.parent / ('run2_wait.json' if case.name.endswith('_run2')
                                                         else 'run1_wait.json')]
        build, = re.findall(r'^full(\d+)_run', case.name)
        directory = cut.PHASE / f'build{build}'
        receipts += [directory / 'terminal_handles.json', directory / 'build_wait.json']
    identities = [{'pid': pid, 'start_ticks': ticks}
                  for pid, ticks in sorted({row for receipt in receipts for row in process_ids(cut.read(receipt))})]
    assert all(cut.terminal(row) for row in identities)
    pins = cut.read(RESTORED / 'source_pins.json')
    assert pins == cut.read(N19 / 'A_checkpoint_source_pins.json')
    assert all(cut.sha(path) == digest for path, digest in pins.items())
    status = cut.read(RESTORED / 'build_status.json')
    assert status['exit_code'] == 0 and status['pins_verified']
    assert all(cut.sha(status[name]) == status[f'{name}_sha256'] for name in ('binary', 'test_binary'))
    host = RESTORED / 'restored_host_preflight.json'
    assert not cut.read(host)['active_OWNED_heavy_processes']
    audit = {'build': 583, 'source_pins_verified': len(pins), 'binary_sha256': status['binary_sha256'],
             'test_binary_sha256': status['test_binary_sha256'], 'identities': identities,
             'all_owned_handles_terminal': True, 'host_preflight': str(host)}
    cut.save(N19 / 'terminal_audit.json', audit)
    return audit


def archive_case(case):
    paths = [*case.glob('*.json'), case / 'run.cfg', case / 'outer.time.txt']
    logs = [case / name for name in ('run.log', 'history.csv', 'host_resources.jsonl')]
    logs += [path for path in sorted(case.glob('rank_output/*/rank.*/*'))
             if path.is_file() and path.name in ('stdout', 'stderr')]
    for log in logs:
        packed = cut.pack(log)
        with gzip.open(packed, 'rb') as source:
            assert hashlib.file_digest(source, 'sha256').hexdigest() == cut.sha(log)
        paths.append(packed)
    return paths


def main():
    assert Path('/proc/1/comm').read_text().strip() == 'systemd'
    case, = list(map(Path, sys.argv[1:]))
    case = case.resolve()
    diagnostics = cut.read(N19 / 'diagnostic_checkpoint.json')
    a_commit = cut.read(N19 / 'A_checkpoint.json')['commit']
    revert = git('rev-parse', 'HEAD')
    assert diagnostics['commit'] in git('log', '-1', '--format=%B')
    assert set(git('diff-tree', '--no-commit-id', '--name-only', '-r', 'HEAD').splitlines()) == set(F_PATHS)
    assert not git('diff', a_commit, 'HEAD', '--name-only', '--', *sorted(set(SOURCES) | set(F_PATHS)))
    build, = map(int, re.findall(r'^full(\d+)_run', case.name))
    cases = sorted(case.parent.glob('full*_run*'))
    audit = terminal_audit(cases)
    result, _ = check(case)
    restored = N19 / 'Restored.md'
    restored.write_text(
        f"Part A was pushed at `{a_commit[:10]}`. Diagnostic build{build} used "
        f"`{diagnostics['commit'][:10]}`. Its three-path overlay was reverted by `{revert[:10]}`.\n\n"
        f"Fresh default build583 reverified {audit['source_pins_verified']:,} source pins and both binary hashes. "
        f"The audit verified {len(audit['identities'])} owned identities terminal and found no active owned computation. "
        "[Terminal audit](terminal_audit.json)\n"
    )
    assert all(path.is_file() for path in (case / 'status.json', N19 / 'PartA.md', N19 / 'source_check.md'))
    rank_logs = []
    for path in cases:
        rank_logs.extend(archive_case(path))
    subprocess.run([cut.PYTHON, str(N19 / 'report_m29.py'), 'report', str(case), '--build', str(build),
                    '--part-a', str(N19 / 'PartA.md'), '--restored', str(restored)],
                   check=result['passed'])
    report = N19 / f'reports/M29_{build}.md'
    counts = [row['counts'] for row in result['ranks'].values()]
    state = ('passed' if result['passed'] else 'incomplete')
    if result['first_difference']:
        disposition = f" The first witness difference was {result['first_difference']}."
    else:
        disposition = ' All four N18 failure comparisons matched.'
    note = (f"N19 {state}. Part A build581 checkpoint `{a_commit[:10]}`. Build{build} printed "
            f"level counts by rank {counts}.{disposition} Diagnostic reverted at `{revert[:10]}`. "
            f"Default build583 restored. {audit['source_pins_verified']:,} verified source pins and "
            f"{len(audit['identities'])} terminal owned identities.")
    progress = cut.NIGHT / 'progress.md'
    text = progress.read_text()
    current = re.compile(r'## Current item — N19, measurement M29\n\n.*?\n\n(?=## Current state)', re.S)
    text, replacements = current.subn(f'## Current state — N19\n\n{note} [M29 report](N19/reports/{report.name}). '
                                     'Wait for the next user order.\n\n', text, count=1)
    assert replacements == 1
    progress.write_text(text + f'\n{note}\n')
    handoff = cut.REPO / 'HANDOFF_Codex.md'
    text = handoff.read_text()
    title, body = text.split('\n\n', 1)
    rel_report = report.relative_to(cut.REPO)
    handoff.write_text(f'{title}\n\nN19 M29 {state}. Default build583 is active. '
                       f'[Measurement and evidence]({rel_report})\n\n' +
                       body.replace('and build579 is active', 'with build579 restored at that checkpoint')
                       .replace('The work order ends at N7. Resume on a new user order.',
                                'N19 ends with its measurement report. Resume on a new user order.'))
    questions = cut.NIGHT / 'QUESTIONS_FOR_REVIEWER.md'
    with questions.open('a') as ledger:
        ledger.write(f'\nN19 A commit reference for its implementation questions: `{a_commit}`.\n')
    paths = [progress, handoff, questions, restored, report, report.with_name('M29_dump.txt'), *rank_logs,
             N19 / 'A_checkpoint.json', N19 / 'terminal_audit.json', N19 / 'diagnostic_checkpoint.json',
             N19 / 'run_b.py', N19 / 'report_m29.py', N19 / 'finish_m29.py']
    paths += list(N19.glob('PartB*_launch.json')) + list(N19.glob('Restore*_launch.json'))
    paths += list(N19.glob('PartB*_controller.log')) + list(N19.glob('Restore*_controller.log'))
    paths += list(case.parent.glob('*.json'))
    correction = case.parent / 'R3_prelaunch_controller_source'
    if correction.is_dir():
        paths += [case.parent / 'prelaunch_correction.md', *correction.glob('*.json'),
                  *correction.glob('*.log'), *correction.glob('*/run.cfg')]
    paths += [RESTORED / 'build.py', *RESTORED.glob('*.json')]
    for attempt in cases:
        number, = re.findall(r'^full(\d+)_run', attempt.name)
        directory = cut.PHASE / f'build{number}'
        paths += [directory / 'build.py', *directory.glob('*.json')]
    head = checkpoint('Report M29 and verify restored default native 3D build', paths)
    print(head, flush=True)


if __name__ == '__main__':
    main()
