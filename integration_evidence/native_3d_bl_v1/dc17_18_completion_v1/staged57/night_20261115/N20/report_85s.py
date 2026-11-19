#!/usr/bin/env python3
"""Render the N20 result from sealed receipts and saved rank logs."""

import argparse
from collections import defaultdict
import csv
import json
import os
from pathlib import Path
import runpy
import sys
import tempfile

sys.dont_write_bytecode = True
N20 = Path(__file__).resolve().parent
NIGHT = N20.parent
REPO = NIGHT.parents[4]
PHASE = NIGHT / 'N4/section6_3_after_N11'
BRANCH = PHASE / 'round_8_5s_W'
WORKFLOW = BRANCH / 'G5'
T0 = PHASE / 'round_8_5r_B/G5/full574'
REPORTS = N20 / 'reports'
M28 = runpy.run_path(str(NIGHT / 'N18/drafts/reporting/report_m28.py'))
REQUEST = 'NATIVE3D_FINE_REQUEST'
MOVE = 'NATIVE3D_FINE_MOVE'
STUCK = M28['STUCK']
CENSUS = M28['CENSUS']
DUMP_TAGS = {REQUEST, MOVE, STUCK, 'NATIVE3D_M29_BEGIN', 'NATIVE3D_M29_CELL', 'NATIVE3D_M29_END'}
M28['STUDIED'].update(DUMP_TAGS)


def link(label, path, base=REPORTS):
    return f'[{label}]({os.path.relpath(path, base)})'


def optional_json(path):
    return json.loads(path.read_text()) if path and path.is_file() else None


def read_rank_events(case):
    """Use the archived M28 event grouping and line parser on plain or gzip rank stdout."""
    rank_events = {}
    for rank_dir in sorted((case / 'rank_output/1').glob('rank.*')):
        path = rank_dir / 'stdout'
        if not path.is_file():
            path = path.with_suffix('.gz')
        if path.is_file():
            rank = int(rank_dir.name.removeprefix('rank.'))
            rank_events[rank], _ = M28['read_rows'](path, rank)
    return rank_events


def by_event(rank_events):
    events = defaultdict(dict)
    for rank, rank_rows in sorted(rank_events.items()):
        for event, tags in enumerate(rank_rows, 1):
            events[event][rank] = tags
    return dict(events)


def marker_rows(ranks, tags):
    """Keep event-local rank ordering and the exact order within each rank log."""
    for rank_tags in ranks.values():
        rows = (row for tag in tags for row in rank_tags.get(tag, ()))
        yield from sorted(rows, key=lambda row: row.number)


def rejected_events(case, events):
    """Use the bounded controller's first rejection receipt, then the gathered native summary."""
    receipt = optional_json(case / 'first_rejected_event.json')
    if receipt and any(row.get('complete') == '0' for row in receipt.get('per_rank', {}).values()):
        return {int(receipt['event'])}
    return {event for event, ranks in events.items()
            if any(row.fields.get('complete') == '0' for row in marker_rows(ranks, ['NATIVE3D']))}


def failed_event_dump(events, rejected):
    """Emit only failure-event diagnostics with exactly the parsed marker and field text."""
    return ''.join(row.text.rstrip() + '\n' for event in sorted(rejected)
                   for row in marker_rows(events.get(event, {}), DUMP_TAGS))


def row_order(row):
    return int(row.fields['round']), int(row.fields['rank']), row.number


def feature_vertices(stuck):
    """The M28 singleton feature records recover the old vertex IDs."""
    vertices = set()
    for record in stuck.fields.get('f', '').split(','):
        kind, separator, body = record.partition(':')
        if kind == '1' and separator:
            vertices.add(body.split(':', 1)[0])
    return vertices


def terminal_shape_round(ranks):
    """A full all-rank final census can prove shape clearance, without naming the edit."""
    if set(ranks) != set(range(6)):
        return None
    finals = [next((row for row in reversed(tags.get('NATIVE3D_STAGE', ()))
                    if row.fields.get('stage') == 'exit'), None) for tags in ranks.values()]
    if all(row and all(row.fields.get(key) == '1' for key in ('census_complete', 'shape_complete'))
           and row.fields.get('q_bad') == row.fields.get('j_bad') == '0' and 'rounds' in row.fields
           for row in finals):
        return max(int(row.fields['rounds']) for row in finals)
    return None


