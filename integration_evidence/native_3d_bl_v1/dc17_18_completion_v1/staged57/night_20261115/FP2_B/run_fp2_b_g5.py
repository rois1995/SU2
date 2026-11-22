"""Run FP2-B G5 in the required order S1, default scope, OFF, CORE, S2."""

from argparse import ArgumentParser
from pathlib import Path
import runpy
import subprocess
import sys

sys.dont_write_bytecode = True
import run_lab as lab
import run_scoped as scope

HERE = lab.HERE
FP2_A = HERE.parent / 'FP2_A'
INHERITED = FP2_A / 'run_g5.py'
BASE = runpy.run_path(str(INHERITED))
workflow, cut, sha = (BASE[key] for key in ('workflow', 'cut', 'sha'))
DEFAULT_CONFIG = BASE['config_for']
SURFACE_CONFIG = HERE / 'run.cfg'
SURFACE_CONFIG_SHA = '07b91a94996bf3cb48352041d8e1643ffe17a61bed07432085cfcf87b1725382'
MODES = ('S1', 'OFF', 'CORE', 'S2')


def config_for(mode, publish=False):
    if mode in ('OFF', 'CORE'):
        return DEFAULT_CONFIG(mode)
    original = DEFAULT_CONFIG('S1')
    assert sha(original) == SURFACE_CONFIG_SHA
    if publish:
        for source, target in ((original, SURFACE_CONFIG),
                               (original.with_suffix('.cfg.sha256'), SURFACE_CONFIG.with_suffix('.cfg.sha256'))):
            if not target.exists():
                with target.open('xb') as copy:
                    copy.write(source.read_bytes())
    assert sha(SURFACE_CONFIG) == SURFACE_CONFIG_SHA
    checksum = SURFACE_CONFIG.with_suffix('.cfg.sha256')
    assert checksum.read_text().split() == [SURFACE_CONFIG_SHA, SURFACE_CONFIG.name], checksum
    return SURFACE_CONFIG


def bind_inherited():
    # Keep the inherited controller, receipt verification and standing gates.
    context = BASE['launch_full'].__globals__
    context.update(config_for=config_for, __file__=str(Path(__file__).resolve()), HERE=HERE)


def verify_run(work, mode, build, payloads):
    bind_inherited()
    row = BASE['verify_run'](work, mode, build, payloads)
    assert row['test_binary_sha256'] == build['test_binary_sha256'], work
    return row


def expected_hashes(mode, runs, payloads):
    if mode == 'OFF':
        return workflow.off_reference(payloads)
    if mode == 'CORE':
        return BASE['core_reference'](payloads)
    if mode == 'S2':
        return runs['S1']['hashes']
    return None


def verify_sequence(output, mode, build, payloads):
    runs = {}
    for earlier in MODES[:MODES.index(mode)]:
        row = verify_run(output / earlier, earlier, build, payloads)
        expected = expected_hashes(earlier, runs, payloads)
        assert expected is None or row['hashes'] == expected, earlier
        runs[earlier] = row
    return runs


def finish_mode(output, mode, build, record, runs, default_proof):
    if mode in ('CORE', 'S2'):
        fingerprints = {key: build[key] for key in lab.FINGERPRINTS}
        receipts = (default_proof, output / 'OFF/run_receipt.json', output / 'CORE/run_receipt.json')
        default = {**fingerprints, 'passed': True, 'receipts': {str(path): sha(path) for path in receipts}}
        workflow.sealed_write(output / 'default_proven.json', default)
    if mode == 'S2':
        s1 = {**runs['S1'], 'time_rule': BASE['event_one_time'](runs['CORE'], runs['S1'])}
        assert record['time_rule'] == BASE['event_one_time'](runs['CORE'], record)
        return BASE['publish_reference'](output, build, {**runs, 'S1': s1, 'S2': record})
    return 'PASSED'


