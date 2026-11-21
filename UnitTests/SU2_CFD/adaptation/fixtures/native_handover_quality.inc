/*!
 * \file native_handover_quality.inc
 * \brief Read-only quality-option measurements for the local-core handover lab.
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md).
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */
#include <algorithm>
#include <array>
#include <cstring>
#include <ostream>
#include <stdexcept>
#include <type_traits>
#include <vector>

// Match the core's floating-point flags for the move probe's centroid and trial coordinates.
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC push_options
#pragma GCC optimize("no-fast-math", "no-unsafe-math-optimizations", "fp-contract=off")
#endif

namespace SU2NativeLocal3D {
namespace {
enum class QualityReason {
  Accepted,
  Physical,
  RankBorder,
  RingSmall,
  RingLarge,
  Blocked,
  Capacity,
  RimTopology,
  LongDiagonal,
  FloatNonpositive,
  DpTopology,
  NoRise,
  Prepare,
  ExactNonpositive,
  LongEdge,
  SurfacePoint,
  BorderPoint,
  EmptyStar,
  NoTensorSample,
  Count
};

constexpr std::array<const char*, static_cast<size_t>(QualityReason::Count)> QualityReasons{
    "accepted", "physical", "rank_border", "ring_lt3", "ring_gt7", "blocked", "fits", "rim_topology",
    "long_diagonal", "float_nonpositive", "dp_topology", "no_rise", "prepare", "exact_nonpositive",
    "long_edge", "surface_point", "border_point", "empty_star", "no_tensor_sample"};

size_t ReasonIndex(QualityReason reason) { return static_cast<size_t>(reason); }

struct OptionCounts {
  std::array<size_t, QualityReasons.size()> options{}, only_cells{};
  size_t accepted_cells = 0;

  template <size_t N>
  void Add(const std::array<QualityReason, N>& reasons) {
    for (const auto reason : reasons) ++options[ReasonIndex(reason)];
    if (std::all_of(reasons.begin(), reasons.end(), [&](auto reason) { return reason == reasons.front(); }))
      ++only_cells[ReasonIndex(reasons.front())];
    accepted_cells += std::find(reasons.begin(), reasons.end(), QualityReason::Accepted) != reasons.end();
  }
};

struct QualityClassCounts {
  size_t cells = 0, no_free = 0, four_surface = 0, no_closed_edge = 0;
  std::array<OptionCounts, 3> operations;
};

template <size_t N>
void PrintReasons(std::ostream& out, const char* label, const std::array<QualityReason, N>& reasons) {
  out << ' ' << label << '=';
  for (size_t i = 0; i < N; ++i) {
    if (i) out << ',';
    out << QualityReasons[ReasonIndex(reasons[i])];
  }
}

class PointProbe {
 public:
  PointProbe(PointRow& row, const PointRow& trial) : row(row) {
    static_assert(std::is_trivially_copyable<PointRow>::value, "The temporary point needs byte-exact restoration.");
    std::memcpy(original.data(), &row, sizeof(row));
    row = trial;
  }

  ~PointProbe() { std::memcpy(&row, original.data(), sizeof(row)); }

  PointProbe(const PointProbe&) = delete;
  PointProbe& operator=(const PointProbe&) = delete;

