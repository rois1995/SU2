/*!
 * \file CNativeImport3D.hpp
 * \brief Owned SU2 tetrahedra, immutable faceted authority and distributed reader return.
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md).
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */
#pragma once
#include "CNativeDonor3D.hpp"
#include "CNativePartition3D.hpp"
#include "CReaderSlices.hpp"
#include "../containers/C2DContainer.hpp"
class CConfig;
class CGeometry;
namespace SU2Native3D {
struct ReferenceState {
  std::shared_ptr<const PlanarReference> original;
  std::vector<Facet> facets;
  std::vector<std::string> marker_names;
};
struct BridgeStats {
  double total_seconds = 0, points_seconds = 0, cells_seconds = 0, faces_seconds = 0;
  double source_seconds = 0, query_seconds = 0, reader_seconds = 0, reduction_seconds=0, other_seconds = 0;
  uint64_t batches = 0, batch_retries = 0, requests = 0, hits = 0, misses = 0, compositions = 0;
  uint64_t invalid_queries = 0, failures = 0, evictions = 0;
  uint64_t field_builds = 0, source_point_queries = 0, source_point_payloads = 0;
  double source_point_seconds = 0, source_point_search_seconds = 0, source_point_interpolation_seconds = 0;
  double field_build_seconds = 0, field_search_seconds = 0, interpolation_seconds = 0, composition_seconds = 0;
  // World-wrapper payload includes self traffic; calls/time omit direct MPI/vendor work and are not MPI wait.
  uint64_t tracked_bytes = 0, tracked_calls = 0;
  double tracked_collective_seconds = 0;
  size_t maximum_native_bytes = 0;
};
/*! Native requested-storage models, including map nodes and immutable reference buffers. */
size_t OwnedStorageBytes(const Owned& cells);
size_t ReferenceStorageBytes(const ReferenceState& reference, bool include_authority = true);
struct ImportedGeometry {
  Owned cells;
  std::vector<DonorCell> original;  // Frozen owner-sourced sensors; never changed with working connectivity.
  BridgeStats stats;
};
/*! Collective on the CFD communicator. Reads actual owner coordinates/sensors, checks halos and matches all
 * triangular volume faces against uniquely owned physical labels. Initializes a bounded closed immutable
 * reference once, then rebinds by finite oriented geometric containment after reader renumbering.
 * The accepted geometry and metric are read-only. A null metric uses identity for geometry preflight.
 * Source embedding is an accepted-CFD-mesh precondition; independent overlap audits remain necessary. */
ImportedGeometry ImportGeometry(const CConfig& config, const CGeometry& geometry, const su2activematrix* metric,
                                ReferenceState& reference);
/*! Recoverable collective storage admission. Output and reference remain unchanged on rejection.
 * stats describes the attempted import. Bounds cover house point directory/face exchange payloads and
 * construction scratch; CFD, allocator/RSS and MPI-internal storage are separate. */
bool ImportGeometry(const CConfig& config, const CGeometry& geometry, const su2activematrix* metric,
                    ReferenceState& reference, ImportedGeometry& output, std::string& reason, BridgeStats& stats,
                    size_t ceiling, size_t caller_bytes = 0);
/*! A complete proposal covers every candidate cell. Uncertainty is an empirical indicator, not a bound. */
struct ComplexityReassessment3D {
  double value=0,uncertainty=INFINITY,standard_error=0,block_standard_error=0,centroid_estimate=0;
  uint64_t samples=0;
  bool estimated=false;
  std::string reason;
};
/*! Reassess the SAME original P1/actual-query composition. Candidate-cell densities only select sample
 * probabilities; they never replace the source. Resource/time limits retain an explicit unknown indicator;
 * invalid field/source/protocol errors return false. Output is unchanged on such errors. */
bool ReassessComplexity(World& parent,const Owned& candidate,DonorImport& original,
    const PlanarReference& reference,MetricComposition composition,ComplexityReassessment3D& output,
    std::string& reason,BridgeStats& stats,size_t ceiling=size_t(2)<<30,size_t caller_bytes=0,
    uint64_t count=8192,uint64_t seed=20261017,std::function<bool()> withinBudget={});
/*! Bounded point-support batches on parent (M<=N return uses empty inputs on inactive ranks).
 * Weight and reader functions leave their output unchanged on elected failures. Each cache hit composes
 * at the actual query. No nodal geometric BL tensor is spread into the original source.
 * Native byte models exclude CFD allocations, allocator/RSS and reader/partitioner internal workspaces.
 * ReaderSlices must run on the actual CFD communicator, since the house reader uses that communicator. */
bool WorkingWeights(World& parent, const Owned& cells, DonorImport& source, std::vector<uint32_t>& weights,
                    std::string& reason, BridgeStats& stats, MetricComposition composition = {},
                    size_t ceiling = size_t(2) << 30, size_t caller_bytes = 0, bool query_timings = true);
bool ReaderSlices(World& parent, const Owned& cells, const std::vector<DonorCell>& original,
                  const ReferenceState& reference, CReaderSlices& slices, std::string& reason, BridgeStats& stats,
                  MetricComposition composition = {}, size_t ceiling = size_t(2) << 30, size_t caller_bytes = 0,
                  bool query_timings = true,DonorImport* prepared_source=nullptr);
}  // namespace SU2Native3D
