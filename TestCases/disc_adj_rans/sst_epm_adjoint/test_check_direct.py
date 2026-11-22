#!/usr/bin/env python3
"""Local tests of the helpers of check_direct.py (no SU2 run needed).

Run:  python3 -m unittest test_check_direct -v
"""

import math
import unittest

import check_direct as cd

ROWS = [
    {"case": "1C_KEEP", "probe_pair": "3", "dJ_da_at_zero": "-3.39e-05"},
    {"case": "1C_KEEP", "probe_pair": "0", "dJ_da_at_zero": "-3.0e-05"},
    {"case": "1C_SWAP", "probe_pair": "3", "dJ_da_at_zero": "1.33e-05"},
    {"case": "2C_KEEP", "probe_pair": "3", "dJ_da_at_zero": "-4.36e-06"},
    {"case": "2C_SWAP", "probe_pair": "3", "dJ_da_at_zero": "1.92e-05"},
    {"case": "3C", "probe_pair": "3", "dJ_da_at_zero": "2.45e-05"},
]


class CanonicalGradients(unittest.TestCase):
    def test_canonical_rows_only(self):
        gradients = cd.canonical_gradients(ROWS)
        self.assertEqual(set(gradients), set(cd.MODES))
        self.assertEqual(gradients["1C_KEEP"], -3.39e-05)

    def test_missing_case(self):
        with self.assertRaises(ValueError):
            cd.canonical_gradients([r for r in ROWS if r["case"] != "3C"])

    def test_duplicate_case(self):
        with self.assertRaises(ValueError):
            cd.canonical_gradients(ROWS + [dict(ROWS[0])])


class Histories(unittest.TestCase):
    A = [{"Time_Iter": "0", "Inner_Iter": "0", "rms[Rho]": "-10.1", "CD": "0.0083", "WALL_TIME": "1.0"},
         {"Time_Iter": "0", "Inner_Iter": "1", "rms[Rho]": "-10.2", "CD": "0.0084", "WALL_TIME": "2.0"}]

    def test_identical_ignores_wall_time(self):
        b = [dict(row, WALL_TIME="9") for row in self.A]
        self.assertEqual(cd.history_mismatches(self.A, b), [])

    def test_value_difference(self):
        b = [dict(self.A[0]), dict(self.A[1], CD="0.00840000001")]
        self.assertEqual(cd.history_mismatches(self.A, b), [(1, "CD", "0.0084", "0.00840000001")])

    def test_length_and_columns(self):
        self.assertTrue(cd.history_mismatches(self.A, self.A[:1]))
        self.assertTrue(cd.history_mismatches(self.A, [{k: v for k, v in r.items() if k != "CD"} for r in self.A]))
        self.assertTrue(cd.history_mismatches([], []))


class Taylor(unittest.TestCase):
    def test_quadratic_objective_is_exact(self):
        # J(A) = J0 + g A + c A^2: the central difference is exact, the forward secant is off by c h.
        j0, g, c, h = 0.0083, 1.3e-5, 4e-4, 0.01
        result = cd.taylor(j0 + g*h + c*h*h, j0 - g*h + c*h*h, j0, h, g, rel_tol=1e-3)
        self.assertTrue(result["passed"])
        self.assertAlmostEqual(result["central"], g, delta=1e-15)
        self.assertAlmostEqual(result["forward"], g + c*h, delta=1e-13)
        self.assertAlmostEqual(result["backward"], g - c*h, delta=1e-13)
        self.assertLess(result["rel_error"], 1e-9)

    def test_clip_after_blend_factor_fails(self):
        # The SWAP mismatch of the classical solver path (secant/g ~ 1.8) must fail.
        j0, g, h = 0.0083, 1.3e-5, 0.01
        result = cd.taylor(j0 + 1.8*g*h, j0 - 1.8*g*h, j0, h, g, rel_tol=1e-3)
        self.assertFalse(result["passed"])
        self.assertAlmostEqual(result["rel_error"], 0.8, places=9)

    def test_tolerance_has_absolute_floor(self):
        result = cd.taylor(1 + 1e-14, 1 - 1e-14, 1, 0.01, 0.0, rel_tol=1e-3, abs_tol=1e-11)
        self.assertTrue(result["passed"])
        self.assertFalse(cd.taylor(1 + 1e-6, 1 - 1e-6, 1, 0.01, 0.0, rel_tol=1e-3, abs_tol=1e-11)["passed"])

    def test_nonfinite_fails(self):
        self.assertFalse(cd.taylor(math.nan, 1, 1, 0.01, 1e-5)["passed"])


class Endpoint(unittest.TestCase):
    def test_compare_endpoint(self):
        self.assertTrue(cd.endpoint_comparison(0.0091, 0.0091 * (1 + 1e-10), True, True)["passed"])
        self.assertFalse(cd.endpoint_comparison(0.0091, 0.0092, True, True)["passed"])
        # Unconverged endpoints are reported, not compared.
        result = cd.endpoint_comparison(0.0091, 0.0092, True, False)
        self.assertIsNone(result["passed"])


class Overrides(unittest.TestCase):
    def test_direct_overrides(self):
        options = cd.direct_overrides("2C_SWAP", -0.01)
        self.assertEqual(options["SST_EPM_DIRECT"], "YES")
        self.assertEqual(options["SST_EPM_DIRECT_AMPLITUDE"], "-0.01")
        self.assertEqual((options["UQ_COMPONENT"], options["UQ_PERMUTE"]), ("2", "YES"))
        self.assertEqual(options["SST_OPTIONS"], "V2003m")
        self.assertEqual(float(cd.direct_overrides("3C", 1e-3)["SST_EPM_DIRECT_AMPLITUDE"]), 1e-3)

    def test_classical_overrides(self):
        options = cd.classical_overrides("1C_SWAP")
        self.assertEqual(options["SST_OPTIONS"], "(V2003m, UQ)")
        self.assertEqual((options["UQ_URLX"], options["UQ_DELTA_B"]), ("1", "1"))
        self.assertEqual(options["UQ_PERMUTE"], "YES")
        self.assertNotIn("SST_EPM_DIRECT", options)


if __name__ == "__main__":
    unittest.main()
