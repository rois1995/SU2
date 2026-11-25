"""Run FP2-P3 scoped controls, repeatable prefixes and BL/2D checks."""
from argparse import ArgumentParser
from pathlib import Path
import re
import subprocess
import traceback

from build_p3 import HERE, cut, verify, verify_build
import run_g3_p3 as bl
import run_prefix_p3 as prefix

N = HERE.parent
G4_REFERENCE = bl.SCOPE / 'g4_case_paths.json'
CATCH = re.compile(r'All tests passed \((\d+) assertions? in (\d+) test cases?\)')
ANSI = re.compile(r'\x1b\[[0-9;]*m')
G4_RANKS = (1, 2, 4)
FILTER = '[NativeEngine2D],[NativeMesh2D],[NativeBL2D]'


def fingerprint(build):
    return {key: build[key] for key in ('build', 'binary_sha256', 'test_binary_sha256', 'source_pins_sha256')}


def catch_summary(log):
    return sorted(CATCH.findall(ANSI.sub('', log.read_text(errors='replace'))))


def g4(build, shared):
    expected = cut.read(G4_REFERENCE)
    assert expected['passed'] and ','.join(expected['required_filters']) == FILTER
    rows = cut.read(shared)
    assert fingerprint(rows) == fingerprint(build)
    verify(rows['pins'])
    pins = {str(G4_REFERENCE): cut.sha(G4_REFERENCE), str(shared): cut.sha(shared)}
    by_rank = {}
    for ref in expected['new_cases']:
        folder = Path(ref['case'])
        command = cut.read(folder / 'command.json')
        assert FILTER in command['argv'], command['argv']
        old = folder / 'run.log'
        assert cut.sha(old) == ref['run_log_sha256']
        pins[str(old)] = ref['run_log_sha256']
        pins[str(folder / 'command.json')] = cut.sha(folder / 'command.json')
        by_rank[command['ranks']] = catch_summary(old)
    assert set(by_rank) == set(G4_RANKS)
    results = {}
    for rank in G4_RANKS:
        part = rows['subruns'][f'2d{rank}']
        actual = None
        first = part.get('first_failure')
        if part['status'] == 'passed':
            receipt = part['switch_receipt']
            actual_path = Path(receipt['path'])
            case = actual_path.parent
            command = cut.read(case / 'command.json')
            assert command['ranks'] == rank and FILTER in command['argv']
            assert command['argv'][command['argv'].index('timeout') + 1] == '600'
            actual = catch_summary(actual_path)
            assert len(by_rank[rank]) == rank, by_rank[rank]
            if actual != by_rank[rank]:
                first = {'field': 'Catch summaries', 'expected': by_rank[rank], 'actual': actual}
        status = 'FAILED' if first and part['status'] != 'OPEN' else part['status']
        results[str(rank)] = {'status': status, 'historical_summaries': by_rank[rank], 'actual_summaries': actual,
                              'first_difference': first}
    status = combine(results)
    return {'status': status, 'passed': status == 'passed', 'ranks': results, 'pins': pins,
            'required_filters': expected['required_filters'], 'executed_receipt': str(shared)}


def combine(items):
    values = {row['status'] for row in items.values()}
    if 'FAILED' in values:
        return 'FAILED'
    if values == {'passed'}:
        return 'passed'
    return 'OPEN'


def shared_run(build, closure, group):
    label = build['build']
    path = HERE / 'shared' / f'build{label}' / f'closure{closure}' / group / 'case_summary.json'
    if not path.is_file():
        subprocess.run([cut.PYTHON, '-B', str(HERE / 'run_shared.py'), str(label), str(closure), group], check=False)
    data = cut.read(path)
    assert fingerprint(data) == fingerprint(build)
    verify(data['pins'])
    return path, data


def control_prefix(build, root, gate, prerequisites):
    capture = prefix.capture(build, root / gate, gate, 0, prerequisites)
    if capture['status'] == 'passed':
        report = prefix.old_comparison(gate, Path(capture['case']))
        capture.update(status='passed' if report['passed'] else 'FAILED',
                       comparison=report, first_difference=report.get('first_difference'))
    return capture


def staged_g4(build, closure):
    shared, _ = shared_run(build, closure, 'native2d')
    return g4(build, shared)


def control_shared(build, group):
    shared, raw = shared_run(build, 0, group)
    return {'status': raw['status'], 'receipts': raw['subruns'], 'pins': {str(shared): cut.sha(shared)}}


