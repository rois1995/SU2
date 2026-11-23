"""Replay build652's intentional N1 proof failure with the continuation diagnostic rows."""

from argparse import ArgumentParser
from hashlib import sha256
import json
from pathlib import Path
import re
import subprocess
from unittest.mock import patch

import run_c2_lab as c2

lab, a, cut = c2.lab, c2.a, c2.a.cut
DRIVER = Path(__file__).resolve()
HERE = c2.HERE
ORDER = HERE / 'inputs/FP2_C2_CONTINUE_1.md'
ORDER_SHA = '7480af812c7173c1c574c24be6406b4db2d421701bd4bc569263b82ba3056ee2'
SOURCE_CHECK = HERE / 'continuation1_source_check.md'
BASE = '71d4ba519b'
PRODUCTION = '553ea0f147'
BASE_BUILD = HERE / 'build652'
BASE_CASE = HERE / 'cases/build652/N1'
RUNS = {'N1': c2.RUNS['N1']}
WATCHDOG = c2.WATCHDOG
LINE = re.compile(c2.LINE.pattern.replace('|NATIVE3D_LOCAL_SURFACE',
                                          '|FP2_C2 LAB_(?:ABOVE|MODEL_GAP)|NATIVE3D_LOCAL_SURFACE'))
EXPECTED = {'collapses_surface': '5937', 'classify_above_tol': '4', 'classify_excess_bins': '0,2,1,0,1',
            'classify_unexplained': '4', 'qualified': '0'}
INPUT_DATA = lab.b0.input_data
EXPECTED_ASSERTION = re.compile(r': FAILED:\s+REQUIRE\(\s*census\.unexplained == 0\s*\)')


def production_pins(manifest):
    directories = (cut.REPO / 'Common', cut.REPO / 'SU2_CFD')
    return {name: digest for name, digest in cut.read(manifest).items()
            if any(Path(name).is_relative_to(directory) for directory in directories)}


def verify_production(build, receipt):
    subprocess.run(['git', 'diff', '--exit-code', PRODUCTION, '--', 'Common', 'SU2_CFD'],
                   cwd=cut.REPO, check=True)
    assert production_pins(build / 'source_pins.json') == production_pins(BASE_BUILD / 'source_pins.json')
    assert receipt['binary_sha256'] == cut.read(BASE_BUILD / 'build_status.json')['binary_sha256']


def pinned_input(ranks):
    config, sensor, copies, pins, extra = INPUT_DATA(ranks)
    historical = cut.read(BASE_CASE / 'provenance.json')['pins']
    assert sha256(config).hexdigest() == historical[str(BASE_CASE / 'run.cfg')]
    assert a.sha(sensor) == historical[str(sensor)]
    for source, digest in copies.items():
        assert digest == historical[str(BASE_CASE / source.name)], source
    return config, sensor, copies, pins, extra


def evaluate(name, data, unused):
    rank = data.get(0, {})
    observed, differences = {}, []
    for marker, fields in (('NATIVE3D_LOCAL_SURFACE', ('collapses_surface',)),
                           ('LAB_S_PROOF', tuple(EXPECTED)[1:])):
        rows = rank.get(marker, ())
        if len(rows) > 1:
            differences.append({'row': marker, 'observed_rows': len(rows), 'expected_rows': 1})
        if rows:
            for field in fields:
                observed[field] = rows[0].get(field)
                if observed[field] != EXPECTED[field]:
                    differences.append({'field': field, 'observed': observed[field], 'expected': EXPECTED[field]})

    # The reproduced mesh-side proof failure is the expected outcome in this diagnostic.
    gates = [row for row in c2.observed_failures(name, data)
             if row['field'] not in ('proof_unqualified', 'classify_unexplained')]
    return {'expected': EXPECTED, 'observed': observed, 'differences': differences,
            'failed_limits': gates, 'problems': [], 'above': rank.get('LAB_ABOVE', []),
            'model_gap': rank.get('LAB_MODEL_GAP', []), 'entities': rank.get('LAB_S_ENTITY', [])}


def diagnostic_problems(measurement):
    above, gaps = measurement['above'], measurement['model_gap']
    above_keys = 'key v_ids v0 v1 v2 facet body sheet marker c_face bound direct dims edges'.split()
    gap_keys = 'body sheet marker facet f0 f1 f2 faces_near'.split()
    problems = []
    if (len(above) != 4 or any(not set(above_keys) <= row.keys() for row in above)
            or len({row.get('key') for row in above}) != 4):
        problems.append({'row': 'LAB_ABOVE', 'expected_distinct_faces': 4, 'observed': above})
    if not gaps or any(not (set(gap_keys) <= row.keys() or
                           ({'body', 'sheet', 'marker', 'failed'} <= row.keys() and row['failed']))
                       for row in gaps):
        problems.append({'row': 'LAB_MODEL_GAP', 'observed': gaps})
    return problems


def disposition(result, log, retry):
    measurement = result['measurement']
    failures = log.count('FAILED:')
    expected_failure = failures == 1 and EXPECTED_ASSERTION.search(log) is not None
    peak = result['peak_lower_bound_bytes']
    stop = result['supervisor_stop']
    if (measurement['differences'] or measurement['failed_limits'] or peak > a.MEMORY * 1024
            or (failures and not expected_failure)):
        return 'result_failed'
    if stop == 'wrapper_time_limit':
        return 'open' if retry else 'retry_allowed'
    if (stop is None and result['wait']['returncode'] == 1 and expected_failure and peak > 0
            and measurement['expected'] == measurement['observed'] and not diagnostic_problems(measurement)):
        return 'matched_expected_failure'
    return 'result_failed'


