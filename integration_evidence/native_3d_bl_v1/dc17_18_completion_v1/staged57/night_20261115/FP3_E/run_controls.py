"""Run and seal FP3-E's Step 1 checks using the pinned P3 validators."""

from argparse import ArgumentParser
from contextlib import contextmanager
from pathlib import Path
import re
import runpy
import subprocess
import sys
import traceback
from tempfile import TemporaryDirectory
from types import SimpleNamespace
from unittest.mock import patch
from xml.etree.ElementTree import ParseError

from build_fp3 import HERE, cut, verify_build

P3 = HERE.parent / 'FP2_P3'
sys.path.insert(0, str(P3))
import checkpoint as p3_checkpoint
import p3_fixtures as fixtures

# Older fixture imports load a different run_scoped module of the same name.
P3_SCOPED = P3 / 'run_scoped.py'
scoped = SimpleNamespace(**runpy.run_path(str(P3_SCOPED)))

SELF = Path(__file__).resolve()
ORDER = HERE / 'inputs/FP3_E_QUALITY_STALL_HANDOFF.md'
ORDER_SUM = ORDER.parent / 'SHA256SUMS'
ORDER_SHA = 'a1ce55145f281a64a355d1724a9155dcdee6ddbcaaee8e88a4dd17948ea8c9a1'
SOURCE_CHECK = HERE / 'reports/source_check.md'
ACCEPTED = P3 / 'accepted_reference.json'
ACCEPTED_SHA = '9d191c742b37e6c0dcba284442966d00068475ce74ba2d3d0a527373a2f53734'
PROBE = 'SU2_NATIVE_FP3_PROBE'
SHARED = {'first-collapse': ('first-collapse',), 'exact-c492': ('bl',), 'merit481': ('merit481',),
          'native2d': ('2d1', '2d2', '2d4'), 'saved-current': ('saved_current',)}
G1 = ('G1-serial', 'G1-mpi2', 'G1-mpi4')
HIDDEN = {'legacy': ((2, '[.][FP3B]', 1), '0', 'D'),
          'cells': ((3, '[.][FP3D]', 1), '0', 'D'),
          'fp3b_control': ((3, '[.][FP3D]', 1), '3', 'D'),
          'stall_serial': ((1, '[.][fp3_e][serial]', 1), '0', 'E'),
          'stall_mpi': ((2, '[.][fp3_e][mpi]', 1), '0', 'E'),
          'stall_mpi_fp3b': ((2, '[.][fp3_e][mpi]', 1), '3', 'E'),
          'stall_mpi_fp3d': ((2, '[.][fp3_e][mpi]', 1), '4', 'E')}
CHECKS = (*G1, 'hidden', 'G2', 'GQ', *SHARED, 'G3', 'G4')
EXIT = {'passed': 0, 'FAILED': 1, 'OPEN': 75, 'not evaluated': 75}
fingerprint = p3_checkpoint.fingerprint
pins_for = p3_checkpoint.pins_for
verify_pins = p3_checkpoint.verify_pins
host_preflight = p3_checkpoint.host_preflight


def accepted():
    assert cut.sha(ACCEPTED) == ACCEPTED_SHA
    row = cut.read(ACCEPTED)
    assert row['accepted'] and row['build'] == 673
    assert row['accepted_commit'].startswith('bab6bcf0ef')
    return row


def pinned(row):
    path = Path(row['path'])
    verify_pins({str(path): row['sha256']})
    return path


def verify_order():
    assert cut.sha(ORDER) == ORDER_SHA
    assert ORDER_SUM.read_text().split() == [ORDER_SHA, ORDER.name]


def support(build):
    verify_order()
    return (SELF, ORDER, ORDER_SUM, SOURCE_CHECK, ACCEPTED, P3_SCOPED,
            HERE / 'build_fp3.py', build / 'build_status.json', build / 'source_pins.json')


