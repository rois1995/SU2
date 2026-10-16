"""Coordinate a free-front coarsening with all incident transition and core tetrahedra."""
import collections,csv,json,math,shutil,sys,time
from pathlib import Path
import numpy as np
HERE=Path(__file__).resolve().parent;sys.path.insert(0,str(HERE.parent))
from audit_full_metric import CylinderField,integrate
from construct import sha,write_su2
start=time.monotonic();parent=HERE.parent/'joint_front_core_v1';source=HERE.parent/'native_front_constant_v1';field=CylinderField(source,'core_construction_rejected.json')
data=json.loads((parent/'candidate.json').read_text());proposal=json.loads((parent/'result.json').read_text());nodes={n[0]:np.array(n[1:],float) for n in data['nodes']}
radius=proposal['radii'][-1];old={n for n,p in nodes.items() if abs(np.linalg.norm(p[:2])-radius)<1e-12};angular=len(old)//2;assert len(old)==84 and angular%2==0
phase=proposal['free_fronts']*proposal['phase_per_front_radians'];step=2*math.pi/angular;groups=collections.defaultdict(list)
for n in old:
 p=nodes[n];i=int(round(((math.atan2(p[1],p[0])-phase)%(2*math.pi))/step))%angular
 groups[i//2,float(p[2])].append(n)
assert len(groups)==angular and all(len(v)==2 for v in groups.values())
collapsed={};next_id=max(nodes)+1
for (i,z),group in sorted(groups.items()):
 angle=(2*i+.5)*step+phase;nodes[next_id]=np.array([radius*math.cos(angle),radius*math.sin(angle),z])
 for n in group:collapsed[n]=next_id
 next_id+=1
cells=[];regions=[];discarded=0;negative=0
for record in data['cells']:
 c=[collapsed.get(n,n) for n in record['nodes']]
 if len(set(c))<4:discarded+=1;continue
 p=np.array([nodes[n] for n in c]);v=np.linalg.det(p[1:]-p[0]);assert v!=0
 if v<0:negative+=1;c[0],c[1]=c[1],c[0]
 cells.append(c);regions.append(record['region'])
uses=collections.defaultdict(list)
for cell in cells:
 a,b,c,d=cell
 for face in ((b,c,d),(a,d,c),(a,b,d),(a,c,b)):
  sign=(-1)**sum(face[i]>face[j] for i in range(3) for j in range(i+1,3));uses[tuple(sorted(face))].append((face,sign))
assert all(len(v)==1 or len(v)==2 and sum(x[1] for x in v)==0 for v in uses.values()),'Nonconforming coordinated quotient'
markers={f['id']:data['marker_names'][f['marker']] for f in data['facets']};roots={name:min(f['id'] for f in data['facets'] if markers[f['id']]==name) for name in data['marker_names']};surface=[]
for records in uses.values():
 if len(records)!=1:continue
 face=records[0][0]
 if all(abs(np.linalg.norm(nodes[n][:2])-field.radius)<1e-12 for n in face):facet=roots['cylinder']
 elif max(abs(nodes[n][2]) for n in face)<1e-12:facet=roots['cap0']
 else:assert max(abs(nodes[n][2]-field.z1) for n in face)<1e-12;facet=roots['cap1']
 surface.append({'nodes':face,'facet':facet})
p=np.array([[nodes[n] for n in c] for c in cells]);m=field.batch(p.mean(axis=1));e=p[:,None,:,:]-p[:,:,None,:];squared=np.einsum('nabi,nij,nabj->nab',e,m,e)
weighted=np.linalg.det(p[:,1:]-p[:,:1])*np.sqrt(np.linalg.det(m));i,j=np.triu_indices(4,1);q=12*(weighted/2)**(2/3)/squared[:,i,j].sum(axis=1)
products=np.array([np.prod(np.delete(squared[:,k,:],k,axis=1),axis=1) for k in range(4)]).T;jac=np.min(math.sqrt(2)*weighted[:,None]/np.sqrt(products),axis=1)
region_report={name:{'cells':regions.count(name),'minimum_q':float(q[np.array(regions)==name].min()),'minimum_j':float(jac[np.array(regions)==name].min())} for name in sorted(set(regions))}
used={n for c in cells for n in c};output=dict(data,nodes=[[n]+nodes[n].tolist() for n in sorted(used)],cells=[{'id':i,'nodes':c,'region':regions[i]} for i,c in enumerate(cells)],surface=surface)
(HERE/'candidate.json').write_text(json.dumps(output,separators=(',',':'))+'\n')
shape_pass=q.min()>=.2 and jac.min()>=.05
report={'status':'POINT_SHAPE_AND_FACE_CONFORMITY_ONLY','cells':len(cells),'removed_degenerate_tets':discarded,'negative_tets_after_mapping':negative,'merged_front_vertices':len(old)-len(groups),'minimum_q':float(q.min()),'minimum_j':float(jac.min()),'regions':region_report,
        'candidate_sha256':sha(HERE/'candidate.json'),'parent_sha256':sha(parent/'candidate.json'),'helper_sha256':sha(Path(__file__)),'field_helper_sha256':sha(HERE.parent/'audit_full_metric.py')}
violations=[];maximum=indicator=0.;pairs=sorted({tuple(sorted((c[i],c[j]))) for c in cells for i in range(4) for j in range(i+1,4)})
if shape_pass:
 with (HERE/'full_edges.csv').open('w') as out:
  writer=csv.writer(out);writer.writerow(['a','b','length','estimated_error'])
  for number,(a,b) in enumerate(pairs):
   length,error,calls=integrate(field,nodes[a],nodes[b]);maximum=max(maximum,length);indicator=max(indicator,error);writer.writerow([a,b,format(length,'.17g'),format(error,'.17g')])
   if length+error>1.8:violations.append({'a':a,'b':b,'length':length,'indicator':error})
   if (number+1)%1000==0:out.flush();print('Independent full edges:',number+1,'/',len(pairs),flush=True)
status='PASS_COMPLETE_POINT_SHAPE_AND_FULL_EDGES_GEOMETRY_PENDING' if shape_pass and not violations else 'REJECTED_POINT_SHAPE_OR_FULL_EDGES'
name='candidate_proposal.su2' if status.startswith('PASS') else 'candidate_rejected.su2'
write_su2(HERE/name,nodes,cells,surface,{f['id']:f['marker'] for f in data['facets']},data['marker_names']);shutil.copyfile(source/'core_initial.su2.native_ref',HERE/(name+'.native_ref'))
report.update(status=status,full_edges=len(pairs) if shape_pass else 0,maximum_full_length=maximum,maximum_estimated_quadrature_error=indicator,violating_edges=violations,
              cells_plus_recorded_surface_work=len(cells)+642,elapsed_seconds=time.monotonic()-start,
              scope='Whole high-ratio proposal, fixed original first-height band; paired free-front angular vertices merged with all incident transition/core cells reconstructed by the simplicial quotient. Degenerate duplicate-ID tets removed, global oriented-face conformity and actual-point shapes checked. Complete adaptive full-edge quadrature runs only on shape-admissible candidates. Exact embedding/height/authority/source and native work/time/MPI/CFD acceptance remain separate.')
(HERE/'result.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))
