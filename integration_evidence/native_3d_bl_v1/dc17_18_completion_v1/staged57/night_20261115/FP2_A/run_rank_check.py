"""Run the FP2-A Correction 2 MPI rank checks and preserve terminal or partial evidence."""

from argparse import ArgumentParser
from decimal import Decimal, InvalidOperation
import json
import os
from pathlib import Path
import re
import runpy
import signal
import subprocess
import sys
import time

sys.dont_write_bytecode = True
import run_a as a
from run_fp0 import TMPDIR

RUNS = {'K1': (2, True), 'K2': (2, False), 'K3': (6, True)}
WALL = 1200
PASS_ALARM = Decimal(600)
SELECTOR = '[.][NativeLocalCoreSurfaceRankCheck]'
ORDER = a.HERE / 'inputs/FP2_A_CORRECTION_2.md'
ORDER_SHA = 'bf671f5646c4414a817692e8c30ceed88a42171b414db0bdd371e2b2c20fd94a'
REPLAY = a.FP1 / 'resume4/run_replay.py'
HOUSE = a.FP1.parents[1] / 'p7p8_r3_20261115/run_full_workflow.py'
DEADLINE = 'timeout: sending signal KILL to command '
MARKERS = {
    'FP2_A RANK_PASS': 'passes', 'NATIVE3D_LOCAL_SURFACE': 'surface', 'NATIVE3D_LOCAL_SWEEP': 'sweeps',
    'FP2_A RANK_EXACT': 'exact', 'FP2_A RANK_CHECK': 'final_state',
}
ROW = re.compile(r'(?P<marker>' + '|'.join(map(re.escape, MARKERS)) + r') (?P<fields>.*)$')
GLOBAL = ('splits', 'total_owned_faces', 'faces', 'points', 'traces', 'revision', 'snapshot_bytes', 'state_rebuilt')
TIMES = ('seconds_pass', 'seconds_surface_length', 'seconds_surface_distance',
         'seconds_length_work', 'seconds_quality_work')


def rank_input(lab1, ranks):
    config = lab1[0]
    assert config.count(b'ADAP_NATIVE_RANKS= 1\n') == 1
    config = config.replace(b'ADAP_NATIVE_RANKS= 1\n', f'ADAP_NATIVE_RANKS= {ranks}\n'.encode(), 1)
    return config, *lab1[1:]


def command(case, binary, config, sensor, ranks, surface):
    settings = ('LC_ALL=C', 'OMP_NUM_THREADS=1', 'OPENBLAS_NUM_THREADS=1', 'MKL_NUM_THREADS=1', 'OMPI_MCA_osc=pt2pt',
                f'TMPDIR={TMPDIR}', f'SU2_NATIVE_FROZEN_CONFIG={config}', f'SU2_NATIVE_FROZEN_SENSOR3D={sensor}',
                f'SU2_FP2_A_RANK_SURFACE={int(surface)}')
    return ['env', '-C', str(case), *settings, 'nice', '-n', '19', '/usr/bin/time', '-v', '-o', str(case / 'time.txt'),
            'timeout', '--verbose', '--signal=KILL', str(WALL), 'mpirun', '--bind-to', 'none', '-n', str(ranks),
            '--output-filename', str(case / 'rank_output'), str(binary), SELECTOR]


