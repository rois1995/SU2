"""Apply each reviewed N22 draft after its predecessor is terminal, then seal its gates."""
from argparse import ArgumentParser
from pathlib import Path
import runpy
import shutil
import subprocess
import sys
import time

from gq_baseline import ITEM, MANIFEST, cut, git
from run_de import checkpoint
from seal_f import archive

scoped = runpy.run_path(str(ITEM / 'run_scoped.py'))
BUILDS = scoped['BUILDS']
case_artifacts = scoped['case_artifacts']
paths = scoped['paths']


def draft_files(part):
    draft = ITEM / 'drafts' / part
    sources = {path.relative_to(draft): path for path in draft.rglob('*')
               if path.is_file() and path.suffix in ('.cpp', '.hpp')}
    if part == 'C':
        sources[Path('UnitTests/Common/adaptation/CNativeDistributed3D_tests.cpp')] = draft / 'census_fixture.inc'
    return sources


def validate_draft(part):
    receipt = cut.read(ITEM / 'draft_pins.json')[part]
    sources = draft_files(part)
    assert set(receipt) == {str(path) for path in sources}
    assert all(cut.sha(draft) == receipt[str(path)] for path, draft in sources.items())
    return sources


def apply(part):
    previous, build = BUILDS[part]
    local, _, _, _ = paths(part, build)
    local.mkdir(exist_ok=True)
    base = cut.read(MANIFEST) if part == 'M' else cut.read(ITEM / 'M_checkpoint.json')
    assert base['passed'] and base['commit'] == git('rev-parse', 'HEAD')
    assert cut.terminal(cut.read(ITEM / 'gq_reference_controller_launch.json'))
    assert not git('diff', '--name-only', 'HEAD', '--', 'Common', 'SU2_CFD', 'UnitTests')
    pins = cut.read(cut.PHASE / f'build{previous}/source_pins.json')
    assert all(cut.sha(path) == digest for path, digest in pins.items())
    sources = validate_draft(part)
    subprocess.run([cut.PYTHON, str(cut.PREFLIGHT), str(local / 'before_edit_host_preflight.json')], check=True)
    for path, draft in sources.items():
        target = cut.REPO / path
        assert str(target) in pins
        if draft.suffix == '.inc':
            with target.open('ab') as output:
                output.write(draft.read_bytes())
        else:
            shutil.copyfile(draft, target)
    assert set(git('diff', '--name-only', 'HEAD', '--', 'Common', 'SU2_CFD', 'UnitTests').splitlines()) == {
        str(path) for path in sources}
    git('diff', '--check', '--', *sources)
    progress(part, f'Applied the reviewed draft after the predecessor was terminal. Starting build{build}.')
    return list(sources)


def progress(part, message):
    with (cut.NIGHT / 'progress.md').open('a') as out:
        out.write(f'\nN22 {part}. {message}\n')


def evidence(part, build, local, g5):
    roots = [local, g5, *(case for case in g5.glob(f'full{build}_run*') if case.is_dir())]
    result = case_artifacts(*roots)
    result.update(Path(path) for path in cut.read(local / 'scoped_paths.json'))
    result.update((cut.NIGHT / 'progress.md', cut.NIGHT / 'QUESTIONS_FOR_REVIEWER.md'))
    if part == 'M':
        result.update(Path(path) for path in cut.read(ITEM / 'gq_reference_paths.json'))
        result.update(path for path in ITEM.iterdir() if path.is_file() and path.suffix in ('.json', '.md', '.py'))
        result.update((ITEM / 'inputs').iterdir())
        result.update((cut.NIGHT / 'FP0/inputs').iterdir())
    return result


def settle_part(part, build, sources):
    local, _, _, branch = paths(part, build)
    g5 = branch / 'G5'
    result = cut.read(g5 / 'acceptance_summary.json')
    assert result['standing_gates_passed'] and result['build'] == build
    if not result['passed']:
        assert part == 'C' and result['status'] == 'OPEN for the user' and result['retained']
    progress(part, f"Build{build} reached {result['status']} with passing standing workflow gates. Preparing checkpoint.")
    files = evidence(part, build, local, g5)
    if part == 'M':
        recovery = ITEM / 'R3_M_archive'
        if recovery.is_dir():
            files.update(recovery.iterdir())
    message = f'N22 {part} {result["status"]}'
    commit = checkpoint(message, [*sources, *files])
    cut.save(ITEM / f'{part}_checkpoint.json', {'passed': result['passed'], 'status': result['status'],
             'commit': commit, 'build': build, 'workflow': str(g5 / 'acceptance_summary.json')})
    progress(part, f'Pushed `{commit[:10]}`. ' + ('Run Part C next.' if part == 'M' else 'N22 reporting follows.'))


def run_part(part):
    _, build = BUILDS[part]
    local, _, _, _ = paths(part, build)
    sources = apply(part)
    cut.run_gate(local, 'scoped', [cut.PYTHON, '-B', str(ITEM / 'run_scoped.py'), part])
    cut.run_gate(local, 'workflow', [cut.PYTHON, '-B', str(ITEM / 'run_g5_85u.py'), part])
    settle_part(part, build, sources)


def recover_m():
    """Resume a passed M workflow whose archive was made while the scope log was open."""
    assert cut.terminal(cut.read(ITEM / 'ordered_launch.json'))
    local, _, target, _ = paths('M', 590)
    for name in ('scoped', 'workflow'):
        wait = cut.read(local / f'{name}_wait.json')
        assert wait['terminal'] and wait['returncode'] == 0 and cut.terminal(wait['controller'])
    scope = cut.read(local / 'scoped_acceptance.json')
    assert scope['passed'] and scope['build'] == 590
    assert all(cut.sha(path) == digest for path, digest in cut.read(target / 'source_pins.json').items())
    assert git('rev-parse', 'HEAD') == cut.read(MANIFEST)['commit']
    sources = validate_draft('M')
    assert all(cut.sha(cut.REPO / path) == cut.sha(draft) for path, draft in sources.items())
    log = local / 'scoped_outer.log'
    incomplete = log.with_suffix('.log.gz')
    recovery = ITEM / 'R3_M_archive'
    recovery.mkdir(exist_ok=True)
    incomplete.rename(recovery / 'scoped_while_open.log.gz')
    archive(log)
    (ITEM / 'ordered.log.gz').rename(recovery / 'ordered_while_open.log.gz')
    archive(ITEM / 'ordered.log')
    progress('M', 'The two-event workflow and scopes are terminal. Repacked the final scoped log after the open-log '
             'archive mismatch. Reusing the passing numerical results for the checkpoint.')
    settle_part('M', 590, list(sources))


def main():
    parser = ArgumentParser(description=__doc__)
    parser.add_argument('part', choices=('M', 'C', 'ordered', 'recover-M'))
    args = parser.parse_args()
    assert Path('/proc/1/comm').read_text().strip() == 'systemd'
    if args.part in ('M', 'ordered'):
        launch = cut.read(ITEM / 'gq_reference_controller_launch.json')
        while not cut.terminal(launch):
            time.sleep(10)
        assert cut.read(MANIFEST)['passed']
        while not (ITEM / 'gq_reference_paths.json').is_file():
            time.sleep(5)
        run_part('M')
    if args.part == 'recover-M':
        recover_m()
    if args.part in ('C', 'ordered', 'recover-M'):
        run_part('C')


if __name__ == '__main__':
    main()
