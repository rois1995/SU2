#!/usr/bin/env python3
"""Build full unit-sensor probes with the exact original native wing orientation."""
from pathlib import Path
import hashlib,json
Q=Path(__file__).resolve().parent
reference=Q.parent/'oneram6_frozen_v1/Coarse.su2.native_ref'
assert hashlib.sha256(reference.read_bytes()).hexdigest()=='1594d8108ad4d9536b6fca11f6538fc0503ec8a547b5939758e21e5ef5506235'
points={};coordinates=[];faces=[]
with reference.open() as f:
 assert f.readline().strip()=='SU2_NATIVE_REFERENCE_3D 1'
 markers,count=map(int,f.readline().split());names=[json.loads(f.readline()) for _ in range(markers)]
 for _ in range(count):
  row=f.readline().split()
  if names[int(row[1])]!='WING3D':continue
  face=[]
  for k in range(3):
   node=int(row[2+4*k]);p=tuple(map(float,row[3+4*k:6+4*k]))
   if node not in points:points[node]=len(coordinates);coordinates.append(p)
   assert coordinates[points[node]]==p
   face.append(points[node])
  faces.append(face)
 assert f.read()==''
assert len(coordinates)==19506 and len(faces)==38800
body=['NATIVE_BL_REPLAY_3D 2','19506 38800','2e-6 1.2 .01']
body += [' '.join(format(x,'.17g') for x in p) for p in coordinates]
body += [' '.join(map(str,p)) for p in faces]
for gid in (22,32):
 folder=Q/('cell'+str(gid)+'_probe');source=json.loads((folder/'fixture.json').read_text())
 for name in ['probe.log','exit_code.txt','replay_rule_150.csv','replay_rule_125.csv','replay_samples.csv']:
  p=folder/name
  if p.exists():p.rename(folder/('normal_'+name))
 fixture=folder/'native_wall_unit.dat'
 fixture.write_text('\n'.join(body+[' '.join(format(x,'.17g') for x in p) for p in source['coordinates']]+['1']+['1 0 0 0 1 0 0 0 1']*4)+'\n')
 r={'scope':'Version2 full sensor-plus-geometric-BL density; FOUR IDENTITY SENSORS/core1, not captured runtime sensors. Complete original wing orientation/order follows native FromNativeReference exactly. Corner angle only affects2D, so explicit3D constructor default is equivalent here.','source_cell':gid,'source_point_ids_sorted':source['source_point_ids_sorted'],'sha256':{'native_reference':hashlib.sha256(reference.read_bytes()).hexdigest(),'fixture':hashlib.sha256(fixture.read_bytes()).hexdigest(),'helper':hashlib.sha256(Path(__file__).read_bytes()).hexdigest()}}
 (folder/'full_unit_fixture.json').write_text(json.dumps(r,indent=2)+'\n')
print('Prepared actual volume-oriented wall probes for cells22/32; full identity sensor only.')
