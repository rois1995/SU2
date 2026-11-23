"""Run FP2-D off-path fixtures against S0, FP1 and G1."""

from contextlib import ExitStack
from pathlib import Path
import re
import sys
from unittest.mock import patch

from run_c2_lab import continuation_pins

sys.dont_write_bytecode = True
HERE = Path(__file__).resolve().parent
C2 = HERE.with_name('FP2_C2')
sys.path.insert(0, str(C2))
import run_c2_fixtures as prior

hidden = prior.hidden
a, cut, sha = prior.a, prior.cut, prior.sha
S0 = '240330d3aa'
ORDER = HERE / 'inputs/FAST_PATH_STAGE2_D1_REV2.md'
ORDER_SHA = '69178513638b8741ae412cd689c50bc0b2396676c3395d867bb3ba868a27d8f1'
SOURCE = HERE / 'source_check.md'
BASE_SCOPE = C2 / 'scoped/build655'
BASE_FIXTURES = C2 / 'fixtures/build655'
D_FIELDS = {'d_error_edges', 'd_marks_error', 'd_marks_longest', 'd_centroid_only', 'd_nonfinite'}


def compare_zero_fields(logs):
    count = 0
    for log in logs:
        for line in log.read_text(errors='replace').splitlines():
            if 'NATIVE3D_LOCAL_SURFACE ' not in line:
                continue
            fields = dict(re.findall(r'\b(d_[a-z_]+)=(-?\d+)', line))
            assert fields.keys() >= D_FIELDS and all(fields[key] == '0' for key in D_FIELDS), (log, line)
            count += 1
    return count


def reference_pins():
    acceptance = BASE_FIXTURES / 'c2_acceptance.json'
    reference = cut.read(acceptance)
    assert reference['passed'] and reference['build'] == 655
    paths = {Path(path): digest for path, digest in reference['receipts'].items()}
    scope_receipt = BASE_SCOPE / 'fixtures.json'
    scope = cut.read(scope_receipt)
    assert scope['passed'] and scope['G1'] == [375, 114, 228] and scope['fp1_cohorts'] == 18
    paths.update({BASE_SCOPE / name: digest for name, digest in scope['receipts'].items()})
    for path in (BASE_FIXTURES / 'wall/test_outer.log',
                 *(BASE_FIXTURES / name / 'rows.json' for name in ('F1-F4', 'F5'))):
        paths[path] = sha(path)
    for path, digest in paths.items():
        assert sha(path) == digest, path
    for path in (acceptance, scope_receipt):
        prior.scope.c3.c2.committed(path, S0)
        paths[path] = sha(path)
    return paths


def run(build, directional_only=False):
    assert sha(ORDER) == ORDER_SHA and ORDER.with_suffix('.sha256').read_text().split() == [ORDER_SHA, ORDER.name]
    hidden.clear_options()
    binary, fingerprints, pins = hidden.prepare(build)
    baseline = reference_pins()
    base = HERE / 'fixtures' / build.name
    pins.update({str(path): sha(path) for path in (ORDER, ORDER.with_suffix('.sha256'), *continuation_pins())})
    spec = (1, '[.][NativeFP2D]', 15)
    d = hidden.attempt(base / 'distance/attempt1', binary, fingerprints, pins, spec, 1800)
    if d['watchdog'] and not d['assertion_failed']:
        d = hidden.attempt(base / 'distance/attempt2', binary, fingerprints, pins, spec, 5400)
    if directional_only or not d['passed']:
        status = 'open_watchdog' if d['watchdog'] and not d['assertion_failed'] else 'failed'
        return {**d, 'status': 'passed' if d['passed'] else status}
    directional = Path(d['case']) / 'result.json'
    assert d['passed'] and all(d[key] == value for key, value in fingerprints.items())
    assert all(sha(directional.parent / name) == digest for name, digest in d['artifacts'].items())
    artifacts = {str(directional): sha(directional)}
    with ExitStack() as stack:
        stack.enter_context(patch.multiple(hidden, HERE=HERE,
                                            ORDER_FILES=(ORDER, ORDER.with_suffix('.sha256'), *continuation_pins()),
                                            SOURCE_CHECK=SOURCE))
        stack.enter_context(patch.multiple(prior, HERE=HERE, BASE_SCOPE=BASE_SCOPE, BASE_FIXTURES=BASE_FIXTURES,
                                            DRIVER=Path(__file__).resolve(), reference_pins=lambda: baseline))
        stack.enter_context(patch.object(prior.scope, 'base_receipt', lambda path: cut.read(path)))
        original_patch = prior.patch.multiple

        def scope_patch(module, **kwargs):
            if module is prior.scope and 'BASE' in kwargs:
                kwargs.update(ORDER=ORDER, ORDER_SHA=ORDER_SHA, BASE=S0)
            return original_patch(module, **kwargs)

        stack.enter_context(patch.object(prior.patch, 'multiple', scope_patch))
        # Preserve the inherited source verification for the run itself.
        stack.enter_context(patch.object(hidden, 'verify_orders', lambda: None))
        for name in hidden.CASES:
            result = hidden.run_hidden(build, name)
            path, status = prior.verify_hidden(base, fingerprints, name)
            if status != 'passed':
                return result
            old = cut.read(BASE_FIXTURES / name / 'status.json')
            candidate = cut.read(path.parent / next(reversed(result['attempts'])))
            reference = cut.read(BASE_FIXTURES / name / next(reversed(old['attempts'])))
            assert candidate['catch_summaries'] == reference['catch_summaries'], name
            artifacts[str(path)] = sha(path)
        counts, receipts = prior.run_cohort(build, fingerprints)
    for path in receipts:
        artifacts[str(path)] = sha(path)
    scoped = cut.read(receipts[0])
    assert scoped['fp1_cohorts'] == 18
    logs = [*base.glob('**/test_outer.log'), *base.glob('**/rank.*/stdout'),
            *receipts[0].parent.glob('**/test_outer.log'), *receipts[0].parent.glob('**/rank.*/stdout')]
    off_rows = compare_zero_fields(log for log in logs if 'distance' not in log.relative_to(HERE).parts)
    receipt = {**fingerprints, 'passed': True, 'G1': counts, 'fp1_cohorts': scoped['fp1_cohorts'],
               'option_off_print_rows': off_rows, 'artifacts': artifacts}
    a.sealed(base / 'd_acceptance.json', receipt)
    return receipt


if __name__ == '__main__':
    try:
        result = run(Path(sys.argv[1]).resolve(), '--directional-only' in sys.argv[2:])
    except prior.WatchdogOpen as error:
        print(error, flush=True)
        raise SystemExit(75)
    print(result, flush=True)
    raise SystemExit(0 if result['passed'] else 75 if result['status'].startswith('open') else 1)
