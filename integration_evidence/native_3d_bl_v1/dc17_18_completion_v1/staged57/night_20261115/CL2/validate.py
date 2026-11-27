"""Read CL2 artifacts and apply the standing per-event gates without launching SU2."""

from argparse import ArgumentParser
from datetime import datetime, timezone
import importlib.util
import json
import math
from pathlib import Path
import re
import struct

import numpy as np

HERE = Path(__file__).resolve().parent
STAGED = HERE.parents[1]
REPO = next(parent for parent in HERE.parents if (parent / 'TestCases/adaptation/capability/capcheck.py').is_file())
CYCLES = 4
RANKS = set(range(6))
MAX_EVENT_SECONDS = 5400
MAX_JOB_BYTES = 19073 * 1024 * 1024
TRANSFER_FIELDS = ('Density', 'Momentum-x', 'Momentum-y', 'Momentum-z', 'Energy')
CERT_FIELDS = ('cells', 'measured', 'q_measured', 'j_measured', 'q_bad', 'j_bad', 'size_bad', 'geometry_bad')
CERT_FLAGS = ('cache_consistent', 'census_complete', 'shape_complete', 'geometry_complete')
STAMP = re.compile(r'([A-Z][a-z]{2} [A-Z][a-z]{2} [ \d]\d \d{2}:\d{2}:\d{2} \d{4})<stdout>:')
NATIVE = re.compile(
    r'\b(NATIVE3D(?:_(?:STAGE|DETAIL|LOCAL_SURFACE|QUALITY|TIMER_SUMMARY))?)\s+(?=\w+=)')


def module(name, path):
    spec = importlib.util.spec_from_file_location(name, path)
    obj = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(obj)
    return obj


def gate(status='not evaluated', reason=None, values=None):
    return {'status': status, 'reason': reason, 'values': values or {}}


def read_text(path):
    if not path.is_file():
        return ''
    return path.read_text(errors='replace')


def complete_lines(text):
    return (line for line in text.splitlines(keepends=True) if line.endswith(('\n', '\r')))


def native_records(helpers, raw, path, line_number):
    clean = helpers.timer.strip_mpi_prefixes(raw)
    matches = list(NATIVE.finditer(clean))
    timestamps = STAMP.findall(raw)
    where = {'source_file': str(path), 'source_line': line_number,
             'timestamp': timestamps[-1] if timestamps else None}
    for index, match in enumerate(matches):
        end = matches[index + 1].start() if index + 1 < len(matches) else len(clean)
        fields = dict(re.findall(r'(\w+)=([^\s]+)', clean[match.end():end]))
        yield match[1], fields | where


def rank_packets(helpers, path, rank):
    """Finalize a rank-local packet only at its terminal remesh root."""
    records, lines, completed, timers = [], [], [], []
    for line_number, raw in enumerate(complete_lines(read_text(path)), 1):
        lines.append(raw)
        local = list(native_records(helpers, raw, path, line_number))
        records.extend(local)
        if not any(kind == 'NATIVE3D_TIMER_SUMMARY' and row.get('root') == 'remesh'
                   and row.get('boundary') == 'final' and row.get('rank') == str(rank)
                   for kind, row in local):
            continue
        try:
            snapshots = [item for item in helpers.timer.snapshots(''.join(lines)) if (
                item['summary'].get('root') == 'remesh' and item['summary'].get('boundary') == 'final'
                and item['summary'].get('rank') == str(rank))]
        except (KeyError, ValueError):
            snapshots = []
        timers.append(snapshots[0] if len(snapshots) == 1 else None)
        completed.append(records)
        records, lines = [], []
    return completed, records, timers