def confirmed_repair_round(ranks, last, accepted, clear_round):
    """Use a single published repair between the stuck census and same-round clean exit."""
    if len(accepted) != 1 or clear_round is None:
        return None
    move = accepted[0]
    if int(move.fields['round']) != clear_round or move.fields['rank'] != last.fields['rank']:
        return None
    owner = move.fields['rank']
    prior_round = last.fields['round']
    committed = 0
    for rank, tags in ranks.items():
        census = [row.fields for row in tags.get('NATIVE3D_QUALITY', ()) if row.fields.get('round')]
        if len(census) < 2:
            return None
        prior, final = census[-2:]
        if prior['round'] != prior_round or final['round'] != str(clear_round) or final.get('stage') != 'exit':
            return None
        commits = final.get('commits')
        if commits not in ('0', '1'):
            return None
        if str(rank) == owner:
            removed = int(final.get('owned_q_removed', '0')) + int(final.get('owned_j_removed', '0'))
            if commits != '1' or removed <= 0:
                return None
        committed += int(commits)
    return clear_round if committed == 1 else None


def trajectories(ranks):
    """Associate old cell vertices with local fine trials and preserve the attribution boundary."""
    cells = defaultdict(list)
    for row in marker_rows(ranks, [STUCK]):
        cells[row.fields['cell']].append(row)
    moves = sorted(marker_rows(ranks, [MOVE]), key=row_order)
    clear_round = terminal_shape_round(ranks)
    result = []
    for cell, observations in sorted(cells.items(), key=lambda item: int(item[0])):
        observations.sort(key=row_order)
        first, last = observations[0], observations[-1]
        start = int(first.fields['round'])
        vertices = set().union(*(feature_vertices(row) for row in observations))
        accepted = [row for row in moves if row.fields.get('vertex') in vertices and
                    row.fields.get('accepted') == '1' and int(row.fields['round']) >= start]
        repair_round = confirmed_repair_round(ranks, last, accepted, clear_round)
        result.append({'cell': cell, 'first': first, 'last': last, 'accepted': accepted,
                       'interval': [(int(row.fields['round']), int(row.fields['round']) - start) for row in accepted],
                       'repair_round': repair_round, 'repair_delta': None if repair_round is None else repair_round - start,
                       'clear_round': clear_round, 'delta_to_clear': None if clear_round is None else clear_round - start,
                       'stuck_at_exit': any(row.fields.get('stage') == 'exit' for row in observations)})
    return result


def last_event_censuses(events):
    """Retain the last census of every rank of every event, using M28's ordering and overflow rules."""
    rows = []
    for event, ranks in events.items():
        for row in M28['gather_stuck']({rank: [tags] for rank, tags in ranks.items()}):
            rows.append(dict(row, event=event))
    return rows


def gate_lines(section, summary, safeguards):
    """Render available measurements without promoting partial receipts to a pass."""
    result = ['', f'## Part {section} scoped gates', '']
    gates = optional_json(summary)
    if gates:
        result.append(link('Scoped gate receipt', summary))
        result.append('')
        counts = gates.get('G1_records', [])
        if not counts and (g1 := gates.get('G1')):
            counts = [g1[cohort]['records'] for cohort in ('serial', 'mpi2', 'mpi4')]
        if counts:
            result.append(f"- G1 reference records at serial / 2 / 4 ranks: {' / '.join(map(str, counts))}.")
        if ranks := gates.get('wide_repeat_ranks'):
            result.append(f"- Exact wide-repeat fixture ranks: {' / '.join(map(str, ranks))}.")
        if g2 := gates.get('G2'):
            result.append(f"- G2 work {g2.get('work', 'unavailable')} and collapse counts "
                          f"{g2.get('accepted_collapses', 'unavailable')}.")
        if g3 := gates.get('G3'):
            if g3.get('identity_is_gate', True):
                edits = f"{g3.get('common_edit_count', 'unavailable')} common edits"
            else:
                edits = f"edit identity report, first difference {g3.get('first_mismatch_index')}"
            result.append(f"- G3 accepted splits {g3.get('accepted_splits', 'unavailable')}, {edits}.")
        if g4 := gates.get('G4'):
            cases = g4.get('new_cases', [])
            if cases:
                times = ' / '.join(f"{row['status']['seconds']:.3f}" for row in cases)
                result.append(f'- G4 wall times at 1 / 2 / 4 ranks: {times} s.')
        if gq := gates.get('GQ'):
            result.append(f"- GQ prefix stop {gq.get('seconds_until_stop')} s.")
        if any(gates.get(label, {}).get('fixedstorage') == 4120 for label in ('G2', 'GQ')):
            result.append('- Fixed timer storage 4,120 B.')
    else:
        result.append('No sealed scoped gate summary was available.')
    guards = optional_json(safeguards)
    if guards:
        measured = ', '.join(f"{name} {row['seconds']:.3f} s" for name, row in guards['safeguards'].items())
        result.extend(['', f"Standing geometry safeguards {'passed' if guards.get('passed') else 'failed'} ({measured}). "
                       + link('Safeguard receipts', safeguards)])
    return result


