"""Run one test-only FP2-M2 fixture or serial lab case with separate attempt receipts."""

from argparse import ArgumentParser
import fcntl
import os
from pathlib import Path
import re
import subprocess
import sys
import threading
import time
from unittest.mock import patch

sys.dont_write_bytecode = True
HERE = Path(__file__).resolve().parent
M1 = HERE.parent / 'FP2_M1'
sys.path.insert(0, str(M1))
import run_m1 as m1

lab, a, cut = m1.lab, m1.a, m1.cut
BASE = '0957977801'
ORDER = HERE / 'inputs/FP2_M2_RULE_AND_PROOF.md'
ORDER_SHA = '6ea8a6633ec03d02acb9737b4de2893a1ad4bb0af8bc22727e8c3398584b2075'
SOURCE_CHECK = HERE / 'source_check.md'
TIMEOUT = 3261
TRIALS = {'BASELINE': '1E-4', 'ANATOMY': '1E-4', '2E-4': '2E-4', '5E-4': '5E-4', '1E-3': '1E-3'}
FIXTURES = ('off-serial', 'off-mpi', 'curved', 'model')
RUNS = {'N1': m1.CASES['N1']}
LINE = re.compile(m1.inherited.LINE.pattern.replace('^(', '^(FP2_M2 [A-Z_]+|', 1))
HAUSD = re.compile(rb'(?m)^([ \t]*ADAP_HAUSD[ \t]*=[ \t]*)1E-4(?=[ \t]*(?:%[^\r\n]*)?\r?$)')
TRACE_FILES = ('anatomy.json', 'anatomy_sample.log', 'anatomy_unresolved.log')
MODEL_FILES = ('model_edges.csv', 'model_facets.csv')
M2 = 'SU2_NATIVE_FP2_M2'
TRACE = m1.TRACE
LAB_COMMAND = lab.command
OLD_666 = M1 / 'fixtures/build666'
REFERENCE = 'SU2_NATIVE_FP2_M2_REFERENCE'
CEILING = 'SU2_NATIVE_FP2_M2_CEILING'


def order_sources():
    checksum = ORDER.with_suffix('.sha256')
    assert a.sha(ORDER) == ORDER_SHA
    assert checksum.read_text().split() == [ORDER_SHA, ORDER.name]
    assert SOURCE_CHECK.is_file(), SOURCE_CHECK
    return ORDER, checksum, SOURCE_CHECK


def prepare(build):
    with patch.object(lab, 'HERE', m1.inherited.c1.B):
        receipt, binary, pins = m1.inherited.c3.prior.BASELINE_PREPARE(build)
    production = m1.production_pins(build, receipt)
    sources = (*order_sources(), *production, Path(__file__), m1.HERE / 'run_m1.py',
               m1.C3 / 'run_c2_lab.py', m1.C3 / 'compare_lab.py')
    pins.update({str(path): a.sha(path) for path in sources})
    return receipt, binary, pins


def derived_input(tolerance, original=lab.b0.input_data):
    config, *rest = original(1)  # verifies every frozen input before the one assignment edit
    result, assignments = HAUSD.subn(lambda match: match[1] + tolerance.encode('ascii'), config)
    assert assignments == 1, 'Expected exactly one pinned ADAP_HAUSD= 1E-4 assignment.'
    return result, *rest


def table_files(mode):
    tolerance = TRIALS[mode]
    return tuple(f'handover_{kind}_{tolerance}.csv' for kind in ('faces', 'edges'))


def env_for(mode):
    return {M2: '1', f'{M2}_TABLE': str(int(mode != 'ANATOMY')),
            TRACE: str(int(mode == 'ANATOMY')), m1.OPTION: '0'}


def launch_recipe(case, binary, config, sensor, ranks, surface, mode):
    recipe = LAB_COMMAND(case, binary, config, sensor, ranks, surface)
    index = recipe.index('nice')
    values = [f'{key}={value}' for key, value in env_for(mode).items()]
    return recipe[:index] + values + recipe[index:]


def base_dir(label, mode):
    branch = 'fixtures' if mode in FIXTURES else 'trials'
    return HERE / branch / f'build{label}' / mode


