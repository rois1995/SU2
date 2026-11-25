"""Qualify the FP2-P3 OFF, CORE and S workflows with the inherited validators."""

from argparse import ArgumentParser
from pathlib import Path
import json
import re
import runpy
import sys
from tempfile import TemporaryDirectory
from unittest.mock import patch

HERE = Path(__file__).resolve().parent
NIGHT = HERE.parent
P2 = NIGHT / 'FP2_P2'
C1 = NIGHT / 'FP2_C1'
FX2 = NIGHT / 'FX2'
sys.path.insert(0, str(P2))
import run_s as inherited

from build_p3 import cut, verify_build

SELF = Path(__file__).resolve()
C3 = inherited.runner
BASE = C3.prior.BASE
workflow = inherited.workflow
sha = cut.sha
SWITCH = 'SU2_NATIVE_CONFORMING_WITNESS'
TRACE = 'SU2_NATIVE_GEOMETRY_WORK_TRACE'
MARKER = 'NATIVE_SURFACE_CONFORMING_WITNESS enabled=0'
ORDER = HERE / 'inputs/FP2_P3_CLOSURE_DEFAULT.md'
ORDER_SHA = '16fec22406c931490718e23e58da2840cd4bb05b1748189d6a12e14bff5c05e5'
OFF = FX2 / 'G5/build645/OFF/full645_run1'
P2_S = P2 / 'G5/build672/S/run_receipt.json'
MODES = {'B2b': 'OFF', 'B2c': 'CORE', 'C7': 'S', 'C5': 'OFF', 'C6': 'CORE'}
SHORT = ('B', 'C1', 'C2', 'C3', 'C4')
REQUIRED = {'B2b': ('B0', 'B1', 'B2a'), 'B2c': ('B0', 'B1', 'B2a', 'B2b')}
FINGERPRINTS = ('build', 'binary_sha256', 'test_binary_sha256', 'source_pins_sha256')


def root(label, part):
    return HERE / 'G5' / f'build{label}' / part


def settings(part):
    return {TRACE: '0', **({SWITCH: '0'} if part.startswith('B') else {})}


def exports(part):
    return [token for name in settings(part) for token in ('-x', name)]


def unset_and_bind(part):
    return f'env.pop({SWITCH!r}, None)\n    env.update({settings(part)!r})'


def config_for(mode):
    if mode == 'OFF':
        path = OFF / 'run.cfg'
        assert path.read_bytes() == BASE['config_for']('OFF').read_bytes()
        assert sha(path) == workflow.OFF_CONFIG_SHA
        return path
    if mode == 'CORE':
        path = BASE['config_for']('CORE')
        assert sha(path) == BASE['CORE_CONFIG_SHA']
        return path
    assert mode == 'S'
    return C3.prior.config_for('S')


def core_source(part, retry=False):
    with patch.object(workflow, 'core_source', inherited.ORIGINAL_CORE):
        source = C3.retry_controller() if retry else inherited.ORIGINAL_CORE()
    source = inherited.replace(source, '    env.update(clean_provenance["environment"])',
        f'    env.update(clean_provenance["environment"])\n    {unset_and_bind(part)}\n'
        f'    i = argv.index("mpirun") + 1\n    argv[i:i] = {exports(part)!r}')
    source = inherited.replace(source, 'if key in clean_provenance["environment"]',
        f'if key in clean_provenance["environment"] or key in {tuple(settings(part))!r}')
    if part == 'C5':
        # The section6_2 wrapper adds historical equality; retain the underlying assessor for C5.
        source = inherited.replace(source,
            '                p6_collapse[\'work_equals_P6c\'] = p6_collapse[\'upper_work_sum\'] == 90809869\n'
            '                p6_collapse[\'passed\'] = (p6_collapse[\'passed\'] and p6_collapse[\'count_prediction_matches\']\n'
            '                                        and p6_collapse[\'work_equals_P6c\'])',
            "                p6_collapse['count_difference_is_not_failure'] = False\n"
            "                p6_collapse['passed'] = (p6_collapse['passed'] and p6_collapse['count_prediction_matches'])")
        source += '\nP3_C5_ASSESS = assess_p6\n'
    return source


