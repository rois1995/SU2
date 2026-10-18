#!/usr/bin/env python3
"""Bounded source-policy experiment on saved geometry; no native-construction claim."""
import collections,hashlib,itertools,json,math,resource,sys,time
from fractions import Fraction as F
from pathlib import Path
import numpy as np
HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[3]
sys.path.insert(0,str(ROOT/'TestCases/adaptation/capability'))
from capcheck import read_su2
sys.path.insert(0,str(ROOT/'integration_evidence/native_3d_core_v1'))
from audit_cavities import determinant,overlap
START=time.monotonic();MAX_SECONDS=150;MAX_PAIR_TESTS=12000
LD=np.longdouble

def sha(p):
 h=hashlib.sha256()
 with p.open('rb') as f:
  while b:=f.read(1024**2):h.update(b)
 return h.hexdigest()
def budget():
 if time.monotonic()-START>MAX_SECONDS:raise RuntimeError('Replay time admission exhausted; partial evidence only.')
def exact(p):return tuple(F(float(x)) for x in p)
def positive_weights(t,p):
 t=[exact(x) for x in t];p=exact(p);d=determinant(t)
 assert d
 w=[]
 for k in range(4):
  q=t[:];q[k]=p;w.append(determinant(q)/d)
 return w if min(w)>=0 else None

def checks():
 # Exact counterexample, not merely an observed near-tie in rounded geometry.
 # Fluid is the complement of the positive x/y wedge; both feet are nearest.
 d=F(1,2**20);p=(d,d,F(1,2));feet=[(F(0),d,p[2]),(d,F(0),p[2])]
 distances=[sum((a-b)**2 for a,b in zip(p,q)) for q in feet]
 tensors=[(1+q[0],2+q[1],F(3)) for q in feet]
 assert distances==[d*d,d*d] and tensors[0]!=tensors[1]
 # A single common offset edge instead carries one common trace, with nonnegative weights.
 assert all(x>0 for t in tensors for x in t)
 return dict(distance=str(d),squared_distances=list(map(str,distances)),trace_diagonals=[list(map(str,t)) for t in tensors],
  conclusion='Deterministic nearest-face tie breaking does not make a nonconstant original trace continuous; opposite one-sided limits differ.')

def prepare(points,cells,faces,markers,tensors):
 begin=time.monotonic();points=np.asarray(points,float);cells=np.asarray(cells,int);faces=np.asarray(faces,int)
 assert len(faces)==len(markers) and np.all(np.linalg.eigvalsh(tensors)>0)
 keys={tuple(sorted(map(int,f))):i for i,f in enumerate(faces)};assert len(keys)==len(faces)
 parents=np.full(len(faces),-1,int);opposite=np.empty((len(faces),3),float)
 for ci,c in enumerate(cells):
  for k in range(4):
   face=tuple(sorted(int(c[j]) for j in range(4) if j!=k));i=keys.get(face)
   if i is not None:
    assert parents[i]<0;parents[i]=ci;opposite[i]=points[c[k]]
 assert np.all(parents>=0)
 tri=points[faces].astype(LD);u=tri[:,1]-tri[:,0];v=tri[:,2]-tri[:,0]
 normals=np.cross(u,v);norm=np.sqrt(np.sum(normals*normals,axis=1));assert np.all(norm>0)
 normals/=norm[:,None];sign=np.sum(normals*(opposite.astype(LD)-tri[:,0]),axis=1);assert np.all(sign!=0)
 normals[sign>0]*=-1
 edges=collections.defaultdict(list);stars=collections.defaultdict(list)
 for i,face in enumerate(faces):
  for node in face:stars[int(node)].append(i)
  for a,b in itertools.combinations(face,2):edges[tuple(sorted((int(a),int(b))))].append(i)
 assert all(len(v)==2 for v in edges.values()),'Expected a complete closed saved physical surface.'
 uu=np.sum(u*u,axis=1);uv=np.sum(u*v,axis=1);vv=np.sum(v*v,axis=1);gram=uu*vv-uv*uv;assert np.all(gram>0)
 return dict(points=points,cells=cells,faces=faces,markers=markers,tensors=tensors,tri=tri,normals=normals,
  u=u,v=v,uu=uu,uv=uv,vv=vv,gram=gram,edges=edges,stars=stars,parents=parents,
  face_lo=np.min(tri,axis=1),face_hi=np.max(tri,axis=1),cell_lo=points[cells].min(axis=1),cell_hi=points[cells].max(axis=1),
  preparation_seconds=time.monotonic()-begin,prepared_array_bytes=sum(a.nbytes for a in (points,cells,faces,tri,normals,u,v,uu,uv,vv,gram,parents)),
  note='Array bytes exclude Python topology dictionaries, original NPZ buffers and interpreter; process max RSS is separate.')

