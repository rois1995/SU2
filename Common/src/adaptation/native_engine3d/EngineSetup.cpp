// Executes original setup stages and collectives while preserving canonical ownership and lifetimes.
#include "EngineSetup.hpp"
#include "EngineTrial.hpp"

EngineSetup::EngineSetup() = default;
EngineSetup::~EngineSetup() = default;

void EngineSetup::CheckInitialNumericPolicies(EngineContext& context, bool curved,
                                              SU2NativeGeometry::AtlasBudget& atlas_budget, CLocalFailure& failure) {
  auto& world = context.world;
  auto& options = context.options;
  auto& result = *context.control.result;
  auto& stats = result.stats;
  auto& composition = context.composition;
  auto& accepted = context.accepted;
  auto& atlas_geometry = *context.setup.atlas_geometry;
  auto& target_geometry_lease = *context.setup.target_geometry_lease;
  // BEGIN_MOVED_SOURCE
  const auto curved_lo = TimedAllreduce(int(curved), CPassiveComm::Op::MIN, world.comm);
  const auto curved_hi = TimedAllreduce(int(curved), CPassiveComm::Op::MAX, world.comm);
  const double surface_lo = TimedAllreduce(options.surface_tolerance, CPassiveComm::Op::MIN, world.comm);
  const double surface_hi = TimedAllreduce(options.surface_tolerance, CPassiveComm::Op::MAX, world.comm);
  if (curved_lo != curved_hi || surface_lo != surface_hi || !std::isfinite(surface_lo) || (curved && !(surface_lo > 0)))
    failure.Set(1, 0, "Native3D analytic runtime requires a consistent positive frozen surface tolerance.");
  const auto ownership_lo = TimedAllreduce(int(options.phase_ownership), CPassiveComm::Op::MIN, world.comm);
  const auto ownership_hi = TimedAllreduce(int(options.phase_ownership), CPassiveComm::Op::MAX, world.comm);
  if (ownership_lo != ownership_hi || ownership_lo < 0 || ownership_hi > int(PhaseOwnership::DiagnosticAlways))
    failure.Set(1, 0, "Native3D phase ownership controls differ or are invalid.");
  const auto composition_lo = TimedAllreduce(int(bool(composition)), CPassiveComm::Op::MIN, world.comm);
  const auto composition_hi = TimedAllreduce(int(bool(composition)), CPassiveComm::Op::MAX, world.comm);
  const auto bands_lo = TimedAllreduce(int(bool(options.edge_breakpoints)), CPassiveComm::Op::MIN, world.comm);
  const auto bands_hi = TimedAllreduce(int(bool(options.edge_breakpoints)), CPassiveComm::Op::MAX, world.comm);
  if (composition_lo != composition_hi || bands_lo != bands_hi || (composition_hi && !bands_lo))
    failure.Set(1, 0, "Native3D frozen composition/band policies differ or composed integration is missing.");
  const auto lo = TimedAllreduce(options.maximum_rounds, CPassiveComm::Op::MIN, world.comm);
  const auto hi = TimedAllreduce(options.maximum_rounds, CPassiveComm::Op::MAX, world.comm);
  if (lo != hi) failure.Set(1, 0, "Native 3D round limit must be equal across workers.");
  const auto work_lo = TimedAllreduce(options.maximum_work, CPassiveComm::Op::MIN, world.comm);
  const auto work_hi = TimedAllreduce(options.maximum_work, CPassiveComm::Op::MAX, world.comm);
  const auto window_lo = TimedAllreduce(options.stagnation_window, CPassiveComm::Op::MIN, world.comm);
  const auto window_hi = TimedAllreduce(options.stagnation_window, CPassiveComm::Op::MAX, world.comm);
  const double time_lo = TimedAllreduce(options.maximum_seconds, CPassiveComm::Op::MIN, world.comm);
  const double time_hi = TimedAllreduce(options.maximum_seconds, CPassiveComm::Op::MAX, world.comm);
  if (work_lo != work_hi || window_lo != window_hi || !window_hi || !std::isfinite(time_lo) ||
      !std::isfinite(time_hi) || time_lo != time_hi || time_lo < 0)
    failure.Set(1, 0, "Native 3D work/time/progress controls differ or are invalid.");
  const auto global_cells = TimedAllreduce(uint64_t(accepted.size()), CPassiveComm::Op::SUM, world.comm);
  stats.work_limit = options.maximum_work == SIZE_MAX ? std::max<size_t>(4096, transfer_memory::Mul(global_cells, 64))
                                                      : options.maximum_work;
  if (stats.work_limit == SIZE_MAX) failure.Set(1, 0, "Native 3D automatic work ceiling overflows.");
  if (atlas_geometry) {
    const auto allowance = transfer_memory::Mul(transfer_memory::Add(global_cells, stats.work_limit),
                                                SU2NativeGeometry::MappedFacetReference::ClearanceWork);
    if (allowance == SIZE_MAX)
      failure.Set(1, 0, "Native3D exact atlas arithmetic work reservation overflows.");
    else
      atlas_budget.maximum_work =
          options.shared_geometry_budget
              ? std::min(atlas_budget.maximum_work, transfer_memory::Add(atlas_budget.work, allowance))
              : allowance;
  }
  const auto lease_low =
      TimedAllreduce(unsigned(bool(options.geometry_reservation)), CPassiveComm::Op::MIN, world.comm);
  const auto lease_high =
      TimedAllreduce(unsigned(bool(options.geometry_reservation)), CPassiveComm::Op::MAX, world.comm);
  if (lease_low != lease_high) failure.Set(1, 0, "Native3D target geometry reservation differs across workers.");
  if (atlas_geometry && options.geometry_reservation) try {
      target_geometry_lease = options.geometry_reservation(atlas_budget);
    } catch (const std::exception& e) {
      failure.Set(2, 0, e.what());
    }
  // END_MOVED_SOURCE
}

void EngineSetup::CheckInitialWallPolicies(EngineContext& context, CLocalFailure& failure) {
  auto& world = context.world;
  auto& options = context.options;
  auto& composition = context.composition;
  auto& intent = context.intent;
  auto& bidirectional = context.bidirectional;
  auto& surface_target = context.surface_target;
  // BEGIN_MOVED_SOURCE
  const auto batch_lo = TimedAllreduce(options.candidate_batch, CPassiveComm::Op::MIN, world.comm);
  const auto batch_hi = TimedAllreduce(options.candidate_batch, CPassiveComm::Op::MAX, world.comm);
  if (batch_lo != batch_hi || !batch_hi || batch_hi > MaximumCandidateBatch)
    failure.Set(1, 0, "Native 3D candidate batch must be bounded and equal across workers.");
  const auto mode_lo = TimedAllreduce(16 * int(bool(surface_target)) + 2 * int(intent) + int(bidirectional),
                                      CPassiveComm::Op::MIN, world.comm);
  const auto mode_hi = TimedAllreduce(16 * int(bool(surface_target)) + 2 * int(intent) + int(bidirectional),
                                      CPassiveComm::Op::MAX, world.comm);
  if (mode_lo != mode_hi) failure.Set(1, 0, "Native 3D adaptation intent differs across workers.");
  if (options.wall_heights.size() > MaximumDirectoryFeatures)
    failure.Set(1, 0, "Native3D wall-height policy exceeds bounded marker count.");
  for (const auto& row : options.region_walls) {
    const auto h = options.wall_heights.find(row.first);
    const auto p = row.second;
    if (h == options.wall_heights.end() || h->second != p.height || !(p.growth >= 1 && p.thickness >= p.height) ||
        !std::isfinite(p.height + p.growth + p.thickness))
      failure.Set(1, 0, "Native3D region/height policies are inconsistent or invalid.");
  }
  for (const auto& h : options.wall_heights)
    if (!(h.second > 0) || !std::isfinite(h.second))
      failure.Set(1, 0, "Native3D wall-height policy is nonfinite/nonpositive.");
  // One first-height tolerance T (ADAP_BL_FIRST_HEIGHT_TOL) on every worker; no rank-local gate.
  const auto height_tolerance_lo = TimedAllreduce(options.wall_heights.tolerance, CPassiveComm::Op::MIN, world.comm);
  const auto height_tolerance_hi = TimedAllreduce(options.wall_heights.tolerance, CPassiveComm::Op::MAX, world.comm);
  if (!(height_tolerance_lo > 0 && height_tolerance_hi < 1) || height_tolerance_lo != height_tolerance_hi)
    failure.Set(1, 0, "Native3D first-height tolerance must lie in (0,1) and agree across workers.");
  // END_MOVED_SOURCE
}

