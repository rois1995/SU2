#!/usr/bin/env python3
"""Worker-authored candidate checks; independent acceptance remains external."""
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


def protected():
    lock = json.loads((ROOT / 'candidate.json').read_text())
    for path, digest in lock['protected_files'].items():
        require(sha(ROOT / path) == digest, 'Candidate changed: ' + path)
    for path, digest in lock['historical_files'].items():
        require(sha(ROOT.parent / path) == digest, 'Historical accepted file changed: ' + path)
    old = json.loads((ROOT.parent / 'SourceGate/candidate.json').read_text())
    for path, digest in old['protected_files'].items():
        require(sha(ROOT.parent / 'SourceGate' / path) == digest, 'Accepted source gate changed')
    return lock


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path, help='New directory; contains full logs and trials')
    parser.add_argument('--allow-busy-two-threads', action='store_true', help='Lean only, requires machine-owner authorization')
    args = parser.parse_args()
    limits()
    lock = protected()
    out = args.output.resolve()
    out.mkdir(parents=True, exist_ok=False)
    toolchain = Path(shutil.which('lean')).resolve().parents[1]
    environment = json.loads((ROOT.parent / 'Lean/environment.json').read_text())
    for name, digest in environment['binary_fingerprints'].items():
        require(sha(toolchain / name) == digest, 'Pinned Lean binary mismatch')
    version = subprocess.check_output(['lean', '--version'], text=True).strip()
    accepted = {p['name']: p for p in json.loads((ROOT.parent / 'Lean/lake-manifest.json').read_text())['packages']}
    for project in [ROOT.parent / 'Lean', ROOT.parent / 'SourceGate', ROOT]:
        for package in json.loads((project / 'lake-manifest.json').read_text())['packages']:
            if package['type'] == 'path':
                require(package['name'] in {'su2lean', 'su2sourcegate'}, 'Unknown path dependency')
                continue
            require(package['rev'] == accepted[package['name']]['rev'], 'Changed dependency pin')
            directory = project / '.lake/packages' / package['name']
            require(git(directory, 'rev-parse', 'HEAD') == package['rev'], 'Wrong package checkout')
            require(not git(directory, 'status', '--porcelain', '--untracked-files=no'), 'Dirty dependency source')
    generate(out / 'generated')
    for path in (out / 'generated').rglob('*'):
        if path.is_file():
            require(path.read_bytes() == (ROOT / path.relative_to(out / 'generated')).read_bytes(), 'Regenerated source changed')
    build = ROOT / '.lake/build'
    require(not build.is_symlink(), 'Refusing symlinked build cleanup')
    if build.exists():
        shutil.rmtree(build)
    for name, project, audit in [('foundation', ROOT.parent / 'Lean', 'Integrity.lean'),
                                 ('historical-source', ROOT.parent / 'SourceGate', 'SourceAudit.lean'),
                                 ('euler', ROOT, 'EulerAudit.lean')]:
        before_work(args.allow_busy_two_threads)
        run(['lake', 'build'], project, out / (name + '-build.log'))
        before_work(args.allow_busy_two_threads)
        run(['lake', 'env', 'lean', '-DwarningAsError=true', audit], project, out / (name + '-audit.log'))
    require(lock['audit_inventory'] in (out / 'euler-audit.log').read_text(), 'Changed audit declaration inventory')
    inventories = {}
    for project, directory, import_name in [(ROOT.parent / 'Lean', 'SU2', 'SU2'),
                                            (ROOT.parent / 'SourceGate', 'SU2Source', 'SU2Source'),
                                            (ROOT, 'SU2Euler', 'SU2Euler')]:
        names = []
        for file in sorted((project / directory).glob('*.lean')):
            source = file.read_text()
            require(not re.search(r'\b(sorry|admit|axiom|native_decide|unsafe)\b', source), 'Forbidden proof source token')
            namespace, = re.findall(r'^namespace (\S+)', source, re.M)
            names += [namespace + '.' + n for n in re.findall(r'\btheorem\s+(\w+)', source)]
        inventories[directory] = names
        types = out / (directory + '-types.lean')
        types.write_text('import ' + import_name + '\nset_option pp.universes true\n' +
                         ''.join('#check ' + n + '\n#print axioms ' + n + '\n' for n in names))
        before_work(args.allow_busy_two_threads)
        run(['lake', 'env', 'lean', '-DwarningAsError=true', types], project, out / (directory + '-types.log'))
    require(len(inventories['SU2']) == 52, 'Accepted theorem inventory changed')
    require(inventories['SU2Euler'] == lock['explicit_theorems'], 'Candidate theorem inventory changed')
    negative = out / 'ExternalAxiomProbe.lean'
    negative.write_text((ROOT / 'EulerAudit.lean').read_text().replace('open Lean Elab Command in',
        'axiom ExternalBad : False\ntheorem SU2Euler.RejectedProbe : False := ExternalBad\nopen Lean Elab Command in'))
    before_work(args.allow_busy_two_threads)
    run(['lake', 'env', 'lean', negative], ROOT, out / 'negative-audit.log', expected=1)
    require('Unapproved axiom ExternalBad' in (out / 'negative-audit.log').read_text(), 'Transitive negative audit failed')
    trials = {}
    for variant in ['original', 'refactored', 'subtract', 'currentRead', 'wrongTarget']:
        source = (out / 'generated/fixtures' / ('update_' + variant + '.cpp')).read_text().removesuffix('\n')
        trial = out / ('scalar-' + variant + '.lean')
        trial.write_text('import SU2Euler\nopen SU2Source\nset_option maxRecDepth 100000\n' +
            'def selected : Program := parsedProgram ' + json.dumps(source) + ' (by decide)\n' +
            'example : WitnessContract selected := by\n' +
            '  refine ⟨Function.update witnessMemory (0, 0) (some 5), ?_, ?_⟩\n' +
            '  · rfl\n  · exact store_contract integerOperations witnessMemory (0, 0) (1, 0) 2 3 rfl\n')
        expected = 0 if variant in {'original', 'refactored'} else 1
        before_work(args.allow_busy_two_threads)
        log = out / ('scalar-' + variant + '.log')
        run(['lake', 'env', 'lean', '-DwarningAsError=true', trial], ROOT, log, expected=expected)
        if expected:
            require('rfl' in log.read_text() and 'execute' in log.read_text(), 'Unrelated scalar mutation failure')
        trials[variant] = {'expected_exit': expected, 'log_sha256': sha(log)}
    protected()
    result = {'status': 'PASS local Lean checks; new independent acceptance pending', 'lean': version,
              'candidate_lock_sha256': sha(ROOT / 'candidate.json'), 'fresh_euler_build': True,
              'dependency_builds': 'Pinned existing mathlib artifacts in development; cold recipe supplied separately',
              'theorems': inventories, 'audit_inventory': lock['audit_inventory'],
              'scalar_trials': trials, 'composed_mutants': 'Four parsed semantic mutants formally violate the same TestContract; positive and refactor controls proved',
              'negative_audit': 'External unapproved axiom rejected transitively',
              'logs': {p.name: sha(p) for p in sorted(out.glob('*.log'))}}
    (out / 'result.json').write_text(json.dumps(result, indent=2) + '\n')
    print('Frozen candidate proofs, source regeneration, types, axiom audit and controls passed.', flush=True)


if __name__ == '__main__':
    main()
