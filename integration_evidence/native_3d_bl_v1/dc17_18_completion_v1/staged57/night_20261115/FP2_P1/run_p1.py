"""Run and seal FP2-P1 fixtures, lab trials and exact comparisons."""

from argparse import ArgumentParser
import fcntl
import json
from pathlib import Path
import subprocess
import sys
from unittest.mock import patch

sys.dont_write_bytecode = True
HERE = Path(__file__).resolve().parent
M3 = HERE.parent / 'FP2_M3'
sys.path.insert(0, str(M3))
import run_m3 as m3

m2, m1, lab, a, cut = m3.m2, m3.m1, m3.lab, m3.a, m3.cut
g1, inherited, comparison = m3.g1, m1.inherited, m1.comparison
BASE = '3228ca0e1e'
ORDER = HERE / 'inputs/FP2_P1_CONFORMING_WITNESS.md'
SWITCH = 'SU2_NATIVE_CONFORMING_WITNESS'
TRACE = m3.TRACE
MARKER = 'NATIVE_SURFACE_CONFORMING_WITNESS enabled=1'
TIMEOUTS = {'N6P': 2575, 'N1': 3261, 'N6': 4522}
G1 = m3.G1[1:]
INHERITED = m3.FIXTURES
HIDDEN = ('direct', 'public-off', 'public-on')
TRIALS = {'default-N1': ('default', 'N1', 1), 'anatomy-N1': ('anatomy', 'N1', 1)}
CAMPAIGN = tuple(f'campaign-{name}-{trial}' for name in TIMEOUTS for trial in (1, 2))
TRIALS.update({mode: ('campaign', mode.split('-')[1], int(mode[-1])) for mode in CAMPAIGN})
SHARED = ('first-collapse', 'exact-c492', 'merit481', 'saved-current', 'native2d')
STEP1 = (*G1, *INHERITED, 'default-N1', *SHARED, *HIDDEN)
TERMINAL = {'passed': 0, 'OPEN': 75, 'not evaluated': 75, 'FAILED': 1}


def fingerprint(row):
    return {key: row[key] for key in lab.FINGERPRINTS}


def hashes(paths):
    return {str(path): a.sha(path) for path in paths}


def verify_pins(pins):
    for path, digest in pins.items():
        assert a.sha(Path(path)) == digest, path


def prepare(build):
    import build_p1

    with patch.object(lab, 'HERE', inherited.c1.B):
        receipt, binary, pins = inherited.c3.prior.BASELINE_PREPARE(build)
    assert build == HERE / f"build{receipt['build']}"
    build_p1.verify_build(receipt['build'])
    assert (build / 'build_plan.json').is_file()
    pins.update(hashes((ORDER, ORDER.with_suffix('.sha256'), HERE / 'reports/source_check.md', build_p1.PATCH,
                        Path(__file__), Path(build_p1.__file__), build / 'build_plan.json',
                        M3 / 'run_m3.py', m2.HERE / 'run_m2.py', m1.HERE / 'run_m1.py',
                        inherited.DRIVER, m1.C3 / 'compare_lab.py')))
    return receipt, binary, pins


def baseline(name):
    with patch.object(m1, 'TIMEOUTS', TIMEOUTS):
        return m1.baseline(name)


def environment(closure, trace, lab_observer=False):
    return {**m3.environment(trace, tolerance=lab_observer), SWITCH: str(int(closure))}


def trial_environment(mode):
    return environment(mode != 'default-N1', mode == 'anatomy-N1', TRIALS[mode][1] == 'N1')


def mpi_exports(argv):
    if 'mpirun' in argv and SWITCH not in argv:
        index = argv.index('mpirun') + 1
        argv[index:index] = ['-x', SWITCH, '-x', TRACE]
    return argv


def launch_recipe(case, binary, config, sensor, ranks, surface, mode):
    recipe = m2.LAB_COMMAND(case, binary, config, sensor, ranks, surface)
    index = recipe.index('nice')
    recipe[index:index] = [f'{key}={value}' for key, value in trial_environment(mode).items()]
    return mpi_exports(recipe)


