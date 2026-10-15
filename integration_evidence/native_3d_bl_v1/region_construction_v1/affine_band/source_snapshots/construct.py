#!/usr/bin/env python3
"""Whole-region affine-band construction experiment; never a production remeshing fallback."""
import collections,csv,hashlib,json,math,shlex,time
from pathlib import Path
import numpy as np
HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[2]
SOURCE=ROOT/'integration_evidence/native_3d_bl_v1/wall_reconstruction_v1/curved_metric_complete'
MAX_EDGE=1.8

def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def normal(p):
 n=np.cross(p[1]-p[0],p[2]-p[0]);length=np.linalg.norm(n)
 if not length>0:raise ValueError('Degenerate surface face')
 return n/length

def authorities(path):
 lines=path.read_text().splitlines();assert lines[0]=='SU2_NATIVE_REFERENCE_3D 3'
 markers,facets,count=map(int,lines[1].split());names=[shlex.split(s)[0] for s in lines[2:2+markers]]
 result={}
 for line in lines[2+markers+facets:]:
  v=line.split();assert len(v)==12
  result[int(v[0])]=(int(v[1]),np.array(v[2:5],float),np.array(v[5:8],float),float(v[8]),np.array(v[9:12],float))
 assert len(result)==count
 return result,names

def project(p,g):
 kind,origin,extent,tolerance,axis=g;axis=axis/np.linalg.norm(axis);v=p-origin
 if kind==1:return origin+extent[0]*v/np.linalg.norm(v)
 if kind in (3,4):
  z=v@axis;radial=v-z*axis
  if kind==3:return origin+z*axis+extent[0]*radial/np.linalg.norm(radial)
  return p-z*axis
 raise ValueError('Unsupported authority in geometric experiment')

def refine_surface(nodes,faces,marked,geometry):
 splits=0
 for edge in sorted(marked):
  incident=[(k,f) for k,f in enumerate(faces) if set(edge)<=set(f['nodes'])]
  if not incident:continue
  assert len(incident)==2,'Physical surface is not closed/manifold'
  p=.5*(nodes[edge[0]]+nodes[edge[1]])
  charts=sorted({f['facet'] for _,f in incident},key=lambda f:geometry[f][0]!=4)
  for chart in charts:p=project(p,geometry[chart])
  for chart in charts:assert np.linalg.norm(project(p,geometry[chart])-p)<=4*geometry[chart][3]
  mid=max(nodes)+1;nodes[mid]=p
  children=[]
  for _,f in incident:
   a=f['nodes'];at=next(i for i in range(3) if {a[i],a[(i+1)%3]}==set(edge));x,y,z=a[at],a[(at+1)%3],a[(at+2)%3]
   children.extend([dict(facet=f['facet'],nodes=[x,mid,z]),dict(facet=f['facet'],nodes=[mid,y,z])])
  removed={k for k,_ in incident};faces=[f for k,f in enumerate(faces) if k not in removed]+children;splits+=1
 return faces,splits

def affine_flow(nodes,faces,wall_facets,depth):
 rows=[];rhs=[];constraints=[]
 for f in faces:
  p=np.array([nodes[i] for i in f['nodes']]);n=normal(p);weight=np.linalg.norm(np.cross(p[1]-p[0],p[2]-p[0]))**.5
  for point in p:
   row=np.outer(n,np.r_[point,1]).ravel()
   if f['facet'] in wall_facets:rows.append(weight*row);rhs.append(-weight*depth)
   else:constraints.append(row)
 # Preserve nonwall planes exactly; reject unsupported geometry after construction.
 c=np.array(constraints);_,s,v=np.linalg.svd(c,full_matrices=False)
 rank=np.sum(s>1e-11*s[0]);null=v[rank:].T;assert null.shape[1]>0
 coefficients=null@np.linalg.lstsq(np.array(rows)@null,np.array(rhs),rcond=1e-12)[0]
 displacement=coefficients.reshape(3,4);assert np.max(np.abs(c@coefficients))<1e-11
 transform=np.eye(3)+displacement[:,:3];assert np.linalg.det(transform)>0
 return transform,displacement[:,3]

