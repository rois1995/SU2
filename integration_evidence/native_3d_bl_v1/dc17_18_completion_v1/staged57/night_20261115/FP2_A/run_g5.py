"""Qualify one ordered FP2-A G5 run on the active, corrected Part C build."""

from argparse import ArgumentParser
from decimal import Decimal
from hashlib import sha256
from pathlib import Path
import runpy
import subprocess
import sys

sys.dont_write_bytecode = True
HERE = Path(__file__).resolve().parent
FP1 = HERE.parent / 'FP1'
PRIOR = runpy.run_path(str(FP1 / 'resume5/run_g5.py'))
workflow, cut, sha = (PRIOR[name] for name in ('workflow', 'cut', 'sha'))
ORDERS = {
    HERE / 'inputs/FAST_PATH_STAGE2_A.md': 'c499613b6960740e5666800f1cf75a37f2d1611c99d9a23846f8eab8774f76b3',
    HERE / 'inputs/FP2_A_CORRECTION_1.md': 'c072cc7d48473dbac5f0c83b2fa83ce96444448364c72aab626a1d033b6d4249',
    HERE / 'inputs/FP2_A_CORRECTION_2.md': 'bf671f5646c4414a817692e8c30ceed88a42171b414db0bdd371e2b2c20fd94a',
}
CORE_CONFIG_SHA = '1ec6c26d49d4ec7c62103d5bd096471b5422290cd06c44b2f1073b7489f2b4c1'
CORE_REFERENCE = FP1 / 'on_reference.json'
CORE_ACCEPTANCE = FP1 / 'resume6/G5/acceptance_summary.json'
SURFACE_CONFIG = HERE / 'run.cfg'
SURFACE_LINE = b'ADAP_NATIVE_LOCAL_SURFACE= YES\n'
MODES = ('OFF', 'CORE', 'S1', 'S2')


def verify_orders():
    for order, digest in ORDERS.items():
        assert sha(order) == digest, order
        assert order.with_suffix('.sha256').read_text().split() == [digest, order.name], order


def config_for(mode, publish=False):
    pinned = workflow.ON_CONFIG.read_bytes()
    assert sha256(pinned).hexdigest() == CORE_CONFIG_SHA and pinned == workflow.config_bytes()
    assert pinned.endswith(workflow.ON_LINE) and SURFACE_LINE.split(b'=')[0] not in pinned
    if mode == 'OFF':
        assert workflow.OFF_CONFIG.read_bytes() == pinned.removesuffix(workflow.ON_LINE)
        return workflow.OFF_CONFIG
    if mode == 'CORE':
        return workflow.ON_CONFIG
    config = pinned + SURFACE_LINE
    digest = sha256(config).hexdigest()
    checksum = SURFACE_CONFIG.with_suffix('.cfg.sha256')
    if publish:
        for path, content in ((SURFACE_CONFIG, config), (checksum, f'{digest}  {SURFACE_CONFIG.name}\n'.encode())):
            if not path.exists():
                with path.open('xb') as stream:
                    stream.write(content)
    assert SURFACE_CONFIG.read_bytes() == config
    assert checksum.read_text().split() == [digest, SURFACE_CONFIG.name]
    return SURFACE_CONFIG


def core_reference(payloads):
    reference = cut.read(CORE_REFERENCE)
    assert reference['accepted'] and reference['status'] == 'PASSED'
    assert reference['config_sha256'] == CORE_CONFIG_SHA and sha(CORE_ACCEPTANCE) == reference['acceptance_sha256']
    accepted = cut.read(CORE_ACCEPTANCE)
    assert accepted['standing_gates_passed'] and accepted['repeat_payloads_exact']
    expected = reference['hashes']
    for mode, name in (('ON1', 'first_case'), ('ON2', 'repeat_case')):
        case = Path(reference[name])
        assert accepted['runs'][mode]['case'] == str(case)
        assert accepted['runs'][mode]['hashes'] == expected
        assert payloads['payload_hashes'](case) == expected
    return expected


