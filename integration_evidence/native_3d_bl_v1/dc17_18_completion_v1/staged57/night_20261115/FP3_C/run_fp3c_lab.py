"""Run and seal a single FP3-C lab trial using the FP2-P2 campaign recipe."""

from argparse import ArgumentParser
from pathlib import Path
import os
import sys
from unittest.mock import patch

from collect_fp3_full import FINAL_SWEEP, SWEEP_COUNTS, SWEEP_SECONDS, parse_sweep_fields

sys.dont_write_bytecode = True
DRIVER = Path(__file__).resolve()
HERE = DRIVER.parent
P2 = HERE.parent / 'FP2_P2'
sys.path.insert(0, str(P2))
import run_p2 as p2

lab, a, cut, inherited = p2.lab, p2.a, p2.cut, p2.inherited
BASE = 'f7447473e5'
ORDER = HERE / 'inputs/FP3_C_FINAL_SURFACE_SWEEP.md'
ORDER_SHA = 'f5a611891260558cb67a648cefc27dd68c5fa5592b0298e28f7223bf1d9fd274'
SOURCE_CHECK = HERE / 'reports/source_check.md'
PROBE = 'SU2_NATIVE_FP3_PROBE'
SWEEP = 'SU2_NATIVE_FP3_FINAL_SWEEP'
MARKER = 'FP3_C LAB_SWEEP'
EXPECTED = {'N6P': (6, 2), 'N1': (1, 1)}
PARTS = ('N6P-off', 'N6P-on', 'N1-on')
EXIT = {'passed': 0, 'OPEN': 75, 'not evaluated': 75, 'FAILED': 1}
ALARMS = {'c_sweeps': 49, 'star_max': 1000, 'v_limit': 0}


def verify_order():
    assert a.sha(ORDER) == ORDER_SHA
    assert ORDER.with_suffix('.sha256').read_text().split() == [ORDER_SHA, ORDER.name]
    assert SOURCE_CHECK.is_file(), SOURCE_CHECK


def trial_path(label, part):
    return HERE / 'lab' / f'build{label}' / part / 'case_summary.json'


def environment(part):
    name, setting = part.split('-')
    env = p2.trial_environment(f'campaign-{name}-1')
    # At the accepted base only a value of 0 disables the default closure.
    env.pop(p2.SWITCH)
    return {**env, PROBE: '0', SWEEP: str(int(setting == 'on'))}


def launch_command(part, *args):
    recipe = p2.m2.LAB_COMMAND(*args)
    env = environment(part)
    i = recipe.index('nice')
    recipe[i:i] = [f'{key}={value}' for key, value in env.items()]
    if 'mpirun' in recipe:
        i = recipe.index('mpirun') + 1
        recipe[i:i] = [arg for key in env for arg in ('-x', key)]
    return recipe


def switch_recipe(case, part, argv, seconds):
    env = environment(part)
    ranks, _ = EXPECTED[case.name]
    for key, value in env.items():
        assert [item for item in argv if item.startswith(f'{key}=')] == [f'{key}={value}'], (case, key)
        if ranks > 1:
            assert sum(argv[i:i + 2] == ['-x', key] for i in range(len(argv))) == 1, (case, key)
    assert p2.SWITCH not in argv and not any(arg.startswith(f'{p2.SWITCH}=') for arg in argv)
    assert argv[argv.index('timeout') + 3] == str(seconds) and argv[-1] == inherited.selector(case.name)
    return env


def clean_environment():
    for key in tuple(os.environ):
        if key.startswith(('SU2_NATIVE_', 'SU2_FP1_', 'SU2_FP2_')):
            del os.environ[key]


