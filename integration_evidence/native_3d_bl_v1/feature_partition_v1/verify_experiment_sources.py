#!/usr/bin/env python3
"""Reconstruct saved failed source versions; this does not replay their binaries."""
import hashlib
import json
from pathlib import Path
import subprocess
import tempfile

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]

def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

def main():
    records = sorted(HERE.glob('*_failed_source.json'))
    assert records, 'No source checkpoints found'
    for record in records:
        data = json.loads(record.read_text())
        patch = HERE / record.name.replace('_source.json', '.patch')
        assert digest(patch) == data['patch_sha256'], str(patch)
        with tempfile.TemporaryDirectory(prefix='native3d-source-', dir='/tmp') as folder:
            work = Path(folder)
            for name in data['sources']:
                path = Path(name)
                assert not path.is_absolute() and '..' not in path.parts, name
                target = work / path
                target.parent.mkdir(parents=True, exist_ok=True)
                target.write_bytes(subprocess.check_output(['git', 'show', data['base'] + ':' + name], cwd=ROOT))
            subprocess.run(['git', 'apply', str(patch)], cwd=work, check=True)
            for name, expected in data['sources'].items():
                assert digest(work / name) == expected, record.name + ': ' + name
        print('PASS:', record.name)
    return 0

if __name__ == '__main__':
    raise SystemExit(main())