def switches(case, env, ranks, require_query, observations=None):
    command = cut.read(case / 'command.json')['argv']
    for key, value in env.items():
        assert [item for item in command if item.startswith(f'{key}=')] == [f'{key}={value}'], (case, key)
    if ranks > 1:
        for key in (SWITCH, TRACE):
            assert any(command[index:index + 2] == ['-x', key] for index in range(len(command))), case
    if ranks == 1:
        logs = {0: case / 'test_outer.log'}
    else:
        logs = {int(path.parent.name.rsplit('.', 1)[1]): path
                for path in case.glob('rank_output/*/rank.*/stderr')}
    counts = {}
    for rank in range(ranks):
        path = logs.get(rank)
        count = None
        if path is not None and path.is_file():
            with lab.archived.open_log(path) as stream:
                count = sum(a.ANSI.sub('', line).strip() == MARKER for line in stream)
        counts[str(rank)] = count
    on = env[SWITCH] == '1'
    observed = observations or {}
    expected = [str(rank) for rank in range(ranks)
                if on and (require_query or any(int(row.get('c_bound_queries', 0)) > 0
                                               for row in observed.get(str(rank), {}).get(
                                                   'NATIVE3D_LOCAL_SURFACE', ())))]
    wrong = [rank for rank, count in counts.items()
             if (on and count is not None and count > 1) or (not on and count not in (0, None))
             or (rank in expected and count != 1)]
    if require_query and not on:
        wrong.extend(rank for rank, count in counts.items() if count is None)
    return {'environment': env, 'rank_marker_counts': counts, 'query_evidence_ranks': expected,
            'ranks_with_no_marker': [rank for rank, count in counts.items() if count == 0],
            'rank_stderr_unavailable': [rank for rank, count in counts.items() if count is None],
            'log_pins': hashes(path for path in logs.values() if path.is_file()),
            'command_sha256': a.sha(case / 'command.json'), 'command': str(case / 'command.json'),
            'passed': not wrong, 'first_failure': {'rank': int(wrong[0]), 'field': 'switch marker count',
                                                   'observed': counts[wrong[0]],
                                                   'expected': int(on)} if wrong else None}


def fixture_base(label, mode):
    return HERE / 'fixtures' / f'build{label}' / mode


def summary_path(label, mode, correction=False):
    if mode in TRIALS:
        branch, name, trial = TRIALS[mode]
        root = HERE / branch / f'build{label}' / name / f'trial{trial}'
    else:
        root = fixture_base(label, mode)
    return root / ('r5_retry' if correction else '') / 'case_summary.json'


def correction_pins(correction, mode):
    if correction is None:
        return {}
    old = cut.read(correction)
    assert old['mode'] == mode and old['status'] == 'FAILED' and old['correction_of'] is None, correction
    assert correction == summary_path(old['build'], mode), correction
    return hashes((correction,))


def selected_summary(label, mode):
    retry = summary_path(label, mode, True)
    return retry if retry.is_file() else summary_path(label, mode)


def expected_fixture(mode):
    reference = M3 / 'fixtures/build668' / mode / 'case_summary.json'
    inherited.c3.c2.committed(reference, BASE)
    old = cut.read(reference)
    assert old['status'] == 'done' and old['build'] == 668 and old['details']['catch_passed']
    path = Path(old['used'])
    assert old['attempts'][str(path)] == a.sha(path)
    row = cut.read(path)
    lab.archived.verify_artifacts(path.parent, row['artifacts'])
    return reference, {'result': row}


def launcher_save(path, value):
    if path.name == 'command.json' and 'mpirun' in value['argv']:
        mpi_exports(value['argv'])
    return ORIGINAL_SAVE(path, value)


ORIGINAL_SAVE = cut.save


