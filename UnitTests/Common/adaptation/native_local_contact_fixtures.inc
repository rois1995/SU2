/*!
 * \file native_local_contact_fixtures.inc
 * \brief Serial FP2-C3 contact, collapse and current-cell-index fixtures.
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md).
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */
namespace {
using C3Refusal = local::SurfaceCollapseRefusal;
constexpr Id TentRemove = GridId(1, 1, 0), TentKeep = GridId(2, 1, 0);
constexpr std::array<Face, 4> TentChildren{{{0, 1, 6}, {0, 4, 6}, {4, 6, 9}, {6, 9, 10}}};

local::CoreOptions ContactOptions(bool outward = true) {
  local::CoreOptions options;
  options.shape_test = false;
  options.surface_collapses = true;
  options.outward_collapses = outward;
  return options;
}

void AppendContactComponent(Owned& target, const Owned& component, Id point_offset, Id facet_offset) {
  Id cell_id = target.empty() ? 0 : target.rbegin()->first;
  for (const auto& [id, source] : component) {
    auto row = source;
    row.cell.id = ++cell_id;
    for (auto& node : row.cell.v) node.id += point_offset;
    for (auto& face : row.physical)
      if (face.present) face.facet += facet_offset;
    REQUIRE(target.emplace(row.cell.id, row).second);
  }
}

Owned FirstTentBubble(unsigned mode) {
  // The first bottom cap has vertices p, k, a and b. Its new face is (k,a,b),
  // where a=x=y=0 and b=(1,0,0) are the other vertices of S33's first bottom face.
  const std::array<Point, 4> cap{{{1, 1, .2}, {2, 1, 0}, {0, 0, 0}, {1, 0, 0}}};
  std::vector<Point> points;
  if (mode == 3) {
    // q=(k+a+2b)/4 lies strictly in the child and is exactly on z=0. The
    // other three corners span a plane at depth 1/16 with legs 1/16.
    const Point q{1, .25, 0};
    constexpr double step = 1. / 16;
    points = {q, {q.x, q.y, -step}, {q.x + step, q.y, -step}, {q.x, q.y + step, -step}};
  } else {
    // Shrinking to one quarter about the barycentre gives strictly positive
    // barycentric weights for all four corners. The downshift is the cube height.
    const Point center{1, .5, .05};
    for (const auto p : cap) {
      points.push_back({center.x + (p.x - center.x) / 4, center.y + (p.y - center.y) / 4,
                        center.z + (p.z - center.z) / 4 - (mode == 2 ? 1 : 0)});
    }
  }
  Owned rows;
  // Point ids and facet ids start above all 32 points and at most 4*54 faces of S33.
  AppendContactComponent(rows, BoundMesh(points, {{0, 1, 2, 3}}, .25), LastS33Point + 1, 4 * 54);
  return rows;
}

Owned TentBubble(unsigned mode) {
  auto rows = S33(.2);
  AppendContactComponent(rows, FirstTentBubble(mode), 0, 0);
  return rows;
}

Owned CollapsedTentWithBubble(unsigned mode) {
  // Construct the post-collapse mesh independently: erase tets containing
  // both endpoints, substitute the keeper in the rest of the removed star,
  // and regenerate exterior bindings from connectivity.
  const auto initial = S33();
  std::vector<Point> points(LastS33Point + 1);
  std::vector<Tet> tets;
  for (const auto& [id, row] : initial) {
    auto ids = VertexOrder(row.cell);
    for (const auto& node : row.cell.v) points.at(node.id) = node.p;
    const auto p = std::find(ids.begin(), ids.end(), TentRemove);
    if (p != ids.end()) {
      if (std::find(ids.begin(), ids.end(), TentKeep) != ids.end()) continue;
      *p = TentKeep;
    }
    tets.push_back({ids[0], ids[1], ids[2], ids[3]});
  }
  auto result = BoundMesh(points, tets, .25);
  AppendContactComponent(result, FirstTentBubble(mode), 0, 0);
  return result;
}

struct TentTrial {
  C3Refusal result;
  local::SurfaceCounters stats;
  std::vector<SurfaceFace> children;
};

class TentOverlayGeometry final : public FlatGeometry {
 public:
  TentOverlayGeometry(const Owned& owned, local::StateSurfaceGeometry& overlay) : FlatGeometry(owned), overlay_(overlay) {}

