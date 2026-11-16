/*!
 * \file CNativeWallGeometryLaw3D.hpp
 * \brief Boundary-parameter wall geometry edge-need estimates.
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#pragma once

#include "CNativeBoundaryParameters3D.hpp"

namespace SU2Native3D {

struct WallEndpointScore {
  size_t supported = 0;
  size_t unsupported = 0;
  double score = 0;
  Id limiting_side = 0;
};

struct WallEdgeEstimate {
  bool supported = false;
  double value = 0;
};

std::set<Id> WallEligibleSides(const std::set<Id>& supports, const std::set<Id>& anchors,
                               const BoundaryParameterState& parameters);
double WallEndpointNeedScore(const SU2NativeGeometry::MappedFacetReference::SurfaceEvaluation& evaluation,
                             const Point& chord);
WallEndpointScore ScoreWallEndpoint(const std::set<Id>& supports, const std::set<Id>& anchors,
                                    const BoundaryParameterState& parameters,
                                    const SU2NativeGeometry::SurfaceBinding& binding, const Point& chord);
WallEdgeEstimate EstimateWallEdge(const BoundaryParameterState& parameters, const Node& a,
                                  const Node& b, Id anchor);
bool EstimateSaysNeed(const WallEdgeEstimate& estimate, double tau);

}  // namespace SU2Native3D
