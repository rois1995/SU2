"""Apply the exact N21 wide overlay, then run its inherited scoped checks."""

from hashlib import sha256
from pathlib import Path
import subprocess
import sys

from run_p_scoped import BUILD as P_BUILD, N21, bind_build, bind_identity, cut, git
from run_w import CANDIDATE, F_PATHS, F_SCOPED, bind_g3_gate, overlay, retarget, verify_overlay

W_BUILD = 588
WORK = cut.PHASE / 'N21W/validation'
BRANCH = cut.PHASE / 'round_8_5t_W'


def progress(message):
    with (cut.NIGHT / 'progress.md').open('a') as out:
        out.write(f'\nN21 W. {message}\n')


def apply_wide():
    p_commit = cut.read(N21 / 'P_checkpoint.json')
    assert p_commit['passed'] and p_commit['build'] == P_BUILD
    assert cut.terminal(cut.read(N21 / 'P_scoped_launch.json'))
    assert cut.terminal(cut.read(N21 / 'P_G5_launch.json'))
    pins = cut.PHASE / f'build{P_BUILD}/source_pins.json'
    assert all(cut.sha(path) == digest for path, digest in cut.read(pins).items())
    base = p_commit['commit']
    assert git('rev-parse', 'HEAD') == base
    assert not git('diff', '--cached', '--name-only')
    assert not git('diff', '--name-only', '--', 'Common', 'SU2_CFD', 'UnitTests')
    subprocess.run([cut.PYTHON, str(cut.PREFLIGHT), str(N21 / 'W_entry_host_preflight.json')], check=True)
    patch = overlay()
    for flags in (['--check'], []):
        subprocess.run(['git', 'apply', '--whitespace=error', *flags], input=patch, cwd=cut.REPO, check=True)
    expected = verify_overlay(base)
    assert set(git('diff', '--name-only', '--', 'Common', 'SU2_CFD', 'UnitTests').splitlines()) == set(F_PATHS)
    git('add', '--', *F_PATHS)
    git('diff', '--cached', '--check', '--', *F_PATHS)
    git('commit', '-m', 'N21 W candidate: enable the historical wide repair rounds')
    commit = git('rev-parse', 'HEAD')
    cut.save(N21 / 'W_checkpoint.json', {'commit': commit, 'base_commit': base, 'build': W_BUILD,
             'overlay_source': CANDIDATE, 'overlay_sha256': sha256(patch).hexdigest(),
             'source_sha256': expected, 'state': 'candidate'})
    git('push', 'origin', 'HEAD:refs/heads/codex/native-3d-bl')
    progress(f'Applied and pushed exact three-path wide candidate `{commit[:10]}`. Starting build{W_BUILD}.')
    return bind_build(P_BUILD, W_BUILD, F_PATHS, pins)


def scoped_helpers():
    helpers = retarget(F_SCOPED, (
        ("glob('N18F_G3_*/g3_end_b_gate.json')", "glob('N21W_G3_*/g3_end_b_gate.json')"),
        ('Build580 compiled without warnings.', f'Build{W_BUILD} compiled without warnings.'),
        ('F scoped gates passed', 'Scoped gates passed'),
    ))
    helpers.update(BUILD=W_BUILD, SCOPED=WORK, F=BRANCH, progress=progress)
    return helpers


if __name__ == '__main__':
    assert Path('/proc/1/comm').read_text().strip() == 'systemd'
    target = apply_wide()
    assert bind_identity('N21W', W_BUILD) == WORK
    bind_g3_gate(WORK)
    helpers = scoped_helpers()
    helpers['build_and_test'](target)
    helpers['scoped_checks']()
    helpers['safeguards']()
    progress('Passed all three remaining standing safeguards. Starting G5 run 1.')
    cut.run_gate(N21, 'W_G5', [cut.PYTHON, str(N21 / 'run_g5.py'), 'W'])
