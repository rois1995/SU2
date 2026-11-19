#!/usr/bin/env python3
"""Render the N22 report from the actual gate and timing receipts."""

import argparse
from collections import defaultdict
from decimal import Decimal
import json
import os
from pathlib import Path
import sys

sys.dont_write_bytecode = True
from run_g5_85u import G5, HERE, PHASES, ROUND

REPORTS = HERE / 'reports'
STATES = ('PASSED', 'FAILED', 'OPEN for the user', 'NOT STARTED')


def optional(path):
    return json.loads(path.read_text()) if path.is_file() else {}


def link(title, path, root=REPORTS):
    path = Path(path)
    return f'[{title}]({os.path.relpath(path, root)})' if path.is_file() else title


def table(headers, rows):
    return ['| ' + ' | '.join(headers) + ' |', '| ' + ' | '.join('---' for _ in headers) + ' |',
            *('| ' + ' | '.join(map(str, row)) + ' |' for row in rows)]


def number(value):
    return f'{float(value):,.6f}' if value is not None else '—'


def gate_lines(part):
    scope = HERE / part / 'scoped_acceptance.json'
    fixture_path = HERE / part / 'fixtures.json'
    safeguards = ROUND[part] / 'safeguards/acceptance_summary.json'
    data = optional(scope)
    fixtures = data.get('fixtures', optional(fixture_path))
    lines = [f'### Part {part} validation', '']
    for name, row in fixtures.items():
        if not isinstance(row, dict) or 'state' not in row:
            continue
        values = []
        for key in ('ranks', 'assertions', 'elapsed_seconds'):
            if key in row:
                values.append(f"{key.replace('_', ' ')} {row[key]}")
        detail = ', ' + ', '.join(values) if values else ''
        path = row.get('stdout') or row.get('note') or row.get('path')
        lines.append(f"- {name} {row['state']}{detail}."
                     + (' ' + link('Fixture evidence', path) if path else ''))
    if part == 'C' and data.get('passed'):
        lines.append('- G1 retained the reference case for a census time stop with `maximum_seconds=1e-12`.')
    for gate in ('G1', 'G2', 'G3', 'G4', 'GQ'):
        row = data.get(gate)
        if not isinstance(row, dict):
            continue
        values = [f"{key.replace('_', ' ')} {row[key]}" for key in
                  ('accepted_collapses', 'work', 'accepted_splits', 'common_edit_count',
                   'seconds_until_stop', 'fixedstorage', 'decisions_changed') if key in row]
        if gate == 'G1':
            values.append('serial / 2 / 4 ranks ' + ' / '.join(str(row[key]['records'])
                          for key in ('serial', 'mpi2', 'mpi4') if key in row))
        if gate == 'G4' and row.get('new_cases'):
            values.append('1 / 2 / 4 ranks elapsed ' + ' / '.join(number(case['status']['seconds'])
                          for case in row['new_cases']) + ' s')
        detail = ' · '.join(values)
        check = row.get('comparison') or row.get('case')
        if isinstance(check, (str, Path)) and Path(check).is_file():
            detail += ' · ' + link('Comparison', check)
        lines.append(f"- {gate} {detail}" if detail else f'- {gate} recorded in the scoped receipt.')
    links = [link(label, path) for label, path in (('Scoped validation', scope), ('Fixture receipt', fixture_path),
                                                   ('Shared-engine safeguards', safeguards)) if path.is_file()]
    if links:
        lines.extend(['', ' · '.join(links), ''])
    if part == 'C':
        lines.extend(prefix_calls(data))
    return lines


def prefix_calls(scope):
    """Report the mean calls at each identified prefix timer snapshot."""
    rows = []
    for gate in ('G2', 'GQ'):
        path = scope.get(gate, {}).get('comparison')
        if not path:
            continue
        counts = defaultdict(list)
        for stream in optional(Path(path)).get('timer_rows', {}).values():
            for index, row in enumerate(stream.get('snapshots', []), 1):
                if 'key' not in row:
                    continue
                root, boundary, _ = row['key']
                old = row['communication']['reference']
                new = row['communication']['candidate']
                for phase in old.keys() | new.keys():
                    counts[index, root, boundary, phase].append((
                        old.get(phase, {}).get('calls', 0), new.get(phase, {}).get('calls', 0)))
        for key, samples in sorted(counts.items()):
            rows.append((gate, *key, *(number(sum(side) / len(side)) for side in zip(*samples))))
    return (['### Prefix communication calls', '',
             'The values are means over the rank snapshots in each group.', '',
             *table(('Gate', 'Snapshot', 'Root', 'Boundary', 'Phase', 'Reference', 'Candidate'), rows), '']
            if rows else [])


