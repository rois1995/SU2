"""Read and format the FP2-A Part C §11.4 report rows, including partial attempts."""

from argparse import ArgumentParser
from collections import defaultdict
import gzip
import json
from pathlib import Path
import re
from tempfile import TemporaryDirectory


MODES = ('CORE', 'S1', 'S2')
PHASES = ('preparation', 'construction', 'quality', 'final_proof', 'transfer')
OPERATIONS = ('topology', 'geometry_bound', 'mobility_policy', 'communication')
UNAVAILABLE = ('engine_surface_length_splits', 'engine_surface_geometry_splits',
               'engine_first_census_faces_above_tolerance')
PREFIX = re.compile(r'[A-Z][a-z]{2} [A-Z][a-z]{2} [ \d]\d \d{2}:\d{2}:\d{2} \d{4}<(?:stdout|stderr)>:')
MARKER = re.compile(r'\b(NATIVE3D(?:_LOCAL_SURFACE(?:_STATE)?|_TIMER(?:_SUMMARY)?|_STAGE|_(?:WALL_)?LANE_CENSUS)?)\s+(.*)')
FIELD = re.compile(r'(\w+)=("(?:[^"\\]|\\.)*"|\S+)')


def read_json(path):
    return json.loads(path.read_text()) if path.is_file() else {}


def number(row, key, cast=float):
    try:
        return cast(row[key])
    except (KeyError, TypeError, ValueError):
        return None


def fields(text):
    return {key: re.sub(r'\\(.)', r'\1', value[1:-1]) if value.startswith('"') else value
            for key, value in FIELD.findall(text)}


def scan_rank(lines, rank):
    """Use each rank's own final remesh ledger to delimit adaptation events."""
    events = defaultdict(lambda: {'surface_passes': [], 'surface_states': [], 'native': {}, 'engine_exits': []})
    event, pending, previous = 1, [], None
    for line in lines:
        if not line.endswith(('\n', '\r')):
            continue
        match = MARKER.search(PREFIX.sub('', line))
        if not match:
            continue
        marker, text = match.groups()
        row = fields(text)
        source_rank = number(row, 'rank', int)
        if marker == 'NATIVE3D' and rank == 0 and source_rank is not None and 'complete' in row:
            # Join summaries for all ranks are emitted through rank 0.
            events[event]['native'][source_rank] = row
        elif source_rank != rank:
            continue
        elif marker == 'NATIVE3D_LOCAL_SURFACE':
            events[event]['surface_passes'].append(row)
        elif marker == 'NATIVE3D_LOCAL_SURFACE_STATE':
            events[event]['surface_states'].append(row)
        elif marker == 'NATIVE3D_LANE_CENSUS' and row.get('class') in ('local_wall', 'shared_wall'):
            events[event].setdefault('wall_lane_rows', []).append(row)
        elif marker == 'NATIVE3D_WALL_LANE_CENSUS':
            events[event].setdefault('described_local_wall_quality_rows', []).append(row)
        elif marker == 'NATIVE3D_STAGE':
            item = events[event]
            if 'first_geometry_census' not in item and row.get('census_complete') == row.get('geometry_complete') == '1':
                bad = number(row, 'geometry_bad', int)
                if bad is not None:
                    item['first_geometry_census'] = {'stage': row.get('stage'), 'geometry_invalid_cells': bad}
            if row.get('stage') == 'exit':
                keys = ('length_splits', 'length_collapses', 'surface_rounds', 'interior_rounds', 'rounds')
                item['engine_exits'].append({key: number(row, key, int) for key in keys})
            lower = number(row, 'job_peak_rss_lower', int)
            if lower is not None:
                item['job_peak_lower_bytes'] = max(item.get('job_peak_lower_bytes', 0), lower)
        elif row.get('root') == 'remesh' and row.get('boundary') == 'final':
            if marker == 'NATIVE3D_TIMER':
                pending.append(row)
            elif marker == 'NATIVE3D_TIMER_SUMMARY':
                ledger = {'summary': row, 'rows': pending}
                if ledger != previous:
                    events[event]['ledger'] = ledger
                    event += 1
                    previous = ledger
                pending = []
    if pending:
        events[event]['incomplete_final_timer_rows'] = pending
    return events


