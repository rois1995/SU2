"""Report RCFD1 remesh events and payload hashes, including failed runs."""

from argparse import ArgumentParser
import gzip
from hashlib import file_digest
import json
from math import isfinite
from os.path import relpath
from pathlib import Path
import runpy
import sys
from tempfile import TemporaryDirectory

sys.dont_write_bytecode = True
HERE = Path(__file__).resolve().parent
NIGHT = HERE.parent
C1_S = NIGHT / 'FP2_C1/G5/build647/S'
PARSER = runpy.run_path(str(NIGHT.parent / 'p7p8_r3_20261115/check_connected_events.py'))
FIELDS = PARSER['fields']
RANKS = range(PARSER['RANKS'])
EVENTS = (1, 2)
CORE = 'NATIVE3D_LOCAL_CORE'
SURFACE = 'NATIVE3D_LOCAL_SURFACE'
MESH = 'adapted_mesh_adap_00001.su2'
EXPECTED_PREFIXES = {'S': ('fbe2c11411d2', 'b13ca887e6c0'), 'OFF': ('104591aecdbc', '4fb162414013')}


def read(path):
    return json.loads(path.read_text()) if path.is_file() else {}


def sha(path):
    with path.open('rb') as stream:
        return file_digest(stream, 'sha256').hexdigest()


def reference(mode):
    if mode == 'S':
        path = C1_S / 'run_receipt.json'
        issue = read(path)
        assert issue['passed'] and issue['build'] == 647 and issue['mode'] == 'S', path
    else:
        accepted = NIGHT / 'N21/accepted_reference.json'
        payload = read(accepted)
        assert payload['accepted'], accepted
        path = (accepted.parent / payload['payloads']['path']).resolve()
        assert sha(path) == payload['payloads']['sha256'], path
        issue = read(path)
        assert issue['accepted'], path
    hashes = {name: issue['hashes'][name] for name in (MESH, MESH + '.native_ref')}
    assert all(digest.startswith(prefix) for digest, prefix in zip(hashes.values(), EXPECTED_PREFIXES[mode]))
    return path, hashes


def rank_events(path, rank):
    """Use the terminal remesh boundary to separate events, retaining an unfinished last event."""
    result = {event: {'core': [], 'surface': [], 'last_collapse': None, 'final_timer': None} for event in EVENTS}
    blocks = []
    if path.is_file():
        stream = path.open(errors='replace')
    elif path.with_suffix('.gz').is_file():
        stream = gzip.open(path.with_suffix('.gz'), 'rt', errors='replace')
    else:
        return result, blocks
    with stream:
        lines = [line for line in stream if line.endswith('\n')]
    if rank == 0:
        blocks = PARSER['event_summary_blocks'](lines)
    event = 1
    for number, line in enumerate(lines, 1):
        raw = PARSER['timer'].strip_mpi_prefixes(line).strip()
        marker = raw.split(maxsplit=1)[0] if raw else ''
        if marker not in (CORE, SURFACE, 'NATIVE3D_LENGTH', 'NATIVE3D_TIMER_SUMMARY'):
            continue
        row = FIELDS(raw, marker)
        if not row or row.get('rank') != str(rank):
            continue
        observation = result[event]
        evidence = {'line': number, 'raw': raw}
        if marker == CORE:
            observation['core'].append(evidence)
        elif marker == SURFACE:
            observation['surface'].append(evidence)
        elif marker == 'NATIVE3D_LENGTH' and row.get('pass') == 'collapse':
            observation['last_collapse'] = {**evidence, 'collapse_accepted': row.get('collapse_accepted')}
        elif marker == 'NATIVE3D_TIMER_SUMMARY' and row.get('root') == 'remesh' and row.get('boundary') == 'final':
            observation['final_timer'] = row
            event += 1
            if event > EVENTS[-1]:
                break
    return result, blocks


def six_number(rows, key, conversion, aggregate):
    try:
        values = [conversion(row[key]) for row in rows]
        if len(values) == len(RANKS) and all(map(isfinite, values)):
            return aggregate(values)
    except (KeyError, TypeError, ValueError):
        pass
    return None


