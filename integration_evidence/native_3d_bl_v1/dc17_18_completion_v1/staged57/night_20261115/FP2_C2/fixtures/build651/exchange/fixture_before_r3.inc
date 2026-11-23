/*!
 * \file native_local_surface_collapse_fixtures.inc
 * \brief FP2-C2 surface-collapse fixtures sharing the native surface test mesh builders.
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md).
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */
namespace {
using C2Refusal = local::SurfaceCollapseRefusal;
using CellOrder = std::array<Id, 4>;
constexpr Id LastS33Point = 31;

constexpr Id GridId(size_t x, size_t y, size_t z) { return x + 4 * y + 16 * z; }

Owned S33(double height = 0) {
  constexpr std::array<size_t, 6> ring{1, 2, 3, 7, 4, 5};
  std::vector<Point> points;
  for (size_t z = 0; z < 2; ++z)
    for (size_t y = 0; y < 4; ++y)
      for (size_t x = 0; x < 4; ++x) points.push_back({double(x), double(y), double(z)});
  points[GridId(1, 1, 0)].z = height;
  std::vector<Tet> tets;
  for (size_t y = 0; y < 3; ++y)
    for (size_t x = 0; x < 3; ++x) {
      const std::array<size_t, 8> v{GridId(x, y, 0), GridId(x + 1, y, 0), GridId(x + 1, y + 1, 0), GridId(x, y + 1, 0),
                                    GridId(x, y, 1), GridId(x + 1, y, 1), GridId(x + 1, y + 1, 1), GridId(x, y + 1, 1)};
      for (size_t i = 0; i < ring.size(); ++i) tets.push_back({v[0], v[6], v[ring[i]], v[ring[(i + 1) % ring.size()]]});
    }
  return BoundMesh(points, tets, .25);
}

std::shared_ptr<const PlanarReference> S33Reference(const Owned& owned) {
  auto reference = EntityReference(owned, false);
  const auto& traces = reference->Parameters()->Coverage().traces;
  local::StateSurfaceGeometry geometry(reference, AtlasBudget());
  for (size_t z = 0; z < 2; ++z)
    for (size_t y = 0; y < 4; ++y)
      for (size_t x = 0; x < 4; ++x) {
        const auto id = GridId(x, y, z);
        const bool edge_x = y == 0 || y == 3;
        const bool edge_y = x == 0 || x == 3;
        if (edge_x && x < 3) CHECK(traces.count({id, GridId(x + 1, y, z)}) == 1);
        if (edge_y && y < 3) CHECK(traces.count({id, GridId(x, y + 1, z)}) == 1);
        if (edge_x && edge_y && z == 0) CHECK(traces.count({id, GridId(x, y, 1)}) == 1);
        if (edge_x || edge_y) {
          CHECK(geometry.Dimension(id) < 2);
        } else {
          CHECK(geometry.Dimension(id) == 2);
        }
      }
  return reference;
}

class CollapseFixtureGeometry final : public FlatGeometry {
 public:
  CollapseFixtureGeometry(const Owned& owned, Id keep, double tolerance)
      : FlatGeometry(owned), keep_(keep), tolerance_(tolerance) {}

  double Tolerance() const override { return tolerance_; }
  unsigned Dimension(Id id) const override { return id == keep_ ? 0 : 2; }
  bool OnEntityFace(const SurfaceFace&) const override { return true; }
  void Remove(Id id) override { REQUIRE(removed.insert(id).second); }

  std::set<Id> removed;

 private:
  Id keep_;
  double tolerance_;
};

template <class Geometry>
class OneSplitGeometry final : public Geometry {
 public:
  OneSplitGeometry(const Owned& rows, Edge edge) : Geometry(rows), edge_(edge) {}

  local::SurfacePlacement Place(const std::array<Node, 2>& nodes, const std::array<SurfaceFace, 2>& parents,
                                Id id) override {
    if (EdgeKey(nodes) == edge_) return Geometry::Place(nodes, parents, id);
    local::SurfacePlacement refused;
    refused.refusal = local::SurfaceRefusal::Protected;
    return refused;
  }