def read_logs(case):
    logs = {}
    for path in sorted(case.glob('rank_output/1/rank.*/stdout*')):
        if path.name not in ('stdout', 'stdout.gz'):
            continue
        rank = int(path.parent.name.rsplit('.', 1)[1])
        if rank in logs:
            continue
        opener = gzip.open if path.suffix == '.gz' else open
        with opener(path, 'rt', errors='replace') as stream:
            logs[rank] = scan_rank(stream, rank)
    return logs


def timer_columns(ledger):
    if not ledger:
        return {}, {}, None
    phases = defaultdict(float)
    operations = defaultdict(float)
    for row in ledger.get('rows', ()):
        value = number(row, 'exclusive_seconds')
        if value is None or 'phase' not in row:
            continue
        phases[row['phase']] += value
        if row['phase'] == 'construction' and 'operation' in row:
            operations[row['operation']] += value
    return dict(phases), dict(operations), number(ledger['summary'], 'coverage')


def mesh_counts(case, ordinal, validated):
    """Use the validated cell/point counts and the physical-marker sizes in the published SU2 mesh."""
    counts = dict(validated)
    path = case / f'adapted_mesh_adap_{ordinal:05d}.su2'
    if not counts or not path.is_file():
        return counts
    expected, marker = None, None
    sizes = []
    with path.open('rb') as stream:
        for line in stream:
            key, _, value = line.partition(b'=')
            if key.strip() == b'NMARK':
                expected = int(value)
            elif key.strip() == b'MARKER_TAG':
                marker = value.strip()
            elif key.strip() == b'MARKER_ELEMS' and marker is not None:
                sizes.append(0 if marker == b'SEND_RECEIVE' else int(value))
    if expected is not None and len(sizes) == expected:
        counts['surface_faces'] = sum(sizes)
    return counts


def event_report(case, ordinal, logs, receipt):
    terminal = read_json(case / f'f3b_event{ordinal}_timer_gate.json')
    connected = read_json(case / f'p7p8_event{ordinal}_gate.json')
    summaries = dict(logs.get(0, {}).get(ordinal, {}).get('native', {}))
    summaries.update((int(row['rank']), row) for row in receipt.get('native_summaries', ()))
    ledgers = {int(row['rank']): row['ledger'] for row in terminal.get('ranks', ())}
    ranks = {}
    for rank in sorted(set(logs) | set(summaries) | set(ledgers)):
        raw = logs.get(rank, {}).get(ordinal, {})
        summary = summaries.get(rank, {})
        ledger = ledgers.get(rank, raw.get('ledger'))
        phases, operations, coverage = timer_columns(ledger)
        if not (raw or summary or ledger):
            continue
        states = raw.get('surface_states', [])
        builds = [row for row in states if (number(row, 'splits_all', int) or 0) > 0]
        ranks[rank] = {
            'native_seconds': number(summary, 'total'), 'native_complete': number(summary, 'complete', int),
            'process_peak_bytes': number(summary, 'process_peak_rss', int),
            'job_peak_bytes': number(summary, 'job_peak_rss', int),
            'job_peak_lower_bytes': raw.get('job_peak_lower_bytes'), 'final_timer_present': ledger is not None,
            'timer_coverage': coverage, 'phase_exclusive_seconds': phases, 'construction_exclusive_seconds': operations,
            'surface_passes': raw.get('surface_passes', []), 'surface_states': states,
            'observed_state_builds': len(builds), 'state_build_stages': [row.get('stage') for row in builds],
            'first_reported_geometry_census': raw.get('first_geometry_census'),
            'engine_exit_counters_all_lanes': raw.get('engine_exits', []),
            'engine_wall_lane_rows_mixed_edits': raw.get('wall_lane_rows', []),
            'engine_described_local_wall_quality_rows': raw.get('described_local_wall_quality_rows', []),
        }
        if pending := raw.get('incomplete_final_timer_rows'):
            ranks[rank]['incomplete_final_timer_rows'] = pending

    def observed(key):
        return [row[key] for row in ranks.values() if row[key] is not None]

    coverage = observed('timer_coverage')
    native = observed('native_seconds')
    peaks = observed('job_peak_bytes')
    counts = mesh_counts(case, ordinal, receipt.get('actual_mesh_counts', {}))
    proof = connected.get('proof', {})
    return {
        'event': ordinal, 'ranks': ranks,
        'native_seconds_max_observed': max(native) if native else receipt.get('whole_event_seconds_max_rank'),
        'minimum_timer_coverage_observed': min(coverage) if coverage else None,
        'ranks_with_coverage': len(coverage),
        'job_peak_bytes': connected.get('memory', {}).get('job_peak_rss_bytes', max(peaks) if peaks else None),
        'mesh_counts': counts, 'proof_surface_faces': proof.get('owned_faces_sum'),
        'observed_state_build_stages': sorted({stage for row in ranks.values() for stage in row['state_build_stages']
                                               if stage is not None}),
        **dict.fromkeys(UNAVAILABLE),
    }


