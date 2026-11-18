"""Close work order 2 after the N18 outcome is published."""
from datetime import datetime
from os.path import relpath
from pathlib import Path
import subprocess
import sys

from report_summary import WORKSPACE
from run_de import N18, checkpoint, cut, git

sys.path.insert(0, str(cut.PHASE / 'phase_N6'))
from finish_work_order import (DRAFTS, HANDOFF, NEXT_WORK, NIGHT_REPORT, TIMINGS, baseline_failures)

N6 = cut.NIGHT / 'N6'
N6_PHASE = cut.PHASE / 'phase_N6'


def link(label, target, location):
    return f'[{label}]({relpath(target, location.parent)})'


def verify_final(disposition):
    build = disposition['active_build']
    audit_file = N18 / ('F_terminal_audit.json' if disposition['passed'] else 'restored579_terminal_audit.json')
    n18 = cut.read(audit_file)
    n6 = cut.read(N6_PHASE / 'terminal_audit.json')
    for audit in (n6, n18):
        assert audit['actual_host_proc'] and audit['all_owned_handles_terminal']
        assert all(cut.terminal(row) for row in audit['identities'])
    pins = cut.read(cut.PHASE / f'build{build}/source_pins.json')
    assert all(cut.sha(path) == digest for path, digest in pins.items())
    status = cut.read(cut.PHASE / f'build{build}/build_status.json')
    assert all(cut.sha(status[name]) == status[f'{name}_sha256'] for name in ('binary', 'test_binary'))
    host = N18 / 'N7_before_report_preflight.json'
    subprocess.run([cut.PYTHON, str(cut.PREFLIGHT), str(host)], cwd=cut.REPO, check=True)
    assert not cut.read(host)['active_OWNED_heavy_processes']
    return audit_file, host, len(n18['identities'])


def n18_result(disposition, document):
    f = f"`{disposition['commit'][:10]}`"
    de_code = cut.read(N18 / 'DE_checkpoint.json')['commit']
    de = f"`{de_code[:10]}`"
    report = link('N18 report', disposition['report'], document)
    if disposition['passed']:
        accepted = cut.read(WORKSPACE / 'acceptance_summary.json')
        total = accepted['purpose']['smaller_native_max_sum_seconds']
        return (f"D/E passed at {de}. F passed both full runs and their six repeat-identical files, "
                f"with the isolated restart and a purpose sum of {total} s. The accepted source is "
                f"build580 at {f}. {report}")
    difference = disposition['first_difference']
    return (f"D/E passed at {de}. F {disposition['failure'].lower()}. Its first difference was {difference}. "
            f"The F enabling change was reverted at {f}, and build579 is active. {report}")


def night_report(disposition, audit, terminal_count, n6, qualified):
    def render(name):
        values = {'@N6_COMMIT@': f"`{n6['commit'][:10]}`",
                  '@G5@': f"{qualified['G5_seconds']:.3f}",
                  '@RESTART@': f"{qualified['isolated_restart_seconds']:.3f}",
                  '@TERMINAL@': str(terminal_count),
                  '@PINS@': str(qualified['source_pins_verified'])}
        text = (DRAFTS / name).read_text()
        for old, new in values.items():
            text = text.replace(old, new)
        return text.rstrip()

    top = render('night_top.md')
    anchor = '| §4.5 · N7 |'
    state = 'PASSED' if disposition['passed'] else 'FAILED'
    row = (f"| §4.4a · N18 8.5r2 and M28 | {state} | `{disposition['commit'][:10]}` | "
           f"{n18_result(disposition, NIGHT_REPORT)} |\n")
    assert top.count(anchor) == 1
    top = top.replace(anchor, row + anchor)
    old_audit = link('Audit', N6_PHASE / 'terminal_audit.json', NIGHT_REPORT)
    assert old_audit in top
    top = top.replace(old_audit, link('Audit', audit, NIGHT_REPORT), 1)
    top = top.replace(f'{terminal_count} N6 process identities', f'{terminal_count} N18 process identities')
    top = top.replace('See the focused reports and the terminal audit below.',
                      n18_result(disposition, NIGHT_REPORT))
    if not disposition['passed']:
        diagnosis = cut.read(N18 / 'failure_diagnosis.json')
        first = diagnosis['first_difference']
        cause = diagnosis['cause']
        candidate = disposition['F_commit'][:10]
        revert = disposition['commit'][:10]
        top += (f"\n- **N18 F** first differed at {first}. {cause} Candidate `{candidate}` was reverted by "
                f"`{revert}`. " + link('Failure diagnosis', N18 / 'failure_diagnosis.json', NIGHT_REPORT))
    readings = render('night_readings.md')
    readings += ("\n| N18 | The whole-item check records the live repair key, the timings-only entry, "
                 "reservation eligibility, the preserved integer comparisons and the confirmed purpose thresholds. "
                 + link('N18 source check', N18 / 'source_check.md', NIGHT_REPORT) + ' |')
    NIGHT_REPORT.write_text('\n\n'.join((top, readings, baseline_failures().rstrip(),
                                         render('night_history.md'))) + '\n')