def projected(g,i,p):
 delta=np.asarray(p,LD)-g['tri'][i,0];a=np.dot(delta,g['u'][i]);b=np.dot(delta,g['v'][i]);den=g['gram'][i]
 r=(a*g['vv'][i]-b*g['uv'][i])/den;s=(b*g['uu'][i]-a*g['uv'][i])/den
 weights=np.array([1-r-s,r,s],LD);foot=weights@g['tri'][i]
 return weights,foot,np.sum((np.asarray(p,LD)-foot)**2)

def nearest_triangle(g,i,p):
 weights,foot,distance=projected(g,i,p);best=(distance,weights,foot) if min(weights)>=0 else None
 for a,b in ((0,1),(1,2),(2,0)):
  edge=g['tri'][i,b]-g['tri'][i,a];q=np.asarray(p,LD)-g['tri'][i,a]
  t=np.clip(np.dot(q,edge)/np.dot(edge,edge),0,1);w=np.zeros(3,LD);w[a]=1-t;w[b]=t
  x=w@g['tri'][i];d=np.sum((np.asarray(p,LD)-x)**2)
  if best is None or d<best[0]:best=(d,w,x)
 return best

def relative_jump(a,b):
 a=np.asarray(a,float);b=np.asarray(b,float);values,vectors=np.linalg.eigh((a+b)/2)
 assert min(values)>0;white=vectors/np.sqrt(values)[None,:]
 return float(np.max(abs(np.linalg.eigvalsh(white.T@(a-b)@white))))

def outward_direction(g,node):
 # Small minimum-norm normal-cone QP. This is a numerical proposal, not a general collar theorem.
 normals=np.asarray(g['normals'][g['stars'][node]],float);best=None;attempts=0
 for count in range(1,min(3,len(normals))+1):
  for active in itertools.combinations(range(len(normals)),count):
   attempts+=1;n=normals[list(active)]
   try:lam=np.linalg.solve(n@n.T,np.ones(count))
   except np.linalg.LinAlgError:continue
   if min(lam)<-1e-10:continue
   candidate=n.T@lam
   if np.min(normals@candidate)<1-1e-10:continue
   if best is None or np.dot(candidate,candidate)<np.dot(best,best):best=candidate
 budget()
 if best is None or np.linalg.norm(best)>1000:return None,attempts
 return best*(1+1e-6),attempts

def candidates(g):
 rows=[]
 for edge,uses in g['edges'].items():
  n=g['normals'][uses];cos=float(np.dot(*n))
  if not -1+1e-10<cos<1-1e-10:continue
  direction=n.sum(axis=0);direction/=np.sqrt(np.sum(direction*direction));center=g['points'][list(edge)].astype(LD).mean(axis=0)
  p=center+LD('0.0001')*direction
  if all(min(projected(g,i,p)[0])>LD('1e-12') for i in uses):
   rows.append((g['markers'][uses[0]]!=g['markers'][uses[1]],1-cos,edge,uses,direction))
 rows.sort(key=lambda r:(not r[0],-r[1],r[2]))
 # Reserve distinct shared-marker junction and same-marker crease evidence; no random sampling.
 chosen=rows[:8]
 for row in rows:
  if row[0]:continue
  if all(row[2]!=old[2] for old in chosen):chosen.append(row)
  if len(chosen)>=12:break
 return chosen,len(rows)