 private:
  Edge edge_;
};

std::pair<local::Edge, local::Slot> C2Edge(const local::Mesh& mesh, Edge ids) {
  const local::Edge edge{SlotOf(mesh, ids[0]), SlotOf(mesh, ids[1])};
  for (local::Slot c = 0; c < mesh.Cells().size(); ++c) {
    const auto& row = mesh.Cells()[c];
    if (row.live && std::all_of(edge.begin(), edge.end(), [&](local::Slot p) {
          return std::find(row.vertices.begin(), row.vertices.end(), p) != row.vertices.end();
        }))
      return {edge, c};
  }
  FAIL("Surface collapse fixture could not find its edge.");
  return {};
}

Id C2Split(local::Mesh& mesh, Edge ids, local::SurfaceGeometry& geometry, local::IdSequence& point_ids) {
  const auto [edge, seed] = C2Edge(mesh, ids);
  const Id point = point_ids.Peek();
  local::SurfaceCounters stats;
  const auto result = mesh.SplitSurface(edge, seed, geometry, point_ids, 0, SIZE_MAX, stats, false);
  INFO(stats.first_feature_reason << ' ' << stats.first_trace_reason);
  REQUIRE(result == local::SurfaceRefusal::None);
  return point;
}

C2Refusal C2Collapse(local::Mesh& mesh, Edge ids, local::SurfaceGeometry& geometry,
                     const std::function<void(const SurfaceFace&)>& sink = {}) {
  const auto [edge, seed] = C2Edge(mesh, ids);
  local::SurfaceCounters stats;
  return mesh.CollapseSurface(edge, seed, geometry, 2, stats, true, sink);
}

std::vector<char> FrozenSnapshot(const local::Mesh& mesh) {
  std::vector<char> bytes;
  RecordStream stream(bytes);
  for (auto row : local::SurfaceFixtureAccess::Frozen(mesh)) stream(row.slot, row.node, row.tensor);
  return bytes;
}

std::vector<char> OverlaySnapshot(const local::Mesh& mesh, const local::StateSurfaceGeometry& geometry) {
  std::vector<char> bytes;
  RecordStream stream(bytes);
  auto splits = geometry.Splits(), removals = geometry.Removals(), bytes_used = geometry.ResidentBytes();
  stream(splits, removals, bytes_used);
  for (const auto& row : mesh.Points()) {
    const auto* point = geometry.Find(row.node.id);
    bool found = point != nullptr;
    stream(found);
    if (found) {
      auto record = *point;
      stream(record);
    }
  }
  return bytes;
}

std::vector<std::pair<Face, Id>> OrientedSurface(const local::Mesh& mesh) {
  std::vector<std::pair<Face, Id>> ordered;
  for (const auto& face : Surface(mesh)) ordered.emplace_back(face.v, face.facet);
  std::sort(ordered.begin(), ordered.end());
  return ordered;
}

CellOrder VertexOrder(const Cell& cell) { return {cell.v[0].id, cell.v[1].id, cell.v[2].id, cell.v[3].id}; }

std::map<CellOrder, Cell> LiveCells(const local::Mesh& mesh) {
  std::map<CellOrder, Cell> cells;
  for (const auto& row : mesh.Cells()) {
    if (!row.live) continue;
    Cell cell;
    for (unsigned i = 0; i < 4; ++i) cell.v[i] = mesh.Points()[row.vertices[i]].node;
    REQUIRE(cells.emplace(VertexOrder(cell), cell).second);
  }
  return cells;
}

std::set<CellOrder> OrderedCells(const local::Mesh& mesh) {
  std::set<CellOrder> orders;
  for (const auto& entry : LiveCells(mesh)) orders.insert(entry.first);
  return orders;
}

using Vec3 = std::array<double, 3>;
Vec3 Vector(Point p) { return {p.x, p.y, p.z}; }
Vec3 Cross(const Vec3& a, const Vec3& b) {
  return {a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]};
}

std::array<Vec3, 6> TetEdges(const Cell& cell) {
  std::array<Vec3, 6> edges;
  size_t n = 0;
  for (size_t i = 0; i < 4; ++i)
    for (size_t j = i + 1; j < 4; ++j) edges[n++] = SU2NativeCurved::Sub<3>(Vector(cell.v[j].p), Vector(cell.v[i].p));
  return edges;
}

