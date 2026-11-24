"""Run the FP2-C3 contact fixtures, S0 option-off comparison, FP1 and G1."""

from argparse import ArgumentParser
from contextlib import ExitStack
from pathlib import Path
import re
import sys
from unittest.mock import patch

import run_c2_lab as runner

HERE, D, S0, ORDER, ORDER_SHA = runner.HERE, runner.D, runner.S0, runner.ORDER, runner.ORDER_SHA
DRIVER = Path(__file__).resolve()
sys.path.insert(0, str(HERE.with_name('FP2_C2')))
import run_c2_fixtures as prior

hidden = prior.hidden
a, cut, sha = prior.a, prior.cut, prior.sha
OLD_FIXTURES = D / 'fixtures/build660'
OLD_SCOPE = D / 'scoped/build660'
OLD = OLD_FIXTURES / 'd_acceptance.json'
CONTACT = {*runner.CONTACT_COUNTERS, *runner.CONTACT_CLOCKS, 'c_refused_contact', 'c_refused_contact_remote'}
FOCUSED = {'contact_serial': (1, '[.][NativeFP2C3]~[NativeFP2C3MPI]', 7),
           'contact_mpi': (2, '[.][NativeFP2C3][NativeFP2C3MPI]', 3)}


def verify_orders():
    for path, digest in ((ORDER, ORDER_SHA), (runner.WATCHDOG_ORDER, runner.WATCHDOG_ORDER_SHA)):
        assert sha(path) == digest
        assert path.with_suffix(".sha256").read_text().split() == [digest, path.name]


def reference_pins():
    prior.scope.c3.c2.committed(OLD, S0)
    old = cut.read(OLD)
    assert old['passed'] and old['build'] == 660
    assert old['G1'] == [375, 114, 228] and old['fp1_cohorts'] == 18
    pins = {Path(path): digest for path, digest in old['artifacts'].items()}
    pins.update({OLD_SCOPE / name: digest for name, digest in cut.read(OLD_SCOPE / 'fixtures.json')['receipts'].items()})
    for reference in old['artifacts']:
        path = Path(reference)
        target = path if path.name == 'result.json' else path.parent / 'attempt1/result.json'
        if target.is_file():
            pins[target] = sha(target)
            pins.update({target.parent / name: digest for name, digest in cut.read(target)['artifacts'].items()})
    for path in (OLD_FIXTURES / 'wall/test_outer.log',
                 *(OLD_FIXTURES / sub / 'rows.json' for sub in ('F1-F4', 'F5'))):
        pins[path] = sha(path)
    assert all(sha(path) == digest for path, digest in pins.items())
    return {OLD: sha(OLD), **pins}


def off_rows(logs):
    count = 0
    for path in logs:
        for line in path.read_text(errors='replace').splitlines():
            if 'NATIVE3D_LOCAL_SURFACE ' not in line:
                continue
            values = dict(re.findall(r'\b([a-z_]+)=([^\s]+)', line))
            assert CONTACT <= values.keys(), (path, line)
            assert all(float(values[key]) == 0 for key in CONTACT), (path, line)
            count += 1
    return count


def attest(result, fingerprints):
    path = Path(result['case']) / 'result.json'
    assert all(result[key] == value for key, value in fingerprints.items()), path
    assert all(sha(path.parent / name) == digest for name, digest in result['artifacts'].items()), path
    return path


def focus(directory, binary, fingerprints, pins, spec):
    result = hidden.attempt(directory / 'attempt1', binary, fingerprints, pins, spec, 1350)
    receipts = [attest(result, fingerprints)]
    if result['watchdog'] and not result['assertion_failed']:
        result = hidden.attempt(directory / 'attempt2', binary, fingerprints, pins, spec, 4050)
        receipts.append(attest(result, fingerprints))
    status = ('passed' if result['passed'] else 'open_r6'
              if result['watchdog'] and not result['assertion_failed'] else 'failed')
    return {**result, 'status': status}, receipts


def add_progress(message):
    with (HERE.parent / 'progress.md').open('a') as stream:
        stream.write('\n' + message + '\n')