def owned(case):
    """Monitor only the child session and its witnessed descendants through terminal wait."""
    recipe = a.cut.read(case / 'command.json')
    ranks, surface = RUNS[case.name]
    expected = command(case, recipe['binary'], case / 'run.cfg', recipe['sensor'], ranks, surface)
    assert expected == recipe['argv'] and recipe['selector'] == SELECTOR
    assert hasattr(os, 'pidfd_open') and hasattr(signal, 'pidfd_send_signal')
    supervision = runpy.run_path(str(REPLAY))
    read_processes, descendants, identity_alive = (supervision['house'][name] for name in
                                                   ('read_processes', 'descendants', 'identity_alive'))
    kill = supervision['kill_owned']
    sample_peaks = supervision['sample_peaks']
    peaks, witnessed, killed = {}, {}, []
    stop = None
    with (case / 'resources.jsonl').open('x') as resources:
        started = time.monotonic()
        child = subprocess.Popen(expected, start_new_session=True)
        root = a.cut.actual_identity(child.pid)
        witnessed[root['pid'], root['start_ticks']] = root
        while True:
            rows = read_processes()
            roots = {row['pid'] for row in witnessed.values() if identity_alive(row, rows)}
            leader = rows.get(root['pid'])
            if leader is None or leader['start_ticks'] == root['start_ticks']:
                roots.update(pid for pid, row in rows.items() if row['sid'] == root['pid'])
            for pid in descendants(rows, roots):
                row = rows[pid]
                witnessed.setdefault((pid, row['start_ticks']), row)
            live = [row for row in witnessed.values() if identity_alive(row, rows)]
            sample_peaks(live, peaks)
            elapsed = time.monotonic() - started
            resources.write(json.dumps({'elapsed_seconds': elapsed, 'live_pids': [row['pid'] for row in live],
                                        'rank_processes_sampled': len(peaks),
                                        'sampled_peak_lower_bound_bytes': sum(peaks.values())}) + '\n')
            resources.flush()
            code = child.poll()
            if not stop and sum(peaks.values()) > a.MEMORY * 1024:
                stop = 'job_peak_limit'
            if not stop and elapsed >= WALL and live and code != 0:
                stop = 'wrapper_time_limit'
            if stop:
                for row in live:
                    if kill(row):
                        killed.append({'pid': row['pid'], 'start_ticks': row['start_ticks']})
            if code is not None and not live:
                break
            time.sleep(1)
    outer = (case / 'test_outer.log').read_text(errors='replace')
    if DEADLINE in outer and not stop:
        stop = 'wrapper_time_limit'
    terminal = all(a.cut.terminal(row) for row in witnessed.values())
    a.cut.save(case / 'owned_processes.json', {
        'all_observed_terminal': terminal, 'identities': list(witnessed.values()),
        'sigkill_sent_to': killed, 'sampled_rank_peak_lower_bound_bytes': sum(peaks.values()),
        'sampled_rank_peaks_bytes': [{'pid': pid, 'start_ticks': ticks, 'peak_rss_bytes': peak}
                                     for (pid, ticks), peak in sorted(peaks.items())],
        'child_returncode': code, 'elapsed_seconds': time.monotonic() - started, 'stop': stop,
    })
    assert terminal
    return code if code >= 0 else 128 - code


def launch(case, binary, config, sensor, ranks, surface):
    argv = command(case, binary, config, sensor, ranks, surface)
    wrapper = ['nice', '-n', '19', a.cut.PYTHON, '-B', str(Path(__file__).resolve()), '--owned', str(case)]
    a.cut.save(case / 'command.json', {'argv': argv, 'wrapper_argv': wrapper, 'ranks': ranks, 'selector': SELECTOR,
                                      'seconds_limit': WALL, 'binary': str(binary), 'sensor': str(sensor)})
    try:
        a.cut.run_gate(case, 'test', wrapper)
    except RuntimeError:
        assert a.resumed(case) is not None


def number(row, field):
    try:
        value = Decimal(row.get(field, ''))
    except InvalidOperation:
        return None
    return value if value.is_finite() and value >= 0 else None


def rows_by_rank(logs, ranks):
    by_rank = {}
    for rank in range(ranks):
        rows = {name: [] for name in MARKERS.values()}
        for raw in a.ANSI.sub('', logs.get(rank, '')).splitlines(keepends=True):
            if raw.endswith('\n') and (match := ROW.search(raw.rstrip('\r\n'))):
                fields = dict(a.FIELD.findall(match['fields']))
                if fields.get('rank') == str(rank):
                    rows[MARKERS[match['marker']]].append(fields)
        by_rank[str(rank)] = rows
    return by_rank


