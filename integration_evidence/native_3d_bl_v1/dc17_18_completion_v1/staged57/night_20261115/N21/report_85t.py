#!/usr/bin/env python3
"""Report N21 from the sealed gate receipts and preserved rank logs."""

import argparse
from collections import defaultdict
import csv
import gzip
import json
from math import fsum
from pathlib import Path
import re
import runpy
import sys
from tempfile import TemporaryDirectory

sys.dont_write_bytecode = True
N21 = Path(__file__).resolve().parent
NIGHT = N21.parent
PHASE = NIGHT / 'N4/section6_3_after_N11'
P = PHASE / 'round_8_5t_P'
W = PHASE / 'round_8_5t_W'
REPORTS = N21 / 'reports'
OLD = runpy.run_path(str(NIGHT / 'N20/report_85s.py'))
M28 = OLD['M28']
T0 = OLD['T0']
COLLECT = runpy.run_path(str(PHASE / 'phase_C/collect_g5_summary.py'))['collect_events']
read = OLD['optional_json']
TRACE = re.compile(r'\bNATIVE3D_(?:PARTITION|FINE_\w+|M28_\w+|M29_\w+)\b')
PROOF = ('NATIVE3D_FINAL_PROOF', 'NATIVE3D_FINAL_CERT')
PROOF_MARKER = re.compile(r'\bNATIVE3D_FINAL_(?:PROOF|CERT)\b')
STATUSES = ('PASSED', 'FAILED', 'FAILED its purpose rule', 'NOT STARTED')
RUN_ORDER = ('T0', 'T1', 'run1', 'run2')
TIMING_PHASES = ('preparation', 'construction', 'quality', 'final_proof')
ATLAS = ('full_calls', 'full_work', 'full_seconds', 'traversal_work', 'traversal_seconds',
         'atlas_work', 'atlas_bytes', 'direct_calls', 'direct_work', 'direct_seconds',
         'upper_calls', 'upper_work', 'upper_seconds', 'classification_calls', 'classification_work',
         'magnitude_calls', 'magnitude_work', 'limit_calls', 'limit_work', 'support_calls', 'support_facets',
         'support_seconds', 'scope_build_work', 'scope_build_seconds', 'facet_build_work',
         'facet_build_seconds', 'piece_selection_work', 'piece_selection_seconds',
         'selected_pieces', 'support_pieces', 'memo_maintenance_work')
TRANSPORT = ('collective_calls', 'collective_seconds', 'election_calls', 'election_seconds')


def link(label, path):
    return OLD['link'](label, path, REPORTS)


def references(paths):
    return ' · '.join(link(label, path) for label, path in paths if path.is_file())


def rank_logs(case, channel='stdout'):
    for directory in sorted((case / 'rank_output/1').glob('rank.*')):
        log = directory / channel
        if not log.is_file():
            log = log.with_suffix('.gz')
        if log.is_file():
            yield int(directory.name.split('.')[1]), log


def markers(log, wanted):
    opener = gzip.open if log.suffix == '.gz' else open
    with opener(log, 'rt', errors='replace') as stream:
        for raw in stream:
            if not raw.endswith('\n') or not wanted.search(raw):
                continue
            line = M28['CANONICAL']['strip_mpi_prefixes'](raw).rstrip()
            marks = list(M28['MARKER'].finditer(line))
            for i, mark in enumerate(marks):
                if wanted.fullmatch(mark[0]):
                    end = marks[i + 1].start() if i + 1 < len(marks) else len(line)
                    text = line[mark.start():end].rstrip()
                    yield mark[0], text, dict(M28['FIELD'].findall(text))


