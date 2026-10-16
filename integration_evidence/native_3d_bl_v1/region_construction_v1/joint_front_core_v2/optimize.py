"""Bounded joint terminal-front/core layout search on the complete high-ratio proposal."""
import collections,csv,json,math,shutil,sys,time
from pathlib import Path
import numpy as np
HERE=Path(__file__).resolve().parent;sys.path.insert(0,str(HERE.parent))
from audit_full_metric import CylinderField,integrate
from construct import sha,write_su2
start=time.monotonic();parent=HERE.parent/'joint_front_core_v1';source=HERE.parent/'native_front_constant_v1';field=CylinderField(source,'core_construction_rejected.json')
data=json.loads((parent/'candidate.json').read_text());proposal=json.loads((parent/'result.json').read_text())
ids=[n[0] for n in data['nodes']];positions=np.array([n[1:] for n in data['nodes']],dtype=float);index={n:i for i,n in enumerate(ids)}
cells=np.array([[index[n] for n in c['nodes']] for c in data['cells']]);radii=np.linalg.norm(positions[:,:2],axis=1)
last=np.flatnonzero(abs(radii-proposal['radii'][-2])<1e-12);core=np.flatnonzero(abs(radii-proposal['radii'][-1])<1e-12)
assert len(last)==len(core)==84
changed=np.isin(cells,np.r_[last,core]).any(axis=1);affected=cells[changed]
pairs=np.array(sorted({tuple(sorted((c[i],c[j]))) for c in affected for i in range(4) for j in range(i+1,4)}))
xg,wg=np.polynomial.legendre.leggauss(8);seeds=np.linspace(0,1,5)
# A fixed composite rule is only the search proxy. Complete adaptive integration follows independently.
t=np.array([(a+b)/2+(b-a)/2*x for a,b in zip(seeds,seeds[1:]) for x in xg]);weights=np.array([(b-a)/2*w for a,b in zip(seeds,seeds[1:]) for w in wg])
def geometry(r_last,r_core):
 points=positions.copy();points[last,:2]*=r_last/proposal['radii'][-2];points[core,:2]*=r_core/proposal['radii'][-1];return points

def quality(points):
 p=points[affected];m=field.batch(p.mean(axis=1));edge=p[:,None,:,:]-p[:,:,None,:];squared=np.einsum('nabi,nij,nabj->nab',edge,m,edge)
 volume=np.linalg.det(p[:,1:]-p[:,:1]);assert np.all(volume>0)
 weighted=volume*np.sqrt(np.linalg.det(m));i,j=np.triu_indices(4,1);q=12*(weighted/2)**(2/3)/squared[:,i,j].sum(axis=1)
 products=np.array([np.prod(np.delete(squared[:,k,:],k,axis=1),axis=1) for k in range(4)]).T
 jac=np.min(math.sqrt(2)*weighted[:,None]/np.sqrt(products),axis=1)
 return float(q.min()),float(jac.min())

records=[]
for r_last in np.linspace(.825,.865,9):
 for r_core in np.linspace(.3,.75,10):
  points=geometry(r_last,r_core);q,j=quality(points);records.append({'r_last':float(r_last),'r_core':float(r_core),'q':q,'j':j,'shape_margin':min(q/.2,j/.05)})
records.sort(key=lambda r:r['shape_margin'],reverse=True);best=None
for row in records:
 if row['shape_margin']<1:break
 points=geometry(row['r_last'],row['r_core']);maximum=0.
 for low in range(0,len(pairs),64):
  selected=pairs[low:low+64];a=points[selected[:,0]];direction=points[selected[:,1]]-a
  samples=a[:,None,:]+t[None,:,None]*direction[:,None,:];m=field.batch(samples.reshape((-1,3)))
  speed=np.sqrt(np.einsum('ni,ntij,nj->nt',direction,m.reshape((len(selected),len(t),3,3)),direction))
  maximum=max(maximum,float((speed@weights).max()))
 row['proxy_full_length_max']=maximum
 if maximum<=1.79:best=row;break