def probe_receipt(command, ranks, value='0'):
    argv = command['argv']
    assignments = [word for word in argv if word.startswith(PROBE + '=')]
    if 'environment' in command:
        assert command['environment'][PROBE] == value, command
    else:
        assert assignments == [PROBE + '=' + value], command
    assert not assignments or assignments == [PROBE + '=' + value], command
    assert command.get('ranks', ranks) == ranks
    if ranks > 1:
        mpi = argv.index('mpirun')
        assert argv[argv.index('-n', mpi) + 1] == str(ranks)
        assert argv.count(PROBE) == 1
        index = argv.index(PROBE, mpi)
        assert argv[index - 1] == '-x', command
    else:
        assert 'mpirun' not in argv
    return {'variable': PROBE, 'value': value, 'ranks': ranks, 'mpi_exported': ranks > 1}


def substitute(source, old, new):
    assert source.count(old) == 1, old
    return source.replace(old, new, 1)


def inherit(name, edits, **bindings):
    """Load the exact P3 helper with explicit FP3-E bindings in a private namespace."""
    path = P3 / name
    source = path.read_text()
    for old, new in edits:
        source = substitute(source, old, new)
    scope = {'__name__': 'fp3_' + path.stem, '__file__': str(path)}
    exec(compile(source, str(path), 'exec'), scope)
    scope.update(HERE=HERE, verify_build=verify_build, ORDER=ORDER, SOURCE_CHECK=SOURCE_CHECK,
                 SELF=SELF, PROBE=PROBE, probe_receipt=probe_receipt, **bindings)
    return scope


def prefix_adapter():
    return inherit('run_prefix_p3.py', (
        ("settings = {TRACE: '0', **", "settings = {TRACE: '0', PROBE: '0', **"),
        ("'ranks': counts, 'logs':", "'probe': probe_receipt(p, 6), 'ranks': counts, 'logs':"),
        ("str(Path(__file__).resolve()), str(build), str(attempt), gate",
         "str(SELF), '--worker', 'prefix', str(build), str(attempt), gate"),
    ))


def prefix_control(build, root, gate):
    entry = accepted()['prefixes'][gate]
    reference = Path(entry['reference_case'])
    capture = entry['captures'][0]
    assert Path(capture['case']) == reference
    assert cut.read(pinned(entry['evidence']))['status'] == 'passed'
    reference_pins = dict(capture['rank_logs'])
    for record in (capture['provenance'], *capture['validator_receipts']):
        reference_pins[str(pinned(record))] = record['sha256']
    verify_pins(reference_pins)
    adapter = prefix_adapter()
    adapter['HISTORY'] = {**adapter['HISTORY'], gate: reference}
    sources = (*support(build), P3 / 'run_prefix_p3.py', Path(entry['evidence']['path']))
    result = adapter['capture'](build, root, gate, 1, sources)
    result['pins'].update(reference_pins)
    if result['status'] == 'passed':
        compare = runpy.run_path(str(adapter['N22']))['compare_cases']
        report = compare('C', gate, reference, Path(result['case']))
        result.update(status='passed' if report['passed'] else 'FAILED',
                      first_difference=report.get('first_difference'), comparison=report)
    return result


def g3_reference():
    entry = accepted()['G3']
    physical = pinned(entry['physical_receipt'])
    evidence = pinned(entry['evidence'])
    case = Path(entry['case'])
    child = cut.read(evidence)
    assert child['case'] == str(case) and child['status'] == 'passed'
    assert cut.read(physical)['passed']
    paths = (case/'run.log', case/'nose_target_observation.jsonl', case/'actual_saved_surface_stage_rank0.json')
    verify_pins({str(path): child['pins'][str(path)] for path in paths})
    return case, (physical, evidence, *paths)


