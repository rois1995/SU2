"""Independent rational closed-intersection/coverage, actual P1 and mesh checks for large sources."""
from fractions import Fraction as F
import hashlib,itertools,json,math,sys,tempfile
from pathlib import Path
from audit_cavities import determinant,cross,dot,sub
from audit_coupled import frame,weights

def grid(n,mode="box"):
 xyz=[(0,0,0),(1,0,0),(1,1,0),(0,1,0),(0,0,1),(1,0,1),(1,1,1),(0,1,1)]
 ring=[1,2,3,7,4,5];result=[]
 for z,y,x in itertools.product(range(n),repeat=3):
  nodes=[(a+x,b+y,c+z) for a,b,c in xyz]
  pts=[tuple(F(float(q)/n) for q in p) for p in nodes]
  if mode=='thin8':pts=[(x,y,z/8) for x,y,z in pts]
  if mode=='shear4':pts=[(x+4*z,y,z) for x,y,z in pts]
  ids=[a+(n+1)*(b+(n+1)*c) for a,b,c in nodes]
  for i in range(6):
   indices=[0,6,ring[i],ring[(i+1)%6]]
   result.append(dict(points=[pts[j] for j in indices],key=tuple(sorted(ids[j] for j in indices))))
 return result

def closed_intersection(a,b):
 # Independent full 44-axis rational SAT. Strict gaps only: touching donors are required too.
 def axes(t):return [cross(sub(t[j],t[i]),sub(t[k],t[i])) for i,j,k in itertools.combinations(range(4),3)]
 ea=[sub(a[j],a[i]) for i,j in itertools.combinations(range(4),2)]
 eb=[sub(b[j],b[i]) for i,j in itertools.combinations(range(4),2)]
 for axis in axes(a)+axes(b)+[cross(x,y) for x in ea for y in eb]:
  if not any(axis):continue
  pa=[dot(axis,p) for p in a];pb=[dot(axis,p) for p in b]
  if max(pa)<min(pb) or max(pb)<min(pa):return False
 return True

def query_hit(t,a,b):
 lo,hi=F(0),F(1)
 for x,y in zip(weights(t,a),weights(t,b)):
  if x<0 and y<0:return False
  if x<0:lo=max(lo,x/(x-y))
  if y<0:hi=min(hi,x/(x-y))
  if lo>hi:return False
 return True

