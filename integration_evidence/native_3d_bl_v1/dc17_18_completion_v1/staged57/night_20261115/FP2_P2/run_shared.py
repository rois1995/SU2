"""Run the five FP2-P2 shared-engine safeguards using their existing validators."""
from argparse import ArgumentParser
from pathlib import Path
import json
import os
import re
import runpy
import sys
import subprocess

from build_p2 import HERE, cut, verify_build

NIGHT = HERE.parent
R = NIGHT.parent
PURPOSE = R / 'purpose_query_fix_20261114'
LEGACY = PURPOSE / 'p6c_20261115'
FIRST = PURPOSE / 'run_p5_first_collapse_gate.py'
REMAINING = LEGACY / 'run_remaining_checks.py'
MONITOR = NIGHT / 'FP2_C1/scoped/build647/safeguards/run_build647_safeguards.py'
SHARED = ('first-collapse', 'exact-c492', 'merit481', 'saved-current', 'native2d')
SWITCH = 'SU2_NATIVE_CONFORMING_WITNESS'
TRACE = 'SU2_NATIVE_GEOMETRY_WORK_TRACE'
MARKER = 'NATIVE_SURFACE_CONFORMING_WITNESS enabled=1'
LIMIT = 19073 * 1024**2
SELF = Path(__file__).resolve()
SAVED_REFERENCE = NIGHT / 'N4/section6_3_after_N11/phase_N6/safeguards/acceptance_summary.json'


def replace(source, old, new):
    assert source.count(old) == 1, old
    return source.replace(old, new, 1)


def environment(closure):
    return {SWITCH: str(closure), TRACE: '0'}


def exports(settings):
    return [item for name in settings for item in ('-x', name)]


def first(root, build, closure):
    source = FIRST.read_text()
    source = replace(source, 'assert case != PURPOSE and PURPOSE in case.parents, "case_dir must be a child of purpose_query_fix_20261114"',
                     'assert case == OUTPUT')
    source = replace(source, '    env.update(clean_provenance["environment"])',
                     '    env.update(clean_provenance["environment"])\n    env.update(SETTINGS)\n'
                     '    i = argv.index("mpirun") + 1\n    argv[i:i] = EXPORTS')
    source = replace(source, 'if key in clean_provenance["environment"]', 'if key in clean_provenance["environment"] or key in SETTINGS')
    sys.argv = [str(FIRST), str(root / 'case')]
    additional = root / 'additional_source_pins.json'
    inherited = cut.read(LEGACY / 'unchanged_build494_source_pins.json')
    selected = cut.read(build / 'source_pins.json')
    # The historical extra manifest can pin an older build of the touched source.
    cut.save(additional, {**{name: digest for name, digest in inherited.items() if name not in selected},
                          str(SELF): cut.sha(SELF)})
    ns = {'__name__': 'p1_first', '__file__': str(FIRST), 'OUTPUT': root / 'case',
          'SETTINGS': environment(closure), 'EXPORTS': exports(environment(closure))}
    exec(compile(source, str(FIRST), 'exec'), ns)
    ns.update(BUILD_STATUS=build / 'build_status.json', SOURCE_PINS=build / 'source_pins.json', ADDITIONAL_PINS=additional)
    ns['main']()


