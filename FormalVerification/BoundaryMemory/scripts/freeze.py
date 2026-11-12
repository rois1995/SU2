#!/usr/bin/env python3
"""Create a worker candidate lock after development checks; never approve it."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess

from source import ROOT, COMMIT

PRIOR = '5a6d3a578a4a348b91ef6ce8dcd259e142f60d37'
REPO = ROOT.parents[1]


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('audit_log', type=Path)
    args = parser.parse_args()
    lock = ROOT / 'candidate.json'
    if lock.exists():
        raise ValueError('Candidate already frozen; do not replace its identity')
    inventory, = re.findall(r'Audited \d+ SU2BoundaryMemory logical declarations \(\d+ theorem declarations\); standard logical axioms only\.', args.audit_log.read_text())
    old = subprocess.check_output(['git', '-C', str(REPO), 'ls-tree', '-r', '--name-only', PRIOR, 'FormalVerification'], text=True).splitlines()
    historical = {}
    for rel in old:
        raw = subprocess.check_output(['git', '-C', str(REPO), 'show', PRIOR + ':' + rel])
        if raw != (REPO / rel).read_bytes():
            raise ValueError('Preserved candidate changed: ' + rel)
        historical[str(Path(rel).relative_to('FormalVerification'))] = hashlib.sha256(raw).hexdigest()
    files = [p for p in ROOT.iterdir() if p.is_file()]
    for folder in ['SU2BoundaryMemory', 'scripts', 'fixtures', 'tests', 'coordination', 'evidence']:
        files.extend(p for p in (ROOT / folder).rglob('*') if p.is_file() and '__pycache__' not in p.parts)
    names = []
    for path in sorted((ROOT / 'SU2BoundaryMemory').glob('*.lean')):
        namespace, = re.findall(r'^namespace (\S+)', path.read_text(), re.M)
        names.extend(namespace + '.' + n for n in re.findall(r'\btheorem\s+(\w+)', path.read_text()))
    record = {'status': 'Worker-frozen candidate; external specification approval and independent cold acceptance pending',
              'upstream_baseline': COMMIT, 'production_patches': [], 'investigation_commit': PRIOR,
              'preserved_boundary_lock_sha256': digest(ROOT.parent / 'BoundaryPath/candidate.json'),
              'protected_files': {str(p.relative_to(ROOT)): digest(p) for p in sorted(files)},
              'historical_files': historical, 'explicit_theorems': names, 'audit_inventory': inventory,
              'accepted_coverage': 'None newly accepted by this worker; mathematical/model coverage and source limitations are specified in protected documents.'}
    lock.write_text(json.dumps(record, indent=2) + '\n')
    print(digest(lock))


if __name__ == '__main__':
    main()