 private:
  PointRow& row;
  std::array<unsigned char, sizeof(PointRow)> original;
};
}  // namespace

void SurfaceFixtureAccess::ReportQualityOptions(std::ostream& out, Mesh& mesh, size_t cell_limit, Id point_base,
                                                unsigned pass, const char* checkpoint, int rank) {
  using Reason = QualityReason;
  if (!mesh.options_.float_trials) throw std::logic_error("Handover quality probes require the frozen V2 options.");

  // A failed plan can collapse a DP and a Prepare refusal to the same enum. Test DP feasibility only to disambiguate.
  const auto can_triangulate = [&](Edge edge, const std::vector<Slot>& rim) {
    const auto n = rim.size();
    const auto too_long = [&](size_t i, size_t j) {
      return i + 1 < j && (i != 0 || j != n - 1) && mesh.Length({rim[i], rim[j]}) > LongEdge;
    };
    bool feasible[7][7]{};
    for (size_t i = 0; i + 1 < n; ++i) feasible[i][i + 1] = true;
    for (size_t width = 2; width < n; ++width) {
      for (size_t i = 0, j = width; j < n; ++i, ++j) {
        if (too_long(i, j)) continue;
        for (size_t k = i + 1; k < j; ++k) {
          if (!feasible[i][k] || !feasible[k][j]) continue;
          if (!mesh.TrialPositive({rim[i], rim[k], rim[j], edge[1]}) ||
              !mesh.TrialPositive({rim[k], rim[i], rim[j], edge[0]}))
            continue;
          feasible[i][j] = true;
          break;
        }
      }
    }
    return feasible[0][n - 1];
  };

  const auto edge_reason = [&](Edge edge, Slot seed, bool& has_closed_edge) {
    edge = mesh.ById(edge);
    const auto ring = mesh.EdgeRing(edge, seed);
    has_closed_edge |= ring.Closed();
    if (ring.surface) return Reason::Physical;
    if (ring.border) return Reason::RankBorder;
    const auto n = ring.cells.size();
    if (n < 3) return Reason::RingSmall;
    if (n > 7) return Reason::RingLarge;
    if (mesh.Blocked(ring.cells)) return Reason::Blocked;
    if (!mesh.Fits(n, 2 * (n - 2), cell_limit)) return Reason::Capacity;
    const auto rim = mesh.OrderedRim(edge, ring);
    if (rim.size() != n) return Reason::RimTopology;

    const auto plan = mesh.RemoveEdgePlan(edge, seed, cell_limit);
    switch (plan.refusal) {
      case Refusal::None: return mesh.ExactlyPositive(plan) ? Reason::Accepted : Reason::ExactNonpositive;
      case Refusal::Ineligible: return Reason::NoRise;
      case Refusal::Size: return Reason::LongDiagonal;
      case Refusal::Validity: return can_triangulate(edge, rim) ? Reason::Prepare : Reason::FloatNonpositive;
      case Refusal::Topology: return can_triangulate(edge, rim) ? Reason::Prepare : Reason::DpTopology;
      default: throw std::logic_error("Unexpected edge-removal plan refusal after the prechecks.");
    }
  };

  const auto swap_reason = [&](Slot cell, unsigned face) {
    const auto& a = mesh.cells_[cell];
    if (a.neighbors[face] == Physical) return Reason::Physical;
    if (a.neighbors[face] == Border) return Reason::RankBorder;
    const auto& b = mesh.cells_[a.neighbors[face]];
    if (mesh.Blocked(a.vertices) || mesh.Blocked(b.vertices)) return Reason::Blocked;
    const auto plan = mesh.SwapFacePlan(cell, face, cell_limit);
    switch (plan.refusal) {
      case Refusal::None: return mesh.ExactlyPositive(plan) ? Reason::Accepted : Reason::ExactNonpositive;
      case Refusal::Ineligible: return Reason::NoRise;
      case Refusal::Size: return Reason::LongEdge;
      case Refusal::Memory: return Reason::Capacity;
      case Refusal::Validity:
      case Refusal::Topology: return Reason::Prepare;
      default: throw std::logic_error("Unexpected face-swap plan refusal after the prechecks.");
    }
  };

  const auto move_reason = [&](Slot point) {
    const auto original = mesh.points_[point];
    if (!original.Free()) return original.surface ? Reason::SurfacePoint : Reason::BorderPoint;
    const auto star = mesh.Star(point);
    if (star.empty()) return Reason::EmptyStar;
    if (mesh.Blocked(star)) return Reason::Blocked;
    const auto before = mesh.Worst(star);
    std::vector<Slot> neighbors;
    std::vector<DonorCell> donors;
    donors.reserve(star.size());
    for (const auto cell : star) {
      const auto& row = mesh.cells_[cell];
      DonorCell donor;
      donor.cell.id = row.id;
      for (unsigned k = 0; k < 4; ++k) {
        const auto p = row.vertices[k];
        donor.cell.v[k] = mesh.points_[p].node;
        donor.sensor[k] = mesh.points_[p].tensor;
        if (p != point) neighbors.push_back(p);
      }
      donors.push_back(donor);
    }
    std::sort(neighbors.begin(), neighbors.end(),
              [&](Slot a, Slot b) { return mesh.points_[a].node.id < mesh.points_[b].node.id; });
    neighbors.erase(std::unique(neighbors.begin(), neighbors.end()), neighbors.end());
    std::array<long double, 3> mean{};
    for (const auto neighbor : neighbors) {
      const auto p = mesh.points_[neighbor].node.p;
      mean[0] += p.x;
      mean[1] += p.y;
      mean[2] += p.z;
    }
    for (auto& value : mean) value /= neighbors.size();

    const auto p = original.node.p;
    auto reason = Reason::NoRise;
    for (long double step : {1.L, .5L, .25L}) {
      auto candidate = original;
      candidate.node.p = {double(p.x + step * (mean[0] - p.x)), double(p.y + step * (mean[1] - p.y)),
                          double(p.z + step * (mean[2] - p.z))};
      if (!SampleMovedTensorFloat(donors, candidate.node.p, candidate.tensor)) {
        reason = Reason::NoTensorSample;
        continue;
      }
      const PointProbe trial(mesh.points_[point], candidate);
      if (std::any_of(star.begin(), star.end(), [&](Slot c) { return !mesh.TrialPositive(mesh.cells_[c].vertices); }))
        reason = Reason::FloatNonpositive;
      else if (std::any_of(neighbors.begin(), neighbors.end(),
                           [&](Slot n) { return mesh.Length({point, n}) > LongEdge; }))
        reason = Reason::LongEdge;
      else if (!(mesh.Worst(star) > before))
        reason = Reason::NoRise;
      else
        return std::all_of(star.begin(), star.end(), [&](Slot c) { return mesh.Positive(mesh.cells_[c].vertices); })
                   ? Reason::Accepted : Reason::ExactNonpositive;
    }
    return reason;
  };

  std::array<QualityClassCounts, 4> classes{};
  const auto prefix = [&](const char* row, unsigned group) {
    out << "FP2_B0 " << row << " rank=" << rank << " pass=" << pass << " checkpoint=" << checkpoint
        << " class=" << "abcd"[group];
  };
  for (Slot cell = 0; cell < mesh.cells_.size(); ++cell) {
    const auto& row = mesh.cells_[cell];
    if (!row.live || mesh.Margin(row.vertices) >= BadMargin) continue;
    const auto group = BadClass(mesh, row.vertices, point_base);
    auto& counts = classes.at(group);
    auto vertices = row.vertices;
    std::sort(vertices.begin(), vertices.end(),
              [&](Slot a, Slot b) { return mesh.points_[a].node.id < mesh.points_[b].node.id; });
    std::array<Reason, 6> edges;
    std::array<Reason, 4> swaps, moves;
    bool has_closed_edge = false;
    for (size_t i = 0, n = 0; i < vertices.size(); ++i)
      for (size_t j = i + 1; j < vertices.size(); ++j)
        edges[n++] = edge_reason({vertices[i], vertices[j]}, cell, has_closed_edge);
    for (size_t i = 0; i < vertices.size(); ++i) {
      const auto opposite = vertices[vertices.size() - 1 - i];
      const auto face = std::find(row.vertices.begin(), row.vertices.end(), opposite) - row.vertices.begin();
      swaps[i] = swap_reason(cell, face);
    }
    for (size_t i = 0; i < vertices.size(); ++i) moves[i] = move_reason(vertices[i]);

    const bool no_free = std::none_of(vertices.begin(), vertices.end(), [&](Slot p) { return mesh.points_[p].Free(); });
    const bool four_surface = no_free &&
        std::all_of(vertices.begin(), vertices.end(), [&](Slot p) { return mesh.points_[p].surface; });
    const bool no_closed_edge = four_surface && !has_closed_edge;
    ++counts.cells;
    counts.no_free += no_free;
    counts.four_surface += four_surface;
    counts.no_closed_edge += no_closed_edge;
    counts.operations[0].Add(edges);
    counts.operations[1].Add(swaps);
    counts.operations[2].Add(moves);

    prefix("quality_cell", group);
    out << " cell=" << cell << " vertices=";
    for (size_t i = 0; i < vertices.size(); ++i) {
      if (i) out << ',';
      out << mesh.points_[vertices[i]].node.id;
    }
    PrintReasons(out, "edge", edges);
    PrintReasons(out, "swap", swaps);
    PrintReasons(out, "move", moves);
    out << " no_free=" << unsigned(no_free) << " four_surface=" << unsigned(four_surface)
        << " no_closed_edge=" << unsigned(no_closed_edge) << '\n';
  }

  const std::array<const char*, 3> operations{"edge", "swap", "move"};
  for (unsigned group = 0; group < classes.size(); ++group) {
    const auto& counts = classes[group];
    prefix("quality_summary", group);
    out << " cells=" << counts.cells << " no_free=" << counts.no_free << " four_surface=" << counts.four_surface
        << " no_closed_edge=" << counts.no_closed_edge;
    for (size_t op = 0; op < operations.size(); ++op) {
      const auto& rows = counts.operations[op];
      out << ' ' << operations[op] << "_accepted_options=" << rows.options[ReasonIndex(Reason::Accepted)]
          << ' ' << operations[op] << "_accepted_cells=" << rows.accepted_cells;
    }
    out << '\n';
    for (size_t op = 0; op < operations.size(); ++op) {
      const auto& rows = counts.operations[op];
      for (size_t reason = 0; reason < QualityReasons.size(); ++reason) {
        if (!rows.options[reason]) continue;
        prefix("quality_option", group);
        out << " operation=" << operations[op] << " reason=" << QualityReasons[reason]
            << " only_cells=" << rows.only_cells[reason] << " options=" << rows.options[reason] << '\n';
      }
    }
  }
}
}  // namespace SU2NativeLocal3D

#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC pop_options
#endif