def restart_edits(part):
    return (
        ('argv = ["nice"', f'env.pop({SWITCH!r}, None)\nenv.update({settings(part)!r})\nargv = ["nice"'),
        ('"mpirun", "--oversubscribe"', '"mpirun", ' + ', '.join(map(repr, exports(part))) + ', "--oversubscribe"'),
        ('or k == "TMPDIR"', f'or k == "TMPDIR" or k in {tuple(settings(part))!r}'),
    )


def restart_source(part, name, label, build):
    template, source = inherited.ORIGINAL_RESTART(name, label, build)
    if name != 'run_restart':
        return template, source
    anchor = '            compile(source, str(BASE), "exec")\n'
    addition = f'            for old, new in {restart_edits(part)!r}:\n'
    addition += '                assert source.count(old) == 1, old\n'
    addition += '                source = source.replace(old, new, 1)\n'
    return template, inherited.replace(source, anchor, addition + anchor)


def campaign(part, inputs, retry=False):
    context = inherited.ORIGINAL_CAMPAIGN('on', inputs)
    context.update(core_source=lambda: core_source(part, retry),
                   restart_source=lambda *args: restart_source(part, *args))
    return context


def verify_generated(work, part, label, retry):
    full = (work / 'full_controller.py').read_text()
    restart = (work / f'run_restart{label}.py').read_text()
    assert unset_and_bind(part) in full and f'argv[i:i] = {exports(part)!r}' in full, work
    assert ('"application_watchdog_seconds": 43200' in full) == retry, work
    assert ('P3_C5_ASSESS = assess_p6' in full) == (part == 'C5'), work
    assert all(repr(new) in restart for _, new in restart_edits(part)), work


def launch_child(label, part, trial, retry):
    work = root(label, part) / f'trial{trial}'
    if retry:
        work /= C3.FULL_RETRY
    verify_generated(work, part, label, retry)
    old_runner = workflow.runner

    def bound_runner(*args):
        result = old_runner(*args)
        if part == 'C5':
            result['assess_p6'] = result['P3_C5_ASSESS']
        return result

    context = BASE['launch_full'].__globals__
    with patch.dict(context, {'config_for': config_for, '__file__': str(SELF), 'HERE': HERE}), \
            patch.object(workflow, 'runner', bound_runner):
        BASE['launch_full'](work, HERE / f'build{label}', MODES[part])


def verify_order():
    assert sha(ORDER) == ORDER_SHA
    assert ORDER.with_suffix('.sha256').read_text().split() == [ORDER_SHA, ORDER.name]
    C3.verify_orders()


def reference_run(path, expected_mode, build, payloads):
    row = cut.read(path)
    assert row['passed'] and row['mode'] == expected_mode
    assert all(row[key] == build[key] for key in FINGERPRINTS), path
    assert all(sha(path.parent / name) == digest for name, digest in row['artifacts'].items()), path
    assert payloads['payload_hashes'](Path(row['case'])) == row['hashes'], path
    assert len(row['hashes']) == 6
    return row


def historical(part, payloads):
    if part in ('B2b', 'C5'):
        label = FX2 / 'build645/build_status.json'
        receipt = OFF.parent / 'run_receipt.json'
        ref = reference_run(receipt, 'OFF', cut.read(label), payloads)
        expected = workflow.off_reference(payloads)
        assert ref['hashes'] == expected
        comparison = OFF.parent / 'full_comparison.json'
        assert cut.read(comparison)['passed']
        adopted = cut.read(workflow.N21)
        pins = [(workflow.N21.parent / adopted[key]['path']).resolve()
                for key in ('payloads', 'workflow_acceptance', 'timing_rule')]
        pins += [receipt.parent / name for name in ref['artifacts']]
        return expected, [label, receipt, comparison, workflow.N21, FX2 / 'compare_full.py',
                          FX2 / 'compare_full_self_check.json', *pins]
    if part in ('B2c', 'C6'):
        expected = BASE['core_reference'](payloads)
        return expected, [BASE['CORE_REFERENCE'], BASE['CORE_ACCEPTANCE']]
    assert part == 'C7'
    pins = inherited.reference(payloads)
    build = P2 / 'build672/build_status.json'
    ref = reference_run(P2_S, 'S', cut.read(build), payloads)
    return ref['hashes'], [*pins, build, P2_S, *(P2_S.parent / name for name in ref['artifacts'])]


