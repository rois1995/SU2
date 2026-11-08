#!/usr/bin/env python3
"""Prepare paper configurations and a dependency manifest. Never run a solver."""

import argparse
import hashlib
import json
import math
from pathlib import Path
import re
import shutil
import sys

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
from check_extraction import read_config

HERE = Path(__file__).resolve().parent
CASES = {
    "naca0012": (2, {"airfoil", "farfield"}),
    "nasa_hump": (2, {"wall", "upper", "inlet", "outlet"}),
    "onera_m6": (3, {"WING", "FARFIELD", "SYMMETRY"}),
}
MARKER_ALIASES = {
    "naca0012": {"airfoil": "Wall", "farfield": "Farfield"},
    "onera_m6": {"WING": "WING3D"},
}
MODES = {"1C_KEEP": (1, "NO"), "1C_SWAP": (1, "YES"),
         "2C_KEEP": (2, "NO"), "2C_SWAP": (2, "YES"), "3C": (3, "NO")}


def mesh_metadata(path, case):
    """Check the mesh contract, not geometric quality or physical resolution."""
    headers, markers = {}, []
    with path.open() as stream:
        for line in stream:
            line = line.split("%", 1)[0]
            if "=" not in line:
                continue
            key, value = (s.strip() for s in line.split("=", 1))
            if key == "MARKER_TAG":
                markers.append(value)
            elif key in {"NDIME", "NELEM", "NPOIN", "NMARK"}:
                if key in headers:
                    raise ValueError(f"Duplicate mesh header {key}; require a single-zone ASCII SU2 mesh")
                headers[key] = int(value.split()[0])
    dimension, expected = CASES[case]
    mapping = {}
    aliases = MARKER_ALIASES.get(case, {})
    if set(markers) == {aliases.get(marker, marker) for marker in expected}:
        mapping = aliases
        expected = {aliases.get(marker, marker) for marker in expected}
    if headers.get("NDIME") != dimension or set(markers) != expected:
        raise ValueError(f"{case} needs dimension {dimension} and markers {sorted(expected)}")
    if len(markers) != len(expected) or headers.get("NMARK") != len(expected):
        raise ValueError("Missing or duplicate boundary markers")
    if min(headers.get("NELEM", 0), headers.get("NPOIN", 0)) <= 0:
        raise ValueError("Empty mesh")
    return {"source": str(path.resolve()), "sha256": hashlib.sha256(path.read_bytes()).hexdigest(),
            "dimension": dimension, "points": headers["NPOIN"], "elements": headers["NELEM"],
            "markers": sorted(markers), "marker_mapping": mapping,
            "geometry_and_resolution_verified": False}


