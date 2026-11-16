/*!
 * \file CNativeRequirements3D.hpp
 * \brief Read-only adaptation requirement data for Native3D diagnostics.
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#pragma once
#include "CNativeFirstHeight.hpp"
#include "CNativeRegion3D.hpp"
#include <limits>
#include <map>
#include <cstddef>
#include <iosfwd>

namespace SU2Native3D {
struct EngineOptions;

struct MetricSizeControls {
  double hmin = std::numeric_limits<double>::quiet_NaN();
  double hmax = std::numeric_limits<double>::quiet_NaN();
  double armax = std::numeric_limits<double>::quiet_NaN();
  double hgrad = std::numeric_limits<double>::quiet_NaN();
  double complexity = std::numeric_limits<double>::quiet_NaN();
};

/*! Scalar snapshot with wall maps borrowed from the source EngineOptions lifetime. */
struct AdaptationRequirements {
  struct Size {
    MetricSizeControls controls;
    bool composition_present = false;
  } size;
  struct Geometry {
    bool mapped = false, entity_geometry = false, curved = false;
    double tolerance = std::numeric_limits<double>::quiet_NaN();
  } geometry;
  struct Features {
    bool policy_present = false;
    size_t features = 0, corners = 0, sheets = 0;
    double angle = std::numeric_limits<double>::quiet_NaN();
  } features;
  struct Walls {
    const WallHeights* first_heights = nullptr;
    const std::map<int, RegionWallPolicy>* region_policies = nullptr;
    double height_tolerance = std::numeric_limits<double>::quiet_NaN();
  } walls;
  struct Budgets {
    size_t maximum_work = 0;
    double maximum_seconds = 0;
    size_t memory_ceiling = 0, job_memory_limit = 0;
  } budgets;
};

class PlanarReference;
AdaptationRequirements MakeAdaptationRequirements(const PlanarReference&, bool, const EngineOptions&);
void PrintAdaptationRequirements(const AdaptationRequirements&, int rank, std::ostream&);

}  // namespace SU2Native3D