def collect(root):
    runs = {}
    for mode in MODES:
        parent = root / mode
        cases = sorted(parent.glob('full*_run1'))
        if len(cases) > 1:
            raise ValueError(f'Ambiguous case directory for {mode}: {cases}')
        case = cases[0] if cases else None
        failure = read_json(parent / 'failed_attempt.json')
        success = read_json(parent / 'run_receipt.json')
        summary = {'case': str(case) if case else None, 'state': 'not observed', 'failure': failure or None, 'events': []}
        if case:
            status = read_json(case / 'status.json')
            full = read_json(case / 'full_workflow_receipt.json')
            logs = read_logs(case)
            events = {row['event']: row for row in full.get('events', ())}
            summary.update(state='failed' if failure else 'sealed' if success.get('passed') else 'partial',
                           stop_reason=status.get('stop_reason'), workflow_errors=full.get('errors', []),
                           binary_sha256=success.get('binary_sha256'), time_rule=success.get('time_rule'),
                           events=[event_report(case, i, logs, events.get(i, {})) for i in (1, 2)])
        elif failure:
            summary['state'] = 'failed'
        runs[mode] = summary
    return {'root': str(root), 'runs': runs}


def shown(value, digits=None):
    if value is None:
        return '—'
    return f'{value:.{digits}f}' if digits is not None else str(value)


def table(headers, rows):
    return ['| ' + ' | '.join(headers) + ' |', '| ' + ' | '.join('---' for _ in headers) + ' |',
            *('| ' + ' | '.join(map(str, row)) + ' |' for row in rows), '']


