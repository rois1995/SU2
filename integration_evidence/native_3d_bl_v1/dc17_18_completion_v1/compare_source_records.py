"""Exact identity of diagnostic v1 RecordStream DonorCell snapshots."""
import hashlib,json,struct,sys
from pathlib import Path

def merge(parts):
    result={}
    for part in parts:
        for key,record in part.items():
            if key in result:raise ValueError('Duplicate source cell across partitions')
            result[key]=record
    return result

assert merge([{1:b'a'},{2:b'b'}])=={1:b'a',2:b'b'}
try:merge([{1:b'a'},{1:b'a'}])
except ValueError:pass
else:raise AssertionError('Duplicate partition was admitted')

def read(path,rank=None,ranks=None):
    # Cell id, four (node id, xyz), four six-entry tensors, component, parent:43 scalars.
    data=path.read_bytes()
    if len(data)<16 or len(data)>(1<<30):raise ValueError('Invalid snapshot size')
    version,count=struct.unpack_from('=QQ',data)
    if version!=1 or not 0<count<=2000000 or len(data)!=16+344*count:raise ValueError('Invalid snapshot header/records')
    records={}
    for index in range(count):
        row=data[16+344*index:16+344*(index+1)];key=struct.unpack_from('=Q',row)[0]
        if key in records:raise ValueError('Duplicate source cell')
        component,parent=struct.unpack_from('=QQ',row,328)
        if rank is not None:
            owner=key if component==(1<<64)-1 else parent
            if owner%ranks!=rank:raise ValueError('Source record left its CFD owner')
        records[key]=row
    return records

serial,destination,*parts=map(Path,sys.argv[1:])
if len(parts)<2:raise ValueError('Supply all rank snapshots in rank order')
expected=read(serial);actual=merge([read(p,i,len(parts)) for i,p in enumerate(parts)])
if expected!=actual:raise ValueError('Distributed source differs from serial geometry/sensors/provenance')
canonical=hashlib.sha256()
for key in sorted(expected):canonical.update(expected[key])
result=dict(identical=True,cells=len(expected),ranks=len(parts),canonical_sha256=canonical.hexdigest(),
            byteorder=sys.byteorder,pins={str(p):hashlib.sha256(p.read_bytes()).hexdigest() for p in [serial,*parts,Path(__file__)]})
destination.write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))