def event_packet(streams, event):
    """The master prints global native/refine rows; each worker prints its own exit."""
    packets, timers = {}, {}
    for rank, (complete, pending, snapshots) in streams.items():
        if len(complete) >= event:
            packets[rank] = complete[event - 1]
            if len(snapshots) >= event:
                timers[rank] = snapshots[event - 1]
        elif len(complete) == event - 1 and pending:
            packets[rank] = pending
    master = packets.get(0, [])
    native = [row for kind, row in master if kind == 'NATIVE3D' and all(key in row for key in ('complete', 'total'))]
    details = [row for kind, row in master if kind == 'NATIVE3D_DETAIL' and row.get('stage') == 'refine']
    exits, surface = {}, {}
    for rank, records in packets.items():
        local = [row for kind, row in records if (
            kind == 'NATIVE3D_STAGE' and row.get('stage') == 'exit' and row.get('rank') == str(rank))]
        if local:
            exits[rank] = local[-1]
        surface[rank] = [row for kind, row in records if (
            kind == 'NATIVE3D_LOCAL_SURFACE' and row.get('rank') == str(rank))]
    return packets, timers, native, details, exits, surface


def rank_set(rows):
    return {row.get('rank') for row in rows} == {str(rank) for rank in RANKS} and len(rows) == len(RANKS)


def certification(rows, details, exits, roots_complete, extra_roots):
    values = {'native_summaries': rows, 'refine_details': details, 'last_worker_exits': exits,
              'native_seconds_max_rank': None, 'refine_seconds_max_rank': None,
              'native_seconds_limit': MAX_EVENT_SECONDS}
    for kind, collection in (('native', rows), ('refine', details)):
        seconds = []
        for row in collection:
            if row.get('complete') == '0' or kind == 'refine' and row.get('stop') not in (None, 'complete'):
                return gate('FAILED', f'rank {row.get("rank")} {kind} rejected the event: {row.get("stop")}', values)
            try:
                elapsed = float(row['total'])
            except (KeyError, ValueError):
                continue
            seconds.append(elapsed)
            if not math.isfinite(elapsed) or not 0 <= elapsed <= MAX_EVENT_SECONDS:
                return gate('FAILED', f'rank {row.get("rank")} {kind} elapsed {elapsed} outside 0–5400 s', values)
        if seconds:
            values[f'{kind}_seconds_max_rank'] = max(seconds)
    if extra_roots:
        return gate('FAILED', f'expected four completed native packets on each rank, found {extra_roots}', values)
    if not roots_complete or not rank_set(rows) or not rank_set(details) or set(exits) != RANKS:
        return gate('OPEN', 'incomplete rank-local terminal native packet, summary, detail or exit', values)
    if any(row.get('ranks') != '6' or row.get('workers') != '6' for row in rows + details):
        return gate('FAILED', 'native summaries or refine details do not describe six workers', values)
    if any(row.get('complete') != '1' for row in rows + details) or any('stop' not in row for row in details):
        return gate('OPEN', 'missing native completion evidence', values)
    try:
        summed = {name: sum(int(row[name]) for row in exits.values()) for name in CERT_FIELDS}
        final = sorted({int(row['final']) for row in rows})
        flags = [int(row[flag]) for row in exits.values() for flag in CERT_FLAGS]
    except (KeyError, ValueError):
        return gate('OPEN', 'missing or malformed exit certificate fields', values)
    values.update(certified_sums=summed, final_counts=final)
    fields = ('q_bad', 'j_bad', 'size_bad', 'geometry_bad')
    if any(summed[name] for name in fields):
        return gate('FAILED', 'nonzero final bad-cell counts: ' + repr({name: summed[name] for name in fields}), values)
    measured = [summed[name] for name in ('cells', 'measured', 'q_measured', 'j_measured')]
    if not all(flag == 1 for flag in flags) or measured != [measured[0]] * 4 or final != [measured[0]]:
        return gate('FAILED', 'inconsistent final cell counts or incomplete certification', values)
    return gate('passed', values=values)


