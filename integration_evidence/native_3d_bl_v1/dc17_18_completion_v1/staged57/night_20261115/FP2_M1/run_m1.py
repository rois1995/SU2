"""Run one FP2-M1 fixture or one measured lab case with sealed evidence."""

from argparse import ArgumentParser
from importlib.util import module_from_spec, spec_from_file_location
import math
import os
from pathlib import Path
import re
import subprocess
import sys
from unittest.mock import patch

sys.dont_write_bytecode = True
HERE = Path(__file__).resolve().parent
C3 = HERE.parent / 'FP2_C3'
sys.path.insert(0, str(C3))
import compare_lab as comparison
import run_c2_lab as inherited

fixture_spec = spec_from_file_location('fp2_c3_fixtures_for_m1', C3 / 'run_fixtures.py')
fixtures = module_from_spec(fixture_spec)
fixture_spec.loader.exec_module(fixtures)

lab, a, cut = inherited.lab, inherited.a, inherited.a.cut
BASE = 'af5844543d'
BASE_BUILD = 664
BASE_PROOF = C3 / 'cases/build664/lab_acceptance.json'
BASE_FIXTURES = C3 / 'fixtures/build664/c3_acceptance.json'
ORDER = HERE / 'inputs/FP2_M1_BOUND_SUPPORT.md'
ORDER_SHA = 'f9388a056ba264c7612e1c8a415783dc7b7ce75dd28dbbf263141dccdcef8ef5'
SOURCE_CHECK = HERE / 'source_check.md'
CASES = {key: inherited.RUNS[key] for key in ('N6P', 'N1')}
TIMEOUTS = {'N6P': 2575, 'N1': 3261}
OFF = {'off-serial': 'contact_serial', 'off-mpi': 'contact_mpi'}
GROUPS = ('proved_1', 'proved_2', 'needs_full', 'direct_eligible')
LINE = re.compile(inherited.LINE.pattern.replace('^(', '^(FP2_M1 [A-Z_]+|', 1))
OPTION = 'SU2_NATIVE_FP2_M1'
REFERENCE = 'SU2_NATIVE_FP2_M1_REFERENCE'
TRACE = 'SU2_NATIVE_GEOMETRY_WORK_TRACE'
IN_PROCESS = re.compile(
    r'FAILED:|exceeded (?:its |the )?(?:deadline|.*(?:memory|storage) (?:budget|limit))|'
    r'(?:memory|storage) (?:budget|admission).*exceed|(?:geometry|remesh).*deadline|out of memory',
    re.IGNORECASE,
)


def order_sources():
    checksum = ORDER.with_suffix('.sha256')
    assert a.sha(ORDER) == ORDER_SHA
    assert checksum.read_text().split() == [ORDER_SHA, ORDER.name]
    assert SOURCE_CHECK.is_file(), SOURCE_CHECK
    return ORDER, checksum, SOURCE_CHECK


def baseline(name):
    inherited.c3.c2.committed(BASE_PROOF, BASE)
    proof = cut.read(BASE_PROOF)
    assert proof['build'] == BASE_BUILD and proof['passed'] and proof['cases'][name]['passed']
    path, row = comparison.load_trial(BASE_BUILD, name, 1)
    assert row['passed'] and proof['artifacts'][str(path)] == a.sha(path), path
    assert all(row[key] == proof[key] for key in lab.FINGERPRINTS), path
    seconds = math.ceil(3 * row['wait']['elapsed_seconds'])
    assert TIMEOUTS[name] == seconds, (name, seconds)
    return path, row


def production_pins(build, receipt):
    path, base_pins = (C3 / 'build664' / filename for filename in ('build_status.json', 'source_pins.json'))
    for old in (path, base_pins):
        inherited.c3.c2.committed(old, BASE)
    reference = cut.read(path)
    assert reference['source_pins_sha256'] == a.sha(base_pins)
    assert receipt['build'] > BASE_BUILD and receipt['binary_sha256'] == reference['binary_sha256']
    assert a.sha(Path(receipt['binary'])) == reference['binary_sha256']
    current = cut.read(build / 'source_pins.json')
    pinned = {}
    for filename, digest in cut.read(base_pins).items():
        source = Path(filename)
        if not source.is_relative_to(cut.REPO):
            continue
        if source.relative_to(cut.REPO).parts[0] in ('Common', 'SU2_CFD'):
            assert current.get(filename) == digest and a.sha(source) == digest, source
            pinned[filename] = digest
    assert pinned
    return path, base_pins


