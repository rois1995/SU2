#!/usr/bin/env python3
"""Package the frozen Git identity and source-only offline dependencies."""
import argparse
import io
import json
from pathlib import Path
import shutil
import subprocess
import sys
import tarfile
import tempfile

from source import ROOT, COMMIT
sys.path.insert(0, str(ROOT.parent / 'EvaluationKit'))
from common import git, limits, require, sha
from check import protected, PRIOR

REPO = ROOT.parents[1]
KIT_SHA = '4fb743a739308b0b40b02255c2f636a681d311e039d339ab7d1abe04f740ed78'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('historical_kit', type=Path)
    parser.add_argument('output', type=Path, help='New unpacked handoff directory')
    parser.add_argument('--verification-results', type=Path, required=True, help='Successful frozen check output')
    args = parser.parse_args()
    limits()
    lock = protected()
    require(sha(args.historical_kit) == KIT_SHA, 'Historical offline kit changed')
    validation = args.verification_results.resolve()
    result = json.loads((validation / 'result.json').read_text())
    require(result['candidate_lock_sha256'] == sha(ROOT / 'candidate.json'), 'Validation has a different identity')
    require(result['status'].startswith('PASS worker checks'), 'Frozen check did not pass')
    require(not any(validation.rglob('*.olean')), 'Validation must contain logs/source, not compiled proofs')
    head = git(REPO, 'rev-parse', 'HEAD')
    require(git(REPO, 'rev-parse', head + '^') == PRIOR, 'Unexpected candidate parent')
    require(git(REPO, 'rev-parse', PRIOR + '^') == '73a322b640f45505e8d8fb70fbce46e6ccffd206', 'Frozen boundary parent changed')
    require(git(REPO, 'rev-parse', PRIOR + '^^') == '15ee28adf032a866e292926502a5f022e38199c8', 'Frozen Euler parent changed')
    require(git(REPO, 'rev-parse', PRIOR + '^^^') == COMMIT, 'Pinned baseline chain changed')
    changed = git(REPO, 'diff', '--name-only', PRIOR, head).splitlines()
    require(changed and all(p.startswith('FormalVerification/BoundaryMemory/') for p in changed), 'Changes outside new candidate')
    require(not git(REPO, 'status', '--porcelain', '--untracked-files=no'), 'Tracked working source dirty')
    out = args.output.resolve()
    out.mkdir(parents=True, exist_ok=False)
    observed = json.loads((ROOT / 'baseline.json').read_text())['observed_upstream_develop']
    git(REPO, 'merge-base', '--is-ancestor', COMMIT, observed)
    revisions = sorted(set(git(REPO, 'rev-list', COMMIT + '..' + head).splitlines() +
                           git(REPO, 'rev-list', COMMIT + '..' + observed).splitlines() + [COMMIT]))
    objects = git(REPO, 'rev-list', '--objects', '--no-walk', *revisions)
    ids = '\n'.join(sorted({line.split()[0] for line in objects.splitlines()})) + '\n'
    with tempfile.TemporaryDirectory(prefix='boundary-memory-snapshot-') as tmp:
        bare = Path(tmp) / 'SU2-boundary-memory.git'
        subprocess.run(['git', 'init', '--quiet', '--bare', '--template=', bare], check=True)
        with subprocess.Popen(['git', '-C', REPO, 'pack-objects', '--threads=2', '--stdout'],
                              stdin=subprocess.PIPE, stdout=subprocess.PIPE) as proc:
            proc.stdin.write(ids.encode()); proc.stdin.close()
            subprocess.run(['git', '-C', bare, 'index-pack', '--stdin'], stdin=proc.stdout, stdout=subprocess.DEVNULL, check=True)
            require(proc.wait() == 0, 'Git object packing failed')
        (bare / 'shallow').write_text(COMMIT + '\n')
        git(bare, 'update-ref', 'refs/heads/snapshot', head)
        git(bare, 'update-ref', 'refs/heads/observed-upstream', observed)
        git(bare, 'symbolic-ref', 'HEAD', 'refs/heads/snapshot')
        git(bare, 'fsck', '--full', '--no-reflogs')
        with tarfile.open(out / 'SU2-boundary-memory.git.tar.gz', 'w:gz') as archive:
            archive.add(bare, arcname=bare.name)
    raw = subprocess.check_output(['git', '-C', REPO, 'archive', head, 'FormalVerification'])
    with tarfile.open(fileobj=io.BytesIO(raw)) as archive:
        archive.extractall(out / 'review', filter='data')
    shutil.copyfile(args.historical_kit, out / 'SU2_Lean_evaluator_kit_20261112.tar.zst')
    shutil.copytree(validation, out / 'worker-validation')
    manifest = {'status': 'Frozen worker handoff; new independent acceptance pending',
                'candidate_commit': head, 'candidate_tree': git(REPO, 'rev-parse', head + '^{tree}'),
                'candidate_lock_sha256': sha(ROOT / 'candidate.json'),
                'investigation_commit': PRIOR, 'preserved_boundary_commit': '73a322b640f45505e8d8fb70fbce46e6ccffd206',
                'preserved_boundary_lock_sha256': lock['preserved_boundary_lock_sha256'],
                'observed_upstream_develop': observed,
                'upstream_commit': COMMIT, 'production_patches': [],
                'fork_only': 'https://github.com/rois1995/SU2.git',
                'files': {str(p.relative_to(out)): sha(p) for p in sorted(out.rglob('*')) if p.is_file()}}
    (out / 'manifest.json').write_text(json.dumps(manifest, indent=2) + '\n')
    files = manifest['files'] | {'manifest.json': sha(out / 'manifest.json')}
    (out / 'SHA256SUMS').write_text(''.join(h + '  ' + p + '\n' for p, h in sorted(files.items())))
    print('Frozen source-only handoff prepared:', out)


if __name__ == '__main__':
    main()
