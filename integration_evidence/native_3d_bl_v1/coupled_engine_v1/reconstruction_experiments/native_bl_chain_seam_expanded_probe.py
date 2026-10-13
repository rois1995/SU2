"""Bounded thought experiment: exact oriented-face closure with full analytic BL gates."""
exec(open('integration_evidence/native_3d_bl_v1/coupled_engine_v1/reconstruction_experiments/native_bl_flip_probe.py').read().split('pts[200000]=')[0])
import numpy as np
from scipy.optimize import milp,Bounds,LinearConstraint
from scipy.sparse import coo_matrix
from collections import Counter

def faces(c):
 a,b,c,d=c;return [(b,c,d),(a,d,c),(a,b,d),(a,c,b)]
def sign(f):return (-1)**sum(f[i]>f[j] for i in range(3) for j in range(i+1,3))
old=[row for row in v['cells'] if any(x in row['nodes'] for x in [15,31,47]) or all(x in row['nodes'] for x in [0,4])]
chain=Counter()
for row in old:
 for f in faces(row['nodes']):chain[tuple(sorted(f))]+=sign(f)
skin={f:s for f,s in chain.items() if s}
# Split oriented skin, preserving the exact boundary chain.
newskin={}
for f,s in skin.items():
 tri=list(f)
 if s<0:tri[0],tri[1]=tri[1],tri[0]
 if 0 in f and 4 in f:
  for id in [0,4]:
   child=[200000 if x==id else x for x in tri];newskin[tuple(sorted(child))]=sign(child)
 else:newskin[f]=s
# Coordinated seam insertion and new surface diagonals on each original facet.
for seam,upper,newid in [(31,5,200001),(47,7,200002)]:
 removed=[f for f in newskin if set(f) in [{200000,4,seam},{4,seam,upper}]]
 assert len(removed)==2
 oriented=[]
 for f in removed:
  tri=list(f)
  if newskin.pop(f)<0:tri[0],tri[1]=tri[1],tri[0]
  oriented.append(tri)
 boundary=Counter()
 for tri in oriented:
  for a,b in zip(tri,tri[1:]+tri[:1]):boundary[(a,b)]+=1
 for a,b in list(boundary):
  count=min(boundary[(a,b)],boundary[(b,a)]);boundary[(a,b)]-=count;boundary[(b,a)]-=count
 border={a:b for (a,b),count in boundary.items() if count}
 loop=[200000]
 while len(loop)<4:loop.append(border[loop[-1]])
 assert loop.index(upper)==2
 for tri in [[loop[0],loop[1],newid],[loop[1],loop[2],newid],[loop[0],newid,loop[3]]]:newskin[tuple(sorted(tri))]=sign(tri)
 # Split the second incident marker triangle on that seam.
 for f in list(newskin):
  if seam not in f or upper not in f:continue
  tri=list(f)
  if newskin.pop(f)<0:tri[0],tri[1]=tri[1],tri[0]
  for end in [seam,upper]:
   child=[newid if x==end else x for x in tri];newskin[tuple(sorted(child))]=sign(child)
ids=sorted(set(x for row in old for x in row['nodes'])|{200000,200001,200002})
gl,gw=np.polynomial.legendre.leggauss(24)
def length(a,b):
 chord=np.array(b)-a;dz=chord[2];cuts=[0.,1.]
 if dz:
  cuts += [(z-a[2])/dz for z in [.05,.27,.3] if 0<(z-a[2])/dz<1]
 total=0
 for lo,hi in zip(sorted(cuts),sorted(cuts)[1:]):
  t=(lo+hi)/2+(hi-lo)/2*gl;z=a[2]+dz*t;u=np.clip((z-.27)/.03,0,1);w=1-u*u*(3-2*u);tang=1.5**w;h=np.maximum(.1,2*(.1+.2*z)/2.2);n=h**(-2*w)
  q=(1+tang)/2*(chord[0]**2+chord[1]**2)+(1-tang)*chord[0]*chord[1]+n*dz**2
  total+=(hi-lo)/2*np.dot(gw,np.sqrt(q))
 return float(total)
reports=[]
for z in [.16873350527066577,.2,.25]:
 pts[200001]=[.5,0,.5];pts[200002]=[0,.5,.5]
 pts[200000]=[0,0,z];lengths={e:length(pts[e[0]],pts[e[1]]) for e in itertools.combinations(ids,2)}
 candidates=[]
 for c in itertools.combinations(ids,4):
  if any(lengths[e]>1.8+1e-12 for e in itertools.combinations(c,2)):continue
  q,j,det=measure(c)
  if q<.2 or j<.05:continue
  points=[pts[x] for x in c]
  orientation=dot(sub(points[1],points[0]),cross(sub(points[2],points[0]),sub(points[3],points[0])))
  if orientation<0:c=(c[1],c[0],c[2],c[3])
  # No physical-boundary child can cross the preserved original marker triangle.
  if any(tuple(sorted(f)) in newskin and sign(f)!=newskin[tuple(sorted(f))] for f in faces(c)):continue
  candidates.append(c)
 faceids=sorted(set(newskin)|{tuple(sorted(f)) for c in candidates for f in faces(c)});indices={f:i for i,f in enumerate(faceids)}
 rows=[];cols=[];values=[]
 for i,c in enumerate(candidates):
  for f in faces(c):rows.append(indices[tuple(sorted(f))]);cols.append(i);values.append(sign(f))
 matrix=coo_matrix((values,(rows,cols)),shape=(len(faceids),len(candidates))).tocsc();rhs=np.array([newskin.get(f,0) for f in faceids])
 if len(candidates):
  result=milp(np.ones(len(candidates)),integrality=np.ones(len(candidates)),bounds=Bounds(0,1),constraints=LinearConstraint(matrix,rhs,rhs),options={'time_limit':10})
  chosen=[] if result.x is None else [list(c) for c,x in zip(candidates,result.x) if x>.5]
  status=result.message
 else:chosen=[];status='No admissible candidates'
 report={'split_z':z,'patch_cells':len(old),'points':len(ids),'candidates':len(candidates),'status':status,'replacement':chosen};reports.append(report);print({k:val for k,val in report.items() if k!='replacement'},'chosen',len(chosen),flush=True)
Path('integration_evidence/native_3d_bl_v1/coupled_engine_v1/reconstruction_experiments/native_bl_chain_seam_expanded_probe.json').write_text(json.dumps({'scope':'Thought experiment only; floating placement and MILP, not native acceptance or proof of infeasibility for larger patches/insertions','reports':reports},indent=2)+'\n')
