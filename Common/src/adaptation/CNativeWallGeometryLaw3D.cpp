/*!
 * \file CNativeWallGeometryLaw3D.cpp
 * \brief Boundary-parameter wall geometry edge-need estimates.
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#include "../../include/adaptation/CNativeWallGeometryLaw3D.hpp"
#include "../../include/adaptation/CNativeBoundary3D.hpp"

#include <algorithm>
#include <cmath>
#include <exception>

namespace SU2Native3D {

std::set<Id> WallEligibleSides(const std::set<Id>& supports, const std::set<Id>& anchors,
                               const BoundaryParameterState& parameters) {
  std::set<Id> eligible;
  const auto authority = parameters.Authority();
  for (Id side : supports) {
    for (Id anchor : anchors) {
      const auto& side_facet = authority->Reference().Get(side);
      const auto& anchor_facet = authority->Reference().Get(anchor);
      if (side_facet.marker != anchor_facet.marker) continue;
      if (parameters.Entity(side) != parameters.Entity(anchor)) continue;
      eligible.insert(side);
      break;
    }
  }
  return eligible;
}

double WallEndpointNeedScore(const SU2NativeGeometry::MappedFacetReference::SurfaceEvaluation& evaluation,
                             const Point& chord) {
  const double d1 =
      evaluation.direction1.x * chord.x + evaluation.direction1.y * chord.y + evaluation.direction1.z * chord.z;
  const double d2 =
      evaluation.direction2.x * chord.x + evaluation.direction2.y * chord.y + evaluation.direction2.z * chord.z;
  const double curvature = std::abs(evaluation.curvature1) * d1 * d1 + std::abs(evaluation.curvature2) * d2 * d2;
  const double normal =
      std::abs(evaluation.normal.x * chord.x + evaluation.normal.y * chord.y + evaluation.normal.z * chord.z);
  return std::max(curvature / 8, normal / 4);
}

WallEndpointScore ScoreWallEndpoint(const std::set<Id>& supports, const std::set<Id>& anchors,
                                    const BoundaryParameterState& parameters,
                                    const SU2NativeGeometry::SurfaceBinding& binding, const Point& chord) {
  WallEndpointScore result;
  std::set<Id> eligible;
  try {
    eligible = WallEligibleSides(supports, anchors, parameters);
  } catch (const std::exception&) {
    result.unsupported = 1;
    return result;
  }
  const auto authority = parameters.Authority();
  if (eligible.empty()) {
    result.unsupported = 1;
    return result;
  }
  for (Id side : eligible) {
    try {
      const auto evaluation = authority->EvaluateSurface(side, binding);
      const double score = WallEndpointNeedScore(evaluation, chord);
      if (!std::isfinite(score)) {
        ++result.unsupported;
        continue;
      }
      ++result.supported;
      if (score > result.score) {
        result.score = score;
        result.limiting_side = side;
      }
    } catch (const std::exception&) {
      ++result.unsupported;
    }
  }
  return result;
}

WallEdgeEstimate EstimateWallEdge(const BoundaryParameterState& parameters, const Node& a, const Node& b, Id anchor) {
  const auto* point_a = parameters.Find(a.id);
  const auto* point_b = parameters.Find(b.id);
  if (!point_a || !point_b) return {};
  const Point chord{b.p.x - a.p.x, b.p.y - a.p.y, b.p.z - a.p.z};
  if (!std::isfinite(chord.x) || !std::isfinite(chord.y) || !std::isfinite(chord.z)) return {};
  const auto authority = parameters.Authority();
  std::set<Id> supports_a, supports_b;
  try {
    authority->ForFacets(point_a->binding, [&](Id side) { supports_a.insert(side); });
    authority->ForFacets(point_b->binding, [&](Id side) { supports_b.insert(side); });
  } catch (const std::exception&) {
    return {};
  }
  const std::set<Id> anchors{anchor};
  const auto score_a = ScoreWallEndpoint(supports_a, anchors, parameters, point_a->binding, chord);
  const auto score_b = ScoreWallEndpoint(supports_b, anchors, parameters, point_b->binding, chord);
  if (score_a.supported == 0 || score_b.supported == 0 || score_a.unsupported != 0 || score_b.unsupported != 0) {
    return {};
  }
  return {true, std::max(score_a.score, score_b.score)};
}

bool EstimateSaysNeed(const WallEdgeEstimate& estimate, double tau) {
  return estimate.supported && estimate.value > .75 * tau;
}

}  // namespace SU2Native3D
