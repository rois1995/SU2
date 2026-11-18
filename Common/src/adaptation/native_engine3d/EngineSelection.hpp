// Owns the versioned cell census and its construction and repair queues in one lifetime.
#pragma once

// Shares the published selection state between measurement and candidate scheduling.
class Selection {
 public:
  enum class Lane { Surface, Interior };
  // Starts with empty indexes that borrow the event's geometry, policies and statistics.
  Selection(bool coarsening, bool repairing, const PlanarReference& geometry, EngineStats& stats, bool timings,
            double tolerance, const WallHeights& heights, bool quality_only, bool surface_planning = false,
            wall_diagnostics::Counters* wall_counters = nullptr)
      : coarsening(coarsening),
        track_repairs(repairing),
        repair_event(repairing),
        quality_only(quality_only),
        surface_planning(surface_planning),
        geometry(geometry),
        stats(stats),
        timings(timings),
        tolerance(tolerance),
        heights(heights),
        wall_counters(wall_counters) {}

  bool coarsening, track_repairs, repair_event, prune_collapses = false, track_splits = false;
  const bool quality_only;
  const bool surface_planning;
  bool size_repairs = false, geometry_repairs = false;
  const PlanarReference& geometry;
  EngineStats& stats;
  bool timings;
  double tolerance;
  const WallHeights& heights;
  wall_diagnostics::Counters* wall_counters;

  // Record of the latest disposition and generation age of a tracked edge.
  struct History {
    uint64_t age = 0, round = 0;
    unsigned disposition = 0;
  };

  // Owns the reusable length certificate and incidence counts for a canonical edge.
  struct EdgeState {
    EdgeMeasure measure;
    size_t users = 0, physical_users = 0;
    std::array<size_t, 2> forbidden{};
  };

  // Stores separately measured validity and quality debt without changing its wire encoding.
  struct CellDebt {
    long double q = 0, j = 0, size = 0, height = 0, geometry = 0;
    bool q_measured = false, j_measured = false, height_measured = false, geometry_measured = true,
         height_unresolved = false;
    bool q_bad = false, j_bad = false, size_bad = false, height_bad = false;
    // Serializes fields in the existing wire order.
    template <class S>
    void Fields(S& s) {
      CachedScalar(s, q);
      CachedScalar(s, j);
      CachedScalar(s, size);
      CachedScalar(s, height);
      CachedScalar(s, geometry);
      s(q_measured, j_measured, height_measured, geometry_measured, height_unresolved, q_bad, j_bad, size_bad,
        height_bad);
    }
  };

  // Binds one certified cell version to its edge measurements and diagnostic values.
  struct CellState {
    uint64_t version = 0;
    CellDebt debt;
    double q_value = 0, j_value = 0;
    std::array<Edge, 6> edges{};
    std::array<unsigned char, 6> forbidden{};
    std::array<bool, 6> physical{};
    std::array<Id, 4> vertices{};
    std::array<long double, 6> geometry_scores{};
    bool valid = false, target_valid = false, geometry_invalid = false, construction_valid = false,
         quality_valid = false, shape_measured = false;
    bool surface = false, near_feature = false, feature_known = false;
    unsigned char geometry_types = 0, geometry_excess_bin = 0;
    // Serializes fields in the existing wire order.
    template <class S>
    void Fields(S& s) {
      s(version, debt, q_value, j_value, edges, forbidden, physical, vertices);
      for (auto& x : geometry_scores) CachedScalar(s, x);
      s(valid, target_valid, geometry_invalid, construction_valid, quality_valid, shape_measured);
      s(surface, near_feature, feature_known, geometry_types, geometry_excess_bin);
    }
  };

  // Carries the cell certificate and required-feature history in ownership migration.
  struct CachedRow {
    Id id = 0;
    CellState state;
    std::array<long double, 6> lengths{};
    std::array<uint64_t, 14> repair_epochs{};
    uint16_t repair_active = 0, repair_blocked = 0;
    // Serializes fields in the existing wire order.
    template <class S>
    void Fields(S& s) {
      s(id, state);
      for (auto& x : lengths) CachedScalar(s, x);
      s(repair_epochs, repair_active, repair_blocked);
    }
  };

  // users: cells that ask for this repair. epoch: last service. blocked: last trial was refused.
  // refusals (print only, M28): each ladder's rejection bits since the support last changed.
  struct RepairDemand {
    size_t users = 0;
    uint64_t epoch = 0;
    bool blocked = false;
    std::array<uint8_t, 3> refusals{};  // Fits in the existing padding.
  };
  static_assert(sizeof(RepairDemand) == 3 * sizeof(uint64_t), "M28 must not change the modeled repair bytes.");

  // Counts live cache rows and keeps the topology that indexes their certificates.
  struct CensusState {
    size_t invalid = 0, construction_invalid = 0, quality_invalid = 0, unmeasured_shape = 0;
    StageProgress debt;
    bool progress_consistent = true, census_started = false;
    std::map<Id, CellState> cells;
    std::map<Edge, EdgeState> edges;
    std::map<Id, std::set<Id>> vertex_cells;
    size_t vertex_links = 0;
  } census;

