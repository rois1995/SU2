#!/usr/bin/env python3
"""Compare every compact restart and the textual residual/force history from P0."""

import argparse
import csv
import filecmp
import re
from pathlib import Path


def history(path):
    with path.open(newline="") as file:
        records = csv.reader(file, skipinitialspace=True)
        headers = [value.strip().strip('"') for value in next(records)]
        columns = [i for i, key in enumerate(headers)
                   if key == "Inner_Iter" or key.startswith(("rms[", "max[", "rel[", "bgs[", "res[")) or
                   key in {"CL", "CD", "CMx", "CMy", "CMz", "CFx", "CFy", "CFz"}]
        if "rms[Rho]" not in headers or "Inner_Iter" not in headers:
            raise ValueError(f"{path}: missing density residual or iteration")
        rows = [tuple(row[i].strip() for i in columns) for row in records]
    if len(rows) != 50:
        raise ValueError(f"{path}: expected 50 history rows, got {len(rows)}")
    return tuple(headers[i] for i in columns), rows


def compare(a, b):
    def numbered(directory):
        return {p.name for p in directory.iterdir() if re.fullmatch(r"es4_restart_\d{6}\.dat", p.name)}
    names = numbered(a)
    if names != numbered(b) or len(names) != 50:
        raise ValueError(f"{a}, {b}: expected the same 50 numbered restart files")
    for name in sorted(names):
        if not filecmp.cmp(a / name, b / name, shallow=False):
            raise ValueError(f"{a}, {b}: {name} differs")
    if history(a / "es4_history.csv") != history(b / "es4_history.csv"):
        raise ValueError(f"{a}, {b}: history differs")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("root", type=Path, help="cases_es4_bitwise_r1 or cases_es4_bitwise_r2")
    args = parser.parse_args()
    cases = sorted(args.root.glob("*/b0_*"))
    if len(cases) != 6:
        parser.error(f"expected all six B0 cases, found {len(cases)}")
    for a in cases:
        base = a.name[3:]
        compare(a, a.with_name(f"b1_{base}"))
        compare(a.with_name(f"b2_{base}"), a.with_name(f"b3_{base}"))
        print(f"{a.parent.name}/{base}: B0/B1 and kappa4=0 off/on match for 50 iterations")


if __name__ == "__main__":
    main()