def collect_run(work, mode, build):
    ref_path, ref_hashes = reference(mode)
    case = work / f'full{build}_run1'
    receipt, failure = (work / name for name in ('run_receipt.json', 'failed_attempt.json'))
    sealed, failed = read(receipt), read(failure)
    status = read(case / 'status.json')
    full = read(case / 'full_workflow_receipt.json')
    hashes, sources = {}, {}
    outcome = 'PASSED' if sealed.get('passed') else 'FAILED' if failed else 'pending'
    if status.get('state') == 'terminal' and status.get('full_workflow_accepted') is False:
        outcome = 'FAILED'
    record = sealed or failed
    evidence = {str(path): sha(path) for path in (receipt, failure, case / 'status.json',
                                                case / 'full_workflow_receipt.json') if path.is_file()}
    observed, blocks = {}, []
    for rank in RANKS:
        path = case / f'rank_output/1/rank.{rank}/stdout'
        observed[rank], summaries = rank_events(path, rank)
        if rank == 0:
            blocks = summaries

    events = []
    for event in EVENTS:
        paths = [case / f'f3b_event{event}_timer_gate.json', case / f'p7p8_event{event}_gate.json']
        timed, connected = map(read, paths)
        evidence.update({str(path): sha(path) for path in paths if path.is_file()})
        native = list(blocks[event - 1].values()) if len(blocks) >= event else []
        memory, proof = connected.get('memory', {}), connected.get('proof', {})
        ranks = {str(rank): records[event] for rank, records in observed.items()}
        timer_rows = [row['ledger']['summary'] for row in timed.get('ranks', [])]
        if not timer_rows:
            timer_rows = [row['final_timer'] or {} for row in ranks.values()]
        summary = next((row for row in sealed.get('events', []) if row['event'] == event), {})
        event_receipt = next((row for row in full.get('events', []) if row['event'] == event), {})
        events.append({
            'event': event,
            'native_seconds': summary.get('native_seconds', event_receipt.get(
                'whole_event_seconds_max_rank', six_number(native, 'total', float, max))),
            'job_peak_rss_bytes': memory.get('job_peak_rss_bytes', six_number(native, 'job_peak_rss', int, max)),
            'process_peak_sum_bytes': memory.get('process_peak_sum_bytes', six_number(native, 'process_peak_rss', int, sum)),
            'minimum_timer_coverage': six_number(timer_rows, 'coverage', float, min),
            'proof_faces': proof.get('owned_faces_sum', six_number(native, 'final_proof_faces', int, sum)),
            'surface_faces_by_rank': proof.get('surface_faces_by_rank', [
                int(row['final_proof_surface_faces']) for row in native if row.get('final_proof_surface_faces', '').isdigit()]),
            'timer_passed': timed.get('passed'), 'connected_passed': connected.get('passed'),
            'native_summaries': native, 'ranks': ranks,
        })
        mesh = f'adapted_mesh_adap_{event:05d}.su2'
        restart = f'restart_flow_adap_{event:05d}.dat'
        for name in (mesh, mesh + '.native_ref', restart):
            saved_digest = sealed.get('hashes', {}).get(name) or event_receipt.get('outputs', {}).get(name)
            path = case / name
            if path.is_file():
                hashes[name], sources[name] = sha(path), 'file'
                assert not saved_digest or hashes[name] == saved_digest, path
            else:
                hashes[name] = saved_digest
                sources[name] = 'receipt' if saved_digest else 'unavailable'

    identity = {}
    for name, expected in ref_hashes.items():
        actual = hashes[name]
        identity[name] = {'actual': actual, 'expected': expected, 'reference': str(ref_path),
                          'equal': actual == expected if actual else None}
    return {'case': str(case), 'status': outcome, 'events': events, 'hashes': hashes,
            'hash_sources': sources, 'event1_identity': identity, 'evidence': evidence,
            'failure': record.get('first_observed_failure') or (
                status.get('stop_reason') if outcome == 'FAILED' else None)}


def collect():
    return {mode: collect_run(HERE / 'G5/build647' / mode, mode, 647) for mode in ('S', 'OFF')}


def identity_findings(report):
    return [f'R-{mode} event 1 `{name}` SHA256 was `{row["actual"]}`. '
            f'The reference SHA256 is `{row["expected"]}`.'
            for mode, run in report.items() for name, row in run['event1_identity'].items()
            if row['equal'] is False]


def table(header, rows):
    return ['| ' + ' | '.join(header) + ' |', '| ' + ' | '.join(['---'] * len(header)) + ' |',
            *['| ' + ' | '.join('—' if cell is None else str(cell) for cell in row) + ' |' for row in rows], '']


def link(path, label):
    return f'[{label}]({relpath(path, HERE / "reports")})'


def display(value, fmt):
    return format(value, fmt) if value is not None else None


def outcome(passed):
    return {True: 'passed', False: 'failed', None: 'unavailable'}[passed]


