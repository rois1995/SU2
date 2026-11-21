/*!
 * \file CNativeLocalCore3DSurface_tests.cpp
 * \brief Local surface ring replacement, placement guards and length/distance sweeps.
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md).
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */
#include "catch.hpp"
#include "../../../Common/include/adaptation/CNativeLocalSurface3D.hpp"
#include "../../../Common/include/adaptation/CNativeRuntimeProfile3D.hpp"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <tuple>

namespace SU2NativeLocal3D {
struct SurfaceFixtureAccess {
  static Refusal PrepareSplit(Mesh mesh, Edge edge, Slot seed, SurfaceGeometry& geometry, bool both) {
    const auto ring = mesh.EdgeRing(edge, seed);
    const auto parents = mesh.SurfaceParents(edge, ring);
    REQUIRE(parents.size() == 2);
    const std::array<Node, 2> ends{mesh.points_[edge[0]].node, mesh.points_[edge[1]].node};
    const std::array<SU2Native3D::SurfaceFace, 2> faces{mesh.SurfaceFaceAt(parents[0]), mesh.SurfaceFaceAt(parents[1])};
    const auto placement = geometry.Place(ends, faces, 20);
    REQUIRE(placement.refusal == SurfaceRefusal::None);
    const auto middle = static_cast<Slot>(mesh.points_.size());
    mesh.points_.push_back({placement.point.node, mesh.points_[edge[0]].tensor});
    mesh.points_.back().surface = true;
    std::vector<Vertices> fresh;
    for (const auto cell : ring.cells) {
      for (const auto end : edge) {
        auto child = mesh.cells_[cell].vertices;
        *std::find(child.begin(), child.end(), end) = middle;
        fresh.push_back(child);
      }
    }
    std::vector<Mesh::ReplacedFace> replaced;
    for (size_t i = 0; i < (both ? 2U : 1U); ++i)
      replaced.push_back({parents[i].cell, parents[i].face, edge[0], edge[1], middle});
    const auto plan = mesh.Prepare(ring.cells, fresh, replaced);
    if (plan.Accepted()) CHECK(mesh.ExactlyPositive(plan));
    return plan.refusal;
  }
};
}  // namespace SU2NativeLocal3D

