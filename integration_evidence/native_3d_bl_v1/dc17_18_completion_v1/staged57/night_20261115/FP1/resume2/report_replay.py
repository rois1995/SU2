"""Read-only §4 comparison of frozen native replay receipts and the adopted OFF baselines."""

from argparse import ArgumentParser
from collections import defaultdict
import gzip
import hashlib
import json
from pathlib import Path
import re
import runpy
import sys

sys.dont_write_bytecode = True
HERE = Path(__file__).resolve().parent
FP1 = HERE.parent
NIGHT = FP1.parent
R = NIGHT.parent
FP0 = NIGHT / 'FP0/cases/B/replay'
MORNING = FP1 / 'resume/G5/OFF/run_receipt.json'
TIMER = runpy.run_path(str(R / 'f3b_20261115/check_timer_tree.py'))
STRIP = TIMER['strip_mpi_prefixes']
RANKS = set(range(6))
FIELD = re.compile(r'([A-Za-z_]\w*)=([^\s]*)')
ANSI = re.compile(r'\x1b\[[0-?]*[ -/]*[@-~]')
MARKER = re.compile(r'\b(NATIVE3D(?:_LOCAL_CORE|_LOCAL_SHIFT|_PARTITION|_STAGE|_DETAIL|_TIMER_SUMMARY)?'
                    r'|Native3D rejected event:|Native3D frozen stages:|Native3D candidate complexity:)(?=\s)')
CORE_SECONDS = ('seconds_build', 'seconds_collapse', 'seconds_split', 'seconds_quality',
                'seconds_check', 'seconds_write')
EXIT_COUNTS = ('length_collapses', 'length_splits', 'rounds', 'cells',
               'q_bad', 'j_bad', 'size_bad', 'geometry_bad')


def receipt(path):
    return json.loads(path.read_text()) if path.is_file() else {}