def full_lines(summary, failed):
    """List only workflow and purpose gates supported by executed receipts."""
    lines = ['', '## Part W full workflow', '']
    if summary:
        lines.extend(['| Run | Event | Native max (s) | Process-peak sum (B) | Minimum timer coverage | Proof faces |',
                      '|---|---:|---:|---:|---:|---:|'])
        for run, events in summary['events'].items():
            for event in events:
                lines.append(f"| {run} | {event['event']} | {event['native_seconds']:.3f} | "
                             f"{event['process_peak_sum_bytes']:,} | {event['minimum_timer_coverage']:.4%} | "
                             f"{event['proof_faces']:,} |")
        gate = summary['purpose']
        minima = ' / '.join(row['smaller_native_max_seconds'] for row in gate['events'])
        lines += ['', 'Both workflows passed their standing gates. The first event prefix matched T0. '
                  'The isolated event-two restart passed and all six payloads matched across repeats. '
                  + link('Restart receipt', summary['isolated_restart_receipt']),
                  '',
                  f"Gate 8 {'passed' if gate['passed'] else 'failed'}. Event minima were {minima} s. "
                  f"The sum was {gate['smaller_native_max_sum_seconds']} s against the "
                  f"{gate['combined_limit_seconds']} s ceiling. The individual limits were 2236.870 and 1416.499 s. "
                  + link('Gate 8 receipt', WORKFLOW / 'purpose_rule.json')]
    elif failed:
        lines.append(f"The workflow stopped during {failed['interrupted_stage']}. Gate 8 was unevaluated. "
                     + link('Failed attempt receipt', WORKFLOW / 'failed_attempt.json'))
    else:
        lines.append('No sealed Part W workflow result was available.')
    if summary and summary['passed']:
        lines += ['', 'The six repeat-identical files are the new reference. '
                  + link('Qualified payload hashes', WORKFLOW / 'qualified_reference_payloads.json')]
    return lines


def partial_workflows(cases, timelines):
    """Retain completed event receipts even when a later W gate failed."""
    lines = []
    for label, case in cases:
        status_path = case / 'status.json'
        status = optional_json(status_path)
        if not status:
            continue
        lines += ['', f"### {label}", '', f"Controller outcome: `{status['stop_reason']}`. "
                  + link('Terminal receipt', status_path), '',
                  '| Event | Native completion | Native max (s) | Connected checks | Timer checks |',
                  '|---|---|---:|---|---|']
        for event, ranks in timelines[label].items():
            summaries = {row.fields['rank']: row.fields for row in marker_rows(ranks, ['NATIVE3D'])
                         if 'complete' in row.fields and 'total' in row.fields}
            complete = ', '.join(f"{rank}={values['complete']}" for rank, values in sorted(summaries.items())) or '—'
            native = f"{max(float(row['total']) for row in summaries.values()):.3f}" if summaries else '—'
            gates = []
            for filename in (f'p7p8_event{event}_gate.json', f'f3b_event{event}_timer_gate.json'):
                path = case / filename
                result = optional_json(path)
                gates.append(link('Passed' if result['passed'] else 'Failed', path) if result else 'Unevaluated')
            lines.append(f"| {event} | {complete} | {native} | {' | '.join(gates)} |")
    return lines