namespace {
using namespace SU2Native3D;
namespace local = SU2NativeLocal3D;
using Tet = std::array<size_t, 4>;

constexpr Edge BoxDiagonal{0, 2};

bool Holds(const Face& face, Id id) { return std::find(face.begin(), face.end(), id) != face.end(); }

Face Key(Face face) {
  std::sort(face.begin(), face.end());
  return face;
}

Edge EdgeKey(const std::array<Node, 2>& nodes) {
  return {std::min(nodes[0].id, nodes[1].id), std::max(nodes[0].id, nodes[1].id)};
}

Point Midpoint(Point a, Point b) { return {.5 * (a.x + b.x), .5 * (a.y + b.y), .5 * (a.z + b.z)}; }

Face FaceAt(const Cell& cell, unsigned face) {
  constexpr unsigned order[4][3]{{1, 2, 3}, {0, 3, 2}, {0, 1, 3}, {0, 2, 1}};
  return {cell.v[order[face][0]].id, cell.v[order[face][1]].id, cell.v[order[face][2]].id};
}

Owned BoundMesh(const std::vector<Point>& points, const std::vector<Tet>& tets, double metric) {
  Owned owned;
  std::map<Face, std::vector<std::pair<Id, unsigned>>> uses;
  for (const auto& tet : tets) {
    OwnedCell row;
    row.cell.id = owned.size() + 1;
    row.sensor_present = true;
    row.sensor.fill({metric, 0, 0, metric, 0, metric});
    auto& nodes = row.cell.v;
    for (unsigned k = 0; k < 4; ++k) nodes[k] = {tet[k], points.at(tet[k])};
    if (Orientation(nodes[0].p, nodes[1].p, nodes[2].p, nodes[3].p) < 0) std::swap(nodes[0], nodes[1]);
    REQUIRE(Orientation(nodes[0].p, nodes[1].p, nodes[2].p, nodes[3].p) > 0);
    for (unsigned f = 0; f < 4; ++f) uses[Key(FaceAt(row.cell, f))].emplace_back(row.cell.id, f);
    owned.emplace(row.cell.id, row);
  }
  Id facet = 1;
  for (const auto& entry : uses) {
    REQUIRE(entry.second.size() <= 2);
    if (entry.second.size() != 1) continue;
    const auto [id, face] = entry.second.front();
    owned.at(id).physical[face] = {true, facet++};
  }
  return owned;
}

Owned Box(double metric = 1) {
  return BoundMesh({{0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0}, {0, 0, 1}, {1, 0, 1}, {1, 1, 1}, {0, 1, 1}},
                   {{0, 6, 1, 2}, {0, 6, 2, 3}, {0, 6, 3, 7}, {0, 6, 7, 4}, {0, 6, 4, 5}, {0, 6, 5, 1}}, metric);
}

constexpr double CylinderHeight = 1.25;

Owned Cylinder() {
  constexpr size_t sides = 8, top = sides + 1;
  const double angle = 2 * std::acos(-1.) / sides;
  std::vector<Point> points;
  for (const double z : {0., CylinderHeight}) {
    points.push_back({0, 0, z});
    for (size_t i = 0; i < sides; ++i) points.push_back({std::cos(angle * i), std::sin(angle * i), z});
  }
  std::vector<Tet> tets;
  for (size_t i = 1; i <= sides; ++i) {
    const auto next = i % sides + 1;
    const auto a = std::min(i, next), b = std::max(i, next);
    tets.push_back({0, a, b, b + top});
    tets.push_back({0, a, a + top, b + top});
    tets.push_back({0, top, a + top, b + top});
  }
  return BoundMesh(points, tets, 2.25);
}

std::vector<SurfaceFace> Surface(const Owned& owned) {
  std::vector<SurfaceFace> result;
  for (const auto& entry : owned) {
    const auto& row = entry.second;
    for (unsigned f = 0; f < 4; ++f)
      if (row.physical[f].present) result.push_back({FaceAt(row.cell, f), row.physical[f].facet});
  }
  return result;
}

std::vector<SurfaceFace> Surface(const local::Mesh& mesh) {
  std::vector<SurfaceFace> result;
  for (local::Slot cell = 0; cell < mesh.Cells().size(); ++cell) {
    const auto& row = mesh.Cells()[cell];
    if (!row.live) continue;
    Cell nodes;
    for (unsigned i = 0; i < 4; ++i) nodes.v[i] = mesh.Points()[row.vertices[i]].node;
    for (unsigned f = 0; f < 4; ++f)
      if (row.neighbors[f] == local::Physical) result.push_back({FaceAt(nodes, f), mesh.Bindings(cell)[f].facet});
  }
  return result;
}

local::Slot SlotOf(const local::Mesh& mesh, Id id) {
  const auto& points = mesh.Points();
  const auto found = std::find_if(points.begin(), points.end(), [&](const auto& p) { return !p.removed && p.node.id == id; });
  REQUIRE(found != points.end());
  return found - points.begin();
}

template <class... T>
std::vector<char> Bytes(T... values) {
  std::vector<char> bytes;
  RecordStream stream(bytes);
  stream(values...);
  return bytes;
}

std::vector<char> Snapshot(const local::Mesh& mesh) {
  std::vector<char> bytes;
  RecordStream stream(bytes);
  auto points = mesh.Points().size(), cells = mesh.Cells().size();
  stream(points, cells);
  for (auto p : mesh.Points())
    stream(p.node, p.tensor, p.incident, p.surface, p.border, p.removed, p.wall_near, p.unresolved, p.changed_at);
  for (local::Slot i = 0; i < cells; ++i) {
    auto row = mesh.Cells()[i];
    auto bindings = mesh.Bindings(i);
    stream(row.vertices, row.neighbors, row.id, row.version, row.binding, row.stamp, row.live, row.changed,
           row.failed_at, bindings);
  }
  return bytes;
}

std::vector<char> OwnedBytes(const Owned& owned) {
  std::vector<char> bytes;
  RecordStream stream(bytes);
  for (auto entry : owned) stream(entry.second);
  return bytes;
}

class FlatGeometry : public local::SurfaceGeometry {
 public:
  FlatGeometry(const Owned& owned, double offset = 0, bool accept_entity = true)
      : offset_(offset), accept_entity_(accept_entity) {
    for (const auto& entry : owned)
      for (auto node : entry.second.cell.v) nodes_.emplace(node.id, node.p);
  }

