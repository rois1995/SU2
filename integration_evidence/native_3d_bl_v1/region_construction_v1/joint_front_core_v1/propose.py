"""Whole high-ratio transition/core proposal; not native acceptance or a production mode."""
import collections,csv,json,math,shutil,sys,time
from pathlib import Path
import numpy as np
from scipy.integrate import quad
from scipy.optimize import brentq
HERE=Path(__file__).resolve().parent;sys.path.insert(0,str(HERE.parent))
from audit_full_metric import CylinderField,integrate
from construct import sha,write_su2
start=time.monotonic();source=HERE.parent/'native_front_constant_v1';field=CylinderField(source,'core_construction_rejected.json')
data=json.loads((source/'core_construction_rejected.json').read_text());nodes={n[0]:np.array(n[1:],dtype=float) for n in data['nodes']}
assert np.array_equal(field.axis,[0,0,1]) and np.array_equal(field.origin,[0,0,0])
markers={f['id']:data['marker_names'][f['marker']] for f in data['facets']};wall=[f for f in data['surface'] if markers[f['facet']]=='cylinder'];wall_nodes={n for f in wall for n in f['nodes']}
apices={n for c in data['cells'] if sum(i in wall_nodes for i in c['nodes'])==3 for n in c['nodes'] if n not in wall_nodes}
first=[c['nodes'] for c in data['cells'] if any(n in apices for n in c['nodes'])]
assert len(first)==840 and len(apices)==len(wall)==84
used={n for c in first for n in c};radii={n:np.linalg.norm(nodes[n][:2]) for n in used};r0=min(radii.values())
inner_nodes={n for n in used if abs(radii[n]-r0)<1e-12};assert len(inner_nodes)==len(wall_nodes)
first_map={}
for n in wall_nodes:
 wanted=nodes[n].copy();wanted[:2]*=r0/np.linalg.norm(wanted[:2]);m=min(inner_nodes,key=lambda i:np.linalg.norm(nodes[i]-wanted))
 assert np.linalg.norm(nodes[m]-wanted)<1e-12;first_map[n]=m
faces=[dict(f,nodes=[first_map[n] for n in f['nodes']]) for f in wall]
angular=len(inner_nodes)//2;assert angular*2==len(inner_nodes)
chart={n:(int(round((math.atan2(nodes[n][1],nodes[n][0])%(2*math.pi))*angular/(2*math.pi)))%angular,float(nodes[n][2])) for n in inner_nodes}
ordered=[];arrows={}
for face in faces:
 indices=[chart[n][0] for n in face['nodes']];different=sorted(set(indices));assert len(different)==2
 start_index=angular-1 if different==[0,angular-1] else different[0]
 order=sorted(face['nodes'],key=lambda n:((chart[n][0]-start_index)%angular,chart[n][1],n));ordered.append(order)
 for i in range(3):
  for j in range(i+1,3):
   key=tuple(sorted((order[i],order[j])));pair=(order[i],order[j])
   if key in arrows:assert arrows[key]==pair,'Inconsistent periodic interface diagonal'
   arrows[key]=pair
# Normal-distance profile is proposal design only. Actual tensors and full edges are checked below.
def height(d):return max(field.h,2*(field.h+(field.growth-1)*d)/(field.growth+1))
def primitive(d):
 half=field.h/2
 if d<=half:return d/field.h
 full_value=.5+(field.growth+1)/(2*(field.growth-1))*math.log((field.h+(field.growth-1)*min(d,field.full))/(field.h+(field.growth-1)*half))
 if d<=field.full:return full_value
 def speed(x):
  t=(x-field.full)/field.width;weight=1-t*t*(3-2*t)
  return max(1,height(x)**(-weight))
 fade=quad(speed,field.full,min(d,field.thickness),epsabs=1e-12,epsrel=1e-12)[0]
 return full_value+fade+max(0,d-field.thickness)
count=8;core_radius=.6;first_d=field.radius-r0;last_d=field.radius-core_radius;s0,s1=primitive(first_d),primitive(last_d)
levels=[r0]+[field.radius-brentq(lambda d:primitive(d)-(s0+(s1-s0)*i/count),first_d,last_d,xtol=1e-14) for i in range(1,count+1)]
cells=list(first);regions=['preserved_first_band']*len(first);current={n:n for n in inner_nodes};next_id=max(nodes)+1
for layer in range(1,count+1):
 phi=-layer*math.pi/angular;c,s=math.cos(phi),math.sin(phi);mapping={}
 for n in sorted(inner_nodes):
  p=nodes[n].copy();r=math.hypot(p[0],p[1]);x,y=p[:2];p[:2]=levels[layer]/r*np.array([c*x-s*y,s*x+c*y])
  mapping[n]=next_id;nodes[next_id]=p;next_id+=1
 for a,b,c in ordered:
  a0,b0,c0=current[a],current[b],current[c];aa,bb,cc=mapping[a],mapping[b],mapping[c]
  for tet in ([a0,b0,c0,cc],[a0,b0,cc,bb],[a0,bb,cc,aa]):
   p=np.array([nodes[n] for n in tet]);v=np.linalg.det(p[1:]-p[0]);assert v!=0
   if v<0:tet[0],tet[1]=tet[1],tet[0]
   cells.append(tet);regions.append('transition_'+str(layer))
 current=mapping