def measurement_lines(records):
    """Summarize the existing CSV fields without treating S as a performance gate."""
    def seconds(row, key):
        return '—' if row.get(key) is None else f'{row[key]:.3f}'
    lines = ['', '## M28 measurements', '',
             'Loss attribution uses the reserved cell-ID overlap rule. The quality clocks are rank-local exclusive '
             'time. S sums join wait, communication and clearance across ranks.', '',
             '| Run | Event | Reservation losses, own / other | Quality evaluations / migrations | Scheduling sum (s) | S (s) |',
             '|---|---:|---:|---:|---:|---:|']
    for row in records:
        if row['rank'] == 'sum':
            own, other = (row.get(key, '—') for key in ('lost_own', 'lost_other'))
            lines.append(f"| {row['run']} | {row['event']} | {own} / {other} | "
                         f"{row['quality_ownership_evaluations']} / {row['quality_ownership_migrations']} | "
                         f"{seconds(row, 'quality_scheduling_seconds')} | {seconds(row, 'quality_purpose_seconds')} |")
    return lines


def fine_lines(run, events):
    """Include every fine diagnostic and pair cell observations with supporting stage evidence."""
    lines = ['', f'## Fine placement evidence, {run}', '']
    for event, ranks in events.items():
        fine = [row.text.rstrip() for row in marker_rows(ranks, [REQUEST, MOVE])]
        lines.extend([f'### Event {event}', ''])
        if fine:
            lines += ['```text', *fine, '```', '']
        else:
            lines += ['No fine placement request or trial was printed.', '']
        paths = trajectories(ranks)
        if not paths:
            lines += ['No stuck cell was printed.', '']
            continue
        lines += ['| Cell | First rank/stage/round | Last observed round | Accepted fine trial rounds (delta from first) | '
                  'Shape evidence after the first observation |',
                  '|---:|---|---:|---|---|']
        for row in paths:
            first = row['first'].fields
            attempts = ', '.join(f'{number} ({delta:+d})' for number, delta in row['interval']) or '—'
            if row['stuck_at_exit']:
                latest = 'Still stuck at exit'
            elif row['repair_round'] is not None:
                latest = f"Repair published at round {row['repair_round']} ({row['repair_delta']:+d})"
            elif row['clear_round'] is None:
                latest = 'No all-rank clean exit recorded'
            else:
                latest = f"All ranks shape-clean by round {row['clear_round']} ({row['delta_to_clear']:+d})"
            lines.append(f"| {row['cell']} | {first['rank']}/{first['stage']}/{first['round']} | "
                         f"{row['last'].fields['round']} | {attempts} | {latest} |")
        lines += ['', 'A trial is associated by an exact old vertex ID from M28. A published repair round '
                  'requires a single commit in the matching rank census interval, removal of owned shape debt, '
                  'and an all-rank clean exit in the same round. The clean exit bounds other associations.', '']
    return lines


def fixture_lines(paths):
    if not paths:
        candidates = [N20 / f'fixtures/{name}_outer.log' for name in ('optimizer', 'selection')]
        paths = [path for path in candidates if path.is_file()]
    lines = []
    for path in paths:
        if path.suffix == '.md':
            lines.extend([path.read_text().rstrip(), '', link(path.name, path), ''])
            continue
        rows = path.read_text().splitlines()
        tags = ('NATIVE3D_FINE_FIXTURE', 'NATIVE3D_FINE_SELECTION', 'N20_F')
        tagged = [line for line in rows if any(tag in line for tag in tags)]
        archive = path.with_suffix(path.suffix + '.gz')
        lines.extend([link(path.name, archive if archive.is_file() else path), '',
                      '```text', *(tagged or rows), '```', ''])
    return lines or ['No fixture measurement receipt was available.']


def collect_cases(build, overrides):
    """Discover actual workflow logs; an explicit case option is used for preserved attempts."""
    cases = [(f'run{i}', WORKFLOW / f'full{build}_run{i}') for i in (1, 2)]
    if overrides:
        cases = []
        for spec in overrides:
            label, separator, path = spec.partition('=')
            if not (separator and label and path):
                raise ValueError('--case requires LABEL=PATH')
            cases.append((label, Path(path).resolve()))
    return [(label, case) for label, case in cases if (case / 'rank_output/1').is_dir()]


def collect_measurements(cases):
    """Produce the inherited M28 metrics and N20-only diagnostics from each recorded event."""
    records, audits, timelines, dumps = [], [], {}, []
    for label, case in [('T0', T0), *cases]:
        values, audit = M28['read_case'](label, case)
        events = by_event(read_rank_events(case))
        audit['last_censuses'] = last_event_censuses(events)
        audit.pop('rank_events')  # Rank-log hashes and census locations identify the unchanged input.
        records.extend(values)
        audits.append(audit)
        if label != 'T0':
            timelines[label] = events
            rejected = rejected_events(case, events)
            if rejected:
                dumps.append(failed_event_dump(events, rejected))
    return records, audits, timelines, dumps


