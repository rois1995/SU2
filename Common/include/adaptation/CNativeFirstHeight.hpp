/*!
 * \file CNativeFirstHeight.hpp
 * \brief Shared first wall-normal height gate of the native 2D/3D remeshers (ADAP_BL_FIRST_HEIGHT_TOL).
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md).
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */
#pragma once
#include <cmath>
#include <map>
#include <stdexcept>
#include <utility>

namespace SU2NativeFirstHeight {
/*! Default relative tolerance T of |h-h0|/h0, equal to the ADAP_BL_FIRST_HEIGHT_TOL default. */
constexpr double DefaultTolerance = 0.10;
/*! A configured tolerance must lie strictly between zero and one. */
inline double Checked(double tolerance) {
  if (!(tolerance > 0 && tolerance < 1))
    throw std::invalid_argument("Native first-height tolerance must lie strictly between 0 and 1.");
  return tolerance;
}
/*! Inclusive gate |h-h0|/h0 <= T, compared in long double; a nonfinite error fails. */
inline bool Within(long double relative_error, double tolerance) {
  return relative_error <= static_cast<long double>(tolerance);
}
}  // namespace SU2NativeFirstHeight

namespace SU2Native3D {
/*! Frozen per-marker first heights h0 and their shared relative tolerance T. Empty releases the first-row
 * contract. There is deliberately no conversion from a bare height map: every caller states T explicitly
 * or takes the documented default of a default-constructed policy. */
struct WallHeights : std::map<int, double> {
  using std::map<int, double>::map;
  using std::map<int, double>::operator=;
  double tolerance = SU2NativeFirstHeight::DefaultTolerance;
  WallHeights() = default;
  WallHeights(std::map<int, double> heights, double t) : std::map<int, double>(std::move(heights)), tolerance(t) {}
};
}  // namespace SU2Native3D