def physical(capcheck, case, event, cert, *, native_reference=True):
    mesh_path = case / f'adapted_mesh_adap_{event:05d}.su2'
    ref_path = mesh_path.with_suffix('.su2.native_ref')
    restart_path = case / f'restart_flow_adap_{event:05d}.dat'
    paths = (mesh_path, ref_path, restart_path) if native_reference else (mesh_path, restart_path)
    values = {'files': [str(path) for path in paths]}
    if not all(path.is_file() and path.stat().st_size for path in paths):
        return gate('OPEN', 'missing or empty ' + ('mesh, native reference or restart' if native_reference else 'mesh or restart'), values)
    # Mesh publication precedes the transferred restart; an interrupted cycle may have only the former.
    try:
        mesh = capcheck.read_su2(mesh_path)
        positions, fields, precision = capcheck.read_restart(restart_path)
    except (OSError, IndexError, UnicodeError, ValueError, struct.error) as error:
        return gate('OPEN', f'unreadable mesh or restart: {error}', values)
    values.update(points=len(mesh.P), cells=len(mesh.E), restart_precision=precision)
    if mesh.dim != 3 or not mesh.E.size or not np.isfinite(mesh.P).all():
        return gate('FAILED', 'mesh is not a finite nonempty 3D simplex mesh', values)
    if mesh.E.min() < 0 or mesh.E.max() >= len(mesh.P):
        return gate('FAILED', 'mesh contains out-of-range connectivity', values)
    volumes = capcheck.volumes(mesh.P, mesh.E, mesh.dim)
    if not np.isfinite(volumes).all():
        return gate('FAILED', 'mesh contains nonfinite volumes', values)
    values['minimum_volume'] = float(volumes.min())
    if values['minimum_volume'] <= 0:
        return gate('FAILED', f'minimum volume is {values["minimum_volume"]}', values)
    expected = cert['values'].get('certified_sums', {}).get('cells')
    if expected is not None and expected != len(mesh.E):
        return gate('FAILED', f'{expected} certified cells but {len(mesh.E)} actual mesh cells', values)
    if positions.shape != mesh.P.shape or not np.isfinite(positions).all():
        return gate('FAILED', 'restart coordinates are nonfinite or wrong-sized', values)
    with np.errstate(over='ignore', invalid='ignore'):
        error = float(np.max(np.abs(positions - mesh.P)))
    if math.isfinite(error):
        values['coordinate_maximum_error'] = error
    if not math.isfinite(error) or error > 1e-12:
        return gate('FAILED', f'restart coordinate error {error} exceeds 1e-12', values)
    required = ('Density', 'Momentum_x', 'Momentum_y', 'Momentum_z', 'Energy')
    if any(name not in fields or not np.isfinite(fields[name]).all() for name in required):
        return gate('FAILED', 'restart lacks finite conservative solution fields', values)
    density = fields['Density']
    values['minimum_density'] = float(density.min())
    if values['minimum_density'] <= 0:
        return gate('FAILED', f'minimum density is {values["minimum_density"]}', values)
    with np.errstate(divide='ignore', over='ignore', invalid='ignore'):
        internal = fields['Energy'] - sum(fields[f'Momentum_{axis}'] ** 2 for axis in 'xyz') / (2 * density)
    if not np.isfinite(internal).all():
        return gate('FAILED', 'restart has nonfinite internal energy', values)
    values['minimum_internal_energy_density'] = float(internal.min())
    if values['minimum_internal_energy_density'] <= 0:
        return gate('FAILED', f'minimum internal energy is {values["minimum_internal_energy_density"]}', values)
    return gate('passed', values=values)


