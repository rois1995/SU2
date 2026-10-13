"""Admit the shortest missing cell chain at a branched cavity edge; retain old proposal."""
from pathlib import Path
from collections import Counter
import json,hashlib
HERE=Path(__file__).resolve().parent
INPUT=HERE.parent/'recovery_collapse_v1/mpi1/cylinder_axis1_m1_s0_rejected.json'
raw=json.loads(INPUT.read_text());previous=HERE.parent/'surface_strip_replay_v1/probe.json'
r=json.loads(previous.read_text());b=r['best'];patch=set(b['old_cell_ids']);cells={c['id']:c['nodes'] for c in raw['cells']}
faces=lambda c:((c[1],c[2],c[3]),(c[0],c[3],c[2]),(c[0],c[1],c[3]),(c[0],c[2],c[1]))
owner={}
for cid,c in cells.items():
 for f in faces(c):owner.setdefault(tuple(sorted(f)),[]).append(cid)
trace=[]
for step in range(8):
 counts=Counter(tuple(sorted(f)) for cid in patch for f in faces(cells[cid]));skin=[f for f,n in counts.items() if n==1]
 edges=Counter(tuple(sorted((f[k],f[(k+1)%3]))) for f in skin for k in range(3));bad=sorted(e for e,n in edges.items() if n!=2)
 if not bad:break
 edge=bad[0];missing={cid for cid,c in cells.items() if all(n in c for n in edge)}-patch;groups=[]
 while missing:
  group={missing.pop()};pending=sorted(group)
  for cid in pending:
   for f in faces(cells[cid]):
    if not all(n in f for n in edge):continue
    for near in owner[tuple(sorted(f))]:
     if near in missing:missing.remove(near);group.add(near);pending.append(near)
  groups.append(group)
 assert groups,'No missing chain to close the cavity'
 added=min(groups,key=lambda g:(len(g),sorted(g)));assert len(patch|added)<=64
 trace.append(dict(obstructing_edge=edge,admitted_cells=sorted(added)));patch.update(added)
else:raise RuntimeError('Bounded cavity closure exhausted')
extra=patch-set(b['old_cell_ids']);assert extra and len(patch)<=64
b['old_cell_ids']=sorted(patch);b['cells']+= [cells[cid] for cid in sorted(extra)]
needed={n for c in b['cells'] for n in c};new={n[0]:n for n in b['nodes']};new.update({n[0]:n for n in raw['nodes'] if n[0] in needed and n[0] not in new});b['nodes']=[new[n] for n in sorted(needed)]
counts=Counter(tuple(sorted(f)) for cid in patch for f in faces(cells[cid]));skin={f for f,n in counts.items() if n==1}
newwalls={tuple(sorted(f['nodes'])):f for f in b['surface']}
for f in raw['surface']:
 key=tuple(sorted(f['nodes']))
 if key in skin and key not in newwalls and not all(n in f['nodes'] for n in b['record']['edge']):newwalls[key]=f
b['surface']=list(newwalls.values());b['record'].update(old_cells=len(patch),new_cells=len(b['cells']))
r.update(status='JOINT_STRIP_PROPOSAL_WITH_BOUNDED_CLOSURE',closure=trace,previous_probe_sha256=hashlib.sha256(previous.read_bytes()).hexdigest(),closure_script_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),scope=r['scope']+' Cavity closure admits shortest missing edge-link chain; native manifold/vertex-link and exact audit remain authoritative.')
(HERE/'probe.json').write_text(json.dumps(r,indent=2)+'\n');print(json.dumps(trace));print(r['status'])
