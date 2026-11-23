"""Prove the FP2-C1 OFF and CORE workflows with the inherited full and restart gates."""

from argparse import ArgumentParser
import json
from pathlib import Path
import runpy
import subprocess
import sys

sys.dont_write_bytecode = True
HERE = Path(__file__).resolve().parent
B = HERE.parent / 'FP2_B'
FX2 = HERE.parent / 'FX2'
sys.path[:0] = [str(B), str(FX2)]
import run_lab as lab
from compare_full import compare_cases, self_check as compare_self_check
from run_scoped_fx2 import scope, verify_order as verify_fx2_order

INHERITED = HERE.parent / 'FP2_A/run_g5.py'
BASE = runpy.run_path(str(INHERITED))
workflow, cut, sha = (BASE[key] for key in ('workflow', 'cut', 'sha'))
DEFAULT_CONFIG = BASE['config_for']
OFF = FX2 / 'G5/build645/OFF/full645_run1'
ORDER = HERE / 'inputs/FP2_C1_CONTINUE_1.md'
ORDER_SHA = '650c8f2f3234949c7fd45b3c3fb5c5f871a66e2e617089b109ed268e889b75a7'
MODES = ('OFF', 'CORE')


def verify_order():
    verify_fx2_order()
    BASE['verify_orders']()
    assert sha(ORDER) == ORDER_SHA
    assert ORDER.with_suffix('.sha256').read_text().split() == [ORDER_SHA, ORDER.name]


def config_for(mode):
    assert mode in MODES
    expected = DEFAULT_CONFIG(mode)
    if mode == 'CORE':
        assert sha(expected) == BASE['CORE_CONFIG_SHA']
        return expected
    config = OFF / 'run.cfg'
    assert sha(config) == workflow.OFF_CONFIG_SHA and config.read_bytes() == expected.read_bytes(), config
    assert b'ADAP_NATIVE_LOCAL_' not in config.read_bytes(), config
    return config


def bind_inherited():
    # Keep the inherited controller, receipt verification and standing gates.
    context = BASE['launch_full'].__globals__
    context.update(config_for=config_for, __file__=str(Path(__file__).resolve()), HERE=HERE)


def run_case(work, mode, build_dir, build, config, source_paths, expected, payloads):
    label = build['build']
    case = work / f'full{label}_run1'
    campaign = BASE['PRIOR']['bind_campaign']('on', source_paths)
    campaign['prepare'](work, label, build_dir, config)
    stage = 'full event gates'
    try:
        workflow.run_stage(work, 'full', [cut.PYTHON, '-B', str(Path(__file__).resolve()), str(build_dir),
                                          '--full', mode])
        BASE['status_ok'](case, build, config)
        hashes = workflow.seal_run(case, build['binary_sha256'], config, payloads, mode != 'OFF')
        events = workflow.event_collector()(case)
        stage = 'isolated restart'
        restart = workflow.isolate_restart(work, label, case)
        stage = 'payload identity'
        differences = payloads['compare_hashes'](hashes, expected)
        workflow.sealed_write(work / 'payload_identity.json', {'passed': not differences,
                              'differences': differences, 'expected': expected, 'actual': hashes})
        assert not differences, next(iter(differences.items()), None)
        if mode == 'OFF':
            stage = 'whole-event timer/storage/surface identity and replay'
            comparison = compare_cases(OFF, case, 'off')
            workflow.sealed_write(work / 'full_comparison.json', comparison)
            assert comparison['passed'], comparison['first_difference']
        stage = 'sealing provenance'
        workflow.verify_helpers(work)
        assert sha(Path(build['binary'])) == build['binary_sha256']
        assert sha(Path(build['test_binary'])) == build['test_binary_sha256']
        artifacts = [work / name for name in ('helper_pins.json', 'full_launch.json', 'full_wait.json',
                                              'restart_launch.json', 'restart_wait.json')]
        if mode == 'OFF':
            artifacts.append(work / 'full_comparison.json')
        artifacts.extend(case / name for name in ('provenance.json', 'status.json', 'full_workflow_receipt.json',
                                                   'n8_payload_identity.json'))
        artifacts.extend(case.glob('p7p8_event*_gate.json'))
        artifacts.extend(case.glob('f3b_event*_timer_gate.json'))
        artifacts.extend(case.glob('rank_output/*/rank.*/stdout'))
        artifacts.append(restart)
        artifacts.append(work / 'payload_identity.json')
        fingerprints = {key: build[key] for key in lab.FINGERPRINTS}
        record = {**fingerprints, 'passed': True, 'mode': mode, 'case': str(case), 'hashes': hashes,
                  'restart': str(restart), 'events': events, 'config_sha256': sha(config),
                  'artifacts': {str(path.relative_to(work)): sha(path) for path in artifacts}}
        workflow.sealed_write(work / 'run_receipt.json', record)
    except (AssertionError, KeyError, OSError, RuntimeError, ValueError, subprocess.CalledProcessError) as error:
        BASE['failure'](work, case, mode, stage, error)
        raise
    return record

