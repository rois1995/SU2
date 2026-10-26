"""Independent centroid-shape replay on original connectivity, with exact containment.
No extrapolation, candidate repair, edge certificate or complete-mesh acceptance.
"""
import csv
import json
import re
import sys
from fractions import Fraction
from pathlib import Path
import numpy as np
from prepare import ROOT, mesh

sys.path.insert(0,str(ROOT.parents[2]))
from frozen_field_audit import FrozenField


def determinant(a,b,c):
    return a[0]*(b[1]*c[2]-b[2]*c[1])-a[1]*(b[0]*c[2]-b[2]*c[0])+a[2]*(b[0]*c[1]-b[1]*c[0])


def exact_volume(p):
    q=[[Fraction(float(x)) for x in row] for row in p]
    return determinant(*[[q[i][k]-q[0][k] for k in range(3)] for i in (1,2,3)])


class Original3D(FrozenField):
    def __call__(self, point):
        if point in self.cache:return self.cache[point]
        p=np.asarray(point,dtype=np.longdouble)
        for cell in self.candidates(p):
            xyz=self.points[self.cells[cell]]
            b=xyz[1:]-xyz[0];den=determinant(*b)
            if den==0:continue
            numer=[]
            for i in range(4):
                copy=xyz.copy();copy[i]=p;numer.append(determinant(*(copy[1:]-copy[0])))
            w=np.asarray(numer)/den
            if np.min(w)<-1e-8:continue
            # Broad-phase floating arithmetic never admits a donor: exact binary64 signs do.
            denominator=exact_volume(xyz);weights=[]
            for i in range(4):
                copy=xyz.copy();copy[i]=p;weights.append(exact_volume(copy)/denominator)
            if any(x<0 for x in weights):continue
            assert sum(weights)==1
            weights=np.array([np.longdouble(x.numerator)/np.longdouble(x.denominator) for x in weights])
            value=weights @ self.tensors[self.cells[cell]]
            self.cache[point]=np.array(value,dtype=float).reshape(3,3)
            return self.cache[point]
        raise ValueError('No exact-containing original tetrahedron; curved extension not reconstructed')


def shape(p,m):
    v=float(exact_volume(p))*np.sqrt(np.linalg.det(m))
    squares={}
    for i in range(4):
        for j in range(i+1,4):
            e=p[j]-p[i];squares[i,j]=e@m@e
    q=np.sign(v)*12*np.cbrt(abs(v)/2)**2/sum(squares.values())
    j=min(1.,min(np.sqrt(2)*abs(v)/np.sqrt(np.prod([s for edge,s in squares.items() if i in edge])) for i in range(4)))
    return q,np.sign(v)*j


def main():
    points,cells,_=mesh(ROOT.parent/'validation54/m6_euler/m6.su2')
    dtype=np.dtype([('id','=u8'),('x','=f8',(3,)),('m','=f8',(9,))])
    sensor=np.fromfile(ROOT/'sensor_original.bin',dtype=dtype,offset=8)
    field=Original3D(points,cells,sensor['m'],{})
    # Reuse a real donor centroid as a field interpolation self-check.
    sample=tuple(np.mean(points[cells[0]].astype(np.longdouble),axis=0).astype(float))
    assert np.allclose(field(sample),sensor['m'][cells[0]].mean(axis=0).reshape(3,3),rtol=1e-10,atol=1e-10)
    reports=[]
    for path in sorted((ROOT/'observed').glob('witness*.json')):
        w=json.loads(path.read_text());r=dict(file=path.name,rank=w['rank'],sequence=w['sequence'],
            feature=w['feature'],geometry_candidate=w['geometry_candidate'],metric_failure=w['metric_failure'],
            reason=w['reason'],recorded_q=w['q'],recorded_J=w['J'],final_mesh_accepted=False,
            causal_classification='unresolved',ownership_and_growth_history='not supplied by existing callback')
        for name in ('before','after'):
            patch=w[name];nodes={};ids=set();measurements=[];by_cell={};missing=0;positive=0
            for c in patch['cells']:
                assert c[0] not in ids;ids.add(c[0]);assert len(c)==5
                p=np.array([n[1:] for n in c[1:]])
                for n in c[1:]:
                    if n[0] in nodes:assert nodes[n[0]]==n[1:]
                    nodes[n[0]]=n[1:]
                positive+=exact_volume(p)>0
                center=tuple(np.mean(p.astype(np.longdouble),axis=0).astype(float))
                try:
                    measured=shape(p,field(center));measurements.append(measured);by_cell[str(c[0])]=measured
                except ValueError:missing+=1
            assert all(n in nodes for f in patch['surface'] for n in f[1:])
            assert all(row[0] in nodes for row in patch['parameters'])
            r[name]=dict(cells=len(ids),positive_cells=positive,surface_faces=len(patch['surface']),
                missing_original_source_centroids=missing,q_min=min((v[0] for v in measurements),default=None),
                J_min=min((v[1] for v in measurements),default=None),cell_shapes=by_cell)
        if w['geometry_candidate'] and w['metric_failure']==2 and not r['after']['missing_original_source_centroids']:
            # Streamed production measurement stops at its first failed cell, not the patch minimum.
            failed=re.findall(r'at cell (\d+)',w['reason']);assert len(failed)==2
            q=r['after']['cell_shapes'][failed[0]][0];j=r['after']['cell_shapes'][failed[1]][1]
            r['recorded_failure_cells']=failed
            r['shape_reproduced']=bool(np.isclose(q,w['q'],rtol=1e-8,atol=1e-11) and np.isclose(j,w['J'],rtol=1e-8,atol=1e-11))
            assert r['shape_reproduced'],(path,q,j,w['q'],w['J'])
            r['observed_failure']='positive-volume candidate fails strict shape threshold' if r['after']['positive_cells']==r['after']['cells'] else 'nonpositive-volume candidate'
        else:r['observed_failure']='recorded construction refusal; independent shape result is not admission'
        reports.append(r)
    assert len(reports)<=72 and all(sum(r['rank']==k for r in reports)<=12 for k in range(6))
    (ROOT/'witness_analysis.json').write_text(json.dumps(reports,indent=2)+'\n')
    print(json.dumps(dict(samples=len(reports),shape_reproduced=sum(r.get('shape_reproduced',False) for r in reports),
        missing_centroids=sum(r['after']['missing_original_source_centroids'] for r in reports),
        sampled_ranks=sorted(set(r['rank'] for r in reports))),indent=2))


if __name__=='__main__':main()