bool DisjointTets(const Cell& a, const Cell& b) {
  using SU2NativeCurved::Dot;
  using SU2NativeCurved::Sub;
  const auto a_edges = TetEdges(a), b_edges = TetEdges(b);
  double scale_squared = 0;
  for (const auto* edges : {&a_edges, &b_edges})
    for (const auto& edge : *edges) scale_squared = std::max(scale_squared, Dot<3>(edge, edge));
  const double margin = 1e-12 * std::sqrt(scale_squared);
  const auto origin = Vector(a.v[0].p);
  const auto separates = [&](const Vec3& axis) {
    const double length = std::sqrt(Dot<3>(axis, axis));
    if (length == 0) return false;
    const auto bounds = [&](const Cell& cell) {
      std::array<double, 4> distances;
      for (size_t i = 0; i < distances.size(); ++i)
        distances[i] = Dot<3>(axis, Sub<3>(Vector(cell.v[i].p), origin)) / length;
      const auto range = std::minmax_element(distances.begin(), distances.end());
      return std::make_pair(*range.first, *range.second);
    };
    const auto [alo, ahi] = bounds(a);
    const auto [blo, bhi] = bounds(b);
    return ahi <= blo + margin || bhi <= alo + margin;
  };
  for (const auto* cell : {&a, &b})
    for (size_t i = 0; i < 4; ++i)
      for (size_t j = i + 1; j < 4; ++j)
        for (size_t k = j + 1; k < 4; ++k) {
          const auto p = Vector(cell->v[i].p);
          if (separates(Cross(Sub<3>(Vector(cell->v[j].p), p), Sub<3>(Vector(cell->v[k].p), p)))) return true;
        }
  for (const auto& u : a_edges)
    for (const auto& v : b_edges)
      if (separates(Cross(u, v))) return true;
  return false;
}

long double TetMeasure(const Cell& cell) { return Orientation(cell.v[0].p, cell.v[1].p, cell.v[2].p, cell.v[3].p); }

C2Refusal CollapseWithProof(local::Mesh& mesh, Id remove, Id keep, local::SurfaceGeometry& geometry,
                            const std::vector<Cell>& remote = {}) {
  const auto before = LiveCells(mesh);
  const auto skin = Surface(mesh);
  const auto removed = mesh.Points()[SlotOf(mesh, remove)].node.p;
  std::vector<SurfaceFace> children;
  const auto result = C2Collapse(mesh, {remove, keep}, geometry, [&](const auto& face) { children.push_back(face); });
  if (result != C2Refusal::None) {
    CHECK(children.empty());
    return result;
  }
  const auto after = LiveCells(mesh);
  CHECK(after.size() < before.size());
  REQUIRE(std::none_of(mesh.Points().begin(), mesh.Points().end(),
                       [remove](const auto& row) { return !row.removed && row.node.id == remove; }));
  long double previous_measure = 0, current_measure = 0;
  for (const auto& [order, cell] : before) {
    auto fresh = order;
    const auto p = std::find(fresh.begin(), fresh.end(), remove);
    if (p == fresh.end()) continue;
    previous_measure += TetMeasure(cell);
    if (std::find(fresh.begin(), fresh.end(), keep) != fresh.end()) continue;
    *p = keep;
    const auto found = after.find(fresh);
    REQUIRE(found != after.end());
    const auto measure = TetMeasure(found->second);
    CHECK(measure > 0);
    current_measure += measure;
    const auto check_separation = [&](const Cell& neighbor) {
      const auto other = VertexOrder(neighbor);
      const bool shared = std::any_of(fresh.begin(), fresh.end(),
                                      [&](Id id) { return std::find(other.begin(), other.end(), id) != other.end(); });
      if (!shared) CHECK(DisjointTets(found->second, neighbor));
    };
    for (const auto& entry : after) check_separation(entry.second);
    for (const auto& cell : remote) check_separation(cell);
  }
  std::map<Face, Id> expected_children, received_children;
  for (auto parent : skin) {
    const auto p = std::find(parent.v.begin(), parent.v.end(), remove);
    if (p == parent.v.end() || Holds(parent.v, keep)) continue;
    *p = keep;
    std::rotate(parent.v.begin(), p, parent.v.end());
    REQUIRE(expected_children.emplace(parent.v, parent.facet).second);
  }
  for (const auto& child : children) {
    CHECK(child.v.front() == keep);
    REQUIRE(received_children.emplace(child.v, child.facet).second);
    const auto a = mesh.Points()[SlotOf(mesh, child.v[0])].node.p;
    const auto b = mesh.Points()[SlotOf(mesh, child.v[1])].node.p;
    const auto c = mesh.Points()[SlotOf(mesh, child.v[2])].node.p;
    const auto cap = Orientation(a, b, c, removed);
    CHECK(cap >= 0);
    current_measure += cap;
  }
  REQUIRE_FALSE(expected_children.empty());
  CHECK(received_children == expected_children);
  CHECK(std::abs(current_measure - previous_measure) <= 1e-10L * previous_measure);
  REQUIRE_NOTHROW(mesh.Check());
  return result;
}

