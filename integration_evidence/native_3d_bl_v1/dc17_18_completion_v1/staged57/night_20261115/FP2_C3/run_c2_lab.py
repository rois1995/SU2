"""Run an FP2-C3 trial on the frozen hand-over input."""

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
D = HERE.parent / 'FP2_D'
S0 = '0c6860fd3a'
S0_BUILD = 660
S0_LAB = D / 'cases/build660/lab_acceptance.json'
ORDER = HERE / 'inputs/FAST_PATH_STAGE2_C3.md'
ORDER_SHA = '87b804b6f54de048cb1c05f26dc87ab189096b6e6761526d86a91687849d511f'
SOURCE_CHECK = HERE / 'source_check.md'
CONTINUATION = HERE / 'inputs/FP2_C3_CONTINUE_1.md'
CONTINUATION_SHA = 'f5d5c6ac5bc2e37a06bc53d0318ac4e86c734826ef52bd5c89061dbfb6184bb8'
CONTINUATION_CHECK = HERE / 'continuation1_source_check.md'
BASE_663 = 'ae9ef407fa'
BUILD_663 = 663
RECEIPT_663 = HERE / 'reports/FP2_C3_lab_663_partial.json'
NOT_AVAILABLE = 'not available'
C3_METRIC_FIELDS = ('c_refused_metric', 'short_after_l', 'short_after_d', 'short_after_c')
OBSERVERS = ('BOUND_OBSERVER', 'BOUND_BIN', 'BOUND_ENTITY', 'BOUND_SLOW',
             'REFUSED_OBSERVER', 'REFUSED_FACE', 'REFUSED_REGION')
CONTACT = 'NATIVE3D_LOCAL_CONTACT'
CONTACT_FAILURE = 'Native local surface contact'

WATCHDOG = lab.SECONDS
PASS_TIME_RULE = 600
WATCHDOG_ORDER = C1 / 'inputs/FP2_C1_CONTINUE_2.md'
WATCHDOG_ORDER_SHA = 'f65f2f302302f8f546ffcc23bb84a9e2ababa40defd9995e9779be456a94eda3'
RUNS = {'N6P': (6, 'Production1'), 'N1': (1, 'Serial'), 'N6': (6, 'MPI')}
LINE = re.compile(c1.LINE.pattern.replace('core|cells',
                  'core|cells|collapse_probe|query_budget|strict_observation|entity_faces|bound_refusal')
                  .replace('FP2_A LAB_S_PROOF', 'FP2_A LAB_S_(?:PROOF|ENTITY)')
                  .replace('|NATIVE3D_LOCAL_SURFACE',
                           '|FP2_D (?:altitude|invariant|region|station|SHORT_FACE|REFUSED_OBSERVER|REFUSED_FACE|REFUSED_REGION)'
                           '|FP2_C3 (?:embedding|BOUND_[A-Z_]+)|NATIVE3D_LOCAL_CONTACT'
                           '|FP2_C2 LAB_(?:ABOVE|MODEL_GAP|MISMATCH)|NATIVE3D_LOCAL_SURFACE'))
COUNTERS = frozenset(('collapses_surface', 'c_sweeps', 'c_candidates', 'c_deferred',
                      'faces_before_c', 'points_before_c', 'c_bound_queries'))
REFUSALS = tuple('c_refused_' + reason for reason in (
    'protected', 'border', 'ineligible', 'border_keeper', 'feature_chord', 'link',
    'near', 'outward', 'reverse', 'topology', 'validity', 'size', 'shape', 'distance', 'entity', 'bound', 'contact_remote', 'contact',
))
CONTACT_COUNTERS = ('c_outward_tried', 'collapses_outward', 'c_contact_directions', 'c_contact_candidates',
                    'c_contact_calls', 'c_contact_rebuilds', 'c_contact_fresh_max')
CONTACT_CLOCKS = ('seconds_c_contact', 'seconds_c_contact_build')
REAL_FIELDS = ('c_reverse_max', 'seconds_c', 'seconds_c_samples', 'seconds_c_entity', 'seconds_c_bound', *CONTACT_CLOCKS)
CONTACT_COUNTS = {'screen': ('remote_rows', 'remote_bytes', 'peak_bytes'),
                  'verify': ('owner_rows', 'owner_bytes', 'peak_bytes', 'verify_cells', 'verify_candidates',
                             'verify_calls')}
