"""Run one of the two FP2-C2 repetitions on the frozen hand-over input."""

from argparse import ArgumentParser
import json
import math
from pathlib import Path
import re
import sys
from unittest.mock import patch

sys.dont_write_bytecode = True
DRIVER = Path(__file__).resolve()
HERE = DRIVER.parent
C1 = HERE.parent / 'FP2_C1'
sys.path.insert(0, str(C1))
import run_c1_lab as c1

lab, a, c3 = c1.lab, c1.a, c1.c3
ORDER = HERE / 'inputs/FAST_PATH_STAGE2_C2.md'
ORDER_SHA = 'fdd678fed866cb0fb9171316ca1f8e534ea848357918fb1956d95bdbb2bf7cae'
SOURCE_CHECK = HERE / 'source_check.md'
WATCHDOG = lab.SECONDS
PASS_TIME_RULE = 600
WATCHDOG_ORDER = C1 / 'inputs/FP2_C1_CONTINUE_2.md'
WATCHDOG_ORDER_SHA = 'f65f2f302302f8f546ffcc23bb84a9e2ababa40defd9995e9779be456a94eda3'
RUNS = {'N1': (1, 'Serial'), 'N6': (6, 'MPI'), 'N6P': (6, 'Production1')}
LINE = re.compile(c1.LINE.pattern.replace('core|cells', 'core|cells|collapse_probe')
                  .replace('FP2_A LAB_S_PROOF', 'FP2_A LAB_S_(?:PROOF|ENTITY)'))
COUNTERS = frozenset(('collapses_surface', 'c_sweeps', 'c_candidates', 'c_deferred',
                      'faces_before_c', 'points_before_c'))
REFUSALS = tuple('c_refused_' + reason for reason in (
    'protected', 'border', 'ineligible', 'border_keeper', 'feature_chord', 'link',
    'near', 'outward', 'reverse', 'topology', 'validity', 'size', 'shape', 'distance', 'entity',
))
REAL_FIELDS = ('c_reverse_max', 'seconds_c', 'seconds_c_samples', 'seconds_c_entity')
PROBE_ERRORS = ('distance', 'near', 'entity', 'size')


def session_root(repeat, watchdog_retry=False):
    root = HERE if repeat == 1 else HERE / 'repeat2'
    return root / 'watchdog_retry' if watchdog_retry else root


def case_path(build, name, repeat, watchdog_retry=False):
    return session_root(repeat, watchdog_retry) / 'cases' / f'build{build}' / name


def selector(name):
    return '[.][NativeFP2C2Lab' + RUNS[name][1] + ']'


def changes(surface):
    encoded = surface.get('c_changes', '""')
    if not re.fullmatch(r'"(?:[0-9]+(?:,[0-9]+)*)?"', encoded):
        raise ValueError(f'Invalid per-sweep collapse counts {encoded!r}')
    return [int(field) for field in encoded[1:-1].split(',')] if encoded != '""' else []


def limit(problems, rank, phase, key, value, maximum):
    if not 0 <= value <= maximum:
        problems.append({'rank': rank, 'pass': phase, 'field': key, 'observed': value, 'maximum': maximum})


def observed_failures(name, data):
    """Result gates that can already have a value when the helper watchdog ends."""
    failures = []
    last_pass = 1 if RUNS[name][0] == 1 else 2
    for rank, rows in data.items():
        for row in rows.get('pass', ()):
            limit(failures, rank, int(row['pass']), 'nonpositive', int(row['nonpositive']), 0)
        for row in rows.get('NATIVE3D_LOCAL_SURFACE', ()):
            for key, maximum in (('star_max', 1000), ('sweeps_distance', 39), ('v_limit', 0), ('c_sweeps', 49)):
                limit(failures, rank, int(row['pass']), key, int(row[key]), maximum)
        for row in rows.get('collapse_probe', ()):
            for key in PROBE_ERRORS:
                limit(failures, rank, int(row['pass']), f'collapse_probe_{key}', int(row[key]), 0)
        for row in rows.get('edge_bins', ()):
            if row['checkpoint'] == 'out' and int(row['pass']) == last_pass:
                limit(failures, rank, last_pass, 'lt_1e_2', int(row['lt_1e_2']), 0)
        for row in rows.get('end', ()):
            limit(failures, rank, last_pass, 'final_nonpositive', int(row['nonpositive']), 0)
        for row in rows.get('donor_scan', ()):
            limit(failures, rank, last_pass, 'donor_rejected', int(row['rejected']), 0)
        for row in (*rows.get('proof', ()), *rows.get('LAB_S_PROOF', ())):
            limit(failures, rank, last_pass, 'proof_unqualified', 1 - int(row['qualified']), 0)
            limit(failures, rank, last_pass, 'classify_unexplained', int(row['classify_unexplained']), 0)
        for row in rows.get('state', ()):
            limit(failures, rank, last_pass, 'owned_geometry_mismatch', 1 - int(row['owned_geometry_equal']), 0)
            if RUNS[name][0] > 1:
                limit(failures, rank, last_pass, 'snapshot_mismatch', 1 - int(row['snapshot_equal']), 0)
    return failures


