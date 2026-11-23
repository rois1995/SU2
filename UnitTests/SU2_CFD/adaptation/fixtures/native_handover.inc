/*!
 * \file native_handover.inc
 * \brief Read-only native local hand-over measurements on frozen test inputs.
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md).
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */
#include <limits>
#include <tuple>

namespace {
namespace fp2_b0 {
using Access = local::SurfaceFixtureAccess;
using local::CellRow;
using local::EdgeUse;
using local::Mesh;
using local::PointRow;
using local::Slot;

constexpr char ClassName[] = "abcd";
constexpr const char* GroupName[] = {"surface", "border", "free"};
constexpr const char* SplitPath[] = {"L", "D_face", "D_chord"};
constexpr const char* RatioName[] = {"am", "mb", "rim"};

std::ostream& Row(std::ostream& out, const char* kind, int rank, unsigned pass, const char* checkpoint = nullptr) {
  out << "FP2_B0 " << kind << " rank=" << rank << " pass=" << pass;
  if (checkpoint) out << " checkpoint=" << checkpoint;
  return out;
}

double Space(const Point& a, const Point& b) {
  return std::hypot(a.x - b.x, a.y - b.y, a.z - b.z);
}

unsigned Group(const PointRow& point) { return point.surface ? 0 : point.border ? 1 : 2; }

const char* Origin(const PointRow& point, Id base) {
  if (point.node.id <= base) return "input";
  return point.surface ? "surface_split" : "other_new";
}

void Coordinate(std::ostream& out, const Point& p) { out << p.x << ',' << p.y << ',' << p.z; }

void Endpoint(std::ostream& out, const char* name, const Node& node) {
  out << ' ' << name << '=' << node.id << ' ' << name << "_xyz=";
  Coordinate(out, node.p);
}

std::vector<size_t> LiveIncidence(const Mesh& mesh) {
  std::vector<size_t> counts(mesh.Points().size());
  for (const auto& cell : mesh.Cells())
    if (cell.live)
      for (const auto slot : cell.vertices) ++counts[slot];
  return counts;
}

std::pair<unsigned, int> ShapeBins(const Mesh& mesh, const CellRow& cell) {
  const auto& v = cell.vertices;
  const auto& points = mesh.Points();
  const auto p = [&](Slot slot) -> const Point& { return points[slot].node.p; };
  double low = std::numeric_limits<double>::infinity(), high = 0;
  for (size_t i = 0; i < v.size(); ++i)
    for (size_t j = i + 1; j < v.size(); ++j) {
      const auto length = Space(p(v[i]), p(v[j]));
      low = std::min(low, length);
      high = std::max(high, length);
    }
  const auto ratio = low / high;
  double height = std::numeric_limits<double>::infinity();
  bool physical = false;
  for (unsigned k = 0; k < v.size(); ++k) {
    if (cell.neighbors[k] != local::Physical) continue;
    physical = true;
    const auto face = local::OrientedFace(v, k);
    const auto& a = p(face[0]);
    const auto& b = p(face[1]);
    const auto& c = p(face[2]);
    const auto& d = p(v[k]);
    const Point ab{b.x - a.x, b.y - a.y, b.z - a.z}, ac{c.x - a.x, c.y - a.y, c.z - a.z};
    const Point normal{ab.y * ac.z - ab.z * ac.y, ab.z * ac.x - ab.x * ac.z, ab.x * ac.y - ab.y * ac.x};
    const double face_length = std::max({Space(a, b), Space(b, c), Space(c, a)});
    const auto distance = std::abs(normal.x * (d.x - a.x) + normal.y * (d.y - a.y) + normal.z * (d.z - a.z)) /
                          std::hypot(normal.x, normal.y, normal.z);
    height = std::min(height, distance / face_length);
  }
  return {ratio < .1 ? 0 : ratio <= .3 ? 1 : 2, !physical ? -1 : height < .3 ? 0 : height <= 3. ? 1 : 2};
}

size_t ReportBad(std::ostream& out, const Mesh& mesh, Id base, int rank, unsigned pass, const char* checkpoint,
                 uint32_t before, uint32_t after, bool final_checks) {
  std::array<size_t, 4> counts{}, remembered{}, blocked{};
  std::array<std::array<size_t, 5>, 4> margins{};
  std::map<std::tuple<unsigned, unsigned, bool, bool, unsigned, int>, size_t> bins;
  size_t cells = 0;
  for (const auto& cell : mesh.Cells()) {
    if (!cell.live) continue;
    ++cells;
    const auto margin = mesh.Margin(cell.vertices);
    if (!(margin < local::BadMargin)) continue;
    const auto group = Access::BadClass(mesh, cell.vertices, base);
    ++counts[group];
    const auto surface_points = std::count_if(cell.vertices.begin(), cell.vertices.end(),
                                             [&](Slot p) { return mesh.Points()[p].surface; });
    const bool physical = std::find(cell.neighbors.begin(), cell.neighbors.end(), local::Physical) != cell.neighbors.end();
    const auto [shape, height] = ShapeBins(mesh, cell);
    ++bins[{group, surface_points, physical, cell.stamp > before && cell.stamp <= after, shape, height}];
    if (final_checks) {
      remembered[group] += cell.failed_at != 0 && std::all_of(cell.vertices.begin(), cell.vertices.end(), [&](Slot p) {
        return cell.failed_at >= mesh.Points()[p].changed_at;
      });
      blocked[group] += Access::HandoverBlocked(mesh, cell.vertices);
      ++margins[group][margin < .01 ? 0 : margin < .1 ? 1 : margin < .5 ? 2 : margin < 1. ? 3 : 4];
    }
  }
  Row(out, "checkpoint", rank, pass, checkpoint)
      << " cells=" << cells << " bad=" << counts[0] + counts[1] + counts[2] + counts[3]
      << " bad_a=" << counts[0] << " bad_b=" << counts[1] << " bad_c=" << counts[2] << " bad_d=" << counts[3] << '\n';
  for (const auto& [key, count] : bins) {
    const auto [group, surfaces, physical, made, shape, height] = key;
    Row(out, "bad_cells", rank, pass, checkpoint)
        << " class=" << ClassName[group] << " surface_points=" << surfaces << " physical=" << physical
        << " made_here=" << made << " shape_bin=" << shape << " height_bin=" << height << " count=" << count << '\n';
  }
  if (final_checks) {
    for (unsigned group = 0; group < counts.size(); ++group) {
      Row(out, "bad_c3", rank, pass, checkpoint)
          << " class=" << ClassName[group] << " remembered=" << remembered[group] << " blocked=" << blocked[group];
      for (unsigned k = 0; k < margins[group].size(); ++k) out << " margin_" << k << '=' << margins[group][k];
      out << '\n';
    }
  }
  return counts[3];
}

void ReportStars(std::ostream& out, const Mesh& mesh, Id base, int rank, unsigned pass, const char* checkpoint,
                 bool largest) {
  const auto& points = mesh.Points();
  const auto incidence = LiveIncidence(mesh);
  std::array<std::array<size_t, 4>, 3> bins{};
  std::vector<Slot> slots;
  for (Slot slot = 0; slot < points.size(); ++slot) {
    const auto count = incidence[slot];
    if (!count || points[slot].removed) continue;
    ++bins[Group(points[slot])][count <= 50 ? 0 : count <= 100 ? 1 : count <= 200 ? 2 : 3];
    if (largest) slots.push_back(slot);
  }
  for (unsigned group = 0; group < bins.size(); ++group) {
    const auto& row = bins[group];
    Row(out, "stars", rank, pass, checkpoint) << " group=" << GroupName[group] << " n1_50=" << row[0]
        << " n51_100=" << row[1] << " n101_200=" << row[2] << " over200=" << row[3] << '\n';
  }
  const auto limit = std::min(size_t{20}, slots.size());
  std::partial_sort(slots.begin(), slots.begin() + limit, slots.end(), [&](Slot a, Slot b) {
    if (incidence[a] != incidence[b]) return incidence[a] > incidence[b];
    return points[a].node.id < points[b].node.id;
  });
  for (size_t i = 0; i < limit; ++i) {
    const auto slot = slots[i];
    const auto& point = points[slot];
    Row(out, "star_top", rank, pass, checkpoint) << " order=" << i + 1;
    Endpoint(out, "point", point.node);
    out << " group=" << GroupName[Group(point)] << " origin=" << Origin(point, base) << " cells=" << incidence[slot]
        << '\n';
  }
}

local::SurfaceEdge SortedIds(Id a, Id b) { return {std::min(a, b), std::max(a, b)}; }

void ReportEdges(std::ostream& out, const Mesh& mesh, Id base, int rank, unsigned pass, const char* checkpoint) {
  struct MeasuredEdge {
    EdgeUse use;
    local::SurfaceEdge ids;
    double metric;
  };
  std::array<size_t, 4> counts{};
  std::vector<MeasuredEdge> edges;
  const auto& points = mesh.Points();
  for (const auto& use : mesh.Edges()) {
    const auto length = mesh.Length(use.edge);
    counts[0] += length < 1e-6;
    counts[1] += length < 1e-4;
    counts[2] += length < 1e-2;
    counts[3] += length < .1;
    edges.push_back({use, SortedIds(points[use.edge[0]].node.id, points[use.edge[1]].node.id), length});
  }
  Row(out, "edge_bins", rank, pass, checkpoint) << " lt_1e_6=" << counts[0] << " lt_1e_4=" << counts[1]
      << " lt_1e_2=" << counts[2] << " lt_01=" << counts[3] << '\n';
  const auto top = std::min(size_t{20}, edges.size());
  std::partial_sort(edges.begin(), edges.begin() + top, edges.end(), [](const auto& a, const auto& b) {
    return std::tie(a.metric, a.ids) < std::tie(b.metric, b.ids);
  });
  std::set<local::SurfaceEdge> physical;
  for (const auto& cell : mesh.Cells()) {
    if (!cell.live) continue;
    for (unsigned k = 0; k < 4; ++k) {
      if (cell.neighbors[k] != local::Physical) continue;
      const auto face = local::OrientedFace(cell.vertices, k);
      for (unsigned i = 0; i < 3; ++i)
        physical.insert(SortedIds(points[face[i]].node.id, points[face[(i + 1) % 3]].node.id));
    }
  }
  for (size_t i = 0; i < top; ++i) {
    const auto& row = edges[i];
    const auto& a = points[row.use.edge[0]];
    const auto& b = points[row.use.edge[1]];
    Row(out, "edge", rank, pass, checkpoint)
        << " order=" << i + 1 << " metric=" << row.metric << " space=" << Space(a.node.p, b.node.p);
    Endpoint(out, "a", a.node);
    Endpoint(out, "b", b.node);
    out << " a_group=" << GroupName[Group(a)] << " b_group=" << GroupName[Group(b)] << " a_origin=" << Origin(a, base)
        << " b_origin=" << Origin(b, base) << " physical=" << physical.count(row.ids) << '\n';
  }
}

struct SplitRecord {
  std::array<Node, 2> ends;
  Node middle{};
  unsigned path = 0, longest = 0;
  bool feature = false;
  std::array<double, 3> ratios{};
};

void PrintSplit(std::ostream& out, const SplitRecord& record) {
  out << " kind=" << (record.feature ? "feature" : "sheet") << " path=" << SplitPath[record.path]
      << " longest=" << record.longest;
  Endpoint(out, "a", record.ends[0]);
  Endpoint(out, "b", record.ends[1]);
  Endpoint(out, "m", record.middle);
  for (unsigned axis = 0; axis < record.ratios.size(); ++axis)
    out << ' ' << RatioName[axis] << '=' << record.ratios[axis];
}

class RecordingSurface final : public local::SurfaceGeometry {
 public:
  RecordingSurface(Mesh& mesh, local::StateSurfaceGeometry& geometry)
      : mesh_(mesh), geometry_(geometry), before_(LiveIncidence(mesh)), growth_(before_.size()) {}

