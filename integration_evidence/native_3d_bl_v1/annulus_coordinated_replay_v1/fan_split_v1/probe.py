#!/usr/bin/env python3
"""Three private binary star splits: no single-background cone and no intermediate publication."""
import collections,hashlib,itertools,json,math,sys,time
from fractions import Fraction as F
from pathlib import Path
import numpy as np
HERE=Path(__file__).resolve().parent;PARENT=HERE.parent;ROOT=PARENT.parents[2]
sys.path.insert(0,str(PARENT));import audit
sys.path.insert(0,str(ROOT/'integration_evidence/native_3d_core_v1'));from audit_coupled import surface
BASE=PARENT.with_name('planar_symmetry_reference_v1')/'annulus_v7_n1';STEM='symmetry_annulus_bl_budget_20_m1'
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
def split(data,vertex,newids,positions,axis):
    nodes=audit.points(data);old=[c for c in data['cells'] if vertex in c['nodes']]
    assert old and len(old)<=128
    skin=audit.skin(old);faces=[r[1] for r in skin.values()];edges=collections.defaultdict(list)
    points=[tuple(F(float(x)) for x in p) for p in positions];one=set();zero=set();graph=collections.defaultdict(set);constraints=[]
    physical={tuple(sorted(f['nodes'])) for f in data['surface'] if all(nodes[n][2]==0 for n in f['nodes'])}
    for i,face in enumerate(faces):
        seen=[audit.determinant([p]+[nodes[n] for n in face])>0 for p in points]
        if not any(seen):raise ValueError('Neither proposed apex sees retained face')
        if not seen[0]:one.add(i)
        if not seen[1]:zero.add(i)
        if tuple(sorted(face)) in physical:
            center=sum(nodes[n][axis] for n in face)/3
            if center<nodes[vertex][axis]:zero.add(i)
            else:one.add(i)
        for u,v in zip(face,face[1:]+face[:1]):edges[tuple(sorted((u,v)))].append((i,u,v))
    for uses in edges.values():
        assert len(uses)==2
        (i,u,v),(j,_,_)=uses;d=audit.determinant(points+[nodes[u],nodes[v]])
        if d>=0:graph[j].add(i)
        if d<=0:graph[i].add(j)
        constraints.append((i,j,u,v,d))
    pending=list(one)
    while pending:
        i=pending.pop()
        for j in graph[i]-one:one.add(j);pending.append(j)
    if one&zero:raise ValueError('No positive binary face partition for proposed points/wall ownership')
    result=[]
    for i,face in enumerate(faces):result.append(dict(id=len(result),nodes=[newids[int(i in one)]]+list(face)))
    bridges=0
    for i,j,u,v,d in constraints:
        if (i in one)==(j in one):continue
        order=[newids[0],newids[1],v,u] if i not in one else [newids[1],newids[0],v,u]
        result.append(dict(id=len(result),nodes=order));bridges+=1
    newnodes=dict(nodes)
    for id,p in zip(newids,points):newnodes[id]=p
    for c in result:assert audit.determinant([newnodes[n] for n in c['nodes']])>0
    assert audit.skin(result)==skin
    assert sum(audit.determinant([newnodes[n] for n in c['nodes']]) for c in result)==sum(audit.determinant([nodes[n] for n in c['nodes']]) for c in old)
    for id in newids:surface([[n for n in c['nodes'] if n!=id] for c in result if id in c['nodes']],True)
    old_ids={c['id'] for c in old};keep=[c for c in data['cells'] if c['id'] not in old_ids];first=max(c['id'] for c in data['cells'])+1
    for i,c in enumerate(result):c['id']=first+i
    nodes.pop(vertex)
    for id,p in zip(newids,points):nodes[id]=p
    fresh=dict(data);fresh['cells']=keep+result;fresh['nodes']=[[n]+list(map(float,p)) for n,p in sorted(nodes.items())]
    return fresh,dict(retired=vertex,old=len(old),fresh=len(result),bridges=bridges)
