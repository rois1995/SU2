"""Run one FP2-M3 result gate or lab measurement in its own evidence folder."""

from argparse import ArgumentParser
import fcntl
import json
from pathlib import Path
import re
import runpy
import subprocess
import sys
from unittest.mock import patch

sys.dont_write_bytecode = True
HERE = Path(__file__).resolve().parent
M2 = HERE.parent / 'FP2_M2'
sys.path.insert(0, str(M2))
import run_m2 as m2

m1, lab, a, cut = m2.m1, m2.lab, m2.a, m2.cut
import run_g1 as g1

BASE = 'd9357fcb73'
BASE_BUILD = 667
AR = 'Common/src/adaptation/CNativeAffineReference.cpp'
DIFF = HERE / 'production_trace.diff'
ORDER = HERE / 'inputs/FP2_M3_GROUP_BREAKS.md'
ORDER_SHA = '50603b9671295ffc71ba92a3fa067c32be0d2eb3e678e35edd0bdcebec069ac0'
SOURCE_CHECK = HERE / 'source_check.md'
TRACE = m2.TRACE
FACETS = 'SU2_NATIVE_FP2_M3_FACETS'
EXPORT = 'facet_vertices.csv'
PROVENANCE = ('provenance.log.gz', 'provenance_receipt.json')
TRACE_FILES = (*m2.TRACE_FILES, *PROVENANCE)
LINE = re.compile(m2.LINE.pattern.replace('^(', '^(FP2_M3 FACETS|', 1))
TRIALS = ('TRACE-OFF', 'TRACE-ON')
FIXTURES = ('off-serial', 'off-mpi', 'curved')
G1 = ('G1', 'G1-serial', 'G1-mpi2', 'G1-mpi4')
TERMINAL_CODES = {'done': 0, 'not evaluated': 75, 'OPEN': 75, 'FAILED': 1}


def production_sources(build, candidate):
    root = cut.REPO
    plan = cut.read(build / 'build_plan.json')
    assert plan['base'] == BASE and candidate['build'] > BASE_BUILD
    assert plan['source_pins_sha256'] == candidate['source_pins_sha256']
    assert candidate['production_diff_sha256'] == plan['production_diff_sha256'] == a.sha(DIFF)
    files = subprocess.check_output(['git', 'diff', '--name-only', BASE, '--', 'Common', 'SU2_CFD'],
                                    cwd=root, text=True).splitlines()
    assert files == [AR], files
    actual = subprocess.check_output(['git', 'diff', '--no-ext-diff', BASE, '--', AR], cwd=root)
    assert actual and actual == DIFF.read_bytes(), DIFF
    old = M2 / 'build667/source_pins.json'
    old_status = old.with_name('build_status.json')
    for path in (old, old_status):
        m1.inherited.c3.c2.committed(path, BASE)
    baseline = cut.read(old_status)
    assert baseline['source_pins_sha256'] == a.sha(old)
    assert all(candidate[key] != baseline[key] for key in ('binary_sha256', 'test_binary_sha256'))
    manifest = cut.read(build / 'source_pins.json')
    for path, digest in cut.read(old).items():
        source = Path(path)
        if source.is_relative_to(root) and source.relative_to(root).parts[0] in ('Common', 'SU2_CFD'):
            if source != root / AR:
                assert manifest.get(path) == digest, source
    assert manifest[str(root / AR)] == a.sha(root / AR)
    return DIFF, old, old_status, build / 'build_plan.json'


def prepare(build):
    with patch.object(lab, 'HERE', m1.inherited.c1.B):
        receipt, binary, pins = m1.inherited.c3.prior.BASELINE_PREPARE(build)
    checksum = ORDER.with_suffix('.sha256')
    assert a.sha(ORDER) == ORDER_SHA and checksum.read_text().split() == [ORDER_SHA, ORDER.name]
    old = production_sources(build, receipt)
    sources = (ORDER, checksum, SOURCE_CHECK, Path(__file__), *old, M2 / 'run_m2.py',
               M2 / 'collect_trace.py', m1.HERE / 'run_m1.py', m1.C3 / 'compare_lab.py',
               m1.C3 / 'run_c2_lab.py')
    pins.update({str(path): a.sha(path) for path in sources})
    return receipt, binary, pins