def digest(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def read_log(path):
    if path.suffix == '.gz':
        with gzip.open(path, 'rt', errors='replace') as stream:
            return stream.read()
    return path.read_text(errors='replace')


def streams(case):
    """Use each distinct rank stdout once, preferring the uncompressed original."""
    result = {}
    for folder in sorted((case / 'rank_output').glob('*/rank.*')):
        rank = int(folder.name.rsplit('.', 1)[1])
        path = folder / 'stdout'
        if not path.is_file():
            path = path.with_suffix('.gz')
        if path.is_file():
            if rank in result:
                raise ValueError(f'Ambiguous MPI stdout for rank {rank}: {case}')
            result[rank] = path
    return result


def records(path, rank, event):
    """Use the following rank-local terminal remesh root to delimit one event."""
    text = ANSI.sub('', read_log(path))
    rows = []
    ordinal = 1
    for number, raw in enumerate(text.splitlines(keepends=True), 1):
        if not raw.endswith(('\r', '\n')):
            continue
        line = STRIP(raw).rstrip()
        matches = list(MARKER.finditer(line))
        for index, match in enumerate(matches):
            marker = match[1]
            end = matches[index + 1].start() if index + 1 < len(matches) else len(line)
            body = line[match.end():end].strip()
            fields = dict(FIELD.findall(body))
            if marker == 'NATIVE3D_TIMER_SUMMARY':
                if (fields.get('root') == 'remesh' and fields.get('boundary') == 'final'
                        and fields.get('rank') == str(rank)):
                    ordinal += 1
                continue
            if ordinal == event:
                rows.append({'marker': marker, 'source': str(path), 'line': number,
                             'text': line[match.start():end].rstrip(), 'fields': fields})
    snapshots = [snapshot for snapshot in TIMER['snapshots'](text)
                 if snapshot['summary'].get('root') == 'remesh' and snapshot['summary'].get('boundary') == 'final'
                 and snapshot['summary'].get('rank') == str(rank)]
    return rows, snapshots[event - 1] if len(snapshots) >= event else None


def own_rows(rows, marker, rank):
    return [row for row in rows if row['marker'] == marker and row['fields'].get('rank') == str(rank)]


def mesh_counts(path):
    """Read SU2 mesh header counts without decoding connectivity or coordinates."""
    if path is None or not path.is_file():
        return None
    counts = {}
    with path.open('rb') as stream:
        for line in stream:
            if match := re.match(rb'\s*(NELEM|NPOIN)\s*=\s*(\d+)', line):
                counts[{b'NELEM': 'cells', b'NPOIN': 'points'}[match[1]]] = int(match[2])
                if len(counts) == 2:
                    return counts
    return None


def replay_input(case):
    config = case / 'run.cfg'
    if config.is_file():
        text = config.read_text()
        if match := re.search(r'^\s*MESH_FILENAME\s*=\s*([^%\n]+)', text, re.M):
            return case / match[1].strip()
    return case / 'm6.su2'


def rank_core_time(row):
    fields = row['fields']
    result = {key: float(fields[key]) for key in ('seconds', *CORE_SECONDS) if fields.get(key)}
    if set(result) == {'seconds', *CORE_SECONDS}:
        result['seconds_outside_six_fields'] = result['seconds'] - sum(result[key] for key in CORE_SECONDS)
    return {'stage': fields.get('stage'), 'pass': fields.get('pass'), **result}


def integer_fields(row, keys):
    return {key: int(row['fields'][key]) for key in keys if row['fields'].get(key)}


def engine_entry(ranks, input_counts):
    """Count only a demonstrably entered first engine stage."""
    cells = {}
    entry_point_delta = 0
    points_known = input_counts is not None
    for rank, data in ranks.items():
        if data['last_engine_stage'] is None and not data.get('refine_detail_seconds', 0):
            continue
        core = [row['fields'] for row in data['core_rows']
                if row['fields'].get('stage') == '0' and row['fields'].get('applicable') == '1']
        if core:
            last = int(core[-1]['pass'])
            higher = [row['fields'] for row in data['shift_rows']
                      if row['fields'].get('direction') == 'higher']
            if last == 2 and not higher:
                points_known = False
                continue
            cells[rank] = int(higher[0]['cells_after'] if last == 2 else core[-1]['cells_out'])
            if all('points_out' in row and 'points_in' in row for row in core):
                entry_point_delta += sum(int(row['points_out']) - int(row['points_in']) for row in core)
            else:
                points_known = False
        elif data['partition_rows']:
            cells[rank] = int(data['partition_rows'][0]['fields']['cells_after'])
    return {
        'first_stage_cells_by_rank': cells,
        'first_stage_cells': sum(cells.values()) if set(cells) == RANKS else None,
        'first_stage_points': input_counts['points'] + entry_point_delta
                              if points_known and set(cells) == RANKS else None,
        'points_basis': ('Input SU2 mesh count plus the sum of pass-local points_out minus points_in. '
                         'Passes freeze shared points, and shifts only redistribute ownership.'),
    }


def phase_profile(snapshot, core, shift, stage_seen):
    if snapshot is None:
        return {}
    phases = defaultdict(float)
    for row in snapshot['rows']:
        phases[row['phase']] += float(row['exclusive_seconds'])
    numbers = [row['fields'].get('seconds') for row in [*core, *shift]]
    remainder = None
    if stage_seen and 'construction' in phases and all(value is not None for value in numbers):
        remainder = phases['construction'] - sum(map(float, numbers))
    return {'coverage': float(snapshot['summary']['coverage']),
            'fixedstorage': int(snapshot['summary']['fixedstorage']),
            'root_seconds': float(snapshot['summary']['root_seconds']),
            'unattributed_seconds': float(snapshot['summary']['unattributed']),
            'exclusive_seconds_by_phase': dict(phases),
            'construction_exclusive_less_completed_core_and_shift_seconds': remainder}


def read_event(case, ordinal, input_path, output_path, accepted=None):
    files = streams(case)
    inputs = mesh_counts(input_path)
    ranks = {}
    result_rows, messages, status_rows, refine_details = [], [], {}, {}
    frozen_stages = complexity = None
    for rank, path in files.items():
        rows, snapshot = records(path, rank, ordinal)
        core = own_rows(rows, 'NATIVE3D_LOCAL_CORE', rank)
        shift = own_rows(rows, 'NATIVE3D_LOCAL_SHIFT', rank)
        stages = own_rows(rows, 'NATIVE3D_STAGE', rank)
        terminal_stages = [row for row in stages if row['fields'].get('stage') == 'exit']
        ranks[rank] = {'stdout': str(path), 'stdout_sha256': digest(path), 'core_rows': core, 'shift_rows': shift,
                       'core_seconds_by_pass': [rank_core_time(row) for row in core],
                       'partition_rows': own_rows(rows, 'NATIVE3D_PARTITION', rank),
                       'last_engine_stage': stages[-1] if stages else None,
                       'engine_exits': [integer_fields(row, EXIT_COUNTS) for row in terminal_stages],
                       'timer': phase_profile(snapshot, core, shift, bool(stages))}
        for row in rows:
            fields = row['fields']
            if row['marker'] == 'NATIVE3D' and {'complete', 'total', 'rank'} <= fields.keys():
                if rank == 0 or int(fields['rank']) not in status_rows:
                    status_rows[int(fields['rank'])] = row
                if fields['complete'] == '0':
                    result_rows.append(row)
            elif row['marker'] == 'Native3D rejected event:':
                messages.append(row)
            elif row['marker'] == 'NATIVE3D_DETAIL' and fields.get('stage') == 'refine' and 'total' in fields:
                refine_details[int(fields['rank'])] = float(fields['total'])
            elif rank == 0 and row['marker'] == 'Native3D frozen stages:' and 'count' in fields:
                frozen_stages = int(fields['count'])
            elif rank == 0 and row['marker'] == 'Native3D candidate complexity:' and 'value' in fields:
                complexity = float(fields['value'])

    for rank, row in status_rows.items():
        if rank not in ranks:
            continue
        fields = row['fields']
        ranks[rank].update(native_wall_seconds=float(fields['total']),
                           process_peak_rss_bytes=int(fields['process_peak_rss']) if 'process_peak_rss' in fields else None)
    for rank, seconds in refine_details.items():
        if rank in ranks:
            ranks[rank]['refine_detail_seconds'] = seconds
    if accepted is None and set(status_rows) == RANKS:
        accepted = all(row['fields']['complete'] == '1' for row in status_rows.values())
    full_status = set(status_rows) == RANKS
    timers = [row['timer'] for row in ranks.values() if row['timer']]
    exits = [stage for row in ranks.values() for stage in row['engine_exits']]
    complete_exits = set(ranks) == RANKS and all(row['engine_exits'] for row in ranks.values())
    phase_names = sorted({phase for timer in timers for phase in timer['exclusive_seconds_by_phase']})
    phase_ranges = {}
    if len(timers) == len(RANKS):
        for phase in phase_names:
            values = [timer['exclusive_seconds_by_phase'].get(phase, 0) for timer in timers]
            phase_ranges[phase] = {'min': min(values), 'max': max(values), 'mean': sum(values) / len(values)}
    job_peaks = {int(row['fields']['job_peak_rss']) for row in status_rows.values() if 'job_peak_rss' in row['fields']}
    observed_peaks = [row.get('process_peak_rss_bytes') for row in ranks.values()]
    return {
        'case': str(case), 'log_event_ordinal': ordinal, 'accepted': accepted,
        'first_failure': {'expected_complete': 1, 'observed_complete': 0, 'row': result_rows[0]}
                         if result_rows else None,
        'first_message': messages[0] if messages else None,
        'native_wall_seconds_max_rank': max(float(row['fields']['total']) for row in status_rows.values())
                                        if full_status else None,
        'remesh_timer_root_seconds_max_rank': max(timer['root_seconds'] for timer in timers)
                                               if len(timers) == len(RANKS) else None,
        'job_peak_rss_bytes': next(iter(job_peaks)) if full_status and len(job_peaks) == 1 else None,
        'process_peak_sum_bytes': sum(observed_peaks) if len(observed_peaks) == len(RANKS)
                                  and None not in observed_peaks else None,
        'minimum_timer_coverage': min(timer['coverage'] for timer in timers) if len(timers) == len(RANKS) else None,
        'remesh_exclusive_phase_seconds': phase_ranges,
        'engine_entry': engine_entry(ranks, inputs),
        'engine_exit': {'accepted_mesh': mesh_counts(output_path) if accepted else None,
                        'reported_final_cells_by_rank': {rank: int(row['fields']['final'])
                                                         for rank, row in status_rows.items()
                                                         if 'final' in row['fields']}},
        'engine_accepted_collapses': sum(row['length_collapses'] for row in exits) if complete_exits else None,
        'engine_accepted_splits': sum(row['length_splits'] for row in exits) if complete_exits else None,
        'engine_rounds_by_rank_and_stage': {rank: [stage['rounds'] for stage in row['engine_exits']]
                                            for rank, row in ranks.items()},
        'normalization_stages': frozen_stages, 'complexity_value': complexity,
        'input_mesh': {'path': str(input_path), 'counts': inputs},
        'rank_data': ranks,
    }


def add_frozen_wall(event, result):
    """Retain the wrapper's maximum of individual Remesh call intervals."""
    event['frozen_fixture_result'] = result
    event['comparison_wall_seconds'] = result.get('seconds')
    event['comparison_wall_seconds_basis'] = 'Frozen fixture construction_result seconds'


def compare(case, event, input_mesh):
    morning = receipt(MORNING)
    morning_case = Path(morning['case'])
    morning_event = morning['events'][event - 1]
    morning_mesh = morning_case / ('m6.su2' if event == 1 else 'adapted_mesh_adap_00001.su2')
    report = {'event': event, 'morning_fp1_off': read_event(
        morning_case, event, morning_mesh, morning_case / f'adapted_mesh_adap_{event:05}.su2', accepted=True)}
    assert report['morning_fp1_off']['engine_exit']['accepted_mesh'] == morning_event['actual_mesh_counts']
    report['morning_fp1_off'].update(comparison_wall_seconds=morning_event['native_seconds'],
                                    comparison_wall_seconds_basis='NATIVE3D total maximum over six ranks')
    if event == 1:
        frozen = receipt(FP0 / 'construction_result.json')
        report['fp0_off_replay'] = read_event(FP0, 1, replay_input(FP0), FP0 / 'frozen_geometry_accepted.su2',
                                             accepted=frozen['accepted'])
        add_frozen_wall(report['fp0_off_replay'], frozen)
    if case is not None:
        driver = receipt(case / 'replay_receipt.json')
        native = receipt(case / 'construction_result.json') or driver.get('native') or {}
        on = read_event(case, 1, input_mesh or replay_input(case), case / 'frozen_geometry_accepted.su2',
                        accepted=driver.get('accepted', native.get('accepted')))
        add_frozen_wall(on, native)
        on['wrapper_wait'] = receipt(case / 'test_wait.json') or driver.get('wait')
        on['runner_first_message'] = driver.get('first_message')
        if on['first_message'] is None and driver.get('first_message'):
            on['first_message'] = {'text': driver['first_message'], 'source': str(case / 'replay_receipt.json')}
        report['fp1_on_replay'] = on
    report['notes'] = [
        'The frozen-volume driver advances no CFD, publishes no transfer or restart payload, and does not establish '
        'normalization accuracy. Catch exit code alone does not establish remesh acceptance.',
        'The morning comparator ran full G5. The FP0 event-one comparator used the stage-capture observer.',
        'The ordered frozen-replay comparison uses maximum Remesh call intervals from the fixture receipt. '
        'The native total rows and timer-root seconds are separately retained.',
        'Only final remesh-root ledgers contribute per-phase exclusive seconds. Construction less core and shifts '
        'also contains other construction work. Inclusive operation clocks are nested.',
        'Rank-local pass rows preserve all fields. Shift rows lack an explicit normalization stage field.',
        'Missing rows or exit meshes are reported as unavailable. Post-pass point counts are for the first engine '
        'entry only. A later normalization-stage engine entry lacks a unique prior-stage point count in these logs.',
    ]
    if event == 2:
        report['notes'].append('FP0 has no event-two replay comparator.')
    return report


def main():
    parser = ArgumentParser(description=__doc__)
    parser.add_argument('case', type=Path, nargs='?', help='terminal ON replay case; omit for OFF comparators only')
    parser.add_argument('--event', type=int, choices=(1, 2), help='defaults to replay receipt event, then event 1')
    parser.add_argument('--input-mesh', type=Path)
    args = parser.parse_args()
    case = args.case.resolve() if args.case else None
    driver = receipt(case / 'replay_receipt.json') if case else {}
    event = args.event or driver.get('event', 1)
    if event not in (1, 2):
        parser.error(f'invalid replay event: {event}')
    if case is not None and 'event' in driver and event != driver['event']:
        parser.error('requested event differs from the replay receipt')
    print(json.dumps(compare(case, event, args.input_mesh), indent=2, allow_nan=False))


if __name__ == '__main__':
    main()
