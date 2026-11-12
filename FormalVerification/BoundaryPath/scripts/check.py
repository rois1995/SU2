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

PRIOR = '15ee28adf032a866e292926502a5f022e38199c8'
PRIOR_LOCK = '0a5781dad650c8de5bf24ef57804c6a422dbef3aa2be3f373b7ebc21371ccac6'


def protected():
    lock = json.loads((ROOT / 'candidate.json').read_text())
    require(sha(ROOT.parent / 'EulerPath/candidate.json') == PRIOR_LOCK, 'Pending EulerPath identity changed')
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
    out = args.output.resolve()
    out.mkdir(parents=True, exist_ok=False)
    before_work(args.allow_busy_two_threads)
    previous = [sys.executable, ROOT.parent / 'EulerPath/scripts/check.py', out / 'preserved']
    if args.allow_busy_two_threads: previous.append('--allow-busy-two-threads')
    run(previous, ROOT, out / 'preserved-check.log', timeout=14400)
    accepted = {p['name']: p for p in json.loads((ROOT.parent / 'Lean/lake-manifest.json').read_text())['packages']}
    for package in json.loads((ROOT / 'lake-manifest.json').read_text())['packages']:
        if package['type'] == 'path':
            require(package['name'] in {'su2lean', 'su2sourcegate', 'su2eulerpath'}, 'Unknown path dependency')
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
    run(['lake', 'env', 'lean', '-DwarningAsError=true', 'BoundaryAudit.lean'], ROOT, out / 'audit.log')
    require(lock['audit_inventory'] in (out / 'audit.log').read_text(), 'Changed logical declaration inventory')
    names = []
    for file in sorted((ROOT / 'SU2Boundary').glob('*.lean')):
        source = file.read_text()
        require(not re.search(r'\b(sorry|admit|axiom|native_decide|unsafe)\b', source), 'Forbidden proof token')
        namespace, = re.findall(r'^namespace (\S+)', source, re.M)
        names.extend(namespace + '.' + name for name in re.findall(r'\btheorem\s+(\w+)', source))
    require(names == lock['explicit_theorems'], 'Theorem inventory changed')
    types = out / 'TheoremTypes.lean'
    types.write_text('import SU2Boundary\nset_option pp.universes true\n' +
                     ''.join('#check ' + n + '\n#print axioms ' + n + '\n' for n in names))
    before_work(args.allow_busy_two_threads)
    run(['lake', 'env', 'lean', '-DwarningAsError=true', types], ROOT, out / 'types.log')
    probe = out / 'ExternalAxiomProbe.lean'
    probe.write_text((ROOT / 'BoundaryAudit.lean').read_text().replace('open Lean Elab Command in',
        'axiom ExternalBad : False\ntheorem SU2Boundary.RejectedProbe : False := ExternalBad\nopen Lean Elab Command in'))
    before_work(args.allow_busy_two_threads)
    run(['lake', 'env', 'lean', probe], ROOT, out / 'negative-audit.log', expected=1)
    require('Unapproved axiom ExternalBad' in (out / 'negative-audit.log').read_text(), 'Audit did not reject external axiom')
    # Each source mutation is already proved to violate the same contract in Controls.lean.
    # These extra attempts normalize the same contract to False using its checked counterexample.
    trials = {}
    for name, contract, positive, negative in [
        ('far-sign', 'FluxAccumulationContract Generated.farAdd_wrongSign', 'far_add_positive', 'far_add_mutant_rejected'),
        ('wall-truncation', 'ThreeBufferContract Generated.wallOld_original Generated.wallRes_original Generated.wallTrunc_wrongSign', 'wall_positive', 'wall_truncation_mutant_rejected'),
        ('last-point', 'LoopWitnessContract Generated.points_skipLast Generated.components_original', 'loop_positive', 'point_mutant_rejected'),
        ('energy', 'LoopWitnessContract Generated.points_original Generated.components_skipLast', 'loop_positive', 'component_mutant_rejected'),
        ('timestep', 'TimestepContract Generated.timestep_wrongField', 'timestep_positive', 'timestep_mutant_rejected')]:
        trial = out / (name + '.lean')
        trial.write_text('import SU2Boundary\nopen SU2Boundary\nexample : ' + contract + ' := by\n  simp only [' + negative + ']\n')
        before_work(args.allow_busy_two_threads)
        log = out / (name + '.log')
        run(['lake', 'env', 'lean', '-DwarningAsError=true', trial], ROOT, log, expected=1)
        require('unsolved goals' in log.read_text() and '⊢ False' in log.read_text(), 'Unrelated mutation failure')
        trials[name] = {'negative_theorem': 'SU2Boundary.' + negative, 'expected_exit': 1, 'log_sha256': sha(log)}
    protected()
    record = {'status': 'PASS worker checks; independent specification approval and cold acceptance pending',
              'candidate_lock_sha256': sha(ROOT / 'candidate.json'), 'preserved_euler_lock_sha256': PRIOR_LOCK,
              'lean': subprocess.check_output(['lean', '--version'], text=True).strip(),
              'explicit_theorems': names, 'audit_inventory': lock['audit_inventory'], 'mutation_trials': trials,
              'dependency_artifacts': 'May be warm in worker development; external recipe restores source-only inputs',
              'negative_axiom_audit': 'Unapproved external dependency rejected transitively',
              'logs': {p.name: sha(p) for p in sorted(out.glob('*.log'))}}
    (out / 'result.json').write_text(json.dumps(record, indent=2) + '\n')
    print('Preserved and new proofs, source identities, semantic controls and axiom audits passed.', flush=True)


if __name__ == '__main__':
    main()
