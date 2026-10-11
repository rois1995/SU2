"""Independent actual-byte migration/dual-graph/load audit and adapted original-P1/geometry mesh audit."""
from collections import defaultdict
from fractions import Fraction as F
import hashlib,itertools,json,math,struct,sys
from pathlib import Path
from audit_coupled import frame,parity
from audit_cavities import determinant
log,destination=map(lambda x:Path(x).resolve(),sys.argv[1:3]);ranks=int(sys.argv[3])
print('Case working and retained folder:',destination,flush=True);destination.mkdir(exist_ok=False)
rows=defaultdict(lambda:defaultdict(dict));profiles=defaultdict(list);engines=[]
def decode(raw):
 b=bytes.fromhex(raw);assert len(b)==208
 w=struct.unpack('=26Q',b)
 def value(i):return struct.unpack('=d',struct.pack('=Q',w[i]))[0]
 v=[[w[1+4*i]]+[value(2+4*i+j) for j in range(3)] for i in range(4)]
 p=[[w[18+2*i],w[19+2*i]] for i in range(4)];assert all(x[0] in (0,1) for x in p)
 return dict(id=w[0],version=w[17],vertices=v,physical=p,wire=raw)
for line in log.read_text().splitlines():
 if line.startswith('PART_ROW '):
  p=line.split();assert len(p)==6
  row=decode(p[5]);row.update(owner=int(p[3]),weight=int(p[4]));assert row['id'] not in rows[p[1]][p[2]]
  rows[p[1]][p[2]][row['id']]=row
 elif line.startswith('PART_PROFILE '):
  d=dict(p.split('=',1) for p in line.split()[1:]);profiles[d['case']].append(d)
 elif line.startswith('PART_ENGINE '):engines.append(dict(p.split('=',1) for p in line.split()[1:]))
def faces(vertices):
 a,b,c,d=vertices;return [(b,c,d),(a,d,c),(a,b,d),(a,c,b)]
def graph(records):
 uses=defaultdict(list);nodes={}
 for row in records.values():
  v=row['vertices'];assert determinant([tuple(F(x) for x in p[1:]) for p in v])>0
  for p in v:
   if p[0] in nodes:assert nodes[p[0]]==p[1:]
   nodes[p[0]]=p[1:]
  for f,p in zip(faces([p[0] for p in v]),row['physical']):uses[tuple(sorted(f))].append((row['id'],parity(f),p[0]))
 edges=[]
 for f,u in uses.items():
  assert len(u) in (1,2)
  if len(u)==2:
   assert u[0][1]+u[1][1]==0 and not u[0][2] and not u[1][2];edges.append((u[0][0],u[1][0]))
  else:assert u[0][2]
 return nodes,uses,edges