def pass_timing(data, loadavg):
    passes = []
    for rank, rows in sorted(data.items()):
        for row in rows.get('pass', ()):
            seconds = float(row['seconds_pass'])
            passes.append({'rank': rank, 'pass': int(row['pass']), 'seconds_pass': seconds,
                           'threshold_exceeded': seconds > PASS_TIME_RULE})
    return {'threshold_seconds': PASS_TIME_RULE, 'loadavg_at_start': loadavg, 'passes': passes,
            'threshold_exceeded': any(row['threshold_exceeded'] for row in passes)}


def evaluate(name, data, expected, loadavg):
    ranks = RUNS[name][0]
    pass_rows = {rank: {**rows, 'edge_bins': [row for row in rows.get('edge_bins', ())
                                            if row['checkpoint'] == 'out']}
                 for rank, rows in data.items()}
    result = c3.prior.BASELINE_EVALUATE(name, pass_rows, expected)
    # C1 continuation 2 makes the per-pass timing threshold report-only for C2.
    result['failed_limits'] = observed_failures(name, data)
    result['pass_timing'] = pass_timing(data, loadavg)
    result['problems'].extend(c1.observations(name, data))
    c3.c2.donor_scan(data, ranks, result, True)
    phases = ['1'] if ranks == 1 else ['1', '2']
    for rank, rows in data.items():
        states = rows.get('state', ())
        if len(states) != 1 or states[0].get('rank') != str(rank) or not states[0].get('removals', '').isdigit():
            result['problems'].append({'row': 'state', 'rank': rank, 'observed': states})
        probes = rows.get('collapse_probe', ())
        if [probe['pass'] for probe in probes] != phases:
            result['problems'].append({'row': 'collapse_probe', 'rank': rank, 'expected_passes': phases})
        for probe in probes:
            if (probe['rank'] != str(rank) or not 0 <= int(probe['surviving']) <= int(probe['reported'])):
                result['problems'].append({'row': 'collapse_probe', 'rank': rank, 'observed': probe})
        for surface in rows.get('NATIVE3D_LOCAL_SURFACE', ()):
            phase = int(surface['pass'])
            try:
                sweep_counts = changes(surface)
                counts = {key: int(surface[key]) for key in (*COUNTERS, *REFUSALS)}
                before_faces, before_points = counts['faces_before_c'], counts['points_before_c']
                collapses = counts['collapses_surface']
                valid = (all(value >= 0 for value in counts.values()) and
                         all(math.isfinite(float(surface[key])) and float(surface[key]) >= 0 for key in REAL_FIELDS) and
                         len(sweep_counts) == counts['c_sweeps'] and sum(sweep_counts) == collapses and
                         before_faces == int(surface['faces_out']) + 2 * collapses and
                         before_points == int(surface['points_out']) + collapses)
            except (KeyError, ValueError):
                valid = False
            if not valid:
                result['problems'].append({'row': 'NATIVE3D_LOCAL_SURFACE', 'rank': rank, 'pass': phase,
                                           'observed': surface})
    return result


def verify_launches_terminal():
    for repeat in (1, 2):
        for retry in (False, True):
            root = session_root(repeat, retry)
            for launch in sorted((root / 'cases').glob('build*/*/test_launch.json')):
                wait = a.resumed(launch.parent)
                assert wait and wait['terminal'], launch
                processes = a.cut.read(launch.with_name('owned_processes.json'))
                assert processes['all_observed_terminal'], launch
                assert all(a.cut.terminal(row) for row in processes['identities']), launch


