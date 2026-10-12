from pathlib import Path
from fractions import Fraction as F
from itertools import combinations
import json,math,hashlib
HERE=Path(__file__).resolve().parent
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
def sub(a,b):return tuple(x-y for x,y in zip(a,b))
def dot(a,b):return sum(x*y for x,y in zip(a,b))
def cross(a,b):return (a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0])
def det(a,b,c):return dot(a,cross(b,c))
def volume(p):return det(sub(p[1],p[0]),sub(p[2],p[0]),sub(p[3],p[0]))
def weights(t,p):
 total=volume(t);assert total>0
 result=[]
 for i in range(4):
  q=t[:];q[i]=p;result.append(volume(q)/total)
 return result
def separated(a,b):
 edges=lambda p:[sub(p[j],p[i]) for i,j in combinations(range(4),2)]
 axes=[cross(sub(p[j],p[i]),sub(p[k],p[i])) for p in (a,b) for i,j,k in combinations(range(4),3)]
 axes += [cross(u,v) for u in edges(a) for v in edges(b)]
 for axis in axes:
  if not any(axis):continue
  u=[dot(p,axis) for p in a];v=[dot(p,axis) for p in b]
  if max(u)<=min(v) or max(v)<=min(u):return True
 return False
def oriented(face):
 face=list(face);at=face.index(min(face));return tuple(face[at:]+face[:at])