CONTACT_TIMES = {'screen': ('seconds_gather',), 'verify': ('seconds_gather', 'seconds_verify')}
EMBEDDING_COUNTS = ('violations', 'pairs', 'calls', 'peak_bytes')
PROBE_ERRORS = ('distance', 'near', 'entity', 'size')
PROOF_COUNTS = ('model_gap_entities', 'strict_checked', 'strict_mismatch', 'strict_sink_mismatch')
BUDGET_FIELDS = ('overlay_peak', 'pass_admitted_bytes', 'query_scratch_max', 'conservative_sum', 'ceiling')
METRIC_D_FIELDS = ('d_short_metric', 'd_refused_metric')
D_FIELDS = ('d_error_edges', 'd_marks_error', 'd_marks_longest', 'd_centroid_only', 'd_nonfinite', *METRIC_D_FIELDS)
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
    assert receipt['alarm']['passed'] and receipt['alarm']['status'] == 'passed', S0_LAB
    assert receipt['invariant']['passed'] and set(receipt['cases']) == set(RUNS), S0_LAB
    old_runner = load_module(D / 'run_c2_lab.py', 'fp2_d_runner')
    with patch.dict(sys.modules, {'run_c2_lab': old_runner}):
        old_comparator = load_module(D / 'compare_lab.py', 'fp2_d_comparator')

    def load(name):
        path, row = old_comparator.load_trial(S0_BUILD, name, 1)
        assert row['passed'] and receipt['artifacts'][str(path)] == a.sha(path), path
        assert all(row[key] == receipt[key] for key in lab.FINGERPRINTS), path
        return path, row

    invariant_path, _ = load('N1Invariant')
    assert receipt['invariant']['receipt'] == str(invariant_path), S0_LAB
    references = {}
    for name in RUNS:
        path, row = load(name)
        record = receipt['cases'][name]
        assert record['passed']
        comparison = Path(record['receipt'])
        assert receipt['artifacts'][str(comparison)] == a.sha(comparison), comparison
        qualified = a.cut.read(comparison)
        assert qualified['passed'] and qualified['case'] == name, comparison
        assert qualified['artifacts'][str(path)] == a.sha(path), path
        references[name] = path, row
    return references


def continuation_sources():
    checksum = CONTINUATION.with_suffix('.sha256')
    assert a.sha(CONTINUATION) == CONTINUATION_SHA, CONTINUATION
    assert checksum.read_text().split() == [CONTINUATION_SHA, CONTINUATION.name], checksum
    assert CONTINUATION_CHECK.is_file(), CONTINUATION_CHECK
    return CONTINUATION, checksum, CONTINUATION_CHECK


def history_663_sources():
    fixture = HERE / 'fixtures/build663/c3_acceptance.json'
    build = HERE / 'build663/build_status.json'
    pins = HERE / 'build663/source_pins.json'
    for path in (RECEIPT_663, fixture, build, pins):
        c3.c2.committed(path, BASE_663)
    receipt = a.cut.read(RECEIPT_663)
    assert receipt['build'] == BUILD_663 and receipt['passed'] is False, RECEIPT_663
    assert receipt['first_difference'] == {'rank': 0, 'pass': 2, 'field': 'lt_1e_2',
                                          'observed': 1, 'maximum': 0}, RECEIPT_663
    assert receipt['cases']['N6P']['trial_passed'] == {'repeat1': False, 'repeat2': None}
    assert all(receipt['cases'][name]['passed'] for name in ('N1', 'N6'))
    built, qualified = a.cut.read(build), a.cut.read(fixture)
    assert built['passed'] and qualified['passed'] and built['source_pins_sha256'] == a.sha(pins)
    assert all(built[key] == qualified[key] for key in lab.FINGERPRINTS)
    for path, digest in receipt['artifacts'].items():
        assert a.sha(Path(path)) == digest, path
    return (RECEIPT_663, fixture, build, pins, *(Path(path) for path in receipt['artifacts']))


def first_short_checkpoint(checkpoints):
    """Return the first observed positive count in ordered (checkpoint, count) data."""
    observed = False
    for name, value in checkpoints:
        if re.fullmatch(r'[0-9]+', str(value)):
            observed = True
            if int(value) > 0:
                return name
    return 'none observed' if observed else NOT_AVAILABLE