  double Tolerance() const override { return geometry_.Tolerance(); }
  bool Feature(const local::SurfaceEdge& edge) const override { return geometry_.Feature(edge); }
  unsigned Dimension(Id id) const override { return geometry_.Dimension(id); }
  void Remove(Id id) override { geometry_.Remove(id); }
  bool OnEntityFace(const SurfaceFace& face) const override { return geometry_.OnEntityFace(face); }
  bool WithinTolerance(const SurfaceFace& face) override { return geometry_.WithinTolerance(face); }
  local::SurfaceSamples Samples(const SurfaceFace& face) override { return geometry_.Samples(face); }
  std::optional<double> Chord(const std::array<Node, 2>& ends) override {
    const auto chord = geometry_.Chord(ends);
    chords_[SortedIds(ends[0].id, ends[1].id)] = chord;
    return chord;
  }

  local::SurfacePlacement Place(const std::array<Node, 2>& ends, const std::array<SurfaceFace, 2>& faces,
                                Id id) override {
    last_ = SplitRecord{};
    last_.ends = ends;
    const auto edge = SortedIds(ends[0].id, ends[1].id);
    const auto chord = chords_.find(edge);
    const bool marked_chord = distance_ && Feature(edge) && chord != chords_.end() && chord->second &&
                              *chord->second > Limit();
    last_.path = !distance_ ? 0 : marked_chord ? 2 : 1;
    const auto length = Space(ends[0].p, ends[1].p);
    for (const auto& face : faces) {
      const auto third = std::find_if(face.v.begin(), face.v.end(), [&](Id candidate) {
        return candidate != ends[0].id && candidate != ends[1].id;
      });
      if (third == face.v.end()) throw std::logic_error("Missing physical third vertex in split probe.");
      const auto* point = geometry_.Find(*third);
      if (!point) throw std::logic_error("Missing surface binding in split probe.");
      last_.longest += length >= Space(ends[0].p, point->node.p) && length >= Space(ends[1].p, point->node.p);
    }
    return geometry_.Place(ends, faces, id);
  }