def environment(trace, facet_export=False, tolerance=False):
    return {TRACE: str(int(trace)), FACETS: str(int(facet_export)), m2.M2: str(int(tolerance)),
            f'{m2.M2}_TABLE': '0', m1.OPTION: '0'}


def launch_recipe(case, binary, config, sensor, ranks, surface, mode):
    recipe = m2.LAB_COMMAND(case, binary, config, sensor, ranks, surface)
    index = recipe.index('nice')
    extra = environment(mode == 'TRACE-ON', facet_export=mode == 'TRACE-OFF', tolerance=True)
    return recipe[:index] + [f'{key}={value}' for key, value in extra.items()] + recipe[index:]


def base_dir(label, mode):
    branch = 'trials' if mode in TRIALS else 'fixtures'
    return HERE / branch / f'build{label}' / mode


def summary_path(label, mode, correction=False):
    return base_dir(label, mode) / ('r5_retry' if correction else '') / 'case_summary.json'


def pin_correction(correction, mode):
    if correction:
        record = cut.read(correction)
        assert record['mode'] == mode and record['status'] == 'FAILED' and record.get('correction_of') is None
        return {str(correction): a.sha(correction)}
    return {}


def run_g1(build, mode, correction):
    receipt, _, pins = prepare(build)
    pins.update(pin_correction(correction, mode))
    root = summary_path(receipt['build'], mode, bool(correction)).parent
    root.mkdir(parents=True, exist_ok=True)
    adopted = cut.read(g1.MANIFEST)
    selection = cut.read(g1.SELECTION)
    identity = runpy.run_path(str(g1.PARSER))
    identity['self_check']()
    launch = g1.run_case
    env = environment(False)
    cohorts = ('serial', 'mpi2', 'mpi4') if mode == 'G1' else (mode.removeprefix('G1-'),)
    expected_counts = {'serial': 375, 'mpi2': 114, 'mpi4': 228}
    results = {}

    def with_trace_off(case, binary, ranks, selector, seconds, **kwargs):
        wait = a.resumed(case)
        return wait or launch(case, binary, ranks, selector, seconds, environment=env, **kwargs)

    for cohort in cohorts:
        case = base_dir(receipt['build'], 'G1') / ('r5_retry' if correction else '') / cohort
        with patch.object(g1, 'run_case', with_trace_off):
            try:
                g1.run_cohort(case, cohort, receipt, build / 'source_pins.json', selection,
                              adopted['cohorts'][cohort], identity)
            except RuntimeError:
                if not (case / 'comparison.json').is_file():
                    raise
        comparison = case / 'comparison.json'
        projected = cut.read(comparison)
        passed = projected['identity_pass'] and projected['records'] == expected_counts[cohort]
        results[cohort] = {'status': 'done' if passed else 'FAILED', 'records': projected['records'],
                           'expected_records': expected_counts[cohort], 'used': str(comparison),
                           'comparison_sha256': a.sha(comparison), 'wait': cut.read(case / 'test_wait.json'),
                           'first_difference': next(iter(projected.get('unexpected_changes', ())), None)}
    status = 'done' if all(row['status'] == 'done' for row in results.values()) else 'FAILED'
    summary = {'build': receipt['build'], 'mode': mode, 'status': status, 'cohorts': results,
               'environment': env, 'pins': pins, 'correction_of': str(correction) if correction else None}
    path = root / 'case_summary.json'
    a.sealed(path, summary)
    return path, summary


