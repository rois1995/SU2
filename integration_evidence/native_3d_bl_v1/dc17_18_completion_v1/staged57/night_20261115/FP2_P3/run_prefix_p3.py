"""Bind the inherited G2/GQ controller and comparators to FP2-P3."""
from collections import Counter, defaultdict
from dataclasses import replace
from itertools import zip_longest
import os
from pathlib import Path
import runpy
import sys

from build_p3 import HERE, cut, verify_build

N = HERE.parent
sys.path.insert(0, str(N / 'FP1'))
import run_prefix_fp1 as prefix
sys.path.remove(str(N / 'FP1'))

ORDER = HERE / 'inputs/FP2_P3_CLOSURE_DEFAULT.md'
SOURCE_CHECK = HERE / 'reports/source_check.md'
HISTORY = {'G2': prefix.G2_REFERENCE, 'GQ': prefix.GQ_REFERENCE}
FX1 = N / 'FX1/compare_prefix.py'
N22 = N / 'N22/compare_prefix.py'
SCHEDULING = N / 'FP1/resume6/scheduling_calls_reference.json'
SWITCH = 'SU2_NATIVE_CONFORMING_WITNESS'
TRACE = 'SU2_NATIVE_GEOMETRY_WORK_TRACE'
MARKER = 'NATIVE_SURFACE_CONFORMING_WITNESS enabled=0'
FILES = ('m6.su2', 'm6.su2.native_ref', 'baseline_euler_seed.dat')
WORK_LIMIT = 108459426
COUNTS = (480, 473, 346, 263, 469, 464)
WATCHDOG = 6000


def bind_once(source, old, new):
    assert source.count(old) == 1, old
    return source.replace(old, new, 1)


def controller(build, root, gate, closure, watchdog, prerequisites):
    receipt = verify_build(int(build.name.removeprefix('build')))
    source = runpy.run_path(str(prefix.CAPTURE))['load_runner']()
    original = source['patched_controller']
    status_file, pins_file = build / 'build_status.json', build / 'source_pins.json'
    runtime = source['A2'] / 'validation/unchanged_build499_source_pins.json'
    solver = Path(receipt['binary'])

    def checked_pins(strict=True):
        compiled, historical = cut.read(pins_file), cut.read(runtime)
        assert not compiled.keys() & historical.keys()
        pins = {**compiled, **historical}
        mismatches = {path: digest for path, digest in pins.items() if cut.sha(path) != digest}
        if strict:
            assert not mismatches, mismatches
        return {'binary': str(solver), 'source_build_binary': str(solver), 'source_pins': pins,
                'binary_sha256': cut.sha(solver), 'binary_matches_build_status': cut.sha(solver) == receipt['binary_sha256'],
                'source_pin_mismatches': mismatches, 'build_status': str(status_file),
                'source_pins_sha256': cut.sha(pins_file), 'additional_source_pins_sha256': cut.sha(runtime)}

    def patch():
        text = original()
        bindings = (
            ('GROUP / "source_check.md"', f'Path({str(SOURCE_CHECK)!r})'),
            ('PURPOSE / "validate_g2_gq.py"', f'Path({str(prefix.ORIGINAL / "validate_g2_gq.py")!r})'),
            ('PURPOSE / "run_g2_gq_prefix.py"', f'Path({str(prefix.CAPTURE)!r})'),
            ('BUILD / "terminal_handles.json"', 'BUILD / "build_wait.json"'),
            ('BUILD / "generated_build_graph_after.json"', 'BUILD / "build_plan.json"'),
        )
        for old, new in bindings:
            assert old in text, old
            text = text.replace(old, new)
        support = (ORDER, ORDER.with_suffix('.sha256'), FX1, N22, SCHEDULING, *prerequisites)
        text = bind_once(text, '    input_hashes.update(runtime_pins)',
                         '    input_hashes.update(runtime_pins)\n'
                         f'    for name in {tuple(map(str, support))!r}:\n'
                         '        input_hashes[name] = sha(Path(name))')
        settings = {TRACE: '0', **({SWITCH: '0'} if closure == 0 else {})}
        exports = [part for key in settings for part in ('-x', key)]
        text = bind_once(text, '    env.update(clean_provenance["environment"])',
                         '    env.update(clean_provenance["environment"])\n'
                         f'    env.pop({SWITCH!r}, None)\n'
                         f'    env.update({settings!r})\n'
                         '    mpi = argv.index("mpirun") + 1\n'
                         f'    argv[mpi:mpi] = {exports!r}')
        text = bind_once(text, 'if key in clean_provenance["environment"]',
                         f'if key in clean_provenance["environment"] or key in {tuple(settings)!r}')
        if closure:
            text = bind_once(text,
                    'p6_collapse["work_equals_G2"] = p6_collapse["upper_work_sum"] == EXPECTED_G2_WORK',
                    'p6_collapse["work_within_limit"] = p6_collapse["upper_work_sum"] <= MAX_WORK')
            text = bind_once(text, 'p6_collapse["work_equals_G2"] and', 'p6_collapse["work_within_limit"] and')
        if watchdog != WATCHDOG:
            text = bind_once(text, 'elapsed >= 6000', f'elapsed >= {watchdog}')
            text = bind_once(text, '{gate}_6000_second_controller_watchdog',
                             f'{{gate}}_{watchdog}_second_controller_watchdog')
            # The shell wrapper must outlast the prefix watchdog on the one retry.
            text = bind_once(text, '    provenance = {', f'    argv[argv.index("timeout") + 1] = {str(3 * 14400)!r}\n'
                             '    provenance = {')
            text = bind_once(text, '"application_watchdog_seconds": 14400',
                             f'"application_watchdog_seconds": {3 * 14400}')
        return text

    import importlib.util
    spec = importlib.util.spec_from_file_location('validate_g2_gq', prefix.ORIGINAL / 'validate_g2_gq.py')
    validator = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = validator
    spec.loader.exec_module(validator)
    source.update(HERE=root, BUILD=build, CASES=root / 'cases', patched_controller=patch)
    env, _ = source['namespace']()
    env.update(SCRIPT=Path(__file__).resolve(), verify_pins=checked_pins)
    os.environ['OMPI_MCA_osc'] = 'pt2pt'
    sys.argv = [str(env['N4_SCRIPT']), str(root / 'cases' / gate), '--gate', gate]
    env['main']()


