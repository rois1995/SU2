from pathlib import Path
from fractions import Fraction as F
import json,hashlib,math
from itertools import combinations
HERE=Path(__file__).resolve().parent
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def sub(a,b):return tuple(x-y for x,y in zip(a,b))
def dot(a,b):return sum(x*y for x,y in zip(a,b))
def cross(a,b):return (a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0])
def determinant(a,b,c):return dot(a,cross(b,c))
def separated(a,b):
 edges=lambda p:[sub(p[j],p[i]) for i,j in combinations(range(4),2)]
 axes=[cross(sub(p[j],p[i]),sub(p[k],p[i])) for p in (a,b) for i,j,k in combinations(range(4),3)]
 axes += [cross(u,v) for u in edges(a) for v in edges(b)]
 for axis in axes:
  if not any(axis):continue
  u=[dot(p,axis) for p in a];v=[dot(p,axis) for p in b]
  if max(u)<=min(v) or max(v)<=min(u):return True
 return False
receipt=json.loads((HERE/'validation.json').read_text());assert receipt['status']=='PASS'
results={};inputs={}
for n in (1,2,4):
 rows={}
 for shape in ('sphere','bump'):
  for phase in ('original','split','coarsened'):
   path=HERE/('mpi'+str(n))/(shape+'_'+phase+'.json');inputs[str(path.relative_to(HERE))]=sha(path)
   row=json.loads(path.read_text());nodes={v[0]:tuple(F(x) for x in v[1:]) for v in row['nodes']}
   cells=[[nodes[v] for v in c] for c in row['cells']];qualities=[];jacobians=[];volume=F(0)
   for p in cells:
    determinant_value=determinant(sub(p[1],p[0]),sub(p[2],p[0]),sub(p[3],p[0]));assert determinant_value>0
    volume+=determinant_value/6
    lengths={(i,j):math.sqrt(float(dot(sub(p[i],p[j]),sub(p[i],p[j])))) for i,j in combinations(range(4),2)}
    quality=12*(float(determinant_value)/2)**(2./3)/sum(x*x for x in lengths.values())
    jacobian=min(1.,min(math.sqrt(2)*float(determinant_value)/math.prod(lengths[tuple(sorted((i,j)))] for j in range(4) if i!=j) for i in range(4)))
    assert quality>=.20 and jacobian>=.05
    qualities.append(quality);jacobians.append(jacobian)
   pairs=0
   for a,b in combinations(cells,2):assert separated(a,b);pairs+=1
   signed=sum((determinant(nodes[f[0]],nodes[f[1]],nodes[f[2]])/6 for f in row['surface']),F(0));assert signed==volume
   rows[shape+'_'+phase]={'minimum_identity_mean_ratio':min(qualities),'minimum_identity_scaled_jacobian':min(jacobians),'exact_rational_no_interior_overlap_pairs':pairs,'exact_signed_surface_volume_equals_cell_volume':True}
 results[str(n)]=rows
assert results['1']==results['2']==results['4']
out={'status':'PASS','scope':'Independent identity-metric shape/sliver, exact rational tetrahedron separating-axis no-interior-overlap and signed-skin closure checks on actual private sphere/bump original/split/coarsened proposals. Complements controller skin/node/projection/inverse checks. Does not prove actual CFD target length, global geometry clearance, distributed curved adaptation or solution conservation.','controller_receipt_sha256':sha(HERE/'validation.json'),'script_sha256':sha(Path(__file__)),'inputs_sha256':inputs,'results':results}
(HERE/'private_shape_validation.json').write_text(json.dumps(out,indent=2)+'\n');print('PASS: private curved grids have positive volume, identity shape/sliver gates, exact signed closure and no interior overlap on MPI1/2/4 artifacts')