def check_public(case, mode):
    checker = HERE / 'check_public_query.py'
    log = case / 'test_outer.log'
    check = subprocess.run([sys.executable, '-B', str(checker), '--single',
                            '1' if mode == 'public-on' else '0', str(log)],
                           text=True, capture_output=True, check=False)
    try:
        data = json.loads(check.stdout)
    except json.JSONDecodeError:
        data = None
    return {'passed': check.returncode == 0 and isinstance(data, dict), 'result': data,
            'stdout': check.stdout, 'stderr': check.stderr, 'log': str(log), 'returncode': check.returncode}


def run_fixture(build, mode, correction):
    receipt, binary, pins = prepare(build)
    label = receipt['build']
    path = summary_path(label, mode, bool(correction))
    token = correction_pins(correction, mode)
    pins.update(token)
    if path.is_file():
        return path, verify_summary(receipt, mode, path)
    env = environment(mode == 'public-on', mode in ('curved', 'public-off', 'public-on'))
    if mode in HIDDEN:
        hidden = m1.fixtures.hidden
        hidden.clear_options()
        spec = ((1, '[.][NativeFP2P1ClosureDirect]', 2) if mode == 'direct'
                else (1, '[.][NativeFP2P1ClosurePublic]', 1))
        launch = hidden.c1.run_case
        if mode != 'direct':
            pins.update(hashes((HERE / 'check_public_query.py',)))
        with patch.object(hidden.c1, 'run_case', lambda *args: launch(*args, environment=env)):
            result, attempts = m1.fixtures.focus(path.parent, binary, fingerprint(receipt), pins, spec)
        used = Path(result['case']) / 'result.json'
        details = {'used': str(used), 'attempts': hashes(attempts), 'result': result}
        passed = result['passed']
        if mode != 'direct' and result['passed']:
            details['public_trace'] = check_public(used.parent, mode)
            passed = details['public_trace']['passed']
        status = 'passed' if passed else 'FAILED'
        case = used.parent
        proof_paths = list(attempts)
        rank_count = spec[0]
    else:
        # M3's own helper and checker paths remain intact. Its output is embedded in the P1 summary.
        inherited_summary = path.with_name('inherited_summary.json')
        if inherited_summary.is_file():
            details = cut.read(inherited_summary)
        else:
            with patch.multiple(m3, prepare=lambda _: (receipt, binary, pins.copy()),
                                environment=lambda *_: env, summary_path=lambda *_: inherited_summary,
                                base_dir=lambda _, name: fixture_base(label, name)), \
                    patch.object(cut, 'save', launcher_save), patch.object(m2, 'verified_old_fixture', expected_fixture):
                _, details = (m3.run_g1 if mode in G1 else m3.run_fixture)(build, mode, correction)
        if mode in G1:
            current, = details['cohorts'].values()
            used = Path(current['used'])
            rank_count = {'G1-serial': 1, 'G1-mpi2': 2, 'G1-mpi4': 4}[mode]
            proof_paths = [inherited_summary, used, used.with_name('test_wait.json'), used.with_name('pins.json')]
        else:
            used = Path(details['used'])
            rank_count = 2 if mode == 'off-mpi' else 1
            proof_paths = [inherited_summary, *map(Path, details['attempts'])]
        case = used.parent
        status = 'passed' if details['status'] == 'done' else details['status']
    marker = switches(case, env, rank_count, status == 'passed')
    if not marker['passed']:
        status = 'FAILED'
    path.parent.mkdir(parents=True, exist_ok=True)
    result = {**fingerprint(receipt), 'mode': mode, 'status': status, 'used': str(used), 'switches': marker,
              'details': details, 'pins': {**pins, **hashes(proof_paths), **marker['log_pins'],
                                           marker['command']: marker['command_sha256']},
              'correction_of': str(correction) if correction else None}
    a.sealed(path, result)
    return path, result


def verify_summary(receipt, mode, path):
    record = cut.read(path)
    assert record['mode'] == mode and fingerprint(record) == fingerprint(receipt), path
    verify_pins(record['pins'])
    if mode in TRIALS:
        assert record['switches']['environment'] == trial_environment(mode), path
        for filename, digest in record['attempts'].items():
            measurement = Path(filename)
            assert a.sha(measurement) == digest
            lab.checked_receipt(measurement.parent, fingerprint(receipt))
        assert record['used'] in record['attempts'], path
    elif mode in (*INHERITED, *HIDDEN):
        used = Path(record['used'])
        lab.archived.verify_artifacts(used.parent, cut.read(used)['artifacts'])
    return record


