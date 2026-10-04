#!/usr/bin/env python3
"""Small custom-sensor regressions; run with --binary, --baseline and --output paths."""

import argparse
import json
import math
import os
from pathlib import Path
import re
import shutil
import struct
import subprocess
import time

CASES = Path(__file__).resolve().parent
REPO = CASES.parents[2]


def wait_load():
    while os.getloadavg()[0] > 7:
        print(f"Load {os.getloadavg()[0]:.2f} > 7; waiting", flush=True)
        time.sleep(15)


def options(text, changes):
    lines = []
    for line in text.splitlines():
        key = line.split("=", 1)[0].strip()
        if key in changes:
            value = changes.pop(key)
            if value is not None:
                lines.append(f"{key}= {value}")
        else:
            lines.append(line)
    lines += [f"{key}= {value}" for key, value in changes.items() if value is not None]
    return "\n".join(lines) + "\n"


def mesh(path, n=24, vortex=False):
    """A small rectangular triangle mesh, with no external mesher or dataset."""
    nx, ny = n, n if vortex else n // 2
    length, height, offset = (10, 10, -5) if vortex else (2, 1, 0)
    point = lambda i, j: i + (nx + 1) * j
    with path.open("w") as stream:
        stream.write(f"NDIME= 2\nNELEM= {2*nx*ny}\n")
        elem = 0
        for j in range(ny):
            for i in range(nx):
                a, b, c, d = point(i, j), point(i+1, j), point(i+1, j+1), point(i, j+1)
                stream.write(f"5 {a} {b} {c} {elem}\n5 {a} {c} {d} {elem+1}\n")
                elem += 2
        stream.write(f"NPOIN= {(nx+1)*(ny+1)}\n")
        for j in range(ny+1):
            for i in range(nx+1):
                stream.write(f"{offset+length*i/nx:.16g} {offset+height*j/ny:.16g} {point(i,j)}\n")
        markers = {
            "lower": [(point(i+1, 0), point(i, 0)) for i in range(nx)],
            "right": [(point(nx, j+1), point(nx, j)) for j in range(ny)],
            "upper": [(point(i, ny), point(i+1, ny)) for i in range(nx)],
            "left": [(point(0, j), point(0, j+1)) for j in range(ny)],
        }
        stream.write("NMARK= 4\n")
        for name, edges in markers.items():
            stream.write(f"MARKER_TAG= {name}\nMARKER_ELEMS= {len(edges)}\n")
            for a, b in edges:
                stream.write(f"3 {a} {b}\n")


def point_fields(path):
    """Read the uncompressed raw-appended PARAVIEW fields written by SU2."""
    data = path.read_bytes()
    end = data.index(b"<AppendedData")
    start = data.index(b"_", end) + 1
    header = data[:end].decode()
    fields = {}
    section = header.split("<PointData>", 1)[1].split("</PointData>", 1)[0]
    for tag in re.findall(r"<DataArray[^>]+/>", section):
        name = re.search(r'Name="([^"]*)"', tag)[1]
        offset = int(re.search(r'offset="([0-9]+)"', tag)[1])
        count = struct.unpack_from("<Q", data, start+offset)[0]
        dtype = re.search(r'type="([^"]*)"', tag)[1]
        code = {"Float32": "f", "Float64": "d", "Int32": "i", "UInt64": "Q"}[dtype]
        itemsize = struct.calcsize(code)
        fields[name] = struct.unpack_from(f"<{count//itemsize}{code}", data, start+offset+8)
    return fields


