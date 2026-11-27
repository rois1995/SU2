#!/usr/bin/env python3
"""Compare S3's real assembled residuals, independent x rows and coupling weights."""

import argparse
import csv
import math
from pathlib import Path


def records(directory, name, ranks, keys, duplicate=False):
    out = {}
    for rank in range(ranks):
        path = directory / f"es4_{name}_{ranks}_{rank}.csv"
        with path.open(newline="") as table:
            for row in csv.DictReader(table):
                key = tuple(int(row.pop(field)) for field in keys)
                values = {field: float(value) for field, value in row.items()}
                if not all(math.isfinite(value) for value in values.values()):
                    raise ValueError(f"{path}: nonfinite data at {key}")
                if key in out and (not duplicate or
                                   not math.isclose(out[key]["a"], values["a"], rel_tol=1e-12)):
                    raise ValueError(f"{path}: inconsistent duplicate {key}")
                out[key] = values
    if not out:
        raise ValueError(f"no records for {name} at {ranks} ranks")
    return out


def compare_rows(serial, parallel):
    if serial.keys() != parallel.keys():
        raise ValueError("serial and two-rank owned point IDs differ")
    sensor_max, second_max, error_max = 0.0, 0.0, 0.0
    for key, a in serial.items():
        b = parallel[key]
        sensor_max = max(sensor_max, abs(a["sensor"] - b["sensor"]))
        second_max = max(second_max, *(abs(a[f"eps2{v}"] - b[f"eps2{v}"]) for v in range(4)))
        for kind in ("r", "x"):
            for v in range(4):
                field = f"{kind}{v}"
                incident = max(a[f"s{field}"], b[f"s{field}"])
                diff = abs(a[field] - b[field])
                # Absolute roundoff for subtraction of the full centered residual.
                slack = 5e-15 if kind == "r" else 1e-15
                if diff > 1e-12 * incident + slack:
                    raise ValueError(f"point {key[0]} {field}: difference {diff:g}, incident scale {incident:g}")
                error_max = max(error_max, max(0.0, diff - slack) / incident if incident else 0.0)
    print(f"{len(serial)} points | maximum scaled discrepancy {error_max:.3e} | "
          f"legacy sensor max difference {sensor_max:.3e} | second difference max difference {second_max:.3e} (diagnostics)")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("serial", type=Path)
    parser.add_argument("parallel", type=Path)
    args = parser.parse_args()
    compare_rows(records(args.serial, "rows", 1, ("id",)), records(args.parallel, "rows", 2, ("id",)))
    keys = ("i", "k", "split")
    serial = records(args.serial, "weights", 1, keys)
    parallel = records(args.parallel, "weights", 2, keys, duplicate=True)
    if serial.keys() != parallel.keys():
        raise ValueError("serial and two-rank global coupling IDs differ")
    for key, row in serial.items():
        if not math.isclose(row["a"], parallel[key]["a"], rel_tol=1e-12):
            raise ValueError(f"coupling {key}: weights differ")
    print(f"{len(serial)} true/split coupling weights agree by global endpoint IDs")


if __name__ == "__main__":
    main()
