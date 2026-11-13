"""Freeze original485 nose faces and authority supports before target build/run; no native queries."""
from pathlib import Path
import hashlib,json,math
from inspect_native_failure3d import _read_owned
R=Path(__file__).resolve().parent
sha=lambda p:hashlib.file_digest(Path(p).open('rb'),'sha256').hexdigest()
s=json.loads((R/'saved_surface485_checkpoint.json').read_text())
for name in ('owned','parameters','prepared_reference'):
 assert sha(s[name]['path'])==s[name]['sha256']
with Path(s['prepared_reference']['path']).open() as f:
 assert f.readline().strip()=='SU2_NATIVE_REFERENCE_3D 9'
 counts=f.readline().split();names=[json.loads(f.readline()) for _ in range(int(counts[0]))]
 markers={}
 for _ in range(int(counts[1])):
  row=f.readline().split();markers[int(row[0])]=int(row[1])
wing=names.index('WING3D');faces=[];layout=((1,2,3),(0,3,2),(0,1,3),(0,2,1));minima={}
for cell in _read_owned(Path(s['owned']['path']),s['current_cells'],200_000_000):
 for b,indices in zip(cell['physical'],layout):
  if not b['present'] or markers[b['facet']]!=wing:continue
  nodes=[cell['nodes'][k] for k in indices];faces.append(dict(facet=b['facet'],nodes=nodes))
  for n in nodes:
   x,y,z=n['xyz'];band=math.floor(y/.025);minima[band]=min(minima.get(band,x),x)
selected=[];supports={}
for face in faces:
 ids={n['id'] for n in face['nodes']};mask=int(7594 in ids)+2*int(17814 in ids)
 if any(n['xyz'][0]<=minima[math.floor(n['xyz'][1]/.025)]+.015 for n in face['nodes']):mask|=4
 if mask:
  selected.append(dict(face,mask=mask));supports[face['facet']]=supports.get(face['facet'],0)|mask
assert all(any(f['mask']&flag for f in selected) for flag in (1,2,4))
manifest=dict(input=s['owned'],parameters=s['parameters'],authority=s['prepared_reference'],faces=selected,
 selection='Wing faces incident to7594 (bit1),17814 (bit2); nose band(bit4): any vertex x <= minimum wing x in floor(y/.025) bin + .015. Frozen before run; geometric selection only.',
 coverage='Post-run same original authority facet supports; changed cross-facet faces require explicit coverage qualification, never assumed exact descendants.',
 supports=len(supports),counts={str(flag):sum(bool(f['mask']&flag) for f in selected) for flag in (1,2,4)})
p=R/'nose492_fixed_faces.json';assert not p.exists();p.write_text(json.dumps(manifest,indent=2)+'\n')
(R/'nose492_fixed_facets.dat').write_text(str(len(supports))+'\n'+'\n'.join(f'{facet} {mask}' for facet,mask in sorted(supports.items()))+'\n')
print(dict(counts=manifest['counts'],supports=len(supports),manifest_sha256=sha(p)))