def other(root, build, mode, closure):
    sys.path.insert(0, str(R))
    import run_cases as checks
    settings = environment(closure)
    status = cut.read(build / 'build_status.json')
    monitor = runpy.run_path(str(MONITOR))
    monitor['install_run_monitor'].__globals__['HERE'] = root
    original = monitor['install_run_monitor'](status['test_binary'])
    monitored = subprocess.Popen
    save = checks.save

    def command(path, record):
        if path.name == 'command.json':
            record = {**record, 'argv': list(record['argv']), 'environment': {**record['environment'], **settings}}
            if 'mpirun' in record['argv']:
                record['argv'][record['argv'].index('mpirun') + 1:record['argv'].index('mpirun') + 1] = ['--tag-output', *exports(settings)]
        if path.name == 'pins.json':
            record.update({str(item): cut.sha(item) for item in (SELF, MONITOR, REMAINING)})
        save(path, record)

    def popen(argv, *args, **kwargs):
        if isinstance(argv, (tuple, list)) and status['test_binary'] in argv and argv[0] != 'ldd':
            argv = list(argv)
            kwargs['env'] = {**kwargs.get('env', os.environ), **settings}
            if 'mpirun' in argv:
                pos = argv.index('mpirun') + 1
                argv[pos:pos] = ['--tag-output', *exports(settings)]
        return monitored(argv, *args, **kwargs)

    source = REMAINING.read_text()
    source = replace(source, 'HERE=Path(__file__).resolve().parent;R=HERE.parent.parent;REPO=R.parents[3]',
                     'HERE=OUTPUT;R=STAGED;REPO=R.parents[3]')
    source = replace(source, 'for p in Path(\'/proc\').glob(\'[0-9]*\'):', 'for p in ():')
    source = replace(source, "gate_path=HERE/'check_p6_bl.py'", "gate_path=LEGACY/'check_p6_bl.py'")
    source = source.replace("HERE.parent/'check_bl_regression.py'", "LEGACY.parent/'check_bl_regression.py'")
    source = source.replace("HERE.parent/'bl_output_comparison.json'", "LEGACY.parent/'bl_output_comparison.json'")
    source = source.replace("R/'build498_", "HERE/'build498_")
    source = source.replace("'build498'", repr(build.name))
    source = source.replace("build498_", f"{build.name}_")
    source = replace(source, "module.RESULT=HERE/'geometry_merit496_serial_assessment.json'",
                     "module.RESULT=HERE/'geometry_merit496_serial_assessment.json'\n"
                     " import inspect\n"
                     " definition=inspect.getsource(module.main).replace(\"(R / 'cases')\",\"CASE_ROOT\")\n"
                     " module.CASE_ROOT=HERE/'cases';exec(compile(definition,str(spec.origin),'exec'),module.__dict__)")
    source = replace(source, "exec(compile(text,str(path),'exec'),{'__name__':'__main__','__file__':str(path)})",
                     "text=text.replace(\"(R/'cases').glob(case+'_*')\",\"CASE_ROOT.glob(case+'_*')\")\n"
                     " text=text.replace(\"(R/'" + build.name + "_terminal_handles.json')\",\"(OUTPUT/'" + build.name + "_terminal_handles.json')\")\n"
                     " exec(compile(text,str(path),'exec'),{'__name__':'__main__','__file__':str(path),"
                     "'CASE_ROOT':HERE/'cases','OUTPUT':HERE})")
    source = replace(source, "gate=importlib.util.module_from_spec(spec);spec.loader.exec_module(gate)",
                     "gate_source=gate_path.read_text();gate=importlib.util.module_from_spec(spec)\n"
                     "   gate_source=gate_source.replace(\"k!='SU2_NATIVE_FROZEN_CONFIG'\","
                     "\"k not in ('SU2_NATIVE_FROZEN_CONFIG','SU2_NATIVE_CONFORMING_WITNESS','SU2_NATIVE_GEOMETRY_WORK_TRACE')\")\n"
                     "   exec(compile(gate_source,str(gate_path),'exec'),gate.__dict__)")
    # Legacy helpers expect their build aliases beside the fresh output.
    cut.save(root / 'build_status.json', status)
    cut.save(root / 'source_pins.json', cut.read(build / 'source_pins.json'))
    checks.ROOT = root
    checks.save = command
    sys.argv = [str(REMAINING), mode]
    subprocess.Popen = popen
    try:
        exec(compile(source, str(REMAINING), 'exec'), {'__name__': '__main__', '__file__': str(REMAINING),
             'OUTPUT': root, 'STAGED': R, 'LEGACY': LEGACY})
    finally:
        subprocess.Popen = original
        checks.save = save


def markers(logs, closure):
    entered, absent, mismatches = [], [], []
    for rank, path in sorted(logs.items()):
        count = 0
        with path.open(errors='replace') as stream:
            for line in stream:
                count += line.count(MARKER)
        (entered if count else absent).append(rank)
        if count not in ((0, 1) if closure else (0,)):
            mismatches.append({'rank': rank, 'count': count})
    return {'entered': entered, 'no_entry_marker': absent, 'mismatches': mismatches,
            'paths': {str(rank): str(path) for rank, path in logs.items()}}


def caught(log, ranks, closure):
    counts = dict.fromkeys(range(ranks), 0)
    assertions = []
    with log.open(errors='replace') as stream:
        for line in stream:
            tag = re.match(r'\[\d+,(\d+)\]<[^>]+>:', line)
            if MARKER in line:
                counts[int(tag[1]) if tag else 0] += line.count(MARKER)
            if 'All tests passed' in line:
                assertions.append(line.strip())
    return {'entered': [r for r, n in counts.items() if n], 'no_entry_marker': [r for r, n in counts.items() if not n],
            'mismatches': [{'rank': r, 'count': n} for r, n in counts.items() if n not in ((0, 1) if closure else (0,))],
            'catch': assertions, 'path': str(log)}