def historical(name):
    old_build = P2 / 'build672'
    path = p2.selected_summary(672, f'campaign-{name}-1')
    status = old_build / 'build_status.json'
    source = old_build / 'source_pins.json'
    for item in (path, status, source):
        inherited.c3.c2.committed(item, BASE)
    build = cut.read(status)
    assert build['passed'] and a.sha(source) == build['source_pins_sha256'], source
    summary = cut.read(path)
    fingerprint = p2.fingerprint(build)
    assert p2.fingerprint(summary) == fingerprint and summary['case'] == name and summary['trial'] == 1
    assert summary['status'] == 'passed' and summary['evaluate_qualified'] and summary['switches']['passed']
    binary_aliases = {build['binary']: build['binary_sha256'], build['test_binary']: build['test_binary_sha256']}
    assert all(summary['pins'][filename] == digest for filename, digest in binary_aliases.items())
    # These historical binary paths are the shared build's mutable aliases. The old digest is sealed in Git.
    p2.verify_pins({filename: digest for filename, digest in summary['pins'].items() if filename not in binary_aliases})
    selected = Path(summary['used'])
    assert selected.is_relative_to(path.parent)
    inherited.c3.c2.committed(selected, BASE)
    assert a.sha(selected) == summary['attempts'][str(selected)], selected
    trial = lab.checked_receipt(selected.parent, fingerprint)
    assert trial['passed'] and trial['supervisor_stop'] is None and trial['wait']['returncode'] == 0
    assert trial['ranks'] == EXPECTED[name][0] and trial['case'] == name
    processes = cut.read(selected.with_name('owned_processes.json'))
    assert processes['all_observed_terminal'] and all(cut.terminal(row) for row in processes['identities'])
    assert summary['switches']['environment'] == p2.trial_environment(summary['mode']), path
    return path, selected, trial


def check_input_data(name, selected):
    config, sensor, copies, pins, counts = p2.ORIGINAL_INPUT_DATA(EXPECTED[name][0])
    old = cut.read(selected.with_name('pins.json'))
    assert config == selected.with_name('run.cfg').read_bytes()
    assert len(p2.m2.HAUSD.findall(config)) == 1, 'Expected one frozen ADAP_HAUSD= 1E-4 assignment.'
    assert old[str(sensor)] == pins[str(sensor)]
    assert all(old[str(selected.with_name(path.name))] == digest for path, digest in copies.items())
    return config, sensor, copies, pins, counts


def sweep_findings(row):
    findings = []
    context = {field: row[field] for field in ('rank', 'stage', 'pass')}

    def check(field, actual, expected, *, maximum=False):
        wrong = actual > expected if maximum else actual != expected
        if wrong:
            findings.append({**context, 'field': field, 'observed': actual,
                             'maximum' if maximum else 'expected': expected})

    check('sweep_removed_faces', row['faces_before'] - row['faces_after'], 2 * row['collapses'])
    for field, maximum in ALARMS.items():
        check(field, row[field], maximum, maximum=True)
    if not row['collapses']:
        check('repair_seconds', row['repair_seconds'], 0)
    if not row['candidates']:
        for field in ('prepared_directions', 'collapses', 'metric_floor', 'bound_queries',
                      *(key for key in SWEEP_COUNTS if key.startswith('c_refused_'))):
            check(field, row[field], 0)
    return findings


def observations(paths, name, enabled):
    ranks, last = EXPECTED[name]
    rows, markers, problems = [], [], []
    for rank in range(ranks):
        sweeps, switches = [], []
        path = paths.get(rank)
        if path:
            with lab.archived.open_log(path) as stream:
                for number, raw in enumerate(stream, 1):
                    line = a.ANSI.sub('', raw).rstrip('\r\n')
                    if line.startswith(MARKER):
                        switches.append((number, line))
                    elif line.startswith(FINAL_SWEEP):
                        atoms = line[len(FINAL_SWEEP):].split()
                        fields = dict(a.FIELD.findall(line[len(FINAL_SWEEP):]))
                        sweeps.append((number, fields, not raw.endswith(('\n', '\r')) or
                                       len(fields) != len(atoms) or
                                       any(f'{key}={value}' not in atoms for key, value in fields.items())))
        expected = f'{MARKER} rank={rank} enabled={int(enabled)} probe=0'
        if [line for _, line in switches] == [expected]:
            markers.append({'rank': rank, 'line': switches[0][0], 'value': int(enabled), 'probe': 0})
        else:
            problems.append({'rank': rank, 'field': 'lab switch marker',
                             'expected': expected, 'observed': switches})
        if len(sweeps) != int(enabled):
            problems.append({'rank': rank, 'field': 'final sweep row count',
                             'expected': int(enabled), 'observed': len(sweeps)})
            continue
        for line, fields, cut_row in sweeps:
            try:
                row = parse_sweep_fields(fields, cut=cut_row)
                if (row['rank'], row['stage'], row['pass']) != (rank, 0, last):
                    raise ValueError(f'Expected rank={rank}, stage=0, pass={last}')
                rows.append(row)
            except ValueError as error:
                problems.append({'rank': rank, 'line': line, 'field': 'final sweep row', 'detail': str(error)})
    return rows, markers, problems


