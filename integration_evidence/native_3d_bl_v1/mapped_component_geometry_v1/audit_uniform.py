#!/usr/bin/env python3
"""Independent rational finite-support/embedding and affine-state audit; uniform-target controls only."""
import collections,copy,hashlib,importlib.util,itertools,json,math,shlex,sys,time
from fractions import Fraction as F
from pathlib import Path
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2]
spec=importlib.util.spec_from_file_location('annular_auditor',HERE.with_name('planar_symmetry_reference_v1')/'audit_annulus.py')
house=importlib.util.module_from_spec(spec);spec.loader.exec_module(house);base=house.base

def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def q(x):return F(float(x))
def face_rows(ids):
 a,b,c,d=ids;return [(b,c,d),(a,d,c),(a,b,d),(a,c,b)]
def parse(path):
 lines=iter(path.read_text().splitlines());assert next(lines) in ('SU2_NATIVE_REFERENCE_3D 5','SU2_NATIVE_REFERENCE_3D 6')
 nm,nf,na,np,ne,nv,ns,version,revision=map(int,next(lines).split());assert na==0
 names=[shlex.split(next(lines))[0] for _ in range(nm)];facets={};original={}
 for _ in range(nf):
  row=next(lines).split();id,marker=map(int,row[:2]);vertices=[]
  for k in range(3):
   n=int(row[2+4*k]);p=tuple(map(q,row[3+4*k:6+4*k]));assert n not in original or original[n]==p;original[n]=p;vertices.append(n)
  assert id not in facets;facets[id]=(marker,vertices)
 planes={}
 for _ in range(np):
  row=next(lines).split();planes[int(row[0])]=(tuple(map(q,row[4:7])),tuple(map(q,row[7:10])),q(row[10]))
 errors={}
 for _ in range(ne):row=next(lines).split();errors[int(row[0])]=q(row[1])
 points={}
 for _ in range(nv):
  row=next(lines).split();n=int(row[0]);assert n not in points and int(row[5])==version
  points[n]=(tuple(map(q,row[1:4])),int(row[4]),int(row[6]),q(row[7]),q(row[8]))
 skin=[list(map(int,next(lines).split())) for _ in range(ns)];assert not list(lines)
 return names,facets,original,planes,errors,points,skin

