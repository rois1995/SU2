/*!
 * \file CNativeDonor3D.hpp
 * \brief Worker-routed immutable original sensor-only tetrahedral donor imports.
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md).
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */
#pragma once
#include "CNativeDistributed3D.hpp"
#include "CNativeField3D.hpp"
#include <memory>
class CADTElemClass;
namespace SU2Native3D {
using DonorRegion = std::array<double, 6>;  // Closed lower/upper corners; may be a point or an edge box.
struct DonorPatch {
  std::vector<DonorCell> cells;
  bool valid = false;
  std::string reason;
  std::vector<int> source_rank; /*!< Same order as returned cells; ranks in this source's World, valid until ownership rebuild. */
};
struct DonorImportStats {
  uint64_t builds = 0, imports = 0, rejected = 0, owner_requests = 0, index_candidates = 0, payloads = 0;
  uint64_t filter_candidates = 0, filtered = 0, reused_candidates = 0, box_filtered = 0;
  KernelStats filter_kernel;
  size_t maximum_patch = 0, maximum_work_bytes = 0;
  double build_seconds = 0, import_seconds = 0, routing_seconds = 0, search_seconds = 0, exchange_seconds = 0;
  double filter_seconds = 0;  // Nested in owner search, not an exclusive import phase.
};
/*! Original source stays immutable while working mesh ownership/connectivity changes.
 * Matching collective calls involve only world.comm; empty region lists are inactive participants.
 * Box imports include all original AABB intersections. Cell imports additionally reject strictly separated
 * face planes with exact signs when the original source exceeds the patch cap; smaller sources keep the
 * cheaper box path. Touching donors remain eligible. Neither is a geometric coverage certificate.
 * Metric-cell imports search actual centroid/edge boxes separately when filtering is required,
 * retaining every closed intersection. Per-cell owner masks pack queries without repeating geometry;
 * the sender centroid is retained and grouped requests are admitted before allocation.
 * Certified segment/box separation avoids exact tetrahedron tests; touching/unresolved cases retain them.
 * Exact FrozenField queries/edge coverage remain authoritative. Compose locally at actual queries, never on wire.
 * Regions <= MaximumCavityCells and donor union <= MaximumDonors; oversized imports reject as a whole.
 * Source IDs, canonical simplices, shared coordinates/sensors and SPD are checked across ranks at Build.
 * Source embedding/conformity is a caller precondition. Byte models describe requested storage, not allocator/RSS.
 */
class DonorImport {
 public:
  explicit DonorImport(World& world, bool timings = false);
  ~DonorImport();
  bool Build(const std::vector<DonorCell>& original, std::string& reason,
             size_t ceiling = SIZE_MAX, size_t caller_bytes = 0);
  DonorPatch Import(const std::vector<DonorRegion>& regions, size_t ceiling = SIZE_MAX, size_t caller_bytes = 0);
  /*! Exact closed point/segment footprint, including every touching original/extension donor.
   * Avoids importing the entire segment box when the frozen source is large. */
  DonorPatch ImportSegments(const std::vector<std::array<Point,2>>& segments,
                            size_t ceiling=SIZE_MAX,size_t caller_bytes=0);
  /*! Complete exact original-source point footprints. A valid empty result certifies that no source cell
   * intersects these points; failed/oversized searches remain invalid, never outside-domain zeros. */
  DonorPatch ProbePoints(const std::vector<Point>& points, size_t ceiling = SIZE_MAX, size_t caller_bytes = 0);
  DonorPatch ImportCells(const std::vector<Cell>& cells, size_t ceiling = SIZE_MAX, size_t caller_bytes = 0);
  /*! Only covers these cells' actual centroids and six closed edge segments; not arbitrary interior queries.
   * Reconstruction callers must retain full-cell imports until every private query is known. */
  DonorPatch ImportMetricCells(const std::vector<Cell>& cells, size_t ceiling = SIZE_MAX, size_t caller_bytes = 0);
  size_t ResidentBytes() const;
  const DonorImportStats& Statistics() const { return stats; }
 private:
  World& world;
  const bool timings;
  bool initialized = false, filter_cells = false;
  std::vector<DonorCell> donors;
  std::vector<double> boxes;
  std::unique_ptr<CADTElemClass> index;
  std::unique_ptr<CRankBoxTree> ranks;
  DonorImportStats stats;
  DonorPatch ImportRegions(const std::vector<DonorRegion>& regions, const std::vector<Cell>* cells,
                          size_t ceiling, size_t caller_bytes, bool metric_queries = false,
                          const std::vector<std::array<Point,2>>* segments = nullptr,
                          const std::vector<Point>* points = nullptr);
  bool Vote(const CLocalFailure& failure, std::string& reason);
  bool Admit(size_t bytes, size_t ceiling, std::string& reason);
};
}  // namespace SU2Native3D
