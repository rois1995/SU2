#!/usr/bin/env python3
"""Reproduce the frozen worker checks; independent acceptance is external."""
import argparse
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys

from source import ROOT, generate
sys.path.insert(0, str(ROOT.parent / 'EvaluationKit'))
from common import before_work, git, limits, require, run, sha

PRIOR = '5a6d3a578a4a348b91ef6ce8dcd259e142f60d37'
PRIOR_LOCK = '7c6a795c2dc8ef748bd4cdd9cdba3dc31330e55decb4b504b947d592654ef37e'


def protected():
    lock = json.loads((ROOT / 'candidate.json').read_text())
    require(sha(ROOT.parent / 'BoundaryPath/candidate.json') == PRIOR_LOCK, 'Frozen BoundaryPath identity changed')
    for rel, digest in lock['historical_files'].items():
        require(sha(ROOT.parent / rel) == digest, 'Preserved file changed: ' + rel)
    for rel, digest in lock['protected_files'].items():
        require(sha(ROOT / rel) == digest, 'Candidate changed: ' + rel)
    return lock


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path, help='Fresh directory for complete verification logs')
    parser.add_argument('--allow-busy-two-threads', action='store_true', help='Lean only; host-owner authorization required')
    args = parser.parse_args()
    limits()
    lock = protected()
    repo = ROOT.parents[1]
    baseline = json.loads((ROOT / 'baseline.json').read_text())
    pinned = baseline['selected_source_baseline']
    observed = baseline['observed_upstream_develop']
    require(git(repo, 'diff', '--name-only', pinned, observed).splitlines() == baseline['changed_files'], 'Upstream gate diff changed')
    require(not git(repo, 'diff', '--name-only', pinned, observed, '--', '.', ':(exclude).github'), 'Numerical source changed')
    for folder, pins in baseline['numerical_tree_ids'].items():
        for revision, expected in pins.items():
            require(git(repo, 'rev-parse', revision + ':' + folder) == expected, 'Baseline subtree changed')
    out = args.output.resolve()
    out.mkdir(parents=True, exist_ok=False)
    before_work(args.allow_busy_two_threads)
    previous = [sys.executable, ROOT.parent / 'BoundaryPath/scripts/check.py', out / 'preserved']
    if args.allow_busy_two_threads: previous.append('--allow-busy-two-threads')
    run(previous, ROOT, out / 'preserved-check.log', timeout=14400)
    accepted = {p['name']: p for p in json.loads((ROOT.parent / 'Lean/lake-manifest.json').read_text())['packages']}
    for package in json.loads((ROOT / 'lake-manifest.json').read_text())['packages']:
        if package['type'] == 'path':
            require(package['name'] in {'su2lean', 'su2sourcegate', 'su2eulerpath', 'su2boundarypath'}, 'Unknown path dependency')
            continue
        require(package['rev'] == accepted[package['name']]['rev'], 'Changed mathematical dependency')
        checkout = ROOT / '.lake/packages' / package['name']
        require(git(checkout, 'rev-parse', 'HEAD') == package['rev'], 'Wrong dependency checkout')
        require(not git(checkout, 'status', '--porcelain', '--untracked-files=no'), 'Dirty dependency source')
    generate(out / 'generated')
    for path in (out / 'generated').rglob('*'):
        if path.is_file():
            require(path.read_bytes() == (ROOT / path.relative_to(out / 'generated')).read_bytes(), 'Source regeneration differs')
    build = ROOT / '.lake/build'
    require(not build.is_symlink(), 'Refusing symlinked candidate build cleanup')
    if build.exists(): shutil.rmtree(build)
    before_work(args.allow_busy_two_threads)
    run(['lake', 'build'], ROOT, out / 'build.log', timeout=14400)
    before_work(args.allow_busy_two_threads)
    run(['lake', 'env', 'lean', '-DwarningAsError=true', 'MemoryAudit.lean'], ROOT, out / 'audit.log')
    require(lock['audit_inventory'] in (out / 'audit.log').read_text(), 'Changed logical declaration inventory')
    names = []
    for file in sorted((ROOT / 'SU2BoundaryMemory').glob('*.lean')):
        source = file.read_text()
        require(not re.search(r'\b(sorry|admit|axiom|native_decide|unsafe)\b', source), 'Forbidden proof token')
        namespace, = re.findall(r'^namespace (\S+)', source, re.M)
        names.extend(namespace + '.' + name for name in re.findall(r'\btheorem\s+(\w+)', source))
    require(names == lock['explicit_theorems'], 'Theorem inventory changed')
    types = out / 'TheoremTypes.lean'
    types.write_text('import SU2BoundaryMemory\nset_option pp.universes true\n' +
                     ''.join('#check ' + n + '\n#print axioms ' + n + '\n' for n in names))
    before_work(args.allow_busy_two_threads)
    run(['lake', 'env', 'lean', '-DwarningAsError=true', types], ROOT, out / 'types.log')
    probe = out / 'ExternalAxiomProbe.lean'
    probe.write_text((ROOT / 'MemoryAudit.lean').read_text().replace('open Lean Elab Command in',
        'axiom ExternalBad : False\ntheorem SU2BoundaryMemory.RejectedProbe : False := ExternalBad\nopen Lean Elab Command in'))
    before_work(args.allow_busy_two_threads)
    run(['lake', 'env', 'lean', probe], ROOT, out / 'negative-audit.log', expected=1)
    require('Unapproved axiom ExternalBad' in (out / 'negative-audit.log').read_text(), 'Audit did not reject external axiom')
    # False postconditions, proved false in Lean, must fail semantically under the same contract.
    trials = {}
    controls = [
        ('discard', 'WallMemoryContract wallCode true', 'discarded_writes_rejected'),
        ('wrong-buffer', 'WallMemoryContract wrongBufferCode false', 'wrong_buffer_rejected'),
        ('partial-projection', 'WallMemoryContract refreshedCode false', 'refreshed_projection_rejected'),
        ('cache-stride', 'CacheMemoryContract .shortStride false', 'cache_stride_rejected'),
        ('cache-marker', 'CacheMemoryContract .firstMarker false', 'cache_marker_rejected'),
        ('cache-vertex', 'CacheMemoryContract .firstVertex false', 'cache_vertex_rejected'),
        ('untouched-cache', 'CacheMemoryContract .normal true', 'untouched_cache_rejected'),
        ('repeated-visit', 'RepeatedVisitContract true', 'repeated_visit_omission_rejected')]
    for name, contract, negative in controls:
        trial = out / (name + '.lean')
        trial.write_text('import SU2BoundaryMemory\nopen SU2BoundaryMemory\nexample : ' + contract + ' := by\n  simp only [' + negative + ']\n')
        before_work(args.allow_busy_two_threads)
        log = out / (name + '.log')
        run(['lake', 'env', 'lean', '-DwarningAsError=true', trial], ROOT, log, expected=1)
        require('unsolved goals' in log.read_text() and '⊢ False' in log.read_text(), 'Unrelated mutation failure')
        trials[name] = {'negative_theorem': 'SU2BoundaryMemory.' + negative, 'expected_exit': 1, 'log_sha256': sha(log)}
    protected()
    record = {'status': 'PASS worker checks; independent specification approval and cold acceptance pending',
              'candidate_lock_sha256': sha(ROOT / 'candidate.json'), 'preserved_boundary_lock_sha256': PRIOR_LOCK,
              'lean': subprocess.check_output(['lean', '--version'], text=True).strip(),
              'explicit_theorems': names, 'audit_inventory': lock['audit_inventory'], 'mutation_trials': trials,
              'dependency_artifacts': 'May be warm in worker development; external recipe restores source-only inputs',
              'negative_axiom_audit': 'Unapproved external dependency rejected transitively',
              'logs': {p.name: sha(p) for p in sorted(out.glob('*.log'))}}
    (out / 'result.json').write_text(json.dumps(record, indent=2) + '\n')
    print('Preserved and new proofs, source identities, semantic controls and axiom audits passed.', flush=True)


if __name__ == '__main__':
    main()
