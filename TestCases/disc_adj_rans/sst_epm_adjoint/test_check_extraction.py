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
        self.assertLess(ce.compare_fields(reference, converged, tolerance=ce.CONVERGENCE_RELATIVE_TOLERANCE), 1e-8)


if __name__ == "__main__":
    unittest.main()