def run(binary, directory, config, ranks=1, expect_error=None):
    wait_load()
    directory.mkdir(parents=True, exist_ok=True)
    (directory / "run.cfg").write_text(config)
    command = [str(binary), "run.cfg"]
    if ranks > 1:
        # Four ranks share two CPUs: validation never uses more than the allotted CPU budget.
        command = ["mpirun", "--oversubscribe", "--bind-to", "none", "-n", str(ranks)] + command
    with (directory / "run.log").open("w") as log:
        result = subprocess.run(command, cwd=directory, stdout=log, stderr=subprocess.STDOUT,
                                env={**os.environ, "OMP_NUM_THREADS": "1", "OPENBLAS_NUM_THREADS": "1"})
    text = (directory / "run.log").read_text(errors="replace")
    if expect_error:
        # The serial CBaseMPI::Abort implementation exits with zero; require the actual diagnostic.
        assert expect_error in text, (directory, result.returncode, text[-2500:])
        assert "Exit Success (SU2_CFD)" not in text, (directory, "Expected configuration rejection")
    else:
        assert result.returncode == 0, (directory, result.returncode, text[-2500:])
    return result.returncode


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--binary", required=True, type=Path)
    parser.add_argument("--baseline", type=Path)
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--cases", nargs="*")
    parser.add_argument("--ranks", type=int, default=1)
    args = parser.parse_args()
    binary = args.binary.resolve()
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    if hasattr(os, "sched_getaffinity"):
        os.sched_setaffinity(0, sorted(os.sched_getaffinity(0))[:2])
    results = []
    selected = args.cases or ["euler_no_muscl", "euler_jst", "ns_plate", "rans_sa", "rans_sst",
                              "window_average", "fixed_point", "predict", "window_average_nonlinear",
                              "fixed_point_nonlinear", "predict_nonlinear", "naca_legacy"]
    for name in selected:
        config = (CASES / f"{name}.cfg").read_text()
        directory = output / name
        directory.mkdir(exist_ok=True)
        if name == "naca_legacy":
            shutil.copyfile(REPO / "QuickStart/mesh_NACA0012_inv.su2", directory / "mesh.su2")
        else:
            mesh(directory / "mesh.su2", vortex=name.split("_nonlinear")[0] in ("window_average", "fixed_point", "predict"))
        start = time.monotonic()
        run(binary, directory, config, args.ranks)
        record = {"case": name, "ranks": args.ranks, "exit": 0}
        vtk_files = sorted(directory.glob("flow*.vtu"))
        assert vtk_files, (name, "Missing PARAVIEW output")
        fields = point_fields(vtk_files[-1])
        if name != "naca_legacy":
            sensors = {key for key in fields if key.startswith("SENSOR_")}
            expected = {"SENSOR_T", "SENSOR_S"} if name.startswith("rans_") else {"SENSOR_S"}
            assert sensors == expected, (name, sensors, expected)
            assert not any(key.startswith("Hessian_") and key.split("_")[1] not in {"S", "T"} for key in fields)
            for key, values in fields.items():
                assert all(math.isfinite(value) for value in values), (name, key)
            record["output_sensors"] = sorted(sensors)
        if name == "naca_legacy" and args.baseline:
            reference = output / "naca_baseline"
            reference.mkdir(exist_ok=True)
            shutil.copyfile(directory / "mesh.su2", reference / "mesh.su2")
            run(args.baseline.resolve(), reference, config, args.ranks)
            files = sorted(path for path in reference.iterdir() if path.suffix in (".dat", ".su2", ".vtu"))
            assert len(files) > 3
            for path in files:
                assert path.read_bytes() == (directory / path.name).read_bytes(), ("Bitwise mismatch", path.name)
            record["bitwise_files"] = [path.name for path in files]
        if name.split("_nonlinear")[0] in ("window_average", "fixed_point", "predict"):
            if name.endswith("_nonlinear"):
                error = max(abs(v-p*p)/max(1.0, p*p) for v, p in zip(fields["SENSOR_S"], fields["Pressure"]))
                assert error < 3e-6, ("Nonlinear instantaneous sample mismatch", name, error)
                record["nonlinear_value_max_rel_error"] = error
            else:
                control = output / f"{name}_control"
                control.mkdir(exist_ok=True)
                shutil.copyfile(directory / "mesh.su2", control / "mesh.su2")
                control_config = options(config, {"ADAP_CUSTOM_SENSORS": None, "ADAP_SENSOR": "(PRESSURE)",
                                                 "VOLUME_OUTPUT": "(COORDINATES, SOLUTION)"})
                run(binary, control, control_config, args.ranks)
                files = sorted(directory.glob("restart_flow*.dat"))
                assert files
                for path in files:
                    assert path.read_bytes() == (control / path.name).read_bytes(), ("History changed", name, path.name)
                record["unchanged_history_files"] = len(files)
            # Restart after the first replacement. Rebinding must use the replacement mesh/solvers.
            restart = output / f"{name}_restart"
            restart.mkdir(exist_ok=True)
            mesh_file = directory / "mesh_out_00002.su2"
            assert mesh_file.exists(), (name, "Missing mesh replacement")
            shutil.copyfile(mesh_file, restart / "mesh.su2")
            for step in (0, 1):
                for path in directory.glob(f"restart_flow_{step:05d}.*"):
                    shutil.copyfile(path, restart / path.name)
            restart_config = options(config, {"RESTART_SOL": "YES", "RESTART_ITER": "2",
                                               "SOLUTION_FILENAME": "restart_flow"})
            run(binary, restart, restart_config, args.ranks)
            continuation = sorted(restart.glob("restart_flow_0000[2-5].dat"))
            assert len(continuation) == 4, (name, "Missing restart continuation outputs")
            for path in continuation:
                assert path.read_bytes() == (directory / path.name).read_bytes(), ("Restart mismatch", name, path.name)
            record["restart"] = "bitwise continuation"
            record["restart_files"] = len(continuation)
        record["seconds"] = round(time.monotonic()-start, 3)
        results.append(record)
        (output / "results.json").write_text(json.dumps(results, indent=2)+"\n")
        print(json.dumps(record), flush=True)
    if "euler_no_muscl" in selected:
        base = (CASES / "euler_no_muscl.cfg").read_text()
        definitions = "; ".join(f"A{i} : {i}" for i in range(21))
        names = ", ".join(f"A{i}" for i in range(21))
        rejection = output / "reject_21"
        rejection.mkdir(exist_ok=True)
        mesh(rejection / "mesh.su2")
        negative = options(base, {"ADAP_CUSTOM_SENSORS": f"'{definitions}'", "ADAP_SENSOR": f"({names})"})
        code = run(binary, rejection, negative, args.ranks, "At most 20")
        results.append({"case": "reject_21", "ranks": args.ranks, "exit": code, "expected_rejection": True})
        (output / "results.json").write_text(json.dumps(results, indent=2)+"\n")
    print(f"PASS: {len(results)} cases", flush=True)


if __name__ == "__main__":
    main()
