"""Closed comparison bundle identity and existing core-tool access."""
from pathlib import Path
import hashlib,json,sys

def sha(path):
    h=hashlib.sha256()
    with Path(path).open('rb') as f:
        for data in iter(lambda:f.read(1024*1024),b''):h.update(data)
    return h.hexdigest()

def verify(folder):
    folder=Path(folder).resolve();record=json.loads((folder/'comparison_manifest.json').read_text())
    if record['status']!='PREPARED_NOT_VALIDATED':raise ValueError('Invalid comparison status')
    if any(p.is_symlink() for p in folder.rglob('*')):raise ValueError('Comparison bundle contains a link')
    actual={str(p.relative_to(folder)) for p in folder.rglob('*') if p.is_file()}
    if actual!=set(record['files_sha256'])|{'comparison_manifest.json'}:raise ValueError('Comparison file set changed')
    for name,digest in record['files_sha256'].items():
        path=folder/name;path.resolve().relative_to(folder)
        if Path(name).is_absolute() or sha(path)!=digest:raise ValueError('Comparison artifact changed: '+name)
    return record

def core(folder):
    folder=Path(folder).resolve()
    sys.path[:0]=[str(folder/'shared/core'),str(folder/'shared/audits')]
    import run
    return run