  double Tolerance() const override { return .5; }
  unsigned Dimension(Id id) const override { return id == TentKeep ? 0 : 2; }
  bool OnEntityFace(const SurfaceFace&) const override { return true; }
  bool WithinTolerance(const SurfaceFace&) override { return true; }
  void Remove(Id id) override {
    REQUIRE(removed.insert(id).second);
    overlay_.Remove(id);
  }

  std::set<Id> removed;

 private:
  local::StateSurfaceGeometry& overlay_;
};

TentTrial TryContactTent(local::Mesh& mesh, local::SurfaceGeometry& geometry, C3Refusal early,
                         local::ContactContext* context, bool check_tent = true) {
  const auto [edge, seed] = C2Edge(mesh, {TentRemove, TentKeep});
  const auto [refusal, fresh, caps] = local::SurfaceFixtureAccess::TentPrepared(mesh, edge, seed, geometry);
  REQUIRE(refusal == early);
  if (check_tent && early == C3Refusal::None) CHECK(fresh == 9);
  TentTrial outcome;
  outcome.result = mesh.CollapseSurface(edge, seed, geometry, 2, outcome.stats, true,
                                        [&](const auto& face) { outcome.children.push_back(face); }, context);
  return outcome;
}

void CheckContactMesh(const local::Mesh& mesh, Owned rows) {
  REQUIRE_NOTHROW(mesh.Check());
  for (const auto& [ids, cell] : LiveCells(mesh)) CHECK(TetMeasure(cell) > 0);
  CHECK(native_fp2_c3::EmbeddingViolations(mesh).violations == 0);
  local::IdSequence cells(rows.rbegin()->first);
  mesh.WriteBack(rows, cells);
  World world;
  REQUIRE(world.size == 1);
  auto memory = ExchangeBudget();
  REQUIRE_NOTHROW(local::VerifyPassContact(world, rows, memory, 0, true));
}

size_t ContactCoverage(const std::map<CellOrder, Cell>& cells, Point witness) {
  size_t containing = 0;
  for (const auto& [ids, cell] : cells) {
    Tetrahedron corners;
    for (size_t i = 0; i < corners.size(); ++i) corners[i] = cell.v[i].p;
    std::array<long double, 4> weights;
    if (Barycentric(corners, witness, weights)) ++containing;
  }
  return containing;
}

void CheckContactTent(double height, size_t limit) {
  CAPTURE(height, limit);
  const auto rows = S33(height);
  CollapseFixtureGeometry geometry(rows, TentKeep, .5);
  REQUIRE(geometry.Limit() == Approx(.35));
  local::Mesh mesh(rows, false, ContactOptions());
  local::LocalContactIndex index(mesh, limit);
  local::ContactContext contact{index, nullptr};
  const auto [edge, seed] = C2Edge(mesh, {TentRemove, TentKeep});
  const auto [early, fresh, caps] = local::SurfaceFixtureAccess::TentPrepared(mesh, edge, seed, geometry);
  REQUIRE(early == C3Refusal::None);
  CHECK(caps == (height > 0 ? 4 : 0));
  // This is the barycentre of the first cap, with equal weights on p,k,a,b.
  const Point witness{1, .5, height / 4};
  if (height > 0) CHECK(ContactCoverage(LiveCells(mesh), witness) == 0);
  const auto trial = TryContactTent(mesh, geometry, C3Refusal::None, &contact);
  REQUIRE(trial.result == C3Refusal::None);
  if (height > 0) CHECK(ContactCoverage(LiveCells(mesh), witness) == 1);
  CHECK(trial.stats.c_contact_directions == 1);
  CHECK(trial.stats.c_contact_calls > 0);
  CHECK(trial.stats.collapses_outward == (height > 0 ? 1 : 0));
  CHECK(trial.stats.c_refusals[static_cast<size_t>(C3Refusal::Outward)] == 0);
  CHECK(trial.stats.c_contact_rebuilds == (limit == 1 ? 1 : 0));
  CHECK(trial.stats.c_reverse_max <= std::abs(height) + 1e-14);
  CHECK(geometry.removed == std::set<Id>{TentRemove});
  std::vector<Face> children;
  double altitude = std::numeric_limits<double>::max();
  for (const auto& child : trial.children) {
    children.push_back(Key(child.v));
    std::array<Point, 3> points;
    for (size_t i = 0; i < points.size(); ++i) {
      points[i] = mesh.Points()[SlotOf(mesh, child.v[i])].node.p;
      CHECK(points[i].z == 0);
    }
    altitude = std::min(altitude, local::SurfaceFixtureAccess::SmallestAltitude(points[0], points[1], points[2]));
  }
  std::sort(children.begin(), children.end());
  REQUIRE(children.size() == TentChildren.size());
  CHECK(std::equal(children.begin(), children.end(), TentChildren.begin()));
  // The first and fourth triangles have doubled area 1 and longest edge sqrt(5)
  // and sqrt(2), respectively; the second and third have doubled area 2.
  CHECK(altitude == Approx(1 / std::sqrt(5.)));
  CheckContactMesh(mesh, rows);
}
}  // namespace

