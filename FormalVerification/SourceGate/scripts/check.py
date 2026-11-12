#!/usr/bin/env python3
"""Recheck the source-connected candidate; never makes an independent acceptance decision."""
import argparse
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT.parent / 'EvaluationKit'))
from common import before_work, git, limits, require, run, sha
from source import generate
from native import check_native


def protected():
    lock = json.loads((ROOT / 'candidate.json').read_text())
    for name, expected in lock['protected_files'].items():
        require(sha(ROOT / name) == expected, 'Candidate changed: ' + name)
    foundation = ROOT.parent / 'Lean'
    for line in (foundation / 'SHA256SUMS').read_text().splitlines():
        expected, name = line.split(maxsplit=1)
        require(sha(foundation / name) == expected, 'Frozen Phase 1 changed: ' + name)
    return lock


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path, help='New directory for full logs, generated source and native fixtures')
    parser.add_argument('--allow-busy-two-threads', action='store_true', help='Lean only; requires machine-owner authorization')
    args = parser.parse_args()
    limits()
    lock = protected()
    out = args.output.resolve()
    out.mkdir(parents=True, exist_ok=False)
    toolchain = Path(shutil.which('lean')).resolve().parents[1]
    for name, expected in json.loads((ROOT.parent / 'Lean/environment.json').read_text())['binary_fingerprints'].items():
        require(sha(toolchain / name) == expected, 'Pinned compiler fingerprint mismatch: ' + name)
    version = subprocess.check_output(['lean', '--version'], text=True).strip()
    packages = json.loads((ROOT / 'lake-manifest.json').read_text())['packages']
    accepted = {p['name']: p for p in json.loads((ROOT.parent / 'Lean/lake-manifest.json').read_text())['packages']}
    for package in packages:
        if package['type'] == 'path':
            require(package['name'] == 'su2lean' and package['dir'] == './../Lean', 'Wrong foundation path')
            continue
        require(package['rev'] == accepted[package['name']]['rev'], 'Changed dependency pin')
        directory = ROOT / '.lake/packages' / package['name']
        require(git(directory, 'rev-parse', 'HEAD') == package['rev'], 'Dependency checkout changed')
        require(not git(directory, 'status', '--porcelain', '--untracked-files=no'), 'Dirty dependency source')
    generate(out / 'generated')
    for path in ['SU2Source/Generated.lean', 'source-manifest.json'] + [
            'fixtures/' + p.name for p in sorted((ROOT / 'fixtures').glob('*.cpp'))]:
        require((out / 'generated' / path).read_bytes() == (ROOT / path).read_bytes(), 'Source derivation changed: ' + path)
    build = ROOT / '.lake/build'
    require(not build.is_symlink(), 'Refusing to clean a symlinked build directory')
    if build.exists():
        shutil.rmtree(build)
    before_work(args.allow_busy_two_threads)
    run(['lake', 'build'], ROOT, out / 'build.log')
    before_work(args.allow_busy_two_threads)
    run(['lake', 'env', 'lean', '-DwarningAsError=true', 'SourceAudit.lean'], ROOT, out / 'audit.log')
    require(lock['audit_inventory'] in (out / 'audit.log').read_text(), 'Changed declaration inventory')
    names = []
    for file in sorted((ROOT / 'SU2Source').glob('*.lean')):
        source = file.read_text()
        require(not re.search(r'\b(sorry|admit|axiom|native_decide|unsafe)\b', source), 'Forbidden source token')
        namespace, = re.findall(r'^namespace (\S+)', source, re.M)
        names += [namespace + '.' + n for n in re.findall(r'^theorem (\w+)', source, re.M)]
    require(names == lock['explicit_theorems'], 'Theorem inventory changed')
    types = out / 'TheoremTypes.lean'
    types.write_text('import SU2Source\n' + ''.join('#check ' + n + '\n#print axioms ' + n + '\n' for n in names))
    before_work(args.allow_busy_two_threads)
    run(['lake', 'env', 'lean', '-DwarningAsError=true', types], ROOT, out / 'types.log')
    rejected = out / 'RejectedAudit.lean'
    rejected.write_text((ROOT / 'SourceAudit.lean').read_text().replace('open Lean Elab Command in',
                        'axiom SU2Source.RejectedProbe : False\nopen Lean Elab Command in'))
    before_work(args.allow_busy_two_threads)
    run(['lake', 'env', 'lean', rejected], ROOT, out / 'negative-audit.log', expected=1)
    require('Unapproved axiom SU2Source.RejectedProbe' in (out / 'negative-audit.log').read_text(),
            'Audit did not reject the injected axiom')
    # Exactly the same mathematical postcondition and proof attempt for every raw-source variant.
    trials = {}
    for fixture in sorted((out / 'generated/fixtures').glob('*.cpp')):
        name = fixture.stem
        trial = out / (name + '-contract.lean')
        trial.write_text('import SU2Source\nopen SU2Source\nset_option maxRecDepth 100000\n' +
            'def selected : Program := parsedProgram ' + json.dumps(fixture.read_text().removesuffix('\n')) +
            ' (by decide)\n' +
            'example : WitnessContract selected := by\n' +
            '  refine ⟨Function.update witnessMemory (0, 0) (some 5), ?_, ?_⟩\n' +
            '  · rfl\n' +
            '  · exact store_contract integerOperations witnessMemory (0, 0) (1, 0) 2 3 rfl\n')
        expected = 0 if name in {'original', 'refactored'} else 1
        before_work(args.allow_busy_two_threads)
        log = out / (name + '-contract.log')
        run(['lake', 'env', 'lean', '-DwarningAsError=true', trial], ROOT, log, expected=expected)
        if expected:
            require('rfl' in log.read_text() and 'execute' in log.read_text(), 'Unrelated negative-proof failure')
        trials[name] = {'expected_exit': expected, 'trial_sha256': sha(trial), 'log_sha256': sha(log)}
    native = check_native(out)
    protected()
    result = {'status': 'Worker checks passed; independent source-gate acceptance pending',
              'lean': version, 'candidate_lock_sha256': sha(ROOT / 'candidate.json'),
              'fresh_gate_build': True, 'dependency_builds': 'Existing pinned artifacts; cold rebuild recipe documented separately',
              'explicit_theorems': names, 'audit_inventory': lock['audit_inventory'],
              'contract_trials': trials, 'native': native,
              'limitations': 'Restricted-source semantics only; source/context extraction and C++ correspondence remain unverified. No IEEE-754 rounding proof.'}
    result['logs'] = {p.name: sha(p) for p in sorted(out.glob('*.log'))}
    (out / 'result.json').write_text(json.dumps(result, indent=2) + '\n')
    print('All candidate checks passed; external acceptance is still required.', flush=True)


if __name__ == '__main__':
    main()
