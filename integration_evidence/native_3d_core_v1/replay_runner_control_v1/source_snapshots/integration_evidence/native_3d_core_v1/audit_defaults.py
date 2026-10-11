from pathlib import Path
from collections import Counter,defaultdict
from fractions import Fraction as F
import json,shlex,copy,sys
root=Path(__file__).resolve().parents[2];where=Path(__file__).resolve().parent
sys.path.insert(0,str(where))
from audit_geometry_bridge import su2_signature,signature
from audit_cavities import determinant
from audit_coupled import frame
folder=Path(sys.argv[1]).resolve();baseline_folder=Path(sys.argv[2]).resolve()
records=[]
for n in (1,2,4):
 work=folder/('mpi'+str(n));cells,boundary=su2_signature(work/'default_native3d.su2')
 assert all(count==1 for count in cells.values()) and all(count==1 for count in boundary.values())
 coordinates=sorted({p for cell in cells for p in cell});index={p:i for i,p in enumerate(coordinates)}
 data=dict(nodes=[[i,*p] for i,p in enumerate(coordinates)],cells=[],surface=[],facets=[],facet_groups=[])
 incidence=defaultdict(list)
 for i,t in enumerate(cells):
  v=[index[p] for p in t]
  if determinant([tuple(F(x) for x in coordinates[j]) for j in v])<0:v[0],v[1]=v[1],v[0]
  data['cells'].append(dict(id=i,nodes=v))
  faces=((v[1],v[2],v[3]),(v[0],v[3],v[2]),(v[0],v[1],v[3]),(v[0],v[2],v[1]))
  for face in faces:incidence[tuple(sorted(coordinates[k] for k in face))].append(face)
 lines=(work/'default_native3d.su2.native_ref').read_text().splitlines();assert lines[0]=='SU2_NATIVE_REFERENCE_3D 1'
 nm,nf=map(int,lines[1].split());assert len(lines)==2+nm+nf
 names=[shlex.split(line)[0] for line in lines[2:2+nm]];assert len(set(names))==nm
 data['marker_names']=names;groups=defaultdict(list)
 for line in lines[2+nm:]:
  parts=line.split();assert len(parts)==14
  id,marker=map(int,parts[:2]);assert 0<=marker<nm
  vertices=[[int(parts[i]),*map(float,parts[i+1:i+4])] for i in (2,6,10)]
  data['facets'].append(dict(id=id,marker=marker,vertices=vertices))
  plane=next((d,vertices[0][d+1]) for d in range(3) if len({p[d+1] for p in vertices})==1)
  groups[(marker,*plane)].append(id)
 data['facet_groups']=list(groups.values())
 for (marker,tri),count in boundary.items():
  assert len(incidence[tri])==1
  group=next(g for key,g in groups.items() if names[key[0]]==marker and all(p[key[1]]==key[2] for p in tri))
  data['surface'].append(dict(facet=min(group),nodes=list(incidence[tri][0])))
 # The new C++ fixture is the same pinned six-tetrahedron unit cube, now with constant sensor 4I.
 baseline=json.loads((baseline_folder/('mpi'+str(n))/'runtime_barycentric_m1_d0_s0.json').read_text())
 data['original']=copy.deepcopy(baseline['original'])
 for cell in data['original']:
  for v in cell['vertices']:v[4]=[4.,0.,0.,4.,0.,4.]
 path=work/'default_independent_mesh.json';path.write_text(json.dumps(data,indent=2)+'\n')
 record,audited=frame(path,True);assert signature(audited)==(cells,boundary);records.append(record)
result=dict(status='PASS',frames=records,scope='Independent actual default/no-native-repartition SU2 mesh positive volumes, manifold/embedding/marker/strict shape/sliver/edge checks. Geometry and marker triangles come from actual raw output/sidecar; original domain and constant 4I source use the pinned C++ unit-cube fixture. State preservation and transported sensor are separately checked in the C++ tests.')
(folder/'independent_audit.json').write_text(json.dumps(result,indent=2)+'\n')
print('PASS',len(records),'default actual output mesh audits')
