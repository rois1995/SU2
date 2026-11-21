/*!
 * \file CNativeLocalCore3DSurface_tests.cpp
 * \brief Local surface ring replacement, placement guards and length/distance sweeps.
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md).
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */
#include "catch.hpp"
#include "../../../Common/include/adaptation/CNativeLocalSurface3D.hpp"

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