def side_faces(a,b,inner):
 # Shared quadrilateral diagonal depends only on the original global edge identity.
 if a<b:return [(a,inner[a],inner[b]),(a,inner[b],b)]
 return [(a,inner[a],b),(inner[a],inner[b],b)]

def topology(cells,surface):
 uses=collections.defaultdict(list)
 for c in cells:
  a,b,d,e=c
  for face in [(b,d,e),(a,e,d),(a,b,e),(a,d,b)]:
   sign=(-1)**sum(face[i]>face[j] for i in range(3) for j in range(i+1,3));uses[tuple(sorted(face))].append(sign)
 assert all(len(v)==1 or len(v)==2 and sum(v)==0 for v in uses.values()),'Nonmanifold/inconsistent cell faces'
 boundary={f for f,v in uses.items() if len(v)==1};assert boundary=={tuple(sorted(f['nodes'])) for f in surface}
 assert len(boundary)==len(surface),'Duplicate physical faces'

def write_su2(path,nodes,cells,surface,facet_marker,names):
 used=sorted({n for c in cells for n in c});ids={n:i for i,n in enumerate(used)}
 with path.open('w') as out:
  out.write('NDIME= 3\nNELEM= %d\n'%len(cells))
  for c in cells:out.write('10 '+' '.join(str(ids[n]) for n in c)+'\n')
  out.write('NPOIN= %d\n'%len(used))
  for n in used:out.write(' '.join(format(float(x),'.17g') for x in nodes[n])+'\n')
  out.write('NMARK= %d\n'%len(names))
  for k,name in enumerate(names):
   fs=[f for f in surface if facet_marker[f['facet']]==k];out.write('MARKER_TAG= %s\nMARKER_ELEMS= %d\n'%(name,len(fs)))
   for f in fs:out.write('5 '+' '.join(str(ids[n]) for n in f['nodes'])+'\n')

