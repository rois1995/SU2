"""Run N6 checkpoints in order on the actual host."""
from pathlib import Path
import shutil
import subprocess
import sys
import time

from run_cut import N6, cut, progress, qualify


def git(*arguments):
    return subprocess.check_output(['git', *map(str, arguments)], cwd=cut.REPO, text=True).strip()


def launch(label, *arguments):
    with (N6 / f'{label}_campaign.log').open('x') as log:
        child = subprocess.Popen([cut.PYTHON, *map(str, arguments)], cwd=cut.REPO, stdout=log,
                                 stderr=subprocess.STDOUT)
        handle = cut.actual_identity(child.pid)
        cut.save(N6 / f'{label}_campaign_launch.json', handle)
        code = child.wait()
    terminal = cut.terminal(handle)
    cut.save(N6 / f'{label}_campaign_wait.json', dict(handle, returncode=code, terminal=terminal))
    assert code == 0 and terminal, (label, code, log.name)


def preflight(prior, build, label):
    assert cut.terminal(cut.read(N6 / f'{prior}_campaign_launch.json'))
    checkpoint = N6 / f'{prior}_checkpoint.json'
    passed = cut.read(checkpoint)
    assert passed['build'] == build and passed['passed'] and git('rev-parse', 'HEAD') == passed['commit']
    assert not git('diff', '--cached', '--name-only')
    pins = cut.read(cut.PHASE / f'build{build}/source_pins.json')
    assert all(cut.sha(path) == digest for path, digest in pins.items())
    receipt = N6 / f'{label}_before_edit_preflight.json'
    subprocess.run([cut.PYTHON, str(cut.PREFLIGHT), str(receipt)], cwd=cut.REPO, check=True)
    return [checkpoint, receipt]


def apply_optimizer(draft):
    manifest = cut.read(draft / 'manifest.json')
    rows = manifest['files']
    for row in rows:
        original = cut.REPO / row['path']
        assert (cut.sha(original) if original.exists() else None) == row['base_sha256']
        assert cut.sha(draft / row['draft']) == row['sha256']
    for row in rows:
        shutil.copyfile(draft / row['draft'], cut.REPO / row['path'])
    return [row['path'] for row in rows]


def apply_selection(draft):
    patch = draft / 'selection.patch'
    subprocess.run(['sha256sum', '-c', 'SHA256SUMS'], cwd=draft, check=True)
    paths = [row.split('\t')[-1] for row in git('apply', '--numstat', patch).splitlines()]
    assert len(paths) == 12 and all(path.startswith('Common/src/adaptation/') for path in paths), paths
    git('apply', '--check', '--whitespace=error', patch)
    git('apply', '--whitespace=error', patch)
    main = cut.REPO / 'Common/src/adaptation/CNativeEngine3D.cpp'
    anchor = b'#include "native_engine3d/MeshOptimizer.cpp"'
    reference = (draft.parent / 'optimizer' / main.name).read_bytes()
    assert main.read_bytes().split(anchor)[1] == reference.split(anchor)[1]
    return paths


def substep(label):
    if label == 'Optimizer':
        prior, previous, build, step = 'Remesh', 576, 577, 10
        evidence = ('manifest.json', 'readability.json', 'render_optimizer.py')
        apply = apply_optimizer
    else:
        assert label == 'Selection'
        prior, previous, build, step = 'Optimizer', 577, 578, 11
        evidence = ('REVIEW.md', 'SHA256SUMS', 'mechanical_checks.json', 'extract_selection.py', 'selection.patch',
                    'EngineSelection.hpp', 'EngineSelectionCensus.cpp', 'EngineSelectionQueues.cpp')
        apply = apply_selection
    draft = N6 / 'drafts' / label.lower()
    assert (draft / 'syntax_attempt1.exit').read_text().strip() == '0'
    extra = preflight(prior, previous, label)
    changed = apply(draft)
    git('diff', '--check', '--', *changed)
    extra += [draft / name for name in (*evidence, 'syntax_attempt1.exit', 'syntax_attempt1.log')]
    extra += [N6 / f'{label}_campaign_launch.json', N6 / 'continue_cut.py', N6 / 'drafts/check_engine_syntax.py']
    progress(f'Applied the reviewed {label} extraction. Starting build{build}.')
    qualify(label, previous, build, step, changed, extra)


def main():
    remesh = cut.read(N6 / 'Remesh_campaign_launch.json')
    while not cut.terminal(remesh):
        time.sleep(15)
    launch('Optimizer', Path(__file__).resolve(), 'Optimizer')
    launch('Selection', Path(__file__).resolve(), 'Selection')
    controller = cut.PHASE / 'phase_N6/run_g5.py'
    launch('G5', controller, 578)
    launch('standing_safeguards', controller.parent / 'run_standing_safeguards.py')


if __name__ == '__main__':
    assert Path('/proc/1/comm').read_text().strip() == 'systemd'
    if len(sys.argv) == 2:
        substep(sys.argv[1])
    else:
        main()