def prepare(build):
    with patch.object(lab, 'HERE', inherited.c1.B):
        receipt, binary, pins = inherited.c3.prior.BASELINE_PREPARE(build)
    prior = production_pins(build, receipt)
    sources = (*order_sources(), *prior, Path(__file__), HERE / 'report_m1.py',
               C3 / 'run_c2_lab.py', C3 / 'compare_lab.py')
    pins.update({str(path): a.sha(path) for path in sources})
    return receipt, binary, pins


def shadow_input(case, config):
    entries = [line.split('=', 1)[1].split('%', 1)[0].strip().strip('"')
               for line in config.read_text().splitlines() if re.match(r'^\s*MESH_FILENAME\s*=', line)]
    filename, = entries
    path = (case / (filename + '.native_ref')).resolve()
    assert path.is_file(), path
    return path


LAB_COMMAND = lab.command


def command(case, binary, config, sensor, ranks, surface):
    recipe = LAB_COMMAND(case, binary, config, sensor, ranks, surface)
    index = recipe.index('nice')
    return recipe[:index] + [f'{OPTION}=1', f'{REFERENCE}={shadow_input(case, config)}', f'{TRACE}=0'] + recipe[index:]


def log_problems(case, ranks):
    problems = []
    for path in inherited.lab_logs(case, ranks):
        if not path.is_file():
            continue
        with lab.archived.open_log(path) as stream:
            for line in stream:
                row = a.ANSI.sub('', line).strip()
                if 'FP2_M1 ' not in row and (inherited.CONTACT_FAILURE in row or IN_PROCESS.search(row)):
                    problems.append({'log': str(path), 'line': row})
                    break
    return problems


def validate_observer(name, rows, proof):
    expected = set(range(CASES[name][0]))
    open_ranks = {int(rank) for rank, raw in rows.items()
                  if any(row.get('status', '').lower() == 'open' for marker in ('SHADOW', 'OBSERVER')
                         for row in raw.get(marker, ()))}
    shadow_open = open_ranks == expected
    issues = []
    if open_ranks and not shadow_open:
        issues.append({'row': 'SHADOW', 'open_ranks': sorted(open_ranks), 'expected': sorted(expected)})
    phases = ['1'] if name == 'N1' else ['1', '2']
    handover_calls = strict_observations = 0
    for rank in sorted(expected):
        raw = rows.get(str(rank), {})
        for phase in phases if not shadow_open else ():
            def table(marker):
                return [row for row in raw.get(marker, ())
                        if row.get('pass') == phase and row.get('rank') == str(rank)]
            checks = (
                ('OBSERVER', [row.get('status') for row in table('OBSERVER')], ['ok']),
                ('SUPPORT', sorted((row.get('stage'), row.get('interleaved')) for row in table('SUPPORT')),
                 [('1', '0'), ('2', '0')]),
                ('FALLBACK', sorted(row.get('group') for row in table('FALLBACK')), sorted(GROUPS)),
                ('MEMO', sorted((row.get('at'), row.get('authority')) for row in table('MEMO')),
                 sorted((at, authority) for at in ('start', 'end') for authority in ('pass', 'shadow'))),
                ('SUPPORT_BIN', {row.get('kind') for row in table('SUPPORT_BIN')}, {'length', 'fate'}),
            )
            for marker, actual, wanted in checks:
                if actual != wanted:
                    issues.append({'rank': rank, 'pass': int(phase), 'row': marker,
                                   'observed': sorted(actual) if isinstance(actual, set) else actual,
                                   'expected': sorted(wanted) if isinstance(wanted, set) else wanted})
        handover = [row for row in raw.get('HANDOVER', ()) if row.get('rank') == str(rank)]
        strict = raw.get('strict_observation', ())
        calls = handover[0].get('calls', '') if len(handover) == 1 else ''
        checked = strict[0].get('checked', '') if len(strict) == 1 else ''
        if not re.fullmatch(r'[0-9]+', calls) or not re.fullmatch(r'[0-9]+', checked):
            issues.append({'rank': rank, 'row': 'HANDOVER', 'observed': handover, 'strict_observation': strict})
        else:
            handover_calls += int(calls)
            strict_observations += int(checked)
    strict_checked = proof.get('strict_checked')
    if not (str(strict_checked).isdigit() and handover_calls == strict_observations == int(strict_checked)):
        issues.append({'row': 'HANDOVER', 'observed': handover_calls, 'strict_observations': strict_observations,
                       'strict_checked': strict_checked})
    status = 'FAILED' if issues else 'OPEN' if shadow_open else 'PASS'
    return {'status': status, 'problems': issues, 'open_ranks': sorted(open_ranks),
            'handover_calls': handover_calls, 'strict_observations': strict_observations,
            'strict_checked': strict_checked}


