// Owns Census slots and measures live Context state; Adapt retains activation and payload lifetimes.
#pragma once

#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <vector>

struct EngineContext;

struct EngineCensus {
  std::optional<Selection> selection;
  std::optional<GenerationProgress3D> edge_progress;
  std::optional<uint64_t> edge_census_round;
  std::optional<std::map<Edge, EdgeMeasure>> retired_edges;
  std::optional<std::map<Edge, EdgeMeasure>> campaign_edges;
  std::optional<std::shared_ptr<const FrozenMetricIdentity>> field_identity;
  std::optional<std::vector<Id>> initial;
  std::optional<std::vector<Id>> eligible_extensions;
  std::optional<size_t> seed_slot;

  bool ScopeCensusComplete(EngineContext&) const;
  void RefreshSurfaceCensus(EngineContext&) const;
  bool GenerationCensus(EngineContext&);
  GenerationDebt CurrentGenerationDebt(EngineContext&) const;
  bool Measure(EngineContext&, const std::vector<Id>&, bool seed, double* enclosing_seconds);
  bool ImportMeasurementBatch(EngineContext&, const std::vector<Id>& ids, size_t slot, size_t& batch,
                              size_t plan_scratch, double* enclosing_seconds, CLocalFailure&,
                              std::map<Edge, EdgeMeasure>& reuse, DonorPatch& imported);
  bool MeasureBatchCells(EngineContext&, const std::vector<Id>& ids, size_t slot, size_t batch, bool seed,
                         size_t plan_scratch, CLocalFailure&, DonorPatch& imported, std::map<Edge, EdgeMeasure>& reuse);
  void MeasureCell(EngineContext&, Id id, bool seed, FrozenField& field, PatchMetric& planned,
                   std::map<Edge, EdgeMeasure>& reuse);
  bool SeedNext(EngineContext&, bool local_active = true);

  EngineCensus();
  ~EngineCensus();
  EngineCensus(const EngineCensus&) = delete;
  EngineCensus& operator=(const EngineCensus&) = delete;
  EngineCensus(EngineCensus&&) = delete;
  EngineCensus& operator=(EngineCensus&&) = delete;
};