def run_case(work, mode, build_dir, build, config, source_paths, expected, runs, payloads):
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
        if expected is not None:
            stage = 'payload identity'
            differences = payloads['compare_hashes'](hashes, expected)
            workflow.sealed_write(work / 'payload_identity.json', {'passed': not differences,
                                  'differences': differences, 'expected': expected, 'actual': hashes})
            assert not differences, next(iter(differences.items()), None)
        stage = 'sealing provenance'
        workflow.verify_helpers(work)
        assert sha(Path(build['binary'])) == build['binary_sha256']
        assert sha(Path(build['test_binary'])) == build['test_binary_sha256']
        artifacts = [work / name for name in ('helper_pins.json', 'full_launch.json', 'full_wait.json',
                                              'restart_launch.json', 'restart_wait.json')]
        artifacts.extend(case / name for name in ('provenance.json', 'status.json', 'full_workflow_receipt.json',
                                                   'n8_payload_identity.json'))
        artifacts.extend(case.glob('p7p8_event*_gate.json'))
        artifacts.extend(case.glob('f3b_event*_timer_gate.json'))
        artifacts.extend(case.glob('rank_output/*/rank.*/stdout'))
        artifacts.append(restart)
        if expected is not None:
            artifacts.append(work / 'payload_identity.json')
        fingerprints = {key: build[key] for key in lab.FINGERPRINTS}
        record = {**fingerprints, 'passed': True, 'mode': mode, 'case': str(case), 'hashes': hashes,
                  'restart': str(restart), 'events': events, 'config_sha256': sha(config),
                  'artifacts': {str(path.relative_to(work)): sha(path) for path in artifacts}}
        if mode == 'S2':
            record['time_rule'] = BASE['event_one_time'](runs['CORE'], record)
        workflow.sealed_write(work / 'run_receipt.json', record)
    except (AssertionError, KeyError, OSError, RuntimeError, ValueError, subprocess.CalledProcessError) as error:
        BASE['failure'](work, case, mode, stage, error)
        raise
    return record


def run(build_dir, mode):
    bind_inherited()
    BASE['verify_orders']()
    build, _, _ = lab.prepare(build_dir)
    label = build['build']
    output = HERE / 'G5' / f'build{label}'
    work = output / mode
    source_paths = [Path(__file__).resolve(), INHERITED, Path(lab.__file__).resolve(),
                    Path(scope.__file__).resolve(), lab.ORDER, lab.ORDER.with_suffix('.sha256'), HERE / 'source_check.md',
                    build_dir / 'build_status.json', build_dir / 'source_pins.json']
    source_paths += [path for order in BASE['ORDERS'] for path in (order, order.with_suffix('.sha256'))]
    payloads = runpy.run_path(str(workflow.PAYLOADS))
    runs = verify_sequence(output, mode, build, payloads)
    source_paths += [output / earlier / 'run_receipt.json' for earlier in runs]
    scoped = HERE / 'scoped' / f'build{label}'
    fingerprints = {key: build[key] for key in lab.FINGERPRINTS}
    scope.verify_fixtures(scoped, fingerprints)
    source_paths.append(scoped / 'fixtures.json')
    source_paths += [HERE / 'cases' / f'build{label}' / name / 'measurement.json' for name in ('N1', 'N6')]
    default_proof = scoped / 'scoped.json'
    if mode != 'S1':
        scope.verified(default_proof, fingerprints)
        source_paths.append(default_proof)
    if mode == 'S2':
        default = cut.read(output / 'default_proven.json')
        assert default['passed'] and all(default[key] == value for key, value in fingerprints.items())
        assert all(sha(Path(path)) == digest for path, digest in default['receipts'].items())
        source_paths.append(output / 'default_proven.json')

    config = config_for(mode, publish=mode == 'S1')
    if mode in ('S1', 'S2'):
        source_paths += [FP2_A / 'run.cfg', SURFACE_CONFIG.with_suffix('.cfg.sha256')]
    if mode in ('CORE', 'S2'):
        source_paths += [BASE['CORE_REFERENCE'], BASE['CORE_ACCEPTANCE']]
    expected = expected_hashes(mode, runs, payloads)
    if (work / 'run_receipt.json').is_file():
        record = verify_run(work, mode, build, payloads)
        assert expected is None or record['hashes'] == expected, mode
    else:
        assert not (work / 'failed_attempt.json').exists(), work
        record = run_case(work, mode, build_dir, build, config, source_paths, expected, runs, payloads)
    outcome = finish_mode(output, mode, build, record, runs, default_proof)
    print(f'FP2-B {mode} {outcome}.', flush=True)


def self_check():
    BASE['verify_orders']()
    assert sha(DEFAULT_CONFIG('S1')) == SURFACE_CONFIG_SHA
    assert b'ADAP_NATIVE_LOCAL_SURFACE=' not in config_for('CORE').read_bytes()
    assert b'ADAP_NATIVE_LOCAL_CORE=' not in config_for('OFF').read_bytes()
    core = {'events': [{'native_seconds': 100}]}
    s1 = {'events': [{'native_seconds': 105}]}
    assert BASE['event_one_time'](core, s1)['passed']
    s1['events'][0]['native_seconds'] = 105.001
    assert not BASE['event_one_time'](core, s1)['passed']


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
        parser.error('a build directory is required')
    build = args.build.resolve()
    if args.full:
        bind_inherited()
        label = cut.read(build / 'build_status.json')['build']
        BASE['launch_full'](HERE / 'G5' / f'build{label}' / args.full, build, args.full)
    else:
        run(build, args.only)


if __name__ == '__main__':
    main()
