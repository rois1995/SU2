"""Reproduce the third final-certification entity from the complete actual raw snapshot."""
from pathlib import Path
import hashlib,json,shutil,struct,time,os
import inspect_native_failure3d as reader
R=Path(__file__).resolve().parent;A=R/'m6_runs/euler_build283_pinched_graph_witness_mach_only_real_cfd50k';O=A/'final_certificate_entity2_v1'
sha=lambda p:hashlib.file_digest(Path(p).open('rb'),'sha256').hexdigest()
started=time.monotonic();assert not O.exists();assert json.loads((A/'terminal_handles.json').read_text())['all_owned_handles_terminal']
ready=json.loads((A/'capture_ready.json').read_text());assert ready['raw_capture_complete'] and ready['owned_rows']==736019 and ready['missing_owned_rows']==0
assert json.loads((A/'owned_stop_once.json').read_text())['signal_sent']
for p,item in ready['files'].items():assert sha(p)==item['sha256']
inputs={p:dict(item) for p,item in ready['files'].items()}
for p in [A/'m6.su2.native_ref',A/'capture_ready.json',A/'terminal_handles.json',Path(__file__).resolve(),Path(reader.__file__).resolve()]:inputs[str(p)]={'bytes':p.stat().st_size,'sha256':sha(p)}
assert len({v['sha256'] for v in ready['files'].values() if v['role']=='reference'})==1
manifests,missing=reader._load_manifests([str(A)]);assert not missing and len(manifests)==6
reference={};groups={}
for row in reader._jsonl(manifests[0]['_resolved_files']['reference'],manifests[0]['_caps']['reference'],'reference'):
 key,clean=reader._validate_reference(row)
 if key[0]=='facet':
  entity=tuple(clean[k] for k in ('body','sheet','marker'));reference[clean['id']]=entity;groups.setdefault(entity,[]).append(clean['id'])
entities=sorted(groups);entity=entities[2];assert entity==(0,10154,2) and len(groups[entity])==37202
faces={};nodes={};all_faces=set();by_entity={k:0 for k in entities};scanned=0
opposite=((1,2,3),(0,3,2),(0,1,3),(0,2,1))
for m in manifests:
 assert m['complete_shard'] and m['metrics_complete']
 for cell in reader._read_owned(m['_resolved_files']['owned'],m['written_rows'],m['_caps']['owned']):
  scanned+=1
  for slot,binding in enumerate(cell['physical']):
   if not binding['present']:continue
   selected=[cell['nodes'][i] for i in opposite[slot]];ids=[n['id'] for n in selected];key=tuple(sorted(ids));assert key not in all_faces;all_faces.add(key)
   identity=reference[binding['facet']];by_entity[identity]+=1
   if identity!=entity:continue
   first=ids.index(min(ids));selected=selected[first:]+selected[:first]
   for node in selected:
    p=tuple(node['xyz']);assert nodes.setdefault(node['id'],p)==p
    assert all(struct.pack('<d',x)==struct.pack('<d',float(format(x,'.17g'))) for x in p)
   faces[key]=[tuple(n['xyz']) for n in selected]
assert scanned==736019 and 0<len(faces)<=100000
exit_event=next(e for e in json.loads((A/'construction_progress_assessment.json').read_text())['events'] if e['stage']=='exit')
assert all(int(row['working_faces'])==len(all_faces) for row in exit_event['rank_rows'].values())
O.mkdir();query=O/'entity2.txt';support=sorted(groups[entity])
with query.open('x') as out:
 out.write(f'{len(support)} {len(faces)} 0.0001 20000000\n')
 for facet in support:out.write(f'{facet}\n')
 for key in sorted(faces):out.write(' '.join(format(x,'.17g') for point in faces[key] for x in point)+'\n')
shutil.copy2(A/'m6.su2.native_ref',O/'m6.su2.native_ref')
assert all(Path(p).stat().st_size==v['bytes'] and sha(p)==v['sha256'] for p,v in inputs.items())
outputs={p.name:{'bytes':p.stat().st_size,'sha256':sha(p)} for p in [query,O/'m6.su2.native_ref']}
result={'schema':'native_current_geometry_query_preparation_v1','entity':entity,'ordered_entities':[{'entity':k,'reference_facets':len(groups[k]),'physical_faces':by_entity[k]} for k in entities],'support_facets':len(support),'physical_faces':len(faces),'all_physical_faces':len(all_faces),'scanned_cells':scanned,'input_hashes_before':inputs,'input_hashes_after':inputs,'outputs':outputs,'seconds':time.monotonic()-started,'scope':'Complete actual third entity in ordered final query. Physical faces sorted by FaceKey, vertices rotated by FaceOrder, coordinates bit-exact; no cropping or extra mesh export. Three full calls imply failure during this entity, not proof that the preceding entities were within tolerance. Direction must be diagnosed.'}
(O/'geometry_query_preparation.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps({k:result[k] for k in ('entity','support_facets','physical_faces','all_physical_faces','seconds')}))
