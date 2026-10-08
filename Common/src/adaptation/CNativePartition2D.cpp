/*!
 * \file CNativePartition2D.cpp
 * \brief Distributed cell-dual graph and cost-weighted native working-mesh migration.
 * \version 8.5.0 "Harrier"
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md)
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */

#include "../../include/adaptation/CNativePartition2D.hpp"
#include "../../include/adaptation/CNativeField2D.hpp"
#if defined(HAVE_MPI) && defined(HAVE_PARMETIS)
#include "parmetis.h"
#endif
#include <numeric>

namespace SU2NativeBoundary2D {

std::vector<uint32_t> WorkWeights(const std::map<Id, Cell>& input, const MetricComposition& composition,
                                  const std::map<int, double>& wallRows) {
  std::vector<uint32_t> result;
  result.reserve(input.size());
  for (const auto& entry : input) {
    const auto& cell = entry.second;
    FieldPatch field;
    field.cells.push_back({cell.t, cell.marker, cell.nodal_target});
    field.composition = composition;
    auto sampled = cell;
    CacheTarget(sampled, checked([&](Point p) { return field.evaluate(p); }), 0);
    Tensor sensor{0, 0, 0};
    for (const auto& m : cell.nodal_target) {
      sensor.xx += m.xx / 3;
      sensor.xy += m.xy / 3;
      sensor.yy += m.yy / 3;
    }
    const auto scale = std::max({std::abs(sensor.xx), std::abs(sensor.xy), std::abs(sensor.yy)});
    const long double sensorCells = 4 / std::sqrt(3.L) * area(cell.t) * scale *
                                    std::sqrt(NormalizedDeterminant(sensor.xx, sensor.xy, sensor.yy));
    long double work = 1 + sensorCells;
    const auto largest = *std::max_element(sampled.target_cache.begin() + 1, sampled.target_cache.end());
    const auto smallest = *std::min_element(sampled.target_cache.begin() + 1, sampled.target_cache.end());
    // Actual-query direction/length residuals predict difficulty, without integrating
    // sampled fine wall tensors over the coarse cell's full area.
    work += 8 * (std::max(0., std::log2(largest / 1.8)) +
                 std::max(0., std::log2(.18 / sampled.target_cache[0])) +
                 std::max(0., std::log2(.65 / smallest)));
    for (const auto marker : cell.marker) {
      const auto rows = wallRows.find(marker);
      if (marker && rows != wallRows.end()) work += 2 * rows->second;
    }
    if (!(std::isfinite(work) && work > 0)) throw std::runtime_error("Invalid native partition work estimate.");
    // ponytail: bounded static predictor, not measured transaction cost. Refit only
    // if recorded post-partition rank costs show these weights miss the bottleneck.
    result.push_back(uint32_t(std::ceil(std::min(1024.L, work))));
  }
  return result;
}

namespace {
struct RankWork {
  int rank = 0;
  uint64_t cells = 0, work = 0;
  template <class S> void Fields(S& s) { s(rank, cells, work); }
};
struct DualFace {
  Edge edge;
  uint64_t local = 0, global = 0;
  int owner = 0;
  template <class S> void Fields(S& s) { s(edge.first, edge.second, local, global, owner); }
};
struct Neighbor {
  uint64_t local = 0, global = 0;
  template <class S> void Fields(S& s) { s(local, global); }
};
struct WeightedCell {
  Cell cell;
  uint32_t weight = 0;
  template <class S> void Fields(S& s) { s(cell, weight); }
};
void Loads(World& world, size_t cells, uint64_t work, std::vector<uint64_t>& cellCounts,
           std::vector<uint64_t>& workCounts) {
  cellCounts.resize(world.size);
  workCounts.resize(world.size);
  for (const auto& r : world.metadata(std::vector<RankWork>{{world.rank, cells, work}})) {
    cellCounts[r.rank] = r.cells;
    workCounts[r.rank] = r.work;
  }
}
}  // namespace

bool Repartition(World& world, std::map<Id, Cell>& input, const std::vector<uint32_t>& weights, int parts,
                 PartitionStats& stats, size_t migrationCeiling) {
  CLocalFailure failure;
  if (parts < 1 || parts > world.size || weights.size() != input.size())
    failure.Set(1, 0, "Invalid native working partition controls.");
  for (const auto w : weights)
    if (!w || w > 1024) failure.Set(1, 0, "Native partition weights must be between 1 and 1024.");
  for (const auto& entry : input)
    if (entry.first != entry.second.t.id || !(area(entry.second.t) > 0))
      failure.Set(1, entry.first, "Invalid native cell in working partition.");
  CollectiveFailure(failure, CURRENT_FUNCTION);
  const uint64_t localWork = std::accumulate(weights.begin(), weights.end(), uint64_t(0));
  Loads(world, input.size(), localWork, stats.cells_before, stats.work_before);
  const auto totalCells = std::accumulate(stats.cells_before.begin(), stats.cells_before.end(), uint64_t(0));
  if (totalCells < uint64_t(parts)) failure.Set(1, 0, "Native working partition has fewer cells than partitions.");
  CollectiveFailure(failure, CURRENT_FUNCTION);
  std::vector<int> colors(input.size(), 0);
  if (parts > 1) {
#if defined(HAVE_MPI) && defined(HAVE_PARMETIS)
    const auto graphStarted = world.seconds();
    const auto indexMax = uint64_t(std::numeric_limits<idx_t>::max());
    const auto totalWork = std::accumulate(stats.work_before.begin(), stats.work_before.end(), uint64_t(0));
    if (totalCells > indexMax / 3 || totalWork > indexMax)
      failure.Set(1, 0, "Native working graph exceeds the ParMETIS index/weight range.");
    CollectiveFailure(failure, CURRENT_FUNCTION);
    const auto prefix = std::accumulate(stats.cells_before.begin(), stats.cells_before.begin() + world.rank, uint64_t(0));
    std::vector<std::vector<DualFace>> to(world.size);
    Directory routing(world);
    size_t local = 0;
    for (const auto& entry : input) {
      const auto& t = entry.second.t;
      for (int k = 0; k < 3; ++k) {
        const auto face = edge(t.v[k].id, t.v[(k + 1) % 3].id);
        const auto owner = routing.rendezvous(face.first ^ (face.second + uint64_t(0x9e3779b97f4a7c15ULL)));
        to[owner].push_back({face, local, prefix + local, world.rank});
      }
      ++local;
    }
    auto faces = world.exchange(to);
    std::sort(faces.begin(), faces.end(), [](const auto& a, const auto& b) {
      return std::tie(a.edge, a.global) < std::tie(b.edge, b.global);
    });
    std::vector<std::vector<Neighbor>> reply(world.size);
    for (size_t first = 0; first < faces.size();) {
      size_t last = first + 1;
      while (last < faces.size() && faces[last].edge == faces[first].edge) ++last;
      if (last - first > 2 || (last - first == 2 && faces[first].global == faces[first + 1].global))
        failure.Set(1, faces[first].edge.first, "Nonmanifold or duplicate face in native working graph.");
      else if (last - first == 2) {
        const auto& a = faces[first];
        const auto& b = faces[first + 1];
        reply[a.owner].push_back({a.local, b.global});
        reply[b.owner].push_back({b.local, a.global});
      }
      first = last;
    }
    CollectiveFailure(failure, CURRENT_FUNCTION);
    auto neighbors = world.exchange(reply);
    std::sort(neighbors.begin(), neighbors.end(), [](const auto& a, const auto& b) {
      return std::tie(a.local, a.global) < std::tie(b.local, b.global);
    });
    std::vector<idx_t> xadj(input.size() + 1), adjacency, vwgt, part(input.size());
    adjacency.reserve(neighbors.size());
    vwgt.reserve(2 * input.size());
    size_t next = 0;
    for (size_t i = 0; i < input.size(); ++i) {
      while (next < neighbors.size() && neighbors[next].local == i) {
        adjacency.push_back(idx_t(neighbors[next].global));
        ++next;
      }
      xadj[i + 1] = idx_t(adjacency.size());
      vwgt.push_back(idx_t(weights[i]));
      vwgt.push_back(1);  // Also balance cell storage, independent of predicted work.
    }
    if (next != neighbors.size()) failure.Set(1, 0, "Invalid local ordinal in native working graph.");
    CollectiveFailure(failure, CURRENT_FUNCTION);
    // ParMETIS requires a vertex on every participating rank. Exclude empty CFD
    // partitions from this graph-only communicator; graph ordinals stay unchanged.
    MPI_Comm graphComm = MPI_COMM_NULL;
    MPI_Comm_split(SU2_MPI::GetComm(), input.empty() ? MPI_UNDEFINED : 0, world.rank, &graphComm);
    std::vector<idx_t> vtxdist{0};
    for (const auto n : stats.cells_before)
      if (n) vtxdist.push_back(vtxdist.back() + idx_t(n));
    stats.graph_seconds = world.seconds() - graphStarted;
    const auto partitionStarted = world.seconds();
    idx_t ncon = 2, nparts = parts, edgecut = 0;
    std::vector<real_t> tpwgts(2 * parts, real_t(1. / parts));
    real_t tolerance[2]{1.15, 1.15};
    int error = METIS_OK;
    if (!input.empty()) {
      if (vtxdist.size() == 2) {
        idx_t nvtxs = input.size();
        idx_t options[METIS_NOPTIONS];
        METIS_SetDefaultOptions(options);
        options[METIS_OPTION_SEED] = 0;
        error = METIS_PartGraphKway(&nvtxs, &ncon, xadj.data(), adjacency.data(), vwgt.data(), nullptr, nullptr,
                                  &nparts, tpwgts.data(), tolerance, options, &edgecut, part.data());
      } else {
        idx_t wgtflag = 2, numflag = 0, options[3]{1, 0, 0};
        error = ParMETIS_V3_PartKway(vtxdist.data(), xadj.data(), adjacency.data(), vwgt.data(), nullptr,
                                   &wgtflag, &numflag, &ncon, &nparts, tpwgts.data(), tolerance, options,
                                   &edgecut, part.data(), &graphComm);
      }
      MPI_Comm_free(&graphComm);
    }
    stats.partition_seconds = world.seconds() - partitionStarted;
    if (error != METIS_OK) failure.Set(1, 0, "Native working graph partitioning failed.");
    for (size_t i = 0; i < part.size(); ++i) {
      if (part[i] < 0 || part[i] >= parts) failure.Set(1, 0, "Invalid native working graph color.");
      colors[i] = int(part[i]);
    }
    CollectiveFailure(failure, CURRENT_FUNCTION);
    stats.edgecut = CPassiveComm::Allreduce(uint64_t(edgecut), CPassiveComm::Op::MAX);
#else
    failure.Set(1, 0, "Native weighted working partition requires MPI and ParMETIS.");
    CollectiveFailure(failure, CURRENT_FUNCTION);
#endif
  }
  const auto migrationStarted = world.seconds();
  std::vector<std::vector<WeightedCell>> to(world.size);
  size_t next = 0;
  uint64_t moved = 0;
  for (const auto& entry : input) {
    to[colors[next]].push_back({entry.second, weights[next]});
    moved += colors[next++] != world.rank;
  }
  bool admitted = true;
  // The original input is retained until every rank validates the full replacement.
  const auto baseline = transfer_memory::Add(transfer_memory::Bytes(weights), transfer_memory::Bytes(colors),
      transfer_memory::Mul(input.size(), sizeof(std::map<Id, Cell>::value_type) + 4 * sizeof(void*)));
  auto received = world.exchange(to, &admitted, migrationCeiling, baseline);
  stats.migration_bytes = world.max_exchange_work_bytes;
  if (!admitted) {
    stats.migration_seconds = world.seconds() - migrationStarted;
    return false;
  }
  const auto staging = transfer_memory::Add(baseline, transfer_memory::Bytes(to), transfer_memory::Bytes(received),
      transfer_memory::Mul(received.size(), sizeof(std::map<Id, Cell>::value_type) + 4 * sizeof(void*)));
  stats.migration_bytes = std::max(stats.migration_bytes, staging);
  if (world.maximum(staging > migrationCeiling)) {
    stats.migration_seconds = world.seconds() - migrationStarted;
    return false;
  }
  std::map<Id, Cell> replacement;
  uint64_t receivedWork = 0;
  try {
    for (auto& record : received) {
      if (!replacement.emplace(record.cell.t.id, std::move(record.cell)).second)
        failure.Set(1, 0, "Duplicate native cell after working migration.");
      receivedWork += record.weight;
    }
  } catch (const std::bad_alloc&) {
    failure.Set(2, 0, "Native working migration allocation failed.");
  }
  Loads(world, replacement.size(), receivedWork, stats.cells_after, stats.work_after);
  if (std::accumulate(stats.cells_after.begin(), stats.cells_after.end(), uint64_t(0)) != totalCells ||
      std::accumulate(stats.work_after.begin(), stats.work_after.end(), uint64_t(0)) !=
          std::accumulate(stats.work_before.begin(), stats.work_before.end(), uint64_t(0)))
    failure.Set(1, 0, "Native working migration changed the cell count or predicted work.");
  CollectiveFailure(failure, CURRENT_FUNCTION);
  input.swap(replacement);
  stats.moved_cells = CPassiveComm::Allreduce(moved, CPassiveComm::Op::SUM);
  stats.migration_seconds = world.seconds() - migrationStarted;
  return true;
}

}  // namespace SU2NativeBoundary2D
