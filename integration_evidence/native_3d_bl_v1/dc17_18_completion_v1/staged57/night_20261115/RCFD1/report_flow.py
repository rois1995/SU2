#!/usr/bin/env python3
"""Print real CFD flow-history, restart, force and ParaView report sections."""
from argparse import ArgumentParser
import csv
import gzip
import math
import os
import re
from contextlib import contextmanager
from pathlib import Path
from statistics import fmean

HERE = Path(__file__).resolve().parent
SEED = HERE.parents[1] / "m6_inputs/euler_original_force_seed_v1/m6.su2"
MODES = ("S", "OFF", "SM", "SG")
RMS = ("rms[Rho]", "rms[RhoU]", "rms[RhoV]", "rms[RhoW]", "rms[RhoE]")
FINAL = (*RMS, "CL", "CD", "Avg CFL")
SUMMARY_ROW = re.compile(
    r"\|\s*([012])\s*\|[^|]+\|\s*(\d+)\s*\|\s*(\d+)\s*\|\s*([^|]+?)\s*\|\s*([^|]+?)\s*\|"
)


@contextmanager
def artifact_lines(path):
    if path.is_file():
        with path.open(errors="replace") as stream:
            yield stream
    elif (compressed := path.with_suffix(path.suffix + ".gz")).is_file():
        with gzip.open(compressed, "rt", errors="replace") as stream:
            yield stream
    else:
        yield ()


def history(path):
    with artifact_lines(path) as lines:
        rows = csv.DictReader(lines, skipinitialspace=True)
        return [{key.strip(): value.strip() for key, value in row.items() if key and value is not None}
                for row in rows]


def summary(case):
    rows = {}
    rank_logs = {path.parent / "stdout" for path in (case / "rank_output").glob("*/rank.0/stdout*")}
    path = next(iter(rank_logs)) if len(rank_logs) == 1 else case / "run.log"
    in_summary = False
    with artifact_lines(path) as lines:
        for line in lines:
            if "Mesh Adaptation Summary" in line:
                in_summary = True
            if in_summary and (match := SUMMARY_ROW.search(line)):
                cycle, points, iterations, solve_time, adapt_time = match.groups()
                rows[cycle] = (points, iterations, solve_time, adapt_time)
                if cycle == "2":
                    break
    return rows


def mesh_size(path):
    counts = {}
    if path.is_file():
        with path.open(errors="replace") as stream:
            for line in stream:
                key, _, value = line.partition("=")
                if key.strip() in ("NPOIN", "NELEM") and value.split():
                    counts[key.strip()] = value.split()[0]
                    if len(counts) == 2:
                        break
    return counts.get("NPOIN"), counts.get("NELEM")


def finite(row, key):
    try:
        number = float(row.get(key, "nan"))
    except ValueError:
        return None
    return number if math.isfinite(number) else None


def peak_and_drop(rows):
    residuals = [(row.get("Inner_Iter"), finite(row, RMS[0])) for row in rows]
    residuals = [(iteration, residual) for iteration, residual in residuals if residual is not None]
    if not residuals:
        return None, None
    peak_index = max(range(len(residuals)), key=lambda index: residuals[index][1])
    peak_iteration, peak = residuals[peak_index]
    tail = residuals[peak_index:]
    threshold_iter = next((iteration for iteration, residual in tail if peak - residual >= 3), None)
    peak_label = f"{peak:.10g} (iter {peak_iteration})"
    if threshold_iter is not None:
        return peak_label, f"iter {threshold_iter}"
    drop = peak - min(residual for _, residual in tail)
    return peak_label, f"not reached (drop {drop:.6g})"


def coefficient_range(rows, key):
    values = [finite(row, key) for row in rows]
    if not values or None in values:
        return None
    return f"{min(values):.10g} to {max(values):.10g}"


def mean_iteration_time(rows):
    values = [finite(row, "Time(sec)") for row in rows if row.get("Inner_Iter") != "0"]
    if not values or None in values:
        return None
    return f"{fmean(values):.9g}"