def first_difference(by_rank, ranks, surface, input_cells, wait, stop):
    if stop:
        return {'field': stop, 'wrapper_seconds_limit': WALL}
    if wait['returncode']:
        return {'field': 'wrapper_returncode', 'observed': wait['returncode'], 'expected': 0}
    states = []
    for rank, data in by_rank.items():
        if [row.get('pass') for row in data['passes']] != ['1', '2']:
            return {'rank': rank, 'field': 'completed_passes', 'rows': data['passes']}
        for row in data['passes']:
            if row.get('surface') != str(int(surface)) or any(number(row, field) is None for field in TIMES):
                return {'rank': rank, 'field': 'pass_fields', 'row': row}
        if [row.get('pass') for row in data['surface']] != ['1', '2'] or any(
                row.get('stage') != '0' or row.get('applicable') != str(int(surface)) for row in data['surface']):
            return {'rank': rank, 'field': 'surface_rows', 'rows': data['surface']}
        if len(data['exact']) != 1 or len(data['final_state']) != 1:
            return {'rank': rank, 'field': 'final_rows', 'exact': data['exact'], 'final_state': data['final_state']}
        exact, state = data['exact'][0], data['final_state'][0]
        expected = {'ranks': str(ranks), 'surface': str(int(surface)), 'snapshot_equal': '1',
                    'exact_nonpositive': '0', 'owned_geometry_equal': '1'}
        if exact.get('nonpositive') != '0' or exact.get('cells') != state.get('output_cells'):
            return {'rank': rank, 'field': 'exact', 'row': exact}
        for field, value in expected.items():
            if state.get(field) != value:
                return {'rank': rank, 'field': field, 'observed': state.get(field), 'expected': value}
        fields = (*GLOBAL, 'input_cells', 'output_cells', 'owned_faces', 'peak_rss')
        if any(not state.get(field, '').isdecimal() for field in fields):
            return {'rank': rank, 'field': 'integer_state_fields', 'row': state}
        states.append({key: int(state[key]) for key in fields})
    reference = states[0]
    for rank, state in enumerate(states):
        for field in GLOBAL:
            if state[field] != reference[field]:
                return {'rank': str(rank), 'field': field, 'observed': state[field], 'expected': reference[field]}
    for name, observed, expected in (
        ('input_cells_sum', sum(row['input_cells'] for row in states), input_cells),
        ('owned_faces_sum', sum(row['owned_faces'] for row in states), reference['faces']),
        ('total_owned_faces', reference['total_owned_faces'], reference['faces']),
        ('state_rebuilt', reference['state_rebuilt'], int(reference['splits'] > 0)),
    ):
        if observed != expected:
            return {'field': name, 'observed': observed, 'expected': expected}
    if not surface and reference['splits'] != 0:
        return {'field': 'surface_off_splits', 'observed': reference['splits'], 'expected': 0}
    if reference['snapshot_bytes'] == 0:
        return {'field': 'snapshot_bytes', 'observed': 0}
    peak = sum(row['peak_rss'] for row in states)
    if peak > a.MEMORY * 1024:
        return {'field': 'job_peak_limit', 'observed_bytes': peak, 'limit_bytes': a.MEMORY * 1024}
    return None


def assess(logs, ranks, surface, input_cells, wait, stop, alarm):
    by_rank = rows_by_rank(logs, ranks)
    first = first_difference(by_rank, ranks, surface, input_cells, wait, stop)
    timed = [row for data in by_rank.values() for row in data['sweeps'] if number(row, 'seconds') is not None]
    longest = sorted(timed, key=lambda row: number(row, 'seconds'), reverse=True)[:5]
    alarms = []
    if alarm:
        for data in by_rank.values():
            for row in data['passes']:
                seconds = number(row, 'seconds_pass')
                if seconds is not None and seconds > PASS_ALARM:
                    alarms.append({key: row.get(key) for key in ('rank', 'pass', 'seconds_pass')})
    return {'ended': first is None, 'state': stop or ('ended' if first is None else 'did_not_end'),
            'first_difference': first, 'ranks': ranks, 'surface': surface, 'by_rank': by_rank,
            'last_sweep_by_rank': {rank: data['sweeps'][-1] if data['sweeps'] else None
                                   for rank, data in by_rank.items()},
            'five_slowest_sweeps': longest, 'alarm_over_600': alarms}


