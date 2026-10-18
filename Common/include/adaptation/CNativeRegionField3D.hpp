/*!
 * \file CNativeRegionField3D.hpp
 * \brief Bounded original-field query service for one admitted region on an explicit MPI world.
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md).
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */
#pragma once
#include "CNativeDonor3D.hpp"
#include "CNativeRegion3D.hpp"
#include <deque>
namespace SU2Native3D {
constexpr size_t RegionFieldWorkspaceBytes =
    2 * MaximumFrozenFieldBytes + 4 * MaximumDonors * sizeof(DonorCell) + MaximumSensorSamples * 512 + 65536;
struct RegionFieldStats {
  uint64_t point_calls = 0, edge_calls = 0, sensor_hits = 0, interior_hits = 0, edge_hits = 0, imports = 0, sampled_points = 0;
  size_t maximum_resident = 0;
};
/*! All ranks call Run together on their existing MPI thread. Rank0 alone executes the private
 * constructor. A cache miss issues a matching canonical-point or positive-interval import command to every rank;
 * other ranks serve until rank0 closes the protocol, including after exceptions. No global donor
 * gather or extra MPI thread. Point memoization stores only the original sensor; composition
 * runs at each actual point query. Scalar edge reuse requires identical endpoints and frozen policy. */
class RegionFieldService {
 public:
  RegionFieldService(World& world, DonorImport& source, const PlanarReference& geometry, MetricComposition composition,
                     MetricEdgeBreakpoints bands, bool timings, size_t ceiling, size_t caller_bytes,
                     std::function<void(const FrozenField&)> retired = {}, std::function<void()> checkpoint = {});
  void Run(const std::function<void(const RegionMetric&)>& root, const std::vector<DonorCell>& seed = {});
  const RegionFieldStats& Statistics() const { return stats; }

 private:
  using Key = std::array<double, 3>;
  using Segment = std::array<Point, 2>;
  using EdgeKey = std::array<Key, 2>;
  World& world;
  DonorImport& source;
  const PlanarReference& geometry;
  MetricComposition composition;
  MetricEdgeBreakpoints bands;
  bool timings;
  size_t ceiling, caller_bytes;
  std::function<void(const FrozenField&)> retired;
  std::function<void()> checkpoint;
  RegionFieldStats stats;
  std::vector<DonorCell> resident;
  std::unique_ptr<FrozenField> field;
  std::map<Key, Tensor> sensors;
  std::deque<Key> sensor_order;
  std::map<EdgeKey, long double> edges;
  std::deque<EdgeKey> edge_order;
  DonorPatch Fetch(const Segment* segment);
  void Load(std::vector<DonorCell> records);
  Tensor PointMetric(Point point);
  long double EdgeLength(Point a, Point b);
  void Retire();
};
}  // namespace SU2Native3D
