"""Extract the C1 S wall-collapse and engine observations beside the accepted FX2 S run."""

from argparse import ArgumentParser
from collections import Counter, defaultdict
from decimal import Decimal
import json
from pathlib import Path
import re
import runpy
import sys

sys.dont_write_bytecode = True
HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
import run_fp2_b_g5 as runner

import report_correction3_s1 as engine

cut, sha, workflow = runner.cut, runner.sha, runner.workflow
C1_CORE_FIELDS = ('collapses_wall_near', 'refused_size_wall', 'refused_shape_wall', 'refused_topology_wall',
                  'refused_valid_wall', 'blocked_wall', 'four_surface_cells', 'short_free', 'short_wall_near',
                  'short_border', 'short_surface')
CORE_TABLE = ('cells_in', 'cells_out', 'collapses', 'collapses_wall_near', 'refused_size_wall',
              'refused_shape_wall', 'refused_topology_wall', 'refused_valid_wall', 'blocked_wall',
              'four_surface_cells', 'short_out', 'short_free', 'short_wall_near', 'short_border',
              'short_surface')
STAGES = set(engine.BOUNDARIES) | {'length_collapse_exit'}
ENGINE_PHASES = ('construction', 'quality', 'geometry repair', 'surface repair', 'size repair')
ZERO = Decimal(0)
CORE_MARKER = re.compile(r'\b(NATIVE3D_LOCAL_(?:CORE|SURFACE(?:_STATE)?))\s+(.*)')
CORE_CLOCKS = (
    ('Core surface length step', 'NATIVE3D_LOCAL_SURFACE', ('seconds_length',)),
    ('Core surface distance step', 'NATIVE3D_LOCAL_SURFACE', ('seconds_distance',)),
    ('Core length work', 'NATIVE3D_LOCAL_CORE', ('seconds_collapse', 'seconds_split')),
    ('Core quality, including V rounds when enabled', 'NATIVE3D_LOCAL_CORE', ('seconds_quality',)),
    ('Core surface-state build', 'NATIVE3D_LOCAL_SURFACE_STATE', ('seconds_gather', 'seconds_build', 'seconds_check')),
)
TIMING_HEAD = (*(name for name, _, _ in CORE_CLOCKS), 'Engine construction', 'Engine quality')
TIMING_TAIL = ('Engine final proof', 'Transfer within the remesh root', 'Sum', 'Rest', 'Native total')


def rank_rows(lines, rank, fields):
    core, stages, collapse = [], [], []
    last_collapse = None
    for line in lines:
        if (row := fields(line, 'NATIVE3D_LOCAL_CORE')) and row.get('rank') == str(rank):
            core.append(row)
        if (row := fields(line, 'NATIVE3D_LENGTH')) and row.get('rank') == str(rank):
            if row.get('pass') == 'collapse':
                last_collapse = row
        if (row := fields(line, 'NATIVE3D_STAGE')) and row.get('rank') == str(rank):
            if row.get('stage') not in STAGES or 'elapsed' not in row:
                continue
            stages.append(row)
            if row['stage'] == 'length_collapse_exit':
                collapse.append({'length': last_collapse, 'exit': row})
                last_collapse = None
    spans = [{'invocation': call, 'phase': phase, 'from': start, 'through': end, 'seconds': str(seconds)}
             for call, phase, start, end, seconds in engine.spans(stages)]
    return {'core_rows': core, 'engine_length_collapse': collapse,
            'engine_quality_entry': [row for row in stages if row['stage'] == 'quality_entry'],
            'engine_spans': spans, 'engine_stage_rows': stages}


