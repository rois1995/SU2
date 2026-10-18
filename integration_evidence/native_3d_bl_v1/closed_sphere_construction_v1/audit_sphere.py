#!/usr/bin/env python3
"""Resumable exact geometry/height audit; complete native full edges remain native evidence."""
import ast,collections,csv,hashlib,itertools,json,math,sys,time
from fractions import Fraction as F
from pathlib import Path
import numpy as np
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2];CASE=HERE/(sys.argv[1] if len(sys.argv)>1 else 'geodesic_v1')
sys.path.insert(0,str(ROOT/'integration_evidence/native_3d_core_v1'))
sys.path.insert(0,str(ROOT/'integration_evidence/native_3d_curved_v1/primitive_audit_v1'))
from audit_cavities import determinant,sub,dot,cross,overlap
from audit_coupled import surface,connected,spd,quad,sources
# The historical script executes its CLI at import; load only its existing exact triangle helper.
primitive_source=ROOT/'integration_evidence/native_3d_curved_v1/primitive_audit_v1/audit_primitives.py'
minimum=next(n for n in ast.parse(primitive_source.read_text()).body if isinstance(n,ast.FunctionDef) and n.name=='minimum_square')
exec(compile(ast.Module(body=[minimum],type_ignores=[]),str(primitive_source),'exec'))

def sha(p):
 h=hashlib.sha256()
 with p.open('rb') as f:
  for b in iter(lambda:f.read(2**20),b''):h.update(b)
 return h.hexdigest()
def read_patch(p):
 words=iter(p.read_text().split());assert next(words)=='NATIVE_PRIVATE_PATCH_V1';nc,nf=int(next(words)),int(next(words));nodes={};cells=[]
 for i in range(nc):
  id=int(next(words));ids=[]
  for k in range(4):
   n=int(next(words));x=tuple(F(float(next(words))) for j in range(3));assert n not in nodes or nodes[n]==x;nodes[n]=x;ids.append(n)
  cells.append((id,tuple(ids)))
 faces=[tuple(int(next(words)) for k in range(4)) for i in range(nf)]
 assert next(words,None) is None and len({x[0] for x in cells})==len(cells) and len(set(nodes.values()))==len(nodes)
 return nodes,cells,faces

