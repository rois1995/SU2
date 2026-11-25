"""Run FP2-P3 shared safeguards with frozen inputs and the inherited validators."""
from argparse import ArgumentParser
from pathlib import Path
from importlib.util import module_from_spec, spec_from_file_location
import inspect
import json
import os
import re
import runpy
import sys
import subprocess
import tempfile

from build_p3 import HERE, cut, verify_build

NIGHT = HERE.parent
R = NIGHT.parent
PURPOSE = R / 'purpose_query_fix_20261114'
LEGACY = PURPOSE / 'p6c_20261115'
FIRST = PURPOSE / 'run_p5_first_collapse_gate.py'
REMAINING = LEGACY / 'run_remaining_checks.py'
MONITOR = NIGHT / 'FP2_C1/scoped/build647/safeguards/run_build647_safeguards.py'
SHARED = ('first-collapse', 'exact-c492', 'merit481', 'saved481', 'saved-current', 'native2d')
SWITCH = 'SU2_NATIVE_CONFORMING_WITNESS'
TRACE = 'SU2_NATIVE_GEOMETRY_WORK_TRACE'
MARKER = 'NATIVE_SURFACE_CONFORMING_WITNESS enabled=0'
LIMIT = 19073 * 1024**2
SELF = Path(__file__).resolve()
ORDER = HERE / 'inputs/FP2_P3_CLOSURE_DEFAULT.md'
SAVED_REFERENCE = NIGHT / 'N4/section6_3_after_N11/phase_N6/safeguards/acceptance_summary.json'


def replace(source, old, new):
    assert source.count(old) == 1, old
    return source.replace(old, new, 1)


def environment(closure):
    return {TRACE: '0'} if closure else {SWITCH: '0', TRACE: '0'}


def exports(settings):
    return [item for name in settings for item in ('-x', name)]


def effective(base, closure):
    result = dict(base)
    result.pop(SWITCH, None)
    result.update(environment(closure))
    return result


def first(root, build, closure):
    source = FIRST.read_text()
    source = replace(source, 'assert case != PURPOSE and PURPOSE in case.parents, "case_dir must be a child of purpose_query_fix_20261114"',
                     'assert case == OUTPUT')
    source = replace(source, '    env.update(clean_provenance["environment"])',
                     '    env.update(clean_provenance["environment"])\n    env.pop(SWITCH, None)\n    env.update(SETTINGS)\n'
                     '    i = argv.index("mpirun") + 1\n    argv[i:i] = EXPORTS')
    source = replace(source, 'if key in clean_provenance["environment"]', 'if key in clean_provenance["environment"] or key in SETTINGS')
    sys.argv = [str(FIRST), str(root / 'case')]
    additional = root / 'additional_source_pins.json'
    inherited = cut.read(LEGACY / 'unchanged_build494_source_pins.json')
    selected = cut.read(build / 'source_pins.json')
    # The historical extra manifest can pin an older build of the touched source.
    cut.save(additional, {**{name: digest for name, digest in inherited.items() if name not in selected},
                          **hashes((SELF, ORDER, ORDER.with_suffix('.sha256'), *prerequisites(build, closure)))})
    ns = {'__name__': 'p1_first', '__file__': str(FIRST), 'OUTPUT': root / 'case',
          'SWITCH': SWITCH, 'SETTINGS': environment(closure), 'EXPORTS': exports(environment(closure))}
    exec(compile(source, str(FIRST), 'exec'), ns)
    ns.update(BUILD_STATUS=build / 'build_status.json', SOURCE_PINS=build / 'source_pins.json', ADDITIONAL_PINS=additional)
    ns['main']()


def other_source(build):
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
                     "gate=LOAD_BL(SECONDS)")
    source = replace(source, "gate.self_check();raise SystemExit(gate.main(bl_case[0]))",
                     "raise SystemExit(gate.main(bl_case[0]))")
    return source


