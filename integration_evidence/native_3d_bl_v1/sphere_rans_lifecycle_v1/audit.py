#!/usr/bin/env python3
"""Independent actual-state integrals and output checks; exact geometry reused only by identity."""
import collections,csv,gzip,hashlib,inspect,itertools,json,math,sys,time
from fractions import Fraction as F
from pathlib import Path
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2];OLD=HERE.with_name('closed_sphere_construction_v1')
CORE=ROOT/'integration_evidence/native_3d_core_v1';sys.path.insert(0,str(CORE))
from audit_cavities import determinant,overlap,sub
from audit_coupled import surface,connected
from audit_geometry_bridge import signature,su2_signature,cgns_signature

def sha(p):
 h=hashlib.sha256()
 with p.open('rb') as f:
  for b in iter(lambda:f.read(2**20),b''):h.update(b)
 return h.hexdigest()
def read(p):return json.loads(p.read_text())
def turbulence(p):
 rows=list(csv.DictReader(p.open()));result={tuple(float(r[k]) for k in ['x','y','z']):[float(r[k]) for k in ['current','n','n1']] for r in rows}
 assert len(result)==len(rows) and all(math.isfinite(x) for r in result.values() for x in r)
 return result
def geometry(row):
 nodes={n[0]:tuple(F(x) for x in n[1:]) for n in row['nodes']};assert len(nodes)==len(row['nodes'])==len(set(nodes.values()))
 assert len({c['id'] for c in row['cells']})==len(row['cells'])
 assert set(nodes)=={n for c in row['cells'] for n in c['nodes']}
 lumped={n:F(0) for n in nodes};volume=F(0)
 for c in row['cells']:
  ids=c['nodes'];assert len(set(ids))==4;v=determinant([nodes[n] for n in ids])/6;assert v>0;volume+=v
  for n in ids:lumped[n]+=v/4
 return nodes,lumped,volume

def fresh_identity_geometry(row,folder):
 assert len(row['cells'])<=1024,'Fresh independent geometry requires a separately bounded larger campaign'
 start=time.monotonic();nodes,_,volume=geometry(row);uses=collections.defaultdict(list);links=collections.defaultdict(list);tets=[];edges=set();qmin=jmin=1.
 for index,c in enumerate(row['cells']):
  ids=c['nodes'];t=[nodes[n] for n in ids];dv=determinant(t);tets.append(t)
  for f in [(ids[1],ids[2],ids[3]),(ids[0],ids[3],ids[2]),(ids[0],ids[1],ids[3]),(ids[0],ids[2],ids[1])]:
   uses[tuple(sorted(f))].append((index,(-1)**sum(f[i]>f[j] for i in range(3) for j in range(i+1,3))))
  for i,n in enumerate(ids):links[n].append(tuple(v for j,v in enumerate(ids) if i!=j))
  lengths={(i,j):math.sqrt(float(sum(x*x for x in sub(t[j],t[i])))) for i,j in itertools.combinations(range(4),2)}
  q=12*(float(dv)/2)**(2./3)/sum(x*x for x in lengths.values())
  jac=min(math.sqrt(2)*float(dv)/math.prod(lengths[tuple(sorted((i,j)))] for j in range(4) if i!=j) for i in range(4))
  assert q>=.2 and jac>=.05;qmin=min(qmin,q);jmin=min(jmin,jac)
  edges.update(tuple(sorted((ids[i],ids[j]))) for i,j in itertools.combinations(range(4),2))
 assert all(len(v)==1 or len(v)==2 and sum(x[1] for x in v)==0 for v in uses.values())
 boundary={f for f,v in uses.items() if len(v)==1};assert collections.Counter(boundary)==collections.Counter(tuple(sorted(f['nodes'])) for f in row['surface']);surface(list(boundary),True)
 boundary_nodes={n for f in boundary for n in f};adjacency={i:set() for i in range(len(tets))}
 for v in uses.values():
  if len(v)==2:a,b=v[0][0],v[1][0];adjacency[a].add(b);adjacency[b].add(a)
 assert connected(adjacency)
 for n,faces in links.items():surface(faces,n not in boundary_nodes)
 maximum=max(math.sqrt(float(sum(x*x for x in sub(nodes[a],nodes[b])))) for a,b in edges);assert maximum<=1.8
 state=folder/'fresh_geometry_progress.json';identity={'mesh_sha256':sha(folder/'event_accepted.json'),'helper_sha256':sha(Path(__file__))}
 saved=read(state) if state.exists() else {'identity':identity,'next_cell':0,'pairs':0,'shared_faces':0,'seconds':0}
 assert saved['identity']==identity
 ids=[set(c['nodes']) for c in row['cells']];pairs=saved['pairs'];shared=saved['shared_faces']
 for i in range(saved['next_cell'],len(tets)):
  for j in range(i+1,len(tets)):
   if len(ids[i]&ids[j])==3:shared+=1;continue
   assert not overlap(tets[i],tets[j]),('Positive overlap',i,j);pairs+=1
  if i%32==0 or i+1==len(tets):
   state.write_text(json.dumps({'identity':identity,'next_cell':i+1,'pairs':pairs,'shared_faces':shared,'seconds':saved['seconds']+time.monotonic()-start},indent=2)+'\n')
   if time.monotonic()-start>180:raise RuntimeError('Fresh exact audit reached180seconds; completed pairs retained for resume')
 return {'kind':'fresh_exact_topology_positive_volumes_no_positive_overlap_and_complete_identity_field','cells':len(tets),'pairs':pairs,'shared_faces':shared,'vertex_links':len(links),'minimum_q':qmin,'minimum_j':jmin,'maximum_full_edge':maximum,'volume':str(volume),'seconds':saved['seconds']+time.monotonic()-start}