def transfer(section, event, *, native=True):
    markers = ['NATIVE3D rank=' if native else 'MMG3D status SUCCESS', '------------------- Solution Transfer (conservative P1) -------------------',
               'Change of the integrals (sum of value x control volume)', 'Time: setup ', 'The problem uses the new mesh:',
               'Mesh replaced in ', f'Writing the adapted mesh of cycle {event}: adapted_mesh_adap_{event:05d}.su2.']
    if not native:
        # The MMG path has no native summary or mandatory publication marker.
        markers.pop()
    values = {'fields': {}, 'projected_inadmissible': None, 'ordered': None}
    _, header, table = section.partition('Per field: S_n content and S_d content')
    if header:
        table = table.partition('States not admissible after the projection')[0]
    for line in table.splitlines():
        key, sep, columns = line.strip().partition(':')
        if not sep or key not in TRANSFER_FIELDS:
            continue
        numeric, has_suffix, suffix = columns.strip().partition('bounds relaxed')
        tokens = numeric.split()
        if len(tokens) != 7:
            continue
        try:
            parsed = [float(value) for value in tokens]
        except ValueError:
            continue
        values['fields'][key] = {
            'columns': [value if math.isfinite(value) else None for value in parsed],
            'residual': parsed[5] if math.isfinite(parsed[5]) else None,
            'bounds_relaxed': ('bounds relaxed' + suffix) if has_suffix else None,
        }
        if not all(math.isfinite(value) for value in parsed) or abs(parsed[5]) > 1e-10:
            return gate('FAILED', f'{key} transfer residual {parsed[5]} or nonfinite numeric columns', values)
    match = re.search(r'States not admissible after the projection[^\n]*?\): (\d+)(?:, U\^\(n-1\): (\d+))?;', section)
    if match:
        values['projected_inadmissible'] = [int(value) for value in match.groups() if value is not None]
        if any(values['projected_inadmissible']):
            return gate('FAILED', f'nonzero inadmissible transfer states: {values["projected_inadmissible"]}', values)
    positions = [section.rfind(markers[0])] + [section.find(marker) for marker in markers[1:]]
    if all(pos >= 0 for pos in positions):
        values['ordered'] = positions == sorted(positions)
        if not values['ordered']:
            return gate('FAILED', 'remesh, transfer or replacement markers out of order', values)
    if values['ordered'] is None or values['projected_inadmissible'] is None or set(values['fields']) != set(TRANSFER_FIELDS):
        return gate('OPEN', 'missing transfer columns, inadmissible-state count or ordered boundary markers', values)
    return gate('passed', values=values)


def timers_and_proof(helpers, packets, timers, rows):
    values = {'rank_timers': {}, 'owned_proof_faces_sum': None, 'surface_faces_by_rank': {}}
    known_ranks = {row.get('rank') for row in rows}
    if known_ranks.issubset({str(rank) for rank in RANKS}):
        for row in rows:
            try:
                size = int(row['final_proof_surface_faces'])
            except (KeyError, ValueError):
                continue
            values['surface_faces_by_rank'][row['rank']] = size
        surface = set(values['surface_faces_by_rank'].values())
        if len(surface) > 1:
            return gate('FAILED', f'proof surface sizes differ across ranks: {sorted(surface)}', values)
        if rank_set(rows):
            try:
                owned = sum(int(row['final_proof_faces']) for row in rows)
            except (KeyError, ValueError):
                owned = None
            values['owned_proof_faces_sum'] = owned
            if len(values['surface_faces_by_rank']) == len(RANKS) and owned is not None:
                if len(surface) != 1 or next(iter(surface)) <= 0 or owned != next(iter(surface)):
                    return gate('FAILED', f'owned faces sum {owned} does not match surface sizes {sorted(surface)}', values)
    for rank, snapshot in timers.items():
        try:
            if not snapshot:
                continue
            ledger = helpers.timer.assess(snapshot)
            timing = ledger['summary']
            if not ledger['rows']:
                continue
            coverage = float(timing['coverage'])
            join = sum(float(row['exclusive_seconds']) for row in ledger['rows'] if row['operation'] == 'join_wait')
            rank_quality = [row for kind, row in packets[rank] if (
                kind == 'NATIVE3D_QUALITY' and row.get('rank') == str(rank))]
            deltas = [float(row['join_call_seconds']) for row in rank_quality]
        except (KeyError, ValueError, ZeroDivisionError):
            continue
        values['rank_timers'][str(rank)] = {
            'coverage': coverage if math.isfinite(coverage) else None,
            'ledger_gates': ledger['gates'],
            'join_wait_seconds': join if math.isfinite(join) else None,
            'legacy_join_seconds': sum(deltas) if all(math.isfinite(value) for value in deltas) else None,
        }
        if not ledger['passed']:
            return gate('FAILED', f'rank {rank} timer ledger or 95% coverage failed (coverage={coverage})', values)
        if not rank_quality or not any(row.get('stage') == 'exit' for row in rank_quality):
            del values['rank_timers'][str(rank)]
            continue
        if not helpers.join_matches(join, sum(deltas)):
            return gate('FAILED', f'rank {rank} JoinWait {join} disagrees with {sum(deltas)}', values)
    if (len(values['rank_timers']) != len(RANKS) or len(values['surface_faces_by_rank']) != len(RANKS)
            or values['owned_proof_faces_sum'] is None):
        return gate('OPEN', 'missing timer, JoinWait or final proof evidence', values)
    coverage = [row['coverage'] for row in values['rank_timers'].values()]
    values['minimum_coverage'] = min(coverage)
    return gate('passed', values=values)