def trace_rows(case):
    """Read stdout diagnostics and group stderr certificates with their rank's proof."""
    events = defaultdict(list)
    for rank, log in rank_logs(case):
        event = 0
        for tag, text, fields in markers(log, TRACE):
            if tag == 'NATIVE3D_PARTITION':
                event += fields.get('rank') == str(rank)
            elif event:
                events[event].append((rank, tag, text))
    for rank, log in rank_logs(case, 'stderr'):
        event = 0
        # The workflow ends at the first rejection. A missing proof leaves no next group.
        for tag, text, fields in markers(log, PROOF_MARKER):
            if tag == PROOF[0]:
                if fields.get('rank') != str(rank):
                    raise ValueError(f'{log}: proof row has the wrong rank')
                event += 1
            if not event or (tag == PROOF[1] and rank):
                raise ValueError(f'{log}: certificate row lacks its rank-0 proof')
            events[event].append((rank, tag, text))
    return events


def timer_ledgers(case, event):
    receipt = read(case / f'f3b_event{event}_timer_gate.json')
    if not receipt:
        return {}
    return {int(row['rank']): row['ledger'] for row in receipt['ranks']}


def proof_seconds(ledgers):
    seconds = {}
    for rank, ledger in ledgers.items():
        rows = [float(row['exclusive_seconds']) for row in ledger['rows']
                if row['phase'] == 'final_proof' and row['operation'] == 'certification']
        if rows:
            seconds[rank] = fsum(rows)
    return seconds


def compare_proof(p_case, purpose):
    lines = ['', '### Final proof certification', '',
             'The exclusive clocks are from each rank’s final remesh timer ledger.', '',
             '| Event | Rank | T0 (s) | T1 (s) |', '|---:|---:|---:|---:|']
    maxima = []
    for event in (1, 2):
        before = proof_seconds(timer_ledgers(T0, event))
        after = proof_seconds(timer_ledgers(p_case, event))
        for rank in sorted(before | after):
            lines.append(f'| {event} | {rank} | {decimal(before.get(rank))} | {decimal(after.get(rank))} |')
        if len(after) == 6:
            maxima.append((event, max(after.values())))
    if purpose:
        for row in purpose['events']:
            decision = 'passed' if row['purpose_passed'] else 'failed'
            lines.append(f"\nEvent {row['event']} rank maximum was "
                         f"{row['maximum_certification_exclusive_seconds']} s against "
                         f"{row['certification_limit_seconds']} s ({decision}). "
                         f"Owned roots totaled {row['own_roots_sum']:,}.")
    elif len(maxima) < 2:
        lines.append('\nThe Part P purpose rule lacks a complete two-event timer receipt.')
    else:
        lines.append('\nThe Part P purpose decision receipt was unavailable.')
    return lines


def decimal(value):
    return '—' if value is None else f'{value:.6f}'


def proof_lines(label, traces):
    lines = ['', f'### Final proofs and stored certificates, {label}', '']
    for event in (1, 2):
        lines.extend([f'Event {event}', ''])
        rows = [text for _, tag, text in traces.get(event, ()) if tag in PROOF]
        if rows:
            lines.extend(['```text', *rows, '```', ''])
        else:
            lines.extend(['No complete final proof or certificate line was observed.', ''])
    return lines


def full_events(case):
    status = read(case / 'status.json')
    if status and status.get('full_workflow_accepted'):
        return COLLECT(case)
    return []


def workflow_table(events):
    lines = ['', '## Workflow measurements', '',
             '| Run | Event | Native max (s) | Process-peak sum (B) | Minimum timer coverage | Proof faces |',
             '|---|---:|---:|---:|---:|---:|']
    for label in RUN_ORDER:
        for event in events.get(label, ()):
            lines.append(f"| {label} | {event['event']} | {event['native_seconds']:.9f} | "
                         f"{event['process_peak_sum_bytes']:,} | {event['minimum_timer_coverage']:.4%} | "
                         f"{event['proof_faces']:,} |")
    return lines