def g3_adapter():
    reference, _ = g3_reference()
    return inherit('run_g3_p3.py', (
        ("    env = command['environment']", "    probe = probe_receipt(command, 1)\n    env = command['environment']"),
        ("'log': str(parsed), 'raw':", "'probe': probe, 'log': str(parsed), 'raw':"),
        ('("SU2_NATIVE_FROZEN_CONFIG", SWITCH, TRACE)', '("SU2_NATIVE_FROZEN_CONFIG", SWITCH, TRACE, PROBE)'),
        ("excluded = (SWITCH, TRACE, 'SU2_NATIVE_FROZEN_CONFIG')",
         "excluded = (SWITCH, TRACE, PROBE, 'SU2_NATIVE_FROZEN_CONFIG')"),
        ("    exec(compile(source, str(END), 'exec'), namespace)",
         "    exec(compile(source, str(END), 'exec'), namespace)\n    namespace['G3_REFERENCE'] = REFERENCE"),
        ("report.update(passed=closure == 1 or first is None, first_difference=first,\n"
         "                  identity_is_gate=closure == 0, switch_receipt=marker)",
         "report.update(passed=first is None, first_difference=first, identity_is_gate=True, switch_receipt=marker)"),
        ("        os.environ[TRACE] = '0'", "        os.environ[TRACE] = '0'\n        os.environ[PROBE] = '0'"),
        ("'trace': '0', 'seconds_limit': timeout", "'trace': '0', 'probe': '0', 'seconds_limit': timeout"),
        ("str(Path(__file__).resolve()), str(build),", "str(SELF), '--worker', 'g3', str(build),"),
        ("    if status['exit_code'] != 124:\n        raise AssertionError",
         "    if status['exit_code'] != 124:\n"
         "        log = parsed.read_text(errors='replace')\n"
         "        failure = re.search(r'(?m)^.*FAILED:.*(?:\\n[^\\n]*){0,5}', log)\n"
         "        if failure and status['exit_code'] > 0:\n"
         "            return finish('FAILED', {'field': 'Catch assertion', 'expected_exit': 0,\n"
         "                                     'observed_exit': status['exit_code'], 'detail': failure[0]})\n"
         "        raise AssertionError"),
        ("    return finish('OPEN', {'field': 'outer watchdog', 'seconds': timeout})",
         "    difference = partial_g3_difference(case, REFERENCE, PATH_IDENTITY)\n"
         "    if difference is not None:\n"
         "        return finish('FAILED', difference)\n"
         "    return finish('OPEN', {'field': 'outer watchdog', 'seconds': timeout})"),
    ), CASE='FP3E_G3', REFERENCE=reference, partial_g3_difference=partial_g3_difference,
       PATH_IDENTITY=HERE.parent/'N9/section6_6/drafts/check_g3_identity_66.py')


def partial_g3_difference(case, reference, helper):
    observation = case/'nose_target_observation.jsonl'
    if not observation.is_file():
        return None
    identity = runpy.run_path(str(helper))
    left, left_observer, left_stage, *_ = identity['records'](reference)
    original = Path.read_text

    def complete_rows(path, *args, **kwargs):
        value = original(path, *args, **kwargs)
        if path in (observation, case/'run.log') and value and not value.endswith('\n'):
            return value.rpartition('\n')[0]
        return value

    with patch.object(Path, 'read_text', complete_rows):
        right, right_observer, right_stage, *_ = identity['records'](case)
    for index, (expected, actual) in enumerate(zip(left, right)):
        if expected != actual:
            return {'field': 'accepted_split', 'index': index, 'expected': expected, 'actual': actual}
    if not right_observer or not any(row.get('stage') == 'surface_required_entry' for row in right_stage):
        return None
    work = identity['observer_adjusted_work']
    expected = work(left_stage, left_observer)['observer_adjusted_entry_work']
    actual = work(right_stage, right_observer)['observer_adjusted_entry_work']
    return None if expected == actual else {'field': 'observer_adjusted_entry_work',
                                            'expected': expected, 'actual': actual}


def g3_control(build, root):
    _, files = g3_reference()
    adapter = g3_adapter()
    sources = (*support(build), P3 / 'run_g3_p3.py', *files)
    row = adapter['run'](build, root, 1, sources)
    row['pins'].update(pins_for(sources))
    return row


