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
    seed_definition = definition["seed"]
    if seed_definition["type"] != "SINE_PHYSICAL_V1":
        raise ValueError("supported seed: SINE_PHYSICAL_V1")
    frequencies = np.asarray(seed_definition["wave_vectors"], dtype=float)
    phases = np.asarray(seed_definition["phases"], dtype=float)
    if frequencies.shape != (5, nodes.shape[1]) or phases.shape != (5,) or not np.isfinite(frequencies).all() or not np.isfinite(phases).all():
        raise ValueError("seed needs five physical wave vectors and phases")
    seed = np.sin(nodes @ frequencies.T + phases)
    if nodes.shape[1] == 2:
        seed[:, 3:] = 0
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
    definition["generator"] = {"backend": "P1_HELMHOLTZ_NEUMANN_SINE_V1", "numpy": np.__version__,
                               "scipy": scipy.__version__, "smoothing_relative_residual": residual,
                               "mesh_sha256": hashlib.sha256(Path(args.mesh).read_bytes()).hexdigest()}
    report = write_field(args.output, nodes, cells, coefficients, definition, binary=not args.text)
    report["seconds"] = time.perf_counter()-start
    print(json.dumps(report, indent=2))


if __name__ == "__main__":
    main()