def field(g,weights,face):return np.einsum('k,kij->ij',weights,g['tensors'][g['faces'][face]].astype(LD))

def run_case(name,g,constant=False):
 begin=time.monotonic();selected,eligible=candidates(g);assert selected
 chosen_faces=sorted({i for r in selected for i in r[3]});nodes=sorted({int(n) for i in chosen_faces for n in g['faces'][i]})
 directions={};qp_work=0
 for node in nodes:
  direction,work=outward_direction(g,node);directions[node]=direction;qp_work+=work
 rows=[];collars=[];exact_pairs=0
 for band in (1e-13,1e-8,1e-4):
  budget();mapped={n:g['points'][n]+band*v for n,v in directions.items() if v is not None};tets=[];folded=[]
  for i in chosen_faces:
   ids=sorted(map(int,g['faces'][i]))
   if not all(n in mapped for n in ids):folded.append(i);continue
   a,b,c=[g['points'][n] for n in ids];aa,bb,cc=[mapped[n] for n in ids]
   old=[a,b,c];fresh=[aa,bb,cc]
   index=((0,1,2,5),(0,1,4,5),(0,3,4,5));allp=old+fresh;allm=[g['tensors'][n] for n in ids]*2
   created=[];signs=[]
   for slot,ix in enumerate(index):
    p=np.array([allp[j] for j in ix]);m=np.array([allm[j] for j in ix]);det=determinant([exact(q) for q in p]);signs.append((1 if det>0 else -1 if det<0 else 0)*(1 if slot!=1 else -1))
    if det<0:p[[0,1]]=p[[1,0]];m[[0,1]]=m[[1,0]]
    created.append((p,m,i))
   if not signs[0] or len(set(signs))!=1:folded.append(i)
   else:tets.extend(created)
  conflicts=0;pair_tests=0
  # Exact interior-overlap checks within the selected paired-face collar, not a complete global shell audit.
  for ai,(a,am,af) in enumerate(tets):
   lo=a.min(axis=0);hi=a.max(axis=0)
   for b,bm,bf in tets[ai+1:]:
    if np.any(hi<b.min(axis=0)) or np.any(b.max(axis=0)<lo):continue
    pair_tests+=1;exact_pairs+=1
    if exact_pairs>MAX_PAIR_TESTS:raise RuntimeError('Exact pair-test admission exhausted; partial evidence only.')
    if overlap([exact(x) for x in a],[exact(x) for x in b]):conflicts+=1
   budget()
  covered=0;physical=0;jumps=[];differences=[];nonnearest=0
  for junction,angle,edge,uses,direction in selected:
   center=g['points'][list(edge)].astype(LD).mean(axis=0);p=np.asarray(center+LD(str(band))*.7*direction,float)
   in_original=False;physical_candidates=np.flatnonzero(np.all(g['cell_lo']<=p,axis=1)&np.all(p<=g['cell_hi'],axis=1))
   for ci in physical_candidates:
    if positive_weights(g['points'][g['cells'][ci]],p) is not None:in_original=True;break
   if in_original:physical+=1;continue # Original P1 always takes priority; no extension value is selected.
   data=[projected(g,i,p) for i in uses]
   if not all(min(row[0])>=0 for row in data):continue
   values=[field(g,row[0],i) for row,i in zip(data,uses)];jump=relative_jump(*values);jumps.append(jump)
   # Global finite-face candidate screening. Exact tie proof is separate above; these are near-bisector samples.
   near=np.flatnonzero(np.all(g['face_lo']-band<=p,axis=1)&np.all(p<=g['face_hi']+band,axis=1))
   nearest=[]
   for i in near:
    distance,weights,foot=nearest_triangle(g,i,p)
    nearest.append((distance,int(i)))
   nearest.sort();winner=nearest[0][1] if nearest else -1
   if winner not in uses:nonnearest+=1
   sensor=None
   for t,m,face in tets:
    if np.any(p<t.min(axis=0)) or np.any(p>t.max(axis=0)):continue
    weights=positive_weights(t,p)
    if weights is None:continue
    sample=np.einsum('k,kij->ij',np.array([LD(w.numerator)/LD(w.denominator) for w in weights]),m.astype(LD))
    assert np.linalg.eigvalsh(np.asarray(sample,float)).min()>0
    if sensor is not None:assert relative_jump(sensor,sample)<5e-12,'Shared collar trace disagrees.'
    sensor=sample
   if sensor is not None:
    covered+=1;differences.append(max(relative_jump(sensor,v) for v in values))
   rows.append(dict(band=band,edge=list(edge),faces=uses,markers=[g['markers'][i] for i in uses],point=p.tolist(),
    relative_projected_trace_difference=jump,distance_squared=list(map(float,[r[2] for r in data])),
    paired_face_interior_feet=True,nearest_admitted_face=winner,nearest_full_triangle_verification=True,
    common_collar_covered=sensor is not None,physical_source_candidates=len(physical_candidates)))
  collars.append(dict(band=band,selected_faces=len(chosen_faces),positive_tetrahedra=len(tets),degenerate_or_folded_faces=folded,
   numerical_cone_failures=[n for n in nodes if directions[n] is None],exact_collar_pair_tests=pair_tests,interior_overlap_pairs=conflicts,
   outside_original_samples=len(jumps),physical_source_priority_samples=physical,collar_covered_samples=covered,
   other_interior_face_nearer_samples=nonnearest,max_relative_projected_trace_difference=max(jumps,default=0),
   max_collar_vs_projection_difference=max(differences,default=0)))
 result=dict(status='BOUNDED_POLICY_COMPARISON',case=name,constant_sensor=constant,points=len(g['points']),original_cells=len(g['cells']),
  boundary_faces=len(g['faces']),eligible_reentrant_pair_edges=eligible,selected_edges=len(selected),qp_active_set_solves=qp_work,
  preparation_seconds=g['preparation_seconds'],prepared_array_bytes=g['prepared_array_bytes'],array_memory_scope=g['note'],
  experiment_seconds=time.monotonic()-begin,bands=collars,samples=rows,
  scope=['Numerical finite-triangle distances include edges/vertices and all AABB candidates within the sampled band; exact rational wedge counterexample supplies the discontinuity proof.',
   'Collar: same source-node trace on common generated vertices, exact positive tetrahedra and selected-patch interior-overlap checks only.',
   'No global shell/source-overlap/complete-envelope coverage certificate, production geometry mapping, edge quadrature, MPI runtime, transfer, restart or accepted remeshing claim.',
   'The selected 0.7*band bisector samples are a coverage probe, not a cover of every permitted mapped point.'])
 (HERE/(name+'.json')).write_text(json.dumps(result,indent=2)+'\n');print(name,[(r['band'],r['max_relative_projected_trace_difference'],r['collar_covered_samples'],r['outside_original_samples'],r['interior_overlap_pairs']) for r in collars],flush=True)
 return result