def rule6_values(events, result, ledgers, event, rank):
    stage = next((row.fields for row in reversed(events.get(event, {}).get(rank, {}).get('NATIVE3D_STAGE', ()))
                  if row.fields.get('stage') == 'exit'), {})
    # The terminal receipt carries each gathered rank's detail and summary.
    native = next((row for row in result.get('native_summaries', ()) if int(row['rank']) == rank), {})
    detail = next((row for row in result.get('refine_details', ()) if int(row['rank']) == rank), {})
    ledger = ledgers.get(rank, {})
    timers = {row['operation']: row for row in ledger.get('rows', ()) if row['phase'] == 'final_proof'}
    values = {f'stage.exit.{key}': stage.get(key) for key in ATLAS + TRANSPORT}
    for key in ('atlas_arithmetic_work', 'atlas_peak_bytes', 'native_bound', 'tracked_calls'):
        values[f'refine.{key}'] = detail.get(key)
    # This printed native_bound is the remesher's maximum_native_bytes.
    for key in ('native_bound', 'tracked_calls'):
        values[f'native.{key}'] = native.get(key)
    for operation in ('certification', 'communication'):
        for key in ('calls', 'exclusive_seconds'):
            values[f'final_proof.{operation}.{key}'] = timers.get(operation, {}).get(key)
    values['timer.fixedstorage'] = ledger.get('summary', {}).get('fixedstorage')
    return values


def write_rule6(path, cases, timelines):
    labels = ('T0', 'T1')
    workflows = {label: {int(row['event']): row for row in
                         (read(cases[label] / 'full_workflow_receipt.json') or {}).get('events', ())}
                 for label in labels}
    with path.open('x', newline='') as stream:
        writer = csv.writer(stream, lineterminator='\n')
        writer.writerow(('event', 'rank', 'row', 'T0', 'T1'))
        for event in (1, 2):
            ledgers = {label: timer_ledgers(cases[label], event) for label in labels}
            for rank in range(6):
                first, second = (rule6_values(timelines.get(label, {}), workflows[label].get(event, {}),
                                              ledgers[label], event, rank) for label in labels)
                for key in first:
                    if first[key] is not None or second[key] is not None:
                        writer.writerow((event, rank, key, first[key], second[key]))


def phase_table(cases):
    lines = ['', '## Mean rank-exclusive seconds by phase', '',
             'Each mean uses the six final remesh ledgers. The whole-event wall clock has a wider scope.', '',
             '| Run | Event | Preparation | Construction | Quality | Final proof |',
             '|---|---:|---:|---:|---:|---:|']
    for label in ('T1', 'run1'):
        if label not in cases:
            continue
        for event in (1, 2):
            ledgers = timer_ledgers(cases[label], event)
            if len(ledgers) != 6:
                continue
            means = [fsum(float(row['exclusive_seconds']) for ledger in ledgers.values()
                          for row in ledger['rows'] if row['phase'] == phase) / 6 for phase in TIMING_PHASES]
            lines.append(f"| {label} | {event} | {' | '.join(map(decimal, means))} |")
    return lines


def w_purpose(result, receipt):
    gate = (result or {}).get('purpose') or read(receipt)
    if not gate:
        return ['', 'Part W time rule unevaluated.']
    lines = ['', '### Part W time rule', '',
             f"Factor {gate['factor']} · {'PASSED' if gate['passed'] else 'FAILED'}. "
             + link('Time receipt', receipt), '',
             '| Event | Selected minimum (s) | T1 ceiling (s) | Result |',
             '|---:|---:|---:|---|']
    for row in gate['events']:
        limit = row.get('T1_native_limit_seconds') or row.get('T1_native_max_seconds')
        lines.append(f"| {row['event']} | {row['smaller_native_max_seconds']} | {limit} | "
                     f"{'Passed' if row['passed'] else 'Failed'} |")
    lines.append(f"| Combined | {gate['smaller_native_max_sum_seconds']} | "
                 f"{gate['combined_limit_seconds']} | {'Passed' if gate['combined_passed'] else 'Failed'} |")
    return lines