def run_fixture(build, mode, correction):
    receipt, binary, pins = prepare(build)
    label = receipt['build']
    prior = m1.fixtures.hidden
    fingerprints = {key: receipt[key] for key in lab.FINGERPRINTS}
    prior.clear_options()
    pins.update(pin_correction(correction, mode))
    reference, old = m2.verified_old_fixture(mode)
    pins[str(reference)] = a.sha(reference)
    env = environment(mode == 'curved')
    target = summary_path(label, mode, bool(correction))
    spec = ((1, '[.][NativeFP2M1Support]', 1) if mode == 'curved'
            else m1.fixtures.FOCUSED[m1.OFF[mode]])
    if mode == 'curved':
        original = m1.HERE / 'check_fixture_trace.py'
        leaf_pin = HERE / 'check_leaf_pin.py'
        for helper in (original, leaf_pin):
            pins[str(helper)] = a.sha(helper)
    launch = prior.c1.run_case

    def launch_with_env(case, binary, ranks, selector, seconds):
        return launch(case, binary, ranks, selector, seconds, environment=env)

    with patch.object(prior.c1, 'run_case', launch_with_env):
        result, attempts = m1.fixtures.focus(target.parent, binary, fingerprints, pins, spec)
    case = Path(result['case'])
    unchanged = result['catch_summaries'] == old['result']['catch_summaries']
    inherited_passed = result['passed'] and unchanged
    details = {'environment': env, 'unchanged_assertion_counts': unchanged, 'catch_passed': bool(inherited_passed)}
    artifacts = {}
    status = 'done' if inherited_passed else 'FAILED'
    if mode == 'curved' and inherited_passed:
        log = case / 'test_outer.log'
        sidecar = case / 'fp2_m1_curved_strip.native_ref'
        artifacts = m2.seal_artifacts(case, (sidecar.name,))
        for helper, args, key in ((original, [log], 'trace'), (leaf_pin, [log, sidecar], 'leaf_pin')):
            check = subprocess.run([sys.executable, '-B', str(helper), *map(str, args)], capture_output=True, text=True)
            try:
                value = json.loads(check.stdout)
            except json.JSONDecodeError:
                value = None
            details[key] = {'returncode': check.returncode, 'stdout': check.stdout, 'stderr': check.stderr,
                            'result': value}
        if details['trace']['returncode'] or details['leaf_pin']['returncode'] or details['leaf_pin']['result'] is None:
            status = 'FAILED'
        elif details['leaf_pin']['result']['status'] == 'not evaluated':
            status = 'not evaluated'
        elif details['leaf_pin']['result']['status'] != 'passed':
            status = 'FAILED'
    output = {**fingerprints, 'mode': mode, 'status': status, 'used': str(case / 'result.json'),
              'attempts': {str(path): a.sha(path) for path in attempts},
              'details': details, 'extra_artifacts': artifacts,
              'correction_of': str(correction) if correction else None}
    a.sealed(target, output)
    return target, output


def verify_terminal():
    for launch in HERE.glob('trials/**/test_launch.json'):
        wait = a.resumed(launch.parent)
        assert wait and wait['terminal'], launch
        processes = cut.read(launch.with_name('owned_processes.json'))
        assert processes['all_observed_terminal'] and all(cut.terminal(row) for row in processes['identities']), launch


def attempt(build, mode, root, support):
    baseline, _ = m1.baseline('N1')
    label = cut.read(build / 'build_status.json')['build']
    case = m2.trial_case(label, root)
    verify_terminal()
    case.mkdir(parents=True, exist_ok=True)
    mode_path = case / 'm3_mode.json'
    a.sealed(mode_path, {'mode': mode, 'tolerance': '1E-4',
                        'environment': environment(mode == 'TRACE-ON', mode == 'TRACE-OFF', True)})
    observer_files = TRACE_FILES if mode == 'TRACE-ON' else (EXPORT,)

    def pinned(directory):
        receipt, binary, pins = prepare(directory)
        sources = (m1.BASE_PROOF, baseline, mode_path, *support)
        if mode == 'TRACE-ON':
            sources += (HERE / 'collect_provenance.py',)
        pins.update({str(path): a.sha(path) for path in sources})
        return receipt, binary, pins

    def evaluate(name, data, expected):
        host = cut.read(case / 'test_host_preflight.json')
        return m1.inherited.evaluate(name, data, expected, host['loadavg'])

    def command(*args):
        return launch_recipe(*args, mode)

    with patch.multiple(lab, HERE=root, RUNS=m2.RUNS, LINE=LINE, __file__=__file__, selector=m1.inherited.selector,
                        SECONDS=m2.TIMEOUT, prepare=pinned, evaluate=evaluate, command=command), \
            patch.object(lab.b0, 'input_data', lambda ranks: m2.derived_input('1E-4')), \
            patch.object(a, 'ARTIFACTS', (*a.ARTIFACTS, 'm3_mode.json', *observer_files)):
        result = lab.run(build, 'N1')
    verify_terminal()
    return case, result


