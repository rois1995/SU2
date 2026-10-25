/*!
 * \file CNativeNearContainment.hpp
 * \brief DC-16: tolerant donor coverage at the boundary of the original domain. A point that no original donor contains
 *        exactly but that lies within a coordinate-scaled tolerance (1e-12 x max(1, |x|_inf)) of a donor is evaluated in
 *        that donor with clamped barycentric weights. Points on a boundary plane realized off it by rounding (any plane
 *        orientation, also noisy "planes" of an imported mesh) then stay measurable; interior points are unaffected
 *        (exact containment is always tried first). Included only by the field, donor and segment kernels.
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md).
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */
#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include "CNativeMesh3D.hpp"
namespace SU2Native3D {
inline long double NearCoverageTolerance(Point a, Point b) {
  const long double s = std::max({1.L, std::abs((long double)a.x), std::abs((long double)a.y), std::abs((long double)a.z),
                                  std::abs((long double)b.x), std::abs((long double)b.y), std::abs((long double)b.z)});
  return 1e-12L * s;
}
inline long double NearCoverageTolerance(Point a) { return NearCoverageTolerance(a, a); }
/*! Clamped barycentric weights of p in tetrahedron t (long double solve) and the distance from p to the point those
 *  weights represent; infinity for a degenerate tetrahedron. */
inline long double NearWeights(const std::array<Point, 4>& t, Point p, std::array<long double, 4>& w) {
  long double M[3][3], r[3];
  for (int i = 0; i < 3; ++i) {
    const auto& q = t[i + 1];
    M[0][i] = (long double)q.x - t[0].x; M[1][i] = (long double)q.y - t[0].y; M[2][i] = (long double)q.z - t[0].z;
  }
  r[0] = (long double)p.x - t[0].x; r[1] = (long double)p.y - t[0].y; r[2] = (long double)p.z - t[0].z;
  const long double det = M[0][0] * (M[1][1] * M[2][2] - M[1][2] * M[2][1]) - M[0][1] * (M[1][0] * M[2][2] - M[1][2] * M[2][0]) +
                          M[0][2] * (M[1][0] * M[2][1] - M[1][1] * M[2][0]);
  if (!(std::abs(det) > 0)) return INFINITY;
  long double x[3];
  for (int c = 0; c < 3; ++c) {
    long double A[3][3];
    for (int i = 0; i < 3; ++i) for (int j = 0; j < 3; ++j) A[i][j] = j == c ? r[i] : M[i][j];
    x[c] = (A[0][0] * (A[1][1] * A[2][2] - A[1][2] * A[2][1]) - A[0][1] * (A[1][0] * A[2][2] - A[1][2] * A[2][0]) +
            A[0][2] * (A[1][0] * A[2][1] - A[1][1] * A[2][0])) / det;
  }
  w = {1 - x[0] - x[1] - x[2], x[0], x[1], x[2]};
  long double sum = 0;
  for (auto& v : w) { v = std::max(0.L, v); sum += v; }
  if (!(sum > 0)) return INFINITY;
  for (auto& v : w) v /= sum;
  long double q[3] = {0, 0, 0};
  for (int k = 0; k < 4; ++k) { q[0] += w[k] * t[k].x; q[1] += w[k] * t[k].y; q[2] += w[k] * t[k].z; }
  const long double distance = std::sqrt((q[0] - p.x) * (q[0] - p.x) + (q[1] - p.y) * (q[1] - p.y) + (q[2] - p.z) * (q[2] - p.z));
  // review 7: never more than 1e-6 of the donor's longest edge (a translated domain far from the origin, where the
  // coordinate-scaled tolerance would be large, still admits only rounding-level exterior points)
  long double edge = 0;
  for (int i = 0; i < 4; ++i) for (int j = i + 1; j < 4; ++j) {
    const long double dx = (long double)t[i].x - t[j].x, dy = (long double)t[i].y - t[j].y, dz = (long double)t[i].z - t[j].z;
    edge = std::max(edge, std::sqrt(dx * dx + dy * dy + dz * dz));
  }
  return distance <= 1e-6L * edge ? distance : INFINITY;
}
}  // namespace SU2Native3D