def ownership_table(before, after):
    """Put all old and new evaluation slots side by side, leaving missing slots visible."""
    def slots(rows):
        return {(row['event'], row['evaluation']): row for row in rows}
    old, new = slots(before), slots(after)
    values = []
    for key in sorted(old.keys() | new.keys()):
        a, b = old.get(key, {}), new.get(key, {})
        pair = lambda name: f"{a.get(name, '—')} / {b.get(name, '—')}"
        values.append([*key, pair('quality'), f"{number(a.get('predicted_imbalance'))} / {number(b.get('predicted_imbalance'))}",
                       number(a.get('predicted_gain_seconds')), number(a.get('predicted_cost_seconds')),
                       number(b.get('predicted_gain')), number(b.get('predicted_cost')),
                       *[number(b.get(name)) for name in ('demand_sum', 'global_cells', 'measured', 'attempts')],
                       pair('migrate')])
    return table(('Event', 'Evaluation', 'Quality old / new', 'Imbalance old / new', 'Old gain (s)', 'Old cost (s)',
                  'New gain', 'New cost', 'Demand', 'Cells', 'Measured', 'Attempts', 'Migrate old / new'), values)


def timing_lines(pairs, include_coverage=True):
    header = ('Run', 'Event', 'Phase', 'Exclusive phase (s)', 'Communication calls',
              'Exclusive communication (s)', 'Inclusive census (s)')
    native, phases, coverage = [], [], []
    for label, events in pairs:
        for event in events:
            number_of_event = event['event']
            native.append((label, number_of_event, number(event['native_seconds'])))
            for phase in PHASES:
                values = event['phase_means'][phase]
                phases.append((label, number_of_event, phase, *[number(values[key]) for key in
                               ('seconds_mean', 'communication_calls_mean', 'communication_seconds_mean',
                                'census_inclusive_seconds_mean')]))
            coverage.append((label, number_of_event, *[f"{100 * float(event['coverage_by_rank'][str(rank)]):.5f}%"
                             for rank in range(6)]))
    lines = [*table(('Run', 'Event', 'Maximum native time (s)'), native), '',
             'Each phase row is a mean over the six final remesh ledgers. The native clock has a wider scope.', '',
             *table(header, phases), '']
    if include_coverage:
        lines += table(('Run', 'Event', *[f'Rank {rank} coverage' for rank in range(6)]), coverage)
    return lines


def failures(part):
    path = G5[part] / 'failed_attempt.json'
    failure = optional(path)
    if not failure:
        return []
    timing = [(Path(case['case']).name, row['event'], row['rank'], row['coverage'], row['unattributed_seconds'])
              for case in failure.get('cases', []) for row in case['timing']]
    lines = ['', f'### Part {part} failed workflow', '', failure['error'], '', link('Terminal rank tails and receipt', path)]
    if timing:
        lines.extend(['', *table(('Run', 'Event', 'Rank', 'Coverage', 'Unattributed seconds'), timing)])
    return lines


def changed_lines(result):
    changes = result.get('changed_evaluations', [])
    if not changes:
        return []
    old = changes[0]['reference'] or {}
    new = changes[0]['candidate'] or {}
    event = (old or new)['event']
    index = (old or new)['evaluation']
    return [f"The first unexpected G5 evaluation difference is event {event}, evaluation {index}, "
            f"with migrate {old.get('migrate', 'missing')} → {new.get('migrate', 'missing')}.",
            link('All ownership differences', G5['M'] / 'ownership_comparison.json'), '']


