"""Qualify N6 with the accepted six-rank workflow and isolated restart."""
from datetime import datetime
from pathlib import Path
import runpy
import shutil
import subprocess
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'phase_D'))
from run_identity import cut


ROOT = Path(__file__).resolve().parent
WORKSPACE = ROOT / 'G5'
REFERENCE = ROOT.parent / 'round_8_5r_B/G5/full574'
EXPECTED = cut.NIGHT / 'section6_1/accepted_reference_payloads.json'


def prepare(build):
    WORKSPACE.mkdir()
    source = ROOT.parent / 'phase_D/G5'
    for name in ('check_connected_events.py', 'check_complete_events.py', 'check_timer_tree.py',
                 'unchanged_build499_source_pins.json'):
        shutil.copyfile(source / name, WORKSPACE / name)
    for stem in ('run_full', 'run_restart', 'validate_restart'):
        runner = WORKSPACE / f'{stem}{build}.py'
        runner.write_text((source / f'{stem}572.py').read_text().replace('572', str(build)))
        compile(runner.read_bytes(), str(runner), 'exec')
    cut.save(WORKSPACE / 'retargeted_helper_pins.json',
             {str(path): cut.sha(path) for path in WORKSPACE.iterdir()})


def preflight(build):
    assert Path('/proc/1/comm').read_text().strip() == 'systemd'
    selection = cut.read(cut.PHASE / 'N6Selection/validation/acceptance_summary.json')
    assert selection['state'] == 'PASSED' and selection['build'] == build
    assert all(selection[gate]['passed'] for gate in ('G2', 'G3', 'G4', 'GQ'))
    assert cut.terminal(cut.read(cut.NIGHT / 'N6/Selection_campaign_launch.json'))
    for manifest in (WORKSPACE / 'retargeted_helper_pins.json',
                     cut.PHASE / f'build{build}/source_pins.json'):
        assert all(cut.sha(path) == digest for path, digest in cut.read(manifest).items())
    assert cut.read(REFERENCE.parent / 'acceptance_summary.json')['passed']
    reference = cut.read(REFERENCE / 'n8_payload_identity.json')
    assert reference['passed'] and reference['workflow_gates_passed']
    assert cut.read(REFERENCE / 'strict_full_identity.json')['passed']
    assert reference['hashes'] == cut.read(EXPECTED)['hashes']
    for pins in (reference['hashes'], reference['rank_stdout_sha256']):
        assert all(cut.sha(REFERENCE / path) == digest for path, digest in pins.items())
    for stem in ('run_full', 'run_restart', 'validate_restart'):
        runner = WORKSPACE / f'{stem}{build}.py'
        with (WORKSPACE / f'{runner.stem}_self_check.json').open('x') as output:
            subprocess.run([cut.PYTHON, str(runner), '--self-check'], stdout=output, check=True)


def progress(item, message):
    with (cut.NIGHT / 'progress.md').open('a') as output:
        output.write(f'\n- {datetime.now().astimezone().isoformat(timespec="seconds")} {item}. {message}\n')


def verify_restart(build, full, restart):
    receipt = cut.read(restart / 'post_terminal_handles.json')
    assert receipt['actual_host_proc'] and receipt['all_owned_handles_terminal']
    identities = [receipt['controller_identity'], receipt['controller_wrapper_identity']]
    identities += [row['identity'] for row in receipt['owned_identity_checks']]
    assert all(cut.terminal(row) for row in identities)
    validator = WORKSPACE / f'validate_restart{build}.py'
    with (WORKSPACE / 'restart_post_only_validation.log').open('x') as output:
        subprocess.run([cut.PYTHON, str(validator), '--source-case', str(full), '--case-dir', str(restart)],
                       stdout=output, stderr=subprocess.STDOUT, check=True)
    assert cut.read(restart / 'disk_restart_validation.json')['disk_restart_verified']


def main(build):
    preflight(build)
    full = WORKSPACE / f'full{build}'
    restart = WORKSPACE / f'disk_restart_{full.name}_event2'
    try:
        progress('N6', f'Build{build}, scoped Selection receipts passed. Starting full G5.')
        cut.run_gate(WORKSPACE, 'G5_full',
                     [cut.PYTHON, str(WORKSPACE / f'run_full{build}.py'), '--case', str(full)])
        compare = runpy.run_path(str(cut.PHASE / 'candidate_full_preparation/validate_full.py'))['full_identity']
        identity = compare(REFERENCE, full)
        cut.save(full / 'strict_full_identity.json', identity)
        assert identity['passed'], 'See strict_full_identity.json for the first full-workflow difference.'
        progress('N6', 'Both G5 events and six payload identities passed. Starting isolated restart.')
        cut.run_gate(WORKSPACE, 'G5_restart',
                     [cut.PYTHON, str(WORKSPACE / f'run_restart{build}.py'), '--source-case', str(full)])
        verify_restart(build, full, restart)
        qualify = runpy.run_path(str(ROOT.parent / 'phase_C/collect_g5_summary.py'))['qualify']
        qualify(full, restart, EXPECTED, WORKSPACE / 'acceptance_summary.json')
        progress('N6', 'G5 and isolated restart passed. Standing safeguards and phase report pending.')
    except Exception as error:
        progress('N6', f'Controller stopped at {error}. Inspect the preserved receipts and verify all handles.')
        raise


if __name__ == '__main__':
    if len(sys.argv) == 3 and sys.argv[1] == '--prepare':
        prepare(int(sys.argv[2]))
    else:
        assert len(sys.argv) == 2
        main(int(sys.argv[1]))