def preflight(build_dir, scoped):
    assert Path('/proc/1/comm').read_text().strip() == 'systemd'
    build_path, pins_path = (build_dir / name for name in ('build_status.json', 'source_pins.json'))
    build = cut.read(build_path)
    _, source_sha = workflow.verify_build(build, pins_path)
    assert build['passed'] and not build['new_warnings'] and not build.get('changed')
    wait = build['wait']
    assert wait['terminal'] and wait['returncode'] == 0 and cut.terminal(wait['controller'])
    solver = build['binary_sha256']
    assert sha(Path(build['binary'])) == solver
    scope_paths = [scoped / name for name in ('scoped.json', 'safeguards.json')]
    for path in scope_paths:
        gate = cut.read(path)
        assert gate['passed'] and gate['build'] == build['build'], path
        assert gate['binary_sha256'] == solver and gate['source_pins_sha256'] == source_sha, path
    return build, [build_path, pins_path, *scope_paths]


def launch_full(work, build_dir, mode):
    config = config_for(mode)
    manifest = workflow.verify_helpers(work)
    build = cut.read(build_dir / 'build_status.json')['build']
    assert manifest['build'] == build and manifest['config'] == str(config)
    assert manifest['config_sha256'] == sha(config)
    namespace = workflow.runner(work, build_dir, mode != 'OFF', config, [*manifest['source'], *manifest['bound']])
    assert namespace['PURPOSE'] == work and namespace['R'] == workflow.R
    namespace['SCRIPT'] = Path(__file__).resolve()
    sys.path[:0] = [str(work), str(workflow.CORE.parent)]
    for name in ('check_complete_events', 'check_connected_events'):
        module = __import__(name)
        assert Path(module.__file__).resolve() == work / f'{name}.py'
    sys.argv = [str(Path(__file__).resolve()), str(work / f'full{build}_run1'), '--full']
    namespace['main']()


def status_ok(case, build, config):
    status = cut.read(case / 'status.json')
    assert status['exit_code'] == 0 and status['full_workflow_accepted'], status.get('full_workflow_errors')
    assert status['inputs_unchanged'] and status['source_and_binary_pins_unchanged'], status
    provenance = cut.read(case / 'provenance.json')
    assert provenance['source_pins_sha256'] == build['source_pins_sha256']
    assert provenance['config_sha256'] == sha(case / 'run.cfg') == sha(config)


def verify_run(work, mode, build, payloads):
    path = work / 'run_receipt.json'
    assert not (work / 'failed_attempt.json').exists(), work
    record = cut.read(path)
    case = work / f'full{build["build"]}_run1'
    config = config_for(mode)
    assert record['passed'] and record['mode'] == mode and record['case'] == str(case)
    for field in ('build', 'binary_sha256', 'source_pins_sha256'):
        assert record[field] == build[field], (path, field)
    for name, digest in record['artifacts'].items():
        assert sha(work / name) == digest, name
    workflow.verify_helpers(work)
    status_ok(case, build, config)
    assert record['config_sha256'] == sha(config)
    assert workflow.workflow_payloads(case, build['binary_sha256'], payloads) == record['hashes']
    for name in ('full', 'restart'):
        wait = cut.read(work / f'{name}_wait.json')
        assert wait['controller'] == cut.read(work / f'{name}_launch.json')['controller']
        assert wait['returncode'] == 0 and wait['terminal'] and cut.terminal(wait['controller']), wait
    restart = Path(record['restart'])
    isolated = cut.read(restart)
    assert isolated['disk_restart_verified'] and isolated['all_inputs_and_pins_unchanged']
    assert isolated['source_event_receipt_sha256'] == sha(case / 'full_workflow_receipt.json')
    assert all(sha(restart.parent / name) == digest for name, digest in isolated['artifact_sha256'].items())
    return record