def switch_receipt(part, case, provenance, full):
    record = cut.read(provenance)
    settings_expected = settings(part)
    env, argv = record['environment'], record['argv']
    assert env.get(SWITCH) == settings_expected.get(SWITCH), provenance
    if SWITCH not in settings_expected:
        assert SWITCH not in env, provenance
    assert env.get(TRACE) == '0', provenance
    assert 'mpirun' in argv and argv[argv.index('-n') + 1] == '6', provenance
    for key in (SWITCH, TRACE):
        expected = int(key in settings_expected)
        assert argv.count(key) == expected and sum(argv[i:i+2] == ['-x', key] for i in range(len(argv))) == expected
    logs = {int(path.parent.name.split('.')[-1]): path
            for path in case.glob('rank_output/*/rank.*/stderr')}
    assert set(logs) == set(range(6)), case
    rank_stdout = {rank: path.with_name('stdout') for rank, path in logs.items()}
    if full:
        gate = cut.read(case / 'final_source_first_collapse_gate.json')
        observed = {int(row['rank']) for row in gate['per_rank'] if int(row['upper_calls']) > 0}
        assert observed == set(range(6)), (case, observed)
        basis = {'path': str(case / 'final_source_first_collapse_gate.json'),
                 'sha256': sha(case / 'final_source_first_collapse_gate.json')}
    else:
        config = case / 'run.cfg'
        assert re.search(r'(?m)^ADAP_LOOP\s*=\s*NO\s*$', config.read_text()), config
        observed = set()
        basis = {'path': str(config), 'sha256': sha(config)}
    counts = {str(rank): path.read_text(errors='replace').count(MARKER) for rank, path in logs.items()}
    assert counts == {str(rank): int(rank in observed and SWITCH in settings_expected)
                      for rank in logs}, (provenance, counts)
    assert all(MARKER not in path.read_text(errors='replace') for path in rank_stdout.values()), case
    return {'effective_environment': {SWITCH: env.get(SWITCH), TRACE: env.get(TRACE)},
            'command': str(provenance), 'marker_counts': counts, 'entry_evidence': basis,
            'logs': {str(path): sha(path) for path in logs.values()}}


def compare_off(reference, candidate):
    compare = runpy.run_path(str(FX2 / 'compare_full.py'))
    return compare['compare_cases'](reference, candidate, 'off')


def authenticate_run(work, label, part, build, payloads):
    mode = MODES[part]
    with patch.dict(BASE['verify_run'].__globals__, {'config_for': config_for}):
        row = BASE['verify_run'](work, mode, build, payloads)
    assert all(row[key] == build[key] for key in FINGERPRINTS), work
    assert row['config_sha256'] == sha(config_for(mode)), work
    assert row['closure_variable'] == settings(part).get(SWITCH), work
    for evidence in row['switches'].values():
        assert all(sha(Path(name)) == digest for name, digest in evidence['logs'].items()), work
        basis = evidence['entry_evidence']
        assert sha(Path(basis['path'])) == basis['sha256'], work
    return row


