"""Extend the pinned FP2-C1 S launcher with the same workflow for S2."""

from argparse import ArgumentParser
from decimal import Decimal
import json
from pathlib import Path
import runpy
from tempfile import TemporaryDirectory
from unittest.mock import patch

import run_fp2_b_g5 as prior

HERE = prior.HERE
DRIVER = Path(__file__).resolve()
S_LAUNCHER = Path(prior.__file__).resolve()
BUILD = HERE / 'build647'
OUTPUT = HERE / 'G5/build647'
ORDER = HERE / 'inputs/FP2_C1_CONTINUE_1.md'
ORDER_SHA = '650c8f2f3234949c7fd45b3c3fb5c5f871a66e2e617089b109ed268e889b75a7'
CONTINUATION = HERE / 'inputs/FP2_C1_CONTINUE_2.md'
CONTINUATION_SHA = 'f65f2f302302f8f546ffcc23bb84a9e2ababa40defd9995e9779be456a94eda3'
COMPARE = HERE.parent / 'FX2/compare_full.py'
cut, sha, workflow = prior.cut, prior.sha, prior.workflow
config_s = prior.config_for
original_isolate, original_seal = workflow.isolate_restart, workflow.sealed_write


def config_for(mode):
    assert mode in ('S', 'S2')
    return config_s('S')


def s2_mode():
    # The original path is a source pin of both the first S run and RCFD1.
    return patch.multiple(prior, __file__=str(DRIVER), config_for=config_for)


def verify_order():
    prior.verify_order()
    for path, digest in ((ORDER, ORDER_SHA), (CONTINUATION, CONTINUATION_SHA)):
        assert sha(path) == digest, path
        assert path.with_suffix('.sha256').read_text().split() == [digest, path.name], path


def restart_watchdog(work):
    wait = work / 'restart_wait.json'
    return wait.is_file() and cut.read(wait)['returncode'] == 124


def isolate_with_watchdog_retry(work, build, case):
    try:
        return original_isolate(work, build, case)
    except RuntimeError:
        if not restart_watchdog(work):
            raise
        from retry_c1_restart import retry_restart
        return retry_restart(work, build, case)


def seal_with_retry_artifacts(path, value):
    work = OUTPUT / 'S2'
    manifest = work / 'restart_attempt1_watchdog/manifest.json'
    if path == work / 'run_receipt.json' and manifest.is_file():
        helper = work / 'run_restart647_1800.py'
        value['artifacts'].update({str(item.relative_to(work)): sha(item)
                                   for item in (helper, manifest)})
    original_seal(path, value)


def verify_default(build):
    receipt = OUTPUT / 'default_proven.json'
    proof = cut.read(receipt)
    assert proof['passed'] and all(proof[key] == build[key] for key in prior.lab.FINGERPRINTS), receipt
    required = [HERE / 'scoped/build647/scoped.json', OUTPUT / 'OFF/run_receipt.json',
                OUTPUT / 'CORE/run_receipt.json']
    assert set(proof['receipts']) == set(map(str, required)), receipt
    for path in required:
        assert sha(path) == proof['receipts'][str(path)], path
        record = cut.read(path)
        assert record['passed'] and all(record[key] == build[key] for key in prior.lab.FINGERPRINTS), path
    return [receipt, *required]


def timing(reference, run):
    rows = []
    for baseline, current in zip(reference['events'], run['events'], strict=True):
        assert baseline['event'] == current['event']
        ref = Decimal(str(baseline['native_seconds']))
        elapsed = Decimal(str(current['native_seconds']))
        limit = ref * Decimal('1.05')
        rows.append({'event': current['event'], 'reference_seconds': str(ref),
                     'native_seconds': str(elapsed), 'limit_seconds': str(limit), 'passed': elapsed <= limit})
    assert [row['event'] for row in rows] == [1, 2]
    return rows


def compare_payloads(first, second, payloads):
    for run in (first, second):
        assert payloads['payload_hashes'](Path(run['case'])) == run['hashes'], run['case']
    assert len(first['hashes']) == len(second['hashes']) == 6
    differences = payloads['compare_hashes'](second['hashes'], first['hashes'])
    work = OUTPUT / 'S2'
    identity = {'passed': not differences, 'differences': differences,
                'expected': first['hashes'], 'actual': second['hashes']}
    workflow.sealed_write(work / 'payload_identity.json', identity)
    if differences:
        error = ValueError(f'First S/S2 payload difference: {next(iter(differences.items()))}')
        prior.BASE['failure'](work, Path(second['case']), 'S2', 'payload identity', error)
        raise error