def other(root, build, mode, closure, seconds):
    sys.path.insert(0, str(R))
    import run_cases as checks
    settings = environment(closure)
    status = cut.read(build / 'build_status.json')
    monitor = runpy.run_path(str(MONITOR))
    monitor['install_run_monitor'].__globals__['HERE'] = root
    original = monitor['install_run_monitor'](status['test_binary'])
    monitored = subprocess.Popen
    save = checks.save

    def bind_argv(argv):
        bound = list(argv)
        i = bound.index('timeout') + 1
        assert bound[i] == '600', argv
        bound[i] = str(seconds)
        if 'mpirun' in bound:
            i = bound.index('mpirun') + 1
            bound[i:i] = ['--tag-output', *exports(settings)]
        return bound

    def command(path, record):
        if path.name == 'command.json':
            record = {**record, 'argv': bind_argv(record['argv']),
                      'environment': effective(record['environment'], closure), 'timeout_seconds': seconds,
                      'closure_setting': 'absent' if closure else '0'}
        if path.name == 'pins.json':
            record.update(hashes((SELF, MONITOR, REMAINING, ORDER, ORDER.with_suffix('.sha256'),
                                  *prerequisites(build, closure))))
        save(path, record)

    def popen(argv, *args, **kwargs):
        if isinstance(argv, (tuple, list)) and status['test_binary'] in argv and argv[0] != 'ldd':
            argv = bind_argv(argv)
            kwargs['env'] = effective(kwargs.get('env', os.environ), closure)
        return monitored(argv, *args, **kwargs)

    source = other_source(build)
    # Legacy helpers expect their build aliases beside the fresh output.
    cut.save(root / 'build_status.json', status)
    cut.save(root / 'source_pins.json', cut.read(build / 'source_pins.json'))
    checks.ROOT = root
    checks.save = command
    sys.argv = [str(REMAINING), mode]
    subprocess.Popen = popen
    try:
        exec(compile(source, str(REMAINING), 'exec'), {'__name__': '__main__', '__file__': str(REMAINING),
             'OUTPUT': root, 'STAGED': R, 'LEGACY': LEGACY, 'LOAD_BL': load_bl_guard, 'SECONDS': seconds})
    finally:
        subprocess.Popen = original
        checks.save = save


def hashes(paths):
    return {str(path): cut.sha(path) for path in paths}


def prerequisites(build, closure):
    from checkpoint import require

    label = int(build.name.removeprefix('build'))
    if closure:
        return require(label, 'B')
    return require(label, 'B0', 'B1')


def command_check(record, closure, ranks, seconds):
    argv = record['argv']
    env = record['environment']
    assert env.get(TRACE) == '0'
    assert SWITCH not in env if closure else env.get(SWITCH) == '0'
    assert str(seconds) == argv[argv.index('timeout') + 1], argv
    if 'timeout_seconds' in record:
        assert record['timeout_seconds'] == seconds
    if 'ranks' in record:
        assert record['ranks'] == ranks
    if ranks > 1:
        assert 'mpirun' in argv and argv[argv.index('-n', argv.index('mpirun')) + 1] == str(ranks)
        assert argv.count(TRACE) == 1 and argv[argv.index(TRACE) - 1] == '-x'
        assert argv.count(SWITCH) == (0 if closure else 1)
        if not closure:
            assert argv[argv.index(SWITCH) - 1] == '-x'
    else:
        assert 'mpirun' not in argv
    return {'closure_setting': 'absent' if closure else '0', 'trace_setting': '0',
            'ranks': ranks, 'timeout_seconds': seconds}


def load_bl_guard(seconds):
    assert seconds in (600, 1800), seconds
    path = LEGACY / 'check_p6_bl.py'
    spec = spec_from_file_location('p3_c492_guard', path)
    guard = module_from_spec(spec)
    source = replace(path.read_text(), "k!='SU2_NATIVE_FROZEN_CONFIG'",
                     "k not in ('SU2_NATIVE_FROZEN_CONFIG', "
                     "'SU2_NATIVE_CONFORMING_WITNESS', 'SU2_NATIVE_GEOMETRY_WORK_TRACE')")
    source = replace(source, "(case/'run.log').read_text", "projected_log(case/'run.log').read_text")
    guard.projected_log = projected_log
    exec(compile(source, str(path), 'exec'), guard.__dict__)
    guard.self_check()
    definition = inspect.getsource(guard.prior.conditions)
    definition = replace(definition, 'command.get("timeout_seconds") == 600',
                         f'command.get("timeout_seconds") == {seconds}')
    exec(compile(definition, guard.prior.__file__, 'exec'), guard.prior.__dict__)
    return guard