void EngineSetup::ExchangeInitialWallPolicies(EngineContext& context, CLocalFailure& failure, bool& ready) {
  auto& world = context.world;
  auto& options = context.options;
  auto& result = *context.control.result;
  auto& stats = result.stats;
  // BEGIN_MOVED_SOURCE
  if (ready)
    ready = context.exchange.Admit(
        context,
        transfer_memory::Add(transfer_memory::Mul(world.size, sizeof(std::vector<HeightPolicyRow>)),
                             transfer_memory::Mul(transfer_memory::Mul(world.size, options.wall_heights.size()),
                                                  2 * sizeof(HeightPolicyRow))));
  if (ready) {
    std::vector<std::vector<HeightPolicyRow>> to(world.size);
    for (auto& peer : to)
      for (const auto& h : options.wall_heights) {
        const auto p = options.region_walls.find(h.first);
        peer.push_back({h.first, h.second, p == options.region_walls.end() ? 0 : p->second.growth,
                        p == options.region_walls.end() ? 0 : p->second.thickness});
      }
    bool admitted = false;
    const auto rows = world.exchange(to, &admitted, options.memory_ceiling, context.exchange.Bytes(context));
    if (!admitted) {
      result.reason = "Native3D wall-height policy exchange exceeds budget.";
      ready = false;
    } else {
      if (rows.size() != size_t(world.size) * options.wall_heights.size())
        failure.Set(1, 0, "Native3D wall-height policies differ across workers.");
      else
        for (size_t i = 0; i < rows.size(); ++i) {
          auto h = options.wall_heights.begin();
          std::advance(h, i % options.wall_heights.size());
          const auto p = options.region_walls.find(h->first);
          if (rows[i].marker != h->first || rows[i].height != h->second ||
              rows[i].growth != (p == options.region_walls.end() ? 0 : p->second.growth) ||
              rows[i].thickness != (p == options.region_walls.end() ? 0 : p->second.thickness))
            failure.Set(1, 0, "Native3D wall-height/region policies differ across workers.");
        }
      ready = context.exchange.Vote(context, failure);
    }
  }
  // END_MOVED_SOURCE
}

void EngineSetup::PopulateInitialStateAndBuildIndexes(EngineContext& context, bool curved, CLocalFailure& failure,
                                                      bool& ready, Id& maximum_node, Id& maximum_cell) {
  auto& world = context.world;
  auto& options = context.options;
  auto& result = *context.control.result;
  auto& stats = result.stats;
  auto& accepted = context.accepted;
  auto& original = context.original;
  auto& geometry = context.geometry;
  auto& bidirectional = context.bidirectional;
  auto& surface_target = context.surface_target;
  auto& coarsening = *context.control.coarsening;
  auto& repairing = *context.control.repairing;
  auto& accepted_bytes = *context.control.accepted_bytes;
  auto& working = *context.ownership.working;
  auto& selection = *context.census.selection;
  auto& field_identity = *context.census.field_identity;
  auto& initial = *context.census.initial;
  auto& eligible_extensions = *context.census.eligible_extensions;
  auto& directory = *context.exchange.directory;
  auto& source = *context.exchange.source;
  // BEGIN_MOVED_SOURCE
  if (ready)
    ready = context.exchange.Admit(
        context, transfer_memory::Add(
                     accepted_bytes, 64, transfer_memory::Mul(geometry.ExtensionComponents().size(), sizeof(Id)),
                     transfer_memory::Mul(std::min<uint64_t>(stats.work_limit, 4096),
                                          bidirectional ? sizeof(Edge) + sizeof(SplitRecord)
                                          : coarsening  ? sizeof(Edge)
                                                        : sizeof(SplitRecord)),
                     transfer_memory::Mul(accepted.size(), context.control.SelectionBound(context, repairing))));
  if (ready) try {
      field_identity = std::make_shared<const FrozenMetricIdentity>(
          surface_target ? FrozenMetricIdentity::Scope::SurfacePlanning : FrozenMetricIdentity::Scope::FullTarget);
      working = accepted;
      stats.maximum_owned = working.size();
      if (coarsening || bidirectional) result.collapses.reserve(std::min<uint64_t>(stats.work_limit, 4096));
      if (!coarsening && !repairing) result.splits.reserve(std::min<uint64_t>(stats.work_limit, 4096));
      initial.reserve(working.size());
      eligible_extensions = geometry.ExtensionComponents();
      if (curved)
        for (const auto& donor : original) {
          maximum_cell = std::max(maximum_cell, donor.cell.id);
          for (auto node : donor.cell.v) maximum_node = std::max(maximum_node, node.id);
        }
      for (const auto& row : working) {
        if (!surface_target || selection.RequestedWall(row.second)) initial.push_back(row.first);
        maximum_cell = std::max(maximum_cell, row.first);
        for (auto node : row.second.cell.v) maximum_node = std::max(maximum_node, node.id);
      }
    } catch (const std::exception& e) {
      failure.Set(2, 0, e.what());
    }
  if (ready) ready = context.exchange.Vote(context, failure);
  if (ready && !world.maximum(!working.empty())) {
    failure.Set(1, 0, "Native 3D adaptation input is empty.");
    ready = context.exchange.Vote(context, failure);
  }
  if (ready) {
    maximum_node = TimedAllreduce(maximum_node, CPassiveComm::Op::MAX, world.comm);
    maximum_cell = TimedAllreduce(maximum_cell, CPassiveComm::Op::MAX, world.comm);
    ready = directory->Build(working, result.reason, options.memory_ceiling,
                             transfer_memory::Add(context.exchange.Bytes(context), source.ResidentBytes()));
  }
  if (ready)
    ready = source.Build(original, result.reason, options.memory_ceiling,
                         transfer_memory::Add(context.exchange.Bytes(context), directory->ResidentBytes()));
  // END_MOVED_SOURCE
}

template <class Finish>
std::optional<EngineResult> EngineSetup::RefreshWorkingSurface(EngineContext& context,
                                                               SU2NativeGeometry::AtlasBudget& atlas_budget,
                                                               Finish& finish) {
  auto& world = context.world;
  auto& options = context.options;
  auto& geometry = context.geometry;
  auto& result = *context.control.result;
  auto& stats = result.stats;
  auto& working = *context.ownership.working;
  auto& source = *context.exchange.source;
  auto& directory = *context.exchange.directory;
  auto& atlas_geometry = *context.setup.atlas_geometry;
  const auto& atlas_work_initial = *context.setup.control_prefix.atlas_work_initial;
  // BEGIN_MOVED_SOURCE
  // Freeze donors first; normalization changes working associations, not the source interpretation.
  if (atlas_geometry && !geometry.EntityGeometry()) {
    if (!context.exchange.Admit(context, 0)) return std::optional<EngineResult>(finish());
    SU2NativeGeometry::BoundaryCoverage refreshed;
    CLocalFailure failure;
    const auto before_work = atlas_budget.work;
    try {
      if (!SU2NativeGeometry::RebuildWorkingSurface(*geometry.Parameters(), refreshed, atlas_budget, result.reason))
        failure.Set(2, 0, result.reason);
    } catch (const std::exception& error) {
      failure.Set(2, 0, error.what());
    }
    if (!context.exchange.Vote(context, failure)) return std::optional<EngineResult>(finish());
    if (world.maximum(!refreshed.faces.empty())) {
      // BEGIN_TAIL_CALL
      auto done = EngineSetup::PublishRefreshedSurface(context, atlas_budget, refreshed, failure, before_work, finish);
      if (done) return done;
      // END_TAIL_CALL
    }
  }
  // END_MOVED_SOURCE
  return std::nullopt;
}

template <class Finish>
std::optional<EngineResult> EngineSetup::PublishRefreshedSurface(EngineContext& context,
                                                                 SU2NativeGeometry::AtlasBudget& atlas_budget,
                                                                 SU2NativeGeometry::BoundaryCoverage& refreshed,
                                                                 CLocalFailure& failure, size_t before_work,
                                                                 Finish& finish) {
  auto& world = context.world;
  auto& options = context.options;
  auto& result = *context.control.result;
  auto& working = *context.ownership.working;
  auto& source = *context.exchange.source;
  auto& directory = *context.exchange.directory;
  // BEGIN_MOVED_SOURCE
  std::set<Face> affected;
  std::vector<Dependency> removed;
  std::vector<OwnedCell> added;
  const auto face_storage =
      transfer_memory::Add(SU2Native3D::BoundaryCoverageBytes(refreshed),
                           transfer_memory::Mul(refreshed.faces.size(), sizeof(Face) + 4 * sizeof(void*)));
  if (!context.exchange.Admit(context, face_storage)) return std::optional<EngineResult>(finish());
  size_t count = 0;
  auto owns = [&](const OwnedCell& row) {
    const auto faces = Faces(row.cell);
    for (unsigned k = 0; k < 4; ++k)
      if (row.physical[k].present && affected.count(Key(faces[k]))) return true;
    return false;
  };
  try {
    for (const auto& row : refreshed.faces) affected.insert(Key(row.first));
    for (const auto& row : working) count += owns(row.second);
  } catch (const std::exception& error) {
    failure.Set(2, 0, error.what());
  }
  if (!context.exchange.Vote(context, failure)) return std::optional<EngineResult>(finish());
  const auto local_storage =
      transfer_memory::Add(face_storage, transfer_memory::Mul(count, sizeof(Dependency) + sizeof(OwnedCell)));
  if (!context.exchange.Admit(context, local_storage)) return std::optional<EngineResult>(finish());
  try {
    removed.reserve(count);
    added.reserve(count);
    for (const auto& row : working)
      if (owns(row.second)) {
        if (row.second.version == UINT64_MAX) throw std::overflow_error("Working-surface owner version overflows.");
        removed.push_back({row.first, row.second.version, world.rank});
        added.push_back(row.second);
        ++added.back().version;
      }
  } catch (const std::exception& error) {
    failure.Set(2, 0, error.what());
  }
  if (!context.exchange.Vote(context, failure)) return std::optional<EngineResult>(finish());
  const SU2NativeGeometry::BoundaryCoverage empty;
  if (!directory->Publish(
          working, removed, added, true, result.reason, options.memory_ceiling,
          transfer_memory::Add(context.exchange.RealBytes(context), source.ResidentBytes(), local_storage), {},
          world.rank ? empty : refreshed, &atlas_budget))
    return std::optional<EngineResult>(finish());
  if (options.timings)
    std::cout << "NATIVE3D working surface refresh rank=" << world.rank << " faces=" << refreshed.faces.size()
              << " owners=" << added.size() << " arithmetic_work=" << atlas_budget.work - before_work << '\n';
  // END_MOVED_SOURCE
  return std::nullopt;
}

