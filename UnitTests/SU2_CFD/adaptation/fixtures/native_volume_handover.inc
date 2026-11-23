/*!
 * \file native_volume_handover.inc
 * \brief Production local-core passes on the frozen FP2-B0 lab inputs.
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md).
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */

#include "../../../../Common/include/adaptation/CNativeCurvedReference.hpp"

#include <iterator>
#include <limits>
#include <tuple>

namespace SU2NativeLocal3D {
std::vector<Edge> SurfaceFixtureAccess::CollapsePhysicalEdges(const Mesh& mesh) {
  std::map<SU2Native3D::Edge, Edge> edges;
  for (const auto& face : mesh.PhysicalFaces()) {
    const auto corners = OrientedFace(mesh.cells_[face.cell].vertices, face.face);
    for (unsigned i = 0; i < 3; ++i) {
      const auto edge = mesh.ById({corners[i], corners[(i + 1) % 3]});
      edges.try_emplace(SU2Native3D::Edge{mesh.points_[edge[0]].node.id, mesh.points_[edge[1]].node.id}, edge);
    }
  }
  std::vector<Edge> result;
  result.reserve(edges.size());
  for (const auto& entry : edges) result.push_back(entry.second);
  return result;
}

std::vector<std::vector<Slot>> SurfaceFixtureAccess::CollapseStars(const Mesh& mesh) {
  // A rank may own more than one face-connected sector of a border point's star.
  std::vector<std::vector<Slot>> stars(mesh.points_.size());
  for (Slot c = 0; c < mesh.cells_.size(); ++c)
    if (mesh.cells_[c].live)
      for (const auto p : mesh.cells_[c].vertices)
        if (mesh.points_[p].surface) stars[p].push_back(c);
  return stars;
}

std::vector<Slot> SurfaceFixtureAccess::CollapseRing(const Mesh& mesh, const std::vector<Slot>& star, Edge edge) {
  std::vector<Slot> cells;
  for (const auto c : star) {
    const auto& vertices = mesh.cells_[c].vertices;
    if (std::find(vertices.begin(), vertices.end(), edge[1]) != vertices.end()) cells.push_back(c);
  }
  return cells;
}

std::vector<Face> SurfaceFixtureAccess::CollapseSurfaceParents(const Mesh& mesh, Edge edge,
                                                              const std::vector<Slot>& cells) {
  Mesh::Ring ring;
  ring.cells = cells;
  std::vector<Face> parents;
  for (const auto& face : mesh.SurfaceParents(edge, ring))
    parents.push_back(OrientedFace(mesh.cells_[face.cell].vertices, face.face));
  return parents;
}

std::vector<std::pair<Face, SurfaceFace>> SurfaceFixtureAccess::CollapseSkin(const Mesh& mesh,
                                                                            const std::vector<Slot>& star,
                                                                            Slot point) {
  std::vector<std::pair<Face, SurfaceFace>> skin;
  for (const auto c : star) {
    const auto& cell = mesh.cells_[c];
    for (unsigned f = 0; f < 4; ++f)
      if (cell.neighbors[f] == Physical && cell.vertices[f] != point)
        skin.emplace_back(OrientedFace(cell.vertices, f), mesh.SurfaceFaceAt({c, f}));
  }
  return skin;
}

bool SurfaceFixtureAccess::CollapseTrialPositive(const Mesh& mesh, Vertices vertices) {
  return mesh.TrialPositive(vertices);
}

bool SurfaceFixtureAccess::CollapsePositive(const Mesh& mesh, Vertices vertices) {
  return mesh.Positive(vertices);
}

double SurfaceFixtureAccess::CollapseWorst(const Mesh& mesh, const std::vector<Slot>& star) {
  return mesh.Worst(star);
}
}  // namespace SU2NativeLocal3D