def main():
 start=time.monotonic();case=HERE/'affine_band';case.mkdir(exist_ok=False)
 data=json.loads((SOURCE/'audited_grid.json').read_text());receipt=json.loads(SOURCE.with_suffix('.json').read_text())
 for name in ['audited_grid.json','grid_edges.csv','curved_cylinder_initial_euler.su2.native_ref']:assert sha(SOURCE/name)==receipt['artifacts'][name]
 geometry,names=authorities(SOURCE/'curved_cylinder_initial_euler.su2.native_ref')
 marker={f['id']:f['marker'] for f in data['facets']};wall_facets={f for f in marker if names[marker[f]]=='cylinder'}
 h=.05;growth=1.2;thickness=.2  # Frozen experiment input, identical to the recorded native target.
 # Growth-law normal spacing is proposal design only; native queries remain the final authority.
 requested=.9*MAX_EDGE;depth=h*(.5*(growth+1)*math.exp((requested-.5)*2*(growth-1)/(growth+1))-1)/(growth-1)
 assert h<depth<.9*thickness
 faces=data['surface'];boundary={n for f in faces for n in f['nodes']}
 nodes={int(n[0]):np.array(n[1:],float) for n in data['nodes'] if n[0] in boundary}
 physical={tuple(sorted((f['nodes'][i],f['nodes'][(i+1)%3]))) for f in faces for i in range(3)}
 marked={tuple(sorted((int(r['a']),int(r['b'])))) for r in csv.DictReader((SOURCE/'grid_edges.csv').open()) if float(r['length'])>MAX_EDGE}&physical
 faces,splits=refine_surface(nodes,faces,marked,geometry)
 transform,shift=affine_flow(nodes,faces,wall_facets,depth)
 # A complete star-shaped region is rebuilt; old interior connectivity is not retained.
 original_nodes={int(n[0]):np.array(n[1:],float) for n in data['nodes']};weighted=np.zeros(3);volume=0.
 for c in data['cells']:
  p=np.array([original_nodes[n] for n in c['nodes']]);v=np.linalg.det((p[1:]-p[0]).T)/6;assert v>0
  weighted+=v*p.mean(axis=0);volume+=v
 kernel=weighted/volume
 assert all(normal(np.array([nodes[n] for n in f['nodes']]))@(kernel-nodes[f['nodes'][0]])<0 for f in faces),'Region lacks the certified proposed kernel'
 inner={n:max(nodes)+1+k for k,n in enumerate(sorted(nodes))};mapped={inner[n]:transform@p+shift for n,p in nodes.items()};nodes.update(mapped)
 core=max(nodes)+1;nodes[core]=transform@kernel+shift
 adjacency=collections.defaultdict(list)
 for f in faces:
  for i in range(3):adjacency[tuple(sorted((f['nodes'][i],f['nodes'][(i+1)%3])))].append(f)
 cells=[];surface=[];kind=[];apices=[]
 for f in faces:
  a,b,c=f['nodes'];front=[inner[n] for n in f['nodes']]
  cells.append([core]+front);kind.append('core')
  if f['facet'] not in wall_facets:
   surface.append(dict(facet=f['facet'],nodes=front));continue
  p=np.array([nodes[n] for n in f['nodes']]);apex=max(nodes)+1;nodes[apex]=p.mean(axis=0)-h*normal(p);apices.append(dict(face=f['nodes'],apex=apex))
  skin=[(a,b,c),tuple(reversed(front))]
  for x,y in [(a,b),(b,c),(c,a)]:
   side=side_faces(x,y,inner);skin.extend(side)
   other=[g for g in adjacency[tuple(sorted((x,y)))] if g is not f];assert len(other)==1
   if other[0]['facet'] not in wall_facets:
    surface.extend(dict(facet=other[0]['facet'],nodes=list(t)) for t in side)
  cells.extend([[apex]+list(t) for t in skin]);kind.extend(['wall_band']*len(skin));surface.append(f)
 determinants=np.array([np.linalg.det((np.array([nodes[n] for n in c])[1:]-nodes[c[0]]).T) for c in cells])
 topology(cells,surface)
 output=case/'candidate_rejected.su2';write_su2(output,nodes,cells,surface,marker,names)
 candidate=dict(nodes=[[n]+list(map(float,p)) for n,p in sorted(nodes.items())],cells=[dict(id=i,nodes=c,region=kind[i]) for i,c in enumerate(cells)],surface=surface,facets=data['facets'],facet_groups=data['facet_groups'],marker_names=names,original=data['original'],first_rows=apices)
 (case/'candidate.json').write_text(json.dumps(candidate,separators=(',',':'))+'\n')
 report=dict(status='GEOMETRY_PROPOSAL_ONLY' if np.all(determinants>0) else 'REJECTED_NONPOSITIVE_GEOMETRY',cells=len(cells),wall_faces=len(apices),surface_faces=len(surface),surface_splits=splits,minimum_determinant=float(determinants.min()),nonpositive=int(np.sum(determinants<=0)),depth=depth,transform=transform.tolist(),shift=shift.tolist(),input_sha256=sha(SOURCE/'audited_grid.json'),input_receipt_sha256=sha(SOURCE.with_suffix('.json')),helper_sha256=sha(Path(__file__)),candidate_sha256=sha(output),elapsed_seconds=time.monotonic()-start,scope='Complete actual1402-cell-boundary region proposal only. No production fallback, accepted mesh, native field/height/geometry acceptance, MPI, CFD or speedup claim. Original sensor/BL/fade/geometry/height/q/J/full-length targets unchanged. Old interior connectivity replaced, all physical boundaries participate.')
 (case/'construction.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))
if __name__=='__main__':main()
