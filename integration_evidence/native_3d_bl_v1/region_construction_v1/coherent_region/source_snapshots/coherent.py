#!/usr/bin/env python3
"""Whole-region wall/edge-dual construction with a closed transition shell; research only."""
import collections,csv,hashlib,json,math,time
from pathlib import Path
import numpy as np
from design import Design
from construct import HERE,ROOT,SOURCE,MAX_EDGE,sha,normal,authorities,project,refine_surface,affine_flow,topology,write_su2

def band(nodes,faces,inner,apices,physical_outer,side_authority,design):
 cells=[];surface=[];roles=[];edges=collections.defaultdict(list)
 for i,f in enumerate(faces):
  a,b,c=f['nodes'];p=apices[i]
  cells.extend([[p,a,b,c],[p,inner[c],inner[b],inner[a]]]);roles.extend(['outer','inner'])
  if physical_outer(f):surface.append(f)
  for a,b in [(a,b),(b,c),(c,a)]:edges[tuple(sorted((a,b)))].append((i,a,b))
 for key,uses in sorted(edges.items()):
  i,a,b=uses[0];p=apices[i];cycle=[a,inner[a],inner[b],b]
  if len(uses)==2:
   j,x,y=uses[1];assert (x,y)==(b,a)
   q=apices[j]
   # Triangulate the complete shared bipyramid; a nonconvex dual edge is not admissible.
   options=[[[p,q,cycle[k],cycle[(k+1)%4]] for k in range(4)]]
   for tris in [[(0,1,2),(0,2,3)],[(0,1,3),(1,2,3)]]:
    fs=[[cycle[k] for k in t] for t in tris]
    options.append([[p]+f for f in fs]+[[q]+list(reversed(f)) for f in fs])
   scored=[(design.score(option,nodes),k,option) for k,option in enumerate(options)]
   score,k,chosen=max(scored,key=lambda row:row[0]);assert np.isfinite(score),'No positive shared-interface triangulation'
   design.margins.append(score);design.patterns[k]+=1
   cells.extend(chosen);roles.extend(['interface_'+str(k)]*len(chosen))
  else:
   assert len(uses)==1 and key in side_authority
   facet=side_authority[key];q=max(nodes)+1;nodes[q]=project(np.mean([nodes[n] for n in cycle],axis=0),side_authority['geometry'][facet])
   for k in range(4):
    face=[q,cycle[k],cycle[(k+1)%4]];cells.append([p]+face);roles.append('junction');surface.append(dict(facet=facet,nodes=face))
 return cells,surface,roles

def distance(p,g):
 kind,origin,extent,_,axis=g;v=p-origin
 if kind==1:return abs(np.linalg.norm(v)-extent[0])
 assert kind==3
 axis=axis/np.linalg.norm(axis);z=v@axis;r=np.linalg.norm(v-z*axis)
 return math.hypot(r-extent[0],max(0.,-z,z-extent[1]))

