"""Push the passed Part B scope, then run the frozen full-workflow time reference."""
from pathlib import Path
import runpy
import shutil
import subprocess
import sys
import time

from run_scoped import N16, cut

BUILD = 574
SCOPE = cut.PHASE / 'N16B/validation'
WORKSPACE = cut.PHASE / 'round_8_5r_B/G5'
CASE = WORKSPACE / 'full574'
BASE = cut.PHASE / 'phase_D/G5'


def progress(message):
    with (N16.parent / 'progress.md').open('a') as file:
        file.write(f'\nN16 B. {message}\n')


def checkpoint():
    identity = cut.read(SCOPE / 'acceptance_summary.json')
    assert identity['state'] == 'PASSED' and identity['build'] == BUILD
    assert all(identity[gate]['passed'] for gate in ('G2', 'G3', 'G4', 'GQ'))
    assert [row['records'] for row in identity['G1'].values()] == [375, 114, 228]
    assert identity['G2']['work'] == 90_809_869
    manifest = cut.read(N16 / 'B_draft/manifest.json')
    assert all(cut.sha(cut.REPO / name) == row['draft'] for name, row in manifest.items())
    inputs = cut.read(cut.PHASE / f'build{BUILD}/source_pins.json')
    assert all(cut.sha(path) == digest for path, digest in inputs.items())
    progress(f'Build{BUILD} passed scoped identity. G1 375 / 114 / 228 unchanged, G2 work 90,809,869, '
             f"G3 {identity['G3']['accepted_splits']} splits and {identity['G3']['common_edit_count']} common edits exact, "
             f"G4 at 1 / 2 / 4 ranks, GQ at {identity['GQ']['seconds_until_stop']:.3f} s. Full T0 pending.")
    paths = [cut.REPO / name for name in manifest]
    paths += [Path(path) for path in cut.read(SCOPE / 'checkpoint_paths.json')]
    paths += [N16.parent / 'progress.md', SCOPE / 'checkpoint_paths.json', SCOPE / 'run_g2_gq_prefix.py']
    paths += [N16 / name for name in ('A_checkpoint.json', 'B_before_edit_preflight.json', 'B_campaign_launch.json',
                                    'B_campaign.log', 'run_scoped.py', 'draft_b.py', 'B_draft/manifest.json',
                                    'run_t0.py')]
    subprocess.run(['git', 'diff', '--cached', '--quiet'], check=True)
    subprocess.run(['git', 'add', '-f', '--', *map(str, paths)], check=True)
    subprocess.run(['git', 'diff', '--cached', '--check', '--', *manifest], check=True)
    subprocess.run(['git', 'commit', '-qm', 'Add disabled wider repair round capacity'], check=True)
    subprocess.run(['git', 'push', 'origin', 'HEAD:refs/heads/codex/native-3d-bl'], check=True)
    head = subprocess.check_output(['git', 'rev-parse', 'HEAD'], text=True).strip()
    cut.save(N16 / 'B_checkpoint.json', {'commit': head, 'build': BUILD, 'scoped_identity_passed': True})
    progress(f'Pushed `{head[:10]}`. Running the two-event T0 workflow.')
    print(f'N16 B checkpoint pushed: {head}', flush=True)


def run_t0():
    WORKSPACE.mkdir(parents=True)
    for name in ('check_connected_events.py', 'check_complete_events.py', 'check_timer_tree.py',
                 'unchanged_build499_source_pins.json'):
        shutil.copyfile(BASE / name, WORKSPACE / name)
    runner = WORKSPACE / 'run_full574.py'
    runner.write_text((BASE / 'run_full572.py').read_text().replace('572', '574'))
    with (WORKSPACE / 'self_check.json').open('x') as output:
        subprocess.run([cut.PYTHON, str(runner), '--self-check'], stdout=output, check=True)
    cut.run_gate(WORKSPACE, 'T0_full', [cut.PYTHON, str(runner), '--case', str(CASE)])
    expected = cut.read(N16.parent / 'section6_1/accepted_reference_payloads.json')['hashes']
    output = cut.read(CASE / 'n8_payload_identity.json')
    assert output['passed'] and output['workflow_gates_passed'] and output['hashes'] == expected
    assert all(cut.sha(CASE / name) == digest for name, digest in expected.items())
    previous = BASE / 'full572'
    reference = cut.read(previous / 'n8_payload_identity.json')
    assert reference['passed'] and reference['workflow_gates_passed']
    assert all(cut.sha(previous / name) == digest for name, digest in reference['rank_stdout_sha256'].items())
    comparator = runpy.run_path(str(cut.PHASE / 'candidate_full_preparation/validate_full.py'))['full_identity']
    identity = comparator(previous, CASE)
    cut.save(CASE / 'strict_full_identity.json', identity)
    assert identity['passed'], 'First difference is in strict_full_identity.json.'
    events = runpy.run_path(str(cut.PHASE / 'phase_C/collect_g5_summary.py'))['collect_events'](CASE)
    status = cut.read(CASE / 'status.json')
    assert status['all_owned_handles_terminal'] and all(cut.terminal(row) for row in status['owned_identities'])
    assert status['source_and_binary_pins_unchanged'] and status['inputs_unchanged']
    cut.save(WORKSPACE / 'acceptance_summary.json', {'passed': True, 'build': BUILD,
             'seconds_total': status['seconds_total'], 'reference_hashes': expected,
             'same_numerical_result': True, 'timer_snapshots_exact': identity['timer_rows']['timer_snapshots'],
             'events': events, 'case': str(CASE), 'no_restart_as_ordered': True})
    progress(f"T0 passed in {status['seconds_total']:.3f} s. All six payloads and old rows matched. Next is Part C.")


if __name__ == '__main__':
    assert Path('/proc/1/comm').read_text().strip() == 'systemd'
    controller = cut.read(N16 / 'B_campaign_launch.json')
    while not cut.terminal(controller):
        time.sleep(15)
    checkpoint()
    run_t0()