double DistanceToSaddleChildren(Point p, Point k, Point a, Point b, Point c) {
  const auto x = Vector(p);
  auto distance = [&](Point u, Point v) {
    double s, t;
    const auto q = SU2NativeCurved::detail::ClosestOnTriangle(x, Vector(k), Vector(u), Vector(v), s, t);
    const auto d = SU2NativeCurved::Sub<3>(x, q);
    return std::sqrt(SU2NativeCurved::Dot<3>(d, d));
  };
  return std::min(distance(a, b), distance(b, c));
}
}  // namespace

TEST_CASE("An S33 sheet midpoint collapses onto the corner and frees its slot", "[.][NativeFP2C2]") {
  const auto rows = S33();
  local::StateSurfaceGeometry geometry(S33Reference(rows), AtlasBudget());
  local::Mesh mesh(rows);
  local::IdSequence points(LastS33Point);
  constexpr Edge diagonal{GridId(0, 0, 0), GridId(1, 1, 0)};
  const auto cells = OrderedCells(mesh);
  const auto faces = OrientedSurface(mesh);
  const Id midpoint = C2Split(mesh, diagonal, geometry, points);
  const auto slot = SlotOf(mesh, midpoint);
  REQUIRE(CollapseWithProof(mesh, midpoint, diagonal[0], geometry) == C2Refusal::None);
  CHECK(OrderedCells(mesh) == cells);
  CHECK(OrientedSurface(mesh) == faces);
  CHECK(geometry.Find(midpoint) == nullptr);
  CHECK(geometry.Splits() == 1);
  CHECK(geometry.Removals() == 1);
  const Id next = C2Split(mesh, diagonal, geometry, points);
  CHECK(next != midpoint);
  CHECK(SlotOf(mesh, next) == slot);
  REQUIRE_NOTHROW(mesh.Check());
}

TEST_CASE("Step C refuses a step L or D reversal that violates its size or reverse guard", "[.][NativeFP2C2]") {
  SECTION("step L") {
    const auto rows = Box(2.25);
    local::Mesh mesh(rows);
    OneSplitGeometry<FlatGeometry> split_geometry(rows, BoxDiagonal);
    local::IdSequence points(7);
    local::PassStats stats;
    local::SurfaceFixtureAccess::LengthStep(mesh, split_geometry, points, stats);
    REQUIRE(stats.surface.splits_length == 1);
    REQUIRE(split_geometry.created.size() == 1);
    const Id midpoint = split_geometry.created.begin()->first;
    CollapseFixtureGeometry geometry(rows, BoxDiagonal[0], .02);
    const auto snapshot = Snapshot(mesh), frozen = FrozenSnapshot(mesh);
    CHECK(C2Collapse(mesh, {midpoint, BoxDiagonal[0]}, geometry) == C2Refusal::Size);
    CHECK(Snapshot(mesh) == snapshot);
    CHECK(FrozenSnapshot(mesh) == frozen);
    CHECK(geometry.removed.empty());
  }
  SECTION("step D") {
    const auto rows = Cylinder();
    constexpr Edge side_diagonal{1, 11};
    local::Mesh mesh(rows);
    OneSplitGeometry<CylinderGeometry> split_geometry(rows, side_diagonal);
    local::IdSequence points(17);
    local::PassStats stats;
    local::SurfaceFixtureAccess::DistanceWork(mesh, split_geometry, points, stats);
    REQUIRE(stats.surface.splits_distance == 1);
    REQUIRE(split_geometry.created.size() == 1);
    const Id midpoint = split_geometry.created.begin()->first;
    CollapseFixtureGeometry geometry(rows, side_diagonal[0], .1);
    CHECK(1 - std::cos(std::acos(-1.) / 8) > geometry.Limit());
    const auto snapshot = Snapshot(mesh), frozen = FrozenSnapshot(mesh);
    CHECK(C2Collapse(mesh, {midpoint, side_diagonal[0]}, geometry) == C2Refusal::Reverse);
    CHECK(Snapshot(mesh) == snapshot);
    CHECK(FrozenSnapshot(mesh) == frozen);
    CHECK(geometry.removed.empty());
  }
}

