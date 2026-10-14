#!/usr/bin/env python3
"""Exact rational wall-normal footprints in the published planar production evidence."""
import hashlib,json
from fractions import Fraction as F
from pathlib import Path
HERE=Path(__file__).resolve().parent
SOURCE=HERE.parent/'production_policy_v1'
def sub(a,b):return tuple(x-y for x,y in zip(a,b))
def cross(a,b):return (a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0])
def dot(a,b):return sum(x*y for x,y in zip(a,b))
def footprints(data,marker):
 nodes={n[0]:tuple(map(F,n[1:])) for n in data['nodes']}
 markers={f['id']:f['marker'] for f in data['facets']};rows=[]
 for f in data['surface']:
  if data['marker_names'][markers[f['facet']]]!=marker:continue
  cells=[c for c in data['cells'] if set(f['nodes']).issubset(c['nodes'])];assert len(cells)==1
  c=cells[0];apex=next(n for n in c['nodes'] if n not in f['nodes']);a,b,d=[nodes[n] for n in f['nodes']]
  n=cross(sub(b,a),sub(d,a));area2=dot(n,n);assert area2>0
  relative=[sub(x,nodes[apex]) for x in (a,b,d)]
  weights=[dot(cross(relative[(j+1)%3],relative[(j+2)%3]),n)/area2 for j in range(3)]
  assert sum(weights)==1
  rows.append({'facet':f['facet'],'cell':c['id'],'face':f['nodes'],'apex':apex,'minimum_barycentric':str(min(weights)),'outside':min(weights)<0})
 return rows
if __name__=='__main__':
 frames=[]
 for ranks in (1,2,4):
  receipt=json.loads((SOURCE/f'mpi{ranks}.json').read_text());assert receipt['status']=='PASS'
  for p in sorted((SOURCE/f'mpi{ranks}').glob('policy_m*_s0.json')):
   sha=hashlib.sha256(p.read_bytes()).hexdigest();assert receipt['artifacts'][p.name]==sha
   rows=footprints(json.loads(p.read_text()),'wall');assert len(rows)==2 and sum(r['outside'] for r in rows)==1
   frames.append({'file':str(p.relative_to(HERE.parent)),'sha256':sha,'rows':rows})
 assert len(frames)==6
 result={'status':'REPRODUCED_ALIGNMENT_DEFECT','scope':'Exact rational normal projection onto each declared planar wall triangle; independently verifies that the published coarsened meshes have one outside footprint. Centroid tilt alone is not the acceptance criterion.','frames':frames,'helper_sha256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest()}
 (HERE/'baseline.json').write_text(json.dumps(result,indent=2)+'\n')
 print('Reproduced six outside-footprint failures from unchanged published meshes.')