TEST_CASE("FP2-C3 tent collapses in all three directions with contact enabled", "[.][NativeFP2C3]") {
  World world;
  if (world.size != 1) return;
  SECTION("The outward direction is refused at test 4 with the option off") {
    const auto rows = S33(.2);
    CollapseFixtureGeometry geometry(rows, TentKeep, .5);
    local::Mesh mesh(rows, false, ContactOptions(false));
    const auto trial = TryContactTent(mesh, geometry, C3Refusal::Outward, nullptr);
    CHECK(trial.result == C3Refusal::Outward);
    CHECK(trial.stats.c_contact_directions == 0);
    CHECK(geometry.removed.empty());
  }
  SECTION("An enabled direct caller without a context cannot commit") {
    const auto rows = S33(.2);
    CollapseFixtureGeometry geometry(rows, TentKeep, .5);
    local::Mesh mesh(rows, false, ContactOptions());
    const auto before = Snapshot(mesh);
    const auto trial = TryContactTent(mesh, geometry, C3Refusal::None, nullptr);
    CHECK(trial.result == C3Refusal::Contact);
    CHECK(Snapshot(mesh) == before);
    CHECK(geometry.removed.empty());
  }
  SECTION("With a current-cell index") {
    // S33 has three tets on the p-k edge and nine other cells in Star(p).
    // The two bottom fans cover the same planar polygon; a vertical witness
    // for each original fan sample is at distance at most |height|.
    for (const auto height : {.2, -.1, 0.})
      for (const size_t limit : {size_t(4096), size_t(1)}) CheckContactTent(height, limit);
  }
}

TEST_CASE("FP2-C3 embedding and contact checks detect an overlap and a forbidden point touch", "[.][NativeFP2C3]") {
  World world;
  if (world.size != 1) return;
  for (const unsigned mode : {1U, 2U, 3U}) {
    CAPTURE(mode);
    const auto rows = TentBubble(mode);
    // All input components are disjoint; S33 contains conforming face,
    // edge and vertex pairs among the tested boxes.
    const local::Mesh old(rows);
    const auto old_embedding = native_fp2_c3::EmbeddingViolations(old);
    CHECK(old_embedding.violations == 0);
    CHECK(old_embedding.calls > 0);
    const auto changed = CollapsedTentWithBubble(mode);
    const local::Mesh changed_mesh(changed);
    const auto changed_embedding = native_fp2_c3::EmbeddingViolations(changed_mesh);
    if (mode == 2) {
      CHECK(changed_embedding.violations == 0);
    } else {
      CHECK(changed_embedding.violations > 0);
      auto memory = ExchangeBudget();
      CHECK_THROWS_AS(local::VerifyPassContact(world, changed, memory, 0, true), CElectedFailure);
    }
    for (const size_t limit : {size_t(4096), size_t(1)}) {
      CAPTURE(limit);
      // Let the fake geometry forward any removal to the real overlay so a
      // mutation before the N2 result is visible in both states.
      local::StateSurfaceGeometry overlay(EntityReference(S33(.2), false), AtlasBudget());
      TentOverlayGeometry geometry(rows, overlay);
      local::Mesh mesh(rows, false, ContactOptions());
      local::LocalContactIndex index(mesh, limit);
      local::ContactContext context{index, nullptr};
      const auto points_and_cells = Snapshot(mesh), frozen = FrozenSnapshot(mesh);
      const auto bindings_and_overlay = OverlaySnapshot(mesh, overlay);
      const local::ContactBox all{-1, -1, -2, 4, 4, 2};
      const auto candidates = index.Candidates(all);
      const auto trial = TryContactTent(mesh, geometry, C3Refusal::None, &context);
      CHECK(trial.stats.c_contact_directions == 1);
      CHECK(trial.stats.c_outward_tried == 1);
      CHECK(trial.stats.c_contact_calls > 0);
      if (mode == 2) {
        REQUIRE(trial.result == C3Refusal::None);
        CHECK(trial.stats.c_contact_rebuilds == (limit == 1 ? 1 : 0));
        CheckContactMesh(mesh, rows);
      } else {
        REQUIRE(trial.result == C3Refusal::Contact);
        CHECK(trial.children.empty());
        CHECK(Snapshot(mesh) == points_and_cells);
        CHECK(FrozenSnapshot(mesh) == frozen);
        CHECK(OverlaySnapshot(mesh, overlay) == bindings_and_overlay);
        CHECK(geometry.removed.empty());
        CHECK(index.Candidates(all) == candidates);
        CHECK(index.FreshSize() == 0);
        CHECK(trial.stats.c_contact_rebuilds == 0);
      }
    }
  }
}

