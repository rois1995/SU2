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
    sensor=base.metric(original,p);s=F(4)+12*p[0]
    assert sensor==[s,0,0,s,0,s], 'Declared original six-cell scalar P1 sensor changed'
    z=float(p[2]);h=max(.1,2*(.1+.2*z)/2.2)
    u=max(0.,min(1.,(z-.27)/.03));w=1-u*u*(3-2*u)
    normal=h**(-2*w)*4**(1-w)
    # Sensor >=4 dominates both geometric tangent eigenvalues throughout the unchanged fade.
    return (s,F(0),F(0),s,F(0),max(s,F(normal)))

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
    # The max(sensor, normal BL) crossover is another quadrature kink on this declared scalar field.
    def crossover(t):
        x=float((1-t)*a[0]+t*b[0]);z=float((1-t)*a[2]+t*b[2]);h=max(.1,2*(.1+.2*z)/2.2)
        u=max(0.,min(1.,(z-.27)/.03));w=1-u*u*(3-2*u)
        return h**(-2*w)*4**(1-w)-(4+12*x)
    roots=[]
    for lower,upper in zip(cuts,cuts[1:]):
        samples=[lower+(upper-lower)*k/32 for k in range(33)];values=[crossover(t) for t in samples]
        for k in range(1,32):
            if values[k]==0 and (values[k-1]!=0 or values[k+1]!=0):roots.append(samples[k])
        for left,right,fl,fr in zip(samples,samples[1:],values,values[1:]):
            if fl*fr>=0:continue
            for _ in range(64):
                middle=(left+right)/2
                if middle==left or middle==right:break
                fm=crossover(middle)
                if fm==0:left=right=middle;break
                if fl*fm<0:right=middle
                else:left=middle;fl=fm
            roots.append((left+right)/2)
    cuts=sorted(set(cuts+roots))
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
    stage=sys.argv[1] if len(sys.argv)==2 else 'sensor';assert len(sys.argv)<=2 and stage in ('sensor','sensor_mpi2','sensor_mpi4')
    receipt=json.loads((HERE/(stage+'.json')).read_text());folder=HERE/stage;log=folder/'native.log'
    assert all(digest(folder/p)==h for p,h in receipt['artifacts'].items())
    ranks=receipt['contract']['MPI_ranks'];prefix='finer_grouped_'
    initial=read_su2(folder/('finer_grouped_initial_euler_n'+str(ranks)+'.su2'));assert initial.dim==3 and len(initial.E)==6
    corners=[(0,0,0),(1,0,0),(1,1,0),(0,1,0),(0,0,1),(1,0,1),(1,1,1),(0,1,1)]
    native={tuple(map(float,p)):i for i,p in enumerate(corners)}
    vertices=[[native[tuple(p)],*map(float,p)] for p in initial.P]
    original=[dict(vertices=[vertices[int(i)]+[[4+12*vertices[int(i)][1],0,0,4+12*vertices[int(i)][1],0,4+12*vertices[int(i)][1]]] for i in cell]) for cell in initial.E]
    triangles=[]
    for name,marker_faces in initial.M.items():
        marker=int(name.split('_')[1])
        for face in marker_faces:
            points=[vertices[int(i)] for i in face]
            triangles.append((tuple(sorted(p[0] for p in points)),marker,points))
    triangles.sort(key=lambda row:row[0]);assert len(triangles)==12 and len({row[0] for row in triangles})==12
    facets=[dict(id=i,marker=marker,vertices=points) for i,(_,marker,points) in enumerate(triangles)]
    groups=defaultdict(list)
    if prefix:
        for f in facets:
            points=[v[1:] for v in f['vertices']]
            f['marker']=10 if all(p[2]==0 for p in points) else 11
            planes=[(i,points[0][i]) for i in range(3) if all(p[i]==points[0][i] for p in points)]
            assert len(planes)==1;groups[planes[0]].append(f['id'])
    meshes=defaultdict(dict)
    for line in log.read_text().splitlines():
        r=line.split()
        if not r or r[0]!='MESH' or r[1] not in {prefix+'coupled_BL',prefix+'coupled_BL_rejected',prefix+'released_Euler',prefix+'released_Euler_rejected'}:continue
        assert len(r)==28 and int(r[2]) not in meshes[r[1]]
        meshes[r[1]][int(r[2])]=dict(id=int(r[2]),version=int(r[3]),
            vertices=[[int(r[4+4*i])]+list(map(float,r[5+4*i:8+4*i])) for i in range(4)],
            physical=[list(map(int,r[20+2*i:22+2*i])) for i in range(4)])
    assert meshes,'No complete mesh-frame records'
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
        path=folder/('independent_'+label+'_n'+str(ranks)+'.json')
        data=dict(nodes=list(nodes.values()),cells=cells,surface=physical,facets=facets,original=original)
        if prefix:data['facet_groups']=list(groups.values())
        path.write_text(json.dumps(data,indent=2)+'\n')
        released='released_Euler' in label
        measured,_=base.frame(path,False,validate_shape=receipt['status']=='PASS',metric_query=base.metric if released else tensor,edge_query=base.length if released else edge)
        assert measured['volume']=='1'
        lookup={i:tuple(map(F,p)) for i,*p in data['nodes']};errors=[];tilts=[];footprints=[]
        for face in physical:
            points=[lookup[i] for i in face['nodes']]
            if released or any(p[2]!=0 for p in points):continue
            owners=[c for c in cells if set(face['nodes']).issubset(c['nodes'])];assert len(owners)==1
            apex=lookup[next(i for i in owners[0]['nodes'] if i not in face['nodes'])]
            normal=base.cross(base.sub(points[1],points[0]),base.sub(points[2],points[0]));length=math.sqrt(float(base.dot(normal,normal)))
            n=[float(x)/length for x in normal];center=[float(sum(p[j] for p in points)/3) for j in range(3)]
            d=[float(apex[j])-center[j] for j in range(3)];height=sum(x*y for x,y in zip(d,n))
            a,b,c=points;q=apex
            def area(u,v,w):return (v[0]-u[0])*(w[1]-u[1])-(v[1]-u[1])*(w[0]-u[0])
            total=area(a,b,c);foot=min(area(q,b,c)/total,area(a,q,c)/total,area(a,b,q)/total)
            footprints.append(float(foot));assert foot>=F(-1e-14)
            errors.append(abs(abs(height)/.1-1));tilts.append(math.sqrt(sum((x-height*y)**2 for x,y in zip(d,n)))/.1)
        assert released or (errors and max(errors)<1e-8)
        measured.update(height_faces=len(errors),max_relative_height_error=max(errors,default=0),max_tilt=max(tilts,default=0),
            minimum_footprint_barycentric=min(footprints,default=1/3),full_target_valid=measured['maximum_length']<=1.8+1e-8 and measured['minimum_mean_ratio']>=.2-1e-8 and measured['minimum_scaled_jacobian']>=.05-1e-8)
        reports.append(measured)
    if receipt['status']=='PASS':assert len(reports)==2 and all(r['full_target_valid'] for r in reports),'Completed native event fails the independent full-target gates'
    result=dict(status='REJECTED_PROGRESS' if receipt['status']!='PASS' else 'PASS_SERIAL_LIMITED_SCOPE' if ranks==1 else 'PASS_MPI_LIMITED_SCOPE',MPI_ranks=ranks,frames=reports,native_status=receipt['status'],
        log=log.name,log_sha256=digest(log),helper_sha256=digest(Path(__file__)),
        reused_auditor_sha256=digest(ROOT/'integration_evidence/native_3d_core_v1/audit_coupled.py'),
        scope='Declared cube source diag(4+12*x), exact geometry/coverage and independent composed square-wall field/core4/fade/full edges/shape/height. Failed native progress remains rejected even if some local checks pass. MPI scope is the declared N=M engine rank count; no CFD/transfer, physical accuracy or performance qualification.')
    (HERE/('independent_'+stage+'.json')).write_text(json.dumps(result,indent=2)+'\n')
    print(result['status'],json.dumps(reports),flush=True)

if __name__=='__main__':main()