WORK_LIMIT = 108459426
DEADLINE = 1500
RANKS = set(range(6))


def first_checks(rows, peaks, reason, stopped, wrapper):
    complete = set(rows) == RANKS
    work = sum(int(row['upper_work']) for row in rows.values())
    refinements = {rank: int(row['upper_refinements']) for rank, row in rows.items()
                   if 'upper_refinements' in row}
    memory = sum(peaks.values())
    failures = {}
    if work > WORK_LIMIT:
        failures['work'] = {'observed': work, 'limit': WORK_LIMIT, 'partial': not complete}
    refined = next(((rank, value) for rank, value in refinements.items() if value), None)
    if refined:
        failures['refinements'] = {'rank': refined[0], 'observed': refined[1], 'expected': 0}
    if memory > LIMIT:
        failures['resource'] = {'observed_bytes': memory, 'limit_bytes': LIMIT,
                                'partial': not (complete and len(peaks) == 6)}
    if reason == '1500_second_deadline' or (not complete and stopped >= DEADLINE):
        failures['deadline'] = {'observed_seconds': stopped, 'limit_seconds': DEADLINE}
    judged = {'work': complete, 'refinements': set(refinements) == RANKS,
              'resource': complete and len(peaks) == 6 and all(peaks.values()),
              'deadline': complete and reason == 'all_six_first_collapse_exit_rows' and not wrapper}
    checks = {name: 'FAILED' if name in failures else 'passed' if evaluated else 'not evaluated'
              for name, evaluated in judged.items()}
    return checks, failures