records=[];expected_parts={1,ranks,max(1,ranks-1),max(1,ranks//2)}
assert set(profiles)=={'grid4_l%d_m%d'%(layout,m) for layout in (0,1,2) for m in expected_parts}
for label,data in sorted(rows.items()):
 before,after=data['input'],data['output'];assert before and after
 if label.startswith('grid4_'):
  assert len(before)==len(after)==384 and {i:r['wire'] for i,r in before.items()}=={i:r['wire'] for i,r in after.items()},'Migration changed record bytes'
  nodes,uses,edges=graph(before);assert len(nodes)==125 and len(edges)==672
  # An independent Kuhn tiling (six axis permutations per voxel) certifies the entire initial cube.
  expected=set();n=4
  def node(x,y,z):return (1<<56)+(z*5+y)*5+x
  for x,y,z in itertools.product(range(4),repeat=3):
   for axes in itertools.permutations(range(3)):
    p=[x,y,z];vertices=[node(*p)]
    for axis in axes:p[axis]+=1;vertices.append(node(*p))
    expected.add(tuple(sorted(vertices)))
  assert {tuple(sorted(p[0] for p in r['vertices'])) for r in before.values()}==expected
  for id,p in nodes.items():
   index=id-(1<<56);x=index%5;y=index//5%5;z=index//25
   assert p==[x/4,y/4,z/4]
  for row in before.values():assert row['weight']==(4 if sum(p[1] for p in row['vertices'])/4<.5 else 1)
  m=int(label.split('_m')[1]);loads=[0]*ranks;work=[0]*ranks;moved=0
  for id,row in after.items():
   assert 0<=row['owner']<m;loads[row['owner']]+=1;work[row['owner']]+=before[id]['weight'];moved+=row['owner']!=before[id]['owner']
  cut=sum(after[a]['owner']!=after[b]['owner'] for a,b in edges)
  assert max(loads)<=1.3*384/m and max(work)<=1.3*960/m
  assert len(profiles[label])==ranks and sorted(int(d['rank']) for d in profiles[label])==list(range(ranks))
  for d in profiles[label]:
   rank=int(d['rank']);assert int(d['edgecut'])==cut and int(d['arcs'])==1344 and int(d['moved'])==moved
   assert int(d['cells_after'])==loads[rank] and int(d['work_after'])==work[rank]
   for k,v in d.items():
    if k.endswith('_s'):assert math.isfinite(float(v)) and float(v)>=0
   assert abs(sum(float(d[k]) for k in ['graph_s','partition_s','migration_s','other_s'])-float(d['total_s']))<1e-10
  # Retain only compact ownership/graph facts; geometry is the certified unchanged tiling.
  records.append(dict(case=label,cells=384,points=125,arcs=1344,edgecut=cut,moved=moved,cells_after=loads,work_after=work,
     ownership={str(i):r['owner'] for i,r in sorted(after.items())},input_wire_sha256=hashlib.sha256(''.join(r['wire'] for i,r in sorted(before.items())).encode()).hexdigest()))
 else:
  assert label.startswith('engine_m') and len(before)==6 and 6<len(after)<=128
  nodes,_,_=graph(before);facets={};original=[]
  for row in before.values():
   original.append(dict(vertices=[p+[[4,0,0,4,0,4]] for p in row['vertices']]))
   for f,(present,id) in zip(faces(row['vertices']),row['physical']):
    if present:assert id not in facets;facets[id]=dict(id=id,marker=id+10,vertices=f)
  assert len(facets)==12
  for stage,table in [('initial',before),('adapted',after)]:
   mesh_nodes={};cells=[];physical=[]
   for row in table.values():
    cells.append(dict(id=row['id'],nodes=[p[0] for p in row['vertices']]))
    for p in row['vertices']:
     if p[0] in mesh_nodes:assert mesh_nodes[p[0]]==p
     mesh_nodes[p[0]]=p
    for f,(present,id) in zip(faces(row['vertices']),row['physical']):
     if present:physical.append(dict(facet=id,nodes=[p[0] for p in f]))
   path=destination/(label+'_'+stage+'.json')
   path.write_text(json.dumps(dict(nodes=[mesh_nodes[i] for i in sorted(mesh_nodes)],cells=cells,surface=physical,
          facets=[facets[i] for i in sorted(facets)],original=original),indent=2)+'\n')
   measured,_=frame(path,stage=='adapted');assert measured['volume']=='1';records.append(measured)
assert {d['workers'] for d in engines}=={str(m) for m in expected_parts}
assert all(d['valid']=='1' and d['source_preserved']=='1' for d in engines)
result=dict(status='PASS',records=records,profiles=dict(profiles),engines=engines,source_log_sha256=hashlib.sha256(log.read_bytes()).hexdigest(),
 scope='Independent decoding of actual scalar wire records, exact original Kuhn tiling/positive orientation/manifold triangular graph, byte-identical global migration, true observed cut/moved/load counts and closed per-rank partition phases. Actual weighted-worker/subset re-partition/refinement/return independently passes geometry/marker/embedding/original-P1 q/J/length audits and exports SU2 plus source JSON. No actual CFD state/CGNS or work-estimate calibration/scaling qualification.')
result['artifact_sha256']={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in destination.iterdir() if p.is_file()}
(destination/'independent_audit.json').write_text(json.dumps(result,indent=2)+'\n')
print('PASS',len(records),'independent partition/mesh records',flush=True)