def event_memory(rows, exits):
    values = {'limit_bytes': MAX_JOB_BYTES, 'job_peak_bytes': None, 'process_peak_sum_bytes': None,
              'exit_lower_bounds_bytes': {}}
    try:
        job = {row['rank']: int(row['job_peak_rss']) for row in rows if 'job_peak_rss' in row}
        process = {row['rank']: int(row['process_peak_rss']) for row in rows if 'process_peak_rss' in row}
        lower = {str(rank): int(row['job_peak_rss_lower']) for rank, row in exits.items() if 'job_peak_rss_lower' in row}
    except (KeyError, ValueError):
        return gate('OPEN', 'malformed native peak evidence', values)
    values['exit_lower_bounds_bytes'] = lower
    peaks = set(job.values())
    if peaks:
        values['job_peak_bytes'] = max(peaks)
    if any(value > MAX_JOB_BYTES for value in peaks.union(lower.values())):
        return gate('FAILED', f'job peak or lower bound exceeds {MAX_JOB_BYTES} bytes', values)
    if len(peaks) > 1:
        return gate('FAILED', f'job peak values disagree across ranks: {sorted(peaks)}', values)
    if any(value <= 0 for value in peaks.union(process.values()).union(lower.values())):
        return gate('FAILED', 'nonpositive native peak RSS', values)
    if not rank_set(rows) or not all(len(item) == len(RANKS) for item in (job, process, lower)):
        return gate('OPEN', 'missing six-rank process peaks, common job peak, or exit lower bounds', values)
    values['process_peak_sum_bytes'] = sum(process.values())
    if values['process_peak_sum_bytes'] != values['job_peak_bytes']:
        return gate('FAILED', f'job_peak_rss {values["job_peak_bytes"]} != SUM(process_peak_rss) '
                    f'{values["process_peak_sum_bytes"]}', values)
    if max(lower.values()) > values['job_peak_bytes']:
        return gate('FAILED', 'worker exit lower bound exceeds final job peak', values)
    return gate('passed', values=values)


def whole_job_memory(peaks):
    values = {'rank_peaks_bytes': peaks, 'sum_bytes': None, 'limit_bytes': MAX_JOB_BYTES}
    if (peaks is None or len(peaks) != len(RANKS)
            or any(not isinstance(value, int) or isinstance(value, bool) or value <= 0 for value in peaks)):
        return gate('OPEN', 'six terminal rank wait/rusage peaks are required', values)
    values['sum_bytes'] = sum(peaks)
    if values['sum_bytes'] > MAX_JOB_BYTES:
        return gate('FAILED', f'terminal rank peak sum {values["sum_bytes"]} exceeds {MAX_JOB_BYTES}', values)
    return gate('passed', values=values)


def event_text(helpers, master, total):
    log = ''.join(helpers.timer.strip_mpi_prefixes(line) for line in complete_lines(master))
    starts = {int(match[1]): match.start() for match in re.finditer(rf'(?m)^\s*Cycle (\d+) of {total}:', log)}
    sections = {}
    for event in range(1, total + 1):
        start = starts.get(event - 1)
        if start is not None:
            sections[event] = log[start:starts.get(event, len(log))]
    return sections, starts


def read_samples(case):
    path = case / 'resources.jsonl'
    if not path.is_file():
        path = case / 'host_resources.jsonl'
    rows = []
    for line in complete_lines(read_text(path)):
        try:
            row = json.loads(line)
            host = row.get('host', row)
            stamp = float(host.get('unix', row['unix']))
            if math.isfinite(stamp) and ('loadavg' in host):
                rows.append((stamp, host['loadavg']))
        except (TypeError, ValueError, KeyError):
            continue
    return rows