def evaluate_first(root, closure, wait):
    case = root / 'case'
    raw = case / 'status.json'
    result = cut.read(raw)
    provenance = cut.read(case / 'provenance.json')
    assert all(provenance['environment'].get(k) == v for k, v in environment(closure).items())
    assert provenance['max_work_units'] == WORK_LIMIT
    assert provenance['application_watchdog_seconds'] == 14400
    assert 'or 1500 seconds from process launch' in provenance['stop_rule']
    gate = result.get('first_collapse_gate')
    terminal = result['state'] == 'terminal'
    records = case / 'collapse_records.json'
    if records.is_file():
        rows = {int(k): row for k, row in cut.read(records).items()}
        if gate:
            assert gate['captured_ranks'] == sorted(rows) and gate['upper_work_limit'] == WORK_LIMIT
            assert gate['upper_work_sum'] == sum(int(row['upper_work']) for row in rows.values())
    else:
        assert not gate or not gate['captured_ranks']
        rows = runpy.run_path(str(FIRST))['collapse_rows'](case)

    logs = {int(p.parent.name.split('.')[-1]): p for p in case.glob('rank_output/*/rank.*/stderr')}
    receipt = markers(logs, closure)
    peaks, ids = {}, {}
    stopped = result.get('seconds_until_stop', 0)
    with (case / 'host_resources.jsonl').open() as stream:
        for line in stream:
            if not line.endswith(chr(10)):
                continue
            sample = json.loads(line)
            if not terminal:
                stopped = max(stopped, sample['elapsed'])
            for row in sample['owned_processes']:
                identity = (row['pid'], row['start_ticks'])
                ids[identity] = row
                if row['comm'] == 'SU2_CFD':
                    peaks[identity] = max(peaks.get(identity, 0), row.get('VmHWM', 0) * 1024, row.get('VmRSS', 0) * 1024)
    complete = set(rows) == RANKS
    reason = result.get('stop_reason')
    handles_ok = wait['terminal'] and cut.terminal(wait['controller']) and all(cut.terminal(row) for row in ids.values())
    if terminal:
        handles_ok = (handles_ok and result['all_owned_handles_terminal']
                      and all(cut.terminal(row) for row in result['owned_identities']))
    pin_ok = (result['inputs_unchanged'] and result['source_and_binary_pins_unchanged'] if terminal else
              all(cut.sha(path) == digest for path, digest in provenance['input_hashes'].items()))
    wrapper_footer = case / 'outer.time.txt'
    argv = provenance['argv']
    wrapper = (not complete and handles_ok and pin_ok and argv[argv.index('timeout') + 1] == '14400'
               and wrapper_footer.is_file() and 'Exit status: 124' in wrapper_footer.read_text()
               and (result.get('exit_code') == 124 or wait['returncode'] == 124))
    failure_text = re.compile(r'NATIVE_SURFACE_REFINE |Curved atlas (?:work|storage) exhausted|adaptation rejected', re.I)
    observed = next(({'field': 'native output', 'path': str(path), 'line': number, 'observed': line.strip()}
                     for path in logs.values() for number, line in enumerate(path.open(errors='replace'), 1)
                     if failure_text.search(line)), None)
    checks, failures = first_checks(rows, peaks, reason, stopped, wrapper)
    failed = next(({'field': name, **value} for name, value in failures.items()), None)
    failed = failed or observed or ('owned handles terminal' if not handles_ok else None)
    failed = failed or ('inputs and source/binary pins' if not pin_ok else None)
    failed = failed or ('switch markers' if receipt['mismatches'] or (complete and set(logs) != RANKS) else None)
    if not wrapper:
        failed = failed or ('native case started' if not terminal or not result['native_case_started'] else None)
        failed = failed or ('first-collapse completion' if not complete or reason != 'all_six_first_collapse_exit_rows' else None)

    count_reference = cut.read(R / 'shared_engine_accepted_count_reference.json')
    expected = gate['accepted_by_rank_expected'] if gate else None
    edits = next(({'field': 'accepted_by_rank', 'rank': rank, 'expected': expected[rank],
                   'actual': int(row['accepted'])} for rank, row in sorted(rows.items())
                  if expected is not None and int(row['accepted']) != expected[rank]), None)
    assert count_reference['upper_work_limit'] == WORK_LIMIT
    missed = edits if closure else None
    failed = failed or (edits if not closure else None)
    if not wrapper and not (edits and closure and wait['returncode'] == 1) and wait['returncode']:
        failed = failed or 'controller exit'
    receipt.update(hard_checks=checks, hard_failures=failures, wrapper_only=bool(wrapper and not failed),
                   captured_ranks=sorted(rows), stop_reason=reason, seconds_until_stop=stopped,
                   work_limit=WORK_LIMIT, observed_work=sum(int(row['upper_work']) for row in rows.values()),
                   observed_solver_peak_sum_bytes=sum(peaks.values()),
                   observed_solver_peak_by_pid_bytes={str(pid): value for (pid, _), value in peaks.items()},
                   resource_limit_bytes=LIMIT, procfs_unit='KiB', bytes_per_kib=1024,
                   resource_complete=len(peaks) == 6 and all(peaks.values()) and complete)
    return raw, receipt, failed, missed


def evaluate(root, mode, closure, wait):
    if mode == 'first-collapse':
        return evaluate_first(root, closure, wait)
    case, = (root / 'cases').glob('*')
    raw = case / 'status.json'
    r = cut.read(raw)
    command = cut.read(case / 'command.json')
    receipt = caught(case / 'run.log', command['ranks'], closure)
    assert all(command['environment'][k] == v for k, v in environment(closure).items())
    monitor = cut.read(case / 'safeguard_process_receipt.json')
    failed = next((k for k, ok in (
        ('test exit', r['exit_code'] == 0), ('pins_verified', r['pins_verified']),
        ('owned handles terminal', monitor['state'] == 'terminal' and not monitor['live_owned_descendants']),
        ('job peak', monitor['peak_sampled_tree_rss_bytes'] <= LIMIT),
        ('switch markers', not receipt['mismatches'])) if not ok), None)
    missed = None
    if mode == 'bl':
        raw = case / 'p6_bl_receipt.json'
        if raw.is_file():
            bl = cut.read(raw)
            failed = failed or next((k for k, ok in bl['gates'].items() if not ok), None)
        else:
            raw = case / 'status.json'
            failed = failed or 'missing exact C492 validator result'
    if mode in ('merit481', 'saved_current') and not failed and not wait['returncode']:
        old = cut.read(SAVED_REFERENCE)['safeguards'][mode]
        reference = Path(old['case'])
        sys.path.insert(0, str(NIGHT / 'N16'))
        from run_c import compare_safeguard
        compared = compare_safeguard(mode, reference, case)
        assert all(row['reference'] == old['payloads'][name]['candidate'] for name, row in compared['payloads'].items())
        raw = root / 'historical_comparison.json'
        cut.save(raw, compared)
        if not compared['exact'] and (closure or compared['require_identity']):
            first = next(({'field': name, **row} for name, row in compared['payloads'].items()
                          if row['reference'] != row['candidate']), {'field': 'actual_bl_cavity_planning.jsonl'})
            if closure:
                missed = first
            else:
                failed = first
    if not failed and wait['returncode']:
        failed = 'controller exit'
    return raw, receipt, failed, missed