  double Tolerance() const override { return .02; }
  bool Feature(const Edge&) const override { return false; }
  local::SurfaceSamples Samples(const SurfaceFace&) override { return {}; }
  std::optional<double> Chord(const std::array<Node, 2>&) override { return std::nullopt; }
  local::SurfacePlacement Place(const std::array<Node, 2>& nodes, const std::array<SurfaceFace, 2>& parents,
                                Id id) override {
    auto point = Midpoint(nodes[0].p, nodes[1].p);
    if (EdgeKey(nodes) == BoxDiagonal) point.z += offset_;
    return Placement(point, id, parents[0].facet);
  }
  bool OnEntity(const SurfaceFace&, const BoundaryPoint&) override { return accept_entity_; }
  void Accept(local::SurfacePlacement&& placement) override {
    const auto node = placement.point.node;
    REQUIRE(nodes_.emplace(node.id, node.p).second);
    REQUIRE(created.emplace(node.id, node).second);
  }

  std::map<Id, Node> created;

 protected:
  static local::SurfacePlacement Placement(Point point, Id id, Id facet) {
    local::SurfacePlacement result;
    result.point.node = {id, point};
    result.point.binding.feature = facet;
    result.refusal = local::SurfaceRefusal::None;
    return result;
  }
  Point At(Id id) const { return nodes_.at(id); }

 private:
  double offset_;
  bool accept_entity_;
  std::map<Id, Point> nodes_;
};

class CylinderGeometry : public FlatGeometry {
 public:
  explicit CylinderGeometry(const Owned& owned) : FlatGeometry(owned) {
    for (const auto& face : Surface(owned)) {
      const auto p = At(face.v[0]), q = At(face.v[1]), r = At(face.v[2]);
      const int cap = p.z == q.z && p.z == r.z ? (p.z == 0 ? 0 : 1) : -1;
      caps_.emplace(face.facet, cap);
    }
  }

  double Tolerance() const override { return .1; }
  bool Feature(const Edge& edge) const override {
    const auto a = At(edge[0]), b = At(edge[1]);
    return a.z == b.z && (a.z == 0 || a.z == CylinderHeight) && OnCylinder(a) && OnCylinder(b);
  }
  local::SurfaceSamples Samples(const SurfaceFace& face) override {
    if (caps_.at(face.facet) >= 0) return {};
    const auto a = At(face.v[0]), b = At(face.v[1]), c = At(face.v[2]);
    const Point center{(a.x + b.x + c.x) / 3, (a.y + b.y + c.y) / 3, (a.z + b.z + c.z) / 3};
    return {{Error(center), Error(Midpoint(a, b)), Error(Midpoint(b, c)), Error(Midpoint(c, a))}, 0};
  }
  std::optional<double> Chord(const std::array<Node, 2>& edge) override {
    return Error(Midpoint(edge[0].p, edge[1].p));
  }
  local::SurfacePlacement Place(const std::array<Node, 2>& edge, const std::array<SurfaceFace, 2>& parents,
                                Id id) override {
    const auto key = EdgeKey(edge);
    if (key == Edge{1, 2}) {
      // Leave one measurable feature error so the final reason accounting has a nonzero case.
      local::SurfacePlacement result;
      result.refusal = local::SurfaceRefusal::Feature;
      return result;
    }
    auto point = Midpoint(edge[0].p, edge[1].p);
    if (caps_.at(parents[0].facet) < 0 || caps_.at(parents[1].facet) < 0) {
      const auto radius = Radius(point);
      point.x /= radius;
      point.y /= radius;
    }
    auto result = Placement(point, id, parents[0].facet);
    if (Feature(key)) result.parent = key;
    return result;
  }
  bool OnEntity(const SurfaceFace& face, const BoundaryPoint& middle) override {
    const auto cap = caps_.at(face.facet);
    return std::all_of(face.v.begin(), face.v.end(), [&](Id id) {
      const auto p = id == middle.node.id ? middle.node.p : At(id);
      return cap < 0 ? OnCylinder(p) : p.z == (cap == 0 ? 0 : CylinderHeight);
    });
  }

