"""Validate FP2-B's small fixtures, G1, and post-S1 default prefixes and safeguards."""

from argparse import ArgumentParser
from pathlib import Path
import json
import runpy
import sys

sys.dont_write_bytecode = True
import run_lab as lab
import run_c_scoped as prior

a = lab.a
HERE = lab.HERE
VOLUME = '[NativeLocalCoreVolumeB]'
LOCKED = '[.][NativeLocalCoreSurfaceB]'


def verified(path, fingerprints):
    gate = a.cut.read(path)
    assert gate['passed'] and all(gate[key] == value for key, value in fingerprints.items()), path
    for name, digest in gate['receipts'].items():
        assert a.sha(path.parent / name) == digest, name
    return gate


def fixture_path(case):
    return case / 'fixture_status.json'


def volume_fixture(output, binary, provenance):
    case = output / 'volume'
    a.fixture(case, binary, 1, VOLUME, {}, provenance)
    return fixture_path(case)


def verify_fixture(path):
    item = a.cut.read(path)
    assert item['passed']
    assert item['provenance_sha256'] == a.sha(path.parent / 'provenance.json'), path
    lab.archived.verify_artifacts(path.parent, item['artifacts'])
    assert item['wait'] == a.cut.read(path.parent / 'test_wait.json')
    assert item['wait']['terminal'] and a.cut.terminal(item['wait']['controller'])


def verify_fixtures(output, fingerprints):
    path = output / 'fixtures.json'
    gate = verified(path, fingerprints)
    assert gate['fp1_cohorts'] == 18 and gate['G1'] == list(prior.COHORTS.values())
    for name in gate['receipts']:
        receipt = output / name
        if receipt.name == 'fixture_status.json':
            verify_fixture(receipt)
    for cohort, count in prior.COHORTS.items():
        case = output / 'G1' / cohort
        item = a.cut.read(case / 'comparison.json')
        pins = a.cut.read(case / 'pins.json')
        assert item['identity_pass'] and item['records'] == count
        assert pins['test_binary'] == fingerprints['test_binary_sha256']
        assert pins['test_source_pins'] == fingerprints['source_pins_sha256']
        wait = a.resumed(case)
        assert wait['returncode'] == item['candidate_returncode']
        lab.archived.verify_artifacts(case, {str(Path(row['stdout']).relative_to(case)): row['sha256']
                                               for row in item['candidate_sources']})
    for name in ('N1', 'N6'):
        case = HERE / 'cases' / f'build{fingerprints["build"]}' / name
        handover = lab.checked_receipt(case, fingerprints)
        assert handover['passed'] and handover['case'] == name, case
    return gate


def fixtures(build, output, receipt, binary, provenance, original, measurements, in1, parity):
    specs = {
        'box': (a.NAME_BOX, {}, None),
        'IN1': (a.NAME_IN1, {key: row['path'] for key, row in in1.items()},
                lambda log: a.summarize_in1(log, measurements)),
        'locked': (LOCKED, {}, None),
    }
    paths = [volume_fixture(output, binary, provenance)]
    for name, (selector, environment, measure) in specs.items():
        case = output / name
        a.fixture(case, binary, 1, selector, environment, provenance, measure=measure)
        paths.append(fixture_path(case))
    prior.part_b_fixtures(output, binary, provenance)
    cases = [output / f'exchange/rank{ranks}' for ranks in prior.b.RANKS]
    paths.extend(case / 'state_comparison.json' for case in cases)
    inherited = prior.inherited_fixtures(output, binary, provenance, parity)
    paths.extend(fixture_path(case) for case in [*cases, *inherited])
    paths += prior.run_g1(output / 'G1', build / 'build_status.json', build / 'source_pins.json', receipt)
    paths += [path.parent / name for path in paths if path.name == 'comparison.json'
              for name in ('pins.json', 'test_wait.json')]
    original['verify'](provenance['pins'])
    fingerprints = {key: receipt[key] for key in lab.FINGERPRINTS}
    a.sealed(output / 'fixtures.json', {**fingerprints, 'passed': True, 'fp1_cohorts': len(inherited),
                                         'G1': list(prior.COHORTS.values()),
                                         'receipts': {str(path.relative_to(output)): a.sha(path) for path in paths}})
    return verify_fixtures(output, fingerprints)


