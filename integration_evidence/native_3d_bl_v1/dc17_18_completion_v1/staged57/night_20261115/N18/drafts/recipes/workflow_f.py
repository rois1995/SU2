"""Run the ordered N18 Part F full workflows against the accepted T0 input."""
import argparse
from decimal import Decimal
from pathlib import Path
import runpy
import shutil
import subprocess
import sys

sys.dont_write_bytecode = True
NIGHT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(NIGHT / 'N16'))

from run_scoped import cut  # noqa: E402

N16_C = cut.PHASE / 'round_8_5r_C/G5'
WORKSPACE = cut.PHASE / 'round_8_5r2_F/G5'
T0 = cut.PHASE / 'round_8_5r_B/G5/full574'
SUMMARY = cut.PHASE / 'phase_C/collect_g5_summary.py'
COMPARATOR = NIGHT / 'section6_2/compare_reference_payloads.py'
RUNNERS = ('run_full', 'run_restart', 'validate_restart')
HELPERS = ('check_connected_events.py', 'check_complete_events.py', 'check_timer_tree.py',
           'unchanged_build499_source_pins.json')
T0_LIMITS = {1: Decimal('2236.870'), 2: Decimal('1416.499')}
COMBINED_LIMIT = Decimal('3288.032')


def purpose(first, second):
    """Gate 8 uses both accepted runs and the written per-event and combined limits."""
    clocks = []
    for run in (first, second):
        assert [row['event'] for row in run] == [1, 2]
        values = {row['event']: Decimal(str(row['native_seconds'])) for row in run}
        assert all(seconds.is_finite() and seconds > 0 for seconds in values.values())
        clocks.append(values)
    selected = {event: min(clock[event] for clock in clocks) for event in T0_LIMITS}
    combined = sum(selected.values(), Decimal(0))
    events = [
        {'event': event, 'T0_native_limit_seconds': str(limit),
         'run1_native_max_seconds': str(clocks[0][event]), 'run2_native_max_seconds': str(clocks[1][event]),
         'smaller_native_max_seconds': str(selected[event]), 'passed': selected[event] <= limit}
        for event, limit in T0_LIMITS.items()
    ]
    combined_pass = combined <= COMBINED_LIMIT
    return {'passed': combined_pass and all(row['passed'] for row in events), 'events': events,
            'smaller_native_max_sum_seconds': str(combined), 'combined_limit_seconds': str(COMBINED_LIMIT),
            'combined_passed': combined_pass, 'S_is_report_only': True}


def prepare(build):
    """Bind the tested N16 C helper chain in one fresh G5 workspace."""
    WORKSPACE.mkdir(parents=True)
    targets = {}
    for name in HELPERS:
        source = N16_C / name
        target = WORKSPACE / name
        shutil.copyfile(source, target)
        targets[target] = source
    for stem in RUNNERS:
        source = N16_C / f'{stem}575.py'
        target = WORKSPACE / f'{stem}{build}.py'
        text = source.read_text()
        assert 'build575' in text
        text = text.replace('575', str(build)).replace('Part C run one', 'N18 F run one')
        compile(text, str(target), 'exec')
        target.write_text(text)
        targets[target] = source
    cut.save(WORKSPACE / 'helper_pins.json', {'build': build,
             'source': {str(path): cut.sha(path) for path in targets.values()},
             'bound': {str(path): cut.sha(path) for path in targets},
             'comparison_source': {str(path): cut.sha(path) for path in (
                 NIGHT / 'N16/event_prefix.py', SUMMARY, COMPARATOR, Path(__file__).resolve())}})
    print(WORKSPACE, flush=True)


def verify_pins(manifest):
    for name, digest in manifest.items():
        assert cut.sha(name) == digest, name


def terminal_case(case):
    """Require the actual PID/start pairs of a finished full run to be terminal."""
    status = cut.read(case / 'status.json')
    assert status['all_owned_handles_terminal'] and status['state'] == 'terminal'
    assert all(cut.terminal(row) for row in status['owned_identities'])
    return status


