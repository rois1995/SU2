#!/usr/bin/env python3
"""Local tests of the log/field helpers of check_extraction.py (no SU2 run needed).

Run:  python3 -m unittest test_check_extraction -v
"""

import unittest

import numpy as np

import check_extraction as ce

# Excerpt of the 2-rank mpi_extract_field log of cluster job 664755 (serial adjoint loaded on 2 ranks).
REJECTED_LOG = """Objective function value: 0.008355488771649244

EPM extract-only: residual of one adjoint iteration from the loaded adjoint (update discarded, criterion CONV_RESIDUAL_MINVAL= -10): FAILED
  RMS_ADJ_DENSITY = -7.15985
  RMS_ADJ_MOMENTUM-X = -6.75753
  RMS_ADJ_MOMENTUM-Y = -6.08949
  RMS_ADJ_ENERGY = -6.88548
  RMS_ADJ_TKE = -7.25253
  RMS_ADJ_DISSIPATION = -12.0643
  (MPI: residual of the total adjoint, halo copies summed into their owners; the history values above the extraction measure the owned part only.)



Error in "void CDiscAdjSinglezoneDriver::ExtractEPMSensitivities()":
-------------------------------------------------------------------------
EPM extraction requires the native adjoint convergence criterion to pass.
"""

PASSED_LOG = """EPM extract-only: residual of one adjoint iteration from the loaded adjoint (update discarded, criterion CONV_RESIDUAL_MINVAL= -10): passed
  RMS_ADJ_DENSITY = -10.0217
  RMS_ADJ_TKE = -10.5685

All convergence criteria satisfied.
"""


# Guard of the 2-rank extract_only from the run's own (lossy) restart, cluster job 664770 (2D): the worst expected
# rejection (RMS_ADJ_TKE -5.46).
LOSSY_LOG = REJECTED_LOG.replace("-7.15985", "-7.01272").replace("-6.75753", "-7.03131") \
    .replace("-6.08949", "-6.7575").replace("-6.88548", "-8.50756").replace("-7.25253", "-5.46406") \
    .replace("-12.0643", "-10.409")
CONV_2D = ["RMS_ADJ_DENSITY", "RMS_ADJ_MOMENTUM-X", "RMS_ADJ_MOMENTUM-Y", "RMS_ADJ_ENERGY", "RMS_ADJ_TKE",
           "RMS_ADJ_DISSIPATION"]

# Cluster job 664770, 3D, 2 ranks: first and last rms rows of "enabled" (from a zero adjoint) and last of "restarted".
RMS_3D = ["rms[A_Rho]", "rms[A_RhoU]", "rms[A_RhoV]", "rms[A_RhoW]", "rms[A_E]", "rms[A_k]", "rms[A_w]"]
ENABLED_FIRST_3D = dict(zip(RMS_3D, ["-4.066033774400671", "-4.306755873744074", "-4.707926668448635",
                                     "-5.439802363626545", "-5.659523324193763", "-4.727274158161164",
                                     "-10.73468824301553"]), LinSolIter="14")
ENABLED_LAST_3D = dict(zip(RMS_3D, ["-10.00559031042068", "-11.75163682786777", "-11.57854349982532",
                                    "-11.70395213470011", "-11.4626757896374", "-10.84909377796904",
                                    "-15.83869337595136"]), LinSolIter="13")
RESTARTED_LAST_3D = dict(zip(RMS_3D, ["-10.03360725070384", "-11.42024225040932", "-11.37088411083081",
                                      "-11.31939315466477", "-11.46763734187326", "-10.29584535453758",
                                      "-14.47896291398262"]), LinSolIter="13")