def main():
 proof=checks();(HERE/'exact_counterexample.json').write_text(json.dumps(proof,indent=2)+'\n')
 saved=HERE.parent/'annulus_v1_n1/affine_annulus_bl_budget_20_m1_initial.json';d=json.loads(saved.read_text());nodes={n[0]:n[1:] for n in d['nodes']};assert sorted(nodes)==list(range(len(nodes)))
 points=np.array([nodes[i] for i in range(len(nodes))]);cells=np.array([c['nodes'] for c in d['cells']]);faces=np.array([[v[0] for v in f['vertices']] for f in d['facets']]);markers=[d['marker_names'][f['marker']] for f in d['facets']]
 value=np.array([[5.060904459181861,0,0],[0,5.060904459181861,0],[0,0,5.060904459181861]])
 assert all(v[4]==[value[0,0],0,0,value[1,1],0,value[2,2]] for c in d['original'] for v in c['vertices'])
 tensor=np.broadcast_to(value,(len(points),3,3)).copy();results=[]
 results.append(run_case('saved_annulus_constant',prepare(points,cells,faces,markers,tensor),True))
 u=points*np.array([.5,.3,.2])+np.array([1,.2,.4]);manufactured=np.eye(3)[None,:,:]+100*u[:,:,None]*u[:,None,:]
 results.append(run_case('saved_annulus_manufactured_anisotropic',prepare(points,cells,faces,markers,manufactured)))
 budget();base=ROOT/'integration_evidence/native_3d_bl_v1/oneram6_frozen_field_v1';capture=json.loads((base/'result.json').read_text());npz=base/'frozen_trial.npz'
 assert capture['status']=='PASS_EXACT_COMPLETE_FIRST_TRIAL_FIELD_CAPTURE' and sha(npz)==capture['sha256']['frozen_trial.npz']
 mesh=read_su2(base/'Coarse.su2');data=np.load(npz,allow_pickle=False)
 for rank in range(4):
  path=base/('rank%d_coordinates.f64'%rank);assert sha(path)==capture['sha256'][path.name]
  assert np.array_equal(np.fromfile(path,dtype='<f8').reshape(-1,3),mesh.P[data['rank%d_global'%rank]])
 raw=data['sensor_owner'];tensor=np.tril(raw)+np.swapaxes(np.tril(raw,-1),-2,-1)
 faces=np.concatenate(list(mesh.M.values()));markers=[name for name,values in mesh.M.items() for _ in values]
 results.append(run_case('saved_m6_owner_sensor',prepare(mesh.P,mesh.E,faces,markers,tensor)))
 report=dict(status='COMPLETE_BOUNDED_POLICY_COMPARISON',elapsed_seconds=time.monotonic()-START,maximum_seconds=MAX_SECONDS,
  maximum_exact_pair_tests=MAX_PAIR_TESTS,process_maxrss_KiB=resource.getrusage(resource.RUSAGE_SELF).ru_maxrss,
  exact_counterexample=proof,inputs={str(p.relative_to(ROOT)):sha(p) for p in (Path(__file__).resolve(),saved,npz,base/'Coarse.su2',base/'result.json',ROOT/'TestCases/adaptation/capability/capcheck.py',ROOT/'integration_evidence/native_3d_core_v1/audit_cavities.py')},
  case_summaries=[{k:v for k,v in r.items() if k!='samples'} for r in results],
  frozen_m6_scope='Canonical saved owner-node sensor P1 diagnostic; raw parent-rank halo tensors remain unchanged in the original archive. This is not a retrofitted declared geometry or a new normalized M6 target.',
  decision='Nearest-face tie breaking is discontinuous in general. Shared-vertex normal-cone collars also fold on two selected M6 faces at the wider band despite sampled coverage. Neither is admitted as a general source policy.')
 (HERE/'result.json').write_text(json.dumps(report,indent=2)+'\n');print(report['status'],report['elapsed_seconds'],report['process_maxrss_KiB'],flush=True)
if __name__=='__main__':
 try:main()
 except BaseException as e:
  (HERE/'failure.json').write_text(json.dumps(dict(type=type(e).__name__,reason=str(e),elapsed_seconds=time.monotonic()-START,process_maxrss_KiB=resource.getrusage(resource.RUSAGE_SELF).ru_maxrss),indent=2)+'\n');raise