def write_reports(args):
    results = optional_json(WORKFLOW / 'acceptance_summary.json') if args.w_status != 'NOT STARTED' else None
    failed = optional_json(WORKFLOW / 'failed_attempt.json') if args.w_status != 'NOT STARTED' else None
    if (args.w_status == 'PASSED') != bool(results and results['passed']):
        raise ValueError('Part W passed status and sealed acceptance disagree')
    if args.w_status == 'FAILED its purpose rule' and not (
            results and results['standing_gates_passed'] and results['repeat_payloads_exact']
            and not results['purpose']['passed']):
        raise ValueError('Purpose failure requires standing, repeat and purpose receipts')
    root_case = collect_cases(args.build, args.case)
    records, audit, timelines, dumps = collect_measurements(root_case)
    diagnosis = optional_json(args.diagnosis)
    REPORTS.mkdir(exist_ok=True)
    stem = REPORTS / f'STEP_8_5S_{args.build}'
    report, csv_path = stem.with_suffix('.md'), stem.with_suffix('.csv')
    stuck, sources = stem.with_suffix('.stuck.md'), stem.with_suffix('.provenance.json')
    outputs = [report, csv_path, stuck, sources]
    if dumps:
        outputs.append(REPORTS / 'W_dump.txt')
    if any(path.exists() for path in outputs):
        raise FileExistsError('Report outputs already exist. Preserve the earlier report before regeneration.')
    lines = [f'# Step 8.5s, build{args.build}', '',
             f'Part A {args.a_status}. Part W {args.w_status}. '
             + link('Pinned specification', N20 / 'inputs/IMPLEMENTATION_8_5S.md')
             + ' · ' + link('Whole-item source check', N20 / 'source_check.md')]
    for label, value in (('Part A', args.a_commit), ('Part W', args.w_commit),
                         ('Revert', args.revert), ('Active checkpoint', args.active_commit)):
        if value:
            lines.append(f'- {label}: `{value}`.')
    if args.active_build:
        lines.append(f'- Active build: {args.active_build}.')
    lines += gate_lines('A', args.a_summary, args.a_safeguards)
    fine_receipt = N20 / 'A_fine_rows.json'
    if (fine_rows := optional_json(fine_receipt)) is not None:
        lines += ['', f"Part A emitted {fine_rows['fine_rows']} fine rows across "
                   f"{fine_rows['scoped_logs']} scoped logs. " + link('Print rows', fine_receipt)]
    lines += ['', '## Fixtures', '', *fixture_lines(args.fixtures)]
    correction = N20 / 'R3_G3_prelaunch/correction.md'
    if correction.is_file():
        lines += ['', '## Fixture adapter recovery', '', correction.read_text().strip(),
                  link('First setup receipt', correction.parent / 'G3_wait.json')]
    if args.w_status != 'NOT STARTED':
        lines += gate_lines('W', args.w_summary, args.w_safeguards)
        lines += full_lines(results, failed)
        if not results:
            lines += partial_workflows(root_case, timelines)
    if diagnosis:
        lines += ['', '## Disposition', '']
        for label, key in (('First result difference', 'first_difference'), ('Cause', 'cause'),
                           ('Unevaluated gates', 'unevaluated')):
            if diagnosis.get(key):
                lines.append(f'{label}: {diagnosis[key]}')
        lines.append(link('Diagnosis receipt', args.diagnosis))
    if args.a_full:
        full = optional_json(args.a_full)
        lines += ['', '## Part A full identity fallback', '', link('Fallback gate receipt', args.a_full)]
        if full and full.get('passed'):
            lines += ['', 'The two-event workflow and six payload identities passed.']
    for label, events in timelines.items():
        lines += fine_lines(label, events)
    lines += measurement_lines(records)
    lines += ['', link('Per-rank CSV', csv_path) + ' · ' + link('Rank-log SHA256 and census locations', sources)]
    if dumps:
        lines.extend(['', link('Rejected-event M29, M28 and fine placement dump', outputs[-1])])
    table = M28['stuck_table'](audit)
    lines += ['', table.rstrip()]

    with M28['LEGACY_CSV'].open(newline='') as stream:
        fields = next(csv.reader(stream)) + list(M28['M28_COLUMNS'])
    with csv_path.open('x', newline='') as stream:
        writer = csv.DictWriter(stream, fields, lineterminator='\n')
        writer.writeheader()
        writer.writerows(records)
    with stuck.open('x') as stream:
        stream.write(table)
    with sources.open('x') as stream:
        json.dump(audit, stream, indent=2)
        stream.write('\n')
    if dumps:
        with outputs[-1].open('x') as stream:
            stream.write(''.join(dumps))
    with report.open('x') as stream:
        stream.write('\n'.join(lines) + '\n')
    if args.docs_draft:
        draft_docs(args, report, results, records)
    return outputs