def projected_log(path):
    """Remove only an exact marker inserted inside a stdout line, keeping the raw bytes."""
    raw = path.read_bytes()
    marker = (MARKER + chr(10)).encode()
    if raw.count(marker) != 1:
        return path
    offset = raw.index(marker)
    if not offset or raw[offset - 1] == 10:
        return path
    derived = path.with_name('stdout_without_interleaved_marker.log')
    data = raw[:offset] + raw[offset + len(marker):]
    proof = path.with_name('stdout_rejoined.json')
    if derived.exists():
        assert derived.read_bytes() == data, derived
    else:
        derived.write_bytes(data)
    if not proof.exists():
        cut.save(proof, {'raw': str(path), 'projected': str(derived), 'byte_offset': offset,
                         'removed': marker.decode(), 'pins': hashes((path, derived))})
    return derived


def marker_receipt(counts, closure, required):
    entered = [rank for rank, n in counts.items() if n]
    return {'entered': entered, 'no_entry_marker': [rank for rank in counts if rank not in entered],
            'mismatches': [{'rank': rank, 'count': count, 'expected': int(not closure and rank in required)}
                           for rank, count in counts.items()
                           if count != int(not closure and rank in required)]}


def markers(logs, closure, required):
    counts = {}
    for rank, path in sorted(logs.items()):
        with path.open(errors='replace') as stream:
            counts[rank] = sum(line.count(MARKER) for line in stream)
    return {**marker_receipt(counts, closure, required),
            'paths': {str(rank): str(path) for rank, path in logs.items()}}


def caught(log, ranks, closure, required):
    counts = dict.fromkeys(range(ranks), 0)
    with log.open(errors='replace') as stream:
        for line in stream:
            tag = re.match(r'\[\d+,(\d+)\]<[^>]+>:', line)
            if MARKER in line:
                counts[int(tag[1]) if tag else 0] += line.count(MARKER)
    path = projected_log(log) if ranks == 1 else log
    signature = re.compile(r'All tests passed \(\d+ assertions? in \d+ test cases?\)')
    text = re.sub(r'\x1b\[[0-9;]*m', '', path.read_text(errors='replace'))
    return {**marker_receipt(counts, closure, required), 'catch': signature.findall(text),
            'path': str(path), 'raw': str(log)}


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
    if reason == '1500_second_deadline' or stopped >= DEADLINE:
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
    receipt = markers(logs, closure, set(rows))
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
    receipt['command'] = command_check(provenance, closure, 6, 14400)
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
    missed = None
    failed = failed or edits
    if not wrapper and wait['returncode']:
        failed = failed or 'controller exit'
    receipt.update(hard_checks=checks, hard_failures=failures, wrapper_only=bool(wrapper and not failed),
                   captured_ranks=sorted(rows), stop_reason=reason, seconds_until_stop=stopped,
                   work_limit=WORK_LIMIT, observed_work=sum(int(row['upper_work']) for row in rows.values()),
                   observed_solver_peak_sum_bytes=sum(peaks.values()),
                   observed_solver_peak_by_pid_bytes={str(pid): value for (pid, _), value in peaks.items()},
                   resource_limit_bytes=LIMIT, procfs_unit='KiB', bytes_per_kib=1024,
                   resource_complete=len(peaks) == 6 and all(peaks.values()) and complete)
    return raw, receipt, failed, missed


def difference(before, after):
    for index, (a, b) in enumerate(zip(before, after)):
        if a != b:
            return {'index': index, 'reference': a, 'candidate': b}
    if len(before) != len(after):
        index = min(len(before), len(after))
        return {'index': index, 'reference': before[index] if index < len(before) else None,
                'candidate': after[index] if index < len(after) else None}
    return None


def first_saved_difference(before_hashes, after_hashes, before_plan, after_plan):
    for name, digest in before_hashes.items():
        if after_hashes.get(name) != digest:
            return {'field': name, 'reference': digest, 'candidate': after_hashes.get(name)}
    mismatch = difference(before_plan, after_plan)
    return {'field': 'actual_bl_cavity_planning.jsonl', **mismatch} if mismatch is not None else None