def shared_adapter(build):
    adapter = inherit('run_shared.py', (
        ("return {TRACE: '0'} if closure", "return {TRACE: '0', PROBE: '0'} if closure"),
        ("        if path.name == 'pins.json':", "            probe_receipt(record, record['ranks'])\n"
         "        if path.name == 'pins.json':"),
        ("    env = record['environment']\n    assert env.get(TRACE)",
         "    probe = probe_receipt(record, ranks)\n    env = record['environment']\n    assert env.get(TRACE)"),
        ("'ranks': ranks, 'timeout_seconds': seconds}", "'ranks': ranks, 'timeout_seconds': seconds, 'probe': probe}"),
        ("'SU2_NATIVE_CONFORMING_WITNESS', 'SU2_NATIVE_GEOMETRY_WORK_TRACE')",
         "'SU2_NATIVE_CONFORMING_WITNESS', 'SU2_NATIVE_GEOMETRY_WORK_TRACE', 'SU2_NATIVE_FP3_PROBE')"),
        ("ignored = ('SU2_NATIVE_FROZEN_CONFIG', SWITCH, TRACE)",
         "ignored = ('SU2_NATIVE_FROZEN_CONFIG', SWITCH, TRACE, PROBE)"),
        ("            if not closure or mode != 'saved_current':\n                failed = comparison['historical_first_difference']",
         "            failed = comparison['historical_first_difference']"),
        ("str(SELF), str(label), str(closure),", "str(SELF), '--worker', 'shared', str(label), str(closure),"),
    ))
    def prerequisites(directory, closure):
        assert closure == 1 and directory == build
        return (SOURCE_CHECK, ACCEPTED, P3/'run_shared.py', HERE/'build_fp3.py')

    def disjoint(paths):
        extra = pins_for(paths)
        primary = cut.read(build/'source_pins.json')
        for name in primary.keys() & extra.keys():
            assert primary[name] == extra.pop(name), name
        return extra

    original_compare = adapter['compare_saved']
    def compare_saved(root, mode, case):
        return compare_current(adapter, root, case) if mode == 'saved_current' else original_compare(root, mode, case)

    adapter.update(prerequisites=prerequisites, compare_saved=compare_saved)
    return adapter, disjoint


def compare_current(adapter, root, case):
    entry = accepted()['saved_current']
    result = cut.read(pinned(entry['evidence']))
    assert result['status'] == 'passed'
    old = entry['trials'][0]
    assert entry['reference_case'] == old['case']
    before, after = old['payload_hashes'], {}
    assert set(before) == {'trajectory_working_owned.bin', 'trajectory_working_parameters.bin',
                           'correction_unchanged_owned.bin'}
    paths = [ACCEPTED, Path(entry['evidence']['path'])]
    for name, expected in before.items():
        source, output = Path(old['case'])/name, case/name
        assert cut.sha(source) == expected
        after[name] = cut.sha(output)
        paths.extend((source, output))
    frozen = pinned(old['planning_projection'])
    helper = adapter['NIGHT'] / 'N16/run_c.py'
    sys.path.insert(0, str(helper.parent))
    from run_c import PRIVATE, planning_rows
    now = case/'actual_bl_cavity_planning.jsonl'
    projection = root/'planning_projection.json'
    before_plan, after_plan = cut.read(frozen), planning_rows(now)
    if not projection.is_file():
        cut.save(projection, after_plan)
    assert cut.read(projection) == after_plan
    paths.extend((helper, frozen, now, projection))
    first = adapter['first_saved_difference'](before, after, before_plan, after_plan)
    private = bool(PRIVATE.search((case/'run.log').read_text()))
    return {'case': str(case), 'reference_case': old['case'], 'historical_first_difference': first,
            'historical_exact': first is None, 'payloads': after, 'reference_payloads': before,
            'planning_projection': str(projection), 'planning_count': len(after_plan),
            'private_work': private, 'pins': pins_for(paths)}


def shared_control(build, root, group):
    adapter, _ = shared_adapter(build)
    label = int(build.name.removeprefix('build'))
    subruns = {mode: adapter['job'](root/mode, label, 1, group, mode) for mode in SHARED[group]}
    status = scoped.combine(subruns)
    return {'status': status, 'subruns': subruns, 'environment': adapter['environment'](1),
            'first_difference': next((row['first_failure'] for row in subruns.values() if row['first_failure']), None),
            'pins': {path: digest for row in subruns.values() for path, digest in row['pins'].items()}}


