"""Check saved outputs from the steady JST entropy-budget bitwise regression.

Use gen_ent.py --bitwise and keep each run in a separate directory. Save the
feature runs' output as solver.log with SU2_ENTROPY_BUDGET_HASH=1. Use the same
mesh, assembly strategy, rank count and thread count in every run. Use the
optional volume run with diagnostic fields and WRT_RESTART_COMPACT=NO.
"""

import argparse
import csv
import filecmp
import mmap
import re
import struct
from pathlib import Path


HASH = re.compile(
    r"SU2_ENTROPY_BUDGET_HASH rank=(\d+) iter=(\d+) (spatial|rhs|solution)=([0-9a-f]+)"
    r"(?: jacobian=([0-9a-f]+))?"
)
AERO = {"CD", "CL", "CSF", "CMx", "CMy", "CMz", "CFx", "CFy", "CFz", "CEff", "RefForce", "Buffet"}
BASE_FIELDS = ("Density", "Momentum_x", "Momentum_y", "Energy")
RESTART_HEADER = struct.Struct("=5i")
RESTART_FIELD_SIZE = 33
RESTART_MAGIC = 535532


def hashes(path, ranks, iterations):
    result = {}
    with path.open() as log:
        for line in log:
            for rank, iteration, phase, rows, jac in HASH.findall(line):
                key = int(rank), int(iteration), phase
                if key in result or (phase != "solution" and not jac):
                    raise ValueError(f"{path}: malformed or repeated hash {key}")
                result[key] = rows, jac
    expected = {(rank, i, phase) for rank in range(ranks) for i in range(iterations)
                for phase in ("spatial", "rhs", "solution")}
    if result.keys() != expected:
        missing = sorted(expected - result.keys())[:3]
        extra = sorted(result.keys() - expected)[:3]
        raise ValueError(f"{path}: missing hashes {missing}, unexpected hashes {extra}")
    return result


def history(path, iterations):
    with path.open(newline="") as file:
        reader = csv.DictReader(file, skipinitialspace=True)
        fields = [field for field in reader.fieldnames or []
                  if field.startswith(("rms[", "max[", "bgs[")) or field in AERO]
        if not {"rms[Rho]", "CD", "CL"}.issubset(fields):
            raise ValueError(f"{path}: expected RMS_RES and AERO_COEFF columns")
        rows = [tuple(row[field].strip() for field in fields) for row in reader]
    if len(rows) != iterations:
        raise ValueError(f"{path}: expected {iterations} history rows, read {len(rows)}")
    return fields, rows


def restart_layout(data):
    magic, fields, points, width, _ = RESTART_HEADER.unpack_from(data)
    width = width or 8
    offset = RESTART_HEADER.size + fields * RESTART_FIELD_SIZE
    if (magic != RESTART_MAGIC or fields <= 0 or points <= 0 or width not in (4, 8)
            or len(data) < offset + fields * points * width):
        raise ValueError("invalid SU2 binary restart")
    names = []
    for start in range(RESTART_HEADER.size, offset, RESTART_FIELD_SIZE):
        names.append(data[start:start + RESTART_FIELD_SIZE].split(b"\0", 1)[0].decode())
    return names, points, width, offset


def compare_solution(compact_path, full_path):
    with compact_path.open("rb") as compact_file, full_path.open("rb") as full_file:
        with mmap.mmap(compact_file.fileno(), 0, access=mmap.ACCESS_READ) as compact:
            with mmap.mmap(full_file.fileno(), 0, access=mmap.ACCESS_READ) as full:
                names, npoints, width, offset = restart_layout(compact)
                full_names, full_points, full_width, full_offset = restart_layout(full)
                if (npoints, width) != (full_points, full_width) or "ENT_DISSIP" not in full_names:
                    raise ValueError(f"{full_path}: expected a matching restart with entropy fields")
                fields = BASE_FIELDS
                if "Momentum_z" in names:
                    fields += ("Momentum_z",)
                cols = [(name, names.index(name) * width, full_names.index(name) * width) for name in fields]
                for point in range(npoints):
                    for name, left, right in cols:
                        start, full_start = offset + left, full_offset + right
                        if compact[start:start + width] != full[full_start:full_start + width]:
                            raise ValueError(f"{full_path}: {name} differs at point {point}")
                    offset += len(names) * width
                    full_offset += len(full_names) * width


def compare(args):
    baseline, repeated, off, on = args.runs
    expected_history = history(baseline / "history.csv", args.iterations)
    for run in (repeated, off, on):
        fields, rows = history(run / "history.csv", args.iterations)
        if fields != expected_history[0]:
            raise ValueError(f"{run}: residual and aerodynamic history headers differ")
        for iteration, (actual, expected) in enumerate(zip(rows, expected_history[1])):
            if actual != expected:
                raise ValueError(f"{run}: residual or aerodynamic history differs at iteration {iteration}")

    for iteration in range(args.iterations):
        name = f"restart_flow_{iteration:06d}.dat"
        for run in (repeated, off, on):
            if not filecmp.cmp(baseline / name, run / name, shallow=False):
                raise ValueError(f"{run / name}: restart bytes differ")
    if args.volume:
        compare_solution(on / name, args.volume / name)

    off_hash = hashes(off / "solver.log", args.ranks, args.iterations)
    on_hash = hashes(on / "solver.log", args.ranks, args.iterations)
    for key, value in off_hash.items():
        if on_hash[key] != value:
            raise ValueError(f"feature hashes differ at rank/iteration/phase {key}: {value} != {on_hash[key]}")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("runs", type=Path, nargs=4, metavar="RUN",
                        help="baseline, repeated baseline, feature off, feature on (in that order)")
    parser.add_argument("--ranks", type=int, default=1)
    parser.add_argument("--iterations", type=int, default=50)
    parser.add_argument("--volume", type=Path)
    args = parser.parse_args()
    if args.ranks < 1 or args.iterations < 1:
        parser.error("ranks and iterations must be positive")
    try:
        compare(args)
    except (OSError, ValueError, struct.error) as error:
        parser.exit(1, f"{error}\n")
    print("Compact restart bytes, residual/aero history and feature hashes match."
          + (" Noncompact solution columns match." if args.volume else ""))


if __name__ == "__main__":
    main()
