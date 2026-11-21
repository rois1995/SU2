"""Run FP2-A C fixtures, G1, OFF prefixes, G3, G4 and saved-state safeguards."""

from argparse import ArgumentParser
import json
import os
from pathlib import Path
import runpy
import subprocess
import sys

sys.dont_write_bytecode = True
import run_a as a
import run_b as b
import run_prefix_fp1 as prefix
import run_resume_scoped as legacy

FX1 = a.HERE.parent / 'FX1/compare_prefix.py'
SCHEDULING = a.FP1 / 'resume6/scheduling_calls_reference.json'
CORRECTION = a.HERE / 'inputs/FP2_A_CORRECTION_1.md'
PART_B = a.HERE / 'part_b/build624/part_b_receipt.json'
PINS = {
    str(CORRECTION): 'c072cc7d48473dbac5f0c83b2fa83ce96444448364c72aab626a1d033b6d4249',
    str(SCHEDULING): 'd4b1a99a5b9e98043542df7bc0b3fb7765c0df27a3300b43c32d96e44dddd1b6',
    str(PART_B): '5ece1d16a0aaabab5d6136358decb98e90bf887987acb53f9bf2bc32dfd5d79b',
}
COHORTS = {'serial': 375, 'mpi2': 114, 'mpi4': 228}


def part_a_fixtures(output, binary, provenance, original, measurements, in1, lab1):
    a.fixture(output / 'box', binary, 1, a.NAME_BOX, {}, provenance)
    environment = {key: row['path'] for key, row in in1.items()}
    a.fixture(output / 'IN1', binary, 1, a.NAME_IN1, environment, provenance,
              measure=lambda log: a.summarize_in1(log, measurements))
    lab_case = output / 'lab/LAB1_V2'
    lab_case.mkdir(parents=True, exist_ok=True)
    config, sensor, bound = original['prepare'](lab_case, lab1, provenance['pins'])
    env = {'SU2_NATIVE_FROZEN_CONFIG': str(config), 'SU2_NATIVE_FROZEN_SENSOR3D': str(sensor),
           'SU2_NATIVE_LOCAL_LAB_VARIANT': 'V2'}
    lab = a.fixture(lab_case, binary, 1, a.NAME_LAB, env, {**provenance, 'pins': bound},
                    measure=lambda log: a.summarize_lab(log, lab1[-1]), lab=a.setup_lab(output))
    original['verify'](bound)
    alarm = a.correction_counts(lab['measurement'])
    a.sealed(output / 'correction_1_alarm.json', alarm)
    assert not alarm['stopped'], alarm


def part_b_fixtures(output, binary, provenance):
    for ranks in b.RANKS:
        case = output / f'exchange/rank{ranks}'
        result = a.fixture(case, binary, ranks, b.EXCHANGE, {}, provenance)
        logs = {}
        for name in result['artifacts']:
            path = case / name
            if path.name == 'stdout':
                rank = int(path.parent.name.rsplit('.', 1)[1])
                assert rank not in logs, path
                logs[rank] = path.read_text(errors='replace')
        state = b.compare_build(logs, ranks, b.INPUT_CELLS)
        a.sealed(case / 'state_comparison.json', state)
        assert state['passed'], state.get('first_difference')


def inherited_fixtures(output, binary, provenance, parity):
    cases = []
    for part, cohorts in a.FIXTURES.items():
        for ranks, selector in cohorts:
            case = output / f'FP1/{part}/rank{ranks}'
            env = {'SU2_FP1_G_OWNED_BASELINE': parity} if part == 'H' else {}
            a.fixture(case, binary, ranks, selector, env, provenance)
            cases.append(case)
    assert len(cases) == 18
    return cases


def run_g1(output, status_path, manifest, receipt):
    subprocess.run([a.cut.PYTHON, '-B', str(a.FP1 / 'run_g1.py'), str(output), str(status_path), str(manifest)],
                   cwd=a.cut.REPO, check=True)
    reports = []
    for cohort, expected in COHORTS.items():
        case = output / cohort
        report = case / 'comparison.json'
        row = a.cut.read(report)
        assert row['identity_pass'] and row['records'] == expected, row.get('first_difference')
        pins = a.cut.read(case / 'pins.json')
        assert pins['test_binary'] == receipt['test_binary_sha256']
        assert pins['test_source_pins'] == receipt['source_pins_sha256']
        assert a.resumed(case)['returncode'] == row['candidate_returncode']
        reports.append(report)
    return reports


def exact_prefixes(build, output, receipt, compare):
    results = {}
    prefix.compare_cases = compare['compare_cases']
    for gate in ('G2', 'GQ'):
        report = output / f'{gate}_comparison.json'
        if not report.is_file():
            assert prefix.run(build, gate, output, corrected=True), report
        result = a.cut.read(report)
        assert result['passed'] and result['fixedstorage'] == 4120, result.get('first_difference')
        assert result['scheduling_reference'] == str(SCHEDULING)
        assert compare['schedule_difference'](result['scheduling_calls'], gate) is None
        case = output / 'cases' / gate
        status = a.cut.read(case / 'status.json')
        assert status['gate_passed'] and status['inputs_unchanged'] and status['source_and_binary_pins_unchanged']
        assert status['all_owned_handles_terminal'] and all(a.cut.terminal(row) for row in status['owned_identities'])
        source = a.cut.read(case / 'provenance.json')
        assert source['source_pins_sha256'] == receipt['source_pins_sha256']
        assert source['binary_sha256'] == receipt['binary_sha256']
        results[gate] = str(report)
    return results