def session_root(repeat, watchdog_retry=False):
    root = HERE if repeat == 1 else HERE / 'repeat2'
    return root / 'watchdog_retry' if watchdog_retry else root


def case_path(build, name, repeat, watchdog_retry=False):
    return session_root(repeat, watchdog_retry) / 'cases' / f'build{build}' / name


def selector(name):
    return '[.][NativeFP2C3Lab' + RUNS[name][1] + ']'


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
        for row in rows.get('embedding', ()):
            if row.get('violations', '').isdigit():
                limit(failures, rank, int(row['pass']), 'embedding_violations', int(row['violations']), 0)
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


def nonnegative_numbers(row, counts, clocks):
    try:
        return (all(row[key].isdigit() for key in counts) and
                all(math.isfinite(float(row[key])) and float(row[key]) >= 0 for key in clocks))
    except (KeyError, ValueError):
        return False


def contact_problems(rank, rows, phases, ranks):
    kinds = ('verify',) if ranks == 1 else ('screen', 'verify')
    expected = [(phase, kind) for phase in phases for kind in kinds]
    contact = rows.get(CONTACT, ())
    problems = []
    actual = [(row.get('pass'), row.get('kind')) for row in contact]
    if sorted(actual, key=str) != sorted(expected):
        problems.append({'row': CONTACT, 'rank': rank, 'expected': expected, 'observed': actual})
    cells = {row.get('pass'): row.get('cells_out') for row in rows.get('pass', ())}
    for row in contact:
        kind = row.get('kind')
        if (kind not in CONTACT_COUNTS or row.get('rank') != str(rank) or not row.get('stage')
                or not nonnegative_numbers(row, CONTACT_COUNTS[kind], CONTACT_TIMES[kind])
                or (kind == 'verify' and row.get('verify_cells') != cells.get(row.get('pass')))):
            problems.append({'row': CONTACT, 'rank': rank, 'observed': row})
    embedding = rows.get('embedding', ())
    if [row.get('pass') for row in embedding] != phases:
        problems.append({'row': 'embedding', 'rank': rank, 'expected_passes': phases})
    for row in embedding:
        if row.get('rank') != str(rank) or not nonnegative_numbers(row, EMBEDDING_COUNTS, ('seconds',)):
            problems.append({'row': 'embedding', 'rank': rank, 'observed': row})
    return problems


def continuation_surface_problems(rank, rows, phases=None):
    problems = []
    if phases is not None:
        for phase in phases:
            if not any(row.get('pass') == phase for row in rows):
                for field in C3_METRIC_FIELDS:
                    problems.append({'rank': rank, 'pass': int(phase), 'field': field, 'observed': None})
    for row in rows:
        for field in C3_METRIC_FIELDS:
            raw = row.get(field)
            invalid = raw is None or not re.fullmatch(r'[0-9]+', raw)
            problem = {'rank': rank, 'pass': int(row['pass']), 'field': field, 'observed': raw}
            if invalid:
                problems.append(problem)
            elif field == 'c_refused_metric' and re.fullmatch(r'[0-9]+', row.get('c_refused_size', '')):
                maximum = int(row['c_refused_size'])
                if int(raw) > maximum:
                    problems.append({**problem, 'maximum': maximum})
    return problems


