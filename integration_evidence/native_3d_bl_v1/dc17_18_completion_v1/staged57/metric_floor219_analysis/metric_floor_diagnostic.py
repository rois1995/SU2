"""Isolated numerical diagnostic; never changes SU2 sources or runs adaptation."""
import argparse
import hashlib
import json
import time
from pathlib import Path
import sys
import numpy as np

REPO = Path('/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated')
sys.dont_write_bytecode = True
sys.path.insert(0, str(REPO / 'TestCases/adaptation/capability'))
import capcheck

RUN = REPO / 'integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/m6_runs/euler_build217_real_cfd50k_inspection_metric'
OUT = Path(__file__).parent


def sensor_spectrum(H, volume, local_floor=False, target=50000, norm=1):
    """Lp sensor construction; local_floor is an unqualified alternative."""
    dim = H.shape[-1]
    eig, vectors = np.linalg.eigh(H)
    eig = np.abs(eig)
    if local_floor:
        largest = eig.max(axis=1)
        eig = np.maximum(eig, largest[:, None] * 1e-14)
    else:
        scale = np.max(np.abs(H))
        eig = np.maximum(eig / (scale if scale else 1), 1e-16)
    active = eig.max(axis=1) > 0
    result = np.zeros_like(eig)
    if not active.any():
        result[:] = (target / volume.sum()) ** (2 / dim)
        return result, vectors
    logs = np.log(eig[active])
    logdet = logs.sum(axis=1)
    terms = np.log(volume[active]) + norm * logdet / (2*norm+dim)
    peak = terms.max()
    log_integral = peak + np.log(np.exp(terms - peak).sum())
    metric_logs = logs - logdet[:, None] / (2*norm+dim) + (2 / dim) * (np.log(target) - log_integral)
    result[active] = np.exp(metric_logs)
    result = np.maximum(result, result.max(axis=1)[:, None] * 1e-14)
    return result, vectors


def normalize_bounded(eig, volume, target=50000):
    """Test the physical bounds and normalization without native gradation."""
    lo, hi = -700., 700.
    with np.errstate(divide='ignore'):
        logs = np.log(eig)
    for _ in range(100):
        mid = (lo + hi) / 2
        bounded = np.exp(np.clip(logs + mid, np.log(1e-4), np.log(1e12)))
        bounded = np.maximum(bounded, bounded.max(axis=1)[:, None] / 1e12)
        complexity = np.sum(volume * np.sqrt(np.prod(bounded, axis=1)))
        if complexity < target:
            lo = mid
        else:
            hi = mid
    assert np.isfinite(bounded).all() and (bounded > 0).all()
    assert abs(complexity / target - 1) < 1e-10
    return bounded


