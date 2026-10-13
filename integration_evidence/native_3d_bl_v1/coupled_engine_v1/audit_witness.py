#!/usr/bin/env python3
"""Independently certify the saved native joint witness; not an engine/CFD run."""
import json
import math
from collections import defaultdict
from pathlib import Path
from fractions import Fraction as F
import audit

HERE=Path(__file__).resolve().parent

def main():
    receipt=json.loads((HERE/'joint_witness.json').read_text())
    assert receipt['status']=='PASS'
    assert audit.digest(HERE/receipt['log'])==receipt['log_sha256']
    native=HERE/'grouped_joint_witness.json'
    frame=json.loads(native.read_text())
    seed=HERE/'independent_coupled_BL_rejected_n1.json'
    original=json.loads(seed.read_text())
    facets=original['facets']
    groups=defaultdict(list)
    for f in facets:
        points=[n[1:] for n in f['vertices']]
        f['marker']=10 if all(p[2]==0 for p in points) else 11
        planes=[(i,points[0][i]) for i in range(3) if all(p[i]==points[0][i] for p in points)]
        assert len(planes)==1
        groups[planes[0]].append(f['id'])
    cells=[dict(id=i,nodes=c) for i,c in enumerate(frame['cells'])]
    surface=[dict(nodes=f[:3],facet=f[3]) for f in frame['surface']]
    data=dict(nodes=frame['nodes'],cells=cells,surface=surface,facets=facets,
              facet_groups=list(groups.values()),original=original['original'])
    path=HERE/'independent_grouped_joint_witness.json'
    path.write_text(json.dumps(data,indent=2)+'\n')
    measured,_=audit.base.frame(path,True,metric_query=audit.tensor,edge_query=audit.edge)
    assert measured['volume']=='1'
    nodes={i:tuple(F(float(x)) for x in p) for i,*p in data['nodes']}
    errors=[];tilts=[]
    for face in surface:
        points=[nodes[i] for i in face['nodes']]
        if any(p[2]!=0 for p in points):continue
        owners=[c for c in cells if set(face['nodes']).issubset(c['nodes'])]
        assert len(owners)==1
        apex=nodes[next(i for i in owners[0]['nodes'] if i not in face['nodes'])]
        normal=audit.base.cross(audit.base.sub(points[1],points[0]),audit.base.sub(points[2],points[0]))
        norm=math.sqrt(float(audit.base.dot(normal,normal)))
        normal=[float(x)/norm for x in normal]
        center=[float(sum(p[i] for p in points)/3) for i in range(3)]
        d=[float(apex[i])-center[i] for i in range(3)];height=sum(x*y for x,y in zip(d,normal))
        errors.append(abs(abs(height)/.1-1));tilts.append(math.sqrt(sum((x-height*y)**2 for x,y in zip(d,normal)))/.1)
    assert len(errors)==2 and max(errors)<1e-8
    measured.update(height_faces=len(errors),maximum_height_error=max(errors),maximum_tilt=max(tilts))
    # A standalone inspectable grid; preserve the native point identities in JSON.
    ids={row[0]:i for i,row in enumerate(data['nodes'])}
    markers=defaultdict(list)
    byfacet={f['id']:f['marker'] for f in facets}
    for face in surface:markers[byfacet[face['facet']]].append(face['nodes'])
    with (HERE/'grouped_joint_witness.su2').open('w') as out:
        out.write('NDIME= 3\nNELEM= '+str(len(cells))+'\n')
        for c in cells:out.write('10 '+' '.join(str(ids[i]) for i in c['nodes'])+'\n')
        out.write('NPOIN= '+str(len(ids))+'\n')
        for row in data['nodes']:out.write(' '.join(format(x,'.17g') for x in row[1:])+' '+str(ids[row[0]])+'\n')
        out.write('NMARK= '+str(len(markers))+'\n')
        for marker,faces in sorted(markers.items()):
            out.write('MARKER_TAG= marker_'+str(marker)+'\nMARKER_ELEMS= '+str(len(faces))+'\n')
            for f in faces:out.write('5 '+' '.join(str(ids[i]) for i in f)+'\n')
    report=dict(status='PASS',frame=measured,native_sha256=audit.digest(native),seed_sha256=audit.digest(seed),
                receipt_sha256=audit.digest(HERE/'joint_witness.json'),helper_sha256=audit.digest(Path(__file__)),
                reused_auditors={p.name:audit.digest(p) for p in [HERE/'audit.py',audit.ROOT/'integration_evidence/native_3d_core_v1/audit_coupled.py',audit.ROOT/'integration_evidence/native_3d_core_v1/audit_cavities.py']},
                scope='Exact whole-mesh embedding/coverage/volume, original-P1 full analytic BL lengths and shape, geometric first height and tilt for one private joint witness; not full engine discovery, MPI publication, Euler release or CFD')
    (HERE/'independent_witness_audit.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report),flush=True)

if __name__=='__main__':main()