def compare_saved(root, mode, case):
    sys.path.insert(0, str(NIGHT / 'N16'))
    from run_c import PRIVATE, planning_rows

    historical = cut.read(SAVED_REFERENCE)['safeguards'][mode]
    assert historical['passed'] and historical['require_identity'] and historical['exact']
    reference = Path(historical['case'])
    before_hashes = {}
    after_hashes = {}
    pins = [SAVED_REFERENCE, NIGHT / 'N16/run_c.py']
    missing = []
    for name, adopted in historical['payloads'].items():
        assert adopted['reference'] == adopted['candidate']
        before_hashes[name] = adopted['candidate']
        source, output = reference / name, case / name
        if source.is_file():
            assert cut.sha(source) == adopted['candidate'], source
            pins.append(source)
        else:
            missing.append(name)
        after_hashes[name] = cut.sha(output)
        pins.append(output)
    if missing:
        tracked = SAVED_REFERENCE.relative_to(cut.REPO)
        prior = subprocess.check_output(['git', 'show', f'8f1325ef35:{tracked}'], cwd=cut.REPO)
        assert prior == SAVED_REFERENCE.read_bytes(), SAVED_REFERENCE
    before_plan = after_plan = []
    projection = None
    if mode != 'merit481':
        planning = tuple(item / 'actual_bl_cavity_planning.jsonl' for item in (reference, case))
        pins.extend(planning)
        before_plan, after_plan = (planning_rows(path) for path in planning)
        projection = root / 'planning_projection.json'
        if not projection.exists():
            cut.save(projection, after_plan)
        else:
            assert cut.read(projection) == after_plan, projection
        pins.append(projection)
    first = first_saved_difference(before_hashes, after_hashes, before_plan, after_plan)
    private = bool(PRIVATE.search(projected_log(case / 'run.log').read_text()))
    return {'case': str(case), 'reference_case': str(reference), 'historical_first_difference': first,
            'historical_exact': first is None, 'missing_reference_payloads': missing, 'payloads': after_hashes,
            'reference_payloads': before_hashes, 'planning_projection': str(projection) if projection else None,
            'planning_count': len(after_plan), 'private_work': private, 'pins': hashes(pins)}


def strip_wrapper_sigterm(text):
    # Catch reports SIGTERM from the external timeout as a failed assertion.
    signal = re.search(r'FAILED:\n\s+\{Unknown expression after the reported line\}\n'
                       r'due to a fatal error condition:\n\s+SIGTERM - Termination request signal\n', text)
    if signal and 'Command exited with non-zero status 124' in text[signal.end():]:
        tail = re.sub(r'^(?:test cases|assertions):[^\n]*\| +1 failed[ \t]*$', '', text[signal.end():],
                      flags=re.MULTILINE)
        return text[:signal.start()] + tail
    return text


def bl_timeout_passes(case, command, status, text, seconds):
    guard = load_bl_guard(seconds)
    source = R / 'cases/curved492_saved_production_surface_serial_20261113T163756_3724702'

    def source_env(record):
        ignored = ('SU2_NATIVE_FROZEN_CONFIG', SWITCH, TRACE)
        return {k: v for k, v in record['environment'].items()
                if k.startswith('SU2_NATIVE_') and k not in ignored}

    if source_env(command) != source_env(cut.read(source / 'command.json')):
        return False
    if (case / 'run.cfg').read_bytes() != (source / 'run.cfg').read_bytes():
        return False
    stages = [dict(re.findall(r'(\w+)=(\S+)', line)) for line in text.splitlines()
              if 'NATIVE3D_STAGE ' in line]
    exits = [r for r in stages if r.get('stage') == 'exit' and r.get('rank') == '0']
    if any(int(row.get('upper_refinements', 0)) for row in stages):
        return False
    if any(int(row.get('length_splits', 0)) < 300 for row in exits):
        return False
    actual = case / 'actual_saved_surface_stage_rank0.json'
    if actual.is_file():
        tests = guard.prior.conditions(status, command, cut.read(actual),
                                        dict.fromkeys(guard.prior.FILES, True))
        tests.pop('executed_successfully')
        if not all(tests.values()):
            return False
    return True


def wrapper_timeout(case, mode, status, command, receipt, wait, seconds):
    if mode.startswith('2d') or status['exit_code'] != 124 or wait['returncode'] != 124:
        return None
    text = Path(receipt['path']).read_text(errors='replace')
    if 'Exit status: 124' not in text:
        return None
    checked = strip_wrapper_sigterm(text)
    failure = r'test cases:.*failed|assertions:.*failed|Curved atlas (?:work|storage) exhausted|\bFAILED:'
    if re.search(failure, checked):
        return None
    if mode == 'bl' and not bl_timeout_passes(case, command, status, checked, seconds):
        return None
    return {'seconds': seconds, 'status': str(case / 'status.json'),
            'log': receipt['raw'], 'catch_sigterm': checked != text}