def collect(receipt_path, *, candidate=False):
    receipt = cut.read(receipt_path)
    assert receipt['passed'], receipt_path
    case = Path(receipt['case'])
    parse = runpy.run_path(str(workflow.N22 / 'check_connected_events.py'))
    events = []
    for summary in receipt['events']:
        event = summary['event']
        observed = parse['read_event'](case, event)
        assert observed is not None, (case, event)
        _, local, _, _ = observed
        ranks = {str(rank): rank_rows(lines, rank, parse['fields'])
                 for rank, lines in sorted(local.items())}
        missing = []
        for rank, rows in ranks.items():
            for name in ('core_rows', 'engine_length_collapse', 'engine_quality_entry', 'engine_spans'):
                if not rows[name]:
                    missing.append({'rank': rank, 'row': name})
            for index, row in enumerate(rows['core_rows'], 1):
                if candidate and row.get('applicable') == '1':
                    if absent := [key for key in C1_CORE_FIELDS if key not in row]:
                        missing.append({'rank': rank, 'row': 'core_rows', 'number': index, 'fields': absent})
        events.append({'summary': summary, 'ranks': ranks, 'missing_observations': missing})
    return {'build': receipt['build'], 'receipt': str(receipt_path), 'receipt_sha256': sha(receipt_path),
            'events': events, 'hashes': receipt['hashes']}


def table(header, rows):
    return ['| ' + ' | '.join(header) + ' |',
            '| ' + ' | '.join('---' for _ in header) + ' |',
            *['| ' + ' | '.join(map(str, row)) + ' |' for row in rows], '']


def markdown(report):
    events, core, collapses, clocks, repairs, quality, missing = [], [], [], [], [], [], []
    for name, run in report.items():
        for item in run['events']:
            summary = item['summary']
            event = summary['event']
            mesh = summary['actual_mesh_counts']
            events.append([event, name, f'{summary["native_seconds"]:.3f}', mesh['cells'], mesh['points'],
                           summary['proof_faces'], f'{summary["minimum_timer_coverage"]:.3%}',
                           summary['job_peak_rss_bytes']])
            missing.extend({'event': event, 'run': name, **row} for row in item['missing_observations'])
            for rank, rows in item['ranks'].items():
                key = [event, rank, name]
                for row in rows['core_rows']:
                    core.append([*key, row.get('stage', '—'), row.get('pass', '—'),
                                 *(row.get(field, '—') for field in CORE_TABLE)])
                for seq, pair in enumerate(rows['engine_length_collapse'], 1):
                    length, exit_row = pair['length'] or {}, pair['exit']
                    elapsed = length.get('elapsed')
                    collapses.append([*key, seq, f'{float(elapsed):.6f}' if elapsed else '—',
                                      exit_row.get('rounds', '—'), length.get('collapse_accepted', '—')])
                phase_seconds = {phase: Decimal(0) for phase in ENGINE_PHASES}
                for span in rows['engine_spans']:
                    phase = span['phase']
                    phase_seconds[phase] += Decimal(span['seconds'])
                    if 'repair' in phase:
                        repairs.append([*key, span['invocation'], phase, span['from'], span['through'],
                                        f'{Decimal(span["seconds"]):.6f}'])
                clocks.append([*key, *(f'{phase_seconds[phase]:.6f}' for phase in ENGINE_PHASES)])
                for seq, row in enumerate(rows['engine_quality_entry'], 1):
                    quality.append([*key, seq, *(row.get(field, '—') for field in
                                                 ('q_measured', 'q_bad', 'q_debt', 'j_measured', 'j_bad', 'j_debt'))])

    lines = ['# FP2-C1 S full-workflow observations', '',
             'The baseline is FX2 build645 S. The candidate is the single build tested by the C1 S workflow.',
             'The candidate hash list records the changed mesh. The two workflows each passed their own standing '
             'gates and isolated restart.', '',
             *table(('Event', 'Run', 'Native maximum s', 'Cells', 'Points', 'Proof faces',
                     'Lowest timer coverage', 'Job peak bytes'), sorted(events)),
             '## Candidate payload SHA256', '',
             *table(('File', 'SHA256'), sorted(report['C1']['hashes'].items())),
             '## Core pass counters', '',
             'Full per-rank `NATIVE3D_LOCAL_CORE` rows are in [S_report.json](S_report.json).',
             'The baseline predates the wall-coarsening and short-edge class counters.', '',
             *table(('Event', 'Rank', 'Run', 'Stage', 'Pass', *CORE_TABLE), sorted(core)),
             '## Engine collapse and phase clocks', '',
             'Collapse seconds are the emitted engine clock at the `NATIVE3D_LENGTH pass=collapse` boundary. '
             'That clock includes engine setup. Rounds are the engine counter at the adjacent '
             '`length_collapse_exit` stage.', '',
             *table(('Event', 'Rank', 'Run', 'Pass', 'Collapse boundary s', 'Rounds', 'collapse_accepted'),
                    sorted(collapses)),
             'Construction starts at engine start or a return from quality. Spans use the engine elapsed clock '
             'at quality and repair boundaries. The final phase ends at engine exit.', '',
             *table(('Event', 'Rank', 'Run', *(f'{phase} s' for phase in ENGINE_PHASES)), sorted(clocks)),
             '### Individual repair spans', '',
             *table(('Event', 'Rank', 'Run', 'Engine invocation', 'Phase', 'Start', 'End', 'Seconds'),
                    sorted(repairs)),
             '## Shape census at quality entry', '',
             *table(('Event', 'Rank', 'Run', 'Entry', 'q measured', 'q bad', 'q debt',
                     'J measured', 'J bad', 'J debt'), sorted(quality))]
    if missing:
        lines += ['## Missing observations', '', *(f'- {row}' for row in missing), '']
    return '\n'.join(lines)