def table(headers, rows):
    def render(values):
        return "| " + " | ".join("—" if value is None or value == "" else str(value) for value in values) + " |"

    print(render(headers))
    print(render(["---"] * len(headers)))
    for row in rows:
        print(render(row))
    print()


def files_row(mode, stage, path):
    relative = os.path.relpath(path, HERE / "reports")
    if not path.is_file():
        return mode, stage, f"`{relative}`", "missing"
    return mode, stage, f"[`{path.name}`]({relative})", "present" if path.stat().st_size else "empty"


def print_report(g5_directory, modes=MODES):
    cycles, final_rows, restart_rows, force_rows, paraview_rows = [], [], [], [], []
    for mode in modes:
        name = f"R-{mode}"
        work = g5_directory / mode
        case = work / "full647_run1"
        restart = work / "disk_restart_full647_run1_event2"
        all_history = history(case / "history.csv")
        summary_rows = summary(case)
        restart_history = history(restart / "continued508_history.csv")
        restart_zero = next((row for row in restart_history if row.get("Inner_Iter") == "0"), {})

        for cycle in ("0", "1", "2"):
            rows = [row for row in all_history if row.get("Adap_Cycle") == cycle]
            last = rows[-1] if rows else {}
            mesh = case / ("m6.su2" if cycle == "0" else f"adapted_mesh_adap_{int(cycle):05d}.su2")
            summary_points, n_iter, solve_time, adapt_time = summary_rows.get(cycle, (None,) * 4)
            points, cells = mesh_size(mesh)
            points = points or summary_points
            peak, threshold = peak_and_drop(rows)
            cycles.append((name, cycle, points, cells, n_iter, len(rows), peak, threshold,
                           coefficient_range(rows[-100:], "CL"), coefficient_range(rows[-100:], "CD"),
                           mean_iteration_time(rows), solve_time, "none" if adapt_time == "-" else adapt_time))
            final_rows.append((name, cycle, last.get("Inner_Iter"), *(last.get(key) for key in FINAL)))
            paraview_rows.append(files_row(name, f"cycle {cycle}", case / f"flow_adap_{int(cycle):05d}.vtu"))

            if cycle == "2":
                restart_rows.extend((name, source, row.get("Inner_Iter"), row.get(RMS[0]), row.get("CL"), row.get("CD"))
                                    for source, row in (("Full run, final cycle", last), ("Isolated restart", restart_zero)))
                force_rows.append((name, points, cells, last.get("CL"), last.get("CD")))
        paraview_rows.append(files_row(name, "isolated restart", restart / "flow.vtu"))

    print("## Flow histories\n")
    print("The solver records the base-10 logarithm of each RMS residual. The three-order check starts at the largest "
          "finite residual and uses the first subsequent qualifying iteration. If the stage falls short, the table "
          "gives the largest observed drop in log orders. Ranges use up to the last 100 history rows. Mean iteration "
          "time excludes iteration 0. Solve includes metric generation. An em dash marks unavailable data.\n")
    table(("Run", "Cycle", "Points", "Cells", "Iterations (summary)", "History rows", "Peak rms[Rho]",
           "First 3-order drop", "CL range", "CD range", "Mean Time(sec)", "Solve [s]", "Adapt [s]"), cycles)
    print("### Last recorded history row\n")
    table(("Run", "Cycle", "Inner_Iter", *FINAL), final_rows)

    print("## Isolated restart\n")
    table(("Run", "Stage", "Inner_Iter", RMS[0], "CL", "CD"), restart_rows)
    print("## Aerodynamic coefficients and mesh size\n")
    table(("Run", "Points", "Cells", "CL", "CD"),
          [*sorted(force_rows, key=lambda row: row[0] != "R-OFF"),
           ("Original converged seed", *mesh_size(SEED), "0.269535", "0.0135359")])
    print("## ParaView artifacts\n")
    table(("Run", "Stage", "File", "Status"), paraview_rows)


if __name__ == "__main__":
    parser = ArgumentParser(description=__doc__)
    parser.add_argument("--modes", nargs="+", choices=MODES, default=MODES,
                        help="Select which runs to report, such as S SM SG for RCFD2.")
    print_report(HERE / "G5/build647", parser.parse_args().modes)
