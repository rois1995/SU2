/*!
 * \file native_local_contact_mpi_fixtures.inc
 * \brief FP2-C3 rank contact and post-pass verification fixtures.
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md).
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */
namespace {
constexpr std::array<Point, 4> ContactTet{{{0, 0, 0}, {2, 0, 0}, {0, 2, 0}, {0, 0, 2}}};
constexpr Tet OneContactTet{0, 1, 2, 3};

void TranslateContact(Owned& rows, Point offset) {
  for (auto& entry : rows)
    for (auto& node : entry.second.cell.v) {
      node.p.x += offset.x;
      node.p.y += offset.y;
      node.p.z += offset.z;
    }
}

void RenumberContactCell(Owned& rows, Id id) {
  REQUIRE(rows.size() == 1);
  auto row = rows.extract(rows.begin());
  row.key() = id;
  row.mapped().cell.id = id;
  REQUIRE(rows.insert(std::move(row)).inserted);
}

std::vector<Point> NestedContactTet(const std::array<Point, 4>& tet, Point center, double scale) {
  std::vector<Point> points;
  for (const auto p : tet)
    points.push_back({center.x + scale * (p.x - center.x), center.y + scale * (p.y - center.y),
                      center.z + scale * (p.z - center.z)});
  return points;
}

void CheckElectedContact(World& world, const Owned& rows, Id first, Id second, int source_rank) {
  auto memory = ExchangeBudget();
  std::optional<CElectedFailure> failure;
  try {
    local::VerifyPassContact(world, rows, memory, memory.OwnedBytes(rows.size()), true);
  } catch (const CElectedFailure& elected) {
    failure = elected;
  }
  REQUIRE_FALSE(world.maximum(!failure.has_value()));
  REQUIRE(failure->any);
  INFO(failure->message);
  CHECK(failure->message.find("Native local surface contact:") != std::string::npos);
  CHECK(failure->message.find("cell " + std::to_string(first)) != std::string::npos);
  CHECK(failure->message.find("cell " + std::to_string(second)) != std::string::npos);
  CHECK(failure->message.find("rank " + std::to_string(source_rank)) != std::string::npos);
}

void CheckRemoteTrial(const TentTrial& trial) {
  CHECK(trial.stats.c_contact_directions == 1);
  CHECK(trial.stats.c_outward_tried == 1);
  CHECK(trial.stats.c_refusals[static_cast<size_t>(C3Refusal::ContactRemote)] == 1);
  CHECK(trial.stats.c_contact_calls == 0);
  CHECK(trial.result == C3Refusal::ContactRemote);
}
}  // namespace

