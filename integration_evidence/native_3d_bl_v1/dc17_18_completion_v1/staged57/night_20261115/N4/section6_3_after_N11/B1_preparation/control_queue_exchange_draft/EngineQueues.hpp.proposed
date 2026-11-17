// Owns empty queue-state slots; Adapt locals activate values, and no decision logic lives here.
#pragma once

struct EngineContext;

struct EngineQueues {
  std::optional<std::map<Id, std::set<Id>>> inactive_blocked_edges;
  std::optional<size_t> inactive_blocked_links;
  std::optional<std::array<bool, 2>> length_blocks_dirty;

  void LengthDirection(EngineContext& context, bool collapse);

  EngineQueues();
  ~EngineQueues();
  EngineQueues(const EngineQueues&) = delete;
  EngineQueues& operator=(const EngineQueues&) = delete;
  EngineQueues(EngineQueues&&) = delete;
  EngineQueues& operator=(EngineQueues&&) = delete;
};