class HelperTests(unittest.TestCase):
    def test_guard_values_rejected(self):
        guard = ce.guard_values(REJECTED_LOG)
        self.assertFalse(guard["passed"])
        self.assertEqual(len(guard["fields"]), 6)
        self.assertEqual(guard["fields"]["RMS_ADJ_MOMENTUM-Y"], -6.08949)
        self.assertIn(ce.GUARD_ERROR, REJECTED_LOG)

    def test_guard_values_passed_and_missing(self):
        guard = ce.guard_values(PASSED_LOG)
        self.assertTrue(guard["passed"])
        self.assertEqual(guard["fields"], {"RMS_ADJ_DENSITY": -10.0217, "RMS_ADJ_TKE": -10.5685})
        self.assertIsNone(ce.guard_values("no guard here\n"))

    def test_linear_solver_stats(self):
        history = [{"rms[A_Rho]": "-5", "LinSolIter": "12", "LinSolRes": "-10.5", "LinSolIterTurb": "3"},
                   {"rms[A_Rho]": "-6", "LinSolIter": "50", "LinSolRes": "-7.2", "LinSolIterTurb": "4"}]
        self.assertEqual(ce.linear_solver_stats(history), {"LinSolIter": 50.0, "LinSolRes": -7.2,
                                                           "LinSolIterTurb": 4.0})

    def _fields(self, s_values):
        n = len(s_values)
        fields = {"kind": np.zeros(n), "key_a": np.arange(n, dtype=float), "key_b": np.arange(n, dtype=float) + 1,
                  "uses": np.ones(n), "S_pk": np.array(s_values, dtype=float), "S_pw": np.zeros(n)}
        for j in range(9):
            fields["S_tau_%d" % j] = np.array(s_values, dtype=float)
        return fields

    def test_compare_fields_tolerances(self):
        reference = self._fields([1.0, 2.0, 1e-3])
        close = self._fields([1.0 + 1e-11, 2.0, 1e-3])
        converged = self._fields([1.0 + 1e-8, 2.0, 1e-3 + 1e-9])
        ce.compare_fields(reference, close)
        with self.assertRaises(AssertionError):
            ce.compare_fields(reference, converged)
        self.assertLess(ce.compare_fields(reference, converged, tolerance=1e-6), 1e-8)


def orientation_fields(edge_normal, boundary_normal):
    """Unit square [0,1]^2: EDGE (0,0)-(1,0) as key_a-key_b, BOUNDARY at x = 1, NODE at the centre."""
    nan = float("nan")
    rows = [  # kind, coord_a, coord_b, normal
        (0, (0.0, 0.0, 0.0), (1.0, 0.0, 0.0), edge_normal),
        (1, (1.0, 0.5, 0.0), (nan, nan, nan), boundary_normal),
        (2, (0.0, 1.0, 0.0), (nan, nan, nan), (nan, nan, nan)),
    ]
    fields = {"kind": np.array([float(r[0]) for r in rows]), "uses": np.ones(len(rows))}
    for i in range(3):
        fields[f"coord_a_{i}"] = np.array([r[1][i] for r in rows])
        fields[f"coord_b_{i}"] = np.array([r[2][i] for r in rows])
        fields[f"normal_{i}"] = np.array([r[3][i] for r in rows])
    return fields


class FadeGapTests(unittest.TestCase):
    def test_rows(self):
        self.assertEqual(ce.fade_gap_of_rows([{"case": "3C"}, {"case": "3C", "fade_gap": ""}]), 0.0)
        self.assertEqual(ce.fade_gap_of_rows([{"fade_gap": "0.050000000000000003"}, {"fade_gap": "0.05"}]), 0.05)
        with self.assertRaises(ValueError):
            ce.fade_gap_of_rows([{"fade_gap": "0.05"}, {"fade_gap": "0.01"}])

    def test_options(self):
        """Only EPM extraction and direct runs get UQ_FADE_GAP (plain SST rejects it); 0 adds nothing."""
        plain = {"SOLVER": "RANS"}
        extraction = dict(plain, SST_EPM_POSTPROCESS="YES")
        direct = dict(plain, SST_EPM_DIRECT="YES")
        self.assertIs(ce.with_fade_gap(plain, 0.05), plain)
        self.assertIs(ce.with_fade_gap(extraction, 0.0), extraction)
        self.assertEqual(float(ce.with_fade_gap(extraction, 0.05)["UQ_FADE_GAP"]), 0.05)
        self.assertEqual(float(ce.with_fade_gap(direct, 0.01)["UQ_FADE_GAP"]), 0.01)
        self.assertNotIn("UQ_FADE_GAP", extraction)


