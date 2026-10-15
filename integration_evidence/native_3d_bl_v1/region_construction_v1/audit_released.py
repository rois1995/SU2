#!/usr/bin/env python3
"""Exact released-mesh geometry and constant original-P1 sensor checks; no BL altitude contract."""
import collections
import hashlib
import itertools
import json
import math
import sys
import time
from fractions import Fraction as F
from pathlib import Path
import numpy as np
from construct import ROOT, authorities
sys.path.insert(0,str(ROOT/'integration_evidence/native_3d_core_v1'))
from audit_cavities import determinant, sub, dot, cross, overlap
from audit_coupled import surface, connected, spd, quad

def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

def main():
    start=time.monotonic()
    case=Path(sys.argv[1]).resolve()
    receipt_path=Path(str(case)+'.json')
    receipt=json.loads(receipt_path.read_text())
    assert receipt['status']=='PASS'
    name='curved_cylinder_released.json'
    assert sha(case/name)==receipt['artifacts'][name]
    data=json.loads((case/name).read_text())
    samples=[tuple(F(float(x)) for x in n[4]) for c in data['original'] for n in c['vertices']]
    assert samples and all(m==samples[0] for m in samples),'This audit requires a constant original-P1 tensor'
    metric=samples[0];dm=spd(metric)
    nodes={n[0]:tuple(F(float(x)) for x in n[1:]) for n in data['nodes']}
    assert len(nodes)==len(data['nodes']) and len(set(nodes.values()))==len(nodes)
    uses=collections.defaultdict(list);links=collections.defaultdict(list)
    tets=[];boxes=[];volume=F(0);qmin=jmin=1.;maximum=0.
    for index,c in enumerate(data['cells']):
        ids=c['nodes'];t=[nodes[n] for n in ids];dv=determinant(t)
        assert dv>0
        tets.append(t);volume+=dv/6
        boxes.append(tuple((min(p[k] for p in t),max(p[k] for p in t)) for k in range(3)))
        a,b,d,e=ids
        for face in [(b,d,e),(a,e,d),(a,b,e),(a,d,b)]:
            parity=(-1)**sum(face[i]>face[j] for i in range(3) for j in range(i+1,3))
            uses[tuple(sorted(face))].append((index,parity))
        for i,n in enumerate(ids):links[n].append(tuple(v for j,v in enumerate(ids) if i!=j))
        lengths={}
        for i,j in itertools.combinations(range(4),2):
            sq=quad(metric,sub(t[j],t[i]));assert sq<=F(1.8)**2
            lengths[i,j]=math.sqrt(float(sq));maximum=max(maximum,lengths[i,j])
        weighted=float(dv)*math.sqrt(float(dm))
        q=12*(weighted/2)**(2/3)/sum(v*v for v in lengths.values())
        jac=min(1.,min(math.sqrt(2)*weighted/math.prod(lengths[tuple(sorted((i,j)))] for j in range(4) if i!=j) for i in range(4)))
        assert q>=.2 and jac>=.05
        qmin=min(qmin,q);jmin=min(jmin,jac)
    assert all(len(v)==1 or len(v)==2 and sum(x[1] for x in v)==0 for v in uses.values())
    boundary={f for f,v in uses.items() if len(v)==1}
    physical={tuple(sorted(f['nodes'])) for f in data['surface']}
    assert boundary==physical and len(physical)==len(data['surface'])
    for f in data['surface']:
        face=f['nodes'];parity=(-1)**sum(face[i]>face[j] for i in range(3) for j in range(i+1,3))
        assert uses[tuple(sorted(face))][0][1]==parity
    surface(list(boundary),True)
    adjacency=collections.defaultdict(set)
    for i in range(len(tets)):adjacency[i]
    for pairs in uses.values():
        if len(pairs)==2:
            a,b=pairs[0][0],pairs[1][0];adjacency[a].add(b);adjacency[b].add(a)
    assert connected(adjacency)
    boundary_nodes={n for face in boundary for n in face}
    for n,triangles in links.items():surface(triangles,n not in boundary_nodes)
    candidates=0
    for i,j in itertools.combinations(range(len(tets)),2):
        if any(boxes[i][k][1]<=boxes[j][k][0] or boxes[j][k][1]<=boxes[i][k][0] for k in range(3)):continue
        candidates+=1
        assert not overlap(tets[i],tets[j]),(i,j)
    reference=case/'curved_cylinder_initial_euler.su2.native_ref'
    assert sha(reference)==receipt['artifacts'][reference.name]
    geometry,_=authorities(reference);error=sag=0.
    for face in data['surface']:
        kind,origin,extent,tolerance,axis=geometry[face['facet']]
        axis=axis/np.linalg.norm(axis)
        points=np.array([[float(x) for x in nodes[n]] for n in face['nodes']])
        v=points-origin;z=v@axis;radial=v-z[:,None]*axis
        if kind==4:
            error=max(error,float(np.max(np.abs(z))))
            assert np.max(np.abs(z))<=tolerance and np.max(np.linalg.norm(radial,axis=1))<=extent[0]+tolerance
        else:
            assert kind==3
            error=max(error,float(np.max(np.abs(np.linalg.norm(radial,axis=1)-extent[0]))))
            assert np.max(np.abs(np.linalg.norm(radial,axis=1)-extent[0]))<=tolerance
            assert z.min()>=-tolerance and z.max()<=extent[1]+tolerance
            signs=[np.cross(radial[(k+1)%3]-radial[k],-radial[k])@axis for k in range(3)]
            area=np.cross(radial[1]-radial[0],radial[2]-radial[0])@axis
            minimum=math.inf
            if area!=0 and (min(signs)>=0 or max(signs)<=0):minimum=0.
            for k in range(3):
                a,b=radial[k],radial[(k+1)%3];d=b-a
                t=np.clip(-a@d/(d@d),0,1) if d@d else 0
                minimum=min(minimum,float(np.linalg.norm(a+t*d)))
            sag=max(sag,float(extent[0]-minimum))
    assert sag<=.04
    report=dict(status='PASS_RELEASED_CONSTANT_P1_GEOMETRY_METRIC',cells=len(tets),nodes=len(nodes),
                minimum_q=qmin,minimum_j=jmin,maximum_metric_length=maximum,maximum_authority_error=error,
                maximum_surface_error=sag,vertex_links=len(links),overlap_pair_candidates=candidates,
                volume=str(volume),tensor=[float(x) for x in metric],source_receipt_sha256=sha(receipt_path),
                mesh_json_sha256=sha(case/name),helper_sha256=sha(Path(__file__)),
                predicates_sha256=sha(ROOT/'integration_evidence/native_3d_core_v1/audit_cavities.py'),
                topology_sha256=sha(ROOT/'integration_evidence/native_3d_core_v1/audit_coupled.py'),
                geometry_parser_sha256=sha(Path(__file__).parent/'construct.py'),elapsed_seconds=time.monotonic()-start,
                scope='Independent released-mesh positive geometry, complete oriented ball topology/all vertex links/no positive overlap, analytic authority/surface error and exact constant original-P1 tensor edge norms/shape. The same tensor is asserted across every original donor sample. No wall altitude is requested. Native validation separately establishes source coverage; no CFD/transfer/restart, variable sensor, integration, accuracy or performance claim.')
    (case/'released_independent.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({k:report[k] for k in ('status','cells','minimum_q','minimum_j','maximum_metric_length','maximum_surface_error')}))

if __name__=='__main__':main()
