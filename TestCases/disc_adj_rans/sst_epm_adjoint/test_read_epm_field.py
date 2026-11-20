#!/usr/bin/env python3
"""Local tests of read_epm_field.py on synthetic per-rank files (no SU2 run needed).

Run:  python3 -m unittest test_read_epm_field -v
"""

import contextlib
import io
import json
import math
import os
import tempfile
import unittest

import numpy as np

import read_epm_field as rd

CASES = ["1C_KEEP", "1C_SWAP", "2C_KEEP", "2C_SWAP", "3C"]
KINDS = {"EDGE": 0, "BOUNDARY": 1, "NODE": 2}


def writer_columns():
    """Column list in the exact order of CEPMFieldWriter.cpp::ColumnNames()."""
    names = ["kind", "key_a", "key_b"]
    names += ["coord_a_%d" % i for i in range(3)] + ["coord_b_%d" % i for i in range(3)]
    names += ["rho", "mu_t", "k"]
    names += ["grad_%d" % i for i in range(9)] + ["normal_%d" % i for i in range(3)]
    names += ["area", "volume", "prod_limit", "pk0", "pw0", "p_raw0", "alfa_blended", "floor_active", "cap_active"]
    names += ["S_tau_%d" % i for i in range(9)] + ["S_pk", "S_pw"]
    names += ["dtau_%d_%d" % (c, j) for c in range(5) for j in range(9)]
    names += ["dpk_%d" % c for c in range(5)] + ["dpw_%d" % c for c in range(5)]
    names += ["uses"]
    assert len(names) == 100
    return names


COLUMNS = writer_columns()


def make_row(kind, key_a, key_b, S_tau=None, S_pk=0.0, S_pw=0.0, dtau=None, dpk=None, dpw=None, uses=1.0, **state):
    """One 100-column record; dtau is a (5, 9) array, S_tau a length-9 array, state sets other columns by name."""
    values = dict.fromkeys(COLUMNS, 0.0)
    values.update(kind=float(KINDS[kind]), key_a=float(key_a), key_b=float(key_b), S_pk=S_pk, S_pw=S_pw, uses=uses)
    for name, value in state.items():
        assert name in values, name
        values[name] = value
    for j, s in enumerate(np.zeros(9) if S_tau is None else S_tau):
        values["S_tau_%d" % j] = float(s)
    for c in range(5):
        for j in range(9):
            if dtau is not None:
                values["dtau_%d_%d" % (c, j)] = float(dtau[c][j])
        if dpk is not None:
            values["dpk_%d" % c] = float(dpk[c])
        if dpw is not None:
            values["dpw_%d" % c] = float(dpw[c])
    return [values[name] for name in COLUMNS]


def write_case(directory, rank_rows, objective="DRAG", dtype="<f8", name_override=None):
    """Write sst_epm_field_<OBJ>_rank<r>.bin per rank and the master JSON; returns the JSON path."""
    base = "sst_epm_field_" + objective
    ranks = []
    for r, rows in enumerate(rank_rows):
        file_name = "%s_rank%d.bin" % (base, r)
        array = np.array(rows, dtype=dtype).reshape(-1, len(COLUMNS))
        array.tofile(os.path.join(directory, file_name))
        ranks.append({"file": file_name, "count": len(rows)})
    header = {
        "columns": COLUMNS, "dtype": "float64", "record_bytes": 8 * len(COLUMNS), "ranks": ranks, "nDim": 2,
        "objective": objective, "delta_b": 1, "urlx": 1, "gap_tolerance": 1e-10, "mpi_ranks": len(rank_rows),
        "su2_version": "8.6.0 Harrier", "source_commit": "unknown", "ref_dimensionalization": "DIMENSIONAL",
        "case_names": CASES, "kinds": KINDS, "S_tau_note": "symmetrized over nDim block; only sym part meaningful",
        "extract_only_residuals": None,
    }
    if name_override:
        header.update(name_override)
    path = os.path.join(directory, base + ".json")
    with open(path, "w") as stream:
        json.dump(header, stream)
    return path


