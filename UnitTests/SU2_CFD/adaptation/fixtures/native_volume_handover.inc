/*!
 * \file native_volume_handover.inc
 * \brief Production local-core passes on the frozen FP2-B0 lab inputs.
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md).
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */

namespace {
void RunVolumeHandover(bool parallel) {
  const auto start = Clock::now();
  World world;
  REQUIRE(world.size == (parallel ? 6 : 1));
  ReferenceState reference;
  auto input = LoadMesh(&reference, parallel);
  const auto global_cells = world.reduce<uint64_t>(input.cells.size(), CPassiveComm::Op::SUM);
  REQUIRE(global_cells == 303359);
  REQUIRE((reference.marker_names == std::vector<std::string>{"FARFIELD", "SYMMETRY", "WING3D"}));
  REQUIRE(reference.original->Parameters()->Mobility().identity == 0);
  REQUIRE_FALSE(reference.original->Parameters()->EntityGeometry());
  CheckLabStep(world, [&] { native_fp2_fixture::PrepareIN1(reference, input.ceiling, OwnedStorageBytes(input.cells)); });
  const auto& original = *reference.original->Parameters();
  REQUIRE(original.Surface().size() == 48954);
  REQUIRE(original.Records().size() == 24479);
  const auto reference_bytes = reference.original->ResidentBytes();

  PartitionStats routing;
  if (parallel) {
    std::string reason;
    const std::vector<uint32_t> weights(input.cells.size(), 1);
    const auto partitioned = Repartition(world, input.cells, weights, world.size, routing, reason, input.ceiling,
                                         transfer_memory::Add(reference_bytes, transfer_memory::Bytes(weights)));
    INFO(reason);
    REQUIRE(partitioned);
    REQUIRE_FALSE(world.maximum(input.cells.empty()));
  }

  constexpr auto owned_row_bytes = sizeof(Owned::value_type) + 4 * sizeof(void*);
  local::MemoryBudget memory{input.ceiling, reference_bytes, owned_row_bytes, routing.maximum_native_bytes};
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
      CheckLabStep(world, [&] {
        local::Mesh mesh(input.cells, parallel, options);
        if (parallel) {
          mesh.MarkBorderSurface(meeting);
          surface->SetCallerBytes(transfer_memory::Add(reference_bytes, owned));
        }
        const auto pass_begin = Clock::now();
        const auto stats = mesh.RunPass(point_ids, cell_limit, true, options, surface.get());
        const auto run_seconds = Seconds(pass_begin);
        std::ostringstream row;
        row << std::setprecision(17) << "NATIVE3D_LOCAL_SURFACE rank=" << world.rank << " stage=0 pass=" << pass
            << " applicable=1";
        local::PrintSurfaceFields(row, stats.surface);
        std::cout << row.str() << '\n' << std::flush;
        size_t faces_out = 0;
        for (const auto& cell : mesh.Cells())
          if (cell.live) faces_out += std::count(cell.neighbors.begin(), cell.neighbors.end(), local::Physical);
        mesh.WriteBack(input.cells, cell_ids);
        const auto pass_seconds = Seconds(pass_start);
        exact = CountExact(input.cells);
        census_matches = faces_out == stats.surface.faces_out && input.cells.size() == stats.cells_out;
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
}
}  // namespace

TEST_CASE("FP2-B production volume repair equivalence on one rank", "[.][NativeLocalCoreVolumeEquivalenceSerial]") {
  RunVolumeHandover(false);
}

TEST_CASE("FP2-B production volume repair equivalence on six ranks", "[.][NativeLocalCoreVolumeEquivalenceMPI]") {
  RunVolumeHandover(true);
}