def failure(work, case, mode, stage, error):
    status_path = case / 'status.json'
    status = cut.read(status_path) if status_path.exists() else {}
    reasons = status.get('full_workflow_errors') or []
    detail = str(error) or type(error).__name__
    first = reasons[0] if stage == 'full event gates' and reasons else detail
    files = list(case.glob('p7p8_event*_gate.json')) + list(case.glob('f3b_event*_timer_gate.json'))
    files += list(case.glob('rank_output/*/rank.*/stdout'))
    files += [status_path, case / 'provenance.json', case / 'full_workflow_receipt.json', work / 'payload_identity.json']
    files += [work / f'{name}_{step}.json' for name in ('full', 'restart') for step in ('launch', 'wait')]
    files += [work / 'helper_pins.json']
    workflow.sealed_write(work / 'failed_attempt.json', {
        'passed': False, 'mode': mode, 'stage': stage, 'case': str(case), 'error': detail, 'status': status,
        'first_observed_failure': first,
        'artifacts': {str(path.relative_to(work)): sha(path) for path in files if path.is_file()},
    })


def event_one_time(core, surface):
    old, new = (Decimal(str(run['events'][0]['native_seconds'])) for run in (core, surface))
    limit = old * Decimal('1.05')
    return dict(event=1, core_seconds=str(old), native_seconds=str(new), limit_seconds=str(limit), passed=new <= limit)


def publish_reference(output, build, runs):
    outcome = 'PASSED' if all(runs[mode]['time_rule']['passed'] for mode in ('S1', 'S2')) else 'OPEN for the user'
    summary = output / 'acceptance_summary.json'
    workflow.sealed_write(summary, {
        'standing_gates_passed': True, 'repeat_payloads_exact': True, 'status': outcome,
        'build': build['build'], 'binary_sha256': build['binary_sha256'],
        'source_pins_sha256': build['source_pins_sha256'], 'runs': runs,
    })
    workflow.sealed_write(HERE / 'on_surface_reference.json', {
        'accepted': True, 'hashes': runs['S1']['hashes'], 'binary_sha256': build['binary_sha256'],
        'first_case': runs['S1']['case'], 'repeat_case': runs['S2']['case'],
        'config_sha256': runs['S1']['config_sha256'], 'status': outcome, 'acceptance_sha256': sha(summary),
    })
    return outcome


