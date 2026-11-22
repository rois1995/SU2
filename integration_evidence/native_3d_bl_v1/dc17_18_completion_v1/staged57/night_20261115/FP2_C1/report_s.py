"""Extract the C1 S wall-collapse and engine observations beside the accepted FX2 S run."""

from argparse import ArgumentParser
from decimal import Decimal
import json
from pathlib import Path
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


def main():
    parser = ArgumentParser(description=__doc__)
    parser.add_argument('build', type=int, nargs='?')
    parser.add_argument('--self-check', action='store_true')
    args = parser.parse_args()
    runner.reference_sources()
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