def switch_receipt(case, closure):
    p = cut.read(case / 'provenance.json')
    settings = p['environment']
    argv = p['argv']
    assert settings.get(SWITCH) == ('0' if closure == 0 else None), settings.get(SWITCH)
    assert settings[TRACE] == '0'
    mpi = argv.index('mpirun')
    assert argv.count(SWITCH) == (1 if closure == 0 else 0) and argv.count(TRACE) == 1
    assert argv[mpi + 1:mpi + (5 if closure == 0 else 3)] == (
        ['-x', TRACE, '-x', SWITCH] if closure == 0 else ['-x', TRACE])
    logs = {int(path.parent.name.rsplit('.', 1)[1]): path
            for path in case.glob('rank_output/*/rank.*/stderr')}
    counts = {rank: path.read_text(errors='replace').count(MARKER) for rank, path in logs.items()}
    return {'environment': {SWITCH: settings.get(SWITCH), TRACE: settings[TRACE]},
            'ranks': counts, 'logs': {str(path): cut.sha(path) for path in logs.values()}}


def verify_attempt(build_dir, root, gate, closure, watchdog, reference):
    case = root / 'cases' / gate
    status = cut.read(case / 'status.json')
    p = cut.read(case / 'provenance.json')
    wait = cut.read(root / f'{gate}_wait.json')
    assert wait['terminal'] and cut.terminal(wait['controller'])
    assert status['all_owned_handles_terminal'] and all(cut.terminal(row) for row in status['owned_identities'])
    assert status['inputs_unchanged'] and status['source_and_binary_pins_unchanged']
    build = cut.read(build_dir / 'build_status.json')
    assert p['source_pins_sha256'] == build['source_pins_sha256'] and p['binary_sha256'] == build['binary_sha256']
    ref = cut.read(reference / 'provenance.json')
    assert p['config_sha256'] == ref['config_sha256'] == cut.sha(case / 'run.cfg')
    for k in FILES:
        hashes = [{Path(path).name: digest for path, digest in record['input_hashes'].items()}
                  for record in (p, ref)]
        assert hashes[0][k] == hashes[1][k], k
    pins = {str(path): cut.sha(path) for path in (
        root / f'{gate}_wait.json', root / f'{gate}_host_preflight.json',
        case / 'status.json', case / 'provenance.json', case / 'comparison.json',
        *case.glob('*collapse_records.json'), reference / 'provenance.json', *case.glob('*gate_receipt.json'),
        *case.glob('rank_output/*/rank.*/std*'), *reference.glob('rank_output/*/rank.*/stdout'))}
    assert all(cut.sha(path) == digest for path, digest in p['input_hashes'].items())
    switch = switch_receipt(case, closure)
    record = {'case': str(case), 'gate': gate, 'watchdog_seconds': watchdog, 'switch': switch,
              'pins': pins, 'stop_reason': status['stop_reason']}
    if status['gate_passed']:
        assert not wait['returncode'] and status['native_case_started']
        assert status['stop_reason'] == f'{gate}_prefix_passed'
        assert status['first_collapse_gate']['upper_work_sum'] <= WORK_LIMIT
        assert tuple(row['accepted'] for row in status['first_collapse_gate']['per_rank']) == COUNTS
        assert set(switch['ranks']) == set(range(6))
        assert all(count == (1 if closure == 0 else 0) for count in switch['ranks'].values())
        return {**record, 'status': 'passed', 'receipt': status['gate_receipt']}
    first = status['stop_reason']
    prefix_gate = case / 'G2_gate_receipt.json'
    if prefix_gate.is_file():
        one = cut.read(prefix_gate)
        if not one['passed']:
            first = one
    if (first == f'{gate}_{watchdog}_second_controller_watchdog' and
            all(count <= (1 if closure == 0 else 0) for count in switch['ranks'].values())):
        rows = status['first_collapse_gate']['per_rank']
        bad = next((row for row in rows if row['accepted'] != COUNTS[row['rank']]), None)
        refinements = [row for path in case.glob('*collapse_records.json')
                       for row in cut.read(path).values() if int(row['upper_refinements']) != 0]
        work = status['first_collapse_gate']['upper_work_sum']
        limit = WORK_LIMIT if closure or len(rows) < 6 else 90809869
        if bad is None and not refinements and work <= limit and (closure or len(rows) < 6 or work == limit):
            return {**record, 'status': 'OPEN', 'first_difference': first}
        first = bad or (refinements[0] if refinements else {'observed_work': work, 'limit': limit})
    return {**record, 'status': 'FAILED', 'first_difference': first}