def rng_arrays(seed):
    rng = np.random.RandomState(seed)
    return rng.randn(5, 9), rng.randn(5), rng.randn(5)


def manual_gradients(rows):
    """Hand-written reference: loop over rows and cases (uses > 0 only)."""
    index = {name: i for i, name in enumerate(COLUMNS)}
    total = [0.0] * 5
    for row in rows:
        if row[index["uses"]] == 0:
            continue
        for c in range(5):
            for j in range(9):
                total[c] += row[index["S_tau_%d" % j]] * row[index["dtau_%d_%d" % (c, j)]]
            total[c] += row[index["S_pk"]] * row[index["dpk_%d" % c]]
            total[c] += row[index["S_pw"]] * row[index["dpw_%d" % c]]
    return dict(zip(CASES, total))


class ReaderTests(unittest.TestCase):
    def setUp(self):
        self._tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self._tmp.cleanup)
        self.dir = self._tmp.name

    def _two_rank_case(self):
        """Rank 0: EDGE(3,7) share 0.25 + EDGE(1,2); rank 1: EDGE(3,7) share 0.75 + NODE(5,0)."""
        dtau, dpk, dpw = rng_arrays(1)
        dtau2, dpk2, dpw2 = rng_arrays(2)
        dtau3, dpk3, dpw3 = rng_arrays(3)
        s_a = np.arange(9) * 0.1
        s_b = np.arange(9) * 0.3
        rank0 = [
            make_row("EDGE", 3, 7, S_tau=0.25 * np.ones(9), S_pk=0.5, S_pw=-0.25, dtau=dtau, dpk=dpk, dpw=dpw, rho=1.2),
            make_row("EDGE", 1, 2, S_tau=s_a, S_pk=0.1, S_pw=0.2, dtau=dtau2, dpk=dpk2, dpw=dpw2),
        ]
        rank1 = [
            make_row("EDGE", 3, 7, S_tau=0.75 * np.ones(9), S_pk=1.5, S_pw=-0.75, dtau=dtau, dpk=dpk, dpw=dpw, rho=1.2),
            make_row("NODE", 5, 0, S_tau=s_b, S_pk=-0.3, S_pw=0.4, dtau=dtau3, dpk=dpk3, dpw=dpw3),
        ]
        return rank0, rank1

    def test_merge_sums_halo_copies(self):
        rank0, rank1 = self._two_rank_case()
        fields = rd.load(write_case(self.dir, [rank0, rank1]))
        self.assertEqual(len(fields["kind"]), 4)
        merged = rd.merge(fields)
        self.assertEqual(len(merged["kind"]), 3)
        shared = (merged["kind"] == KINDS["EDGE"]) & (merged["key_a"] == 3) & (merged["key_b"] == 7)
        self.assertEqual(int(shared.sum()), 1)
        self.assertEqual(merged["S_tau_1"][shared][0], 1.0)
        self.assertEqual(merged["S_pk"][shared][0], 2.0)
        self.assertEqual(merged["S_pw"][shared][0], -1.0)
        self.assertEqual(merged["rho"][shared][0], 1.2)  # state kept from the first copy
        for c in range(5):
            self.assertEqual(merged["dpk_%d" % c][shared][0], fields["dpk_%d" % c][0])
        # first-appearance order is preserved: (3,7) EDGE, (1,2) EDGE, (5,0) NODE
        self.assertEqual(list(merged["key_a"]), [3.0, 1.0, 5.0])
        # the input is not modified
        self.assertEqual(len(fields["kind"]), 4)
        self.assertEqual(fields["S_tau_1"][0], 0.25)

    def test_classical_matches_manual(self):
        rank0, rank1 = self._two_rank_case()
        fields = rd.load(write_case(self.dir, [rank0, rank1]))
        expected = manual_gradients(rank0 + rank1)
        got = rd.classical_gradients(fields)
        self.assertEqual(list(got), CASES)
        for name in CASES:
            self.assertIsInstance(got[name], float)
            self.assertTrue(math.isclose(got[name], expected[name], rel_tol=1e-13, abs_tol=1e-14), name)

    def test_classical_same_on_merged_and_unmerged(self):
        rank0, rank1 = self._two_rank_case()
        fields = rd.load(write_case(self.dir, [rank0, rank1]))
        unmerged = rd.classical_gradients(fields)
        merged = rd.classical_gradients(rd.merge(fields))
        for name in CASES:
            self.assertTrue(math.isclose(unmerged[name], merged[name], rel_tol=1e-12, abs_tol=1e-14), name)

    def test_inconsistent_copy_raises(self):
        rank0, rank1 = self._two_rank_case()
        dtau, dpk, dpw = rng_arrays(1)
        dtau = dtau.copy()
        dtau[2, 4] *= 1 + 1e-8  # beyond 1e-10 relative and beyond the column floor
        rank1[0] = make_row("EDGE", 3, 7, S_tau=0.75 * np.ones(9), dtau=dtau, dpk=dpk, dpw=dpw)
        fields = rd.load(write_case(self.dir, [rank0, rank1]))
        with self.assertRaises(ValueError):
            rd.merge(fields)

    def test_rounding_level_difference_is_accepted(self):
        rank0, rank1 = self._two_rank_case()
        dtau, dpk, dpw = rng_arrays(1)
        dtau = dtau.copy()
        dtau[2, 4] *= 1 + 1e-14
        rank1[0] = make_row("EDGE", 3, 7, S_tau=0.75 * np.ones(9), dtau=dtau, dpk=dpk, dpw=dpw)
        merged = rd.merge(rd.load(write_case(self.dir, [rank0, rank1])))
        self.assertEqual(len(merged["kind"]), 3)

    def test_cluster_3d_round_off_between_copies_is_accepted(self):
        # 2-rank 3D smoke run (job 664723): the two copies of EDGE (230, 293) differ at round-off, because the ranks
        # orient the edge differently and the gradient correction is not bitwise symmetric in (i, j).
        dtau, dpk, dpw = rng_arrays(1)
        first, second = dtau.copy(), dtau.copy()
        first[2, 2], second[2, 2] = -5.4376830634819001e-10, -5.4376830634671484e-10
        rows0 = [make_row("EDGE", 230, 293, S_tau=np.ones(9), dtau=first, dpk=dpk, dpw=dpw)]
        rows1 = [make_row("EDGE", 230, 293, S_tau=np.ones(9), dtau=second, dpk=dpk, dpw=dpw)]
        merged = rd.merge(rd.load(write_case(self.dir, [rows0, rows1])))
        self.assertEqual(len(merged["kind"]), 1)
        self.assertEqual(merged["dtau_2_2"][0], first[2, 2])  # kept from the first copy
        self.assertEqual(merged["S_tau_0"][0], 2.0)

    def test_small_value_round_off_is_accepted_by_the_column_floor(self):
        # Relative to itself the difference is large, but it is below 1e-10 times the column's max norm.
        dtau, dpk, dpw = rng_arrays(1)
        big, small_a, small_b = dtau.copy(), dtau.copy(), dtau.copy()
        big[0, 3] = 1e-5
        small_a[0, 3], small_b[0, 3] = 1e-20, 3e-20
        rows0 = [make_row("EDGE", 1, 2, dtau=big, dpk=dpk, dpw=dpw), make_row("EDGE", 3, 4, dtau=small_a)]
        rows1 = [make_row("EDGE", 3, 4, dtau=small_b)]
        merged = rd.merge(rd.load(write_case(self.dir, [rows0, rows1])))
        self.assertEqual(len(merged["kind"]), 2)

    def test_small_value_above_the_column_floor_raises(self):
        dtau, dpk, dpw = rng_arrays(1)
        big, small_a, small_b = dtau.copy(), dtau.copy(), dtau.copy()
        big[0, 3] = 1.0
        small_a[0, 3], small_b[0, 3] = 1e-3, 1e-3 * (1 + 1e-6)  # difference 1e-9 > floor 1e-10
        rows0 = [make_row("EDGE", 1, 2, dtau=big, dpk=dpk, dpw=dpw), make_row("EDGE", 3, 4, dtau=small_a)]
        rows1 = [make_row("EDGE", 3, 4, dtau=small_b)]
        with self.assertRaises(ValueError):
            rd.merge(rd.load(write_case(self.dir, [rows0, rows1])))

    def test_scaled_close(self):
        a = np.array([1.0, 1e-20, 1e-3, np.nan, np.inf])
        b = np.array([1.0 + 5e-11, 3e-20, 1e-3 + 1e-9, np.nan, np.inf])
        self.assertEqual(list(rd.scaled_close(a, b, 1.0)), [True, True, False, True, True])
        self.assertEqual(list(rd.scaled_close(a, b, 0.0)), [True, False, False, True, True])

    def test_zero_dtau_copies_agree(self):
        # scale-aware check: exact zeros on both copies are consistent
        rows0 = [make_row("EDGE", 1, 2, S_tau=np.ones(9))]
        rows1 = [make_row("EDGE", 1, 2, S_tau=np.ones(9))]
        merged = rd.merge(rd.load(write_case(self.dir, [rows0, rows1])))
        self.assertEqual(merged["S_tau_0"][0], 2.0)

    def test_unused_rows_do_not_trigger_check_and_contribute_nothing(self):
        dtau, dpk, dpw = rng_arrays(1)
        used = make_row("EDGE", 3, 7, S_tau=np.ones(9), S_pk=2.0, dtau=dtau, dpk=dpk, dpw=dpw, rho=1.2, uses=1.0)
        # never evaluated on rank 0: all zero except the key, uses == 0 (so dtau differs from the used copy)
        unused = make_row("EDGE", 3, 7, uses=0.0)
        for order in ([unused], [used]), ([used], [unused]):
            sub = tempfile.mkdtemp(dir=self.dir)
            fields = rd.load(write_case(sub, list(order)))
            merged = rd.merge(fields)
            self.assertEqual(len(merged["kind"]), 1)
            self.assertEqual(merged["rho"][0], 1.2)  # state from the evaluated copy whatever the order
            self.assertEqual(merged["uses"][0], 1.0)
            self.assertEqual(merged["S_pk"][0], 2.0)
            for c in range(5):
                self.assertEqual(merged["dpk_%d" % c][0], dpk[c])
            expected = manual_gradients([used])
            got = rd.classical_gradients(merged)
            for name in CASES:
                self.assertTrue(math.isclose(got[name], expected[name], rel_tol=1e-13, abs_tol=1e-14))
        # a group with only unused copies stays one unused row
        sub = tempfile.mkdtemp(dir=self.dir)
        merged = rd.merge(rd.load(write_case(sub, [[unused], [unused]])))
        self.assertEqual(len(merged["kind"]), 1)
        self.assertEqual(merged["uses"][0], 0.0)

    def test_unused_row_with_nan_state_is_ignored_by_classical(self):
        dtau, dpk, dpw = rng_arrays(1)
        used = make_row("EDGE", 1, 2, S_tau=np.ones(9), dtau=dtau, dpk=dpk, dpw=dpw)
        nan_row = make_row("NODE", 4, 0, uses=0.0, rho=float("nan"))
        nan_row[COLUMNS.index("dtau_0_0")] = float("nan")
        fields = rd.load(write_case(self.dir, [[used, nan_row]]))
        got = rd.classical_gradients(fields)
        expected = manual_gradients([used])
        for name in CASES:
            self.assertTrue(math.isclose(got[name], expected[name], rel_tol=1e-13, abs_tol=1e-14))
        self.assertTrue(np.isnan(fields["rho"][1]))  # the NaN row is preserved by load

    def test_kind_and_key_distinguish_sites(self):
        # same key pair but different kind, or different key_b, are different sites
        rows = [make_row("EDGE", 3, 7, S_pk=1.0), make_row("BOUNDARY", 3, 7, S_pk=2.0), make_row("EDGE", 3, 8, S_pk=4.0)]
        merged = rd.merge(rd.load(write_case(self.dir, [rows])))
        self.assertEqual(list(merged["S_pk"]), [1.0, 2.0, 4.0])

    def test_empty_rank_file(self):
        rank0, rank1 = self._two_rank_case()
        fields = rd.load(write_case(self.dir, [rank0, [], rank1]))
        self.assertEqual(len(fields["kind"]), 4)
        self.assertEqual(list(fields.keys()), COLUMNS)
        fields = rd.load(write_case(tempfile.mkdtemp(dir=self.dir), [[], []]))
        self.assertEqual(len(fields["kind"]), 0)
        self.assertEqual(rd.classical_gradients(fields), dict.fromkeys(CASES, 0.0))
        self.assertEqual(len(rd.merge(fields)["kind"]), 0)

    def test_load_checks_file_size(self):
        rank0, rank1 = self._two_rank_case()
        path = write_case(self.dir, [rank0, rank1])
        with open(path) as stream:
            header = json.load(stream)
        header["ranks"][0]["count"] = 5
        with open(path, "w") as stream:
            json.dump(header, stream)
        with self.assertRaises(ValueError):
            rd.load(path)

    def test_load_checks_record_bytes(self):
        rank0, _ = self._two_rank_case()
        path = write_case(self.dir, [rank0], name_override={"record_bytes": 808})
        with self.assertRaises(ValueError):
            rd.load(path)

    def _csv(self, values, extra_rows=()):
        path = os.path.join(self.dir, "sst_epm_sensitivities.csv")
        with open(path, "w") as stream:
            stream.write("case,objective,dimension,mpi_ranks,probe_pair,probe_angle,J_baseline,dJ_da_at_zero\n")
            for name in CASES:
                stream.write("%s,DRAG,2,2,3,0,0.5,%.17g\n" % (name, values[name]))
            for row in extra_rows:
                stream.write(row + "\n")
        return path

    def _run_cli(self, argv):
        out = io.StringIO()
        with contextlib.redirect_stdout(out), contextlib.redirect_stderr(out):
            code = rd.main(argv)
        return code, out.getvalue()

    def test_cli_pass_and_fail(self):
        rank0, rank1 = self._two_rank_case()
        json_path = write_case(self.dir, [rank0, rank1])
        reference = manual_gradients(rank0 + rank1)
        # non-canonical probe rows with wrong values must be ignored
        junk = ["%s,DRAG,2,2,0,1.5707963267948966,0.5,12345.0" % name for name in CASES]
        csv_path = self._csv(reference, junk)
        code, text = self._run_cli([json_path, "--csv", csv_path])
        self.assertEqual(code, 0, text)
        for name in CASES:
            self.assertIn(name, text)
        bad = dict(reference)
        bad["3C"] = bad["3C"] * (1 + 1e-6) + 1e-3
        code, text = self._run_cli([json_path, "--csv", self._csv(bad)])
        self.assertEqual(code, 1, text)

    def test_cli_inconsistent_halo_fails(self):
        rank0, rank1 = self._two_rank_case()
        dtau, dpk, dpw = rng_arrays(1)
        rank1[0] = make_row("EDGE", 3, 7, S_tau=0.75 * np.ones(9), dtau=dtau * 1.001, dpk=dpk, dpw=dpw)
        json_path = write_case(self.dir, [rank0, rank1])
        csv_path = self._csv(manual_gradients(rank0 + rank1))
        code, _ = self._run_cli([json_path, "--csv", csv_path])
        self.assertEqual(code, 1)


if __name__ == "__main__":
    unittest.main()
