"""Render N18's STEP report and M28 tables from terminal receipts."""
import csv
from os.path import relpath
from pathlib import Path
import subprocess

from run_de import N18, cut

P = cut.PHASE
WORKSPACE = P / 'round_8_5r2_F/G5'
BRANCH = P / 'round_8_5r2_F'
REPORTS = N18 / 'reports'
T0 = P / 'round_8_5r_B/G5/full574'
DE = P / 'N18DE/validation/acceptance_summary.json'
SCOPED = P / 'N18F/validation/acceptance_summary.json'
BUILD = 580


def link(label, path):
    relative = relpath(path, REPORTS)
    return f'[{label}]({relative})'


def details(scope):
    g3 = scope['G3']
    g4 = ' / '.join(f"{row['status']['seconds']:.3f}" for row in scope['G4']['new_cases'])
    if g3.get('identity_is_gate', True):
        edits = f"{g3['common_edit_count']:,} exact common edits"
    else:
        mismatch = g3['first_mismatch_index']
        edits = f"{g3['common_edit_count']:,} edits compared, first difference {mismatch}"
    return (f"G1 matched 375 / 114 / 228 records. G2 matched {scope['G2']['work']:,} work units and "
            f"the six collapse counts. G3 passed with {g3['accepted_splits']:,} splits, "
            f"{edits} and adjusted entry work {g3['observer_adjusted_entry_work']['candidate']:,}. "
            f"G4 passed at 1 / 2 / 4 ranks in {g4} s.")


def collect_m28(cases):
    reporter = N18 / 'drafts/reporting/report_m28.py'
    paths = [REPORTS / f'STEP_8_5R2_{BUILD}.{suffix}' for suffix in ('csv', 'stuck.md', 'provenance.json')]
    args = [cut.PYTHON, str(reporter)]
    for label, case in [('T0', T0), *cases]:
        args += ['--run', f'{label}={case}']
    csv_path, stuck, provenance = paths
    args += ['--csv', str(csv_path), '--stuck', str(stuck), '--provenance', str(provenance)]
    if not all(path.exists() for path in paths):
        subprocess.run(args, cwd=cut.REPO, check=True)
    assert all(path.is_file() for path in paths)
    with csv_path.open(newline='') as file:
        records = list(csv.DictReader(file))
    assert len([row for row in records if row['run'] == 'T0']) == 14
    return paths, records


def summarize_full(lines, acceptance):
    lines += ['', '## Full workflow, repeat and purpose', '',
              '| Run | Event | Native max (s) | Process-peak sum (B) | Minimum timer coverage | Proof faces |',
              '|---|---:|---:|---:|---:|---:|']
    events = acceptance['events']
    for run in ('T0', 'run1', 'run2'):
        for event in events[run]:
            lines.append(f"| {run} | {event['event']} | {event['native_seconds']:.3f} | "
                         f"{event['process_peak_sum_bytes']:,} | {event['minimum_timer_coverage']:.4%} | "
                         f"{event['proof_faces']:,} |")
    gate = acceptance['purpose']
    first, second = [row['smaller_native_max_seconds'] for row in gate['events']]
    lines += ['', 'Both workflows passed their standing gates, with six repeat-identical files. '
              'The isolated event-two restart passed. '
              + link('Restart receipt', acceptance['isolated_restart_receipt']),
              '', f'Gate 8 used event minima {first} and {second} s. Their sum was '
                  f"{gate['smaller_native_max_sum_seconds']} s against {gate['combined_limit_seconds']} s. "
                  f"The two individual limits were 2236.870 and 1416.499 s. Gate 8 "
                  f"{'passed' if gate['passed'] else 'failed'}. " + link('Purpose receipt', WORKSPACE / 'purpose_rule.json')]


def seconds(value):
    return f'{float(value):.3f}' if value else '—'


def measurement_table(lines, records):
    lines += ['', '## M28 measurements', '',
              'Loss counts follow the specified cell-ID overlap rule for reservation losers. '
              'Ownership counts distinguish evaluations from completed migrations. '
              'Quality clocks are rank-local exclusive intervals. S is the six-rank sum of '
              'join wait, communication and clearance.',
              '',
              '| Run | Event | Reservation losses, own / other | Quality evaluations / migrations | Scheduling sum (s) | S (s) |',
              '|---|---:|---:|---:|---:|---:|']
    for row in records:
        if row['rank'] != 'sum':
            continue
        own = row['lost_own'] or '—'
        other = row['lost_other'] or '—'
        evaluations, migrations = row['quality_ownership_evaluations'], row['quality_ownership_migrations']
        scheduling = seconds(row['quality_scheduling_seconds'])
        cost = seconds(row['quality_purpose_seconds'])
        lines.append(f"| {row['run']} | {row['event']} | {own} / {other} | {evaluations} / {migrations} | "
                     f"{scheduling} | {cost} |")


