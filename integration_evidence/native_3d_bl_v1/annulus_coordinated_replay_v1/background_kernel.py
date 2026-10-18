#!/usr/bin/env python3
"""Bounded linear-program diagnosis of the existing single-background centroid constructor."""
import hashlib,json,sys,warnings
from fractions import Fraction as F
from pathlib import Path
import numpy as np
from scipy.optimize import linprog
import audit
HERE=Path(__file__).resolve().parent
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
def main():
    proof=json.loads((HERE/'shared_height_fan.json').read_text());base=HERE.with_name('planar_symmetry_reference_v1')/'annulus_v7_n1';path=base/'symmetry_annulus_bl_budget_20_m1_rejected_rank0.json'
    data=json.loads(path.read_text());nodes=audit.points(data);apex=proof['common_apex'];cells=[c for c in data['cells'] if apex in c['nodes']];skin=audit.skin(cells)
    assert len(cells)<=64
    def plane(tet,kind):
        zero=[F(0)]*3;constant=audit.determinant([zero]+tet);gradient=[]
        for axis in range(3):
            e=[F(0)]*3;e[axis]=1;gradient.append(audit.determinant([e]+tet)-constant)
        norm=float(sum(x*x for x in gradient))**.5;assert norm>0
        return dict(a=[float(-x)/norm for x in gradient],b=float(constant)/norm,kind=kind)
    original=[plane([nodes[n] for n in f[1]],'skin') for f in skin.values()]
    walls=[]
    for row in proof['independently_placed_first_row_seeds']:
        baseface=[nodes[n] for n in row['face']];tip=tuple(map(F,row['apex']))
        for i in range(3):
            child=list(baseface);child[i]=tip;walls.append(plane(child,'centroid_child'))
    ids={n for c in cells for n in c['nodes']};bounds=[(min(float(nodes[n][axis]) for n in ids),max(float(nodes[n][axis]) for n in ids)) for axis in range(3)]
    def solve(c,rows,coordinate_bounds=None):
        with warnings.catch_warnings():
            warnings.simplefilter('ignore',category=UserWarning)
            result=linprog(c,A_ub=[r['a'] for r in rows],b_ub=[r['b'] for r in rows],bounds=bounds if coordinate_bounds is None else coordinate_bounds,method='highs',options={'threads':1})
        return dict(status=int(result.status),message=result.message,point=result.x.tolist() if result.x is not None else None,objective=float(result.fun) if result.fun is not None else None)
    maximum=solve([0,0,-1],original);minimum=solve([0,0,1],walls,bounds[:2]+[(0,None)]);joint=solve([0,0,0],original+walls)
    assert maximum['status']==minimum['status']==0
    result=dict(status='SINGLE_BACKGROUND_GEOMETRY_LP_DIAGNOSIS',retired_apex=apex,admitted_cells=len(cells),skin_faces=len(skin),maximum_background_z=-maximum['objective'],minimum_centroid_constructor_background_z=minimum['objective'],joint=joint,kernel=maximum,centroid_requirements=minimum,
        limits=['Floating LP diagnosis, not an exact infeasibility certificate','Uses the existing per-wall-face centroid seeds and one common background only; different coordinated construction remains possible','Geometry only; no complete mesh, full metric, transfer or MPI qualification'],source_sha256={str(p.relative_to(audit.ROOT)):sha(p) for p in [path,HERE/'shared_height_fan.json',Path(__file__).resolve()]})
    (HERE/'background_kernel.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))
if __name__=='__main__':main()
