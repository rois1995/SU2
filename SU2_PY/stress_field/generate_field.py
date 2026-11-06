#!/usr/bin/env python3
"""Generate a saved scenario on a supplied, positively oriented simplex mesh."""
import argparse
import hashlib
import json
from pathlib import Path
import time

import numpy as np
import scipy
from scipy.sparse.linalg import splu
from field_io import assemble, write_field


def make_seed(nodes, definition):
    dim=nodes.shape[1]
    if definition.get("type") == "SINE_PHYSICAL_V1":
        frequencies=np.asarray(definition["wave_vectors"],dtype=float)
        phases=np.asarray(definition["phases"],dtype=float)
        if frequencies.shape!=(5,dim) or phases.shape!=(5,) or not np.isfinite(frequencies).all() or not np.isfinite(phases).all():
            raise ValueError("seed needs five finite physical wave vectors and phases")
        seed=np.sin(nodes@frequencies.T+phases)
        if dim==2:seed[:,3:]=0
        return seed
    if definition.get("type") != "LOCALIZED_PHYSICAL_V1":
        raise ValueError("supported seeds: SINE_PHYSICAL_V1, LOCALIZED_PHYSICAL_V1")
    terms=definition["terms"]
    if not 1<=len(terms)<=64:raise ValueError("localized seed needs 1..64 terms")
    seed=np.zeros((len(nodes),5))
    for term in terms:
        center=np.asarray(term["center"],dtype=float);width=np.asarray(term["width"],dtype=float)
        direction=np.asarray(term["coefficients"],dtype=float)
        if center.shape!=(dim,) or width.shape!=(dim,) or direction.shape!=(5,) or not all(np.isfinite(a).all() for a in (center,width,direction)) or np.any(width<=0):
            raise ValueError("invalid localized center, width or tensor coefficients")
        if dim==2 and np.any(direction[3:]!=0):raise ValueError("out-of-plane localized seed")
        seed += np.exp(-.5*np.sum(((nodes-center)/width)**2,axis=1))[:,None]*direction
    return seed


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--mesh", required=True, help="NPZ with nodes and cells; no pickle")
    parser.add_argument("--definition", required=True, help="JSON geometry, scales and analytic seed")
    parser.add_argument("--output", required=True)
    parser.add_argument("--text", action="store_true")
    args = parser.parse_args()
    start = time.perf_counter()
    definition = json.loads(Path(args.definition).read_text())
    if definition.get("periodic_pairs"):
        raise ValueError("periodic identification is not implemented; use a nonperiodic mesh")
    with np.load(args.mesh, allow_pickle=False) as mesh:
        nodes, cells = mesh["nodes"], mesh["cells"]
    mass, stiffness, measure, _ = assemble(nodes, cells)
    seed = make_seed(nodes, definition["seed"])
    operator = (mass + float(definition["ell"])**2 * stiffness).tocsc()
    rhs = mass @ seed
    coefficients = splu(operator).solve(rhs)
    residual = float(np.linalg.norm(operator@coefficients-rhs) / max(np.linalg.norm(rhs), 1e-300))
    if not np.isfinite(coefficients).all() or residual > 1e-10:
        raise ValueError("sparse smoothing failed its residual check")
    n_inf = np.linalg.norm(coefficients, axis=1).max()
    n_h1 = np.sqrt(np.sum(coefficients*(operator@coefficients))/measure)
    denominator = max(n_inf, n_h1)
    if denominator > 0:
        coefficients *= float(definition["epsilon"]) / denominator
    definition["generator"] = {"backend": "P1_HELMHOLTZ_NEUMANN_V1", "numpy": np.__version__,
                               "scipy": scipy.__version__, "smoothing_relative_residual": residual,
                               "mesh_sha256": hashlib.sha256(Path(args.mesh).read_bytes()).hexdigest()}
    report = write_field(args.output, nodes, cells, coefficients, definition, binary=not args.text)
    report["seconds"] = time.perf_counter()-start
    print(json.dumps(report, indent=2))


if __name__ == "__main__":
    main()
