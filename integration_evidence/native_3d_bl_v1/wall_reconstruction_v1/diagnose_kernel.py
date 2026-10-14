#!/usr/bin/env python3
"""Reproduce the saved single-background numerical LP diagnosis; not an exact feasibility certificate."""
import collections,hashlib,json
from pathlib import Path
import numpy as np
from scipy.optimize import linprog
HERE=Path(__file__).resolve().parent
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def faces(v):
 a,b,c,d=v;return [(b,c,d),(a,d,c),(a,b,d),(a,c,b)]
def main():
 d=json.loads((HERE/'dependency_diagnosis.json').read_text())
 matches=[p for p in HERE.glob('sensor*/native.log') if sha(p)==d['log_sha256']];assert len(matches)==1
 log=matches[0];rows=[]
 for line in log.read_text().splitlines():
  r=line.split()
  if r[:2]!=['MESH','finer_grouped_coupled_BL_rejected']:continue
  assert len(r)==28
  rows.append({'id':int(r[2]),'vertices':[[int(r[4+4*i])]+list(map(float,r[5+4*i:8+4*i])) for i in range(4)],'physical':[list(map(int,r[20+2*i:22+2*i])) for i in range(4)]})
 assert len(rows)==d['mesh_cells']
 results=[];point=(36**(2/3)-4)/12
 for label,cells in [('required',d['required_closure']),('combined',[c for c in rows if c['id'] in d['combined_closure_ids']])]:
  nodes={n[0]:np.array(n[1:]) for c in cells for n in c['vertices']};skin={};uses=collections.Counter();physical={}
  for c in cells:
   for f,mark in zip(faces([n[0] for n in c['vertices']]),c['physical']):
    key=tuple(sorted(f));uses[key]+=1;skin[key]=f
    if mark[0]:physical[key]=mark[1]
  skin={key:f for key,f in skin.items() if uses[key]==1};nodes[-1]=np.array([point,point,0.]);changed=[]
  for key,f in skin.items():
   if {0,2}.issubset(f) and key in physical:
    left=list(f);right=list(f);left[left.index(0)]=-1;right[right.index(2)]=-1;changed.extend([(tuple(left),True),(tuple(right),True)])
   else:changed.append((f,key in physical))
  a=[];b=[];wa=[];wb=[]
  def plane(points):
   x,y,z=points;normal=np.cross(y-x,z-x);norm=np.linalg.norm(normal);assert norm>0
   normal/=norm;return [*normal,1.],float(normal@x)
  for f,marked in changed:
   points=[nodes[n] for n in f];v,t=plane(points);a.append(v);b.append(t)
   if marked and all(p[2]==0 for p in points):
    center=sum(points)/3;normal=np.cross(points[1]-points[0],points[2]-points[0]);normal/=np.linalg.norm(normal);w=center-.1*normal
    for j in range(3):
     triangle=list(points);triangle[j]=w;v,t=plane(triangle);wa.append(v);wb.append(t)
  for policy in ('skin_only','wall_children'):
   aa=a+(wa if policy=='wall_children' else []);bb=b+(wb if policy=='wall_children' else [])
   r=linprog([0,0,0,-1],A_ub=aa,b_ub=bb,bounds=[(None,None)]*4,method='highs');assert r.success
   results.append({'closure':label,'policy':policy,'cells':len(cells),'point':r.x[:3].tolist(),'margin':float(r.x[3]),'point_residual':float(min(np.array(bb)-np.array(aa)[:,:3]@r.x[:3])),'constraints':len(aa)})
 assert results[0]['margin']>0 and all(r['margin']<0 for r in results[1:])
 result={'status':'REPRODUCED_NUMERICAL_PLACEMENT_DIAGNOSIS','balanced_wall_split':point,'results':results,'log':str(log.relative_to(HERE)),'log_sha256':sha(log),'helper_sha256':sha(Path(__file__)),'scope':'Floating-point LP over terminal cavities from the preserved failing version. Tests a single background, not arbitrary reconstruction or global metric feasibility; production does not use SciPy.'}
 (HERE/'kernel_geometry_reproduced.json').write_text(json.dumps(result,indent=2)+'\n');print(result['status'])
if __name__=='__main__':main()