if best is None:
 with (HERE/'search.csv').open('w') as out:
  writer=csv.DictWriter(out,fieldnames=['r_last','r_core','q','j','shape_margin','proxy_full_length_max']);writer.writeheader();writer.writerows(records)
 report={'status':'REJECTED_JOINT_FIXED_TOPOLOGY_LAYOUT_FAMILY','best_shape':records[0],'joint_trials':len(records),'helper_sha256':sha(Path(__file__)),'parent_candidate_sha256':sha(parent/'candidate.json'),'field_helper_sha256':sha(HERE.parent/'audit_full_metric.py'),'scope':'All90 joint terminal/core layouts fail the point-shape/proxy screening; no native run or complete mesh acceptance. This is evidence against retaining fixed angular topology through the fade, not global infeasibility.'}
 (HERE/'result.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2));raise SystemExit(1)
points=geometry(best['r_last'],best['r_core']);nodes={n:points[i] for i,n in enumerate(ids)}
all_pairs=sorted({tuple(sorted((c['nodes'][i],c['nodes'][j]))) for c in data['cells'] for i in range(4) for j in range(i+1,4)})
full=[];maximum=indicator=0.
with (HERE/'full_edges.csv').open('w') as out:
 writer=csv.writer(out);writer.writerow(['a','b','length','estimated_error'])
 for number,(a,b) in enumerate(all_pairs):
  length,error,calls=integrate(field,nodes[a],nodes[b]);maximum=max(maximum,length);indicator=max(indicator,error)
  writer.writerow([a,b,format(length,'.17g'),format(error,'.17g')])
  if length+error>1.8:full.append({'a':a,'b':b,'length':length,'indicator':error})
  if (number+1)%1000==0:out.flush();print('Independent full edges:',number+1,'/',len(all_pairs),flush=True)
output=dict(data,nodes=[[n]+nodes[n].tolist() for n in ids]);(HERE/'candidate.json').write_text(json.dumps(output,separators=(',',':'))+'\n')
name='candidate_rejected.su2' if full else 'candidate_proposal.su2'
write_su2(HERE/name,nodes,[c['nodes'] for c in data['cells']],data['surface'],{f['id']:f['marker'] for f in data['facets']},data['marker_names']);shutil.copyfile(source/'core_initial.su2.native_ref',HERE/(name+'.native_ref'))
with (HERE/'search.csv').open('w') as out:
 writer=csv.DictWriter(out,fieldnames=['r_last','r_core','q','j','shape_margin','proxy_full_length_max']);writer.writeheader();writer.writerows(records)
report={'status':'PASS_COMPLETE_POINT_SHAPE_AND_FULL_METRIC_EDGES_GEOMETRY_PENDING' if not full else 'REJECTED_FULL_METRIC_EDGES',
 'best':best,'joint_trials':len(records),'cells':len(data['cells']),'full_edges':len(all_pairs),'maximum_full_length':maximum,'maximum_estimated_quadrature_error':indicator,
 'violating_edges':full,'cells_plus_recorded_surface_and_search_work':len(data['cells'])+642+len(records),'unchanged_first_band':True,
 'parent_candidate_sha256':sha(parent/'candidate.json'),'candidate_sha256':sha(HERE/'candidate.json'),'edges_sha256':sha(HERE/'full_edges.csv'),'helper_sha256':sha(Path(__file__)),
 'field_helper_sha256':sha(HERE.parent/'audit_full_metric.py'),'elapsed_seconds':time.monotonic()-start,
 'scope':'Whole high-ratio fixed first-row proposal with joint terminal-front/core placement over90 layouts, actual-point q/J and all full composed edges independently integrated. Search length proxy is not acceptance. Complete exact embedding/height/authority/source and native work/time/publication/MPI/CFD remain unqualified.'}
(HERE/'result.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))