def faces(c):return ((c[1],c[2],c[3]),(c[0],c[3],c[2]),(c[0],c[1],c[3]),(c[0],c[2],c[1]))
receipt=json.loads((HERE/'validation.json').read_text());assert receipt['status']=='PASS'
inputs={};results={};constant=(1.,.2,.1,.05,3.)
for ranks in (1,2,4):
 work=HERE/('mpi'+str(ranks));result={}
 for workers in range(1,ranks+1):
  name='curved_runtime_m'+str(workers)
  def read(suffix):
   p=work/(name+suffix);inputs[str(p.relative_to(HERE))]=sha(p);return json.loads(p.read_text())
  row=read('_accepted.json');old=read('_initial.json');source=read('_source.json');states=read('_states.json')
  nodes={n[0]:tuple(F(x) for x in n[1:]) for n in row['nodes']}
  cells=[[nodes[v] for v in c['nodes']] for c in row['cells']];assert len(cells)>6
  total=sum((volume(c)/6 for c in cells),F(0));assert all(volume(c)>0 for c in cells)
  for a,b in combinations(cells,2):assert separated(a,b)
  incidence={}
  for c in row['cells']:
   for face in faces(c['nodes']):incidence.setdefault(tuple(sorted(face)),[]).append(face)
  for values in incidence.values():
   assert len(values)<=2
   if len(values)==2:assert oriented(values[0])==oriented(tuple(reversed(values[1])))
  skin={oriented(v[0]) for v in incidence.values() if len(v)==1}
  observed={oriented(f['nodes']) for f in row['surface']};assert skin==observed
  signed=sum((det(*(nodes[v] for v in f['nodes']))/6 for f in row['surface']),F(0));assert signed==total
  authority=F(1)+F(1,100)/36
  assert abs(total-authority)<abs(F(1)-authority)
  facets={f['id']:f for f in row['facets']};height=lambda x,y:-.01*x*(1-x)*y*(1-y)
  max_error=0.
  for face in row['surface']:
   points=[tuple(float(x) for x in nodes[v]) for v in face['nodes']]
   marker=row['marker_names'][facets[face['facet']]['marker']]
   for i in range(21):
    for j in range(21-i):
     u=i/20.;v=j/20.;p=tuple(points[0][k]*(1-u-v)+points[1][k]*u+points[2][k]*v for k in range(3))
     if marker.startswith('z_minus'):
      residual=abs(p[2]-height(p[0],p[1]));max_error=max(max_error,residual);assert residual<=.003
     else:
      axis={'x_minus':0,'x_plus':0,'y_minus':1,'y_plus':1,'z_plus':2}[marker]
      expected=0 if marker.endswith('minus') else 1;assert abs(p[axis]-expected)<=1e-14
  source_points=[[tuple(F(x) for x in n[1:4]) for n in s['vertices']] for s in source]
  maxid=(1<<64)-1;extended=0
  def covered(p):
   global extended
   for exterior in (False,True):
    active=[]
    for record,t in zip(source,source_points):
     if (record['component']!=maxid)!=exterior:continue
     w=weights(t,p)
     if min(w)>=0:active.append(tuple(sorted(record['vertices'][i][0] for i,value in enumerate(w) if value>0)))
    if active:
     assert len(set(active))==1
     if exterior:extended+=1
     return True
   return False
  edges=set()
  for cell,ids in zip(cells,[c['nodes'] for c in row['cells']]):
   centroid=tuple(F(float(sum(p[k] for p in cell)/4)) for k in range(3));assert covered(centroid)
   lengths={(i,j):math.sqrt(float(dot(sub(cell[i],cell[j]),sub(cell[i],cell[j])))) for i,j in combinations(range(4),2)}
   q=12*(float(volume(cell))/2)**(2./3)/sum(x*x for x in lengths.values());assert q>=.20
   jac=min(math.sqrt(2)*float(volume(cell))/math.prod(lengths[tuple(sorted((i,j)))] for j in range(4) if i!=j) for i in range(4));assert jac>=.05
   for i,j in combinations(range(4),2):edges.add(tuple(sorted((ids[i],ids[j]))))
  for p in nodes.values():assert covered(p)
  for a,b in edges:
   p,q=nodes[a],nodes[b];intervals=[];cuts={F(0),F(1)}
   for t in source_points:
    lo,hi=F(0),F(1);wa,wb=weights(t,p),weights(t,q)
    for x,y in zip(wa,wb):
     if x<0 and y<0:lo,hi=F(1),F(0);break
     if x<0:lo=max(lo,x/(x-y))
     if y<0:hi=min(hi,x/(x-y))
    if lo<=hi:intervals.append((lo,hi));cuts.update((lo,hi))
   cuts=sorted(cuts)
   for lo,hi in zip(cuts,cuts[1:]):assert any(a<=lo and b>=hi for a,b in intervals)
   assert math.sqrt(.5*float(dot(sub(q,p),sub(q,p))))<=1.8
  assert extended>0
  for sample in states['accepted']:
   for i,value in enumerate(sample['states']):assert abs(value-constant[i%5]*(.96875 if i>=10 else 1))<=1e-11
   assert max(abs(value-(.5 if i in (0,3,5) else 0)) for i,value in enumerate(sample['metric']))<=1e-12
   rho,mx,my,mz,energy=sample['states'][:5];assert rho>0 and energy-(mx*mx+my*my+mz*mz)/(2*rho)>0
  output=work/(name+'_mesh_00001.su2');assert output.is_file()
  reference=Path(str(output)+'.native_ref');assert reference.is_file()
  assert reference.read_bytes()==(work/(name+'_accepted.native_ref')).read_bytes()==(work/(name+'.su2.native_ref')).read_bytes()
  inputs[str(output.relative_to(HERE))]=sha(output);inputs[str(reference.relative_to(HERE))]=sha(reference)
  result[str(workers)]={'cells':len(cells),'volume':float(total),'analytic_domain_volume':float(authority),'volume_approaches_authority':True,'exact_skin_closure_and_no_interior_overlap':True,'sampled_maximum_bump_error':max_error,'original_or_declared_source_coverage':True,'extended_queries':extended,'current_bdf_constant_and_metric_transport':True,'positive_states':True,'actual_su2_sidecar':True}
 results[str(ranks)]=result
out={'status':'PASS','scope':'Independent accepted curved CFD replacement checks: exact positive volumes/skin conformity/no-interior-overlap, volume toward analytic bump authority, sampled surface error, original-or-declared source centroid/vertex and exact edge coverage, identity-equivalent shape/sliver and constant sensor length, transported constant current/BDF states/positivity, actual SU2 writer and unchanged analytic sidecars. Prescribed sensor; sampled surface check complements production conservative bounds, not general Hausdorff proof. No conservation-algorithm/restart/solved CFD-derived-metric or full Goal2 claim.','controller_sha256':sha(HERE/'validation.json'),'script_sha256':sha(Path(__file__)),'inputs_sha256':inputs,'results':results}
(HERE/'independent_runtime_validation.json').write_text(json.dumps(out,indent=2)+'\n');print('PASS: accepted curved runtime grids/states/source/volume/sidecars')
