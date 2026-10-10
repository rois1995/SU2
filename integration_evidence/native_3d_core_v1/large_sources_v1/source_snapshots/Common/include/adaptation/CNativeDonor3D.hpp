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
};
struct DonorImportStats {
  uint64_t builds = 0, imports = 0, rejected = 0, owner_requests = 0, index_candidates = 0, payloads = 0;
  size_t maximum_patch = 0, maximum_work_bytes = 0;
  double build_seconds = 0, import_seconds = 0, routing_seconds = 0, search_seconds = 0, exchange_seconds = 0;
};
/*! Original source stays immutable while working mesh ownership/connectivity changes.
 * Matching collective calls involve only world.comm; empty region lists are inactive participants.
 * Imports conservatively include all original AABB intersections (not a geometric coverage certificate).
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
  size_t ResidentBytes() const;
  const DonorImportStats& Statistics() const { return stats; }
 private:
  World& world;
  const bool timings;
  bool initialized = false;
  std::vector<DonorCell> donors;
  std::vector<double> boxes;
  std::unique_ptr<CADTElemClass> index;
  std::unique_ptr<CRankBoxTree> ranks;
  DonorImportStats stats;
  bool Vote(const CLocalFailure& failure, std::string& reason);
  bool Admit(size_t bytes, size_t ceiling, std::string& reason);
};
}  // namespace SU2Native3D
