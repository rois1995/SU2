/*!
 * \file CNativePartition2D.hpp
 * \brief Cost-weighted partition of sensor-bearing native working cells, independent of CFD ownership.
 * \version 8.5.0 "Harrier"
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md)
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */

#pragma once
#include "CNativeDistributed2D.hpp"

namespace SU2NativeBoundary2D {

struct PartitionStats {
  double graph_seconds = 0, partition_seconds = 0, migration_seconds = 0;
  uint64_t moved_cells = 0, edgecut = 0;
  size_t migration_bytes = 0;
  std::vector<uint64_t> cells_before, cells_after, work_before, work_after;
};

// Weights follow map iteration order. Samples of the composed field are temporary;
// the original nodal sensor values and geometry are never changed.
std::vector<uint32_t> WorkWeights(const std::map<Id, Cell>& input, const MetricComposition& composition,
                                  const std::map<int, double>& wallRows);

// Collective on the CFD communicator. Only ownership changes; accepted CFD state
// and frozen sensor values stay intact. False means migration admission failed:
// input remains exactly unchanged on every rank. Graph storage is O(local cells).
bool Repartition(World& world, std::map<Id, Cell>& input, const std::vector<uint32_t>& weights, int parts,
                 PartitionStats& stats, size_t migrationCeiling = SIZE_MAX);

}  // namespace SU2NativeBoundary2D