def audit(folder,stage):
 start=time.monotonic();receipt=read(folder/'result.json');proof=read(folder/'provenance.json');assert receipt['exit_code']==0
 assert sha(folder/'provenance.json')==receipt['provenance_sha256'] and sha(folder/'run.log')==receipt['log_sha256']
 capsule=json.loads(gzip.decompress((folder/'executed_source.json.gz').read_bytes()))
 for name,wanted in proof['pins'].items():
  if name in capsule:assert hashlib.sha256(capsule[name].encode()).hexdigest()==wanted,name
  else:assert sha(ROOT/name)==wanted,name
 for name,wanted in proof['inputs'].items():assert sha(ROOT/name)==wanted,name
 for name,wanted in receipt['artifacts'].items():assert sha(folder/name)==wanted,name
 row=read(folder/'event_accepted.json');old=read(folder/'event_initial.json');states=read(folder/'event_states.json');ledger=read(folder/'event_projection.json')
 nodes,lumped,volume=geometry(row);oldnodes,oldlumped,oldvolume=geometry(old)
 previous=OLD/('geodesic_v1' if stage in ('construct','update') else 'release_regional_v1')
 oracle=read(previous/'independent.json');assert oracle['status'].startswith('PASS_NATIVE_ACCEPTED_')
 for name,wanted in oracle['identity'].items():
  p=ROOT/name
  if sha(p)!=wanted and name.endswith('/audit_sphere.py'):p=OLD/'audit_sphere_construct_executed.py'
  assert sha(p)==wanted,name
 old_source=json.loads(gzip.decompress((previous/'executed_source.json.gz').read_bytes()))
 for name in ['SU2_CFD/src/adaptation/CBoundaryLayerMetric.cpp','SU2_CFD/include/adaptation/CBoundaryLayerMetric.hpp','Common/src/adaptation/CNativeField3D.cpp','Common/include/adaptation/CNativeField3D.hpp','Common/src/adaptation/CNativeMetric3D.cpp']:
  assert sha(ROOT/name)==hashlib.sha256(old_source[name].encode()).hexdigest(),name
 sig=signature(row,{'sphere'});original_signature=su2_signature(previous/'accepted.su2',{'sphere'})
 assert sig[1]==original_signature[1],'Curved boundary differs from retained authority/geometry proof'
 if sig==original_signature:geometry_proof={'kind':'exact_mesh_identity_reuses_retained_geometry_and_native_field_oracle'}
 else:
  assert stage=='release','Changed BL geometry requires a separate full composed-field audit'
  geometry_proof=fresh_identity_geometry(row,folder)
 # Bind the retained small-case parser's read guard to this already bounded completed mesh.
 reader_source=inspect.getsource(cgns_signature);guard='0<count<=1024 and 0<volume_count<=4096'
 assert reader_source.count(guard)==1 and len(nodes)<=4096 and len(row['cells'])<=6144
 reader_source=reader_source.replace(guard,'count==%d and volume_count==%d'%(len(nodes),len(row['cells'])))
 (folder/'cgns_reader_executed.py').write_text(reader_source)
 namespace=dict(cgns_signature.__globals__);exec(compile(reader_source,str(folder/'cgns_reader_executed.py'),'exec'),namespace)
 for fmt,reader in [('SU2',su2_signature),('CGNS',namespace['cgns_signature'])]:
  file=folder/('event_written_'+fmt+('.cgns' if fmt=='CGNS' else '.su2'));assert reader(file,{'sphere'})==sig
  assert Path(str(file)+'.native_ref').read_bytes()==(previous/'accepted.su2.native_ref').read_bytes()
  loaded=read(folder/('event_written_'+fmt+'_reload.json'));assert signature(loaded,{'sphere'})==sig
 for d in old['original']:
  for v in d['vertices']:assert v[4]==[1,0,0,1,0,1]
 if stage in ('construct','update'):
  initial=(folder/'run.cfg').read_text()
  original=(OLD/'geodesic_v1/run.cfg').read_text()
  options=lambda t:{line.split('=',1)[0].strip():line.split('=',1)[1].strip() for line in t.splitlines() if '=' in line and not line.startswith('%')}
  a,b=options(initial),options(original)
  for k in ['ADAP_BL_MARKER','ADAP_BL_FIRST_HEIGHT','ADAP_BL_GROWTH','ADAP_BL_THICKNESS','ADAP_HMIN','ADAP_HMAX','ADAP_ARMAX','ADAP_HAUSD']:assert a[k]==b[k],k
 metric_error=0.;integrals={}
 for label,points,weights,csvfile in [('accepted',nodes,lumped,'accepted_turb.csv'),('donor',oldnodes,oldlumped,'before_turb.csv')]:
  values={tuple(r['point']):r for r in states[label]};nu=turbulence(folder/csvfile)
  assert len(values)==len(points) and set(values)==set(nu)=={tuple(float(x) for x in p) for p in points.values()}
  total=[F(0)]*12
  for n,p in points.items():
   xyz=tuple(float(x) for x in p);r=values[xyz];u=r['states'];assert len(u)==15 and all(math.isfinite(x) for x in u)
   for off in [0,5,10]:assert u[off]>0 and u[off+4]-(sum(x*x for x in u[off+1:off+4]))/(2*u[off])>0
   assert min(nu[xyz])>=0 and max(abs(u[k]-u[k+5]) for k in range(5))<1e-10
   for level,offset in [(0,0),(1,10)]:
    for k in range(5):total[6*level+k]+=weights[n]*F(u[offset+k])
    total[6*level+5]+=weights[n]*F(nu[xyz][0 if level==0 else 2])
   if label=='accepted':metric_error=max(metric_error,max(abs(x-y) for x,y in zip(r['metric'],[1,0,0,1,0,1])))
  integrals[label]=list(map(float,total))
 assert metric_error<2e-14 and ledger['time_levels']==2 and ledger['flow_fields']==5 and len(ledger['new_total'])==12
 defects=[]
 for k,(got,donor,target,new) in enumerate(zip(integrals['accepted'],integrals['donor'],ledger['target_total'],ledger['new_total'])):
  scale=max(1e-300,ledger['scale'][k]);bounded=ledger['turbulence_bound_change'][k//6]*scale if k%6==5 else 0
  errors=[abs(got-new)/scale,abs(donor-ledger['donor_total'][k])/scale,abs(got-target-bounded)/scale,abs(target-ledger['donor_total'][k]-ledger['fill_open'][k]+ledger['lost_open'][k])/scale]
  assert max(errors)<2e-10,(k,errors);defects.append(max(errors))
 assert abs(float(volume)-ledger['target_volume'])<1e-10 and abs(float(oldvolume)-ledger['donor_volume'])<1e-10
 assert abs(ledger['target_volume']-ledger['donor_volume']-ledger['fill_volume']+ledger['lost_volume'])<1e-10
 if stage!='construct':
  previous_state=read(folder.with_name('construct')/'event_states.json')['accepted'];before=read(folder/'before_states.json')['accepted']
  expected={tuple(r['point']):r['states'] for r in previous_state};actual={tuple(r['point']):r['states'] for r in before};assert set(actual)==set(expected)
  assert max(abs(x-y)/max(1.,abs(x)) for p,v in actual.items() for x,y in zip(v,expected[p]))<2e-10
  a=turbulence(folder/'before_turb.csv');b=turbulence(folder.with_name('construct')/'accepted_turb.csv');assert set(a)==set(b)
  nu_scale=max(1e-30,max(abs(x) for v in b.values() for x in v))
  assert max(abs(x-y)/nu_scale for p,v in a.items() for x,y in zip(v,b[p]))<2e-10
 clocks=[read(p) for p in sorted(folder.glob('lifecycle_rank*.json'))];assert len(clocks)==proof['ranks']
 assert all(c['workers']==proof['workers'] and c['CFD_steps']==0 and c['original_sensor_only'] and c['work']<=6144 for c in clocks)
 result={'status':'PASS_ACTUAL_RANS_STATE_TRANSFER_RESTART_AND_OUTPUT','stage':stage,'ranks':proof['ranks'],'workers':proof['workers'],'cells':len(row['cells']),'nodes':len(nodes),'maximum_actual_integral_defect':max(defects),'transported_sensor_error':metric_error,'target_volume':float(volume),'fill_volume':ledger['fill_volume'],'lost_volume':ledger['lost_volume'],'geometry_oracle_sha256':sha(previous/'independent.json'),'geometry_verification':geometry_proof,'clocks':clocks,'seconds':time.monotonic()-start,'auditor_sha256':sha(Path(__file__)),'cgns_reader_sha256':sha(folder/'cgns_reader_executed.py'),'scope':'Exact positive geometry/topology/no positive overlap by mesh identity reuse or declared fresh audit; retained identical curved-boundary/height proof. BL full-field checks retain native scope; released identity field has an independent complete oracle; prescribed sensors, not complexity normalization/budget selection; actual current+BDF flow/SA integrals including open domain and solver-bound changes; both mesh formats/reload; restart for continuation. No CFD advance, independent spherical BL tensor oracle, convergence or scalability claim.'}
 (folder/'independent.json').write_text(json.dumps(result,indent=2)+'\n');print(stage,result['status'],flush=True)
 return result
if __name__=='__main__':
 case=HERE/sys.argv[1];stages=sys.argv[2:] or ['construct','release'];assert all(s in ['construct','update','release'] for s in stages)
 for stage in stages:audit(case/stage,stage)