def checkpoint_parts(build, receipt):
    label = receipt['build']
    parts = {}
    for mode in STEP1:
        path = (HERE / 'shared' / f'build{label}' / 'closure0' / mode / 'case_summary.json'
                if mode in SHARED else selected_summary(label, mode))
        if not path.is_file():
            parts[mode] = {'status': 'not evaluated', 'path': None, 'sha256': None}
            continue
        if mode in SHARED:
            row = cut.read(path)
            assert fingerprint(row) == fingerprint(receipt), path
            assert row['environment'][SWITCH] == row['environment'][TRACE] == '0', path
            verify_pins(row['pins'])
        else:
            row = verify_summary(receipt, mode, path)
        assert row['status'] in TERMINAL, path
        parts[mode] = {'status': row['status'], 'path': str(path), 'sha256': a.sha(path)}
    return parts


def combined_status(parts):
    values = set(parts)
    return 'FAILED' if 'FAILED' in values else 'passed' if values == {'passed'} else 'OPEN'


def cross_public(receipt, parts, check_path):
    checker = HERE / 'check_public_query.py'
    logs = [Path(cut.read(Path(parts[mode]['path']))['details']['public_trace']['log'])
            for mode in ('public-off', 'public-on')]
    if not check_path.with_suffix('.receipt.json').is_file():
        result = subprocess.run([sys.executable, '-B', str(checker), *map(str, logs), str(check_path)],
                                check=False)
        status = 'passed' if result.returncode == 0 else 'FAILED'
        a.sealed(check_path.with_suffix('.receipt.json'),
                 {**fingerprint(receipt), 'status': status,
                  'pins': hashes(path for path in (checker, *logs, check_path) if path.is_file())})
    row = cut.read(check_path.with_suffix('.receipt.json'))
    assert fingerprint(row) == fingerprint(receipt)
    verify_pins(row['pins'])
    return row


def verify_checkpoint(build, publish=False):
    receipt, _, pins = prepare(build)
    path = build / 'p1_checkpoint.json'
    parts = checkpoint_parts(build, receipt)
    if all(parts[mode]['status'] == 'passed' for mode in ('public-off', 'public-on')):
        cross_path = build / 'public_query_comparison.json'
        if not publish:
            assert cross_path.with_suffix('.receipt.json').is_file(), cross_path
        cross = cross_public(receipt, parts, cross_path)
        row = cross_path.with_suffix('.receipt.json')
        parts['public-query-comparison'] = {'status': cross['status'], 'path': str(row), 'sha256': a.sha(row)}
    status = combined_status(row['status'] for row in parts.values())
    record = {**fingerprint(receipt), 'status': status, 'parts': parts, 'pins': pins}
    if publish:
        a.sealed(path, record)
    else:
        assert cut.read(path) == record, path
    return path, record


def prerequisites(build, mode, receipt):
    label = receipt['build']
    if mode == 'default-N1':
        paths = [selected_summary(label, fixture) for fixture in (*G1, *INHERITED)]
        for path, fixture in zip(paths, (*G1, *INHERITED), strict=True):
            assert verify_summary(receipt, fixture, path)['status'] == 'passed', path
    else:
        path, checkpoint = verify_checkpoint(build)
        assert checkpoint['status'] in ('passed', 'OPEN'), path
        paths = [path]
        if mode == 'anatomy-N1':
            selected = selected_summary(label, 'campaign-N1-1')
            verify_summary(receipt, 'campaign-N1-1', selected)
            paths.append(selected)
    return paths


def verify_terminal():
    for pattern in ('default/**/test_launch.json', 'campaign/**/test_launch.json', 'anatomy/**/test_launch.json'):
        for launch in HERE.glob(pattern):
            wait = a.resumed(launch.parent)
            assert wait and wait['terminal'], launch
            processes = cut.read(launch.with_name('owned_processes.json'))
            assert processes['all_observed_terminal']
            assert all(cut.terminal(row) for row in processes['identities']), launch