def preflight(build, scoped, safeguards):
    """Bind this campaign to the passed scoped gates and its frozen final binary."""
    assert Path('/proc/1/comm').read_text().strip() == 'systemd'
    for path in (scoped, safeguards):
        receipt = cut.read(path)
        assert receipt['passed'] and receipt['build'] == build, path
    assert cut.read(T0.parent / 'acceptance_summary.json')['passed']
    # T0 source and binary are historical. Its terminal identities and retained rank logs are still verifiable.
    assert terminal_case(T0)['full_workflow_accepted']
    ref = cut.read(T0 / 'n8_payload_identity.json')
    assert ref['passed'] and ref['workflow_gates_passed'] and len(ref['rank_stdout_sha256']) == 6
    assert cut.sha(T0 / 'full_workflow_receipt.json') == ref['workflow_receipt_sha256']
    verify_pins({str(T0 / path): digest for path, digest in ref['rank_stdout_sha256'].items()})
    pins = cut.read(WORKSPACE / 'helper_pins.json')
    assert pins['build'] == build
    verify_pins(pins['bound'])
    verify_pins(pins['source'])
    verify_pins(pins['comparison_source'])
    sealed = cut.PHASE / f'build{build}'
    status = cut.read(sealed / 'build_status.json')
    handles = cut.read(sealed / 'terminal_handles.json')
    assert status['state'] == 'terminal' and status['exit_code'] == 0
    assert status['pins_verified'] and not status['changed']
    assert handles['all_owned_handles_terminal'] and all(cut.terminal(row) for row in handles['identities'])
    verify_pins(cut.read(sealed / 'source_pins.json'))
    assert cut.sha(status['binary']) == status['binary_sha256']
    assert cut.sha(status['test_binary']) == status['test_binary_sha256']
    for stem in RUNNERS:
        output = WORKSPACE / f'{stem}_selfcheck.json'
        if not output.exists():
            script = WORKSPACE / f'{stem}{build}.py'
            with output.open('x') as file:
                subprocess.run([cut.PYTHON, str(script), '--self-check'], cwd=cut.REPO, stdout=file, check=True)
        assert cut.read(output)['passed']
    return status['binary_sha256']


def run_or_resume(build, key, stem, *args):
    """Start a fresh stage or reuse its recorded zero exit and verified terminal controller."""
    waited = WORKSPACE / f'{key}_wait.json'
    launched = WORKSPACE / f'{key}_launch.json'
    if not waited.exists():
        assert not launched.exists(), f'Unverified prior stage: {launched}'
        script = WORKSPACE / f'{stem}{build}.py'
        cut.run_gate(WORKSPACE, key, [cut.PYTHON, str(script), *map(str, args)])
    receipt = cut.read(waited)
    launch = cut.read(launched)
    assert receipt['controller'] == launch['controller']
    assert receipt['terminal'] and cut.terminal(receipt['controller'])
    if receipt['returncode'] != 0:
        raise RuntimeError(f'{key} exit {receipt["returncode"]}; see {waited}')


def compare_event_one(reference, candidate):
    """Reuse the accepted N16 comparison with only new M28 families observational."""
    compare = runpy.run_path(str(NIGHT / 'N16/event_prefix.py'))['compare']
    globals_ = compare.__globals__
    marked = globals_['marked_rows']

    def old_rows(log, rank):
        return (row for row in marked(log, rank) if not row[0].startswith('NATIVE3D_M28_'))

    globals_['marked_rows'] = old_rows
    return compare(reference, candidate)


def prefix_identity(case):
    """Use the accepted N16 comparator and write its exact per-rank diff receipt."""
    result = compare_event_one(T0, case)
    pin = case / 'event_one_construction_identity.json'
    if not pin.exists():
        cut.save(pin, result)
    assert cut.read(pin) == result
    assert result['passed'], pin
    return result


def verify_full(case, binary_sha):
    """Check the inherited standing gates, output digests, and current binary provenance."""
    status = terminal_case(case)
    assert status['exit_code'] == 0 and status['full_workflow_accepted']
    assert cut.read(case / 'provenance.json')['binary_sha256'] == binary_sha
    identity = cut.read(case / 'n8_payload_identity.json')
    assert identity['passed'] and identity['workflow_gates_passed'] and not identity['mismatches']
    assert identity['workflow_receipt_sha256'] == cut.sha(case / 'full_workflow_receipt.json')
    verify_pins({str(case / path): digest for path, digest in identity['rank_stdout_sha256'].items()})
    compare_payloads = runpy.run_path(str(COMPARATOR))
    assert compare_payloads['payload_hashes'](case) == identity['hashes']
    return identity


def verify_restart(build, source_case):
    """Retain the inherited post-only isolated restart validator and PID checks."""
    case = WORKSPACE / f'disk_restart_{source_case.name}_event2'
    receipt = cut.read(case / 'post_terminal_handles.json')
    assert receipt['actual_host_proc'] and receipt['all_owned_handles_terminal']
    handles = [receipt['controller_identity'], receipt['controller_wrapper_identity']]
    handles.extend(row['identity'] for row in receipt['owned_identity_checks'])
    assert all(cut.terminal(handle) for handle in handles)
    validation = case / 'disk_restart_validation.json'
    if not validation.exists():
        validator = WORKSPACE / f'validate_restart{build}.py'
        with (WORKSPACE / 'restart_post_only.log').open('x') as file:
            subprocess.run([cut.PYTHON, str(validator), '--source-case', str(source_case), '--case-dir', str(case)],
                           cwd=cut.REPO, stdout=file, stderr=subprocess.STDOUT, check=True)
    result = cut.read(validation)
    assert result['disk_restart_verified'] and result['all_inputs_and_pins_unchanged']
    assert result['source_event_receipt_sha256'] == cut.sha(source_case / 'full_workflow_receipt.json')
    verify_pins({str(case / path): digest for path, digest in result['artifact_sha256'].items()})
    return validation


