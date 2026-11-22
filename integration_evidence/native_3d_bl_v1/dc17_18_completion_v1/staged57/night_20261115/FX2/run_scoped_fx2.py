"""Run the FX2 scoped fixture, regression, or hand-over gates."""

from argparse import ArgumentParser
from copy import deepcopy
import json
from pathlib import Path
import runpy
import sys
from unittest.mock import patch

sys.dont_write_bytecode = True
DRIVER = Path(__file__).resolve()
HERE = DRIVER.parent
B = HERE.parent / 'FP2_B'
sys.path.insert(0, str(B))
import run_correction3_lab as c3
import run_correction3_sheet as sheet

lab, a = c3.lab, c3.a
scope = c3.prior.scope
prior = scope.prior
ORDER = HERE / 'inputs/FIXED_PARTS_2.md'
ORDER_SHA = '78503c39659c9afa41bd9baeb7f1695bffe7d1e383612c8cb3dd6b77d26a08cd'
SOURCE_CHECK = HERE / 'source_check.md'
BASE = 'c843610249'
BASE_SCOPE = B / 'scoped/build640'
BASE_CASES = B / 'cases/build640'
SHEET_PINS = B / 'inputs/FP2_B_CORRECTION_3_sheet_pins.json'
PROOF_ROW = {'N1': 'LAB_S_PROOF', 'N6P': 'proof'}


def verify_order():
    assert a.sha(ORDER) == ORDER_SHA
    assert ORDER.with_suffix('.sha256').read_text().split() == [ORDER_SHA, ORDER.name]


def base_receipt(path):
    c3.c2.committed(path, BASE)
    result = a.cut.read(path)
    assert result['passed'], path
    return result


def prepare(build, extra=()):
    verify_order()
    receipt, binary, pins = c3.prior.BASELINE_PREPARE(build)
    paths = (DRIVER, ORDER, ORDER.with_suffix('.sha256'), SOURCE_CHECK, *extra)
    pins.update({str(path): a.sha(path) for path in paths})
    return receipt, binary, pins


def scoped_inputs(build):
    support = (Path(scope.__file__).resolve(), Path(prior.__file__).resolve(),
               Path(prior.b.__file__).resolve(), Path(prior.legacy.__file__).resolve(),
               Path(prior.prefix.__file__).resolve(), prior.FX1, prior.SCHEDULING.parent / 'compare_scheduling.py',
               prior.prefix.CAPTURE, a.FP1.parents[1] / 'shared_engine_regression_gates.json',
               BASE_SCOPE / 'fixtures.json')
    receipt, binary, pins = prepare(build, support)
    original, measurements, inherited_pins, in1, _, parity = a.evidence_inputs()
    pins.update({**inherited_pins, **prior.PINS})
    original['verify'](pins)
    output = HERE / 'scoped' / f'build{receipt["build"]}'
    return receipt, binary, pins, original, measurements, in1, parity, output


def fixtures(build):
    receipt, binary, pins, original, measurements, in1, parity, output = scoped_inputs(build)
    source = base_receipt(BASE_SCOPE / 'fixtures.json')
    fingerprints = c3.prior.build_fingerprints(receipt)
    provenance = {**fingerprints, 'output': str(output), 'pins': pins}
    output.mkdir(parents=True, exist_ok=True)
    a.sealed(output / 'provenance.json', provenance)
    # The accepted sheet fixture has a separate receipt in build640.
    with patch.object(sheet, 'HERE', HERE), patch.object(lab, 'prepare', prepare):
        sheet.run(build, SHEET_PINS)
    if not (output / 'fixtures.json').is_file():
        with patch.object(scope, 'verify_fixtures', c3.prior.scoped_fixtures):
            scope.fixtures(build, output, receipt, binary, provenance, original, measurements, in1, parity)
    result = c3.prior.scoped_fixtures(output, fingerprints)
    assert result['receipts'].keys() == source['receipts'].keys()
    scope.verify_fixture(output / 'sheet/fixture_status.json')
    return {'passed': True, 'build': receipt['build'], 'G1': result['G1'],
            'fixtures': str(output / 'fixtures.json'), 'sheet': str(output / 'sheet/fixture_status.json')}


def regression(build):
    receipt, _, pins, original, _, _, _, output = scoped_inputs(build)
    fingerprints = c3.prior.build_fingerprints(receipt)
    fixture = c3.prior.scoped_fixtures(output, fingerprints)
    if (output / 'scoped.json').is_file():
        return scope.verified(output / 'scoped.json', fingerprints)

    reference_files = [BASE_SCOPE / name for name in ('scoped.json', 'safeguards.json',
                                                      'prefix/G2_comparison.json', 'prefix/GQ_comparison.json')]
    references = {path.stem: base_receipt(path) for path in reference_files}
    compare = runpy.run_path(str(prior.FX1))
    prior.legacy.legacy_build_receipts(build, receipt)
    label = receipt['build']
    comparisons = prior.exact_prefixes(build, output / 'prefix', receipt, compare)
    for gate, path in comparisons.items():
        old = references[f'{gate}_comparison']
        new = a.cut.read(Path(path))
        for field in ('reference', 'scheduling_reference', 'scheduling_calls'):
            assert old[field] == new[field], (gate, field, old[field], new[field])

    g3 = prior.legacy.g3(output / 'g3', build, label)
    g4 = prior.legacy.g4(output / 'g4', build, label)
    safeguards = prior.legacy.saved_safeguards(output / 'safeguards', build, label)
    standing = references['safeguards']
    assert g3['reference'] == standing['G3']['reference']
    assert g4['required_filters'] == standing['G4']['required_filters']
    assert safeguards.keys() == standing['safeguards'].keys()
    original['verify'](pins)
    a.sealed(output / 'safeguards.json', {**fingerprints, 'passed': True, 'G3': g3, 'G4': g4,
                                         'safeguards': safeguards})
    paths = [output / name for name in ('provenance.json', 'fixtures.json', 'safeguards.json')]
    paths.extend(Path(path) for path in comparisons.values())
    a.sealed(output / 'scoped.json', {**fingerprints, 'passed': True, 'G1': fixture['G1'],
                                     **comparisons, 'G3': g3, 'G4': g4,
                                     'build640_references': {str(path): a.sha(path) for path in reference_files},
                                     'receipts': {str(path.relative_to(output)): a.sha(path) for path in paths}})
    return scope.verified(output / 'scoped.json', fingerprints)