bool EngineSetup::CheckRegionApplicability(EngineContext& context, bool& region_applicable, bool prismatic) {
  auto& world = context.world;
  auto& options = context.options;
  auto& geometry = context.geometry;
  auto& result = *context.control.result;
  // BEGIN_MOVED_SOURCE
  if (region_applicable) {
    std::set<Id> walls;
    if (!options.region_walls.empty()) {
      const auto policy = options.region_walls.begin()->second;
      const double g = policy.growth, requested = .9 * MaximumMetricEdge;
      const double depth =
          g == 1 ? policy.height * requested
                 : policy.height *
                       (.5 * (g - 1) + .5 * (g + 1) * std::expm1((requested - .5) * 2 * (g - 1) / (g + 1))) / (g - 1);
      region_applicable = std::isfinite(depth) && depth > policy.height && depth < policy.thickness;
    }
    for (const auto& row : geometry.Facets()) {
      const Id id = geometry.Canonical(row.first);
      const auto a = geometry.Analytic().find(id);
      if (!geometry.CompleteAnalytic(id) || a == geometry.Analytic().end()) {
        region_applicable = false;
        break;
      }
      const bool radial =
          a->second.kind == AnalyticSurface::Kind::Cylinder || a->second.kind == AnalyticSurface::Kind::Sphere;
      if (radial) {
        walls.insert(id);
        region_applicable =
            region_applicable && (options.region_walls.empty() || options.region_walls.count(row.second.marker));
      } else
        region_applicable = region_applicable && a->second.kind == AnalyticSurface::Kind::Disk &&
                            !options.region_walls.count(row.second.marker);
    }
    region_applicable = region_applicable && walls.size() == 1;
  }
  region_applicable = region_applicable || prismatic;
  const int region_lo = TimedAllreduce(int(region_applicable), CPassiveComm::Op::MIN, world.comm);
  const int region_hi = TimedAllreduce(int(region_applicable), CPassiveComm::Op::MAX, world.comm);
  if (region_lo != region_hi) {
    result.reason = "Native3D region applicability differs across workers.";
    return false;
  }
  return true;
  // END_MOVED_SOURCE
}

template <class Finish>
std::optional<EngineResult> EngineSetup::ConstructRegion(EngineContext& context, uint64_t global_original,
                                                         CLocalFailure& failure, Finish& finish) {
  auto& world = context.world;
  auto& original = context.original;
  auto& result = *context.control.result;
  auto& stats = result.stats;
  auto& working = *context.ownership.working;
  auto& maximum_node = *context.commit.maximum_node;
  auto& maximum_cell = *context.commit.maximum_cell;
  auto& coarsening = *context.control.coarsening;
  // BEGIN_SOURCE_BODY
  // Fresh regional identities must also exceed frozen donor identities after earlier coarsening.
  for (const auto& donor : original) {
    maximum_cell = std::max(maximum_cell, donor.cell.id);
    for (auto node : donor.cell.v) maximum_node = std::max(maximum_node, node.id);
  }
  maximum_node = TimedAllreduce(maximum_node, CPassiveComm::Op::MAX, world.comm);
  maximum_cell = TimedAllreduce(maximum_cell, CPassiveComm::Op::MAX, world.comm);
  const size_t global_input = TimedAllreduce(working.size(), CPassiveComm::Op::SUM, world.comm);
  // shortcut: a bounded single-worker visible region; partition into connected regions for large sources.
  const size_t region_cells = stats.work_limit;
  const auto scratch = transfer_memory::Add(transfer_memory::Mul(global_input, 8192),
                                            transfer_memory::Mul(region_cells, 16384), RegionFieldWorkspaceBytes);
  if (!context.exchange.Admit(context, scratch)) return std::optional<EngineResult>(finish());

  auto done = EngineSetup::GatherRegion(context, global_original, region_cells, scratch, failure, finish);
  if (done) return done;
  return std::nullopt;
  // END_SOURCE_BODY
}

template <class Finish>
std::optional<EngineResult> EngineSetup::GatherRegion(EngineContext& context, uint64_t global_original,
                                                      size_t region_cells, size_t scratch, CLocalFailure& failure,
                                                      Finish& finish) {
  auto& world = context.world;
  auto& options = context.options;
  auto& geometry = context.geometry;
  auto& result = *context.control.result;
  auto& working = *context.ownership.working;
  // BEGIN_SOURCE_BODY
  std::vector<std::vector<OwnedCell>> buckets(world.size);
  try {
    buckets[0].reserve(working.size());
    for (const auto& row : working) buckets[0].push_back(row.second);
  } catch (const std::exception& e) {
    failure.Set(2, 0, e.what());
  }
  if (!context.exchange.Vote(context, failure)) return std::optional<EngineResult>(finish());
  bool admitted = false;
  auto rows = world.exchange(buckets, &admitted, options.memory_ceiling,
                             transfer_memory::Add(context.exchange.Bytes(context), scratch));
  if (!admitted) {
    result.reason = "Native3D region input exchange exceeds storage.";
    return std::optional<EngineResult>(finish());
  }
  std::vector<DonorRegion> regions;
  Patch old, fresh;
  std::vector<OwnedCell> replacement;
  std::vector<BoundaryParameterEdit> region_parameter_edits;
  bool core_declined = false;
  try {
    if (!world.rank) {
      std::sort(rows.begin(), rows.end(), [](const OwnedCell& a, const OwnedCell& b) { return a.cell.id < b.cell.id; });
      old = GeometryPatch(Imported{rows, true, {}}, &geometry);
      Point lo = rows.front().cell.v[0].p, hi = lo;
      for (const auto& c : old.cells)
        for (auto n : c.v) {
          lo = {std::min(lo.x, n.p.x), std::min(lo.y, n.p.y), std::min(lo.z, n.p.z)};
          hi = {std::max(hi.x, n.p.x), std::max(hi.y, n.p.y), std::max(hi.z, n.p.z)};
        }
      regions.push_back({lo.x, lo.y, lo.z, hi.x, hi.y, hi.z});
    }
  } catch (const std::exception& e) {
    failure.Set(2, 0, e.what());
  }
  if (!context.exchange.Vote(context, failure)) return std::optional<EngineResult>(finish());

  auto done =
      EngineSetup::PrepareRegionField(context, global_original, region_cells, scratch, failure, admitted, rows, regions,
                                      old, fresh, replacement, region_parameter_edits, core_declined, finish);
  if (done) return done;
  return std::nullopt;
  // END_SOURCE_BODY
}