TEST_CASE("Step C protects feature points and preserves box traces", "[.][NativeFP2C2]") {
  SECTION("TwoCubes feature and corner points") {
    const auto rows = TwoCubes();
    local::StateSurfaceGeometry geometry(EntityReference(rows), AtlasBudget());
    local::Mesh mesh(rows);
    CHECK(geometry.Dimension(1) == 1);
    CHECK(geometry.Dimension(0) == 0);
    CHECK(geometry.Dimension(7) == 1);
    CHECK(C2Collapse(mesh, {1, 0}, geometry) == C2Refusal::Protected);
    CHECK(C2Collapse(mesh, {1, 7}, geometry) == C2Refusal::Protected);
  }
  SECTION("S33 sheet point onto a feature") {
    const auto rows = S33();
    const auto reference = S33Reference(rows);
    local::StateSurfaceGeometry geometry(reference, AtlasBudget());
    local::Mesh mesh(rows);
    local::IdSequence points(LastS33Point);
    constexpr Edge side{GridId(1, 0, 0), GridId(1, 1, 0)};
    CHECK(geometry.Dimension(side[0]) == 1);
    const Id midpoint = C2Split(mesh, side, geometry, points);
    const auto& old_coverage = reference->Parameters()->Coverage();
    REQUIRE(CollapseWithProof(mesh, midpoint, side[0], geometry) == C2Refusal::None);
    SU2NativeGeometry::BoundaryCoverage coverage;
    for (const auto& entry : old_coverage.traces) {
      const auto* trace = geometry.FindTrace(entry.first);
      REQUIRE(trace);
      coverage.traces.emplace(entry.first, *trace);
    }
    SU2NativeGeometry::BoundaryCoverage expected;
    expected.traces = old_coverage.traces;
    CHECK(EncodeBoundaryAtlas(coverage, {}) == EncodeBoundaryAtlas(expected, {}));
  }
}

TEST_CASE("Step C requires inward children and screens the reverse gap", "[.][NativeFP2C2]") {
  constexpr Id remove = GridId(1, 1, 0), keep = GridId(2, 1, 0);
  local::CoreOptions options;
  options.shape_test = false;
  const auto check_height = [&](double height, C2Refusal expected) {
    CAPTURE(height);
    const auto rows = S33(height);
    CollapseFixtureGeometry geometry(rows, keep, .02);
    local::Mesh mesh(rows, false, options);
    const auto snapshot = Snapshot(mesh), frozen = FrozenSnapshot(mesh);
    REQUIRE(CollapseWithProof(mesh, remove, keep, geometry) == expected);
    if (expected == C2Refusal::None) {
      CHECK(geometry.removed == std::set<Id>{remove});
    } else {
      CHECK(Snapshot(mesh) == snapshot);
      CHECK(FrozenSnapshot(mesh) == frozen);
      CHECK(geometry.removed.empty());
    }
  };
  check_height(.005, C2Refusal::Outward);
  check_height(-.01, C2Refusal::None);
  check_height(-.02, C2Refusal::Reverse);
}

