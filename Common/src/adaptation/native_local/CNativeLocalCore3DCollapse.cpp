/*!
 * \file CNativeLocalCore3DCollapse.cpp
 * \brief Inside-only, fixed-keeper surface contraction.
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md).
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */
#include "../../../include/adaptation/CNativeLocalCore3D.hpp"
#include "../../../include/adaptation/CNativeLocalSurface3D.hpp"
#include "../../../include/adaptation/CNativeCurvedReference.hpp"
#include <algorithm>
#include <cmath>
#include <limits>

namespace SU2NativeLocal3D {
namespace {
using Reason = SurfaceCollapseRefusal;
using Vec = SU2NativeCurved::Vec<3>;
using SU2NativeCurved::Axpy;
using SU2NativeCurved::Cross;
using SU2NativeCurved::Dot;
using SU2NativeCurved::Sub;
using SU2NativeCurved::detail::ClosestOnTriangle;

template <class V, class T>
bool Has(const V& row, T value) {
  return std::find(row.begin(), row.end(), value) != row.end();
}

template <class V>
V Replace(V row, Slot p, Slot k) {
  *std::find(row.begin(), row.end(), p) = k;
  return row;
}

Edge SortedEdge(Slot a, Slot b) { return {std::min(a, b), std::max(a, b)}; }

Vec Coordinates(Point point) { return {point.x, point.y, point.z}; }

struct VertexLink {
  std::set<Slot> points;
  std::set<Edge> edges;
  std::set<Face> faces;

  void Add(Edge edge) {
    points.insert(edge.begin(), edge.end());
    edges.insert(edge);
  }

