"""Shared first wall-normal height gate |h-h0|/h0 <= T of the native audit helpers (ADAP_BL_FIRST_HEIGHT_TOL).

Recorded historical evidence used the exact-placement gate 1e-8; it stays the default so those results reproduce.
Audits of meshes built under a first-height tolerance must pass it explicitly through SU2_BL_FIRST_HEIGHT_TOL
(e.g. 0.10, the solver default). The comparison is inclusive; a missing or nonfinite error fails."""
import math
import os

HISTORICAL = 1e-8


def tolerance():
    value = float(os.environ.get('SU2_BL_FIRST_HEIGHT_TOL', HISTORICAL))
    if not 0 < value < 1:
        raise ValueError('SU2_BL_FIRST_HEIGHT_TOL must lie strictly between 0 and 1')
    return value


def within(error):
    return error is not None and math.isfinite(error) and error <= tolerance()