TEST_CASE("A saddle catches the crossing reverse witness beyond the simple witnesses", "[.][NativeFP2C2]") {
  constexpr double h = 3.5e-4;
  constexpr Point k{0, 0, 0}, a{1, 0, h}, b{1, 1, 0}, c{0, 1, h}, p{.2, .02, .2 * h}, q{.5, .5, -1};
  const auto rows = BoundMesh({k, a, b, c, p, q}, {{4, 0, 1, 5}, {4, 1, 2, 5}, {4, 2, 3, 5}, {4, 3, 0, 5}}, .25);
  CollapseFixtureGeometry geometry(rows, 0, 1e-4);
  CHECK(DistanceToSaddleChildren(p, k, a, b, c) <= geometry.Limit());
  for (const auto& spoke : {k, a, b, c})
    CHECK(DistanceToSaddleChildren(Midpoint(p, spoke), k, a, b, c) <= geometry.Limit());
  const std::array<Point, 4> rim{k, a, b, c};
  for (size_t i = 0; i < rim.size(); ++i) {
    const auto u = rim[i], v = rim[(i + 1) % rim.size()];
    const Point center{(p.x + u.x + v.x) / 3, (p.y + u.y + v.y) / 3, (p.z + u.z + v.z) / 3};
    CHECK(DistanceToSaddleChildren(center, k, a, b, c) <= geometry.Limit());
  }
  // The 3-D closest point on (p,c) to the line (k,b) differs from its planar intersection by O(h²).
  const auto u = SU2NativeCurved::Sub<3>(Vector(c), Vector(p));
  const auto v = SU2NativeCurved::Sub<3>(Vector(b), Vector(k));
  const auto w = SU2NativeCurved::Sub<3>(Vector(p), Vector(k));
  using SU2NativeCurved::Dot;
  const auto uu = Dot<3>(u, u), vv = Dot<3>(v, v), uv = Dot<3>(u, v), uw = Dot<3>(u, w), vw = Dot<3>(v, w);
  const double t = (uv * vw - vv * uw) / (uu * vv - uv * uv);
  const double s = (uu * vw - uv * uw) / (uu * vv - uv * uv);
  REQUIRE(t > 0);
  REQUIRE(t < 1);
  REQUIRE(s > 0);
  REQUIRE(s < 1);
  const Point crossing{p.x + t * u[0], p.y + t * u[1], p.z + t * u[2]};
  const double distance = DistanceToSaddleChildren(crossing, k, a, b, c);
  CHECK(distance == Approx(1.13e-4).margin(1e-6));
  CHECK(distance > geometry.Limit());
  local::Mesh mesh(rows);
  const auto snapshot = Snapshot(mesh), frozen = FrozenSnapshot(mesh);
  CHECK(C2Collapse(mesh, {4, 0}, geometry) == C2Refusal::Reverse);
  CHECK(Snapshot(mesh) == snapshot);
  CHECK(FrozenSnapshot(mesh) == frozen);
  CHECK(geometry.removed.empty());
}

TEST_CASE("Step C work-list and full scans produce the same S33 cells and faces", "[.][NativeFP2C2]") {
  const auto rows = S33();
  const auto reference = S33Reference(rows);
  const auto run = [&](bool work_list) {
    local::CoreOptions options;
    options.work_list = work_list;
    options.surface_collapses = true;
    local::Mesh mesh(rows, false, options);
    local::StateSurfaceGeometry geometry(reference, AtlasBudget());
    local::IdSequence points(LastS33Point);
    C2Split(mesh, {GridId(0, 0, 0), GridId(1, 1, 0)}, geometry, points);
    C2Split(mesh, {GridId(2, 2, 0), GridId(3, 3, 0)}, geometry, points);
    local::PassStats stats;
    local::SurfaceFixtureAccess::SurfaceCollapseWork(mesh, geometry, stats);
    CHECK(geometry.Removals() > 0);
    REQUIRE_NOTHROW(mesh.Check());
    return std::make_pair(OrderedCells(mesh), OrientedSurface(mesh));
  };
  CHECK(run(true) == run(false));
}

namespace {
template <class Function>
void C2OnRank(World& world, int owner, const Function& action) {
  bool ok = true;
  std::string reason;
  if (world.rank == owner) {
    try {
      action();
    } catch (const Catch::TestFailureException&) {
      ok = false;
    } catch (const std::exception& error) {
      ok = false;
      reason = error.what();
    }
  }
  INFO(reason);
  REQUIRE(world.maximum(!ok) == 0);
}

Owned C2Partition(const Owned& initial, int rank, bool along_x) {
  Owned owned;
  for (const auto& entry : initial) {
    const auto center = Centroid(entry.second.cell);
    const int owner = (along_x ? center.x < 1 : center.y < 2) ? 0 : 1;
    if (owner == rank) owned.emplace(entry);
  }
  return owned;
}

std::vector<Cell> C2RemoteCells(World& world, const Owned& owned, int owner) {
  const int other = 1 - owner;
  auto encoded = world.rank == other ? OwnedBytes(owned) : std::vector<char>{};
  CPassiveComm::BcastRounds(encoded, other, world.comm);
  RecordStream stream(static_cast<const std::vector<char>&>(encoded));
  std::vector<Cell> cells;
  while (!stream.End()) {
    OwnedCell record;
    stream(record);
    cells.push_back(record.cell);
  }
  return cells;
}

void C2SplitOnRank(World& world, int owner, Owned& working, const local::BorderMeeting& meeting,
                   local::StateSurfaceGeometry& geometry, Edge edge, Id expected, local::IdSequence& points,
                   local::IdSequence& cells) {
  C2OnRank(world, owner, [&] {
    local::Mesh mesh(working, true);
    mesh.MarkBorderSurface(meeting);
    REQUIRE(C2Split(mesh, edge, geometry, points) == expected);
    REQUIRE_NOTHROW(mesh.Check());
    mesh.WriteBack(working, cells);
  });
}

std::shared_ptr<const BoundaryParameterState> C2CheckState(World& world, const Owned& working,
                                                           const local::StateSurfaceGeometry& geometry,
                                                           local::MemoryBudget& memory, size_t splits,
                                                           size_t removals) {
  const auto caller = transfer_memory::Add(geometry.State().ResidentBytes(), memory.OwnedBytes(working.size()));
  const auto rebuilt = local::BuildSurfaceState(world, working, geometry, memory, caller);
  REQUIRE(world.maximum(rebuilt.state == nullptr) == 0);
  CHECK(rebuilt.splits == splits);
  CHECK(rebuilt.removals == removals);
  CHECK(rebuilt.state->Revision() == geometry.State().Revision() + 1);
  const auto snapshot = rebuilt.state->Snapshot();
  auto first = world.rank == 0 ? snapshot : std::vector<char>{};
  CPassiveComm::BcastRounds(first, 0, world.comm);
  REQUIRE_FALSE(world.maximum(snapshot != first));
  return rebuilt.state;
}
}  // namespace

