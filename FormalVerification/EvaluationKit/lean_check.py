#!/usr/bin/env python3
"""Cold rebuild from offline source; an evaluator must run and assess this outside worker control."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys
from common import (ARCHIVE, FROZEN_SHA, RELEASE, WIDGET_ASSETS, before_work, git, limits, payload_manifest,
                    require, restore_repo, run, sha, source_provenance, unpack)


def frozen_checks(project, lock, log):
    require(lock['archive_sha256'] == FROZEN_SHA, 'Evaluator lock names a different archive')
    handoff = json.loads((project / 'handoff.json').read_text())
    for items in [handoff['protected_files'], lock['locked_specification']]:
        for name, digest in items.items():
            require((project / name).resolve().is_relative_to(project), 'Invalid locked path')
            require(sha(project / name) == digest, 'Locked file mismatch: ' + name)
    run(['sha256sum', '-c', 'SHA256SUMS'], project, log)


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('payload', type=Path)
    p.add_argument('output', type=Path, help='Must not exist; retains logs and freshly built dependencies')
    p.add_argument('--lock', type=Path, required=True, help='External evaluator-owned JSON, read only')
    p.add_argument('--allow-busy-two-threads', action='store_true', help='Only with machine-owner authorization')
    args = p.parse_args()
    limits()
    payload, out = args.payload.resolve(), args.output.resolve()
    manifest = payload_manifest(payload)
    lock_bytes = args.lock.read_bytes()
    lock = json.loads(lock_bytes)
    require(lock['archive_sha256'] == FROZEN_SHA, 'Evaluator archive pin does not match')
    out.mkdir(parents=True, exist_ok=False)
    unpack(payload / ARCHIVE, out)
    project = out / 'SU2_Lean'
    log = out / 'identity.log'
    frozen_checks(project, lock, log)
    unpack(payload / RELEASE, out)
    toolchain = out / 'lean-4.19.0-linux'
    os.environ['PATH'] = str(toolchain / 'bin') + os.pathsep + os.environ['PATH']
    fingerprints = json.loads((project / 'environment.json').read_text())['binary_fingerprints']
    for name, digest in fingerprints.items():
        require(sha(toolchain / name) == digest, 'Compiler fingerprint mismatch: ' + name)
    version = subprocess.check_output(['lean', '--version'], text=True).strip()
    require('version 4.19.0,' in version and '6caaee842e94' in version, 'Wrong Lean compiler')
    records = {r['name']: r for r in manifest['repositories']}
    packages = json.loads((project / 'lake-manifest.json').read_text())['packages']
    for pkg in packages:
        record = records[pkg['name']]
        require(record['commit'] == pkg['rev'] and record['url'] == pkg['url'], 'Wrong dependency pin')
        dest = project / '.lake/packages' / pkg['name']
        restore_repo(payload, record, dest, out / 'repositories')
        require(not any(dest.rglob('*.olean')), 'Precompiled Lean dependency was supplied')
        require(not (dest / '.lake/build').exists(), 'Dependency build cache was supplied')
    display_build = project / '.lake/packages/proofwidgets/.lake/build'
    unpack(payload / WIDGET_ASSETS, display_build)
    supplied = sorted(str(f.relative_to(display_build)) for f in display_build.rglob('*') if f.is_file())
    require(supplied == manifest['proofwidgets_display_assets']['selected_members'], 'Unexpected display asset files')
    require(all(n.endswith('.js') or n == 'js/lake.trace' for n in supplied), 'Non-display cache supplied')
    restore_repo(payload, records['SU2'], out / 'SU2-source', out / 'repositories')
    source_provenance(out / 'SU2-source', project)
    before_work(args.allow_busy_two_threads)
    print('Starting cold Lean dependency and project rebuild; see cold-build.log', flush=True)
    run(['lake', 'build'], project, out / 'cold-build.log')
    before_work(args.allow_busy_two_threads)
    run(['lake', 'env', 'lean', '-DwarningAsError=true', 'Integrity.lean'], project, out / 'axioms.log')
    require('Audited 231 SU2 logical declarations (113 theorem declarations)' in
            (out / 'axioms.log').read_text(), 'Audit inventory changed')
    # Export explicit theorem types and standard Lean axiom reports for evaluator inspection.
    names = []
    for f in sorted((project / 'SU2').glob('*.lean')):
        text = f.read_text()
        require(not re.search(r'\b(sorry|admit|axiom|native_decide|unsafe)\b', text), 'Forbidden source token')
        namespaces = re.findall(r'^namespace (\S+)', text, re.M)
        require(len(namespaces) == 1, 'Review namespace inventory')
        names.extend(namespaces[0] + '.' + name for name in re.findall(r'\btheorem\s+(\w+)', text))
    require(len(names) == len(set(names)) == 52, 'Explicit theorem inventory changed')
    export = out / 'TheoremTypes.lean'
    export.write_text('import SU2\nset_option pp.universes true\n' +
                      ''.join('#check ' + n + '\n#print axioms ' + n + '\n' for n in names))
    before_work(args.allow_busy_two_threads)
    run(['lake', 'env', 'lean', '-DwarningAsError=true', export], project, out / 'theorem-types.log')
    probe = out / 'Rejected.lean'
    probe.write_text((project / 'Integrity.lean').read_text().replace(
        'open Lean Elab Command in', 'axiom SU2.RejectedAuditProbe : False\n\nopen Lean Elab Command in'))
    before_work(args.allow_busy_two_threads)
    run(['lake', 'env', 'lean', probe], project, out / 'negative-audit.log', expected=1)
    require('Unapproved axiom SU2.RejectedAuditProbe' in (out / 'negative-audit.log').read_text(),
            'Audit failed to reject injected axiom')
    before_work()
    run([sys.executable, 'scripts/source_experiment.py'], project, out / 'source-slice.log')
    frozen_checks(project, lock, log)
    for pkg in packages:
        require(git(project / '.lake/packages' / pkg['name'], 'diff', '--name-only', 'HEAD') == '',
                'Dependency source changed during build')
    result = {'status': 'recipe checks passed; no independent acceptance decision made by this script',
              'lean': version, 'explicit_theorems': names, 'logical_declarations': 231,
              'cold_dependency_build': True, 'accepted_axioms': ['propext', 'Classical.choice', 'Quot.sound'],
              'external_lock_sha256': hashlib.sha256(lock_bytes).hexdigest(),
              'negative_fixture': 'rejected; outside accepted project',
              'next': 'Evaluator must inspect specifications, types, assumptions and transitive dependencies independently.'}
    (out / 'result.json').write_text(json.dumps(result, indent=2) + '\n')
    print('Cold rebuild and local checks passed; independent acceptance remains external.', flush=True)


if __name__ == '__main__':
    main()