def markdown(report):
    event_rows, collapse_rows, identity_rows, hash_rows, raw = [], [], [], [], []
    lines = ['## Remesh events', '', 'A dash means the corresponding measurement was unavailable.', '']
    for mode, run in report.items():
        label = f'R-{mode}'
        links = [link(path, Path(path).name) for path in run['evidence']]
        state = f'{label} **{run["status"]}**.'
        if run['failure']:
            state += f' The first recorded failure was `{run["failure"]}`.'
        lines.extend([' '.join(part for part in (state, ', '.join(links)) if part), ''])
        for event in run['events']:
            number = event['event']
            surfaces = sorted(set(event['surface_faces_by_rank']))
            event_rows.append((label, number, display(event['native_seconds'], '.3f'), event['job_peak_rss_bytes'],
                               event['process_peak_sum_bytes'], display(event['minimum_timer_coverage'], '.3%'),
                               event['proof_faces'], ', '.join(map(str, surfaces)) or None,
                               outcome(event['timer_passed']), outcome(event['connected_passed'])))
            for rank, rows in event['ranks'].items():
                last = rows['last_collapse'] or {}
                collapse_rows.append((label, number, rank, last.get('collapse_accepted')))
            if mode == 'S':
                rows = sorted((int(rank), row['line'], row['raw']) for rank, measurements in event['ranks'].items()
                              for key in ('core', 'surface') for row in measurements[key])
                raw.extend([f'<details><summary>{label} event {number} local core and surface rows</summary>', '',
                            '```text', *(line for _, _, line in rows), *([] if rows else ['No rows were available.']),
                            '```', '', '</details>', ''])
            else:
                core = sum(len(rows['core']) for rows in event['ranks'].values())
                surface = sum(len(rows['surface']) for rows in event['ranks'].values())
                raw.extend([f'{label} event {number} recorded {core} local core rows and {surface} local surface rows.',
                            ''])
        for name, row in run['event1_identity'].items():
            result = {True: 'equal', False: 'different', None: 'unavailable'}[row['equal']]
            ref_label = 'C1 S' if mode == 'S' else 'N21 W'
            identity_rows.append((label, f'`{name}`', row['actual'], row['expected'], link(row['reference'], ref_label),
                                  result))
        hash_rows.extend((label, f'`{name}`', digest, run['hash_sources'][name]) for name, digest in run['hashes'].items())
    lines += table(('Run', 'Event', 'Native maximum s', 'Job peak bytes', 'Sum of process peaks bytes',
                    'Lowest timer coverage', 'Owned proof faces', 'Surface faces', 'F3b', 'P7/P8'), event_rows)
    lines += ['### Last engine collapse row by rank', '',
              *table(('Run', 'Event', 'Rank', 'collapse_accepted'), collapse_rows),
              '### Local core and surface rows', '', *raw,
              '## Event 1 mesh identity', '',
              *table(('Run', 'File', 'Observed SHA256', 'Reference SHA256', 'Reference', 'Result'), identity_rows),
              '## Workflow payload SHA256', '',
              'A digest from a failed run records the observed artifact.', '',
              *table(('Run', 'File', 'SHA256', 'Source'), hash_rows)]
    return '\n'.join(lines).rstrip() + '\n'


def self_check():
    accepted = collect_run(C1_S, 'S', 647)
    assert accepted['status'] == 'PASSED' and not identity_findings({'S': accepted})
    for event, expected in zip(accepted['events'], ('714', '588'), strict=True):
        rank0 = event['ranks']['0']
        assert len(rank0['core']) == len(rank0['surface']) == 2
        assert rank0['last_collapse']['collapse_accepted'] == expected
        assert event['timer_passed'] and event['connected_passed']
    off = collect_run(NIGHT / 'FX2/G5/build645/OFF', 'OFF', 645)
    assert off['status'] == 'PASSED' and not identity_findings({'OFF': off})
    assert all(not row['core'] and not row['surface'] for event in off['events'] for row in event['ranks'].values())
    with TemporaryDirectory() as directory:
        work = Path(directory)
        path = work / 'full647_run1/rank_output/1/rank.0/stdout'
        path.parent.mkdir(parents=True)
        path.write_text('NATIVE3D_LENGTH rank=0 pass=collapse collapse_accepted=7\n'
                        'NATIVE3D_TIMER_SUMMARY root=remesh boundary=final rank=0\n'
                        'NATIVE3D_LENGTH rank=0 pass=collapse collapse_accepted=8\n'
                        'NATIVE3D_LENGTH rank=0 pass=collapse collapse_accepted=9')
        (work / 'failed_attempt.json').write_text('{"first_observed_failure": "test"}\n')
        (work / 'full647_run1' / MESH).write_bytes(b'partial')
        partial = collect_run(work, 'S', 647)
        assert partial['status'] == 'FAILED' and partial['failure'] == 'test'
        assert [event['ranks']['0']['last_collapse']['collapse_accepted']
                for event in partial['events']] == ['7', '8']
        assert all(event['minimum_timer_coverage'] is None for event in partial['events'])
        assert len(identity_findings({'S': partial})) == 1
        assert partial['event1_identity'][MESH + '.native_ref']['equal'] is None


def main():
    parser = ArgumentParser(description=__doc__)
    group = parser.add_mutually_exclusive_group()
    group.add_argument('--self-check', action='store_true')
    group.add_argument('--json', action='store_true')
    args = parser.parse_args()
    if args.self_check:
        self_check()
        print('RCFD1 event reporting verified on C1 S, FX2 OFF and a partial failed run.')
        return
    report = collect()
    print(json.dumps(report, indent=2) if args.json else markdown(report), end='\n' if args.json else '')


if __name__ == '__main__':
    main()