def case_path(label, name, root=HERE):
    return root / 'cases' / f'build{label}' / name


def verify_terminal():
    for root in (HERE, HERE / 'watchdog_retry', HERE / 'r5_retry', HERE / 'r5_retry/watchdog_retry'):
        for launch in root.glob('cases/build*/*/test_launch.json'):
            wait = a.resumed(launch.parent)
            assert wait and wait['terminal'], launch
            observed = cut.read(launch.with_name('owned_processes.json'))
            assert observed['all_observed_terminal'] and all(cut.terminal(row) for row in observed['identities'])


def attempt(build, name, root, prerequisite):
    ref_path, _ = baseline(name)
    seconds = TIMEOUTS[name]
    label = cut.read(build / 'build_status.json')['build']
    case = case_path(label, name, root)
    verify_terminal()
    inherited.verify_launches_terminal()

    def pinned(directory):
        receipt, binary, pins = prepare(directory)
        sources = (BASE_PROOF, ref_path, *prerequisite)
        pins.update({str(path): a.sha(path) for path in sources})
        return receipt, binary, pins

    def evaluate(case_name, data, expected):
        host = cut.read(case / 'test_host_preflight.json')
        return inherited.evaluate(case_name, data, expected, host['loadavg'])

    with patch.multiple(lab, HERE=root, RUNS=CASES, LINE=LINE, __file__=__file__, selector=inherited.selector,
                        SECONDS=seconds, prepare=pinned, evaluate=evaluate, command=command):
        result = lab.run(build, name)
    verify_terminal()
    return case, result


def watchdog(case, result, already_retried):
    if result['supervisor_stop'] != 'wrapper_time_limit':
        return {}
    failed = log_problems(case, result['ranks'])
    result_gates = result['measurement']['failed_limits']
    observed_fields = [item for rank, rows in result['rows_by_rank'].items()
                       for item in inherited.continuation_surface_problems(
                           int(rank), rows.get('NATIVE3D_LOCAL_SURFACE', ()))]
    observed_problems = [problem for problem in result['measurement']['problems']
                         if isinstance(problem.get('observed'), dict) and problem['observed']]
    memory_failed = result['peak_lower_bound_bytes'] > a.MEMORY * 1024
    status = 'result_failed' if failed or result_gates or observed_fields or observed_problems or memory_failed else (
        'open' if already_retried else 'retry_allowed')
    evidence = {'status': status, 'seconds_limit': TIMEOUTS[result['case']], 'failure_logs': failed,
                'failed_limits': result_gates, 'observed_field_failures': observed_fields,
                'observed_problems': observed_problems,
                'job_peak_exceeded': memory_failed, 'host_preflight': cut.read(case / 'test_host_preflight.json'),
                'measurement_sha256': a.sha(case / 'measurement.json')}
    a.sealed(case / 'watchdog.json', evidence)
    return evidence