def envelope(work, label, part, build, config, events, payloads, hashes, restart):
    case = work / f'full{label}_run1'
    restart_case = restart.parent
    switches = {
        'full': switch_receipt(part, case, case / 'provenance.json', full=True),
        'restart': switch_receipt(part, restart_case, restart_case / 'command.json', full=False),
    }
    artifacts = [work / name for name in ('helper_pins.json', 'full_launch.json', 'full_wait.json',
                                          'restart_launch.json', 'restart_wait.json')]
    artifacts += [case / name for name in ('provenance.json', 'status.json', 'full_workflow_receipt.json',
                   'n8_payload_identity.json', 'final_source_first_collapse_gate.json')]
    artifacts += sorted(case.glob('p7p8_event*_gate.json')) + sorted(case.glob('f3b_event*_timer_gate.json'))
    artifacts += sorted(case.glob('rank_output/*/rank.*/std*'))
    artifacts += [restart, restart_case / 'command.json', restart_case / 'run.cfg']
    artifacts += sorted(restart_case.glob('rank_output/*/rank.*/stderr'))
    manifest = work / 'restart_attempt1_watchdog/manifest.json'
    if manifest.is_file():
        artifacts += [manifest, work / f'run_restart{label}_1800.py',
                      work / 'restart_retry_start_load.json', work / 'restart_retry_timing.json']
    workflow.verify_helpers(work)
    for kind in ('binary', 'test_binary'):
        assert sha(Path(build[kind])) == build[f'{kind}_sha256']
    assert len(hashes) == 6 and payloads['payload_hashes'](case) == hashes
    workflow.sealed_write(work / 'run_receipt.json', {
        **{key: build[key] for key in FINGERPRINTS}, 'passed': True, 'mode': MODES[part], 'case': str(case),
        'closure_variable': settings(part).get(SWITCH), 'config': str(config), 'config_sha256': sha(config),
        'hashes': hashes, 'restart': str(restart), 'events': events, 'switches': switches,
        'rank_timers': inherited.rank_timers(case),
        'artifacts': {str(path.relative_to(work)): sha(path) for path in artifacts if path.is_file()},
    })
    return work / 'run_receipt.json'


def attempt(work, label, part, trial, build, sources, payloads, retry=False):
    config, mode = config_for(MODES[part]), MODES[part]
    build_dir = HERE / f'build{label}'
    case = work / f'full{label}_run1'
    if (work / 'run_receipt.json').is_file():
        authenticate_run(work, label, part, build, payloads)
        return work / 'run_receipt.json'
    campaign(part, sources, retry)['prepare'](work, label, build_dir, config)
    stage = 'full event gates'
    try:
        cmd = [cut.PYTHON, '-B', str(SELF), str(label), part, '--worker', str(trial)]
        if retry:
            cmd.append('--retry-full')
        try:
            workflow.run_stage(work, 'full', cmd)
        finally:
            C3.require_clean_contact_logs(work, case)
        BASE['status_ok'](case, build, config)
        if part == 'C5':
            collapse = cut.read(case / 'final_source_first_collapse_gate.json')
            assert collapse['passed'] and collapse['count_prediction_matches'], collapse
            assert collapse['zero_upper_refinements'] and collapse['upper_work_sum'] <= 108459426, collapse
        hashes = workflow.seal_run(case, build['binary_sha256'], config, payloads, mode != 'OFF')
        events = workflow.event_collector()(case)
        assert [row['event'] for row in events] == [1, 2]
        stage = 'isolated restart'
        restart = C3.isolate_with_retry(work, label, case)
        stage = 'sealing provenance'
        return envelope(work, label, part, build, config, events, payloads, hashes, restart)
    except (AssertionError, KeyError, OSError, RuntimeError, ValueError) as error:
        if not (work / 'failed_attempt.json').is_file():
            BASE['failure'](work, case, mode, stage, error)
        raise


def trial_run(label, part, trial, build, sources, payloads):
    work = root(label, part) / f'trial{trial}'
    try:
        return attempt(work, label, part, trial, build, sources, payloads)
    except (AssertionError, RuntimeError):
        if (work / 'restart_watchdog.json').is_file():
            raise C3.WatchdogOpen('Isolated restart reached both helper watchdogs.')
        if not C3.full_watchdog(work, label, C3.FULL_WATCHDOG):
            raise
    extra = [work / name for name in ('full_watchdog.json', 'failed_attempt.json', 'full_host_preflight.json')]
    retry = work / C3.FULL_RETRY
    try:
        return attempt(retry, label, part, trial, build, [*sources, *extra], payloads, retry=True)
    except (AssertionError, RuntimeError):
        if (retry / 'restart_watchdog.json').is_file():
            raise C3.WatchdogOpen('Isolated restart reached both helper watchdogs.')
        if C3.full_watchdog(retry, label, 3 * C3.FULL_WATCHDOG):
            raise C3.WatchdogOpen('Full workflow reached both helper watchdogs.')
        raise