def timing_rows(stream, rank, local):
    for line in stream:
        if line.endswith(('\n', '\r')) and (match := CORE_MARKER.search(line)):
            marker, body = match.groups()
            row = engine.report.full.fields(engine.report.full.PREFIX.sub('', body))
            if row.get('rank') == str(rank) and row.get('applicable') != '0':
                local[marker].append(row)
        yield line


def continuation_clocks(case):
    clocks = {}
    for rank in range(6):
        path = case / f'rank_output/1/rank.{rank}/stdout'
        if not path.is_file() and not path.with_suffix('.gz').is_file():
            continue
        with engine.report.lab.archived.open_log(path) as stream:
            for event in (1, 2):
                local = {marker: [] for _, marker, _ in CORE_CLOCKS}
                stages = engine.scan_engine(timing_rows(stream, rank, local), rank)['stages']
                clocks[event, rank] = local, stages
    return clocks


def rank_timing(local, stages, ledger, native):
    def pass_keys(rows):
        return {(row['stage'], row['pass']) for row in rows}

    core = local['NATIVE3D_LOCAL_CORE']
    surface = local['NATIVE3D_LOCAL_SURFACE']
    states = local['NATIVE3D_LOCAL_SURFACE_STATE']
    stages_seen = {row['stage'] for row in core}
    expected = {(stage, index) for stage in stages_seen for index in ('1', '2')}
    if not expected or len(core) != len(expected) or pass_keys(core) != expected or \
            len(surface) != len(expected) or pass_keys(surface) != expected or \
            len(states) != len(stages_seen) or {row['stage'] for row in states} != stages_seen:
        raise ValueError('local core, surface or surface-state rows are incomplete')
    parts = {name: sum((Decimal(row[field]) for row in local[marker] for field in fields), ZERO)
             for name, marker, fields in CORE_CLOCKS}
    spans = list(engine.spans(stages))
    if not spans or spans[-1][3] != 'exit' or any(row[0] != 1 or row[4] < 0 for row in spans):
        raise ValueError('the engine rows do not describe one closed invocation')
    if sum((span[4] for span in spans), ZERO) != Decimal(stages[-1]['elapsed']):
        raise ValueError('the phase spans do not reconcile to engine elapsed at exit')

    repairs = Counter()
    for _, phase, _, _, seconds in spans:
        name = f'Engine {phase}'
        if phase not in ('construction', 'quality'):
            repairs[phase] += 1
            name += f' {repairs[phase]}'
        parts[name] = parts.get(name, ZERO) + seconds

    phases = defaultdict(lambda: ZERO)
    for row in ledger['rows']:
        phases[row['phase']] += Decimal(row['exclusive_seconds'])
    # The terminal engine span already includes final proof.
    proof = phases['final_proof']
    if proof > spans[-1][4]:
        raise ValueError('final-proof ledger exceeds the terminal engine span')
    parts[name] -= proof
    parts['Engine final proof'] = proof
    parts['Transfer within the remesh root'] = phases['transfer']
    parts['Sum'] = sum(parts.values(), ZERO)
    parts['Rest'] = native - parts['Sum']
    parts['Native total'] = native
    if min(parts.values()) < 0:
        raise ValueError('the accounting exceeds the native time on this rank')
    return parts


