#!/usr/bin/env python3
"""Offline source-only restoration; does not decide independent acceptance."""
import argparse
import json
from pathlib import Path
import shlex
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT.parent / 'EvaluationKit'))
from common import RELEASE, WIDGET_ASSETS, git, limits, payload_manifest, require, restore_repo, sha, unpack


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('handoff', type=Path)
    parser.add_argument('output', type=Path, help='Fresh directory')
    parser.add_argument('--historical-kit', type=Path, required=True)
    parser.add_argument('--cfdlean-source', type=Path, required=True, help='Authorized local Git repository containing accepted M1 and M2b')
    parser.add_argument('--expected-candidate-sha256', required=True, help='Reviewer-selected identity')
    args = parser.parse_args()
    limits()
    bundle = args.handoff.resolve()
    manifest = json.loads((bundle / 'manifest.json').read_text())
    for rel, digest in manifest['files'].items():
        path = bundle / rel
        require(path.resolve().is_relative_to(bundle), 'Invalid bundle path')
        require(sha(path) == digest, 'Handoff file changed: ' + rel)
    require(manifest['candidate_lock_sha256'] == args.expected_candidate_sha256, 'Wrong evaluation identity')
    out = args.output.resolve()
    out.mkdir(parents=True, exist_ok=False)
    unpack(bundle / 'SU2-interior.git.tar.gz', out / 'repositories')
    bare = out / 'repositories/SU2-interior.git'
    require(git(bare, 'rev-parse', 'HEAD') == manifest['candidate_commit'], 'Wrong candidate commit')
    git(bare, 'fsck', '--full', '--no-reflogs')
    repo = out / 'SU2'
    subprocess.run(['git', 'clone', '--quiet', '--no-hardlinks', str(bare), str(repo)], check=True)
    require(sha(repo / 'FormalVerification/InteriorPath/candidate.json') == args.expected_candidate_sha256, 'Candidate lock changed')
    require(sha(repo / 'FormalVerification/BoundaryPath/candidate.json') ==
            '7c6a795c2dc8ef748bd4cdd9cdba3dc31330e55decb4b504b947d592654ef37e', 'Frozen BoundaryPath changed')
    require(sha(repo / 'FormalVerification/EulerPath/candidate.json') ==
            '0a5781dad650c8de5bf24ef57804c6a422dbef3aa2be3f373b7ebc21371ccac6', 'Pending EulerPath changed')
    require(sha(args.historical_kit) == '4fb743a739308b0b40b02255c2f636a681d311e039d339ab7d1abe04f740ed78', 'Wrong reusable offline kit')
    unpack(args.historical_kit, out / 'historical')
    payload = out / 'historical/SU2_Lean_evaluator_kit/payload'
    previous = payload_manifest(payload)
    records = {r['name']: r for r in previous['repositories']}
    unpack(payload / RELEASE, out / 'toolchain')
    toolchain = out / 'toolchain/lean-4.19.0-linux'
    env = json.loads((repo / 'FormalVerification/Lean/environment.json').read_text())
    for rel, digest in env['binary_fingerprints'].items():
        require(sha(toolchain / rel) == digest, 'Compiler binary differs')
    foundation = repo / 'FormalVerification/Lean'
    for package in json.loads((foundation / 'lake-manifest.json').read_text())['packages']:
        record = records[package['name']]
        require(record['commit'] == package['rev'], 'Dependency pin changed')
        dest = foundation / '.lake/packages' / package['name']
        restore_repo(payload, record, dest, out / 'repositories/dependencies')
        require(not any(dest.rglob('*.olean')), 'Compiled Lean artifact supplied')
        require(not (dest / '.lake/build').exists(), 'Dependency build cache supplied')
        for name in ['SourceGate', 'EulerPath', 'BoundaryPath', 'BoundaryMemory', 'Initialization', 'Preprocessing', 'InteriorPath']:
            target = repo / 'FormalVerification' / name / '.lake/packages' / package['name']
            target.parent.mkdir(parents=True, exist_ok=True)
            target.symlink_to(dest, target_is_directory=True)
    dependency = repo / 'FormalVerification/Initialization/.lake/packages/cfdlean'
    subprocess.run(['git','clone','--quiet','--no-hardlinks','--no-checkout',str(args.cfdlean_source.resolve()),str(dependency)],check=True)
    git(dependency,'checkout','--quiet','--detach','0a88e36b3810cd5adc3881db526ca8a35f81afbb')
    git(dependency,'remote','set-url','origin','https://github.com/rois1995/CFD_Lean.git')
    require(not any(dependency.rglob('*.olean')), 'Compiled private dependency supplied')
    for package in json.loads((foundation/'lake-manifest.json').read_text())['packages']:
        target = dependency/'.lake/packages'/package['name']
        target.parent.mkdir(parents=True,exist_ok=True)
        target.symlink_to(foundation/'.lake/packages'/package['name'],target_is_directory=True)
    display = foundation / '.lake/packages/proofwidgets/.lake/build'
    unpack(payload / WIDGET_ASSETS, display)
    require(sorted(str(p.relative_to(display)) for p in display.rglob('*') if p.is_file()) ==
            previous['proofwidgets_display_assets']['selected_members'], 'Unexpected display artifact')
    for name in ['meson', 'ninja', 'eigen', 'mel']:
        require(git(repo, 'rev-parse', manifest['upstream_commit'] + ':externals/' + name) == records[name]['commit'],
                'Native dependency pin changed')
        restore_repo(payload, records[name], out / 'externals' / name, out / 'repositories/native')
    target = repo / 'FormalVerification/Preprocessing/.lake/packages/cfdlean'
    target.symlink_to(dependency,target_is_directory=True)
    # Parent candidates retain their separate accepted M1 environment.
    independent = repo/'FormalVerification/InteriorPath/.lake/packages/cfdlean'
    subprocess.run(['git','clone','--quiet','--no-hardlinks','--no-checkout',str(args.cfdlean_source.resolve()),str(independent)],check=True)
    git(independent,'checkout','--quiet','--detach','c92c0f66360f85c0a3339ab9b30fc00c39ee5f93')
    git(independent,'remote','set-url','origin','https://github.com/rois1995/CFD_Lean.git')
    require(not any(independent.rglob('*.olean')), 'Compiled independent specification supplied')
    for package in json.loads((foundation/'lake-manifest.json').read_text())['packages']:
        target = independent/'.lake/packages'/package['name']
        target.parent.mkdir(parents=True,exist_ok=True)
        target.symlink_to(foundation/'.lake/packages'/package['name'],target_is_directory=True)
    require(sha(repo/'FormalVerification/Preprocessing/candidate.json')=='712f4346563815686a6d5537b6cef68daf9947768506f8f386b959608685e29e','Frozen preprocessing changed')
    q = lambda x: shlex.quote(str(x))
    project = repo / 'FormalVerification/InteriorPath'
    commands = ['export PATH=' + q(toolchain / 'bin') + ':"$PATH"', 'cd ' + q(project),
        'python3 scripts/check.py ' + q(out / 'lean-results') + ' --preserved-checks',
        'python3 ../EulerPath/scripts/build_baseline.py ' + ' '.join(map(q,[repo,out / 'externals',out / 'native-build'])),
        'python3 ../Preprocessing/scripts/native.py ' + q(out / 'preprocessing-native'),
        'python3 ../Preprocessing/scripts/actual.py ' + q(out / 'native-build') + ' ' + q(out / 'preprocessing-actual'),
        'python3 scripts/native.py ' + q(out / 'interior-controls'),
        'python3 scripts/actual.py ' + q(out / 'native-build') + ' ' + q(out / 'interior-actual'),
        'python3 ../Initialization/scripts/native.py ' + q(out / 'initialization-native'),
        'python3 ../BoundaryMemory/scripts/native.py ' + q(out / 'native-build') + ' ' + q(out / 'memory-native'),
        'python3 ../BoundaryPath/scripts/native.py ' + q(out / 'native-build') + ' ' + q(out / 'boundary-native'),
        'python3 ../EulerPath/scripts/native.py ' + q(out / 'preserved-native'),
        'python3 ../EulerPath/scripts/integration.py ' + q(out / 'native-build/SU2/build/SU2_CFD/src/SU2_CFD') + ' ' + q(out / 'integration')]
    (out / 'commands.txt').write_text('\n'.join(commands) + '\n')
    (out / 'restore-result.json').write_text(json.dumps({'status': 'PASS source-only restoration, no proof acceptance',
        'candidate_commit': manifest['candidate_commit'], 'candidate_lock_sha256': args.expected_candidate_sha256,
        'previous_candidates_unchanged': True, 'literature_approval': 'M1 and M2-J/A/R/L accepted; full Roe and boundary mathematics pending', 'supplied_project_or_dependency_olean_files': 0}, indent=2) + '\n')
    print('Source-only inputs restored. Execute commands sequentially; use the busy-host Lean override only with host-owner permission.\n' + '\n'.join(commands))


if __name__ == '__main__':
    main()