class NormalOrientationTests(unittest.TestCase):
    def test_convention_passes(self):
        ce.check_normal_orientation(orientation_fields((1.0, 0.0, 0.0), (1.0, 0.0, 0.0)), ce.FIELD_KINDS)

    def test_edge_from_key_b_to_key_a_fails(self):
        with self.assertRaisesRegex(AssertionError, "EDGE"):
            ce.check_normal_orientation(orientation_fields((-1.0, 0.0, 0.0), (1.0, 0.0, 0.0)), ce.FIELD_KINDS)

    def test_inward_boundary_fails(self):
        with self.assertRaisesRegex(AssertionError, "BOUNDARY"):
            ce.check_normal_orientation(orientation_fields((1.0, 0.0, 0.0), (-1.0, 0.0, 0.0)), ce.FIELD_KINDS)

    def test_unused_rows_are_ignored(self):
        fields = orientation_fields((-1.0, 0.0, 0.0), (1.0, 0.0, 0.0))
        fields["kind"] = np.append(fields["kind"], 0.0)
        fields["uses"] = np.append(fields["uses"], 0.0)
        for i in range(3):
            for name in ("coord_a", "coord_b", "normal"):
                fields[f"{name}_{i}"] = np.append(fields[f"{name}_{i}"], 0.0)
        fields["normal_0"][0] = 1.0  # the used edge is fine, the unused zero row is skipped
        ce.check_normal_orientation(fields, ce.FIELD_KINDS)


class CompareNormalsTests(unittest.TestCase):
    def _fields(self, edge_normal, keys=((0.0, 3.0, 7.0), (2.0, 5.0, 0.0))):
        fields = orientation_fields(edge_normal, (1.0, 0.0, 0.0))
        fields = {name: values[[0, 2]] for name, values in fields.items()}  # EDGE and NODE rows
        for name, column in zip(("kind", "key_a", "key_b"), zip(*keys)):
            fields[name] = np.array(column)
        return fields

    def test_same_convention_agrees(self):
        self.assertLess(ce.compare_normals(self._fields((1.0, 0.0, 0.0)), self._fields((1.0 + 1e-16, 0.0, 0.0))),
                        1e-15)

    def test_flipped_edge_normal_fails(self):
        with self.assertRaises(AssertionError):
            ce.compare_normals(self._fields((1.0, 0.0, 0.0)), self._fields((-1.0, 0.0, 0.0)))


class GuardDecisionTests(unittest.TestCase):
    STATUS = "rejected_by_guard_partition_dependent_adjoint"

    def test_expected_rejections(self):
        status, guard = ce.extract_only_outcome(1, REJECTED_LOG, CONV_2D, self.STATUS)
        self.assertEqual(status, self.STATUS)
        self.assertEqual(guard["fields"]["RMS_ADJ_MOMENTUM-Y"], -6.08949)
        status, guard = ce.extract_only_outcome(1, LOSSY_LOG, CONV_2D, "rejected_by_guard_lossy_mpi_restart")
        self.assertEqual(status, "rejected_by_guard_lossy_mpi_restart")
        self.assertEqual(guard["fields"]["RMS_ADJ_TKE"], -5.46406)

    def test_gross_rejection_fails(self):
        # A zero or wrongly loaded adjoint: residual of the first iteration from zero (rms[A_Rho] -3.5 in 2D).
        for value in ("-3.50849", "-1.2", "-4.99"):
            with self.assertRaises(AssertionError):
                ce.extract_only_outcome(1, REJECTED_LOG.replace("-7.15985", value), CONV_2D, self.STATUS)
        ce.extract_only_outcome(1, REJECTED_LOG.replace("-7.15985", "-5.0"), CONV_2D, self.STATUS)

    def test_other_failures(self):
        with self.assertRaises(AssertionError):  # failed, but not because of the guard
            ce.extract_only_outcome(1, REJECTED_LOG.replace(ce.GUARD_ERROR, "Segmentation fault"), CONV_2D,
                                    self.STATUS)
        with self.assertRaises(AssertionError):  # failed before the guard
            ce.extract_only_outcome(1, "Error: cannot open restart_adj_cd.dat\n", CONV_2D, self.STATUS)
        with self.assertRaises(AssertionError):  # a convergence field missing from the guard block
            ce.extract_only_outcome(1, REJECTED_LOG, CONV_2D + ["RMS_ADJ_MOMENTUM-Z"], self.STATUS)
        with self.assertRaises(AssertionError):  # nonfinite guard value
            ce.extract_only_outcome(1, REJECTED_LOG.replace("-7.15985", "nan"), CONV_2D, self.STATUS)
        with self.assertRaises(AssertionError):  # exit code 0 with a failed guard
            ce.extract_only_outcome(0, REJECTED_LOG, CONV_2D, self.STATUS)

    def test_passed(self):
        log = PASSED_LOG.replace("  RMS_ADJ_TKE", "".join(f"  {n} = -11\n" for n in CONV_2D[1:4]) + "  RMS_ADJ_TKE")
        log = log.replace("  RMS_ADJ_TKE = -10.5685\n", "  RMS_ADJ_TKE = -10.5685\n  RMS_ADJ_DISSIPATION = -15.1\n")
        status, guard = ce.extract_only_outcome(0, log, CONV_2D, self.STATUS)
        self.assertEqual(status, "passed")
        self.assertTrue(guard["passed"])