def run(build, selected):
    verify_orders()
    hidden.clear_options()
    binary, fingerprints, pins = hidden.prepare(build)
    baseline = reference_pins()
    pins.update({str(path): sha(path) for path in
                 (DRIVER, ORDER, ORDER.with_suffix('.sha256'), runner.SOURCE_CHECK, *baseline)})
    base = HERE / 'fixtures' / build.name
    artifacts = {}
    off_logs = []
    open_r6 = []
    for name, spec in FOCUSED.items():
        if selected not in ('all', name):
            continue
        result, paths = focus(base / name, binary, fingerprints, pins, spec)
        artifacts.update({str(path): sha(path) for path in paths})
        if selected != 'all' or result['status'] == 'failed':
            return result
        if result['status'] == 'open_r6':
            open_r6.append(name)
    add_progress(f'FP2-C3 build{fingerprints["build"]} focused serial/MPI receipts sealed. '
                 f'R6-open groups: {open_r6}. Starting option-off, FP1 and G1.')

    distance, paths = focus(base / 'd_control', binary, fingerprints, pins, (1, '[.][NativeFP2D]', 18))
    artifacts.update({str(path): sha(path) for path in paths})
    if not distance['passed']:
        return distance
    candidate = Path(distance['case']) / 'test_outer.log'
    reference = OLD_FIXTURES / 'distance/attempt1/test_outer.log'
    d_rows = lambda path: [line for line in path.read_text().splitlines() if line.startswith('FP2_D ')]
    assert d_rows(candidate) == d_rows(reference), (d_rows(candidate), d_rows(reference))
    assert distance['catch_summaries'] == cut.read(reference.with_name('result.json'))['catch_summaries']
    off_logs.append(candidate)

    with ExitStack() as stack:
        stack.enter_context(patch.multiple(hidden, HERE=HERE, ORDER_FILES=(ORDER, ORDER.with_suffix('.sha256')),
                                            SOURCE_CHECK=runner.SOURCE_CHECK))
        stack.enter_context(patch.multiple(prior, HERE=HERE, BASE_SCOPE=OLD_SCOPE, BASE_FIXTURES=OLD_FIXTURES,
                                            DRIVER=DRIVER, reference_pins=lambda: baseline))
        stack.enter_context(patch.object(prior.scope, 'base_receipt', lambda path: cut.read(path)))
        original_patch = prior.patch.multiple

        def scope_patch(module, **kwargs):
            if module is prior.scope and 'BASE' in kwargs:
                kwargs.update(ORDER=ORDER, ORDER_SHA=ORDER_SHA, BASE=S0)
            return original_patch(module, **kwargs)

        stack.enter_context(patch.object(prior.patch, 'multiple', scope_patch))
        stack.enter_context(patch.object(hidden, 'verify_orders', lambda: None))
        for name in hidden.CASES:
            status = hidden.run_hidden(build, name)
            path, state = prior.verify_hidden(base, fingerprints, name)
            if state != 'passed':
                return status
            candidate = path.parent / next(reversed(status['attempts']))
            old = cut.read(OLD_FIXTURES / name / 'status.json')
            reference = OLD_FIXTURES / name / next(reversed(old['attempts']))
            assert cut.read(candidate)['catch_summaries'] == cut.read(reference)['catch_summaries'], name
            artifacts[str(path)] = sha(path)
            off_logs.extend(candidate.parent / p for p in cut.read(candidate)['artifacts']
                            if p.endswith(('stdout', 'test_outer.log')))
        counts, receipts = prior.run_cohort(build, fingerprints)

    artifacts.update({str(path): sha(path) for path in receipts})
    scoped = cut.read(receipts[0])
    assert scoped['fp1_cohorts'] == 18
    off_logs.extend(p for archive in (receipts[0].parent, base / 'wall') for p in archive.glob('**/*')
                    if p.name in ('stdout', 'test_outer.log'))
    receipt = {**fingerprints, 'passed': not open_r6, 'status': 'open_r6' if open_r6 else 'passed',
               'G1': counts, 'fp1_cohorts': scoped['fp1_cohorts'], 'r6_open': open_r6,
               'option_off_print_rows': off_rows(off_logs), 'artifacts': artifacts}
    a.sealed(base / 'c3_acceptance.json', receipt)
    return receipt


if __name__ == '__main__':
    parser = ArgumentParser(description=__doc__)
    parser.add_argument('build', type=Path, nargs='?')
    parser.add_argument('--only', choices=('all', *FOCUSED), default='all')
    parser.add_argument('--self-check', action='store_true')
    args = parser.parse_args()
    if args.self_check:
        verify_orders()
        print(f'Pinned {len(reference_pins())} FP2-D option-off artifacts.')
        sys.exit(0)
    try:
        result = run(args.build.resolve(), args.only)
    except prior.WatchdogOpen as error:
        print(error, flush=True)
        sys.exit(75)
    print(result, flush=True)
    sys.exit(0 if result['passed'] else 75 if result.get('status', '').startswith('open') else 1)