template <class Rows, class Finish>
std::optional<EngineResult> EngineSetup::PrepareRegionField(EngineContext& context, uint64_t global_original,
                                                            size_t region_cells, size_t scratch, CLocalFailure& failure,
                                                            bool& admitted, Rows& rows,
                                                            std::vector<DonorRegion>& regions, Patch& old, Patch& fresh,
                                                            std::vector<OwnedCell>& replacement,
                                                            std::vector<BoundaryParameterEdit>& region_parameter_edits,
                                                            bool& core_declined, Finish& finish) {
  auto& world = context.world;
  auto& options = context.options;
  auto& geometry = context.geometry;
  auto& composition = context.composition;
  auto& result = *context.control.result;
  auto& stats = result.stats;
  auto& source = *context.exchange.source;
  const auto& started = *context.setup.control_prefix.started;
  auto& directory = *context.exchange.directory;
  // BEGIN_SOURCE_BODY
  DonorPatch imported;
  imported.valid = true;
  if (global_original <= MaximumDonors)
    imported =
        source.Import(regions, options.memory_ceiling, transfer_memory::Add(context.exchange.Bytes(context), scratch));
  if (!imported.valid) {
    result.reason = imported.reason;
    return std::optional<EngineResult>(finish());
  }
  RegionReport report;
  auto tick = [&]() {
    if (options.maximum_seconds &&
        std::chrono::duration<double>(Clock::now() - started).count() >= options.maximum_seconds)
      throw std::runtime_error("Native3D region construction exhausts elapsed-time budget.");
  };
  RegionFieldService service(
      world, source.Bootstrap(), geometry, composition, options.edge_breakpoints, options.timings,
      options.memory_ceiling,
      transfer_memory::Add(context.exchange.Bytes(context), scratch, directory->ResidentBytes()),
      [&](const FrozenField& field) { FieldCounts(stats, field); }, tick);
  try {
    service.Run(
        [&](const RegionMetric& target) {
          EngineSetup::EvaluateRegionTarget(context, region_cells, target, rows, old, fresh, report, core_declined,
                                            region_parameter_edits, replacement);
        },
        imported.cells);
  } catch (const std::exception& e) {
    failure.Set(2, 0, e.what());
  }
  auto done = EngineSetup::RunAndFinalizeRegion(context, service, report, failure, scratch, admitted, fresh,
                                                replacement, region_parameter_edits, finish);
  if (done) return done;
  return std::nullopt;
  // END_SOURCE_BODY
}

template <class Finish>
std::optional<EngineResult> EngineSetup::RunAndFinalizeRegion(
    EngineContext& context, RegionFieldService& service, RegionReport& report, CLocalFailure& failure, size_t scratch,
    bool& admitted, Patch& fresh, std::vector<OwnedCell>& replacement,
    std::vector<BoundaryParameterEdit>& region_parameter_edits, Finish& finish) {
  auto& world = context.world;
  auto& options = context.options;
  auto& result = *context.control.result;
  auto& stats = result.stats;
  const auto& started = *context.setup.control_prefix.started;
  auto& composition = context.composition;
  // BEGIN_SOURCE_BODY
  const auto& field_stats = service.Statistics();
  stats.region_field_points = field_stats.point_calls;
  stats.region_field_sensor_hits = field_stats.sensor_hits;
  stats.region_field_interior_hits = field_stats.interior_hits;
  stats.region_field_edges = field_stats.edge_calls;
  stats.region_field_edge_hits = field_stats.edge_hits;
  stats.region_field_imports = field_stats.imports;
  stats.region_field_maximum_resident = field_stats.maximum_resident;
  stats.field_requests += field_stats.sensor_hits + field_stats.sampled_points;
  stats.field_hits += field_stats.sensor_hits;
  stats.field_misses += field_stats.sampled_points;
  if (composition) stats.field_compositions += field_stats.sensor_hits + field_stats.sampled_points;

  stats.region_surface_trials = report.surface_trials;
  stats.region_junction_trials = report.junction_trials;
  stats.region_core_trials = report.core_trials;
  stats.region_core_regions = report.core_regions;
  stats.region_transition_fronts = report.transition_fronts;
  stats.region_front_trials = report.front_trials;
  stats.region_cap_seconds = report.cap_seconds;
  stats.region_sampling_seconds = report.sampling_seconds;
  stats.region_first_band_seconds = report.first_band_seconds;
  stats.region_transition_seconds = report.transition_seconds;
  stats.region_junction_seconds = report.junction_seconds;
  stats.region_core_seconds = report.core_seconds;
  stats.region_validation_seconds = report.validation_seconds;
  stats.global_work = TimedAllreduce(uint64_t(report.generated_cells + report.surface_trials + report.junction_trials +
                                              report.core_trials + report.front_trials),
                                     CPassiveComm::Op::MAX, world.comm);
  if (!context.exchange.Vote(context, failure)) {
    if (world.maximum(report.exhausted_work))
      stats.stop = EngineStop::WorkBudget;
    else if (options.maximum_seconds &&
             world.maximum(std::chrono::duration<double>(Clock::now() - started).count() >= options.maximum_seconds))
      stats.stop = EngineStop::TimeBudget;
    return std::optional<EngineResult>(finish());
  }
  return EngineSetup::FinalizeRegionTail(context, scratch, admitted, failure, fresh, replacement,
                                         region_parameter_edits, finish);
  // END_SOURCE_BODY
}

template <class Finish>
std::optional<EngineResult> EngineSetup::FinalizeRegionTail(EngineContext& context, size_t scratch, bool& admitted,
                                                            CLocalFailure& failure, Patch& fresh,
                                                            std::vector<OwnedCell>& replacement,
                                                            std::vector<BoundaryParameterEdit>& region_parameter_edits,
                                                            Finish& finish) {
  auto& world = context.world;
  auto& options = context.options;
  auto& result = *context.control.result;
  auto& stats = result.stats;
  auto& working = *context.ownership.working;
  auto& selection = *context.census.selection;
  auto& initial = *context.census.initial;
  auto& maximum_node = *context.commit.maximum_node;
  auto& maximum_cell = *context.commit.maximum_cell;
  auto& coarsening = *context.control.coarsening;
  auto& surface_target = context.surface_target;
  auto& region_rebuilt = *context.setup.control_prefix.region_rebuilt;
  auto& region_active = *context.control.region_active;
  auto& region_started = *context.control.region_started;
  // BEGIN_SOURCE_BODY
  bool changed = false;
  auto done = EngineSetup::PublishRegionResult(context, scratch, admitted, failure, fresh, replacement,
                                               region_parameter_edits, changed, finish);
  if (done) return done;
  region_rebuilt = changed;
  stats.global_work = TimedAllreduce(stats.global_work, CPassiveComm::Op::MAX, world.comm);
  if (changed) try {
      initial.clear();
      initial.reserve(working.size());
      for (const auto& row : working) {
        if (!surface_target || selection.RequestedWall(row.second)) initial.push_back(row.first);
        maximum_cell = std::max(maximum_cell, row.first);
        for (auto node : row.second.cell.v) maximum_node = std::max(maximum_node, node.id);
      }
    } catch (const std::exception& e) {
      failure.Set(2, 0, e.what());
    }
  if (!context.exchange.Vote(context, failure)) return std::optional<EngineResult>(finish());
  maximum_node = TimedAllreduce(maximum_node, CPassiveComm::Op::MAX, world.comm);
  maximum_cell = TimedAllreduce(maximum_cell, CPassiveComm::Op::MAX, world.comm);
  // Valid coarsening inputs and finer interior fields retain the existing volume search.
  if (options.timings)
    stats.reconstruction_seconds += std::chrono::duration<double>(Clock::now() - region_started).count();
  region_active = false;
  result.valid = false;
  result.reason.clear();
  return std::nullopt;
  // END_SOURCE_BODY
}

template <class Finish>
std::optional<EngineResult> EngineSetup::PublishRegionResult(EngineContext& context, size_t scratch, bool& admitted,
                                                             CLocalFailure& failure, Patch& fresh,
                                                             std::vector<OwnedCell>& replacement,
                                                             std::vector<BoundaryParameterEdit>& region_parameter_edits,
                                                             bool& changed, Finish& finish) {
  auto& world = context.world;
  auto& options = context.options;
  auto& geometry = context.geometry;
  auto& result = *context.control.result;
  auto& stats = result.stats;
  auto& working = *context.ownership.working;
  auto& directory = *context.exchange.directory;
  auto& wall_counters = *context.setup.wall_counters;
  // BEGIN_SOURCE_BODY
  changed = world.maximum(!replacement.empty());
  if (changed) {
    std::vector<std::vector<OwnedCell>> to(world.size);
    try {
      for (auto row : replacement) to[row.cell.id % world.size].push_back(row);
    } catch (const std::exception& e) {
      failure.Set(2, 0, e.what());
    }
    if (!context.exchange.Vote(context, failure)) return std::optional<EngineResult>(finish());
    const auto added = world.exchange(to, &admitted, options.memory_ceiling,
                                      transfer_memory::Add(context.exchange.Bytes(context), scratch));
    if (!admitted) {
      result.reason = "Native3D region result exchange exceeds storage.";
      return std::optional<EngineResult>(finish());
    }
    std::vector<Dependency> removed;
    try {
      for (const auto& row : working) removed.push_back({row.first, row.second.version, world.rank});
    } catch (const std::exception& e) {
      failure.Set(2, 0, e.what());
    }
    if (!context.exchange.Vote(context, failure)) return std::optional<EngineResult>(finish());
    if (!directory->Publish(working, removed, added, true, result.reason, options.memory_ceiling,
                            transfer_memory::Add(context.exchange.RealBytes(context), scratch), region_parameter_edits,
                            fresh.coverage, geometry.AtlasReservation()))
      return std::optional<EngineResult>(finish());
    if (!world.rank) {
      ++stats.accepted;
      wall_counters.Accepted();
    }
    stats.maximum_owned = std::max(stats.maximum_owned, working.size());
    stats.region_cells = TimedAllreduce(working.size(), CPassiveComm::Op::SUM, world.comm);
  }

  return std::nullopt;
  // END_SOURCE_BODY
}

