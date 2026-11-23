"""Resume a sealed build647 workflow after its isolated-restart watchdog."""
from argparse import ArgumentParser
from pathlib import Path
import runpy
import subprocess

import run_default_g5 as runner

HERE = runner.HERE
cut, sha, workflow = runner.cut, runner.sha, runner.workflow
ORDER = HERE / 'inputs/FP2_C1_CONTINUE_2.md'
ORDER_SHA = 'f65f2f302302f8f546ffcc23bb84a9e2ababa40defd9995e9779be456a94eda3'
ARCHIVE = 'restart_attempt1_watchdog'
REFERENCE = HERE.parent / 'FP2_B/G5/build640/CORE/restart_wait.json'


def helper_source(work, build):
    original = f'run_restart{build}.py'
    target = work / f'run_restart{build}_1800.py'
    source = (work / original).read_text().replace(original, target.name)
    anchor = '            compile(source, str(BASE), "exec")\n'
    source = workflow.replace_once(source, anchor,
        '            source = source.replace(\'"timeout", "600"\', \'"timeout", "1800"\', 1)\n'
        + anchor)
    source = workflow.replace_once(source,
        '            source = source.replace(\'"timeout", "600"\', \'"timeout", "1800"\', 1)\n',
        '            old_timeout = \'"timeout", "600"\'\n'
        '            assert source.count(old_timeout) == 1, old_timeout\n'
        '            source = source.replace(old_timeout, \'"timeout", "1800"\', 1)\n')
    compile(source, str(target), 'exec')
    return target, source


def terminal_wait(work, stage, returncode):
    wait = cut.read(work / f'{stage}_wait.json')
    assert wait['controller'] == cut.read(work / f'{stage}_launch.json')['controller']
    assert wait['actual_host_proc'] and wait['terminal'] and cut.terminal(wait['controller'])
    assert wait['returncode'] == returncode, wait
    return wait


def archive_watchdog(work, restart_name):
    archive = work / ARCHIVE
    manifest = archive / 'manifest.json'
    if not manifest.exists():
        terminal_wait(work, 'restart', 124)
        restart = work / restart_name
        old_status = cut.read(restart / 'status.json')
        assert old_status['state'] == 'terminal' and old_status['exit_code'] == 124, old_status
        assert old_status['pins_verified'] and old_status['resource_gate']['passed'], old_status
        post = cut.read(restart / 'post_terminal_handles.json')
        assert post['actual_host_proc'] and post['all_owned_handles_terminal'], post
        handles = [post['controller_identity'], post['controller_wrapper_identity']]
        handles.extend(row['identity'] for row in post['owned_identity_checks'])
        assert all(cut.terminal(handle) for handle in handles)
        argv = cut.read(restart / 'command.json')['argv']
        assert argv[argv.index('timeout') + 1] == '600', argv
        required = ['restart_host_preflight.json', 'restart_launch.json', 'restart_wait.json',
                    restart_name, f'{restart_name}_controller_launch.json']
        optional = ['failed_attempt.json', 'observed_payload_identity.json']
        paths = [*(work / name for name in required),
                 *(work / name for name in optional if (work / name).is_file()),
                 *sorted(work.glob('restart_outer.log*')), *sorted(work.glob('restart_post_only.log*'))]
        assert all(path.exists() for path in paths) and (work / 'restart_outer.log').is_file()
        files = sorted(file for path in paths for file in (path.rglob('*') if path.is_dir() else [path])
                       if file.is_file())
        assert not any(path.is_symlink() for path in (*paths, *files))
        archive.mkdir(exist_ok=True)
        cut.save(manifest, {'moved_paths': [path.name for path in paths], 'files': {
            str(file.relative_to(work)): {'old_path': str(file), 'sha256': sha(file)} for file in files}})
    record = cut.read(manifest)
    for name in record['moved_paths']:
        if not (archive / name).exists():
            (work / name).rename(archive / name)
    for name, row in record['files'].items():
        assert row['old_path'] == str(work / name) and sha(archive / name) == row['sha256'], name
    return manifest