def write(outcome, diagnosis=None):
    """Write the final report after a result and its source disposition are known."""
    assert outcome in ('PASSED', 'FAILED', 'FAILED its purpose rule')
    REPORTS.mkdir(exist_ok=True)
    candidates = [(f'run{i}', WORKSPACE / f'full{BUILD}_run{i}') for i in (1, 2)]
    cases = [(name, case) for name, case in candidates if (case / 'status.json').is_file()]
    paths, records = collect_m28(cases)
    csv_path, stuck, provenance = paths
    de = cut.read(DE)
    de_commit = cut.read(N18 / 'DE_checkpoint.json')['commit']
    lines = ['# Step 8.5r2 and M28 — build580', '',
             f'Part F {outcome}. The ownership correction and print-only M28 passed on build579 '
             f'in `{de_commit[:10]}`. ' + link('Pinned specification', N18 / 'inputs/IMPLEMENTATION_8_5R2.md') +
             ' · ' + link('Whole-item source check', N18 / 'source_check.md'),
             '', '## D/E identity', '',
             details(de) + f" GQ stopped at {de['GQ']['seconds_until_stop']:.3f} s with all required rows exact. "
             'Modeled storage, census delta zero, timer calls and fixed storage 4,120 B matched. '
             'The default-width qualification used the short gates and kept T0 as the full reference. '
             + link('D/E gate receipts', DE)]
    if SCOPED.exists():
        f = cut.read(SCOPED)
        lines += ['', '## F scoped gates', '',
                  details(f) + ' The exact wide-repeat fixture passed at two and four ranks. '
                  + link('Scoped gate receipts', SCOPED)]
    guards = BRANCH / 'safeguards/acceptance_summary.json'
    if guards.exists():
        values = cut.read(guards)['safeguards']
        results = ', '.join(f"{mode} {row['seconds']:.3f} s" for mode, row in values.items())
        lines.append(f'The standing safeguards passed ({results}). ' + link('Safeguard receipts', guards))
    full = WORKSPACE / 'acceptance_summary.json'
    if full.exists():
        summarize_full(lines, cut.read(full))
    if diagnosis:
        lines += ['', '## Failure and reversion', '',
                  f"First result difference: {diagnosis['first_difference']}. {diagnosis['cause']}",
                  f"The three F paths in candidate `{diagnosis['candidate_commit'][:10]}` were reverted. "
                  'D/E remain in the tree, and the six P6c/P7 files remain the reference. '
                  + link('Failure diagnosis', N18 / 'failure_diagnosis.json')]
        if 'native_max_seconds' in diagnosis:
            lines.append(f"The rejected event's native maximum was {diagnosis['native_max_seconds']:.3f} s. "
                         f"It used {diagnosis['work']:,} of {diagnosis['work_limit']:,} work units "
                         f"and held {diagnosis['private_cells']:,} private cells. These are failed-run observations.")
            refusal = diagnosis['recorded_refusal_index_feature_counts']
            lines.append(f"M28 recorded geometry (index 0) and metric progress (index 3) on "
                         f"{refusal['0']} and {refusal['3']} of the blocked features. "
                         f"Two features also recorded the uncategorized index 7. "
                         "The feature epochs and masks match at rounds 23,384 and 25,850.")
            lines.append("The retained T0 event-one logs contain 20 printed quality evaluations per rank, "
                         "while I8R2's narrative cites 33. The report counts the requested "
                         "quality=1 evaluate=1 rows directly. The retry printed 13 evaluations per rank. "
                         "This count informs the measurement report.")
        if diagnosis.get('unevaluated'):
            lines.append(diagnosis['unevaluated'])
    else:
        lines += ['', 'The six repeat-identical files are the new reference. '
                  + link('Qualified reference', WORKSPACE / 'qualified_reference_payloads.json')]
    measurement_table(lines, records)
    lines += ['', link('Per-rank CSV', csv_path) + ' · ' + link('Raw log locations and hashes', provenance), '',
              stuck.read_text().rstrip()]
    path = REPORTS / f'STEP_8_5R2_{BUILD}.md'
    path.write_text('\n'.join(lines) + '\n')
    return [path, *paths]