def evaluate(case, part, data, expected):
    name = case.name
    ranks, last = EXPECTED[name]
    enabled = part.endswith('-on')
    sweep, markers, problems = observations(lab.b0.log_paths(case, ranks), name, enabled)
    by_rank = {row['rank']: row for row in sweep}
    evaluate_base = inherited.c3.prior.BASELINE_EVALUATE

    def final_face_census(name, data, expected):
        projected = {}
        for rank, rows in data.items():
            final = by_rank.get(rank)
            measured = str(final['faces_after']) if final is not None else None
            if enabled and measured is None:
                # The sweep measurement already failed. Keep the other evaluator checks active without
                # mistaking a comparison to the legacy pre-sweep row for a production result.
                measured = next((row.get('faces_out') for row in rows.get('pass', ())
                                 if row.get('pass') == str(last)), None)
            surface = [{**row, 'faces_out': measured} if measured is not None and row['pass'] == str(last) else row
                       for row in rows.get('NATIVE3D_LOCAL_SURFACE', ())]
            projected[rank] = {**rows, 'NATIVE3D_LOCAL_SURFACE': surface}
        return evaluate_base(name, projected, expected)

    host = cut.read(case / 'test_host_preflight.json')
    with patch.object(inherited.c3.prior, 'BASELINE_EVALUATE', final_face_census):
        result = inherited.evaluate(name, data, expected, host['loadavg'])
    result['sweep_rows'] = sweep
    result['sweep_switches'] = markers
    result['sweep_measurement_problems'] = problems
    for row in sweep:
        rank = row['rank']
        result['failed_limits'].extend(sweep_findings(row))
        current = data.get(rank, {})
        if current.get('pass') and current['pass'][-1]['pass'] == str(last):
            observed = current['pass'][-1]
            for field, other in (('faces_after', 'faces_out'), ('bad_after', 'bad_out')):
                if row[field] != int(observed[other]):
                    result['failed_limits'].append({'rank': rank, 'pass': last, 'field': field,
                                                    'observed': row[field], 'expected': int(observed[other])})
    result['problems'].extend(problems)
    return result


def previous_terminal():
    for launch in HERE.glob('lab/**/test_launch.json'):
        wait = a.resumed(launch.parent)
        assert wait and wait['terminal'], launch
        previous = cut.read(launch.with_name('owned_processes.json'))
        assert previous['all_observed_terminal']
        assert all(cut.terminal(row) for row in previous['identities']), launch


