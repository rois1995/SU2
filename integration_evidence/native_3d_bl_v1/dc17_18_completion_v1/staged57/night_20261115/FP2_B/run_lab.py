"""Pin and run the production RunPass equivalence and hand-over labs."""

from argparse import ArgumentParser
from collections import defaultdict
from decimal import Decimal
from hashlib import sha256
from pathlib import Path
import json
import os
import re
import runpy
import subprocess
import sys

sys.dont_write_bytecode = True
HERE = Path(__file__).resolve().parent
B0 = HERE.parent / 'FP2_B0'
sys.path.insert(0, str(B0))
import run_b0 as b0
import report_b0 as archived

a = b0.a
ORDER = HERE / 'inputs/FAST_PATH_STAGE2_B.md'
ORDER_SHA = 'c3ce29f5a4a217bc76377a26056e8595b0a6a4a19e93efac921c70604a777ca4'
REFERENCE = 'bb352600c5'
SECONDS = 2700
RUNS = {'E1': (1, 'EquivalenceSerial'), 'E6': (6, 'EquivalenceMPI'),
        'N1': (1, 'HandoverSerial'), 'N6': (6, 'HandoverMPI')}
GATE_E = {
    'E1': {'handover_bad': 3756, 'final_cells': 561067, 'faces': [84102],
           'bad_before_v': [11296], 'v_collapses': 172, 'v_splits': 1175, 'rounds': 7},
    'E6': {'handover_bad': 3741, 'final_cells': 556192, 'faces': [84124, 84184],
           'bad_before_v': [11644, 3843], 'v_collapses': 209, 'v_splits': 1237},
}
LINE = re.compile(r'^(FP2_B (?:pass|end|bad|state|proof)|FP2_A LAB_S_PROOF|NATIVE3D_LOCAL_SURFACE) (.+)$')
REASONS = ('refused', 'short', 'border', 'limit', 'neighbor_locked', 'unexplained')
FINGERPRINTS = ('build', 'binary_sha256', 'test_binary_sha256', 'source_pins_sha256')


def prepare(build):
    b0.host()
    b0.verify_terminal()
    assert a.sha(ORDER) == ORDER_SHA
    assert ORDER.with_suffix('.sha256').read_text().split() == [ORDER_SHA, ORDER.name]
    status, manifest = (build / name for name in ('build_status.json', 'source_pins.json'))
    receipt = a.cut.read(status)
    binary, _ = a.verify_build(receipt, manifest)
    assert receipt['wait']['terminal'] and receipt['wait']['returncode'] == 0
    assert a.cut.terminal(receipt['wait']['controller']) and not receipt['new_warnings']
    solver = Path(receipt['binary'])
    assert a.sha(solver) == receipt['binary_sha256']
    for key in tuple(os.environ):
        if key.startswith(('SU2_NATIVE_', 'SU2_FP1_', 'SU2_FP2_')):
            del os.environ[key]
    support = (ORDER, ORDER.with_suffix('.sha256'), HERE / 'source_check.md', status, manifest, binary, solver)
    pins = {str(path): a.sha(path) for path in support}
    return receipt, binary, pins


def checked_receipt(case, fingerprints, name='measurement.json'):
    path = case / name
    receipt = a.cut.read(path)
    assert all(receipt[key] == value for key, value in fingerprints.items()), path
    archived.verify_artifacts(case, receipt['artifacts'])
    assert receipt['wait'] == a.cut.read(case / 'test_wait.json'), path
    assert a.cut.terminal(receipt['wait']['controller']), path
    return receipt


def verify_completed(case, fingerprints, expected_rounds):
    result = checked_receipt(case, fingerprints)
    provenance = a.cut.read(case / 'provenance.json')
    pins = a.cut.read(case / 'pins.json')
    assert pins == provenance['pins']
    driver = str(Path(__file__).resolve())
    if pins[driver] != a.sha(Path(driver)):
        assert case.name == 'E1' and result['build'] == 630
        initial = HERE / 'inputs/run_lab.e1_initial.py'
        assert a.sha(initial) == pins[driver]
        pins = {name: digest for name, digest in pins.items() if name != driver}
    for path, digest in pins.items():
        assert a.sha(Path(path)) == digest, path
    processes = a.cut.read(case / 'owned_processes.json')
    assert processes['all_observed_terminal'] and all(a.cut.terminal(row) for row in processes['identities'])
    logs = b0.log_paths(case, result['ranks'])
    data = {rank: parse_rows(path) for rank, path in logs.items()}
    assert {str(rank): rows for rank, rows in data.items()} == result['rows_by_rank']
    assert evaluate(case.name, data, expected_rounds) == result['measurement']
    catch = a.summarize_box(list(logs.values()))
    assert [[list(match) for match in group] for group in catch['catch_summaries']] == result['catch_summaries']
    return result


