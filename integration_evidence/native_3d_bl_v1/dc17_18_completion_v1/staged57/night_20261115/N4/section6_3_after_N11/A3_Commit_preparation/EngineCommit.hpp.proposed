// Owns Commit payload slots only; original Adapt scopes activate them without decisions.
#pragma once

struct EngineCommit {
  std::optional<Owned> working;
  std::optional<std::vector<Id>> batch_changed;
  std::optional<std::set<Id>> batch_touched;
  std::optional<std::set<std::array<Id, 4>>> batch_supports;
  std::optional<Id> maximum_node;
  std::optional<Id> maximum_cell;

  EngineCommit();
  ~EngineCommit();
  EngineCommit(const EngineCommit&) = delete;
  EngineCommit& operator=(const EngineCommit&) = delete;
  EngineCommit(EngineCommit&&) = delete;
  EngineCommit& operator=(EngineCommit&&) = delete;
};
