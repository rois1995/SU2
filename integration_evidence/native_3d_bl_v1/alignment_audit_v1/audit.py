#!/usr/bin/env python3
"""Independent cube-policy geometry, full metric, state and restart audit; no CFD-accuracy claim."""
import hashlib,itertools,json,math,sys
from collections import defaultdict
from fractions import Fraction as F
from pathlib import Path
import numpy as np
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2]
sys.path.insert(0,str(ROOT/'integration_evidence/native_3d_core_v1'))
import audit_coupled as base

def digest(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def matrix(m):return np.array([[float(m[0]),float(m[1]),float(m[2])],[float(m[1]),float(m[3]),float(m[4])],[float(m[2]),float(m[4]),float(m[5])]])
def packed(m):return tuple(F(float(m[i,j])) for i,j in [(0,0),(0,1),(0,2),(1,1),(1,2),(2,2)])
def target(original,p):
 s=matrix(base.metric(original,p));z=float(p[2]);h=max(.1,2*(.1+.2*z)/2.2)
 u=max(0.,min(1.,(z-.27)/.03));w=1-u*u*(3-2*u)
 core=min(float(np.linalg.eigvalsh(matrix(m))[0]) for cell in original for m in cell['metrics'])
 low=.5**w*core**(1-w);high=1.5**w*core**(1-w);normal=h**(-2*w)*core**(1-w)
 b=np.array([[(low+high)/2,(low-high)/2,0],[(low-high)/2,(low+high)/2,0],[0,0,normal]])
 # Independent SPD intersection in the sensor frame; no source/BL interpolation shortcut.
 values,vectors=np.linalg.eigh(s);root=(vectors*np.sqrt(values))@vectors.T;inv=(vectors/np.sqrt(values))@vectors.T
 values,vectors=np.linalg.eigh(inv@b@inv);out=root@(vectors*np.maximum(values,1))@vectors.T@root
 return packed((out+out.T)/2)
def edge(original,a,b):
 d=base.sub(b,a)
 def speed(t):return math.sqrt(float(base.quad(target(original,tuple(F(float((1-t)*x+t*y)) for x,y in zip(a,b))),d)))
 cuts=[0.,1.]
 if a[2]!=b[2]:cuts += [float((F(z)-a[2])/(b[2]-a[2])) for z in [.05,.27,.3] if 0<float((F(z)-a[2])/(b[2]-a[2]))<1]
 def integrate(a,b,fa,fm,fb,old,tol,depth):
  mid=(a+b)/2;fl,fr=speed((a+mid)/2),speed((mid+b)/2);left=(mid-a)*(fa+4*fl+fm)/6;right=(b-mid)*(fm+4*fr+fb)/6;delta=left+right-old
  if abs(delta)<=15*tol:return left+right+delta/15
  assert depth<24,'Independent full-edge reference did not converge'
  return integrate(a,mid,fa,fl,fm,left,tol/2,depth+1)+integrate(mid,b,fm,fr,fb,right,tol/2,depth+1)
 total=0
 cuts=sorted(set(cuts))
 for lower,upper in zip(cuts,cuts[1:]):
  fa,fm,fb=speed(lower),speed((lower+upper)/2),speed(upper);total+=integrate(lower,upper,fa,fm,fb,(upper-lower)*(fa+4*fm+fb)/6,1e-10*(upper-lower),0)
 return total

def signature(data):
 nodes={n[0]:tuple(map(float,n[1:])) for n in data['nodes']}
 for c in data['cells']:assert base.determinant([tuple(map(F,nodes[n])) for n in c['nodes']])>0
 def oriented(face):
  f=tuple(nodes[n] for n in face);return min(f[i:]+f[:i] for i in range(3))
 return {'cells':sorted(tuple(sorted(nodes[n] for n in c['nodes'])) for c in data['cells']),
  'surface':sorted((f['facet'],oriented(f['nodes'])) for f in data['surface']),
  'facets':data['facets'],'facet_groups':data['facet_groups'],'marker_names':data['marker_names'],
  'source':sorted(tuple(sorted((tuple(v[1:4]),tuple(v[4])) for v in c['vertices'])) for c in data['original'])}
def integrals(data,records,donor=False):
 values={r['id']:r['states'] for r in records};result=[F(0)]*15
 if donor:
  rows=[([v[0] for v in c['vertices']],[tuple(map(F,v[1:4])) for v in c['vertices']]) for c in data['original']]
 else:
  nodes={n[0]:tuple(map(F,n[1:])) for n in data['nodes']};rows=[(c['nodes'],[nodes[n] for n in c['nodes']]) for c in data['cells']]
 for ids,points in rows:
  volume=base.determinant(points)/6
  for k in range(15):result[k]+=volume*sum(F(values[n][k]) for n in ids)/4
 return list(map(float,result))
def gradation(original,composed):
 pairs=[]
 for c in original:
  for a,b in itertools.combinations(c['points'],2):
   q=[tuple((1-F(k,4))*x+F(k,4)*y for x,y in zip(a,b)) for k in range(5)];pairs.extend(zip(q,q[1:]));pairs.append((a,b))
 if composed:
  q=[(F(1,2),F(1,2),F(z)) for z in [0,.025,.05,.1,.2,.27,.2775,.285,.2925,.3,.5,1]];pairs.extend(zip(q,q[1:]))
 maxima=[1.,1.];bad=[0,0]
 for a,b in pairs:
  for x,y in [(a,b),(b,a)]:
   for k,query in enumerate([base.metric,target] if composed else [base.metric]):
    s=matrix(query(original,x));t=matrix(query(original,y));v,q=np.linalg.eigh(s);inv=q/np.sqrt(v);d=np.array(list(map(float,base.sub(y,x))));factor=(1+math.log(1.3)*math.sqrt(max(0.,float(d@t@d))))**-2
    ratio=float(np.linalg.eigvalsh(factor*(inv.T@t@inv))[-1]);maxima[k]=max(maxima[k],ratio);bad[k]+=ratio>1+1e-5
 return {'maximum_sensor_ratio':maxima[0],'maximum_composed_ratio':maxima[1] if composed else None,'sensor_violations':bad[0],'composed_violations':bad[1] if composed else None,'scope':'Original donor edges, their dyadic subintervals, and an independent wall-normal/fade column. Finite probes, not a global certificate; no target correction.'}
def main():
 legacy=sys.argv[1].isdigit()
 folder=HERE/('mpi'+sys.argv[1]) if legacy else Path(sys.argv[1]).resolve()
 receipt_path=folder.with_suffix('.json');receipt=json.loads(receipt_path.read_text());assert receipt['status']=='PASS'
 ranks=receipt['contract']['MPI_ranks'];assert not legacy or ranks==int(sys.argv[1])
 assert all(digest(folder/p)==h for p,h in receipt['artifacts'].items()),'Native evidence changed'
 reports=[]
 for path in sorted(folder.glob('policy_m*_s[01].json')):
  data=json.loads(path.read_text());original,_=base.sources(data);composed=path.stem.endswith('_s0')
  assert max(abs(float(m[i])-([1,0,0,1,0,1][i])) for c in original for m in c['metrics'] for i in range(6))<2e-14,'This analytic cube oracle requires the declared clipped identity sensor'
  measured,_=base.frame(path,True,metric_query=target if composed else base.metric,edge_query=edge if composed else base.length);assert measured['volume']=='1'
  height=[];tilt=[];footprint=[];nodes={n[0]:tuple(map(F,n[1:])) for n in data['nodes']}
  for face in data['surface']:
   points=[nodes[n] for n in face['nodes']]
   if not composed or any(p[2]!=0 for p in points):continue
   cells=[c for c in data['cells'] if set(face['nodes']).issubset(c['nodes'])];assert len(cells)==1;apex=nodes[next(n for n in cells[0]['nodes'] if n not in face['nodes'])]
   a,b,c=points;q=(apex[0],apex[1],F(0))
   def area(u,v,w):return (v[0]-u[0])*(w[1]-u[1])-(v[1]-u[1])*(w[0]-u[0])
   total=area(a,b,c);weights=[area(q,b,c)/total,area(a,q,c)/total,area(a,b,q)/total];footprint.append(float(min(weights)))
   assert min(weights)>=F(-1e-14),'Accepted first-row normal foot leaves wall triangle'
   height.append(abs(float(apex[2])/.1-1));center=[float(sum(p[k] for p in points)/3) for k in range(2)];tilt.append(math.hypot(float(apex[0])-center[0],float(apex[1])-center[1])/.1)
  assert not composed or height and max(height)<1e-8
  states=json.loads(path.with_name(path.stem+'_states.json').read_text());donor=integrals(data,states['donor'],True);accepted=integrals(data,states['accepted']);defect=max(abs(a-b)/max(1.,abs(a)) for a,b in zip(donor,accepted));assert defect<1e-10
  transport=0
  for row in states['accepted']:
   expected=base.metric(original,tuple(map(F,row['point'])));transport=max(transport,max(abs(float(a)-b) for a,b in zip(expected,row['metric'])))
   for level in range(3):
    u=row['states'][5*level:5*level+5];assert u[0]>0 and u[4]-sum(x*x for x in u[1:4])/(2*u[0])>0
  assert transport<2e-14
  projection=json.loads(path.with_name(path.stem+'_projection.json').read_text());assert projection['time_levels']==2
  projection_defect=max(abs(a-b)/max(1.,abs(sc)) for a,b,sc in zip(projection['new_total'],projection['target_total'],projection['scale']));assert projection_defect<1e-10
  reloads=[]
  for reload in sorted(folder.glob(path.stem+'_written_*_reload_target.json')):
   assert signature(json.loads(reload.read_text()))==signature(data),'Reload changed geometry/reference/original P1 target'
   reloads.append({'file':reload.name,'sha256':digest(reload),'geometry_and_target_equal':True})
  assert len(reloads)==2,'Configured build requires SU2 and CGNS reload'
  measured.update(first_height_faces=len(height),maximum_height_error=max(height,default=0),maximum_tilt=max(tilt,default=0),minimum_footprint_barycentric=min(footprint,default=1/3),independent_flow_BDF_integral_defect=defect,projection_all_field_defect=projection_defect,transported_sensor_error=transport,gradation=gradation(original,composed),reloads=reloads)
  reports.append(measured);print(path.name,'PASS',flush=True)
 for path in sorted(folder.glob('policy_m*_restart_states.json')):
  d=json.loads(path.read_text());wanted={tuple(r['point']):r['states'] for r in d['donor']};assert len(d['accepted'])==len(wanted)
  assert max(abs(a-b)/max(1.,abs(a)) for r in d['accepted'] for a,b in zip(wanted[tuple(r['point'])],r['states']))<1e-10
 result={'status':'PASS_LIMITED_SCOPE','MPI_ranks':ranks,'frames':reports,'native_receipt_sha256':digest(receipt_path),'helper_sha256':digest(Path(__file__)),'house_geometry_auditor_sha256':digest(ROOT/'integration_evidence/native_3d_core_v1/audit_coupled.py'),'scope':'Exact cube embedding/coverage, independent full composed metric lengths/shape/h0, actual original P1 sensor transport, current+BDF flow integrals/positivity and real restart, and geometry/target-equivalent SU2/CGNS reload. Native checks separately cover turbulence bounds/history. Composed gradation violations are reported, not corrected. No arbitrary geometry/sensors, CFD convergence/accuracy or performance qualification.'}
 (HERE/('independent_mpi'+str(ranks)+'.json') if legacy else folder/'independent_policy.json').write_text(json.dumps(result,indent=2)+'\n')
if __name__=='__main__':main()
