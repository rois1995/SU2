#!/usr/bin/env python3
"""Exact house geometry audit plus independent analytic square-wall BL metric."""
from collections import defaultdict
from fractions import Fraction as F
import hashlib
import json
import math
from pathlib import Path
import sys

HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[2]
sys.path.insert(0,str(ROOT/'integration_evidence/native_3d_core_v1'))
sys.path.insert(0,str(ROOT/'TestCases/adaptation/capability'))
import audit_coupled as base
from capcheck import read_su2


def digest(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def faces(v):
    a,b,c,d=v
    return [(b,c,d),(a,d,c),(a,b,d),(a,c,b)]

def tensor(original,p):
    sensor=base.metric(original,p)
    assert sensor==[1,0,0,1,0,1], 'This independent analytic target requires the declared identity P1 sensor'
    z=float(p[2]);h=max(.1,2*(.1+.2*z)/2.2)
    u=max(0.,min(1.,(z-.27)/.03));w=1-u*u*(3-2*u)
    tangent=1.5**w;normal=h**(-2*w)
    return tuple(F(x) for x in ((1+tangent)/2,(1-tangent)/2,0,(1+tangent)/2,0,normal))

def edge(original,a,b):
    chord=base.sub(b,a)
    def speed(t):
        p=tuple(F(float((1-t)*x+t*y)) for x,y in zip(a,b))
        return math.sqrt(float(base.quad(tensor(original,p),chord)))
    cuts=[0.,1.]
    if a[2]!=b[2]:
        for z in (.05,.27,.3):
            t=float((F(z)-a[2])/(b[2]-a[2]))
            if 0<t<1:cuts.append(t)
    cuts=sorted(set(cuts))
    def integrate(a,b,fa,fm,fb,old,tol,depth):
        mid=(a+b)/2;l=(a+mid)/2;r=(mid+b)/2;fl,fr=speed(l),speed(r)
        left=(mid-a)*(fa+4*fl+fm)/6;right=(b-mid)*(fm+4*fr+fb)/6
        error=left+right-old
        if abs(error)<=15*tol:return left+right+error/15
        assert depth<24,'Independent Simpson reference did not converge'
        return integrate(a,mid,fa,fl,fm,left,tol/2,depth+1)+integrate(mid,b,fm,fr,fb,right,tol/2,depth+1)
    total=0
    for lower,upper in zip(cuts,cuts[1:]):
        fa,fm,fb=speed(lower),speed((lower+upper)/2),speed(upper);old=(upper-lower)*(fa+4*fm+fb)/6
        total+=integrate(lower,upper,fa,fm,fb,old,1e-10*(upper-lower),0)
    return total

def main():
    receipt=json.loads((HERE/'coupled_mpi1.json').read_text())
    log=HERE/receipt['log'];assert digest(log)==receipt['log_sha256']
    initial=read_su2(HERE/'initial_euler_n1.su2');assert initial.dim==3 and len(initial.E)==6
    corners=[(0,0,0),(1,0,0),(1,1,0),(0,1,0),(0,0,1),(1,0,1),(1,1,1),(0,1,1)]
    native={tuple(map(float,p)):i for i,p in enumerate(corners)}
    vertices=[[native[tuple(p)],*map(float,p)] for p in initial.P]
    original=[dict(vertices=[vertices[int(i)]+[[1,0,0,1,0,1]] for i in cell]) for cell in initial.E]
    facets=[]
    for name,triangles in initial.M.items():
        marker=int(name.split('_')[1])
        assert len(triangles)==1
        facets.append(dict(id=marker-10,marker=marker,vertices=[vertices[int(i)] for i in triangles[0]]))
    meshes=defaultdict(dict)
    for line in log.read_text().splitlines():
        r=line.split()
        if not r or r[0]!='MESH' or r[1]!='coupled_BL_rejected':continue
        assert len(r)==28 and int(r[2]) not in meshes[r[1]]
        meshes[r[1]][int(r[2])]=dict(id=int(r[2]),version=int(r[3]),
            vertices=[[int(r[4+4*i])]+list(map(float,r[5+4*i:8+4*i])) for i in range(4)],
            physical=[list(map(int,r[20+2*i:22+2*i])) for i in range(4)])
    assert meshes,'No rejected-frame records; accepted-frame collection must be added when the pilot succeeds'
    reports=[]
    for label,rows in meshes.items():
        nodes={};cells=[];physical=[]
        for row in rows.values():
            cells.append(dict(id=row['id'],nodes=[n[0] for n in row['vertices']]))
            for n in row['vertices']:
                if n[0] in nodes:assert nodes[n[0]]==n
                nodes[n[0]]=n
            for face,(present,facet) in zip(faces(row['vertices']),row['physical']):
                if present:physical.append(dict(nodes=[n[0] for n in face],facet=facet))
        path=HERE/('independent_'+label+'_n1.json')
        data=dict(nodes=list(nodes.values()),cells=cells,surface=physical,facets=facets,original=original)
        path.write_text(json.dumps(data,indent=2)+'\n')
        measured,_=base.frame(path,False,metric_query=tensor,edge_query=edge)
        assert measured['volume']=='1'
        lookup={i:tuple(map(F,p)) for i,*p in data['nodes']};errors=[];tilts=[]
        for face in physical:
            points=[lookup[i] for i in face['nodes']]
            if any(p[2]!=0 for p in points):continue
            owners=[c for c in cells if set(face['nodes']).issubset(c['nodes'])];assert len(owners)==1
            apex=lookup[next(i for i in owners[0]['nodes'] if i not in face['nodes'])]
            normal=base.cross(base.sub(points[1],points[0]),base.sub(points[2],points[0]));length=math.sqrt(float(base.dot(normal,normal)))
            n=[float(x)/length for x in normal];center=[float(sum(p[j] for p in points)/3) for j in range(3)]
            d=[float(apex[j])-center[j] for j in range(3)];height=sum(x*y for x,y in zip(d,n))
            errors.append(abs(abs(height)/.1-1));tilts.append(math.sqrt(sum((x-height*y)**2 for x,y in zip(d,n)))/.1)
        assert errors and max(errors)<1e-8
        measured.update(height_faces=len(errors),max_relative_height_error=max(errors),max_tilt=max(tilts),
            full_target_valid=measured['maximum_length']<=1.8+1e-8)
        reports.append(measured)
    result=dict(status='REJECTED' if any(not r['full_target_valid'] for r in reports) else 'PASS',frames=reports,
        log=log.name,log_sha256=digest(log),helper_sha256=digest(Path(__file__)),
        reused_auditor_sha256=digest(ROOT/'integration_evidence/native_3d_core_v1/audit_coupled.py'),
        scope='Exact positive volume, global no-overlap/manifold/oriented skin and finite facet coverage; independent analytic full composed square-wall metric, integrated lengths, first height and tilt. No CFD/transfer/production qualification.')
    (HERE/'independent_audit.json').write_text(json.dumps(result,indent=2)+'\n')
    print(result['status'],json.dumps(reports),flush=True)

if __name__=='__main__':main()