def candidate_prefixes(build, root, gate, prerequisites):
    results = [prefix.capture(build, root / gate / f'capture{i}', gate, 1, prerequisites)
               for i in (1, 2)]
    status = combine({idx: row for idx, row in enumerate(results)})
    pair = prefix.compare_pair(gate, *results) if status == 'passed' else {'status': status}
    return {**pair, 'captures': results, 'pins': {
        path: digest for row in results for path, digest in row['pins'].items()}}


def steps(build_dir, root, phase, build, prerequisites):
    def g3(closure):
        return bl.run(build_dir, root / 'G3', closure, prerequisites)

    if phase == 'B2a':
        for gate in ('G2', 'GQ'):
            yield gate, lambda gate=gate: control_prefix(build_dir, root, gate, prerequisites)
        yield 'G3', lambda: g3(0)
        yield 'G4', lambda: staged_g4(build, 0)
        for group in ('merit481', 'saved481', 'saved-current'):
            yield group, lambda group=group: control_shared(build, group)
    elif phase == 'C3':
        for gate in ('G2', 'GQ'):
            yield gate, lambda gate=gate: candidate_prefixes(build_dir, root, gate, prerequisites)
    else:
        yield 'G3', lambda: g3(1)
        yield 'G4', lambda: staged_g4(build, 1)


def run(build, phase, finalize_measurement=False):
    from checkpoint import require
    label = int(build.name.removeprefix('build'))
    receipt = verify_build(label)
    prereqs = require(label, *('B0', 'B1') if phase == 'B2a' else ('B',))
    out = HERE / 'scoped' / build.name
    out.mkdir(parents=True, exist_ok=True)
    root = out / phase
    root.mkdir(exist_ok=True)
    output = out / f'{phase}.json'
    if output.is_file():
        archived = cut.read(output)
        assert fingerprint(archived) == fingerprint(receipt)
        verify(archived['pins'])
        return archived
    helpers = (Path(__file__).resolve(), Path(prefix.__file__).resolve(), Path(bl.__file__).resolve(),
               prefix.ORDER, prefix.ORDER.with_suffix('.sha256'), prefix.SOURCE_CHECK,
               build / 'build_status.json', build / 'source_pins.json', *prereqs)
    parts = {}
    pins = {str(file): cut.sha(file) for file in helpers}
    for name, measure in steps(build, root, phase, receipt, prereqs):
        destination = root / f'{name}.json'
        if destination.is_file():
            row = cut.read(destination)
            verify(row.get('pins', {}))
        else:
            try:
                row = measure()
            except (AssertionError, KeyError, OSError, RuntimeError, ValueError, subprocess.CalledProcessError) as error:
                with (root / f'{name}_measurement_errors.log').open('a') as log:
                    log.write(traceback.format_exc())
                print(f'{phase} {name} measurement needs R3: {error}', flush=True)
                if not finalize_measurement:
                    continue
                row = {'status': 'FAILED', 'measurement_error': str(error),
                       'first_difference': {'measurement_error': str(error)}}
            cut.save(destination, row)
        parts[name] = row
        pins[str(destination)] = cut.sha(destination)
        pins.update(row.get('pins', {}))
        print(f'{phase} {name} {row["status"]}: {destination}', flush=True)
        with (N / 'progress.md').open('a') as f:
            f.write(f'\nFP2-P3 build{label} {phase}/{name} {row["status"]}. Receipt {destination}.\n')
    if len(parts) != {'B2a': 7, 'C3': 2, 'C4': 2}[phase]:
        raise RuntimeError(f'{phase} has unsealed measurements under {root}; see *_measurement_errors.log')
    status = combine(parts)
    result = {**fingerprint(receipt), 'phase': phase, 'status': status, 'passed': status == 'passed',
              'parts': {name: {'status': row['status'], 'path': str(root / f'{name}.json'),
                               'first_difference': row.get('first_difference')} for name, row in parts.items()},
              'pins': pins}
    cut.save(output, result)
    return result


if __name__ == '__main__':
    parser = ArgumentParser(description=__doc__)
    parser.add_argument('build', type=Path)
    parser.add_argument('phase', choices=('B2a', 'C3', 'C4'))
    parser.add_argument('--finalize-measurement-failure', action='store_true')
    args = parser.parse_args()
    result = run(args.build.resolve(), args.phase, args.finalize_measurement_failure)
    print(result['status'], flush=True)
    raise SystemExit({'passed': 0, 'FAILED': 1, 'OPEN': 75}[result['status']])