def first_difference(diff, before, after):
    if not diff:
        return None
    first = {**diff[0]}
    rank, marker = first.get('rank'), first.get('row')
    if rank is not None and marker:
        key = comparison.compare_lab_c1.row_key(marker)
        index = max(first.get('index', 1) - 1, 0)
        candidates = [source.get(str(rank), {}).get(key, ()) for source in (after, before)]
        for rows in candidates:
            if len(rows) > index and 'pass' in rows[index]:
                first['pass'] = int(rows[index]['pass'])
                break
    first.setdefault('pass', 'not available')
    return first


def fixture_result(label, mode):
    return HERE / 'fixtures' / f'build{label}' / mode / 'case_summary.json'


def run_fixture(build, mode):
    receipt, binary, pins = prepare(build)
    label = receipt['build']
    prior, function = fixtures.hidden, fixtures.focus
    fingerprints = {key: receipt[key] for key in lab.FINGERPRINTS}
    prior.clear_options()
    ref = None
    if mode in OFF:
        cohort = OFF[mode]
        spec = fixtures.FOCUSED[cohort]
        ref = C3 / 'fixtures/build664' / cohort / 'attempt1/result.json'
        inherited.c3.c2.committed(BASE_FIXTURES, BASE)
        assert cut.read(BASE_FIXTURES)['artifacts'][str(ref)] == a.sha(ref), ref
        old = cut.read(ref)
        assert old['passed']
        lab.archived.verify_artifacts(ref.parent, old['artifacts'])
        pins.update({str(ref): a.sha(ref), str(BASE_FIXTURES): a.sha(BASE_FIXTURES)})
    else:
        spec = (1, '[.][NativeFP2M1Support]', 1)
        checker = HERE / 'check_fixture_trace.py'
        pins[str(checker)] = a.sha(checker)
    target = fixture_result(label, mode)
    launch = prior.c1.run_case
    env = {TRACE: '1'} if mode == 'fixture' else {TRACE: '0', OPTION: '0'}

    def launch_with_env(case, binary, ranks, selector, seconds):
        return launch(case, binary, ranks, selector, seconds, environment=env)

    with patch.object(prior.c1, 'run_case', launch_with_env):
        result, attempts = function(target.parent, binary, fingerprints, pins, spec)
    details = {'environment': env}
    if result['passed']:
        if ref:
            details['unchanged_assertion_counts'] = result['catch_summaries'] == old['catch_summaries']
            result['passed'] = details['unchanged_assertion_counts']
        else:
            log = Path(result['case']) / 'test_outer.log'
            trace = subprocess.run([sys.executable, '-B', str(checker), str(log)], capture_output=True, text=True)
            details.update(trace_returncode=trace.returncode, trace_stdout=trace.stdout, trace_stderr=trace.stderr)
            sidecar = log.with_name('fp2_m1_curved_strip.native_ref')
            if sidecar.is_file():
                details['generated_sidecar'] = {'path': str(sidecar), 'sha256': a.sha(sidecar)}
            result['passed'] = trace.returncode == 0
    status = 'PASS' if result['passed'] else 'OPEN' if result['status'] == 'open_r6' else 'FAILED'
    summary = {**fingerprints, 'status': status,
               'attempts': {str(path): a.sha(path) for path in attempts}, 'result': result, 'details': details}
    a.sealed(target, summary)
    return summary


