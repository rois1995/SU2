/*!
 * \file native_handover_volume.inc
 * \brief Test-only FP2-B0 volume operations on a fixed surface.
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md).
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */
#include <algorithm>
#include <chrono>
#include <cmath>
#include <limits>
#include <optional>
#include <ostream>
#include <set>
#include <stdexcept>
#include <tuple>

namespace SU2NativeLocal3D {
namespace {

template <class Visit>
void VisitVolumeEdges(const Vertices& vertices, Visit visit) {
  for (unsigned a = 0; a < 4; ++a)
    for (unsigned b = a + 1; b < 4; ++b)
      visit(Edge{std::min(vertices[a], vertices[b]), std::max(vertices[a], vertices[b])});
}

Vertices ReplaceVolumeVertex(Vertices vertices, Slot from, Slot to) {
  *std::find(vertices.begin(), vertices.end(), from) = to;
  return vertices;
}

}  // namespace

void SurfaceFixtureAccess::VolumeRounds(std::ostream& out, Mesh& mesh, IdSequence& point_ids, size_t cell_limit,
                                       PassStats& stats, Id point_base, int rank, unsigned pass,
                                       const std::string& mode) {
  using Clock = std::chrono::steady_clock;
  using Candidate = std::tuple<double, std::array<Id, 4>, Slot>;
  using BoundaryFace = std::tuple<int32_t, std::array<Id, 3>, bool, Id>;
  const bool collapse = mode == "collapse" || mode == "both";
  const bool split = mode == "split" || mode == "both";
  if (!collapse && !split) throw std::invalid_argument("Unknown FP2-B0 volume experiment.");

  const auto boundary = [&] {
    std::vector<BoundaryFace> faces;
    for (Slot cell = 0; cell < mesh.cells_.size(); ++cell) {
      const auto& row = mesh.cells_[cell];
      if (!row.live) continue;
      for (unsigned face = 0; face < 4; ++face) {
        if (row.neighbors[face] >= 0) continue;
        const auto slots = OrientedFace(row.vertices, face);
        std::array<Id, 3> ids{mesh.points_[slots[0]].node.id, mesh.points_[slots[1]].node.id,
                              mesh.points_[slots[2]].node.id};
        std::rotate(ids.begin(), std::min_element(ids.begin(), ids.end()), ids.end());
        const auto binding = mesh.Bindings(cell)[face];
        faces.emplace_back(row.neighbors[face], ids, binding.present, binding.facet);
      }
    }
    std::sort(faces.begin(), faces.end());
    return faces;
  };
  const auto before = boundary();
  const auto fixed_points = mesh.frozen_.size();

  const auto candidates = [&] {
    std::vector<Candidate> rows;
    for (Slot cell = 0; cell < mesh.cells_.size(); ++cell) {
      const auto& row = mesh.cells_[cell];
      if (!row.live || row.stamp == mesh.sweep_ || BadClass(mesh, row.vertices, point_base) == 3) continue;
      const auto margin = mesh.Margin(row.vertices);
      if (margin < BadMargin) rows.emplace_back(margin, mesh.Key(row.vertices), cell);
    }
    std::sort(rows.begin(), rows.end());
    return rows;
  };
  const auto editable = [&](const std::vector<Slot>& old) {
    return !old.empty() && !mesh.Blocked(old) &&
           std::none_of(old.begin(), old.end(), [&](Slot cell) { return mesh.cells_[cell].stamp == mesh.sweep_; });
  };
  const auto old_edges = [&](const std::vector<Slot>& old) {
    std::set<Edge> edges;
    for (auto cell : old)
      VisitVolumeEdges(mesh.cells_[cell].vertices, [&](Edge edge) { edges.insert(edge); });
    return edges;
  };
  const auto improved = [&](const std::vector<Slot>& old, const std::vector<Vertices>& fresh,
                            const std::set<Edge>& edges, double worst_old) -> std::optional<double> {
    if (fresh.empty() || !mesh.Fits(old.size(), fresh.size(), cell_limit)) return std::nullopt;
    double worst_new = std::numeric_limits<double>::infinity();
    std::set<Edge> checked;
    for (const auto& vertices : fresh) {
      bool short_enough = true;
      VisitVolumeEdges(vertices, [&](Edge edge) {
        if (short_enough && !edges.count(edge) && checked.insert(edge).second && !(mesh.Length(edge) <= LongEdge))
          short_enough = false;
      });
      if (!short_enough) return std::nullopt;
      worst_new = std::min(worst_new, mesh.Margin(vertices));
      if (!(worst_new > worst_old)) return std::nullopt;
    }
    return worst_new;
  };
  const auto commit = [&](const std::vector<Slot>& old, const std::vector<Vertices>& fresh) {
    const auto plan = mesh.Prepare(old, fresh);
    if (!plan.Accepted() || !mesh.ExactlyPositive(plan)) return false;
    mesh.Commit(plan, mesh.sweep_);
    return true;
  };

  const auto split_cell = [&](Slot seed) {
    using SplitOption = std::tuple<double, std::array<Id, 2>, Edge>;
    std::vector<SplitOption> edges;
    VisitVolumeEdges(mesh.cells_[seed].vertices, [&](Edge edge) {
      edge = mesh.ById(edge);
      const auto& a = mesh.points_[edge[0]].node;
      const auto& b = mesh.points_[edge[1]].node;
      edges.emplace_back(-std::hypot(a.p.x - b.p.x, a.p.y - b.p.y, a.p.z - b.p.z),
                         std::array<Id, 2>{a.id, b.id}, edge);
    });
    std::sort(edges.begin(), edges.end());
    for (const auto& option : edges) {
      const auto& edge = std::get<2>(option);
      auto ring = mesh.EdgeRing(edge, seed);
      if (!ring.Closed() || !editable(ring.cells)) continue;
      std::sort(ring.cells.begin(), ring.cells.end(),
                [&](Slot a, Slot b) { return mesh.Key(mesh.cells_[a].vertices) < mesh.Key(mesh.cells_[b].vertices); });
      if (mesh.points_.size() == NoSlot) throw std::length_error("Too many FP2-B0 points.");
      const auto middle = static_cast<Slot>(mesh.points_.size());
      const auto& a = mesh.points_[edge[0]];
      const auto& b = mesh.points_[edge[1]];
      const auto& x = a.tensor;
      const auto& y = b.tensor;
      const Point p{0.5 * (a.node.p.x + b.node.p.x), 0.5 * (a.node.p.y + b.node.p.y),
                    0.5 * (a.node.p.z + b.node.p.z)};
      const Tensor mean{0.5 * (x.xx + y.xx), 0.5 * (x.xy + y.xy), 0.5 * (x.xz + y.xz),
                        0.5 * (x.yy + y.yy), 0.5 * (x.yz + y.yz), 0.5 * (x.zz + y.zz)};
      std::vector<Vertices> fresh;
      fresh.reserve(2 * ring.cells.size());
      for (auto cell : ring.cells)
        for (auto end : edge)
          fresh.push_back(ReplaceVolumeVertex(mesh.cells_[cell].vertices, end, middle));

      const auto edges_before = old_edges(ring.cells);
      const auto worst_before = mesh.Worst(ring.cells);
      mesh.points_.push_back({{point_ids.Peek(), p}, mean});
      Mesh::Plan plan;
      try {
        if (improved(ring.cells, fresh, edges_before, worst_before)) plan = mesh.Prepare(ring.cells, fresh);
        if (plan.Accepted() && !mesh.ExactlyPositive(plan)) plan.refusal = Refusal::Validity;
      } catch (...) {
        mesh.points_.pop_back();
        throw;
      }
      if (plan.Accepted()) {
        mesh.Commit(plan, mesh.sweep_);
        point_ids.Take();
        return true;
      }
      mesh.points_.pop_back();
    }
    return false;
  };

  const auto collapse_cell = [&](Slot seed) {
    auto vertices = mesh.cells_[seed].vertices;
    std::sort(vertices.begin(), vertices.end(),
              [&](Slot a, Slot b) { return mesh.points_[a].node.id < mesh.points_[b].node.id; });
    for (auto remove : vertices) {
      const auto& point = mesh.points_[remove];
      if (!point.Free() || !point.wall_near) continue;
      const auto old = mesh.Star(remove);
      if (!editable(old)) continue;
      std::set<Slot> neighbors;
      for (auto cell : old)
        for (auto neighbor : mesh.cells_[cell].vertices)
          if (neighbor != remove) neighbors.insert(neighbor);
      const auto make_fresh = [&](Slot keep) {
        std::vector<Vertices> fresh;
        fresh.reserve(old.size());
        for (auto cell : old) {
          const auto& cell_vertices = mesh.cells_[cell].vertices;
          if (std::find(cell_vertices.begin(), cell_vertices.end(), keep) == cell_vertices.end())
            fresh.push_back(ReplaceVolumeVertex(cell_vertices, remove, keep));
        }
        return fresh;
      };
      const auto edges_before = old_edges(old);
      const auto worst_before = mesh.Worst(old);
      std::vector<std::tuple<double, Id, Slot>> keeps;
      for (auto keep : neighbors) {
        const auto score = improved(old, make_fresh(keep), edges_before, worst_before);
        if (score) keeps.emplace_back(-*score, mesh.points_[keep].node.id, keep);
      }
      std::sort(keeps.begin(), keeps.end());
      for (const auto& choice : keeps)
        if (commit(old, make_fresh(std::get<2>(choice)))) return true;
    }
    return false;
  };

  const auto sweep = [&](const auto& operation) {
    size_t accepted = 0;
    for (const auto& candidate : candidates()) {
      const auto cell = std::get<2>(candidate);
      if (!mesh.cells_[cell].live || mesh.cells_[cell].stamp == mesh.sweep_) continue;
      accepted += operation(cell);
    }
    return accepted;
  };
  for (unsigned round = 1; round <= 10; ++round) {
    const auto start = Clock::now();
    ++mesh.sweep_;
    mesh.MarkWallNear();
    const size_t collapsed = collapse ? sweep(collapse_cell) : 0;
    const size_t split_count = split ? sweep(split_cell) : 0;
    const auto previous_quality = stats.sweeps_quality;
    mesh.QualityWork(point_ids, cell_limit, stats);
    out << "FP2_B0 volume_round rank=" << rank << " pass=" << pass << " round=" << round
        << " collapses=" << collapsed << " splits=" << split_count << " cells=" << mesh.live_cells_
        << " quality_sweeps=" << stats.sweeps_quality - previous_quality
        << " seconds=" << std::chrono::duration<double>(Clock::now() - start).count() << std::endl;
    if (collapsed == 0 && split_count == 0) break;
  }
  if (before != boundary() || fixed_points != mesh.frozen_.size())
    throw std::runtime_error("FP2-B0 volume rounds changed the fixed boundary.");
  const auto faces =
      std::count_if(before.begin(), before.end(), [](const auto& face) { return std::get<0>(face) == Physical; });
  out << "FP2_B0 volume_boundary rank=" << rank << " pass=" << pass << " faces=" << faces
      << " border_faces=" << before.size() - faces << " fixed_points=" << fixed_points << " unchanged=1" << std::endl;
}

}  // namespace SU2NativeLocal3D
