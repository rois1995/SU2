#!/usr/bin/env python3
"""Small serial baseline/candidate NS manufactured-solution comparison; creates a fresh output directory."""
import argparse
import csv
import json
import math
import os
from pathlib import Path
import re
import subprocess


def mesh_text(n, aspect, curved):
    points = []
    for j in range(n + 1):
        for i in range(n + 1):
            x, y = i / n, j / n
            if 0 < i < n and 0 < j < n:
                if not curved:
                    x += .1 / n * math.sin(12.9898 * (i + (n + 1) * j))
                y += .1 / n * math.cos(78.233 * (i + (n + 1) * j))
            if curved:
                angle, radius = .8 * x, 1 + y / aspect
                points.append((radius * math.sin(angle), radius * math.cos(angle)))
            else:
                y /= aspect
                points.append((math.cos(.37) * x - math.sin(.37) * y,
                               math.sin(.37) * x + math.cos(.37) * y))
    triangles = []
    for j in range(n):
        for i in range(n):
            a = i + (n + 1) * j
            b, c, d = a + 1, a + n + 2, a + n + 1
            triangles += [(a, b, c), (a, c, d)] if (i + j) % 2 else [(a, b, d), (b, c, d)]
    for a, b, c in triangles:
        xa, ya = points[a]; xb, yb = points[b]; xc, yc = points[c]
        if (xb - xa) * (yc - ya) - (yb - ya) * (xc - xa) <= 0:
            raise ValueError("Invalid triangle in the manufactured mesh")
    lines = ["NDIME= 2", "NELEM= {}".format(len(triangles))]
    lines += ["5 {} {} {} {}".format(*t, k) for k, t in enumerate(triangles)]
    lines += ["NPOIN= {}".format(len(points))]
    lines += ["{:.17g} {:.17g} {}".format(x, y, k) for k, (x, y) in enumerate(points)]
    boundaries = {
        "lower": [(i, i + 1) for i in range(n)],
        "right": [(n + j * (n + 1), n + (j + 1) * (n + 1)) for j in range(n)],
        "upper": [(n * (n + 1) + i + 1, n * (n + 1) + i) for i in range(n)],
        "left": [((j + 1) * (n + 1), j * (n + 1)) for j in range(n)],
    }
    lines += ["NMARK= 4"]
    for tag, edges in boundaries.items():
        lines += ["MARKER_TAG= " + tag, "MARKER_ELEMS= {}".format(len(edges))]
        lines += ["3 {} {}".format(a, b) for a, b in edges]
    return "\n".join(lines) + "\n"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("baseline", type=Path)
    parser.add_argument("candidate", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    binaries = {label: path.resolve(strict=True) for label, path in
                [("baseline", args.baseline), ("candidate", args.candidate)]}
    if not all(path.is_file() and os.access(path, os.X_OK) for path in binaries.values()):
        parser.error("Baseline and candidate must be executable files")
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    root = Path(__file__).resolve().parents[2]
    template = (root / "TestCases/mms/fvm_navierstokes/lam_mms_roe.cfg").read_text()
    environment = dict(os.environ, OMP_NUM_THREADS="1", OPENBLAS_NUM_THREADS="1")
    results = []
    for shape, aspect, curved in [("regular", 1.0, False), ("thin", 1e4, False), ("curved", 1e4, True)]:
        for n in [12, 24]:
            for label, binary in binaries.items():
                case = output / "{}_{}_{}".format(shape, n, label)
                case.mkdir()
                (case / "mesh.su2").write_text(mesh_text(n, aspect, curved))
                config = template
                replacements = {"MESH_FILENAME": "mesh.su2", "NUM_METHOD_GRAD": "WEIGHTED_LEAST_SQUARES",
                                "ITER": "300", "CFL_NUMBER": "5.0" if aspect == 1 else "0.5",
                                "LINEAR_SOLVER_ERROR": "1E-9", "CONV_RESIDUAL_MINVAL": "-11",
                                "OUTPUT_WRT_FREQ": "1000000"}
                for key, value in replacements.items():
                    config = re.sub(r"^" + key + r"\s*=.*$", key + "= " + value, config, flags=re.MULTILINE)
                config += "\nMGLEVEL= 0\nNUM_METHOD_GRAD_RECON= LEAST_SQUARES\nOUTPUT_FILES= (RESTART_ASCII)\n"
                (case / "case.cfg").write_text(config)
                with (case / "run.log").open("w") as log:
                    process = subprocess.run(["nice", "-n", "10", str(binary), "case.cfg"], cwd=case,
                                             env=environment, stdout=log, stderr=subprocess.STDOUT, timeout=120)
                errors = {name: float(value) for name, value in re.findall(
                    r"RMS Error\s*\[([^]]+)\]:\s*([\deE.+-]+)", (case / "run.log").read_text())}
                history = list(csv.DictReader((case / "history.csv").open())) if (case / "history.csv").exists() else []
                row = dict(shape=shape, n=n, aspect=aspect, label=label, exit_code=process.returncode,
                           rms_error=errors, last_history=history[-1] if history else {})
                results.append(row)
                (output / "results.json").write_text(json.dumps(results, indent=2) + "\n")
                print(shape, n, label, "exit=", process.returncode, "RMS=", errors, flush=True)
                if (process.returncode or set(errors) != {"Rho", "RhoU", "RhoV", "RhoE"} or
                        not all(math.isfinite(v) for v in errors.values())):
                    raise RuntimeError("CFD run failed; inspect " + str(case / "run.log"))


if __name__ == "__main__":
    main()
