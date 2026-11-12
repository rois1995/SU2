"""Small shared helpers for the supplemental, worker-authored evaluation recipe."""
import hashlib
import json
import os
from pathlib import Path
import subprocess
import tarfile
import time

KIT = Path(__file__).resolve().parent
FROZEN_SHA = '30369d2f0eb53ae63227d8a4523bcba6949d692077b7ee072673c49f0542b480'
LEAN_SHA = '6fe3ce97a58f44e2b3567d455b994eacec5bfe9ae7774f2a573444480ba813fe'
COMMIT = 'e0d3ee8d72598767227e62a9be5834a4554532b6'
PARENT = '371fcae6f2d408f881f9db130f9feedd6a791e9d'
TREE = '86084d611eaebd65c0d9216cf838eac56525e732'
ARCHIVE = 'SU2_Lean_phase1_20261112.tar.gz'
RELEASE = 'lean-4.19.0-linux.tar.zst'
WIDGET_ASSETS = 'proofwidgets-v0.0.57-js.tar.gz'
WIDGET_RELEASE_SHA = 'fc6fa33a1272b7e2ec38a2b6427ea7645557e79a9338828ace91bf7403449f8e'
WALL = 'SU2_CFD/include/solvers/CFVMFlowSolverBase.inl'


def sha(path):
    with Path(path).open('rb') as f:
        return hashlib.file_digest(f, 'sha256').hexdigest()


def require(condition, message):
    if not condition:
        raise ValueError(message)


def git(repo, *args):
    return subprocess.check_output(['git', '-C', str(repo), *args], text=True).strip()


def limits():
    # An inherited Lake/Lean search path must not supply outside compiled objects.
    for name in list(os.environ):
        if name in {'LEAN', 'LAKE', 'ELAN', 'LD_LIBRARY_PATH', 'LD_PRELOAD'} or name.startswith(
                ('LEAN_', 'LAKE_', 'ELAN_', 'GIT_')):
            os.environ.pop(name)
    os.environ.update(LEAN_NUM_THREADS='2', OMP_NUM_THREADS='1',
                      MATHLIB_NO_CACHE_ON_UPDATE='1', LAKE_NO_CACHE='1', GIT_ALLOW_PROTOCOL='file',
                      GIT_CONFIG_NOSYSTEM='1', GIT_CONFIG_GLOBAL='/dev/null')
    if hasattr(os, 'sched_getaffinity'):
        os.sched_setaffinity(0, sorted(os.sched_getaffinity(0))[:2])


def before_work(allow_busy=False):
    subprocess.run(['uptime'], check=True)
    while not allow_busy and os.getloadavg()[0] >= 6:
        print('Waiting for one-minute load below 6.', flush=True)
        time.sleep(15)


def run(args, cwd, log, timeout=7200, expected=0):
    args = list(map(str, args))
    with Path(log).open('a') as f:
        f.write('$ ' + ' '.join(args) + '\n')
        f.flush()
        result = subprocess.run(args, cwd=cwd, stdout=f, stderr=subprocess.STDOUT, timeout=timeout)
        f.write(f'EXIT {result.returncode}\n')
    require(result.returncode == expected, f'Exit {result.returncode}; expected {expected}; see {log}')


def unpack(archive, dest):
    dest.mkdir(parents=True, exist_ok=True)
    if str(archive).endswith('.zst'):
        with subprocess.Popen(['zstd', '-dc', str(archive)], stdout=subprocess.PIPE) as proc:
            with tarfile.open(fileobj=proc.stdout, mode='r|') as tar:
                tar.extractall(dest, filter='data')
            require(proc.wait() == 0, 'zstd extraction failed')
    else:
        with tarfile.open(archive) as tar:
            tar.extractall(dest, filter='data')


def payload_manifest(payload):
    require((payload / 'manifest.json').read_bytes() == (KIT / 'payload-manifest.json').read_bytes(),
            'Payload manifest differs from the reviewed kit manifest')
    manifest = json.loads((payload / 'manifest.json').read_text())
    for name, digest in manifest['files'].items():
        require(Path(name).name == name, 'Unexpected asset path')
        require(sha(payload / name) == digest, f'Asset digest mismatch: {name}')
    require(sha(payload / ARCHIVE) == FROZEN_SHA, 'Frozen release changed')
    require(sha(payload / RELEASE) == LEAN_SHA, 'Lean release changed')
    return manifest


def restore_repo(payload, record, dest, scratch):
    unpack(payload / (record['name'] + '.git.tar.gz'), scratch)
    bare = scratch / (record['name'] + '.git')
    require(git(bare, 'rev-parse', 'HEAD') == record['commit'], 'Wrong snapshot HEAD')
    require(git(bare, 'rev-parse', 'HEAD^{tree}') == record['tree'], 'Wrong snapshot tree')
    git(bare, 'fsck', '--full', '--no-reflogs')
    subprocess.run(['git', 'clone', '--quiet', '--no-hardlinks', str(bare), str(dest)], check=True)
    git(dest, 'remote', 'set-url', 'origin', record['url'])
    require(git(dest, 'status', '--porcelain') == '', 'Source checkout is dirty')


def source_provenance(repo, project):
    require(git(repo, 'rev-parse', COMMIT + '^{tree}') == TREE, 'SU2 tree mismatch')
    require(git(repo, 'rev-parse', COMMIT + '^') == PARENT, 'SU2 parent mismatch')
    manifest = json.loads((project / 'source/manifest.json').read_text())
    for item in manifest['files']:
        require(git(repo, 'rev-parse', COMMIT + ':' + item['path']) == item['git_blob'],
                'Source tree membership mismatch: ' + item['path'])
        data = subprocess.check_output(['git', '-C', str(repo), 'show', COMMIT + ':' + item['path']])
        require(hashlib.sha256(data).hexdigest() == item['sha256'], 'Source blob mismatch')
    before = subprocess.check_output(['git', '-C', str(repo), 'show', PARENT + ':' + WALL])
    require(hashlib.sha256(before).hexdigest() == manifest['before_sha256'], 'Parent source mismatch')
    patch = subprocess.check_output(['git', '-C', str(repo), 'diff', '--no-ext-diff', '--no-textconv', '--abbrev=10', PARENT, COMMIT])
    require(patch == (project / 'source/accepted-moving-wall.patch').read_bytes(), 'Repair patch mismatch')
