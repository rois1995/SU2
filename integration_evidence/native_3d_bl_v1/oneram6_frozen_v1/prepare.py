#!/usr/bin/env python3
"""Prepare a saved-state native adaptation replay and authority from the supplied original mesh."""
from pathlib import Path
import hashlib,json,re,sys
import numpy as np
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2]
sys.path.insert(0,str(ROOT/'TestCases/adaptation/capability'))
from capcheck import read_su2,read_restart
SOURCE=HERE.parent/'oneram6_coarse_v1';mesh=read_su2(SOURCE/'Coarse.su2')
points,fields,_=read_restart(SOURCE/'flow_checkpoint/restart_flow.dat')
assert np.array_equal(points,mesh.P)
assert hashlib.sha256((SOURCE/'Coarse.su2').read_bytes()).hexdigest()=='8c839170eb5e359f7de6a41434aa6a9c2d620053f37de6d9360bd04d2a63088b'
assert np.linalg.det(mesh.P[mesh.E[:,1:]]-mesh.P[mesh.E[:,:1]]).min()>0
names=['FARFIELD','SYMMETRY','WING3D'] # CConfig marker type order for this declared configuration.
labels={tuple(sorted(face)):(marker,i) for marker,name in enumerate(names) for i,face in enumerate(mesh.M[name])}
assert len(labels)==sum(len(mesh.M[name]) for name in names)
found={};uses={}
for slots in ((1,2,3),(0,3,2),(0,1,3),(0,2,1)):
 faces=mesh.E[:,slots]
 for i,face in enumerate(faces):
  key=tuple(sorted(face));label=labels.get(key)
  if label is not None:
   assert key not in found
   found[key]=(label[0],tuple(int(x) for x in face))
assert len(found)==len(labels)
rows=sorted(found.values())
# The complete volume-oriented physical skin must be an oriented closed manifold.
for _,face in rows:
 for i in range(3):
  a,b=face[i],face[(i+1)%3];key=(min(a,b),max(a,b));count,sign=uses.get(key,(0,0));uses[key]=(count+1,sign+(1 if a<b else -1))
assert all(v==(2,0) for v in uses.values())
path=HERE/'Coarse.su2.native_ref'
with path.open('w') as f:
 f.write('SU2_NATIVE_REFERENCE_3D 1\n'+str(len(names))+' '+str(len(rows))+'\n')
 for name in names:f.write(json.dumps(name)+'\n')
 for i,(marker,face) in enumerate(rows):
  line=[str(i),str(marker)]
  for node in face:line+=[str(node)]+[format(x,'.17g') for x in mesh.P[node]]
  f.write(' '.join(line)+'\n')
# Verify exact coordinate round-trip before passing the file to native's own decoder/import validation.
with path.open() as f:
 assert f.readline().strip()=='SU2_NATIVE_REFERENCE_3D 1';assert list(map(int,f.readline().split()))==[3,len(rows)]
 assert [json.loads(f.readline()) for _ in names]==names
 for i,(marker,face) in enumerate(rows):
  v=f.readline().split();assert [int(v[0]),int(v[1])]==[i,marker]
  for k,node in enumerate(face):assert int(v[2+4*k])==node and np.array_equal(np.array(v[3+4*k:6+4*k],float),mesh.P[node])
 assert f.read()==''
for name,target in [('Coarse.su2','../oneram6_coarse_v1/Coarse.su2'),('donor_flow.dat','../oneram6_coarse_v1/flow_checkpoint/restart_flow.dat')]:
 p=HERE/name
 if not p.exists():p.symlink_to(target)
 assert p.resolve().is_file() and ROOT in p.resolve().parents
cfg=(SOURCE/'retry_workspace_fix/run.cfg').read_text()
changes={'RESTART_SOL':'YES','ITER':'1','ADAP_FLOW_ITER':'( 1 )','SOLUTION_FILENAME':'donor_flow',
         'RESTART_FILENAME':'frozen_flow','CONV_FILENAME':'frozen_history','VOLUME_FILENAME':'frozen_flow',
         'SURFACE_FILENAME':'frozen_surface','WRT_RESTART_COMPACT':'NO','OUTPUT_WRT_FREQ':'1',
         'VOLUME_OUTPUT':'( COORDINATES, SOLUTION, PRIMITIVE, METRIC, ADAP_SENSOR )'}
for key,value in changes.items():
 pattern=r'^'+key+r'\s*=.*$';assert re.search(pattern,cfg,re.M);cfg=re.sub(pattern,key+'= '+value,cfg,flags=re.M)
(HERE/'run.cfg').write_text(cfg)
report={'status':'PREPARED_NOT_RUN','scope':'Known original mesh authority and matching120-step SA restart; one initial CFD iteration, one follow-up iteration only if adaptation is accepted, one native event. Native decoder/import still must validate this authority. No rebase of an adapted mesh.',
        'source_mesh':str(SOURCE/'Coarse.su2'),'source_restart':str(SOURCE/'flow_checkpoint/restart_flow.dat'),
        'markers':names,'facets':len(rows),'closed_oriented_surface_edges':len(uses),'restart_coordinates_exact':True,
        'sha256':{p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in [HERE/'Coarse.su2',HERE/'donor_flow.dat',path,HERE/'run.cfg',Path(__file__)]}}
(HERE/'preparation.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))
