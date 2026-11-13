#!/usr/bin/env python3
"""Reproduce worker checks. Evaluators must independently approve specifications and results."""
import argparse
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
from source import ROOT,REPO,generate
sys.path.insert(0,str(ROOT.parent/'EvaluationKit'))
from common import before_work,git,limits,require,run,sha


def protected():
    lock = json.loads((ROOT/'candidate.json').read_text())
    for rel,digest in lock['historical_files'].items():
        require(sha(ROOT.parent/rel)==digest,'Historical file changed: '+rel)
    for rel,digest in lock['protected_files'].items():
        require(sha(ROOT/rel)==digest,'Candidate changed: '+rel)
    dep = ROOT/'.lake/packages/cfdlean'
    require(git(dep,'rev-parse','HEAD')==lock['cfdlean_commit'],'Wrong CFDLean revision')
    require(not git(dep,'status','--porcelain','--untracked-files=no'),'Dirty CFDLean source')
    for rel,digest in lock['cfdlean_source_files'].items():
        require(sha(dep/rel)==digest,'Independent source changed: '+rel)
    return lock


def inventory(path):
    rows = [json.loads(line.split('DECL ',1)[1]) for line in path.read_text().splitlines() if 'DECL {' in line]
    require(rows,'No elaborated theorem inventory')
    require(len(rows)==len({row['name'] for row in rows}),'Duplicate theorem declarations')
    return sorted(rows,key=lambda row:row['name'])


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('output',type=Path)
    p.add_argument('--allow-busy-two-threads',action='store_true')
    p.add_argument('--preserved-checks',action='store_true')
    args = p.parse_args()
    limits()
    lock = protected()
    require('version 4.19.0,' in subprocess.check_output(['lean','--version'],text=True),'Wrong Lean version')
    env = json.loads((ROOT.parent/'Lean/environment.json').read_text())
    compiler = Path(shutil.which('lean')).resolve().parent.parent
    for rel,digest in env['binary_fingerprints'].items():
        require(sha(compiler/rel)==digest,'Wrong pinned compiler binary: '+rel)
    for package in json.loads((ROOT/'lake-manifest.json').read_text())['packages']:
        if package['type']=='git':
            dep = ROOT/'.lake/packages'/package['name']
            require(git(dep,'rev-parse','HEAD')==package['rev'],'Wrong package '+package['name'])
            require(not git(dep,'status','--porcelain','--untracked-files=no'),'Dirty package '+package['name'])
        else:
            require(package['name'] in lock['path_dependencies'],'Unexpected path dependency')
    require(not git(REPO,'diff','--name-only',lock['source_commit'],lock['checked_upstream'],'--','SU2_CFD','Common','externals'),'Numerical baseline changed')
    out = args.output.resolve()
    out.mkdir(parents=True,exist_ok=False)
    generate(out/'generated')
    for f in (out/'generated').rglob('*'):
        if f.is_file():
            rel = f.relative_to(out/'generated')
            expected = ROOT/rel
            require(f.read_bytes()==expected.read_bytes(),'Source regeneration differs: '+str(rel))
    if args.preserved_checks:
        command = [sys.executable,ROOT.parent/'Preprocessing/scripts/check.py',out/'preserved']
        if args.allow_busy_two_threads: command.append('--allow-busy-two-threads')
        command.append('--preserved-checks')
        before_work(args.allow_busy_two_threads)
        run(command,ROOT,out/'preserved.log',timeout=14400)
    build = ROOT/'.lake/build'
    require(not build.is_symlink(),'Refusing symlink build cleanup')
    if build.exists(): shutil.rmtree(build)
    before_work(args.allow_busy_two_threads)
    run(['lake','build'],ROOT,out/'build.log',timeout=14400)
    # A clean new-project build elaborates InteriorAudit; reuse that exact output.
    shutil.copyfile(out/'build.log',out/'audit.log')
    declarations = inventory(out/'audit.log')
    require(declarations==lock['elaborated_theorems'],'Elaborated declarations/types/axioms changed')
    bad = out/'AxiomProbe.lean'
    bad.write_text((ROOT/'InteriorAudit.lean').read_text().replace('open Lean Elab Command in',
        'axiom ExternalBad : False\ntheorem SU2Interior.AAAARejectedProbe : False := ExternalBad\nopen Lean Elab Command in'))
    before_work(args.allow_busy_two_threads)
    run(['lake','env','lean',bad],ROOT,out/'negative-audit.log',expected=1)
    require('Unapproved axiom ExternalBad' in (out/'negative-audit.log').read_text(),'Transitive axiom probe failed for unrelated reason')
    probes = [('uncapped','CappedStoreContract Generated.time_uncapped','time_uncapped_rejected'),
        ('wrong-sign','ScatterMemoryContract Generated.scatter_wrongSign','scatter_memory_wrong_sign_rejected'),
        ('active-padding','LastEdgeOnce (activatePadding (allLanes 1 3))','active_padding_rejected')]
    for name,contract,rejection in probes:
        probe = out/(name+'.lean')
        probe.write_text('import SU2Interior\nopen SU2Interior\nexample : '+contract+' := by\n  simp only ['+rejection+']\n')
        before_work(args.allow_busy_two_threads)
        log = out/(name+'.log')
        run(['lake','env','lean',probe],ROOT,log,expected=1)
        require('unsolved goals' in log.read_text() and '⊢ False' in log.read_text(),'Unrelated semantic-control failure')
    protected()
    result = dict(status='PASS worker kernel/type/axiom/source/semantic checks; independent cold acceptance pending',
        theorem_count=len(declarations),candidate_sha256=sha(ROOT/'candidate.json'),audit_sha256=sha(out/'audit.log'),
        semantic_negatives=[x[0] for x in probes],source_commit=lock['source_commit'],
        dependency_mode='pinned local dependencies; coldness must be established independently',production_patches=[])
    (out/'result.json').write_text(json.dumps(result,indent=2)+'\n')
    print(result['status'],flush=True)

if __name__ == '__main__': main()
