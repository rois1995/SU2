#!/usr/bin/env python3
"""Reader of the SU2 SST-EPM field files.

SU2 (discrete adjoint, EPM extraction) writes, per objective <OBJ> (DRAG or LIFT):
  sst_epm_field_<OBJ>_rank<r>.bin   raw little-endian float64 records, one per site, len(columns) columns each
  sst_epm_field_<OBJ>.json          master header: column names, per-rank file names and record counts, case names, ...

This module loads the files, merges halo duplicates by the global key (kind, key_a, key_b), recomputes the five
classical gradients g_m = sum over sites of S_tau.dtau[m] + S_pk*dpk[m] + S_pw*dpw[m], and compares them with the
canonical (probe_pair == 3) rows of sst_epm_sensitivities.csv.

    python3 read_epm_field.py sst_epm_field_DRAG.json --csv sst_epm_sensitivities.csv

Only numpy and the standard library are used (Python >= 3.8, numpy >= 1.24).
"""

import argparse
import csv
import json
import math
import os
import sys

import numpy as np

KEY_COLUMNS = ("kind", "key_a", "key_b")
DEFAULT_CASE_NAMES = ("1C_KEEP", "1C_SWAP", "2C_KEEP", "2C_SWAP", "3C")
# Field consistency tolerance (design spec 9.3): 1e-10 relative, with an absolute floor scaled by the field's max norm
# (the largest |value| of the column). Shared with check_extraction.py.
FIELD_RELATIVE_TOLERANCE = 1e-10
CSV_REL_TOL = 1e-10
CSV_ABS_TOL = 1e-14
CANONICAL_PROBE_PAIR = "3"
_TINY = 1e-300


class Fields(dict):
    """dict column name -> 1D array (all the same length), plus the master header in `.meta`."""

    def __init__(self, columns=(), meta=None):
        dict.__init__(self, columns)
        self.meta = dict(meta) if meta else {}


def _like(fields, columns):
    return Fields(columns, getattr(fields, "meta", None))


def load(json_path):
    """Read the master JSON and all rank files; returns Fields with one array per column (all ranks concatenated,
    rank order, record order within the rank). Rows are not merged; empty ranks are fine."""
    json_path = os.fspath(json_path)
    with open(json_path) as stream:
        header = json.load(stream)
    columns = header["columns"]
    if header.get("dtype", "float64") != "float64":
        raise ValueError("Unsupported dtype %r (expected float64)." % (header.get("dtype"),))
    if len(set(columns)) != len(columns):
        raise ValueError("Duplicate column names in %s." % json_path)
    if header.get("record_bytes", 8 * len(columns)) != 8 * len(columns):
        raise ValueError("record_bytes (%s) does not match %d float64 columns." % (header.get("record_bytes"), len(columns)))
    directory = os.path.dirname(os.path.abspath(json_path))
    blocks = []
    for entry in header["ranks"]:
        path = os.path.join(directory, entry["file"])
        data = np.fromfile(path, dtype="<f8")  # JSON does not record the byte order; the writer is little-endian.
        if data.size != entry["count"] * len(columns):
            raise ValueError("%s holds %d values, expected %d records x %d columns."
                             % (path, data.size, entry["count"], len(columns)))
        blocks.append(data.reshape(-1, len(columns)))
    table = np.concatenate(blocks, axis=0) if blocks else np.zeros((0, len(columns)))
    meta = {key: value for key, value in header.items() if key != "columns"}
    return Fields(((name, np.ascontiguousarray(table[:, i])) for i, name in enumerate(columns)), meta)


def _dtau_names(fields):
    return [name for name in fields if name.startswith("dtau_")]


def scaled_close(a, b, scale, tolerance=FIELD_RELATIVE_TOLERANCE):
    """Elementwise |a-b| <= tolerance * max(|a|, |b|, scale), the field consistency test of the design (spec 9.3,
    tolerance 1e-10). `scale` is the field's max norm (broadcast, e.g. one value per column). Equal values
    (infinities included) and NaN pairs agree."""
    with np.errstate(invalid="ignore"):
        bound = tolerance * np.maximum(np.maximum(np.abs(a), np.abs(b)), np.maximum(scale, _TINY))
        close = np.abs(a - b) <= bound
    return close | (a == b) | (np.isnan(a) & np.isnan(b))