  bool OnEntity(const SurfaceFace& face, const BoundaryPoint& middle) override {
    return geometry_.OnEntity(face, middle);
  }

  void Accept(local::SurfacePlacement&& placement) override {
    last_.feature = placement.parent.has_value();
    last_.middle = placement.point.node;
    const auto& a = last_.ends[0];
    const auto& b = last_.ends[1];
    const auto& m = last_.middle;
    const auto scale = Space(a.p, b.p);
    double nearest = std::numeric_limits<double>::infinity();
    std::map<Slot, size_t> visits;
    bool have_rim = false;
    const auto star = Access::AcceptedSplitStar(mesh_, m);
    for (const auto cell : star) {
      for (const auto slot : mesh_.Cells()[cell].vertices) {
        const auto& point = mesh_.Points()[slot];
        const auto id = point.node.id;
        if (id == a.id || id == b.id || id == m.id) continue;
        have_rim = true;
        nearest = std::min(nearest, Space(m.p, point.node.p));
        if (point.Free()) ++visits[slot];
      }
    }
    if (!(scale > 0) || !have_rim)
      throw std::logic_error("Missing edge length or rim in accepted split.");
    last_.ratios = {Space(a.p, m.p) / scale, Space(m.p, b.p) / scale, nearest / scale};
    const unsigned path = last_.path == 0 ? 0 : last_.path == 2 ? 4 : 3 - last_.longest;
    for (const auto& [slot, count] : visits) {
      if (count % 2) throw std::logic_error("A surface split did not produce paired children at a rim point.");
      growth_.at(slot)[path] += count / 2;
    }
    records_.push_back(last_);
    geometry_.Accept(std::move(placement));
  }

