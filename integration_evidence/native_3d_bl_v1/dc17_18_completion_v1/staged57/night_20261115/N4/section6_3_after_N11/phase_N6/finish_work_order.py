"""Publish the qualified N6 report and then close work order 2."""
from collections import defaultdict
from datetime import datetime
from pathlib import Path
import subprocess
import sys

from run_g5 import ROOT, WORKSPACE, cut, progress
from seal_phase import N6
from write_phase_report import HANDBOOK, link

DRAFTS = N6 / 'drafts/final_report'
PHASE_REPORT = HANDBOOK / 'ADAPTATION_CUT_N6_20261118.md'
TIMINGS = HANDBOOK / 'ADAPTATION_TIMINGS.md'
NIGHT_REPORT = HANDBOOK / 'ADAPTATION_NIGHT_REPORT_20261116.md'
HANDOFF = cut.REPO / 'HANDOFF_Codex.md'
NEXT_WORK = cut.REPO / 'HowToProceed.md'


def git(*args):
    return subprocess.check_output(['git', *map(str, args)], text=True, cwd=cut.REPO).strip()


def checkpoint(message, paths):
    assert not git('diff', '--cached', '--name-only')
    files = sorted(set(paths))
    assert files and all(path.is_file() for path in files)
    git('add', '-f', '--', *files)
    git('diff', '--cached', '--check')
    git('commit', '-m', message)
    git('push', 'origin', 'HEAD:refs/heads/codex/native-3d-bl')
    return git('rev-parse', 'HEAD')


def document_timings():
    g5 = cut.read(WORKSPACE / 'acceptance_summary.json')
    events = g5['events']
    native = ' / '.join(f"{row['native_seconds']:.3f}" for row in events)
    coverage = ' / '.join(f"{row['minimum_timer_coverage']:.4%}" for row in events)
    intro = (f"G5 passed in {g5['seconds_total']:.3f} s, with native maxima {native} s and "
             f"minimum timer coverage {coverage}. The isolated restart took {g5['isolated_restart']['seconds']:.3f} s. "
             'The six payloads, old work and timer calls matched the full build574 reference. '
             '[N6 report](ADAPTATION_CUT_N6_20261118.md)')
    rows = ['', '## N6 — build578', '', intro, '',
            '| Final ledger phase | Event 1 exclusive s, min–max | Event 2 exclusive s, min–max | Event 1 calls, sum | Event 2 calls, sum |',
            '|---|---:|---:|---:|---:|']
    for phase in ('preparation', 'metric', 'construction', 'surface', 'quality', 'final_proof', 'transfer', 'output'):
        clocks = [event['phase_exclusive_clocks'].get(phase, {}) for event in events]
        spans = [f"{row.get('seconds_min', 0):.3f}–{row.get('seconds_max', 0):.3f}" for row in clocks]
        sums = [f"{row.get('timer_calls_sum', 0):,}" for row in clocks]
        rows.append(f"| {phase.replace('_', ' ')} | {' | '.join(spans + sums)} |")
    gaps = ' / '.join(f"{row['unattributed_seconds_max']:.3f}" for row in events)
    rows.extend(('', f'Maximum unattributed time was {gaps} s. '
                     'Each rank contributes one set of exclusive times. Event and remesh-root clocks are nested, '
                     'and the build comparisons include host variation. '
                     + link('Per-event timing and work', WORKSPACE / 'acceptance_summary.json'), ''))
    header, body = TIMINGS.read_text().split('\n', 1)
    assert '## N6 — build578' not in body
    TIMINGS.write_text(header + '\n' + '\n'.join(rows) + body)


def phase_paths():
    files = [PHASE_REPORT, TIMINGS, cut.NIGHT / 'progress.md', cut.NIGHT / 'QUESTIONS_FOR_REVIEWER.md',
             Path(__file__).resolve()]
    files += list(N6.glob('*_checkpoint.json')) + list(N6.glob('*_campaign_launch.json'))
    files += list(N6.glob('*_campaign_wait.json')) + list(N6.glob('*_before_edit_preflight.json'))
    for scope in (ROOT, WORKSPACE, ROOT / 'safeguards'):
        files += [path for path in scope.iterdir() if path.is_file() and path.suffix in ('.py', '.json')]
    full = WORKSPACE / 'full578'
    restart = WORKSPACE / 'disk_restart_full578_event2'
    files += [full / 'run.cfg', restart / 'run.cfg', full / 'outer.time.txt']
    files += list(full.glob('*.json')) + list(restart.glob('*.json'))
    guard_summary = cut.read(ROOT / 'safeguards/acceptance_summary.json')
    for row in guard_summary['safeguards'].values():
        files += list(Path(row['case']).glob('*.json'))
    files += [Path(path) for path in cut.read(ROOT / 'log_archives.json')]
    return files