@contextmanager
def bound_test(value='0'):
    fixtures.p2.m1.fixtures.hidden.clear_options()
    prior_save = cut.save
    environment = {**fixtures.settings(False, False), PROBE: value}

    def save(path, row):
        if path.name == 'command.json' and 'argv' in row:
            argv = row['argv']
            assert argv.count(PROBE + '=' + value) == 1
            fixtures.bind_argv(argv, False)
            if 'mpirun' in argv:
                mpi = argv.index('mpirun') + 3
                argv[mpi:mpi] = ['-x', PROBE]
            row['fp3_probe'] = probe_receipt(row, row['ranks'], value)
        return prior_save(path, row)

    with patch.object(cut, 'save', save):
        yield environment


def fixture_artifacts(case):
    paths = [p for p in case.glob('*.json') if p.is_file()]
    paths.extend(p for p in (case/'time.txt', case/'test_outer.log') if p.is_file())
    paths.extend(case.glob('rank_output/*/rank.*/std*'))
    return pins_for(paths)


def require_wrapper_exit(case):
    wait = cut.read(case/'test_wait.json')
    assert wait['returncode'] == 124 and wait['terminal'] and cut.terminal(wait['controller'])
    assert re.search(r'(?m)^\s*Exit status: 124\s*$', (case/'time.txt').read_text())


def completed_g1(parser, case, ranks, reference):
    if ranks == 1:
        logs = [('0', case/'test_outer.log')]
    else:
        logs = [(path.parent.name.split('.')[-1], path)
                for path in case.glob('rank_output/*/rank.*/stdout')]
    expected = {(row['rank'], row['name']): parser['comparable'](row) for row in reference['case_results']}
    for rank, path in sorted(logs):
        text = path.read_text(errors='replace')
        for match in re.finditer(r'<TestCase\b.*?</TestCase>', text, re.S):
            case_xml = match.group()
            if 'SIGTERM - Termination request signal' in case_xml:
                continue
            rows, _ = parser['parse_xml']('<Catch>'+case_xml+'</Catch>', rank, path)
            observed = parser['comparable'](rows[0])
            key = (rank, observed['name'])
            if expected.get(key) != observed:
                return {'field': 'G1 completed testcase', 'expected': expected.get(key), 'observed': observed}
    return None


def g1_control(build, root, name, receipt):
    g1 = fixtures.p2.m3.g1
    cohort = name.removeprefix('G1-')
    limits = cut.read(g1.MANIFEST)['cohorts'][cohort]
    selection = cut.read(g1.SELECTION)
    reference = g1.REF/f'G1_candidate_{cohort}.json'
    assert cut.sha(reference) == limits['report_sha256']
    parser = runpy.run_path(str(g1.PARSER))
    parser['self_check']()
    ranks = limits['ranks']
    assert limits['case_records'] == {'serial': 375, 'mpi2': 114, 'mpi4': 228}[cohort]
    original = g1.run_case
    attempts = []
    for index, factor in enumerate((1, 3), 1):
        case = root/f'attempt{index}'
        seconds = limits['wrapper_seconds'] * factor
        with bound_test() as env:
            def launch(path, binary, ranks, selector, _, **kwargs):
                host_preflight()
                return original(path, binary, ranks, selector, seconds, environment=env, **kwargs)

            try:
                with patch.object(g1, 'run_case', launch):
                    g1.run_cohort(case, cohort, receipt, build/'source_pins.json', selection, limits, parser)
            except (RuntimeError, ValueError, ParseError):
                if not (case/'test_wait.json').is_file():
                    raise
                wait = cut.read(case/'test_wait.json')
                if wait['returncode'] != 124 and not (case/'comparison.json').is_file():
                    raise
        wait = cut.read(case/'test_wait.json')
        assert wait['terminal'] and cut.terminal(wait['controller'])
        command = cut.read(case/'command.json')
        probe = probe_receipt(command, ranks)
        assert command['seconds_limit'] == seconds
        if wait['returncode'] == 124:
            require_wrapper_exit(case)
            first = completed_g1(parser, case, ranks, cut.read(reference))
            status = 'FAILED' if first else 'OPEN'
            first = first or {'field': 'outer watchdog', 'seconds': seconds}
            comparison = None
        else:
            comparison = cut.read(case/'comparison.json')
            first = next(iter(comparison['unexpected_changes']), None)
            assert comparison['records'] == limits['case_records']
            status = 'passed' if comparison['identity_pass'] else 'FAILED'
        row = {'case': str(case), 'status': status, 'first_difference': first,
               'wrapper_seconds': seconds, 'probe': probe, 'comparison': comparison,
               'pins': fixture_artifacts(case)}
        attempts.append(row)
        if status != 'OPEN':
            break
    pins = pins_for((reference, g1.MANIFEST, g1.SELECTION, g1.PARSER, Path(g1.__file__), Path(fixtures.__file__)))
    pins.update({path: digest for attempt in attempts for path, digest in attempt['pins'].items()})
    return {**attempts[-1], 'attempts': attempts, 'pins': pins}