 private:
  static double Radius(Point point) { return std::hypot(point.x, point.y); }
  static double Error(Point point) { return std::abs(1 - Radius(point)); }
  static bool OnCylinder(Point point) { return Error(point) < 1e-12; }
  std::map<Id, int> caps_;
};

local::Edge BottomEdge(const local::Mesh& mesh) {
  return {SlotOf(mesh, BoxDiagonal[0]), SlotOf(mesh, BoxDiagonal[1])};
}

void CheckChildBindings(const Owned& initial, const local::Mesh& split, Id midpoint) {
  std::map<Face, Id> expected, observed;
  for (const auto& parent : Surface(initial)) {
    if (!Holds(parent.v, BoxDiagonal[0]) || !Holds(parent.v, BoxDiagonal[1])) continue;
    for (const auto end : BoxDiagonal) {
      auto child = parent.v;
      *std::find(child.begin(), child.end(), end) = midpoint;
      expected.emplace(Key(child), parent.facet);
    }
  }
  for (const auto& child : Surface(split))
    if (Holds(child.v, midpoint)) REQUIRE(observed.emplace(Key(child.v), child.facet).second);
  REQUIRE(expected.size() == 4);
  CHECK(observed == expected);
}

struct PassResult {
  std::vector<char> bytes;
  local::PassStats stats;
  size_t left_distance = 0;
};

PassResult Run(Owned owned, FlatGeometry& geometry, bool work_list) {
  local::CoreOptions options;
  options.surface_splits = true;
  options.work_list = work_list;
  local::Mesh mesh(owned, false, options);
  Id last_id = 0;
  for (const auto& point : mesh.Points()) last_id = std::max(last_id, point.node.id);
  local::IdSequence points(last_id), cells(owned.rbegin()->first);
  auto stats = mesh.RunPass(points, SIZE_MAX, true, options, &geometry);
  REQUIRE_NOTHROW(mesh.Check());
  for (const auto& [id, node] : geometry.created) {
    const auto& actual = mesh.Points()[SlotOf(mesh, id)];
    CHECK(actual.surface);
    CHECK(Bytes(actual.node) == Bytes(node));
  }
  size_t left = 0;
  for (const auto& face : Surface(mesh)) {
    const auto samples = geometry.Samples(face).distances;
    left += *std::max_element(samples.begin(), samples.end()) > geometry.Limit();
  }
  mesh.WriteBack(owned, cells);
  return {OwnedBytes(owned), std::move(stats), left};
}

auto Counts(const local::SurfaceCounters& row) {
  return std::tie(row.faces_in, row.faces_out, row.points_in, row.points_out, row.splits_length, row.splits_distance,
                  row.splits_feature, row.sweeps_length, row.sweeps_distance, row.left_long, row.left_border,
                  row.left_distance, row.left_all_feature, row.left_refused, row.left_limit, row.feature_dominated,
                  row.refused_walk, row.refused_protected, row.refused_feature, row.refused_trace, row.refused_duplicate,
                  row.refused_outward, row.refused_size, row.refused_valid, row.refused_topology, row.refused_memory,
                  row.refused_entity, row.chord_unknown, row.samples, row.samples_unconverged);
}
}  // namespace

TEST_CASE("Surface Prepare matches the children of both old physical parents", "[.][NativeLocalCoreSurfaceA]") {
  const auto rows = Box();
  const local::Mesh mesh(rows);
  FlatGeometry flat(rows);
  const auto edge = BottomEdge(mesh);
  CHECK(local::SurfaceFixtureAccess::PrepareSplit(mesh, edge, 0, flat, true) == local::Refusal::None);
  CHECK(local::SurfaceFixtureAccess::PrepareSplit(mesh, edge, 0, flat, false) == local::Refusal::Topology);
}

