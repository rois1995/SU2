/*!
 * \file CNativeMesh3D.hpp
 * \brief Passive geometric and metric measures for the native tetrahedral kernel.
 * \version 8.5.0 "Harrier"
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md).
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */
#pragma once
#include <array>
#include <cstdint>
#include <vector>

namespace SU2Native3D {
struct Point {
  double x = 0, y = 0, z = 0;
  template <class Stream>
  void Fields(Stream& s) {
    s(x, y, z);
  }
};
struct Tensor {
  double xx = 1, xy = 0, xz = 0, yy = 1, yz = 0, zz = 1;
  template <class Stream>
  void Fields(Stream& s) {
    s(xx, xy, xz, yy, yz, zz);
  }
};
using Id = uint64_t;
struct Node {
  Id id = 0;
  Point p;
  template <class Stream>
  void Fields(Stream& s) {
    s(id, p);
  }
};
struct Cell {
  Id id = 0;
  std::array<Node, 4> v;
  template <class Stream>
  void Fields(Stream& s) {
    s(id, v);
  }
};
using Tetrahedron = std::array<Point, 4>;
/*! Actual binary64 centroid used by metric checks and their donor footprint. */
Point Centroid(const Cell& cell);
struct KernelStats {
  uint64_t filtered_orientations = 0, exact_orientations = 0, measured_cells = 0;
  uint64_t filtered_norms = 0, exact_norms = 0;
  uint64_t segment_separated = 0;  // Strict face-plane rejects before rational interval construction.
};
/*! Exact sign for finite binary64 inputs; magnitude rounded to long double. */
long double Orientation(Point a, Point b, Point c, Point d, KernelStats* stats = nullptr);
/*! Exact orientation sign at the mathematical centroid of three finite binary64 points; no rounded centroid. */
int OrientationCentroidSign(Point a,Point b,const std::array<Point,3>& triangle,Point d,KernelStats* stats=nullptr);
/*! Positive donor, exact containment signs and magnitude-controlled P1 weights; outside leaves weights unchanged. */
bool Barycentric(const Tetrahedron& donor, Point point, std::array<long double, 4>& weights,
                 KernelStats* stats = nullptr);
/*! Original P1 weights at the exact midpoint of binary64 endpoints, including closed boundary ties. */
bool BarycentricMidpoint(const Tetrahedron& donor,Point a,Point b,std::array<long double,4>& weights,KernelStats* stats=nullptr);
/*! Exact closed segment/tetrahedron intersection, including endpoint-only and zero-length point queries.
 * Positive finite donor required; no rounded interval roots or containment tolerance. */
bool IntersectsSegment(const Tetrahedron& donor, Point a, Point b, KernelStats* stats = nullptr);
/*! Positive-length intersection, including face/edge intervals; isolated contact is excluded. */
bool IntersectsSegmentPositiveLength(const Tetrahedron& donor, Point a, Point b, KernelStats* stats = nullptr);
/*! Exact tetrahedral separation/contact. Interiors must be disjoint, and every closed contact must
 * lie in the simplex of shared node identities with identical coordinates. Unrelated tangencies reject. */
bool ConformingDisjoint(const Cell& a, const Cell& b, KernelStats* stats = nullptr);
/*! Finite components and exact Sylvester signs; no tensor repair or condition-number clipping. */
void ValidateTensor(const Tensor& tensor, KernelStats* stats = nullptr);
/*! sqrt(det(M)) with magnitude-controlled exact determinant fallback; never repairs the supplied tensor. */
double MetricDensity(const Tensor& tensor,KernelStats* stats=nullptr,long double* condition=nullptr);
/*! Positive tetrahedral volume with magnitude-controlled determinant evaluation. */
long double TetrahedronVolume(const Tetrahedron& tet,KernelStats* stats=nullptr);
constexpr size_t MaximumSegmentDonors = 256;
struct SegmentPiece {
  size_t donor = 0;
  long double begin = 0, end = 1, width = 1;
  std::array<long double, 4> begin_weights{}, end_weights{};
};
/*! Exact rational original-cell clipping/order/coverage; canonical input order breaks conformal face/edge ties.
 * Requires an embedded conforming source in each priority group. With primary_donors<size, the prefix is
 * authoritative wherever covered; the suffix fills uncovered intervals only. Ambiguous same-priority overlaps
 * reject. Exact endpoint weights and width avoid cancellation of near-equal cuts. */
std::vector<SegmentPiece> TraceSegment(const std::vector<const Cell*>& donors, Point a, Point b,
                                       KernelStats* stats = nullptr, size_t primary_donors = SIZE_MAX);
/*! Magnitude-controlled metric norm of the actual binary64 chord; tensor must be SPD. */
long double MetricNorm(Point a, Point b, const Tensor& metric, KernelStats* stats = nullptr);
struct MetricMeasures {
  double mean_ratio = 0, minimum_scaled_jacobian = 0, rms_edge = 0, maximum_edge = 0;
};
/*! Constant actual-query tensor supplied by the caller; no nodal BL averaging or target modification. */
MetricMeasures Measure(const Tetrahedron& tet, const Tensor& metric, KernelStats* stats = nullptr);

/*! Initial frozen 3D shape/sliver gates; acceptance additionally needs geometry, topology and query edge checks. */
constexpr double MinimumMeanRatio = .20;
constexpr double MinimumScaledJacobian = .05;
}  // namespace SU2Native3D
