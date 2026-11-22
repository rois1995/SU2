"""Run FX2 OFF and S with the inherited FP2-B full and isolated-restart gates."""

from argparse import ArgumentParser
import json
from pathlib import Path
import runpy
import subprocess
import sys

HERE = Path(__file__).resolve().parent
sys.path[:0] = [str(HERE.parent / 'FP2_B'), str(HERE)]
import run_lab as lab
from compare_full import compare_cases, self_check
from run_scoped_fx2 import scope, verify_order

B = lab.HERE
FP2_A = HERE.parent / 'FP2_A'
INHERITED = FP2_A / 'run_g5.py'
BASE = runpy.run_path(str(INHERITED))
workflow, cut, sha = (BASE[key] for key in ('workflow', 'cut', 'sha'))
DEFAULT_CONFIG = BASE['config_for']
REFERENCE = B / 'G5/build640'
SURFACE_CONFIG_SHA = '07b91a94996bf3cb48352041d8e1643ffe17a61bed07432085cfcf87b1725382'
MODES = ('OFF', 'S')


def reference_case(mode):
    reference_mode = 'OFF' if mode == 'OFF' else 'S1'
    return REFERENCE / reference_mode / 'full640_run1'


def config_for(mode):
    config = reference_case(mode) / 'run.cfg'
    expected = DEFAULT_CONFIG('OFF') if mode == 'OFF' else FP2_A / 'run.cfg'
    assert config.read_bytes() == expected.read_bytes(), config
    if mode == 'S':
        assert sha(config) == SURFACE_CONFIG_SHA
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
        stage = 'whole-event timer/storage/surface identity and replay'
        comparison = compare_cases(reference_case(mode), case, mode.lower())
        workflow.sealed_write(work / 'full_comparison.json', comparison)
        assert comparison['passed'], comparison['first_difference']
        stage = 'sealing provenance'
        workflow.verify_helpers(work)
        assert sha(Path(build['binary'])) == build['binary_sha256']
        assert sha(Path(build['test_binary'])) == build['test_binary_sha256']
        artifacts = [work / name for name in ('helper_pins.json', 'full_launch.json', 'full_wait.json',
                                              'restart_launch.json', 'restart_wait.json', 'full_comparison.json')]
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



def run(build_dir, mode):
    bind_inherited()
    verify_order()
    BASE['verify_orders']()
    build, _, _ = lab.prepare(build_dir)
    label = build['build']
    scoped = HERE / 'scoped' / f'build{label}'
    fingerprints = {key: build[key] for key in lab.FINGERPRINTS}
    scope.verified(scoped / 'scoped.json', fingerprints)
    lab_proofs = [HERE / 'cases' / f'build{label}' / name / 'identity.json' for name in ('N1', 'N6P')]
    for path in lab_proofs:
        row = cut.read(path)
        assert row['passed'] and row['build'] == label
        assert sha(Path(row['candidate'])) == row['candidate_sha256']
    output = HERE / 'G5' / f'build{label}'
    work = output / mode
    source_paths = [Path(__file__).resolve(), INHERITED, Path(lab.__file__).resolve(),
                    B / 'run_fp2_b_g5.py', HERE / 'inputs/FIXED_PARTS_2.md',
                    HERE / 'inputs/FIXED_PARTS_2.sha256', HERE / 'source_check.md',
                    HERE / 'compare_full.py', HERE / 'compare_full_self_check.json',
                    build_dir / 'build_status.json', build_dir / 'source_pins.json',
                    scoped / 'scoped.json', scoped / 'safeguards.json', *lab_proofs]
    source_paths += [path for order in BASE['ORDERS'] for path in (order, order.with_suffix('.sha256'))]
    assert cut.read(HERE / 'compare_full_self_check.json') == json.loads(json.dumps(self_check()))
    reference_run = reference_case(mode)
    reference_receipt = reference_run.parent / 'run_receipt.json'
    pinned_reference = cut.read(reference_receipt)
    assert pinned_reference['passed'] and pinned_reference['build'] == 640
    for path in sorted(reference_run.glob('rank_output/*/rank.*/stdout')):
        assert sha(path) == pinned_reference['artifacts'][str(path.relative_to(reference_run.parent))]
        source_paths.append(path)
    source_paths.append(reference_receipt)
    payloads = runpy.run_path(str(workflow.PAYLOADS))
    if mode == 'OFF':
        expected = workflow.off_reference(payloads)
    else:
        off = BASE['verify_run'](output / 'OFF', 'OFF', build, payloads)
        assert off['hashes'] == workflow.off_reference(payloads)
        assert cut.read(output / 'OFF/full_comparison.json')['passed']
        source_paths += [output / 'OFF/run_receipt.json', output / 'OFF/full_comparison.json']
        on = B / 'on_surface_reference.json'
        reference = cut.read(on)
        assert reference['accepted'] and reference['status'] == 'PASSED'
        assert reference['config_sha256'] == SURFACE_CONFIG_SHA
        acceptance = REFERENCE / 'acceptance_summary.json'
        assert sha(acceptance) == reference['acceptance_sha256']
        expected = reference['hashes']
        for name in ('first_case', 'repeat_case'):
            assert payloads['payload_hashes'](Path(reference[name])) == expected, name
        source_paths += [on, acceptance]

    config = config_for(mode)
    source_paths.append(config)
    if (work / 'run_receipt.json').is_file():
        row = BASE['verify_run'](work, mode, build, payloads)
        assert row['hashes'] == expected
        assert cut.read(work / 'full_comparison.json')['passed']
    else:
        assert not (work / 'failed_attempt.json').exists(), work
        run_case(work, mode, build_dir, build, config, source_paths, expected, payloads)
    print(f'FX2 {mode} PASSED.', flush=True)


def main():
    parser = ArgumentParser(description=__doc__)
    parser.add_argument('build', type=Path)
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument('--only', choices=MODES)
    group.add_argument('--full', choices=MODES, help='Internal supervised full controller.')
    args = parser.parse_args()
    build = args.build.resolve()
    if args.full:
        bind_inherited()
        label = cut.read(build / 'build_status.json')['build']
        BASE['launch_full'](HERE / 'G5' / f'build{label}' / args.full, build, args.full)
    else:
        run(build, args.only)


if __name__ == '__main__':
    main()
