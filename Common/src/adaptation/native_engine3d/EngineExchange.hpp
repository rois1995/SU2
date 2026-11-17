// Owns Exchange slots only; Adapt-local ContextValues activate them without collectives or decisions.
#pragma once

struct EngineContext;

struct EngineExchange {
  std::optional<bool> construction_claims;
  std::optional<std::unique_ptr<Directory>> directory;
  std::optional<CavityMetricSource> source;
  std::optional<DirectoryStats> retired_directory;
  std::optional<bool> private_repair;

  void RetireDirectory(const DirectoryStats& old);

  CElectedFailure ElectTrialFailure(EngineContext&, const CLocalFailure&);
  bool Vote(EngineContext&, const CLocalFailure&);
  size_t Bytes(EngineContext&);
  size_t RealBytes(EngineContext&);
  bool Admit(EngineContext&, size_t extra);
  DonorPatch ImportSource(EngineContext&, const std::vector<Cell>& cells, size_t extra, bool layer,
                          const std::vector<unsigned char>* requests = nullptr);

  EngineExchange();
  ~EngineExchange();
  EngineExchange(const EngineExchange&) = delete;
  EngineExchange& operator=(const EngineExchange&) = delete;
  EngineExchange(EngineExchange&&) = delete;
  EngineExchange& operator=(EngineExchange&&) = delete;
};