def main():
 start=time.monotonic();case=HERE/'coherent_region';case.mkdir(exist_ok=False)
 data=json.loads((SOURCE/'audited_grid.json').read_text());receipt=json.loads(SOURCE.with_suffix('.json').read_text())
 for name in ['audited_grid.json','grid_edges.csv','curved_cylinder_initial_euler.su2.native_ref']:assert sha(SOURCE/name)==receipt['artifacts'][name]
 geometry,names=authorities(SOURCE/'curved_cylinder_initial_euler.su2.native_ref');marker={f['id']:f['marker'] for f in data['facets']}
 # Declared unchanged experiment policy; no production Euler/BL dispatch exists here.
 h=.05;growth=1.2;thickness=.2;walls={f for f in marker if names[marker[f]]=='cylinder'}
 faces=data['surface'];boundary={n for f in faces for n in f['nodes']};nodes={int(n[0]):np.array(n[1:],float) for n in data['nodes'] if n[0] in boundary}
 physical={tuple(sorted((f['nodes'][i],f['nodes'][(i+1)%3]))) for f in faces for i in range(3)}
 marked={tuple(sorted((int(r['a']),int(r['b'])))) for r in csv.DictReader((SOURCE/'grid_edges.csv').open()) if float(r['length'])>MAX_EDGE}&physical
 faces,splits=refine_surface(nodes,faces,marked,geometry)
 requested=.9*MAX_EDGE;depth=h*(.5*(growth+1)*math.exp((requested-.5)*2*(growth-1)/(growth+1))-1)/(growth-1)
 transform,shift=affine_flow(nodes,faces,walls,depth)
 old={int(n[0]):np.array(n[1:],float) for n in data['nodes']};volume=0.;weighted=np.zeros(3)
 for c in data['cells']:
  p=np.array([old[n] for n in c['nodes']]);v=np.linalg.det(p[1:]-p[0])/6;assert v>0;volume+=v;weighted+=v*p.mean(axis=0)
 kernel=weighted/volume
 assert all(normal(np.array([nodes[n] for n in f['nodes']]))@(kernel-nodes[f['nodes'][0]])<0 for f in faces)
 front={n:max(nodes)+1+k for k,n in enumerate(sorted(nodes))};mapped={front[n]:transform@p+shift for n,p in nodes.items()}
 incident=collections.defaultdict(list)
 for f in faces:
  if f['facet'] not in walls:
   for n in f['nodes']:incident[n].append(f['facet'])
 for n,charts in incident.items():
  for chart in charts:mapped[front[n]]=project(mapped[front[n]],geometry[chart])
 nodes.update(mapped);kernel=transform@kernel+shift
 wall_faces=[f for f in faces if f['facet'] in walls];apices=[];first=[]
 design=Design(geometry[next(iter(walls))],h,growth,thickness)
 for f in wall_faces:
  p=np.array([nodes[n] for n in f['nodes']]);apex=max(nodes)+1;nodes[apex]=p.mean(axis=0)-h*normal(p);apices.append(apex);first.append(dict(face=f['nodes'],apex=apex))
 edge_uses=collections.defaultdict(list)
 for f in faces:
  a=f['nodes']
  for i in range(3):edge_uses[tuple(sorted((a[i],a[(i+1)%3])))].append((f,a[i],a[(i+1)%3]))
 side={'geometry':geometry};rim=collections.defaultdict(list)
 for key,uses in edge_uses.items():
  assert len(uses)==2
  wall=[r for r in uses if r[0]['facet'] in walls];other=[r for r in uses if r[0]['facet'] not in walls]
  if len(wall)==1:
   assert len(other)==1;f,a,b=other[0];side[key]=f['facet'];rim[marker[f['facet']]].append((front[a],front[b],f['facet']))
 cells,surface,roles=band(nodes,wall_faces,front,apices,lambda f:True,side,design);regions=['wall_'+r for r in roles]
 inner_faces=[dict(facet=f['facet'],nodes=[front[n] for n in f['nodes']]) for f in wall_faces]
 # Coarsen each planar nonwall cap as a region, rather than retaining old tiny cap triangles.
 for m,loop in sorted(rim.items()):
  outgoing={a:b for a,b,_ in loop};incoming={b:a for a,b,_ in loop};assert len(outgoing)==len(loop)==len(incoming)
  at=min(outgoing);walk=[]
  for _ in loop:walk.append(at);at=outgoing[at]
  assert at==walk[0] and len(set(walk))==len(loop),'Only one closed cap rim is admitted in this experiment'
  facet=loop[0][2];center=max(nodes)+1;nodes[center]=project(kernel,geometry[facet])
  for a,b,f in loop:inner_faces.append(dict(facet=f,nodes=[center,a,b]))
  n=normal(np.array([nodes[n] for n in inner_faces[-1]['nodes']]))
  assert all(np.cross(nodes[b]-nodes[a],nodes[center]-nodes[a])@n>0 for a,b,_ in loop),'Cap kernel is outside its finite polygon'
 # Continue only the wall band; preserve cap planes instead of creating thin isotropic cap layers.
 unique={n for f in inner_faces for n in f['nodes']}
 trial,delta=affine_flow(nodes,inner_faces,walls,thickness);direction=trial-np.eye(3)
 wall_geometry=[geometry[f] for f in sorted(walls)]
 def clearance(step):return min(distance(nodes[n]+step*(direction@nodes[n]+delta),g) for n in unique for g in wall_geometry)
 assert clearance(1)>thickness*(1+1e-10)
 lo,hi=0.,1.
 for _ in range(48):
  t=.5*(lo+hi)
  if clearance(t)>=thickness*(1+1e-10):hi=t
  else:lo=t
 step=hi;last={n:max(nodes)+1+k for k,n in enumerate(sorted(unique))}
 mapped={last[n]:nodes[n]+step*(direction@nodes[n]+delta) for n in unique}
 for m,loop in rim.items():
  for a,b,facet in loop:
   mapped[last[a]]=project(mapped[last[a]],geometry[facet]);mapped[last[b]]=project(mapped[last[b]],geometry[facet])
 nodes.update(mapped);kernel=kernel+step*(direction@kernel+delta)
 transition=[];transition_faces=[f for f in inner_faces if f['facet'] in walls]
 for f in transition_faces:
  apex=max(nodes)+1;nodes[apex]=.5*(np.mean([nodes[n] for n in f['nodes']],axis=0)+np.mean([nodes[last[n]] for n in f['nodes']],axis=0));transition.append(apex)
 side2={'geometry':geometry}
 for m,loop in rim.items():
  for a,b,facet in loop:side2[tuple(sorted((a,b)))]=facet
 more,skin,part=band(nodes,transition_faces,last,transition,lambda f:False,side2,design)
 cells.extend(more);surface.extend(skin);regions.extend('transition_'+r for r in part)
 core=max(nodes)+1;nodes[core]=kernel
 final_faces=[dict(facet=f['facet'],nodes=[last[n] for n in f['nodes']]) for f in transition_faces]
 for m,loop in rim.items():
  facet=loop[0][2];center=max(nodes)+1;nodes[center]=project(kernel,geometry[facet])
  for a,b,facet in loop:final_faces.append(dict(facet=facet,nodes=[center,last[a],last[b]]))
 for f in final_faces:
  cells.append([core]+f['nodes']);regions.append('core')
  if f['facet'] not in walls:surface.append(f)
 topology(cells,surface)
 determinants=np.array([np.linalg.det(np.array([nodes[n] for n in c])[1:]-nodes[c[0]]) for c in cells])
 output=case/'candidate_rejected.su2';write_su2(output,nodes,cells,surface,marker,names)
 candidate=dict(nodes=[[n]+list(map(float,p)) for n,p in sorted(nodes.items())],cells=[dict(id=i,nodes=c,region=regions[i]) for i,c in enumerate(cells)],surface=surface,facets=data['facets'],facet_groups=data['facet_groups'],marker_names=names,original=data['original'],first_rows=first)
 (case/'candidate.json').write_text(json.dumps(candidate,separators=(',',':'))+'\n')
 report=dict(status='GEOMETRY_PROPOSAL_ONLY' if np.all(determinants>0) else 'REJECTED_NONPOSITIVE_GEOMETRY',cells=len(cells),wall_faces=len(first),surface_faces=len(surface),surface_splits=splits,minimum_determinant=float(determinants.min()),nonpositive=int(np.sum(determinants<=0)),work_proxy=len(cells)+splits,transition_step=step,minimum_core_wall_distance=clearance(step),interface_patterns=dict(design.patterns),minimum_proxy_interface_margin=min(design.margins),proxy_below_guard=int(sum(x<1 for x in design.margins)),depth=depth,candidate_sha256=sha(output),input_sha256=sha(SOURCE/'audited_grid.json'),input_receipt_sha256=sha(SOURCE.with_suffix('.json')),helper_sha256=sha(Path(__file__)),proxy_sha256=sha(HERE/'design.py'),shared_helper_sha256=sha(HERE/'construct.py'),elapsed_seconds=time.monotonic()-start,scope='Whole actual target-boundary reconstruction, paired-apex edge duals, regional cap coarsening and closed transition. Positive/oriented topology is not full geometry or metric acceptance. Work proxy is generated cells plus surface edits, not equivalent to old local-operator work counters. No production integration, MPI/CFD or performance qualification.')
 (case/'construction.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))
if __name__=='__main__':main()
