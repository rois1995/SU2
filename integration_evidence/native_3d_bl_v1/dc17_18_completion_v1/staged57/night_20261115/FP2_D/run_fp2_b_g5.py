"""Run the FP2-D S workflow using the pinned FP2-C1 S controller and validators."""

from argparse import ArgumentParser
from importlib.util import module_from_spec, spec_from_file_location
from pathlib import Path
import runpy
import sys
from unittest.mock import patch

sys.dont_write_bytecode = True
DRIVER = Path(__file__).resolve()
HERE = DRIVER.parent
C1 = HERE.parent / 'FP2_C1'
sys.path.insert(0, str(C1))

# This script and the pinned C1 script have the same basename.
spec = spec_from_file_location('c1_surface_g5', C1 / DRIVER.name)
prior = module_from_spec(spec)
spec.loader.exec_module(prior)
from retry_c1_restart import retry_restart, terminal_wait
from run_c2_lab import C2, ORDER, ORDER_SHA, S0, S0_BUILD as S0_BUILD_NUMBER, a, continuation_pins, verify_checkpoints

cut, sha, workflow = prior.cut, prior.sha, prior.workflow
WATCHDOG_ORDER = C1 / 'inputs/FP2_C1_CONTINUE_2.md'
WATCHDOG_ORDER_SHA = 'f65f2f302302f8f546ffcc23bb84a9e2ababa40defd9995e9779be456a94eda3'
S0_BUILD = C2 / 'build655/build_status.json'
S0_WORK = C2 / 'G5/build655/S'
FULL_WATCHDOG = 14400
FULL_RETRY = 'full_watchdog_retry'
ORIGINAL_ISOLATE = workflow.isolate_restart
ORIGINAL_SEAL = workflow.sealed_write


class WatchdogOpen(RuntimeError):
    """The allowed 3x helper retry hit its watchdog again."""


def verify_orders():
    prior.verify_order()
    continuation_pins()
    for path, digest in ((ORDER, ORDER_SHA), (WATCHDOG_ORDER, WATCHDOG_ORDER_SHA)):
        assert sha(path) == digest, path
        assert path.with_suffix('.sha256').read_text().split() == [digest, path.name], path


def s0_receipt():
    original = S0_WORK / 'run_receipt.json'
    retry = S0_WORK / FULL_RETRY / 'run_receipt.json'
    path, = (p for p in (original, retry) if p.is_file() and cut.read(p)['passed'])
    return path


def reference_sources(payloads):
    path = s0_receipt()
    build = cut.read(S0_BUILD)
    assert build['build'] == S0_BUILD_NUMBER and build['passed']
    for source in (path, S0_BUILD):
        from run_c2_lab import c3
        c3.c2.committed(source, S0)
    first = prior.verify_run(path.parent, 'S', build, payloads)
    assert first['config_sha256'] == prior.SURFACE_CONFIG_SHA and len(first['hashes']) == 6
    return [path, S0_BUILD, *(path.parent / name for name in first['artifacts'])]


def prerequisite_sources(build):
    label = build['build']
    fingerprints = {key: build[key] for key in prior.lab.FINGERPRINTS}
    fixtures = verify_checkpoints(label, fingerprints)
    lab_proof = HERE / 'cases' / f'build{label}/lab_acceptance.json'
    row = cut.read(lab_proof)
    assert row['passed'] and row['alarm']['passed'] and row['invariant']['passed'], lab_proof
    assert row['baseline_build'] == S0_BUILD_NUMBER and set(row['cases']) == {'N1', 'N6', 'N6P'}
    assert all(row[key] == value for key, value in fingerprints.items())
    artifacts = [Path(name) for name in row['artifacts']]
    assert artifacts and all(path.is_absolute() and sha(path) == row['artifacts'][str(path)] for path in artifacts)
    return list(dict.fromkeys((*fixtures, lab_proof, *artifacts)))


def restart_timed_out(work, case, seconds):
    wait = work / 'restart_wait.json'
    if not wait.is_file() or cut.read(wait)['returncode'] != 124:
        return False
    terminal_wait(work, 'restart', 124)
    restart = work / f'disk_restart_{case.name}_event2'
    status = cut.read(restart / 'status.json')
    assert status['state'] == 'terminal' and status['exit_code'] == 124
    if not status['pins_verified'] or not status['resource_gate']['passed']:
        return False
    argv = cut.read(restart / 'command.json')['argv']
    assert argv[argv.index('timeout') + 1] == str(seconds), argv
    post = cut.read(restart / 'post_terminal_handles.json')
    assert post['actual_host_proc'] and post['all_owned_handles_terminal'], post
    handles = [post['controller_identity'], post['controller_wrapper_identity']]
    handles += [row['identity'] for row in post['owned_identity_checks']]
    assert all(cut.terminal(handle) for handle in handles)
    return True


