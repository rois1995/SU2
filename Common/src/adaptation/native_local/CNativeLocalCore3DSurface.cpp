/*!
 * \file CNativeLocalCore3DSurface.cpp
 * \brief Validity-first native surface edge splits and deterministic sweeps.
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md).
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */
#include "../../../include/adaptation/CNativeLocalSurface3D.hpp"
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <ostream>
#include <set>
#include <stdexcept>
#include <tuple>

namespace SU2NativeLocal3D {
namespace {
constexpr unsigned SurfaceLengthLimit = 50, SurfaceDistanceLimit = 40;
using EdgeSet = std::set<SurfaceEdge>;
using EdgeUses = std::map<SurfaceEdge, EdgeUse>;

template <class T, size_t N>
std::array<T, N> Sorted(std::array<T, N> key) {
  std::sort(key.begin(), key.end());
  return key;
}

template <class V, class P>
bool Has(const V& vertices, P point) {
  return std::find(vertices.begin(), vertices.end(), point) != vertices.end();
}

double ReportClock(bool report) { return report ? SU2_MPI::Wtime() : 0.; }

constexpr std::pair<const char*, size_t SurfaceCounters::*> Counts[] = {
    {"faces_in", &SurfaceCounters::faces_in}, {"faces_out", &SurfaceCounters::faces_out},
    {"points_in", &SurfaceCounters::points_in}, {"points_out", &SurfaceCounters::points_out},
    {"splits_length", &SurfaceCounters::splits_length}, {"splits_distance", &SurfaceCounters::splits_distance},
    {"splits_feature", &SurfaceCounters::splits_feature}, {"sweeps_length", &SurfaceCounters::sweeps_length},
    {"sweeps_distance", &SurfaceCounters::sweeps_distance}, {"left_long", &SurfaceCounters::left_long},
    {"left_border", &SurfaceCounters::left_border}, {"left_distance", &SurfaceCounters::left_distance},
    {"left_all_feature", &SurfaceCounters::left_all_feature}, {"left_refused", &SurfaceCounters::left_refused},
    {"left_limit", &SurfaceCounters::left_limit}, {"feature_dominated", &SurfaceCounters::feature_dominated},
    {"refused_walk", &SurfaceCounters::refused_walk}, {"refused_protected", &SurfaceCounters::refused_protected},
    {"refused_feature", &SurfaceCounters::refused_feature}, {"refused_trace", &SurfaceCounters::refused_trace},
    {"refused_duplicate", &SurfaceCounters::refused_duplicate}, {"refused_outward", &SurfaceCounters::refused_outward},
    {"refused_size", &SurfaceCounters::refused_size}, {"refused_valid", &SurfaceCounters::refused_valid},
    {"refused_topology", &SurfaceCounters::refused_topology}, {"refused_memory", &SurfaceCounters::refused_memory},
    {"refused_entity", &SurfaceCounters::refused_entity}, {"chord_unknown", &SurfaceCounters::chord_unknown},
    {"samples", &SurfaceCounters::samples}, {"samples_unconverged", &SurfaceCounters::samples_unconverged}};

using Counter = size_t SurfaceCounters::*;
Counter RefusalCount(SurfaceRefusal refusal) {
  switch (refusal) {
    case SurfaceRefusal::None: return nullptr;
    case SurfaceRefusal::Border: return &SurfaceCounters::left_border;
    case SurfaceRefusal::Walk: return &SurfaceCounters::refused_walk;
    case SurfaceRefusal::Protected: return &SurfaceCounters::refused_protected;
    case SurfaceRefusal::Feature: return &SurfaceCounters::refused_feature;
    case SurfaceRefusal::Trace: return &SurfaceCounters::refused_trace;
    case SurfaceRefusal::Duplicate: return &SurfaceCounters::refused_duplicate;
    case SurfaceRefusal::Outward: return &SurfaceCounters::refused_outward;
    case SurfaceRefusal::Size: return &SurfaceCounters::refused_size;
    case SurfaceRefusal::Validity: return &SurfaceCounters::refused_valid;
    case SurfaceRefusal::Memory: return &SurfaceCounters::refused_memory;
    case SurfaceRefusal::Entity: return &SurfaceCounters::refused_entity;
    case SurfaceRefusal::Ineligible:
    case SurfaceRefusal::Topology: return &SurfaceCounters::refused_topology;
  }
  return nullptr;
}

struct Candidate {
  EdgeUse use;
  SurfaceEdge ids;
  double order;
  bool operator<(const Candidate& other) const { return std::tie(order, ids) < std::tie(other.order, other.ids); }
};

SurfaceEdge Ids(const std::vector<PointRow>& points, Edge edge) {
  return Sorted(SurfaceEdge{points[edge[0]].node.id, points[edge[1]].node.id});
}

double Distance(Point a, Point b) { return std::hypot(a.x - b.x, a.y - b.y, a.z - b.z); }

std::array<Edge, 3> FaceEdges(Face face) {
  return {{{face[0], face[1]}, {face[1], face[2]}, {face[2], face[0]}}};
}

double Height(Point a, Point b, Point c, Point m) {
  const Point ab{b.x - a.x, b.y - a.y, b.z - a.z}, ac{c.x - a.x, c.y - a.y, c.z - a.z};
  const Point n{ab.y * ac.z - ab.z * ac.y, ab.z * ac.x - ab.x * ac.z, ab.x * ac.y - ab.y * ac.x};
  return -(n.x * (m.x - a.x) + n.y * (m.y - a.y) + n.z * (m.z - a.z)) / std::hypot(n.x, n.y, n.z);
}

Tensor Mean(const Tensor& a, const Tensor& b) {
  return {0.5 * (a.xx + b.xx), 0.5 * (a.xy + b.xy), 0.5 * (a.xz + b.xz),
          0.5 * (a.yy + b.yy), 0.5 * (a.yz + b.yz), 0.5 * (a.zz + b.zz)};
}

template <class V, class P>
V Replace(V vertices, P old, P next) {
  *std::find(vertices.begin(), vertices.end(), old) = next;
  return vertices;
}

enum class Remaining { Limit, Border, Refused, AllFeature, FeatureDominated };

struct SampleRow {
  SurfaceSamples measured;
  Remaining reason = Remaining::Limit;
};

struct DistanceMark {
  Remaining reason = Remaining::Limit;
};
}  // namespace

void PrintSurfaceFields(std::ostream& out, const SurfaceCounters& s) {
  for (const auto& field : Counts) out << ' ' << field.first << '=' << s.*field.second;
  out << " outward_max=" << s.outward_max << " seconds_length=" << s.seconds_length
      << " seconds_distance=" << s.seconds_distance << " seconds_samples=" << s.seconds_samples
      << " seconds_place=" << s.seconds_place << " seconds_entity=" << s.seconds_entity;
  if (!s.first_feature_reason.empty()) out << " first_feature_reason=" << std::quoted(s.first_feature_reason);
  if (!s.first_trace_reason.empty()) out << " first_trace_reason=" << std::quoted(s.first_trace_reason);
}

std::vector<Mesh::PhysicalFace> Mesh::SurfaceParents(Edge edge, const Ring& ring) const {
  std::vector<PhysicalFace> parents;
  for (auto cell : ring.cells)
    for (unsigned k = 0; k < 4; ++k)
      if (cells_[cell].neighbors[k] == Physical && cells_[cell].vertices[k] != edge[0] &&
          cells_[cell].vertices[k] != edge[1])
        parents.push_back({cell, k});
  return parents;
}

std::vector<Mesh::PhysicalFace> Mesh::PhysicalFaces() const {
  std::vector<PhysicalFace> faces;
  for (Slot cell = 0; cell < cells_.size(); ++cell) {
    const auto& row = cells_[cell];
    if (!row.live || row.binding < 0) continue;
    for (unsigned k = 0; k < 4; ++k)
      if (row.neighbors[k] == Physical) faces.push_back({cell, k});
  }
  return faces;
}

SurfaceFace Mesh::SurfaceFaceAt(PhysicalFace parent) const {
  const auto vertices = OrientedFace(cells_[parent.cell].vertices, parent.face);
  return {{{points_[vertices[0]].node.id, points_[vertices[1]].node.id, points_[vertices[2]].node.id}},
          Bindings(parent.cell)[parent.face].facet};
}

SurfaceRefusal Mesh::SplitSurface(Edge edge, Slot seed, SurfaceGeometry& geometry, IdSequence& point_ids,
                                  uint32_t stamp, size_t cell_limit, SurfaceCounters& stats, bool progress_test,
                                  bool report) {
  const auto refuse = [&](SurfaceRefusal code) {
    if (report) ++(stats.*RefusalCount(code));
    return code;
  };
  edge = ById(edge);
  auto ring = EdgeRing(edge, seed);
  if (ring.border) return refuse(SurfaceRefusal::Border);
  const auto parents = SurfaceParents(edge, ring);
  if (parents.size() != 2) return refuse(SurfaceRefusal::Ineligible);
  if (!Fits(ring.cells.size(), 2 * ring.cells.size(), cell_limit)) return refuse(SurfaceRefusal::Memory);

  const auto& a = points_[edge[0]];
  const auto& b = points_[edge[1]];
  const std::array<Node, 2> ends{a.node, b.node};
  const std::array<SurfaceFace, 2> old_faces{SurfaceFaceAt(parents[0]), SurfaceFaceAt(parents[1])};
  const double place_start = ReportClock(report);
  auto placement = geometry.Place(ends, old_faces, point_ids.Peek());
  if (report) stats.seconds_place += ReportClock(report) - place_start;
  if (!placement.Accepted()) {
    if (report && placement.refusal == SurfaceRefusal::Feature && stats.first_feature_reason.empty())
      stats.first_feature_reason = placement.reason;
    if (report && placement.refusal == SurfaceRefusal::Trace && stats.first_trace_reason.empty())
      stats.first_trace_reason = placement.reason;
    return refuse(placement.refusal);
  }

  const auto& point = placement.point;
  double outward = 0;
  // Inside both parent half-spaces, positive children lie in the old cavity. An outward cap thinner
  // than tol cannot reach another resolved body part. The engine handles larger steps with contact checks.
  for (const auto& parent : parents) {
    const auto face = OrientedFace(cells_[parent.cell].vertices, parent.face);
    const double h = Height(points_[face[0]].node.p, points_[face[1]].node.p, points_[face[2]].node.p, point.node.p);
    if (!std::isfinite(h)) return refuse(SurfaceRefusal::Validity);
    if (h < -geometry.Tolerance()) return refuse(SurfaceRefusal::Outward);
    outward = std::max(outward, -h);
  }

  PointRow middle{point.node, Mean(a.tensor, b.tensor)};
  middle.surface = true;
  if (progress_test && options_.split_progress) {
    const auto limit = Length(edge) * (1 - 1e-12);
    if (!TrialEdgeShorter(a, middle, limit) || !TrialEdgeShorter(middle, b, limit))
      return refuse(SurfaceRefusal::Size);
    for (auto cell : ring.cells)
      for (auto p : cells_[cell].vertices)
        if (p != edge[0] && p != edge[1] && !TrialEdgeShorter(middle, points_[p], limit))
          return refuse(SurfaceRefusal::Size);
  }
  std::sort(ring.cells.begin(), ring.cells.end(),
            [this](Slot l, Slot r) { return Key(cells_[l].vertices) < Key(cells_[r].vertices); });
  const bool append = free_points_.empty();
  if (append && points_.size() == NoSlot) throw std::length_error("Too many native local core points.");
  const Slot slot = append ? static_cast<Slot>(points_.size()) : free_points_.back();
  std::vector<Vertices> fresh;
  fresh.reserve(2 * ring.cells.size());
  for (auto c : ring.cells)
    for (auto end : edge) fresh.push_back(Replace(cells_[c].vertices, end, slot));
  std::vector<ReplacedFace> replaced;
  for (const auto& parent : parents) replaced.push_back({parent.cell, parent.face, edge[0], edge[1], slot});

  PointRow previous;
  if (append) {
    points_.push_back(middle);
  } else {
    previous = points_[slot];
    points_[slot] = middle;
  }
  const auto restore = [&] {
    if (append) points_.pop_back();
    else points_[slot] = previous;
  };
  Plan plan;
  try {
    plan = Prepare(ring.cells, fresh, replaced);
    if (plan.refusal == Refusal::Validity || (plan.Accepted() && !ExactlyPositive(plan))) {
      restore();
      return refuse(SurfaceRefusal::Validity);
    }
    if (!plan.Accepted()) {
      restore();
      return refuse(SurfaceRefusal::Topology);
    }
    const auto entity_start = ReportClock(report);
    for (auto face : old_faces)
      for (const auto& end : ends) {
        auto child = face;
        child.v = Replace(child.v, end.id, point.node.id);
        if (!geometry.OnEntity(child, point)) {
          if (report) stats.seconds_entity += ReportClock(report) - entity_start;
          restore();
          return refuse(SurfaceRefusal::Entity);
        }
      }
    if (report) stats.seconds_entity += ReportClock(report) - entity_start;
  } catch (...) {
    restore();
    throw;
  }

  const bool feature = placement.parent.has_value();
  RememberWorkPoints(plan);
  if (!append) free_points_.pop_back();
  point_ids.Take();
  Commit(plan, stamp);
  geometry.Accept(std::move(placement));
  frozen_.push_back({slot, middle.node, middle.tensor});
  if (report) {
    stats.splits_feature += feature;
    stats.outward_max = std::max(stats.outward_max, outward);
  }
  return SurfaceRefusal::None;
}

void Mesh::SurfaceLength(SurfaceGeometry& geometry, IdSequence& point_ids, size_t cell_limit, PassStats& stats) {
  const auto begin = ReportClock(stats.report);
  const bool work_list = options_.work_list && options_.memory;
  std::vector<Edge> deferred;
  EdgeSet refused, border;
  for (unsigned sweep = 0; sweep < SurfaceLengthLimit; ++sweep) {
    ++sweep_;
    std::vector<EdgeUse> uses;
    if (work_list && sweep) {
      uses = NextEdges(deferred);
    } else {
      EdgeUses physical;
      for (const auto face : PhysicalFaces())
        for (const auto edge : FaceEdges(OrientedFace(cells_[face.cell].vertices, face.face)))
          physical.try_emplace(Ids(points_, edge), EdgeUse{ById(edge), face.cell});
      for (const auto& entry : physical) uses.push_back(entry.second);
    }

    std::vector<Candidate> candidates;
    for (const auto& use : uses) {
      const auto ids = Ids(points_, use.edge);
      if (refused.count(ids) || border.count(ids)) continue;
      const double length = Length(use.edge);
      if (length > LongEdge && EdgeRing(use.edge, use.cell).surface)
        candidates.push_back({use, ids, -length});
    }
    if (candidates.empty()) break;
    stats.Increment(stats.surface.sweeps_length);
    deferred.clear();
    work_points_.clear();
    work_start_ = work_list ? change_ : 0;
    std::sort(candidates.begin(), candidates.end());
    size_t accepted = 0;
    for (const auto& candidate : candidates) {
      const auto& [edge, seed] = candidate.use;
      const auto& row = cells_[seed];
      if (!row.live || row.stamp == sweep_ || !Has(row.vertices, edge[0]) || !Has(row.vertices, edge[1])) {
        if (work_list) deferred.push_back(edge);
        continue;
      }
      const auto ring = EdgeRing(edge, seed);
      if (std::any_of(ring.cells.begin(), ring.cells.end(), [this](Slot c) { return cells_[c].stamp == sweep_; })) {
        if (work_list) deferred.push_back(edge);
        continue;
      }
      const auto result = SplitSurface(edge, seed, geometry, point_ids, sweep_, cell_limit, stats.surface, false,
                                       stats.report);
      if (result == SurfaceRefusal::None) {
        ++accepted;
        stats.Increment(stats.surface.splits_length);
      } else {
        (result == SurfaceRefusal::Border ? border : refused).insert(candidate.ids);
      }
    }
    work_start_ = 0;
    if (!accepted) break;
  }
  if (stats.report) stats.surface.seconds_length = ReportClock(true) - begin;
}

void Mesh::SurfaceDistance(SurfaceGeometry& geometry, IdSequence& point_ids, size_t cell_limit, PassStats& stats) {
  const auto begin = ReportClock(stats.report);
  const double limit = geometry.Limit();
  EdgeSet refused, border;
  std::map<SurfaceEdge, std::optional<double>> chords;
  std::map<SurfaceFaceKey, SampleRow> samples;
  const auto sample = [&](const SurfaceFace& face, const SurfaceFaceKey& key) -> SampleRow& {
    auto [it, inserted] = samples.try_emplace(key);
    if (inserted) {
      const double start = ReportClock(stats.report);
      it->second.measured = geometry.Samples(face);
      if (stats.report) {
        stats.surface.seconds_samples += ReportClock(true) - start;
        stats.surface.samples += 4;
        stats.surface.samples_unconverged += it->second.measured.unconverged;
      }
    }
    return it->second;
  };
  const auto length = [&](Edge edge) {
    return Distance(points_[edge[0]].node.p, points_[edge[1]].node.p);
  };
  const auto above = [limit](const auto& distances) {
    return *std::max_element(distances.begin(), distances.end()) > limit;
  };

  for (unsigned sweep = 0; sweep < SurfaceDistanceLimit; ++sweep) {
    ++sweep_;
    const auto faces = PhysicalFaces();
    EdgeUses edges;
    for (const auto face : faces)
      for (auto edge : FaceEdges(OrientedFace(cells_[face.cell].vertices, face.face))) {
        edge = ById(edge);
        edges.try_emplace(Ids(points_, edge), EdgeUse{edge, face.cell});
      }
    std::map<SurfaceEdge, DistanceMark> marks;
    for (const auto& [ids, use] : edges) {
      if (!geometry.Feature(ids)) continue;
      auto [where, inserted] = chords.try_emplace(ids);
      if (inserted) {
        where->second = geometry.Chord({points_[use.edge[0]].node, points_[use.edge[1]].node});
        if (!where->second) stats.Increment(stats.surface.chord_unknown);
      }
      if (where->second && *where->second > limit && !refused.count(ids) && !border.count(ids))
        marks.try_emplace(ids);
    }
    std::vector<std::pair<SampleRow*, SurfaceEdge>> chosen;
    for (const auto parent : faces) {
      const auto face = SurfaceFaceAt(parent);
      auto& row = sample(face, face.v);
      const auto& values = row.measured.distances;
      if (!above(values)) continue;
      const auto uses = FaceEdges(OrientedFace(cells_[parent.cell].vertices, parent.face));
      std::array<SurfaceEdge, 3> ids{Ids(points_, uses[0]), Ids(points_, uses[1]), Ids(points_, uses[2])};
      bool chord = false, dominated = false, all_feature = true;
      const auto high_samples = std::count_if(values.begin(), values.end(), [limit](double v) { return v > limit; });
      std::optional<SurfaceEdge> best;
      double longest = 0;
      for (size_t k = 0; k < ids.size(); ++k) {
        if (geometry.Feature(ids[k])) {
          const auto value = chords.at(ids[k]);
          if (value && *value > limit) {
            if (border.count(ids[k])) row.reason = Remaining::Border;
            else if (refused.count(ids[k])) row.reason = Remaining::Refused;
            else chosen.emplace_back(&row, ids[k]);
            chord = true;
            break;
          }
          dominated |= high_samples == 1 && values[k + 1] > limit;
        } else {
          all_feature = false;
          if (refused.count(ids[k])) continue;
          const auto measure = length(uses[k]);
          if (!best || measure > longest || (measure == longest && ids[k] < *best)) {
            best = ids[k];
            longest = measure;
          }
        }
      }

      if (chord) continue;
      if (dominated) {
        row.reason = Remaining::FeatureDominated;
      } else if (!best) {
        row.reason = all_feature ? Remaining::AllFeature : Remaining::Refused;
      } else if (border.count(*best)) {
        row.reason = Remaining::Border;
      } else {
        marks.try_emplace(*best);
        chosen.emplace_back(&row, *best);
      }
    }
    if (marks.empty()) break;
    stats.Increment(stats.surface.sweeps_distance);
    std::vector<Candidate> ordered;
    for (const auto& entry : marks) {
      const auto use = edges.at(entry.first);
      ordered.push_back({use, entry.first, -length(use.edge)});
    }
    std::sort(ordered.begin(), ordered.end());
    for (const auto& candidate : ordered) {
      const auto& [edge, seed] = candidate.use;
      const auto& row = cells_[seed];
      if (!row.live || row.stamp == sweep_ || !Has(row.vertices, edge[0]) || !Has(row.vertices, edge[1])) continue;
      const auto ring = EdgeRing(edge, seed);
      if (std::any_of(ring.cells.begin(), ring.cells.end(), [this](Slot c) { return cells_[c].stamp == sweep_; }))
        continue;
      const auto result = SplitSurface(edge, seed, geometry, point_ids, sweep_, cell_limit, stats.surface, false,
                                       stats.report);
      if (result == SurfaceRefusal::None) {
        stats.Increment(stats.surface.splits_distance);
      } else if (result == SurfaceRefusal::Border) {
        border.insert(candidate.ids);
        marks.at(candidate.ids).reason = Remaining::Border;
      } else {
        refused.insert(candidate.ids);
        marks.at(candidate.ids).reason = Remaining::Refused;
      }
    }
    for (const auto& entry : chosen) entry.first->reason = marks.at(entry.second).reason;
  }

  if (stats.report) {
    for (const auto parent : PhysicalFaces()) {
      const auto face = SurfaceFaceAt(parent);
      const auto& row = sample(face, face.v);
      if (!above(row.measured.distances)) continue;
      ++stats.surface.left_distance;
      switch (row.reason) {
        case Remaining::Limit: ++stats.surface.left_limit; break;
        case Remaining::Refused: ++stats.surface.left_refused; break;
        case Remaining::AllFeature: ++stats.surface.left_all_feature; break;
        case Remaining::FeatureDominated: ++stats.surface.feature_dominated; break;
        case Remaining::Border: break;
      }
    }
    stats.surface.seconds_distance = ReportClock(true) - begin;
  }
}

void Mesh::MarkUnresolved() {
  for (auto& point : points_)
    if (!point.border) point.unresolved = false;
  for (const auto face : PhysicalFaces())
    for (const auto edge : FaceEdges(OrientedFace(cells_[face.cell].vertices, face.face)))
      if (Length(edge) > LongEdge)
        for (auto vertex : edge) points_[vertex].unresolved = true;
}

void Mesh::SurfaceWork(SurfaceGeometry& geometry, IdSequence& point_ids, size_t cell_limit, PassStats& stats) {
  const auto report = [&](size_t& faces, size_t& points) {
    if (!stats.report) return;
    faces = PhysicalFaces().size();
    points = std::count_if(points_.begin(), points_.end(), [](const auto& p) { return p.surface && !p.removed; });
  };
  report(stats.surface.faces_in, stats.surface.points_in);
  SurfaceLength(geometry, point_ids, cell_limit, stats);
  SurfaceDistance(geometry, point_ids, cell_limit, stats);
  MarkUnresolved();
  report(stats.surface.faces_out, stats.surface.points_out);
  if (stats.report) {
    EdgeSet long_edges;
    for (const auto face : PhysicalFaces())
      for (const auto edge : FaceEdges(OrientedFace(cells_[face.cell].vertices, face.face)))
        if (Length(edge) > LongEdge) long_edges.insert(Ids(points_, edge));
    stats.surface.left_long = long_edges.size();
  }
}
}  // namespace SU2NativeLocal3D
