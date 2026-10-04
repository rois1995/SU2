/*!
 * \file TransferTolerances.hpp
 * \brief Tolerances of the solution transfers in double and single precision builds.
 * \version 8.5.0 "Harrier"
 *
 * SU2 Project Website: https://su2code.github.io
 *
 * The SU2 Project is maintained by the SU2 Foundation
 * (http://su2foundation.org)
 *
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md)
 *
 * SU2 is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * SU2 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with SU2. If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <limits>
#include <type_traits>

#include "../code_config.hpp"

/*!
 * \brief The three classes of transfer tolerances. In double builds every function returns its double argument
 *        unchanged (double results are not affected); they differ when passivedouble is float (single precision).
 *
 *        (a) Numerical acceptance gates (conservation defect, redistribution residuals, CG tolerances): explicit values
 *            for each precision, TransferTol(dp, sp).
 *        (b) Geometric decision thresholds (overlap, gap, full coverage, centroid, nearest-face margin): not scaled
 *            with the precision; only raised to the round-off floor of the compared quantity, 16 eps of passivedouble
 *            (3.6e-15 in double, below every such threshold; 1.9e-6 in single precision).
 *        (c) Diagnostic counters (values moved by a limiter, points off the domain): scaled by eps_float / eps_double
 *            = 2^29, they only count.
 */
namespace transfer_tol {
constexpr bool kSinglePrecision = std::is_same<passivedouble, float>::value;
}  // namespace transfer_tol

/*! \brief (a) Numerical acceptance gate: dp in double builds, sp in single precision builds. */
constexpr passivedouble TransferTol(double dp, double sp) {
  return static_cast<passivedouble>(transfer_tol::kSinglePrecision ? sp : dp);
}

/*! \brief (b) Geometric decision threshold k, raised to the round-off floor 16 eps of passivedouble. */
constexpr passivedouble DecisionThreshold(double k) {
  const double floor = 16.0 * static_cast<double>(std::numeric_limits<passivedouble>::epsilon());
  return static_cast<passivedouble>(k > floor ? k : floor);
}

/*! \brief (c) Diagnostic tolerance: dp in double builds, dp 2^29 in single precision builds. */
constexpr passivedouble DiagnosticTol(double dp) { return TransferTol(dp, dp * 536870912.0); }