def compare_hashes(expected, actual):
    assert len(expected) == len(actual) == 6 and set(expected) == set(actual)
    differences = {name: {'expected': expected[name], 'actual': actual[name]}
                   for name in sorted(expected) if expected[name] != actual[name]}
    return {'equal': not differences, 'differences': differences,
            'first_difference': next(({'file': name, **row} for name, row in differences.items()), None)}


def compare_s(paths, records):
    references = [inherited.OLD, P2_S, *paths]
    names = ['build664', 'build672', 'trial1', 'trial2']
    rows = [cut.read(path) for path in references[:2]] + records
    return {name: {'receipt': str(path), 'sha256': sha(path), 'events': inherited.event_measurements(row),
                   'rank_timers': inherited.rank_timers(Path(row['case'])),
                   'payload_hashes': row['hashes'], 'restart': row['restart']}
            for name, path, row in zip(names, references, rows, strict=True)}


def selected_runs(work):
    trials = [work / f'trial{i}' for i in (1, 2)]
    return [path for trial in trials for item in (trial, trial / C3.FULL_RETRY)
            if (path := item / 'run_receipt.json').is_file()]


def collapse_difference(part, status):
    if status.get('stop_reason') != 'final_source_first_collapse_gate_failed':
        return None
    gate = status['p6_check2']
    expected = gate['work_limit'] if part == 'C5' else 90809869
    work = gate['upper_work_sum']
    if work > expected or (part != 'C5' and work != expected):
        return {'field': 'upper_work_sum', 'expected': expected, 'candidate': work}
    for row in gate['per_rank']:
        accepted, required = int(row['length_collapses']), row['predicted_collapses']
        if accepted != required:
            return {'field': 'length_collapses', 'rank': int(row['rank']),
                    'expected': required, 'candidate': accepted}
        if int(row['upper_refinements']):
            return {'field': 'upper_refinements', 'rank': int(row['rank']),
                    'expected': 0, 'candidate': int(row['upper_refinements'])}
    return None


