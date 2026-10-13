#!/usr/bin/env python3
"""Independent geometry checks and SU2 export; these are kernel grids, not CFD qualification."""
import hashlib,json,math
from pathlib import Path
ROOT=Path(__file__).resolve().parent

def delta(a,b):return [x-y for x,y in zip(a,b)]
def dot(a,b):return sum(x*y for x,y in zip(a,b))
def cross(a,b):return [a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]]
def digest(p):return hashlib.sha256(p.read_bytes()).hexdigest()

def check(name,height,baseline):
 p=ROOT/(name+'.json');data=json.loads(p.read_text());nodes={n[0]:n[1:] for n in data['nodes']}
 original={n[0]:n[1:] for n in baseline['nodes']}
 assert all(nodes.get(i)==x for i,x in original.items())
 assert data['surface']==baseline['surface']
 volume=0
 for cell in data['cells']:
  a,b,c,d=[nodes[i] for i in cell];v=dot(delta(b,a),cross(delta(c,a),delta(d,a)))/6
  assert v>0;volume+=v
 assert abs(volume-1)<1e-12
 errors=[];tilts=[]
 for face in data['surface']:
  ids=face[:3]
  if not all(nodes[i][2]==0 for i in ids):continue
  cells=[c for c in data['cells'] if set(ids).issubset(c)]
  assert len(cells)==1
  if height is None:continue
  apex=nodes[next(i for i in cells[0] if i not in ids)]
  a,b,c=[nodes[i] for i in ids];normal=cross(delta(b,a),delta(c,a));length=math.sqrt(dot(normal,normal));normal=[x/length for x in normal]
  center=[(a[j]+b[j]+c[j])/3 for j in range(3)];v=delta(apex,center);altitude=dot(v,normal)
  errors.append(abs(abs(altitude)/height-1));tilts.append(math.sqrt(dot(delta(v,[altitude*x for x in normal]),delta(v,[altitude*x for x in normal])))/height)
 if height is not None:
  assert len(errors)==2 and max(errors)<1e-8 and max(tilts)<1e-12
 ids={n:i for i,n in enumerate(sorted(nodes))}
 lines=['NDIME= 3','NELEM= '+str(len(data['cells']))]
 lines.extend('10 '+' '.join(str(ids[n]) for n in c) for c in data['cells'])
 lines.append('NPOIN= '+str(len(nodes)))
 lines.extend(' '.join(format(x,'.17g') for x in nodes[n])+' '+str(ids[n]) for n in sorted(nodes))
 groups={'wall':[],'far':[]}
 for f in data['surface']:groups['wall' if all(nodes[i][2]==0 for i in f[:3]) else 'far'].append(f[:3])
 lines.append('NMARK= 2')
 for marker,faces in groups.items():
  lines.extend(['MARKER_TAG= '+marker,'MARKER_ELEMS= '+str(len(faces))]);lines.extend('5 '+' '.join(str(ids[n]) for n in f) for f in faces)
 out=ROOT/(name+'_kernel.su2');out.write_text('\n'.join(lines)+'\n')
 return {'name':name,'status':'PASS','cells':len(data['cells']),'nodes':len(nodes),'volume':volume,'height':height,
         'max_relative_height_error':max(errors,default=0),'max_tilt':max(tilts,default=0),
         'input_sha256':digest(p),'mesh':out.name,'mesh_sha256':digest(out)}

baseline=json.loads((ROOT/'first_height_initial.json').read_text())
results=[check('first_height_initial',None,baseline),check('first_height_constructed',.05,baseline),check('first_height_updated',.025,baseline)]
(ROOT/'independent_geometry.json').write_text(json.dumps({'status':'PASS','helper_sha256':digest(Path(__file__)),
 'scope':'positive volume, fixed boundary, preserved original coordinates, unique wall coverage, altitude/tilt; C++ handles exact pairwise conformity',
 'limitations':['kernel geometry only; metric/CFD/transfer/MPI/publication not qualified'],'cases':results},indent=2)+'\n')
print('Independent kernel geometry checks and SU2 exports: PASS')