def hidden_case(build, root, receipt, spec, value, marker):
    hidden = fixtures.p2.m1.fixtures.hidden
    original = hidden.c1.run_case
    pins = pins_for((*support(build), Path(hidden.__file__), Path(hidden.c1.__file__)))
    attempts = []
    binary = Path(receipt['test_binary'])
    for index, seconds in enumerate((1350, 4050), 1):
        case = root/f'attempt{index}'

        def assertion_failed(logs):
            if cut.read(case/'test_wait.json')['returncode'] != 124:
                return original_failure(logs)
            require_wrapper_exit(case)
            # Exclude the XML-independent Catch row emitted for wrapper SIGTERM.
            fatal = (r'^\s*FAILED:\n\s+\{Unknown expression after the reported line\}\n'
                     r'due to a fatal error condition:\n\s+SIGTERM - Termination request signal\n')
            for path in logs:
                log = scoped.ANSI.sub('', path.read_text(errors='replace'))
                if re.search(r'^\s*(?:[^\r\n]+:\d+:\s*)?FAILED:',
                             re.sub(fatal, '', log, flags=re.MULTILINE), re.MULTILINE):
                    return True
            return False

        original_failure = hidden.has_assertion_failure
        with bound_test(value) as env:
            def launch(*args):
                host_preflight()
                return original(*args, environment=env)

            with patch.object(hidden.c1, 'run_case', launch), patch.object(
                    hidden, 'has_assertion_failure', assertion_failed):
                result = hidden.attempt(case, binary, fingerprint(receipt), pins, spec, seconds)
        receipt_path = case/'command.json'
        probe = probe_receipt(cut.read(receipt_path), spec[0], value)
        assert cut.read(receipt_path)['seconds_limit'] == seconds
        if result['passed']:
            probe['observed'] = hidden_probe_ranks(case, spec[0], value, marker)
        attempts.append({**result, 'probe': probe})
        pins.update(fixture_artifacts(case))
        if result['watchdog'] and not result['assertion_failed']:
            require_wrapper_exit(case)
            continue
        break
    status = 'passed' if result['passed'] else 'OPEN' if result['watchdog'] and not result['assertion_failed'] else 'FAILED'
    if status == 'FAILED' and not result['assertion_failed']:
        raise RuntimeError('Missing or malformed hidden Catch completion')
    first = None if status == 'passed' else {'field': 'hidden Catch results',
                                             'expected_cases': spec[2], 'observed': result['catch_summaries']}
    return {'status': status, 'first_difference': first, 'attempts': attempts, 'pins': pins}


