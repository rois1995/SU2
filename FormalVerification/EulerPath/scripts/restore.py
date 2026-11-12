#!/usr/bin/env python3
"""Restore a cache-free offline review workspace; does not accept the candidate."""
import argparse
import json
import os
from pathlib import Path
import shlex
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT.parent / 'EvaluationKit'))
from common import RELEASE, WIDGET_ASSETS, git, limits, payload_manifest, require, restore_repo, sha, unpack


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('handoff', type=Path, help='Extracted portable handoff root')
    parser.add_argument('output', type=Path, help='Must not exist')
    parser.add_argument('--expected-candidate-sha256', required=True, help='Identity selected by the reviewer outside this script')
    args = parser.parse_args()
    limits()
    bundle = args.handoff.resolve()
    manifest = json.loads((bundle / 'manifest.json').read_text())
    for name, digest in manifest['files'].items():
        path = bundle / name
        require(path.resolve().is_relative_to(bundle), 'Invalid handoff path')
        require(sha(path) == digest, 'Handoff identity mismatch: ' + name)
    require(manifest['candidate_lock_sha256'] == args.expected_candidate_sha256, 'Unexpected review candidate')
    out = args.output.resolve()
    out.mkdir(parents=True, exist_ok=False)
    unpack(bundle / 'SU2-upstream.git.tar.gz', out / 'repositories')
    bare = out / 'repositories/SU2-upstream.git'
    require(git(bare, 'rev-parse', 'HEAD') == manifest['candidate_commit'], 'Wrong candidate commit')
    git(bare, 'fsck', '--full', '--no-reflogs')
    repo = out / 'SU2'
    subprocess.run(['git', 'clone', '--quiet', '--no-hardlinks', str(bare), str(repo)], check=True)
    require(sha(repo / 'FormalVerification/EulerPath/candidate.json') == args.expected_candidate_sha256,
            'Restored candidate lock changed')
    unpack(bundle / 'SU2_Lean_evaluator_kit_20261112.tar.zst', out / 'historical')
    payload = out / 'historical/SU2_Lean_evaluator_kit/payload'
    previous = payload_manifest(payload)
    records = {record['name']: record for record in previous['repositories']}
    unpack(payload / RELEASE, out / 'toolchain')
    toolchain = out / 'toolchain/lean-4.19.0-linux'
    env = json.loads((repo / 'FormalVerification/Lean/environment.json').read_text())
    for path, digest in env['binary_fingerprints'].items():
        require(sha(toolchain / path) == digest, 'Wrong Lean binary')
    foundation = repo / 'FormalVerification/Lean'
    packages = json.loads((foundation / 'lake-manifest.json').read_text())['packages']
    for package in packages:
        record = records[package['name']]
        require(record['commit'] == package['rev'], 'Dependency pin mismatch')
        dest = foundation / '.lake/packages' / package['name']
        restore_repo(payload, record, dest, out / 'repositories/dependencies')
        require(not any(dest.rglob('*.olean')), 'Compiled Lean dependency supplied')
        require(not (dest / '.lake/build').exists(), 'Dependency build cache supplied')
        for project in ['SourceGate', 'EulerPath']:
            target = repo / 'FormalVerification' / project / '.lake/packages' / package['name']
            target.parent.mkdir(parents=True, exist_ok=True)
            target.symlink_to(dest, target_is_directory=True)
    display = foundation / '.lake/packages/proofwidgets/.lake/build'
    unpack(payload / WIDGET_ASSETS, display)
    require(sorted(str(p.relative_to(display)) for p in display.rglob('*') if p.is_file()) ==
            previous['proofwidgets_display_assets']['selected_members'], 'Unexpected ProofWidgets display assets')
    for name in ['meson', 'ninja', 'eigen', 'mel']:
        require(git(repo, 'rev-parse', manifest['upstream_commit'] + ':externals/' + name) == records[name]['commit'],
                'Upstream native dependency pin changed')
        restore_repo(payload, records[name], out / 'externals' / name, out / 'repositories/native')
    commands = [
        'export PATH=' + shlex.quote(str(toolchain / 'bin')) + ':"$PATH"',
        'cd ' + shlex.quote(str(repo / 'FormalVerification/EulerPath')),
        'python3 scripts/check.py ' + shlex.quote(str(out / 'lean-results')),
        'python3 scripts/build_baseline.py ' + ' '.join(shlex.quote(str(p)) for p in [repo, out / 'externals', out / 'native-build']),
        'python3 scripts/integration.py ' + ' '.join(shlex.quote(str(p)) for p in [out / 'native-build/SU2/build/SU2_CFD/src/SU2_CFD', out / 'integration-results']),
        'python3 scripts/native.py ' + shlex.quote(str(out / 'native-results')),
    ]
    (out / 'commands.txt').write_text('\n'.join(commands) + '\n')
    print('Restored source-only review inputs. Run these commands sequentially; add --allow-busy-two-threads to check.py only with host-owner approval.\n' + '\n'.join(commands))


if __name__ == '__main__':
    main()