def optional_json(path):
    return cut.read(path) if path.is_file() else {}


def mesh_counts(path):
    counts = {}
    if path.is_file():
        with path.open() as stream:
            for line in stream:
                key, _, value = line.partition('=')
                if key.strip() in ('NPOIN', 'NELEM'):
                    counts[key.strip()] = int(value.split()[0])
                    if len(counts) == 2:
                        break
    return {'cells': counts.get('NELEM', '—'), 'points': counts.get('NPOIN', '—')}


def continuation_case(case):
    full = optional_json(case / 'full_workflow_receipt.json')
    native_blocks = []
    rank0 = case / 'rank_output/1/rank.0/stdout'
    if rank0.is_file() or rank0.with_suffix('.gz').is_file():
        parse = runpy.run_path(str(workflow.N22 / 'check_connected_events.py'))
        with engine.report.lab.archived.open_log(rank0) as stream:
            native_blocks = parse['event_summary_blocks'](stream)
    local = continuation_clocks(case)
    results = {}
    for event in (1, 2):
        data = next((row for row in full.get('events', ()) if row['event'] == event), {})
        recorded = native_blocks[event - 1].values() if len(native_blocks) >= event else ()
        native = {int(row['rank']): Decimal(row['total']) for row in data.get('native_summaries', recorded)}
        proof = optional_json(case / f'p7p8_event{event}_gate.json').get('proof', {})
        counts = data.get('actual_mesh_counts') or mesh_counts(case / f'adapted_mesh_adap_{event:05d}.su2')
        summary = (f'{max(native.values()):.3f}' if native else '—',
                   counts['cells'], counts['points'], proof.get('owned_faces_sum', '—'))
        try:
            timed = optional_json(case / f'f3b_event{event}_timer_gate.json')
            ledgers = {int(row['rank']): row['ledger'] for row in timed.get('ranks', ()) if row['ledger']}
            if set(native) != set(range(6)) or set(native) != set(ledgers):
                raise ValueError('six-rank native summaries or final remesh ledgers are unavailable')
            slowest = min(native, key=lambda rank: (-native[rank], rank))
            recorded_max = Decimal(str(data.get('whole_event_seconds_max_rank', native[slowest])))
            if abs(native[slowest] - recorded_max) > Decimal('0.000001'):
                raise ValueError('the maximum rank differs from the full workflow receipt')
            parts = {rank: rank_timing(*local[event, rank], ledgers[rank], native[rank]) for rank in native}
            results[event] = summary, slowest, parts, None
        except (KeyError, ValueError, ArithmeticError) as error:
            results[event] = summary, None, {}, str(error)
    return results