namespace {
bool CellBoxContains(const local::Mesh& mesh, local::Vertices vertices, Point sample) {
  auto lo = Vector(mesh.Points()[vertices[0]].node.p), hi = lo;
  for (const auto vertex : vertices) {
    const auto p = Vector(mesh.Points()[vertex].node.p);
    for (size_t axis = 0; axis < lo.size(); ++axis) {
      lo[axis] = std::min(lo[axis], p[axis]);
      hi[axis] = std::max(hi[axis], p[axis]);
    }
  }
  const auto p = Vector(sample);
  for (size_t axis = 0; axis < lo.size(); ++axis)
    if (p[axis] < lo[axis] || p[axis] > hi[axis]) return false;
  return true;
}
}  // namespace

TEST_CASE("FP2-C3 local contact queries return current cells over a commit and a rebuild", "[.][NativeFP2C3]") {
  World world;
  if (world.size != 1) return;
  const auto rows = S33(.2);
  for (const size_t limit : {size_t(4096), size_t(1)}) {
    CAPTURE(limit);
    CollapseFixtureGeometry geometry(rows, TentKeep, .5);
    local::Mesh mesh(rows, false, ContactOptions());
    local::LocalContactIndex index(mesh, limit);
    local::ContactContext context{index, nullptr};
    const auto old = mesh.Cells();
    const auto trial = TryContactTent(mesh, geometry, C3Refusal::None, &context);
    REQUIRE(trial.result == C3Refusal::None);
    // Nine new rows go to the fresh list, or directly to the rebuilt tree
    // when the test limit is one. Compare point-box queries with all live rows.
    CHECK(index.FreshSize() == (limit == 1 ? 0 : 9));
    CHECK(trial.stats.c_contact_rebuilds == (limit == 1 ? 1 : 0));
    bool fresh_only = false, dead_skipped = false, reused_skipped = false;
    for (int z = 0; z <= 4; ++z) {
      for (int y = 0; y <= 12; ++y) {
        for (int x = 0; x <= 12; ++x) {
          // Quarter-grid samples include the cube boundaries and z=1/4,
          // just above the old raised bottom point. Closed queries are points.
          const Point sample{x / 4., y / 4., z / 4.};
          const local::ContactBox query{sample.x, sample.y, sample.z, sample.x, sample.y, sample.z};
          const auto actual = index.Candidates(query);
          std::vector<local::Slot> expected;
          for (local::Slot slot = 0; slot < mesh.Cells().size(); ++slot) {
            const auto& cell = mesh.Cells()[slot];
            const bool new_hit = cell.live && CellBoxContains(mesh, cell.vertices, sample);
            const bool old_hit = slot < old.size() && CellBoxContains(mesh, old[slot].vertices, sample);
            const bool current_hit = std::binary_search(actual.begin(), actual.end(), slot);
            if (new_hit) expected.push_back(slot);
            if (new_hit && !old_hit && current_hit) fresh_only = true;
            if (old_hit && !cell.live && !current_hit) dead_skipped = true;
            if (old_hit && cell.live && old[slot].vertices != cell.vertices && !new_hit && !current_hit)
              reused_skipped = true;
          }
          CHECK(actual == expected);
        }
      }
    }
    CHECK(fresh_only);
    CHECK(dead_skipped);
    CHECK(reused_skipped);
    CheckContactMesh(mesh, rows);
  }
}