def audit(folder,ranks):
 log=folder/('mpi'+str(ranks)+'.stdout');imports={};queries=[];events={};work={};support={};support_queries=[];limits=[]
 for line in log.read_text().splitlines():
  row=line.split()
  if not row or not row[0].startswith('LARGE_'):continue
  assert all('=' in item for item in row[1:]);v=dict(item.split('=',1) for item in row[1:])
  if row[0]=='LARGE_IMPORT':
   key=(int(v['n']),int(v['cell']),int(v['rank']));assert key not in imports;imports[key]=v
  elif row[0]=='LARGE_QUERY':queries.append(v)
  elif row[0]=='LARGE_SUPPORT':
   key=(int(v['n']),int(v['cell']),v['mode'],int(v['rank']));assert key not in support;support[key]=v
  elif row[0]=='LARGE_SUPPORT_QUERY':support_queries.append(v)
  elif row[0]=='LARGE_SUPPORT_LIMIT':limits.append(v)
  elif row[0]=='LARGE_EVENT':
   key=(v['case'],int(v['rank']));assert key not in events;events[key]=v
  elif row[0]=='LARGE_ENGINE':
   key=(v['case'],int(v['rank']));assert key not in work;work[key]=v
 expected_m={ranks,max(1,ranks-1),max(1,ranks//2)}
 assert set(imports)=={(n,i,r) for n in [3,4,5,6] for i in range(6) for r in range(ranks)}
 assert len(queries)==48
 originals={n:grid(n) for n in [3,4,5,6]};coarse=grid(1);closed_checks=0;required=0;overimports=0
 for (n,i,r),row in imports.items():
  assert bool(int(row['valid']))==(n<5)
  ids=[int(x) for x in row['ids'].split(',') if x];assert len(ids)==len(set(ids))==int(row['count'])
  if r or n>=5:assert not ids;continue
  assert len(ids)==(162 if n==3 else 170)
  needed={j for j,c in enumerate(originals[n]) if closed_intersection(coarse[i]['points'],c['points'])}
  assert needed<=set(ids),'Closed-intersecting source excluded'
  assert set(ids)<=set(range(len(originals[n])))
  required+=len(needed);overimports+=len(set(ids)-needed);closed_checks+=len(originals[n])
 for row in queries:
  n,i=int(row['n']),int(row['cell']);p=tuple(F(float(row[k])) for k in ('x','y','z'))
  assert min(weights(coarse[i]['points'],p))>=0
  valid=[c['key'] for c in originals[n] if min(weights(c['points'],p))>=0]
  assert valid and tuple(int(x) for x in row['donor'].split(',') if x)==min(valid)
  sensor=1+.1*float(p[0]);assert abs(float(row['sensor'])-sensor)<2e-14
  assert abs(float(row['target'])-sensor-float(p[2]))<2e-14
 support_cases=[(3,'box'),(4,'box'),(5,'box'),(6,'box'),(4,'thin8'),(4,'shear4')]
 assert set(support)=={(n,i,mode,r) for n,mode in support_cases for i in range(6) for r in range(ranks)}
 assert len(support_queries)==36 and limits==[dict(n='7',valid='0',source='2058')]
 support_checks=0
 for (n,i,mode,r),row in support.items():
  ids=[int(x) for x in row['ids'].split(',') if x];assert len(ids)==len(set(ids))==int(row['count'])
  if r:assert not ids;continue
  t=grid(1,mode)[i]['points'];center=tuple(sum(p[j] for p in t)/4 for j in range(3))
  source=grid(n,mode)
  needed={j for j,c in enumerate(source) if query_hit(c['points'],center,center) or
   any(query_hit(c['points'],a,b) for a,b in itertools.combinations(t,2))}
  assert needed<=set(ids),'Closed query-support donor excluded'
  assert (set(ids)==needed if n>3 else len(ids)==162)
  support_checks+=len(source)
 for row in support_queries:
  n,i=int(row['n']),int(row['cell']);mode=row['mode'];t=grid(1,mode)[i]['points'];p=tuple(F(float(row[k])) for k in ('x','y','z'))
  assert p==tuple(sum(v[j] for v in t)/4 for j in range(3))
  valid=[c['key'] for c in grid(n,mode) if min(weights(c['points'],p))>=0]
  assert valid and tuple(int(x) for x in row['donor'].split(',') if x)==min(valid)
  sensor=1+.1*float(p[0]);assert abs(float(row['sensor'])-sensor)<2e-14
  assert abs(float(row['target'])-sensor-float(p[2]))<2e-14
  lengths=[float(x) for x in row['lengths'].split(',') if x];assert len(lengths)==6
  # Independent analytic integral of sqrt((1+0.1*x(t))*|edge|^2); the source is affine.
  for measured,(a,b) in zip(lengths,itertools.combinations(t,2)):
   u,v=1+.1*float(a[0]),1+.1*float(b[0]);d=math.sqrt(sum(float(x-y)**2 for x,y in zip(a,b)))
   expected=d*(math.sqrt(u) if u==v else 2*(u+math.sqrt(u*v)+v)/(3*(math.sqrt(u)+math.sqrt(v))))
   assert abs(measured-expected)<3e-14
 assert sum(query_hit(c['points'],tuple(sum(p[j] for p in coarse[0]['points'])/4 for j in range(3)),
  tuple(sum(p[j] for p in coarse[0]['points'])/4 for j in range(3))) or
  any(query_hit(c['points'],a,b) for a,b in itertools.combinations(coarse[0]['points'],2)) for c in grid(7))>256
 kinds=('coarse','wedge','dense','dense_wedge','dense_coarsen','dense_repair','dense_reject')
 labels={kind+'_m'+str(m) for kind in kinds for m in expected_m}
 assert set(events)=={(label,r) for label in labels for r in range(ranks)}
 assert set(work)=={(label,r) for label in labels for r in range(int(label.rsplit('_m',1)[1]))}
 frames=[]
 for kind in kinds:
  path=folder/('mpi'+str(ranks)+'_meshes')/(kind+'_initial.json')
  result,_=frame(path,kind in ('coarse','dense','dense_coarsen'));result['accepted_event']=False;result['role']='initial'
  assert result['cells']==dict(coarse=6,wedge=1,dense=6,dense_wedge=1,dense_coarsen=2,dense_repair=4,dense_reject=1)[kind]
  assert result['volume']==('1' if kind in ('coarse','dense') else '1/6')
  if 'wedge' in kind:assert result['maximum_length']>1.8
  frames.append(result)
 for label in sorted(labels):
  kind=label.rsplit('_m',1)[0];m=int(label.rsplit('_m',1)[1]);valid=kind!='dense_reject'
  for r in range(ranks):
   v=events[(label,r)];assert int(v['ranks'])==ranks and int(v['workers'])==m and bool(int(v['valid']))==valid
   parts=sum(float(v[k]) for k in ('migration_s','worker_comm_s','worker_s','return_s','other_s'))
   assert float(v['other_s'])>=-1e-12 and abs(parts-float(v['parent_s']))<1e-10
  for r in range(m):
   v=work[(label,r)];assert bool(int(v['valid']))==valid and int(v['max_patch'])<=256
   if kind in ('dense','dense_wedge','dense_reject'):assert int(v['query_import_retries'])>0
   assert (int(v['private_query_retries'])>0)==(kind.startswith('dense_'))
   parts=sum(float(v[k]) for k in ('init_s','seed_s','selection_s','dependency_s','reservation_s','donor_s',
   'reconstruction_s','metric_s','routing_s','publication_s','update_s','completion_s','other_s'))
   assert float(v['other_s'])>=-1e-12 and abs(parts-float(v['total_s']))<1e-10
   assert 0<=float(v['filter_nested_s'])<=float(v['search_nested_s'])+1e-10
  accepted=sum(int(work[(label,r)]['accepted']) for r in range(m))
  assert (accepted>0)==(kind not in ('coarse','dense'))
  path=folder/('mpi'+str(ranks)+'_meshes')/(label+('' if valid else '_rejected')+'.json');result,_=frame(path,valid)
  result['accepted_event']=valid;result['role']='accepted' if valid else 'rejected'
  assert result['volume']==('1' if kind in ('coarse','dense') else '1/6')
  assert result['cells']==int(events[(label,0)]['final'])
  frames.append(result)
 result=dict(status='PASS',closed_sat_checks=closed_checks,required_donor_intersections=required,
 conservative_extra_donors=overimports,query_support_checks=support_checks,support_queries=len(support_queries),queries=len(queries),frames=frames,events=list(events.values()),work=list(work.values()),
 source_log_sha256=hashlib.sha256(log.read_bytes()).hexdigest(),
 scope='Independent rational full closed-intersection SAT proves no donor omissions for the declared coarse tetrahedra; canonical original P1 queries and actual composition, source384 coarse/wedge engine events on explicit worker subsets, source750 valid coarse no-op, known-query refine/coarsen/repair and round-exhaustion rejection; source2058 query-support cap rejection, exact accepted global geometry/marker/metric/SU2 audits. No runtime/CFD/CGNS/scaling qualification.')
 out=folder/('mpi'+str(ranks)+'_large_audit.json');out.write_text(json.dumps(result,indent=2)+'\n');return result
def negative_controls(folder):
 lines=(folder/'mpi1.stdout').read_text().splitlines()
 first=next(i for i,v in enumerate(lines) if v.startswith('LARGE_IMPORT n=3 cell=0 rank=0 '))
 variants=[lines[:first]+lines[first+1:]]
 changed=lines[:];parts=changed[first].split();values=dict(v.split('=',1) for v in parts[1:])
 ids=[int(x) for x in values['ids'].split(',') if x];assert 0 in ids;ids.remove(0)
 values['ids']=','.join(map(str,ids))+',';values['count']=str(len(ids))
 changed[first]=parts[0]+' '+' '.join(k+'='+v for k,v in values.items());variants.append(changed)
 changed=lines[:];i=next(i for i,v in enumerate(changed) if v.startswith('LARGE_QUERY '));parts=changed[i].split()
 for j,p in enumerate(parts):
  if p.startswith('sensor='):parts[j]='sensor='+str(float(p.split('=')[1])+1)
 changed[i]=' '.join(parts);variants.append(changed)
 changed=lines[:];i=next(i for i,v in enumerate(changed) if v.startswith('LARGE_SUPPORT n=5 cell=0 rank=0 '))
 variants.append(changed[:i]+changed[i+1:])
 parts=changed[i].split();values=dict(v.split('=',1) for v in parts[1:]);ids=[int(x) for x in values['ids'].split(',') if x]
 ids.pop();values['ids']=','.join(map(str,ids))+',';values['count']=str(len(ids))
 changed[i]=parts[0]+' '+' '.join(k+'='+v for k,v in values.items());variants.append(changed)
 changed=lines[:];i=next(i for i,v in enumerate(changed) if v.startswith('LARGE_SUPPORT_QUERY '));parts=changed[i].split()
 for j,p in enumerate(parts):
  if p.startswith('lengths='):
   values=p.split('=',1)[1].split(',');values[0]=str(float(values[0])+1);parts[j]='lengths='+','.join(values)
 changed[i]=' '.join(parts);variants.append(changed)
 for bad in variants:
  with tempfile.TemporaryDirectory(prefix='su2_large_source_audit_') as temporary:
   case=Path(temporary);(case/'mpi1.stdout').write_text('\n'.join(bad)+'\n');failed=False
   try:audit(case,1)
   except (AssertionError,KeyError,ValueError):failed=True
   assert failed,'Missing closed donor or invalid P1 observation accepted'
if __name__=='__main__':
 folder=Path(sys.argv[1]);ranks=int(sys.argv[2]);r=audit(folder,ranks)
 print('PASS',r['closed_sat_checks'],'closed SAT pairs;',len(r['frames']),'mesh frames')
 if ranks==1:
  negative_controls(folder);print('PASS 6 full/query-support omission and P1/edge oracle adversaries')