def format_markdown(report):
    lines = ['# FP2-A Part C workflow comparison', '',
             'Each row uses the final remesh timer ledger for that rank. Partial evidence lists only observed values. '
             'Times are seconds and memory is bytes.', '',
             'R4 marks exact engine surface length splits, geometry splits, and first above-tolerance face counts '
             'unavailable. The engine rows give all-lane length splits, surface-lane rounds, mixed-edit wall-lane '
             'commits, and geometry-invalid cells at the first emitted complete geometry census.', '']
    for mode, run in report['runs'].items():
        line = f'- **{mode}** {run["state"]}.'
        if failure := run['failure']:
            reason = failure.get('first_observed_failure', failure.get('error', ''))
            line += f' Stage {failure.get("stage", "unavailable")}. {reason}'
        elif run.get('workflow_errors'):
            line += f' First workflow error {run["workflow_errors"][0]}.'
        elif run['state'] == 'partial' and run.get('stop_reason'):
            line += f' Controller stop reason `{run["stop_reason"]}`.'
        if timing := run.get('time_rule'):
            outcome = 'passed' if timing['passed'] else 'OPEN for the user'
            line += f' Event 1 time rule {outcome} at {timing["native_seconds"]} s with a {timing["limit_seconds"]} s limit.'
        lines.append(line)
    lines.append('')
    for ordinal in (1, 2):
        lines += [f'## Event {ordinal}', '']
        rows, ranks, engine, surface = [], [], [], []
        for mode, run in report['runs'].items():
            event = next((item for item in run['events'] if item['event'] == ordinal), None)
            if event is None or not event['ranks']:
                rows.append((mode, *(['—'] * 7)))
                continue
            counts = event['mesh_counts']
            coverage = event['minimum_timer_coverage_observed']
            rows.append((mode, shown(event['native_seconds_max_observed'], 3), shown(counts.get('cells')),
                         shown(counts.get('points')), shown(counts.get('surface_faces')),
                         f'{100 * coverage:.4f} ({event["ranks_with_coverage"]} ranks)' if coverage is not None else '—',
                         shown(event['job_peak_bytes']), len(event['observed_state_build_stages'])))
            for rank, row in event['ranks'].items():
                timers = row['final_timer_present']
                get_seconds = lambda ledger, key: shown(row[ledger].get(key, 0. if timers else None), 3)
                ranks.append((mode, rank, shown(row['native_seconds'], 3),
                              *(get_seconds('phase_exclusive_seconds', p) for p in PHASES),
                              *(get_seconds('construction_exclusive_seconds', op) for op in OPERATIONS),
                              shown(row['process_peak_bytes']), row['observed_state_builds']))
                counts = row['first_reported_geometry_census'] or {}
                exits = row['engine_exit_counters_all_lanes']
                lane = row['engine_wall_lane_rows_mixed_edits']
                commits = {stage: [number(item, 'commits', int) for item in lane if item.get('stage') == stage]
                           for stage in ('construction', 'quality')}
                engine.append((mode, rank, ', '.join(shown(x['length_splits']) for x in exits) or '—',
                               ', '.join(shown(x['surface_rounds']) for x in exits) or '—',
                               *(shown(sum(values)) if values and None not in values else '—' for values in commits.values()),
                               shown(counts.get('stage')), shown(counts.get('geometry_invalid_cells'))))
                for title, key in (('pass', 'surface_passes'), ('state', 'surface_states')):
                    for record in row[key]:
                        fields = ' '.join(f'{name}={json.dumps(value) if any(c.isspace() for c in value) else value}'
                                          for name, value in record.items())
                        surface.append(f'{mode} {title} {fields}')
        lines += table(('Case', 'Native max', 'Cells', 'Points', 'Surface faces', 'Min coverage %', 'Job peak', 'Builds observed'), rows)
        if ranks:
            lines += table(('Case', 'Rank', 'Native', *PHASES, *OPERATIONS, 'Process peak', 'Builds observed'), ranks)
            lines += table(('Case', 'Rank', 'Length splits by engine exit', 'Surface rounds by engine exit',
                            'Construction wall commits', 'Quality wall commits',
                            'First census stage emitted', 'Geometry-invalid cells'), engine)
        if surface:
            lines += ['### Surface rows', '', '```text', *surface, '```', '']
    return '\n'.join(lines)