namespace {
namespace fp2_c2 {
enum Test { T1, T2, T2s, T2e, T3, T4, T5, T5f, T6, T7, T8, T8m, T9, T9c, T10, T11, T12, T12L, T13, TestCount };
enum Outcome { Skipped, Fail, Pass, Error };

struct Direction {
  local::Slot remove = local::NoSlot, keep = local::NoSlot;
  std::array<Outcome, TestCount> tests{};
  std::vector<local::Slot> star{};
  std::vector<SurfaceFace> children{};
  double longest = 0;
  double height = -std::numeric_limits<double>::infinity();
  bool exact_failure = false;
};

struct Contact {
  double distance = std::numeric_limits<double>::infinity();
  Id id = 0;
  Point point{};
};

using Stars = std::vector<std::vector<local::Slot>>;

using Access = local::SurfaceFixtureAccess;
using Skin = std::vector<std::pair<local::Face, SurfaceFace>>;
using Slots = std::set<local::Slot>;
using Edges = std::set<local::Edge>;

template <class Collection>
bool Includes(const Collection& items, local::Slot slot) {
  return std::find(items.begin(), items.end(), slot) != items.end();
}

template <class Check>
void ProbeTest(Direction& direction, Test test, Check check) {
  try {
    direction.tests[test] = check() ? Pass : Fail;
  } catch (...) {
    direction.tests[test] = Error;
  }
}

Slots CellPoints(const local::Mesh& mesh, const std::vector<local::Slot>& cells) {
  Slots points;
  for (const auto c : cells) {
    const auto& vertices = mesh.Cells()[c].vertices;
    points.insert(vertices.begin(), vertices.end());
  }
  return points;
}

Slots SkinNeighbors(const Skin& skin, local::Slot point) {
  Slots neighbors;
  for (const auto& face : skin) neighbors.insert(face.first.begin(), face.first.end());
  neighbors.erase(point);
  return neighbors;
}

Edges OppositeLinkEdges(const local::Mesh& mesh, const std::vector<local::Slot>& cells, local::Edge edge) {
  Edges edges;
  for (const auto c : cells) {
    const auto& v = mesh.Cells()[c].vertices;
    for (unsigned i = 0; i < v.size(); ++i)
      for (unsigned j = i + 1; j < v.size(); ++j)
        if (!Includes(edge, v[i]) && !Includes(edge, v[j]))
          edges.insert({std::min(v[i], v[j]), std::max(v[i], v[j])});
  }
  return edges;
}

double SpaceDistance(Point a, Point b) {
  return std::hypot(a.x - b.x, a.y - b.y, a.z - b.z);
}

double SmallestAltitude(Point a, Point b, Point c) {
  const Point ab{b.x - a.x, b.y - a.y, b.z - a.z}, ac{c.x - a.x, c.y - a.y, c.z - a.z};
  const Point cross{ab.y * ac.z - ab.z * ac.y, ab.z * ac.x - ab.x * ac.z, ab.x * ac.y - ab.y * ac.x};
  return std::hypot(cross.x, cross.y, cross.z) /
         std::max({SpaceDistance(a, b), SpaceDistance(a, c), SpaceDistance(b, c)});
}

double Height(Point a, Point b, Point c, Point m) {
  const Point ab{b.x - a.x, b.y - a.y, b.z - a.z}, ac{c.x - a.x, c.y - a.y, c.z - a.z};
  const Point n{ab.y * ac.z - ab.z * ac.y, ab.z * ac.x - ab.x * ac.z, ab.x * ac.y - ab.y * ac.x};
  return -(n.x * (m.x - a.x) + n.y * (m.y - a.y) + n.z * (m.z - a.z)) / std::hypot(n.x, n.y, n.z);
}

using Triangle = std::array<Point, 3>;

Triangle Coordinates(const local::Mesh& mesh, local::Face face) {
  const auto& points = mesh.Points();
  return {points[face[0]].node.p, points[face[1]].node.p, points[face[2]].node.p};
}

bool ReachesChild(Point p, const std::vector<Triangle>& children, double limit) {
  const auto values = [](Point point) { return SU2NativeCurved::Vec<3>{point.x, point.y, point.z}; };
  for (const auto& child : children) {
    double u, v;
    const auto q = SU2NativeCurved::detail::ClosestOnTriangle(
        values(p), values(child[0]), values(child[1]), values(child[2]), u, v);
    if (SpaceDistance(p, {q[0], q[1], q[2]}) <= limit) return true;
  }
  return false;
}

void ProbeMesh(const local::Mesh& mesh, const Stars& stars, const std::vector<int>& dimensions,
               double tolerance, double limit, Direction& direction) {
  const auto p = direction.remove, k = direction.keep;
  const local::Edge edge{p, k};
  const auto& p_star = stars[p];
  const auto& k_star = stars[k];
  std::vector<local::Slot> ring;
  std::vector<local::Face> parents;
  ProbeTest(direction, T1, [&] {
    ring = Access::CollapseRing(mesh, p_star, edge);
    parents = Access::CollapseSurfaceParents(mesh, edge, ring);
    return parents.size() == 2;
  });
  if (direction.tests[T1] != Pass) return;
  direction.star = p_star;

  const auto p_points = CellPoints(mesh, p_star), k_points = CellPoints(mesh, k_star);
  const auto ring_points = CellPoints(mesh, ring);
  const auto p_skin = Access::CollapseSkin(mesh, p_star, p);
  const auto p_neighbors = SkinNeighbors(p_skin, p);
  std::vector<local::Face> children;
  for (const auto& [face, parent] : p_skin) {
    if (Includes(face, k)) continue;
    auto child = face;
    auto surface = parent;
    const auto opposite = std::find(child.begin(), child.end(), p) - child.begin();
    child[opposite] = k;
    surface.v[opposite] = mesh.Points()[k].node.id;
    direction.children.push_back(surface);
    children.push_back(child);
  }

  std::vector<local::Vertices> fresh;
  for (const auto c : p_star) {
    auto v = mesh.Cells()[c].vertices;
    if (Includes(v, k)) continue;
    *std::find(v.begin(), v.end(), p) = k;
    fresh.push_back(v);
  }

  ProbeTest(direction, T2, [&] {
    Slots common, rim = ring_points;
    std::set_intersection(p_points.begin(), p_points.end(), k_points.begin(), k_points.end(),
                          std::inserter(common, common.end()));
    for (const auto endpoint : edge) {
      common.erase(endpoint);
      rim.erase(endpoint);
    }
    return common == rim;
  });
  ProbeTest(direction, T2s, [&] {
    Slots common, rim;
    const auto k_neighbors = SkinNeighbors(Access::CollapseSkin(mesh, k_star, k), k);
    std::set_intersection(p_neighbors.begin(), p_neighbors.end(), k_neighbors.begin(), k_neighbors.end(),
                          std::inserter(common, common.end()));
    for (const auto& face : parents)
      for (const auto vertex : face)
        if (!Includes(edge, vertex)) rim.insert(vertex);
    return common == rim;
  });
  ProbeTest(direction, T2e, [&] {
    const auto p_edges = OppositeLinkEdges(mesh, p_star, edge);
    const auto k_edges = OppositeLinkEdges(mesh, k_star, edge);
    const auto rim = OppositeLinkEdges(mesh, ring, edge);
    for (const auto& e : p_edges)
      if (k_edges.count(e) && !rim.count(e)) return false;
    return true;
  });
  ProbeTest(direction, T3, [&] {
    for (const auto& child : children) {
      for (const auto c : k_star) {
        const auto& v = mesh.Cells()[c].vertices;
        if (std::all_of(child.begin(), child.end(), [&](auto slot) { return Includes(v, slot); })) return false;
      }
    }
    return true;
  });
  ProbeTest(direction, T4, [&] {
    if (!std::all_of(fresh.begin(), fresh.end(),
                     [&](auto v) { return Access::CollapseTrialPositive(mesh, v); }))
      return false;
    direction.exact_failure = !std::all_of(fresh.begin(), fresh.end(),
                                           [&](auto v) { return Access::CollapsePositive(mesh, v); });
    return true;
  });
  ProbeTest(direction, T5, [&] {
    for (const auto& v : fresh)
      for (const auto x : v)
        if (!ring_points.count(x)) direction.longest = std::max(direction.longest, mesh.Length({k, x}));
    return direction.longest <= local::LongEdge;
  });
  ProbeTest(direction, T5f, [&] {
    for (const auto& child : children) {
      for (const auto x : child) {
        if (x == k || k_points.count(x) || dimensions[k] == 2 || dimensions[x] == 2) continue;
        if (dimensions[k] < 0 || dimensions[x] < 0)
          throw std::runtime_error("FP2-C2 feature chord has an unknown mobility dimension.");
        return false;
      }
    }
    return true;
  });
  ProbeTest(direction, T6, [&] {
    auto worst = std::numeric_limits<double>::infinity();
    for (const auto& v : fresh) worst = std::min(worst, mesh.Margin(v));
    return !(worst < local::BadMargin && worst < Access::CollapseWorst(mesh, p_star));
  });

  const auto point = mesh.Points()[p].node.p;
  std::vector<Triangle> triangles;
  triangles.reserve(children.size());
  for (const auto face : children) triangles.push_back(Coordinates(mesh, face));
  ProbeTest(direction, T7, [&] {
    for (const auto& child : triangles)
      if (!(SmallestAltitude(child[0], child[1], child[2]) >= limit)) return false;
    return true;
  });
  ProbeTest(direction, T8m, [&] { return ReachesChild(point, triangles, limit); });
  ProbeTest(direction, T8, [&] {
    if (direction.tests[T8m] == Error) throw std::runtime_error("FP2-C2 reverse witness evaluation failed.");
    if (direction.tests[T8m] != Pass) return false;
    for (const auto spoke : p_neighbors) {
      const auto v = mesh.Points()[spoke].node.p;
      if (!ReachesChild({(point.x + v.x) / 2, (point.y + v.y) / 2, (point.z + v.z) / 2}, triangles, limit))
        return false;
    }
    for (const auto& face : p_skin) {
      const auto [a, b, c] = Coordinates(mesh, face.first);
      if (!ReachesChild({(a.x + b.x + c.x) / 3, (a.y + b.y + c.y) / 3, (a.z + b.z + c.z) / 3},
                        triangles, limit))
        return false;
    }
    return true;
  });
  bool finite_heights = true;
  ProbeTest(direction, T9, [&] {
    for (const auto& child : triangles) {
      const auto h = Height(child[0], child[1], child[2], point);
      finite_heights &= std::isfinite(h);
      direction.height = std::max(direction.height, h);
    }
    return finite_heights && direction.height <= tolerance;
  });
  direction.tests[T9c] = direction.tests[T9] == Error ? Error : (finite_heights && direction.height <= 0 ? Pass : Fail);
}

Contact OtherContact(const local::Mesh& mesh, const Stars& stars, local::Slot remove,
                     const std::vector<local::Slot>& surface_points) {
  const auto adjacent = SkinNeighbors(Access::CollapseSkin(mesh, stars[remove], remove), remove);
  auto excluded = adjacent;
  excluded.insert(remove);
  for (const auto neighbor : adjacent)
    for (const auto& face : Access::CollapseSkin(mesh, stars[neighbor], neighbor))
      excluded.insert(face.first.begin(), face.first.end());

  const auto& points = mesh.Points();
  const auto p = points[remove].node.p;
  Contact contact;
  double nearest_squared = std::numeric_limits<double>::infinity();
  for (const auto v : surface_points) {
    const auto& node = points[v].node;
    const auto dx = p.x - node.p.x, dy = p.y - node.p.y, dz = p.z - node.p.z;
    const auto squared = dx * dx + dy * dy + dz * dz;
    if (squared > nearest_squared || (squared == nearest_squared && node.id >= contact.id) || excluded.count(v))
      continue;
    nearest_squared = squared;
    contact.id = node.id;
    contact.point = node.p;
  }
  contact.distance = std::sqrt(nearest_squared);
  return contact;
}
}  // namespace fp2_c2

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

namespace fp2_c2 {
constexpr std::array<const char*, TestCount> Names{
    "T1", "T2", "T2s", "T2e", "T3", "T4", "T5", "T5f", "T6", "T7", "T8", "T8m", "T9", "T9c",
    "T10", "T11", "T12", "T12L", "T13"};
constexpr double StrictSeconds = 25 * 60;

struct CapturedChild {
  Id facet;
  std::array<SU2NativeGeometry::SurfaceBinding, 3> bindings;
  std::array<Point, 3> points;
};

struct Trial {
  Id remove, keep;
  Point xyz;
  const char* pclass;
  const char* kclass;
  double length;
  std::array<Outcome, TestCount> tests{};
  double longest = 0, height = -std::numeric_limits<double>::infinity();
  bool exact_failure = false, sample10 = false, strict_selected = false, missing_binding = false;
  Contact other{};
  size_t unconverged = 0, calls_T10 = 0, calls_T11 = 0, calls_T12 = 0;
  std::vector<local::Slot> star{};
  std::vector<CapturedChild> strict{};
};

struct ShortSurfaceEdge {
  local::Edge slots;
  Edge ids;
  double length;
};

struct PassObservation {
  unsigned number;
  double tolerance, limit;
  double seconds_T10 = 0, seconds_T11 = 0, seconds_T12 = 0, seconds_other = 0;
  std::vector<ShortSurfaceEdge> edges{};
  std::vector<Trial> trials{};
};

const char* PointClass(const local::PointRow& point, int dimension) {
  if (point.border) return "border";
  switch (dimension) {
    case 0: return "corner";
    case 1: return "feature";
    case 2: return "sheet";
    default: throw std::logic_error("C2 could not classify a nonborder surface point.");
  }
}

template <class Evaluate>
Outcome TestOutcome(Evaluate evaluate) {
  try {
    return evaluate() ? Pass : Fail;
  } catch (...) {
    return Error;
  }
}

void ProbeGeometry(local::StateSurfaceGeometry& surface, const Direction& direction, PassObservation& pass,
                   Trial& trial) {
  auto start = Clock::now();
  trial.tests[T10] = TestOutcome([&] {
    bool close = true;
    for (const auto& face : direction.children) {
      ++trial.calls_T10;
      const auto samples = surface.Samples(face);
      trial.unconverged += samples.unconverged;
      close &= std::all_of(samples.distances.begin(), samples.distances.end(),
                           [&](double distance) { return distance <= pass.limit; });
    }
    return close;
  });
  pass.seconds_T10 += Seconds(start);
  start = Clock::now();
  trial.tests[T11] = TestOutcome([&] {
    bool on_entity = true;
    for (const auto& face : direction.children) {
      const auto* first = surface.Find(face.v[0]);
      if (!first) throw std::logic_error("C2 entity test needs a child binding.");
      ++trial.calls_T11;
      on_entity &= surface.OnEntity(face, *first);
    }
    return on_entity;
  });
  pass.seconds_T11 += Seconds(start);
}

bool EligibleForStrict(const Trial& trial) {
  for (auto test : {T1, T2, T2s, T2e, T3, T4, T5, T5f, T6, T7, T9, T10, T11})
    if (trial.tests[test] != Pass) return false;
  return true;
}

void CaptureStrictChildren(const local::StateSurfaceGeometry& surface, const Direction& direction, Trial& trial) {
  for (const auto& face : direction.children) {
    CapturedChild child{};
    child.facet = face.facet;
    for (unsigned i = 0; i < 3; ++i) {
      const auto* point = surface.Find(face.v[i]);
      if (!point) {
        trial.missing_binding = true;
        return;
      }
      child.bindings[i] = point->binding;
      child.points[i] = point->node.p;
    }
    trial.strict.push_back(std::move(child));
  }
}

class CollapseObservation {
 public:
  void Observe(const local::Mesh& mesh, local::StateSurfaceGeometry& surface, unsigned number, double short_edge) {
    const auto started = Clock::now();
    const auto tol = surface.State().GeometryTolerance();
    PassObservation pass{number, tol, local::SurfaceDistanceFactor * tol};
    const auto& points = mesh.Points();
    std::vector<int> dimensions(points.size(), -1);
    std::vector<local::Slot> physical_points;
    const auto& state = surface.State();
    for (local::Slot p = 0; p < points.size(); ++p) {
      const auto& point = points[p];
      if (!point.surface || point.removed) continue;
      physical_points.push_back(p);
      const auto* bound = surface.Find(point.node.id);
      if (bound)
        dimensions[p] = state.Geometry().MobilityDimension(bound->binding, state.Mobility());
      else if (!point.border)
        throw std::logic_error("C2 nonborder surface point has no binding: " + std::to_string(point.node.id));
    }
    for (const auto& edge : local::SurfaceFixtureAccess::CollapsePhysicalEdges(mesh)) {
      const double length = mesh.Length(edge);
      if (length < short_edge)
        pass.edges.push_back({edge, {points[edge[0]].node.id, points[edge[1]].node.id}, length});
    }
    std::sort(pass.edges.begin(), pass.edges.end(), [](const ShortSurfaceEdge& a, const ShortSurfaceEdge& b) {
      return std::tie(a.length, a.ids) < std::tie(b.length, b.ids);
    });
    const auto stars = local::SurfaceFixtureAccess::CollapseStars(mesh);
    std::vector<std::pair<Outcome, Contact>> contact(points.size());
    pass.trials.reserve(2 * pass.edges.size());
    size_t sheet_ordinal = 0;
    for (const auto& edge : pass.edges) {
      for (unsigned end = 0; end < 2; ++end) {
        const auto p = edge.slots[end], k = edge.slots[1 - end];
        Trial trial{points[p].node.id, points[k].node.id, points[p].node.p,
                    PointClass(points[p], dimensions[p]), PointClass(points[k], dimensions[k]), edge.length};
        if (!points[p].border && dimensions[p] == 2) {
          trial.sample10 = ++sheet_ordinal % 10 == 0;
          Direction direction{p, k};
          ProbeMesh(mesh, stars, dimensions, pass.tolerance, pass.limit, direction);
          trial.tests = direction.tests;
          trial.longest = direction.longest;
          trial.height = direction.height;
          trial.exact_failure = direction.exact_failure;
          if (trial.tests[T1] == Pass) {
            ProbeGeometry(surface, direction, pass, trial);
            auto& cached = contact[p];
            if (cached.first == Skipped)
              cached.first = TestOutcome([&] {
                cached.second = OtherContact(mesh, stars, p, physical_points);
                return true;
              });
            trial.tests[T13] = cached.first;
            trial.other = cached.second;
            const bool eligible = EligibleForStrict(trial);
            trial.strict_selected = eligible || trial.sample10;
            if (trial.strict_selected) CaptureStrictChildren(surface, direction, trial);
            if (eligible && trial.tests[T8] == Pass) trial.star = std::move(direction.star);
          }
        }
        pass.trials.push_back(std::move(trial));
      }
    }
    pass.seconds_other = std::max(0., Seconds(started) - pass.seconds_T10 - pass.seconds_T11);
    passes_.push_back(std::move(pass));
  }

