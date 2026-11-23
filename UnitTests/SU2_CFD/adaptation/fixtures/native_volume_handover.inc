/*!
 * \file native_volume_handover.inc
 * \brief Production local-core passes on the frozen FP2-B0 lab inputs.
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md).
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */

namespace {
enum class SurfaceObservation { None, Invariant, Diagnosis };
enum class ProductionPartition { None, Event1, Event2 };

void ReportCollapseCore(std::ostream& out, const local::PassStats& stats, int rank, unsigned pass) {
  out << "FP2_C core rank=" << rank << " pass=" << pass
      << " cells_in=" << stats.cells_in << " cells_out=" << stats.cells_out
      << " points_in=" << stats.points_in << " points_out=" << stats.points_out
      << " surface_points=" << stats.surface_points << " border_points=" << stats.border_points
      << " wall_near_points=" << stats.wall_near_points << " collapses=" << stats.collapses
      << " splits=" << stats.splits << " edge_removals=" << stats.edge_removals << " face_swaps=" << stats.face_swaps
      << " moves=" << stats.moves << " deferred=" << stats.deferred << " skipped_memory=" << stats.skipped_memory
      << " evaluated_collapse=" << stats.evaluated_collapse << " refused_valid=" << stats.refused_valid
      << " refused_size=" << stats.refused_size << " refused_topology=" << stats.refused_topology
      << " refused_shape=" << stats.refused_shape << " refused_exact=" << stats.refused_exact
      << " blocked=" << stats.blocked << " short_out=" << stats.short_out << " edges_out=" << stats.edges_out
      << " bad_in=" << stats.bad_in << " bad_out=" << stats.bad_out << " sweeps_collapse=" << stats.sweeps_collapse
      << " sweeps_split=" << stats.sweeps_split << " sweeps_quality=" << stats.sweeps_quality
      << " rounds=" << stats.rounds << " limit=" << stats.limit << " round_changes=" << stats.round_changes
      << " collapses_wall_near=" << stats.collapses_wall_near
      << " refused_size_wall=" << stats.refused_size_wall << " refused_shape_wall=" << stats.refused_shape_wall
      << " refused_topology_wall=" << stats.refused_topology_wall << " refused_valid_wall=" << stats.refused_valid_wall
      << " blocked_wall=" << stats.blocked_wall << " four_surface_cells=" << stats.four_surface_cells
      << " short_free=" << stats.short_free << " short_wall_near=" << stats.short_wall_near
      << " short_border=" << stats.short_border << " short_surface=" << stats.short_surface
      << " seconds_collapse=" << stats.seconds_collapse << " seconds_split=" << stats.seconds_split
      << " seconds_quality=" << stats.seconds_quality << '\n';
}

void ReportCellDigest(std::ostream& out, const Owned& owned, int rank) {
  struct OrderedCell {
    std::array<Id, 4> ids;
    std::array<const Node*, 4> nodes;
  };
  std::vector<OrderedCell> cells;
  cells.reserve(owned.size());
  for (const auto& row : owned) {
    OrderedCell cell;
    const auto& vertices = row.second.cell.v;
    for (unsigned i = 0; i < 4; ++i) cell.nodes[i] = &vertices[i];
    std::sort(cell.nodes.begin(), cell.nodes.end(), [](const Node* a, const Node* b) { return a->id < b->id; });
    for (unsigned i = 0; i < 4; ++i) cell.ids[i] = cell.nodes[i]->id;
    cells.push_back(cell);
  }
  std::sort(cells.begin(), cells.end(), [](const OrderedCell& a, const OrderedCell& b) { return a.ids < b.ids; });
  uint64_t digest = 14695981039346656037ULL;
  const auto append = [&](uint64_t word) {
    for (unsigned byte = 0; byte < 8; ++byte) {
      digest ^= (word >> (8 * byte)) & 0xff;
      digest *= 1099511628211ULL;
    }
  };
  for (const auto& cell : cells) {
    for (const auto id : cell.ids) append(id);
    for (const auto* node : cell.nodes) {
      append(node->id);
      for (double coordinate : {node->p.x, node->p.y, node->p.z}) {
        uint64_t bits;
        std::memcpy(&bits, &coordinate, sizeof(bits));
        append(bits);
      }
    }
  }
  out << "FP2_C cells rank=" << rank << " cells=" << cells.size() << " digest=" << digest << '\n';
}

void ReportVolumeShortEdges(std::ostream& out, const local::Mesh& mesh, int rank, unsigned pass) {
  for (const auto& use : mesh.Edges()) {
    const auto metric = mesh.Length(use.edge);
    if (!(metric < 1e-2)) continue;
    const auto a = mesh.Points()[use.edge[0]].node.id, b = mesh.Points()[use.edge[1]].node.id;
    out << "FP2_B short_edge rank=" << rank << " pass=" << pass << " a=" << std::min(a, b)
        << " b=" << std::max(a, b) << " metric=" << metric << '\n';
  }
}

size_t ReportVolumeBad(std::ostream& out, const local::Mesh& mesh, Id point_base, int rank, unsigned pass,
                       const char* checkpoint = "bad") {
  std::array<size_t, 4> classes{};
  for (const auto& cell : mesh.Cells()) {
    if (!cell.live || !(mesh.Margin(cell.vertices) < local::BadMargin)) continue;
    ++classes[local::SurfaceFixtureAccess::BadClass(mesh, cell.vertices, point_base)];
  }
  const auto total = classes[0] + classes[1] + classes[2] + classes[3];
  out << "FP2_B " << checkpoint << " rank=" << rank << " pass=" << pass << " bad=" << total
      << " bad_a=" << classes[0]
      << " bad_b=" << classes[1] << " bad_c=" << classes[2] << " bad_d=" << classes[3] << '\n';
  return total;
}

template <class Value>
std::map<Face, Value> GatherVolumeFaces(const World& world, const std::map<Face, Value>& local_faces,
                                        const char* duplicate) {
  using PackedFace = std::array<Id, 4>;
  std::vector<char> packed;
  CheckLabStep(world, [&] {
    packed.resize(local_faces.size() * sizeof(PackedFace));
    size_t offset = 0;
    for (const auto& [face, value] : local_faces) {
      const PackedFace row{face[0], face[1], face[2], static_cast<Id>(value)};
      std::memcpy(packed.data() + offset, row.data(), sizeof(row));
      offset += sizeof(row);
    }
  });

  std::map<Face, Value> gathered;
  for (int rank = 0; rank < world.size; ++rank) {
    auto rows = world.rank == rank ? std::move(packed) : std::vector<char>{};
    CPassiveComm::BcastRounds(rows, rank, world.comm);
    CheckLabStep(world, [&] {
      if (world.rank != 0) return;
      for (const auto& row : CPassiveComm::FromBytes<PackedFace>(rows)) {
        if (!gathered.emplace(Face{row[0], row[1], row[2]}, static_cast<Value>(row[3])).second)
          throw std::runtime_error(duplicate);
      }
    });
  }
  return gathered;
}

Face CollapseFaceKey(Face v) {
  std::sort(v.begin(), v.end());
  return v;
}

std::set<Face> RemoveCollapseReasons(const World& world, const std::set<Face>& collapse_faces,
                                    SurfaceReasons& reasons) {
  std::set<Face> gathered;
  for (int rank = 0; rank < world.size; ++rank) {
    std::vector<Face> rows;
    CheckLabStep(world, [&] {
      if (world.rank == rank) rows.assign(collapse_faces.begin(), collapse_faces.end());
    });
    CPassiveComm::Bcast(rows, rank, world.comm);
    CheckLabStep(world, [&] {
      if (world.rank != 0) return;
      for (const auto& face : rows) {
        reasons.erase(face);
        gathered.insert(face);
      }
    });
  }
  return gathered;
}

void ReportCollapseFaces(std::ostream& out, const local::Mesh& mesh, local::SurfaceGeometry& geometry,
                         const std::map<Face, SurfaceFace>& reported, int rank, unsigned pass) {
  size_t surviving = 0, distance = 0, near = 0, entity = 0, size = 0, strict_above = 0;
  const auto limit = geometry.Limit();
  const auto& points = mesh.Points();
  const auto& cells = mesh.Cells();
  for (local::Slot cell = 0; cell < cells.size() && !reported.empty(); ++cell) {
    if (!cells[cell].live) continue;
    for (unsigned side = 0; side < 4; ++side) {
      if (cells[cell].neighbors[side] != local::Physical) continue;
      const auto slots = local::OrientedFace(cells[cell].vertices, side);
      const auto& a = points[slots[0]].node;
      const auto& b = points[slots[1]].node;
      const auto& c = points[slots[2]].node;
      const auto found = reported.find(CollapseFaceKey({a.id, b.id, c.id}));
      if (found == reported.end()) continue;
      const auto& face = found->second;
      if (face.facet != mesh.Bindings(cell)[side].facet)
        throw std::logic_error("FP2-C2 collapse sink and surviving face disagree on the binding.");
      const auto samples = geometry.Samples(face);
      distance += std::any_of(samples.distances.begin(), samples.distances.end(),
                              [&](double sample) { return !std::isfinite(sample) || sample > limit; });
      near += !(local::SurfaceFixtureAccess::SmallestAltitude(a.p, b.p, c.p) >= limit);
      entity += !geometry.OnEntityFace(face);
      strict_above += !geometry.WithinTolerance(face);
      size += !(mesh.Length({slots[0], slots[1]}) <= local::LongEdge &&
                mesh.Length({slots[1], slots[2]}) <= local::LongEdge &&
                mesh.Length({slots[2], slots[0]}) <= local::LongEdge);
      ++surviving;
    }
  }
  out << "FP2_C collapse_probe rank=" << rank << " pass=" << pass << " reported=" << reported.size()
      << " surviving=" << surviving << " distance=" << distance << " near=" << near
      << " entity=" << entity << " size=" << size << " strict_above=" << strict_above << '\n';
}

LabSurfaceAnswers ObserveLabStrictFaces(World& world, const Owned& owned, local::StateSurfaceGeometry& surface,
                                        local::MemoryBudget& memory, size_t caller) {
  constexpr size_t row_bytes = sizeof(LabSurfaceAnswers::value_type) + 4 * sizeof(void*);
  size_t faces = 0;
  for (const auto& entry : owned)
    for (const auto& side : entry.second.physical) faces += side.present;
  const auto global_faces = world.reduce<uint64_t>(faces, CPassiveComm::Op::SUM);
  const auto baseline = transfer_memory::Add(caller, transfer_memory::Mul(faces, row_bytes));
  const auto gathered_bytes = world.size > 1 && world.rank == 0 ? transfer_memory::Mul(global_faces, row_bytes) : 0;
  memory.Require(world, transfer_memory::Add(baseline, surface.ResidentBytes(), gathered_bytes),
                 "FP2-C2 strict face answer maps exceed modeled storage.");
  LabSurfaceAnswers answers;
  CheckLabStep(world, [&] {
    surface.SetCallerBytes(baseline);
    for (const auto& entry : owned) {
      const auto& row = entry.second;
      for (unsigned side = 0; side < row.physical.size(); ++side) {
        if (!row.physical[side].present) continue;
        const auto vertices = local::OrientedFace({0, 1, 2, 3}, side);
        const SurfaceFace face{{row.cell.v[vertices[0]].id, row.cell.v[vertices[1]].id,
                                row.cell.v[vertices[2]].id}, row.physical[side].facet};
        const auto within = surface.WithinTolerance(face);
        if (!answers.emplace(CollapseFaceKey(face.v), within).second)
          throw std::runtime_error("FP2-C2 strict face answer has duplicate local ownership.");
      }
    }
  });
  return answers;
}

bool ProveVolumeHandover(const BoundaryParameterState& state, size_t cells, size_t ceiling, size_t caller,
                         Clock::time_point start, const SurfaceReasons& remaining, const SurfaceFaceBorders* borders,
                         const std::set<Face>* collapse_faces, const LabSurfaceAnswers* answers) {
  auto budget = native_fp2_fixture::Budget(cells, caller, start, 5400, ceiling);
  std::vector<SurfaceFace> faces;
  faces.reserve(state.Surface().size());
  for (const auto& row : state.Surface()) faces.push_back(row.second);
  std::set<BoundaryParameterState::EntityKey> keys;
  for (const auto& row : state.Geometry().Reference().Facets()) keys.insert(state.Entity(row.first));

  const auto reverse_start = Clock::now();
  const auto reverse = state.OwnedFaceReverseBounds(faces, budget);
  const auto reverse_seconds = Seconds(reverse_start);
  const auto forward_start = Clock::now();
  const auto& forward = state.GeometryCertificates(budget, false);
  const auto forward_seconds = Seconds(forward_start);
  const bool complete = !keys.empty() && reverse.size() == keys.size() && forward.size() == keys.size();
  bool accepted = complete && state.ReverseProofComplete(reverse);
  const auto tolerance = state.GeometryTolerance();
  std::ostringstream out;
  out << std::setprecision(17);
  if (complete) {
    size_t i = 0;
    for (const auto& key : keys) {
      const auto& [body, sheet, marker] = key;
      const auto mesh_to_model = reverse[i++];
      const auto& certificate = forward.at(key);
      const auto model_to_mesh = certificate.model_to_mesh;
      const bool valid = !certificate.outside_tolerance && mesh_to_model >= 0 && mesh_to_model <= tolerance &&
                         model_to_mesh >= 0 && model_to_mesh <= tolerance;
      out << "FP2_B proof_entity rank=0 body=" << body << " sheet=" << sheet << " marker=" << marker
          << " mesh_to_model=" << mesh_to_model << " model_to_mesh=" << model_to_mesh << " accepted=" << valid << '\n';
      accepted &= valid;
    }
  }

  const auto classify_start = Clock::now();
  const auto census = ClassifyLabSurface(state, budget, remaining, borders, collapse_faces != nullptr);
  const auto model_gap_entities = collapse_faces ? LabModelGapEntities(forward, census) : 0;
  const auto strict = CompareLabStrict(state, census, collapse_faces, answers);
  const auto accounted = census.left_refused + census.left_short + census.left_border + census.left_limit +
                         census.left_neighbor_locked;
  const bool qualified = complete && census.faces == state.Surface().size() && census.unexplained == 0 &&
                         (accepted || census.above == accounted) && !model_gap_entities &&
                         strict.complete && !strict.sink_mismatch;
  out << "FP2_B proof rank=0 entities=" << keys.size() << " classified=" << census.faces
      << " classify_above_tol=" << census.above << " accepted=" << accepted << " qualified=" << qualified
      << " classify_left_refused=" << census.left_refused << " classify_left_short=" << census.left_short
      << " classify_left_border=" << census.left_border << " classify_left_limit=" << census.left_limit
      << " classify_left_neighbor_locked=" << census.left_neighbor_locked
      << " classify_unexplained=" << census.unexplained;
  PrintLabExcessBins(out, census, borders != nullptr);
  if (collapse_faces) out << " model_gap_entities=" << model_gap_entities;
  if (answers) PrintLabStrict(out, strict);
  out << " seconds_mesh_to_model=" << reverse_seconds << " seconds_model_to_mesh=" << forward_seconds
      << " seconds_classify=" << Seconds(classify_start) << " work=" << budget.work
      << " modeled_peak=" << budget.peak_bytes << '\n';
  std::cout << out.str() << std::flush;
  if (collapse_faces) PrintLabC2Diagnosis(state, census, remaining, *collapse_faces, forward, budget, answers, strict);
  return qualified;
}

void CheckVolumeHandoverGeometry(World& world, const LabInput& input, const ReferenceState& reference,
                                 local::StateSurfaceGeometry& surface, local::MemoryBudget& memory,
                                 size_t global_cells, SurfaceReasons& remaining, Clock::time_point start,
                                 const SurfaceFaceBorders* borders, const std::set<Face>* collapse_faces) {
  auto caller = transfer_memory::Add(reference.original->ResidentBytes(), OwnedStorageBytes(input.cells));
  const bool parallel = world.size > 1;
  LabSurfaceAnswers local_answers, global_answers;
  if (collapse_faces) {
    const auto observation_start = Clock::now();
    local_answers = ObserveLabStrictFaces(world, input.cells, surface, memory, caller);
    if (parallel)
      global_answers = GatherVolumeFaces(world, local_answers, "FP2-C2 strict face answers have duplicate rank ownership.");
    constexpr size_t row_bytes = sizeof(LabSurfaceAnswers::value_type) + 4 * sizeof(void*);
    const auto answer_rows = transfer_memory::Add(local_answers.size(), global_answers.size());
    caller = transfer_memory::Add(caller, transfer_memory::Mul(answer_rows, row_bytes));
    std::ostringstream observation;
    observation << std::setprecision(17) << "FP2_C strict_observation rank=" << world.rank
                << " checked=" << local_answers.size() << " seconds=" << Seconds(observation_start) << '\n';
    std::cout << observation.str() << std::flush;
  }
  const auto result = local::BuildSurfaceState(world, input.cells, surface, memory, caller);
  const auto removals = collapse_faces ? result.removals : 0;
  REQUIRE_FALSE(world.maximum(!result.state || (result.splits == 0 && removals == 0)));
  const auto& original = *reference.original->Parameters();
  const auto& state = *result.state;
  size_t owned_faces = 0;
  CheckLabStep(world, [&] { owned_faces = CheckRankOwnedGeometry(input.cells, state, true); });
  const auto total_owned_faces = world.reduce<uint64_t>(owned_faces, CPassiveComm::Op::SUM);
  REQUIRE_FALSE(world.maximum(total_owned_faces != state.Surface().size() ||
                              state.Revision() != original.Revision() + 1 ||
                              state.Surface().size() + 2 * removals != original.Surface().size() + 2 * result.splits ||
                              state.Records().size() + removals != original.Records().size() + result.splits));

  const auto live = transfer_memory::Add(caller, surface.ResidentBytes(), state.ResidentBytes());
  std::ostringstream row;
  row << std::setprecision(17) << "FP2_B state rank=" << world.rank << " splits=" << result.splits
      << " owned_faces=" << owned_faces << " total_owned_faces=" << total_owned_faces
      << " faces=" << state.Surface().size() << " points=" << state.Records().size()
      << " traces=" << state.Coverage().traces.size() << " revision=" << state.Revision()
      << " owned_geometry_equal=1 seconds_gather=" << result.seconds_gather << " seconds_build=" << result.seconds_build
      << " seconds_check=" << result.seconds_check << " modeled_peak=" << result.modeled_peak;
  if (collapse_faces) row << " removals=" << removals;
  if (parallel) {
    const auto snapshot_bytes = CheckSnapshotAgreement(world, state, memory, live);
    row << " snapshot_bytes=" << snapshot_bytes << " snapshot_equal=1";
  }
  std::cout << row.str() << '\n' << std::flush;
  if (collapse_faces && world.rank == 0) {
    std::map<BoundaryParameterState::EntityKey, size_t> faces_by_entity;
    for (const auto& entry : state.Surface()) ++faces_by_entity[state.Entity(entry.second.facet)];
    std::ostringstream entities;
    for (const auto& [entity, count] : faces_by_entity) {
      entities << "FP2_C entity_faces rank=0";
      PrintLabEntity(entities, entity);
      entities << " faces=" << count << '\n';
    }
    std::cout << entities.str() << std::flush;
  }

  const auto proof_live = [&](const SurfaceReasons& reasons) {
    const auto reason_bytes = transfer_memory::Mul(reasons.size(), sizeof(SurfaceReasons::value_type) + 4 * sizeof(void*));
    const auto bound_rows = collapse_faces ? transfer_memory::Mul(
        state.Surface().size(), sizeof(std::pair<const Face, double>) + 4 * sizeof(void*)) : 0;
    return transfer_memory::Add(live, reason_bytes, bound_rows,
                                transfer_memory::Mul(state.Surface().size(), sizeof(SurfaceFace)));
  };
  if (!parallel) {
    if (collapse_faces)
      for (const auto& face : *collapse_faces) remaining.erase(face);
    std::cout << std::setprecision(17);
    ProveLabSurface(state, global_cells, input.ceiling, proof_live(remaining), start, remaining, collapse_faces,
                    collapse_faces ? &local_answers : nullptr);
    return;
  }

  auto global_reasons = GatherVolumeFaces(world, remaining, "FP2-B final surface reason has duplicate rank ownership.");
  const auto global_collapse_faces =
      collapse_faces ? RemoveCollapseReasons(world, *collapse_faces, global_reasons) : std::set<Face>{};
  const auto global_borders =
      borders ? GatherVolumeFaces(world, *borders, "FP2-B longest-edge ring has duplicate rank ownership.") :
                SurfaceFaceBorders{};
  bool qualified = world.rank != 0;
  CheckLabStep(world, [&] {
    if (world.rank != 0) return;
    const auto retained = transfer_memory::Mul(remaining.size(), sizeof(SurfaceReasons::value_type) + 4 * sizeof(void*));
    constexpr auto border_row_bytes = sizeof(SurfaceFaceBorders::value_type) + 4 * sizeof(void*);
    const auto border_bytes = borders ? transfer_memory::Mul(
        transfer_memory::Add(borders->size(), global_borders.size()), border_row_bytes) : 0;
    qualified = ProveVolumeHandover(state, global_cells, input.ceiling,
                                    transfer_memory::Add(proof_live(global_reasons), retained, border_bytes),
                                    start, global_reasons, borders ? &global_borders : nullptr,
                                    collapse_faces ? &global_collapse_faces : nullptr,
                                    collapse_faces ? &global_answers : nullptr);
  });
  REQUIRE_FALSE(world.maximum(!qualified));
}

class LabBoundObserver final : public local::SurfaceGeometry {
 public:
  LabBoundObserver(const local::Mesh& mesh, local::SurfaceGeometry& geometry, const BoundaryParameterState& state)
      : mesh_(mesh), geometry_(geometry), state_(state) {}