  void BeginDistance() { distance_ = true; }

  void Report(std::ostream& out, int rank, unsigned pass) const {
    std::array<std::array<size_t, 3>, 3> longest{};
    std::array<std::array<std::array<size_t, 5>, 3>, 2> bins{};
    std::array<std::vector<const SplitRecord*>, 2> by_kind;
    for (const auto& record : records_) {
      PrintSplit(Row(out, "split", rank, pass), record);
      out << '\n';
      ++longest[record.path][record.longest];
      by_kind[record.feature].push_back(&record);
      for (unsigned axis = 0; axis < record.ratios.size(); ++axis) {
        const auto value = record.ratios[axis];
        ++bins[record.feature][axis][value < .01 ? 0 : value < .1 ? 1 : value < .25 ? 2 : value <= .4 ? 3 : 4];
      }
    }
    for (unsigned path = 0; path < longest.size(); ++path) {
      const auto& counts = longest[path];
      Row(out, "split_longest", rank, pass) << " path=" << SplitPath[path] << " neither=" << counts[0]
          << " one=" << counts[1] << " both=" << counts[2] << '\n';
    }
    for (unsigned kind = 0; kind < by_kind.size(); ++kind) {
      auto& sorted = by_kind[kind];
      const auto top = std::min(size_t{20}, sorted.size());
      for (unsigned axis = 0; axis < RatioSize; ++axis) {
        Row(out, "split_bins", rank, pass) << " kind=" << (kind ? "feature" : "sheet") << " ratio=" << RatioName[axis];
        for (unsigned bin = 0; bin < bins[kind][axis].size(); ++bin) out << " bin_" << bin << '=' << bins[kind][axis][bin];
        out << '\n';
        std::partial_sort(sorted.begin(), sorted.begin() + top, sorted.end(), [&](const auto* a, const auto* b) {
          return std::tie(a->ratios[axis], a->middle.id) < std::tie(b->ratios[axis], b->middle.id);
        });
        for (size_t i = 0; i < top; ++i) {
          auto& row = Row(out, "split_min", rank, pass) << " ratio=" << RatioName[axis] << " order=" << i + 1;
          PrintSplit(row, *sorted[i]);
          out << '\n';
        }
      }
    }
    const auto after = LiveIncidence(mesh_);
    size_t points = 0, total = 0;
    std::array<size_t, 5> split{};
    for (Slot p = 0; p < before_.size(); ++p) {
      if (!mesh_.Points()[p].Free() || after[p] <= before_[p] + 50) continue;
      ++points;
      total += after[p] - before_[p];
      for (unsigned i = 0; i < split.size(); ++i) split[i] += growth_[p][i];
    }
    Row(out, "star_growth", rank, pass, "c1") << " points=" << points << " total=" << total << " L=" << split[0]
        << " D_face_both=" << split[1] << " D_face_one=" << split[2] << " D_face_none=" << split[3]
        << " D_chord=" << split[4] << '\n';
  }

