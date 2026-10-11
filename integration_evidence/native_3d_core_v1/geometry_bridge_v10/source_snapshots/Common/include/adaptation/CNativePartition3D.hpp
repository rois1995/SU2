/*!
 * \file CNativePartition3D.hpp
 * \brief Cost-weighted tetrahedral working partition independent of accepted CFD ownership.
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md).
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */
#pragma once
#include "CNativeDistributed3D.hpp"
#include "CNativeField3D.hpp"
namespace SU2Native3D {
constexpr uint32_t MaximumPartitionWeight = 1024;
struct PartitionStats {
  double total_seconds = 0, graph_seconds = 0, partition_seconds = 0, migration_seconds = 0, other_seconds = 0;
  double directory_seconds = 0;  // Nested in graph preparation, including identity/coordinate admission.
  uint64_t moved_cells = 0, edgecut = 0, graph_arcs = 0;
  size_t maximum_native_bytes = 0;
  std::vector<uint64_t> cells_before, cells_after, work_before, work_after;
  std::vector<int> destinations;  // Original local map order; permits co-location of an initial matching donor copy.
};
/*! Bounded existing-cell sample batch (<=128), preserving order. Sensor-only centroid metric-volume predicts
 * unit tetrahedra; actual composed centroid/vertex demands predict refinement/coarsening/shape/sliver work.
 * The caller imports the original sensor support. No composed nodal tensor is spread over cell volume.
 * These static weights neither alter the target nor certify a metric edge integral or predict measured CPU. */
std::vector<uint32_t> WorkWeights(const std::vector<Cell>& cells, FrozenField& field);
/*! Collective on the explicit world (possibly a subset). Builds triangular-face dual adjacency, excludes empty
 * graph ranks, balances predicted work and cell storage, then stages all migrated records before publication.
 * IDs/version/geometry/physical bindings remain byte-identical. Original sensor/CFD solution/history stores
 * are caller-owned and untouched; original donors must separately be made available to M workers.
 * False returns the elected reason and leaves input unchanged on every rank, including storage failures.
 * ceiling/caller_bytes bound requested native storage, not allocator/RSS or internal (Par)METIS workspace. */
bool Repartition(World& world, Owned& input, const std::vector<uint32_t>& weights, int parts, PartitionStats& stats,
                 std::string& reason, size_t ceiling = SIZE_MAX, size_t caller_bytes = 0);
}  // namespace SU2Native3D
