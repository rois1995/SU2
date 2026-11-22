/*!
 * \file native_volume_handover.inc
 * \brief Production local-core passes on the frozen FP2-B0 lab inputs.
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md).
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */

namespace {
enum class SurfaceObservation { None, Invariant, Diagnosis };
enum class ProductionPartition { None, Event1, Event2 };

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

bool ProveVolumeHandover(const BoundaryParameterState& state, size_t cells, size_t ceiling, size_t caller,
                         Clock::time_point start, const SurfaceReasons& remaining, const SurfaceFaceBorders* borders) {
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
  const auto census = ClassifyLabSurface(state, budget, remaining, borders);
  const auto accounted = census.left_refused + census.left_short + census.left_border + census.left_limit +
                         census.left_neighbor_locked;
  const bool qualified = complete && census.faces == state.Surface().size() && census.unexplained == 0 &&
                         (accepted || census.above == accounted);
  out << "FP2_B proof rank=0 entities=" << keys.size() << " classified=" << census.faces
      << " classify_above_tol=" << census.above << " accepted=" << accepted << " qualified=" << qualified
      << " classify_left_refused=" << census.left_refused << " classify_left_short=" << census.left_short
      << " classify_left_border=" << census.left_border << " classify_left_limit=" << census.left_limit
      << " classify_left_neighbor_locked=" << census.left_neighbor_locked
      << " classify_unexplained=" << census.unexplained;
  PrintLabExcessBins(out, census, borders != nullptr);
  out << " seconds_mesh_to_model=" << reverse_seconds << " seconds_model_to_mesh=" << forward_seconds
      << " seconds_classify=" << Seconds(classify_start) << " work=" << budget.work
      << " modeled_peak=" << budget.peak_bytes << '\n';
  std::cout << out.str() << std::flush;
  return qualified;
}

void CheckVolumeHandoverGeometry(World& world, const LabInput& input, const ReferenceState& reference,
                                 const local::StateSurfaceGeometry& surface, local::MemoryBudget& memory,
                                 size_t global_cells, const SurfaceReasons& remaining, Clock::time_point start,
                                 const SurfaceFaceBorders* borders) {
  const auto caller = transfer_memory::Add(reference.original->ResidentBytes(), OwnedStorageBytes(input.cells));
  const auto result = local::BuildSurfaceState(world, input.cells, surface, memory, caller);
  REQUIRE_FALSE(world.maximum(!result.state || result.splits == 0));
  const auto& original = *reference.original->Parameters();
  const auto& state = *result.state;
  size_t owned_faces = 0;
  CheckLabStep(world, [&] { owned_faces = CheckRankOwnedGeometry(input.cells, state, true); });
  const auto total_owned_faces = world.reduce<uint64_t>(owned_faces, CPassiveComm::Op::SUM);
  REQUIRE_FALSE(world.maximum(total_owned_faces != state.Surface().size() ||
                              state.Revision() != original.Revision() + 1 ||
                              state.Surface().size() != original.Surface().size() + 2 * result.splits ||
                              state.Records().size() != original.Records().size() + result.splits));

  const bool parallel = world.size > 1;
  const auto live = transfer_memory::Add(caller, surface.ResidentBytes(), state.ResidentBytes());
  std::ostringstream row;
  row << std::setprecision(17) << "FP2_B state rank=" << world.rank << " splits=" << result.splits
      << " owned_faces=" << owned_faces << " total_owned_faces=" << total_owned_faces
      << " faces=" << state.Surface().size() << " points=" << state.Records().size()
      << " traces=" << state.Coverage().traces.size() << " revision=" << state.Revision()
      << " owned_geometry_equal=1 seconds_gather=" << result.seconds_gather << " seconds_build=" << result.seconds_build
      << " seconds_check=" << result.seconds_check << " modeled_peak=" << result.modeled_peak;
  if (parallel) {
    const auto snapshot_bytes = CheckSnapshotAgreement(world, state, memory, live);
    row << " snapshot_bytes=" << snapshot_bytes << " snapshot_equal=1";
  }
  std::cout << row.str() << '\n' << std::flush;

  const auto proof_live = [&](const SurfaceReasons& reasons) {
    const auto reason_bytes = transfer_memory::Mul(reasons.size(), sizeof(SurfaceReasons::value_type) + 4 * sizeof(void*));
    return transfer_memory::Add(live, reason_bytes,
                                transfer_memory::Mul(state.Surface().size(), sizeof(SurfaceFace)));
  };
  if (!parallel) {
    std::cout << std::setprecision(17);
    ProveLabSurface(state, global_cells, input.ceiling, proof_live(remaining), start, remaining);
    return;
  }

  const auto global_reasons = GatherVolumeFaces(world, remaining, "FP2-B final surface reason has duplicate rank ownership.");
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
                                    start, global_reasons, borders ? &global_borders : nullptr);
  });
  REQUIRE_FALSE(world.maximum(!qualified));
}

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

void RunVolumeHandover(bool parallel, bool handover = false, SurfaceObservation observation = SurfaceObservation::None,
                       ProductionPartition partition = ProductionPartition::None) {
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
  const unsigned pass_count = parallel ? local::PassCount : 1;
  local::BorderMeeting previous;
  SurfaceReasons remaining;
  try {
    for (unsigned pass = 1; pass <= pass_count; ++pass) {
      const auto pass_start = Clock::now();
      const auto owned = OwnedStorageBytes(input.cells);
      memory.caller_bytes = transfer_memory::Add(reference_bytes, surface->ResidentBytes());
      local::BorderMeeting meeting;
      if (parallel) meeting = local::MeetBorderPoints(world, input.cells, memory, previous.Bytes());
      const auto retained = transfer_memory::Add(reference_bytes, previous.Bytes(), meeting.Bytes());
      const auto extra_bytes = parallel ? local::Mesh::SurfaceArrayBytes(input.cells.size()) : 0;
      memory.Require(world, transfer_memory::Add(memory.caller_bytes, previous.Bytes(), meeting.Bytes(), owned,
                                                 local::Mesh::ArrayBytes(input.cells.size()), extra_bytes),
                     "FP2-B frozen core admission.");
      const auto bytes_per_cell = owned_row_bytes + local::Mesh::BytesPerCell +
                                  (parallel ? local::Mesh::SurfaceArrayBytes(1) : 0);
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
        if (handover && pass == pass_count) {
          stats.surface_remaining_sink = [&](Face face, local::SurfaceRemaining reason) {
            std::sort(face.begin(), face.end());
            if (!remaining.emplace(face, reason).second)
              throw std::runtime_error("FP2-B final surface reason was recorded twice on one rank.");
          };
        }
        std::optional<local::SurfaceLongestEdgeProbe> probe;
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
        stats = mesh.RunPass(point_ids, cell_limit, std::move(stats), options, geometry);
        const auto run_seconds = Seconds(pass_begin) - measurement_seconds;
        bool walk_probe_matches = true;
        std::ostringstream row;
        row << std::setprecision(17);
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
        std::cout << row.str() << '\n' << std::flush;
        if (!walk_probe_matches) throw std::logic_error("Tolerance placement counts or accepted split differ.");
        bool bad_matches = true;
        if (handover) {
          const auto measurement_start = Clock::now();
          row.str("");
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
            << " seconds_run_pass=" << run_seconds << '\n';
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
                                 classify_border ? &borders : nullptr);
    } catch (const CElectedFailure& error) {
      FAIL(error.message);
    }
  }
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