def hidden_probe_ranks(case, ranks, value, marker):
    observed = {}
    logs = [(0, case/'test_outer.log')] if ranks == 1 else [
        (int(log.parent.name.rsplit('.', 1)[1]), log) for log in sorted(case.glob('rank_output/*/rank.*/stdout'))]
    for rank, log in logs:
        output = scoped.ANSI.sub('', log.read_text(errors='replace'))
        matches = re.findall(rf'^FP3_{marker} HIDDEN_PROBE rank=(\d+) value=(\d+)$', output, re.MULTILINE)
        assert rank not in observed and matches == [(str(rank), value)], (log, matches)
        observed[rank] = {'value': value, 'path': str(log)}
    assert set(observed) == set(range(ranks)), observed
    return observed


def hidden_control(build, root, receipt):
    parts = {name: hidden_case(build, root/name, receipt, spec, value, marker)
             for name, (spec, value, marker) in HIDDEN.items()}
    status = scoped.combine(parts)
    first = next((row['first_difference'] for row in parts.values() if row['status'] == status), None)
    return {'status': status, 'first_difference': first, 'parts': parts,
            'pins': {path: digest for row in parts.values() for path, digest in row['pins'].items()}}


def run(label, names, finalize_measurement=False):
    build = HERE / f'build{label}'
    root = HERE / 'controls' / build.name
    root.mkdir(parents=True, exist_ok=True)
    receipt = verify_build(label)
    expected = fingerprint(receipt)
    common = pins_for(support(build))
    for name in names:
        child = root/name
        child.mkdir(exist_ok=True)
        summary = child/'case_summary.json'
        if summary.is_file():
            existing = cut.read(summary)
            assert fingerprint(existing) == expected
            verify_pins(existing['pins'])
            continue
        try:
            if name in G1:
                row = g1_control(build, child, name, receipt)
            elif name == 'hidden':
                row = hidden_control(build, child, receipt)
            elif name in ('G2', 'GQ'):
                row = prefix_control(build, child, name)
            elif name in SHARED:
                row = shared_control(build, child, name)
            elif name == 'G3':
                row = g3_control(build, child)
            else:
                native = root/'native2d/case_summary.json'
                if not native.is_file() and not finalize_measurement:
                    continue
                row = scoped.g4(receipt, native) if native.is_file() else {'status': 'not evaluated'}
        except (AssertionError, KeyError, OSError, RuntimeError, ValueError, subprocess.CalledProcessError) as error:
            log = child/'measurement_errors.log'
            with log.open('a') as out:
                traceback.print_exc(file=out)
            print(f'FP3-E {name}: measurement needs R3 at {log}', flush=True)
            if not finalize_measurement:
                continue
            row = {'status': 'FAILED', 'failure_kind': 'measurement', 'first_difference': str(error),
                   'pins': pins_for((log,))}
        row.update(expected)
        row.update(name=name, passed=row['status']=='passed', pins={**common, **row.get('pins', {})})
        if row['status'] == 'FAILED':
            row.setdefault('failure_kind', 'result')
        cut.save(summary, row)
        with (HERE.parent/'progress.md').open('a') as out:
            out.write(f"\nFP3-E build{label} Step 1 {name} {row['status']}, receipt {summary}.\n")
        print(f"FP3-E {name}: {row['status']} at {summary}", flush=True)
    parts, pins = {}, common.copy()
    for name in CHECKS:
        child = root/name/'case_summary.json'
        if child.is_file():
            row = cut.read(child)
            assert fingerprint(row) == expected
            verify_pins(row['pins'])
            parts[name] = {key: row.get(key) for key in ('status', 'failure_kind', 'first_difference')}
            parts[name].update(path=str(child), sha256=cut.sha(child))
            pins.update({str(child): cut.sha(child), **row['pins']})
    if len(parts) < len(CHECKS):
        if len(names)==1 and names[0] in parts:
            return parts[names[0]]['status']
        raise RuntimeError('Unsealed FP3-E Step 1 measurements; correct each logged harness error under R3.')
    status = scoped.combine(parts)
    first = next((row for row in parts.values() if row['failure_kind']=='result'),
                 next((row for row in parts.values() if row['failure_kind']=='measurement'), {}))
    summary = root/'step1_scoped.json'
    record = {**expected, 'status': status, 'passed': status=='passed', 'parts': parts, 'pins': pins,
              'failure_kind': first.get('failure_kind'), 'first_difference': first.get('first_difference')}
    if summary.is_file():
        assert cut.read(summary) == record
    else:
        cut.save(summary, record)
    return status


