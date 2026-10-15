/*!
 * \file CNativeRegion3D.hpp
 * \brief Coordinated surface, normal wall band and transition construction.
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md).
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */
#pragma once
#include "CNativeBoundary3D.hpp"
#include "CNativeField3D.hpp"
namespace SU2Native3D {
struct RegionWallPolicy {
  double height = 0, growth = 0, thickness = 0;
  template <class S>
  void Fields(S& s) {
    s(height, growth, thickness);
  }
};
struct RegionLimits {
  size_t maximum_cells = 0, maximum_work = 0;
  std::function<void(const Patch&, const std::string&)> rejected;
  std::function<void()> checkpoint;  // Caller enforces event work, time and memory before accepting output.
};
struct RegionReport {
  size_t surface_splits = 0, surface_collapses = 0, surface_trials = 0, junction_trials = 0;
  size_t generated_cells = 0;
  bool exhausted_work = false;
  double minimum_quality = 1, minimum_jacobian = 1;
  long double maximum_edge = 0;
};
/*! Private whole-region construction, independent of primitive cavity limits. Declared complete radial
 * wall and planar caps must enclose a visible ball; other geometries explicitly reject. No accepted
 * state is mutated. Empty height policies omit the normal band and test the metric-driven core. New IDs start at
 * caller-reserved global maxima; unchanged retained nodes keep IDs. Uses actual original-P1/composed queries
 * throughout, followed by whole-region embedding/height/metric validation. Distributed dependency admission, policy
 * agreement and atomic publication belong to caller. */
bool ConstructWallRegion(const std::vector<Facet>& skin, Point kernel, const PlanarReference& geometry,
                         const std::map<int, RegionWallPolicy>& policies, double surface_tolerance, Id first_node,
                         Id first_cell, FrozenField& target, const RegionLimits& limits, Patch& output,
                         RegionReport& report, std::string& reason);
}  // namespace SU2Native3D