  void Finish(const BoundaryParameterState& state, int rank, size_t cells, size_t caller, size_t ceiling) {
    std::vector<std::pair<PassObservation*, Trial*>> pending;
    for (auto& pass : passes_)
      for (auto& trial : pass.trials)
        if (trial.strict_selected) pending.push_back({&pass, &trial});
    std::sort(pending.begin(), pending.end(), [](const auto& a, const auto& b) {
      const auto order = [](const auto& row) {
        const auto& t = *row.second;
        return std::make_tuple(t.length, std::min(t.remove, t.keep), std::max(t.remove, t.keep),
                               t.remove, row.first->number);
      };
      return order(a) < order(b);
    });
    const auto deadline = Clock::now();
    auto budget = native_fp2_fixture::Budget(cells, caller, deadline, StrictSeconds, ceiling);
    for (const auto& [pass, trial] : pending) {
      if (Seconds(deadline) >= StrictSeconds) break;
      const auto start = Clock::now();
      bool above_limit = false;
      bool stopped = false;
      const auto result = TestOutcome([&] {
        if (trial->missing_binding) throw std::logic_error("C2 strict test needs all child bindings.");
        bool close = true;
        for (const auto& child : trial->strict) {
          if (Seconds(deadline) >= StrictSeconds) {
            stopped = true;
            break;
          }
          ++trial->calls_T12;
          const auto bound = state.EntityFaceError(child.facet, child.bindings, child.points, &budget,
                                                   SU2NativeGeometry::SurfaceQueryNeed::Classify());
          close &= bound <= pass->tolerance;
          above_limit |= !(bound <= pass->limit);
        }
        return close;
      });
      pass->seconds_T12 += Seconds(start);
      if (stopped || (result == Error && Seconds(deadline) >= StrictSeconds)) break;
      trial->tests[T12] = result;
      trial->tests[T12L] = result == Error ? Error : above_limit ? Fail : Pass;
    }
    for (const auto& pass : passes_) Print(pass, rank);
  }