def worker(kind, args):
    if kind == 'prefix':
        build, root, gate, closure, watchdog, *prerequisites = args
        prefix_adapter()['controller'](Path(build), Path(root), gate, int(closure), int(watchdog),
                                       list(map(Path, prerequisites)))
        return 0
    if kind == 'g3':
        build, root, closure, timeout, *prerequisites = args
        return g3_adapter()['owned'](Path(build), Path(root), int(closure), int(timeout),
                                     list(map(Path, prerequisites)))
    label, closure, group, worker_flag, mode, dir_flag, root, sec_flag, seconds = args
    assert (worker_flag, dir_flag, sec_flag) == ('--worker','--attempt-dir','--seconds') and closure=='1'
    build = HERE / f'build{label}'
    adapter, disjoint = shared_adapter(build)
    if mode == 'first-collapse':
        adapter['hashes'] = disjoint
        adapter['first'](Path(root), build, 1)
    else:
        adapter['other'](Path(root), build, mode, 1, int(seconds))
    return 0


def self_check():
    verify_order()
    assert Path(scoped.g4.__code__.co_filename) == P3_SCOPED
    assert scoped.combine({'one': {'status': 'passed'}}) == 'passed'
    accepted()
    prefix_adapter()
    g3_adapter()
    shared, _ = shared_adapter(HERE/'build679')
    for seconds in (600, 1800):
        shared['load_bl_guard'](seconds)
    _, files = g3_reference()
    assert all(path.is_file() for path in files)
    for ranks in (1, 2, 4, 6):
        mpi = ['mpirun','-x',PROBE,'-n',str(ranks)] if ranks > 1 else []
        assert probe_receipt({'argv':['env',PROBE+'=0','nice','-n','19',*mpi,'driver']}, ranks)['value']=='0'
    # Drive the actual hidden command writer with the heavy launch intercepted.
    runner = fixtures.p2.m1.fixtures.hidden.c1.run_case
    wait = {'controller': {}}
    for (rank_count, selector, _), value, marker in HIDDEN.values():
        with TemporaryDirectory(prefix='fp3-e-controls-') as temp, bound_test(value) as env, \
                patch.object(cut, 'run_gate', return_value=wait) as launch, \
                patch.object(cut, 'terminal', return_value=True):
            case = Path(temp)
            runner(case, Path('driver'), rank_count, selector, 1350, environment=env)
            record = cut.read(case/'command.json')
            assert record['fp3_probe'] == probe_receipt(record, rank_count, value)
            assert record['argv'] == launch.call_args.args[-1]
            assert record['argv'][1:3] == ['-u', fixtures.SWITCH]
            for rank in range(rank_count):
                log = case / ('test_outer.log' if rank_count == 1 else f'rank_output/1/rank.{rank}/stdout')
                log.parent.mkdir(parents=True, exist_ok=True)
                log.write_text(f'\x1b[0mFP3_{marker} HIDDEN_PROBE rank={rank} value={value}\n')
            assert len(hidden_probe_ranks(case, rank_count, value, marker)) == rank_count
    print('FP3-E controls verified without launching native work.')


if __name__ == '__main__':
    if sys.argv[1:2] == ['--worker']:
        raise SystemExit(worker(sys.argv[2], sys.argv[3:]))
    parser = ArgumentParser(description=__doc__)
    parser.add_argument('build', type=int, nargs='?')
    parser.add_argument('control', choices=CHECKS, nargs='?')
    parser.add_argument('--self-check', action='store_true')
    parser.add_argument('--finalize-measurement-failure', action='store_true')
    args = parser.parse_args()
    if args.self_check:
        self_check()
    else:
        assert args.build is not None
        status = run(args.build, (args.control,) if args.control else CHECKS, args.finalize_measurement_failure)
        raise SystemExit(EXIT[status])