def run(label, closure, group):
    verify_build(label)
    build = HERE / f'build{label}'
    if closure:
        from seal_d1 import verify_d1
        assert verify_d1(build)[1]['promote']
        if group != 'first-collapse':
            from seal_d2 import verify_d2
            assert verify_d2(build)[1]['promote']
    root = HERE / 'shared' / build.name / f'closure{closure}' / group
    root.mkdir(parents=True, exist_ok=True)
    plan = {'first-collapse': ['first-collapse'], 'exact-c492': ['bl'], 'merit481': ['merit481'],
            'saved-current': ['saved_current'], 'native2d': ['2d1', '2d2', '2d4']}
    results = {}
    for mode in plan[group]:
        attempt = root / mode
        attempt.mkdir()
        try:
            wait = cut.run_gate(attempt, 'controller', [sys.executable, '-B', str(SELF), str(label), str(closure), group,
                                                        '--worker', mode])
        except RuntimeError:
            wait = cut.read(attempt / 'controller_wait.json')
        try:
            raw, receipt, failed, missed = evaluate(attempt, mode, closure, wait)
        except (OSError, KeyError, ValueError) as exc:
            raw, receipt, failed, missed = attempt / 'controller_outer.log', {}, f'missing evaluation: {exc}', None
        incomplete = mode == 'first-collapse' and any(value == 'not evaluated' for value in receipt.get('hard_checks', {}).values())
        part = 'FAILED' if failed else 'OPEN' if missed or incomplete else 'passed'
        results[mode] = {'status': part, 'raw': str(raw), 'raw_sha256': cut.sha(raw), 'first_failure': failed,
                         'missed_comparison': missed, 'switch_receipt': receipt, 'wait': wait}
    verify_build(label)
    fingerprint = {key: cut.read(build / 'build_status.json')[key] for key in
                   ('build', 'binary_sha256', 'test_binary_sha256', 'source_pins_sha256')}
    files = {Path(part['raw']) for part in results.values()}
    files.update((SELF, FIRST, REMAINING, MONITOR, SAVED_REFERENCE, NIGHT / 'N16/run_c.py', build / 'build_status.json', build / 'source_pins.json'))
    files.update(root.glob('*/controller_*.json'))
    files.update(root.glob('*/cases/*/status.json'))
    files.update(root.glob('*/cases/*/pins.json'))
    files.update(root.glob('*/cases/*/command.json'))
    files.update(root.glob('*/cases/*/safeguard_process_receipt.json'))
    files.update(root.glob('*/case/provenance.json'))
    if group == 'first-collapse':
        for pattern in ('host_resources.jsonl', 'outer.time.txt', '*collapse_records.json', 'rank_output/*/rank.*/std*'):
            files.update(root.glob(f'first-collapse/case/{pattern}'))
    if closure:
        files.add(build / 'd1_promotion.json')
        if group != 'first-collapse':
            files.add(build / 'd2_promotion.json')
    pins = {str(path): cut.sha(path) for path in sorted(files)}
    status = 'FAILED' if any(p['status'] == 'FAILED' for p in results.values()) else (
        'OPEN' if any(p['status'] == 'OPEN' for p in results.values()) else 'passed')
    cut.save(root / 'case_summary.json', {**fingerprint, 'status': status, 'environment': environment(closure), 'pins': pins,
                                         'subruns': results})
    print(group, status, flush=True)
    return 1 if status == 'FAILED' else 0


if __name__ == '__main__':
    parser = ArgumentParser(description=__doc__)
    parser.add_argument('build', type=int)
    parser.add_argument('closure', type=int, choices=(0, 1))
    parser.add_argument('group', choices=SHARED)
    parser.add_argument('--worker')
    args = parser.parse_args()
    if args.worker:
        build = HERE / f'build{args.build}'
        root = HERE / 'shared' / build.name / f'closure{args.closure}' / args.group / args.worker
        if args.worker == 'first-collapse':
            first(root, build, args.closure)
        else:
            other(root, build, args.worker, args.closure)
    else:
        raise SystemExit(run(args.build, args.closure, args.group))