def summary_path(label, mode, correction=False):
    return base_dir(label, mode) / ('r5_retry' if correction else '') / 'case_summary.json'


def read_rows(path):
    with patch.object(lab, 'LINE', LINE):
        return lab.parse_rows(path)


def seal_artifacts(path, files):
    return {str(path / name): a.sha(path / name) for name in files if (path / name).is_file()}


def verified_old_fixture(mode):
    key = 'fixture' if mode == 'curved' else mode
    path = OLD_666 / key / 'case_summary.json'
    m1.inherited.c3.c2.committed(path, BASE)
    saved = cut.read(path)
    assert saved['status'] == 'PASS'
    for filename, checksum in saved['attempts'].items():
        attempt = Path(filename)
        assert a.sha(attempt) == checksum
        lab.archived.verify_artifacts(attempt.parent, cut.read(attempt)['artifacts'])
    return path, saved


def model_inputs():
    config, _, copies, tracked, _ = lab.b0.input_data(1)
    ref, = [path for path in copies if path.name.endswith('.native_ref')]
    value, = re.findall(rb'(?m)^ADAP_NATIVE_MEMORY_LIMIT\s*=\s*([0-9]+)[ \t]*$', config)
    return ref.resolve(), int(value) << 20, tracked


def run_fixture(build, mode, correction):
    receipt, binary, pins = prepare(build)
    label = receipt['build']
    prior = m1.fixtures.hidden
    fingerprints = {key: receipt[key] for key in lab.FINGERPRINTS}
    prior.clear_options()
    environment = {M2: '0', f'{M2}_TABLE': '0', m1.OPTION: '0', TRACE: '0'}
    old = None
    if mode == 'model':
        spec = 1, '[.][NativeFP2M2Model]', 1
        reference, ceiling, tracked = model_inputs()
        environment.update({REFERENCE: str(reference), CEILING: str(ceiling)})
        pins.update(tracked)
    else:
        old_path, old = verified_old_fixture(mode)
        pins[str(old_path)] = a.sha(old_path)
        spec = ((1, '[.][NativeFP2M1Support]', 1) if mode == 'curved' else
                m1.fixtures.FOCUSED[m1.OFF[mode]])
        if mode == 'curved':
            environment[TRACE] = '1'
            checker = M1 / 'check_fixture_trace.py'
            pins[str(checker)] = a.sha(checker)
    target = summary_path(label, mode, correction is not None)
    if correction is not None:
        assert cut.read(correction)['mode'] == mode
        pins[str(correction)] = a.sha(correction)
    launch = prior.c1.run_case

    def launch_with_env(case, binary, ranks, selector, seconds):
        return launch(case, binary, ranks, selector, seconds, environment=environment)

    with patch.object(prior.c1, 'run_case', launch_with_env):
        result, attempts = m1.fixtures.focus(target.parent, binary, fingerprints, pins, spec)
    case = Path(result['case'])
    passed = result['passed']
    details, artifacts = {'environment': environment}, {}
    if old is not None:
        unchanged = result['catch_summaries'] == old['result']['catch_summaries']
        details['unchanged_assertion_counts'] = unchanged
        passed &= unchanged
    if mode == 'curved' and result['passed']:
        check = subprocess.run([sys.executable, '-B', str(checker), str(case / 'test_outer.log')],
                               capture_output=True, text=True, check=False)
        details.update(trace_returncode=check.returncode, trace_stdout=check.stdout, trace_stderr=check.stderr)
        passed &= check.returncode == 0
        artifacts = seal_artifacts(case, ('fp2_m1_curved_strip.native_ref',))
    if mode == 'model':
        rows = read_rows(case / 'test_outer.log').get('MODEL', ())
        artifacts = seal_artifacts(case, MODEL_FILES)
        details['model'] = rows
        passed &= len(rows) == 1 and rows[0].get('status') == 'complete' and len(artifacts) == len(MODEL_FILES)
    record = {**fingerprints, 'mode': mode, 'status': 'done' if passed else 'FAILED',
              'used': str(case / 'result.json'), 'attempts': {str(path): a.sha(path) for path in attempts},
              'details': details, 'extra_artifacts': artifacts,
              'correction_of': str(correction) if correction else None}
    a.sealed(target, record)
    return target, record