def geometry(data,path):
 started=time.monotonic();names,facets,original,planes,errors,records,skin=parse(path)
 assert not planes,'This independent uniform oracle does not qualify declared affine-plane footprints'
 assert names==data['marker_names'];nodes={r[0]:tuple(map(q,r[1:])) for r in data['nodes']};assert len(nodes)==len(data['nodes'])==len(set(nodes.values()))
 cells=[c['nodes'] for c in data['cells']];assert 0<len(cells)<=4096
 tets=[[nodes[n] for n in c] for c in cells];volumes=[base.determinant(t)/6 for t in tets];assert min(volumes)>0
 incidence=collections.defaultdict(list);links=collections.defaultdict(list)
 for c in cells:
  for f in face_rows(c):incidence[tuple(sorted(f))].append(base.parity(f))
  for n in c:links[n].append([i for i in c if i!=n])
 assert all(len(v)==1 or len(v)==2 and sum(v)==0 for v in incidence.values())
 boundary={f:v[0] for f,v in incidence.items() if len(v)==1};assert {n for f in boundary for n in f}==records.keys()
 physical={tuple(sorted(r[:3])):(r[:3],r[3]) for r in skin};assert len(physical)==len(skin) and physical.keys()==boundary.keys()
 assert {tuple(sorted(f['nodes'])):f['facet'] for f in data['surface']}=={k:v[1] for k,v in physical.items()}
 border_nodes={n for f in boundary for n in f}
 for n,triangles in links.items():base.surface(triangles,n not in border_nodes)
 edges=sorted({tuple(sorted((v[k],v[(k+1)%3]))) for _,v in facets.values() for k in range(3)})
 # Independently discover exact planar components; never consume native component IDs as proof.
 parents={i:i for i in facets}
 def component(i):
  while parents[i]!=i:i=parents[i]
  return i
 if path.read_text().splitlines()[0].endswith(' 6'):
  shared=collections.defaultdict(list)
  for i,(_,ids) in facets.items():
   for a,b in zip(ids,ids[1:]+ids[:1]):shared[tuple(sorted((a,b)))].append(i)
  for uses in shared.values():
   assert len(uses)==2
   a,b=uses
   if facets[a][0]!=facets[b][0]:continue
   first=[original[n] for n in facets[a][1]]
   if all(base.determinant(first+[original[n]])==0 for n in facets[b][1]):
    x,y=component(a),component(b);parents[max(x,y)]=min(x,y)
 members=collections.defaultdict(list)
 for i in facets:members[component(i)].append(i)
 ideal={};supports={};maximum_error=F(0)
 for n,(actual,feature,dim,u,v) in records.items():
  assert actual==nodes[n]
  if dim==0:assert u==v==0;coefficients={feature:F(1)}
  elif dim==1:assert 0<u<1 and v==0;a,b=edges[feature];coefficients={a:1-u,b:u}
  else:
   assert dim==2 and u>0 and v>0 and u+v<1;a,b,c=facets[feature][1];coefficients={a:1-u-v,b:u,c:v}
  incident=[i for i,(_,ids) in facets.items() if set(coefficients)<=set(ids)];assert incident
  p=tuple(sum(w*original[i][k] for i,w in coefficients.items()) for k in range(3));err=sum(abs(a-b) for a,b in zip(actual,p))
  assert err<=min(errors.get(j,F(0)) for i in incident for j in members[component(i)]);maximum_error=max(maximum_error,err);ideal[n]=p;supports[n]=coefficients
  for i in incident:
   if i in planes:
    origin,normal,limit=planes[i];residual=base.dot(normal,base.sub(actual,origin));assert residual**2<=limit**2*base.dot(normal,normal)
 groups=collections.defaultdict(list);actual_skin_volume=ideal_skin_volume=F(0)
 original_triangles={};axes={}
 for root,ids in members.items():
  first=[original[n] for n in facets[root][1]];normal=base.cross(base.sub(first[1],first[0]),base.sub(first[2],first[0]))
  axis=max(range(3),key=lambda k:abs(normal[k]));axes[root]=axis
  project=lambda p:tuple(p[k] for k in range(3) if k!=axis)
  original_triangles[root]=[list(map(project,(original[n] for n in facets[i][1]))) for i in ids]
 for key,(face,facet) in physical.items():
  assert base.parity(face)==boundary[key];assert facet in members
  axis=axes[facet];project=lambda p:tuple(p[k] for k in range(3) if k!=axis)
  flat=[]
  for n in face:
   assert any(set(supports[n])<=set(facets[i][1]) for i in members[facet]);flat.append(project(ideal[n]))
  direction=1 if house.twice_area(original_triangles[facet][0])>0 else -1
  assert direction*house.twice_area(flat)>0
  if direction<0:flat=list(reversed(flat))
  groups[facet].append(flat)
  p=[nodes[n] for n in face];r=[original[n] for n in facets[facet][1]]
  assert base.dot(base.cross(base.sub(p[1],p[0]),base.sub(p[2],p[0])),base.cross(base.sub(r[1],r[0]),base.sub(r[2],r[0])))>0
  actual_skin_volume+=base.determinant([(F(0),)*3]+p)/6;ideal_skin_volume+=base.determinant([(F(0),)*3]+[ideal[n] for n in face])/6
 assert groups.keys()==members.keys()
 for root,triangles in groups.items():
  originals=[t if house.twice_area(t)>0 else list(reversed(t)) for t in original_triangles[root]]
  assert sum(map(house.twice_area,triangles))==sum(map(house.twice_area,originals))
  for triangle in triangles:
   # Exact clipped area verifies complete finite containment, including holes and nonconvex borders.
   assert sum(house.intersection_area(triangle,t) for t in originals)==house.twice_area(triangle)
  for a,b in itertools.combinations(triangles,2):assert house.intersection_area(a,b)==0
 original_volume=sum(base.determinant([(F(0),)*3]+[original[n] for n in ids])/6 for _,ids in facets.values())
 assert sum(volumes)==actual_skin_volume and ideal_skin_volume==original_volume and original_volume>0
 pairs=0
 for i,a in enumerate(tets):
  for b in tets[i+1:]:
   if any(max(p[k] for p in a)<=min(p[k] for p in b) or max(p[k] for p in b)<=min(p[k] for p in a) for k in range(3)):continue
   assert not base.overlap(a,b);pairs+=1
  assert time.monotonic()-started<120,'Bounded independent audit exhausted'
 minimum_q=minimum_j=1.;maximum_length=0.
 for t in tets:
  determinant=float(base.determinant(t));lengths={(a,b):4*float(base.dot(base.sub(t[b],t[a]),base.sub(t[b],t[a]))) for a,b in itertools.combinations(range(4),2)}
  scaled=8*determinant;qvalue=12*(scaled/2)**(2/3)/sum(lengths.values());j=min(math.sqrt(2)*scaled/math.prod(math.sqrt(lengths[tuple(sorted((a,b)))]) for b in range(4) if a!=b) for a in range(4))
  assert qvalue>=.2 and j>=.05;minimum_q=min(minimum_q,qvalue);minimum_j=min(minimum_j,j);maximum_length=max(maximum_length,math.sqrt(max(lengths.values())))
 assert maximum_length<=1.8+1e-12
 return dict(cells=len(cells),nodes=len(nodes),exact_overlap_pairs=pairs,finite_ideal_components=len(groups),maximum_mapping_L1_error=float(maximum_error),volume=float(sum(volumes)),original_volume=float(original_volume),relative_volume_change=float(abs(sum(volumes)-original_volume)/original_volume),minimum_quality=minimum_q,minimum_scaled_jacobian=minimum_j,maximum_full_edge=maximum_length,audit_seconds=time.monotonic()-started)