 private:
  static constexpr unsigned RatioSize = 3;
  Mesh& mesh_;
  local::StateSurfaceGeometry& geometry_;
  std::vector<size_t> before_;
  std::vector<std::array<size_t, 5>> growth_;
  std::map<local::SurfaceEdge, std::optional<double>> chords_;
  std::vector<SplitRecord> records_;
  SplitRecord last_{};
  bool distance_ = false;
};

template <class OnSurface>
void ReportRegion(std::ostream& out, const Mesh& mesh, Id base, int rank, OnSurface on_surface) {
  const double infinity = std::numeric_limits<double>::infinity();
  Point lo{infinity, infinity, infinity}, hi{-infinity, -infinity, -infinity};
  const auto& points = mesh.Points();
  std::array<size_t, 4> counts{};
  size_t cells = 0, faces = 0;
  for (const auto& cell : mesh.Cells()) {
    if (!cell.live) continue;
    ++cells;
    unsigned group = 3;
    for (const auto slot : cell.vertices) {
      const auto& row = points[slot];
      const auto sensor_surface = row.surface || on_surface(row.node.id);
      group = std::min(group, sensor_surface ? (row.node.id > base ? 0U : 1U) : row.border ? 2U : 3U);
    }
    ++counts[group];
    for (unsigned f = 0; f < cell.neighbors.size(); ++f) {
      if (cell.neighbors[f] != local::Physical) continue;
      ++faces;
      for (const auto slot : local::OrientedFace(cell.vertices, f)) {
        const auto& p = points[slot].node.p;
        lo = {std::min(lo.x, p.x), std::min(lo.y, p.y), std::min(lo.z, p.z)};
        hi = {std::max(hi.x, p.x), std::max(hi.y, p.y), std::max(hi.z, p.z)};
      }
    }
  }
  out << "FP2_B0 region rank=" << rank << " cells=" << cells << " faces=" << faces << " cells_a=" << counts[0]
      << " cells_b=" << counts[1] << " cells_c=" << counts[2] << " cells_d=" << counts[3] << " has_faces=" << (faces != 0)
      << " bbox_min=";
  if (faces) Coordinate(out, lo); else out << "none";
  out << " bbox_max=";
  if (faces) Coordinate(out, hi); else out << "none";
  out << '\n';
}
}  // namespace fp2_b0
}  // namespace

