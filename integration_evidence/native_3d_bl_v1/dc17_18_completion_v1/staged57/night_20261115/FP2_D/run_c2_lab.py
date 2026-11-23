"""Run an FP2-D trial on the frozen hand-over input."""

from argparse import ArgumentParser
from importlib.util import module_from_spec, spec_from_file_location
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
C2 = HERE.parent / 'FP2_C2'
S0 = '240330d3aa'
S0_BUILD = 655
S0_LAB = C2 / 'cases/build655/lab_acceptance.json'
ORDER = HERE / 'inputs/FAST_PATH_STAGE2_D1_REV2.md'
ORDER_SHA = '69178513638b8741ae412cd689c50bc0b2396676c3395d867bb3ba868a27d8f1'
SOURCE_CHECK = HERE / 'source_check.md'
WATCHDOG = lab.SECONDS
PASS_TIME_RULE = 600
WATCHDOG_ORDER = C1 / 'inputs/FP2_C1_CONTINUE_2.md'
WATCHDOG_ORDER_SHA = 'f65f2f302302f8f546ffcc23bb84a9e2ababa40defd9995e9779be456a94eda3'
RUNS = {'N1': (1, 'Serial'), 'N6': (6, 'MPI'), 'N6P': (6, 'Production1'), 'N1Invariant': (1, 'Invariant')}
LINE = re.compile(c1.LINE.pattern.replace('core|cells',
                  'core|cells|collapse_probe|query_budget|strict_observation|entity_faces|bound_refusal')
                  .replace('FP2_A LAB_S_PROOF', 'FP2_A LAB_S_(?:PROOF|ENTITY)')
                  .replace('|NATIVE3D_LOCAL_SURFACE',
                           '|FP2_D (?:altitude|invariant|region|station)|FP2_C2 LAB_(?:ABOVE|MODEL_GAP|MISMATCH)|NATIVE3D_LOCAL_SURFACE'))
COUNTERS = frozenset(('collapses_surface', 'c_sweeps', 'c_candidates', 'c_deferred',
                      'faces_before_c', 'points_before_c', 'c_bound_queries'))
REFUSALS = tuple('c_refused_' + reason for reason in (
    'protected', 'border', 'ineligible', 'border_keeper', 'feature_chord', 'link',
    'near', 'outward', 'reverse', 'topology', 'validity', 'size', 'shape', 'distance', 'entity', 'bound',
))
REAL_FIELDS = ('c_reverse_max', 'seconds_c', 'seconds_c_samples', 'seconds_c_entity', 'seconds_c_bound')
PROBE_ERRORS = ('distance', 'near', 'entity', 'size')
PROOF_COUNTS = ('model_gap_entities', 'strict_checked', 'strict_mismatch', 'strict_sink_mismatch')
BUDGET_FIELDS = ('overlay_peak', 'pass_admitted_bytes', 'query_scratch_max', 'conservative_sum', 'ceiling')
D_FIELDS = ('d_error_edges', 'd_marks_error', 'd_marks_longest', 'd_centroid_only', 'd_nonfinite')
DIAGNOSTIC_ROWS = ('LAB_ABOVE', 'LAB_MODEL_GAP', 'LAB_MISMATCH')


def load_module(path, name):
    spec = spec_from_file_location(name, path)
    module = module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def reference_trials():
    c3.c2.committed(S0_LAB, S0)
    receipt = a.cut.read(S0_LAB)
    assert receipt['passed'] and receipt['build'] == S0_BUILD, S0_LAB
    old_runner = load_module(C2 / 'run_c2_lab.py', 'fp2_c2_runner')
    with patch.dict(sys.modules, {'run_c2_lab': old_runner}):
        old_comparator = load_module(C2 / 'compare_lab.py', 'fp2_c2_comparator')
    references = {}
    for name in ('N1', 'N6', 'N6P'):
        path, row = old_comparator.load_trial(S0_BUILD, name, 1)
        assert row['passed'] and receipt['artifacts'][str(path)] == a.sha(path), path
        references[name] = path, row
    return references


def session_root(repeat, watchdog_retry=False):
    root = HERE if repeat == 1 else HERE / 'repeat2'
    return root / 'watchdog_retry' if watchdog_retry else root


def case_path(build, name, repeat, watchdog_retry=False):
    return session_root(repeat, watchdog_retry) / 'cases' / f'build{build}' / name


def selector(name):
    return '[.][NativeFP2DLab' + RUNS[name][1] + ']'


def changes(surface):
    encoded = surface.get('c_changes', '""')
    if not re.fullmatch(r'"(?:[0-9]+(?:,[0-9]+)*)?"', encoded):
        raise ValueError(f'Invalid per-sweep collapse counts {encoded!r}')
    return [int(field) for field in encoded[1:-1].split(',')] if encoded != '""' else []