TEST_CASE("FP2-C3 mixed inward and outward caps can collapse without contact", "[.][NativeFP2C3]") {
  World world;
  if (world.size != 1) return;
  auto rows = S33(.1);
  // At p's projection, the affine weights in the first child (a,b,k) are
  // (1,-1,1). Lowering b's height to -.15 puts this extrapolated plane at
  // +.15, above p=.1. The other child planes remain z=0. Both fans project
  // onto the same footprint and lie in a vertical slab of thickness .25.
  for (auto& [id, row] : rows)
    for (auto& node : row.cell.v)
      if (node.id == GridId(1, 0, 0)) node.p.z = -.15;
  for (const auto& [id, row] : rows) REQUIRE(TetMeasure(row.cell) > 0);
  for (const size_t limit : {size_t(4096), size_t(1)}) {
    CAPTURE(limit);
    CollapseFixtureGeometry geometry(rows, TentKeep, .5);
    local::Mesh mesh(rows, false, ContactOptions());
    const auto [edge, seed] = C2Edge(mesh, {TentRemove, TentKeep});
    const auto [early, fresh, caps] = local::SurfaceFixtureAccess::TentPrepared(mesh, edge, seed, geometry);
    REQUIRE(early == C3Refusal::None);
    CHECK(caps == 3);
    local::LocalContactIndex index(mesh, limit);
    local::ContactContext context{index, nullptr};
    const auto trial = TryContactTent(mesh, geometry, C3Refusal::None, &context);
    REQUIRE(trial.result == C3Refusal::None);
    CHECK(trial.stats.c_contact_directions == 1);
    CHECK(trial.stats.c_contact_calls > 0);
    CHECK(trial.stats.collapses_outward == 1);
    CHECK(trial.stats.c_contact_rebuilds == (limit == 1 ? 1 : 0));
    CHECK(trial.stats.c_reverse_max <= .25);
    CheckContactMesh(mesh, rows);
  }
}

TEST_CASE("FP2-C3 new-cell contact rejects an overlapping positive plan", "[.][NativeFP2C3]") {
  World world;
  if (world.size != 1) return;
  // A has three axis legs of length two from keeper k. B shares only k:
  // each remaining vertex has one coordinate 1 and two coordinates 1/4.
  // Positive coordinates and sum 3/2 < 2 put those vertices inside A.
  // The three legs of B have eigenvalues 3/2, 3/4, 3/4, so det(B)>0.
  const auto rows = BoundMesh({{0, 0, 0}, {2, 0, 0}, {0, 2, 0}, {0, 0, 2},
                               {1, .25, .25}, {.25, 1, .25}, {.25, .25, 1}},
                              {{0, 1, 2, 3}, {0, 4, 5, 6}}, .25);
  const local::Mesh mesh(rows);
  REQUIRE(TetMeasure(rows.at(1).cell) > 0);
  REQUIRE(TetMeasure(rows.at(2).cell) > 0);
  REQUIRE_FALSE(ConformingDisjoint(rows.at(1).cell, rows.at(2).cell));
  const std::vector<local::Vertices> plan{mesh.Cells()[0].vertices, mesh.Cells()[1].vertices};
  for (const size_t limit : {size_t(4096), size_t(1)}) {
    local::LocalContactIndex index(mesh, limit);
    local::SurfaceCounters counters;
    CHECK_FALSE(index.NewCellsDisjoint(plan, counters, true));
    CHECK(counters.c_contact_calls == 1);
    CHECK(index.FreshSize() == 0);
  }
}