class ConvergenceToleranceTests(unittest.TestCase):
    def test_cluster_3d_levels(self):
        enabled = ce.convergence_level(ENABLED_FIRST_3D, [ENABLED_FIRST_3D, ENABLED_LAST_3D])
        restarted = ce.convergence_level(ENABLED_FIRST_3D, [RESTARTED_LAST_3D])
        # The omega adjoint decides in both: 5.10 and 3.74 decades below its first update.
        self.assertAlmostEqual(enabled, 10 ** (-15.83869337595136 + 10.73468824301553), places=12)
        self.assertAlmostEqual(restarted, 10 ** (-14.47896291398262 + 10.73468824301553), places=12)
        tolerance = ce.convergence_tolerance(ENABLED_FIRST_3D, [ENABLED_LAST_3D], [RESTARTED_LAST_3D])
        self.assertAlmostEqual(tolerance, enabled + restarted, places=15)
        self.assertTrue(1.8e-4 < tolerance < 2.0e-4, tolerance)

    def test_cluster_3d_restarted_field_passes(self):
        # Failure of job 664770 at the former fixed 1e-6: S_pw at NODE 27 (scaled error 1.03e-6; column worst 3.1e-6).
        tolerance = ce.convergence_tolerance(ENABLED_FIRST_3D, [ENABLED_LAST_3D], [RESTARTED_LAST_3D])
        a, b, scale = -3.880947525339641e-11, -3.880990968556661e-11, 4.212014932520764e-10
        self.assertTrue(ce.epm_field.scaled_close(np.array([a]), np.array([b]), scale, tolerance)[0])
        self.assertFalse(ce.epm_field.scaled_close(np.array([a]), np.array([b]), scale, 1e-6)[0])
        # An O(1) defect (halo double count) and a 1e-2 one are still caught.
        for wrong in (2 * a, a + 1e-2 * scale):
            self.assertFalse(ce.epm_field.scaled_close(np.array([a]), np.array([wrong]), scale, tolerance)[0])

    def test_cap(self):
        poorly = dict(RESTARTED_LAST_3D, **{"rms[A_w]": "-13.0"})  # 2.3 decades below the omega scale
        with self.assertRaises(AssertionError):
            ce.convergence_tolerance(ENABLED_FIRST_3D, [ENABLED_LAST_3D], [poorly])
        with self.assertRaises(AssertionError):  # a component missing from the compared history
            ce.convergence_level(ENABLED_FIRST_3D, [{k: v for k, v in ENABLED_LAST_3D.items() if k != "rms[A_w]"}])


if __name__ == "__main__":
    unittest.main()