def trial_case(label, root):
    return root / 'cases' / f'build{label}' / 'N1'


def verify_terminal():
    for launch in HERE.glob('trials/**/test_launch.json'):
        wait = a.resumed(launch.parent)
        assert wait and wait['terminal'], launch
        processes = cut.read(launch.with_name('owned_processes.json'))
        assert processes['all_observed_terminal'] and all(cut.terminal(row) for row in processes['identities']), launch


def attempt(build, mode, root, support):
    baseline, _ = m1.baseline('N1')
    label = cut.read(build / 'build_status.json')['build']
    case = trial_case(label, root)
    case.mkdir(parents=True, exist_ok=True)
    mode_path = case / 'm2_mode.json'
    a.sealed(mode_path, {'mode': mode, 'tolerance': TRIALS[mode], 'environment': env_for(mode)})
    observer_files = TRACE_FILES if mode == 'ANATOMY' else table_files(mode)
    verify_terminal()

    def pinned(directory):
        receipt, binary, pins = prepare(directory)
        sources = (m1.BASE_PROOF, baseline, mode_path, *support)
        if mode == 'ANATOMY':
            sources += (HERE / 'collect_trace.py',)
        pins.update({str(path): a.sha(path) for path in sources})
        return receipt, binary, pins

    def evaluate(name, data, expected):
        return m1.inherited.evaluate(name, data, expected, cut.read(case / 'test_host_preflight.json')['loadavg'])

    def command(*args):
        return launch_recipe(*args, mode)

    with patch.multiple(lab, HERE=root, RUNS=RUNS, LINE=LINE, __file__=__file__, selector=m1.inherited.selector,
                        SECONDS=TIMEOUT, prepare=pinned, evaluate=evaluate, command=command), \
            patch.object(lab.b0, 'input_data', lambda ranks: derived_input(TRIALS[mode])), \
            patch.object(a, 'ARTIFACTS', (*a.ARTIFACTS, 'm2_mode.json', *observer_files)):
        result = lab.run(build, 'N1')
    verify_terminal()
    return case, result


def projection_ready(before, after, trial):
    if trial['wait']['returncode'] == 0 and trial['supervisor_stop'] is None:
        return True
    return before.keys() == after.keys() and all(
        len(before[rank].get(marker, ())) == len(after[rank].get(marker, ()))
        for rank in before for name in m1.comparison.ROW_NAMES if m1.comparison.applies('N1', name)
        for marker in (m1.comparison.compare_lab_c1.row_key(name),)
    )


def first_finding(trial, watchdog):
    measurement = trial['measurement']
    values = (measurement['failed_limits'] or watchdog.get('failure_logs') or
              watchdog.get('observed_field_failures') or measurement['problems'])
    return values[0] if values else {'supervisor_stop': trial['supervisor_stop'],
                                    'returncode': trial['wait']['returncode'],
                                    'peak_lower_bound_bytes': trial['peak_lower_bound_bytes']}


def trial_observations(case, mode, trial):
    interrupted = trial['supervisor_stop'] is not None
    rows = trial['rows_by_rank'].get('0', {})
    if mode == 'ANATOMY':
        evidence = seal_artifacts(case, TRACE_FILES)
        completion = 'failed'
        if (case / 'anatomy.json').is_file():
            completion = cut.read(case / 'anatomy.json')['completion']
            if completion != 'failed' and interrupted:
                completion = 'partial'
        if len(evidence) != len(TRACE_FILES):
            completion = 'failed'
        return {'completion': completion, 'artifacts': evidence}
    tables = rows.get('TABLE', ())
    evidence = seal_artifacts(case, table_files(mode))
    complete = len(tables) == 1 and tables[0].get('status') == 'complete' and len(evidence) == 2
    reason = None if complete else ('child interrupted' if interrupted else 'inherited trial failed'
                                   if not trial['passed'] and not tables else
                                   'observer reported incomplete' if tables and tables[0].get('status') == 'incomplete'
                                   else 'missing complete table receipt or CSV')
    return {'completion': 'complete' if complete else 'incomplete', 'reason': reason,
            'reported': list(tables), 'artifacts': evidence}