TEST_CASE("Step C protects a rank-border point and keeper without mutating either state",
          "[.][NativeFP2C2][NativeFP2C2MPI]") {
  World world;
  REQUIRE(world.size == 2);
  const auto initial = S33();
  local::StateSurfaceGeometry geometry(S33Reference(initial), AtlasBudget());
  auto working = C2Partition(initial, world.rank, true);
  auto memory = ExchangeBudget();
  const auto meeting = local::MeetBorderPoints(world, working, memory);
  C2OnRank(world, 0, [&] {
    constexpr Edge diagonal{GridId(0, 0, 0), GridId(1, 1, 0)};
    local::Mesh mesh(working, true);
    mesh.MarkBorderSurface(meeting);
    REQUIRE(mesh.Points()[SlotOf(mesh, diagonal[1])].border);
    REQUIRE(geometry.Dimension(diagonal[1]) == 2);
    CHECK(C2Collapse(mesh, diagonal, geometry) == C2Refusal::Border);
    local::IdSequence points(LastS33Point, world.rank, world.size);
    const auto midpoint = C2Split(mesh, diagonal, geometry, points);
    const auto snapshot = Snapshot(mesh), frozen = FrozenSnapshot(mesh), overlay = OverlaySnapshot(mesh, geometry);
    CHECK(C2Collapse(mesh, {midpoint, diagonal[1]}, geometry) == C2Refusal::BorderKeeper);
    CHECK(Snapshot(mesh) == snapshot);
    CHECK(FrozenSnapshot(mesh) == frozen);
    CHECK(OverlaySnapshot(mesh, geometry) == overlay);
    REQUIRE_NOTHROW(mesh.Check());
  });
}