def trace_helpers():
    return (HERE / 'collect_p1.py', m2.HERE / 'collect_trace.py', m2.HERE / 'analyze_trace.py',
            m2.HERE / 'report_m2.py')


def attempt(build, mode, root, support):
    _, name, _ = TRIALS[mode]
    label = cut.read(build / 'build_status.json')['build']
    case = root / 'cases' / f'build{label}' / name
    verify_terminal()
    case.mkdir(parents=True, exist_ok=True)
    mode_path = case / 'p1_mode.json'
    a.sealed(mode_path, {'mode': mode, 'environment': trial_environment(mode),
                        'seconds_limit': TIMEOUTS[name], 'tolerance': '1E-4'})
    observer_files = m2.TRACE_FILES if mode == 'anatomy-N1' else ()

    def pinned(directory):
        receipt, binary, pins = prepare(directory)
        pins.update(hashes((mode_path, *support, *(trace_helpers() if observer_files else ()))))
        return receipt, binary, pins

    def input_data(ranks):
        config, *inputs = ORIGINAL_INPUT_DATA(ranks)
        assert len(m2.HAUSD.findall(config)) == 1, 'Expected one frozen ADAP_HAUSD= 1E-4 assignment.'
        return config, *inputs

    def evaluate(name, data, expected):
        host = cut.read(case / 'test_host_preflight.json')
        return inherited.evaluate(name, data, expected, host['loadavg'])

    def command(*args):
        return launch_recipe(*args, mode)

    with patch.multiple(lab, HERE=root, RUNS=inherited.RUNS, LINE=m2.LINE, __file__=__file__,
                        selector=inherited.selector, SECONDS=TIMEOUTS[name], prepare=pinned,
                        evaluate=evaluate, command=command), patch.object(lab.b0, 'input_data', input_data), \
            patch.object(a, 'ARTIFACTS', (*a.ARTIFACTS, 'p1_mode.json', *observer_files)):
        result = lab.run(build, name)
    verify_terminal()
    return case, result


ORIGINAL_INPUT_DATA = lab.b0.input_data


def owned(case):
    recorded = cut.read(case / 'p1_mode.json')
    mode = recorded['mode']
    _, name, _ = TRIALS[mode]
    recipe = cut.read(case / 'command.json')
    assert recorded['environment'] == trial_environment(mode)
    assert case.name == name and recipe['seconds_limit'] == recorded['seconds_limit'] == TIMEOUTS[name]

    def command(*args):
        return launch_recipe(*args, mode)

    with patch.multiple(lab, RUNS=inherited.RUNS, SECONDS=TIMEOUTS[name], selector=inherited.selector,
                        command=command):
        if mode != 'anatomy-N1':
            return lab.owned(case)
        import collect_p1
        with patch.dict(sys.modules, collect_trace=collect_p1):
            return m2.owned_anatomy(case, recipe['argv'])


def projection_ready(name, before, after, trial):
    if trial['wait']['returncode'] == 0 and trial['supervisor_stop'] is None:
        return True
    return before.keys() == after.keys() and all(
        len(before[rank].get(key, ())) == len(after[rank].get(key, ()))
        for rank in before for marker in comparison.ROW_NAMES if comparison.applies(name, marker)
        for key in (comparison.compare_lab_c1.row_key(marker),))


def identity(name, reference, trial):
    path, old = reference
    result = {'status': 'not evaluated', 'baseline': str(path) if path else None,
              'baseline_sha256': a.sha(path) if path else None}
    if old is not None:
        before, after = old['rows_by_rank'], trial['rows_by_rank']
        if projection_ready(name, before, after, trial):
            differences = comparison.repeat_differences(name, before, after)
            result.update(status='different' if differences else 'equal', differences=differences,
                          first_difference=m1.first_difference(differences, before, after))
    return result


