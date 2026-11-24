#!/usr/bin/env python3
"""Compare SU2_GA_DUMP prefixes across MPI partitions for gate GA."""

import argparse
import glob
import math
import os
from pathlib import Path
import re
import struct
import sys

import numpy as np


HEADER = struct.Struct("=8s5Q")
BLOCK_TOL = 1e-12
FLOOR_FACTOR = 1e-14
CHUNK_SIZE = 16_384


def rank_files(prefix, kind):
    folder, name = os.path.split(prefix)
    pattern = f"{glob.escape(name)}.{kind}.rank*.bin"
    return sorted(
        path for path in Path(folder).glob(pattern)
        if re.fullmatch(r"[0-9]+", path.name.rsplit(".rank", 1)[1][:-4])
    )


def read_rank(path):
    with path.open("rb") as stream:
        header = stream.read(HEADER.size)
        if len(header) != HEADER.size:
            raise ValueError(f"{path}: truncated header")
        magic, version, nvar, npoint, nblock, scalar_size = HEADER.unpack(header)
        if magic != b"SU2GADMP" or version != 1 or not nvar or not scalar_size:
            raise ValueError(f"{path}: unsupported dump header")

        size = HEADER.size + 8 * (npoint * (1 + nvar) + nblock * (2 + nvar**2))
        if path.stat().st_size != size:
            raise ValueError(f"{path}: file size does not match header (expected {size} bytes)")

        residual_type = np.dtype([("row", "=u8"), ("values", "=f8", (nvar,))])
        block_type = np.dtype([("row", "=u8"), ("col", "=u8"), ("values", "=f8", (nvar**2,))])
        residual = np.fromfile(stream, dtype=residual_type, count=npoint)
        blocks = np.fromfile(stream, dtype=block_type, count=nblock)
        if len(residual) != npoint or len(blocks) != nblock:
            raise ValueError(f"{path}: truncated records")
        return (nvar, scalar_size), residual, blocks


def merge_records(parts, keys, label):
    records = parts[0] if len(parts) == 1 else np.concatenate(parts)
    records.sort(order=list(keys))
    ids = records[list(keys)]
    duplicates = ids[1:] == ids[:-1]
    if duplicates.any():
        index = int(duplicates.argmax())
        raise ValueError(f"{label}: duplicate global key {key_at(records, keys, index)}")
    return records


def read_set(paths):
    metadata, residual_parts, block_parts = zip(*(read_rank(path) for path in paths))
    if len(set(metadata)) != 1:
        raise ValueError(f"{paths[0]}: inconsistent nVar or matrix scalar size across ranks")
    residual = merge_records(residual_parts, ("row",), f"{paths[0]} residual")
    blocks = merge_records(block_parts, ("row", "col"), f"{paths[0]} Jacobian")
    diagonal_rows = blocks["row"][blocks["row"] == blocks["col"]]
    if not np.array_equal(diagonal_rows, residual["row"]):
        raise ValueError(f"{paths[0]}: Jacobian diagonal does not match owned residual rows")
    return metadata[0][0], residual, blocks


def key_at(records, keys, index):
    return tuple(int(records[key][index]) for key in keys)


def first_key_difference(ref, new, keys):
    shared = min(len(ref), len(new))
    differs = ref[:shared][list(keys)] != new[:shared][list(keys)]
    index = int(differs.argmax()) if differs.any() else shared
    if index == len(ref) == len(new):
        return None
    ref_key = key_at(ref, keys, index) if index < len(ref) else None
    new_key = key_at(new, keys, index) if index < len(new) else None
    if new_key is None or (ref_key is not None and ref_key < new_key):
        return f"global key {ref_key} missing from new"
    return f"global key {new_key} missing from ref"


def row_limits(ref, tol):
    values = ref["values"]
    norms = np.empty(len(ref))
    for start in range(0, len(ref), CHUNK_SIZE):
        span = slice(start, start + CHUNK_SIZE)
        norms[span] = np.abs(values[span]).max(axis=1)
    floor = FLOOR_FACTOR * norms.max(initial=0.0)
    if len(ref) and "col" in ref.dtype.names:
        # Sorted blocks are contiguous by owned row, including the diagonal and halo columns.
        rows = ref["row"]
        starts = np.r_[0, np.flatnonzero(rows[1:] != rows[:-1]) + 1]
        norms = np.repeat(np.maximum.reduceat(norms, starts), np.diff(starts, append=len(ref)))
    return tol * np.maximum(norms, floor)