TEST_CASE("FP2-C3 work-list and full scans agree on three tents", "[.][NativeFP2C3]") {
  World world;
  if (world.size != 1) return;
  Owned rows;
  constexpr Id points_per_tent = LastS33Point + 1;
  for (size_t i = 0; i < 3; ++i) {
    auto component = S33(.2);
    for (auto& [id, row] : component)
      for (auto& node : row.cell.v) node.p.x += 5 * i;
    // Each component spans three unit cubes, with a gap of two before the next.
    AppendContactComponent(rows, component, points_per_tent * i, 4 * component.size() * i);
  }
  class ThreeTentGeometry final : public FlatGeometry {
   public:
    explicit ThreeTentGeometry(const Owned& rows) : FlatGeometry(rows) {}
    double Tolerance() const override { return .5; }
    unsigned Dimension(Id id) const override { return id % (LastS33Point + 1) == TentKeep ? 0 : 2; }
    bool OnEntityFace(const SurfaceFace&) const override { return true; }
    bool WithinTolerance(const SurfaceFace&) override { return true; }
    void Remove(Id id) override { REQUIRE(removed.insert(id).second); }
    std::set<Id> removed;
  };
  const auto run = [&](bool work_list) {
    CAPTURE(work_list);
    auto options = ContactOptions();
    options.work_list = work_list;
    local::Mesh mesh(rows, false, options);
    ThreeTentGeometry geometry(rows);
    local::PassStats report;
    report.report = true;
    local::SurfaceFixtureAccess::SurfaceCollapseWork(mesh, geometry, report);
    CHECK(report.surface.collapses_surface >= 3);
    CHECK(report.surface.c_contact_calls > 0);
    CheckContactMesh(mesh, rows);
    return std::make_tuple(OrderedCells(mesh), OrientedSurface(mesh), geometry.removed);
  };
  CHECK(run(true) == run(false));
}

TEST_CASE("FP2-C3 collapse guards the minimum length of each new edge", "[.][NativeFP2C3]") {
  World world;
  if (world.size != 1) return;
  // The ring of 5-6 has opposite vertices {1,10,22,26}. The other cells
  // in Star(5), after dropping those vertices, give {0,4,9,21,25}.
  // Relative to (2,1,0) these have squared distances {5,4,2,2,3}.
  // Scaling by (minimum / sqrt(2))^2 makes the two shortest edges
  // exactly minimum in the metric. Both are short in the .009 arm.
  const auto run = [](double minimum, bool outward, C3Refusal expected) {
    CAPTURE(minimum, outward);
    const auto scale = minimum / std::sqrt(2.);
    auto rows = S33();
    for (auto& entry : rows)
      entry.second.sensor.fill({scale * scale, 0, 0, scale * scale, 0, scale * scale});
    local::StateSurfaceGeometry overlay(EntityReference(rows, false), AtlasBudget());
    TentOverlayGeometry geometry(rows, overlay);
    local::Mesh mesh(rows, false, ContactOptions(outward));
    const auto [edge, seed] = C2Edge(mesh, {TentRemove, TentKeep});
    const auto [early, fresh, caps] = local::SurfaceFixtureAccess::TentPrepared(mesh, edge, seed, geometry);
    REQUIRE(early == expected);
    CHECK(fresh == (expected == C3Refusal::Size ? 0 : 9));
    CHECK(caps == 0);
    const auto snapshot = Snapshot(mesh), frozen = FrozenSnapshot(mesh);
    const auto saved_overlay = OverlaySnapshot(mesh, overlay);
    const auto keeper_edges = [&] {
      std::map<Id, double> lengths;
      for (const auto& use : mesh.Edges()) {
        const auto a = mesh.Points()[use.edge[0]].node.id, b = mesh.Points()[use.edge[1]].node.id;
        if (a == TentKeep || b == TentKeep) lengths.emplace(a == TentKeep ? b : a, mesh.Length(use.edge));
      }
      return lengths;
    };
    const auto old_edges = keeper_edges();
    const auto live_point_count = [&] {
      return std::count_if(mesh.Points().begin(), mesh.Points().end(), [](const auto& point) { return !point.removed; });
    };
    const auto old_points = live_point_count();
    std::optional<local::LocalContactIndex> index;
    std::optional<local::ContactContext> contact;
    if (outward) {
      index.emplace(mesh);
      contact.emplace(local::ContactContext{*index, nullptr});
    }
    local::SurfaceCounters stats;
    REQUIRE(mesh.CollapseSurface(edge, seed, geometry, 2, stats, true, {}, contact ? &*contact : nullptr) == expected);
    const bool refused = expected == C3Refusal::Size;
    CHECK(stats.c_refused_metric == (refused ? 1 : 0));
    CHECK(stats.c_refusals[static_cast<size_t>(C3Refusal::Size)] == (refused ? 1 : 0));
    CHECK(stats.c_refusals[static_cast<size_t>(C3Refusal::Protected)] == 1);
    if (refused) {
      CHECK(Snapshot(mesh) == snapshot);
      CHECK(FrozenSnapshot(mesh) == frozen);
      CHECK(OverlaySnapshot(mesh, overlay) == saved_overlay);
      CHECK(live_point_count() == old_points);
      CHECK(geometry.removed.empty());
    } else {
      CHECK(live_point_count() == old_points - 1);
      CHECK(geometry.removed == std::set<Id>{TentRemove});
      CHECK(overlay.Removals() == 1);
      const std::map<Id, unsigned> squared_distances{{0, 5}, {4, 4}, {9, 2}, {21, 2}, {25, 3}};
      std::vector<Id> new_neighbors;
      size_t short_edges = 0;
      for (const auto& [id, length] : keeper_edges()) {
        if (old_edges.count(id)) continue;
        new_neighbors.push_back(id);
        REQUIRE(squared_distances.count(id) == 1);
        CHECK(length == Approx(scale * std::sqrt(squared_distances.at(id))));
        short_edges += length < local::HandoverMetricEdge;
      }
      CHECK(new_neighbors == std::vector<Id>{0, 4, 9, 21, 25});
      CHECK(short_edges == (outward ? 0 : 2));
    }
  };
  run(.009, true, C3Refusal::Size);
  run(.011, true, C3Refusal::None);
  run(.009, false, C3Refusal::None);
}

