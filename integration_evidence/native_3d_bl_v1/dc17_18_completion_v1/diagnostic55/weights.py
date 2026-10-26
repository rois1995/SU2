"""Offline descriptors; not a production metric or calibrated runtime predictor."""
import csv
import json
from pathlib import Path
import numpy as np
from prepare import ROOT, mesh

G = np.full((3, 3), .5); np.fill_diagonal(G, 1.)
GI = np.linalg.inv(np.linalg.cholesky(G))


def mismatch(points, target):
    """Density of the generalized-eigenvalue SPD intersection; no nodal metric edit."""
    b = np.swapaxes(points[:, 1:] - points[:, :1], 1, 2)
    gram = np.swapaxes(b, 1, 2) @ target @ b
    lam = np.linalg.eigvalsh(GI @ gram @ GI.T)
    assert np.isfinite(lam).all() and np.all(lam > 0)
    volume = np.abs(np.linalg.det(b)) / 6
    # All natural metrics map K to a unit regular tet: |K| sqrt(det(M_K)) = 1/(6 sqrt(2)).
    old = np.full(len(points), 1/(6*np.sqrt(2)))
    new = volume * np.sqrt(np.linalg.det(target))
    common = old * np.sqrt(np.prod(np.maximum(1, lam), axis=1))
    assert np.all(common >= np.maximum(old, new)*(1-1e-9))
    terms = np.column_stack((common-old, common-new, new))
    assert np.all(terms >= -1e-9*np.maximum(common[:, None], 1))
    return np.maximum(terms, 0)


def proxy(points, nodal):
    target = nodal.mean(axis=1)  # Original affine P1 centroid, sensor=target for this Euler replay.
    edges = [(i, j) for i in range(4) for j in range(i+1, 4)]
    e = np.stack([points[:, j]-points[:, i] for i,j in edges], axis=1)
    squares = np.einsum('nei,nij,nej->ne', e, target, e)
    assert np.all(squares > 0)
    b = np.swapaxes(points[:, 1:]-points[:, :1], 1, 2)
    frame = np.abs(np.linalg.det(b))*np.sqrt(np.linalg.det(target))
    q = 12*np.cbrt(frame/2)**2 / squares.sum(axis=1)
    lengths = np.sqrt(squares)
    jacobians = []
    for i in range(4):
        at = [k for k,(a,b) in enumerate(edges) if i in (a,b)]
        jacobians.append(np.sqrt(2)*frame/np.prod(lengths[:, at], axis=1))
    j = np.minimum(1, np.min(jacobians, axis=0))
    work = 1 + np.sqrt(2)*frame
    for k,(a,b) in enumerate(edges):
        end_a = np.einsum('ni,nij,nj->n', e[:, k], nodal[:, a], e[:, k])
        end_b = np.einsum('ni,nij,nj->n', e[:, k], nodal[:, b], e[:, k])
        length = np.sqrt(np.maximum.reduce((squares[:, k], end_a, end_b)))
        work += np.maximum(0, np.log2(length/1.8)) + np.maximum(0, np.log2(.9/length))
    work += np.maximum(0, np.log2(.2/q)) + np.maximum(0, np.log2(.05/j))
    assert np.isfinite(work).all() and np.all(work > 0)
    return np.ceil(np.minimum(1024, work)).astype(np.uint32), work, target, q, j


def selfcheck():
    # Construct an anisotropic tet whose natural metric is diag(100,1,1).
    b = np.diag([.1,1,1]) @ np.linalg.cholesky(G).T
    points = np.concatenate((np.zeros((1,3)), b.T))[None]
    aligned, rotated = np.diag([100.,1,1]), np.diag([1.,100,1])
    a = mismatch(points, aligned[None])[0]
    r = mismatch(points, rotated[None])[0]
    assert np.allclose(a[:2], 0, atol=1e-12)
    assert np.isclose(a[2], r[2]) and np.all(r[:2] > .9)
    weights, raw, _, q, j = proxy(points, np.tile(aligned, (1,4,1,1)))
    assert np.allclose(q,1) and np.allclose(j,1) and np.allclose(raw,2)
    # Exact descriptors are invariant under simultaneous rigid rotation and metric transport.
    rotation = np.array([[0.,-1,0],[1,0,0],[0,0,1]])
    assert np.allclose(mismatch(points @ rotation.T, (rotation@rotated@rotation.T)[None])[0], r)
    return dict(aligned=a.tolist(), rotated=r.tolist(), uniform_expected_raw_weight=2,
                interpretation='Equal target densities and anisotropy; mismatch insertion/collapse terms differ')


def main():
    controls = selfcheck()
    points, cells, _ = mesh(ROOT.parent/'validation54/m6_euler/m6.su2')
    dtype=np.dtype([('id','=u8'),('x','=f8',(3,)),('m','=f8',(9,))])
    original=np.fromfile(ROOT/'sensor_original.bin',dtype=dtype,offset=8)
    metric=original['m'].reshape(-1,3,3)
    weights=[]; unclipped=[]; terms=[]; quality=[]; jac=[]
    for begin in range(0,len(cells),4096):
        ids=cells[begin:begin+4096];p=points[ids];m=metric[ids]
        w, raw, target,q,j=proxy(p,m)
        weights.append(w);unclipped.append(raw);terms.append(mismatch(p,target));quality.append(q);jac.append(j)
    weights=np.concatenate(weights);unclipped=np.concatenate(unclipped);terms=np.concatenate(terms)
    centers=points[cells].mean(axis=1)
    # Spatial bins only; they are not archived MPI ownership or independent cost observations.
    median=np.median(centers,axis=0)
    region=((centers>median)*np.array([1,2,4])).sum(axis=1)
    baseline=json.loads((ROOT/'baseline_records.json').read_text())
    expected=sum(int(r['work_before']) for r in baseline if r.get('stage')=='partition')
    aggregate_match=int(weights.sum())==expected
    result=dict(cells=len(cells),replica_sum=int(weights.sum()),production_sum=expected,
        aggregate_match=aggregate_match,per_cell_exactness_verified=False,
        current_weight_quantiles=np.quantile(weights,[0,.5,.95,1]).tolist(),
        at_1024=int((weights==1024).sum()),raw_above_1024=int((unclipped>1024).sum()),
        clipping_fraction=float((unclipped>1024).mean()),
        implementation='Independent binary64 transcription of current Euler formula; production uses scaled long-double predicates and source queries',
        controls=controls,held_out_prediction_accuracy=None,
        limitation='No regional useful-compute labels; descriptors are not a fitted or validated runtime model')
    with (ROOT/'weight_predictor_comparison.csv').open('w') as f:
        out=csv.writer(f);out.writerow(['spatial_region','cells','uniform_sum','current_replica_sum',
            'target_complexity_sum','mismatch_insert','mismatch_collapse','mismatch_optimize',
            'equal_coefficient_mismatch_sum','at_1024','raw_above_1024','useful_compute_seconds'])
        for k in range(8):
            select=region==k; t=terms[select].sum(axis=0)
            out.writerow([k,int(select.sum()),int(select.sum()),int(weights[select].sum()),t[2],*t,t.sum(),
                int((weights[select]==1024).sum()),int((unclipped[select]>1024).sum()),''])
    np.savez_compressed(ROOT/'offline_weight_descriptors.npz',weights=weights,unclipped=unclipped,
        mismatch_terms=terms,region=region,quality=np.concatenate(quality),jacobian=np.concatenate(jac))
    (ROOT/'weight_summary.json').write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps(result,indent=2))


if __name__=='__main__':
    main()
