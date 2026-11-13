"""Audit saved exact-query coverage and face identity before another native experiment."""
import collections,gzip,json,struct
from pathlib import Path
from assess_surface_transaction470 import sha
from inspect_native_failure3d import _read_owned
R=Path(__file__).resolve().parent
s=json.loads((R/'saved_surface485_checkpoint.json').read_text());source=Path(s['owned']['path']);assert sha(source)==s['owned']['sha256']
reference=Path(s['prepared_reference']['path']);assert sha(reference)==s['prepared_reference']['sha256']
with reference.open() as f:
 assert f.readline().strip()=='SU2_NATIVE_REFERENCE_3D 9'
 h=f.readline().split();names=[json.loads(f.readline()) for _ in range(int(h[0]))]
 markers={}
 for _ in range(int(h[1])):
  x=f.readline().split();assert len(x)==14;markers[int(x[0])]=int(x[1])
 for _ in range(int(h[2])):f.readline()
 affine={}
 for _ in range(int(h[3])):
  x=f.readline().split();assert len(x)==11;affine[int(x[0])]=[float(v) for v in x[4:10]]
queries=json.loads(gzip.decompress((R/'geometry_refusals477_queries.json.gz').read_bytes()))
def key(x):return tuple(sorted(tuple(x[k:k+3]) for k in range(0,9,3)))
def bits(p):return [struct.pack('>d',v).hex() for v in p]
known=collections.defaultdict(list)
for q in queries:
 if len(q['triangles_bits'])==1:known[key(q['triangles_bits'][0])].append(q)
pairs=[('near_refused_tip',7594,7592),('corrected_root',17814,64065077),('clean_control',6070,6072)]
stars={a:[] for label,a,b in pairs};faces=((1,2,3),(0,3,2),(0,1,3),(0,2,1))
for c in _read_owned(source,s['current_cells'],200_000_000):
 ids={n['id'] for n in c['nodes']}
 for label,a,b in pairs:
  if a in ids:stars[a].append(c)
report=[]
for label,a,b in pairs:
 nodes={n['id']:n['xyz'] for c in stars[a] for n in c['nodes']};assert b in nodes
 old=[]
 for c in stars[a]:
  for slot,f in enumerate(c['physical']):
   if f['present']:old.append((tuple(c['nodes'][i]['id'] for i in faces[slot]),f['facet']))
 fresh=[]
 for ids,facet in old:
  out=tuple(b if i==a else i for i in ids)
  if len(set(out))<3:continue
  at=out.index(min(out));fresh.append((out[at:]+out[:at],facet))
 unchanged=[(ids,facet) for ids,facet in old if a not in ids]
 changed=[(ids,facet) for ids,facet in old if a in ids]
 out=dict(context=label,removed=a,retained=b,removed_star_cells=len(stars[a]),physical_faces_old=len(old),physical_faces_fresh=len(fresh),
  changed_original_faces_by_marker=dict(collections.Counter(names[markers[f]] for ids,f in changed)),
  changed_original_faces_with_affine=sum(f in affine for ids,f in changed),
  unchanged_physical_faces=len(unchanged),unchanged_faces_rotated_by_finish=sum(ids[0]!=min(ids) for ids,f in unchanged),sides=[])
 for side,rows in [('old',old),('fresh',fresh)]:
  matched=[]
  for ids,facet in rows:
   k=key([v for i in ids for v in bits(nodes[i])]);hit=known.get(k,[])
   if hit:matched.append(dict(vertices=ids,facet=facet,records=len(hit),supports=[q['support'] for q in hit]))
  out['sides'].append(dict(side=side,physical_faces=len(rows),exact_triangle_matches=len(matched),matches=matched))
 report.append(out)
result=dict(input_owned=s['owned'],reference=s['prepared_reference'],old_query_file_sha256=sha(R/'geometry_refusals477_queries.json.gz'),
 saved_query_records=len(queries),affine_facets_by_marker=dict(collections.Counter(names[markers[f]] for f in affine)),contexts=report,
 classification='Evidence coverage/source-fidelity audit only; no new native measurements/candidates/acceptance.',
 limitations=['Fresh physicalskin follows the already-tested directed contraction and native cyclic Finish policy, not an independent reconstruction.',
  'Triangle matches alone would not establish exact support/association/purpose/binary agreement or certificate reuse.',
  'No geometry-order sensitivity or genuine physical-distance regression proved by static face identity.'],script_sha256=sha(__file__))
(R/'geometry_evidence489_audit.json').write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps(result,indent=2))