def watchdog(case, result, second_attempt, exact, marker):
    old_log_problems = m1.log_problems

    def failures(case, ranks):
        problems = old_log_problems(case, ranks)
        problems.extend({'log': line, 'field': 'Catch/production failure'}
                        for line in inherited.failure_logs(inherited.lab_logs(case, ranks), catch=True))
        if exact['status'] == 'different':
            problems.append({'row': 'exact projection', **exact['first_difference']})
        if not marker['passed']:
            problems.append(marker['first_failure'])
        return problems

    with patch.multiple(m1, TIMEOUTS=TIMEOUTS, log_problems=failures):
        return m1.watchdog(case, result, second_attempt)


def anatomy_reference():
    path = m2.HERE / 'trials/build667/ANATOMY/case_summary.json'
    inherited.c3.c2.committed(path, BASE)
    old = cut.read(path)
    selected = Path(old['used'])
    assert old['status'] == 'done' and old['observer']['completion'] == 'complete'
    assert a.sha(selected) == old['attempts'][str(selected)]
    row = cut.read(selected)
    lab.archived.verify_artifacts(selected.parent, row['artifacts'])
    trace = selected.with_name('anatomy.json')
    assert a.sha(trace) == row['artifacts'][trace.name]
    return path, selected, trace


def anatomy_observations(case, trial):
    from analyze_trace import PHASES
    from report_m2 import stages

    observer = m2.trial_observations(case, 'ANATOMY', trial)
    summary = case / 'anatomy.json'
    if observer['completion'] == 'complete' and summary.is_file():
        stream = cut.read(summary)
        if not stream['stream']['forwards_started'] or not all(
                stream['phases'][phase]['ended_segments'] for phase in PHASES):
            observer.update(completion='failed', reason='Missing required forward or phase markers.')
    old_summary, old_selected, old_trace = anatomy_reference()
    observer['baseline'] = {'summary': str(old_summary), 'used': str(old_selected), 'trace': str(old_trace),
                            'pins': hashes((old_summary, old_selected, old_trace)),
                            'stage_rows': stages(cut.read(old_selected)['rows_by_rank']['0'].get('STAGE', ()))}
    raw = trial['rows_by_rank'].get('0', {}).get('STAGE', ())
    observer['stage_rows'] = []
    for row in raw:
        try:
            observer['stage_rows'].extend(stages((row,)))
        except (KeyError, ValueError, TypeError):
            if observer['completion'] == 'complete':
                observer.update(completion='failed', reason='Invalid completed M2 stage row.')
    if observer['completion'] == 'complete' and not observer['stage_rows']:
        observer.update(completion='failed', reason='Missing completed M2 stage rows.')
    return observer