def continuation_markdown(cases):
    runs = {name: continuation_case(case) for name, case in cases.items()}
    loads = {name: optional_json(case.parent / 'full_host_preflight.json').get('loadavg', '—').split()[0]
             for name, case in cases.items()}
    summary = [(event, name, *run[event][0], loads[name]) for event in (1, 2) for name, run in runs.items()]
    lines = [
        '## Mesh and native time', '',
        *table(('Event', 'Run', 'Native maximum s', 'Cells', 'Points', 'Proof faces',
                'Host load at preflight (1 min)'), summary),
        '## Where the time is', '',
        'Each event uses the rank with the largest native time in the sealed summaries. '
        'Maxima are computed independently over all six ranks, and the maximum sum is the maximum '
        'of rank sums. Core surface clocks include their nested sampling and placement work. Core '
        'quality includes V rounds when enabled. Surface-state build covers gathering, construction '
        'and checking of the state. Ordinary pass setup, exchange and write-back remain in Rest. '
        'Engine phases use nonoverlapping elapsed stage spans. The remesh ledger’s final-proof '
        'exclusive seconds are moved from the terminal engine phase to the separate proof row. '
        'Transfer is the remesh ledger’s transfer phase. Replacement and mesh output have distinct '
        'timer roots outside the native total. Zero marks a part that did not run, and — marks '
        'unavailable evidence.', '',
    ]
    for event in (1, 2):
        lines += [f'### Event {event}', '']
        event_runs = {name: run[event] for name, run in runs.items()}
        repairs = {key for _, _, rank_rows, _ in event_runs.values() for parts in rank_rows.values() for key in parts}
        repairs.difference_update((*TIMING_HEAD, *TIMING_TAIL))
        repairs.add('Engine geometry repair 1')
        repairs = sorted(repairs, key=lambda name: (name.rsplit(' ', 1)[0], int(name.rsplit(' ', 1)[1])))
        header = ['Part']
        for name, (_, slowest, _, _) in event_runs.items():
            header.extend((f'{name} rank {slowest if slowest is not None else "—"}', f'{name} max over ranks'))
        rows = []
        for part in (*TIMING_HEAD, *repairs, *TIMING_TAIL):
            row = [part]
            for _, slowest, ranks, _ in event_runs.values():
                if slowest is None:
                    row.extend(('—', '—'))
                else:
                    row.extend((f'{ranks[slowest].get(part, ZERO):.3f}',
                                f'{max(values.get(part, ZERO) for values in ranks.values()):.3f}'))
            rows.append(row)
        lines += table(header, rows)
        lines += [f'- {name} timing unavailable. {error[0].upper() + error[1:]}.'
                  for name, (_, _, _, error) in event_runs.items() if error]
        lines.append('')
    return '\n'.join(lines)


def main():
    parser = ArgumentParser(description=__doc__)
    parser.add_argument('build', type=int, nargs='?')
    parser.add_argument('--self-check', action='store_true')
    parser.add_argument('--continuation', action='store_true', help='Print the C1 S, S2 and FX2 S timing report.')
    args = parser.parse_args()
    runner.reference_sources()
    if args.continuation:
        if args.build is None:
            parser.error('an FP2-C1 continuation build is required')
        root = HERE / 'G5' / f'build{args.build}'
        cases = {'FX2 S': Path(cut.read(runner.S0_S)['case']),
                 **{mode: root / mode / f'full{args.build}_run1' for mode in ('S', 'S2')}}
        print(continuation_markdown(cases))
        return
    baseline = collect(runner.S0_S)
    if args.self_check:
        assert [item['summary']['event'] for item in baseline['events']] == [1, 2]
        assert all(not item['missing_observations'] for item in baseline['events'])
        rank0 = baseline['events'][0]['ranks']['0']
        assert rank0['engine_length_collapse'][0]['exit']['rounds'] == '8882'
        assert rank0['engine_length_collapse'][0]['length']['collapse_accepted'] == '745'
        assert rank0['engine_quality_entry'][0]['q_bad'] == '834'
        assert any(span['phase'] == 'geometry repair' and span['through'] == 'quality_exit'
                   for span in rank0['engine_spans'])
        timing = continuation_case(Path(cut.read(runner.S0_S)['case']))
        assert timing[1][0] == ('614.463', 550027, 113007, 80650)
        assert timing[2][0] == ('410.522', 575733, 116342, 79274)
        assert [timing[event][1] for event in (1, 2)] == [1, 4]
        assert all(len(timing[event][2]) == 6 and timing[event][3] is None for event in (1, 2))
        print('FP2-C1 report parser verified on the accepted two-event FX2 S receipt.')
        return
    if args.build is None:
        parser.error('an accepted C1 S run build is required')
    work = HERE / 'G5' / f'build{args.build}/S'
    candidate = collect(work / 'run_receipt.json', candidate=True)
    report = {'FX2 S0': baseline, 'C1': candidate}
    workflow.sealed_write(work / 'S_report.json', report)
    text = markdown(report)
    target = work / 'S_report.md'
    if target.exists():
        assert target.read_text() == text, target
    else:
        with target.open('x') as stream:
            stream.write(text)
    print(target)


if __name__ == '__main__':
    main()
