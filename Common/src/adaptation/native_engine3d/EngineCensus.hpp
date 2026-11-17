// Owns the canonical Census payloads; it never starts a value, performs a collective, or changes a decision.
#pragma once

#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <vector>

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

  EngineCensus();
  ~EngineCensus();
  EngineCensus(const EngineCensus&) = delete;
  EngineCensus& operator=(const EngineCensus&) = delete;
  EngineCensus(EngineCensus&&) = delete;
  EngineCensus& operator=(EngineCensus&&) = delete;
};