  void Add(Face face) {
    std::sort(face.begin(), face.end());
    faces.insert(face);
    Add(Edge{face[0], face[1]});
    Add(Edge{face[0], face[2]});
    Add(Edge{face[1], face[2]});
  }
};

VertexLink VolumeLink(const std::vector<CellRow>& cells, const std::vector<Slot>& star, Slot point) {
  VertexLink link;
  for (auto slot : star) {
    const auto vertices = cells[slot].vertices;
    link.Add(OrientedFace(vertices, std::find(vertices.begin(), vertices.end(), point) - vertices.begin()));
  }
  return link;
}

template <class T>
bool CommonEquals(const std::set<T>& a, const std::set<T>& b, const std::set<T>& expected) {
  auto next = expected.begin();
  for (const auto& value : a) {
    if (!b.count(value)) continue;
    if (next == expected.end() || *next != value) return false;
    ++next;
  }
  return next == expected.end();
}

bool VolumeLinkMatches(const std::vector<CellRow>& cells, const std::vector<Slot>& star_p,
                       const std::vector<Slot>& star_k, const std::vector<Slot>& ring, Edge direction) {
  const auto p = VolumeLink(cells, star_p, direction[0]);
  const auto k = VolumeLink(cells, star_k, direction[1]);
  VertexLink edge;
  for (auto slot : ring) {
    Edge opposite{};
    std::copy_if(cells[slot].vertices.begin(), cells[slot].vertices.end(), opposite.begin(),
                 [&](Slot v) { return !Has(direction, v); });
    edge.Add(SortedEdge(opposite[0], opposite[1]));
  }
  return CommonEquals(p.points, k.points, edge.points) && CommonEquals(p.edges, k.edges, edge.edges) &&
         CommonEquals(p.faces, k.faces, edge.faces);
}

VertexLink SkinLink(const std::vector<Face>& faces, Slot point) {
  VertexLink link;
  for (const auto& face : faces) {
    Edge opposite{};
    std::copy_if(face.begin(), face.end(), opposite.begin(), [&](Slot v) { return v != point; });
    link.Add(SortedEdge(opposite[0], opposite[1]));
  }
  return link;
}

std::optional<Vec> InteriorSpokeWitness(const Vec& p, const Vec& x, const Vec& k, const Vec& y) {
  const auto spoke = Sub<3>(x, p);
  const auto diagonal = Sub<3>(y, k);
  const auto normal = Cross(spoke, diagonal);
  const auto denominator = Dot<3>(normal, normal);
  if (denominator == 0.) return {};
  const auto offset = Sub<3>(k, p);
  const double s = Dot<3>(Cross(offset, diagonal), normal) / denominator;
  const double t = Dot<3>(Cross(offset, spoke), normal) / denominator;
  if (s > 0. && s < 1. && t > 0. && t < 1.) return Axpy<3>(p, s, spoke);
  return {};
}

bool ReverseWithin(const std::vector<PointRow>& points, Edge direction, const std::vector<Face>& old,
                   const std::vector<Face>& children, const std::set<Slot>& spokes,
                   const std::set<Slot>& diagonals, double limit, double& maximum) {
  const auto coordinate = [&](Slot slot) { return Coordinates(points[slot].node.p); };
  const auto p = coordinate(direction[0]);
  const auto k = coordinate(direction[1]);
  std::vector<std::array<Vec, 3>> triangles;
  for (const auto& face : children)
    triangles.push_back({coordinate(face[0]), coordinate(face[1]), coordinate(face[2])});
  const auto within = [&](const Vec& witness) {
    double nearest = std::numeric_limits<double>::infinity();
    for (const auto& face : triangles) {
      double u, v;
      const auto q = ClosestOnTriangle(witness, face[0], face[1], face[2], u, v);
      nearest = std::min(nearest, std::hypot(witness[0] - q[0], witness[1] - q[1], witness[2] - q[2]));
    }
    maximum = std::max(maximum, nearest);
    return nearest <= limit;
  };

  // New-face samples miss removed geometry. For two fans that are graphs over a
  // common plane the height difference is linear between old-spoke/new-diagonal
  // intersections. Nearest-child distance can peak where the nearest child
  // changes, so this screen still requires the final surface proof.
  if (!within(p)) return false;
  for (auto x : spokes)
    if (!within(Axpy<3>(p, 0.5, Sub<3>(coordinate(x), p)))) return false;
  for (const auto& face : old) {
    const auto a = coordinate(face[0]), b = coordinate(face[1]), c = coordinate(face[2]);
    if (!within({(a[0] + b[0] + c[0]) / 3, (a[1] + b[1] + c[1]) / 3, (a[2] + b[2] + c[2]) / 3}))
      return false;
  }
  for (auto x : spokes) {
    if (x == direction[1]) continue;
    for (auto y : diagonals) {
      if (x == y) continue;
      const auto witness = InteriorSpokeWitness(p, coordinate(x), k, coordinate(y));
      if (witness && !within(*witness)) return false;
    }
  }
  return true;
}
}  // namespace

Mesh::SurfaceCollapseTrial Mesh::PrepareSurfaceCollapse(Edge direction, const Ring& ring,
                                                       const std::vector<PhysicalFace>& parents,
                                                       const SurfaceGeometry& geometry) const {
  const auto [p, k] = direction;
  const auto dimension = [&](Slot point) { return geometry.Dimension(points_[point].node.id); };
  const auto refused = [](Reason reason, Reason order = Reason::None) {
    SurfaceCollapseTrial attempt;
    attempt.refusal = reason;
    attempt.failure_order = static_cast<unsigned>(order == Reason::None ? reason : order);
    return attempt;
  };

  // Keep model features and require complete stars on this rank.
  if (dimension(p) != 2) return refused(Reason::Protected);
  if (points_[p].border) return refused(Reason::Border);
  if (parents.size() != 2) return refused(Reason::Ineligible);
  if (points_[k].border) return refused(Reason::BorderKeeper);

  const auto star_p = Star(p);
  const auto star_k = Star(k);
  const auto fan = [&](const std::vector<Slot>& star, Slot point, std::vector<PhysicalFace>* uses) {
    std::vector<Face> faces;
    for (auto cell : star) {
      const auto& row = cells_[cell];
      for (unsigned face = 0; face < 4; ++face) {
        if (row.neighbors[face] != Physical || row.vertices[face] == point) continue;
        faces.push_back(OrientedFace(row.vertices, face));
        if (uses) uses->push_back({cell, face});
      }
    }
    return faces;
  };
  std::vector<PhysicalFace> uses;
  const auto old = fan(star_p, p, &uses);
  const auto skin_p = SkinLink(old, p);
  const auto skin_k = SkinLink(fan(star_k, k, nullptr), k);
  std::vector<PhysicalFace> changed;
  std::vector<Face> children;
  std::set<Slot> diagonals;
  for (size_t i = 0; i < old.size(); ++i) {
    if (Has(old[i], k)) continue;
    changed.push_back(uses[i]);
    children.push_back(Replace(old[i], p, k));
    for (auto y : old[i])
      if (y != p && !skin_k.points.count(y)) diagonals.insert(y);
  }

  // A new chord between two features would have no trace.
  if (dimension(k) < 2 &&
      std::any_of(diagonals.begin(), diagonals.end(), [&](Slot y) { return dimension(y) < 2; }))
    return refused(Reason::FeatureChord);

  // Equal link complexes prevent a duplicate edge, face or cell (Dey et al.,
  // "Topology preserving edge contraction").
  std::set<Slot> skin_edge;
  for (const auto parent : parents)
    for (auto v : OrientedFace(cells_[parent.cell].vertices, parent.face))
      if (!Has(direction, v)) skin_edge.insert(v);
  if (!VolumeLinkMatches(cells_, star_p, star_k, ring.cells, direction) ||
      !CommonEquals(skin_p.points, skin_k.points, skin_edge) ||
      !CommonEquals(skin_p.edges, skin_k.edges, {}) ||
      std::any_of(changed.begin(), changed.end(), [&](PhysicalFace face) { return Has(ring.cells, face.cell); }))
    return refused(Reason::Link);

  // A child thinner than the geometry resolution cannot support its binding.
  const double limit = geometry.Limit();
  for (const auto& face : children) {
    if (!(SmallestAltitude(points_[face[0]].node.p, points_[face[1]].node.p, points_[face[2]].node.p) >= limit))
      return refused(Reason::Near);
  }

  // The entry mesh is embedded and conforming. The nonnegative caps and the
  // positive new cells share the old cavity boundary and fill it once by degree.
  // Volumes vary linearly as p approaches k, with no sign reversal. Prepare
  // matches oriented faces, so the new cells only meet kept cells on shared
  // faces. An outward cap would instead require a contact test.
  for (const auto parent : changed) {
    auto cap = Replace(cells_[parent.cell].vertices, p, k);
    cap[parent.face] = p;
    if (Positive(cap)) return refused(Reason::Outward);
  }

  SurfaceCollapseTrial attempt;
  if (!ReverseWithin(points_, direction, old, children, skin_p.points, diagonals, limit, attempt.reverse_max))
    return refused(Reason::Reverse);

  std::vector<Vertices> fresh;
  std::vector<Slot> incident;
  for (auto cell : star_p) {
    const auto vertices = cells_[cell].vertices;
    if (Has(vertices, k))
      incident.insert(incident.end(), vertices.begin(), vertices.end());
    else
      fresh.push_back(Replace(vertices, p, k));
  }
  std::sort(incident.begin(), incident.end());
  incident.erase(std::unique(incident.begin(), incident.end()), incident.end());
  // Commit must remove only p, even if an opposite ring vertex loses its last old cell.
  for (auto v : incident) {
    if (Has(direction, v) ||
        std::any_of(fresh.begin(), fresh.end(), [&](Vertices cell) { return Has(cell, v); }))
      continue;
    const auto star = Star(v);
    if (std::all_of(star.begin(), star.end(), [&](Slot cell) { return Has(star_p, cell); }))
      return refused(Reason::Topology);
  }
  std::vector<ReplacedFace> edits;
  for (const auto use : uses) edits.push_back({use.cell, use.face, p, NoSlot, k});
  attempt.plan = Prepare(star_p, fresh, edits);
  if (attempt.plan.refusal == Refusal::Validity) return refused(Reason::Validity, Reason::Topology);
  if (!attempt.plan.Accepted()) return refused(Reason::Topology);

  // Keep the volume size/shape rule and also bound inherited surface edges.
  double longest_child = 0.;
  for (const auto& face : children)
    for (unsigned i = 0; i < 3; ++i)
      longest_child = std::max(longest_child, Length({face[i], face[(i + 1) % 3]}));
  MeasureCollapse(attempt.plan, k, incident, longest_child);
  if (attempt.plan.refusal == Refusal::Size) return refused(Reason::Size);
  if (attempt.plan.refusal == Refusal::Shape) return refused(Reason::Shape);

  // The floating-point filter cannot certify the edits.
  if (!ExactlyPositive(attempt.plan)) return refused(Reason::Validity);
  for (size_t i = 0; i < changed.size(); ++i) {
    auto child = SurfaceFaceAt(changed[i]);
    for (unsigned v = 0; v < 3; ++v) child.v[v] = points_[children[i][v]].node.id;
    attempt.children.push_back(child);
  }
  return attempt;
}

// At the same metric and sample inputs, step C makes no edge marked by step L
// and no face marked by step D's samples. Conformity, feature demand or a
// changed metric can still split the patch.
SurfaceCollapseRefusal Mesh::CollapseSurface(Edge edge, Slot seed, SurfaceGeometry& geometry, uint32_t stamp,
                                            SurfaceCounters& counters, bool report,
                                            const std::function<void(const SurfaceFace&)>& sink) {
  edge = ById(edge);
  const auto ring = EdgeRing(edge, seed);
  const auto parents = SurfaceParents(edge, ring);
  std::array<SurfaceCollapseTrial, 2> trials{PrepareSurfaceCollapse(edge, ring, parents, geometry),
                                            PrepareSurfaceCollapse({edge[1], edge[0]}, ring, parents, geometry)};
  const auto count = [&](Reason reason) {
    if (report && reason != Reason::None) ++counters.c_refusals[static_cast<unsigned>(reason)];
  };
  for (const auto& trial : trials) count(trial.refusal);
  const auto clock = [&] { return report ? SU2_MPI::Wtime() : 0.; };
  const auto test_geometry = [&](const std::vector<SurfaceFace>& children) {
    // Forward samples screen the whole child fan in the model tolerance.
    const double first = clock();
    const bool sampled = std::all_of(children.begin(), children.end(), [&](const auto& face) {
      const auto samples = geometry.Samples(face);
      return std::all_of(samples.distances.begin(), samples.distances.end(), [&](double distance) {
        return std::isfinite(distance) && distance <= geometry.Limit();
      });
    });
    if (report) counters.seconds_c_samples += clock() - first;
    if (!sampled) return Reason::Distance;
    // The inherited facet must satisfy the same binding test as state build.
    const double second = clock();
    const bool entity = std::all_of(children.begin(), children.end(),
                                    [&](const auto& face) { return geometry.OnEntityFace(face); });
    if (report) counters.seconds_c_entity += clock() - second;
    if (!entity) return Reason::Entity;
    // The four samples screen points. The final classifier bounds the whole face.
    // The model-to-mesh side stays with the final proof.
    const double third = clock();
    const bool bounded = std::all_of(children.begin(), children.end(), [&](const auto& face) {
      if (report) ++counters.c_bound_queries;
      return geometry.WithinTolerance(face);
    });
    if (report) counters.seconds_c_bound += clock() - third;
    return bounded ? Reason::None : Reason::Bound;
  };
  const unsigned first = trials[0].plan.score < trials[1].plan.score ? 0 : 1;
  for (unsigned i : {first, 1 - first}) {
    auto& trial = trials[i];
    if (trial.refusal != Reason::None) continue;
    const auto reason = test_geometry(trial.children);
    if (reason != Reason::None) {
      count(reason);
      trial.refusal = reason;
      trial.failure_order = static_cast<unsigned>(reason);
      continue;
    }
    const auto removed = points_[edge[i]].node.id;
    const auto kept = points_[edge[1 - i]].node.id;
    RememberWorkPoints(trial.plan);
    Commit(trial.plan, stamp);
    frozen_.erase(std::remove_if(frozen_.begin(), frozen_.end(), [&](const auto& row) {
      return row.slot == edge[i];
    }), frozen_.end());
    geometry.Remove(removed);
    if (report) {
      ++counters.collapses_surface;
      counters.c_reverse_max = std::max(counters.c_reverse_max, trial.reverse_max);
      if (sink)
        for (auto face : trial.children) {
          std::rotate(face.v.begin(), std::find(face.v.begin(), face.v.end(), kept), face.v.end());
          sink(face);
        }
    }
    return Reason::None;
  }
  return (trials[0].failure_order > trials[1].failure_order ? trials[0] : trials[1]).refusal;
}
}  // namespace SU2NativeLocal3D