def limit(problems, rank, phase, key, value, maximum):
    if not 0 <= value <= maximum:
        problems.append({'rank': rank, 'pass': phase, 'field': key, 'observed': value, 'maximum': maximum})


def observed_failures(name, data, strict=False):
    """Result gates that can already have a value when the helper watchdog ends."""
    failures = []
    last_pass = 1 if RUNS[name][0] == 1 else 2
    for rank, rows in data.items():
        for row in rows.get('altitude', ()):
            if int(row['pass']) != last_pass:
                continue
            actual, resolution = float(row['altitude']), float(row['limit'])
            if not math.isfinite(actual) or not math.isfinite(resolution) or actual < resolution:
                failures.append({'rank': rank, 'pass': last_pass, 'field': 'altitude',
                                 'observed': row['altitude'], 'minimum': row['limit']})
        for row in rows.get('invariant', ()) if name == 'N1Invariant' else ():
            for field, value in (('violations', int(row['violations'])),
                                 ('probe_count_difference', abs(int(row['checked']) - int(row['splits_distance'])))):
                limit(failures, rank, int(row['pass']), field, value, 0)
        for row in rows.get('pass', ()):
            limit(failures, rank, int(row['pass']), 'nonpositive', int(row['nonpositive']), 0)
        for row in rows.get('NATIVE3D_LOCAL_SURFACE', ()):
            if all(key in row for key in D_FIELDS):
                for key in D_FIELDS:
                    if int(row[key]) < 0:
                        failures.append({'rank': rank, 'pass': int(row['pass']), 'field': key,
                                         'observed': row[key], 'minimum': 0})
                observed = int(row['d_marks_error']) + int(row['d_marks_longest'])
                if observed != int(row['splits_distance']):
                    failures.append({'rank': rank, 'pass': int(row['pass']), 'field': 'd_marks_sum',
                                     'observed': observed, 'expected': int(row['splits_distance'])})
            for key, maximum in (('star_max', 1000), ('sweeps_distance', 39), ('v_limit', 0), ('c_sweeps', 49)):
                limit(failures, rank, int(row['pass']), key, int(row[key]), maximum)
        for row in rows.get('collapse_probe', ()):
            for key in (*PROBE_ERRORS, 'strict_above') if strict and 'strict_above' in row else PROBE_ERRORS:
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
            if strict:
                for field in ('model_gap_entities', 'strict_sink_mismatch'):
                    if row.get(field, '').isdigit():
                        limit(failures, rank, last_pass, field, int(row[field]), 0)
                if row.get('strict_checked', '').isdigit():
                    observed = int(row['strict_checked'])
                    expected = int(rows['state'][0]['faces']) if rows.get('state') else int(row['classified'])
                    if observed != expected or observed != int(row['classified']):
                        failures.append({'rank': rank, 'pass': last_pass, 'field': 'strict_checked',
                                         'observed': observed, 'expected': expected, 'classified': int(row['classified'])})
        for row in rows.get('state', ()):
            limit(failures, rank, last_pass, 'owned_geometry_mismatch', 1 - int(row['owned_geometry_equal']), 0)
            if RUNS[name][0] > 1:
                limit(failures, rank, last_pass, 'snapshot_mismatch', 1 - int(row['snapshot_equal']), 0)
    return failures


