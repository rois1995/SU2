/*!
 * \file CAccurateSum.cpp
 * \brief Compensated (Neumaier) sums. This unit is compiled without fast math and without floating-point contraction
 *        (its own static library, see Common/src/meson.build); nothing here may be moved to a header.
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

#include "../../include/adaptation/CAccurateSum.hpp"

#include <cfloat>
#include <cmath>
#include <limits>

#if defined(__FAST_MATH__) || defined(__ASSOCIATIVE_MATH__)
#error "CAccurateSum.cpp must be compiled without fast math (-fno-fast-math): the compensation terms would be lost."
#endif

#if defined(__GNUC__)
#define SU2_ACCURATE_NOINLINE __attribute__((noinline))
#else
#define SU2_ACCURATE_NOINLINE
#endif

namespace {

constexpr double kMaxAbsSum = DBL_MAX / 4;

/*--- One Neumaier step: s + c accumulates x. ---*/
inline void Step(double x, double& s, double& c) {
  const double t = s + x;
  if (std::fabs(s) >= std::fabs(x)) {
    c += (s - t) + x;
  } else {
    c += (x - t) + s;
  }
  s = t;
}

}  // namespace

SU2_ACCURATE_NOINLINE bool CAccurateSum::Local(const double* x, size_t n, size_t stride, double* triple,
                                               size_t* firstBad) {
  double s = 0.0, c = 0.0, a = 0.0;
  size_t bad = n;
  for (size_t i = 0; i < n; ++i) {
    const double xi = x[i * stride];
    if (!std::isfinite(xi) && bad == n) bad = i;
    Step(xi, s, c);
    a += std::fabs(xi);
  }
  if (firstBad != nullptr) *firstBad = bad;
  const bool ok = bad == n && std::isfinite(s) && std::isfinite(c) && std::isfinite(a) && a < kMaxAbsSum;
  if (!ok) {
    const double nan = std::numeric_limits<double>::quiet_NaN();
    s = c = a = nan;
  }
  triple[0] = s;
  triple[1] = c;
  triple[2] = a;
  return ok;
}

SU2_ACCURATE_NOINLINE bool CAccurateSum::Merge(const double* triples, size_t nRank, double* result,
                                               double* absSum) {
  double s = 0.0, c = 0.0, a = 0.0;
  bool ok = true;
  for (size_t r = 0; r < nRank; ++r) {
    const double* t = triples + 3 * r;
    ok = ok && std::isfinite(t[0]) && std::isfinite(t[1]) && std::isfinite(t[2]);
    Step(t[0], s, c);
    Step(t[1], s, c);
    a += t[2];
    ok = ok && std::isfinite(s) && std::isfinite(c);
  }
  const double sum = s + c;
  ok = ok && std::isfinite(sum) && std::isfinite(a) && a < kMaxAbsSum;
  *result = ok ? sum : std::numeric_limits<double>::quiet_NaN();
  if (absSum != nullptr) *absSum = a;
  return ok;
}

SU2_ACCURATE_NOINLINE bool CAccurateSum::MergePairs(const double* pairs, size_t nPair, double* result) {
  double s = 0.0, c = 0.0;
  bool ok = true;
  for (size_t i = 0; i < 2 * nPair; ++i) {
    ok = ok && std::isfinite(pairs[i]);
    Step(pairs[i], s, c);
  }
  const double sum = s + c;
  ok = ok && std::isfinite(s) && std::isfinite(c) && std::isfinite(sum);
  *result = ok ? sum : std::numeric_limits<double>::quiet_NaN();
  return ok;
}

SU2_ACCURATE_NOINLINE void CNeumaierSum::Add(double x) {
  Step(x, s, c);
  a += std::fabs(x);
}

SU2_ACCURATE_NOINLINE double CNeumaierSum::Sum() const { return s + c; }