def watchdog_evidence(case, result):
    if result['supervisor_stop'] != 'wrapper_time_limit':
        return {}
    log_failures = [str(path) for path in lab.b0.log_paths(case, result['ranks']).values()
                    if re.search(r'^\s*FAILED:', path.read_text(errors='replace'), re.MULTILINE)]
    status = 'result_failed' if result['measurement']['failed_limits'] or log_failures else 'retry_allowed'
    command = a.cut.read(case / 'command.json')
    if status == 'retry_allowed' and command['seconds_limit'] == 3 * WATCHDOG:
        status = 'open'
    evidence = {'status': status, 'seconds_limit': command['seconds_limit'],
                'host_preflight': a.cut.read(case / 'test_host_preflight.json'),
                'measurement_sha256': a.sha(case / 'measurement.json'), 'result_failure_logs': log_failures}
    a.sealed(case / 'watchdog.json', evidence)
    return evidence


def verify_checkpoints(label, fingerprints):
    scoped = HERE / 'scoped' / f'build{label}'
    hidden = HERE / 'fixtures' / f'build{label}' / 'c2_acceptance.json'
    c3.prior.scoped_fixtures(scoped, fingerprints)
    receipt = a.cut.read(hidden)
    assert receipt['passed'] and all(receipt[key] == value for key, value in fingerprints.items()), hidden
    return scoped / 'fixtures.json', hidden


def run(build, name, repeat, verify_only, watchdog_retry):
    assert a.sha(ORDER) == ORDER_SHA
    assert ORDER.with_suffix('.sha256').read_text().split() == [ORDER_SHA, ORDER.name]
    assert a.sha(WATCHDOG_ORDER) == WATCHDOG_ORDER_SHA
    label = a.cut.read(build / 'build_status.json')['build']
    root = session_root(repeat, watchdog_retry)
    case = case_path(label, name, repeat, watchdog_retry)
    verify_launches_terminal()
    watchdog_inputs = ()
    if watchdog_retry:
        first = case_path(label, name, repeat)
        initial = a.cut.read(first / 'watchdog.json')
        assert initial['status'] == 'retry_allowed' and initial['seconds_limit'] == WATCHDOG
        assert a.sha(first / 'measurement.json') == initial['measurement_sha256']
        watchdog_inputs = first / 'watchdog.json', first / 'measurement.json'

    def prepare(directory):
        with patch.object(lab, 'HERE', c1.B):
            receipt, binary, pins = c3.prior.BASELINE_PREPARE(directory)
        fingerprints = {key: receipt[key] for key in lab.FINGERPRINTS}
        checkpoints = verify_checkpoints(label, fingerprints)
        support = (DRIVER, ORDER, ORDER.with_suffix('.sha256'), SOURCE_CHECK, WATCHDOG_ORDER,
                   c1.DRIVER, c3.DRIVER, c3.c2.DRIVER, c3.prior.DRIVER, c3.prior.OLD_LAB, *checkpoints,
                   *watchdog_inputs)
        pins.update({str(path): a.sha(path) for path in support})
        return receipt, binary, pins

    def measure(name, data, expected):
        preflight = a.cut.read(case / 'test_host_preflight.json')
        return evaluate(name, data, expected, preflight['loadavg'])

    with patch.multiple(lab, HERE=root, RUNS=RUNS, LINE=LINE, __file__=str(DRIVER), selector=selector,
                        SECONDS=WATCHDOG * (3 if watchdog_retry else 1), prepare=prepare, evaluate=measure):
        result = lab.run(build, name, verify_only)
    return result, watchdog_evidence(case, result)


def main():
    parser = ArgumentParser(description=__doc__)
    parser.add_argument('build', type=Path, nargs='?')
    method = parser.add_mutually_exclusive_group(required=True)
    method.add_argument('--only', choices=tuple(RUNS))
    method.add_argument('--owned', type=Path)
    parser.add_argument('--repeat', type=int, choices=(1, 2), default=1)
    parser.add_argument('--verify-only', action='store_true')
    parser.add_argument('--watchdog-retry', action='store_true')
    args = parser.parse_args()
    if args.owned:
        recipe = a.cut.read(args.owned / 'command.json')
        seconds = recipe['seconds_limit']
        assert seconds in (WATCHDOG, 3 * WATCHDOG)
        with patch.multiple(lab, RUNS=RUNS, selector=selector, SECONDS=seconds):
            return lab.owned(args.owned)
    if args.build is None:
        parser.error('a build directory is required with --only')
    result, watchdog = run(args.build.resolve(), args.only, args.repeat, args.verify_only, args.watchdog_retry)
    print(json.dumps({**{key: result[key] for key in ('case', 'passed', 'measurement')}, 'watchdog': watchdog}, indent=2))
    if watchdog.get('status') in ('retry_allowed', 'open'):
        return 75
    return 0 if result['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