def observed(case, mode, trial):
    if mode == 'TRACE-OFF':
        rows = trial['rows_by_rank'].get('0', {}).get('FACETS', ())
        artifacts = m2.seal_artifacts(case, (EXPORT,))
        complete = len(rows) == 1 and rows[0].get('status') == 'complete' and bool(artifacts)
        reason = None if complete else (
            rows[0].get('reason') if len(rows) == 1 and rows[0].get('reason') else
            'child interrupted' if trial['supervisor_stop'] else 'missing complete facet export')
        return {'completion': 'complete' if complete else 'incomplete', 'reported': list(rows),
                'reason': reason, 'artifacts': artifacts}
    artifacts = m2.seal_artifacts(case, TRACE_FILES)
    anatomy = cut.read(case / 'anatomy.json') if (case / 'anatomy.json').is_file() else {}
    receipt = cut.read(case / 'provenance_receipt.json') if (case / 'provenance_receipt.json').is_file() else {}
    complete = len(artifacts) == len(TRACE_FILES) and receipt.get('completion') == 'complete'
    completion = anatomy.get('completion', 'failed') if complete else 'failed'
    stream = anatomy.get('stream', {})
    if completion != 'failed':
        if trial['supervisor_stop']:
            completion = 'partial'
        elif not stream.get('forwards_started') or any(
            not anatomy.get('phases', {}).get(phase, {}).get('ended_segments')
            for phase in ('collapse_report', 'handover_strict', 'proof')
        ):
            completion = 'failed'
    return {'completion': completion, 'anatomy_completion': anatomy.get('completion'), 'provenance': receipt,
            'artifacts': artifacts, 'forwards_started': stream.get('forwards_started')}


def compare_identity(base, old, trial):
    before, after = old['rows_by_rank'], trial['rows_by_rank']
    result = {'status': 'not evaluated', 'baseline': str(base), 'baseline_sha256': a.sha(base)}
    if m2.projection_ready(before, after, trial):
        differences = m1.comparison.repeat_differences('N1', before, after)
        result.update(status='different' if differences else 'equal', differences=differences,
                      first_difference=m1.first_difference(differences, before, after))
    return result


def watchdog_result(case, trial, already_retried, identity):
    logs = m1.log_problems

    def seen(case, ranks):
        results = logs(case, ranks)
        if identity['status'] == 'different':
            results.append({'row': 'exact projection', **identity['first_difference']})
        return results

    with patch.object(m1, 'log_problems', seen):
        return m1.watchdog(case, trial, already_retried)


def run_trial(build, mode, correction):
    label = cut.read(build / 'build_status.json')['build']
    support = tuple(Path(path) for path in pin_correction(correction, mode))
    root = summary_path(label, mode, bool(correction)).parent
    first, trial = attempt(build, mode, root / 'attempt1', support)
    base, old = m1.baseline('N1')
    identity = compare_identity(base, old, trial)
    watchdog = watchdog_result(first, trial, False, identity)
    history, used = [first], first
    if watchdog.get('status') == 'retry_allowed':
        used, trial = attempt(build, mode, root / 'watchdog_retry',
                              [*support, first / 'watchdog.json', first / 'measurement.json'])
        identity = compare_identity(base, old, trial)
        watchdog = watchdog_result(used, trial, True, identity)
        history.append(used)
    observer = observed(used, mode, trial)
    first_problem = None
    if identity['status'] == 'different':
        first_problem = identity['first_difference']
        result_status = 'FAILED'
    elif not trial['passed']:
        first_problem = m2.first_finding(trial, watchdog)
        result_status = 'OPEN' if watchdog.get('status') == 'open' else 'FAILED'
    else:
        result_status = 'done'
    completion = observer['completion']
    if result_status == 'done' and completion != 'complete':
        first_problem = {'field': 'measurement completion', 'observed': completion, 'reason': observer.get('reason')}
    status = result_status
    if status == 'done' and completion != 'complete':
        status = 'FAILED'
    if completion == 'failed':
        status = 'FAILED'
    gates = trial['measurement']
    output = {'build': label, 'mode': mode, 'tolerance': '1E-4', 'status': status, 'result_status': result_status,
              'used': str(used / 'measurement.json'), 'identity': identity, 'observer': observer,
              'evaluate_qualified': not (gates['failed_limits'] or gates['problems'] or
                                        gates.get('first_round_difference')),
              'first_difference': first_problem, 'watchdog': watchdog,
              'correction_of': str(correction) if correction else None,
              'attempts': {str(case / 'measurement.json'): a.sha(case / 'measurement.json') for case in history}}
    target = root / 'case_summary.json'
    a.sealed(target, output)
    return target, output