def self_tests():
    results = {}
    V = np.array([2e6, 1., 1e-18])
    H = np.array([np.eye(3) * 1e-12, np.diag([50., 10., 2.]), np.eye(3) * 1e13])
    for local in [False, True]:
        eig, Q = sensor_spectrum(H, V, local)
        scaled, _ = sensor_spectrum(H * 1e9, V, local)
        assert np.allclose(eig, scaled, rtol=1e-12)
        normalize_bounded(eig, V)
        results['unit_invariance_' + str(local)] = True
    weak = np.array([np.eye(3) * 1e-12, np.eye(3) * 50., np.eye(3)])
    extreme = weak.copy(); extreme[2] *= 1e13
    ratios = {}
    for local in [False, True]:
        a, _ = sensor_spectrum(weak, V, local)
        b, _ = sensor_spectrum(extreme, V, local)
        ratios[str(local)] = [float(a[1, 0] / a[0, 0]), float(b[1, 0] / b[0, 0])]
    assert ratios['False'][1] < ratios['False'][0] / 100
    assert np.isclose(*ratios['True'], rtol=1e-12)
    results['remote_outlier_weak_vs_feature_ratios'] = ratios
    rank_one = np.array([np.diag([0., 0., 20.]), np.zeros((3, 3)), np.eye(3)])
    eig, Q = sensor_spectrum(rank_one, np.ones(3), True)
    assert (eig[0] > 0).all() and (eig[1] == 0).all()
    normalize_bounded(eig, np.ones(3))
    results['rank_deficient_and_locally_flat'] = True
    flat, _ = sensor_spectrum(np.zeros((3, 3, 3)), np.ones(3), True)
    normalize_bounded(flat, np.ones(3))
    results['globally_flat'] = True
    rotation, _ = np.linalg.qr(np.array([[1., 2., 3.], [3., -2., 1.], [2., 1., -3.]]))
    e0, q0 = sensor_spectrum(H, V, True)
    e1, q1 = sensor_spectrum(rotation @ H @ rotation.T, V, True)
    m0 = np.einsum('nik,nk,njk->nij', q0, e0, q0)
    m1 = np.einsum('nik,nk,njk->nij', q1, e1, q1)
    rotated = rotation @ m0 @ rotation.T
    assert np.all(np.linalg.norm(m1-rotated, axis=(1,2)) <= 1e-10*np.linalg.norm(rotated, axis=(1,2)))
    results['rotation_covariance'] = True
    anisotropic = H.copy(); anisotropic[2] = np.diag([1e13,1e3,1e3])
    rotation_errors = {}
    for local in [False,True]:
        a,qa = sensor_spectrum(anisotropic,V,local)
        b,qb = sensor_spectrum(rotation@anisotropic@rotation.T,V,local)
        A = np.einsum('nik,nk,njk->nij',qa,a,qa)
        B = np.einsum('nik,nk,njk->nij',qb,b,qb)
        expected = rotation@A@rotation.T
        error = np.linalg.norm(B-expected,axis=(1,2))/np.linalg.norm(expected,axis=(1,2))
        rotation_errors[str(local)] = error.tolist()
    assert rotation_errors['False'][1] > .1
    assert max(rotation_errors['True'][:2]) < 1e-10
    results['remote_outlier_rotation_errors'] = rotation_errors
    for dim in [2,3]:
        flat = np.zeros((3,dim,dim))
        vals,_ = sensor_spectrum(flat,np.ones(3),True)
        normalize_bounded(vals,np.ones(3))
        mixed = np.stack([np.zeros((dim,dim)),np.diag([0.]*(dim-1)+[20.]),np.eye(dim)])
        vals,_ = sensor_spectrum(mixed,np.ones(3),True)
        normalize_bounded(vals,np.ones(3))
        assert (vals[0]==0).all() and (vals[1]>0).all()
    results['flat_and_rank_deficient_2D_and_3D'] = True
    return results