def publish(build, first, second):
    comparison = runpy.run_path(str(COMPARE))['compare_cases'](Path(first['case']), Path(second['case']), 's')
    compare_path = OUTPUT / 'S2/compare_s_s2.json'
    # Normalize the comparator's integer rank keys for an exact JSON recheck.
    workflow.sealed_write(compare_path, json.loads(json.dumps(comparison)))
    print(f"S/S2 report: {comparison['exact_difference_count']} exact differences, "
          f"first difference {comparison['first_difference']}", flush=True)

    baseline = cut.read(prior.S0_S)
    first_check, second_check = timing(baseline, first), timing(baseline, second)
    passed = all(row['passed'] for row in (*first_check, *second_check))
    status = 'PASSED' if passed else 'OPEN for the user'
    runs = {'OFF': cut.read(OUTPUT / 'OFF/run_receipt.json'),
            'CORE': cut.read(OUTPUT / 'CORE/run_receipt.json'),
            'S': {**first, 'time_rule': first_check},
            'S2': {**second, 'time_rule': second_check}}
    summary = OUTPUT / 'acceptance_summary.json'
    workflow.sealed_write(summary, {
        'standing_gates_passed': True, 'repeat_payloads_exact': True, 'status': status,
        'build': build['build'], 'binary_sha256': build['binary_sha256'],
        'source_pins_sha256': build['source_pins_sha256'], 'runs': runs,
        'comparison': {'path': str(compare_path), 'sha256': sha(compare_path),
                       'exact_difference_count': comparison['exact_difference_count'],
                       'first_difference': comparison['first_difference']},
    })
    reference = HERE / 'on_surface_reference.json'
    workflow.sealed_write(reference, {
        'accepted': True, 'hashes': first['hashes'], 'binary_sha256': build['binary_sha256'],
        'first_case': first['case'], 'repeat_case': second['case'],
        'config_sha256': first['config_sha256'], 'status': status, 'acceptance_sha256': sha(summary),
    })
    label = 'FP2-C1: accepted (S = S2)' if passed else 'FP2-C1: accepted, time rule OPEN for the user'
    print(label, flush=True)


def run():
    verify_order()
    build, _, _ = prior.lab.prepare(BUILD)
    assert build['build'] == 647
    payloads = runpy.run_path(str(workflow.PAYLOADS))
    first = prior.verify_run(OUTPUT / 'S', 'S', build, payloads)
    default_paths = verify_default(build)
    config = config_for('S2')
    lab_proof = HERE / 'cases/build647/lab_acceptance.json'
    sources = [DRIVER, S_LAUNCHER, prior.B / 'run_fp2_b_g5.py', prior.INHERITED,
               Path(prior.lab.__file__).resolve(), Path(prior.scope.__file__).resolve(),
               ORDER, ORDER.with_suffix('.sha256'), CONTINUATION, CONTINUATION.with_suffix('.sha256'),
               HERE / 'retry_c1_restart.py', prior.ORDER, prior.ORDER.with_suffix('.sha256'),
               HERE / 'source_check.md', BUILD / 'build_status.json', BUILD / 'source_pins.json',
               config, prior.FP2_A / 'run.cfg', config.with_suffix('.cfg.sha256'), COMPARE,
               OUTPUT / 'S/run_receipt.json', *default_paths]
    sources += [path for order in prior.BASE['ORDERS'] for path in (order, order.with_suffix('.sha256'))]
    sources += prior.reference_sources()
    sources += prior.prerequisite_sources(build, lab_proof)

    work = OUTPUT / 'S2'
    with s2_mode():
        if not (work / 'run_receipt.json').is_file():
            assert not (work / 'failed_attempt.json').exists(), work
            with patch.multiple(workflow, isolate_restart=isolate_with_watchdog_retry,
                                sealed_write=seal_with_retry_artifacts):
                prior.run_case(work, 'S2', BUILD, build, config, sources, payloads)
        second = prior.verify_run(work, 'S2', build, payloads)
    compare_payloads(first, second, payloads)
    publish(build, first, second)


def self_check():
    verify_order()
    assert config_for('S2') == config_for('S')
    sources = prior.reference_sources()
    build = cut.read(BUILD / 'build_status.json')
    payloads = runpy.run_path(str(workflow.PAYLOADS))
    first = prior.verify_run(OUTPUT / 'S', 'S', build, payloads)
    baseline = cut.read(sources[0])
    checks = timing(baseline, first)
    assert all(row['passed'] for row in checks)
    assert [round(Decimal(row['limit_seconds']), 3) for row in checks] == [
        Decimal('645.186'), Decimal('431.048')]
    slow = {'events': [{'event': 1, 'native_seconds': '645.186'}, baseline['events'][1]]}
    assert not timing(baseline, slow)[0]['passed']
    original = workflow.verify_helpers(OUTPUT / 'S')
    with TemporaryDirectory(prefix='fp2-c1-s2-binding-') as directory:
        work = Path(directory)
        assert not restart_watchdog(work)
        (work / 'restart_wait.json').write_text('{"returncode": 124}')
        assert restart_watchdog(work)
        campaign = prior.BASE['PRIOR']['bind_campaign']('on', [])
        campaign['prepare'](work, 647, BUILD, config_for('S2'))
        rebound = workflow.verify_helpers(work)
        names_and_hashes = lambda pins: {Path(path).name: digest for path, digest in pins['bound'].items()}
        assert names_and_hashes(original) == names_and_hashes(rebound)
    with s2_mode():
        prior.bind_inherited()
        inherited = prior.BASE['launch_full'].__globals__
        assert inherited['__file__'] == str(DRIVER) and inherited['config_for']('S2') == config_s('S')
    print('FP2-C1 S pins, S2 controller and validators, order and timing limits verified.')


def main():
    parser = ArgumentParser(description=__doc__)
    parser.add_argument('build', type=Path, nargs='?')
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument('--only', choices=('S2',))
    group.add_argument('--full', choices=('S2',), help='Internal supervised full controller.')
    group.add_argument('--self-check', action='store_true')
    opts = parser.parse_args()
    if opts.self_check:
        self_check()
    elif opts.build is None or opts.build.resolve() != BUILD:
        parser.error('the pinned FP2_C1/build647 directory is required')
    elif opts.full:
        with s2_mode():
            prior.bind_inherited()
            prior.BASE['launch_full'](OUTPUT / 'S2', BUILD, 'S2')
    else:
        run()


if __name__ == '__main__':
    main()