# Close the metric-planned core and retain the true physical cap planes.
core=next_id;nodes[core]=np.array([0.,0.,field.z1/2]);next_id+=1
skin=[dict(f,nodes=[current[n] for n in f['nodes']]) for f in faces]
roots={name:min(f['id'] for f in data['facets'] if markers[f['id']]==name) for name in data['marker_names']}
links=collections.defaultdict(list)
for f in skin:
 for i in range(3):links[tuple(sorted((f['nodes'][i],f['nodes'][(i+1)%3])))].append((f['nodes'][i],f['nodes'][(i+1)%3]))
for z,name in ((0.,'cap0'),(field.z1,'cap1')):
 center=next_id;nodes[center]=np.array([0.,0.,z]);next_id+=1
 for uses in links.values():
  if len(uses)!=1:continue
  a,b=uses[0]
  if abs(nodes[a][2]-z)<1e-12 and abs(nodes[b][2]-z)<1e-12:skin.append({'nodes':[center,b,a],'facet':roots[name]})
for face in skin:
 tet=[core]+face['nodes'];p=np.array([nodes[n] for n in tet]);v=np.linalg.det(p[1:]-p[0]);assert v!=0
 if v<0:tet[0],tet[1]=tet[1],tet[0]
 cells.append(tet);regions.append('core')
uses=collections.defaultdict(list)
for cell in cells:
 a,b,c,d=cell
 for face in ((b,c,d),(a,d,c),(a,b,d),(a,c,b)):
  sign=(-1)**sum(face[i]>face[j] for i in range(3) for j in range(i+1,3));uses[tuple(sorted(face))].append((face,sign))
assert all(len(v)==1 or len(v)==2 and sum(x[1] for x in v)==0 for v in uses.values()),'Nonconforming whole candidate'
surface=[]
for records in uses.values():
 if len(records)!=1:continue
 face=records[0][0]
 if all(abs(np.linalg.norm(nodes[n][:2])-field.radius)<1e-12 for n in face):facet=roots['cylinder']
 elif max(abs(nodes[n][2]) for n in face)<1e-12:facet=roots['cap0']
 else:assert max(abs(nodes[n][2]-field.z1) for n in face)<1e-12;facet=roots['cap1']
 surface.append({'nodes':face,'facet':facet})
p=np.array([[nodes[n] for n in c] for c in cells]);m=field.batch(p.mean(axis=1));e=p[:,None,:,:]-p[:,:,None,:]
squared=np.einsum('nabi,nij,nabj->nab',e,m,e);weighted=np.linalg.det(p[:,1:]-p[:,:1])*np.sqrt(np.linalg.det(m));i,j=np.triu_indices(4,1)
q=12*(weighted/2)**(2/3)/squared[:,i,j].sum(axis=1)
products=np.array([np.prod(np.delete(squared[:,k,:],k,axis=1),axis=1) for k in range(4)]).T
jac=np.min(math.sqrt(2)*weighted[:,None]/np.sqrt(products),axis=1)
region_report={name:{'cells':regions.count(name),'minimum_q':float(q[np.array(regions)==name].min()),'minimum_j':float(jac[np.array(regions)==name].min())} for name in sorted(set(regions))}
pairs=sorted({tuple(sorted((c[i],c[j]))) for c in cells for i in range(4) for j in range(i+1,4)});a=np.array([nodes[x] for x,y in pairs]);b=np.array([nodes[y] for x,y in pairs]);d=b-a
m=field.batch((a+b)/2);proxy=np.sqrt(np.einsum('ni,nij,nj->n',d,m,d));full=[]
for k in np.argsort(proxy)[-24:][::-1]:
 x,y=pairs[k];value,error,calls=integrate(field,nodes[x],nodes[y]);full.append({'edge':[x,y],'length':value,'indicator':error,'evaluations':calls})
status='COMPLETE_PROPOSAL_POINT_SHAPE_AND_CRITICAL_EDGES_ONLY' if q.min()>=.2 and jac.min()>=.05 and max(r['length'] for r in full)<=1.8 else 'REJECTED_POINT_SHAPE_OR_CRITICAL_FULL_EDGE'
used={n for c in cells for n in c};out=dict(data,nodes=[[n]+nodes[n].tolist() for n in sorted(used)],cells=[{'id':i,'nodes':c,'region':regions[i]} for i,c in enumerate(cells)],surface=surface)
(HERE/'candidate.json').write_text(json.dumps(out,separators=(',',':'))+'\n');name='candidate_rejected.su2' if status.startswith('REJECTED') else 'candidate_proposal.su2'
write_su2(HERE/name,nodes,cells,surface,{f['id']:f['marker'] for f in data['facets']},data['marker_names']);shutil.copyfile(source/'core_initial.su2.native_ref',HERE/(name+'.native_ref'))
report={'status':status,'cells':len(cells),'free_fronts':count,'core_radius':core_radius,'radii':levels,'phase_per_front_radians':-math.pi/angular,'periodic_diagonal_arrows_verified':len(arrows),
        'minimum_q':float(q.min()),'minimum_j':float(jac.min()),'regions':region_report,'critical_full_edges':full,'recorded_surface_work':642,'cells_plus_recorded_surface_work':len(cells)+642,
        'input_sha256':sha(source/'core_construction_rejected.json'),'candidate_sha256':sha(HERE/'candidate.json'),'helper_sha256':sha(Path(__file__)),'field_helper_sha256':sha(HERE.parent/'audit_full_metric.py'),'elapsed_seconds':time.monotonic()-start,
        'scope':'Whole high-ratio proposal, preserved native first-height band, metric-spaced staggered periodic fronts and deeper core interface, consistent3-tet transitions and one core fan. Actual-point tensors and all centroid shapes plus24 critical full edges measured. No complete embedding/all-edge/height/native/work/time/MPI/CFD acceptance claim.'}
(HERE/'result.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))
