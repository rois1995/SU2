from pathlib import Path
from fractions import Fraction as F
import hashlib,json,math
from itertools import combinations
HERE=Path(__file__).resolve().parent;MAX=(1<<64)-1
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
def sub(a,b):return tuple(x-y for x,y in zip(a,b))
def dot(a,b):return sum(x*y for x,y in zip(a,b))
def cross(a,b):return (a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0])
def det(a,b,c):return dot(a,cross(b,c))
def separated(a,b):
 edges=lambda p:[sub(p[j],p[i]) for i,j in combinations(range(4),2)]
 axes=[cross(sub(p[j],p[i]),sub(p[k],p[i])) for p in (a,b) for i,j,k in combinations(range(4),3)]
 axes += [cross(u,v) for u in edges(a) for v in edges(b)]
 for axis in axes:
  if not any(axis):continue
  u=[dot(p,axis) for p in a];v=[dot(p,axis) for p in b]
  if max(u)<=min(v) or max(v)<=min(u):return True
 return False
receipt=json.loads((HERE/'validation.json').read_text());assert receipt['status']=='PASS'
inputs={};results={};normalized={}
for ranks in (1,2,4):
 work=HERE/('mpi'+str(ranks));summary={};normalized[str(ranks)]={}
 for shape in ('bump','sphere'):
  p=work/('generated_'+shape+'_source.json');inputs[str(p.relative_to(HERE))]=sha(p);rows=json.loads(p.read_text())
  geometry_path=work/('generated_'+shape+'.json');inputs[str(geometry_path.relative_to(HERE))]=sha(geometry_path)
  geometry=json.loads(geometry_path.read_text());facets={f['id']:f for f in geometry['facets']}
  components={min(group):group for group in geometry['facet_groups']}
  face_key=lambda f:tuple(sorted(tuple(n[1:4]) for n in f['vertices']))
  component_key=lambda id:(geometry['marker_names'][facets[id]['marker']],tuple(sorted(face_key(facets[f]) for f in components[id])))
  ids={r['id'] for r in rows};assert len(ids)==len(rows)
  old={r['id']:r for r in rows if r['component']==MAX};ext=[r for r in rows if r['component']!=MAX]
  assert (len(old),len(ext))==((6,6) if shape=='bump' else (1,12))
  original_nodes={}
  for row in old.values():
   assert row['parent']==MAX
   for node in row['vertices']:
    if node[0] in original_nodes:assert original_nodes[node[0]]==node
    original_nodes[node[0]]=node
  span=max(original_nodes)+1;all_nodes={};cells=[]
  for row in rows:
   points=[tuple(F(x) for x in n[1:4]) for n in row['vertices']];cells.append(points)
   assert det(sub(points[1],points[0]),sub(points[2],points[0]),sub(points[3],points[0]))>0
   for node in row['vertices']:
    if node[0] in all_nodes:assert all_nodes[node[0]]==node
    all_nodes[node[0]]=node
   if row['component']==MAX:continue
   assert row['parent'] in old
   parent={n[0]:n for n in old[row['parent']]['vertices']}
   assert row['component'] in components
   ancestors={n[0] if n[0]<span else n[0]-span for n in row['vertices']}
   assert len(ancestors)==3 and ancestors.issubset(parent)
   footprint=tuple(sorted(tuple(parent[a][1:4]) for a in ancestors))
   assert footprint in component_key(row['component'])[1]
   for node in row['vertices']:
    ancestor=node[0] if node[0]<span else node[0]-span
    assert ancestor in parent and node[4]==parent[ancestor][4]
    if node[0]<span:assert node==parent[ancestor]
    elif shape=='bump':assert node[1:3]==parent[ancestor][1:3] and -.06250001<node[3]<-.0625
    else:
     origin=parent[ancestor][1:4];ratio=next(node[i+1]/origin[i] for i in range(3) if origin[i])
     assert ratio>3 and ratio<3.01
     assert max(abs(node[i+1]-ratio*origin[i]) for i in range(3))<1e-14
  pairs=0
  for a,b in combinations(cells,2):assert separated(a,b);pairs+=1
  # Actual source mesh for inspection; it is metric coverage, not an adapted CFD mesh.
  nodeids=sorted(all_nodes);index={n:i for i,n in enumerate(nodeids)};lines=['NDIME= 3','NELEM= '+str(len(rows))]
  lines+=['10 '+' '.join(str(index[n[0]]) for n in row['vertices'])+' '+str(i) for i,row in enumerate(rows)]
  lines+=['NPOIN= '+str(len(nodeids))]+[' '.join(format(x,'.17g') for x in all_nodes[n][1:4])+' '+str(index[n]) for n in nodeids]
  faces={}
  for row in rows:
   c=[n[0] for n in row['vertices']]
   for f in ((c[1],c[2],c[3]),(c[0],c[3],c[2]),(c[0],c[1],c[3]),(c[0],c[2],c[1])):faces.setdefault(tuple(sorted(f)),[]).append(f)
  assert all(len(v)<=2 for v in faces.values());skin=[v[0] for v in faces.values() if len(v)==1]
  lines+=['NMARK= 1','MARKER_TAG= coverage_envelope','MARKER_ELEMS= '+str(len(skin))]+['5 '+' '.join(str(index[n]) for n in f) for f in skin]
  (work/('generated_'+shape+'_source.su2')).write_text('\n'.join(lines)+'\n')
  # IDs may change with partitioning. Within-run identity is checked above; across runs compare physical provenance.
  cell_key=lambda row:tuple(sorted(tuple(n[1:4])+tuple(n[4]) for n in row['vertices']))
  normalized[str(ranks)][shape]=sorted((cell_key(row),row['component']!=MAX,
    component_key(row['component']) if row['component']!=MAX else (),
    cell_key(old[row['parent']]) if row['parent']!=MAX else ()) for row in rows)
  summary[shape]={'original_cells':len(old),'extension_cells':len(ext),'exact_no_overlap_pairs':pairs,'parent_sensor_copies_exact':True,'shared_source_id_coordinates_tensors_exact':True,'finite_declared_transform_checked':True}
 results[str(ranks)]=summary
assert normalized['1']==normalized['2']==normalized['4']
out={'status':'PASS','scope':'Actual CFD-generated source records: original/extension count, parent provenance, exact copied sensor values and shared-node coordinates/tensors, positive exact volumes, exact rational no-interior-overlap, finite transform and MPI1/2/4 geometry, marker/component footprint, parent provenance and value consistency; numeric transient IDs are checked within each run, not assumed invariant between partitionings. Coverage meshes are source envelopes, not accepted CFD replacements; mesh publication/solution/history/CFD/restart remain unqualified.','controller_receipt_sha256':sha(HERE/'validation.json'),'script_sha256':sha(Path(__file__)),'inputs_sha256':inputs,'results':results}
(HERE/'generated_source_validation.json').write_text(json.dumps(out,indent=2)+'\n');print('PASS: actual generated source geometry/tensor/parent/MPI consistency')