def run_lab(build, name, correction):
    receipt = cut.read(build / 'build_status.json')
    label = receipt['build']
    prerequisites = [fixture_result(label, mode) for mode in (*OFF, 'fixture')]
    for path in prerequisites:
        status = cut.read(path)
        assert status['status'] == 'PASS', path
        assert all(status[key] == receipt[key] for key in lab.FINGERPRINTS), path
        for pinned, digest in status['attempts'].items():
            p = Path(pinned)
            assert a.sha(p) == digest
            lab.archived.verify_artifacts(p.parent, cut.read(p)['artifacts'])
    root = HERE
    if correction:
        assert cut.read(correction)['name'] == name and cut.read(correction)['status'] == 'FAILED'
        prerequisites.append(correction)
        root = HERE / 'r5_retry'
    first, trial = attempt(build, name, root, prerequisites)
    evidence = watchdog(first, trial, False)
    history, used = [first], first
    if evidence.get('status') == 'retry_allowed':
        used, trial = attempt(build, name, root / 'watchdog_retry',
                              (*prerequisites, first / 'watchdog.json', first / 'measurement.json'))
        evidence = watchdog(used, trial, True)
        history.append(used)
    observer, identity, first_diff = {}, None, None
    if trial['passed']:
        ref_path, old = baseline(name)
        before, after = old['rows_by_rank'], trial['rows_by_rank']
        diffs = comparison.repeat_differences(name, before, after)
        first_diff = first_difference(diffs, before, after)
        identity = {'passed': not diffs, 'baseline': str(ref_path), 'baseline_sha256': a.sha(ref_path),
                    'first_difference': first_diff, 'differences': diffs}
        observer = validate_observer(name, after, trial['measurement']['proof'])
        status = observer['status'] if identity['passed'] else 'FAILED'
    elif evidence.get('status') == 'open':
        status = 'OPEN'
    else:
        status = 'FAILED'
        first = (trial['measurement']['failed_limits'] or evidence.get('failure_logs') or
                 evidence.get('observed_field_failures') or evidence.get('observed_problems') or
                 trial['measurement']['problems'])
        first_diff = first[0] if first else {'supervisor_stop': trial['supervisor_stop'],
                                          'returncode': trial['wait']['returncode'],
                                          'peak_lower_bound_bytes': trial['peak_lower_bound_bytes']}
    summary = {'name': name, 'build': label, 'status': status, 'observer': observer, 'identity': identity,
               'first_difference': first_diff, 'watchdog': evidence, 'correction_of': str(correction) if correction else None,
               'used': str(used / 'measurement.json'),
               'attempts': {str(case / 'measurement.json'): a.sha(case / 'measurement.json') for case in history}}
    a.sealed(case_path(label, name, root) / 'case_summary.json', summary)
    return summary


def owned(case):
    assert os.environ.get(TRACE, '0') == '0', 'Campaign trace must stay off.'
    recipe = cut.read(case / 'command.json')
    name = case.name
    assert recipe['seconds_limit'] == TIMEOUTS[name]
    with patch.multiple(lab, RUNS=CASES, SECONDS=TIMEOUTS[name], selector=inherited.selector, command=command):
        return lab.owned(case)


def main():
    parser = ArgumentParser(description=__doc__)
    parser.add_argument('build', type=Path, nargs='?')
    mode = parser.add_mutually_exclusive_group(required=True)
    mode.add_argument('--owned', type=Path)
    mode.add_argument('--only', choices=(*OFF, 'fixture', *CASES))
    mode.add_argument('--self-check', action='store_true')
    parser.add_argument('--correction-of', type=Path)
    arguments = parser.parse_args()
    if arguments.owned:
        return owned(arguments.owned)
    if arguments.self_check:
        assert fixtures.hidden and [fixtures.FOCUSED[cohort][2] for cohort in OFF.values()] == [9, 3]
        for name in CASES:
            baseline(name)
        assert LINE.fullmatch('FP2_M1 SUPPORT rank=0 pass=1 stage=1 calls=3')
        assert not IN_PROCESS.search('ordinary observation')
        print('FP2-M1 historical trials, timeouts and row parser verified.')
        return 0
    if arguments.build is None:
        parser.error('a build directory is required')
    if arguments.only in CASES:
        result = run_lab(arguments.build.resolve(), arguments.only,
                         arguments.correction_of.resolve() if arguments.correction_of else None)
    else:
        result = run_fixture(arguments.build.resolve(), arguments.only)
    print({key: value for key, value in result.items() if key in ('name', 'build', 'status', 'first_difference')},
          flush=True)
    return {'PASS': 0, 'FAILED': 1, 'OPEN': 75}[result['status']]


if __name__ == '__main__':
    raise SystemExit(main())