template <class Rows>
void EngineSetup::EvaluateRegionTarget(EngineContext& context, size_t region_cells, const RegionMetric& target,
                                       Rows& rows, Patch& old, Patch& fresh, RegionReport& report, bool& core_declined,
                                       std::vector<BoundaryParameterEdit>& region_parameter_edits,
                                       std::vector<OwnedCell>& replacement) {
  auto& options = context.options;
  auto& geometry = context.geometry;
  auto& result = *context.control.result;
  auto& stats = result.stats;
  auto& coarsening = *context.control.coarsening;
  // BEGIN_SOURCE_BODY
  bool already_valid = GeometryGuidanceExcess(old, geometry, options.surface_tolerance).excess == 0;
  bool short_edges = false;
  if (already_valid)
    for (const auto& c : old.cells) {
      for (unsigned a = 0; a < 4 && already_valid; ++a)
        for (unsigned b = a + 1; b < 4; ++b) {
          const auto length = target.edge(c.v[a].p, c.v[b].p);
          if (length > MaximumMetricEdge) {
            already_valid = false;
            break;
          }
          short_edges = short_edges || (coarsening && length < CoarseningMetricEdge);
        }
      if (!already_valid) break;
    }
  if (!already_valid || short_edges) {
    ++stats.region_attempts;
    std::map<Id, Point> points;
    for (const auto& c : old.cells)
      for (auto n : c.v) points.emplace(n.id, n.p);
    std::vector<Facet> skin;
    EngineSetup::PrepareRegionSkin(geometry, old, points, skin);
    bool constructed = false;
    EngineSetup::ConstructRegionVolume(context, region_cells, old, skin, target, fresh, report, constructed);
    EngineSetup::RecordRegionResult(context, old, fresh, rows, report, already_valid, short_edges, constructed,
                                    core_declined, region_parameter_edits, replacement);
  }
  // END_SOURCE_BODY
}

void EngineSetup::PrepareRegionSkin(const PlanarReference& geometry, const Patch& old, std::map<Id, Point>& points,
                                    std::vector<Facet>& skin) {
  // BEGIN_SOURCE_BODY
  for (auto f : old.surface) {
    Facet face;
    face.id = f.facet;
    face.marker = geometry.Get(f.facet).marker;
    for (unsigned k = 0; k < 3; ++k) face.v[k] = {f.v[k], points.at(f.v[k])};
    skin.push_back(face);
  }
  std::sort(skin.begin(), skin.end(), [](const Facet& a, const Facet& b) {
    return Key({a.v[0].id, a.v[1].id, a.v[2].id}) < Key({b.v[0].id, b.v[1].id, b.v[2].id});
  });
  // END_SOURCE_BODY
}

void EngineSetup::ConstructRegionVolume(EngineContext& context, size_t region_cells, Patch& old,
                                        const std::vector<Facet>& skin, const RegionMetric& target, Patch& fresh,
                                        RegionReport& report, bool& constructed) {
  auto& geometry = context.geometry;
  auto& options = context.options;
  auto& result = *context.control.result;
  auto& stats = result.stats;
  auto& maximum_node = *context.commit.maximum_node;
  auto& maximum_cell = *context.commit.maximum_cell;
  const auto& started = *context.setup.control_prefix.started;
  // BEGIN_SOURCE_BODY
  // Surface integrals give one partition-independent region kernel.
  const Point origin = old.cells.front().v[0].p;
  long double volume = 0, x = 0, y = 0, z = 0;
  for (const auto& f : skin) {
    const auto a = f.v[0].p, b = f.v[1].p, c = f.v[2].p;
    const long double v = Orientation(origin, a, b, c);
    volume += v;
    x += v * ((long double)origin.x + a.x + b.x + c.x) / 4;
    y += v * ((long double)origin.y + a.y + b.y + c.y) / 4;
    z += v * ((long double)origin.z + a.z + b.z + c.z) / 4;
  }
  if (!(volume > 0)) throw std::runtime_error("Native3D region has nonpositive closed volume.");
  const Point kernel{double(x / volume), double(y / volume), double(z / volume)};
  if (maximum_node == UINT64_MAX || maximum_cell == UINT64_MAX)
    throw std::overflow_error("Native3D region identity reservation overflows.");
  RegionLimits limits;
  limits.maximum_cells = region_cells;
  limits.maximum_work = stats.work_limit;
  limits.timings = options.timings;
  if (options.rejected_trial)
    limits.rejected = [&](const Patch& candidate, const std::string& why) {
      options.rejected_trial(old, candidate, {}, MetricDiagnosis{}, false, why);
    };
  limits.checkpoint = [&]() {
    if (options.maximum_seconds &&
        std::chrono::duration<double>(Clock::now() - started).count() >= options.maximum_seconds)
      throw std::runtime_error("Native3D region construction exhausts elapsed-time budget.");
  };
  constructed = ConstructWallRegion(skin, kernel, geometry, options.region_walls, options.surface_tolerance,
                                    options.wall_heights.tolerance, maximum_node + 1, maximum_cell + 1, target, limits,
                                    fresh, report, result.reason);
  // END_SOURCE_BODY
}

template <class Rows>
void EngineSetup::RecordRegionResult(EngineContext& context, Patch& old, Patch& fresh, Rows& rows, RegionReport& report,
                                     bool already_valid, bool short_edges, bool constructed, bool& core_declined,
                                     std::vector<BoundaryParameterEdit>& region_parameter_edits,
                                     std::vector<OwnedCell>& replacement) {
  auto& options = context.options;
  auto& geometry = context.geometry;
  auto& result = *context.control.result;
  auto& stats = result.stats;
  auto& coarsening = *context.control.coarsening;
  auto& last_rejection = *context.control.last_rejection;
  // BEGIN_SOURCE_BODY
  if (!constructed) {
    // A height-free cone cannot resolve every interior field; retain the existing volume operators there.
    core_declined =
        report.unsupported_topology || (options.region_walls.empty() && !report.exhausted_work &&
                                        (report.unresolved_core || report.maximum_edge > MaximumMetricEdge));
    if (!core_declined) throw std::runtime_error(result.reason);
    last_rejection = result.reason;
  } else if (coarsening && already_valid && fresh.cells.size() >= old.cells.size()) {
    core_declined = true;
    last_rejection = "Native3D regional coarsening did not reduce the cell count.";
  } else {
    stats.global_work =
        fresh.cells.size() + report.surface_trials + report.junction_trials + report.core_trials + report.front_trials;
    if (stats.global_work > stats.work_limit)
      throw std::runtime_error("Native3D region construction exceeds global event work.");
    if (geometry.Mapped()) {
      geometry.ExportParameters(old);
      geometry.ExportParameters(fresh);
      region_parameter_edits = BoundaryParameterState::Changes(old, fresh, geometry.Parameters()->Revision());
    }
    std::map<Face, Id> physical;
    for (auto f : fresh.surface) physical.emplace(Key(f.v), f.facet);
    for (const auto& c : fresh.cells) {
      OwnedCell row;
      row.cell = c;
      const auto faces = Faces(c);
      for (unsigned f = 0; f < 4; ++f) {
        auto found = physical.find(Key(faces[f]));
        if (found != physical.end()) row.physical[f] = {true, found->second};
      }
      replacement.push_back(row);
    }
    PublishMetrics(rows, replacement);
  }
  // END_SOURCE_BODY
}