def nearest_sample(samples, stamp):
    if not samples or stamp is None:
        return None
    time_unix, load = min(samples, key=lambda row: abs(row[0] - stamp))
    return {'sample_utc': datetime.fromtimestamp(time_unix, timezone.utc).isoformat(),
            'offset_seconds': time_unix - stamp, 'loadavg': load}


def timestamp(value):
    if value is None:
        return None
    try:
        return datetime.strptime(value, '%a %b %d %H:%M:%S %Y').timestamp()
    except ValueError:
        return None


def validate(case, terminal_peaks=None, complete=False, *, total=CYCLES):
    """Return one six-gate record per event, including incomplete and unvisited events."""
    from report_cl2 import read_cycles

    case = Path(case)
    helpers = module('cl1_timer_gates', STAGED / 'f3b_20261115/check_complete_events.py')
    capcheck = module('cl1_capcheck', REPO / 'TestCases/adaptation/capability/capcheck.py')
    paths = {rank: case / f'rank_output/1/rank.{rank}/stdout' for rank in sorted(RANKS)}
    master = read_text(paths[0]) or read_text(case / 'run.log')
    sections, starts = event_text(helpers, master, total)
    streams = {rank: rank_packets(helpers, path, rank) for rank, path in paths.items()}
    cycles = read_cycles(case, total)
    samples = read_samples(case)
    extra_roots = {rank: len(item[0]) for rank, item in streams.items() if len(item[0]) > total}
    events = []
    for event in range(1, total + 1):
        packets, timers, rows, details, exits, surface = event_packet(streams, event)
        section = sections.get(event, '')
        rejection = re.search(r'(?m)^Adaptation skipped: .+Current mesh and solution retained\.$', section)
        started = bool(rows or details or exits or any(surface.values()) or timers or rejection or event in starts
                       or 'Native3D configured working-storage ceiling:' in section or 'NATIVE3D_PARTITION ' in section)
        record = {'event': event, 'gates': {str(index): gate() for index in range(1, 7)}, 'status': 'not evaluated'}
        if not started:
            events.append(record)
            continue
        roots = {rank for rank, stream in streams.items() if len(stream[0]) >= event}
        results = record['gates']
        results['1'] = certification(rows, details, exits, roots == RANKS, extra_roots if event == total else {})
        if rejection:
            record['native_rejection'] = rejection[0]
            results['1'] = gate('FAILED', rejection[0], results['1']['values'])
        results['2'] = physical(capcheck, case, event, results['1'])
        results['3'] = transfer(section, event)
        results['4'] = cycles[event]['history_gate']
        if divergence := cycles[event].get('log_divergence'):
            results['4'] = gate('FAILED', divergence[0], results['4']['values'])
        results['5'] = timers_and_proof(helpers, packets, timers, rows)
        results['6'] = event_memory(rows, exits)
        states = {row['status'] for row in results.values()}
        record['status'] = 'FAILED' if 'FAILED' in states else 'passed' if states == {'passed'} else 'OPEN'
        actual_surface = {rank for rank, observations in surface.items()
                          if any(row.get('applicable') == '1' for row in observations)}
        native_time = results['1']['values']['native_seconds_max_rank']
        end = next((value for row in reversed(rows) if (value := timestamp(row.get('timestamp'))) is not None), None)
        start = (end - native_time if end is not None and native_time is not None
                 and 0 <= native_time <= MAX_EVENT_SECONDS else None)
        record.update(native_seconds=native_time, driver_adapt_seconds=cycles[event - 1].get('adapt_next_seconds'),
                      job_peak_bytes=results['6']['values']['job_peak_bytes'],
                      host_load_start=nearest_sample(samples, start), host_load_end=nearest_sample(samples, end),
                      native_start_utc=datetime.fromtimestamp(start, timezone.utc).isoformat() if start is not None else None,
                      native_end_utc=datetime.fromtimestamp(end, timezone.utc).isoformat() if end is not None else None,
                      local_surface_core_ran=bool(actual_surface) if actual_surface or roots == RANKS else None,
                      local_surface_core_ranks=sorted(actual_surface), terminal_root_ranks=sorted(roots))
        events.append(record)
    whole = whole_job_memory(terminal_peaks)
    failures = [dict(event=row['event'], gate=int(number), reason=result['reason'])
                for row in events for number, result in row['gates'].items() if result['status'] == 'FAILED']
    if whole['status'] == 'FAILED':
        failures.append(dict(event=None, gate=6, reason=whole['reason']))
    passed = complete and not failures and whole['status'] == 'passed' and all(row['status'] == 'passed' for row in events)
    return {'status': 'FAILED' if failures else 'passed' if passed else 'OPEN',
            'events': events, 'cycles': cycles, 'whole_job_memory': whole, 'failures': failures}


