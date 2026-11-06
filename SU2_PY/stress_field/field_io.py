"""Version 1 prescribed stress files. Coordinates are physical Cartesian metres.

The basis, exact P1 norms, smoothing equation and wall map follow the supplied
SA_stress_uncertainty_v07 reference. Field construction is offline, never CFD.
"""
import hashlib
import json
import math
from pathlib import Path

import numpy as np
from scipy import sparse


def assemble(nodes, cells):
    nodes = np.asarray(nodes, dtype=float)
    cells = np.asarray(cells)
    if nodes.ndim != 2 or nodes.shape[1] not in (2, 3):
        raise ValueError("nodes must have shape (N,2) or (N,3)")
    dim = nodes.shape[1]
    if cells.ndim != 2 or cells.shape[1] != dim + 1 or cells.dtype.kind not in "iu":
        raise ValueError("only integer triangle/tetrahedron connectivity is supported")
    if not (dim+1 <= len(nodes) <= 100000 and 0 < len(cells) <= 300000):
        raise ValueError("prototype limit: 100000 nodes and 300000 cells")
    if not np.isfinite(nodes).all() or cells.min() < 0 or cells.max() >= len(nodes):
        raise ValueError("invalid coordinates or connectivity")
    if len(np.unique(np.sort(cells, axis=1), axis=0)) != len(cells):
        raise ValueError("duplicate simplices")
    xy = nodes[cells]
    affine = np.concatenate((np.ones((*xy.shape[:2], 1)), xy), axis=2)
    determinants = np.linalg.det(affine)
    if np.any(determinants <= 0):
        raise ValueError("nonpositive simplex orientation")
    volume = determinants / math.factorial(dim)
    gradients = np.linalg.inv(affine)[:, 1:, :]
    size = dim + 1
    local_m = volume[:, None, None] * (np.ones((size, size)) + np.eye(size)) / (size*(size+1))
    local_k = volume[:, None, None] * np.einsum("tki,tkj->tij", gradients, gradients)
    rows = np.broadcast_to(cells[:, :, None], (len(cells), size, size)).ravel()
    cols = np.broadcast_to(cells[:, None, :], (len(cells), size, size)).ravel()
    shape = (len(nodes), len(nodes))
    mass = sparse.coo_matrix((local_m.ravel(), (rows, cols)), shape=shape).tocsr()
    stiffness = sparse.coo_matrix((local_k.ravel(), (rows, cols)), shape=shape).tocsr()
    diameter = np.linalg.norm(xy[:, :, None, :] - xy[:, None, :, :], axis=-1).max()
    return mass, stiffness, float(volume.sum()), float(diameter)