def selector(name):
    return '[.][NativeLocalCoreVolume' + RUNS[name][1] + ']'


def command(case, binary, config, sensor, ranks, surface):
    assert surface
    env = ['LC_ALL=C', 'OMP_NUM_THREADS=1', 'OPENBLAS_NUM_THREADS=1', 'MKL_NUM_THREADS=1',
           'OMPI_MCA_osc=pt2pt', f'TMPDIR={b0.TMPDIR}', f'SU2_NATIVE_FROZEN_CONFIG={config}',
           f'SU2_NATIVE_FROZEN_SENSOR3D={sensor}']
    argv = ['env', '-C', str(case), *env, 'nice', '-n', '19', '/usr/bin/time', '-v', '-o', str(case / 'time.txt'),
            'timeout', '--verbose', '--signal=KILL', str(SECONDS)]
    if ranks > 1:
        argv += ['mpirun', '--bind-to', 'none', '-n', str(ranks),
                 '--output-filename', str(case / 'rank_output')]
    return [*argv, str(binary), selector(case.name)]


def owned(case):
    runner = b0.legacy
    runner.RUNS = {name: (ranks, True) for name, (ranks, _) in RUNS.items()}
    runner.WALL = SECONDS
    runner.SELECTOR = selector(case.name)
    runner.command = command
    return runner.owned(case)


def reference_rounds(name):
    old = 'X3' if name == 'E1' else 'X6'
    case = B0 / 'cases/build629' / old
    path = case / 'measurement.json'
    committed = subprocess.check_output(['git', 'show', f'{REFERENCE}:{path.relative_to(a.cut.REPO)}'],
                                        cwd=a.cut.REPO)
    assert a.sha(path) == sha256(committed).hexdigest(), path
    receipt = a.cut.read(path)
    assert receipt['passed'] and receipt['case'] == old
    rounds = defaultdict(list)
    pins = {str(path): a.sha(path)}
    for file, digest in receipt['artifacts'].items():
        log = case / file
        if log.name != ('test_outer.log' if old == 'X3' else 'stdout'):
            continue
        archived.verify_artifacts(case, {file: digest})
        pin = log if log.is_file() else archived.archived(log)
        pins[str(pin)] = a.sha(pin)
        with archived.open_log(log) as stream:
            for kind, row in b0.measurement_rows(stream):
                if kind != 'volume_round':
                    continue
                key = int(row['rank']), int(row['pass'])
                assert int(row['round']) == len(rounds[key]) + 1, row
                rounds[key].append(f"c{row['collapses']}s{row['splits']}q{row['quality_sweeps']}")
    assert len(rounds) == (1 if old == 'X3' else 12)
    return dict(rounds), pins


def parse_rows(path):
    rows = defaultdict(list)
    with archived.open_log(path) as stream:
        for line in stream:
            if not line.endswith(('\n', '\r')):
                continue
            line = a.ANSI.sub('', line)
            if match := LINE.fullmatch(line.rstrip('\r\n')):
                marker, values = match.groups()
                rows[marker.rsplit(' ', 1)[-1]].append(dict(a.FIELD.findall(values)))
            elif line.startswith('FP2_B0 '):
                for kind, row in b0.measurement_rows([line]):
                    if kind in ('edge_bins', 'edge', 'stars', 'star_top', 'region'):
                        rows[kind].append(row)
    return dict(rows)