TEST_CASE("Step C exchanges cross-pass and local cancellations and builds with removals only",
          "[.][NativeFP2C2][NativeFP2C2MPI]") {
  World world;
  REQUIRE(world.size == 2);
  const auto initial = S33();
  local::StateSurfaceGeometry geometry(S33Reference(initial), AtlasBudget());
  auto memory = ExchangeBudget();
  local::IdSequence points(LastS33Point, world.rank, world.size);
  local::IdSequence cells(initial.rbegin()->first, world.rank, world.size);
  SECTION("A split created by rank 1 is removed by rank 0 in the next pass") {
    auto working = C2Partition(initial, world.rank, true);
    const auto first = local::MeetBorderPoints(world, working, memory);
    constexpr Edge diagonal{GridId(1, 0, 0), GridId(2, 1, 0)};
    constexpr Id midpoint = LastS33Point + 2;
    C2SplitOnRank(world, 1, working, first, geometry, diagonal, midpoint, points, cells);
    CHECK((geometry.Find(midpoint) != nullptr) == (world.rank == 1));
    geometry.Exchange(world, memory, memory.OwnedBytes(working.size()));
    REQUIRE(world.maximum(geometry.Find(midpoint) == nullptr) == 0);
    local::ShiftBorders(world, working, first, local::Direction::Lower, memory);
    const auto second = local::MeetBorderPoints(world, working, memory);
    const auto remote = C2RemoteCells(world, working, 0);
    C2OnRank(world, 0, [&] {
      local::Mesh mesh(working, true);
      mesh.MarkBorderSurface(second);
      REQUIRE_FALSE(mesh.Points()[SlotOf(mesh, midpoint)].border);
      REQUIRE_FALSE(mesh.Points()[SlotOf(mesh, diagonal[0])].border);
      REQUIRE(CollapseWithProof(mesh, midpoint, diagonal[0], geometry, remote) == C2Refusal::None);
      mesh.WriteBack(working, cells);
    });
    CHECK((geometry.Find(midpoint) == nullptr) == (world.rank == 0));
    geometry.Exchange(world, memory, memory.OwnedBytes(working.size()));
    REQUIRE(world.maximum(geometry.Find(midpoint) != nullptr) == 0);
    const auto state = C2CheckState(world, working, geometry, memory, 1, 1);
    CHECK(state->Records().count(midpoint) == 0);
    CHECK(state->Surface().size() == geometry.State().Surface().size());
  }
  SECTION("A same-pass cancellation does not block a fresh placement on another rank") {
    auto working = C2Partition(initial, world.rank, true);
    constexpr Edge diagonal{GridId(0, 0, 0), GridId(1, 1, 0)};
    constexpr Id cancelled = LastS33Point + 1, later = LastS33Point + 2;
    const auto first = local::MeetBorderPoints(world, working, memory);
    const auto remote = C2RemoteCells(world, working, 0);
    C2OnRank(world, 0, [&] {
      local::Mesh mesh(working, true);
      mesh.MarkBorderSurface(first);
      REQUIRE(C2Split(mesh, diagonal, geometry, points) == cancelled);
      REQUIRE(CollapseWithProof(mesh, cancelled, diagonal[0], geometry, remote) == C2Refusal::None);
      mesh.WriteBack(working, cells);
    });
    geometry.Exchange(world, memory, memory.OwnedBytes(working.size()));
    REQUIRE(world.maximum(geometry.Find(cancelled) != nullptr) == 0);
    local::ShiftBorders(world, working, first, local::Direction::Higher, memory);
    const auto second = local::MeetBorderPoints(world, working, memory);
    C2SplitOnRank(world, 1, working, second, geometry, diagonal, later, points, cells);
    geometry.Exchange(world, memory, memory.OwnedBytes(working.size()));
    REQUIRE(world.maximum(geometry.Find(later) == nullptr) == 0);
    CHECK(Bytes(geometry.Find(later)->node.p) == Bytes(Point{.5, .5, 0}));
    const auto state = C2CheckState(world, working, geometry, memory, 2, 1);
    CHECK(state->Records().count(cancelled) == 0);
    CHECK(state->Records().count(later) == 1);
  }
  SECTION("An original sheet point is removed without a split") {
    auto working = C2Partition(initial, world.rank, false);
    const auto meeting = local::MeetBorderPoints(world, working, memory);
    constexpr Id remove = GridId(1, 1, 0), keep = GridId(2, 1, 0);
    const auto remote = C2RemoteCells(world, working, 0);
    C2OnRank(world, 0, [&] {
      local::CoreOptions options;
      local::Mesh mesh(working, true, options);
      mesh.MarkBorderSurface(meeting);
      const auto result = CollapseWithProof(mesh, remove, keep, geometry, remote);
      if (result == C2Refusal::Shape) {
        std::cout << "FP2_C2 removals_only_shape=refused retry_shape_test=0\n";
        options.shape_test = false;
        local::Mesh retry(working, true, options);
        retry.MarkBorderSurface(meeting);
        REQUIRE(CollapseWithProof(retry, remove, keep, geometry, remote) == C2Refusal::None);
        retry.WriteBack(working, cells);
      } else {
        REQUIRE(result == C2Refusal::None);
        mesh.WriteBack(working, cells);
      }
    });
    geometry.Exchange(world, memory, memory.OwnedBytes(working.size()));
    REQUIRE(world.maximum(geometry.Find(remove) != nullptr) == 0);
    const auto state = C2CheckState(world, working, geometry, memory, 0, 1);
    CHECK(state->Records().count(remove) == 0);
    CHECK(state->Records().count(keep) == 1);
  }
}
