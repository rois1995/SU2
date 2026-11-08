"""Bounded terminal247 surface/volume delivery diagnosis using captured production lengths."""
from pathlib import Path
import hashlib,json,sys
import numpy as np
HERE=Path(__file__).resolve().parent;R=HERE.parent;REPO=R.parents[3];A=R/'m6_runs/euler_build247_canonical_mach_only_real_cfd50k'
sys.path.insert(0,str(REPO/'TestCases/adaptation/capability'));import capcheck
sys.path.insert(0,str(R));import inspect_native_failure3d as failure_reader
orig=capcheck.read_su2(R/'metric_target244_mach_only/L1/m6.su2');P,F,_=capcheck.read_restart(R/'metric_target244_mach_only/L1/restart_flow.dat')
wing=P[np.unique(orig.M['WING3D'])];x0=(wing[:,0]-wing[:,1]/np.sqrt(3)).min()
def stats(a):
 assert len(a) and np.isfinite(a).all()
 return dict(zip(('min','p10','median','p90','max'),map(float,np.quantile(a,[0,.1,.5,.9,1]))))
# Exact record layout from existing validated inspector (416 bytes, little endian).
dt=np.dtype({'names':['format','id','nodes','version','physical','sensor_present','sensor'],'formats':['<u8','<u8',np.dtype([('id','<u8'),('xyz','<f8',(3,))]),'<u8','<u8','<u8','<f8'],'offsets':[0,8,16,144,152,216,224],'itemsize':416})
# Subarray fields need explicit shape to retain four vertices/faces and 24 tensor entries.
dt=np.dtype([('format','<u8'),('id','<u8'),('nodes',np.dtype([('id','<u8'),('xyz','<f8',(3,))]),(4,)),('version','<u8'),('physical','<u8',(4,2)),('sensor_present','<u8'),('sensor','<f8',(4,6))]);assert dt.itemsize==416
seal=json.loads((A/'capture_ready.json').read_text())['files'];hashes={};facets={}
manifest_rows,missing=failure_reader._load_manifests([str(A/'adapted_mesh_inspection_rank*_manifest.json')]);assert not missing
by_rank={m['rank']:m for m in manifest_rows}
p=A/'inspection/reference_records.jsonl'
with p.open() as f:
 for line in f:
  row=json.loads(line)
  if row.get('kind')=='facet':facets[row['id']]={k:row.get(k) for k in ['marker','body','sheet','mapping_error']}
# Original marker order is FARFIELD,SYMMETRY,WING3D (verified actual SU2).
assert [s.split('=')[1].strip() for s in (R/'metric_target244_mach_only/L1/m6.su2').read_text().splitlines() if s.startswith('MARKER_TAG')]==['FARFIELD','SYMMETRY','WING3D']
standard=[(1,2,3),(0,3,2),(0,1,3),(0,2,1)];faces=[];cells={};production_edges={};totals={'q_bad':0,'j_bad':0,'geometry_bad':0};geometry=[];mobility={};reasons={}
for rank in range(6):
 path=A/f'adapted_mesh_inspection_rank{rank}_owned.bin';h=hashlib.sha256(path.read_bytes()).hexdigest();assert h==seal[str(path)]['sha256'];hashes[str(path)]=h
 count=int(np.fromfile(path,dtype='<u8',count=1)[0]);assert path.stat().st_size==8+416*count
 if count:
  data=np.memmap(path,dtype=dt,offset=8,shape=(count,),mode='r');assert (data['format']==data['format'][0]).all()
  mask=data['physical'][:,:,0].any(axis=1)
  for cell in data[mask]:
   local=[]
   for slot,physical in enumerate(cell['physical']):
    if not physical[0]:continue
    ref=facets[int(physical[1])]
    if ref['marker']!=2:continue
    ids=np.array(standard[slot]);xyz=cell['nodes']['xyz'][ids].copy();nodeids=cell['nodes']['id'][ids].copy();c=xyz.mean(axis=0)
    local.append(len(faces));faces.append({'id':int(cell['id']),'facet':int(physical[1]),'xyz':xyz,'nodes':nodeids,'center':c,'reference':ref})
   if local:cells[int(cell['id'])]=local
 # Stream existing exact census rows; no geometry recomputation or repair decision.
 path=by_rank[rank]['_resolved_files']['metrics'];dig=hashlib.file_digest(path.open('rb'),'sha256').hexdigest()
 assert dig==seal[str(path)]['sha256']
 for row in failure_reader._jsonl(path,by_rank[rank]['_caps']['metrics'],'metrics'):
  assert row['cache_valid'] and row['q_measured'] and row['j_measured'] and row['geometry_measured']
  for k in totals:totals[k]+=int(row[k])
  cid=row['id']
  if cid in cells:
   for idx in cells[cid]:faces[idx]['diagnostic']={k:row[k] for k in ['q','J','q_bad','j_bad','geometry_bad','geometry_excess_l2','near_feature','feature_known']}
   if row['geometry_bad']:
    geometry.append({'cell':cid,'facets':[faces[j]['facet'] for j in cells[cid]],'centers':[faces[j]['center'].tolist() for j in cells[cid]],'excess':row['geometry_excess_l2'],'near_feature':row['near_feature'],'bindings':row['bindings']})
   for b in row['bindings']:
    if b is not None:mobility[b['id']]=b
   for aa,bb,length in row['edges']:
    key=tuple(sorted((aa,bb)))
    if key in production_edges:assert abs(production_edges[key]-length)<=1e-11*max(1,length)
    production_edges[key]=length
 assert hashlib.file_digest(path.open('rb'),'sha256').hexdigest()==dig;hashes[str(path)]=dig