def attempt(receipt, part, attempt_name, seconds, support, selected):
    label = receipt['build']
    name = part.split('-')[0]
    root = trial_path(label, part).parent / attempt_name
    case = root / 'cases' / f'build{label}' / name
    previous_terminal()
    case.mkdir(parents=True, exist_ok=True)
    mode = case / 'fp3c_lab_mode.json'
    a.sealed(mode, {'part': part, 'environment': environment(part), 'seconds_limit': seconds, 'tolerance': '1E-4'})
    support = (*support, mode, Path(receipt['binary']), Path(receipt['test_binary']),
               HERE / f'build{label}/source_pins.json', HERE / f'build{label}/build_status.json')

    def prepare(_):
        return receipt, Path(receipt['test_binary']), p2.hashes(support)

    def measure(name, data, expected):
        return evaluate(case, part, data, expected)

    def command(*args):
        return launch_command(part, *args)

    def inputs(ranks):
        assert ranks == EXPECTED[name][0]
        return check_input_data(name, selected)

    with patch.multiple(lab, HERE=root, RUNS=inherited.RUNS, LINE=p2.m2.LINE, __file__=str(DRIVER),
                        selector=inherited.selector, SECONDS=seconds, prepare=prepare, evaluate=measure,
                        command=command), patch.object(lab.b0, 'input_data', inputs), \
            patch.object(a, 'ARTIFACTS', (*a.ARTIFACTS, mode.name)):
        result = lab.run(HERE / f'build{label}', name)
    previous_terminal()
    return case, result


def owned(case):
    mode = cut.read(case / 'fp3c_lab_mode.json')
    part, seconds = mode['part'], mode['seconds_limit']
    name = part.split('-')[0]
    assert part in PARTS and case.name == name
    assert case.resolve().is_relative_to(HERE / 'lab')
    assert mode['environment'] == environment(part)
    assert seconds in (p2.TIMEOUTS[name], 3 * p2.TIMEOUTS[name])
    recipe = cut.read(case / 'command.json')
    assert recipe['seconds_limit'] == seconds and recipe['ranks'] == EXPECTED[name][0]
    switch_recipe(case, part, recipe['argv'], seconds)
    clean_environment()

    def command(*args):
        return launch_command(part, *args)

    with patch.multiple(lab, RUNS=inherited.RUNS, SECONDS=seconds,
                        selector=inherited.selector, command=command):
        return lab.owned(case)


def rank_results(trial):
    name = trial['case']
    totals = trial['measurement']['totals']
    result = {}
    for rank, rows in trial['rows_by_rank'].items():
        if not all(rows.get(key) for key in ('pass', 'core', 'end')):
            continue
        final, core = rows['pass'][-1], rows['core'][-1]
        if (int(final['pass']), int(core['pass'])) != (EXPECTED[name][1],) * 2:
            continue
        result[rank] = {**{field: int(final[field]) for field in ('faces_out', 'cells_out', 'bad_out')},
                        'handover_cells': int(rows['end'][0]['cells_out']), 'short_out': int(core['short_out']),
                        'seconds_pass': float(final['seconds_pass'])}
    return {'by_rank': result, 'totals': totals,
            'handover_faces': int(trial['measurement']['proof']['classified'])
            if 'classified' in trial['measurement']['proof'] else None}


def completed(path, receipt):
    record = cut.read(path)
    assert p2.fingerprint(record) == p2.fingerprint(receipt), path
    p2.verify_pins(record['pins'])
    selected = Path(record['used'])
    assert selected.is_relative_to(path.parent)
    assert a.sha(selected) == record['attempts'][str(selected)], selected
    lab.checked_receipt(selected.parent, p2.fingerprint(receipt))
    return record


def preceding(label, part, receipt):
    if part == 'N6P-off':
        return []
    path = HERE / f'build{label}/step1.json'
    step1 = cut.read(path)
    assert p2.fingerprint(step1) == p2.fingerprint(receipt) and step1['status'] == 'passed'
    p2.verify_pins(step1['pins'])
    off = trial_path(label, 'N6P-off')
    assert completed(off, receipt)['status'] == 'passed'
    return [path, off]