def lab_reference(name):
    path = BASE_CASES / name / 'measurement.json'
    result = base_receipt(path)
    assert result['case'] == name and result['build'] == 640
    return path, result


def compare_lab(name, reference, candidate):
    expected, actual = (row['rows_by_rank'] for row in (reference, candidate))
    differences, seconds_build = [], []
    if expected.keys() != actual.keys():
        differences.append({'row': 'ranks', 'expected': sorted(expected), 'observed': sorted(actual)})
    for rank in sorted(expected.keys() & actual.keys(), key=int):
        for marker in ('NATIVE3D_LOCAL_SURFACE', 'state', PROOF_ROW[name]):
            old, new = (rows[rank].get(marker, []) for rows in (expected, actual))
            if len(old) != len(new):
                differences.append({'rank': int(rank), 'row': marker, 'field': 'row_count',
                                    'expected': len(old), 'observed': len(new)})
                continue
            for index, (target, observed) in enumerate(zip(old, new, strict=True), 1):
                for key in sorted(target.keys() | observed.keys()):
                    if not key.startswith('seconds_') and target.get(key) != observed.get(key):
                        differences.append({'rank': int(rank), 'row': marker, 'index': index, 'field': key,
                                            'expected': target.get(key), 'observed': observed.get(key)})
                if marker == 'state':
                    seconds_build.append({'rank': int(rank), 'row': index,
                                          'build640': target['seconds_build'],
                                          'candidate': observed.get('seconds_build')})
    return {'identical': not differences, 'first_difference': differences[0] if differences else None,
            'differences': differences, 'seconds_build': seconds_build}


def handover(build, name):
    label = a.cut.read(build / 'build_status.json')['build']
    reference_path, reference = lab_reference(name)
    result_path = HERE / 'cases' / f'build{label}' / name / 'measurement.json'
    scoped_path = HERE / 'scoped' / f'build{label}/scoped.json'
    verify_order()
    original = c3.prior.prepare

    def prepare_case(directory, extra=()):
        receipt, binary, pins = original(directory, (*extra, DRIVER, ORDER, ORDER.with_suffix('.sha256'),
                                                     SOURCE_CHECK, reference_path, scoped_path))
        scope.verified(scoped_path, c3.prior.build_fingerprints(receipt))
        return receipt, binary, pins

    with patch.object(lab, 'HERE', HERE), patch.object(c3.prior, 'prepare', prepare_case):
        candidate = c3.run(build, name, verify_only=result_path.is_file())
    comparison = compare_lab(name, reference, candidate)
    data = {'passed': candidate['passed'] and comparison['identical'], 'case': name, 'build': label,
            'reference': str(reference_path), 'reference_sha256': a.sha(reference_path),
            'candidate': str(result_path), 'candidate_sha256': a.sha(result_path),
            'lab_passed': candidate['passed'], **comparison}
    a.sealed(result_path.with_name('identity.json'), data)
    return data


def self_check():
    verify_order()
    reference = base_receipt(BASE_SCOPE / 'scoped.json')
    assert reference['G1'] == list(prior.COHORTS.values())
    for name, digest in reference['receipts'].items():
        assert a.sha(BASE_SCOPE / name) == digest, name
    for gate, path in (('G2', prior.prefix.G2_REFERENCE), ('GQ', prior.prefix.GQ_REFERENCE)):
        report = base_receipt(BASE_SCOPE / f'prefix/{gate}_comparison.json')
        compare = runpy.run_path(str(prior.FX1))
        assert report['reference'] == str(path)
        assert compare['schedule_difference'](report['scheduling_calls'], gate) is None
    assert a.sha(SHEET_PINS) == sheet.SHEET_PIN_SHA
    for name in PROOF_ROW:
        path, old = lab_reference(name)
        lab.archived.verify_artifacts(path.parent, old['artifacts'])
        new = deepcopy(old)
        observed = new['rows_by_rank']['0']
        observed['state'][0]['seconds_build'] = '0'
        assert compare_lab(name, old, new)['identical']
        observed['NATIVE3D_LOCAL_SURFACE'][0]['faces_in'] = '1'
        assert compare_lab(name, old, new)['first_difference']['field'] == 'faces_in'
    print('Build640 receipt and lab comparison self-check passed.')


if __name__ == '__main__':
    parser = ArgumentParser(description=__doc__)
    parser.add_argument('build', type=Path, nargs='?')
    parser.add_argument('mode', choices=('fixtures', 'regression', *PROOF_ROW), nargs='?')
    parser.add_argument('--self-check', action='store_true')
    args = parser.parse_args()
    if args.self_check:
        self_check()
    else:
        if args.build is None or args.mode is None:
            parser.error('provide the candidate build directory and gate mode')
        build = args.build.resolve()
        run = {'fixtures': fixtures, 'regression': regression}
        result = run[args.mode](build) if args.mode in run else handover(build, args.mode)
        print(json.dumps(result, indent=2))
        raise SystemExit(0 if result['passed'] else 1)
