/*!
 * \file CNativeEngine3D.hpp
 * \brief Incremental synchronous tetrahedral refinement on explicit worker communicators.
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md).
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */
#pragma once
#include "CNativeDonor3D.hpp"
#include "CNativeMetric3D.hpp"
namespace SU2Native3D {
struct EngineOptions {
  size_t maximum_rounds = 512, memory_ceiling = size_t(2) << 30, caller_bytes = 0;
  bool timings = false;
};
struct SplitRecord {
  Edge edge{};
  Node point;
};
struct EngineStats {
  uint64_t rounds = 0, proposed = 0, conflicts = 0, attempted = 0, accepted = 0, rejected = 0;
  uint64_t initial_cells = 0, updated_cells = 0, edge_evaluations = 0, selections = 0;  // Edge evaluations here count selection-cache measurements only.
  uint64_t field_requests = 0, field_hits = 0, field_compositions = 0, field_edges = 0;
  uint64_t donor_batches = 0, batch_retries = 0;
  double process_cpu_seconds = 0, reconstruction_cpu_seconds = 0, metric_cpu_seconds = 0, measurement_cpu_seconds = 0;
  // Nested query detail; never add to the exclusive engine phases below.
  double field_build_seconds = 0, field_query_seconds = 0, field_cache_seconds = 0, field_search_seconds = 0;
  double field_interpolation_seconds = 0, field_composition_seconds = 0;
  double edge_seconds = 0, edge_search_seconds = 0, edge_trace_seconds = 0, edge_integral_seconds = 0;
  size_t maximum_work_bytes = 0, maximum_owned = 0;
  double total_seconds = 0, initialization_seconds = 0, seed_seconds = 0, selection_seconds = 0;
  double dependency_seconds = 0, reservation_seconds = 0, donor_seconds = 0;
  double reconstruction_seconds = 0, metric_seconds = 0, routing_seconds = 0;
  double publication_seconds = 0, update_seconds = 0, completion_seconds = 0, other_seconds = 0;
};
struct EngineResult {
  Owned cells;  // Private result; on failure this is rejected progress, never an accepted CFD replacement.
  std::vector<SplitRecord> splits;
  bool valid = false;
  std::string reason;
  EngineStats stats;
};
/*! Complete metric refinement only; coarsening/movement/reconnection are separate required developments.
 * All world ranks call together; accepted and original source remain unchanged. Same options/immutable geometry
 * authority and frozen composition policy on every worker; source must cover the original accepted domain.
 * AABB donor/dependency limits can reject the whole event. Planar split geometry is an exact subdivision of
 * admitted old cells, preserving the domain; caller supplies embedded conforming accepted input and facet authority.
 * Success means all working cells satisfy the frozen metric gates, not CFD/CGNS/partition qualification.
 * Each instance/call runs on one thread per worker. Byte models describe requested storage, not allocator/RSS.
 */
EngineResult Refine(World& world, const Owned& accepted, const std::vector<DonorCell>& original,
                    const PlanarReference& geometry, MetricComposition composition = {}, EngineOptions options = {});
}  // namespace SU2Native3D
