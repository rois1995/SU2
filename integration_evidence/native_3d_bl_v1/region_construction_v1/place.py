#!/usr/bin/env python3
"""Plan metric-aware junction centers for the whole construction; exact native revalidation is mandatory."""
import collections,csv,json,math,shutil,time
from pathlib import Path
import numpy as np
from scipy.optimize import minimize
from construct import HERE,SOURCE,sha,write_su2,normal,topology
from assess_apices import shape

def main():
 start=time.monotonic();parent=HERE/'surface_region';case=HERE/'metric_region';case.mkdir(exist_ok=False)
 data=json.loads((parent/'candidate.json').read_text());nodes={n[0]:np.array(n[1:],float) for n in data['nodes']}
 audit=json.loads((parent/'native_field.json').read_text());assert audit['status']=='REJECTED_NATIVE_METRIC'
 for name,h in audit['artifacts'].items():assert sha(parent/'native_field'/name)==h
 exported=json.loads((parent/'native_field/audited_grid.json').read_text());exported_nodes={n[0]:tuple(n[1:]) for n in exported['nodes']};exported_cells={c['id']:c for c in exported['cells']}
 samples={int(r['cell']):r for r in csv.DictReader((parent/'native_field/grid_samples.csv').open())}
 groups=collections.defaultdict(list);points_by_id={}
 for c in data['cells']:
  actual=exported_cells[c['id']];assert sorted(tuple(nodes[n]) for n in c['nodes'])==sorted(exported_nodes[n] for n in actual['nodes'])
  if c['region'].endswith('junction'):groups[c['nodes'][1]].append(c)
 proposed={};results=[];total_evaluations=0
 for center,cells in groups.items():
  assert len(cells)==4
  edges={c['nodes'][2]:c['nodes'][3] for c in cells};assert len(edges)==4
  at=min(edges);cycle=[]
  for _ in range(4):cycle.append(at);at=edges[at]
  assert at==cycle[0]
  quad=np.array([nodes[n] for n in cycle]);n=normal(quad[:3]);u=quad[1]-quad[0];u/=np.linalg.norm(u);v=np.cross(n,u);basis=np.stack([u,v],axis=1);origin=nodes[center]
  points=np.array([[nodes[k] for k in c['nodes']] for c in cells]);metrics=[]
  for c in cells:
   s=samples[c['id']];xx,xy,xz,yy,yz,zz=[float(s[k]) for k in ('xx','xy','xz','yy','yz','zz')];metrics.append([[xx,xy,xz],[xy,yy,yz],[xz,yz,zz]])
  metrics=np.array(metrics)
  def values(x):
   p=points.copy();p[:,1,:]=origin+basis@x[:2];return shape(p,metrics)
  def constraints(x):
   q,j,det=values(x);position=origin+basis@x[:2]
   inside=[np.cross(quad[(k+1)%4]-quad[k],position-quad[k])@n for k in range(4)]
   return np.r_[q-.22*x[2],j-.055*x[2],det,inside]
  q,j,det=values(np.zeros(3))
  for k,c in enumerate(cells):
   s=samples[c['id']];assert abs(q[k]-float(s['q']))<1e-10 and abs(j[k]-float(s['j']))<1e-10
  initial=min(q.min()/.22,j.min()/.055)
  fit=minimize(lambda x:-x[2]+1e-5*(x[0]*x[0]+x[1]*x[1]),[0,0,float(initial)],method='SLSQP',constraints={'type':'ineq','fun':constraints},bounds=[(None,None),(None,None),(0,3)],options={'maxiter':100,'ftol':1e-10})
  q,j,det=values(fit.x);assert det.min()>0
  p=origin+basis@fit.x[:2];assert abs((p-origin)@n)<1e-12
  assert all(np.cross(quad[(k+1)%4]-quad[k],p-quad[k])@n>=-1e-12 for k in range(4))
  proposed[center]=p;total_evaluations+=int(fit.nfev)
  results.append(dict(point=center,initial_proxy_margin=float(initial),planned_proxy_margin=float(min(q.min()/.22,j.min()/.055)),success=bool(fit.success),iterations=int(fit.nit),evaluations=int(fit.nfev),displacement=float(np.linalg.norm(p-origin))))
 nodes.update(proposed);cells=[c['nodes'] for c in data['cells']];topology(cells,data['surface'])
 determinants=np.array([np.linalg.det(np.array([nodes[n] for n in c])[1:]-nodes[c[0]]) for c in cells]);assert np.all(determinants>0)
 data['nodes']=[[n]+list(map(float,p)) for n,p in sorted(nodes.items())]
 marker={f['id']:f['marker'] for f in data['facets']};output=case/'candidate_rejected.su2';write_su2(output,nodes,cells,data['surface'],marker,data['marker_names'])
 (case/'candidate.json').write_text(json.dumps(data,separators=(',',':'))+'\n')
 shutil.copyfile(SOURCE/'curved_cylinder_initial_euler.su2.native_ref',case/'candidate_rejected.su2.native_ref')
 report=dict(status='PLACEMENT_PROPOSAL_ONLY',cells=len(cells),nonpositive=0,minimum_determinant=float(determinants.min()),points=len(proposed),maximum_displacement=max(r['displacement'] for r in results),minimum_planned_proxy_margin=min(r['planned_proxy_margin'] for r in results),optimizer_evaluations=total_evaluations,parent_construction_sha256=sha(parent/'construction.json'),parent_native_audit_sha256=sha(parent/'native_field.json'),helper_sha256=sha(Path(__file__)),shape_helper_sha256=sha(HERE/'assess_apices.py'),writer_sha256=sha(HERE/'construct.py'),candidate_sha256=sha(output),elapsed_seconds=time.monotonic()-start,results=results,scope='All regional junction centers planned within their physical cap quadrilaterals using actual previous native tensors as fixed design proxies. Connectivity, geometry authority, original sensors/BL/fade and prescribed wall height unchanged. This is not native acceptance; all new actual centroids and full edges must be re-queried. No production/MPI/CFD or speedup claim.')
 (case/'construction.json').write_text(json.dumps(report,indent=2)+'\n');print({k:v for k,v in report.items() if k!='results'})
if __name__=='__main__':main()
