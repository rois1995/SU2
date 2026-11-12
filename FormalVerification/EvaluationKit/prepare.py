#!/usr/bin/env python3
"""Package exact Git objects, without worker-built dependency artifacts or full histories."""
import argparse
import json
from pathlib import Path
import shutil
import subprocess
import tarfile
import tempfile
from common import ARCHIVE, COMMIT, FROZEN_SHA, LEAN_SHA, PARENT, RELEASE, WIDGET_ASSETS, WIDGET_RELEASE_SHA, git, limits, require, sha


def snapshot(repo, name, commit, url, output, parent=None):
    revisions = [commit] + ([parent] if parent else [])
    objects = git(repo, 'rev-list', '--objects', '--no-walk', *revisions)
    object_ids = '\n'.join(sorted({line.split()[0] for line in objects.splitlines()})) + '\n'
    with tempfile.TemporaryDirectory(prefix='snapshot-') as tmp:
        bare = Path(tmp) / (name + '.git')
        subprocess.run(['git', 'init', '--quiet', '--bare', '--template=', str(bare)], check=True)
        with subprocess.Popen(['git', '-C', str(repo), 'pack-objects', '--threads=2', '--stdout'],
                              stdin=subprocess.PIPE, stdout=subprocess.PIPE) as pack:
            pack.stdin.write(object_ids.encode())
            pack.stdin.close()
            subprocess.run(['git', '-C', str(bare), 'index-pack', '--stdin'], stdin=pack.stdout,
                           stdout=subprocess.DEVNULL, check=True)
            require(pack.wait() == 0, 'Git packing failed')
        (bare / 'shallow').write_text((parent or commit) + '\n')
        git(bare, 'update-ref', 'refs/heads/snapshot', commit)
        git(bare, 'symbolic-ref', 'HEAD', 'refs/heads/snapshot')
        git(bare, 'fsck', '--full', '--no-reflogs')
        with tarfile.open(output / (name + '.git.tar.gz'), 'w:gz') as tar:
            tar.add(bare, arcname=bare.name)
    return {'name': name, 'commit': commit, 'tree': git(repo, 'rev-parse', commit + '^{tree}'),
            'url': url, 'shallow_boundary': parent or commit}


def widget_assets(archive, output):
    require(sha(archive) == WIDGET_RELEASE_SHA, 'Wrong official ProofWidgets release')
    with tarfile.open(archive) as source, tarfile.open(output / WIDGET_ASSETS, 'w:gz') as dest:
        names = []
        for member in source.getmembers():
            parts = Path(member.name).parts
            if len(parts) == 2 and parts[0] == 'js':
                require(member.isfile() and (parts[1].endswith('.js') or parts[1] == 'lake.trace'),
                        'Unexpected display asset')
                member.name = 'js/' + parts[1]
                dest.addfile(member, source.extractfile(member))
                names.append(member.name)
        require(len(names) == 20 and 'js/lake.trace' in names, 'Review display asset inventory')
    return {'url': 'https://github.com/leanprover-community/ProofWidgets4/releases/download/v0.0.57/ProofWidgets4.tar.gz',
            'upstream_archive_sha256': WIDGET_RELEASE_SHA, 'selected_members': sorted(names),
            'selection': 'Only publisher JavaScript and its original build trace; no Lean compiled objects.'}


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--project', type=Path, required=True, help='Frozen project with pinned source dependency clones')
    p.add_argument('--su2', type=Path, required=True, help='Read-only SU2 Git object source')
    p.add_argument('--externals', type=Path, required=True, help='Read-only directory containing eigen, meson, ninja, mel Git repos')
    p.add_argument('--archive', type=Path, required=True)
    p.add_argument('--lean-release', type=Path, required=True)
    p.add_argument('--proofwidgets-release', type=Path, required=True)
    p.add_argument('--output', type=Path, required=True, help='Must not exist')
    args = p.parse_args()
    limits()
    require(sha(args.archive) == FROZEN_SHA, 'Unapproved frozen archive')
    require(sha(args.lean_release) == LEAN_SHA, 'Unexpected official release download')
    args.output.mkdir(parents=True, exist_ok=False)
    shutil.copyfile(args.archive, args.output / ARCHIVE)
    shutil.copyfile(args.lean_release, args.output / RELEASE)
    records = []
    lock = json.loads((args.project / 'lake-manifest.json').read_text())
    for pkg in lock['packages']:
        print('Packing source:', pkg['name'], flush=True)
        records.append(snapshot(args.project / '.lake/packages' / pkg['name'], pkg['name'],
                                pkg['rev'], pkg['url'], args.output))
    records.append(snapshot(args.su2, 'SU2', COMMIT, 'https://github.com/rois1995/SU2.git', args.output, PARENT))
    for name, url in [('eigen', 'https://gitlab.com/libeigen/eigen.git'),
                      ('meson', 'https://github.com/mesonbuild/meson.git'),
                      ('ninja', 'https://github.com/ninja-build/ninja.git'),
                      ('mel', 'https://github.com/pcarruscag/MEL.git')]:
        rev = git(args.su2, 'rev-parse', COMMIT + ':externals/' + name)
        records.append(snapshot(args.externals / name, name, rev, url, args.output))
    widgets = widget_assets(args.proofwidgets_release, args.output)
    manifest = {'proofwidgets_display_assets': widgets, 'format': 1, 'repositories': records,
                'lean_release_url': 'https://github.com/leanprover/lean4/releases/download/v4.19.0/' + RELEASE,
                'lean_publisher_digest': None,
                'provenance': 'Content identity only; shallow source snapshots, not signed attestations.',
                'files': {f.name: sha(f) for f in sorted(args.output.iterdir()) if f.is_file()}}
    (args.output / 'manifest.json').write_text(json.dumps(manifest, indent=2) + '\n')
    print('Payload ready:', args.output, flush=True)


if __name__ == '__main__':
    main()