def compare_rounds(observed, expected):
    for rank, phase in sorted(set(observed) | set(expected)):
        actual = observed.get((rank, phase), [])
        baseline = expected.get((rank, phase), [])
        for i in range(max(len(actual), len(baseline))):
            seen = actual[i] if i < len(actual) else None
            want = baseline[i] if i < len(baseline) else None
            if seen != want:
                return {'rank': rank, 'pass': phase, 'round': i + 1, 'observed': seen, 'expected': want}
    return None


def evaluate(name, logs, expected_rounds):
    ranks, _ = RUNS[name]
    passes = 1 if ranks == 1 else 2
    handover = name.startswith('N')
    grid = list(range(1, passes + 1))
    expected = GATE_E.get(name, {})
    totals = {'handover_bad': 0, 'final_cells': 0, 'faces': [0] * passes,
              'bad_before_v': [0] * passes, 'v_collapses': 0, 'v_splits': 0, 'rounds': 0}
    problems, limits, rounds = [], [], {}
    counts = dict.fromkeys('abcd', 0)
    edge_bins = dict.fromkeys(('lt_1e_6', 'lt_1e_4', 'lt_1e_2', 'lt_01'), 0)
    min_edge, proof = None, {}
    for rank in range(ranks):
        rows = logs.get(rank, {})
        fields = ('pass', 'NATIVE3D_LOCAL_SURFACE')
        if handover:
            fields += ('bad', 'edge_bins')
        for field in fields:
            observed = [int(row['pass']) for row in rows.get(field, ())]
            if observed != grid:
                problems.append({'rank': rank, 'row': field, 'passes': observed, 'expected': grid})
        if any(problem.get('rank') == rank for problem in problems):
            continue
        for field in ('end', 'region') if handover else ('end',):
            if len(rows.get(field, ())) != 1:
                problems.append({'rank': rank, 'row': field, 'rows': len(rows.get(field, ())), 'expected': 1})
        if any(problem.get('rank') == rank for problem in problems):
            continue
        end = rows['end'][0]
        if int(end['rank']) != rank or int(end['nonpositive']) != 0:
            problems.append({'rank': rank, 'row': 'end', 'observed': end})
        totals['final_cells'] += int(end['cells_out'])
        totals['handover_bad'] += int(rows['pass'][-1]['bad_out'])
        for i, (run_pass, surface) in enumerate(zip(rows['pass'], rows['NATIVE3D_LOCAL_SURFACE'])):
            phase = i + 1
            if (int(run_pass['rank']) != rank or int(surface['rank']) != rank or int(run_pass['nonpositive']) != 0
                    or int(run_pass['faces_out']) != int(surface['faces_out'])):
                problems.append({'rank': rank, 'pass': phase, 'row': 'pass', 'observed': run_pass})
            counts_this_round = surface['v_changes'].split(',')
            if len(counts_this_round) != int(surface['v_rounds']) or not all(
                    re.fullmatch(r'c[0-9]+s[0-9]+q[0-9]+', token) for token in counts_this_round):
                problems.append({'rank': rank, 'pass': phase, 'v_changes': surface['v_changes']})
            rounds[rank, phase] = counts_this_round
            totals['faces'][i] += int(surface['faces_out'])
            totals['bad_before_v'][i] += int(surface['bad_before_v'])
            for key in ('v_collapses', 'v_splits'):
                totals[key] += int(surface[key])
            totals['rounds'] += int(surface['v_rounds'])
            if handover:
                bad = rows['bad'][i]
                classified = {key: int(bad[f'bad_{key}']) for key in counts}
                if sum(classified.values()) != int(bad['bad']) or int(bad['bad']) != int(run_pass['bad_out']):
                    problems.append({'rank': rank, 'pass': phase, 'row': 'bad', 'observed': bad})
                conditions = {'star_max': (int(surface['star_max']), 1000),
                              'sweeps_distance': (int(surface['sweeps_distance']), 39),
                              'seconds_pass': (float(run_pass['seconds_pass']), 600),
                              'v_limit': (int(surface['v_limit']), 0)}
                for key, (actual, maximum) in conditions.items():
                    if not 0 <= actual <= maximum:
                        limits.append({'rank': rank, 'pass': phase, 'field': key, 'observed': actual,
                                       'maximum': maximum})
        if not handover:
            continue
        if int(rows['region'][0]['cells']) != int(end['cells_out']):
            problems.append({'rank': rank, 'row': 'region', 'observed': rows['region'][0], 'expected': end['cells_out']})
        last = rows['bad'][-1]
        for key in counts:
            counts[key] += int(last[f'bad_{key}'])
        last = rows['edge_bins'][-1]
        for key in edge_bins:
            edge_bins[key] += int(last[key])
        edges = [row for row in rows.get('edge', ()) if int(row['pass']) == passes]
        stars = [row for row in rows.get('stars', ()) if int(row['pass']) == passes]
        top = [row for row in rows.get('star_top', ()) if int(row['pass']) == passes]
        if len(edges) != 20 or len(top) != 20 or {row['group'] for row in stars} != {'surface', 'border', 'free'}:
            problems.append({'rank': rank, 'pass': passes, 'final_edges': len(edges), 'star_groups': len(stars),
                             'largest_stars': len(top)})
        if edges:
            smallest = min(edges, key=lambda row: Decimal(row['metric']))
            if min_edge is None or Decimal(smallest['metric']) < Decimal(min_edge['metric']):
                min_edge = smallest
        if ranks > 1:
            states = rows.get('state', ())
            if len(states) != 1 or any(states[0].get(key) != '1'
                                       for key in ('snapshot_equal', 'owned_geometry_equal')):
                problems.append({'rank': rank, 'row': 'state', 'observed': states})
        if rank == 0:
            proofs = rows.get('LAB_S_PROOF' if ranks == 1 else 'proof', ())
            if len(proofs) == 1:
                proof = proofs[0]
            if (proof.get('qualified') != '1' or any(f'classify_left_{key}' not in proof for key in REASONS[:-1])
                    or proof.get('classify_unexplained') != '0'):
                problems.append({'rank': rank, 'row': 'proof', 'observed': proofs})
    if handover:
        for key, actual, maximum in (('lt_1e_2', edge_bins['lt_1e_2'], 0),
                                     ('handover_bad', totals['handover_bad'], 4000)):
            if actual > maximum:
                limits.append({'field': key, 'observed': actual, 'maximum': maximum})
        for phase, faces in enumerate(totals['faces'], 1):
            if faces > 126153:
                limits.append({'field': 'faces', 'pass': phase, 'observed': faces, 'maximum': 126153})
        return {'totals': totals, 'bad_by_class': counts, 'edge_bins': edge_bins, 'smallest_edge': min_edge,
                'proof': proof, 'problems': problems, 'failed_limits': limits}
    difference = compare_rounds(rounds, expected_rounds)
    return {'totals': totals, 'expected': expected, 'problems': problems, 'first_round_difference': difference,
            'failed_limits': [{'field': key, 'observed': totals[key], 'expected': value}
                              for key, value in expected.items() if totals[key] != value]}


