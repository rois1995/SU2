#!/usr/bin/env python3
"""Check the immutable source snapshot against its recorded digest inventory."""
import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def check_source():
    manifest = json.loads((ROOT / 'source/manifest.json').read_text())
    records = [(item['path'], item['sha256']) for item in manifest['files']]
    records += [('CFVMFlowSolverBase.before.inl', manifest['before_sha256']),
                ('accepted-moving-wall.patch', manifest['patch_sha256'])]
    for relative, expected in records:
        actual = hashlib.sha256((ROOT / 'source' / relative).read_bytes()).hexdigest()
        if actual != expected:
            raise ValueError(f'Source digest mismatch: {relative}')
    return manifest


if __name__ == '__main__':
    print('Source snapshot verified:', check_source()['commit'])