def observe_failed_case(case):
    """Hash untouched terminal logs and retain prefix evidence even when acceptance failed."""
    if not (case / 'status.json').is_file():
        return {}
    try:
        status = terminal_case(case)
    except (AssertionError, OSError, ValueError, KeyError) as error:
        return {'case': str(case), 'terminal_status_verified': False, 'observation_error': str(error)}
    paths = sorted(case.glob('rank_output/*/rank.*/stdout'))
    observations = {'case': str(case), 'status_sha256': cut.sha(case / 'status.json'),
                    'stop_reason': status['stop_reason'], 'exit_code': status['exit_code'],
                    'rank_stdout_sha256': {str(path.relative_to(case)): cut.sha(path) for path in paths}}
    if case.name.endswith('_run1') and len(paths) == 6 and all(
            'NATIVE3D_PRIVATE_WORK ' in path.read_text(errors='replace') for path in paths):
        try:
            observations['event_one_prefix'] = compare_event_one(T0, case)
        except (AssertionError, OSError, ValueError, KeyError) as error:
            observations['event_one_prefix_observation_error'] = str(error)
    return observations


def run(build, scoped, safeguards):
    """Serialize G5 run one, its restart, and run two, then apply purpose gate eight."""
    binary_sha = preflight(build, scoped, safeguards)
    first = WORKSPACE / f'full{build}_run1'
    second = WORKSPACE / f'full{build}_run2'
    cases = [(first, 'run1'), (second, 'run2')]
    gate_name = 'G5 run one'
    try:
        run_or_resume(build, 'F_run1', 'run_full', '--case', first)
        first_receipt = verify_full(first, binary_sha)
        gate_name = 'event-one construction prefix'
        prefix_identity(first)
        gate_name = 'isolated event-two restart'
        run_or_resume(build, 'F_restart', 'run_restart', '--source-case', first)
        restart = verify_restart(build, first)
        gate_name = 'G5 run two and repeat payloads'
        run_or_resume(build, 'F_run2', 'run_full', '--case', second)
        second_receipt = verify_full(second, binary_sha)
        assert first_receipt['hashes'] == second_receipt['hashes']
    except (AssertionError, OSError, ValueError, KeyError, RuntimeError, subprocess.CalledProcessError) as error:
        failed = {'passed': False, 'interrupted_stage': gate_name, 'error': str(error),
                  'gate8_evaluated': False, 'binary_sha256': binary_sha,
                  'terminal_cases': [observed for case, _ in cases if (observed := observe_failed_case(case))]}
        cut.save(WORKSPACE / 'failed_attempt.json', failed)
        raise
    collect = runpy.run_path(str(SUMMARY))['collect_events']
    clock_events = {label: collect(case) for case, label in ((T0, 'T0'), *cases)}
    purpose_receipt = purpose(clock_events['run1'], clock_events['run2'])
    cut.save(WORKSPACE / 'purpose_rule.json', purpose_receipt)
    passed = purpose_receipt['passed']
    result = {'passed': passed, 'build': build, 'binary_sha256': binary_sha, 'standing_gates_passed': True,
              'event_one_prefix_identity': True, 'isolated_restart_receipt': str(restart),
              'repeat_payloads_exact': True, 'purpose': purpose_receipt, 'events': clock_events,
              'two_run_payload_sha256': second_receipt['hashes']}
    cut.save(WORKSPACE / 'acceptance_summary.json', result)
    if passed:
        cut.save(WORKSPACE / 'qualified_reference_payloads.json',
                 {'accepted': True, 'hashes': second_receipt['hashes'], 'binary_sha256': binary_sha,
                  'first_case': str(first), 'repeat_case': str(second),
                  'acceptance_sha256': cut.sha(WORKSPACE / 'acceptance_summary.json')})
    return passed


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('action', choices=('prepare', 'run'))
    parser.add_argument('--build', type=int, default=580)
    parser.add_argument('--scoped', type=Path)
    parser.add_argument('--safeguards', type=Path)
    args = parser.parse_args()
    if args.action == 'prepare':
        prepare(args.build)
        return
    if args.scoped is None or args.safeguards is None:
        parser.error('run requires --scoped and --safeguards receipts')
    raise SystemExit(0 if run(args.build, args.scoped, args.safeguards) else 2)


if __name__ == '__main__':
    main()