assert totals=={'q_bad':1148,'j_bad':484,'geometry_bad':1044},totals
C=np.array([f['center'] for f in faces]);xyz=np.array([f['xyz'] for f in faces]);normal=np.cross(xyz[:,1]-xyz[:,0],xyz[:,2]-xyz[:,0]);normal/=np.linalg.norm(normal,axis=1)[:,None]
span=np.tile([1/np.sqrt(3),1,0.],(len(faces),1));span-=np.einsum('ni,ni->n',span,normal)[:,None]*normal;span/=np.linalg.norm(span,axis=1)[:,None];nose=np.cross(normal,span)
report={'scope':'Actual current physical wing faces joined to reference facet/body/sheet and complete247 cached production metric edge lengths; NOT recomputed current-point target, no acceptance/causal claim. Directional geometric bins do not isolate individual bad-face certificate contributions.','census_totals':totals,'wing_faces':len(faces),'stations':[],'geometry_bad_wing_cells':len(geometry)}
M=np.zeros((len(P),3,3))
for a,b,k in [(0,0,'XX'),(0,1,'XY'),(0,2,'XZ'),(1,1,'YY'),(1,2,'YZ'),(2,2,'ZZ')]:M[:,a,b]=M[:,b,a]=F['Metric_'+k]
oldT=orig.M['WING3D'];oldC=P[oldT].mean(axis=1)
for y in [.2,.4,.6,.8,1.0]:
 band=wing[abs(wing[:,1]-y)<.025];xi=band[:,0]-band[:,1]/np.sqrt(3);chord=np.ptp(xi);xle=xi.min()
 mask=(abs(C[:,1]-y)<.025)&((C[:,0]-C[:,1]/np.sqrt(3)-xle)/chord<.03);idx=np.flatnonzero(mask);assert len(idx)>0
 oldmask=(abs(oldC[:,1]-y)<.025)&((oldC[:,0]-oldC[:,1]/np.sqrt(3)-xle)/chord<.03)
 extspan=np.ptp(np.einsum('nvi,ni->nv',xyz[idx],span[idx]),axis=1);extnose=np.ptp(np.einsum('nvi,ni->nv',xyz[idx],nose[idx]),axis=1)
 edges=[];sdir=[];bnd=[];feature=[]
 for j in idx:
  face=faces[j];bnd.extend(mobility[int(k)]['mobility_dimension'] for k in face['nodes'] if int(k) in mobility);feature.append(face['diagnostic']['near_feature'])
  for a,b in [(0,1),(1,2),(2,0)]:
   key=tuple(sorted(map(int,face['nodes'][[a,b]])));assert key in production_edges;edges.append(production_edges[key]);delta=face['xyz'][b]-face['xyz'][a];sdir.append(abs(delta@span[j])>abs(delta@nose[j]))
 arr=np.array(edges);sdir=np.array(sdir)
 report['stations'].append({'y':y,'current_triangles':len(idx),'original_triangles':int(oldmask.sum()),'triangle_ratio':len(idx)/int(oldmask.sum()),'geometric_span_over_nose_extent':stats(extspan/extnose),'production_metric_lengths_all_face_edges':stats(arr),'production_metric_lengths_span_dominant':stats(arr[sdir]),'production_metric_lengths_nose_dominant':stats(arr[~sdir]),'fraction_edges_below_09':float((arr<.9).mean()),'fraction_edges_below_05':float((arr<.5).mean()),'fraction_faces_near_genuine_feature':float(np.mean(feature)),'mobility_dimension_face_vertex_occurrences':{str(k):bnd.count(k) for k in set(bnd)},'geometry_bad_faces':sum(faces[j]['diagnostic']['geometry_bad'] for j in idx),'reference_body_sheet_counts':{str(key):sum((faces[j]['reference']['body'],faces[j]['reference']['sheet'])==key for j in idx) for key in set((faces[j]['reference']['body'],faces[j]['reference']['sheet']) for j in idx)}})
# Region classification uses ACTUAL wing-associated faces, not volume centroid proxy.
near_le=[]
for row in geometry:
 cs=np.array(row['centers']);near=any(.15<c[1]<1.15 and c[0]-c[1]/np.sqrt(3)-x0<.03*.8 for c in cs)
 if near:near_le.append(row)
report['geometry_bad_actual_wing_face_near_leading_edge_approx_band']=len(near_le)
report['geometry_bad_classification_note']='Actual physical-wing reference associations; geographic LE band is swept x-xLE<0.024,y0.15..1.15. Not identical to side investigator unspecified midspan band; not a face-level causal attribution.'
report['geometry_bad_le_near_feature']=sum(r['near_feature'] for r in near_le);report['geometry_bad_le_cells_with_multiple_wing_faces']=sum(len(r['facets'])>1 for r in near_le)
report['representative_le_bad_cells']=sorted(near_le,key=lambda r:r['excess'],reverse=True)[:8]
report['sha256']=hashes;report['sha256'][str(Path(__file__))]=hashlib.sha256(Path(__file__).read_bytes()).hexdigest()
report['sha256'][str(R/'inspect_native_failure3d.py')]=hashlib.sha256((R/'inspect_native_failure3d.py').read_bytes()).hexdigest()
(HERE/'leading_edge_diagnosis.json').write_text(json.dumps(report,indent=2,allow_nan=False)+'\n');print(json.dumps({k:report[k] for k in ['wing_faces','census_totals','geometry_bad_wing_cells','geometry_bad_actual_wing_face_near_leading_edge_approx_band','geometry_bad_le_near_feature']}))