def run_trial(label, part):
    from build_fp3 import verify_build

    verify_order()
    lab.b0.host()
    clean_environment()
    receipt = verify_build(label)
    path = trial_path(label, part)
    if path.is_file():
        return path, completed(path, receipt)
    name = part.split('-')[0]
    history, historical_measurement, reference = historical(name)
    support = [DRIVER, ORDER, ORDER.with_suffix('.sha256'), SOURCE_CHECK, Path(p2.__file__),
               Path(__file__).with_name('collect_fp3_full.py'), inherited.DRIVER, p2.m1.C3 / 'compare_lab.py',
               Path(p2.m2.__file__), Path(p2.m1.__file__), Path(p2.m3.__file__),
               HERE / f'build{label}/build_status.json', HERE / f'build{label}/source_pins.json',
               history, historical_measurement, *preceding(label, part, receipt)]
    attempts, seconds = {}, p2.TIMEOUTS[name]
    identity = None
    for number, folder in enumerate(('attempt1', 'watchdog_retry')):
        limit = seconds * (3 if number else 1)
        case, trial = attempt(receipt, part, folder, limit, support, historical_measurement)
        measurement = case / 'measurement.json'
        attempts[str(measurement)] = a.sha(measurement)
        argv = cut.read(case / 'command.json')['argv']
        env = switch_recipe(case, part, argv, limit)
        if part == 'N6P-off':
            identity = p2.identity(name, (historical_measurement, reference), trial)
        gates = trial['measurement']
        problems = gates['sweep_measurement_problems']
        complete_problems = [problem for problem in gates['problems'] if problem not in problems]
        raw_failures = p2.m1.log_problems(case, trial['ranks'])
        raw_failures.extend({'field': 'Catch/production failure', 'log': line}
                            for line in inherited.failure_logs(inherited.lab_logs(case, trial['ranks']), catch=True))
        timed_out = trial['supervisor_stop'] == 'wrapper_time_limit'
        results = list(gates['failed_limits']) + raw_failures
        if identity is not None and identity['status'] == 'different':
            results.insert(0, identity['first_difference'])
        memory_limit = a.MEMORY * 1024
        if trial['peak_lower_bound_bytes'] > memory_limit:
            results.append({'field': 'job peak bytes', 'observed': trial['peak_lower_bound_bytes'],
                            'maximum': memory_limit})
        # A cut run can lack expected rows. Existing complete result evidence still takes precedence.
        results.extend(problem for problem in complete_problems
                       if not timed_out or isinstance(problem.get('observed'), dict) and problem['observed'])
        watchdog = {}
        if timed_out:
            watchdog = {'status': 'result_failed' if results else 'open' if number else 'retry_allowed',
                        'seconds_limit': limit, 'measurement_sha256': attempts[str(measurement)],
                        'result_findings': results,
                        'host_preflight': cut.read(case / 'test_host_preflight.json')}
            seal = case / 'watchdog.json'
            a.sealed(seal, watchdog)
            support.extend((seal, measurement))
            if watchdog['status'] == 'retry_allowed':
                continue
        break
    passed = trial['passed'] and not results and not problems and (identity is None or identity['status'] == 'equal')
    status = 'passed' if passed else 'OPEN' if watchdog.get('status') == 'open' else 'FAILED'
    first = (results or problems or [p2.m2.first_finding(trial, watchdog)])[0] if not passed else None
    kind = None if passed else 'helper_watchdog' if status == 'OPEN' else 'measurement' if problems and not results else 'result'
    context = reference if part != 'N6P-on' else cut.read(Path(completed(trial_path(label, 'N6P-off'), receipt)['used']))
    context_name = 'FP2-P2 build672' if part != 'N6P-on' else 'FP3-C N6P-off'
    rows = trial['measurement']['sweep_rows']
    pins = {**p2.hashes(support), **attempts}
    for used in attempts:
        candidate = Path(used).parent
        pins.update({str(candidate / filename): digest for filename, digest in cut.read(Path(used))['artifacts'].items()})
    result = {**p2.fingerprint(receipt), 'part': part, 'case': name, 'status': status, 'passed': passed,
              'failure_kind': kind, 'first_difference': first, 'used': str(measurement), 'attempts': attempts,
              'pins': pins, 'identity': identity, 'watchdog': watchdog,
              'switches': {'environment': env, 'ranks': trial['measurement']['sweep_switches']},
              'sweep_rows': rows,
              'sweep_sums': {field: sum(row[field] for row in rows) for field in (*SWEEP_COUNTS, *SWEEP_SECONDS)},
              'report': {'trial': rank_results(trial), 'comparison_run': context_name,
                         'comparison': rank_results(context)}}
    a.sealed(path, result)
    return path, result