namespace SU2NativeLocal3D {
unsigned SurfaceFixtureAccess::BadClass(const Mesh& mesh, const Vertices& vertices, Id base) {
  unsigned group = 3;
  for (const auto slot : vertices) {
    const auto& point = mesh.points_[slot];
    group = std::min(group, point.surface ? (point.node.id > base ? 0U : 1U) : point.border ? 2U : 3U);
  }
  return group;
}

std::vector<Slot> SurfaceFixtureAccess::AcceptedSplitStar(const Mesh& mesh, const Node& node) {
  if (!mesh.free_points_.empty() || mesh.points_.back().node.id != node.id)
    throw std::logic_error("A measured surface split did not append its accepted point.");
  return mesh.Star(static_cast<Slot>(mesh.points_.size() - 1));
}

PassStats SurfaceFixtureAccess::HandoverPass(std::ostream& out, Mesh& mesh, IdSequence& point_ids, size_t cell_limit,
                                             CoreOptions options, StateSurfaceGeometry* surface, Id base, int rank,
                                             unsigned pass, unsigned pass_count, const std::string& extra) {
  using namespace fp2_b0;
  PassStats stats;
  mesh.options_ = options;
  stats.cells_in = mesh.live_cells_;
  stats.points_in = mesh.points_.size();
  for (const auto& cell : mesh.cells_)
    if (cell.live && mesh.Margin(cell.vertices) < BadMargin) ++stats.bad_in;

  bool first = true;
  std::vector<RankRefusedEdge> refused;
  stats.sink = [&](const SweepRow& sweep) {
    const bool volume = std::strcmp(sweep.kind, "surface_length") && std::strcmp(sweep.kind, "surface_distance");
    ReportRankSweep(mesh, sweep, rank, pass, volume && std::exchange(first, false));
  };
  stats.surface_refusal_sink = [&](const char* kind, const SurfaceEdge& edge, SurfaceRefusal reason) {
    refused.push_back({kind, edge, reason});
  };
  const World world;
  stats.surface_sink = [&](const SurfaceCounters& counts) { ReportRankSurface(world, pass, true, counts, mesh, refused); };
  const auto before = mesh.sweep_;
  if (surface) {
    RecordingSurface recorder(mesh, *surface);
    const auto report = [&](size_t& faces, size_t& points) {
      faces = mesh.PhysicalFaces().size();
      points = std::count_if(mesh.points_.begin(), mesh.points_.end(), [](const auto& p) { return p.surface && !p.removed; });
    };
    report(stats.surface.faces_in, stats.surface.points_in);
    mesh.SurfaceLength(recorder, point_ids, cell_limit, stats);
    recorder.BeginDistance();
    mesh.SurfaceDistance(recorder, point_ids, cell_limit, stats);
    mesh.MarkUnresolved();
    report(stats.surface.faces_out, stats.surface.points_out);
    std::set<SurfaceEdge> long_edges;
    for (const auto& [cell, face] : mesh.PhysicalFaces()) {
      const auto v = OrientedFace(mesh.cells_[cell].vertices, face);
      for (unsigned k = 0; k < v.size(); ++k) {
        const Edge edge{v[k], v[(k + 1) % v.size()]};
        if (mesh.Length(edge) > LongEdge)
          long_edges.insert(SortedIds(mesh.points_[edge[0]].node.id, mesh.points_[edge[1]].node.id));
      }
    }
    stats.surface.left_long = long_edges.size();
    for (const auto count : LiveIncidence(mesh)) {
      stats.surface.star_max = std::max(stats.surface.star_max, count);
      stats.surface.star_gt100 += count > 100;
      stats.surface.star_gt1000 += count > 1000;
    }
    stats.surface_sink(stats.surface);
    recorder.Report(out, rank, pass);
  }
  const auto after = mesh.sweep_;
  const bool last = pass == pass_count, trial = extra != "none";
  auto checkpoint = [&](const char* name, bool details, bool stars) {
    const auto free = ReportBad(out, mesh, base, rank, pass, name, before, after, details);
    ReportStars(out, mesh, base, rank, pass, name, stars);
    return free;
  };

  checkpoint("c1", false, pass == 1);
  mesh.LengthWork(point_ids, cell_limit, stats);
  checkpoint("c2", false, false);
  const auto quality_start = Clock::now();
  mesh.QualityWork(point_ids, cell_limit, stats);
  stats.seconds_quality = Seconds(quality_start);
  auto bad_free = checkpoint("c3", true, last && !trial);
  if (surface && last) ReportQualityOptions(out, mesh, cell_limit, base, pass, "c3", rank);
  if (trial) {
    VolumeRounds(out, mesh, point_ids, cell_limit, stats, base, rank, pass, extra);
    bad_free = checkpoint("c4", true, last);
    if (last) ReportQualityOptions(out, mesh, cell_limit, base, pass, "c4", rank);
  }
  ReportEdges(out, mesh, base, rank, pass, trial ? "c4" : "c3");
  const auto check_start = Clock::now();
  mesh.Check();
  mesh.MarkWallNear();
  mesh.ReportResult(stats);
  stats.seconds_check = Seconds(check_start);
  if (!surface) ReportRankSurface(world, pass, false, stats.surface, mesh, refused);
  Row(out, "pass", rank, pass) << " cells_in=" << stats.cells_in << " cells_out=" << stats.cells_out
      << " bad_in=" << stats.bad_in << " bad_out=" << stats.bad_out << " bad_out_free=" << stats.bad_out_free
      << " checkpoint_free=" << bad_free << " free_matches=" << (bad_free == stats.bad_out_free)
      << " faces_out=" << mesh.PhysicalFaces().size() << " splits_surface="
      << stats.surface.splits_length + stats.surface.splits_distance << " quality_limit=" << stats.limit
      << " seconds_quality=" << stats.seconds_quality << " seconds_check=" << stats.seconds_check << '\n';
  stats.sink = {};
  stats.surface_sink = {};
  stats.surface_refusal_sink = {};
  out << std::flush;
  return stats;
}
}  // namespace SU2NativeLocal3D

