#!/usr/bin/env python3
"""Whole saved-field residual diagnosis, not an accepted geometry or mesh certificate."""
import hashlib,itertools,json,math
from pathlib import Path
import numpy as np
HERE=Path(__file__).resolve().parent;BASE=HERE.with_name('planar_symmetry_reference_v1');STEM='symmetry_annulus_bl_budget_20_m1'
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
def main():
    reports={}
    for version in ('v4','v7'):
        folder=BASE/('annulus_'+version+'_n1');path=folder/(STEM+'_rejected_rank0.json');data=json.loads(path.read_text());profile=json.loads((folder/(STEM+'_frozen_profile.json')).read_text())
        assert profile['parameter']==0 and profile['original_field'];theta=profile['core'];nodes={n[0]:np.array(n[1:],float) for n in data['nodes']}
        def normal(z):
            h=np.maximum(.1,(.1+.2*z)/1.1);u=np.clip((z-.27)/.03,0,1);w=1-u*u*(3-2*u)
            return np.exp(np.maximum(np.log(theta),w*(-2*np.log(h))+(1-w)*np.log(theta)))
        edges={tuple(sorted(e)) for c in data['cells'] for e in itertools.combinations(c['nodes'],2)};lengths={};gx,gw=np.polynomial.legendre.leggauss(48)
        for e in edges:
            a,b=[nodes[n] for n in e];d=b-a;cuts=[0.,1.]
            if d[2]:cuts += [(z-a[2])/d[2] for z in (.05,.27,.3) if 0<(z-a[2])/d[2]<1]
            cuts.sort();value=0.
            for low,high in zip(cuts,cuts[1:]):
                t=(low+high)/2+(high-low)*gx/2;value+=(high-low)/2*float(gw@np.sqrt(theta*(d[0]**2+d[1]**2)+normal(a[2]+t*d[2])*d[2]**2))
            lengths[e]=float(value)
        cells=[]
        for c in data['cells']:
            p=np.array([nodes[n] for n in c['nodes']]);v=p[1:]-p[0];det=float(v[0]@np.cross(v[1],v[2]));assert det>0
            n=float(normal(p[:,2].mean()));dv=det*theta*math.sqrt(n)
            squared={(i,j):theta*sum((p[j,:2]-p[i,:2])**2)+n*(p[j,2]-p[i,2])**2 for i,j in itertools.combinations(range(4),2)}
            q=float(12*(dv/2)**(2/3)/sum(squared.values()));jac=min(math.sqrt(2)*dv/math.prod(math.sqrt(squared[tuple(sorted((i,k)))]) for k in range(4) if k!=i) for i in range(4))
            longest=max(lengths[tuple(sorted(e))] for e in itertools.combinations(c['nodes'],2))
            if q<.2 or jac<.05 or longest>1.8:cells.append(dict(id=c['id'],nodes=c['nodes'],q=q,J=jac,maximum_edge=longest,shape=q<.2 or jac<.05,size=longest>1.8))
        bad=[dict(edge=e,length=l,points=[list(nodes[n]) for n in e]) for e,l in lengths.items() if l>1.8];bad.sort(key=lambda r:-r['length'])
        reports[version]=dict(cells=len(data['cells']),nodes=len(nodes),invalid_cells=len(cells),bad_shape_cells=sum(c['shape'] for c in cells),bad_size_cells=sum(c['size'] for c in cells),oversized_edges=len(bad),maximum_edge=max(lengths.values()),squared_excess=sum(max(0,l-1.8)**2 for l in lengths.values()),bad_cells=cells,bad_edges=bad,source_sha256=sha(path),profile_sha256=sha(folder/(STEM+'_frozen_profile.json')))
        print(version,json.dumps({k:v for k,v in reports[version].items() if k not in ['bad_cells','bad_edges']}),flush=True)
    assert reports['v4']['profile_sha256']==reports['v7']['profile_sha256']
    result=dict(status='SAME_FIELD_TERMINAL_RESIDUAL_DIAGNOSIS',reports=reports,auditor_sha256=sha(Path(__file__)),limitations=['Single planar wall/constant original sensor; quadrature is a numerical reference, not a proven bound','No independent full geometry/height/transfer or accepted-mesh claim','Queue counts can be zero after blocked candidates retire; these are independently reconstructed field residuals'])
    (HERE/'terminal_residuals.json').write_text(json.dumps(result,indent=2)+'\n')
if __name__=='__main__':main()