def handoff(disposition, audit, terminal_count):
    result = n18_result(disposition, HANDOFF)
    audit_link = link('Terminal audit', audit, HANDOFF)
    n6_report = '[N6 report](Handbook/ADAPTATION_CUT_N6_20261118.md)'
    caution = ('The saved BL stage remains qualified by its fixture gates.')
    text = (
        '# Native adaptation handoff\n\n'
        f'Work order 2 and both 2026-11-18 amendments are complete. {result}\n\n'
        'N6 build578 passed both no-BL adaptation events in 4041.079 s. The six P6c/P7 payloads and 96 '
        'timer snapshots matched. Its isolated restart passed in 234.819 s with zero coordinate error. '
        f'Phase D passed at `c9176e9439`. {caution} {n6_report}\n\n'
        'N16 retained Parts A and B. Its first 64-wide production run failed event-one quality '
        'with one q-bad cell and was reverted by `3276cbb862`. '
        + link('N16 report', cut.NIGHT / 'N16/reports/STEP_8_5R_575.md', HANDOFF) + '\n\n'
        f'The final audit verified {terminal_count} N18 process identities terminal. '
        f'{audit_link} · [Timings](Handbook/ADAPTATION_TIMINGS.md) · '
        '[Night report](Handbook/ADAPTATION_NIGHT_REPORT_20261116.md)\n\n'
        'The work order ends at N7. Resume on a new user order. At session start, read AGENTS.md and progress. '
        + link('Progress', cut.NIGHT / 'progress.md', HANDOFF) + ' · '
        + link('R13 ledger', cut.NIGHT / 'QUESTIONS_FOR_REVIEWER.md', HANDOFF) + '\n'
    )
    HANDOFF.write_text(text)
    NEXT_WORK.write_text('# Work order status\n\nWork order 2 ended at N7. '
                         + n18_result(disposition, NEXT_WORK)
                         + '\n\nWait for the next user order. At the next start, read AGENTS.md and the current progress file. '
                         + link('Progress', cut.NIGHT / 'progress.md', NEXT_WORK)
                         + ' · [Final report](Handbook/ADAPTATION_NIGHT_REPORT_20261116.md)\n')


def update_timings(disposition):
    if not disposition['passed']:
        return []
    receipt = cut.read(WORKSPACE / 'acceptance_summary.json')
    lines = ['', '## Step 8.5r2 — build580', '',
             'Both full workflows passed and produced six matching payloads. '
             + link('N18 gate and M28 report', disposition['report'], TIMINGS), '',
             '| Run | Event | Native max (s) | Preparation max (s) | Construction max (s) | Quality max (s) | Final proof max (s) | Transfer max (s) |',
             '|---|---:|---:|---:|---:|---:|---:|---:|']
    for name in ('run1', 'run2'):
        for event in receipt['events'][name]:
            clocks = event['phase_exclusive_clocks']
            values = [clocks.get(phase, {}).get('seconds_max', 0) for phase in
                      ('preparation', 'construction', 'quality', 'final_proof', 'transfer')]
            lines.append(f"| {name} | {event['event']} | {event['native_seconds']:.3f} | "
                         + ' | '.join(f'{value:.3f}' for value in values) + ' |')
    gate = receipt['purpose']
    lines += ['', f"The confirmed purpose comparison selected {gate['smaller_native_max_sum_seconds']} s "
                  f"across the two event minima, within {gate['combined_limit_seconds']} s. The per-event "
                  'rows use exclusive phase maxima. The native maximum contains nested clocks, and the '
                  'quality scheduling and S measurements are rank-local sums. '
                  + link('Full receipt', WORKSPACE / 'acceptance_summary.json', TIMINGS), '']
    title, body = TIMINGS.read_text().split('\n', 1)
    assert '## Step 8.5r2 — build580' not in body
    TIMINGS.write_text(title + '\n' + '\n'.join(lines) + body)
    return [TIMINGS]


def progress_current(disposition):
    path = cut.NIGHT / 'progress.md'
    text = path.read_text()
    start = text.index('## Current state — ')
    stop = text.index('## Earlier integrated reference — phase C')
    stamp = datetime.now().astimezone().isoformat(timespec='seconds')
    current = ('## Current state — work order 2 complete\n\n'
               f'Updated {stamp}. {n18_result(disposition, path)} All owned processes are terminal. '
               'N7 closes work order 2. Wait for the next user order.\n\n'
               '## N6 and N18 entry reference\n\n')
    details = text[start:stop].split('\n\n', 1)[1]
    path.write_text(text[:start] + current + details + text[stop:])
    return path


def main():
    assert Path('/proc/1/comm').read_text().strip() == 'systemd'
    receipt = N18 / 'F_checkpoint.json'
    final = cut.read(receipt)
    assert final['commit'] == git('rev-parse', 'HEAD')
    audit, host, terminal = verify_final(final)
    n6 = cut.read(N6 / 'qualification_checkpoint.json')
    qualified = cut.read(N6_PHASE / 'acceptance_summary.json')
    night_report(final, audit, terminal, n6, qualified)
    handoff(final, audit, terminal)
    summaries = update_timings(final)
    progress_path = progress_current(final)
    commit = checkpoint('Close work order 2 with the N18 outcome and final handoff',
                        [receipt, host, progress_path, Path(final["report"]), NIGHT_REPORT, HANDOFF, NEXT_WORK,
                         Path(__file__).resolve(), *summaries,
                         *(DRAFTS / name for name in ('night_top.md', 'night_readings.md', 'night_history.md'))])
    print(commit, flush=True)


if __name__ == '__main__':
    main()
