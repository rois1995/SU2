"""Apply one reviewed N6 draft and seal the identical-results gates."""
from pathlib import Path
import shutil
import subprocess
import sys

N6 = Path(__file__).resolve().parent
sys.path.insert(0, str(N6.parent / 'N16'))
from run_scoped import bind_identity, check_build, cut, seal


def progress(text):
    with (N6.parent / 'progress.md').open('a') as file:
        file.write(f'\nN6. {text}\n')


def bind_build(label, previous, build, changed):
    prior = cut.PHASE / f'build{previous}'
    target = cut.PHASE / f'build{build}'
    pins = cut.read(prior / 'source_pins.json')
    pins.update({str(cut.REPO / path): cut.sha(cut.REPO / path) for path in changed})
    assert all(cut.sha(path) == digest for path, digest in pins.items())
    target.mkdir()
    cut.save(target / 'source_pins.json', pins)
    builder = (prior / 'build.py').read_text().replace(str(previous), str(build))
    (target / 'build.py').write_text(builder)
    return target


def apply_remesh():
    draft = N6 / 'drafts/remesh'
    manifest = cut.read(draft / 'manifest.json')
    assert cut.read(draft / 'readability.json')['passed']
    assert (draft / 'syntax_attempt1.exit').read_text().strip() == '0'
    for name, pins in manifest.items():
        source, proposed = cut.REPO / name, draft / Path(name).name
        assert (cut.sha(source) if source.is_file() else None) == pins['base']
        assert cut.sha(proposed) == pins['candidate']
    for name in manifest:
        shutil.copyfile(draft / Path(name).name, cut.REPO / name)
    return list(manifest)


def commit(label, build, workspace, changed, extra):
    summary = cut.read(workspace / 'acceptance_summary.json')
    assert summary['state'] == 'PASSED' and summary['build'] == build
    assert [row['records'] for row in summary['G1'].values()] == [375, 114, 228]
    assert all(summary[name]['passed'] for name in ('G2', 'G3', 'GQ'))
    g3 = summary['G3']
    progress(f"{label} build{build} passed G1 {375}/{114}/{228}, G2 {summary['G2']['work']:,}, "
             f"G3 {g3['accepted_splits']} splits with {g3['common_edit_count']} exact shared edits, "
             f"and GQ at {summary['GQ']['seconds_until_stop']:.3f} s.")
    files = [Path(path) for path in cut.read(workspace / 'checkpoint_paths.json')]
    files += [cut.REPO / path for path in changed]
    files += [workspace / 'checkpoint_paths.json', workspace / 'run_g2_gq_prefix.py',
              N6.parent / 'progress.md', N6.parent / 'QUESTIONS_FOR_REVIEWER.md']
    files += extra
    subprocess.run(['git', 'diff', '--cached', '--quiet'], check=True)
    subprocess.run(['git', 'add', '-f', '--', *map(str, files)], check=True)
    subprocess.run(['git', 'diff', '--cached', '--check', '--', *changed], check=True)
    subprocess.run(['git', 'commit', '-qm', f'Extract native 3D {label} with identical results'], check=True)
    subprocess.run(['git', 'push', 'origin', 'HEAD:refs/heads/codex/native-3d-bl'], check=True)
    head = subprocess.check_output(['git', 'rev-parse', 'HEAD'], text=True).strip()
    cut.save(N6 / f'{label}_checkpoint.json', {'commit': head, 'build': build, 'passed': True})
    progress(f"{label} checkpoint pushed as `{head[:10]}`.")


def qualify(label, previous, build, step, changed, extra):
    name = f'N6{label}'
    target = bind_build(label, previous, build, changed)
    workspace = bind_identity(name, build)
    cut.qualify(step, build, workspace, target)
    check_build(target)
    seal(name, step, build, workspace, target)
    commit(label, build, workspace, changed, extra)


def remesh():
    failure = cut.read(N6.parent / 'N16/C_failure/revert_checkpoint.json')
    assert failure['C_failed'] and failure['revert'] == subprocess.check_output(
        ['git', 'rev-parse', 'HEAD'], text=True).strip()
    assert cut.terminal(cut.read(N6.parent / 'N16/C_campaign_launch.json'))
    assert cut.read(cut.PHASE / 'round_8_5r_B/G5/acceptance_summary.json')['passed']
    preflight = N6 / 'Remesh_before_edit_preflight.json'
    subprocess.run([cut.PYTHON, str(cut.PREFLIGHT), str(preflight)], check=True)
    changed = apply_remesh()
    progress('Applied the reviewed Remesh extraction. The public entry is 50 formatted lines. Starting build576.')
    directory = N6 / 'drafts/remesh'
    extra = [N6 / name for name in ('source_check.md', 'draft_remesh.py', 'run_cut.py',
                                   'Remesh_campaign_launch.json', 'Remesh_campaign.log')]
    extra += [preflight, N6.parent / 'N16/C_failure/revert_checkpoint.json', N6 / 'drafts/RemeshReporting.inc']
    extra += [directory / name for name in ('manifest.json', 'readability.json', 'syntax_attempt1.exit',
                                             'check_syntax.py')]
    qualify('Remesh', 574, 576, 10, changed, extra)


if __name__ == '__main__':
    assert Path('/proc/1/comm').read_text().strip() == 'systemd'
    {'Remesh': remesh}[sys.argv[1]]()
