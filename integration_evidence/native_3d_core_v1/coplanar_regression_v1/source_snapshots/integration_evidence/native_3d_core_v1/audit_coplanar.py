"""Exact finite same-marker planar patch audits of actual MPI records, including subset returns."""
from collections import defaultdict
from copy import deepcopy
from fractions import Fraction as F
import hashlib
import json
from pathlib import Path
import sys
from audit_coupled import frame, reference_patches
from audit_cavities import cross, sub

log,destination=map(lambda x:Path(x).resolve(),sys.argv[1:3]);ranks=int(sys.argv[3])
print('Case working and retained folder:',destination,flush=True)
destination.mkdir(exist_ok=False)
groups=defaultdict(list);source=defaultdict(list)
for line in log.read_text().splitlines():
    if line.startswith('MESH coplanar_'):
        p=line.split();assert len(p)==28
        vertices=[[int(p[4+4*i])]+list(map(float,p[5+4*i:8+4*i])) for i in range(4)]
        physical=[[int(p[20+2*i]),int(p[21+2*i])] for i in range(4)]
        groups[p[1]].append(dict(id=int(p[2]),version=int(p[3]),vertices=vertices,physical=physical))
    elif line.startswith('COPLANAR_SOURCE '):
        p=line.split();assert len(p)==44
        vertices=[[int(p[4+10*i])]+list(map(float,p[5+10*i:8+10*i]))+
                  [list(map(float,p[8+10*i:14+10*i]))] for i in range(4)]
        source[p[1]].append(dict(vertices=vertices))
assert len(groups['coplanar_initial'])==4
facets={}
def faces(v):
    a,b,c,d=v;return [(b,c,d),(a,d,c),(a,b,d),(a,c,b)]
for row in groups['coplanar_initial']:
    for face,(present,id) in zip(faces(row['vertices']),row['physical']):
        if present:
            assert id not in facets
            facets[id]=dict(id=id,marker=70 if all(v[3]==0 for v in face) else id+10,vertices=face)
assert len(facets)==8
base=sorted(id for id,f in facets.items() if f['marker']==70);assert len(base)==4
patches=[base]+[[id] for id in sorted(set(facets)-set(base))]
records=[];data_by_name={};workers=sorted({ranks,max(1,ranks-1),max(1,ranks//2)})
assert set(groups)=={'coplanar_initial'}|{'coplanar_m%d_s%d'%(m,s) for m in workers for s in range(3)}, 'Incomplete/rejected cycle'
for label,rows in sorted(groups.items()):
    original=source[label] if label!='coplanar_initial' else source['coplanar_m%d_s0'%workers[0]]
    assert len(original)==4
    tensor=[4,0,0,4,0,4] if label.endswith('_s1') else [.25,0,0,.25,0,.25]
    assert all(v[4]==tensor for cell in original for v in cell['vertices'])
    assert sorted([v[:4] for c in original for v in c['vertices']])==sorted([v for c in groups['coplanar_initial'] for v in c['vertices']]), 'Frozen source geometry changed'
    nodes={};cells=[];physical=[]
    for row in rows:
        cells.append(dict(id=row['id'],nodes=[v[0] for v in row['vertices']]))
        for v in row['vertices']:
            if v[0] in nodes:assert nodes[v[0]]==v
            nodes[v[0]]=v
        for face,(present,id) in zip(faces(row['vertices']),row['physical']):
            if present:physical.append(dict(facet=id,nodes=[v[0] for v in face]))
    data=dict(nodes=[nodes[id] for id in sorted(nodes)],cells=cells,surface=physical,
              facets=[facets[id] for id in sorted(facets)],facet_groups=patches,original=original)
    path=destination/(label+'.json');path.write_text(json.dumps(data,indent=2)+'\n')
    measured,_=frame(path,True);assert measured['volume']=='1/3'
    if label.endswith('_s0'):assert measured['cells']==2 and measured['physical_faces']==6
    data_by_name[label]=data;records.append(measured)
# The authority grouping itself is re-proved with exact geometry; a supplied label is no certificate.
def expect_rejected(name,data):
    path=destination/(name+'_rejected.json');path.write_text(json.dumps(data,indent=2)+'\n')
    try:frame(path,True)
    except (AssertionError,KeyError):return name
    raise AssertionError('Audit admitted adversary '+name)
chosen=data_by_name['coplanar_m%d_s0'%workers[0]]
bad=deepcopy(chosen);bad['facet_groups']=[sorted(facets)]
adversaries=[expect_rejected('cross_marker_group',bad)]
bad=deepcopy(chosen);bad['facet_groups']=[[id] for id in sorted(facets)]
adversaries.append(expect_rejected('original_seam_constraint',bad))
bad=deepcopy(chosen);bad['surface'][0]['facet']=next(id for id in facets if id not in base)
adversaries.append(expect_rejected('wrong_facet_binding',bad))
# A planar annulus must not be certified as its convex hull even if it has the same marker.
ring=[(0,0,0),(3,0,0),(3,3,0),(0,3,0),(1,1,0),(2,1,0),(2,2,0),(1,2,0)];hole={}
for i in range(4):
    j=(i+1)%4
    for tri in [(i,j,4+j),(i,4+j,4+i)]:
        points=[tuple(map(F,ring[n])) for n in tri];normal=cross(sub(points[1],points[0]),sub(points[2],points[0]))
        id=len(hole);hole[id]=dict(vertices=[[n]+list(ring[n]) for n in tri],points=points,normal=normal,axis=2,marker=70)
try:reference_patches(hole,[list(hole)])
except AssertionError:adversaries.append('annulus_convexification')
else:raise AssertionError('Audit certified a hole as filled')
result=dict(status='PASS',frames=records,adversaries=adversaries,source_log_sha256=hashlib.sha256(log.read_bytes()).hexdigest(),
            scope='Actual MPI native-engine wall coarsening across original coplanar same-marker seams, refinement and coarsening with N=M/M<N returns. Independent exact convex disk/oriented boundary/finite marker area coverage, embedding/manifold/positive volume and unchanged original sensor-P1 q/J/edge checks. No CFD state/ParMETIS/CGNS or general nonconvex wall freedom certificate.')
result['artifact_sha256']={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in destination.iterdir() if p.is_file()}
(destination/'independent_audit.json').write_text(json.dumps(result,indent=2)+'\n')
print('PASS',len(records),'finite-patch MPI frames and',len(adversaries),'adversaries',flush=True)