def draft_docs(args, report, receipt, measurements):
    """Write current-state snippets for review before touching the standing documents."""
    state = f'N20 Part A {args.a_status}. Part W {args.w_status}.'
    if args.active_build:
        state += f' Build{args.active_build} is active.'
    data = {
        label: state + ' ' + link('Step 8.5s report', report, base)
        for label, base in (('handoff_current', REPO), ('proceed_current', REPO), ('progress_current', NIGHT))
    }
    if receipt and receipt['passed']:
        purpose = receipt['purpose']
        lines = [f'## Step 8.5s — build{args.build}', '',
                 f"The two wide-round workflows passed and produced the same six payload hashes. "
                 f"The native event minima totaled {purpose['smaller_native_max_sum_seconds']} s "
                 f"against the {purpose['combined_limit_seconds']} s ceiling. Both individual limits passed. "
                 + link('Step 8.5s report', report, REPO / 'Handbook'), '',
                 '| Run | Event | Native max (s) | Min. timer coverage | Quality S (rank-summed s) | '
                 'Maximum unattributed (s) |',
                 '|---|---:|---:|---:|---:|---:|']
        sums = {(row['run'], row['event']): row.get('quality_purpose_seconds') for row in measurements
                if row['rank'] == 'sum'}
        for run, events in receipt['events'].items():
            for event in events:
                cost = sums.get((run, event['event']))
                seconds = '—' if cost is None else f'{cost:.3f}'
                lines.append(f"| {run} | {event['event']} | {event['native_seconds']:.3f} | "
                             f"{event['minimum_timer_coverage']:.4%} | {seconds} | "
                             f"{event['unattributed_seconds_max']:.3f} |")
        lines += ['', '| Run / event | Phase | Rank-exclusive seconds, min–max | Calls, six-rank sum |',
                  '|---|---|---:|---:|']
        for run in ('run1', 'run2'):
            for event in receipt['events'][run]:
                for phase, clock in event['phase_exclusive_clocks'].items():
                    lines.append(f"| {run} / {event['event']} | {phase} | "
                                 f"{clock['seconds_min']:.3f}–{clock['seconds_max']:.3f} | "
                                 f"{clock['timer_calls_sum']:,} |")
        lines.extend(['', 'Native event clocks and rank-local quality intervals describe different scopes. '
                      'The two-run minima account for host variation under Gate 8.'])
        data['timings_section'] = '\n'.join(lines)
    with args.docs_draft.open('x') as stream:
        json.dump(data, stream, indent=2)
        stream.write('\n')


