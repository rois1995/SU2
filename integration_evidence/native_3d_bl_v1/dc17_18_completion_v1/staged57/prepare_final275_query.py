"""Extract the actual first final-certification entity, without changing its supports."""
from pathlib import Path
import hashlib,json,shutil,time
R=Path(__file__).resolve().parent;A=R/'m6_runs/euler_build274_metric_cleanup_mach_only_real_cfd50k';D=A/'inspection';O=A/'final_certificate_query_v1'
sha=lambda p:hashlib.file_digest(Path(p).open('rb'),'sha256').hexdigest()
assert not O.exists();assert json.loads((A/'inspection_reader_terminal_handles.json').read_text())['all_owned_handles_terminal']
inputs=[D/'rejected_mesh.su2',D/'reference_records.jsonl',A/'m6.su2.native_ref',A/'capture_ready.json',Path(__file__)]
before={str(p):{'bytes':p.stat().st_size,'sha256':sha(p)} for p in inputs};groups={}
for line in (D/'reference_records.jsonl').open():
 row=json.loads(line)
 if row['kind']=='facet':groups.setdefault((row['body'],row['sheet'],row['marker']),[]).append(row['id'])
entity=min(groups);assert entity==(0,0,0);assert all(k==entity for k in groups if k[2]==0)
points=[];physical=[]
with (D/'rejected_mesh.su2').open() as src:
 def line():
  while True:
   s=next(src).strip()
   if s and not s.startswith('%'):return s
 assert line()=='NDIME= 3';s=line();assert s.startswith('NELEM=');cells=int(s.split('=')[1]);assert cells==734806
 for _ in range(cells):next(src)
 s=line();assert s.startswith('NPOIN=');count=int(s.split('=')[1]);assert count==148781
 for i in range(count):
  row=line().split();assert int(row[3])==i;points.append(tuple(map(float,row[:3])))
 s=line();assert s.startswith('NMARK=');markers=int(s.split('=')[1])
 for _ in range(markers):
  tag=line().split('=')[1].strip();faces=int(line().split('=')[1])
  for _ in range(faces):
   row=list(map(int,line().split()));assert row[0]==5
   if tag=='Marker_0':physical.append(tuple(points[i] for i in row[1:]))
assert physical;O.mkdir();support=sorted(groups[entity]);out=O/'entity0.txt'
with out.open('x') as dst:
 dst.write(f'{len(support)} {len(physical)} 0.0001 1000000\n')
 for facet in support:dst.write(f'{facet}\n')
 for face in physical:dst.write(' '.join(format(x,'.17g') for point in face for x in point)+'\n')
shutil.copy2(A/'m6.su2.native_ref',O/'m6.su2.native_ref')
after={str(p):{'bytes':p.stat().st_size,'sha256':sha(p)} for p in inputs};assert before==after
outputs={p.name:{'bytes':p.stat().st_size,'sha256':sha(p)} for p in [out,O/'m6.su2.native_ref']}
result={'schema':'native_current_geometry_query_preparation_v1','entity':entity,'support_facets':len(support),'physical_faces':len(physical),'input_hashes_before':before,'input_hashes_after':after,'outputs':outputs,'scope':'Exact first entity in complete final GeometryCertificates map order; source-to-current-mesh query only diagnoses traversal. No surface simplification, support cropping or acceptance claim.'}
(O/'geometry_query_preparation.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps({k:result[k] for k in ('entity','support_facets','physical_faces')}))