def evaluate_other(root, mode, closure, wait, seconds):
    case, = (root / 'cases').iterdir()
    raw = case / 'status.json'
    r = cut.read(raw)
    command = cut.read(case / 'command.json')
    ranks = int(mode[-1]) if mode.startswith('2d') else 1
    log = case / 'run.log'
    required = set() if mode.startswith('2d') else {0}
    receipt = caught(log, ranks, closure, required)
    receipt['command'] = command_check(command, closure, ranks, seconds)
    monitor = cut.read(case / 'safeguard_process_receipt.json')
    failed = next((name for name, ok in (
        ('source or runtime pins', r['pins_verified'] and not r['changed']),
        ('owned handles terminal', wait['terminal'] and cut.terminal(wait['controller'])
         and monitor['state'] == 'terminal' and not monitor['live_owned_descendants']
         and all(cut.terminal(identity) for identity in monitor['identities'])),
        ('job peak', 0 < monitor['peak_sampled_tree_rss_bytes'] <= LIMIT),
        ('switch markers', not receipt['mismatches'] or
         (r['exit_code'] == 124 and not closure and not receipt['entered'] and not mode.startswith('2d'))),
    ) if not ok), None)
    watchdog = None if failed else wrapper_timeout(case, mode, r, command, receipt, wait, seconds)
    comparison = None
    if not watchdog:
        failed = failed or ('test exit' if r['exit_code'] else None)
        failed = failed or ('controller exit' if wait['returncode'] else None)
        failed = failed or ('Catch completion' if len(receipt['catch']) != ranks else None)
        if mode == 'bl':
            bl = case / 'p6_bl_receipt.json'
            if bl.is_file():
                raw = bl
                failed = failed or next((name for name, ok in cut.read(bl)['gates'].items() if not ok), None)
            else:
                failed = failed or 'missing exact C492 validator result'
        elif not failed and mode in ('merit481', 'saved481', 'saved_current'):
            comparison = compare_saved(root, mode, case)
            raw = root / 'historical_comparison.json'
            if not raw.exists():
                cut.save(raw, comparison)
            else:
                assert cut.read(raw) == comparison, raw
            if not closure or mode != 'saved_current':
                failed = comparison['historical_first_difference']
    disposition = 'OPEN' if watchdog else 'FAILED' if failed else 'passed'
    return {'status': disposition, 'raw': str(raw), 'raw_sha256': cut.sha(raw), 'first_failure': failed,
            'case': str(case), 'switch_receipt': receipt, 'wait': wait, 'watchdog': watchdog,
            'comparison': comparison}


def evaluate(root, mode, closure, wait, seconds):
    if mode != 'first-collapse':
        return evaluate_other(root, mode, closure, wait, seconds)
    raw, receipt, failed, _ = evaluate_first(root, closure, wait)
    incomplete = any(value == 'not evaluated' for value in receipt['hard_checks'].values())
    return {'status': 'FAILED' if failed else 'OPEN' if incomplete else 'passed', 'raw': str(raw),
            'raw_sha256': cut.sha(raw), 'first_failure': failed, 'switch_receipt': receipt, 'wait': wait,
            'case': str(raw.parent), 'watchdog': None}


def verified_pins(receipt):
    for path, digest in receipt['pins'].items():
        assert cut.sha(path) == digest, path


def inputs_for(root, record):
    common = ('controller_host_preflight.json', 'controller_launch.json', 'controller_wait.json',
              'controller_outer.log', 'additional_source_pins.json', 'historical_comparison.json')
    files = [root / name for name in common if (root / name).is_file()]
    case = Path(record['case'])
    names = ('provenance.json', 'command.json', 'run.cfg', 'status.json', 'pins.json', 'run.log', 'resources.jsonl',
             'safeguard_process_receipt.json', 'terminal_handles.json', 'actual_saved_surface_stage_rank0.json',
             'nose_target_observation.jsonl', 'collapse_records.json', 'host_resources.jsonl', 'outer.time.txt',
             'stdout_without_interleaved_marker.log', 'stdout_rejoined.json')
    files.extend(case / name for name in names if (case / name).is_file())
    files.extend(case.glob('rank_output/*/rank.*/std*'))
    files.extend(Path(name) for name in (record.get('comparison') or {}).get('pins', ()) if name)
    files.append(Path(record['raw']))
    return files