def column_scale(values, rows=None):
    """Max norm of each column of `values` (2D) over `rows` (boolean mask or None for all), ignoring non-finite."""
    values = values if rows is None else values[rows]
    finite = np.where(np.isfinite(values), np.abs(values), 0.0)
    return np.max(finite, axis=0, initial=0.0)


def merge(fields):
    """Merge rows with the same (kind, key_a, key_b), as produced by halo duplication across ranks.

    * S_* columns (S_tau_*, S_pk, S_pw) are summed over the copies: each copy carries its own share.
    * All other columns (state, dtau_*, dpk_*, dpw_*, uses) are kept from the first copy.
    * Rows with uses == 0 were never evaluated on their rank (their S and d columns are 0): they add nothing to the
      sums, they are excluded from the dtau consistency check, and they are never the 'first copy' when an evaluated
      copy exists. A site with no evaluated copy is kept as its first (unused) row.
    * Evaluated copies of a site agree only to round-off: the ranks orient a shared edge by their local point
      numbering, and SU2's gradient correction is not bitwise symmetric in (i, j). If an evaluated copy's dtau_*
      differs from the first copy's beyond scaled_close (1e-10 relative, floor 1e-10 times the column's max norm over
      evaluated rows), ValueError is raised.
    The output has one row per site, in order of first appearance; the input is not modified.
    """
    n = len(fields["kind"])
    names = list(fields)
    if n == 0:
        return _like(fields, ((name, np.array(fields[name], copy=True)) for name in names))
    keys = np.column_stack([fields[name] for name in KEY_COLUMNS])
    _, inverse = np.unique(keys, axis=0, return_inverse=True)
    inverse = np.asarray(inverse).reshape(-1)  # numpy 2.0.0 gave this a different shape for axis != None
    groups = int(inverse.max()) + 1
    rows = np.arange(n)
    used = fields["uses"] != 0 if "uses" in fields else np.ones(n, dtype=bool)

    first_any = np.full(groups, n)
    np.minimum.at(first_any, inverse, rows)
    first_used = np.full(groups, n)
    np.minimum.at(first_used, inverse[used], rows[used])
    representative = np.where(first_used < n, first_used, first_any)

    dtau = _dtau_names(fields)
    if dtau:
        mine = np.column_stack([fields[name] for name in dtau])
        reference = mine[representative[inverse]]
        bad = used[:, None] & ~scaled_close(mine, reference, column_scale(mine, used))
        if bad.any():
            row, col = np.argwhere(bad)[0]
            site = tuple(float(fields[name][row]) for name in KEY_COLUMNS)
            raise ValueError("Inconsistent halo copies of site (kind, key_a, key_b) = %r: %s is %.17g, first copy has "
                             "%.17g (difference above %g relative, with a floor scaled by the column max %.17g)."
                             % (site, dtau[col], mine[row, col], reference[row, col], FIELD_RELATIVE_TOLERANCE,
                                column_scale(mine, used)[col]))

    order = np.argsort(first_any, kind="stable")  # groups in order of first appearance
    merged = Fields((), getattr(fields, "meta", None))
    for name in names:
        if name.startswith("S_"):
            summed = np.bincount(inverse, weights=fields[name], minlength=groups)
            merged[name] = summed[order]
        else:
            merged[name] = np.array(fields[name][representative[order]], copy=True)
    return merged


def _case_names(fields, case_names):
    if case_names is not None:
        return list(case_names)
    meta_names = getattr(fields, "meta", {}).get("case_names")
    if meta_names:
        return list(meta_names)
    count = len([name for name in fields if name.startswith("dpk_")])
    return list(DEFAULT_CASE_NAMES) if count == len(DEFAULT_CASE_NAMES) else [str(m) for m in range(count)]


