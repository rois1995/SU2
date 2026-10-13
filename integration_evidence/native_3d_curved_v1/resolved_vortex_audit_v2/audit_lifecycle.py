from pathlib import Path
from fractions import Fraction as F
from decimal import Decimal,localcontext
from itertools import combinations
from collections import Counter
import json,math,hashlib,sys
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2];CASE=HERE.parent/sys.argv[1]
sys.path.insert(0,str(ROOT/'integration_evidence/native_3d_core_v1'))
from audit_coupled import sources,metric,weights,quad,spd,surface
from audit_cavities import determinant,sub,overlap
from audit_geometry_bridge import signature,su2_signature,cgns_signature
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
MAX=(1<<64)-1
inputs={};results={}
def read(p):
 inputs[str(p.relative_to(ROOT))]=sha(p);return json.loads(p.read_text())
def source_tree(records):
 lo=tuple(min(float(c['lo'][k]) for _,c in records) for k in range(3))
 hi=tuple(max(float(c['hi'][k]) for _,c in records) for k in range(3))
 if len(records)<=16:return lo,hi,records,None
 axis=max(range(3),key=lambda k:hi[k]-lo[k]);records=sorted(records,key=lambda r:float(r[1]['lo'][axis]+r[1]['hi'][axis]))
 mid=len(records)//2;return lo,hi,None,(source_tree(records[:mid]),source_tree(records[mid:]))
def query_sources(tree,a,b):
 lo=tuple(min(float(a[k]),float(b[k])) for k in range(3));hi=tuple(max(float(a[k]),float(b[k])) for k in range(3));result=[];pending=[tree]
 while pending:
  lower,upper,records,children=pending.pop()
  if any(upper[k]<lo[k] or hi[k]<lower[k] for k in range(3)):continue
  if children:pending.extend(children)
  else:
   for priority,c in records:
    if all(float(c['hi'][k])>=lo[k] and hi[k]>=float(c['lo'][k]) for k in range(3)):result.append((priority,c))
 return sorted(result,key=lambda r:(r[0],r[1]['key']))
def indexed_metric(tree,p):return metric([c for _,c in query_sources(tree,p,p)],p)
def exact_length(tree,a,b):
 chord=sub(b,a);intervals=[];cuts={F(0),F(1)}
 for priority,c in query_sources(tree,a,b):
  if any(max(a[i],b[i])<c['lo'][i] or min(a[i],b[i])>c['hi'][i] for i in range(3)):continue
  wa,wb=weights(c['points'],a),weights(c['points'],b);lo,hi=F(0),F(1)
  for x,y in zip(wa,wb):
   if x<0 and y<0:hi=F(-1);break
   if x<0:lo=max(lo,x/(x-y))
   if y<0:hi=min(hi,x/(x-y))
  if lo>=hi:continue
  intervals.append((lo,hi,priority,c,wa,wb));cuts.update((lo,hi))
 cuts=sorted(cuts)
 with localcontext() as ctx:
  ctx.prec=80;total=Decimal(0)
  dec=lambda x:Decimal(x.numerator)/Decimal(x.denominator)
  for lo,hi in zip(cuts,cuts[1:]):
   mid=(lo+hi)/2;active=[r for r in intervals if r[0]<=mid<=r[1]];assert active,'Uncovered metric edge interval'
   active.sort(key=lambda r:(r[2],r[3]['key']));chosen=active[0];priority=chosen[2]
   supports=[]
   for _,_,pr,c,wa,wb in active:
    if pr==priority:
     w=[x+mid*(y-x) for x,y in zip(wa,wb)];supports.append(tuple(sorted(n for n,v in zip(c['ids'],w) if v>0)))
   assert len(set(supports))==1,'Ambiguous same-priority source interval'
   _,_,_,c,wa,wb=chosen;q=[]
   for t in (lo,hi):
    w=[x+t*(y-x) for x,y in zip(wa,wb)];m=[sum(w[i]*c['metrics'][i][j] for i in range(4)) for j in range(6)];q.append(dec(quad(m,chord)))
   assert min(q)>0
   total+=dec(hi-lo)*Decimal(2)/3*(q[0]+(q[0]*q[1]).sqrt()+q[1])/(q[0].sqrt()+q[1].sqrt())
  return float(total)
def near_pairs(tets):
 boxes=[(tuple(min(float(p[k]) for p in t) for k in range(3)),tuple(max(float(p[k]) for p in t) for k in range(3)),t) for t in tets]
 active=[]
 for lo,hi,t in sorted(boxes,key=lambda row:row[0][0]):
  active=[row for row in active if row[1][0]>=lo[0]]
  for alo,ahi,a in active:
   if all(ahi[k]>=lo[k] and hi[k]>=alo[k] for k in (1,2)):yield a,t
  active.append((lo,hi,t))