def phase_checkpoint():
    assert cut.read(ROOT / 'acceptance_summary.json')['passed']
    document_timings()
    g5 = cut.read(WORKSPACE / 'acceptance_summary.json')
    progress('N6', f"Build578 passed G5 in {g5['seconds_total']:.3f} s, six payloads and 96 timer snapshots exact. "
                   f"The isolated restart passed in {g5['isolated_restart']['seconds']:.3f} s. "
                   'All standing safeguards passed, source pins verified, and all owned process identities are terminal.')
    head = checkpoint('Qualify the native 3D N6 cuts on build578', phase_paths())
    cut.save(N6 / 'qualification_checkpoint.json', {'commit': head, 'build': 578, 'passed': True})
    progress('N6', f'Qualification pushed as `{head[:10]}`. N18 follows under the 2026-11-18 amendment.')
    print(head, flush=True)


def baseline_failures():
    path = cut.PHASE / 'phase_C/accepted_base_g1_failures.md'
    cases = defaultdict(set)
    for line in path.read_text().splitlines():
        if not line.startswith('| ') or line.startswith('| Case '):
            continue
        case, source, _ = line[2:].split(' | ', 2)
        cases[case].add(source)
    assert len(cases) == 9
    lines = ['## Unit tests that fail on the accepted base', '',
             'G1 matched the adopted N14 case and failed-assertion tables at serial, two and four ranks. '
             'The linked table gives both printed values and the cohort and rank for each failure variant. '
             + link('Complete accepted-base failure table', path), '']
    lines += [f"- {case} · {', '.join(sorted(sources))}" for case, sources in sorted(cases.items())]
    return '\n'.join(lines) + '\n'


def night_checkpoint():
    assert not (cut.NIGHT / 'N18/inputs').exists(), 'N7 must follow N18. Update the closing report for N18 first.'
    audit = cut.read(ROOT / 'terminal_audit.json')
    assert audit['actual_host_proc'] and audit['all_owned_handles_terminal']
    assert all(cut.terminal(row) for row in audit['identities'])
    final_preflight = N6 / 'N7_before_report_preflight.json'
    subprocess.run([cut.PYTHON, str(cut.PREFLIGHT), str(final_preflight)], cwd=cut.REPO, check=True)
    assert not cut.read(final_preflight)['active_OWNED_heavy_processes']
    accepted = cut.read(ROOT / 'acceptance_summary.json')
    qualification = N6 / 'qualification_checkpoint.json'
    n6 = cut.read(qualification)
    assert n6['passed'] and accepted['passed'] and git('rev-parse', 'HEAD') == n6['commit']
    replacements = {
        '@N6_COMMIT@': f"`{n6['commit'][:10]}`",
        '@G5@': f"{accepted['G5_seconds']:.3f}",
        '@RESTART@': f"{accepted['isolated_restart_seconds']:.3f}",
        '@TERMINAL@': str(accepted['terminal_identities']),
        '@PINS@': f"{accepted['source_pins_verified']:,}",
    }

    def render(name):
        text = (DRAFTS / name).read_text()
        for old, new in replacements.items():
            text = text.replace(old, new)
        return text

    NIGHT_REPORT.write_text('\n\n'.join((render('night_top.md').rstrip(),
                           render('night_readings.md').rstrip(), baseline_failures().rstrip(),
                           render('night_history.md').rstrip())) + '\n')
    HANDOFF.write_text(render('HANDOFF_Codex.md'))
    NEXT_WORK.write_text(render('HowToProceed.md'))
    file = cut.NIGHT / 'progress.md'
    text = file.read_text().replace('## Current state — phase C passed', '## Earlier integrated reference — phase C', 1)
    title, body = text.split('\n\n', 1)
    stamp = datetime.now().astimezone().isoformat(timespec='seconds')
    current = (f"## Current state — work order 2 complete\n\nUpdated {stamp}. "
               f"N6 qualification {replacements['@N6_COMMIT@']} accepted build578. "
               f"G5 passed in {replacements['@G5@']} s and the restart in {replacements['@RESTART@']} s. "
               'All six P6c/P7 files and 96 timer snapshots matched. The actual-host audits verified all owned '
               'processes terminal. N16 C was reverted, and N16 A/B remain in the accepted source. '
               'N7 closes the work order. Wait for the next user order.\n\n')
    file.write_text(title + '\n\n' + current + body)
    print(checkpoint('Close work order 2 and update the adaptation handoff',
                     [NIGHT_REPORT, HANDOFF, NEXT_WORK, file, qualification, final_preflight]), flush=True)


if __name__ == '__main__':
    assert Path('/proc/1/comm').read_text().strip() == 'systemd'
    {'N6': phase_checkpoint, 'N7': night_checkpoint}[sys.argv[1]]()
