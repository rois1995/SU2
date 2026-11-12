#!/usr/bin/env python3
"""Check a frozen candidate; approval remains the external evaluator's decision."""
import argparse
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
from source import ROOT, REPO, generate
sys.path.insert(0,str(ROOT.parent/'EvaluationKit'))
from common import before_work, git, limits, require, run, sha


def protected():
    lock = json.loads((ROOT/'candidate.json').read_text())
    for rel,digest in lock['historical_files'].items():
        require(sha(ROOT.parent/rel)==digest,'Historical file changed: '+rel)
    for rel,digest in lock['protected_files'].items():
        require(sha(ROOT/rel)==digest,'Candidate changed: '+rel)
    dependency = ROOT/'.lake/packages/cfdlean'
    require(git(dependency,'rev-parse','HEAD')==lock['cfdlean_commit'],'Wrong CFDLean revision')
    require(not git(dependency,'status','--porcelain','--untracked-files=no'),'Dirty CFDLean sources')
    for rel,digest in lock['cfdlean_source_files'].items():
        require(sha(dependency/rel)==digest,'Changed independent dependency: '+rel)
    return lock


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('output',type=Path)
    p.add_argument('--allow-busy-two-threads',action='store_true')
    p.add_argument('--preserved-checks',action='store_true',help='Also rerun all frozen BoundaryMemory checks')
    args = p.parse_args()
    limits()
    lock = protected()
    version = subprocess.check_output(['lean','--version'],text=True).strip()
    require('version 4.19.0,' in version,'Wrong Lean version')
    env = json.loads((ROOT.parent/'Lean/environment.json').read_text())
    compiler = Path(shutil.which('lean')).resolve().parent.parent
    for rel,digest in env['binary_fingerprints'].items():
        require(sha(compiler/rel)==digest,'Wrong pinned compiler binary')
    out = args.output.resolve()
    out.mkdir(parents=True,exist_ok=False)
    for p in json.loads((ROOT/'lake-manifest.json').read_text())['packages']:
        if p['type']=='git':
            require(git(ROOT/'.lake/packages'/p['name'],'rev-parse','HEAD')==p['rev'],'Wrong package '+p['name'])
            require(not git(ROOT/'.lake/packages'/p['name'],'status','--porcelain','--untracked-files=no'),'Dirty package '+p['name'])
        else:
            require(p['name'] in {'su2lean','su2sourcegate','su2eulerpath','su2boundarypath','su2boundarymemory'},'Unexpected path dependency')
    require(not git(REPO,'diff','--name-only','d162016382f0c122433000c114e88d808b48f7a7','103442f30ca2fba1ff48ef12aa1ae93480256a30','--','SU2_CFD','Common','externals'),'Numerical baseline changed')
    generate(out/'generated')
    for f in (out/'generated').rglob('*'):
        if f.is_file(): require(f.read_bytes()==(ROOT/f.relative_to(out/'generated')).read_bytes(),'Source regeneration differs')
    names = []
    for f in sorted((ROOT/'SU2Initialization').glob('*.lean')):
        text = f.read_text()
        require(not re.search(r'\b(sorry|admit|axiom|native_decide|unsafe)\b',text),'Unapproved proof token')
        namespace, = re.findall(r'^namespace (\S+)',text,re.M)
        names.extend(namespace+'.'+n for n in re.findall(r'\btheorem\s+(\w+)',text))
    require(names==lock['explicit_theorems'],'Declaration inventory changed')
    if args.preserved_checks:
        command = [sys.executable,ROOT.parent/'BoundaryMemory/scripts/check.py',out/'preserved']
        if args.allow_busy_two_threads: command.append('--allow-busy-two-threads')
        before_work(args.allow_busy_two_threads)
        run(command,ROOT,out/'preserved-check.log',timeout=14400)
    build = ROOT/'.lake/build'
    require(not build.is_symlink(),'Refusing symlinked build cleanup')
    if build.exists(): shutil.rmtree(build)
    before_work(args.allow_busy_two_threads)
    run(['lake','build'],ROOT,out/'build.log',timeout=14400)
    before_work(args.allow_busy_two_threads)
    run(['lake','env','lean','-DwarningAsError=true','InitializationAudit.lean'],ROOT,out/'audit.log')
    types = out/'Types.lean'
    types.write_text('import SU2Initialization\n'+''.join('#check '+n+'\n#print axioms '+n+'\n' for n in names))
    before_work(args.allow_busy_two_threads)
    run(['lake','env','lean','-DwarningAsError=true',types],ROOT,out/'types.log')
    bad = out/'AxiomProbe.lean'
    bad.write_text((ROOT/'InitializationAudit.lean').read_text().replace('open Lean Elab Command in',
        'axiom ExternalBad : False\ntheorem SU2Initialization.RejectedProbe : False := ExternalBad\nopen Lean Elab Command in'))
    before_work(args.allow_busy_two_threads)
    run(['lake','env','lean',bad],ROOT,out/'negative-audit.log',expected=1)
    require('Unapproved axiom ExternalBad' in (out/'negative-audit.log').read_text(),'Audit failed to reject transitive bad axiom')
    negatives = [('copy-wrong','CopyContract 0 (SU2Boundary.visits 2)','copy_wrong_buffer_rejected'),
        ('copy-incomplete','CopyContract 1 ((SU2Boundary.visits 2).filter (fun c => SU2Boundary.offset c < 7))','copy_incomplete_rejected'),
        ('clear-wrong','ClearContract 6','clear_wrong_buffer_rejected'),
        ('fallback-stale','FallbackContract false','fallback_stale_primitives_rejected'),
        ('omit-kinetic','PressureContract true','kinetic_omission_rejected'),
        ('omit-density','SeedContract true','density_factor_rejected'),
        ('erase-tail','TailContract true','primitive_tail_erasure_rejected')]
    for name,contract,rejection in negatives:
        probe = out/(name+'.lean')
        probe.write_text('import SU2Initialization\nopen SU2Initialization\nexample : '+contract+' := by\n  simp only ['+rejection+']\n')
        before_work(args.allow_busy_two_threads)
        run(['lake','env','lean',probe],ROOT,out/(name+'.log'),expected=1)
        log = (out/(name+'.log')).read_text()
        require('unsolved goals' in log and '⊢ False' in log,'Mutation failed for an unrelated reason')
    protected()
    result = dict(status='PASS worker Lean/source checks; independent acceptance pending',
        candidate_lock_sha256=sha(ROOT/'candidate.json'),lean=version,explicit_theorems=names,
        mathematical_dependency=lock['cfdlean_commit'],negative_controls=[x[0] for x in negatives],
        preserved_checks_rerun=args.preserved_checks,
        logs={f.name:sha(f) for f in sorted(out.glob('*.log'))},
        limitations='Worker may use pinned warm dependencies. Kernel cold acceptance, full native SU2 and unverified path interfaces remain external obligations.')
    (out/'result.json').write_text(json.dumps(result,indent=2)+'\n')
    print(result['status'],flush=True)


if __name__ == '__main__':
    main()