def run_trial(build, mode, correction):
    label = cut.read(build / 'build_status.json')['build']
    support = []
    root = base_dir(label, mode)
    if correction is not None:
        prior = cut.read(correction)
        assert prior['mode'] == mode and prior['status'] == 'FAILED' and prior['correction_of'] is None
        support.append(correction)
        root /= 'r5_retry'
    first, trial = attempt(build, mode, root / 'attempt1', support)
    wd = m1.watchdog(first, trial, False)
    history, used = [first], first
    if wd.get('status') == 'retry_allowed':
        used, trial = attempt(build, mode, root / 'watchdog_retry',
                              [*support, first / 'watchdog.json', first / 'measurement.json'])
        wd = m1.watchdog(used, trial, True)
        history.append(used)
    baseline, old = m1.baseline('N1')
    differences, first_problem = None, None
    identity = {'status': 'not evaluated', 'baseline': str(baseline), 'baseline_sha256': a.sha(baseline)}
    comparison_mode = mode in ('BASELINE', 'ANATOMY')
    before, after = old['rows_by_rank'], trial['rows_by_rank']
    if comparison_mode and projection_ready(before, after, trial):
        differences = m1.comparison.repeat_differences('N1', before, after)
        identity.update(status='different' if differences else 'equal', differences=differences,
                        first_difference=m1.first_difference(differences, before, after))
    observation = trial_observations(used, mode, trial)
    if not trial['passed']:
        first_problem = first_finding(trial, wd)
        status = 'finding' if not comparison_mode else ('OPEN' if wd.get('status') == 'open' else 'FAILED')
    elif differences:
        first_problem = identity['first_difference']
        status = 'FAILED'
    else:
        status = 'done'
    if mode == 'ANATOMY' and observation['completion'] == 'failed':
        first_problem = first_problem or {'field': 'anatomy completion', 'observed': 'failed'}
        status = 'FAILED'
    gates = trial['measurement']
    proof = gates.get('proof', {})
    if not isinstance(proof, dict):
        proof = {}
    record = {'build': label, 'mode': mode, 'tolerance': TRIALS[mode], 'status': status,
              'used': str(used / 'measurement.json'), 'identity': identity, 'observer': observation,
              'evaluate_qualified': not (gates['failed_limits'] or gates['problems'] or
                                        gates.get('first_round_difference')),
              'proof_qualified': proof.get('qualified'), 'proof_accepted': proof.get('accepted'),
              'first_difference': first_problem, 'watchdog': wd,
              'correction_of': str(correction) if correction else None,
              'attempts': {str(case / 'measurement.json'): a.sha(case / 'measurement.json') for case in history}}
    target = root / 'case_summary.json'
    a.sealed(target, record)
    return target, record


def owned(case):
    mode = cut.read(case / 'm2_mode.json')['mode']
    recipe = cut.read(case / 'command.json')
    assert recipe['seconds_limit'] == TIMEOUT

    def command(*args):
        return launch_recipe(*args, mode)

    with patch.multiple(lab, RUNS=RUNS, SECONDS=TIMEOUT, selector=m1.inherited.selector, command=command):
        if mode != 'ANATOMY':
            return lab.owned(case)
        return owned_anatomy(case, recipe['argv'])


def owned_anatomy(case, expected):
    from collect_trace import drain_stderr

    original = subprocess.Popen
    readers = []

    def drain(pipe):
        try:
            drain_stderr(pipe, sys.stderr, case)
        finally:
            pipe.close()

    def spawn(argv, *args, **kwargs):
        if argv != expected:
            return original(argv, *args, **kwargs)
        assert not readers
        child = original(argv, *args, stderr=subprocess.PIPE, **kwargs)
        reader = threading.Thread(target=drain, args=(child.stderr,), name='fp2-m2-trace')
        readers.append(reader)
        reader.start()
        return child

    try:
        with patch.object(subprocess, 'Popen', spawn):
            return lab.owned(case)
    finally:
        for reader in readers:
            reader.join()


def controller(build, mode, correction):
    with (HERE / '.owned.lock').open('a') as lock:
        fcntl.flock(lock, fcntl.LOCK_EX | fcntl.LOCK_NB)
        target, result = (run_fixture if mode in FIXTURES else run_trial)(build, mode, correction)
        print(f"{result['mode']} {result['status']} {target}", flush=True)
        return {'done': 0, 'finding': 0, 'OPEN': 75, 'FAILED': 1}[result['status']]