def self_check():
    """Prove event attribution with a two-event fixture and archived N19 rejection evidence."""
    with tempfile.TemporaryDirectory() as directory:
        case = Path(directory)
        for rank in range(6):
            log = case / f'rank_output/1/rank.{rank}/stdout'
            log.parent.mkdir(parents=True)
            rows = [f'NATIVE3D_PARTITION rank={rank} parts=6']
            if rank == 0:
                rows += [
                    'NATIVE3D_QUALITY rank=0 stage=window census=1 complete=1 round=5 stuck_shape=1',
                    'NATIVE3D_M28_STUCK rank=0 stage=window round=5 cell=42 f=1:11:0:0:3:0:0:0,1:12:0:0:3:0:0:0',
                    'Thu Nov 19 12:30:00 2026<stdout>:NATIVE3D_FINE_REQUEST rank=0 round=7 stuck=1 reopened=2',
                    'NATIVE3D_FINE_MOVE rank=0 round=7 vertex=11 cells=10 candidate=1 accepted=1 moves=1 debt_reduction=1',
                    'NATIVE3D_QUALITY rank=0 stage=exit census=2 complete=1 round=8 stuck_shape=0',
                ]
            rows += [f'NATIVE3D_STAGE rank={rank} stage=exit rounds=8 census_complete=1 shape_complete=1 q_bad=0 j_bad=0',
                     f'NATIVE3D rank={rank} complete=1',
                     f'NATIVE3D_PARTITION rank={rank} parts=6']
            if rank == 1:
                rows += ['NATIVE3D_QUALITY rank=1 stage=exit census=1 complete=1 round=4 stuck_shape=1',
                         'NATIVE3D_M28_STUCK rank=1 stage=exit round=4 cell=51',
                         'NATIVE3D_M29_BEGIN rank=1 round=4 stuck=1',
                         'NATIVE3D_M29_END rank=1 level0=1 level1=2 level2=3']
            rows += [f'NATIVE3D rank={rank} complete=0']
            log.write_text('\n'.join(rows) + '\n')
        events = by_event(read_rank_events(case))
        assert rejected_events(case, events) == {2}
        dump = failed_event_dump(events, {2})
        assert 'cell=51' in dump and 'cell=42' not in dump and REQUEST not in dump
        cell, = trajectories(events[1])
        assert cell['cell'] == '42' and cell['interval'] == [(7, 2)] and cell['delta_to_clear'] == 3
        assert not cell['stuck_at_exit'] and trajectories(events[2])[0]['stuck_at_exit']
        assert [(row['event'], row['rank'], row['round']) for row in last_event_censuses(events)] == [
            (1, 0, 8), (2, 1, 4)]
        report = '\n'.join(fine_lines('test', events))
        assert '7 (+2)' in report and 'All ranks shape-clean by round 8 (+3)' in report
        assert report.count(REQUEST) == report.count(MOVE) == 1

    old = NIGHT / 'N19/PartB/full582_run1'
    events = by_event(read_rank_events(old))
    assert rejected_events(old, events) == {1}
    dump = failed_event_dump(events, {1})
    assert sum(line.startswith('NATIVE3D_M29_CELL ') for line in dump.splitlines()) == 521
    assert 'stage=exit' in dump and 'cell=152845591' in dump
    assert all(line.startswith(('NATIVE3D_M29_', STUCK + ' ')) for line in dump.splitlines())
    seen, = trajectories(events[1])
    assert seen['cell'] == '152845591' and seen['stuck_at_exit'] and seen['interval'] == []
    values, _, _, _ = collect_measurements([('N19 archived diagnostic', old)])
    assert len([row for row in values if row['run'] == 'T0']) == 14
    print('Passed synthetic attribution and rejected-event isolation, archived N19 and prior T0 checks.')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    actions = parser.add_subparsers(dest='action', required=True)
    actions.add_parser('self-check')
    report = actions.add_parser('write')
    report.add_argument('--build', type=int, required=True)
    report.add_argument('--a-status', choices=('PASSED', 'FAILED', 'HELD', 'NOT STARTED'), required=True)
    report.add_argument('--w-status', choices=('PASSED', 'FAILED', 'FAILED its purpose rule', 'NOT STARTED'),
                        required=True)
    report.add_argument('--a-summary', type=Path, default=PHASE / 'N20A/validation/acceptance_summary.json')
    report.add_argument('--a-safeguards', type=Path,
                        default=PHASE / 'round_8_5s_A/safeguards/acceptance_summary.json')
    report.add_argument('--w-summary', type=Path, default=PHASE / 'N20W/validation/acceptance_summary.json')
    report.add_argument('--w-safeguards', type=Path, default=BRANCH / 'safeguards/acceptance_summary.json')
    report.add_argument('--fixtures', type=Path, action='append')
    report.add_argument('--case', action='append', metavar='LABEL=PATH')
    for option in ('diagnosis', 'docs-draft', 'a-full'):
        report.add_argument(f'--{option}', type=Path)
    for option in ('a-commit', 'w-commit', 'revert', 'active-commit'):
        report.add_argument(f'--{option}')
    report.add_argument('--active-build', type=int)
    args = parser.parse_args()
    if args.action == 'self-check':
        self_check()
        return
    for output in write_reports(args):
        print(output)


if __name__ == '__main__':
    main()