TEST_CASE("Step C screens a disconnected bubble on a different rank", "[.][NativeFP2C3][NativeFP2C3MPI]") {
  World world;
  REQUIRE(world.size == 2);
  constexpr Id bubble_id = 55;
  constexpr double tolerance = .5, margin = 3 * tolerance;
  const auto tent = S33(.2);
  for (unsigned position = 0; position < 3; ++position) {
    CAPTURE(position);
    auto bubble = FirstTentBubble(1);
    RenumberContactCell(bubble, bubble_id);
    // The shrunk cap has z in [.0375,.0875] with x,y inside [0,2]x[0,1].
    // Moving down one cube height gives a gap of .9125 to z=0. Three heights give 2.9125.
    constexpr std::array<double, 3> down{0, 1, 3};
    TranslateContact(bubble, {0, 0, -down[position]});
    double bubble_top = -1e3;
    for (const auto& node : bubble.begin()->second.cell.v) {
      CHECK(node.p.x > 0);
      CHECK(node.p.x < 2);
      CHECK(node.p.y > 0);
      CHECK(node.p.y < 1);
      bubble_top = std::max(bubble_top, node.p.z);
    }
    CHECK(bubble_top == Approx(.0875 - down[position]));
    CHECK(margin == 1.5);
    if (position == 1) CHECK((-bubble_top < margin && bubble_top < 0));
    if (position == 2) CHECK(-bubble_top > margin);

    auto working = world.rank == 0 ? tent : bubble;
    auto memory = ExchangeBudget();
    const auto meeting = local::MeetBorderPoints(world, working, memory);
    auto remote = local::GatherRemoteSurface(world, working, margin, memory, memory.OwnedBytes(working.size()), true);
    for (int owner = 0; owner < world.size; ++owner) {
      C2OnRank(world, owner, [&] {
        local::Mesh mesh(working, true, ContactOptions());
        mesh.MarkBorderSurface(meeting);
        REQUIRE(std::none_of(mesh.Points().begin(), mesh.Points().end(), [](const auto& p) { return p.border; }));
        if (owner == 0) {
          CollapseFixtureGeometry absent(working, TentKeep, tolerance);
          local::PassStats stats;
          REQUIRE_THROWS(local::SurfaceFixtureAccess::SurfaceCollapseWork(mesh, absent, stats));
          local::StateSurfaceGeometry overlay(EntityReference(working, false), AtlasBudget());
          TentOverlayGeometry geometry(working, overlay);
          local::LocalContactIndex index(mesh);
          local::ContactContext context{index, &remote};
          constexpr local::ContactBox cap_box{0, 0, 0, 2, 1, .2};
          constexpr local::ContactBox global_box{0, 0, 0, 3, 3, 1};
          CHECK(remote.Near(cap_box) == (position != 2));
          const auto snapshot = Snapshot(mesh), frozen = FrozenSnapshot(mesh), state = OverlaySnapshot(mesh, overlay);
          const auto candidates = index.Candidates(global_box);
          const auto trial = TryContactTent(mesh, geometry, C3Refusal::None, &context);
          if (position != 2) {
            CheckRemoteTrial(trial);
            CHECK(Snapshot(mesh) == snapshot);
            CHECK(FrozenSnapshot(mesh) == frozen);
            CHECK(OverlaySnapshot(mesh, overlay) == state);
            CHECK(geometry.removed.empty());
            CHECK(index.Candidates(global_box) == candidates);
            CHECK(index.FreshSize() == 0);
          } else {
            REQUIRE(trial.result == C3Refusal::None);
            CHECK(trial.stats.collapses_outward == 1);
          }
        }
        if (position == 2) {
          REQUIRE_NOTHROW(mesh.Check());
          for (const auto& entry : LiveCells(mesh)) CHECK(TetMeasure(entry.second) > 0);
          CHECK(native_fp2_c3::EmbeddingViolations(mesh).violations == 0);
          local::IdSequence cells(bubble_id, world.rank, world.size);
          mesh.WriteBack(working, cells);
        }
      });
    }
    if (position == 2) {
      const auto verify = local::VerifyPassContact(world, working, memory, memory.OwnedBytes(working.size()), true);
      CHECK(verify.verify_cells == working.size());
    }
  }
}

TEST_CASE("Pass verification elects rank contacts and accepts a conforming partition",
          "[.][NativeFP2C3][NativeFP2C3MPI]") {
  World world;
  REQUIRE(world.size == 2);
  SECTION("f1: a cell crosses the other rank's physical face") {
    auto rows = BoundMesh(std::vector<Point>(ContactTet.begin(), ContactTet.end()), {OneContactTet}, .25);
    if (world.rank == 1) {
      Owned remote;
      AppendContactComponent(remote, rows, 4, 4);
      RenumberContactCell(remote, 2);
      // Translating it one unit along x puts its origin inside the first
      // tetrahedron's x interval and extends the tetrahedron past x+y+z=2.
      TranslateContact(remote, {1, 0, 0});
      rows.swap(remote);
    }
    CheckElectedContact(world, rows, 1, 2, 1);
  }
  SECTION("f2: a remote boundary cell is inside an interior grandchild") {
    const auto [a, b, c, d] = ContactTet;
    // O is the mean of A,B,C,D. Q is the mean of A,B,C,O. The
    // grandchild A,B,O,Q is first and has no face in the original boundary.
    const Point o{.5, .5, .5}, q{.625, .625, .125};
    constexpr Id child = 1, bubble = 8;
    Owned rows;
    C2OnRank(world, 0, [&] {
      rows = BoundMesh({a, b, c, d, o, q}, {{0, 1, 4, 5}, {0, 1, 2, 5}, {0, 2, 4, 5}, {1, 2, 4, 5},
                                          {0, 1, 3, 4}, {0, 2, 3, 4}, {1, 2, 3, 4}}, .25);
      const auto& physical = rows.at(child).physical;
      REQUIRE(std::none_of(physical.begin(), physical.end(), [](const auto& face) { return face.present; }));
    });
    C2OnRank(world, 1, [&] {
      // The bubble is a quarter-scale copy centered at the grandchild's centroid.
      const Point center{.78125, .28125, .15625};
      const auto points = NestedContactTet({a, b, o, q}, center, .25);
      AppendContactComponent(rows, BoundMesh(points, {OneContactTet}, .25), 6, 16);
      RenumberContactCell(rows, bubble);
    });
    CheckElectedContact(world, rows, child, bubble, 1);
  }
  SECTION("f3: matched S33 faces, edges and points") {
    const auto rows = C2Partition(S33(), world.rank, true);
    auto memory = ExchangeBudget();
    const auto meeting = local::MeetBorderPoints(world, rows, memory);
    local::Mesh mesh(rows, true);
    mesh.MarkBorderSurface(meeting);
    REQUIRE(std::any_of(mesh.Points().begin(), mesh.Points().end(), [](const auto& point) { return point.border; }));
    const auto caller = memory.OwnedBytes(rows.size());
    const auto one = local::VerifyPassContact(world, rows, memory, caller, true);
    const auto two = local::VerifyPassContact(world, rows, memory, caller, true);
    CHECK(one.verify_cells == rows.size());
    CHECK(one.verify_candidates > 0);
    CHECK(one.verify_calls > 0);
    CHECK(one.owner_rows == two.owner_rows);
    CHECK(one.verify_cells == two.verify_cells);
    CHECK(one.verify_candidates == two.verify_candidates);
    CHECK(one.verify_calls == two.verify_calls);
  }
  SECTION("f4: the two tetrahedra are separated") {
    auto rows = BoundMesh(std::vector<Point>(ContactTet.begin(), ContactTet.end()), {OneContactTet}, .25);
    if (world.rank == 1) {
      Owned remote;
      AppendContactComponent(remote, rows, 4, 4);
      RenumberContactCell(remote, 2);
      // A shift of twice the edge length gives strictly separate x intervals.
      TranslateContact(remote, {4, 0, 0});
      rows.swap(remote);
    }
    auto memory = ExchangeBudget();
    const auto result = local::VerifyPassContact(world, rows, memory, memory.OwnedBytes(rows.size()), true);
    CHECK(result.verify_cells == 1);
    CHECK(result.verify_calls == 0);
  }
}