  // Keeps ordered construction and repair candidates, blocked directions, and waiting age.
  struct QueueState {
    bool split_lanes = false;
    Lane lane = Lane::Surface;
    std::set<std::tuple<long double, Id, Edge>> geometry_queue;
    std::map<Edge, long double> geometry_split_priorities;
    bool geometry_priority_mode = false;
    std::set<std::pair<long double, Edge>> surface_queue;
    // A generation contains its entry edges even while their measurements are seeded.
    // Publishing new cells cannot extend it.
    std::set<Edge> construction_frontier;
    std::map<Edge, History> edge_history;
    std::set<std::pair<long double, Edge>> queue;
    std::map<Id, std::set<Id>> blocked_edges;
    size_t blocked_links = 0;
    std::map<Id, std::vector<RepairFeature>> cell_repairs;
    uint64_t repair_epoch = 0;
    std::map<RepairFeature, RepairDemand> repair_users;
    std::set<std::pair<uint64_t, RepairFeature>> repair_queue;
    std::map<Id, long double> repair_cell_severity;
    std::map<RepairFeature, long double> repair_severity;
    std::set<std::pair<long double, RepairFeature>> repair_severity_queue;
    std::set<RepairFeature> repair_zero;
    size_t repair_bytes = 0;
  } queues;

  long double GeometryPriority(long double raw, Edge edge) const;
  size_t GeometryPriorityBytes() const;
  void PrioritizeGeometrySplits(Clock::time_point started, double maximum_seconds);
  void Note(Edge edge, unsigned disposition);
  void RefreshHistory(bool generation);
  long double Priority(long double length) const;
  bool Candidate(long double length) const;
  void Queue(Edge edge, long double length, bool insert);
  size_t Bytes() const;
  long double RepairWeight(const CellState& row) const;
  void AddRepairSeverity(const RepairFeature& key, long double value);
  void RemoveRepairSeverity(const RepairFeature& key, long double value);
  void UpdateRepairWeight(Id id, const CellState& row);
  void Account(const CellState& row, bool add);
  bool RequestedWall(const OwnedCell& owned) const;
  std::array<size_t, 3> SurfaceCensus(const Owned& owned) const;
  StageProgress Progress(size_t owned) const;
  bool Diagnose(const OwnedCell& owned, CellDiagnostic& out) const;
  Choice Best(bool repairing) const;
  std::vector<Choice> RepairBatch(const Owned& working, size_t limit, bool& oversized) const;
  std::vector<Choice> Batch(const Owned& working, size_t limit);
  void Cached(const Cell& cell, std::map<Edge, EdgeMeasure>& values) const;
  void Block(Edge edge, Feature feature, bool repairing);
  void NoteRepairRefusal(const Feature& feature, unsigned kind, unsigned index);
  void Forget(Edge edge);
  void TouchRepairSupport(const std::array<Id, 4>& vertices);
  void Touch(Id vertex);
  bool FilterCollapse(const Choice& choice, const Owned& working);
  void Drop(Id id);
  void RemoveRepairs(Id id);
  void UpdateRepairs(const Cell& cell, const CellState& row);
  void ChangePhase(bool improvement, const Owned& working, const std::shared_ptr<const FrozenMetricIdentity>& identity);
  void EnableSizeRepairs(const Owned& working);
  void EnableGeometryRepairs(const Owned& working);
  void ClassifyFeatures(const Cell& cell, CellState& row) const;
  void RefreshQuality(const OwnedCell& owned, const PatchMetric& metric, bool valid, bool quality_valid,
                      const FirstHeightReport* height, bool height_valid,
                      const std::shared_ptr<const FrozenMetricIdentity>& identity);
  void EnableCollapseFilter();
  bool LeavesFacet(const OwnedCell& owned, Id removed, Point retained);
  unsigned char Forbidden(const OwnedCell& owned, Edge edge);
  void Add(const OwnedCell& owned, const PatchMetric& metric, bool valid, bool quality_valid, const std::string& reason,
           const FirstHeightReport* height = nullptr, bool height_valid = true);
  void AddRepairs(const Cell& cell, bool length_only = false);
  void Service(Feature feature, bool accepted);
  void FinishRepairs();
  bool HasCoarseningDemand() const;
  void SeedFrontier(const Owned& working);
  bool ServiceEdge(Edge edge);
  size_t ServiceClaimedVertex(Id vertex);
  void Restore(const OwnedCell& owned, const CachedRow& record,
               const std::shared_ptr<const FrozenMetricIdentity>& identity);
  static std::array<RepairFeature, 14> RepairKeys(const CellState& row);
  void SaveRepairState(CachedRow& record) const;
  void RestoreRepairState(const std::vector<CachedRow>& records);
  void SwapState(Selection& other) noexcept;
  void Reset(bool improvement);
  void CollectCellDebt(const OwnedCell& owned, const PatchMetric& metric, const FirstHeightReport* height,
                       bool height_valid, CellState& row) const;
  void MeasureGeometryDebt(const OwnedCell& owned, const BoundaryParameterState* parameters, CellState& row,
                           std::map<Edge, long double>& geometry_scores);
  void MeasureGeometryFace(const OwnedCell& owned, const Face& face, size_t slot,
                           const BoundaryParameterState* parameters, CellState& row,
                           std::map<Edge, long double>& geometry_scores);
  void AccumulateGeometryScores(const Face& face, const std::array<Point, 3>& triangle, double error, double allowed,
                                const WallFaceWeights& law_weights, std::map<Edge, long double>& geometry_scores) const;
  void AddCellEdges(const OwnedCell& owned, const PatchMetric& metric, std::map<Edge, long double>& geometry_scores,
                    CellState& row);
  void CollectRepairStar(const Owned& working, const RepairFeature& key, std::vector<Id>& ids, std::set<Id>& vertices,
                         bool& too_large) const;
};