def classical_gradients(fields, case_names=None):
    """g_m = sum_sites sum_j S_tau_j*dtau_m_j + S_pk*dpk_m + S_pw*dpw_m, for each case m; dict case name -> float.

    Works on unmerged and merged rows alike (duplicate copies carry their own S share and the same d). Rows with
    uses == 0 contribute nothing (their S, d are 0; this also keeps unevaluated NaN placeholder rows out)."""
    names = _case_names(fields, case_names)
    n = len(fields["kind"])
    used = fields["uses"] != 0 if "uses" in fields else np.ones(n, dtype=bool)
    result = {}
    for m, name in enumerate(names):
        contribution = fields["S_pk"] * fields["dpk_%d" % m] + fields["S_pw"] * fields["dpw_%d" % m]
        j = 0
        while "S_tau_%d" % j in fields:
            contribution = contribution + fields["S_tau_%d" % j] * fields["dtau_%d_%d" % (m, j)]
            j += 1
        result[name] = math.fsum(contribution[used].tolist())
    return result


def read_canonical_csv(csv_path, objective=None):
    """dict case -> dJ_da_at_zero of the rows with probe_pair == 3 (restricted to `objective` when the column exists)."""
    values = {}
    with open(csv_path) as stream:
        for row in csv.DictReader(stream, skipinitialspace=True):
            row = {key.strip(): (value or "").strip() for key, value in row.items() if key is not None}
            if row.get("probe_pair") != CANONICAL_PROBE_PAIR:
                continue
            if objective and row.get("objective") and row["objective"] != objective:
                continue
            values[row["case"]] = float(row["dJ_da_at_zero"])
    return values


def _within(value, reference):
    rel = abs(value - reference) / max(abs(reference), _TINY)
    ok = math.isfinite(value) and math.isfinite(reference) and (rel <= CSV_REL_TOL or abs(value - reference) <= CSV_ABS_TOL)
    return ok, rel


def main(argv=None):
    parser = argparse.ArgumentParser(description="Recompute the classical EPM gradients from the SU2 field files and "
                                                 "compare with sst_epm_sensitivities.csv.")
    parser.add_argument("json", help="sst_epm_field_<OBJ>.json written by SU2")
    parser.add_argument("--csv", required=True, help="sst_epm_sensitivities.csv written by SU2")
    args = parser.parse_args(argv)

    fields = load(args.json)
    print("Loaded %d records, %d columns, %d ranks (objective %s)."
          % (len(fields["kind"]), len(fields), len(fields.meta.get("ranks", [])), fields.meta.get("objective", "?")))
    try:
        merged = merge(fields)
    except ValueError as error:
        print("FAIL: %s" % error)
        return 1
    print("Merged to %d sites." % len(merged["kind"]))
    unmerged_g = classical_gradients(fields)
    merged_g = classical_gradients(merged)
    reference = read_canonical_csv(args.csv, fields.meta.get("objective"))

    failed = False
    print("%-8s %24s %24s %12s %12s  %s" % ("case", "csv dJ_da_at_zero", "field (unmerged)", "rel diff", "rel (merged)", ""))
    for name in unmerged_g:
        if name not in reference:
            print("%-8s missing from %s (probe_pair == %s)" % (name, args.csv, CANONICAL_PROBE_PAIR))
            failed = True
            continue
        ok_u, rel_u = _within(unmerged_g[name], reference[name])
        ok_m, rel_m = _within(merged_g[name], reference[name])
        failed = failed or not (ok_u and ok_m)
        print("%-8s %24.16e %24.16e %12.3e %12.3e  %s" % (name, reference[name], unmerged_g[name], rel_u, rel_m,
                                                           "ok" if ok_u and ok_m else "FAIL"))
    print("%s (tolerance: rel %g or abs %g)" % ("FAIL" if failed else "PASS", CSV_REL_TOL, CSV_ABS_TOL))
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