TEST_CASE("Surface splits bind and freeze new points and refuse excess outward motion atomically",
          "[.][NativeLocalCoreSurfaceA]") {
  const auto rows = Box();
  struct PlacementCase {
    double offset;
    bool entity;
    local::SurfaceRefusal refusal;
  };
  using local::SurfaceRefusal;
  for (const PlacementCase trial : {PlacementCase{.03, true, SurfaceRefusal::None},
                                    PlacementCase{-.01, true, SurfaceRefusal::None},
                                    PlacementCase{-.03, true, SurfaceRefusal::Outward},
                                    PlacementCase{.01, false, SurfaceRefusal::Entity}}) {
    CAPTURE(trial.offset, trial.entity);
    local::Mesh mesh(rows);
    FlatGeometry geometry(rows, trial.offset, trial.entity);
    const auto before = Snapshot(mesh);
    local::IdSequence ids(7);
    local::SurfaceCounters stats;
    REQUIRE(mesh.SplitSurface(BottomEdge(mesh), 0, geometry, ids, 1, SIZE_MAX, stats, false) == trial.refusal);
    if (trial.refusal != SurfaceRefusal::None) {
      CHECK(Snapshot(mesh) == before);
      CHECK(geometry.created.empty());
      CHECK(ids.Peek() == 8);
      CHECK(stats.refused_outward == (trial.refusal == SurfaceRefusal::Outward ? 1 : 0));
      CHECK(stats.refused_entity == (trial.refusal == SurfaceRefusal::Entity ? 1 : 0));
    } else {
      REQUIRE(geometry.created.size() == 1);
      const auto& midpoint = mesh.Points()[SlotOf(mesh, 8)];
      CHECK(midpoint.surface);
      CHECK(midpoint.node.p.z == trial.offset);
      CHECK(stats.outward_max == Approx(std::max(0., -trial.offset)));
      CHECK(ids.Peek() == 9);
      CheckChildBindings(rows, mesh, 8);
    }
    CHECK_NOTHROW(mesh.Check());
  }

  const auto larger = Box(2);
  FlatGeometry listed(larger, -.01), scanned(larger, -.01);
  const auto list = Run(larger, listed, true), scan = Run(larger, scanned, false);
  REQUIRE(list.stats.surface.splits_length > 0);
  REQUIRE(listed.created.count(8) == 1);
  CHECK(listed.created.at(8).p.z == -.01);
  CHECK((list.bytes == scan.bytes));
  CHECK(list.stats.surface.splits_length == scan.stats.surface.splits_length);
  CHECK(list.stats.surface.splits_distance == scan.stats.surface.splits_distance);
  std::cout << "FP2A_FIXTURE surface_box splits_length=" << list.stats.surface.splits_length
            << " volume_splits=" << list.stats.splits << " cells=" << list.stats.cells_out << '\n';
}

TEST_CASE("Cylinder surface length and distance steps repeat and account for every residual face",
          "[.][NativeLocalCoreSurfaceA]") {
  const auto rows = Cylinder();
  CylinderGeometry first(rows), second(rows);
  const auto a = Run(rows, first, true), b = Run(rows, second, true);
  const auto& stats = a.stats.surface;
  REQUIRE(stats.splits_length > 0);
  REQUIRE(stats.splits_distance > 0);
  CHECK(stats.splits_feature > 0);
  CHECK(stats.sweeps_length <= 50);
  CHECK(stats.sweeps_distance <= 40);
  CHECK((Counts(stats) == Counts(b.stats.surface)));
  CHECK((a.bytes == b.bytes));
  REQUIRE(a.left_distance > 0);
  CHECK(a.left_distance == stats.left_distance);
  CHECK(stats.left_distance == stats.left_all_feature + stats.left_refused + stats.left_limit + stats.feature_dominated);
  CHECK(stats.left_refused > 0);
  CHECK(stats.left_border == 0);
  std::cout << "FP2A_FIXTURE surface_cylinder splits_length=" << stats.splits_length
            << " splits_distance=" << stats.splits_distance << " left_distance=" << stats.left_distance
            << " left_refused=" << stats.left_refused << '\n';
}