def durable_launch(build, mode, correction):
    label = cut.read(build / 'build_status.json')['build']
    directory = HERE / 'supervisors' / f'build{label}' / mode / str(time.time_ns())
    directory.mkdir(parents=True)
    unit = f'fp2-m2-{label}-{mode.lower()}-{directory.name}'
    payload = [sys.executable, '-B', __file__, str(build), '--only', mode, '--supervisor', str(directory)]
    if correction:
        payload += ['--correction-of', str(correction)]
    command = ['systemd-run', '--user', '--quiet', '--collect', f'--unit={unit}', '--property=Type=exec',
               f'--property=WorkingDirectory={cut.REPO}', f'--property=StandardOutput=append:{directory / "controller.log"}',
               '--property=StandardError=inherit', *payload]
    environment = os.environ.copy()
    runtime = Path(f'/run/user/{os.getuid()}')
    if (runtime / 'bus').exists():
        environment.setdefault('XDG_RUNTIME_DIR', str(runtime))
        environment.setdefault('DBUS_SESSION_BUS_ADDRESS', f'unix:path={runtime / "bus"}')
    a.sealed(directory / 'launch.json', {'unit': unit, 'command': command, 'mode': mode, 'build': label})
    result = subprocess.run(command, env=environment, text=True, capture_output=True, check=False)
    a.sealed(directory / 'submitted.json', {'returncode': result.returncode, 'stdout': result.stdout,
                                            'stderr': result.stderr})
    print(directory, flush=True)
    if result.returncode:
        print(result.stderr, file=sys.stderr, end='')
    return result.returncode


def supervise(build, mode, correction, directory):
    a.sealed(directory / 'started.json', cut.actual_identity(os.getpid()))
    code = 1
    try:
        code = controller(build, mode, correction)
        return code
    finally:
        a.sealed(directory / 'exit.json', {'returncode': code, 'ended_unix': time.time()})


def self_check():
    _, old = m1.baseline('N1')
    assert TIMEOUT == m1.TIMEOUTS['N1']
    config = b'BASE= x\nADAP_HAUSD= 1E-4\nOther= 1E-4\n'
    for mode, tolerance in TRIALS.items():
        expected = config.replace(b'HAUSD= 1E-4', b'HAUSD= ' + tolerance.encode())
        assert derived_input(tolerance, lambda ranks: (config, 'sensor')) == (expected, 'sensor')
        assert env_for(mode)[TRACE] == str(int(mode == 'ANATOMY'))
    assert LINE.fullmatch('FP2_M2 STAGE rank=0 pass=1 bin=0 direct_eligible=0')
    assert projection_ready(old['rows_by_rank'], old['rows_by_rank'],
                            {'wait': {'returncode': 1}, 'supervisor_stop': 'wrapper_time_limit'})
    assert not projection_ready(old['rows_by_rank'], {},
                                {'wait': {'returncode': 1}, 'supervisor_stop': 'wrapper_time_limit'})
    print('FP2-M2 baseline seal, config derivation and projection completion verified.')


def main():
    parser = ArgumentParser(description=__doc__)
    parser.add_argument('build', type=Path, nargs='?')
    target = parser.add_mutually_exclusive_group(required=True)
    target.add_argument('--only', choices=(*FIXTURES, *TRIALS))
    target.add_argument('--owned', type=Path)
    target.add_argument('--self-check', action='store_true')
    parser.add_argument('--correction-of', type=Path)
    parser.add_argument('--launch', action='store_true')
    parser.add_argument('--supervisor', type=Path)
    args = parser.parse_args()
    if args.owned:
        return owned(args.owned)
    if args.self_check:
        self_check()
        return 0
    if args.build is None:
        parser.error('a build directory is required')
    build, mode = args.build.resolve(), args.only
    correction = args.correction_of.resolve() if args.correction_of else None
    if args.launch:
        return durable_launch(build, mode, correction)
    if args.supervisor:
        return supervise(build, mode, correction, args.supervisor.resolve())
    return controller(build, mode, correction)


if __name__ == '__main__':
    raise SystemExit(main())
