#!/usr/bin/env python3
"""Exact-rational rounding experiment using the traced original composed tensor."""
import hashlib,importlib.util,itertools,json,re,sys,time
from fractions import Fraction as F
from pathlib import Path
import numpy as np
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2]
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
def main():
    start=time.monotonic();source=HERE/'mpi2_rejected_merged.json';trace=HERE/'mpi2_patch_v2/run.log'
    mesh=json.loads(source.read_text());row=next(l for l in trace.read_text().splitlines() if l.startswith('HEIGHT_STAR_FIELD'))
    field={k:float(v) for k,v in re.findall(r'(\w+)=([^ ]+)',row)}
    residual=HERE/'mpi2_residuals.json';owners=json.loads(residual.read_text())['wall_ownership'];assert len(owners)==1
    vertex=owners[0]['apex'];nodes={n[0]:tuple(F(float(x)) for x in n[1:]) for n in mesh['nodes']};origin=nodes[vertex]
    probe=HERE.with_name('annulus_coordinated_replay_v1')/'fan_split_v1/probe_v4.py'
    spec=importlib.util.spec_from_file_location('probe',probe);m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
    walls=owners[0]['wall_faces']
    def covered(point):
        result=set()
        for i,face in enumerate(walls):
            triangle=[nodes[n][:2] for n in face];area=m.audit.annulus.twice_area(triangle)
            if all(m.audit.annulus.cross2(triangle[k],triangle[(k+1)%3],point[:2])/area>=0 for k in range(3)):result.add(i)
        return result
    def bracket(x):
        q=float(x);f=F(q)
        return (q,q) if f==x else ((q,float(np.nextafter(q,np.inf))) if f<x else (float(np.nextafter(q,-np.inf)),q))
    reports=[]
    for factor,mode in itertools.product((1.,1.25,1.5,1.75),('exact_midpoint_bounds','adjacent_binary64_box')):
        step=float(np.longdouble(factor)/(8*np.sqrt(np.longdouble(max(field['xx'],field['yy'])))))
        def propose(a,b):return tuple(float(np.longdouble(float(o))+np.longdouble(step)*d) for o,d in zip(origin,(a,b,0)))
        leaves=[];observed=[]
        for a,b in ((-1,-1),(-1,1),(1,-1),(1,1)):
            nearest=tuple(F(x) for x in propose(a,b));endpoint=tuple(F(x) for x in propose(2*a,2*b))
            target=tuple((o+e)/2 for o,e in zip(origin,endpoint));bounds=[bracket(x) for x in target]
            assert all(F(lo)<=x<=F(hi) for x,(lo,hi) in zip(target,bounds))
            if mode=='adjacent_binary64_box':
                bounds=[(float(np.nextafter(float(x),-np.inf)),float(x),float(np.nextafter(float(x),np.inf))) for x in nearest]
            candidates=[nearest]+[tuple(F(x) for x in p) for p in itertools.product(*bounds)]
            best=max(candidates,key=lambda p:(len(covered(p)),-sum((x-y)**2 for x,y in zip(p,nearest))))
            leaves.append(tuple(map(float,best)));observed.append(dict(nearest=list(map(float,nearest)),admitted=list(map(float,best)),before=sorted(covered(nearest)),after=sorted(covered(best))))
        initial=set().union(*(set(r['before']) for r in observed));final=set().union(*(set(r['after']) for r in observed))
        result=dict(factor=factor,mode=mode,step=step,initial_covered=len(initial),admitted_covered=len(final),wall_faces=len(walls),leaves=observed)
        if factor==1 and mode=='adjacent_binary64_box':
            assert len(initial)<len(walls) and len(final)==len(walls)
            data,s0=m.split(mesh,vertex,[1000000,1000001],[propose(-1,0),propose(1,0)],0,field['core'],{1000000,1000001})
            data,s1=m.split(data,1000000,[1000002,1000003],leaves[:2],1,field['core'],{1000001})
            data,s2=m.split(data,1000001,[1000004,1000005],leaves[2:],1,field['core'],set())
            result['offline_metric']=m.measures(data,field['core']);assert result['offline_metric']['bad_shape']==0 and result['offline_metric']['oversized_edges']==0
        reports.append(result)
    out=dict(status='EXACT_RATIONAL_COMPONENTWISE_ADMISSION_EXPERIMENT',field=field,reports=reports,seconds=time.monotonic()-start,
        source_sha256={str(p.resolve().relative_to(ROOT)):sha(p) for p in (source,trace,residual,probe,Path(__file__))},
        limits=['Actual native tensor trace plus exact footprint checks; not an interval proof of every runtime query',
            'Offline candidate does not qualify native implementation or complete geometry/transfer/MPI',
            'Exact midpoint bracketing alone may contain no footprint-compatible binary64 point',
            'Adjacent-neighbor admission changes each proposed coordinate by at most one binary64 neighbor, does not loosen geometry/height checks and does not prove arbitrary-plane representability'])
    (HERE/'coordinate_admission.json').write_text(json.dumps(out,indent=2)+'\n');print(json.dumps(out,indent=2),flush=True)
if __name__=='__main__':main()