namespace {
constexpr Edge LowerCrease{1, 2}, UpperCrease{7, 8};
constexpr Id LastOriginalPoint = 11;
constexpr size_t ExchangeCeiling = size_t(1) << 28;

Owned TwoCubes() {
  constexpr std::array<size_t, 6> ring{1, 2, 3, 7, 4, 5};
  std::vector<Point> points;
  for (size_t z = 0; z < 2; ++z)
    for (size_t y = 0; y < 2; ++y)
      for (size_t x = 0; x < 3; ++x) points.push_back({double(x), double(y), double(z)});
  std::vector<Tet> tets;
  for (size_t x = 0; x < 2; ++x) {
    const std::array<size_t, 8> v{x, x + 1, x + 4, x + 3, x + 6, x + 7, x + 10, x + 9};
    for (size_t i = 0; i < ring.size(); ++i)
      tets.push_back({v[0], v[6], v[ring[i]], v[ring[(i + 1) % ring.size()]]});
  }
  return BoundMesh(points, tets, 1);
}

SU2NativeGeometry::AtlasBudget AtlasBudget() { return {size_t(1) << 29, ExchangeCeiling, 4096}; }

std::shared_ptr<const PlanarReference> EntityReference(const Owned& rows) {
  const auto surface = Surface(rows);
  std::map<Id, Node> nodes;
  for (const auto& entry : rows)
    for (const auto& node : entry.second.cell.v) nodes.emplace(node.id, node);
  std::vector<Facet> facets;
  std::map<Id, double> errors;
  for (const auto& face : surface) {
    facets.push_back({face.facet, 0, {nodes.at(face.v[0]), nodes.at(face.v[1]), nodes.at(face.v[2])}});
    errors.emplace(face.facet, 1e-12);
  }
  auto definition = std::make_shared<PlanarReference>(facets);
  auto mapped = std::make_shared<SU2NativeGeometry::MappedFacetReference>(definition, errors, 71, SIZE_MAX, false);
  auto budget = AtlasBudget();
  std::string reason;
  SU2NativeGeometry::MobilityPolicy policy;
  INFO(reason);
  REQUIRE(mapped->BuildMobilityPolicy(45, {{0, 2}}, policy, budget, reason));
  BoundaryPoints points;
  for (const auto& [id, node] : nodes) points.emplace(id, BoundaryPoint{node, mapped->Vertex(id)});
  const BoundaryParameterState initial(mapped, points, surface, 1, ExchangeCeiling, 0, {}, policy, &budget);
  const auto entity = initial.WithEntityGeometry(1e-4, budget, ExchangeCeiling);
  REQUIRE(entity->Coverage().traces.count(LowerCrease) == 1);
  REQUIRE(entity->Coverage().traces.count(UpperCrease) == 1);
  return PlanarReference::WithParameters(definition, entity);
}

std::array<SurfaceFace, 2> Parents(const Owned& rows, const Edge& edge) {
  std::vector<SurfaceFace> parents;
  for (const auto& face : Surface(rows))
    if (Holds(face.v, edge[0]) && Holds(face.v, edge[1])) parents.push_back(face);
  REQUIRE(parents.size() == 2);
  return {parents[0], parents[1]};
}

local::SurfacePlacement Propose(local::StateSurfaceGeometry& geometry, const Owned& initial, const Edge& edge, Id id) {
  const auto* a = geometry.Find(edge[0]);
  const auto* b = geometry.Find(edge[1]);
  REQUIRE((a && b));
  auto placement = geometry.Place({a->node, b->node}, Parents(initial, edge), id);
  INFO(placement.reason);
  REQUIRE(placement.Accepted());
  REQUIRE(placement.parent == edge);
  REQUIRE(placement.children.size() == 2);
  return placement;
}

void SplitOnRank(World& world, int owner, Owned& working, const local::BorderMeeting& meeting,
                 local::StateSurfaceGeometry& geometry, const Edge& edge, local::IdSequence& points,
                 local::IdSequence& cells) {
  bool ok = true;
  std::string reason;
  if (world.rank == owner) {
    try {
      local::Mesh mesh(working, true);
      mesh.MarkBorderSurface(meeting);
      const auto point = [&](Id id) {
        const auto& all = mesh.Points();
        const auto found = std::find_if(all.begin(), all.end(), [id](const auto& row) { return row.node.id == id; });
        if (found == all.end()) throw std::invalid_argument("Surface exchange fixture owner lacks a split endpoint.");
        return static_cast<local::Slot>(found - all.begin());
      };
      const local::Edge slots{point(edge[0]), point(edge[1])};
      const auto holds = [&slots](const local::CellRow& row) {
        return std::all_of(slots.begin(), slots.end(), [&](local::Slot slot) {
          return std::find(row.vertices.begin(), row.vertices.end(), slot) != row.vertices.end();
        });
      };
      const auto seed = std::find_if(mesh.Cells().begin(), mesh.Cells().end(), holds);
      if (seed == mesh.Cells().end()) throw std::invalid_argument("Surface exchange fixture owner lacks a child ring.");
      local::SurfaceCounters stats;
      const auto result = mesh.SplitSurface(slots, seed - mesh.Cells().begin(), geometry, points, 1, SIZE_MAX, stats, false);
      ok = result == local::SurfaceRefusal::None && stats.splits_feature == 1;
      if (!ok) {
        reason = std::to_string(static_cast<int>(result)) + " " + stats.first_feature_reason + " " + stats.first_trace_reason;
      } else {
        mesh.Check();
        mesh.WriteBack(working, cells);
      }
    } catch (const std::exception& error) {
      ok = false;
      reason = error.what();
    }
  }
  INFO(reason);
  REQUIRE(world.maximum(!ok) == 0);
}

local::MemoryBudget ExchangeBudget() { return {ExchangeCeiling, 0, sizeof(Owned::value_type) + 4 * sizeof(void*)}; }
}  // namespace

