#!/usr/bin/env python3
"""Freeze proposed worker contracts; external approval is a separate decision."""
import argparse
import json
from pathlib import Path
import re
import subprocess
import sys
from source import ROOT,REPO,COMMIT
from check import inventory
sys.path.insert(0,str(ROOT.parent/'EvaluationKit'))
from common import git,require,sha
PARENT = '2ffc490435e57ebb1c59bc6cc706f24c2ce9b86e'
def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('successful_audit',type=Path)
    args = parser.parse_args()
    require(not (ROOT/'candidate.json').exists(),'Candidate already frozen; preserve it and propose a separate correction')
    log = args.successful_audit.read_text()
    require('Build completed successfully.' in log and 'all proof dependencies checked' in log and 'EXIT 0' in log,'Audit did not complete')
    declarations = inventory(args.successful_audit)
    explicit = []
    for path in sorted((ROOT/'SU2Interior').glob('*.lean')):
        # Supplemental lexical count only; actual types/axioms come from elaborated declarations.
        explicit.extend(re.findall(r'^(?:@\[[^\n]*\]\s*)?theorem\s+([A-Za-z0-9_\']+)',path.read_text(),flags=re.M))
    historical = {}
    files = git(REPO,'ls-tree','-r','--name-only',PARENT,'FormalVerification').splitlines()
    for rel in files:
        path = REPO/rel
        expected = subprocess.check_output(['git','-C',str(REPO),'show',PARENT+':'+rel])
        require(path.read_bytes()==expected,'Frozen parent changed: '+rel)
        historical[str(path.relative_to(ROOT.parent))] = sha(path)
    dependency = ROOT/'.lake/packages/cfdlean'
    require(git(dependency,'rev-parse','HEAD')=='c92c0f66360f85c0a3339ab9b30fc00c39ee5f93','Unapproved mathematical pin')
    dependency_files = git(dependency,'ls-files','*.lean','lean-toolchain','lakefile.toml','lake-manifest.json').splitlines()
    for rel in dependency_files:
        expected = subprocess.check_output(['git','-C',str(dependency),'show','HEAD:'+rel])
        require((dependency/rel).read_bytes()==expected,'Independent source changed: '+rel)
    protected = {}
    for path in sorted(ROOT.rglob('*')):
        rel = path.relative_to(ROOT)
        if path.is_file() and not any(p in ['.lake','__pycache__'] for p in rel.parts):protected[str(rel)] = sha(path)
    manifest = json.loads((ROOT/'lake-manifest.json').read_text())
    lock = dict(status='Frozen proposed worker candidate; external independent acceptance pending',source_commit=COMMIT,
        checked_upstream='103442f30ca2fba1ff48ef12aa1ae93480256a30',parent_candidate=PARENT,production_patches=[],
        cfdlean_commit=git(dependency,'rev-parse','HEAD'),cfdlean_source_files={rel:sha(dependency/rel) for rel in dependency_files},
        private_cfdlean_source_published=False,private_source_sharing='Keep local; evaluator obtains separately authorized input',
        path_dependencies=[p['name'] for p in manifest['packages'] if p['type']=='path'],historical_files=historical,
        protected_files=protected,elaborated_theorems=declarations,explicit_theorems=explicit,
        explicit_theorem_count=len(explicit),elaborated_theorem_count=len(declarations),
        inventory_method='Elaborated environment with full types, individual transitive axioms and union dependency closure; lexical explicit count supplemental only',
        approval_authority='External evaluator; worker cannot approve requirements or final acceptance')
    (ROOT/'candidate.json').write_text(json.dumps(lock,indent=2)+'\n')
    print('Frozen proposed candidate',sha(ROOT/'candidate.json'),'explicit',len(explicit),'elaborated',len(declarations),'historical files',len(historical))
if __name__=='__main__':main()