  double Tolerance() const override { return geometry_.Tolerance(); }
  bool Feature(const local::SurfaceEdge& edge) const override { return geometry_.Feature(edge); }
  local::SurfaceSamples Samples(const SurfaceFace& face) override { return geometry_.Samples(face); }
  std::optional<double> Chord(const std::array<Node, 2>& ends) override { return geometry_.Chord(ends); }
  local::SurfacePlacement Place(const std::array<Node, 2>& ends, const std::array<SurfaceFace, 2>& faces,
                                Id id) override {
    return geometry_.Place(ends, faces, id);
  }
  bool OnEntity(const SurfaceFace& face, const BoundaryPoint& middle) override {
    return geometry_.OnEntity(face, middle);
  }
  void Accept(local::SurfacePlacement&& placement) override { geometry_.Accept(std::move(placement)); }
  unsigned Dimension(Id id) const override { return geometry_.Dimension(id); }
  void Remove(Id id) override { geometry_.Remove(id); }
  bool OnEntityFace(const SurfaceFace& face) const override { return geometry_.OnEntityFace(face); }

  bool WithinTolerance(const SurfaceFace& face) override {
    const auto stamp = local::SurfaceFixtureAccess::SweepStamp(mesh_);
    if (!first_sweep_) first_sweep_ = stamp;
    const auto within = geometry_.WithinTolerance(face);
    if (!within) ++refusals_[{stamp, state_.Entity(state_.Geometry().Canonical(face.facet))}];
    return within;
  }