def launch_context(label, workspace, result):
    paths = [(f'{label} acceptance', workspace / 'acceptance_summary.json'),
             (f'{label} failure receipt', workspace / 'failed_attempt.json'),
             (f'{label} proof rows', workspace / 'proof_rows.json'),
             (f'{label} purpose receipt', workspace / 'purpose_rule.json'),
             (f'{label} counter identity', workspace / 'identity.json')]
    if result and result.get('isolated_restart_receipt'):
        paths.append((f'{label} isolated restart', Path(result['isolated_restart_receipt'])))
    if label == 'Part W':
        paths.append(('Adopted payload hashes', workspace / 'qualified_reference_payloads.json'))
    return references(paths)


def disposition(path):
    data = read(path)
    if not data:
        return []
    labels = ('first_difference', 'cause', 'unevaluated', 'candidate', 'revert')
    lines = ['', '## Disposition', '']
    for label in labels:
        if value := data.get(label):
            lines.append(f"- {label.replace('_', ' ').capitalize()} {value}")
    lines.extend(['', link('Disposition receipt', path)])
    return lines


def notes(title, paths):
    entries = []
    for path in paths:
        entries.extend(['', path.read_text().strip(), '', link(path.name, path)])
    return ['', f'## {title}', *entries] if entries else []


def source_readings():
    source = N21 / 'source_check.md'
    if source.is_file():
        _, _, section = source.read_text().partition('## Contradictions and readings used\n')
        if section:
            return ['', '## Readings used (R4)', '', section.split('\n## ', 1)[0].strip()]
    return []


def fixture_lines(receipt, additional):
    if not receipt.is_file() and not additional:
        return []
    paths = [('Fixture receipt', receipt), *((path.name, path) for path in additional)]
    lines = ['', '## Part P fixtures', '', references(paths), '']
    for name, row in (read(receipt) or {}).get('fixtures', {}).items():
        if row['state'] == 'R3_NOTE':
            note = Path(row['note'])
            lines.append(f"- {name} recorded an R3 note. " + references((('Note', note),)))
            if note.is_file():
                lines.extend(['', note.read_text().strip(), ''])
            continue
        log = Path(row['log'])
        archive = log.with_suffix(log.suffix + '.gz')
        log = archive if archive.is_file() else log
        checks = ' / '.join(map(str, row.get('checks_per_rank', ())))
        lines.append(f"- {name} {row['state']} at {row['ranks']} rank(s) in {row['seconds']:.3f} s. "
                     f"Checks by rank {checks}. " + references((('Log', log),)))
    return lines


def m28_outputs(stem, records, audit):
    csv_path, provenance = stem.with_suffix('.csv'), stem.with_suffix('.provenance.json')
    with M28['LEGACY_CSV'].open(newline='') as stream:
        fields = next(csv.reader(stream)) + list(M28['M28_COLUMNS'])
    with csv_path.open('x', newline='') as stream:
        writer = csv.DictWriter(stream, fields, lineterminator='\n')
        writer.writeheader()
        writer.writerows(records)
    with provenance.open('x') as stream:
        json.dump(audit, stream, indent=2)
        stream.write('\n')
    return csv_path, provenance