def run_attempt(root, label, closure, group, mode, seconds):
    root.mkdir(parents=True, exist_ok=True)
    wait_path = root / 'controller_wait.json'
    if wait_path.exists():
        wait = cut.read(wait_path)
        assert wait['terminal'] and cut.terminal(wait['controller']), wait_path
    else:
        assert not (root / 'controller_launch.json').exists(), root
        from checkpoint import host_preflight

        host_preflight()
        try:
            wait = cut.run_gate(root, 'controller', [cut.PYTHON, '-B', str(SELF), str(label), str(closure),
                                                      group, '--worker', mode, '--attempt-dir', str(root),
                                                      '--seconds', str(seconds)])
        except RuntimeError:
            wait = cut.read(wait_path)
    record = evaluate(root, mode, closure, wait, seconds)
    record['pins'] = hashes(inputs_for(root, record))
    return record


def job(root, label, closure, group, mode):
    original = root / 'attempt1'
    first = run_attempt(original, label, closure, group, mode, 600)
    if first['watchdog'] is None:
        return first
    first_receipt = root / 'first_watchdog_receipt.json'
    if not first_receipt.exists():
        cut.save(first_receipt, first)
    else:
        assert cut.read(first_receipt) == first
    verified_pins(first)
    retry = run_attempt(root / 'watchdog_retry', label, closure, group, mode, 1800)
    retry['first_watchdog'] = {'path': str(first_receipt), 'sha256': cut.sha(first_receipt),
                               'seconds': first['watchdog']['seconds']}
    retry['pins'].update({**first['pins'], str(first_receipt): cut.sha(first_receipt)})
    return retry


def saved_pair(one, two):
    first, repeat = [row['comparison'] for row in (one, two)]
    plans = [cut.read(row['planning_projection']) for row in (first, repeat)]
    mismatch = first_saved_difference(first['payloads'], repeat['payloads'], *plans)
    return {'status': 'FAILED' if mismatch is not None else 'passed',
            'first_difference': mismatch, 'reference': first, 'repeat': repeat}


def run(label, closure, group):
    build = HERE / f'build{label}'
    status = verify_build(label)
    inputs = prerequisites(build, closure)
    root = HERE / 'shared' / build.name / f'closure{closure}' / group
    root.mkdir(parents=True, exist_ok=True)
    summary = root / 'case_summary.json'
    fingerprint = {key: status[key] for key in ('build', 'binary_sha256', 'test_binary_sha256', 'source_pins_sha256')}
    if summary.exists():
        row = cut.read(summary)
        assert all(row[key] == value for key, value in fingerprint.items()), summary
        verified_pins(row)
        return row
    plan = {'first-collapse': [('first-collapse', 'first-collapse')], 'exact-c492': [('bl', 'bl')],
            'merit481': [('merit481', 'merit481')], 'saved481': [('saved481', 'saved481')],
            'saved-current': [('saved_current', 'saved_current')],
            'native2d': [(mode, mode) for mode in ('2d1', '2d2', '2d4')]}[group]
    if closure and group == 'saved-current':
        plan.append(('saved_current_repeat', 'saved_current'))
    results = {}
    pins = hashes((SELF, ORDER, ORDER.with_suffix('.sha256'), FIRST, REMAINING, MONITOR,
                   LEGACY / 'check_p6_bl.py', PURPOSE / 'check_bl_regression.py', SAVED_REFERENCE, NIGHT / 'N16/run_c.py', build / 'build_status.json',
                   build / 'source_pins.json', *inputs))
    for name, mode in plan:
        try:
            row = job(root / name, label, closure, group, mode)
        except (AssertionError, OSError, KeyError, TypeError, ValueError, subprocess.CalledProcessError) as exc:
            row = {'status': 'FAILED', 'failure_kind': 'measurement',
                   'first_failure': f'{type(exc).__name__}: {exc}'}
        results[name] = row
        pins.update(row.get('pins', {}))
    pair = None
    if closure and group == 'saved-current':
        pair = (saved_pair(*results.values()) if all(row['status'] == 'passed' for row in results.values())
                else {'status': 'not evaluated', 'first_difference': None})
    values = {row['status'] for row in results.values()}
    if pair is not None:
        values.add(pair['status'])
    disposition = 'FAILED' if 'FAILED' in values else 'passed' if values == {'passed'} else 'OPEN'
    first_failure = next((row['first_failure'] for row in results.values() if row.get('first_failure')), None)
    first_failure = first_failure or (pair['first_difference'] if pair else None)
    failure_kinds = {row.get('failure_kind', 'result') for row in results.values() if row['status'] == 'FAILED'}
    if pair and pair['status'] == 'FAILED':
        failure_kinds.add('result')
    failure_kind = 'result' if 'result' in failure_kinds else 'measurement' if failure_kinds else None
    verify_build(label)
    row = {**fingerprint, 'status': disposition, 'first_difference': first_failure, 'failure_kind': failure_kind,
           'environment': environment(closure),
           'closure_setting': 'absent' if closure else '0', 'subruns': results, 'pair': pair, 'pins': pins}
    cut.save(summary, row)
    return row


