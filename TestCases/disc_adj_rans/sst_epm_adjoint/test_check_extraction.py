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