def run_case(case, binary, lab1, base_pins, original):
    ranks, surface = RUNS[case.name]
    case.mkdir(parents=True, exist_ok=True)
    config, sensor, pins = original['prepare'](case, rank_input(lab1, ranks), base_pins)
    wait = a.resumed(case)
    if wait is None:
        launch(case, binary, config, sensor, ranks, surface)
        wait = a.resumed(case)
    processes = a.cut.read(case / 'owned_processes.json')
    assert processes['all_observed_terminal'] and all(a.cut.terminal(row) for row in processes['identities'])
    original['verify'](pins)
    recipe = a.cut.read(case / 'command.json')
    assert recipe['argv'] == command(case, binary, config, sensor, ranks, surface)
    logs = {}
    artifact_paths = [case / name for name in (*a.ARTIFACTS, 'pins.json') if (case / name).is_file()]
    for path in sorted(case.glob('rank_output/*/rank.*/stdout')):
        rank = int(path.parent.name.rsplit('.', 1)[1])
        assert rank not in logs and 0 <= rank < ranks
        logs[rank] = path.read_text(errors='replace')
        artifact_paths.append(path)
        if path.with_name('stderr').is_file():
            artifact_paths.append(path.with_name('stderr'))
    result = assess(logs, ranks, surface, lab1[-1]['cells'], wait, processes['stop'], case.name == 'K3')
    receipt = {'run': case.name, **result, 'wait': wait, 'owned_processes': processes,
               'artifacts': {str(path.relative_to(case)): a.sha(path) for path in artifact_paths}}
    a.sealed(case / 'rank_check_receipt.json', receipt)
    print(f"{case.name}: {receipt['state']} in {wait['elapsed_seconds']:.3f} s.", flush=True)
    return receipt


def prepare(build):
    status, manifest = build / 'build_status.json', build / 'source_pins.json'
    receipt = a.cut.read(status)
    binary, source_sha = a.verify_build(receipt, manifest)
    assert not receipt['new_warnings']
    assert ORDER.with_suffix('.sha256').read_text().split() == [ORDER_SHA, ORDER.name]
    original, _, pins, _, lab1, _ = a.evidence_inputs()
    scripts = (Path(__file__).resolve(), REPLAY, HOUSE, ORDER.with_suffix('.sha256'))
    pins.update({str(path): a.sha(path) for path in scripts})
    pins.update({str(ORDER): ORDER_SHA, str(status): a.sha(status), str(manifest): source_sha,
                 str(binary): receipt['test_binary_sha256']})
    original['verify'](pins)
    return receipt, binary, original, lab1, pins


def summary(results):
    ended = all(results[name]['ended'] for name in RUNS)
    alarms = results['K3'].get('alarm_over_600', [])
    proceeding = ended and not alarms
    order = list(RUNS) if results['K2']['ended'] else ['K2', 'K1', 'K3']
    return {'proceed_11_3': proceeding, 'default_gates_11_2_required': True,
            'status': 'proceed' if proceeding else 'stopped_before_11_3', 'progress_order': order}


def run(build, output, verify_only):
    receipt, binary, original, lab1, pins = prepare(build)
    label = receipt['build']
    output = output or a.HERE / 'rank_check' / f'build{label}'
    if verify_only:
        return {'verified': True, 'launched': False, 'build': label, 'output': str(output)}
    assert Path('/proc/1/comm').read_text().strip() == 'systemd'
    assert receipt['wait']['terminal'] and a.cut.terminal(receipt['wait']['controller'])
    for key in tuple(os.environ):
        if key.startswith(('SU2_FP1_', 'SU2_FP2_', 'SU2_NATIVE_')):
            del os.environ[key]
    output.mkdir(parents=True, exist_ok=True)
    fingerprints = {name: receipt[name] for name in ('build', 'binary_sha256', 'test_binary_sha256', 'source_pins_sha256')}
    provenance = {**fingerprints, 'pins': pins, 'seconds_limit': WALL, 'selector': SELECTOR}
    a.sealed(output / 'provenance.json', provenance)
    results = {}
    for name in RUNS:
        if name == 'K3' and not results['K1']['ended']:
            results[name] = {'ended': False, 'state': 'skipped_k1_did_not_end'}
            assert not (output / name).exists()
            continue
        result = run_case(output / name, binary, lab1, pins, original)
        path = output / name / 'rank_check_receipt.json'
        fields = ('ended', 'state', 'first_difference', 'alarm_over_600')
        results[name] = {**{key: result[key] for key in fields}, 'receipt': str(path), 'receipt_sha256': a.sha(path)}
    original['verify'](pins)
    outcome = {**fingerprints, **summary(results), 'results': results}
    a.sealed(output / 'rank_check_summary.json', outcome)
    return outcome