TEST_CASE("FP2-C3 reports distinct short edges after each surface step", "[.][NativeFP2C3]") {
  World world;
  if (world.size != 1) return;
  class ProtectedGeometry final : public FlatGeometry {
   public:
    explicit ProtectedGeometry(const Owned& rows) : FlatGeometry(rows) {}
    unsigned Dimension(Id) const override { return 0; }
  };

  // Three unit legs meet at the origin and the other three edges join the
  // axis tips at distance sqrt(2). In the constant metric the lengths are
  // .005 and .005*sqrt(2), all below .01. All points and edges lie on the skin.
  const auto rows = BoundMesh({{0, 0, 0}, {1, 0, 0}, {0, 1, 0}, {0, 0, 1}}, {{0, 1, 2, 3}}, .000025);
  for (bool outward : {true, false}) {
    CAPTURE(outward);
    auto options = ContactOptions(outward);
    options.surface_splits = options.distance_error_cut = true;
    local::Mesh mesh(rows, false, options);
    ProtectedGeometry geometry(rows);
    const auto before = Snapshot(mesh);
    local::IdSequence ids(3);
    const auto stats = mesh.RunPass(ids, SIZE_MAX, true, options, &geometry);
    CHECK(Snapshot(mesh) == before);
    CHECK(ids.Peek() == 4);
    CHECK(geometry.created.empty());
    REQUIRE(mesh.Edges().size() == 6);
    const size_t expected = outward ? 6 : 0;
    CHECK(stats.surface.c_refused_metric == 0);
    CHECK(stats.surface.short_after_l == expected);
    CHECK(stats.surface.short_after_d == expected);
    CHECK(stats.surface.short_after_c == expected);
    std::ostringstream fields;
    local::PrintSurfaceFields(fields, stats.surface);
    const auto count = std::to_string(expected);
    CHECK(fields.str().find(" c_refused_metric=0 short_after_l=" + count + " short_after_d=" + count +
                            " short_after_c=" + count + " seconds_c_contact=") != std::string::npos);
    if (!outward) std::cout << "FP2_C3 option_off_surface " << fields.str() << '\n';
  }
}