def run_trial(build, mode, correction):
    receipt, _, pins = prepare(build)
    label = receipt['build']
    _, name, number = TRIALS[mode]
    correct = correction_pins(correction, mode)
    path = summary_path(label, mode, bool(correction))
    if path.is_file():
        return path, verify_summary(receipt, mode, path)
    support = prerequisites(build, mode, receipt)
    reference = (None, None)
    if mode == 'default-N1':
        reference = baseline(name)
        support.extend((m1.BASE_PROOF, reference[0]))
    elif mode == 'anatomy-N1':
        path_n1 = support[-1]
        n1 = cut.read(path_n1)
        if n1['status'] == 'passed':
            source = Path(n1['used'])
            reference = source, cut.read(source)
            support.append(source)
        support.extend(anatomy_reference())
    support.extend(map(Path, correct))
    attempts = []

    def run(folder, second, extra=()):
        case, trial = attempt(build, mode, path.parent / folder, (*support, *extra))
        attempts.append(case / 'measurement.json')
        exact = identity(name, reference, trial)
        marker = switches(case, trial_environment(mode), trial['ranks'], trial['passed'], trial['rows_by_rank'])
        evidence = watchdog(case, trial, second, exact, marker)
        return case, trial, exact, marker, evidence

    case, trial, exact, marker, evidence = run('attempt1', False)
    if evidence.get('status') == 'retry_allowed':
        case, trial, exact, marker, evidence = run('watchdog_retry', True,
                                                   (case / 'watchdog.json', case / 'measurement.json'))
    observer = anatomy_observations(case, trial) if mode == 'anatomy-N1' else None
    first = None
    status = 'passed'
    if exact['status'] == 'different':
        status, first = 'FAILED', exact['first_difference']
    elif not marker['passed']:
        status, first = 'FAILED', marker['first_failure']
    elif not trial['passed']:
        status = 'OPEN' if evidence.get('status') == 'open' else 'FAILED'
        first = m2.first_finding(trial, evidence)
    result_status = status
    if observer:
        if observer['completion'] == 'failed' or (status == 'passed' and observer['completion'] != 'complete'):
            if status != 'FAILED':
                first = {'field': 'anatomy completion', 'observed': observer['completion'],
                         'reason': observer.get('reason')}
            status = 'FAILED'
        elif status == 'passed' and exact['status'] == 'not evaluated':
            status = 'OPEN'
        pins.update(observer['baseline']['pins'])
    pins.update({**hashes(support), **marker['log_pins'], marker['command']: marker['command_sha256']})
    for measurement in attempts:
        watchdog_path = measurement.with_name('watchdog.json')
        if watchdog_path.is_file():
            pins.update(hashes((watchdog_path,)))
    record = {**fingerprint(receipt), 'mode': mode, 'case': name, 'trial': number, 'status': status,
              'result_status': result_status, 'evaluate_qualified': trial['passed'],
              'used': str(case / 'measurement.json'), 'attempts': hashes(attempts), 'pins': pins,
              'identity': exact, 'observer': observer, 'switches': marker, 'first_difference': first,
              'watchdog': evidence, 'correction_of': str(correction) if correction else None}
    a.sealed(path, record)
    return path, record


def timing(trial):
    by_rank = {}
    for rank, rows in trial['rows_by_rank'].items():
        by_rank[rank] = {
            'by_pass': {surface['pass']: {
                'faces': int(surface['faces_out']),
                'removals': int(surface['collapses_surface']),
                'seconds_c': float(surface['seconds_c']),
                'seconds_c_bound': float(surface['seconds_c_bound']),
                'seconds_pass': next((float(row['seconds_pass']) for row in rows.get('pass', ())
                                      if row['pass'] == surface['pass']), None),
            } for surface in rows.get('NATIVE3D_LOCAL_SURFACE', ())},
            'cells': [int(row['cells_out']) for row in rows.get('end', ())],
        }
    return {'elapsed_seconds': trial['wait']['elapsed_seconds'],
            'totals': trial['measurement']['totals'], 'by_rank': by_rank}


def campaign_comparison(build):
    receipt, _, pins = prepare(build)
    checkpoint, stage = verify_checkpoint(build)
    assert stage['status'] in ('passed', 'OPEN')
    pins.update(hashes((checkpoint, m1.BASE_PROOF)))
    trial_status, measurements = {}, {}
    label = receipt['build']
    for mode in CAMPAIGN:
        path = selected_summary(label, mode)
        if not path.is_file():
            trial_status[mode] = {'status': 'not evaluated', 'path': None, 'used': None}
            continue
        row = verify_summary(receipt, mode, path)
        pins.update(hashes((path, Path(row['used']))))
        trial_status[mode] = {key: row[key] for key in ('status', 'used', 'evaluate_qualified', 'first_difference')}
        trial_status[mode]['path'] = str(path)
        measurements[mode] = cut.read(Path(row['used']))
    pairs, historical = {}, {}
    for name in TIMEOUTS:
        a_mode, b_mode = (f'campaign-{name}-{number}' for number in (1, 2))
        old_path, old = baseline(name)
        pins.update(hashes((old_path,)))
        result = {'status': 'not evaluated'}
        if all(trial_status[mode]['status'] == 'passed' and trial_status[mode]['evaluate_qualified']
               for mode in (a_mode, b_mode)):
            first, second = measurements[a_mode], measurements[b_mode]
            result = identity(name, (Path(trial_status[a_mode]['used']), first), second)
            result['candidate'] = trial_status[b_mode]['used']
        pairs[name] = result
        comparison_to_old = {'status': 'not evaluated', 'baseline': str(old_path),
                             'baseline_sha256': a.sha(old_path)}
        if a_mode in measurements:
            comparison_to_old = identity(name, (old_path, old), measurements[a_mode])
        historical[name] = {**comparison_to_old, 'build664': timing(old),
                            'trial1': timing(measurements[a_mode]) if a_mode in measurements else None}
    statuses = [row['status'] for row in trial_status.values()]
    statuses.extend({'equal': 'passed', 'different': 'FAILED', 'not evaluated': 'OPEN'}[row['status']]
                    for row in pairs.values())
    record = {**fingerprint(receipt), 'status': combined_status(statuses), 'checkpoint': str(checkpoint),
              'trials': trial_status, 'pairs': pairs, 'baseline': historical, 'pins': pins}
    path = HERE / 'campaign' / f'build{label}' / 'comparison.json'
    return path, record