def self_check():
    compile(other_source(HERE / 'build673'), str(REMAINING), 'exec')
    inherited = {'OTHER': 'keep', SWITCH: '1'}
    assert effective(inherited, 0) == {'OTHER': 'keep', SWITCH: '0', TRACE: '0'}
    assert effective(inherited, 1) == {'OTHER': 'keep', TRACE: '0'}
    assert marker_receipt({0: 1, 1: 0}, 0, {0})['mismatches'] == []
    assert marker_receipt({0: 1}, 1, set())['mismatches'] == [{'rank': 0, 'count': 1, 'expected': 0}]
    for seconds in (600, 1800):
        guard = load_bl_guard(seconds)
        assert guard.prior.conditions({}, {'ranks': 1, 'timeout_seconds': seconds}, {}, {})['serial_bounded_command']
        assert not guard.prior.conditions({}, {'ranks': 2, 'timeout_seconds': seconds}, {}, {})['serial_bounded_command']
        assert not guard.prior.conditions({}, {'ranks': 1, 'timeout_seconds': 2400 - seconds}, {}, {})['serial_bounded_command']
    assert difference([1], [2]) == {'index': 0, 'reference': 1, 'candidate': 2}
    assert first_saved_difference({'x': 'a'}, {'x': 'a'}, [1], [1, 2]) == {
        'field': 'actual_bl_cavity_planning.jsonl', 'index': 1, 'reference': None, 'candidate': 2}
    with tempfile.TemporaryDirectory(prefix='p3-shared-check-') as temp:
        path = Path(temp) / 'run.log'
        raw = ('a' + MARKER + '\nb\n').encode()
        path.write_bytes(raw)
        assert projected_log(path).read_bytes() == b'ab\n' and path.read_bytes() == raw
        mpi = Path(temp) / 'mpi.log'
        mpi.write_text('[1,0]<stdout>:\x1b[32mAll tests passed\x1b[0m (42 assertions in 3 test cases)\n')
        assert caught(mpi, 2, 0, set())['catch'] == ['All tests passed (42 assertions in 3 test cases)']
    print('Shared FP2-P3 command, watchdog binding, rejoin and comparison self-checks passed.')


if __name__ == '__main__':
    parser = ArgumentParser(description=__doc__)
    parser.add_argument('build', type=int, nargs='?')
    parser.add_argument('closure', type=int, choices=(0, 1), nargs='?')
    parser.add_argument('group', choices=SHARED, nargs='?')
    parser.add_argument('--worker')
    parser.add_argument('--attempt-dir', type=Path)
    parser.add_argument('--seconds', type=int, default=600)
    parser.add_argument('--self-check', action='store_true')
    args = parser.parse_args()
    if args.self_check:
        self_check()
    elif args.worker:
        build = HERE / f'build{args.build}'
        if args.worker == 'first-collapse':
            first(args.attempt_dir, build, args.closure)
        else:
            other(args.attempt_dir, build, args.worker, args.closure, args.seconds)
    else:
        result = run(args.build, args.closure, args.group)
        print(args.group, result['status'], result['first_difference'], flush=True)
        raise SystemExit({'passed': 0, 'FAILED': 1, 'OPEN': 75}[result['status']])