def report(args):
    p_g5, w_g5 = args.p_case.parent, args.run1.parent
    p_result, w_result = (read(path / 'acceptance_summary.json') for path in (p_g5, w_g5))
    p_purpose, w_time = (read(path / 'purpose_rule.json') for path in (p_g5, w_g5))
    for label, status, result in (('P', args.p_status, p_result), ('W', args.w_status, w_result)):
        if (status == 'PASSED') != bool(result and result['passed']):
            raise ValueError(f'Part {label} status and sealed acceptance disagree')
    if args.w_status != 'NOT STARTED' and args.p_status != 'PASSED':
        raise ValueError('Part W depends on an accepted Part P')
    if args.p_status == 'FAILED its purpose rule' and not (
            p_purpose and p_purpose['standing_gates_passed'] and not p_purpose['passed']):
        raise ValueError('Part P purpose failure requires the shared path, call counts and failed time receipt')
    if args.w_status == 'FAILED its purpose rule' and not (
            w_result and w_result['standing_gates_passed'] and w_result['repeat_payloads_exact']
            and w_time and not w_time['passed']):
        raise ValueError('Part W purpose failure requires standing, repeat and failed time receipts')
    cases = {'T0': T0}
    if any(rank_logs(args.p_case)):
        cases['T1'] = args.p_case
    if args.w_status != 'NOT STARTED':
        for label in ('run1', 'run2'):
            case = getattr(args, label)
            if any(rank_logs(case)):
                cases[label] = case

    traces = {label: trace_rows(case) for label, case in cases.items() if label != 'T0'}
    records, audit, timelines, _ = OLD['collect_measurements'](list(cases.items())[1:])
    if 'T1' in timelines:
        timelines['T0'] = OLD['by_event'](OLD['read_rank_events'](T0))
    events = {label: full_events(case) for label, case in cases.items()}
    native_baseline = next((result['events']['T1'] for result in (w_result, p_result)
                            if result and 'T1' in result.get('events', {})), events.get('T1', []))
    if native_baseline:
        events['T1'] = native_baseline

    build = args.w_build if args.w_status != 'NOT STARTED' else args.p_build
    REPORTS.mkdir(exist_ok=True, parents=True)
    stem = REPORTS / f'STEP_8_5T_{build}'
    main = stem.with_suffix('.md')
    rule6 = stem.with_suffix('.rule6.csv')
    outputs = [main, stem.with_suffix('.csv'), stem.with_suffix('.provenance.json')]
    if 'T1' in cases:
        outputs.append(rule6)

    dump = []
    rejected_any = False
    for label, trace in traces.items():
        if label in ('run1', 'run2'):
            rejected = OLD['rejected_events'](cases[label], timelines[label])
            rejected_any |= bool(rejected)
            for event in sorted(rejected):
                dump.extend(text for _, _, text in trace.get(event, ()))
    dump_path = REPORTS / 'W_dump.txt'
    if rejected_any:
        outputs.append(dump_path)
    if args.timings_draft:
        outputs.append(args.timings_draft)
    if any(path.exists() for path in outputs if path != dump_path):
        raise FileExistsError('Preserve the earlier output before regenerating this report.')

    lines = [f'# Step 8.5t, build{build}', '',
             f'Part P {args.p_status}. Part W {args.w_status}. '
             + references((('Pinned specification', N21 / 'inputs/IMPLEMENTATION_8_5T.md'),
                           ('Whole-item source check', N21 / 'source_check.md'))), '']
    lines.extend(f'- {label} `{value}`.' for label, value in (
        ('Part P', args.p_commit), ('Part W', args.w_commit), ('Revert', args.revert),
        ('Active checkpoint', args.active_commit), ('Active build', args.active_build)) if value)
    lines.extend(OLD['gate_lines']('P', args.p_scoped, args.p_safeguards))
    lines.extend(notes('Readings used (R4)', args.r4_notes) if args.r4_notes else source_readings())
    lines.extend(notes('Fixture and reporting corrections (R3)', args.r3_notes))
    lines.extend(fixture_lines(args.p_fixtures, args.p_fixture))
    p_links = launch_context('Part P', p_g5, p_result)
    if p_links:
        lines.extend(['', p_links])
    if args.p_status == 'PASSED':
        lines.extend(['', 'Part P passed both standing workflow gates and six-file identity.'])
    if native_baseline:
        native = ' / '.join(f"{row['native_seconds']:.9f}" for row in native_baseline)
        lines.extend(['', f'Part P native event maxima (T1) were {native} s.'])
    if 'T1' in cases:
        lines.extend(proof_lines('T1', traces['T1']))
        lines.extend(compare_proof(args.p_case, p_purpose))
        write_rule6(rule6, cases, timelines)
        lines.extend(['', link('Rule 6 counters and modeled storage by event and rank, T0 beside T1', rule6)])
    if args.w_status != 'NOT STARTED':
        lines.extend(OLD['gate_lines']('W', args.w_scoped, args.w_safeguards))
        w_links = launch_context('Part W', w_g5, w_result)
        if w_links:
            lines.extend(['', w_links])
        if w_result and w_result.get('standing_gates_passed'):
            lines.extend(['', 'Both W workflows passed the standing gates. The T0 construction prefix, '
                               'isolated event-two restart and six-file payload repeat passed.'])
        if args.w_status == 'PASSED':
            lines.extend(['', 'The six repeat-identical payloads and the selected event times are the new references.'])
        lines.extend(w_purpose(w_result, w_g5 / 'purpose_rule.json'))
    lines.extend(workflow_table(events))
    incomplete = [(label, case) for label, case in list(cases.items())[1:] if not events.get(label)]
    if incomplete:
        lines.extend(OLD['partial_workflows'](incomplete, timelines))
    if any(label in cases for label in ('T1', 'run1')):
        lines.extend(phase_table(cases))
    for label, trace in traces.items():
        if label in ('run1', 'run2'):
            lines.extend(proof_lines(label, trace))
            lines.extend(OLD['fine_lines'](label, timelines[label]))
    lines.extend(OLD['measurement_lines'](records))
    csv_path, provenance = m28_outputs(stem, records, audit)
    lines.extend(['', link('Per-rank M28 measurements', csv_path) + ' · ' + link('Rank-log provenance', provenance)])
    last_census = M28['stuck_table'](audit).replace('## Stuck cells', '## Stuck cells at the last quality census', 1)
    lines.extend(['', last_census.rstrip()])
    if rejected_any:
        if not dump_path.is_file():
            with dump_path.open('x') as stream:
                stream.writelines(row + '\n' for row in dump)
        lines.extend(['', link('Rejected-event M28, M29, fine placement and final proof dump', dump_path)])
    lines.extend(disposition(args.diagnosis))
    with main.open('x') as stream:
        stream.write('\n'.join(lines) + '\n')
    if args.timings_draft:
        timing_excerpt(args, cases, events, main)
    return outputs