def retry_restart(work, build, case):
    """Run the one allowed retry, and return the inherited validation receipt."""
    target, source = helper_source(work, build)
    if not target.exists():
        with target.open('x') as stream:
            stream.write(source)
    assert target.read_text() == source
    for helper in (target, work / f'validate_restart{build}.py'):
        subprocess.run([cut.PYTHON, '-B', str(helper), '--self-check'], cwd=cut.REPO, check=True)
    archive_watchdog(work, f'disk_restart_{case.name}_event2')
    timing = work / 'restart_retry_timing.json'
    load = work / 'restart_retry_start_load.json'
    if not load.exists():
        cut.save(load, {'start_load_1m': float(Path('/proc/loadavg').read_text().split()[0])})
    try:
        workflow.run_stage(work, 'restart', [cut.PYTHON, '-B', str(target), '--source-case', str(case)])
    finally:
        wait_path = work / 'restart_wait.json'
        if wait_path.is_file():
            wait = cut.read(wait_path)
            comparison = {**cut.read(load), 'watchdog_seconds': 1800, 'returncode': wait['returncode'],
                          'elapsed_seconds': wait['elapsed_seconds'], 'build640_elapsed_seconds':
                          cut.read(REFERENCE)['elapsed_seconds']}
            workflow.sealed_write(timing, comparison)
            print(comparison, flush=True)
    verify = runpy.run_path(str(workflow.F_WORKFLOW))['verify_restart']
    verify.__globals__['WORKSPACE'] = work
    return verify(build, case)


def core():
    assert sha(ORDER) == ORDER_SHA
    assert ORDER.with_suffix('.sha256').read_text().split() == [ORDER_SHA, ORDER.name]
    runner.verify_order()
    runner.bind_inherited()
    build, _, _ = runner.lab.prepare(HERE / 'build647')
    label = build['build']
    assert label == 647
    output = HERE / f'G5/build{label}'
    work = output / 'CORE'
    case = work / f'full{label}_run1'
    config = runner.config_for('CORE')
    payloads = runpy.run_path(str(workflow.PAYLOADS))
    expected = runner.BASE['core_reference'](payloads)
    if (work / 'run_receipt.json').is_file():
        runner.verify_run(work, 'CORE', build, payloads, expected)
        return
    runner.prerequisites(build)
    off, _ = runner.reference_off(payloads)
    runner.verify_run(output / 'OFF', 'OFF', build, payloads, off)
    terminal_wait(work, 'full', 0)
    workflow.verify_helpers(work)
    runner.BASE['status_ok'](case, build, config)
    original_failure = work / 'failed_attempt.json'
    if not original_failure.exists():
        original_failure = work / ARCHIVE / original_failure.name
    assert cut.read(original_failure)['stage'] == 'isolated restart'
    genuine = workflow.seal_run(case, build['binary_sha256'], config, payloads, True)
    differences = payloads['compare_hashes'](genuine, expected)
    assert not differences, differences
    restart = retry_restart(work, label, case)
    proof = work / 'payload_identity.json'
    workflow.sealed_write(proof, {'passed': True, 'differences': differences,
                                 'expected': expected, 'actual': genuine})
    artifacts = [work / name for name in ('helper_pins.json', 'full_launch.json', 'full_wait.json',
        'restart_launch.json', 'restart_wait.json', f'run_restart{label}_1800.py',
        f'{ARCHIVE}/manifest.json', 'restart_retry_start_load.json', 'restart_retry_timing.json')]
    artifacts.extend(case / name for name in ('provenance.json', 'status.json', 'full_workflow_receipt.json',
                                               'n8_payload_identity.json'))
    artifacts.extend(case.glob('p7p8_event*_gate.json'))
    artifacts.extend(case.glob('f3b_event*_timer_gate.json'))
    artifacts.extend(case.glob('rank_output/*/rank.*/stdout'))
    artifacts.extend((restart, proof))
    fingerprints = {key: build[key] for key in runner.lab.FINGERPRINTS}
    workflow.sealed_write(work / 'run_receipt.json', {**fingerprints, 'passed': True, 'mode': 'CORE',
        'case': str(case), 'hashes': genuine, 'restart': str(restart),
        'events': workflow.event_collector()(case), 'config_sha256': sha(config),
        'artifacts': {str(path.relative_to(work)): sha(path) for path in artifacts}})
    runner.verify_run(work, 'CORE', build, payloads, expected)
    print('FP2-C1 CORE PASSED after isolated-restart watchdog retry.', flush=True)


if __name__ == '__main__':
    parser = ArgumentParser(description=__doc__)
    parser.add_argument('--self-check', action='store_true')
    args = parser.parse_args()
    if args.self_check:
        work = HERE / 'G5/build647/CORE'
        target, source = helper_source(work, 647)
        assert 'run_restart647.py' not in source
        assert source.count(target.name) == 4
        compile(source, str(target), 'exec')
    else:
        core()
