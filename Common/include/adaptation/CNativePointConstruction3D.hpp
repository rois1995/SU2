/*!
 * \file CNativePointConstruction3D.hpp
 * \brief Exact rounding bounds for source-consistent binary64 coordinate construction.
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#pragma once
#include "CNativeMesh3D.hpp"
namespace SU2Native3D {
/*! Componentwise adjacent binary64 bounds of the EXACT affine point (1-t)*a+t*b.
 * Finite endpoints and binary64 t in[0,1]; includes cancellation and subnormals.
 * Bounds authorize coordinate proposals only, never relaxed containment predicates. */
std::array<Point,2> AffinePointBounds(Point a,Point b,double t);

/*! Ideal affine point origin + u*(first-origin) + v*(second-origin), evaluated over
 * exact binary64 real values. Dependent coefficients must never be rounded separately. */
struct ParametricPoint {
  Point origin{},first{},second{};
  double u=0,v=0;
};
/*! Adjacent binary64 coordinate bounds of the exact ideal point; no containment relaxation. */
std::array<Point,2> ParametricPointBounds(const ParametricPoint& point);
/*! Exact sign in the chart obtained by dropping axis0/1/2 (YZ/XZ/XY).
 * The ideal coordinates remain unrounded, including exact feature incidences. */
int ParametricOrientation(const ParametricPoint& a,const ParametricPoint& b,const ParametricPoint& c,
                          unsigned axis,KernelStats* stats=nullptr);

enum class SegmentFreedom { Unresolved, None, MidpointOnly };
/*! Exact conservative binary64 subdivision certificate. None forbids any interior point;
 * MidpointOnly writes the unique interior point. Unresolved makes no feasibility claim
 * (including axis lattices wider than64bits or with GCD>2). Finite distinct endpoints required. */
SegmentFreedom Binary64SegmentFreedom(Point a,Point b,Point& midpoint);
}
