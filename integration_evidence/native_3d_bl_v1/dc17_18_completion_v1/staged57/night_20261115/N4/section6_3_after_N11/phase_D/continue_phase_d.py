"""Advance the reviewed phase-D cuts at their passed gates."""
from pathlib import Path
import shutil
import subprocess
import time

from run_g5 import ROOT, progress
from run_identity import cut


def git(*arguments):
    return subprocess.check_output(['git', *map(str, arguments)], cwd=cut.REPO, text=True).strip()


def launch(name, argv):
    with (ROOT / f'{name}_campaign.log').open('x') as log:
        child = subprocess.Popen([cut.PYTHON, *map(str, argv)], cwd=cut.REPO,
                                 stdout=log, stderr=subprocess.STDOUT)
        handle = cut.actual_identity(child.pid)
        cut.save(ROOT / f'{name}_campaign_launch.json', handle)
        code = child.wait()
    cut.save(ROOT / f'{name}_campaign_wait.json', {**handle, 'returncode': code, 'terminal': cut.terminal(handle)})
    assert code == 0 and cut.terminal(handle), (name, code, log.name)


def scope_receipts(name, build):
    workspace = cut.PHASE / name / 'validation'
    summary = cut.read(workspace / 'acceptance_summary.json')
    assert summary['state'] == 'PASSED' and summary['build'] == build
    assert [row['records'] for row in summary['G1'].values()] == [375, 114, 228]
    assert summary['G2']['work'] == 90_809_869
    assert summary['G2']['accepted_collapses'] == [480, 473, 346, 263, 469, 464]
    assert all(summary[gate]['passed'] for gate in ('G2', 'G3', 'GQ'))
    if name == 'D2':
        assert summary['G4']['passed']
    for cohort in ('serial', 'mpi2', 'mpi4'):
        result = cut.read(workspace / f'G1_candidate_{cohort}_summary.json')
        receipt = cut.read(Path(result['candidate_attempt']) / 'n11_terminal_receipt.json')
        assert receipt['all_owned_handles_terminal']
        assert all(cut.terminal(row) for row in receipt['owned_processes'])
    pins = cut.read(cut.PHASE / f'build{build}/source_pins.json')
    assert all(cut.sha(path) == digest for path, digest in pins.items())
    assert cut.terminal(cut.read(ROOT / f'{name}_campaign_launch.json'))
    return workspace, summary


def checkpoint(name, build, extra_paths):
    workspace, summary = scope_receipts(name, build)
    source = cut.read(workspace / 'candidate_source.json')
    assert source['base_commit'] == git('rev-parse', 'HEAD')
    assert not git('diff', '--cached', '--name-only')
    paths = [Path(path) for path in cut.read(workspace / 'checkpoint_paths.json')]
    paths += [workspace / 'checkpoint_paths.json', workspace / 'run_g2_gq_prefix.py']
    paths += [cut.REPO / path for path in source['source_paths']]
    paths += [cut.NIGHT / 'progress.md', cut.NIGHT / 'QUESTIONS_FOR_REVIEWER.md']
    paths += list(ROOT.glob(f'{name}_campaign*'))
    paths += extra_paths
    numbers = (f"Build{build} passed G1 (375/114/228), G2 (90,809,869), "
               f"G3 and GQ ({summary['GQ']['seconds_until_stop']:.3f} s).")
    if name == 'D2':
        numbers += ' G4 also passed.'
    progress(name, f'{numbers} Preparing checkpoint.')
    assert all(path.is_file() for path in paths)
    git('add', '-f', '--', *sorted(set(paths)))
    git('diff', '--cached', '--check', '--', cut.SOURCE)
    git('commit', '-m', f'Qualify phase D sub-step {name} on build{build}')
    commit = git('rev-parse', 'HEAD')
    git('push', 'origin', 'HEAD:refs/heads/codex/native-3d-bl')
    cut.save(ROOT / f'{name}_checkpoint.json', {'commit': commit, 'build': build, 'passed': True})
    progress(name, f'{numbers} Pushed `{commit[:10]}`.')
    print(f'{name} checkpoint pushed: {commit}', flush=True)


def apply_draft():
    draft = ROOT / 'D2_draft'
    files = cut.SOURCE / 'CNativeEngine3D.cpp', cut.SOURCE / 'native_engine3d/EngineAdapt.cpp'
    assert not files[1].exists()
    assert all(cut.sha(path) == digest for path, digest in cut.read(draft / 'reviewed_pins.json').items())
    subprocess.run([cut.PYTHON, str(cut.PREFLIGHT), str(ROOT / 'D2_before_edit_preflight.json')],
                   cwd=cut.REPO, check=True)
    for path in files:
        shutil.copyfile(draft / path.name, path)
    git('diff', '--check', '--', cut.SOURCE)
    progress('D2', 'Applied the reviewed extraction. Adapt is 58 formatted lines. Starting build572.')


def main():
    assert Path('/proc/1/comm').read_text().strip() == 'systemd'
    d1 = cut.read(ROOT / 'D1_campaign_launch.json')
    while not cut.terminal(d1):
        time.sleep(15)
    recovery = ROOT / 'D1_R3_amendment_pin'
    checkpoint('D1', 571, [ROOT / file for file in ('run_identity.py', 'source_readings.md',
               'D1_before_edit_preflight.json', 'continue_phase_d.py')] +
               [recovery / file for file in ('correction.json', 'D1_campaign.log', 'D1_campaign_launch.json')])
    apply_draft()
    launch('D2', [ROOT / 'run_identity.py', 'D2', 571, 572])
    checkpoint('D2', 572, [ROOT / 'D1_checkpoint.json', ROOT / 'D2_before_edit_preflight.json'] +
               [ROOT / 'D2_draft' / file for file in ('reviewed_pins.json', 'readability.json',
                'syntax_final.exit', 'syntax_final.log', 'syntax_attempt1.log')])
    launch('G5', [ROOT / 'run_g5.py', 572])
    safeguards = ROOT / 'safeguards'
    runner = safeguards / 'run_build572_safeguards.py'
    with (safeguards / 'no_launch_checks.json').open('x') as output:
        subprocess.run([cut.PYTHON, str(runner), 'self-check'], cwd=cut.REPO, stdout=output, check=True)
    launch('standing_safeguards', [ROOT / 'run_standing_safeguards.py'])


if __name__ == '__main__':
    main()