TEST_CASE("Pass verification finds a split-created contact on one rank", "[.][NativeFP2C3]") {
  World world;
  REQUIRE(world.size == 1);
  const auto [a, b, c, d] = ContactTet;
  const Point m{1, -.1, -.1};
  auto rows = BoundMesh({a, b, c, d, m}, {{0, 4, 2, 3}, {1, 4, 2, 3}}, .25);
  // Positive barycentric weights (.03,.94,.015,.015) in (B,M,C,D)
  // give (1,-.064,-.064). At 1/1000 scale every bubble vertex still has negative y,z.
  const auto points = NestedContactTet({b, m, c, d}, {1, -.064, -.064}, .001);
  for (const auto& p : points) {
    CHECK(p.y < 0);
    CHECK(p.z < 0);
  }
  AppendContactComponent(rows, BoundMesh(points, {OneContactTet}, .25), 5, 8);
  CheckElectedContact(world, rows, 2, 3, 0);
}

TEST_CASE("Step C screens a remote physical face sharing a tent rim point", "[.][NativeFP2C3][NativeFP2C3MPI]") {
  World world;
  REQUIRE(world.size == 2);
  auto working = S33(.2);
  // The cube at (0,1) starts at cell 19. Its third tetrahedron has
  // (v0,v6,v3,v7) = (4,25,8,24); its exterior x=0 face is (4,8,24).
  constexpr Id remote_id = 21, rim = GridId(0, 1, 0);
  const auto& row = working.at(remote_id);
  auto ids = VertexOrder(row.cell);
  std::sort(ids.begin(), ids.end());
  CHECK(ids == CellOrder{4, 8, 24, 25});
  bool has_face = false;
  for (unsigned f = 0; f < 4; ++f)
    has_face |= row.physical[f].present && Key(FaceAt(row.cell, f)) == Face{4, 8, 24};
  REQUIRE(has_face);
  if (world.rank == 0)
    working.erase(remote_id);
  else
    working = {{remote_id, row}};
  auto memory = ExchangeBudget();
  const auto meeting = local::MeetBorderPoints(world, working, memory);
  constexpr double tolerance = .5;
  auto remote = local::GatherRemoteSurface(world, working, local::ContactMargin * tolerance, memory,
                                           memory.OwnedBytes(working.size()), true);
  C2OnRank(world, 0, [&] {
    local::Mesh mesh(working, true, ContactOptions());
    mesh.MarkBorderSurface(meeting);
    REQUIRE(mesh.Points()[SlotOf(mesh, rim)].border);
    REQUIRE_FALSE(mesh.Points()[SlotOf(mesh, TentRemove)].border);
    REQUIRE_FALSE(mesh.Points()[SlotOf(mesh, TentKeep)].border);
    CollapseFixtureGeometry geometry(working, TentKeep, tolerance);
    local::LocalContactIndex index(mesh);
    local::ContactContext context{index, &remote};
    CheckRemoteTrial(TryContactTent(mesh, geometry, C3Refusal::None, &context));
  });
}
