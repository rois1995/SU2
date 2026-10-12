from pathlib import Path
from fractions import Fraction as F
from decimal import Decimal,localcontext
from itertools import combinations
from collections import Counter
import json,math,hashlib,sys
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2]
sys.path.insert(0,str(ROOT/'integration_evidence/native_3d_core_v1'))
from audit_coupled import sources,metric,weights,quad,spd,surface
from audit_cavities import determinant,sub,overlap
from audit_geometry_bridge import signature,su2_signature,cgns_signature
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
MAX=(1<<64)-1
inputs={};results={}
def read(p):
 inputs[str(p.relative_to(HERE.parent))]=sha(p);return json.loads(p.read_text())
def exact_length(src,a,b):
 chord=sub(b,a);intervals=[];cuts={F(0),F(1)}
 for priority,c in src:
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
def minimum_square(p,inner):
 a=p[0];edges=[sub(p[1],a),sub(p[2],a)];u,v=edges
 aa,ab,bb=inner(u,u),inner(u,v),inner(v,v);au,av=inner(a,u),inner(a,v);den=aa*bb-ab*ab
 if den>0:
  t=(ab*av-bb*au)/den;w=(ab*au-aa*av)/den
  if t>=0 and w>=0 and t+w<=1:
   q=tuple(a[k]+t*u[k]+w*v[k] for k in range(3));return inner(q,q)
 best=None
 for i in range(3):
  a=p[i];e=sub(p[(i+1)%3],a);square=inner(e,e)
  t=max(F(0),min(F(1),-inner(a,e)/square)) if square else F(0)
  q=tuple(a[k]+t*e[k] for k in range(3));value=inner(q,q);best=value if best is None else min(best,value)
 return best
def audit_event(work,name,sphere,tilted,format):
 row=read(work/(name+'_accepted.json'));old=read(work/(name+'_initial.json'));raw=read(work/(name+'_source.json'))
 state=read(work/(name+'_states.json'));ledger=read(work/(name+'_projection.json'))
 sources({'original':raw}) # Shared source identities, coordinates, tensors and positive original/extension cells.
 src=[]
 for pr in (0,1):
  records=[r for r in raw if int(r['component']!=MAX)==pr]
  group,_=sources({'original':records})
  bykey={tuple(sorted(n[0] for n in r['vertices'])):r for r in records}
  for c in group:c['ids']=[n[0] for n in bykey[c['key']]['vertices']];src.append((pr,c))
 ordered=[c for pr,c in src];nodes={n[0]:tuple(F(x) for x in n[1:]) for n in row['nodes']}
 tetpoints=[[nodes[n] for n in c['nodes']] for c in row['cells']];volumes=[determinant(p)/6 for p in tetpoints];assert min(volumes)>0
 for a,b in combinations(tetpoints,2):assert not overlap(a,b),'Accepted interior overlap'
 faces={};edges=set();nodal_volume={n:F(0) for n in nodes};minimum_q=1.;minimum_j=1.;maximum_l=0.
 for ids,p,v in zip([c['nodes'] for c in row['cells']],tetpoints,volumes):
  for n in ids:nodal_volume[n]+=v/4
  for f in ((ids[1],ids[2],ids[3]),(ids[0],ids[3],ids[2]),(ids[0],ids[1],ids[3]),(ids[0],ids[2],ids[1])):faces.setdefault(tuple(sorted(f)),[]).append(f)
  centroid=tuple(F(float(sum(x[k] for x in p)/4)) for k in range(3));m=metric(ordered,centroid);dm=spd(m);dv=determinant(p)
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
 for a,b in edges:maximum_l=max(maximum_l,exact_length(src,nodes[a],nodes[b]))
 assert maximum_l<=1.8+1e-10
 accepted={tuple(r['point']):r for r in state['accepted']};assert len(accepted)==len(nodes)
 field_integrals=[F(0)]*15;metric_error=0.
 for n,p in nodes.items():
  values=accepted[tuple(float(x) for x in p)];expected=metric(ordered,p)
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
 ref={f['id']:f for f in row['facets']};max_error=0.;maximum_nodal_error=0.
 origin=tuple(map(F,(2,-1,.5))) if tilted else (F(0),)*3
 def inner(a,b):
  value=sum(x*y for x,y in zip(a,b))
  return value if sphere else value-(sum(a)*sum(b)/3 if tilted else a[2]*b[2])
 for f in row['surface']:
  marker=row['marker_names'][ref[f['facet']]['marker']];points=[nodes[n] for n in f['nodes']]
  if marker.startswith('cap'):
   level=0 if marker=='cap0' else 1
   for p in points:
    d=sub(p,origin);axial=float(sum(d))/math.sqrt(3) if tilted else float(d[2])
    assert abs(axial-level)<1e-12 and float(inner(d,d))<=1+2e-12
  else:
   for p in points:
    d=sub(p,origin);maximum_nodal_error=max(maximum_nodal_error,abs(math.sqrt(float(inner(d,d)))-1))
   error=1-math.sqrt(float(minimum_square([sub(p,origin) for p in points],inner)))
   max_error=max(max_error,error);assert error<=(.22 if sphere else .04)+1e-12
   for i in range(9):
    for j in range(9-i):
     p=tuple(points[0][k]*F(8-i-j,8)+points[1][k]*F(i,8)+points[2][k]*F(j,8) for k in range(3))
     sampled=1-math.sqrt(float(inner(sub(p,origin),sub(p,origin))))
     assert sampled<=error+1e-12
 assert maximum_nodal_error<1e-12
 analytic_volume=4*math.pi/3 if sphere else math.pi
 assert float(sum(volumes))<=analytic_volume+1e-12
 # Match the actual writer by physical mesh/marker signature rather than partition-dependent identities.
 event=int(name.rsplit('_s',1)[1]);stem=name.rsplit('_s',1)[0];file=work/(stem+'_mesh_%05d.'%(event+1)+('cgns' if format=='CGNS' else 'su2'))
 assert (cgns_signature(file,set(row['marker_names'])) if format=='CGNS' else su2_signature(file,set(row['marker_names'])))==signature(row,set(row['marker_names']))
 inputs[str(file.relative_to(HERE.parent))]=sha(file)
 sidecar=Path(str(file)+'.native_ref');inputs[str(sidecar.relative_to(HERE.parent))]=sha(sidecar)
 assert sidecar.read_bytes()==(work/(stem+'.su2.native_ref')).read_bytes()
 return {'cells':len(tetpoints),'points':len(nodes),'volume':float(sum(volumes)),'analytic_domain_volume':analytic_volume,'minimum_metric_mean_ratio':minimum_q,'minimum_metric_scaled_jacobian':minimum_j,'maximum_exact_source_metric_edge':maximum_l,'transported_metric_relative_error':metric_error,'exact_triangle_radial_error':max_error,'maximum_nodal_error':maximum_nodal_error,'fill_volume':ledger['fill_volume'],'lost_volume':ledger['lost_volume'],'conservative_actual_state_integrals_match_target':True,'exact_positive_volume_conformity_no_overlap':True}