def evaluate(name, data, expected, loadavg):
    ranks = RUNS[name][0]
    pass_rows = {rank: {**rows, 'edge_bins': [row for row in rows.get('edge_bins', ())
                                            if row['checkpoint'] == 'out'],
                        'region': [row for row in rows.get('region', ()) if 'bin' not in row]}
                 for rank, rows in data.items()}
    result = c3.prior.BASELINE_EVALUATE(name, pass_rows, expected)
    # Section 5 leaves the 600-second pass threshold as an observation.
    result['failed_limits'] = observed_failures(name, data, strict=True)
    result['pass_timing'] = pass_timing(data, loadavg)
    result['problems'].extend(c1.observations(name, data))
    c3.c2.donor_scan(data, ranks, result, True)
    phases = ['1'] if ranks == 1 else ['1', '2']
    for rank, rows in data.items():
        altitude = rows.get('altitude', ())
        if [row.get('pass') for row in altitude] != phases or any(row.get('rank') != str(rank) for row in altitude):
            result['problems'].append({'row': 'altitude', 'rank': rank, 'expected_passes': phases})
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
        surfaces = rows.get('NATIVE3D_LOCAL_SURFACE', ())
        result['problems'].extend(continuation_surface_problems(rank, surfaces, phases))
        for surface in surfaces:
            phase = int(surface['pass'])
            try:
                sweep_counts = changes(surface)
                counts = {key: int(surface[key]) for key in (*COUNTERS, *REFUSALS, *D_FIELDS, *CONTACT_COUNTERS)}
                before_faces, before_points = counts['faces_before_c'], counts['points_before_c']
                collapses = counts['collapses_surface']
                valid = (all(value >= 0 for value in counts.values()) and
                         counts['c_refused_outward'] == 0 and counts['collapses_outward'] <= collapses and
                         all(math.isfinite(float(surface[key])) and float(surface[key]) >= 0 for key in REAL_FIELDS) and
                         len(sweep_counts) == counts['c_sweeps'] and sum(sweep_counts) == collapses and
                         before_faces == int(surface['faces_out']) + 2 * collapses and
                         before_points == int(surface['points_out']) + collapses)
            except (KeyError, ValueError):
                valid = False
            if not valid:
                result['problems'].append({'row': 'NATIVE3D_LOCAL_SURFACE', 'rank': rank, 'pass': phase,
                                           'observed': surface})
        result['problems'].extend(contact_problems(rank, rows, phases, ranks))
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


def failure_logs(paths, catch=False):
    matches = []
    for path in paths:
        if not path.is_file():
            continue
        with lab.archived.open_log(path) as stream:
            for line in stream:
                cleaned = a.ANSI.sub('', line)
                if CONTACT_FAILURE in cleaned or (catch and re.search(r'^\s*FAILED:|: FAILED:\s*$', cleaned)):
                    matches.append(str(path))
                    break
    return matches


def lab_logs(case, ranks):
    for stdout in lab.b0.log_paths(case, ranks).values():
        yield stdout
        yield stdout.with_name('stderr')


def watchdog_evidence(case, result):
    if result['supervisor_stop'] != 'wrapper_time_limit':
        return {}
    log_failures = failure_logs(lab_logs(case, result['ranks']), catch=True)
    peak_exceeded = result['peak_lower_bound_bytes'] > a.MEMORY * 1024
    fields_failed = [item for rank, rows in result['rows_by_rank'].items()
                     for item in continuation_surface_problems(int(rank), rows.get('NATIVE3D_LOCAL_SURFACE', ()))]
    status = ('result_failed' if result['measurement']['failed_limits'] or log_failures or peak_exceeded
              or fields_failed else 'retry_allowed')
    command = a.cut.read(case / 'command.json')
    if status == 'retry_allowed' and command['seconds_limit'] == 3 * WATCHDOG:
        status = 'open'
    evidence = {'status': status, 'seconds_limit': command['seconds_limit'],
                'host_preflight': a.cut.read(case / 'test_host_preflight.json'),
                'measurement_sha256': a.sha(case / 'measurement.json'), 'result_failure_logs': log_failures,
                'job_peak_exceeded': peak_exceeded, 'continuation_fields_failed': fields_failed}
    a.sealed(case / 'watchdog.json', evidence)
    return evidence


def verify_checkpoints(label, fingerprints):
    fixture = HERE / 'fixtures' / f'build{label}' / 'c3_acceptance.json'
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
    reference = reference_trials()[name][0]
    continuation = continuation_sources()
    history = history_663_sources()
    label = a.cut.read(build / 'build_status.json')['build']
    assert label > BUILD_663, 'Continuation trials require a new build.'
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
                   D / 'run_c2_lab.py', D / 'compare_lab.py',
                   c1.DRIVER, c3.DRIVER, c3.c2.DRIVER, c3.prior.DRIVER, c3.prior.OLD_LAB, *checkpoints,
                   HERE / 'run_lab_campaign.py', HERE / 'compare_lab.py',
                   *continuation, *history, *watchdog_inputs)
        pins.update({str(path): a.sha(path) for path in support})
        return receipt, binary, pins

    def measure(name, data, expected):
        preflight = a.cut.read(case / 'test_host_preflight.json')
        result = evaluate(name, data, expected, preflight['loadavg'])
        for path in failure_logs(lab_logs(case, RUNS[name][0])):
            result['failed_limits'].append({'field': 'contact_failure', 'log': path})
        return result

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