TEST_CASE("Surface overlays deliver true child traces to the post-shift owner", "[.][NativeLocalCoreSurfaceExchange]") {
  World world;
  REQUIRE((world.size == 2 || world.size == 4));
  const auto initial = TwoCubes();
  const auto reference = EntityReference(initial);
  local::StateSurfaceGeometry geometry(reference, AtlasBudget());
  constexpr Id midpoint = LastOriginalPoint + 2;
  const auto expected = Propose(geometry, initial, LowerCrease, midpoint);
  Owned working;
  for (const auto& entry : initial) {
    const int owner = Centroid(entry.second.cell).x < 1 ? 0 : 1;
    if (world.rank == owner) working.emplace(entry);
  }
  const auto input_cells = working.size();
  auto memory = ExchangeBudget();
  local::IdSequence point_ids(LastOriginalPoint, world.rank, world.size);
  local::IdSequence cell_ids(initial.rbegin()->first, world.rank, world.size);
  const auto first = local::MeetBorderPoints(world, working, memory);
  SplitOnRank(world, 1, working, first, geometry, LowerCrease, point_ids, cell_ids);
  CHECK((geometry.Find(midpoint) != nullptr) == (world.rank == 1));
  geometry.Exchange(world, memory, memory.OwnedBytes(working.size()));

  const auto* shared = geometry.Find(midpoint);
  REQUIRE(world.maximum(shared == nullptr) == 0);
  CHECK(Bytes(*shared) == Bytes(expected.point));
  CHECK_FALSE(geometry.Feature(LowerCrease));
  SU2NativeGeometry::BoundaryCoverage actual, oracle;
  for (const auto& [edge, trace] : expected.children) {
    const auto* received = geometry.FindTrace(edge);
    REQUIRE(world.maximum(received == nullptr) == 0);
    actual.traces.emplace(edge, *received);
    oracle.traces.emplace(edge, trace);
  }
  CHECK(EncodeBoundaryAtlas(actual, {}) == EncodeBoundaryAtlas(oracle, {}));

  const auto lower = local::ShiftBorders(world, working, first, local::Direction::Lower, memory);
  CHECK((world.rank == 0 ? lower.received > 0 : true));
  const auto second = local::MeetBorderPoints(world, working, memory);
  const Edge child{LowerCrease[0], midpoint};
  constexpr Id second_midpoint = LastOriginalPoint + 1;
  SplitOnRank(world, 0, working, second, geometry, child, point_ids, cell_ids);
  geometry.Exchange(world, memory, memory.OwnedBytes(working.size()));
  CHECK_FALSE(geometry.Feature(child));
  CHECK(geometry.Find(second_midpoint) != nullptr);
  CHECK(geometry.Feature({child[0], second_midpoint}));
  CHECK(geometry.Feature({second_midpoint, child[1]}));
  local::ShiftBorders(world, working, second, local::Direction::Higher, memory, &first);
  const auto caller = transfer_memory::Add(reference->ResidentBytes(), memory.OwnedBytes(working.size()));
  const auto rebuilt = local::BuildSurfaceState(world, working, geometry, memory, caller);
  REQUIRE(rebuilt.state);
  const auto& state = *rebuilt.state;
  const auto& before = *reference->Parameters();
  CHECK(rebuilt.splits == 2);
  CHECK(state.Revision() == before.Revision() + 1);
  CHECK(state.Surface().size() == 24);
  CHECK(state.Records().size() == 14);
  CHECK(state.Coverage().traces.size() == before.Coverage().traces.size() + 2);
  const auto owned_faces = Surface(working).size();
  const auto summed_faces = world.reduce<uint64_t>(owned_faces, CPassiveComm::Op::SUM);
  CHECK(summed_faces == state.Surface().size());
  const auto snapshot = state.Snapshot();
  auto reference_bytes = world.rank == 0 ? snapshot : std::vector<char>{};
  CPassiveComm::BcastRounds(reference_bytes, 0, world.comm);
  REQUIRE_FALSE(world.maximum(snapshot != reference_bytes));
  std::cout << "FP2_A MPI_BUILD rank=" << world.rank << " ranks=" << world.size << " splits=" << rebuilt.splits
            << " input_cells=" << input_cells << " output_cells=" << working.size() << " owned_faces=" << owned_faces
            << " total_owned_faces=" << summed_faces << " faces=" << state.Surface().size()
            << " points=" << state.Records().size() << " traces=" << state.Coverage().traces.size()
            << " revision=" << state.Revision() << " snapshot_bytes=" << snapshot.size()
            << " snapshot_equal=1 peak_rss=" << ProcessPeakRSS3D() << '\n';
  if (world.rank == 0)
    std::cout << "FP2A_FIXTURE surface_exchange ranks=" << world.size << " owner_before=1 owner_after=0 received="
              << lower.received << " point=" << midpoint << " child_point=" << second_midpoint
              << " traces=" << expected.children.size() << '\n';
}

