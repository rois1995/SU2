// Owns Exchange slots only; Adapt-local ContextValues activate them without collectives or decisions.
#pragma once

struct EngineContext;

struct EngineExchange {
  struct DeferredVotes {
    bool active = false;
    CLocalFailure first;
  };

  class DeferredVoteScope {
   public:
    explicit DeferredVoteScope(DeferredVotes& votes);
    ~DeferredVoteScope();
    DeferredVoteScope(const DeferredVoteScope&) = delete;
    DeferredVoteScope& operator=(const DeferredVoteScope&) = delete;

   private:
    DeferredVotes& votes;
  };

  std::optional<bool> construction_claims;
  std::optional<std::unique_ptr<Directory>> directory;
  std::optional<CavityMetricSource> source;
  std::optional<DirectoryStats> retired_directory;
  std::optional<bool> private_repair;
  std::optional<DeferredVotes> deferred_votes;

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

// Adapts one trial's live private/collective mode without owning its imports or communicator.
struct TrialComm {
  const bool& private_repair;
  World& world;
  CavityMetricSource& source;
  const Imported& imported;

  int Maximum(int value);
  template <class T>
  T Reduce(T value, CPassiveComm::Op op);
  DonorPatch ImportCells(const std::vector<Cell>& cells, size_t ceiling, size_t caller);
  DonorPatch ImportSamples(const std::vector<Cell>& cells, const std::vector<Id>& extensions, size_t ceiling,
                           size_t caller, const std::vector<unsigned char>& requests, bool shape);
  DonorSamples SamplePoints(const std::vector<Point>& points, const std::vector<Id>& extensions, size_t ceiling,
                            size_t caller);
  DonorSamples SampleMidpoints(const std::vector<std::array<Point, 2>>& points, const std::vector<Id>& extensions,
                               size_t ceiling, size_t caller);
};