def audit_event(work,name,amplitude,format):
 row=read(work/(name+'_accepted.json'));old=read(work/(name+'_initial.json'));raw=read(work/(name+'_source.json'))
 state=read(work/(name+'_states.json'));ledger=read(work/(name+'_projection.json'))
 sources({'original':raw}) # Shared source identities, coordinates, tensors and positive original/extension cells.
 src=[]
 for pr in (0,1):
  records=[r for r in raw if int(r['component']!=MAX)==pr]
  group,_=sources({'original':records})
  bykey={tuple(sorted(n[0] for n in r['vertices'])):r for r in records}
  for c in group:c['ids']=[n[0] for n in bykey[c['key']]['vertices']];src.append((pr,c))
 tree=source_tree(src);nodes={n[0]:tuple(F(x) for x in n[1:]) for n in row['nodes']}
 tetpoints=[[nodes[n] for n in c['nodes']] for c in row['cells']];volumes=[determinant(p)/6 for p in tetpoints];assert min(volumes)>0
 for a,b in near_pairs(tetpoints):assert not overlap(a,b),'Accepted interior overlap'
 print(name+': exact positive volumes and no-overlap PASS',flush=True)
 faces={};edges=set();nodal_volume={n:F(0) for n in nodes};minimum_q=1.;minimum_j=1.;maximum_l=0.
 for ids,p,v in zip([c['nodes'] for c in row['cells']],tetpoints,volumes):
  for n in ids:nodal_volume[n]+=v/4
  for f in ((ids[1],ids[2],ids[3]),(ids[0],ids[3],ids[2]),(ids[0],ids[1],ids[3]),(ids[0],ids[2],ids[1])):faces.setdefault(tuple(sorted(f)),[]).append(f)
  centroid=tuple(F(float(sum(x[k] for x in p)/4)) for k in range(3));m=indexed_metric(tree,centroid);dm=spd(m);dv=determinant(p)
  lengths={(i,j):math.sqrt(float(quad(m,sub(p[j],p[i])))) for i,j in combinations(range(4),2)}
  transformed=float(dv)*math.sqrt(float(dm));q=12*(transformed/2)**(2./3)/sum(x*x for x in lengths.values())
  j=min(math.sqrt(2)*transformed/math.prod(lengths[tuple(sorted((i,k)))] for k in range(4) if i!=k) for i in range(4))
  minimum_q=min(minimum_q,q);minimum_j=min(minimum_j,j);assert q>=.20-1e-10 and j>=.05-1e-10
  for i,j in combinations(range(4),2):edges.add(tuple(sorted((ids[i],ids[j]))))
 def oriented(f):f=list(f);at=f.index(min(f));return tuple(f[at:]+f[:at])
 for fs in faces.values():
  assert len(fs)<=2
  if len(fs)==2:assert oriented(fs[0])==oriented(tuple(reversed(fs[1])))
 skin=[fs[0] for fs in faces.values() if len(fs)==1];surface(skin,True)
 assert set(map(oriented,skin))==set(oriented(f['nodes']) for f in row['surface'])
 signed=sum((determinant([(F(0),)*3]+[nodes[n] for n in f])/6 for f in skin),F(0));assert signed==sum(volumes)
 for a,b in edges:maximum_l=max(maximum_l,exact_length(tree,nodes[a],nodes[b]))
 assert maximum_l<=1.8+1e-10
 print(name+': exact source coverage, quality and edge lengths PASS',flush=True)
 accepted={tuple(r['point']):r for r in state['accepted']};assert len(accepted)==len(nodes)
 field_integrals=[F(0)]*15;metric_error=0.
 for n,p in nodes.items():
  values=accepted[tuple(float(x) for x in p)];expected=indexed_metric(tree,p)
  metric_error=max(metric_error,max(abs(float(a)-b)/max(1.,abs(float(a))) for a,b in zip(expected,values['metric'])))
  for k,u in enumerate(values['states']):field_integrals[k]+=nodal_volume[n]*F(u)
  for off in (0,5,10):
   rho,mx,my,mz,energy=values['states'][off:off+5];assert rho>0 and energy-(mx*mx+my*my+mz*mz)/(2*rho)>0
  assert max(abs(a-b)/max(1.,abs(a)) for a,b in zip(values['states'][:5],values['states'][5:10]))<1e-10
 assert metric_error<2e-10
 assert ledger['time_levels']==2 and len(ledger['new_total'])==10
 integral=[float(x) for x in field_integrals[:5]+field_integrals[10:15]]
 for f,(got,wanted) in enumerate(zip(integral,ledger['target_total'])):
  assert abs(got-wanted)<=2e-10*max(1.,ledger['scale'][f]),'Actual state integral misses conservative target'
  assert abs(got-ledger['new_total'][f])<=2e-10*max(1.,ledger['scale'][f])
  assert abs(wanted-ledger['donor_total'][f]-ledger['fill_open'][f]+ledger['lost_open'][f])<=1e-10*max(1.,ledger['scale'][f])
 assert abs(float(sum(volumes))-ledger['target_volume'])<1e-11
 oldnodes={n[0]:tuple(F(x) for x in n[1:]) for n in old['nodes']}
 oldvolume=sum((determinant([oldnodes[n] for n in c['nodes']])/6 for c in old['cells']),F(0));assert abs(float(oldvolume)-ledger['donor_volume'])<1e-11
 assert abs(ledger['target_volume']-ledger['donor_volume']-ledger['fill_volume']+ledger['lost_volume'])<1e-10
 ref={f['id']:f for f in row['facets']};max_error=0.
 for f in row['surface']:
  marker=row['marker_names'][ref[f['facet']]['marker']];points=[[float(x) for x in nodes[n]] for n in f['nodes']]
  for i in range(9):
   for j in range(9-i):
    u,v=i/8.,j/8.;p=[points[0][k]*(1-u-v)+points[1][k]*u+points[2][k]*v for k in range(3)]
    if marker.startswith('z_minus'):
     error=abs(p[2]-amplitude*((p[0]+2)/4)*(1-(p[0]+2)/4)*((p[1]+2)/4)*(1-(p[1]+2)/4));max_error=max(max_error,error);assert error<=.003
    else:
     axis={'x_minus':0,'x_plus':0,'y_minus':1,'y_plus':1,'z_plus':2}[marker];assert abs(p[axis]-(-2 if marker.endswith('minus') else 1 if marker=='z_plus' else 2))<1e-14
 # Match the actual writer by physical mesh/marker signature rather than partition-dependent identities.
 event=int(name.rsplit('_s',1)[1]);stem=name.rsplit('_s',1)[0];file=work/(stem+'_mesh_%05d.'%(event+1)+('cgns' if format=='CGNS' else 'su2'))
 assert (cgns_signature(file) if format=='CGNS' else su2_signature(file))==signature(row)
 inputs[str(file.relative_to(ROOT))]=sha(file)
 sidecar=Path(str(file)+'.native_ref');inputs[str(sidecar.relative_to(ROOT))]=sha(sidecar)
 assert sidecar.read_bytes()==(work/(stem+'.su2.native_ref')).read_bytes()
 return {'cells':len(tetpoints),'points':len(nodes),'volume':float(sum(volumes)),'analytic_domain_volume':16-16*amplitude/36,'minimum_metric_mean_ratio':minimum_q,'minimum_metric_scaled_jacobian':minimum_j,'maximum_exact_source_metric_edge':maximum_l,'transported_metric_relative_error':metric_error,'sampled_surface_error':max_error,'fill_volume':ledger['fill_volume'],'lost_volume':ledger['lost_volume'],'conservative_actual_state_integrals_match_target':True,'exact_positive_volume_conformity_no_overlap':True}

