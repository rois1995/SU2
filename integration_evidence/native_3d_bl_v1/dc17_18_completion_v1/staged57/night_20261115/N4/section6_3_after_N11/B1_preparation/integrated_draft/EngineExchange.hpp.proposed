// Owns Exchange slots only; Adapt-local ContextValues activate them without collectives or decisions.
#pragma once

struct EngineExchange {
  std::optional<bool> construction_claims;
  std::optional<std::unique_ptr<Directory>> directory;
  std::optional<CavityMetricSource> source;
  std::optional<DirectoryStats> retired_directory;
  std::optional<bool> private_repair;

  void RetireDirectory(const DirectoryStats& old);

  EngineExchange();
  ~EngineExchange();
  EngineExchange(const EngineExchange&) = delete;
  EngineExchange& operator=(const EngineExchange&) = delete;
  EngineExchange(EngineExchange&&) = delete;
  EngineExchange& operator=(EngineExchange&&) = delete;
};