def self_check():
    def stream(rank):
        rows = []
        for index in (1, 2):
            rows.extend((
                f'\x1b[0mNATIVE3D_LOCAL_SWEEP rank={rank} stage=0 pass={index} seconds={15 * index + rank} max_star=101',
                f'NATIVE3D_LOCAL_SURFACE rank={rank} stage=0 pass={index} applicable=1',
                f'FP2_A RANK_PASS rank={rank} pass={index} surface=1 seconds_pass=600 seconds_surface_length=1 '
                'seconds_surface_distance=2 seconds_length_work=3 seconds_quality_work=4',
            ))
        rows.extend((
            f'FP2_A RANK_EXACT rank={rank} cells=6 nonpositive=0',
            f'FP2_A RANK_CHECK rank={rank} ranks=2 surface=1 input_cells=5 output_cells=6 splits=2 owned_faces=5 '
            'total_owned_faces=10 faces=10 points=7 traces=2 revision=2 snapshot_bytes=10 snapshot_equal=1 '
            'exact_nonpositive=0 owned_geometry_equal=1 state_rebuilt=1 peak_rss=512',
        ))
        return '\n'.join(rows) + '\n'

    logs = {0: stream(0), 1: stream(1)}
    wait = {'returncode': 0}
    report = assess(logs, 2, True, 10, wait, None, True)
    assert report['ended'] and not report['alarm_over_600']
    assert report['five_slowest_sweeps'][0]['seconds'] == '31' and report['last_sweep_by_rank']['0']['seconds'] == '30'
    late = {**logs, 1: logs[1].replace('seconds_pass=600', 'seconds_pass=600.001', 1)}
    assert assess(late, 2, True, 10, wait, None, True)['alarm_over_600'][0]['rank'] == '1'
    stopped = assess({0: logs[0].splitlines(keepends=True)[0]}, 2, True, 10, {'returncode': 137},
                     'wrapper_time_limit', False)
    assert not stopped['ended'] and stopped['last_sweep_by_rank']['0']['max_star'] == '101'
    decision = summary({'K1': report, 'K2': stopped, 'K3': report})
    assert not decision['proceed_11_3'] and decision['progress_order'][0] == 'K2'
    _, _, _, _, lab, _ = a.evidence_inputs()
    assert b'ADAP_NATIVE_RANKS= 6\n' in rank_input(lab, 6)[0]
    assert a.sha(ORDER) == ORDER_SHA


if __name__ == '__main__':
    parser = ArgumentParser(description=__doc__)
    parser.add_argument('build', nargs='?', type=Path)
    parser.add_argument('--output', type=Path)
    group = parser.add_mutually_exclusive_group()
    group.add_argument('--verify-only', action='store_true')
    group.add_argument('--self-check', action='store_true')
    group.add_argument('--owned', type=Path, help='internal MPI timeout supervisor')
    args = parser.parse_args()
    if args.owned:
        raise SystemExit(owned(args.owned.resolve()))
    if args.self_check:
        self_check()
        print('Rank-check inputs, complete/partial rows, time alarm and stop ordering verified.')
    elif args.build:
        result = run(args.build.resolve(), args.output.resolve() if args.output else None, args.verify_only)
        print(json.dumps(result, indent=2))
        raise SystemExit(not (args.verify_only or result['proceed_11_3']))
    else:
        parser.error('supply the pinned test build directory or --self-check')
