"""Restore hash-checked compact raw JSON evidence for existing native3D auditors; keep compressed originals."""
from pathlib import Path
import gzip,json,sys
from run import sha

def restore(folder):
    folder=Path(folder).resolve();record=json.loads((folder/'result.json').read_text())
    if any(p.is_symlink() for p in folder.rglob('*')):raise ValueError('Result contains a symbolic link')
    for name,digest in record['artifact_sha256'].items():
        p=folder/name;p.resolve().relative_to(folder)
        if not p.is_file() or sha(p)!=digest:raise ValueError('Result artifact changed: '+name)
    count=0
    for case in record['cases']:
        if Path(case['folder']).name!=case['folder']:raise ValueError('Invalid case folder')
        for name,info in case.get('packed',{}).items():
            if Path(name).name!=name or not name.endswith('.json') or not 0<info['bytes']<=512*1024*1024:
                raise ValueError('Invalid packed evidence entry')
            source=folder/case['folder']/(name+'.gz');target=source.with_suffix('')
            if sha(source)!=info['gzip_sha256']:raise ValueError('Compressed evidence changed')
            if target.exists():
                if sha(target)!=info['sha256']:raise ValueError('Refuse to overwrite modified raw evidence')
                continue
            with gzip.open(source,'rb') as f:data=f.read(info['bytes']+1)
            import hashlib
            if len(data)!=info['bytes'] or hashlib.sha256(data).hexdigest()!=info['sha256']:
                raise ValueError('Restored evidence length/hash mismatch')
            target.write_bytes(data);count+=1
    print('Restored',count,'raw JSON files in',folder)
if __name__=='__main__':
    for path in sys.argv[1:]:restore(path)