def reference_off(payloads):
    expected = workflow.off_reference(payloads)
    path = OFF.parent / 'run_receipt.json'
    record = cut.read(path)
    build_path = FX2 / 'build645/build_status.json'
    build = cut.read(build_path)
    for key in lab.FINGERPRINTS:
        assert record[key] == build[key], (path, key)
    assert record['passed'] and record['mode'] == 'OFF' and record['case'] == str(OFF), path
    assert record['config_sha256'] == workflow.OFF_CONFIG_SHA
    for name, digest in record['artifacts'].items():
        assert sha(path.parent / name) == digest, name
    assert record['hashes'] == workflow.workflow_payloads(OFF, build['binary_sha256'], payloads) == expected
    compare_gate = OFF.parent / 'full_comparison.json'
    assert cut.read(compare_gate)['passed'], compare_gate
    paths = [path, build_path, workflow.N21]
    receipt = cut.read(workflow.N21)
    paths.extend((workflow.N21.parent / receipt[key]['path']).resolve()
                 for key in ('payloads', 'workflow_acceptance', 'timing_rule'))
    paths += [path.parent / name for name in record['artifacts']]
    return expected, paths


def verify_run(work, mode, build, payloads, expected):
    row = BASE['verify_run'](work, mode, build, payloads)
    assert row['test_binary_sha256'] == build['test_binary_sha256'], work
    assert row['hashes'] == expected
    assert cut.read(work / 'payload_identity.json')['passed']
    if mode == 'OFF':
        comparison = cut.read(work / 'full_comparison.json')
        assert comparison['passed'] and comparison['reference'] == str(OFF), comparison['first_difference']
    return row


def prerequisites(build):
    label = build['build']
    scoped = HERE / 'scoped' / f'build{label}' / 'scoped.json'
    lab_proof = HERE / 'cases' / f'build{label}' / 'lab_acceptance.json'
    fingerprints = {key: build[key] for key in lab.FINGERPRINTS}
    proof = scope.verified(scoped, fingerprints)
    assert proof['G1'] == [375, 114, 228], scoped
    lab_result = cut.read(lab_proof)
    assert lab_result['passed'] and all(lab_result[key] == value for key, value in fingerprints.items()), lab_proof
    assert lab_result['artifacts'], lab_proof
    for name, digest in lab_result['artifacts'].items():
        assert sha(Path(name)) == digest, name
    return [scoped, lab_proof, *(scoped.parent / name for name in proof['receipts']),
            *map(Path, lab_result['artifacts'])]


def run(build_dir, mode):
    bind_inherited()
    verify_order()
    assert build_dir == HERE / 'build647', build_dir
    build, _, _ = lab.prepare(build_dir)
    label = build['build']
    output = HERE / 'G5' / f'build{label}'
    work = output / mode
    config = config_for(mode)
    source_paths = [Path(__file__).resolve(), INHERITED, Path(lab.__file__).resolve(),
                    B / 'run_fp2_b_g5.py', FX2 / 'run_fp2_b_g5.py', FX2 / 'run_scoped_fx2.py',
                    FX2 / 'inputs/FIXED_PARTS_2.md', FX2 / 'inputs/FIXED_PARTS_2.sha256',
                    ORDER, ORDER.with_suffix('.sha256'), HERE / 'source_check.md', config,
                    build_dir / 'build_status.json', build_dir / 'source_pins.json']
    source_paths += [path for order in BASE['ORDERS'] for path in (order, order.with_suffix('.sha256'))]
    source_paths += prerequisites(build)
    payloads = runpy.run_path(str(workflow.PAYLOADS))
    off, off_sources = reference_off(payloads)
    if mode == 'OFF':
        check_comparator()
        expected = off
        source_paths += [FX2 / 'compare_full.py', FX2 / 'compare_full_self_check.json', *off_sources]
    else:
        verify_run(output / 'OFF', 'OFF', build, payloads, off)
        source_paths += [output / 'OFF/run_receipt.json', output / 'OFF/full_comparison.json',
                         BASE['CORE_REFERENCE'], BASE['CORE_ACCEPTANCE']]
        expected = BASE['core_reference'](payloads)

    if (work / 'run_receipt.json').is_file():
        verify_run(work, mode, build, payloads, expected)
    else:
        assert not (work / 'failed_attempt.json').exists(), work
        run_case(work, mode, build_dir, build, config, source_paths, expected, payloads)
    print(f'FP2-C1 {mode} PASSED.', flush=True)


def check_comparator():
    expected = FX2 / 'compare_full_self_check.json'
    assert cut.read(expected) == json.loads(json.dumps(compare_self_check())), expected


def self_check():
    verify_order()
    for mode in MODES:
        config_for(mode)
    payloads = runpy.run_path(str(workflow.PAYLOADS))
    off, _ = reference_off(payloads)
    assert len(off) == len(BASE['core_reference'](payloads)) == 6
    check_comparator()
    assert compare_cases(OFF, OFF, 'off')['passed']
    campaign = BASE['PRIOR']['bind_campaign']('on', [])
    compile(campaign['core_source'](), str(workflow.CORE), 'exec')
    print('FP2-C1 OFF and CORE configs, reference payloads, comparator and controller verified.')


def main():
    parser = ArgumentParser(description=__doc__)
    parser.add_argument('build', type=Path, nargs='?')
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument('--only', choices=MODES)
    group.add_argument('--full', choices=MODES, help='Internal supervised full controller.')
    group.add_argument('--self-check', action='store_true')
    args = parser.parse_args()
    if args.self_check:
        self_check()
        return
    if not args.build:
        parser.error('the sealed build647 directory is required')
    build = args.build.resolve()
    if args.full:
        bind_inherited()
        label = cut.read(build / 'build_status.json')['build']
        BASE['launch_full'](HERE / 'G5' / f'build{label}' / args.full, build, args.full)
    else:
        run(build, args.only)


if __name__ == '__main__':
    main()
