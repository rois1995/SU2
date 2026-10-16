"""Whole saved-mesh transition replacement; topology/point shape proposal, not acceptance."""
import collections,json,math,sys,time
from pathlib import Path
import numpy as np
HERE=Path(__file__).resolve().parent;sys.path.insert(0,str(HERE.parent))
from audit_full_metric import CylinderField,integrate
from construct import sha,write_su2
start=time.monotonic();case=HERE.parent/'native_core_rotated_v10';field=CylinderField(case)
data=json.loads((case/'core_accepted.json').read_text());nodes={r[0]:np.array(r[1:]) for r in data['nodes']}
wall=[f for f in data['surface'] if data['marker_names'][next(g['marker'] for g in data['facets'] if g['id']==f['facet'])]=='cylinder']
radii={i:np.linalg.norm(p-field.origin-((p-field.origin)@field.axis)*field.axis) for i,p in nodes.items()}
counts=collections.Counter(round(r,8) for r in radii.values());front_radii=sorted((r for r,count in counts.items() if count==len({n for f in wall for n in f['nodes']})),reverse=True)
assert len(front_radii)==3 and front_radii[0]==1
fronts=[{n for n,r in radii.items() if abs(r-radius)<1e-8} for radius in front_radii]
def mapping(outer,inner):
 result={}
 for n in outer:
  p=nodes[n]-field.origin;z=p@field.axis;radial=p-z*field.axis
  wanted=field.origin+z*field.axis+front_radii[1 if outer==fronts[0] else 2]*radial/np.linalg.norm(radial)
  target=min(inner,key=lambda i:np.linalg.norm(nodes[i]-wanted));assert np.linalg.norm(nodes[target]-wanted)<1e-8;result[n]=target
 assert len(set(result.values()))==len(result);return result
first=mapping(fronts[0],fronts[1]);last=mapping(fronts[1],fronts[2])
# Identify existing band cells from their actual face-apex vertices, not cell positions.
first_apices={n for c in data['cells'] if sum(i in fronts[0] for i in c['nodes'])==3 for n in c['nodes'] if n not in fronts[0]}
transition_apices={n for c in data['cells'] if sum(i in fronts[1] for i in c['nodes'])==3 and not any(i in first_apices for i in c['nodes']) for n in c['nodes'] if n not in fronts[1]}
assert len(first_apices)==len(wall) and len(transition_apices)==len(wall)
retained=[c['nodes'] for c in data['cells'] if not any(n in transition_apices for n in c['nodes'])]
mid={n:max(nodes)+1+i for i,n in enumerate(sorted(fronts[1]))}
for n in mid:
 nodes[mid[n]]=(nodes[n]+nodes[last[n]])/2
 p=nodes[mid[n]]-field.origin;radii[mid[n]]=np.linalg.norm(p-(p@field.axis)*field.axis)
proposal=[]
for face in wall:
 a,b,c=sorted(first[n] for n in face['nodes'])
 for mapping,outer in ((mid,(a,b,c)),(last,(a,b,c))):
  if mapping is mid:
   a0,b0,c0=a,b,c;aa,bb,cc=mid[a],mid[b],mid[c]
  else:
   a0,b0,c0=mid[a],mid[b],mid[c];aa,bb,cc=last[a],last[b],last[c]
  for tet in ([a0,b0,c0,cc],[a0,b0,cc,bb],[a0,bb,cc,aa]):
   points=np.array([nodes[n] for n in tet]);det=np.linalg.det(points[1:]-points[0]);assert det!=0
   if det<0:tet[0],tet[1]=tet[1],tet[0]
   proposal.append(tet)
cells=retained+proposal
uses=collections.defaultdict(list)
for cell in cells:
 a,b,c,d=cell
 for face in ((b,c,d),(a,d,c),(a,b,d),(a,c,b)):
  sign=(-1)**sum(face[i]>face[j] for i in range(3) for j in range(i+1,3));uses[tuple(sorted(face))].append((face,sign))
assert all(len(v)==1 or len(v)==2 and sum(x[1] for x in v)==0 for v in uses.values()),'Nonconforming replacement'
surface=[]
roots={name:min(f['id'] for f in data['facets'] if data['marker_names'][f['marker']]==name) for name in data['marker_names']}
for rows in uses.values():
 if len(rows)!=1:continue
 face=rows[0][0];z=[(nodes[n]-field.origin)@field.axis for n in face]
 if all(abs(radii[n]-field.radius)<1e-12 for n in face):facet=roots['cylinder']
 elif max(abs(x) for x in z)<1e-12:facet=roots['cap0']
 else:assert max(abs(x-field.z1) for x in z)<1e-12;facet=roots['cap1']
 surface.append({'nodes':face,'facet':facet})
points=np.array([[nodes[n] for n in c] for c in cells]);m=field.batch(points.mean(axis=1));edges=points[:,None,:,:]-points[:,:,None,:]
squared=np.einsum('nabi,nij,nabj->nab',edges,m,edges);weighted=np.linalg.det(points[:,1:]-points[:,:1])*np.sqrt(np.linalg.det(m));i,j=np.triu_indices(4,1)
q=12*(weighted/2)**(2/3)/squared[:,i,j].sum(axis=1)
products=np.array([np.prod(np.delete(squared[:,k,:],k,axis=1),axis=1) for k in range(4)]).T
jac=np.min(math.sqrt(2)*weighted[:,None]/np.sqrt(products),axis=1)
new_edges={tuple(sorted((c[i],c[j]))) for c in proposal for i in range(4) for j in range(i+1,4)}
pairs=sorted(new_edges);midpoints=np.array([(nodes[a]+nodes[b])/2 for a,b in pairs]);directions=np.array([nodes[b]-nodes[a] for a,b in pairs]);metrics=field.batch(midpoints)
proxy=np.sqrt(np.einsum('ni,nij,nj->n',directions,metrics,directions));selected=np.argsort(proxy)[-8:][::-1];full=[]
for k in selected:
 a,b=pairs[k];value,error,calls=integrate(field,nodes[a],nodes[b]);full.append({'edge':[a,b],'length':value,'indicator':error,'evaluations':calls})
used={n for c in cells for n in c};out=dict(data,nodes=[[n]+nodes[n].tolist() for n in sorted(used)],cells=[{'id':i,'nodes':c} for i,c in enumerate(cells)],surface=surface)
(HERE/'candidate.json').write_text(json.dumps(out,separators=(',',':'))+'\n');write_su2(HERE/'candidate_proposal.su2',nodes,cells,surface,{f['id']:f['marker'] for f in data['facets']},data['marker_names'])
report=dict(status='WHOLE_CONNECTIVITY_AND_SHAPE_PROPOSAL_ONLY',old_cells=len(data['cells']),new_cells=len(cells),transition_tets=len(proposal),removed_transition_tets=len(data['cells'])-len(retained),minimum_q=float(q.min()),minimum_j=float(jac.min()),long_edge_probes=full,helper_sha256=sha(Path(__file__)),field_helper_sha256=sha(HERE.parent/'audit_full_metric.py'),input_sha256=sha(case/'core_accepted.json'),candidate_sha256=sha(HERE/'candidate.json'),elapsed_seconds=time.monotonic()-start,
 scope='Saved constant-sensor curved cylinder; complete first-row/core retained, free transition replaced by two steps of consistent ordered three-tet prisms. Oriented face conformity and actual-query centroid q/J measured; eight longest midpoint-proxy edges receive full independent integrals. No complete embedding/full-edge/height audit, production acceptance, MPI, CFD, speedup or high-ratio construction claim.')
(HERE/'result.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))