def main():
 folder=Path(sys.argv[1]).resolve();result=json.loads((folder/'result.json').read_text());assert result['exit_code']==0
 assert result['log_sha256']==sha(folder/'run.log') and result['provenance_sha256']==sha(folder/'provenance.json')
 assert all(sha(folder/p)==h for p,h in result['artifacts'].items());reports=[]
 for physics in ('euler','rans'):
  stem='mapped_runtime_'+physics;states=json.loads((folder/(stem+'_states.json')).read_text());maximum=0.
  for row in states['accepted']:
   x,y,z=row['point'];expected=[1.2+.1*x-.05*y+.02*z,400+20*x-10*y+5*z,-30+5*x+8*y-3*z,12-4*x+2*y+6*z,2.5e5+1e3*x-2e3*y+500*z]
   if physics=='rans':expected[1:4]=[20*z,3*z,2*z]
   for level,factor in enumerate((1.,1.,.96875)):
    u=row['states'][5*level:5*level+5];assert u[0]>0 and u[4]-sum(v*v for v in u[1:4])/(2*u[0])>0
    maximum=max(maximum,max(abs(a-factor*b)/max(1.,abs(factor*b)) for a,b in zip(u,expected)))
  assert maximum<2e-9
  for fmt,ext in [('SU2','.su2'),('CGNS','.cgns')]:
   path=folder/(stem+'_written_'+fmt+'_reload.json');data=json.loads(path.read_text());reference=folder/(stem+'_written_'+fmt+ext+'.native_ref')
   report=geometry(data,reference);report.update(physics=physics,format=fmt,affine_state_residual=maximum);reports.append(report)
   if physics=='euler' and fmt=='SU2':
    for defect in ('inverted_cell','missing_face','changed_boundary_point'):
     bad=copy.deepcopy(data)
     if defect=='inverted_cell':bad['cells'][0]['nodes'][0],bad['cells'][0]['nodes'][1]=bad['cells'][0]['nodes'][1],bad['cells'][0]['nodes'][0]
     elif defect=='missing_face':bad['surface'].pop()
     else:
      target=bad['surface'][0]['nodes'][0];next(n for n in bad['nodes'] if n[0]==target)[1]+=.001
     try:geometry(bad,reference)
     except AssertionError:pass
     else:raise RuntimeError('Independent audit missed '+defect)

 report=dict(status='INDEPENDENT_MAPPED_COMPONENT_UNIFORM_GEOMETRY_STATE_PASS',cases=reports,negative_controls=['inverted_cell','missing_face','changed_boundary_point'],helpers={str(Path(__file__).resolve().relative_to(ROOT)):sha(Path(__file__)),str(Path(house.__file__).relative_to(ROOT)):sha(Path(house.__file__)),str(Path(base.__file__).relative_to(ROOT)):sha(Path(base.__file__))},native_result_sha256=sha(folder/'result.json'),scope='Exact planar finite-component cover by independent rational clipping, mapping bounds, realized orientation, rational volume/overlap/topology, uniform4I metric and physical affine current/BDF2 flow. No declared affine-plane footprints, BL, SA independent conservation, real solution-file restart,2D or exterior readiness.')
 output=HERE/'audits';output.mkdir(exist_ok=True);(output/(folder.name+'_uniform.json')).write_text(json.dumps(report,indent=2)+'\n');print(report['status'],len(reports))
if __name__=='__main__':main()