def owned(case):
    mode = cut.read(case / 'm3_mode.json')['mode']
    recipe = cut.read(case / 'command.json')
    assert recipe['seconds_limit'] == m2.TIMEOUT

    def command(*args):
        return launch_recipe(*args, mode)

    with patch.multiple(lab, RUNS=m2.RUNS, SECONDS=m2.TIMEOUT, selector=m1.inherited.selector, command=command):
        if mode == 'TRACE-OFF':
            return lab.owned(case)
        import collect_provenance
        with patch.dict(sys.modules, collect_trace=collect_provenance):
            return m2.owned_anatomy(case, recipe['argv'])


def controller(build, mode, correction):
    with (HERE / '.owned.lock').open('a') as lock:
        fcntl.flock(lock, fcntl.LOCK_EX | fcntl.LOCK_NB)
        run = run_g1 if mode in G1 else run_fixture if mode in FIXTURES else run_trial
        path, result = run(build, mode, correction)
        print(f"{mode} {result['status']} {path}", flush=True)
        return TERMINAL_CODES[result['status']]


def launch(build, mode, correction):
    # Keep the controller and its process-group receipts alive when the interactive tool disconnects.
    with patch.multiple(m2, HERE=HERE, __file__=__file__):
        return m2.durable_launch(build, mode, correction)


def supervise(build, mode, correction, directory):
    with patch.object(m2, 'controller', controller):
        return m2.supervise(build, mode, correction, directory)


def self_check():
    _, ref = m1.baseline('N1')
    assert ref['passed']
    assert environment(False)[TRACE] == '0' and environment(True, False, True)[m1.OPTION] == '0'
    assert environment(False, True, True)[FACETS] == '1'
    assert LINE.fullmatch('FP2_M3 FACETS rank=0 status=complete facets=3')
    assert m2.TIMEOUT == m1.TIMEOUTS['N1'] == 3261
    print('FP2-M3 sealed N1 baseline, trace switches, facet parser and watchdog verified.')


def main():
    parser = ArgumentParser(description=__doc__)
    parser.add_argument('build', type=Path, nargs='?')
    targets = parser.add_mutually_exclusive_group(required=True)
    targets.add_argument('--only', choices=(*G1, *FIXTURES, *TRIALS))
    targets.add_argument('--owned', type=Path)
    targets.add_argument('--self-check', action='store_true')
    parser.add_argument('--correction-of', type=Path)
    parser.add_argument('--launch', action='store_true')
    parser.add_argument('--supervisor', type=Path)
    args = parser.parse_args()
    if args.owned:
        return owned(args.owned.resolve())
    if args.self_check:
        self_check()
        return 0
    if args.build is None:
        parser.error('a build directory is required')
    build, mode = args.build.resolve(), args.only
    correction = args.correction_of.resolve() if args.correction_of else None
    if args.launch:
        return launch(build, mode, correction)
    if args.supervisor:
        return supervise(build, mode, correction, args.supervisor.resolve())
    return controller(build, mode, correction)


if __name__ == '__main__':
    raise SystemExit(main())