def run(build, name, verify_only=False):
    receipt, binary, pins = prepare(build)
    label = receipt['build']
    fingerprints = {key: receipt[key] for key in FINGERPRINTS}
    ranks, _ = RUNS[name]
    folder = HERE / 'cases' / f'build{label}'
    # Other cases, including on older binaries, may leave inspectable failure receipts.
    for launch in sorted((HERE / 'cases').glob('build*/*/test_launch.json')):
        a.resumed(launch.parent)
        process_file = launch.parent / 'owned_processes.json'
        assert all(a.cut.terminal(row) for row in a.cut.read(process_file)['identities']), launch
    case = folder / name
    case.mkdir(parents=True, exist_ok=True)
    original = runpy.run_path(str(a.LAB_ORIGINAL))
    support = (Path(__file__).resolve(), Path(b0.__file__).resolve(), Path(b0.legacy.__file__).resolve(),
               Path(archived.__file__).resolve(), a.LAB_ORIGINAL, b0.legacy.REPLAY, b0.legacy.HOUSE, a.cut.PREFLIGHT)
    pins.update({str(path): a.sha(path) for path in support})
    expected_rounds, old_pins = reference_rounds(name) if name in GATE_E else ({}, {})
    if verify_only or (case / 'measurement.json').is_file():
        return verify_completed(case, fingerprints, expected_rounds)
    pins.update(old_pins)
    config, sensor, bound = original['prepare'](case, b0.input_data(ranks), pins)
    proof = {**fingerprints, 'case': name, 'ranks': ranks, 'selector': selector(name), 'seconds_limit': SECONDS,
             'pins': bound}
    a.sealed(case / 'provenance.json', proof)
    recipe = command(case, binary, config, sensor, ranks, True)
    wait = a.resumed(case)
    if wait is None:
        wrapper = ['nice', '-n', '19', a.cut.PYTHON, '-B', str(Path(__file__).resolve()), '--owned', str(case)]
        a.sealed(case / 'command.json', {'argv': recipe, 'wrapper_argv': wrapper, 'ranks': ranks,
                                        'selector': selector(name), 'seconds_limit': SECONDS,
                                        'binary': str(binary), 'sensor': str(sensor)})
        try:
            a.cut.run_gate(case, 'test', wrapper)
        except RuntimeError:
            assert a.resumed(case) is not None
        wait = a.resumed(case)
    assert recipe == a.cut.read(case / 'command.json')['argv']
    processes = a.cut.read(case / 'owned_processes.json')
    assert processes['all_observed_terminal'] and all(a.cut.terminal(row) for row in processes['identities'])
    original['verify'](bound)
    logs = b0.log_paths(case, ranks)
    catch = a.summarize_box(list(logs.values()))
    catch['catch_summaries'] = [[list(match) for match in log] for log in catch['catch_summaries']]
    data = {rank: parse_rows(path) for rank, path in logs.items()}
    measurement = evaluate(name, data, expected_rounds)
    peak = max((a.peak_kib(case / 'time.txt') or 0) * 1024, processes['sampled_rank_peak_lower_bound_bytes'])
    passed = (wait['returncode'] == 0 and processes['stop'] is None and len(logs) == ranks
              and catch['passed'] and 0 < peak <= a.MEMORY * 1024 and not measurement['problems']
              and not measurement['failed_limits'] and not measurement.get('first_round_difference'))
    paths = {case / file for file in (*a.ARTIFACTS, 'pins.json', 'provenance.json') if (case / file).is_file()}
    paths.update(path for log in logs.values() for path in (log, log.with_name('stderr')) if path.is_file())
    result = {**fingerprints, **catch, 'passed': passed, 'case': name, 'ranks': ranks, 'wait': wait,
              'supervisor_stop': processes['stop'], 'peak_lower_bound_bytes': peak, 'measurement': measurement,
              'rows_by_rank': {str(rank): rows for rank, rows in data.items()},
              'artifacts': {str(path.relative_to(case)): a.sha(path) for path in sorted(paths)}}
    a.sealed(case / 'measurement.json', result)
    return result


