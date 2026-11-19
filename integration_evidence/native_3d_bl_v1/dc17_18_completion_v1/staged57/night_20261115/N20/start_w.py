"""Record the exact wide-only candidate and launch the ordered W campaign."""

from hashlib import sha256
from pathlib import Path
import subprocess

from run_a import N20, cut
from run_de import git
from run_w import A_CHECKPOINT, CANDIDATE, F_PATHS, a_checkpoint, overlay, progress, verify_overlay


def main():
    assert Path('/proc/1/comm').read_text().strip() == 'systemd'
    assert cut.terminal(cut.read(N20 / 'A_controller_launch.json'))
    base, pins = a_checkpoint(A_CHECKPOINT)
    assert git('rev-parse', 'HEAD') == base
    assert not git('diff', '--cached', '--name-only')
    assert all(cut.sha(path) == digest for path, digest in cut.read(pins).items())
    patch = overlay()
    for arguments in (['--check', '--whitespace=error'], ['--whitespace=error']):
        subprocess.run(['git', 'apply', *arguments], input=patch, cwd=cut.REPO, check=True)
    assert set(git('diff', '--name-only', '--', 'Common', 'SU2_CFD', 'UnitTests').splitlines()) == set(F_PATHS)
    verify_overlay(base)
    git('add', '--', *F_PATHS)
    git('diff', '--cached', '--check', '--', *F_PATHS)
    git('commit', '-m', 'N20 W candidate: enable the historical wide repair rounds')
    commit = git('rev-parse', 'HEAD')
    git('push', 'origin', 'HEAD:refs/heads/codex/native-3d-bl')
    cut.save(N20 / 'W_checkpoint.json',
             {'commit': commit, 'part_a_commit': base, 'overlay_source': CANDIDATE,
              'overlay_sha256': sha256(patch).hexdigest(), 'paths': list(F_PATHS), 'state': 'candidate'})
    argv = ['nice', '-n', '19', cut.PYTHON, '-B', '-u', str(N20 / 'run_w.py'), 'run']
    with (N20 / 'W_controller.log').open('x') as log:
        controller = subprocess.Popen(argv, stdin=subprocess.DEVNULL, stdout=log, stderr=subprocess.STDOUT,
                                      start_new_session=True)
        handle = cut.actual_identity(controller.pid)
        cut.save(N20 / 'W_controller_launch.json', dict(handle, actual_host_proc=True, argv=argv))
    progress(f'Pushed W candidate `{commit[:10]}`. Controller PID {handle["pid"]} / start {handle["start_ticks"]}.')
    print(commit, handle, flush=True)


if __name__ == '__main__':
    main()