def compare_records(label, ref, new, keys, nvar, tol):
    key_difference = first_key_difference(ref, new, keys)
    if key_difference:
        print(f"  {label}: FAIL, bitwise-equal no, numerical differences undefined")
        print(f"    first difference: {key_difference}")
        return False

    limits = row_limits(ref, tol) if tol else None
    max_abs = max_rel = 0.0
    first = None
    passed = True
    with np.errstate(over="ignore", under="ignore", invalid="ignore", divide="ignore"):
        for start in range(0, len(ref), CHUNK_SIZE):
            span = slice(start, start + CHUNK_SIZE)
            a, b = ref["values"][span], new["values"][span]
            changed = a.view("=u8") != b.view("=u8")
            if first is None and changed.any():
                record, component = np.unravel_index(changed.argmax(), changed.shape)
                key = key_at(ref, keys, start + record)
                entry = divmod(int(component), nvar) if len(keys) == 2 else int(component)
                first = f"global key {key}, component {entry}, ref={a[record, component]:.17g}, new={b[record, component]:.17g}"

            finite = np.isfinite(a) & np.isfinite(b)
            delta = np.where(finite, np.abs(b - a), np.inf)
            relative = np.full_like(delta, np.inf)
            np.divide(delta, np.abs(a), out=relative, where=finite & (a != 0))
            relative[delta == 0] = 0.0
            max_abs = max(max_abs, delta.max())
            max_rel = max(max_rel, relative.max())
            within_limit = (delta.max(axis=1) <= limits[span]).all() if tol else not changed.any()
            passed &= bool(finite.all() and within_limit)

    gate = f"tol={tol:g}" if tol else "exact"
    status = "PASS" if passed else "FAIL"
    bitwise = "no" if first else "yes"
    print(f"  {label}: {status} ({gate}), bitwise-equal {bitwise}, max abs {max_abs:.17g}, max rel {max_rel:.17g}")
    print(f"    first difference: {first or 'none'}")
    return passed


def compare(ref_prefix, new_prefix, exact, res_tol):
    passed = True
    found = False
    for kind in ("flow", "turb"):
        ref_paths, new_paths = rank_files(ref_prefix, kind), rank_files(new_prefix, kind)
        if not ref_paths and not new_paths:
            continue
        found = True
        print(f"{kind} ({len(ref_paths)} ref ranks, {len(new_paths)} new ranks)")
        if not ref_paths or not new_paths:
            print("  FAIL, bitwise-equal no, dump set missing")
            passed = False
            continue

        nvar, ref_residual, ref_blocks = read_set(ref_paths)
        new_nvar, new_residual, new_blocks = read_set(new_paths)
        if nvar != new_nvar:
            print(f"  FAIL, bitwise-equal no, nVar differs (ref={nvar}, new={new_nvar})")
            passed = False
            continue
        passed &= compare_records("residual", ref_residual, new_residual, ("row",), nvar, res_tol)
        passed &= compare_records("Jacobian", ref_blocks, new_blocks, ("row", "col"), nvar, 0.0 if exact else BLOCK_TOL)
    if not found:
        print("FAIL, no matching flow or turbulence dumps", file=sys.stderr)
    return passed and found


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("ref", help="reference SU2_GA_DUMP prefix")
    parser.add_argument("new", help="candidate SU2_GA_DUMP prefix")
    mode = parser.add_mutually_exclusive_group()
    mode.add_argument("--exact", action="store_true", help="require bitwise equality for both fields")
    mode.add_argument("--res-tol", type=float, default=0.0, help="residual row tolerance (default: bitwise equality)")
    args = parser.parse_args()
    if not math.isfinite(args.res_tol) or args.res_tol < 0:
        parser.error("--res-tol must be finite and nonnegative")
    try:
        return 0 if compare(args.ref, args.new, args.exact, args.res_tol) else 1
    except (OSError, ValueError, OverflowError) as error:
        print(f"FAIL, {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
