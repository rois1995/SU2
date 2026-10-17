/*!
 * \file CNativeRemesher3D.hpp
 * \brief Opt-in tetrahedral CRemesher adapter with explicit worker communicators and private publication.
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md).
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */

#pragma once
#include "CRemesher.hpp"
#include "CNativeImport3D.hpp"
#include "CNativeEngine3D.hpp"
namespace SU2Native3D {
struct RemeshStats {
  // Exclusive computation scopes; diagnostic reporting is separate and measured.
  static constexpr std::array<const char*, 14> names{{"support", "reference", "import", "source", "weights",
                                                      "partition", "donor_migration", "communicator", "refine",
                                                      "coarsen", "wait", "return", "policy", "reassessment"}};
  std::array<double, 14> phases{};
  double total_seconds = 0, other_seconds = 0, report_seconds = 0, process_cpu_seconds = 0, idle_cpu_seconds = 0;
  BridgeStats imported, weights, returned, reassessment;
  ComplexityReassessment3D complexity;
  DonorImportStats source;
  PartitionStats partition;
  EngineStats refine, coarsen;
  // Import owns a separate World; weight/return scopes are already inside the parent's totals.
  uint64_t communication_bytes = 0, communication_calls = 0, initial_cells = 0, final_cells = 0;
  uint64_t parent_transport_bytes = 0, parent_transport_calls = 0;
  uint64_t worker_transport_bytes = 0, worker_transport_calls = 0;
  double communication_seconds = 0, parent_transport_seconds = 0, worker_transport_seconds = 0;
  size_t maximum_native_bytes = 0, process_peak_rss_bytes = 0;
  int workers = 0;
  bool complete = false, detailed_timings = false;
};
}  // namespace SU2Native3D
class CNativeRemesher3D final : public CRemesher {
 public:
  using CompositionFactory = std::function<SU2Native3D::MetricComposition(
      const CConfig&, const SU2Native3D::ReferenceState&, double, SU2Native3D::EngineOptions&)>;
  explicit CNativeRemesher3D(std::shared_ptr<SU2Native3D::ReferenceState> retained,
                             SU2Native3D::EngineOptions options = {}, CompositionFactory geometricConstraints = {});
  void PrepareReference(const CConfig& config, const CGeometry& geometry);
  static void CheckSupport(const CConfig& config, const CGeometry& geometry);
  CRemeshResult Remesh(const CConfig& config, const CGeometry& geometry, const su2activematrix& sensor) override;
  const SU2Native3D::RemeshStats& Statistics() const { return stats; }

 private:
  std::shared_ptr<SU2Native3D::ReferenceState> reference;
  SU2Native3D::EngineOptions options;
  CompositionFactory compositionFactory;
  SU2Native3D::RemeshStats stats;
};