case=HERE.parent/sys.argv[1];receipt=read(case/'validation.json');missing=[]
for path,digest in receipt['artifact_sha256'].items():assert sha(case/path)==digest,'Runtime evidence drift'
for ranks in (1,2,4):
 work=case/('mpi'+str(ranks))
 if not work.exists():missing.append({'ranks':ranks,'status':'NOT_RUN'});continue
 rows={}
 for m in range(1,ranks+1):
  for sphere,tilted in ((False,False),(False,True),(True,False)):
   name=('sphere' if sphere else 'cylinder')+'_axis'+str(int(tilted))+'_m'+str(m)
   if not (work/(name+'_s1_accepted.json')).exists():missing.append({'ranks':ranks,'case':name,'status':'NOT_COMPLETED'});continue
   events=[audit_event(work,name+'_s'+str(c),sphere,tilted,'SU2') for c in range(2)]
   restart=read(work/(name+'_restart_states.json'));wanted={tuple(r['point']):r['states'] for r in restart['donor']}
   assert all(max(abs(a-b)/max(1.,abs(a)) for a,b in zip(r['states'],wanted[tuple(r['point'])]))<2e-10 for r in restart['accepted'])
   final=read(work/(name+'_s1_accepted.json'))
   for fmt in ('SU2','CGNS'):
    file=work/(name+'_s1_written_'+fmt+('.cgns' if fmt=='CGNS' else '.su2'))
    assert (cgns_signature(file,set(final['marker_names'])) if fmt=='CGNS' else su2_signature(file,set(final['marker_names'])))==signature(final,set(final['marker_names']))
    inputs[str(file.relative_to(HERE.parent))]=sha(file)
   rows[name]=events
 results[str(ranks)]=rows
assert any(results.values()),'No complete primitive lifecycle to audit'
report=dict(status='PASS_COMPLETED_LIFECYCLES_ONLY',missing=missing,runtime_receipt_status=receipt['status'],runtime_folder=sys.argv[1],results=results,input_sha256=inputs,auditor_sha256=sha(Path(__file__)),scope='Exact geometry, positive signed volumes, manifold skin, no interior overlap, original-priority declared metric coverage, metric length/shape, transported tensors, actual conservative current/BDF integrals, positivity, restart and SU2-CGNS readback for COMPLETED primitive lifecycles only. Missing/rejected variants are not qualified. No performance/cluster/full Goal2 claim.')
(HERE/(sys.argv[1]+'_audit.json')).write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(results,indent=2))