def self_check():
    """Exercise four event boundaries, cut evidence and the historical gate inputs."""
    import tempfile

    helpers = module('cl1_timer_gates_test', STAGED / 'f3b_20261115/check_complete_events.py')
    with tempfile.TemporaryDirectory(prefix='su2-cl1-gates-') as temporary:
        path = Path(temporary) / 'stdout'
        text = ''
        for event in range(1, 5):
            text += (f'NATIVE3D_STAGE rank=0 stage=exit cells={event + 100}\n'
                     f'NATIVE3D_STAGE rank=0 stage=exit cells={event}\n'
                     'NATIVE3D_TIMER_SUMMARY root=remesh boundary=final rank=0 ranks=6\n')
        path.write_text(text + 'NATIVE3D_STAGE rank=0 stage=exit cells=7777\n'
                        + 'NATIVE3D_TIMER_SUMMARY root=remesh boundary=final rank=0 ranks=')
        streams = {0: rank_packets(helpers, path, 0)}
        assert len(streams[0][0]) == 4
        assert [event_packet(streams, event)[4][0]['cells'] for event in range(1, 5)] == ['1', '2', '3', '4']
        assert event_packet(streams, 5)[4][0]['cells'] == '7777'
    missing = whole_job_memory(None)
    assert missing['status'] == 'OPEN' and missing['values']['sum_bytes'] is None
    assert whole_job_memory([100] * 6)['status'] == 'passed'
    assert whole_job_memory([MAX_JOB_BYTES] + [1] * 5)['status'] == 'FAILED'
    transfer_rows = ('Per field: S_n content and S_d content\n'
                     'Density: 0 0 0 0 1 2e-11 0 bounds relaxed (4 values, largest 3 of the range)\n'
                     'States not admissible after the projection (predicate): 0; recovered\n')
    result = transfer(transfer_rows, 1)
    assert result['status'] == 'OPEN' and result['values']['fields']['Density']['residual'] == 2e-11
    assert result['values']['fields']['Density']['bounds_relaxed'].startswith('bounds relaxed (4')
    assert transfer(transfer_rows.replace('2e-11', '2e-10'), 1)['status'] == 'FAILED'
    assert transfer(transfer_rows.replace('): 0;', '): 1;'), 1)['status'] == 'FAILED'

    historical = HERE.parent / 'FP3_D/G5/build678/S1/trial1/full678_run1'
    result = validate(historical, total=2)
    assert result['status'] == 'OPEN' and result['whole_job_memory']['status'] == 'OPEN'
    assert all(event['status'] == 'passed' and event['local_surface_core_ran'] for event in result['events'])
    assert [event['job_peak_bytes'] for event in result['events']] == [9225793536, 9338847232]


if __name__ == '__main__':
    cli = ArgumentParser(description=__doc__)
    cli.add_argument('case', type=Path, nargs='?')
    cli.add_argument('--terminal-rusage', type=Path)
    cli.add_argument('--self-check', action='store_true')
    args = cli.parse_args()
    if args.self_check:
        self_check()
    else:
        if not args.case:
            cli.error('case is required unless running --self-check')
        terminal = json.loads(args.terminal_rusage.read_text()) if args.terminal_rusage else {}
        waits = {row['rank']: row['peak_rss_bytes'] for row in terminal.get('rank_waits', []) if row}
        peaks = [waits[rank] for rank in sorted(RANKS)] if set(waits) == RANKS else None
        complete = (terminal.get('all_owned_handles_terminal') and terminal.get('cause') == 'natural'
                    and terminal.get('returncode') == 0)
        print(json.dumps(validate(args.case, peaks, complete), indent=2, allow_nan=False))