def run(build_dir, scoped, output, mode):
    build, sources = preflight(build_dir, scoped)
    label, solver = build['build'], build['binary_sha256']
    rank_check = HERE / 'rank_check' / f'build{label}' / 'rank_check_summary.json'
    rank_status = cut.read(rank_check)
    assert rank_status['default_gates_11_2_required']
    for key in ('build', 'binary_sha256', 'test_binary_sha256', 'source_pins_sha256'):
        assert rank_status[key] == build[key], key
    if mode in ('S1', 'S2') and not rank_status['proceed_11_3']:
        raise RuntimeError('Correction 2 stopped FP2-A before 11.3.')
    sources.append(rank_check)
    work = output / mode
    case = work / f'full{label}_run1'
    payloads = runpy.run_path(str(workflow.PAYLOADS))
    earlier = MODES[:MODES.index(mode)]
    runs = {name: verify_run(output / name, name, build, payloads) for name in earlier}
    expected = {'OFF': workflow.off_reference(payloads), 'CORE': core_reference(payloads)}
    for name in ('OFF', 'CORE'):
        if name in runs:
            assert runs[name]['hashes'] == expected[name], name
    if mode == 'S2':
        expected['S2'] = runs['S1']['hashes']
    config = config_for(mode, publish=mode == 'S1')
    if (work / 'run_receipt.json').exists():
        runs[mode] = verify_run(work, mode, build, payloads)
        if mode in expected:
            assert runs[mode]['hashes'] == expected[mode], mode
        if mode in ('S1', 'S2'):
            assert runs[mode]['time_rule'] == event_one_time(runs['CORE'], runs[mode])
        if mode == 'S2':
            publish_reference(output, build, runs)
        print(f'FP2-A {mode} sealed G5 receipt verified.', flush=True)
        return
    assert not (work / 'failed_attempt.json').exists(), work
    sources += [Path(__file__).resolve(), HERE / 'source_check.md', CORE_REFERENCE, CORE_ACCEPTANCE,
                *(output / name / 'run_receipt.json' for name in earlier)]
    sources += [path for order in ORDERS for path in (order, order.with_suffix('.sha256'))]
    if mode in ('S1', 'S2'):
        sources.append(SURFACE_CONFIG.with_suffix('.cfg.sha256'))
    env = PRIOR['bind_campaign']('on', sources)
    env['prepare'](work, label, build_dir, config)
    stage = 'full event gates'
    try:
        workflow.run_stage(work, 'full', [cut.PYTHON, '-B', str(Path(__file__).resolve()), str(build_dir),
                                          '--output', str(output), '--full', mode])
        status_ok(case, build, config)
        hashes = workflow.seal_run(case, solver, config, payloads, mode != 'OFF')
        events = workflow.event_collector()(case)
        stage = 'isolated restart'
        restart = workflow.isolate_restart(work, label, case)
        if mode in expected:
            stage = 'payload identity'
            differences = payloads['compare_hashes'](hashes, expected[mode])
            workflow.sealed_write(work / 'payload_identity.json', {
                'passed': not differences, 'differences': differences, 'expected': expected[mode], 'actual': hashes,
            })
            assert not differences, next(iter(differences.items()), None)
        stage = 'sealing provenance'
        workflow.verify_helpers(work)
        assert sha(Path(build['binary'])) == solver
        artifacts = [work / name for name in ('helper_pins.json', 'full_launch.json', 'full_wait.json',
                                              'restart_launch.json', 'restart_wait.json')]
        artifacts += [case / name for name in ('provenance.json', 'status.json', 'full_workflow_receipt.json',
                                               'n8_payload_identity.json')]
        artifacts += list(case.glob('p7p8_event*_gate.json')) + list(case.glob('f3b_event*_timer_gate.json'))
        artifacts += list(case.glob('rank_output/*/rank.*/stdout'))
        artifacts += [restart]
        if mode in expected:
            artifacts.append(work / 'payload_identity.json')
        record = dict(passed=True, mode=mode, build=label, binary_sha256=solver,
                      source_pins_sha256=build['source_pins_sha256'], case=str(case), hashes=hashes,
                      restart=str(restart), events=events, config_sha256=sha(config),
                      artifacts={str(path.relative_to(work)): sha(path) for path in artifacts})
        if mode in ('S1', 'S2'):
            record['time_rule'] = event_one_time(runs['CORE'], record)
        workflow.sealed_write(work / 'run_receipt.json', record)
    except (AssertionError, KeyError, OSError, RuntimeError, ValueError, subprocess.CalledProcessError) as error:
        failure(work, case, mode, stage, error)
        raise
    runs[mode] = record
    outcome = publish_reference(output, build, runs) if mode == 'S2' else 'PASSED'
    print(f'FP2-A {mode} G5 {outcome}.', flush=True)


def main():
    parser = ArgumentParser(description=__doc__)
    parser.add_argument('build', type=Path)
    parser.add_argument('--scoped', type=Path)
    parser.add_argument('--output', type=Path)
    modes = parser.add_mutually_exclusive_group(required=True)
    modes.add_argument('--only', choices=tuple(mode.lower() for mode in MODES))
    modes.add_argument('--full', choices=MODES, help='Internal full controller stage.')
    args = parser.parse_args()
    verify_orders()
    build_dir = args.build.resolve()
    label = cut.read(build_dir / 'build_status.json')['build']
    output = (args.output or HERE / f'G5/build{label}').resolve()
    if args.full:
        launch_full(output / args.full, build_dir, args.full)
    else:
        scoped = (args.scoped or HERE / f'scoped/build{label}').resolve()
        run(build_dir, scoped, output, args.only.upper())


if __name__ == '__main__':
    main()
