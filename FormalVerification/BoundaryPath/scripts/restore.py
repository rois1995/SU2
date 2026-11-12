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
    unpack(bundle / 'SU2-boundary.git.tar.gz', out / 'repositories')
    bare = out / 'repositories/SU2-boundary.git'
    require(git(bare, 'rev-parse', 'HEAD') == manifest['candidate_commit'], 'Wrong candidate commit')
    git(bare, 'fsck', '--full', '--no-reflogs')
    repo = out / 'SU2'
    subprocess.run(['git', 'clone', '--quiet', '--no-hardlinks', str(bare), str(repo)], check=True)
    require(sha(repo / 'FormalVerification/BoundaryPath/candidate.json') == args.expected_candidate_sha256, 'Candidate lock changed')
    require(sha(repo / 'FormalVerification/EulerPath/candidate.json') ==
            '0a5781dad650c8de5bf24ef57804c6a422dbef3aa2be3f373b7ebc21371ccac6', 'Pending EulerPath changed')
    unpack(bundle / 'SU2_Lean_evaluator_kit_20261112.tar.zst', out / 'historical')
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
        for name in ['SourceGate', 'EulerPath', 'BoundaryPath']:
            target = repo / 'FormalVerification' / name / '.lake/packages' / package['name']
            target.parent.mkdir(parents=True, exist_ok=True)
            target.symlink_to(dest, target_is_directory=True)
    display = foundation / '.lake/packages/proofwidgets/.lake/build'
    unpack(payload / WIDGET_ASSETS, display)
    require(sorted(str(p.relative_to(display)) for p in display.rglob('*') if p.is_file()) ==
            previous['proofwidgets_display_assets']['selected_members'], 'Unexpected display artifact')
    for name in ['meson', 'ninja', 'eigen', 'mel']:
        require(git(repo, 'rev-parse', manifest['upstream_commit'] + ':externals/' + name) == records[name]['commit'],
                'Native dependency pin changed')
        restore_repo(payload, records[name], out / 'externals' / name, out / 'repositories/native')
    q = lambda x: shlex.quote(str(x))
    project = repo / 'FormalVerification/BoundaryPath'
    commands = ['export PATH=' + q(toolchain / 'bin') + ':"$PATH"', 'cd ' + q(project),
        'python3 scripts/check.py ' + q(out / 'lean-results'),
        'python3 ../EulerPath/scripts/build_baseline.py ' + ' '.join(map(q,[repo,out / 'externals',out / 'native-build'])),
        'python3 scripts/native.py ' + q(out / 'native-build') + ' ' + q(out / 'boundary-native'),
        'python3 ../EulerPath/scripts/native.py ' + q(out / 'preserved-native'),
        'python3 ../EulerPath/scripts/integration.py ' + q(out / 'native-build/SU2/build/SU2_CFD/src/SU2_CFD') + ' ' + q(out / 'integration')]
    (out / 'commands.txt').write_text('\n'.join(commands) + '\n')
    (out / 'restore-result.json').write_text(json.dumps({'status': 'PASS source-only restoration, no proof acceptance',
        'candidate_commit': manifest['candidate_commit'], 'candidate_lock_sha256': args.expected_candidate_sha256,
        'pending_euler_identity_preserved': True, 'supplied_project_or_dependency_olean_files': 0}, indent=2) + '\n')
    print('Source-only inputs restored. Execute commands sequentially; use the busy-host Lean override only with host-owner permission.\n' + '\n'.join(commands))


if __name__ == '__main__':
    main()