TEST_CASE("Surface exchanges elect duplicate ids and child-trace keys", "[.][NativeLocalCoreSurfaceExchange]") {
  World world;
  REQUIRE((world.size == 2 || world.size == 4));
  const auto initial = TwoCubes();
  const auto reference = EntityReference(initial);
  for (const bool duplicate_id : {true, false}) {
    CAPTURE(duplicate_id);
    local::StateSurfaceGeometry geometry(reference, AtlasBudget());
    auto lower = Propose(geometry, initial, LowerCrease, LastOriginalPoint + 1);
    auto upper = Propose(geometry, initial, UpperCrease, LastOriginalPoint + (duplicate_id ? 1 : 2));
    if (!duplicate_id) {
      auto shared = upper.children.extract(upper.children.begin());
      shared.key() = {LowerCrease[0], LastOriginalPoint + 1};
      REQUIRE(upper.children.insert(std::move(shared)).inserted);
    }
    if (world.rank == 0) geometry.Accept(std::move(lower));
    if (world.rank == 1) geometry.Accept(std::move(upper));
    auto memory = ExchangeBudget();
    std::string message;
    try {
      geometry.Exchange(world, memory, 0);
    } catch (const CElectedFailure& failure) {
      message = failure.message;
    }
    INFO(message);
    CHECK(message == (duplicate_id ? "Duplicate native local surface point id from workers."
                                   : "Duplicate native local surface child edge from workers."));
    if (world.rank == 0)
      std::cout << "FP2A_FIXTURE surface_exchange_duplicate ranks=" << world.size
                << " kind=" << (duplicate_id ? "point" : "trace") << '\n';
  }
}