namespace {
void ReportHandoverDonors(const Owned& owned, int rank) {
  size_t rejected = 0;
  std::ostringstream out;
  out << std::setprecision(17);
  for (const auto& entry : owned) {
    const auto& cell = entry.second.cell;
    try {
      MetricAffineWeights(cell, Centroid(cell));
    } catch (const std::invalid_argument& error) {
      if (++rejected > 5) continue;
      out << "FP2_B donor_reject rank=" << rank << " cell=" << cell.id;
      for (size_t i = 0; i < cell.v.size(); ++i) {
        const auto& v = cell.v[i];
        out << " v" << i << '=' << v.id << '@' << v.p.x << ',' << v.p.y << ',' << v.p.z;
      }
      out << " reason=" << std::quoted(error.what()) << '\n';
    }
  }
  out << "FP2_B donor_scan rank=" << rank << " cells=" << owned.size() << " rejected=" << rejected << '\n';
  std::cout << out.str() << std::flush;
}

void RunHandover(bool parallel, bool scan_donors = false) {
  const auto* label = std::getenv("SU2_FP2_B0_RUN");
  const auto* setting = std::getenv("SU2_FP2_B0_EXTRA");
  REQUIRE(label);
  REQUIRE(setting);
  const std::string run(label), extra(setting);
  REQUIRE((parallel ? (run == "H6" || run == "H6off" || run == "X6")
                    : (run == "H1" || run == "H0" || run == "X1" || run == "X2" || run == "X3")));
  REQUIRE((run == "X6" ? (extra == "split" || extra == "collapse" || extra == "both")
                      : extra == (run == "X1" ? "split" : run == "X2" ? "collapse" : run == "X3" ? "both" : "none")));
  const auto start = Clock::now();
  World world;
  REQUIRE(world.size == (parallel ? 6 : 1));
  const bool active = run != "H0" && run != "H6off";
  if (scan_donors) REQUIRE(run == "H6off");
  ReferenceState reference;
  auto input = LoadMesh(&reference, parallel);
  const auto global_cells = world.reduce<uint64_t>(input.cells.size(), CPassiveComm::Op::SUM);
  REQUIRE(global_cells == 303359);
  REQUIRE((reference.marker_names == std::vector<std::string>{"FARFIELD", "SYMMETRY", "WING3D"}));
  REQUIRE(reference.original->Parameters()->Mobility().identity == 0);
  CheckLabStep(world, [&] { native_fp2_fixture::PrepareIN1(reference, input.ceiling, OwnedStorageBytes(input.cells)); });
  const auto& original = *reference.original->Parameters();
  REQUIRE(original.Surface().size() == 48954);
  REQUIRE(original.Records().size() == 24479);
  const auto reference_bytes = reference.original->ResidentBytes();

  PartitionStats routing;
  if (parallel) {
    std::string reason;
    const std::vector<uint32_t> weights(input.cells.size(), 1);
    const auto partitioned = Repartition(world, input.cells, weights, world.size, routing, reason, input.ceiling,
                                         transfer_memory::Add(reference_bytes, transfer_memory::Bytes(weights)));
    INFO(reason);
    REQUIRE(partitioned);
    REQUIRE_FALSE(world.maximum(input.cells.empty()));
  }
  const auto point_base = world.reduce<uint64_t>(input.point_base, CPassiveComm::Op::MAX);
  const auto cell_base = world.reduce<uint64_t>(input.cell_base, CPassiveComm::Op::MAX);
  local::IdSequence point_ids(point_base, world.rank, world.size), cell_ids(cell_base, world.rank, world.size);
  constexpr auto owned_row_bytes = sizeof(Owned::value_type) + 4 * sizeof(void*);
  local::MemoryBudget memory{input.ceiling, reference_bytes, owned_row_bytes, routing.maximum_native_bytes};
  std::unique_ptr<local::StateSurfaceGeometry> surface;
  if (active) {
    CheckLabStep(world, [&] {
      const auto owned = OwnedStorageBytes(input.cells);
      const auto serial_mesh = parallel ? 0 : local::Mesh::ArrayBytes(input.cells.size());
      const auto caller = transfer_memory::Add(reference_bytes, owned, serial_mesh);
      auto budget = native_fp2_fixture::Budget(global_cells, caller, start, 5400, input.ceiling);
      surface = std::make_unique<local::StateSurfaceGeometry>(reference.original, std::move(budget));
    });
  }
  auto options = Variant("V2");
  options.surface_splits = active;
  const unsigned pass_count = parallel ? local::PassCount : 1;
  local::BorderMeeting previous;
  auto& out = std::cout;
  out << std::setprecision(17) << "FP2_B0 input rank=" << world.rank << " cells=" << input.cells.size()
      << " point_base=" << point_base << " cell_base=" << cell_base << " run=" << run << " extra=" << extra << '\n';
  try {
    for (unsigned pass = 1; pass <= pass_count; ++pass) {
      const auto pass_start = Clock::now();
      const auto owned = OwnedStorageBytes(input.cells);
      memory.caller_bytes = transfer_memory::Add(reference_bytes, surface ? surface->ResidentBytes() : 0);
      local::BorderMeeting meeting;
      if (parallel) meeting = local::MeetBorderPoints(world, input.cells, memory, previous.Bytes());
      const auto meeting_seconds = parallel ? Seconds(pass_start) : 0.;
      const auto retained = transfer_memory::Add(reference_bytes, previous.Bytes(), meeting.Bytes());
      const auto extra_bytes = parallel && surface ? local::Mesh::SurfaceArrayBytes(input.cells.size()) : 0;
      memory.Require(world, transfer_memory::Add(memory.caller_bytes, previous.Bytes(), meeting.Bytes(), owned,
                                                 local::Mesh::ArrayBytes(input.cells.size()), extra_bytes),
                     "FP2-B0 frozen core admission.");
      const auto bytes_per_cell = owned_row_bytes + local::Mesh::BytesPerCell +
                                  (parallel && surface ? local::Mesh::SurfaceArrayBytes(1) : 0);
      const auto cell_limit = (input.ceiling - memory.caller_bytes) / bytes_per_cell;
      CheckLabStep(world, [&] {
        const auto build_start = Clock::now();
        local::Mesh mesh(input.cells, parallel, options);
        const auto build_seconds = Seconds(build_start);
        if (parallel) {
          mesh.MarkBorderSurface(meeting);
          if (surface) surface->SetCallerBytes(transfer_memory::Add(reference_bytes, owned));
        }
        const auto run_start = Clock::now();
        auto stats = local::SurfaceFixtureAccess::HandoverPass(out, mesh, point_ids, cell_limit, options, surface.get(),
                                                              point_base, world.rank, pass, pass_count, extra);
        const auto run_seconds = Seconds(run_start);
        const auto write_start = Clock::now();
        mesh.WriteBack(input.cells, cell_ids);
        stats.seconds_build = build_seconds;
        stats.seconds_write = Seconds(write_start);
        ReportRankPass(world, pass, active, stats, Seconds(pass_start), run_seconds, meeting_seconds);
      });
      if (parallel) {
        if (surface) {
          memory.maximum_bytes = std::max(memory.maximum_bytes, surface->PeakBytes());
          surface->Exchange(world, memory, transfer_memory::Add(retained, OwnedStorageBytes(input.cells)));
          memory.caller_bytes = transfer_memory::Add(reference_bytes, surface->ResidentBytes());
        }
        const bool lower = pass == 1;
        local::ShiftBorders(world, input.cells, meeting, lower ? local::Direction::Lower : local::Direction::Higher,
                            memory, lower ? nullptr : &previous);
        if (lower) previous.rendezvous = std::move(meeting.rendezvous);
      }
    }

    const auto exact = CountExact(input.cells);
    out << "FP2_B0 exact rank=" << world.rank << " cells=" << input.cells.size() << " nonpositive=" << exact.nonpositive
        << " first_nonpositive=" << exact.first << '\n' << std::flush;
    REQUIRE_FALSE(world.maximum(exact.nonpositive != 0));
    if (scan_donors) CheckLabStep(world, [&] { ReportHandoverDonors(input.cells, world.rank); });
    if (active || parallel) {
      CheckLabStep(world, [&] {
        const local::Mesh mesh(input.cells, parallel, options);
        fp2_b0::ReportRegion(out, mesh, point_base, world.rank, [&](Id id) {
          return surface ? surface->Find(id) != nullptr : original.Records().count(id) != 0;
        });
      });
    }
  } catch (const CElectedFailure& error) {
    FAIL(error.message);
  }
  out << "FP2_B0 end rank=" << world.rank << " run=" << run << " cells_out=" << input.cells.size()
      << " modeled_peak=" << memory.maximum_bytes << " seconds=" << Seconds(start) << '\n' << std::flush;
}
}  // namespace

TEST_CASE("FP2-B0 frozen one-rank core hand-over", "[.][NativeLocalCoreHandoverSerial]") { RunHandover(false); }

TEST_CASE("FP2-B0 frozen six-rank core hand-over", "[.][NativeLocalCoreHandoverMPI]") { RunHandover(true); }

TEST_CASE("FP2-B hand-over donor scan without surface splits", "[.][NativeLocalCoreVolumeHandoverOff]") {
  RunHandover(true, true);
}