 private:
  static void Print(const PassObservation& pass, int rank) {
    const auto prefix = [&](std::ostream& out, const char* row) -> std::ostream& {
      return out << "FP2_C2 " << row << " rank=" << rank << " pass=" << pass.number;
    };
    const auto point = [](std::ostream& out, Point p) -> std::ostream& {
      return out << p.x << ',' << p.y << ',' << p.z;
    };
    std::ostringstream row;
    row << std::setprecision(17);
    for (const auto& edge : pass.edges) {
      prefix(row, "edge") << " a=" << edge.ids[0] << " b=" << edge.ids[1] << " length=" << edge.length << '\n';
      std::cout << row.str();
      row.str("");
    }
    size_t pending = 0;
    for (const auto& trial : pass.trials) {
      prefix(row, "direction") << " p=" << trial.remove << " k=" << trial.keep
                                << " pclass=" << trial.pclass << " kclass=" << trial.kclass
                                << " length=" << trial.length;
      for (size_t i = 0; i < TestCount; ++i) row << ' ' << Names[i] << '=' << "-FPE"[trial.tests[i]];
      row << " longest=" << trial.longest << " h=" << trial.height << " exact_failure=" << trial.exact_failure
          << " sample10=" << trial.sample10 << " strict_selected=" << trial.strict_selected
          << " unconverged=" << trial.unconverged << " calls_T10=" << trial.calls_T10
          << " calls_T11=" << trial.calls_T11 << " calls_T12=" << trial.calls_T12
          << " other=" << trial.other.distance << " xyz=";
      point(row, trial.xyz);
      if (std::isfinite(trial.other.distance)) {
        row << " other_id=" << trial.other.id << " other_xyz=";
        point(row, trial.other.point);
      } else {
        row << " other_id=- other_xyz=-";
      }
      row << " star=";
      if (trial.star.empty()) row << '-';
      for (size_t i = 0; i < trial.star.size(); ++i) row << (i == 0 ? "" : ",") << trial.star[i];
      std::cout << row.str() << '\n';
      row.str("");
      pending += trial.strict_selected && trial.tests[T12] == Skipped;
    }
    prefix(row, "timing") << " seconds_T10=" << pass.seconds_T10 << " seconds_T11=" << pass.seconds_T11
                           << " seconds_T12=" << pass.seconds_T12 << " seconds_other=" << pass.seconds_other
                           << " t12_stopped=" << (pending != 0) << " tolerance=" << pass.tolerance
                           << " limit=" << pass.limit << '\n';
    std::cout << row.str() << std::flush;
  }