def default(build, output, receipt, original, pins):
    import run_fp2_b_g5 as full

    fingerprints = {key: receipt[key] for key in lab.FINGERPRINTS}
    run_root = HERE / 'G5' / f'build{receipt["build"]}'
    payloads = runpy.run_path(str(full.workflow.PAYLOADS))
    full.verify_run(run_root / 'S1', 'S1', receipt, payloads)
    verify_fixtures(output, fingerprints)
    if (output / 'scoped.json').is_file():
        return verified(output / 'scoped.json', fingerprints)
    compare = runpy.run_path(str(prior.FX1))
    prior.legacy.legacy_build_receipts(build, receipt)
    label = receipt['build']
    stage = 'G2/GQ'
    try:
        comparisons = prior.exact_prefixes(build, output / 'prefix', receipt, compare)
        stage = 'G3'
        g3 = prior.legacy.g3(output / 'g3', build, label)
        stage = 'G4'
        g4 = prior.legacy.g4(output / 'g4', build, label)
        stage = 'shared-engine safeguards'
        safeguards = prior.legacy.saved_safeguards(output / 'safeguards', build, label)
        original['verify'](pins)
    except (AssertionError, KeyError, OSError, RuntimeError, ValueError) as error:
        a.sealed(output / 'default_failure.json', {**fingerprints, 'passed': False, 'stage': stage,
                                                    'error': str(error) or type(error).__name__})
        raise
    a.sealed(output / 'safeguards.json', {**fingerprints, 'passed': True, 'G3': g3, 'G4': g4,
                                         'safeguards': safeguards})
    paths = [output / name for name in ('provenance.json', 'fixtures.json', 'safeguards.json')]
    paths.extend(Path(path) for path in comparisons.values())
    a.sealed(output / 'scoped.json', {**fingerprints, 'passed': True, 'G1': list(prior.COHORTS.values()),
                                    **comparisons, 'G3': g3, 'G4': g4,
                                    'receipts': {str(path.relative_to(output)): a.sha(path) for path in paths}})
    return verified(output / 'scoped.json', fingerprints)


def run(build, mode):
    receipt, binary, pins = lab.prepare(build)
    fingerprints = {key: receipt[key] for key in lab.FINGERPRINTS}
    label = receipt['build']
    output = HERE / 'scoped' / f'build{label}'
    output.mkdir(parents=True, exist_ok=True)
    if mode == 'volume':
        pins[str(Path(__file__).resolve())] = a.sha(Path(__file__).resolve())
        path = volume_fixture(output / 'A1', binary, {**fingerprints, 'output': str(output), 'pins': pins})
        return a.cut.read(path)
    original, measurements, inherited_pins, in1, _, parity = a.evidence_inputs()
    support = (Path(__file__).resolve(), Path(lab.__file__).resolve(), Path(prior.__file__).resolve(),
               Path(prior.b.__file__).resolve(), Path(prior.legacy.__file__).resolve(),
               Path(prior.prefix.__file__).resolve(), prior.FX1, prior.SCHEDULING.parent / 'compare_scheduling.py',
               prior.prefix.CAPTURE, a.FP1.parents[1] / 'shared_engine_regression_gates.json')
    pins.update({**inherited_pins, **prior.PINS, **{str(path): a.sha(path) for path in support}})
    original['verify'](pins)
    provenance = {**fingerprints, 'output': str(output), 'pins': pins}
    a.sealed(output / 'provenance.json', provenance)
    if mode == 'locked':
        return a.fixture(output / 'locked', binary, 1, LOCKED, {}, provenance)
    for name in ('N1', 'N6'):
        path = HERE / 'cases' / f'build{label}' / name
        assert lab.checked_receipt(path, fingerprints)['passed'], path
    if mode == 'fixtures':
        if (output / 'fixtures.json').is_file():
            return verify_fixtures(output, fingerprints)
        return fixtures(build, output, receipt, binary, provenance, original, measurements, in1, parity)
    assert not (output / 'default_failure.json').exists(), output
    return default(build, output, receipt, original, pins)


def main():
    parser = ArgumentParser(description=__doc__)
    parser.add_argument('build', type=Path)
    parser.add_argument('--only', required=True, choices=('volume', 'locked', 'fixtures', 'default'))
    args = parser.parse_args()
    print(json.dumps(run(args.build.resolve(), args.only), indent=2))


if __name__ == '__main__':
    main()