bool EngineSetup::PlanFirstRows(EngineContext& context, std::set<Id>& pending, CLocalFailure& failure,
                                const size_t& layerScratch, std::vector<Feature>& features, std::vector<Id>& queued,
                                DependencyBatch& dependencies, ImportedBatch& imported, ReservationBatch& reserved) {
  auto& world = context.world;
  auto& options = context.options;
  auto& geometry = context.geometry;
  auto& result = *context.control.result;
  auto& stats = result.stats;
  auto& working = *context.ownership.working;
  auto& directory = *context.exchange.directory;
  auto& source = *context.exchange.source;
  // BEGIN_MOVED_BODY
  Timer timer(stats.layer_metadata_seconds, options.timings, SU2NativeTiming3D::Operation::Topology);
  const uint64_t available = std::min(pending.size(), options.candidate_batch);
  const auto position = CPassiveComm::ExscanSum(available, world.comm);
  const auto remaining = stats.work_limit - stats.global_work;
  const size_t count = position < remaining ? std::min(available, remaining - position) : 0;
  stats.global_work += TimedAllreduce(uint64_t(count), CPassiveComm::Op::SUM, world.comm);
  try {
    auto it = pending.begin();
    for (size_t i = 0; i < count; ++i, ++it) {
      const auto& row = working.at(*it);
      const auto faces = Faces(row.cell);
      bool found = false;
      for (unsigned f = 0; f < 4; ++f)
        if (row.physical[f].present && options.wall_heights.count(geometry.Get(row.physical[f].facet).marker)) {
          features.push_back({faces[f], 3});
          queued.push_back(row.cell.id);
          found = true;
          break;
        }
      if (!found) throw std::runtime_error("Native3D first-row queue lost its wall face.");
    }
  } catch (const std::exception& e) {
    failure.Set(2, 0, e.what());
  }
  if (!context.exchange.Vote(context, failure)) return false;
  ++stats.layer_batches;
  stats.layer_maximum_batch = std::max(stats.layer_maximum_batch, features.size());
  const auto held = transfer_memory::Add(context.exchange.Bytes(context), source.ResidentBytes(), layerScratch);
  dependencies = directory->LookupBatch(features, options.memory_ceiling, held);
  if (!dependencies.valid) {
    result.reason = dependencies.reason;
    return false;
  }
  imported = directory->FetchBatch(dependencies, working, options.memory_ceiling, held);
  if (!imported.valid) {
    result.reason = imported.reason;
    return false;
  }
  try {
    for (size_t i = 0; i < imported.candidates.size(); ++i) {
      const auto& item = imported.candidates[i];
      if (!item.valid || item.cells.size() != 1 || item.cells.front().cell.id != queued.at(i))
        throw std::runtime_error("Native3D first-row batch lacks its complete queued cell.");
    }
  } catch (const std::exception& e) {
    failure.Set(2, 0, e.what());
  }
  if (!context.exchange.Vote(context, failure)) return false;
  // These operators preserve every old face/vertex; only the cell interiors are written.
  reserved = directory->ReserveInteriors(imported, options.memory_ceiling, held);
  if (!reserved.valid) {
    result.reason = reserved.reason;
    return false;
  }
  if (!world.maximum(std::any_of(reserved.chosen.begin(), reserved.chosen.end(), [](bool x) { return x; }))) {
    result.reason = "Native3D first-row batch has no reserved cell.";
    return false;
  }
  return true;
  // END_MOVED_BODY
}

void EngineSetup::PrepareFirstRowPublication(EngineContext& context, CLocalFailure& failure,
                                             std::vector<Patch>& proposals, ReservationBatch& reserved,
                                             ImportedBatch& imported, DependencyBatch& dependencies,
                                             std::vector<std::vector<Dependency>>& toRemove,
                                             std::vector<std::vector<OwnedCell>>& toAdd, size_t& published,
                                             std::vector<BoundaryParameterEdit>& layer_parameter_edits,
                                             SU2NativeGeometry::BoundaryCoverage& layer_coverage) {
  auto& geometry = context.geometry;
  // BEGIN_MOVED_BODY
  try {
    for (size_t k = 0; k < proposals.size(); ++k)
      if (reserved.chosen[k]) {
        const auto& old = imported.candidates[k].cells.front().cell;
        const auto& fresh = proposals[k];
        const auto& dep = dependencies.candidates[k].cells.front();
        if (dep.cell != old.id || dependencies.candidates[k].cells.size() != 1 || fresh.cells.empty())
          throw std::runtime_error("Native3D first-row publication lacks its complete replacement.");
        if (geometry.Mapped()) {
          auto before = GeometryPatch(imported.candidates[k], &geometry);
          auto after = fresh;
          geometry.ExportParameters(after);
          const auto changes = BoundaryParameterState::Changes(before, after, geometry.Parameters()->Revision());
          layer_parameter_edits.insert(layer_parameter_edits.end(), changes.begin(), changes.end());
          for (const auto& cover : after.coverage.faces)
            if (!layer_coverage.faces.emplace(cover).second)
              throw std::invalid_argument("Repeated physical face in layer atlas publication.");
          for (const auto& trace : after.coverage.traces) {
            const auto previous = layer_coverage.traces.find(trace.first);
            std::string why;
            if (previous == layer_coverage.traces.end())
              layer_coverage.traces.emplace(trace);
            else if (!geometry.AtlasReservation() ||
                     !geometry.Parameters()->Geometry().EqualTraces(previous->second, trace.second,
                                                                    *geometry.AtlasReservation(), why))
              throw std::invalid_argument("Conflicting inherited layer atlas edge: " + why);
          }
        }
        toRemove.at(dep.owner).push_back(dep);
        std::map<Face, Id> physical;
        for (const auto& f : fresh.surface) physical.emplace(Key(f.v), f.facet);
        for (const auto& c : fresh.cells) {
          OwnedCell row;
          row.cell = c;
          const auto faces = Faces(row.cell);
          for (unsigned f = 0; f < 4; ++f) {
            const auto found = physical.find(Key(faces[f]));
            if (found != physical.end()) row.physical[f] = {true, found->second};
          }
          std::vector<OwnedCell> metric_rows{row};
          PublishMetrics(imported.candidates.at(k).cells, metric_rows);
          toAdd.at(dep.owner).push_back(metric_rows.front());
        }
        ++published;
      }
  } catch (const std::exception& e) {
    failure.Set(2, 0, e.what());
  }
  // END_MOVED_BODY
}

template <class NeedsLayer>
bool EngineSetup::PublishFirstRows(EngineContext& context, CLocalFailure& failure, std::set<Id>& pending,
                                   const size_t& layerScratch, uint64_t& epoch, std::vector<Patch>& proposals,
                                   ReservationBatch& reserved, ImportedBatch& imported, DependencyBatch& dependencies,
                                   NeedsLayer& needs_layer) {
  auto& world = context.world;
  auto& options = context.options;
  auto& geometry = context.geometry;
  auto& result = *context.control.result;
  auto& stats = result.stats;
  auto& working = *context.ownership.working;
  auto& directory = *context.exchange.directory;
  auto& source = *context.exchange.source;
  // BEGIN_MOVED_BODY
  Timer timer(stats.layer_publication_seconds, options.timings, SU2NativeTiming3D::Operation::Communication);
  std::vector<std::vector<Dependency>> toRemove(world.size);
  std::vector<std::vector<OwnedCell>> toAdd(world.size);
  size_t published = 0;
  std::vector<BoundaryParameterEdit> layer_parameter_edits;
  SU2NativeGeometry::BoundaryCoverage layer_coverage;
  EngineSetup::PrepareFirstRowPublication(context, failure, proposals, reserved, imported, dependencies, toRemove,
                                          toAdd, published, layer_parameter_edits, layer_coverage);
  if (!context.exchange.Vote(context, failure)) return false;
  bool admitted = false;
  const auto baseline = transfer_memory::Add(context.exchange.Bytes(context), source.ResidentBytes(),
                                             directory->ResidentBytes(), layerScratch);
  const auto removed = world.exchange(toRemove, &admitted, options.memory_ceiling, baseline);
  if (!admitted) {
    result.reason = "Native3D first-row removal routing exceeds budget.";
    return false;
  }
  const auto added = world.exchange(toAdd, &admitted, options.memory_ceiling, baseline);
  if (!admitted) {
    result.reason = "Native3D first-row addition routing exceeds budget.";
    return false;
  }
  if (!directory->Publish(
          working, removed, added, true, result.reason, options.memory_ceiling,
          transfer_memory::Add(context.exchange.RealBytes(context), source.ResidentBytes(), layerScratch),
          layer_parameter_edits, layer_coverage, geometry.AtlasReservation()))
    return false;
  try {
    for (const auto& row : removed) pending.erase(row.cell);
    for (const auto& row : added)
      if (needs_layer(row)) pending.insert(row.cell.id);
  } catch (const std::exception& e) {
    failure.Set(2, 0, e.what());
  }
  if (!context.exchange.Vote(context, failure)) return false;
  stats.layer_published += published;
  ++epoch;
  stats.maximum_owned = std::max(stats.maximum_owned, working.size());
  return true;
  // END_MOVED_BODY
}

template <class JointCandidate>
bool EngineSetup::ImportFirstRowDonors(EngineContext& context, CLocalFailure& failure, const size_t& layerScratch,
                                       size_t at, const size_t& slots, size_t& group, size_t& span, DonorPatch& donors,
                                       ImportedBatch& imported, ReservationBatch& reserved,
                                       JointCandidate& joint_candidate) {
  auto& world = context.world;
  auto& options = context.options;
  auto& result = *context.control.result;
  auto& stats = result.stats;
  auto& source = *context.exchange.source;
  // BEGIN_MOVED_BODY
  Timer timer(stats.layer_donor_seconds, options.timings, SU2NativeTiming3D::Operation::Import);
  for (;;) {
    std::vector<Cell> query;
    std::vector<OwnedCell> metric_context;
    try {
      for (size_t k = at; k < std::min(at + group, imported.candidates.size()); ++k)
        if (reserved.chosen[k] && joint_candidate(imported.candidates[k])) {
          query.push_back(imported.candidates[k].cells.front().cell);
          metric_context.push_back(imported.candidates[k].cells.front());
        }
    } catch (const std::exception& e) {
      failure.Set(2, 0, e.what());
    }
    if (!context.exchange.Vote(context, failure)) return false;
    source.SetCavity(&metric_context);
    donors = context.exchange.ImportSource(context, query, layerScratch, true);
    source.SetCavity(nullptr);
    if (donors.valid) break;
    if (!DonorCap(donors.reason) || group == 1) {
      result.reason = donors.reason;
      return false;
    }
    span = (group + 1) / 2;
    group = std::min(span, slots - at);
    ++stats.layer_donor_retries;
  }
  // END_MOVED_BODY
  return true;
}

