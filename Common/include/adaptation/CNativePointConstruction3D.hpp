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

enum class SegmentFreedom { Unresolved, None, MidpointOnly };
/*! Exact conservative binary64 subdivision certificate. None forbids any interior point;
 * MidpointOnly writes the unique interior point. Unresolved makes no feasibility claim
 * (including axis lattices wider than64bits or with GCD>2). Finite distinct endpoints required. */
SegmentFreedom Binary64SegmentFreedom(Point a,Point b,Point& midpoint);
}