def prepare(case, output, mesh=None):
    metadata = mesh_metadata(mesh, case) if mesh else None
    config = read_config(HERE / "common.cfg")
    config.update(read_config(HERE / f"{case}.cfg"))
    if metadata:
        for key, value in config.items():
            if key.startswith("MARKER_"):
                config[key] = re.sub(r"[A-Za-z_][A-Za-z_0-9]*",
                                     lambda match: metadata["marker_mapping"].get(match[0], match[0]), value)
    dimension = CASES[case][0]
    if case == "nasa_hump":
        # Match SU2's Reynolds initialization before applying NASA pressure ratios.
        temperature = float(config["FREESTREAM_TEMPERATURE"])
        gas = float(config["GAS_CONSTANT"])
        mu = float(config["MU_REF"]) * (temperature / float(config["MU_T_REF"]))**1.5
        mu *= (float(config["MU_T_REF"]) + 110.4) / (temperature + 110.4)
        speed = float(config["MACH_NUMBER"]) * math.sqrt(1.4 * gas * temperature)
        density = float(config["REYNOLDS_NUMBER"]) * mu / (speed * float(config["REYNOLDS_LENGTH"]))
        pressure = density * gas * temperature
        config["MARKER_INLET"] = f"(inlet, {1.002*temperature:.16g}, {1.007*pressure:.16g}, 1, 0, 0)"
        config["MARKER_OUTLET"] = f"(outlet, {0.99962*pressure:.16g})"

    output.mkdir(parents=True, exist_ok=False)
    (output / "mesh").mkdir()
    if mesh:
        shutil.copyfile(mesh, output / "mesh" / "mesh.su2")
    jobs = []

    def write_job(name, overrides, adjoint=False):
        directory = output / name
        directory.mkdir(parents=True)
        prefix = "../" * len(Path(name).parts)
        options = dict(config, MESH_FILENAME=prefix + "mesh/mesh.su2")
        fields = ["DENSITY", "MOMENTUM-X", "MOMENTUM-Y"]
        if dimension == 3:
            fields.append("MOMENTUM-Z")
        fields += ["ENERGY", "TKE", "DISSIPATION"]
        tag = "RMS_ADJ_" if adjoint else "RMS_"
        options["CONV_FIELD"] = "(" + ", ".join(tag + field for field in fields) + ")"
        screen = f"INNER_ITER, {tag}DENSITY, {tag}TKE, {tag}DISSIPATION"
        options["SCREEN_OUTPUT"] = "(" + screen + ("" if adjoint else ", DRAG, LIFT") + ")"
        if adjoint:
            options["HISTORY_OUTPUT"] = "(ITER, RMS_RES)"
            options["VOLUME_OUTPUT"] = "(COORDINATES, SOLUTION, SENSITIVITY)"
        if name != "baseline":
            options["SOLUTION_FILENAME"] = prefix + "baseline/restart_flow"
        options.update(overrides)
        path = directory / "run.cfg"
        path.write_text("% Generated by paper_cases/prepare.py; run from this directory.\n" +
                        "".join(f"{key}= {value}\n" for key, value in options.items()))
        jobs.append({"directory": name, "config": name + "/run.cfg",
                     "config_sha256": hashlib.sha256(path.read_bytes()).hexdigest(),
                     "requires": [] if name == "baseline" else ["baseline:converged_restart"],
                     "status": "not_run", "objective": options["OBJECTIVE_FUNCTION"],
                     "epm_mode": name.split("/")[-1] if name.startswith("nonlinear/") else None})

    write_job("baseline", {})
    for objective in ("DRAG", "LIFT"):
        write_job("adjoint_" + objective.lower(), {
            "MATH_PROBLEM": "DISCRETE_ADJOINT", "OBJECTIVE_FUNCTION": objective,
            "SST_EPM_POSTPROCESS": "YES", "OUTPUT_FILES": "(RESTART)",
        }, adjoint=True)
    for mode, (component, permute) in MODES.items():
        write_job("nonlinear/" + mode, {"SST_OPTIONS": "(V2003m, UQ)", "RESTART_SOL": "YES",
                                      "UQ_COMPONENT": str(component), "UQ_PERMUTE": permute})
    manifest = {
        "case": case, "status": "prepared_unrun" if mesh else "mesh_required",
        "mesh": metadata, "jobs": jobs,
        "templates_sha256": {name: hashlib.sha256((HERE / name).read_bytes()).hexdigest()
                             for name in ("common.cfg", f"{case}.cfg", "prepare.py")},
        "nonlinear_amplitudes_available": [1.0],
        "nonlinear_taylor_test": "blocked: continuation interface not implemented",
        "acceptance_gates": ["mesh geometry/scale, refinement and wall y+",
                             "all primal and coupled-adjoint residuals <= 1e-10",
                             "objective drift and tighter-solve numerical noise",
                             "finite sensitivities and recorded eigenspace ambiguity",
                             "nonlinear Taylor verification before derivative claims",
                             "MPI runtime verification before parallel campaign"],
    }
    (output / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
    return manifest


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("case", choices=CASES)
    parser.add_argument("output", type=Path, help="new campaign directory")
    parser.add_argument("--mesh", type=Path, help="ASCII SU2 mesh, copied and fingerprinted; otherwise leave a blocked setup")
    args = parser.parse_args()
    try:
        manifest = prepare(args.case, args.output, args.mesh)
    except (OSError, ValueError) as error:
        parser.exit(1, f"Preparation failed: {error}\n")
    print(f"Prepared {len(manifest['jobs'])} jobs: {manifest['status']}. No solver was launched.")


if __name__ == "__main__":
    main()
