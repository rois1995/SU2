#!/usr/bin/env python3
"""Geometry-only thought experiment: retain wall connectivity and fit shared apices."""
import collections,csv,hashlib,itertools,json,os,sys
os.environ['OPENBLAS_NUM_THREADS']='1'
from pathlib import Path
import numpy as np
HERE=Path(__file__).resolve().parent

def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def faces(v):
 a,b,c,d=v;return [(b,c,d),(a,d,c),(a,b,d),(a,c,b)]
def volume(c,n):
 a,b,d,e=[n[i] for i in c];return float(np.linalg.det(np.array([b-a,d-a,e-a]))/6)
def fit(origin,triangles,height):
 normals=[];rhs=[]
 for a,b,c in triangles:
  n=np.cross(b-a,c-a);n/=np.linalg.norm(n);normals.append(n);rhs.append(float(n@a-height))
 A=np.array(normals);rhs=np.array(rhs);_,singular,vt=np.linalg.svd(A,full_matrices=True)
 rank=sum(singular>1e-12);delta=np.linalg.lstsq(A,rhs-A@origin,rcond=1e-12)[0];p=origin+delta
 residual=float(np.max(np.abs(A@p-rhs))/height)
 if residual>1e-10:return None,dict(rank=int(rank),height_residual=residual,foot_feasible=False)
 null=vt[rank:].T;G=[];offset=[]
 for (a,b,c),normal in zip(triangles,normals):
  inverse=np.linalg.pinv(np.array([b-a,c-a]).T);uv=inverse@(p+height*normal-a)
  gradients=inverse@null
  offset.extend([1-sum(uv),*uv]);G.extend([-sum(gradients),*gradients])
 G=np.array(G);offset=np.array(offset);d=null.shape[1];proposals=[np.zeros(d)]
 for i in range(len(offset)):
  square=float(G[i]@G[i])
  if square>1e-24:proposals.append(-offset[i]*G[i]/square)
 if d==2:
  for i,j in itertools.combinations(range(len(offset)),2):
   rows=G[[i,j]]
   if abs(np.linalg.det(rows))>1e-12:proposals.append(np.linalg.solve(rows,-offset[[i,j]]))
 feasible=[q for q in proposals if np.min(offset+G@q)>=-1e-10]
 if not feasible:return None,dict(rank=int(rank),height_residual=residual,foot_feasible=False)
 q=min(feasible,key=lambda x:float(x@x));point=p+null@q
 return point,dict(rank=int(rank),height_residual=residual,foot_feasible=True,minimum_barycentric=float(np.min(offset+G@q)),displacement=float(np.linalg.norm(point-origin)))

def main():
 z=np.array([[0.,0.,1.],[1.,0.,1.],[1.,1.,1.]])
 x=np.array([[1.,0.,1.],[1.,0.,0.],[1.,1.,1.]])
 point,control=fit(np.array([.9,.3,.8]),[z,x],.05)
 assert point is not None and np.max(np.abs(point-np.array([.95,.3,.95])))<1e-12
 assert fit(np.array([.9,.3,.8]),[z,z-np.array([0.,0.,1.])],.05)[0] is None
 stage=sys.argv[1] if len(sys.argv)>1 else 'curved_costed_complete'
 assert stage in ('curved_costed_complete','curved_feature_residual')
 parent=HERE/stage;rp=HERE/(stage+'.json');receipt=json.loads(rp.read_text());assert receipt['status'] in ('PASS','FAIL')
 for name in ['audited_grid.json','grid_edges.csv']:assert sha(parent/name)==receipt['artifacts'][name]
 data=json.loads((parent/'audited_grid.json').read_text());nodes={r[0]:np.array(r[1:],dtype=float) for r in data['nodes']}
 cells=[c['nodes'] for c in data['cells']];surface=data['surface'];facets={f['id']:data['marker_names'][f['marker']] for f in data['facets']}
 physical=collections.defaultdict(set)
 for f in surface:
  for a,b in itertools.combinations(f['nodes'],2):physical[tuple(sorted((a,b)))].add(facets[f['facet']])
 candidates=[(int(r['a']),int(r['b']),float(r['length'])) for r in csv.DictReader((parent/'grid_edges.csv').open()) if float(r['length'])>1.8 and physical[tuple(sorted((int(r['a']),int(r['b']))))]=={'cylinder'}]
 def owners(rows):
  out=collections.defaultdict(list)
  for row in rows:
   for f in faces(row):out[tuple(sorted(f))].append(next(n for n in row if n not in f))
  return out
 original=owners(cells);rows=[];height=.05
 for a,b,length in candidates:
  point=(nodes[a]+nodes[b])/2;point[:2]/=np.linalg.norm(point[:2]);inserted=max(nodes)+1
  proposed=dict(nodes);proposed[inserted]=point
  split=[]
  for c in cells:
   if a in c and b in c:
    split.extend([[inserted if n==a else n for n in c],[inserted if n==b else n for n in c]])
   else:split.append(c)
  boundary=[];affected=set()
  for f in surface:
   if a in f['nodes'] and b in f['nodes']:
    if facets[f['facet']]=='cylinder':
     owner=original[tuple(sorted(f['nodes']))];assert len(owner)==1;affected.add(owner[0])
    for endpoint in (a,b):boundary.append(dict(f,nodes=[inserted if n==endpoint else n for n in f['nodes']]))
   else:boundary.append(f)
  new=owners(split);fits=[];valid=True
  for apex in sorted(affected):
   triangles=[]
   for f in boundary:
    if facets[f['facet']]!='cylinder':continue
    owner=new[tuple(sorted(f['nodes']))];assert len(owner)==1
    if owner[0]==apex:triangles.append([proposed[n] for n in f['nodes']])
   assert triangles
   fitted,diagnosis=fit(nodes[apex],triangles,height);diagnosis.update(apex=apex,wall_constraints=len(triangles));fits.append(diagnosis)
   if fitted is None:valid=False
   else:proposed[apex]=fitted
  changed=[c for c in split if inserted in c or any(n in affected for n in c)]
  minimum=min(volume(c,proposed) for c in changed);positive=minimum>0
  rows.append(dict(edge=[a,b],input_native_length=length,apices=fits,height_foot_fit=valid,positive_changed_cells=positive,changed_cells=len(changed),minimum_signed_volume=minimum,
      geometry_candidate_only=valid and positive))
 out=HERE/('curved_wall_plane_probe' if stage=='curved_costed_complete' else stage+'_wall_plane_probe');out.mkdir(exist_ok=True)
 report=dict(status='GEOMETRY_ONLY_THOUGHT_EXPERIMENT',input_cells=len(cells),wall_edges=len(rows),height_foot_fits=sum(r['height_foot_fit'] for r in rows),positive_candidates=sum(r['geometry_candidate_only'] for r in rows),rows=rows,
  receipt_sha256=sha(rp),input_sha256={p:sha(parent/p) for p in ['audited_grid.json','grid_edges.csv']},helper_sha256=sha(Path(__file__)),
  scope='Source-pinned actual residual named by its receipt, all oversized pure-cylinder edges. Geometric midpoint projected to the unit cylinder, not native metric-balanced placement. Double-precision plane/foot feasibility and signed volumes only: no native/exact geometric acceptance, overlap/clearance, target metric/quality/full-edge certification, MPI, CFD or performance claim. Failed fits diagnose this retained connectivity, not global target infeasibility.')
 (out/'report.json').write_text(json.dumps(report,indent=2)+'\n');print('wall_edges',len(rows),'height_foot_fits',report['height_foot_fits'],'positive_geometry_candidates',report['positive_candidates'])
if __name__=='__main__':main()