def run(label, part):
    import checkpoint
    verify_order()
    build = verify_build(label)
    work = root(label, part)
    work.mkdir(parents=True, exist_ok=True)
    payloads = runpy.run_path(str(workflow.PAYLOADS))
    prerequisites = checkpoint.require(label, *REQUIRED.get(part, SHORT))
    expected, prior_sources = historical(part, payloads)
    if part == 'C5':
        control, = selected_runs(root(label, 'B2b'))
        before = authenticate_run(control.parent, label, 'B2b', build, payloads)
        prior_sources += [control, *(control.parent / name for name in before['artifacts'])]
    sources = [SELF, ORDER, ORDER.with_suffix('.sha256'), HERE / 'reports/source_check.md',
               inherited.SELF, P2 / 'run_shared.py', C3.WATCHDOG_ORDER,
               C3.WATCHDOG_ORDER.with_suffix('.sha256'), Path(C3.prior.__file__).resolve(),
               Path(checkpoint.__file__), C3.DRIVER, C1 / 'retry_c1_restart.py',
               FX2 / 'compare_full.py', workflow.WRAPPER, config_for(MODES[part]),
               HERE / f'build{label}/build_status.json', HERE / f'build{label}/source_pins.json',
               *prerequisites, *prior_sources]
    sources = list(dict.fromkeys(sources))
    summary = {**{key: build[key] for key in FINGERPRINTS}, 'part': part, 'mode': MODES[part],
               'prerequisites': {str(path): sha(path) for path in prerequisites}, 'settings': settings(part)}
    try:
        paths = [trial_run(label, part, i, build, sources, payloads)
                 for i in range(1, (2 if part.startswith('B') else 3))]
        records = [authenticate_run(path.parent, label, part, build, payloads) for path in paths]
        first = records[0]
        historical_identity = compare_hashes(expected, first['hashes'])
        summary['historical_payloads'] = historical_identity
        if part == 'B2b':
            report = compare_off(OFF, Path(first['case']))
            compare_path = work / 'full_comparison.json'
            workflow.sealed_write(compare_path, json.loads(json.dumps(report)))
            summary['off_comparison'] = {'path': str(compare_path), 'sha256': sha(compare_path),
                                         'exact_difference_count': report['exact_difference_count'],
                                         'first_difference': next(iter(report['exact_differences']), None)}
        if part.startswith('B'):
            failure = historical_identity['first_difference']
            if not failure and part == 'B2b':
                failure = summary['off_comparison']['first_difference']
        else:
            summary['pair'] = compare_hashes(first['hashes'], records[1]['hashes'])
            failure = summary['pair']['first_difference']
            if part == 'C5':
                report = compare_off(Path(before['case']), Path(first['case']))
                compare_path = work / 'full_comparison.json'
                workflow.sealed_write(compare_path, json.loads(json.dumps(report)))
                summary['off_comparison'] = {'path': str(compare_path), 'sha256': sha(compare_path),
                                             'exact_difference_count': report['exact_difference_count'],
                                             'first_difference': next(iter(report['exact_differences']), None)}
            if part == 'C7':
                summary['s_report'] = compare_s(paths, records)
        summary.update(status='FAILED' if failure else 'passed', first_difference=failure,
                       failure_kind='result' if failure else None)
    except C3.WatchdogOpen as error:
        summary.update(status='OPEN', reason=str(error), failure_kind='helper_watchdog')
    except (AssertionError, KeyError, OSError, RuntimeError, ValueError) as error:
        failures = sorted(work.glob('trial*/**/failed_attempt.json'))
        detail = cut.read(failures[-1]) if failures else {}
        stage = detail.get('stage')
        case = Path(detail['case']) if detail else None
        status = detail.get('status', {})
        result = (stage == 'full event gates' and status.get('native_case_started')) or (
            stage == 'isolated restart' and case is not None and
            (case.parent / f'disk_restart_{case.name}_event2/status.json').is_file())
        first = collapse_difference(part, status) or detail.get('first_observed_failure')
        summary.update(status='FAILED', first_difference=first or str(error) or type(error).__name__,
                       failure_kind='result' if result else 'measurement')
    summary['runs'] = [{'receipt': str(path), 'sha256': sha(path)}
                       for path in selected_runs(work)]
    pins = {str(path): sha(path) for path in sorted(work.glob('trial*/**/failed_attempt.json'))}
    for selected in summary['runs']:
        receipt = Path(selected['receipt'])
        pins[str(receipt)] = selected['sha256']
        pins.update({str(receipt.parent / name): digest
                     for name, digest in cut.read(receipt)['artifacts'].items()})
    if 'off_comparison' in summary:
        item = summary['off_comparison']
        pins[item['path']] = item['sha256']
    summary['pins'] = pins
    output = work / 'case_summary.json'
    workflow.sealed_write(output, json.loads(json.dumps(summary)))
    print(f"{part} {summary['status']}: {output}", flush=True)
    return {'passed': 0, 'OPEN': 75, 'FAILED': 1}[summary['status']]


def self_check(label):
    verify_order()
    build = HERE / f'build{label}'
    for part, mode in MODES.items():
        with TemporaryDirectory(prefix='fp2-p3-full-') as folder:
            work = Path(folder)
            for retry in (False, True):
                attempt = work / str(retry)
                campaign(part, [SELF], retry)['prepare'](attempt, label, build, config_for(mode))
                verify_generated(attempt, part, label, retry)
                if (build / 'build_status.json').is_file():
                    namespace = runpy.run_path(str(attempt / f'run_restart{label}.py'))
                    script = namespace['runner_context']().load_base().retargeted_source()
                    assert all(new in script for _, new in restart_edits(part)), attempt
        print(f'{part} generated full and restart scripts checked', flush=True)


def main():
    parser = ArgumentParser(description=__doc__)
    parser.add_argument('label', type=int)
    parser.add_argument('part', nargs='?', choices=MODES)
    parser.add_argument('--worker', type=int, choices=(1, 2))
    parser.add_argument('--retry-full', action='store_true')
    parser.add_argument('--self-check', action='store_true')
    args = parser.parse_args()
    if args.self_check:
        self_check(args.label)
        return 0
    if not args.part or (args.retry_full and not args.worker):
        parser.error('specify a part, and use --retry-full only for a worker')
    if args.worker:
        launch_child(args.label, args.part, args.worker, args.retry_full)
        return 0
    return run(args.label, args.part)


if __name__ == '__main__':
    raise SystemExit(main())