def prepare(build):
    status_path, manifest = build / 'build_status.json', build / 'source_pins.json'
    receipt = a.cut.read(status_path)
    binary, source_hash = a.verify_build(receipt, manifest)
    assert not receipt['new_warnings']
    solver = Path(receipt['binary'])
    assert a.sha(solver) == receipt['binary_sha256']
    assert a.cut.read(PART_B)['passed']
    rank_check = a.HERE / 'rank_check' / f"build{receipt['build']}" / 'rank_check_summary.json'
    rank_status = a.cut.read(rank_check)
    assert rank_status['default_gates_11_2_required'] and set(rank_status['results']) == {'K1', 'K2', 'K3'}
    for key in ('build', 'binary_sha256', 'test_binary_sha256', 'source_pins_sha256'):
        assert rank_status[key] == receipt[key]
    original, measurements, pins, in1, lab1, parity = a.evidence_inputs()
    support = (Path(__file__).resolve(), Path(b.__file__).resolve(), FX1, SCHEDULING.parent / 'compare_scheduling.py',
               Path(prefix.__file__).resolve(), Path(legacy.__file__).resolve(), prefix.CAPTURE,
               a.FP1.parents[1] / 'shared_engine_regression_gates.json', rank_check)
    pins.update({**PINS, **{str(path): a.sha(path) for path in support},
                 str(status_path): a.sha(status_path), str(manifest): source_hash,
                 str(binary): receipt['test_binary_sha256'], str(solver): receipt['binary_sha256']})
    original['verify'](pins)
    return status_path, manifest, receipt, binary, pins, original, measurements, in1, lab1, parity


def run(build, output, verify_only):
    status_path, manifest, receipt, binary, pins, original, measurements, in1, lab1, parity = prepare(build)
    label = receipt['build']
    output = output or a.HERE / 'scoped' / f'build{label}'
    if verify_only:
        return {'verified': True, 'launched': False, 'build': label, 'output': str(output)}

    assert Path('/proc/1/comm').read_text().strip() == 'systemd'
    assert receipt['wait']['terminal'] and a.cut.terminal(receipt['wait']['controller'])
    for key in tuple(os.environ):
        if key.startswith(('SU2_NATIVE_', 'SU2_FP2_', 'SU2_FP1_')):
            del os.environ[key]
    output.mkdir(parents=True, exist_ok=True)
    fingerprints = {key: receipt[key] for key in ('build', 'binary_sha256', 'test_binary_sha256', 'source_pins_sha256')}
    provenance = {**fingerprints, 'output': str(output), 'pins': pins}
    a.sealed(output / 'provenance.json', provenance)

    part_a_fixtures(output, binary, provenance, original, measurements, in1, lab1)
    part_b_fixtures(output, binary, provenance)
    inherited = inherited_fixtures(output, binary, provenance, parity)
    reports = run_g1(output / 'G1', status_path, manifest, receipt)
    cases = [output / name for name in ('box', 'IN1', 'lab/LAB1_V2', 'exchange/rank2', 'exchange/rank4')]
    fixture_receipts = [case / 'fixture_status.json' for case in [*cases, *inherited]]
    fixture_receipts += [output / name for name in ('correction_1_alarm.json', 'exchange/rank2/state_comparison.json',
                                                    'exchange/rank4/state_comparison.json')]
    fixture_receipts.extend(reports)
    fixture_status = {str(path.relative_to(output)): a.sha(path) for path in fixture_receipts}
    a.sealed(output / 'fixtures.json', {**fingerprints, 'passed': True, 'fp1_cohorts': len(inherited),
                                       'G1': list(COHORTS.values()), 'receipts': fixture_status})

    compare = runpy.run_path(str(FX1))
    legacy.legacy_build_receipts(build, receipt)
    comparisons = exact_prefixes(build, output / 'prefix', receipt, compare)
    g3 = legacy.g3(output / 'g3', build, label)
    g4 = legacy.g4(output / 'g4', build, label)
    safeguards = legacy.saved_safeguards(output / 'safeguards', build, label)
    original['verify'](pins)
    a.sealed(output / 'safeguards.json', {**fingerprints, 'passed': True, 'G3': g3, 'G4': g4,
                                         'safeguards': safeguards})
    receipt_paths = [output / name for name in ('provenance.json', 'fixtures.json', 'safeguards.json')]
    receipt_paths.extend(Path(path) for path in comparisons.values())
    path = output / 'scoped.json'
    a.sealed(path, {**fingerprints, 'passed': True, 'G1': list(COHORTS.values()), **comparisons,
                    'G3': g3, 'G4': g4, 'receipts': {str(item.relative_to(output)): a.sha(item)
                                                   for item in receipt_paths}})
    return {'passed': True, 'build': label, 'receipt': str(path)}


def self_check():
    a.self_check()
    b.self_check()
    compare = runpy.run_path(str(FX1))
    compare['self_check']()
    reference = a.cut.read(SCHEDULING)
    assert all(compare['schedule_difference']([row for row in reference if row['case'] == gate], gate) is None
               for gate in ('G2', 'GQ'))
    assert all(a.sha(Path(path)) == digest for path, digest in PINS.items())


if __name__ == '__main__':
    parser = ArgumentParser(description=__doc__)
    parser.add_argument('build', nargs='?', type=Path)
    parser.add_argument('--output', type=Path)
    group = parser.add_mutually_exclusive_group()
    group.add_argument('--self-check', action='store_true')
    group.add_argument('--verify-only', action='store_true')
    args = parser.parse_args()
    if args.self_check:
        self_check()
        print('Part C scoped fixture parsers, prefix comparator and reference pins passed.')
    elif args.build:
        print(json.dumps(run(args.build.resolve(), args.output.resolve() if args.output else None, args.verify_only),
                         indent=2))
    else:
        parser.error('supply the pinned Part C build directory or --self-check')