def run(build, retry=False):
    for path, digest in ((ORDER, ORDER_SHA), (c2.ORDER, c2.ORDER_SHA), (c2.WATCHDOG_ORDER, c2.WATCHDOG_ORDER_SHA)):
        assert a.sha(path) == digest
        assert path.with_suffix('.sha256').read_text().split() == [digest, path.name]
    baseline = (BASE_BUILD / 'build_status.json', BASE_BUILD / 'source_pins.json',
                BASE_CASE / 'provenance.json', BASE_CASE / 'measurement.json')
    for path in baseline:
        c2.c3.c2.committed(path, BASE)
    assert cut.read(baseline[-1])['wait']['returncode'] == 1
    c2.verify_launches_terminal()

    label = cut.read(build / 'build_status.json')['build']
    root = HERE / 'diagnosis'
    first_case = root / 'cases' / f'build{label}' / 'N1'
    retry_pins = ()
    if retry:
        first = first_case / 'diagnosis.json'
        initial = cut.read(first)
        assert initial['status'] == 'retry_allowed'
        assert a.sha(first_case / 'measurement.json') == initial['measurement_sha256']
        retry_pins = (first, first_case / 'measurement.json')
        root /= 'watchdog_retry'
    case = root / 'cases' / f'build{label}' / 'N1'

    def prepare(directory):
        with patch.object(lab, 'HERE', c2.c1.B):
            receipt, binary, pins = c2.c3.prior.BASELINE_PREPARE(directory)
        verify_production(directory, receipt)
        support = (DRIVER, c2.DRIVER, ORDER, ORDER.with_suffix('.sha256'), SOURCE_CHECK,
                   c2.ORDER, c2.ORDER.with_suffix('.sha256'), c2.WATCHDOG_ORDER,
                   c2.WATCHDOG_ORDER.with_suffix('.sha256'), c2.c1.DRIVER, *baseline, *retry_pins)
        pins.update({str(path): a.sha(path) for path in support})
        return receipt, binary, pins

    seconds = WATCHDOG * (3 if retry else 1)
    with patch.multiple(lab, HERE=root, RUNS=RUNS, LINE=LINE, __file__=str(DRIVER), selector=c2.selector,
                        SECONDS=seconds, prepare=prepare, evaluate=evaluate), patch.object(
                            lab.b0, 'input_data', pinned_input):
        result = lab.run(build, 'N1')
    verify_production(build, result)
    with lab.archived.open_log(case / 'test_outer.log') as stream:
        log = a.ANSI.sub('', stream.read())
    status = disposition(result, log, retry)
    receipt = {'passed': status == 'matched_expected_failure', 'status': status,
               'build': label, 'baseline_build': 652, 'case': str(case),
               'production': PRODUCTION, 'production_pins_verified': True,
               'seconds_limit': seconds, 'host_preflight': cut.read(case / 'test_host_preflight.json'),
               'wait': result['wait'], 'supervisor_stop': result['supervisor_stop'],
               'measurement_sha256': a.sha(case / 'measurement.json'),
               'diagnostic_row_problems': diagnostic_problems(result['measurement']),
               **result['measurement']}
    a.sealed(case / 'diagnosis.json', receipt)
    return receipt


def self_check():
    assert pinned_input(1)[3]
    above = {field: '0' for field in 'key v_ids v0 v1 v2 facet body sheet marker c_face bound direct dims edges'.split()}
    measurement = {'differences': [], 'failed_limits': [], 'expected': EXPECTED, 'observed': EXPECTED,
                   'above': [{**above, 'key': str(i)} for i in range(4)],
                   'model_gap': [{'body': '0', 'sheet': '10154', 'marker': '2', 'failed': 'budget'}]}
    trial = {'measurement': measurement, 'supervisor_stop': None, 'wait': {'returncode': 1},
             'peak_lower_bound_bytes': 1024}
    log = 'CNativeLocalCoreLab_tests.cpp:353: FAILED:\n  REQUIRE( census.unexplained == 0 )'
    assert disposition(trial, log, False) == 'matched_expected_failure'
    assert disposition(trial, 'other: FAILED:', False) == 'result_failed'
    timed_out = {**trial, 'supervisor_stop': 'wrapper_time_limit'}
    assert disposition(timed_out, '', False) == 'retry_allowed'
    assert disposition(timed_out, '', True) == 'open'
    wrong = {**measurement, 'differences': [{'field': 'collapses_surface'}]}
    assert disposition({**timed_out, 'measurement': wrong}, '', False) == 'result_failed'
    print('N1 input hashes and diagnostic disposition checks passed.')


def main():
    parser = ArgumentParser(description=__doc__)
    parser.add_argument('build', type=Path, nargs='?')
    parser.add_argument('--owned', type=Path)
    parser.add_argument('--self-check', action='store_true')
    args = parser.parse_args()
    if args.self_check:
        self_check()
        return 0
    if args.owned:
        recipe = cut.read(args.owned / 'command.json')
        seconds = recipe['seconds_limit']
        assert seconds in (WATCHDOG, 3 * WATCHDOG)
        with patch.multiple(lab, RUNS=RUNS, selector=c2.selector, SECONDS=seconds):
            return lab.owned(args.owned)
    if args.build is None:
        parser.error('a build directory is required')
    build = args.build.resolve()
    result = run(build)
    if result['status'] == 'retry_allowed':
        result = run(build, retry=True)
    print(json.dumps(result, indent=2))
    return 0 if result['passed'] else 75 if result['status'] == 'open' else 1


if __name__ == '__main__':
    raise SystemExit(main())