def self_check():
    expected = {(0, 1): ['c1s2q3', 'c0s0q1'], (1, 1): ['c0s0q1']}
    assert compare_rounds(expected, expected) is None
    actual = {**expected, (0, 1): ['c1s2q3']}
    assert compare_rounds(actual, expected) == {'rank': 0, 'pass': 1, 'round': 2, 'observed': None,
                                                'expected': 'c0s0q1'}
    for name in GATE_E:
        rows, _ = reference_rounds(name)
        assert sum(int(token.split('s')[0][1:]) for values in rows.values() for token in values) == GATE_E[name]['v_collapses']
        assert sum(int(token.split('s')[1].split('q')[0]) for values in rows.values() for token in values) == GATE_E[name]['v_splits']


def main():
    parser = ArgumentParser(description=__doc__)
    parser.add_argument('build', type=Path, nargs='?')
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument('--only', choices=tuple(RUNS))
    group.add_argument('--owned', type=Path)
    group.add_argument('--self-check', action='store_true')
    parser.add_argument('--verify-only', action='store_true', help='Check an existing receipt, without a launch.')
    args = parser.parse_args()
    if args.self_check:
        self_check()
        return
    if args.owned:
        raise SystemExit(owned(args.owned))
    if not args.build:
        parser.error('a build directory is required')
    result = run(args.build.resolve(), args.only, args.verify_only)
    print(json.dumps({key: result[key] for key in ('case', 'passed', 'measurement')}, indent=2))
    if not result['passed']:
        raise SystemExit(1)


if __name__ == '__main__':
    main()