void EngineSetup::FirstRowBackgrounds(EngineContext& context, const Patch& old, std::vector<Point>& backgrounds,
                                      std::map<Id, Point>& coordinates) {
  auto& geometry = context.geometry;
  auto& options = context.options;
  // BEGIN_MOVED_BODY
  for (const auto& c : old.cells)
    for (auto n : c.v) coordinates.emplace(n.id, n.p);
  for (const auto& face : old.surface) {
    const auto h = options.wall_heights.find(geometry.Get(face.facet).marker);
    if (h == options.wall_heights.end()) continue;
    const auto a = coordinates.at(face.v[0]), b = coordinates.at(face.v[1]), c = coordinates.at(face.v[2]);
    const std::array<long double, 3> u{(long double)b.x - a.x, (long double)b.y - a.y, (long double)b.z - a.z},
        v{(long double)c.x - a.x, (long double)c.y - a.y, (long double)c.z - a.z};
    auto normal =
        std::array<long double, 3>{u[1] * v[2] - u[2] * v[1], u[2] * v[0] - u[0] * v[2], u[0] * v[1] - u[1] * v[0]};
    const auto length = std::hypot(normal[0], normal[1], normal[2]);
    for (auto& x : normal) x /= length;
    for (double factor : {1.25, 1.5, 2.})
      backgrounds.push_back({double(((long double)a.x + b.x + c.x) / 3 - factor * h->second * normal[0]),
                             double(((long double)a.y + b.y + c.y) / 3 - factor * h->second * normal[1]),
                             double(((long double)a.z + b.z + c.z) / 3 - factor * h->second * normal[2])});
  }
  // END_MOVED_BODY
}

void EngineSetup::ScoreFirstRowBackgrounds(EngineContext& context, const Patch& old, Patch& fresh, Id node, Id cell,
                                           std::unique_ptr<FrozenField>& target, std::vector<Point>& backgrounds,
                                           long double& bestLength, std::map<Edge, EdgeMeasure>& reuse,
                                           bool& approved) {
  auto& geometry = context.geometry;
  auto& options = context.options;
  auto& result = *context.control.result;
  auto& stats = result.stats;
  // BEGIN_MOVED_BODY
  for (const auto& background : backgrounds) {
    Patch candidate;
    FirstHeightReport report;
    std::string why;
    bool built = false;
    {
      Timer timer(stats.layer_geometry_seconds, options.timings, SU2NativeTiming3D::Operation::GeometryBound);
      built = ConstructFirstLayer(old, {node, background}, node + 1, cell, options.wall_heights, geometry, candidate,
                                  report, why);
      if (built && !SameSkin(old, candidate))
        throw std::runtime_error("Native3D interior first-row replacement changed its interface.");
    }
    if (!built) {
      if (!approved) result.reason = why;
      if (why.find("geometry-height incompatibility") != std::string::npos) break;
      continue;
    }
    MetricStats counted;
    PatchMetric measured;
    {
      Timer timer(stats.layer_metric_seconds, options.timings, SU2NativeTiming3D::Operation::MetricNormalization);
      measured = MeasureMetricEdges(candidate.cells, *target, &counted, &reuse);
    }
    reuse = measured.edges;
    stats.layer_edge_evaluations += counted.edge_evaluations;
    stats.layer_edge_reused += counted.edge_reused;
    if (measured.maximum_length < bestLength) {
      bestLength = measured.maximum_length;
      fresh = std::move(candidate);
      approved = true;
      result.reason.clear();
    }
  }
  // END_MOVED_BODY
}

void EngineSetup::ConstructFirstRowJoint(EngineContext& context, const Patch& old, Patch& fresh, Id node, Id cell,
                                         std::unique_ptr<FrozenField>& target, bool& approved) {
  // BEGIN_MOVED_BODY
  std::vector<Point> backgrounds{Centroid(old.cells.front())};
  std::map<Id, Point> coordinates;
  EngineSetup::FirstRowBackgrounds(context, old, backgrounds, coordinates);
  long double bestLength = std::numeric_limits<long double>::infinity();
  std::map<Edge, EdgeMeasure> reuse;
  EngineSetup::ScoreFirstRowBackgrounds(context, old, fresh, node, cell, target, backgrounds, bestLength, reuse,
                                        approved);
  // END_MOVED_BODY
}

template <class JointCandidate>
void EngineSetup::ConstructFirstRowCandidate(EngineContext& context, CLocalFailure& failure, uint64_t& epoch, size_t k,
                                             ImportedBatch& imported, std::vector<Patch>& proposals,
                                             std::unique_ptr<FrozenField>& target, JointCandidate& joint_candidate,
                                             bool& rejected) {
  auto& world = context.world;
  auto& geometry = context.geometry;
  auto& options = context.options;
  auto& result = *context.control.result;
  auto& stats = result.stats;
  auto& maximum_node = *context.commit.maximum_node;
  auto& maximum_cell = *context.commit.maximum_cell;
  // BEGIN_MOVED_BODY
  const auto old = GeometryPatch(imported.candidates[k], &geometry);
  auto& fresh = proposals[k];
  bool approved = false;
  const auto ticket = transfer_memory::Add(
      transfer_memory::Mul(transfer_memory::Add(transfer_memory::Mul(epoch, world.size), world.rank),
                           options.candidate_batch),
      k);
  const auto nodeOffset = transfer_memory::Mul(ticket, 5),
             cellOffset = transfer_memory::Mul(ticket, MaximumReplacementCells);
  if (maximum_node > UINT64_MAX - 5 || nodeOffset > UINT64_MAX - maximum_node - 5 ||
      maximum_cell > UINT64_MAX - MaximumReplacementCells ||
      cellOffset > UINT64_MAX - maximum_cell - MaximumReplacementCells)
    throw std::invalid_argument("Native3D first-row reserved range overflows.");
  if (old.cells.size() != 1 || old.surface.size() > 4)
    throw std::invalid_argument("Native3D first-row identity reservation requires one complete wall cell.");
  const Id node = maximum_node + 1 + nodeOffset, cell = maximum_cell + 1 + cellOffset;
  if (!joint_candidate(imported.candidates[k])) {
    FirstHeightReport report;
    {
      Timer timer(stats.layer_geometry_seconds, options.timings, SU2NativeTiming3D::Operation::GeometryBound);
      approved =
          InsertFirstWallPoint(old, node + 1, cell, options.wall_heights, geometry, fresh, report, result.reason);
      if (approved && !SameSkin(old, fresh))
        throw std::runtime_error("Native3D interior first-row replacement changed its interface.");
    }
  } else {
    EngineSetup::ConstructFirstRowJoint(context, old, fresh, node, cell, target, approved);
  }
  if (!approved) {
    failure.Set(1, old.cells.front().id, "Native3D initial first-row construction rejected: " + result.reason);
    rejected = true;
  }
  // END_MOVED_BODY
}

template <class TimedOut, class JointCandidate>
bool EngineSetup::ConstructFirstRowGroup(EngineContext& context, CLocalFailure& failure, const size_t& layerScratch,
                                         uint64_t& epoch, const size_t& slots, size_t& at, size_t& span,
                                         const bool& ranked_background, ImportedBatch& imported,
                                         ReservationBatch& reserved, std::vector<Id>& queued,
                                         std::vector<Patch>& proposals, TimedOut& timed_out,
                                         JointCandidate& joint_candidate) {
  auto& world = context.world;
  auto& geometry = context.geometry;
  auto& options = context.options;
  auto& composition = context.composition;
  auto& result = *context.control.result;
  auto& stats = result.stats;
  auto& field_identity = *context.census.field_identity;
  const auto& started = *context.setup.control_prefix.started;
  // BEGIN_MOVED_BODY
  if (timed_out()) return false;
  size_t group = std::min(span, slots - at);
  DonorPatch donors;
  if (ranked_background) {
    if (!EngineSetup::ImportFirstRowDonors(context, failure, layerScratch, at, slots, group, span, donors, imported,
                                           reserved, joint_candidate))
      return false;
  }
  std::unique_ptr<FrozenField> target;
  std::unique_ptr<FieldAccounting> accounting;
  try {
    if (ranked_background) {
      Timer timer(stats.layer_metric_seconds, options.timings, SU2NativeTiming3D::Operation::MetricNormalization);
      const auto components = AssociatedExtensions(donors.cells, geometry);
      target = std::make_unique<FrozenField>(std::move(donors.cells), composition, options.timings, components,
                                             options.edge_breakpoints, field_identity, true, MaximumDonors, true);
      accounting.reset(new FieldAccounting{stats, *target});
    }
  } catch (const std::exception& e) {
    failure.Set(2, 0, e.what());
  }
  if (!context.exchange.Vote(context, failure)) return false;
  bool rejected = false, expired = false;
  auto local_deadline = [&]() {
    return options.maximum_seconds &&
           std::chrono::duration<double>(Clock::now() - started).count() >= options.maximum_seconds;
  };
  for (size_t k = at; k < at + group; ++k) {
    if (local_deadline()) {
      expired = true;
      break;
    }
    if (k < imported.candidates.size() && reserved.chosen[k]) try {
        EngineSetup::ConstructFirstRowCandidate(context, failure, epoch, k, imported, proposals, target,
                                                joint_candidate, rejected);
      } catch (const std::exception& e) {
        failure.Set(2, k < queued.size() ? queued[k] : 0, e.what());
        rejected = true;
      }
    if (rejected) break;
    if (local_deadline()) {
      expired = true;
      break;
    }
  }
  // Private operators/query fields are local. Vote once per admitted group, while each worker checks its deadline per
  // candidate.
  if (!context.exchange.Vote(context, failure)) return false;
  if (world.maximum(expired)) {
    stats.stop = EngineStop::TimeBudget;
    result.reason = "Native3D first-row initialization exhausts elapsed-time budget.";
    return false;
  }
  at += group;
  // END_MOVED_BODY
  return true;
}

