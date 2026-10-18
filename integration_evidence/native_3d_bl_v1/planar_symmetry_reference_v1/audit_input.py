#!/usr/bin/env python3
"""Audit an explicitly declared Cartesian input plane without changing any input or frozen field."""
import argparse, hashlib, json, sys
from fractions import Fraction as F
from pathlib import Path
import numpy as np
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2]
sys.path.insert(0,str(ROOT/'TestCases/adaptation/capability'))
from capcheck import read_su2

def sha(p):
    h=hashlib.sha256()
    with p.open('rb') as stream:
        for block in iter(lambda:stream.read(1<<20),b''):h.update(block)
    return h.hexdigest()

def determinant(t):
    a,b,c,d=[[F(float(x)) for x in p] for p in t]
    u=[b[i]-a[i] for i in range(3)];v=[c[i]-a[i] for i in range(3)];w=[d[i]-a[i] for i in range(3)]
    return u[0]*(v[1]*w[2]-v[2]*w[1])-u[1]*(v[0]*w[2]-v[2]*w[0])+u[2]*(v[0]*w[1]-v[1]*w[0])

def main():
    p=argparse.ArgumentParser();p.add_argument('mesh',type=Path);p.add_argument('--marker',required=True)
    p.add_argument('--axis',choices=['X','Y','Z'],required=True);p.add_argument('--coordinate',type=float,required=True)
    p.add_argument('--maximum-deviation',type=float,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
    mesh=read_su2(a.mesh);assert mesh.dim==3 and a.marker in mesh.M
    axis='XYZ'.index(a.axis);ids=np.unique(mesh.M[a.marker]);candidate=mesh.P.copy();candidate[ids,axis]=a.coordinate
    error=np.abs(mesh.P[ids,axis].astype(np.longdouble)-np.longdouble(a.coordinate));assert np.isfinite(error).all()
    touched=np.flatnonzero(np.isin(mesh.E,ids).any(axis=1));before=mesh.P[mesh.E[touched]].astype(np.longdouble);after=candidate[mesh.E[touched]].astype(np.longdouble)
    def values(t):return np.sum((t[:,1]-t[:,0])*np.cross(t[:,2]-t[:,0],t[:,3]-t[:,0]),axis=1)
    old,new=values(before),values(after);den=np.sum(np.abs(before[:,1]-before[:,0])*np.abs(np.cross(before[:,2]-before[:,0],before[:,3]-before[:,0])),axis=1)
    difficult=np.flatnonzero((np.abs(old)<1e-12*den)|(np.abs(new)<1e-12*den)|(old*new<=0))
    exact=[]
    for k in difficult:
        v,w=determinant(before[k]),determinant(after[k]);exact.append({'cell':int(touched[k]),'old_sign':int(v>0)-int(v<0),'new_sign':int(w>0)-int(w<0)})
        assert v and w and (v>0)==(w>0)
    assert np.all(old*new>0) and np.max(error)<=a.maximum_deviation
    regions={}
    for name,faces in mesh.M.items():
        junction=np.intersect1d(np.unique(faces),ids)
        if name!=a.marker and len(junction):regions[name]={'shared_vertices':len(junction),'maximum_displacement':float(np.max(np.abs(mesh.P[junction,axis]-a.coordinate)))}
    edge_uses={}
    for f in mesh.M[a.marker]:
        for i in range(3):
            edge=tuple(sorted((int(f[i]),int(f[(i+1)%3]))));edge_uses[edge]=edge_uses.get(edge,0)+1
    assert set(edge_uses.values())<=set([1,2])
    result={'scope':'Input geometry audit only; no field, reference, remeshing, transfer or mesh mutation',
        'input':str(a.mesh.resolve()),'sha256':sha(a.mesh),'audit_source_sha256':sha(Path(__file__)),
        'plane':{'marker':a.marker,'axis':a.axis,'coordinate':a.coordinate,'maximum_permitted_deviation':a.maximum_deviation},
        'vertices':len(ids),'facets':len(mesh.M[a.marker]),'maximum_original_deviation':float(max(error)),
        'rms_original_deviation':float(np.sqrt(np.mean(error*error))),'nonconforming_vertices':int(np.count_nonzero(error)),
        'shared_surface_regions':regions,'affected_tetrahedra':len(touched),'minimum_signed_volume_ratio':float(min(new/old)),
        'maximum_signed_volume_ratio':float(max(new/old)),'orientation_signs_retained':True,'exact_fallbacks':exact,
        'plane_chart_euler_characteristic':len(ids)-len(edge_uses)+len(mesh.M[a.marker]),
        'chart_boundary_edges':sum(n==1 for n in edge_uses.values()),
        'limitations':['Long-double determinant screening, exact rational signs only near uncertain cells; no interval volume bound',
            'Does not establish global embedding, finite chart coverage or adapted mesh acceptance',
            'Existing off-plane frozen fields/restarts cannot silently adopt this changed initial reference']}
    a.output.write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))
if __name__=='__main__':main()