def verify_campaign(build, publish=False):
    path, record = campaign_comparison(build)
    if publish:
        path.parent.mkdir(parents=True, exist_ok=True)
        a.sealed(path, record)
    else:
        assert cut.read(path) == record, path
    return path, record


def controller(build, mode, correction):
    with (HERE / '.owned.lock').open('a') as lock:
        fcntl.flock(lock, fcntl.LOCK_EX | fcntl.LOCK_NB)
        if mode == 'checkpoint':
            path, result = verify_checkpoint(build, publish=True)
        elif mode == 'compare':
            path, result = verify_campaign(build, publish=True)
        elif mode == 'campaign':
            assert correction is None
            for trial_mode in CAMPAIGN:
                path, result = run_trial(build, trial_mode, None)
                print(f"{trial_mode} {result['status']} {path}", flush=True)
            path, result = verify_campaign(build, publish=True)
        elif mode in TRIALS:
            path, result = run_trial(build, mode, correction)
        else:
            path, result = run_fixture(build, mode, correction)
        print(f"{mode} {result['status']} {path}", flush=True)
        return TERMINAL[result['status']]


def self_check():
    import collect_p1

    collect_p1.self_check()
    for name in TIMEOUTS:
        path, reference = baseline(name)
        assert identity(name, (path, reference), reference)['status'] == 'equal'
        assert timing(reference)['by_rank']
    for mode in INHERITED:
        expected_fixture(mode)
    anatomy_reference()
    recipe = mpi_exports(['env', 'mpirun', '-n', '6', 'test_driver'])
    assert recipe.count(SWITCH) == recipe.count(TRACE) == 1
    assert mpi_exports(recipe).count(SWITCH) == 1
    assert trial_environment('default-N1')[SWITCH] == '0'
    assert trial_environment('anatomy-N1')[TRACE] == '1'
    assert combined_status(('passed', 'not evaluated')) == 'OPEN'
    print('FP2-P1 trial reference seals, inherited fixtures, trace counters and switch recipes verified.')


def main():
    parser = ArgumentParser(description=__doc__)
    parser.add_argument('build', type=Path, nargs='?')
    modes = parser.add_mutually_exclusive_group(required=True)
    modes.add_argument('--only', choices=(*G1, *INHERITED, *HIDDEN, *TRIALS, 'checkpoint', 'campaign', 'compare'))
    modes.add_argument('--owned', type=Path)
    modes.add_argument('--self-check', action='store_true')
    parser.add_argument('--launch', action='store_true')
    parser.add_argument('--supervisor', type=Path)
    parser.add_argument('--correction-of', type=Path)
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
        with patch.multiple(m2, HERE=HERE, __file__=__file__):
            return m2.durable_launch(build, mode, correction)
    if args.supervisor:
        with patch.object(m2, 'controller', controller):
            return m2.supervise(build, mode, correction, args.supervisor.resolve())
    return controller(build, mode, correction)


if __name__ == '__main__':
    raise SystemExit(main())