def self_check():
    with TemporaryDirectory(prefix='fp2a-report-') as directory:
        root = Path(directory)
        for mode in ('CORE', 'S1'):
            case = root / mode / 'full625_run1'
            for rank in (0, 1):
                folder = case / f'rank_output/1/rank.{rank}'
                folder.mkdir(parents=True)
                log = (f'NATIVE3D_STAGE rank={rank} stage=construction_entry census_complete=1 geometry_complete=1 geometry_bad=7\n'
                       f'NATIVE3D_STAGE rank={rank} stage=exit length_splits=3 surface_rounds=2\n'
                       f'NATIVE3D_LANE_CENSUS rank={rank} stage=construction class=local_wall commits=4\n')
                if mode == 'S1':
                    log += (f'NATIVE3D_LOCAL_SURFACE rank={rank} stage=0 pass=1 applicable=1 first_feature_reason="two words"\n'
                            f'NATIVE3D_LOCAL_SURFACE_STATE rank={rank} stage=0 splits_all=2\n')
                if rank == 0:
                    log += '\n'.join(f'NATIVE3D rank={r} complete=1 total={10 + r} process_peak_rss=100 job_peak_rss=200'
                                     for r in (0, 1)) + '\n'
                final = (f'Fri Nov 20 21:54:33 2026<stdout>:NATIVE3D_TIMER root=remesh boundary=final rank={rank} '
                         f'phase=construction operation=topology exclusive_seconds={rank + 1}.0\n'
                         f'NATIVE3D_TIMER_SUMMARY root=remesh boundary=final rank={rank} coverage=0.9'
                         f'Fri Nov 20 21:54:33 2026<stdout>:{8 + rank}\n')
                log += final
                if mode == 'S1' and rank == 0:
                    log += final + 'NATIVE3D_LOCAL_SURFACE rank=0 stage=0 pass=1 applicable=1\n'
                    log += 'NATIVE3D_LOCAL_SURFACE rank=0 stage=0 pa'  # A truncated live row is not a record.
                if rank:
                    with gzip.open(folder / 'stdout.gz', 'wt') as stream:
                        stream.write(log)
                else:
                    (folder / 'stdout').write_text(log)
            if mode == 'CORE':
                (case / 'full_workflow_receipt.json').write_text(json.dumps({'events': [
                    {'event': 1, 'actual_mesh_counts': {'cells': 8, 'points': 9}}
                ]}))
                (case / 'adapted_mesh_adap_00001.su2').write_text('NMARK= 2\nMARKER_TAG= WALL\nMARKER_ELEMS= 12\n'
                                                                 'MARKER_TAG= SEND_RECEIVE\nMARKER_ELEMS= 2\n')
                (case.parent / 'run_receipt.json').write_text('{"passed": true, "events": [1]}')
            else:
                (case.parent / 'failed_attempt.json').write_text(
                    '{"stage":"full event gates","error":"stopped","first_observed_failure":"truncated"}')
        report = collect(root)
        core, s1, s2 = (report['runs'][mode] for mode in MODES)
        assert core['state'] == 'sealed' and s1['state'] == 'failed' and s2['state'] == 'not observed'
        assert core['events'][0]['mesh_counts'] == {'cells': 8, 'points': 9, 'surface_faces': 12}
        assert core['events'][0]['native_seconds_max_observed'] == 11
        assert core['events'][0]['ranks'][1]['phase_exclusive_seconds'] == {'construction': 2.}
        assert core['events'][0]['ranks'][1]['engine_wall_lane_rows_mixed_edits'][0]['commits'] == '4'
        assert s1['events'][0]['observed_state_build_stages'] == ['0']
        assert s1['events'][0]['minimum_timer_coverage_observed'] == .98
        assert s1['events'][0]['ranks'][0]['surface_passes'][0]['first_feature_reason'] == 'two words'
        assert len(s1['events'][1]['ranks'][0]['surface_passes']) == 1
        assert s1['events'][1]['minimum_timer_coverage_observed'] is None
        assert s1['events'][1]['ranks'][0]['phase_exclusive_seconds'] == {}
        assert 'geometry-invalid' in format_markdown(report)
        assert 'truncated' in format_markdown(report)
    print('FP2-A §11.4 report self-check passed.')


def main():
    parser = ArgumentParser(description=__doc__)
    parser.add_argument('root', type=Path, nargs='?')
    parser.add_argument('--json', type=Path, dest='json_path')
    parser.add_argument('--markdown', type=Path)
    parser.add_argument('--self-check', action='store_true')
    args = parser.parse_args()
    if args.self_check:
        self_check()
        return
    if args.root is None:
        parser.error('a G5 build directory is required')
    report = collect(args.root)
    if args.json_path:
        args.json_path.write_text(json.dumps(report, indent=2) + '\n')
    markdown = format_markdown(report)
    if args.markdown:
        args.markdown.write_text(markdown)
    elif not args.json_path:
        print(markdown)


if __name__ == '__main__':
    main()
