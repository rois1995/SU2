#!/usr/bin/env python3
"""Independent exact geometry and changed-edge progress audit; no native acceptance claim."""
import collections,hashlib,importlib.util,itertools,json,math,sys,time
from fractions import Fraction as F
from pathlib import Path
import numpy as np
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2]
sys.path.insert(0,str(ROOT/'integration_evidence/native_3d_core_v1'))
from audit_cavities import determinant,overlap,cross,dot,sub
from audit_coupled import surface,parity
spec=importlib.util.spec_from_file_location('annulus',HERE.with_name('planar_symmetry_reference_v1')/'audit_annulus.py')
annulus=importlib.util.module_from_spec(spec);spec.loader.exec_module(annulus)
BASE=HERE.with_name('planar_symmetry_reference_v1')/'annulus_v4_n1';STEM='symmetry_annulus_bl_budget_20_m1'
OLD=BASE/(STEM+'_rejected_rank0_trial_before.json');GLOBAL=BASE/(STEM+'_rejected_rank0.json');PROFILE=BASE/(STEM+'_frozen_profile.json')
def digest(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def points(data):return {n[0]:tuple(F(float(x)) for x in n[1:]) for n in data['nodes']}
def skin(cells):
    uses=collections.defaultdict(list)
    for c in cells:
        a,b,c,d=c['nodes']
        for f in [(b,c,d),(a,d,c),(a,b,d),(a,c,b)]:uses[tuple(sorted(f))].append((parity(f),f))
    assert all(len(u)==1 or len(u)==2 and sum(s for s,_ in u)==0 for u in uses.values())
    return {f:u[0] for f,u in uses.items() if len(u)==1}
def main():
    started=time.monotonic();old=json.loads(OLD.read_text());whole=json.loads(GLOBAL.read_text());profile=json.loads(PROFILE.read_text())
    assert profile['original_field'] and profile['parameter']==0;theta=profile['core']
    p0=points(old);pg=points(whole);s0=skin(old['cells']);physical0={tuple(sorted(f['nodes'])):f for f in old['surface']}
    artificial={k:v for k,v in s0.items() if k not in physical0};fixed={n for f in artificial for n in f}
    old_ids={c['id'] for c in old['cells']};rest=[c for c in whole['cells'] if c['id'] not in old_ids]
    groups={};membership={};facets={f['id']:f for f in old['facets']}
    for group in old['facet_groups']:
        root=min(group);triangles=[]
        for id in group:
            t=[tuple(F(float(x)) for x in v[1:]) for v in facets[id]['vertices']]
            normal=cross(sub(t[1],t[0]),sub(t[2],t[0]));axis=next(i for i in range(3) if normal[i])
            if not triangles:origin=t[0];normal0=normal;axis0=axis
            assert axis==axis0 and normal[axis]*normal0[axis]>0
            assert all(dot(normal0,sub(p,origin))==0 for p in t)
            triangles.append([tuple(p[k] for k in range(3) if k!=axis) for p in t]);membership[id]=root
        groups[root]=(origin,normal0,axis0,triangles)
    def finite(data,p,sk):
        areas=collections.defaultdict(F)
        for f in data['surface']:
            key=tuple(sorted(f['nodes']));assert key in sk and parity(f['nodes'])==sk[key][0]
            root=membership[f['facet']];o,n,axis,ref=groups[root];t=[p[i] for i in f['nodes']]
            assert all(dot(n,sub(v,o))==0 for v in t),'Point off exact reference plane'
            signed=cross(sub(t[1],t[0]),sub(t[2],t[0]))[axis];assert signed*n[axis]>0
            flat=[tuple(v[k] for k in range(3) if k!=axis) for v in t]
            assert sum(annulus.intersection_area(flat,r) for r in ref)==abs(annulus.twice_area(flat)),'Face bridges a finite hole'
            areas[root]+=signed
        return areas
    areas0=finite(old,p0,s0);volume0=sum(determinant([p0[n] for n in c['nodes']]) for c in old['cells'])
    def normal(z):
        h=np.maximum(.1,(.1+.2*z)/1.1);u=np.clip((z-.27)/.03,0,1);w=1-u*u*(3-2*u)
        return np.exp(np.maximum(np.log(theta),w*(-2*np.log(h))+(1-w)*np.log(theta)))
    def edge_values(data,p,order):
        edges={tuple(sorted(e)) for c in data['cells'] for e in itertools.combinations(c['nodes'],2)}
        gx,gw=np.polynomial.legendre.leggauss(order);values={}
        for e in sorted(edges):
            a,b=[np.array(list(map(float,p[n]))) for n in e];d=b-a;cuts=[0.,1.]
            if d[2]:cuts += [(z-a[2])/d[2] for z in (.05,.27,.3) if 0<(z-a[2])/d[2]<1]
            cuts.sort();value=0.
            for low,high in zip(cuts,cuts[1:]):
                t=(low+high)/2+(high-low)*gx/2
                value+=(high-low)/2*float(gw@np.sqrt(theta*(d[0]**2+d[1]**2)+normal(a[2]+t*d[2])*d[2]**2))
            values[e]=value
        return values
    before=edge_values(old,p0,48);result={}
    for name in ['interior_only','joint_surface_volume']:
        path=HERE/(name+'.json');data=json.loads(path.read_text());p=points(data);sk=skin(data['cells']);physical={tuple(sorted(f['nodes'])):f for f in data['surface']}
        assert len(p)==len(data['nodes'])==len(set(p.values()))
        assert {k:v for k,v in sk.items() if k not in physical}==artificial
        assert all(p[n]==p0[n] for n in fixed),'Artificial interface moved'
        assert finite(data,p,sk)==areas0,'Physical patch coverage changed'
        surface([v[1] for v in sk.values()],True)
        moved={n for n in p0 if p0[n]!=p[n]}
        for n in moved:
            assert {c['id'] for c in whole['cells'] if n in c['nodes']}=={c['id'] for c in old['cells'] if n in c['nodes']},'Incomplete moved vertex star'
        tets=[[p[n] for n in c['nodes']] for c in data['cells']];det=[determinant(t) for t in tets]
        assert min(det)>0 and sum(det)==volume0
        assert len({tuple(sorted(c['nodes'])) for c in data['cells']})==len(tets)
        other=[[pg[n] for n in c['nodes']] for c in rest];pairs=0
        for a,b in itertools.chain(itertools.combinations(tets,2),itertools.product(tets,other)):
            if any(max(v[k] for v in a)<=min(v[k] for v in b) or max(v[k] for v in b)<=min(v[k] for v in a) for k in range(3)):continue
            assert not overlap(a,b),'Positive-volume overlap';pairs+=1
        qmin=jmin=1.
        for t,d in zip(tets,det):
            z=float(sum(v[2] for v in t)/4);nz=float(normal(z));density=theta*math.sqrt(nz)
            lengths={e:theta*float(t[e[1]][0]-t[e[0]][0])**2+theta*float(t[e[1]][1]-t[e[0]][1])**2+nz*float(t[e[1]][2]-t[e[0]][2])**2 for e in itertools.combinations(range(4),2)}
            volume=float(d)*density;q=12*(volume/2)**(2/3)/sum(lengths.values());j=min(math.sqrt(2)*volume/math.prod(math.sqrt(lengths[tuple(sorted((i,k)))]) for k in range(4) if k!=i) for i in range(4))
            qmin=min(qmin,q);jmin=min(jmin,j)
        assert qmin>=.2 and jmin>=.05
        after=edge_values(data,p,48);check=edge_values(data,p,24)
        spread=max(abs(v-check[e]) for e,v in after.items());assert spread<1e-8
        def shared(e):return e in before and e in after and all(p[n]==p0[n] for n in e)
        removed=sum(max(0,l-1.8)**2 for e,l in before.items() if not shared(e))
        added=sum(max(0,l-1.8)**2 for e,l in after.items() if not shared(e))
        progress=removed>0 and added<removed*(1-1e-12) and len(data['cells'])>len(old['cells'])
        result[name]=dict(status='GEOMETRY_SHAPE_PASS',cells=len(tets),moved=sorted(moved),exact_positive_volume_and_interface=True,finite_physical_coverage=True,complete_stars=True,overlap_pairs_checked=pairs,minimum_q=qmin,minimum_J=jmin,removed_squared_excess=removed,added_squared_excess=added,relative_changed_excess_reduction=(removed-added)/removed if removed else 0,refine_progress=progress,maximum_full_edge=max(after.values()),quadrature_order_spread=spread,sha256=digest(path))
        print(name,json.dumps(result[name]),flush=True)
    # The exact SAT checker must recognize overlap, and detect off-plane points.
    assert overlap(tets[0],tets[0])
    bad=dict(p);n=data['surface'][0]['nodes'][0];root=membership[data['surface'][0]['facet']];axis=groups[root][2];v=list(bad[n]);v[axis]+=F(1,100);bad[n]=tuple(v)
    try:finite(data,bad,sk)
    except AssertionError:pass
    else:raise AssertionError('Missing off-plane negative control')
    output=dict(status='INDEPENDENT_PATCH_AUDIT',seconds=time.monotonic()-started,results=result,source_sha256={str(p.resolve().relative_to(ROOT)):digest(p) for p in [OLD,GLOBAL,PROFILE,Path(__file__),HERE/'probe.py',ROOT/'integration_evidence/native_3d_core_v1/audit_cavities.py',ROOT/'integration_evidence/native_3d_core_v1/audit_coupled.py',HERE.with_name('planar_symmetry_reference_v1')/'audit_annulus.py']},limitations=['Analytic reproduction of the saved constant-sensor/single-wall frozen profile; native FrozenField validation still required','Patch progress is not whole-mesh acceptance, transfer or an adapted lifecycle','24/48 point quadrature agreement is empirical, not a proven error bound'])
    (HERE/'independent_audit.json').write_text(json.dumps(output,indent=2)+'\n')
if __name__=='__main__':main()