choice=tuple(map(int,sys.argv[2:]))
plan=[(n,m,e) for n in (1,2,4) for m in (1,2,4) if m<=n for e in range(2)]
assert choice in plan, 'Select an explicit planned N/M/event tuple'
ranks,m,event=choice
receipt=read(CASE/'validation.json');assert receipt['status']=='PASS'
for name,digest in receipt['artifact_sha256'].items():assert sha(CASE/name)==digest,'Runtime evidence drift'
work=CASE/('mpi'+str(ranks));name='resolved_vortex_m'+str(m)
print('START N=%d M=%d event=%d'%choice,flush=True)
result=audit_event(work,name+'_s'+str(event),-.001,'SU2')
tensor_sets=[]
for cycle in range(2):
 records=read(work/(name+'_s'+str(cycle)+'_source.json'))
 tensors=[tuple(n[4]) for r in records if r['component']==MAX for n in r['vertices']]
 assert len(set(tensors))>1 and any(abs(t[k])>1e-8 for t in tensors for k in (1,2,4))
 tensor_sets.append(set(tensors))
assert tensor_sets[0]!=tensor_sets[1], 'Actual solved pressure metric was not refreshed'
restart=read(work/(name+'_restart_states.json'));wanted={tuple(r['point']):r['states'] for r in restart['donor']}
assert all(max(abs(a-b)/max(1.,abs(a)) for a,b in zip(r['states'],wanted[tuple(r['point'])]))<2e-10 for r in restart['accepted'])
out={'status':'PASS_RESOLVED_VORTEX_EVENT','selection':choice,'result':result,'script_sha256':sha(Path(__file__).resolve()),'inputs_sha256':inputs,'helper_sha256':{n:sha(ROOT/'integration_evidence/native_3d_core_v1'/n) for n in ('audit_coupled.py','audit_cavities.py','audit_geometry_bridge.py')},'scope':'One full event, unchanged exact geometry/embedding/source/quality/length/transport/current+BDF conservation and positivity/output/reference checks; also actual two-event metric refresh and real restart. Complete goal requires all twelve planned event certificates.'}
(HERE/('event_n%d_m%d_s%d.json'%choice)).write_text(json.dumps(out,indent=2)+'\n')
print('PASS N=%d M=%d event=%d'%choice,flush=True)