def recover(P, E, fields, symmetry):
    """Independent NumPy fit, NOT an exact replay of Eigen pivoted QR/MPI."""
    N = len(P)
    pairs = np.concatenate([E[:, [a, b]] for a in range(4) for b in range(4) if a != b])
    keys = np.unique(pairs[:, 0] * N + pairs[:, 1])
    offsets = np.searchsorted(keys // N, np.arange(N + 1))
    dest = keys % N
    del pairs, keys
    neighbors = lambda i: dest[offsets[i]:offsets[i + 1]]
    H = np.zeros((N, 2, 3, 3))
    residual = np.full((N, 2), np.inf)
    ring_used = np.zeros((N, 2), np.uint8)
    deadline = time.monotonic() + 100
    for i in range(N):
        if time.monotonic() > deadline:
            raise TimeoutError('100-second recovery bound reached')
        first = neighbors(i)
        for ring in [1, 2]:
            if ring == 1:
                ids = first
            else:
                if len(first) >= 11 and np.all(residual[i] <= .05):
                    break
                ids = np.unique(np.concatenate([first] + [neighbors(j) for j in first]))
                ids = ids[ids != i]
            if len(ids) < 9:
                continue
            D = P[ids] - P[i]
            _, singular, right = np.linalg.svd(D, full_matrices=False)
            if singular[-1] <= 64 * np.finfo(float).eps * singular[0]:
                continue
            W = right.T * (np.sqrt(len(ids)) / singular)[None, :]
            X = D @ W
            A = np.column_stack([X] + [X[:, a] * X[:, b] * (.5 if a == b else 1)
                                      for a in range(3) for b in range(a, 3)])
            weight = 1 / np.maximum(np.linalg.norm(X, axis=1), 1e-12)
            A *= weight[:, None]
            rhs = (fields[ids] - fields[i]) * weight[:, None]
            coefficient, _, rank, _ = np.linalg.lstsq(A, rhs, rcond=1e-10)
            if rank != 9:
                continue
            errors = np.linalg.norm(A @ coefficient - rhs, axis=0) / np.maximum(np.linalg.norm(rhs, axis=0), np.finfo(float).tiny)
            for sensor in range(2):
                if ring == 2 and len(first) >= 11 and residual[i, sensor] <= .05:
                    continue
                if len(first) >= 11 and errors[sensor] >= residual[i, sensor]:
                    continue
                scaled = np.zeros((3, 3)); k = 3
                for a in range(3):
                    for b in range(a, 3):
                        scaled[a, b] = scaled[b, a] = coefficient[k, sensor]; k += 1
                H[i, sensor] = W @ scaled @ W.T
                residual[i, sensor] = errors[sensor]
                ring_used[i, sensor] = ring
        if (i + 1) % 10000 == 0:
            print('Recovered', i + 1, 'of', N, flush=True)
    # Mirror-even scalars on the original y=0 symmetry plane.
    H[symmetry, :, 0, 1] = H[symmetry, :, 1, 0] = 0
    H[symmetry, :, 1, 2] = H[symmetry, :, 2, 1] = 0
    assert (ring_used > 0).all(), 'Incomplete recovery; no silent WLS substitution'
    return H, residual, ring_used


def audit(do_recover):
    paths = [RUN/'m6.su2', RUN/'restart_flow_adap_00000.dat',
             REPO/'SU2_CFD/src/solvers/CSolver.cpp',
             REPO/'SU2_CFD/include/gradients/computeHessiansQuadratic.hpp']
    hashes = {str(p): hashlib.sha256(p.read_bytes()).hexdigest() for p in paths}
    mesh = capcheck.read_su2(paths[0]); P, f, _ = capcheck.read_restart(paths[1])
    assert np.array_equal(P, mesh.P)
    M = np.zeros((len(P), 3, 3))
    for a, b, key in [(0,0,'XX'),(0,1,'XY'),(0,2,'XZ'),(1,1,'YY'),(1,2,'YZ'),(2,2,'ZZ')]:
        M[:,a,b] = M[:,b,a] = f['Metric_'+key]
    eig = np.linalg.eigvalsh(M)
    V = P[mesh.E]
    volume = abs(np.linalg.det(V[:,1:] - V[:,0,None])) / 6
    dual = np.bincount(mesh.E.ravel(), weights=np.repeat(volume / 4, 4), minlength=len(P))
    contributions = dual * np.sqrt(np.prod(eig, axis=1))
    floor = eig.min()
    plateau = np.max(abs(eig - floor), axis=1) < 1e-12 * floor
    total = contributions.sum()
    assert np.isclose(total, 49999.99857718798, rtol=1e-12)
    report = dict(scope='Independent offline diagnostic; no production fix or remeshing claim',
                  source_sha256=hashes, self_tests=self_tests(), complexity=float(total),
                  plateau_nodes=int(plateau.sum()), plateau_complexity=float(contributions[plateau].sum()),
                  plateau_fraction=float(contributions[plateau].sum()/total),
                  minimum_eigenvalue=float(floor), plateau_spacing=float(1/np.sqrt(floor)),
                  constant_floor_entire_domain_complexity=float(dual.sum()*floor**1.5))
    if do_recover:
        H, residual, rings = recover(P, mesh.E, np.column_stack([f['Mach'], f['Pressure']]), np.unique(mesh.M['SYMMETRY']))
        np.savez(OUT/'independent_recovery.npz', hessians=H, residual=residual, rings=rings, dual=dual)
        comparison = {}
        for sensor, name in enumerate(['Mach', 'Pressure']):
            records = {}
            for local in [False, True]:
                lam, _ = sensor_spectrum(H[:,sensor], dual, local)
                bounded = normalize_bounded(lam, dual)
                weights = dual * np.sqrt(np.prod(bounded, axis=1))
                records['local_relative_floor' if local else 'production_global_floor'] = dict(
                    old_plateau_budget_fraction=float(weights[plateau].sum()/weights.sum()),
                    target_complexity=float(weights.sum()),
                    plateau_hmin_median=float(np.median(1/np.sqrt(bounded[plateau].max(axis=1)))))
            comparison[name] = records
        report['independent_recovery'] = dict(comparison=comparison, mean_residual=float(residual.mean()),
            poor_fits=int(np.count_nonzero(residual>.05)), first_ring=int(np.count_nonzero(rings==1)),
            grown=int(np.count_nonzero(rings==2)), maximum_abs_hessian=np.max(abs(H),axis=(0,2,3)).tolist(),
            limitations='NumPy SVD solve rather than Eigen pivoted QR; no floating-resolution curvature cutoff, native gradation, sensor intersection or production acceptance replay. Cubic/quartic corrections were zero in recorded production run.')
    assert hashes == {str(p): hashlib.sha256(p.read_bytes()).hexdigest() for p in paths}
    (OUT/'results.json').write_text(json.dumps(report, indent=2)+'\n')
    print(json.dumps(report, indent=2), flush=True)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(); parser.add_argument('--recover', action='store_true')
    options = parser.parse_args(); audit(options.recover)