def main():
 start=time.monotonic();paths=[Path(__file__).resolve(),CASE/'accepted.dat',CASE/'samples.csv',CASE/'edges.csv',CASE/'result.json',CASE/'source.json',CASE/'run.cfg',CASE/'sphere.su2.native_ref',ROOT/'integration_evidence/native_3d_core_v1/audit_cavities.py',ROOT/'integration_evidence/native_3d_core_v1/audit_coupled.py',ROOT/'integration_evidence/native_3d_curved_v1/primitive_audit_v1/audit_primitives.py']
 identity={str(p.relative_to(ROOT)):sha(p) for p in paths}
 state=CASE/'audit_resume.json';saved=json.loads(state.read_text()) if state.exists() else {'identity':identity,'next_cell':0,'pairs':0,'shared_faces':0,'elapsed_seconds':0}
 assert saved['identity']==identity,'Audit inputs/helpers changed; preserve old evidence before a new version'
 native=json.loads((CASE/'result.json').read_text());assert native['valid'] and native['immutable_input'] and native['unchanged_authority'] and native['work']<=int(next(line.split('=',1)[1] for line in (CASE/'run.cfg').read_text().splitlines() if line.startswith('ADAP_NATIVE_MAX_WORK')))
 nodes,cells,physical=read_patch(CASE/'accepted.dat');samples={int(x['cell']):x for x in csv.DictReader((CASE/'samples.csv').open())};assert len(samples)==len(cells)
 original=json.loads((CASE/'source.json').read_text());sources({'original':original});assert all(v[4]==[1,0,0,1,0,1] for d in original for v in d['vertices'])
 uses=collections.defaultdict(list);links=collections.defaultdict(list);tets=[];volume=F(0);qmin=jmin=1.;shape_difference=0.;all_edges=set()
 for index,(id,ids) in enumerate(cells):
  t=[nodes[n] for n in ids];dv=determinant(t);assert dv>0;volume+=dv/6;tets.append(t)
  a,b,c,d=ids
  for f in [(b,c,d),(a,d,c),(a,b,d),(a,c,b)]:uses[tuple(sorted(f))].append((index,(-1)**sum(f[i]>f[j] for i in range(3) for j in range(i+1,3))))
  for i,n in enumerate(ids):links[n].append(tuple(v for j,v in enumerate(ids) if j!=i))
  row=samples[id];m=tuple(F(float(row[k])) for k in ['xx','xy','xz','yy','yz','zz']);dm=spd(m)
  lengths={(i,j):math.sqrt(float(quad(m,sub(t[j],t[i])))) for i,j in itertools.combinations(range(4),2)}
  weighted=float(dv)*math.sqrt(float(dm));q=12*(weighted/2)**(2/3)/sum(v*v for v in lengths.values());jac=min(1.,min(math.sqrt(2)*weighted/math.prod(lengths[tuple(sorted((i,j)))] for j in range(4) if j!=i) for i in range(4)))
  assert q>=.2 and jac>=.05;difference=max(abs(q-float(row['q'])),abs(jac-float(row['j'])));assert difference<2e-10;shape_difference=max(shape_difference,difference);qmin=min(qmin,q);jmin=min(jmin,jac)
  all_edges.update(tuple(sorted((ids[i],ids[j]))) for i,j in itertools.combinations(range(4),2))
 assert all(len(v)==1 or len(v)==2 and sum(x[1] for x in v)==0 for v in uses.values())
 boundary={f for f,v in uses.items() if len(v)==1};assert boundary=={tuple(sorted(f[:3])) for f in physical};surface(list(boundary),True)
 boundary_nodes={n for f in boundary for n in f};adjacency={i:set() for i in range(len(tets))}
 for values in uses.values():
  if len(values)==2:a,b=values[0][0],values[1][0];adjacency[a].add(b);adjacency[b].add(a)
 assert connected(adjacency)
 for n,triangles in links.items():surface(triangles,n not in boundary_nodes)
 height_error=tilt=sag=authority_error=0.
 height_lines=[line for line in (CASE/'run.cfg').read_text().splitlines() if line.startswith('ADAP_BL_FIRST_HEIGHT')]
 height_target=float(height_lines[0].split('=',1)[1].strip(' ()')) if height_lines else None
 for f in boundary:
  index=uses[f][0][0];apex=next(n for n in cells[index][1] if n not in f);a,b,c=[nodes[n] for n in f];normal=cross(sub(b,a),sub(c,a));nn=dot(normal,normal);signed=dot(sub(nodes[apex],a),normal)
  if height_target is not None:
   height=abs(float(signed))/math.sqrt(float(nn));height_error=max(height_error,abs(height-height_target)/height_target)
   relative=[sub(x,nodes[apex]) for x in (a,b,c)];weights=[dot(cross(relative[(j+1)%3],relative[(j+2)%3]),normal)/nn for j in range(3)];assert min(weights)>=0 and sum(weights)==1
   foot=tuple(nodes[apex][i]-signed*normal[i]/nn for i in range(3));center=tuple((a[i]+b[i]+c[i])/3 for i in range(3));tilt=max(tilt,math.sqrt(float(dot(sub(foot,center),sub(foot,center))))/height_target)
  authority_error=max(authority_error,max(abs(math.sqrt(float(dot(x,x)))-1) for x in [a,b,c]));sag=max(sag,1-math.sqrt(float(minimum_square([a,b,c],dot))))
 assert height_error<=1e-10 and authority_error<=1e-12 and sag<=(min(.04,.5*height_target) if height_target is not None else .04)
 edge_rows=list(csv.DictReader((CASE/'edges.csv').open()));keys=[tuple(sorted((int(r['a']),int(r['b'])))) for r in edge_rows]
 assert set(keys)==all_edges and len(keys)==len(set(keys));maximum=max(float(x['length']) for x in edge_rows);assert maximum<=1.8
 if height_target is None:
  assert json.loads((CASE/'source.json').read_text())==json.loads((HERE/'geodesic_v1/source.json').read_text()),'Changed frozen original source'
  for row in samples.values():assert all(abs(float(row[k])-v)<1e-14 for k,v in zip(['xx','xy','xz','yy','yz','zz'],[1,0,0,1,0,1]))
  for row in edge_rows:
   a,b=nodes[int(row['a'])],nodes[int(row['b'])];expected=math.sqrt(float(dot(sub(a,b),sub(a,b))));assert abs(expected-float(row['length']))<2e-12
 print('PASS primitive geometry, all vertex links/height/authority/shape and complete native full-edge coverage; checking exact overlap pairs',flush=True)
 boxes=np.array([[[min(float(p[k]) for p in t),max(float(p[k]) for p in t)] for k in range(3)] for t in tets]);pairs=saved['pairs'];shared=saved['shared_faces'];ids=[set(c[1]) for c in cells]
 for i in range(saved['next_cell'],len(tets)):
  mask=np.all((boxes[i,:,1]>boxes[:, :,0])&(boxes[:, :,1]>boxes[i,:,0]),axis=1);mask[:i+1]=False
  for j in np.flatnonzero(mask):
   if len(ids[i]&ids[j])==3:shared+=1;continue  # Opposite exact face orientations already prove disjoint interiors.
   assert not overlap(tets[i],tets[j]),('Positive-volume overlap',i,int(j));pairs+=1
  if i%32==0 or i+1==len(tets):
   state.write_text(json.dumps({'identity':identity,'next_cell':i+1,'pairs':pairs,'shared_faces':shared,'elapsed_seconds':saved['elapsed_seconds']+time.monotonic()-start},indent=2)+'\n')
   if time.monotonic()-start>180:print('CHECKPOINT partial exact overlap audit',i+1,len(tets),pairs,flush=True);return 75
 report={'status':'PASS_NATIVE_ACCEPTED_SPHERE_INDEPENDENT_GEOMETRY_HEIGHT_AND_SHAPE' if height_target is not None else 'PASS_NATIVE_ACCEPTED_RELEASE_INDEPENDENT_GEOMETRY_AND_COMPLETE_SENSOR_FIELD','cells':len(cells),'nodes':len(nodes),'wall_faces':len(boundary),'vertex_links':len(links),'positive_volume':str(volume),'pair_candidates_exact':pairs,'shared_faces_certified':shared,'qmin':qmin,'jmin':jmin,'shape_arithmetic_residual':shape_difference,'height_target':height_target,'height_relative_error':height_error if height_target is not None else None,'centroid_tilt':tilt,'sphere_node_error':authority_error,'maximum_surface_sag':sag,'maximum_native_full_edge':maximum,'edges':len(edge_rows),'elapsed_seconds':saved['elapsed_seconds']+time.monotonic()-start,'identity':identity,'scope':'Exact geometry/topology/no positive overlap/normal footprints, sphere geometry and independent exported-tensor q/J arithmetic. Full composed edges are native measurements; no independent composed tensor/edge oracle, CFD transfer/restart, MPI or affordability proof.'}
 (CASE/'independent.json').write_text(json.dumps(report,indent=2)+'\n');print(report['status'],flush=True)
if __name__=='__main__':raise SystemExit(main())