def capture(build, root, gate, closure, prerequisites):
    attempts = []
    for i, watchdog in enumerate((WATCHDOG, 3 * WATCHDOG), 1):
        attempt = root / f'attempt{i}'
        attempt.mkdir(parents=True, exist_ok=True)
        argv = [cut.PYTHON, '-B', str(Path(__file__).resolve()), str(build), str(attempt), gate,
                str(closure), str(watchdog), *map(str, prerequisites)]
        if not (attempt / f'{gate}_wait.json').is_file():
            from checkpoint import host_preflight
            host_preflight()
            try:
                cut.run_gate(attempt, gate, argv)
            except RuntimeError:
                if not (attempt / f'{gate}_wait.json').is_file():
                    raise
        item = verify_attempt(build, attempt, gate, closure, watchdog, HISTORY[gate])
        attempts.append(item)
        if item['status'] != 'OPEN':
            break
    return {**attempts[-1], 'attempts': attempts}


def old_comparison(gate, candidate):
    fx1 = runpy.run_path(str(FX1))
    result = fx1['compare_cases']('C', gate, HISTORY[gate], candidate)
    if result['passed']:
        assert result['fixedstorage'] == 4120 and result['scheduling_reference'] == str(SCHEDULING)
    return result


def count_occurrences(before, after, comparator):
    counts = defaultdict(Counter)
    parser = comparator['PARSER']

    def count(left, right, different):
        for marker in left.keys() | right.keys():
            for a, b in zip_longest(left.get(marker, ()), right.get(marker, ())):
                if a is None:
                    counts[marker]['extra'] += 1
                elif b is None:
                    counts[marker]['missing'] += 1
                elif different(a, b):
                    counts[marker]['changed'] += 1
            counts[marker]

    def project(stream):
        plain, timers = defaultdict(list), defaultdict(list)
        for row in stream.get('plain', ()):
            plain[row.marker].append(replace(
                row, fields={k: v for k, v in row.fields.items() if k not in comparator['C_FIELDS']}))
        for snapshot in stream.get('timers', ()):
            timers['NATIVE3D_TIMER_SUMMARY'].append(snapshot['summary'])
            census = 0
            for row in snapshot['rows']:
                fields = row['fields']
                if fields['operation'] == 'census':
                    census += parser.parse_calls(fields, row['path'], row['line'])
                elif fields['operation'] != 'communication':
                    timers['NATIVE3D_TIMER'].append(fields)
            adjustment = comparator['DELTA']['wall_law_before'](snapshot)
            timers['NATIVE3D_TIMER/census'].append((snapshot['key'], census - adjustment))
        return plain, timers, {'NATIVE3D_STORAGE': stream.get('storage', ())}

    for stream in before.keys() | after.keys():
        old, new = (project(docs.get(stream, {})) for docs in (before, after))
        count(old[0], new[0], comparator['N4'].compare_row)
        for name in old[1].keys() | new[1].keys():
            ignored = (parser.SUMMARY_MEASUREMENTS if name.endswith('SUMMARY') else parser.TIMER_MEASUREMENTS)
            diff = (lambda a, b: a != b) if name.endswith('/census') else (
                lambda a, b: parser.exact_fields(a, b, ignored))
            count({name: old[1].get(name, ())}, {name: new[1].get(name, ())}, diff)
        count(old[2], new[2], lambda a, b: a != b)
    return {key: {name: vals[name] for name in ('changed', 'missing', 'extra')}
            for key, vals in sorted(counts.items())}


def compare_pair(gate, first, second):
    comparator = runpy.run_path(str(N22))
    before = Path(first['case'])
    after = Path(second['case'])
    pair = comparator['compare_cases']('C', gate, before, after)
    old = comparator['compare_cases']('C', gate, HISTORY[gate], before)
    histories = [comparator['parse_case'](root, gate) for root in (HISTORY[gate], before)]
    return {'status': 'passed' if pair['passed'] else 'FAILED', 'pair': pair,
            'first_difference': pair.get('first_difference'),
            'historical': {'first_difference': old.get('first_difference'),
                           'ownership': old['decisions'],
                           'projected_rows': count_occurrences(*histories, comparator)}}


if __name__ == '__main__':
    build, root, gate, closure, watchdog, *prerequisites = sys.argv[1:]
    controller(Path(build), Path(root), gate, int(closure), int(watchdog), list(map(Path, prerequisites)))