  void Report(std::ostream& out, int rank, unsigned pass) const {
    for (const auto& [key, count] : refusals_) {
      const auto& [stamp, entity] = key;
      out << "FP2_C bound_refusal rank=" << rank << " pass=" << pass << " sweep=" << stamp
          << " sweep_number=" << stamp - first_sweep_ + 1;
      PrintLabEntity(out, entity);
      out << " count=" << count << '\n';
    }
  }

 private:
  const local::Mesh& mesh_;
  local::SurfaceGeometry& geometry_;
  const BoundaryParameterState& state_;
  uint32_t first_sweep_ = 0;
  std::map<std::pair<uint32_t, BoundaryParameterState::EntityKey>, size_t> refusals_;
};

size_t PartitionHandover(World& world, LabInput& input, const ReferenceState& reference,
                         PartitionStats& routing, bool production) {
  std::string reason;
  const auto reference_bytes = reference.original->ResidentBytes();
  if (!production) {
    const std::vector<uint32_t> weights(input.cells.size(), 1);
    const auto partitioned = Repartition(world, input.cells, weights, world.size, routing, reason, input.ceiling,
                                         transfer_memory::Add(reference_bytes, transfer_memory::Bytes(weights)));
    INFO(reason);
    REQUIRE(partitioned);
    return routing.maximum_native_bytes;
  }

  DonorImport source(world, true);
  const auto held = ReferenceStorageBytes(reference);
  const auto source_bytes = transfer_memory::Bytes(input.original);
  const auto built = source.Build(input.original, reason, input.ceiling,
                                  transfer_memory::Add(held, OwnedStorageBytes(input.cells)));
  INFO(reason);
  REQUIRE(built);
  std::vector<uint32_t> weights, geometry_weights;
  BridgeStats stats;
  const auto facet_entries = reference.original->Parameters()->Geometry().GeometryStatsSnapshot().facet_entries;
  const auto weighted = WorkingWeights(world, input.cells, source, weights, reason, stats, {}, input.ceiling,
                                       transfer_memory::Add(held, source_bytes), true, reference.original.get(),
                                       input.tolerance, &geometry_weights, PartitionWorkPhase::Coarsening);
  INFO(reason);
  REQUIRE(weighted);
  const auto partitioned = Repartition(world, input.cells, weights, world.size, routing, reason, input.ceiling,
                                       transfer_memory::Add(held, source_bytes, source.ResidentBytes()),
                                       geometry_weights);
  INFO(reason);
  REQUIRE(partitioned);
  REQUIRE(input.cells.size() == routing.cells_after.at(world.rank));
  std::ostringstream row;
  row << std::setprecision(17) << "FP2_B partition rank=" << world.rank
      << " cells_before=" << routing.cells_before.at(world.rank) << " cells_after=" << routing.cells_after.at(world.rank)
      << " metric_before=" << routing.work_before.at(world.rank)
      << " geometry_before=" << routing.geometry_before.at(world.rank)
      << " geometry_weight_scale=" << stats.geometry_weight_scale << " prepared_facet_entries=" << facet_entries
      << " original_geometry_work=" << stats.geometry_work << " original_boundary_faces=" << stats.geometry_faces
      << " original_cross_chart_faces=" << stats.geometry_cross_chart_faces << '\n';
  std::cout << row.str() << std::flush;
  std::vector<DonorCell>().swap(input.original);
  return std::max({routing.maximum_native_bytes, source.Statistics().maximum_work_bytes, stats.maximum_native_bytes});
}

Point Difference(Point a, Point b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
double VectorLength(Point p) { return std::hypot(p.x, p.y, p.z); }
double DotProduct(Point a, Point b) { return a.x*b.x + a.y*b.y + a.z*b.z; }
Point CrossProduct(Point a, Point b) {
  return {a.y*b.z - a.z*b.y, a.z*b.x - a.x*b.z, a.x*b.y - a.y*b.x};
}
Point UnitVector(Point p) {
  const double n = VectorLength(p);
  return {p.x/n, p.y/n, p.z/n};
}

void ReportDistanceGeometry(std::ostream& out, const local::Mesh& mesh, int rank, unsigned pass, double limit) {
  double shortest = INFINITY, altitude = INFINITY, angle = INFINITY;
  size_t faces = 0;
  local::VisitLivePhysicalFaces(mesh, [&](const auto&, const auto& nodes) {
    ++faces;
    std::array<Point, 3> edges;
    std::array<double, 3> lengths;
    for (unsigned k = 0; k < 3; ++k) {
      edges[k] = Difference(nodes[(k + 1) % 3].p, nodes[k].p);
      lengths[k] = VectorLength(edges[k]);
    }
    const double area2 = VectorLength(CrossProduct(edges[0], edges[1]));
    shortest = std::min(shortest, *std::min_element(lengths.begin(), lengths.end()));
    altitude = std::min(altitude, area2 / *std::max_element(lengths.begin(), lengths.end()));
    for (unsigned k = 0; k < 3; ++k)
      angle = std::min(angle, std::atan2(area2, -DotProduct(edges[k], edges[(k + 1) % 3])));
  });
  out << "FP2_D altitude rank=" << rank << " pass=" << pass << " faces=" << faces
      << " shortest_edge=" << shortest << " angle_deg=" << angle * 180 / std::acos(-1.)
      << " altitude=" << altitude << " limit=" << limit << '\n';
}

class WingDistanceRegions {
 public:
  explicit WingDistanceRegions(const BoundaryParameterState& state) : state_(state) {}

  void Split(const std::array<Node, 2>& ends, const std::array<SurfaceFace, 2>& parents, bool error) {
    if (!Wing(parents[0]) && !Wing(parents[1])) return;
    const auto a = ends[0].p, b = ends[1].p;
    const auto [eta, chord] = Planform({(a.x+b.x)/2, (a.y+b.y)/2, (a.z+b.z)/2});
    if (const auto bin = Bin(eta, chord)) ++(error ? bins_[*bin].error : bins_[*bin].longest);
  }

  void Report(std::ostream& out, const local::Mesh& mesh, const SurfaceReasons& remaining,
              const std::map<Face, SurfaceFace>& collapsed) {
    local::VisitLivePhysicalFaces(mesh, [&](const SurfaceFace& face, const std::array<Node, 3>& nodes) {
      if (!Wing(face)) return;
      const auto a = nodes[0].p, b = nodes[1].p, c = nodes[2].p;
      const auto [eta, chord] = Planform({(a.x+b.x+c.x)/3, (a.y+b.y+c.y)/3, (a.z+b.z+c.z)/3});
      if (const auto bin = Bin(eta, chord)) {
        auto& counts = bins_[*bin];
        ++counts.faces;
        const auto key = CollapseFaceKey(face.v);
        if (remaining.count(key) && !collapsed.count(key)) ++counts.left;
      }
      if (chord >= .005) return;
      const std::array<Point, 3> edges{Difference(b,a), Difference(c,b), Difference(a,c)};
      const auto normal = UnitVector(CrossProduct(edges[0], edges[1]));
      const Point direction{.5, std::sqrt(.75), 0};
      const double dot = DotProduct(direction, normal);
      const auto along = UnitVector(Difference(direction, {dot*normal.x, dot*normal.y, dot*normal.z}));
      const auto across = CrossProduct(normal, along);
      double along_size = 0, across_size = 0;
      for (const auto& edge : edges) {
        along_size = std::max(along_size, std::abs(DotProduct(edge, along)));
        across_size = std::max(across_size, std::abs(DotProduct(edge, across)));
      }
      for (auto& station : stations_) {
        if (std::abs(eta - station.eta) >= .03) continue;
        station.along.push_back(along_size);
        station.across.push_back(across_size);
      }
    });
    for (size_t i = 0; i < bins_.size(); ++i) {
      const auto& row = bins_[i];
      out << "FP2_D region rank=0 bin=" << i << " faces=" << row.faces << " error=" << row.error
          << " longest=" << row.longest << " left=" << row.left << '\n';
    }
    for (auto& station : stations_)
      out << "FP2_D station rank=0 eta=" << station.eta << " faces=" << station.along.size()
          << " along=" << Median(station.along) << " across=" << Median(station.across) << '\n';
  }

 private:
  bool Wing(const SurfaceFace& face) const { return std::get<2>(state_.Entity(face.facet)) == 2; }

  static std::pair<double, double> Planform(Point p) {
    const auto pi = std::acos(-1.);
    const double leading = p.y * std::tan(pi / 6), trailing = .8059 + p.y * std::tan(15.8 * pi / 180);
    return {p.y / 1.1963, (p.x - leading) / (trailing - leading)};
  }

  static std::optional<size_t> Bin(double eta, double chord) {
    if (eta >= .98) return 4;
    if (chord < .005) return 0;
    if (chord < .02) return 1;
    if (chord < .10) return 2;
    if (chord > .90) return 3;
    return std::nullopt;
  }

  static double Median(std::vector<double>& values) {
    if (values.empty()) return NAN;
    std::sort(values.begin(), values.end());
    return .5 * (values[values.size()/2] + values[(values.size()-1)/2]);
  }

  const BoundaryParameterState& state_;
  struct BinCounts { size_t faces = 0, error = 0, longest = 0, left = 0; };
  std::array<BinCounts, 5> bins_{};
  struct Station { double eta; std::vector<double> along, across; };
  std::array<Station, 3> stations_{{{.2, {}, {}}, {.5, {}, {}}, {.8, {}, {}}}};
};

void RunVolumeHandover(bool parallel, bool handover = false, SurfaceObservation observation = SurfaceObservation::None,
                       ProductionPartition partition = ProductionPartition::None, bool surface_collapses = false,
                       bool distance_error_cut = false, bool chosen_invariant = false) {
  const auto start = Clock::now();
  World world;
  REQUIRE(world.size == (parallel ? 6 : 1));
  const bool production = partition != ProductionPartition::None;
  const bool event2 = partition == ProductionPartition::Event2;
  ReferenceState reference;
  auto input = LoadMesh(&reference, parallel, production);
  const auto global_cells = world.reduce<uint64_t>(input.cells.size(), CPassiveComm::Op::SUM);
  REQUIRE(global_cells == (event2 ? 552946 : 303359));
  REQUIRE((reference.marker_names == std::vector<std::string>{"FARFIELD", "SYMMETRY", "WING3D"}));
  if (event2) {
    // The published adapted sidecar already has the policy and tolerance Production prepares at event 1.
    const auto& state = *reference.original->Parameters();
    REQUIRE(state.Mobility().identity != 0);
    REQUIRE(state.Mobility().angle == 60);
    REQUIRE((state.Mobility().roles == std::map<int, int>{{0, 2}, {1, 0}, {2, 2}}));
    REQUIRE(state.EntityGeometry());
    REQUIRE(state.GeometryTolerance() == input.tolerance);
  } else {
    REQUIRE(reference.original->Parameters()->Mobility().identity == 0);
    REQUIRE_FALSE(reference.original->Parameters()->EntityGeometry());
    const auto retained = transfer_memory::Add(OwnedStorageBytes(input.cells), transfer_memory::Bytes(input.original));
    CheckLabStep(world, [&] { native_fp2_fixture::PrepareIN1(reference, input.ceiling, retained); });
  }
  const auto& original = *reference.original->Parameters();
  REQUIRE(original.Surface().size() == (event2 ? 76126 : 48954));
  REQUIRE(original.Records().size() == (event2 ? 38065 : 24479));
  const auto reference_bytes = reference.original->ResidentBytes();

  PartitionStats routing;
  size_t partition_peak = 0;
  if (parallel) {
    partition_peak = PartitionHandover(world, input, reference, routing, production);
    REQUIRE_FALSE(world.maximum(input.cells.empty()));
  }

  constexpr auto owned_row_bytes = sizeof(Owned::value_type) + 4 * sizeof(void*);
  local::MemoryBudget memory{input.ceiling, reference_bytes, owned_row_bytes, partition_peak};
  std::unique_ptr<local::StateSurfaceGeometry> surface;
  CheckLabStep(world, [&] {
    const auto serial_mesh = parallel ? 0 : local::Mesh::ArrayBytes(input.cells.size());
    const auto caller = transfer_memory::Add(reference_bytes, OwnedStorageBytes(input.cells), serial_mesh);
    auto budget = native_fp2_fixture::Budget(global_cells, caller, start, 5400, input.ceiling);
    surface = std::make_unique<local::StateSurfaceGeometry>(reference.original, std::move(budget));
  });

  const auto point_base = world.reduce<uint64_t>(input.point_base, CPassiveComm::Op::MAX);
  const auto cell_base = world.reduce<uint64_t>(input.cell_base, CPassiveComm::Op::MAX);
  local::IdSequence point_ids(point_base, world.rank, world.size), cell_ids(cell_base, world.rank, world.size);
  auto options = Variant("V2");
  options.surface_splits = true;
  options.wall_collapses = true;
  options.surface_collapses = surface_collapses;
  options.distance_error_cut = distance_error_cut;
  const unsigned pass_count = parallel ? local::PassCount : 1;
  local::BorderMeeting previous;
  SurfaceReasons remaining;
  std::set<Face> collapse_faces;
  try {
    for (unsigned pass = 1; pass <= pass_count; ++pass) {
      const auto pass_start = Clock::now();
      const auto owned = OwnedStorageBytes(input.cells);
      memory.caller_bytes = transfer_memory::Add(reference_bytes, surface->ResidentBytes());
      local::BorderMeeting meeting;
      if (parallel) meeting = local::MeetBorderPoints(world, input.cells, memory, previous.Bytes());
      const auto retained = transfer_memory::Add(reference_bytes, previous.Bytes(), meeting.Bytes());
      const auto surface_bytes = [&](size_t cells) {
        return local::Mesh::SurfaceArrayBytes(cells, options.distance_error_cut) -
               (parallel ? 0 : local::Mesh::SurfaceArrayBytes(cells));
      };
      const auto extra_bytes = surface_bytes(input.cells.size());
      const auto admitted = transfer_memory::Add(memory.caller_bytes, previous.Bytes(), meeting.Bytes(), owned,
                                                local::Mesh::ArrayBytes(input.cells.size()), extra_bytes);
      memory.Require(world, admitted, "FP2-B frozen core admission.");
      const auto bytes_per_cell = owned_row_bytes + local::Mesh::BytesPerCell +
                                  surface_bytes(1);
      const auto cell_limit = (input.ceiling - memory.caller_bytes) / bytes_per_cell;

      ExactCounts exact;
      bool census_matches = false;
      size_t violations = 0;
      CheckLabStep(world, [&] {
        local::Mesh mesh(input.cells, parallel, options);
        if (parallel) {
          mesh.MarkBorderSurface(meeting);
          surface->SetCallerBytes(transfer_memory::Add(reference_bytes, owned));
        }
        local::PassStats stats;
        stats.report = true;
        std::map<Face, SurfaceFace> pass_collapse_faces;
        if (surface_collapses) {
          stats.surface_collapse_sink = [&](const SurfaceFace& face) {
            pass_collapse_faces.insert_or_assign(CollapseFaceKey(face.v), face);
          };
        }
        if (handover && pass == pass_count) {
          stats.surface_remaining_sink = [&](Face face, local::SurfaceRemaining reason) {
            std::sort(face.begin(), face.end());
            if (!remaining.emplace(face, reason).second)
              throw std::runtime_error("FP2-B final surface reason was recorded twice on one rank.");
          };
        }
        std::optional<local::SurfaceLongestEdgeProbe> probe;
        WingDistanceRegions regions(original);
        std::optional<local::SurfaceChosenEdgeProbe> chosen_probe;
        if (chosen_invariant)
          chosen_probe.emplace(mesh, *surface,
                               [&](const auto& ends, const auto& parents, bool error) {
                                 regions.Split(ends, parents, error);
                               });
        const bool trace = observation == SurfaceObservation::Diagnosis;
        const bool invariant = observation == SurfaceObservation::Invariant;
        if (observation != SurfaceObservation::None) probe.emplace(mesh, *surface, trace);
        if (invariant)
          stats.surface_refusal_sink = [&](const char*, const local::SurfaceEdge& edge, local::SurfaceRefusal refusal) {
            probe->RefusedAfterPlace(edge, refusal);
          };
        double measurement_seconds = 0;
        std::optional<size_t> bad_before_v;
        if (handover) {
          stats.sink = [&](const local::SweepRow& sweep) {
            if (trace) probe->ReportSweep(std::cout, sweep, world.rank, pass);
            const auto measurement_start = Clock::now();
            if (chosen_probe) chosen_probe->EndSweep(sweep);
            std::ostringstream out;
            if (sweep.surface) {
              out << std::setprecision(17) << "FP2_B sweep rank=" << world.rank << " pass=" << pass;
              local::PrintSweepFields(out, sweep);
              local::PrintSurfaceSweepFields(out, sweep);
              out << '\n';
            } else if (!bad_before_v && std::strcmp(sweep.kind, "quality") == 0) {
              const auto& changes = sweep.counters;
              // Initial QualityWork ends on a zero-edit sweep or at QualitySweepLimit (20).
              if (changes.edge_removals + changes.face_swaps + changes.moves == 0 || sweep.sweep == 20)
                bad_before_v = ReportVolumeBad(out, mesh, point_base, world.rank, pass, "bad_before_v");
            }
            std::cout << out.str();
            measurement_seconds += Seconds(measurement_start);
          };
        }
        const auto pass_begin = Clock::now();
        local::SurfaceGeometry* geometry = probe ? &*probe : static_cast<local::SurfaceGeometry*>(surface.get());
        if (chosen_probe) geometry = &*chosen_probe;
        std::optional<LabBoundObserver> bound_observer;
        if (surface_collapses) {
          bound_observer.emplace(mesh, *geometry, surface->State());
          geometry = &*bound_observer;
        }
        stats = mesh.RunPass(point_ids, cell_limit, std::move(stats), options, geometry);
        const auto run_seconds = Seconds(pass_begin) - measurement_seconds;
        bool walk_probe_matches = true;
        std::ostringstream row;
        row << std::setprecision(17);
        if (options.distance_error_cut) ReportDistanceGeometry(row, mesh, world.rank, pass, surface->Limit());
        if (chosen_probe) {
          row << "FP2_D invariant rank=" << world.rank << " pass=" << pass
              << " checked=" << chosen_probe->Checked() << " splits_distance=" << stats.surface.splits_distance
              << " violations=" << chosen_probe->Violations() << '\n';
          regions.Report(row, mesh, remaining, pass_collapse_faces);
        }
        if (probe) {
          if (probe->Checked() != stats.surface.splits_distance)
            throw std::logic_error("Surface probe did not observe each accepted distance split.");
          violations = probe->Violations();
          row << "FP2_B invariant rank=" << world.rank << " pass=" << pass << " violations=" << violations
              << " point_base=" << point_base << '\n';
          if (invariant) walk_probe_matches = probe->ReportTolerance(row, world.rank, pass, stats.surface.walk_tol8);
        }
        row << "NATIVE3D_LOCAL_SURFACE rank=" << world.rank << " stage=0 pass=" << pass
            << " applicable=1";
        local::PrintSurfaceFields(row, stats.surface);
        row << '\n';
        if (bound_observer) bound_observer->Report(row, world.rank, pass);
        ReportCollapseCore(row, stats, world.rank, pass);
        std::cout << row.str() << std::flush;
        if (chosen_probe) {
          REQUIRE(chosen_probe->Checked() == stats.surface.splits_distance);
          REQUIRE(chosen_probe->Violations() == 0);
        }
        if (!walk_probe_matches) throw std::logic_error("Tolerance placement counts or accepted split differ.");
        bool bad_matches = true;
        if (handover) {
          const auto measurement_start = Clock::now();
          row.str("");
          if (surface_collapses) {
            ReportCollapseFaces(row, mesh, *surface, pass_collapse_faces, world.rank, pass);
            for (const auto& entry : pass_collapse_faces) collapse_faces.insert(entry.first);
          }
          const auto bad_after_v = ReportVolumeBad(row, mesh, point_base, world.rank, pass);
          bad_matches = bad_after_v == stats.bad_out && bad_before_v == stats.surface.bad_before_v;
          fp2_b0::ReportEdges(row, mesh, point_base, world.rank, pass, "out");
          if (observation == SurfaceObservation::Diagnosis && pass == pass_count)
            ReportVolumeShortEdges(row, mesh, world.rank, pass);
          if (pass == pass_count) fp2_b0::ReportStars(row, mesh, point_base, world.rank, pass, "out", true);
          std::cout << row.str() << std::flush;
          measurement_seconds += Seconds(measurement_start);
        }
        size_t faces_out = 0;
        for (const auto& cell : mesh.Cells())
          if (cell.live) faces_out += std::count(cell.neighbors.begin(), cell.neighbors.end(), local::Physical);
        mesh.WriteBack(input.cells, cell_ids);
        const auto pass_seconds = Seconds(pass_start) - measurement_seconds;
        exact = CountExact(input.cells);
        census_matches = bad_matches && faces_out == stats.surface.faces_out && input.cells.size() == stats.cells_out;
        row.str("");
        row << "FP2_B pass rank=" << world.rank << " pass=" << pass << " cells_in=" << stats.cells_in
            << " cells_out=" << stats.cells_out << " faces_out=" << faces_out << " bad_out=" << stats.bad_out
            << " nonpositive=" << exact.nonpositive
            << " first_nonpositive=" << exact.first << " seconds_pass=" << pass_seconds
            << " seconds_run_pass=" << run_seconds;
        if (surface_collapses) row << " modeled_peak=" << std::max(memory.maximum_bytes, surface->PeakBytes());
        row << '\n';
        if (surface_collapses)
          row << "FP2_C query_budget rank=" << world.rank << " pass=" << pass
              << " overlay_peak=" << surface->PeakBytes() << " pass_admitted_bytes=" << admitted
              << " query_scratch_max=" << stats.surface.query_scratch_max
              << " conservative_sum=" << transfer_memory::Add(admitted, stats.surface.query_scratch_max)
              << " ceiling=" << input.ceiling << '\n';
        std::cout << row.str() << std::flush;
      });
      REQUIRE_FALSE(world.maximum(exact.nonpositive != 0));
      REQUIRE_FALSE(world.maximum(!census_matches));
      if (observation == SurfaceObservation::Invariant) REQUIRE(violations == 0);
      memory.maximum_bytes = std::max(memory.maximum_bytes, surface->PeakBytes());
      if (parallel) {
        surface->Exchange(world, memory, transfer_memory::Add(retained, OwnedStorageBytes(input.cells)));
        memory.caller_bytes = transfer_memory::Add(reference_bytes, surface->ResidentBytes());
        const bool lower = pass == 1;
        local::ShiftBorders(world, input.cells, meeting, lower ? local::Direction::Lower : local::Direction::Higher,
                            memory, lower ? nullptr : &previous);
        if (lower) previous.rendezvous = std::move(meeting.rendezvous);
      }
    }
  } catch (const CElectedFailure& error) {
    FAIL(error.message);
  }

  const auto exact = CountExact(input.cells);
  std::ostringstream row;
  row << std::setprecision(17) << "FP2_B end rank=" << world.rank << " cells_out=" << input.cells.size()
      << " nonpositive=" << exact.nonpositive << " first_nonpositive=" << exact.first
      << " modeled_peak=" << memory.maximum_bytes << " seconds=" << Seconds(start) << '\n';
  std::cout << row.str() << std::flush;
  REQUIRE_FALSE(world.maximum(exact.nonpositive != 0));

  if (handover) {
    if (observation == SurfaceObservation::None)
      CheckLabStep(world, [&] { ReportHandoverDonors(input.cells, world.rank); });
    const bool classify_border = partition == ProductionPartition::Event1;
    SurfaceFaceBorders borders;
    CheckLabStep(world, [&] {
      const local::Mesh mesh(input.cells, parallel, options);
      std::ostringstream region;
      region << std::setprecision(17);
      if (production) fp2_b0::ReportEdges(region, mesh, point_base, world.rank, 0, "shifted");
      if (classify_border) borders = local::SurfaceFixtureAccess::LongestEdgeBorders(mesh);
      fp2_b0::ReportRegion(region, mesh, point_base, world.rank, [&](Id id) { return surface->Find(id) != nullptr; });
      std::cout << region.str() << std::flush;
    });
    try {
      CheckVolumeHandoverGeometry(world, input, reference, *surface, memory, global_cells, remaining, start,
                                 classify_border ? &borders : nullptr, surface_collapses ? &collapse_faces : nullptr);
    } catch (const CElectedFailure& error) {
      FAIL(error.message);
    }
  }
  CheckLabStep(world, [&] {
    std::ostringstream digest;
    ReportCellDigest(digest, input.cells, world.rank);
    std::cout << digest.str() << std::flush;
  });
}
}  // namespace

TEST_CASE("FP2-B production volume repair equivalence on one rank", "[.][NativeLocalCoreVolumeEquivalenceSerial]") {
  RunVolumeHandover(false);
}

TEST_CASE("FP2-B production volume repair equivalence on six ranks", "[.][NativeLocalCoreVolumeEquivalenceMPI]") {
  RunVolumeHandover(true);
}

TEST_CASE("FP2-B production volume hand-over and proof on one rank", "[.][NativeLocalCoreVolumeHandoverSerial]") {
  RunVolumeHandover(false, true);
}

TEST_CASE("FP2-B production volume hand-over and proof on six ranks", "[.][NativeLocalCoreVolumeHandoverMPI]") {
  RunVolumeHandover(true, true);
}

TEST_CASE("FP2-B event-1 production-partition hand-over", "[.][NativeLocalCoreVolumeHandoverProduction1]") {
  RunVolumeHandover(true, true, SurfaceObservation::None, ProductionPartition::Event1);
}

TEST_CASE("FP2-B event-2 production-partition hand-over", "[.][NativeLocalCoreVolumeHandoverProduction2]") {
  RunVolumeHandover(true, true, SurfaceObservation::None, ProductionPartition::Event2);
}

TEST_CASE("FP2-B accepted distance splits bisect both parents' longest edges",
          "[.][NativeLocalCoreVolumeHandoverInvariant]") {
  RunVolumeHandover(false, true, SurfaceObservation::Invariant);
}

TEST_CASE("FP2-B trace accepted surface splits in the six-rank hand-over",
          "[.][NativeLocalCoreVolumeHandoverDiagnosis]") {
  RunVolumeHandover(true, true, SurfaceObservation::Diagnosis);
}

TEST_CASE("FP2-C2 production surface collapse hand-over on one rank", "[.][NativeFP2C2LabSerial]") {
  RunVolumeHandover(false, true, SurfaceObservation::None, ProductionPartition::None, true);
}

TEST_CASE("FP2-C2 production surface collapse hand-over on six ranks", "[.][NativeFP2C2LabMPI]") {
  RunVolumeHandover(true, true, SurfaceObservation::None, ProductionPartition::None, true);
}

TEST_CASE("FP2-C2 production surface collapse hand-over on the event-1 production partition",
          "[.][NativeFP2C2LabProduction1]") {
  RunVolumeHandover(true, true, SurfaceObservation::None, ProductionPartition::Event1, true);
}

TEST_CASE("FP2-D production hand-over on one rank", "[.][NativeFP2DLabSerial]") {
  RunVolumeHandover(false, true, SurfaceObservation::None, ProductionPartition::None, true, true);
}

TEST_CASE("FP2-D production hand-over on six ranks", "[.][NativeFP2DLabMPI]") {
  RunVolumeHandover(true, true, SurfaceObservation::None, ProductionPartition::None, true, true);
}

TEST_CASE("FP2-D hand-over on the event-1 production partition", "[.][NativeFP2DLabProduction1]") {
  RunVolumeHandover(true, true, SurfaceObservation::None, ProductionPartition::Event1, true, true);
}

TEST_CASE("FP2-D accepted splits carry nonfeature error or cut both parents' chosen edges", "[.][NativeFP2DLabInvariant]") {
  RunVolumeHandover(false, true, SurfaceObservation::None, ProductionPartition::None, true, true, true);
}
