// Owns Working and ownership-boundary markers; original Adapt scopes activate values.
#pragma once

struct EngineOwnership {
  std::optional<Owned> working;
  std::optional<std::array<bool, 2>> ownership_evaluated;
  std::optional<uint64_t> quality_ownership_round;

  EngineOwnership();
  ~EngineOwnership();
  EngineOwnership(const EngineOwnership&) = delete;
  EngineOwnership& operator=(const EngineOwnership&) = delete;
  EngineOwnership(EngineOwnership&&) = delete;
  EngineOwnership& operator=(EngineOwnership&&) = delete;
};