  std::vector<PassObservation> passes_;
};
}  // namespace fp2_c2

void RunVolumeHandover(bool parallel, bool handover = false, SurfaceObservation observation = SurfaceObservation::None,
                       ProductionPartition partition = ProductionPartition::None, bool measure_collapse = false) {
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
  const unsigned pass_count = parallel ? local::PassCount : 1;
  local::BorderMeeting previous;
  SurfaceReasons remaining;
  std::optional<fp2_c2::CollapseObservation> collapse;
  if (measure_collapse) collapse.emplace();
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
        row << '\n';
        ReportCollapseCore(row, stats, world.rank, pass);
        std::cout << row.str() << std::flush;
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
        if (collapse) {
          const auto measurement_start = Clock::now();
          collapse->Observe(mesh, *surface, pass, options.short_edge);
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
  CheckLabStep(world, [&] {
    std::ostringstream digest;
    ReportCellDigest(digest, input.cells, world.rank);
    std::cout << digest.str() << std::flush;
  });
  if (collapse)
    CheckLabStep(world, [&] {
      collapse->Finish(surface->State(), world.rank, global_cells, memory.maximum_bytes, input.ceiling);
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

TEST_CASE("FP2-C2 dry surface collapse on the serial hand-over", "[.][NativeFP2C2Serial]") {
  RunVolumeHandover(false, true, SurfaceObservation::None, ProductionPartition::None, true);
}

TEST_CASE("FP2-C2 dry surface collapse on the production partition", "[.][NativeFP2C2Production1]") {
  RunVolumeHandover(true, true, SurfaceObservation::None, ProductionPartition::Event1, true);
}
