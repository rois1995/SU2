#!/usr/bin/env python3
"""Reproduce shared-offset wall-foot fragility; no metric or tolerance modification."""
import hashlib,json,sys
from fractions import Fraction as F
from pathlib import Path
import numpy as np
HERE=Path(__file__).resolve().parent
sys.path.insert(0,str(HERE.with_name('annulus_coordinated_replay_v1')))
import audit
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
def main():
    base=HERE.with_name('planar_symmetry_reference_v1')/'annulus_v7_n1';stem='symmetry_annulus_bl_budget_20_m1'
    source=base/(stem+'_rejected_rank0.json');profile=base/(stem+'_frozen_profile.json')
    data=json.loads(source.read_text());theta=json.loads(profile.read_text())['core'];points=audit.points(data)
    vertex=int((HERE/'patch.dat').read_text().splitlines()[1].split()[1]);origin=np.array([float(x) for x in points[vertex]],dtype=np.longdouble)
    faces=[f for f in data['surface'] if all(points[n][2]==0 for n in f['nodes'])]
    reports=[]
    for factor in (1.,1.25,1.5,1.75):
        offset=np.longdouble(factor)/(8*np.sqrt(np.longdouble(theta)))
        for mode in ('extended_offset','represented_binary64_offset'):
            step=offset if mode=='extended_offset' else np.longdouble(float(offset))
            leaves=[tuple(F(float(x)) for x in origin+step*np.array([a,b,0],dtype=np.longdouble)) for a,b in ((-1,-1),(-1,1),(1,-1),(1,1))]
            coverage=[]
            for face in faces:
                triangle=[points[n][:2] for n in face['nodes']];area=audit.annulus.twice_area(triangle);weights=[]
                for point in leaves:
                    weights.append(min(audit.annulus.cross2(triangle[k],triangle[(k+1)%3],point[:2])/area for k in range(3)))
                coverage.append(dict(face=face['nodes'],leaves=[i for i,w in enumerate(weights) if w>=0],best_minimum_barycentric=float(max(weights))))
            reports.append(dict(factor=factor,mode=mode,offset=str(step),covered_wall_faces=sum(bool(c['leaves']) for c in coverage),wall_faces=len(faces),coverage=coverage))
    extended=next(r for r in reports if r['factor']==1.25 and r['mode']=='extended_offset')
    represented=next(r for r in reports if r['factor']==1.25 and r['mode']=='represented_binary64_offset')
    assert extended['covered_wall_faces']==6 and represented['covered_wall_faces']==8
    result=dict(status='SHARED_OFFSET_ROUNDING_REPRODUCTION',reports=reports,
        source_sha256={str(p):sha(p) for p in (source,profile,Path(__file__))},
        limits=['Analytic constant tangential sensor spacing from frozen profile; not an interval trace of every native floating tensor query',
                'Exact rational wall-foot classification for the proposed binary64 points; no acceptance tolerance changed',
                'A represented offset does not guarantee every candidate or arbitrary rotated geometry succeeds; native replay and whole lifecycle still required'])
    (HERE/'rounding.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps({k:v for k,v in result.items() if k!='reports'},indent=2))
if __name__=='__main__':main()