def seal_step2(label):
    from build_fp3 import verify_build

    verify_order()
    receipt = verify_build(label)
    step1 = HERE / f'build{label}/step1.json'
    previous = cut.read(step1)
    assert p2.fingerprint(previous) == p2.fingerprint(receipt) and previous['status'] == 'passed'
    p2.verify_pins(previous['pins'])
    parts = {}
    pins = p2.hashes((step1, DRIVER))
    failures = []
    for part in PARTS[1:]:
        path = trial_path(label, part)
        if not path.is_file():
            parts[part] = {'status': 'not evaluated'}
            continue
        row = completed(path, receipt)
        parts[part] = {'status': row['status'], 'path': str(path), 'sha256': a.sha(path),
                       'sweep_sums': row['sweep_sums'], 'first_difference': row['first_difference']}
        pins.update({str(path): a.sha(path), **row['pins']})
        if row['status'] != 'passed':
            failures.append(row)
    status = next((s for s in ('FAILED', 'OPEN', 'not evaluated')
                   if any(row['status'] == s for row in parts.values())), 'passed')
    finding = next((row for row in failures if row['status'] == status), {})
    result = {**p2.fingerprint(receipt), 'status': status, 'passed': status == 'passed', 'parts': parts,
              'first_difference': finding.get('first_difference'), 'failure_kind': finding.get('failure_kind'),
              'pins': pins}
    path = HERE / f'build{label}/step2.json'
    if all('path' in row for row in parts.values()):
        a.sealed(path, result)
    return path, result


def self_check():
    from tempfile import TemporaryDirectory

    off = environment('N6P-off')
    on = environment('N1-on')
    assert off[SWEEP] == '0' and on[SWEEP] == '1' and off[PROBE] == on[PROBE] == '0'
    assert p2.SWITCH not in off and p2.SWITCH not in on and off[p2.TRACE] == on[p2.TRACE] == '0'
    with TemporaryDirectory(prefix='su2-fp3c-lab-') as temp:
        path = Path(temp) / 'stdout'
        row = {'rank': 0, 'stage': 0, 'pass': 1, **dict.fromkeys((*SWEEP_COUNTS, *SWEEP_SECONDS), 0)}
        row.update(faces_before=8, faces_after=8, seconds=.1)
        marker = f'{MARKER} rank=0 enabled=1 probe=0\n'
        text = FINAL_SWEEP + ''.join(f' {key}={value}' for key, value in row.items()) + '\n'
        path.write_text(marker + text)
        rows, switches, problems = observations({0: path}, 'N1', True)
        assert rows == [row] and len(switches) == 1 and not problems and not sweep_findings(row)
        assert sweep_findings({**row, 'faces_after': 7})[0]['field'] == 'sweep_removed_faces'
        path.write_text(marker + text + text)
        assert observations({0: path}, 'N1', True)[2][0]['field'] == 'final sweep row count'
        path.write_text(marker + text.rstrip())
        assert observations({0: path}, 'N1', True)[2][0]['field'] == 'final sweep row'
    print('FP3-C campaign choices, sweep evidence and independent face accounting self-check passed.')


if __name__ == '__main__':
    if len(sys.argv) == 3 and sys.argv[1] == '--owned':
        raise SystemExit(owned(Path(sys.argv[2])))
    parser = ArgumentParser(description=__doc__)
    parser.add_argument('build', type=int)
    parser.add_argument('part', choices=(*PARTS, 'step2', 'self-check'))
    args = parser.parse_args()
    if args.part == 'self-check':
        self_check()
        raise SystemExit(0)
    path, record = seal_step2(args.build) if args.part == 'step2' else run_trial(args.build, args.part)
    print(f"{args.part} {record['status']}: {path}", flush=True)
    raise SystemExit(EXIT[record['status']])
