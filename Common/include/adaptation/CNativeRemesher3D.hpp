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
#include "CComplexityNormalization.hpp"
#include <utility>
namespace SU2Native3D {
struct NormalizationProposal3D {
  bool valid=true,changed=false;
  std::string reason;
  double log_scale=0,core=0,seconds=0,geometry_seconds=0,assessment_seconds=0;
  unsigned trials=0;
  uint64_t density_queries=0,geometry_queries=0;
  size_t retained_bytes=0,maximum_bytes=0;
  SU2Complexity::Estimate estimate;
  SU2Complexity::Status status=SU2Complexity::Status::IntegrationLimited;
  std::function<Tensor(Id)> source_value;
};
struct NormalizationStage3D {
  size_t retained_bytes=0;
  std::function<NormalizationProposal3D(const ComplexityReassessment3D&,const PreparedComplexity3D&,
      const std::vector<DonorCell>&,double,const std::function<bool()>&,size_t,size_t)> propose;
};
struct RemeshStats {
  // Exclusive computation scopes; diagnostic reporting is separate and measured.
  static constexpr std::array<const char*, 15> names{{"support", "reference", "import", "source", "weights",
                                                      "partition", "donor_migration", "communicator", "refine",
                                                      "coarsen", "wait", "return", "policy", "reassessment", "normalization"}};
  std::array<double, 15> phases{};
  unsigned normalization_stages=0;
  uint64_t cumulative_work=0;
  std::array<uint64_t,2> normalization_stage_work{};
  NormalizationProposal3D normalization;
  double total_seconds = 0, other_seconds = 0, report_seconds = 0, process_cpu_seconds = 0, idle_cpu_seconds = 0;
  BridgeStats imported, weights, returned, reassessment, parameter_return;
  ComplexityReassessment3D complexity;
  DonorImportStats source;
  PartitionStats partition;
  EngineStats construction_stats, refine, coarsen;
  // Import owns a separate World; weight/return scopes are already inside the parent's totals.
  uint64_t communication_bytes = 0, communication_calls = 0, initial_cells = 0, final_cells = 0;
  uint64_t parent_transport_bytes = 0, parent_transport_calls = 0;
  uint64_t worker_transport_bytes = 0, worker_transport_calls = 0;
  double communication_seconds = 0, parent_transport_seconds = 0, worker_transport_seconds = 0;
  size_t maximum_native_bytes = 0, process_peak_rss_bytes = 0;
  int workers = 0;
  bool complete = false, detailed_timings = false;
  bool construction = false;      // A FIRST_ROW factory seeded private construction before distributed adaptation.
  std::string construction_report;  // Its JSON report (phases, gates, deficits, acceptance), also on rejection.
};
}  // namespace SU2Native3D
class CNativeRemesher3D final : public CRemesher {
 public:
  using CompositionFactory = std::function<SU2Native3D::MetricComposition(
      const CConfig&, const SU2Native3D::ReferenceState&, double, SU2Native3D::EngineOptions&)>;
  using NormalizationFactory = std::function<SU2Native3D::NormalizationStage3D(
      const CConfig&,const CGeometry&,const su2activematrix&)>;
  /*! Provisional construction on the worker communicator: the accepted import, immutable original donors,
   * pending reference, composition and shared event controls. A valid construction continues through distributed
   * Refine/Coarsen and strict final acceptance before CFD publication. `report` also records rejection. */
  using ConstructionFactory = std::function<SU2Native3D::EngineResult(SU2Native3D::World&,const SU2Native3D::Owned&,
      const std::vector<SU2Native3D::DonorCell>&,const SU2Native3D::ReferenceState&,SU2Native3D::MetricComposition,
      const SU2Native3D::EngineOptions&,std::string& report)>;
  explicit CNativeRemesher3D(std::shared_ptr<SU2Native3D::ReferenceState> retained,
                             SU2Native3D::EngineOptions options = {}, CompositionFactory geometricConstraints = {}, NormalizationFactory normalization = {});
  void PrepareReference(const CConfig& config, const CGeometry& geometry);
  static void CheckSupport(const CConfig& config, const CGeometry& geometry);
  CRemeshResult Remesh(const CConfig& config, const CGeometry& geometry, const su2activematrix& sensor) override;
  const SU2Native3D::RemeshStats& Statistics() const { return stats; }
  /*! Configure before Remesh; callbacks are worker-local and must not enter CFD collectives or retain borrowed data. */
  void SetRejectedStateObserver(decltype(SU2Native3D::EngineOptions::rejected_state) observer) {
    options.rejected_state=std::move(observer);
  }
  void SetRejectedTrialObserver(decltype(SU2Native3D::EngineOptions::rejected_trial) observer) {
    options.rejected_trial=std::move(observer);
  }
  void SetTransactionObserver(SU2Native3D::Feature feature,
                              decltype(SU2Native3D::EngineOptions::transaction_observer) observer) {
    options.capture_feature=feature; options.transaction_observer=std::move(observer);
  }
  /*! Install a construction (ADAP_BL_METHOD= FIRST_ROW); `accepted` runs inside the published result's onAccepted,
   *  after the CFD replacement succeeded (continuation state of the construction is committed there). */
  void SetConstruction(ConstructionFactory construct, std::function<void()> accepted = {}) {
    construction=std::move(construct);constructionAccepted=std::move(accepted);
  }

 private:
  std::shared_ptr<SU2Native3D::ReferenceState> reference;
  SU2Native3D::EngineOptions options;
  CompositionFactory compositionFactory;
  NormalizationFactory normalizationFactory;
  ConstructionFactory construction;
  std::function<void()> constructionAccepted;
  SU2Native3D::RemeshStats stats;
};