def record_continuation(result, data, ranks):
    proof = result['proof']
    counts_valid = all(proof.get(field, '').isdigit() for field in PROOF_COUNTS)
    if (not counts_valid or
            not 0 <= int(proof['strict_sink_mismatch']) <= int(proof['strict_mismatch']) <= int(proof['strict_checked'])):
        result['problems'].append({'row': 'proof continuation fields', 'observed': proof})

    observations = []
    for rank, rows in data.items():
        per_rank = rows.get('strict_observation', ())
        if len(per_rank) != 1 or per_rank[0].get('rank') != str(rank) or not per_rank[0].get('checked', '').isdigit():
            result['problems'].append({'row': 'strict_observation', 'rank': rank, 'observed': per_rank})
            continue
        row = per_rank[0]
        try:
            seconds = float(row['seconds'])
            assert math.isfinite(seconds) and seconds >= 0
        except (AssertionError, KeyError, ValueError):
            result['problems'].append({'row': 'strict_observation', 'rank': rank, 'observed': row})
            continue
        observations.append({'rank': rank, 'checked': int(row['checked']), 'seconds': seconds})
    if counts_valid and len(observations) == ranks:
        checked = sum(row['checked'] for row in observations)
        if checked != int(proof['strict_checked']):
            result['failed_limits'].append({'field': 'strict_checked_rank_sum', 'observed': checked,
                                            'expected': int(proof['strict_checked'])})
    result['strict_observation'] = observations
    result['diagnostics'] = {kind: [row for rows in data.values() for row in rows.get(kind, ())]
                             for kind in DIAGNOSTIC_ROWS}
    for kind in ('entity_faces', 'bound_refusal'):
        result[kind] = [row for rows in data.values() for row in rows.get(kind, ())]


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
                                            if row['checkpoint'] == 'out'],
                        'region': [row for row in rows.get('region', ()) if 'bin' not in row]}
                 for rank, rows in data.items()}
    base_name = 'N1' if name == 'N1Invariant' else name
    result = c3.prior.BASELINE_EVALUATE(base_name, pass_rows, expected)
    # C1 continuation 2 makes the per-pass timing threshold report-only for C2.
    result['failed_limits'] = observed_failures(name, data, strict=True)
    result['pass_timing'] = pass_timing(data, loadavg)
    result['problems'].extend(c1.observations(base_name, data))
    c3.c2.donor_scan(data, ranks, result, True)
    phases = ['1'] if ranks == 1 else ['1', '2']
    for rank, rows in data.items():
        for kind in ('altitude', 'invariant') if name == 'N1Invariant' else ('altitude',):
            observed = rows.get(kind, ())
            if [row.get('pass') for row in observed] != phases or any(row.get('rank') != str(rank) for row in observed):
                result['problems'].append({'row': kind, 'rank': rank, 'expected_passes': phases})
        states = rows.get('state', ())
        if len(states) != 1 or states[0].get('rank') != str(rank) or not states[0].get('removals', '').isdigit():
            result['problems'].append({'row': 'state', 'rank': rank, 'observed': states})
        probes = rows.get('collapse_probe', ())
        if [probe['pass'] for probe in probes] != phases:
            result['problems'].append({'row': 'collapse_probe', 'rank': rank, 'expected_passes': phases})
        for probe in probes:
            if (probe['rank'] != str(rank) or not probe.get('strict_above', '').isdigit()
                    or not 0 <= int(probe['strict_above']) <= int(probe['surviving']) <= int(probe['reported'])):
                result['problems'].append({'row': 'collapse_probe', 'rank': rank, 'observed': probe})
        budgets = rows.get('query_budget', ())
        if [row.get('pass') for row in budgets] != phases:
            result['problems'].append({'row': 'query_budget', 'rank': rank, 'expected_passes': phases})
        for budget in budgets:
            if (budget.get('rank') != str(rank)
                    or not all(budget.get(field, '').isdigit() for field in BUDGET_FIELDS)
                    or int(budget['conservative_sum']) != int(budget['pass_admitted_bytes']) + int(budget['query_scratch_max'])):
                result['problems'].append({'row': 'query_budget', 'rank': rank, 'observed': budget})
        for surface in rows.get('NATIVE3D_LOCAL_SURFACE', ()):
            phase = int(surface['pass'])
            try:
                sweep_counts = changes(surface)
                counts = {key: int(surface[key]) for key in (*COUNTERS, *REFUSALS, *D_FIELDS)}
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
    record_continuation(result, data, ranks)
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
                    if re.search(r'(?:^\s*FAILED:|: FAILED:\s*$)', a.ANSI.sub('', path.read_text(errors='replace')),
                                 re.MULTILINE)]
    peak_exceeded = result['peak_lower_bound_bytes'] > a.MEMORY * 1024
    status = 'result_failed' if result['measurement']['failed_limits'] or log_failures or peak_exceeded else 'retry_allowed'
    command = a.cut.read(case / 'command.json')
    if status == 'retry_allowed' and command['seconds_limit'] == 3 * WATCHDOG:
        status = 'open'
    evidence = {'status': status, 'seconds_limit': command['seconds_limit'],
                'host_preflight': a.cut.read(case / 'test_host_preflight.json'),
                'measurement_sha256': a.sha(case / 'measurement.json'), 'result_failure_logs': log_failures,
                'job_peak_exceeded': peak_exceeded}
    a.sealed(case / 'watchdog.json', evidence)
    return evidence


def verify_checkpoints(label, fingerprints):
    fixture = HERE / 'fixtures' / f'build{label}' / 'd_acceptance.json'
    receipt = a.cut.read(fixture)
    assert receipt['passed'] and receipt['G1'] == [375, 114, 228] and receipt['fp1_cohorts'] == 18
    assert all(receipt[key] == value for key, value in fingerprints.items()), fixture
    for path, digest in receipt['artifacts'].items():
        assert a.sha(Path(path)) == digest, path
    return fixture, *(Path(path) for path in receipt['artifacts'])

def run(build, name, repeat, verify_only, watchdog_retry):
    assert a.sha(ORDER) == ORDER_SHA
    assert ORDER.with_suffix('.sha256').read_text().split() == [ORDER_SHA, ORDER.name]
    assert a.sha(WATCHDOG_ORDER) == WATCHDOG_ORDER_SHA
    reference = reference_trials()['N1' if name == 'N1Invariant' else name][0]
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
        support = (DRIVER, ORDER, ORDER.with_suffix('.sha256'), WATCHDOG_ORDER, SOURCE_CHECK, S0_LAB, reference,
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
