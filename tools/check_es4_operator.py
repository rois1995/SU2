#!/usr/bin/env python3
"""Check -w^T R4 = -x^T J x and conservation on asymmetric random graphs."""

import argparse
import numpy as np


def state(rng, n, dim, gamma):
    rho, p = np.exp(rng.uniform(-3, 3, (2, n)))
    v = rng.uniform(-1, 1, (n, dim))
    v *= (3 * np.sqrt(gamma * p / (dim * rho)))[:, None]
    beta = rho / (2 * p)
    s = np.log(p) - gamma * np.log(rho)
    v2 = np.einsum("ij,ij->i", v, v)
    w = np.column_stack(((gamma - s) / (gamma - 1) - beta * v2,
                         2 * beta[:, None] * v, -2 * beta))
    return w, (rho, p, v, beta, v2)


def enthalpy_action(x, primitive, gamma):
    rho, p, v, beta, v2 = primitive
    db = -0.5 * x[:, -1]
    dv = (0.5 * x[:, 1:-1] - v * db[:, None]) / beta[:, None]
    vdv = np.einsum("ij,ij->i", v, dv)
    ds = -(gamma - 1) * (x[:, 0] + v2 * db + 2 * beta * vdv)
    dr = rho * (db / beta + ds) / (1 - gamma)
    dp = p * (dr / rho - db / beta)
    return np.column_stack((dr, v * dr[:, None] + rho[:, None] * dv,
                            0.5 * v2 * dr + rho * vdv + gamma * dp / (gamma - 1)))


def check_graph(rng, dim, gamma, n=40):
    w, primitive = state(rng, n, dim, gamma)
    i, k = np.triu_indices(n, 1)
    keep = (k == i + 1) | (rng.random(i.size) < 0.11)
    i, k = i[keep], k[keep]
    boundary = rng.random(n) < 0.28
    boundary[:3] = (True, False, False)
    s, t = (~boundary[i] | boundary[k]), (~boundary[k] | boundary[i])
    pi = rng.choice([1.0, 0.5, 0.25], i.size)
    chi = np.exp(rng.uniform(-6, 6, i.size))
    chi[rng.random(i.size) < 0.1] = 0
    # Force an active asymmetric split entry and a zero weight in every graph.
    pi[0], chi[0], chi[-1] = 0.5, 1.0, 0.0
    a = np.sqrt(pi * chi)
    jump = a[:, None] * (w[i] - w[k])
    x = np.zeros_like(w)
    np.add.at(x, i, s[:, None] * jump)
    np.add.at(x, k, -(t[:, None] * jump))
    q = enthalpy_action(x, primitive, gamma)
    flux = a[:, None] * (s[:, None] * q[i] - t[:, None] * q[k])
    residual = np.zeros_like(w)
    np.add.at(residual, i, flux)
    np.add.at(residual, k, -flux)

    work = np.einsum("ij,ij->i", w, residual)
    rate, production = -work.sum(), -np.sum(x * q)
    identity = abs(rate - production) / np.abs(work).sum()
    conservation = np.max(np.abs(residual.sum(axis=0)) / np.abs(flux).sum(axis=0))
    assert production <= 0, production
    assert identity <= 1e-13, identity
    assert conservation <= 1e-14, conservation
    return identity, conservation


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--seed", type=int, default=9724)
    parser.add_argument("--samples", type=int, default=24, help="graphs per dimension")
    args = parser.parse_args()
    if args.samples < 1:
        parser.error("--samples must be positive")
    rng = np.random.default_rng(args.seed)
    errors = [check_graph(rng, dim, 1.4) for dim in (2, 3) for _ in range(args.samples)]
    identity, conservation = np.max(errors, axis=0)
    print(f"{len(errors)} graphs | max relative identity error {identity:.3e} | "
          f"max relative conservation error {conservation:.3e} | all entropy rates nonpositive")


if __name__ == "__main__":
    main()