template <class TimedOut, class JointCandidate>
bool EngineSetup::ConstructFirstRows(EngineContext& context, CLocalFailure& failure, const size_t& layerScratch,
                                     uint64_t& epoch, const size_t& slots, size_t& span, const bool& ranked_background,
                                     ImportedBatch& imported, ReservationBatch& reserved, std::vector<Id>& queued,
                                     std::vector<Patch>& proposals, TimedOut& timed_out,
                                     JointCandidate& joint_candidate) {
  // BEGIN_MOVED_BODY
  for (size_t at = 0; at < slots;) {
    if (!EngineSetup::ConstructFirstRowGroup(context, failure, layerScratch, epoch, slots, at, span, ranked_background,
                                             imported, reserved, queued, proposals, timed_out, joint_candidate))
      return false;
  }
  return true;
  // END_MOVED_BODY
}

template <class NeedsLayer, class Finish>
std::optional<EngineResult> EngineSetup::IterateFirstRows(EngineContext& context, CLocalFailure& failure,
                                                          std::set<Id>& pending, const size_t& layerScratch,
                                                          uint64_t& epoch, NeedsLayer& needs_layer, Finish& finish) {
  auto& world = context.world;
  auto& options = context.options;
  auto& geometry = context.geometry;
  auto& result = *context.control.result;
  auto& stats = result.stats;
  const auto& started = *context.setup.control_prefix.started;
  // BEGIN_MOVED_BODY
  while (world.maximum(!pending.empty())) {
    if (stats.global_work >= stats.work_limit) {
      stats.stop = EngineStop::WorkBudget;
      result.reason = "Native3D first-row initialization exhausts global work budget.";
      return std::optional<EngineResult>(finish());
    }
    auto timed_out = [&]() {
      if (options.maximum_seconds &&
          world.maximum(std::chrono::duration<double>(Clock::now() - started).count() >= options.maximum_seconds)) {
        stats.stop = EngineStop::TimeBudget;
        result.reason = "Native3D first-row initialization exhausts elapsed-time budget.";
        return true;
      }
      return false;
    };
    if (timed_out()) return std::optional<EngineResult>(finish());
    std::vector<Feature> features;
    std::vector<Id> queued;
    DependencyBatch dependencies;
    ImportedBatch imported;
    ReservationBatch reserved;
    if (!EngineSetup::PlanFirstRows(context, pending, failure, layerScratch, features, queued, dependencies, imported,
                                    reserved))
      return std::optional<EngineResult>(finish());
    std::vector<Patch> proposals;
    try {
      proposals.resize(imported.candidates.size());
    } catch (const std::exception& e) {
      failure.Set(2, 0, e.what());
    }
    if (!context.exchange.Vote(context, failure)) return std::optional<EngineResult>(finish());
    const size_t slots = TimedAllreduce(imported.candidates.size(), CPassiveComm::Op::MAX, world.comm);
    auto joint_candidate = [&](const auto& item) {
      unsigned walls = 0;
      for (const auto& face : item.cells.front().physical)
        walls += face.present && options.wall_heights.count(geometry.Get(face.facet).marker);
      return walls > 1;
    };
    bool local_joint = false;
    for (size_t k = 0; k < imported.candidates.size(); ++k)
      if (reserved.chosen[k]) local_joint = local_joint || joint_candidate(imported.candidates[k]);
    const bool ranked_background = world.maximum(local_joint);
    size_t span = options.candidate_batch;
    if (!EngineSetup::ConstructFirstRows(context, failure, layerScratch, epoch, slots, span, ranked_background,
                                         imported, reserved, queued, proposals, timed_out, joint_candidate))
      return std::optional<EngineResult>(finish());
    if (!EngineSetup::PublishFirstRows(context, failure, pending, layerScratch, epoch, proposals, reserved, imported,
                                       dependencies, needs_layer))
      return std::optional<EngineResult>(finish());
  }
  // END_MOVED_BODY
  return std::nullopt;
}

template <class Finish>
std::optional<EngineResult> EngineSetup::InitializeFirstRows(EngineContext& context, Finish& finish) {
  auto& world = context.world;
  auto& options = context.options;
  auto& geometry = context.geometry;
  auto& surface_target = context.surface_target;
  auto& result = *context.control.result;
  auto& stats = result.stats;
  auto& working = *context.ownership.working;
  auto& selection = *context.census.selection;
  auto& initial = *context.census.initial;
  auto& maximum_node = *context.commit.maximum_node;
  auto& maximum_cell = *context.commit.maximum_cell;
  auto& layer_active = *context.control.layer_active;
  auto& layer_started = *context.control.layer_started;
  // BEGIN_MOVED_BODY
  if (!surface_target && !options.wall_heights.empty()) {
    layer_active = true;
    layer_started = Clock::now();
    CLocalFailure failure;
    std::set<Id> pending;
    const auto layerScratch = transfer_memory::Add(
        transfer_memory::Mul(working.size(), sizeof(Id) + 4 * sizeof(void*)),
        transfer_memory::Mul(transfer_memory::Mul(options.candidate_batch, MaximumReplacementCells), 8192),
        MaximumFrozenFieldBytes);
    if (!context.exchange.Admit(context, layerScratch)) return std::optional<EngineResult>(finish());
    auto needs_layer = [&](const OwnedCell& row) {
      bool wall = false;
      for (const auto& f : row.physical)
        wall = wall || (f.present && options.wall_heights.count(geometry.Get(f.facet).marker));
      if (!wall) return false;
      std::string reason;
      return !EngineTrial::HeightCheck(context, GeometryPatch(Imported{{row}, true, {}}, &geometry), reason);
    };
    try {
      for (const auto& row : working)
        if (needs_layer(row.second)) pending.insert(row.first);
    } catch (const std::exception& e) {
      failure.Set(2, 0, e.what());
    }
    if (!context.exchange.Vote(context, failure)) return std::optional<EngineResult>(finish());
    uint64_t epoch = 0;
    auto layer_done =
        EngineSetup::IterateFirstRows(context, failure, pending, layerScratch, epoch, needs_layer, finish);
    if (layer_done) return layer_done;
    try {
      initial.clear();
      initial.reserve(working.size());
      for (const auto& row : working) {
        if (!surface_target || selection.RequestedWall(row.second)) initial.push_back(row.first);
        maximum_cell = std::max(maximum_cell, row.first);
        for (auto n : row.second.cell.v) maximum_node = std::max(maximum_node, n.id);
      }
    } catch (const std::exception& e) {
      failure.Set(2, 0, e.what());
    }
    if (!context.exchange.Vote(context, failure)) return std::optional<EngineResult>(finish());
    maximum_node = TimedAllreduce(maximum_node, CPassiveComm::Op::MAX, world.comm);
    maximum_cell = TimedAllreduce(maximum_cell, CPassiveComm::Op::MAX, world.comm);
    if (options.timings) stats.layer_seconds += std::chrono::duration<double>(Clock::now() - layer_started).count();
    layer_active = false;
  }
  // END_MOVED_BODY
  return std::nullopt;
}

void EngineSetup::BeginConstructionPass(EngineContext& context) {
  if (context.bidirectional) (*context.exchange.directory)->BeginConstructionPass();
}

bool EngineSetup::SeedFrontier(EngineContext& context, CLocalFailure& failure) {
  try {
    context.census.selection->SeedFrontier(*context.ownership.working);
  } catch (const std::exception& e) {
    failure.Set(2, 0, e.what());
  }
  return context.exchange.Vote(context, failure);
}