def render(build, results, states, commits, notes, audit):
    m, c = (results[part] for part in ('M', 'C'))
    lines = [f'# Step 8.5u, build{build}', '']
    if any(state == 'OPEN for the user' for state in states.values()):
        detail = []
        if states['M'] == 'OPEN for the user':
            detail.append('Part M exceeded its report-only five-percent comparison.')
        if states['C'] == 'OPEN for the user':
            detail.append('Part C met its result gates and missed its purpose time. The user order retains the commit.')
        lines += ['**OPEN for the user.** ' + ' '.join(detail), '']
    lines += [f"Part M {states['M']}. Part C {states['C']}.", '']
    lines += changed_lines(m)
    if m.get('GQ_decisions_changed'):
        lines += ['The GQ evaluation sequence changed on the wide reference. This was outside the forecast.', '']
    for part, commit in commits.items():
        if commit:
            lines.append(f'- Part {part} checkpoint `{commit}`.')
    lines.extend(['', link('Pinned implementation', HERE / 'inputs/IMPLEMENTATION_8_5U.md'), ''])
    gq_reference = HERE / 'gq_reference_summary.json'
    if gq_reference.is_file():
        lines.extend(['The unchanged accepted N21 build supplied the GQ reference before the first edit. '
                      + link('Pre-edit GQ receipt', gq_reference), ''])
    source = HERE / 'source_check.md'
    if source.is_file():
        text = source.read_text()
        if text.startswith('# '):
            text = text.partition('\n')[2]
        lines.extend(['## Whole-item source check', '', text.strip(), '', link('Source check evidence', source), ''])
    lines.extend(gate_lines('M'))
    if m:
        lines.extend([f"- Part M followed case {m['case']} of §5.1.",
                      '- Both events passed the standing workflow gates.',
                      '- The six payloads matched ' + ('the adopted N21 W hashes.' if m['case'] == 4
                                                        else 'across the two runs on the same binary.'),
                      '', link('Part M G5 acceptance', G5['M'] / 'acceptance_summary.json'), ''])
        for index, run in enumerate(m['runs'], 1):
            lines.extend([f'### Ownership evaluations, M run {index}', '',
                          'Outcomes agree across ranks within each run. Rank zero supplies the displayed gain and cost. '
                          'The old values are seconds. The new values count cell measurements.', '',
                          *ownership_table(m['N21_reference']['decisions'], run['decisions']), ''])
        clocks = [(f'M run {index}', row['event'], number(row['reference_seconds']),
                   number(row['candidate_seconds']), number(row['limit_seconds']))
                  for index, run in enumerate(m['timing'], 1) for row in run['events']]
        lines.extend(['### Part M native times', '', *table(('Run', 'Event', 'N21 reference (s)', 'Native maximum (s)',
                                                           'Five-percent comparison (s)'), clocks), ''])
        if m['case'] == 5:
            lines.extend(['M run one supplies the operational timing reference for Part C. Both M clocks are reported.', ''])
    lines.extend(failures('M'))
    lines.extend(gate_lines('C'))
    if c:
        timing = c['purpose']
        rows = [(event['event'], number(event['reference_seconds']), number(event['candidate_seconds']),
                 'Pass' if event['within_limit'] else 'Fail') for event in timing['events']]
        rows.append(('Sum', number(timing['combined_limit_seconds']), number(timing['sum_seconds']),
                     'Pass' if Decimal(timing['sum_seconds']) <= Decimal(timing['combined_limit_seconds']) else 'Fail'))
        lines.extend(['- Both events passed the standing workflow gates and all six M payload hashes matched.', '',
                      '### Part C purpose rule', '',
                      'The ordered N21 minima set the entry reference. Under §5.1.7 the qualified Part M run '
                      'supplies the reference for Part C.',
                      '', *table(('Event', 'Limit (s)', 'Native maximum (s)', 'Result'), rows), '',
                      link('Part C G5 acceptance', G5['C'] / 'acceptance_summary.json'), '',
                      '### Part C phase clocks', '',
                      *timing_lines((('M reference', c['M_reference_events']), ('C', c['runs'][0]['events']))), ''])
        if c.get('new_timing_reference'):
            lines.extend(['The Part C times are the new timing reference.', ''])
    lines.extend(failures('C'))
    for path in notes:
        if path.is_file():
            lines.extend(['', path.read_text().strip(), '', link(path.name, path)])
    if audit and audit.is_file():
        lines.extend(['', link('Final actual-host audit', audit)])
    return '\n'.join(lines).rstrip() + '\n'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build', type=int)
    for part in ('m', 'c'):
        parser.add_argument(f'--{part}-status', choices=STATES)
        parser.add_argument(f'--{part}-commit')
    parser.add_argument('--notes', type=Path, action='append', default=[])
    parser.add_argument('--audit', type=Path)
    parser.add_argument('--timings-draft', type=Path)
    args = parser.parse_args()
    results = {part: optional(root / 'acceptance_summary.json') for part, root in G5.items()}
    states = {part: getattr(args, f'{part.lower()}_status') or results[part].get('status', 'NOT STARTED')
              for part in results}
    for part, data in results.items():
        if data:
            assert states[part] == data['status'], (part, states[part], data['status'])
    commits = {'M': args.m_commit, 'C': args.c_commit}
    build = args.build or (results['C'] or results['M']).get('build', 589)
    REPORTS.mkdir(parents=True, exist_ok=True)
    target = REPORTS / f'STEP_8_5U_{build}.md'
    notes = [HERE / 'r3_notes.md', *args.notes]
    output = render(build, results, states, commits, notes, args.audit)
    with target.open('x') as file:
        file.write(output)
    if args.timings_draft and results['C']:
        c = results['C']
        disposition = ('The Part C clocks are the adopted time reference.' if c['new_timing_reference'] else
                       'Part C is retained with the purpose timing OPEN. The qualified Part M clocks remain the time reference.')
        section = [f'## Step 8.5u, build{build}', '',
                   link('Step 8.5u report', target, HERE.parents[5] / 'Handbook'), '',
                   disposition, '',
                   *timing_lines((('M reference', c['M_reference_events']), ('C', c['runs'][0]['events'])),
                                 include_coverage=False), '']
        with args.timings_draft.open('x') as file:
            file.write('\n'.join(section))
    print(target)


if __name__ == '__main__':
    main()
