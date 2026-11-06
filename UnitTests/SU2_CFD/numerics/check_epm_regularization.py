"""Algebraic prototype, not a CFD implementation. Run with Python + NumPy."""
import numpy as np


CASES = ("1C_KEEP", "1C_SWAP", "2C_KEEP", "2C_SWAP", "3C")


def targets(anisotropy, epsilon):
    """Return trace-one regularized full-displacement EPM targets."""
    a = np.asarray(anisotropy, dtype=float)
    if a.shape != (3, 3) or not np.isfinite(a).all():
        raise ValueError("anisotropy must be a finite 3x3 tensor")
    if not np.allclose(a, a.T, rtol=0, atol=1e-14):
        raise ValueError("anisotropy must be symmetric")
    if not np.isfinite(epsilon) or epsilon <= 0:
        raise ValueError("epsilon must be finite and positive")
    values, vectors = np.linalg.eigh(a)
    projectors = []
    for sign in (1, -1):
        scaled = sign * values
        weights = np.exp((scaled - scaled.max()) / epsilon)
        weights /= weights.sum()
        projectors.append((vectors * weights) @ vectors.T)
    high, low = projectors
    return dict(zip(CASES, (high, low, (np.eye(3) - low) / 2,
                           (np.eye(3) - high) / 2, np.eye(3) / 3)))


def check():
    rng = np.random.default_rng(1729)
    rotation, _ = np.linalg.qr(rng.normal(size=(3, 3)))
    epsilon = 0.01  # Test scale only; not a calibrated model constant.
    iso = np.eye(3) / 3
    for values in ([0, 0, 0], [-.2, -.2, .4], [-.4, .2, .2], [-.3, -.1, .4]):
        a = np.diag(values)
        unrotated = targets(a, epsilon)
        rotated = targets(rotation @ a @ rotation.T, epsilon)
        for case in CASES:
            t = unrotated[case]
            np.testing.assert_allclose(rotated[case], rotation @ t @ rotation.T, atol=2e-14)
            np.testing.assert_allclose(t, t.T, atol=2e-14)
            np.testing.assert_allclose(np.trace(t), 1, atol=2e-14)
            assert np.linalg.eigvalsh(t).min() >= -2e-14
    # At isotropy, every case is independent of eigenvector conventions.
    for t in targets(np.zeros((3, 3)), epsilon).values():
        np.testing.assert_allclose(t, iso, atol=2e-14)
    # Converge to the same tensor from different directions, including double ties.
    for base in (np.diag([-.2, 0, .2]), np.diag([-.2, -.2, .4])):
        for q in (np.eye(3), rotation):
            errors = []
            for step in (1e-4, 1e-6, 1e-8):
                errors.append(max(np.linalg.norm(t - iso) for t in targets(step * q @ base @ q.T, epsilon).values()))
            assert errors[1] < .02 * errors[0]
            assert errors[2] < .02 * errors[1]
    # Recover the four classical extreme tensors with separated eigenvalues.
    classical = (np.diag([0., 0., 1.]), np.diag([1., 0., 0.]),
                 np.diag([0., .5, .5]), np.diag([.5, .5, 0.]))
    for case, ref in zip(CASES, classical):
        np.testing.assert_allclose(targets(np.diag([-.3, -.1, .4]), .001)[case], ref, atol=1e-12)
    # The unnormalized strain, not its direction alone, must tend to zero.
    # Check local production loss against the entropy bound in A units.
    for _ in range(30):
        x = rng.normal(size=(3, 3)); a = (x + x.T) / 2
        a -= np.trace(a) * np.eye(3) / 3
        lo, _, hi = np.linalg.eigvalsh(a)
        t = targets(a, epsilon)
        for case, optimum, sign, factor in (
            ("1C_KEEP", hi, 1, 1), ("1C_SWAP", lo, -1, 1),
            ("2C_KEEP", -lo / 2, 1, .5), ("2C_SWAP", -hi / 2, -1, .5)):
            loss = sign * (optimum - np.sum(t[case] * a))
            assert -2e-14 <= loss <= factor * epsilon * np.log(3) + 2e-14
    # Tiny algebraic implicit problem: coupled state response and shared adjoint.
    # This is not a SU2 Taylor test or numerical evidence for the CFD solver.
    matrix = np.array([[2., .2], [.2, 1.5]])
    objective = np.array([.7, -.4])
    adjoint = np.linalg.solve(matrix.T, objective)
    directions = np.array([np.diag([-1., 0., 1.]),
                           [[0., .3, 0.], [.3, 0., .2], [0., .2, 0.]]])
    forcing = np.array([.2, -.1])
    for baseline_a in (np.zeros((3, 3)), np.diag([-.02, -.01, .03])):
        gradients = []
        for case in CASES:
            def correction(q):
                t = targets(baseline_a + np.einsum('i,ijk->jk', q, directions), epsilon)[case]
                return forcing + .03 * np.array([t[0, 0], t[0, 1]])
            gradient = -adjoint @ correction(np.zeros(2))
            gradients.append(gradient)
            errors = []
            for amplitude in (1e-3, 5e-4, 2.5e-4):
                q = np.zeros(2)
                for _ in range(100):
                    new = np.linalg.solve(matrix, -amplitude * correction(q))
                    if np.linalg.norm(new - q) < 1e-15:
                        q = new
                        break
                    q = new
                assert np.linalg.norm(matrix @ q + amplitude * correction(q)) < 1e-13
                errors.append(abs(objective @ q - amplitude * gradient))
            if case != "3C":
                assert 3.8 < errors[0] / errors[1] < 4.2
                assert 3.8 < errors[1] / errors[2] < 4.2
            else:
                assert max(errors) < 1e-15
        if np.linalg.norm(baseline_a) > 0:
            assert np.ptp(gradients) > .001
    for invalid in (0, -1, float('nan')):
        try:
            targets(np.eye(3), invalid)
        except ValueError:
            pass
        else:
            raise AssertionError("invalid smoothing accepted")
    print("PASS: covariance, repeated eigenvalues, PSD/trace, continuity, classical limit,")
    print("production error bound, and shared-adjoint algebraic Taylor checks (five cases).")


if __name__ == '__main__':
    check()