def isolate_with_retry(work, build, case):
    try:
        return ORIGINAL_ISOLATE(work, build, case)
    except (AssertionError, RuntimeError):
        if not restart_timed_out(work, case, 600):
            raise
    try:
        return retry_restart(work, build, case)
    except (AssertionError, RuntimeError):
        if not restart_timed_out(work, case, 1800):
            raise
        ORIGINAL_SEAL(work / 'restart_watchdog.json', {'status': 'OPEN for the user', 'watchdog_seconds': 1800,
            'attempt1_manifest_sha256': sha(work / 'restart_attempt1_watchdog/manifest.json'),
            'retry_timing': cut.read(work / 'restart_retry_timing.json')})
        raise WatchdogOpen('Isolated restart reached both helper watchdogs.')


def seal_with_retry_artifacts(path, value):
    work = path.parent
    archive = work / 'restart_attempt1_watchdog/manifest.json'
    if path.name == 'run_receipt.json' and archive.is_file():
        paths = [archive, work / f'run_restart{value["build"]}_1800.py',
                 work / 'restart_retry_start_load.json', work / 'restart_retry_timing.json']
        value['artifacts'].update({str(item.relative_to(work)): sha(item) for item in paths})
    ORIGINAL_SEAL(path, value)


def retry_controller():
    source = workflow.core_source()
    for old, new in (
        ('    env = os.environ.copy()\n',
         '    timeout_arg = argv.index("timeout") + 1\n'
         '    assert argv[timeout_arg] == "14400", argv\n'
         '    argv[timeout_arg] = "43200"\n'
         '    env = os.environ.copy()\n'),
        ('"application_watchdog_seconds": 14400', '"application_watchdog_seconds": 43200'),
        ('elapsed >= 14500', 'elapsed >= 43300'),
        ('controller_watchdog_after_14400_second_command_timeout',
         'controller_watchdog_after_43200_second_command_timeout'),
    ):
        source = workflow.replace_once(source, old, new)
    return source


def full_watchdog(work, label, seconds):
    """Record a helper timeout only where the terminal evidence shows no failed result gate."""
    case = work / f'full{label}_run1'
    if not (case / 'status.json').is_file() or not (work / 'full_wait.json').is_file():
        return False
    status = cut.read(case / 'status.json')
    is_timeout = ((status['stop_reason'] == 'normal_process_exit' and status['exit_code'] == 124)
                  or status['stop_reason'] == f'controller_watchdog_after_{seconds}_second_command_timeout')
    if not is_timeout:
        return False
    if not (status['inputs_unchanged'] and status['source_and_binary_pins_unchanged']
            and status['all_owned_handles_terminal']):
        return False
    assert status['state'] == 'terminal' and all(cut.terminal(row) for row in status['owned_identities'])
    wait = cut.read(work / 'full_wait.json')
    assert wait['returncode'] != 0
    terminal_wait(work, 'full', wait['returncode'])
    provenance = cut.read(case / 'provenance.json')
    assert provenance['application_watchdog_seconds'] == seconds
    argv = provenance['argv']
    assert argv[argv.index('timeout') + 1] == str(seconds)
    gates = sorted((*case.glob('f3b_event*_timer_gate.json'), *case.glob('p7p8_event*_gate.json')))
    if any(not cut.read(path)['passed'] for path in gates):
        return False
    if any((case / name).exists() for name in ('first_rejected_event.json', 'whole_job_peak_RSS_failure.json')):
        return False
    ORIGINAL_SEAL(work / 'full_watchdog.json', {
        'status': 'retry allowed' if seconds == FULL_WATCHDOG else 'OPEN for the user',
        'watchdog_seconds': seconds, 'host_at_launch': provenance['host_at_launch'],
        'host_preflight': cut.read(work / 'full_host_preflight.json'), 'wait': wait,
        'passed_evaluated_gates': {path.name: sha(path) for path in gates},
        'failure_sha256': sha(work / 'failed_attempt.json'), 'status_sha256': sha(case / 'status.json'),
    })
    return True


def bound():
    return patch.multiple(prior, HERE=HERE, __file__=str(DRIVER), S0=S0, S0_S=s0_receipt())