def timing_excerpt(args, cases, events, report_path):
    active = {'T0', 'T1', 'run1', 'run2'} if args.w_status == 'PASSED' else {'T0', 'T1'}
    if args.p_status != 'PASSED':
        raise ValueError('A timings excerpt requires an accepted result.')
    before, after = (
        ' / '.join(f'{max(proof_seconds(timer_ledgers(cases[run], event)).values()):.3f}'
                   for event in (1, 2)) for run in ('T0', 'T1'))
    handbook = NIGHT.parents[4] / 'Handbook'
    build = f'Part P build{args.p_build}'
    if args.w_status == 'PASSED':
        build += f' and Part W build{args.w_build}'
    lines = [f'## Step 8.5t, {build}', '',
             f'Part P reduced the maximum rank-exclusive final certification from {before} s to {after} s '
             'across events 1 and 2. The six P6c/P7 payloads matched.',
             OLD['link']('Step 8.5t report', report_path, handbook)]
    lines.extend(workflow_table({label: rows for label, rows in events.items() if label in active}))
    lines.extend(phase_table({label: case for label, case in cases.items() if label in active}))
    with args.timings_draft.open('x') as stream:
        stream.write('\n'.join(lines) + '\n')


def self_check():
    with TemporaryDirectory() as tmp:
        case = Path(tmp)
        logs = case / 'rank_output/1/rank.0'
        logs.mkdir(parents=True)
        (logs / 'stdout').write_text('\n'.join([
                'NATIVE3D_PARTITION rank=0 parts=6',
                'NATIVE3D_PARTITION rank=0 parts=6',
                'NATIVE3D_M28_STUCK rank=0 cell=31',
                'NATIVE3D_M29_CELL rank=0 cell=31',
                'NATIVE3D_FINE_REQUEST rank=0 round=38',
                '',
            ]))
        with gzip.open(logs / 'stderr.gz', 'wt') as stream:
            stream.write('Wed Nov 18 12:43:10 2026<stderr>:NATIVE3D_FINAL_PROOF rank=0 shared=1 own_roots=3\n')
            stream.write('NATIVE3D_FINAL_CERT entity=1/2/3 within=1\n')
            stream.write('NATIVE3D_FINAL_PROOF rank=0 shared=1')
        rows = trace_rows(case)
        assert [row[1] for row in rows[1]] == list(PROOF)
        assert [row[1] for row in rows[2]] == ['NATIVE3D_M28_STUCK', 'NATIVE3D_M29_CELL', 'NATIVE3D_FINE_REQUEST']
        assert 'rank=0 shared=1 own_roots=3' in '\n'.join(proof_lines('trial', rows))
        assert 'entity=1/2/3 within=1' in '\n'.join(proof_lines('trial', rows))
        with gzip.open(logs / 'stderr.gz', 'at') as stream:
            stream.write(' own_roots=5\nNATIVE3D_FINAL_CERT entity=4/5/6 within=1\n')
        second = trace_rows(case)[2]
        assert [row[1] for row in second[-2:]] == list(PROOF) and 'own_roots=5' in second[-2][2]
    assert [len(timer_ledgers(T0, event)) for event in (1, 2)] == [6, 6]
    old = full_events(T0)
    assert len(old) == 2 and all(event['proof_faces'] > 0 for event in old)
    sample = OLD['by_event'](OLD['read_rank_events'](T0))
    full = read(T0 / 'full_workflow_receipt.json')
    values = rule6_values(sample, full['events'][0], timer_ledgers(T0, 1), 1, 0)
    assert values['stage.exit.full_calls'] == '4' and values['native.native_bound']
    assert values['final_proof.communication.calls'] == '7' and values['timer.fixedstorage'] == '4120'
    print('N21 report self-check passed with synthetic proof and rejection rows and archived T0 receipts.')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('action', choices=('self-check', 'write'))
    parser.add_argument('--p-build', type=int, default=587)
    parser.add_argument('--w-build', type=int, default=588)
    parser.add_argument('--p-status', choices=STATUSES)
    parser.add_argument('--w-status', choices=STATUSES)
    parser.add_argument('--p-case', type=Path)
    parser.add_argument('--run1', type=Path)
    parser.add_argument('--run2', type=Path)
    parser.add_argument('--p-fixtures', type=Path, default=N21 / 'fixtures/acceptance_summary.json')
    for letter, branch in (('p', P), ('w', W)):
        parser.add_argument(f'--{letter}-scoped', type=Path,
                            default=PHASE / f'N21{letter.upper()}/validation/acceptance_summary.json')
        parser.add_argument(f'--{letter}-safeguards', type=Path,
                            default=branch / 'safeguards/acceptance_summary.json')
    for name in ('p-commit', 'w-commit', 'revert', 'active-commit'):
        parser.add_argument(f'--{name}')
    for name in ('p-fixture', 'r3-notes', 'r4-notes'):
        parser.add_argument(f'--{name}', action='append', type=Path, default=[])
    parser.add_argument('--active-build', type=int)
    parser.add_argument('--diagnosis', type=Path)
    parser.add_argument('--timings-draft', type=Path)
    args = parser.parse_args()
    if args.action == 'self-check':
        self_check()
        return
    if args.p_status is None or args.w_status is None:
        parser.error('write requires --p-status and --w-status')
    if args.p_status != 'PASSED' and args.timings_draft:
        parser.error('--timings-draft requires an accepted Part P')
    args.p_case = (args.p_case or P / f'G5/full{args.p_build}').resolve()
    args.run1 = (args.run1 or W / f'G5/full{args.w_build}_run1').resolve()
    args.run2 = (args.run2 or W / f'G5/full{args.w_build}_run2').resolve()
    for path in report(args):
        print(path)


if __name__ == '__main__':
    main()
