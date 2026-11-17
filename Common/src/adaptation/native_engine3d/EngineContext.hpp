// Borrows canonical inputs and groups owner storage; never copies an active payload.
#pragma once
#include "ContextValue.hpp"
#include "EngineControl.hpp"
#include "EngineCensus.hpp"
#include "EngineQueues.hpp"
#include "EngineExchange.hpp"
#include "EngineCommit.hpp"
#include "EngineOwnership.hpp"
#include "EngineRound.hpp"
#include "EngineSlot.hpp"





struct EngineSetup;
struct EngineContext {
  EngineSetup& setup;
  World& world;
  const Owned& accepted;
  const std::vector<DonorCell>& original;
  const PlanarReference& geometry;
  MetricComposition& composition;
  EngineOptions& options;
  MetricChange& intent;
  bool& bidirectional;
  BoundaryMetric& surface_target;
  EngineControl control;
  EngineCensus census;
  EngineQueues queues;
  EngineExchange exchange;
  EngineCommit commit;
  EngineOwnership ownership;

  EngineContext(EngineSetup&, World&, const Owned&, const std::vector<DonorCell>&, const PlanarReference&,
                MetricComposition&, EngineOptions&, MetricChange&, bool&, BoundaryMetric&);
  ~EngineContext();
  EngineContext(const EngineContext&) = delete;
  EngineContext& operator=(const EngineContext&) = delete;
  EngineContext(EngineContext&&) = delete;
  EngineContext& operator=(EngineContext&&) = delete;
};