def measures(data,theta):
    nodes={n[0]:np.array(n[1:],float) for n in data['nodes']};qmin=jmin=1.;badshape=0;edges=set()
    def normal(z):
        h=np.maximum(.1,(.1+.2*z)/1.1);u=np.clip((z-.27)/.03,0,1);w=1-u*u*(3-2*u)
        return np.exp(np.maximum(np.log(theta),w*(-2*np.log(h))+(1-w)*np.log(theta)))
    for c in data['cells']:
        p=np.array([nodes[n] for n in c['nodes']]);v=p[1:]-p[0];det=float(v[0]@np.cross(v[1],v[2]));assert det>0
        n=float(normal(p[:,2].mean()));dv=det*theta*math.sqrt(n)
        squared={(i,j):theta*sum((p[j,:2]-p[i,:2])**2)+n*(p[j,2]-p[i,2])**2 for i,j in itertools.combinations(range(4),2)}
        q=float(12*(dv/2)**(2/3)/sum(squared.values()));jac=min(math.sqrt(2)*dv/math.prod(math.sqrt(squared[tuple(sorted((i,k)))]) for k in range(4) if k!=i) for i in range(4))
        qmin=min(qmin,q);jmin=min(jmin,jac);badshape+=int(q<.2 or jac<.05);edges.update(tuple(sorted(e)) for e in itertools.combinations(c['nodes'],2))
    gx,gw=np.polynomial.legendre.leggauss(48);longest=0.;badsize=0;excess=0.
    for a,b in edges:
        p=nodes[a];d=nodes[b]-p;cuts=[0.,1.]
        if d[2]:cuts += [(z-p[2])/d[2] for z in (.05,.27,.3) if 0<(z-p[2])/d[2]<1]
        cuts.sort();value=0.
        for low,high in zip(cuts,cuts[1:]):
            t=(low+high)/2+(high-low)*gx/2;value+=(high-low)/2*float(gw@np.sqrt(theta*sum(d[:2]**2)+normal(p[2]+t*d[2])*d[2]**2))
        longest=max(longest,float(value));badsize+=int(value>1.8);excess+=float(max(0,value-1.8)**2)
    return dict(cells=len(data['cells']),nodes=len(nodes),minimum_q=qmin,minimum_J=jmin,bad_shape=badshape,oversized_edges=badsize,maximum_edge=longest,excess=excess)
def main():
    path=BASE/(STEM+'_rejected_rank0.json');original=json.loads(path.read_text());profile=BASE/(STEM+'_frozen_profile.json');theta=json.loads(profile.read_text())['core'];reports=[];start=time.monotonic()
    for delta in (.025,.035,.045,.055,.075,.1,.15,.2):
        if time.monotonic()-start>30:raise RuntimeError('30s bounded replay ceiling')
        steps=[];d=original
        try:
            d,s=split(d,50328,[900000,900001],[(.5-delta,.5,.1),(.5+delta,.5,.1)],0);steps.append(s)
            d,s=split(d,900000,[900002,900003],[(.5-delta,.5-delta,.1),(.5-delta,.5+delta,.1)],1);steps.append(s)
            d,s=split(d,900001,[900004,900005],[(.5+delta,.5-delta,.1),(.5+delta,.5+delta,.1)],1);steps.append(s)
            r=dict(delta=delta,steps=steps,**measures(d,theta));(HERE/('candidate_'+str(delta)+'.json')).write_text(json.dumps(d,separators=(',',':'))+'\n')
        except (ValueError,AssertionError) as e:r=dict(delta=delta,steps=steps,rejected=str(e) or 'exact geometry/topology assertion')
        reports.append(r);print(json.dumps(r),flush=True)
    (HERE/'report.json').write_text(json.dumps(dict(status='PRIVATE_MULTI_APEX_STAR_SPLIT_EXPERIMENT',results=reports,source_sha256=sha(path),profile_sha256=sha(profile),script_sha256=sha(Path(__file__)),seconds=time.monotonic()-start,limits=['Offline proposal coordinates/axis choices; no native implementation or acceptance','Intermediate private star splits enforce geometry/topology but not height; full-height/whole geometry/overlap audit still needed','No source field or acceptance threshold changed']),indent=2)+'\n')
if __name__=='__main__':main()