def write_field(path, nodes, cells, coefficients, definition, binary=True):
    """Validate and export raw coefficients; this function never renormalizes them."""
    path = Path(path)
    if definition.get("frame", "CARTESIAN_METERS") != "CARTESIAN_METERS" or definition.get("periodic_pairs"):
        raise ValueError("physical Cartesian metres and nonperiodic fields required")
    nodes = np.asarray(nodes, dtype=float)
    cells = np.asarray(cells)
    c = np.asarray(coefficients, dtype=float)
    mass, stiffness, measure, diameter = assemble(nodes, cells)
    dim = nodes.shape[1]
    if c.shape != (len(nodes), 5) or not np.isfinite(c).all():
        raise ValueError("expected finite (N,5) raw coefficients")
    if dim == 2 and np.any(c[:, 3:] != 0):
        raise ValueError("planar fields require c3=c4=0")
    uref, lref = float(definition["U_ref"]), float(definition["L_ref"])
    epsilon, ell, wall_length = (float(definition[key]) for key in ("epsilon", "ell", "ell_w"))
    if not all(math.isfinite(v) for v in (uref,lref,epsilon,ell,wall_length)) or min(uref,lref,ell) <= 0 or min(epsilon,wall_length) < 0:
        raise ValueError("invalid reference scales")
    purpose = definition.get("purpose", "SCENARIO")
    if purpose not in ("SCENARIO", "COUPLING_CONTROL"):
        raise ValueError("unknown purpose")
    scenario = definition["scenario"]
    if not scenario or len(scenario) > 128 or any(ch.isspace() for ch in scenario):
        raise ValueError("scenario must be a nonempty token, at most 128 characters")
    n_inf = float(np.linalg.norm(c, axis=1).max())
    n_h1 = float(np.sqrt(np.sum(c*((mass+ell**2*stiffness)@c))/measure))
    if purpose == "SCENARIO" and max(n_inf, n_h1) > epsilon*(1+1e-10):
        raise ValueError("raw norms exceed epsilon; exporter does not renormalize")
    geometry = definition["geometry"]
    geometry_type = geometry["type"]
    geometry_lines = ["GEOMETRY " + geometry_type]
    if geometry_type == "PLANES":
        walls = geometry["walls"]
        if not 1 <= len(walls) <= 32 or wall_length <= 0:
            raise ValueError("invalid plane wall count/length")
        geometry_lines[0] += " " + str(len(walls))
        for wall in walls:
            normal = np.asarray(wall["normal"], dtype=float)
            offset, weight = float(wall["offset"]), float(wall.get("weight", 1))
            if normal.shape != (3,) or not np.isfinite(normal).all() or abs(np.linalg.norm(normal)-1) > 1e-12 or not math.isfinite(offset+weight) or weight < 0:
                raise ValueError("invalid plane normal/offset/weight")
            if dim == 2 and normal[2] != 0:
                raise ValueError("planar normal must have zero z component")
            geometry_lines.append(" ".join(format(v, ".17g") for v in (*normal,offset,weight)))
    elif geometry_type == "CIRCLE":
        center = geometry["center"]
        radius = float(geometry["radius"])
        if dim != 2 or len(center) != 3 or not np.isfinite(center).all() or center[2] != 0 or not math.isfinite(radius) or radius <= 0 or wall_length <= 0:
            raise ValueError("invalid circular geometry")
        geometry_lines[0] += " " + " ".join(format(v, ".17g") for v in (*center,radius))
    elif geometry_type != "NONE":
        raise ValueError("unsupported geometry")
    support = definition["support"]
    support_type = support["type"]
    support_line = "SUPPORT " + support_type
    if support_type == "BOX":
        lo, hi, width = (np.asarray(support[key], dtype=float) for key in ("lower", "upper", "width"))
        if any(x.shape != (3,) or not np.isfinite(x).all() for x in (lo,hi,width)) or np.any(width[:dim] <= 0) or np.any(2*width[:dim] > hi[:dim]-lo[:dim]):
            raise ValueError("invalid box support")
        support_line += " " + " ".join(format(v, ".17g") for v in (*lo,*hi,*width))
    elif support_type == "RADIAL":
        start, end = float(support["start"]), float(support["end"])
        if geometry_type != "CIRCLE" or not radius < start < end:
            raise ValueError("invalid radial support")
        support_line += f" {start:.17g} {end:.17g}"
    elif support_type != "NONE":
        raise ValueError("unsupported support")
    xyz = nodes if dim == 3 else np.column_stack((nodes, np.zeros(len(nodes))))
    values = np.column_stack((xyz, c))
    if binary:
        payload = values.astype("<f8").tobytes() + cells.astype("<u4").tobytes()
    else:
        payload = ("\n".join(" ".join(format(v, ".17g") for v in row) for row in values) + "\n" +
                   "\n".join(" ".join(str(v) for v in row) for row in cells) + "\n").encode()
    header = ["SU2_STRESS_FIELD 1", "SCENARIO " + scenario, "PURPOSE " + purpose,
              "FRAME CARTESIAN_METERS", f"DIM {dim}", f"COUNTS {len(nodes)} {len(cells)}",
              "SCALES " + " ".join(format(v, ".17g") for v in (uref,lref,epsilon,ell,wall_length)),
              "NORMS " + " ".join(format(v, ".17g") for v in (n_inf,n_h1,measure,diameter)),
              *geometry_lines, support_line, "ENCODING " + ("BINARY_LE" if binary else "TEXT"),
              "PAYLOAD_SHA256 " + hashlib.sha256(payload).hexdigest(), "DATA"]
    data = ("\n".join(header) + "\n").encode() + payload
    if len(data) > 64*1024*1024:
        raise ValueError("file exceeds 64 MiB prototype limit")
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(data)
    report = {"schema": "SU2_STRESS_FIELD_1", "definition": definition,
              "file_sha256": hashlib.sha256(data).hexdigest(), "payload_sha256": hashlib.sha256(payload).hexdigest(),
              "norm_inf": n_inf, "norm_H1": n_h1, "measure": measure,
              "maximum_simplex_diameter": diameter, "H_over_ell": diameter/ell,
              "resolution_gate": "PASS" if diameter <= ell/3 else "NOT_RESOLVED",
              "nodes": len(nodes), "cells": len(cells), "CFD_status": "NOT_RUN"}
    path.with_suffix(path.suffix + ".json").write_text(json.dumps(report, indent=2, allow_nan=False) + "\n")
    return report