def run_attempt(work, build_dir, build, config, sources, payloads, full_retry=False):
    original_stage = workflow.run_stage
    bind_campaign = prior.BASE['PRIOR']['bind_campaign']

    def run_stage(path, name, argv):
        if full_retry and name == 'full':
            argv = [*argv, '--retry-full']
        return original_stage(path, name, argv)

    def campaign(mode, inputs):
        context = bind_campaign(mode, inputs)
        if full_retry:
            context['core_source'] = retry_controller
        return context

    with bound():
        if not (work / 'run_receipt.json').is_file():
            assert not (work / 'failed_attempt.json').exists(), work
            with patch.multiple(workflow, run_stage=run_stage, isolate_restart=isolate_with_retry,
                                sealed_write=seal_with_retry_artifacts), \
                    patch.dict(prior.BASE['PRIOR'], bind_campaign=campaign):
                prior.run_case(work, 'S', build_dir, build, config, sources, payloads)
        return prior.verify_run(work, 'S', build, payloads)


def run(build_dir):
    verify_orders()
    build, _, _ = prior.lab.prepare(build_dir)
    label = build['build']
    work = work_path(label)
    config = prior.config_for('S')
    payloads = runpy.run_path(str(workflow.PAYLOADS))
    orders = [ORDER, WATCHDOG_ORDER, prior.ORDER, *prior.BASE['ORDERS']]
    sources = [DRIVER, Path(prior.__file__), prior.B / 'run_fp2_b_g5.py', prior.INHERITED,
               Path(prior.lab.__file__), Path(prior.scope.__file__), C1 / 'retry_c1_restart.py',
               HERE / 'source_check.md', config, config.with_suffix('.cfg.sha256'), prior.FP2_A / 'run.cfg',
               build_dir / 'build_status.json', build_dir / 'source_pins.json']
    sources += [path for order in orders for path in (order, order.with_suffix('.sha256'))]
    sources += list(continuation_pins())
    sources += reference_sources(payloads) + prerequisite_sources(build)
    try:
        run_attempt(work, build_dir, build, config, sources, payloads)
    except (AssertionError, RuntimeError):
        if (work / 'restart_watchdog.json').is_file():
            raise WatchdogOpen('Isolated restart reached both helper watchdogs.')
        if not full_watchdog(work, label, FULL_WATCHDOG):
            raise
        sources += [work / 'full_watchdog.json', work / 'failed_attempt.json', work / 'full_host_preflight.json']
        retry = work / FULL_RETRY
        try:
            run_attempt(retry, build_dir, build, config, sources, payloads, full_retry=True)
        except (AssertionError, RuntimeError):
            if (retry / 'restart_watchdog.json').is_file():
                raise WatchdogOpen('Isolated restart reached both helper watchdogs.')
            if full_watchdog(retry, label, 3 * FULL_WATCHDOG):
                raise WatchdogOpen('Full workflow reached both helper watchdogs.')
            raise
    print('FP2-D: S passed; default path not yet proven', flush=True)


def work_path(label):
    return HERE / 'G5' / f'build{label}' / 'S'


def self_check():
    verify_orders()
    prior.config_for('S')
    reference_sources(runpy.run_path(str(workflow.PAYLOADS)))
    source = retry_controller()
    compile(source, 'full_controller.py', 'exec')
    assert '"application_watchdog_seconds": 43200' in source
    assert 'ADAP_NATIVE_MAX_TIME\\s*=\\s*5400' in source
    print('FP2-D config, C2 S reference and inherited watchdog binding verified.')


def main():
    parser = ArgumentParser(description=__doc__)
    parser.add_argument('build', type=Path, nargs='?')
    method = parser.add_mutually_exclusive_group(required=True)
    method.add_argument('--only', choices=('S',))
    method.add_argument('--full', choices=('S',), help='Internal supervised full controller.')
    method.add_argument('--self-check', action='store_true')
    parser.add_argument('--retry-full', action='store_true', help='Internal tripled full watchdog binding.')
    opts = parser.parse_args()
    if opts.retry_full and not opts.full:
        parser.error('--retry-full requires --full')
    if opts.self_check:
        self_check()
        return 0
    if opts.build is None:
        parser.error('a build directory is required')
    build = opts.build.resolve()
    if opts.full:
        label = cut.read(build / 'build_status.json')['build']
        work = work_path(label)
        if opts.retry_full:
            work /= FULL_RETRY
        with bound():
            prior.bind_inherited()
            prior.BASE['launch_full'](work, build, 'S')
    else:
        try:
            run(build)
        except WatchdogOpen as error:
            print(f'FP2-D: S OPEN for the user. {error}', flush=True)
            return 75
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
