/*!
 * \file native_local_distance_fixtures.inc
 * \brief FP2-D surface distance fixtures.
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md).
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */
namespace {
using local::SurfaceRefusal;
using local::SurfaceRemaining;

local::CoreOptions ErrorCut(bool enabled = true, bool work_list = true) {
  local::CoreOptions options;
  options.surface_splits = true;
  options.distance_error_cut = enabled;
  options.work_list = work_list;
  return options;
}

Id MaxPointId(const local::Mesh& mesh) {
  Id max_id = 0;
  for (const auto& row : mesh.Points()) max_id = std::max(max_id, row.node.id);
  return max_id;
}

using DistanceRefusal = std::pair<Edge, SurfaceRefusal>;

struct DistanceFixtureRun {
  local::SurfaceCounters counts;
  std::map<Face, SurfaceRemaining> remaining;
  std::vector<local::SweepRow> sweeps;
  std::vector<DistanceRefusal> refusals;
};

DistanceFixtureRun RunDistanceFixture(local::Mesh& mesh, local::SurfaceGeometry& geometry) {
  DistanceFixtureRun result;
  local::IdSequence ids(MaxPointId(mesh));
  local::PassStats report;
  report.sink = [&](const local::SweepRow& row) {
    if (row.candidates) result.sweeps.push_back(row);
  };
  report.surface_remaining_sink = [&](Face face, SurfaceRemaining reason) {
    REQUIRE(result.remaining.emplace(Key(face), reason).second);
  };
  report.surface_refusal_sink = [&](const char*, Edge edge, SurfaceRefusal reason) {
    result.refusals.emplace_back(edge, reason);
  };
  local::SurfaceFixtureAccess::DistanceWork(mesh, geometry, ids, report);
  REQUIRE_NOTHROW(mesh.Check());
  result.counts = std::move(report.surface);
  return result;
}

Edge FaceEdge(const SurfaceFace& face, size_t i) {
  const Id a = face.v[i], b = face.v[(i + 1) % face.v.size()];
  return {std::min(a, b), std::max(a, b)};
}

class DistanceSampleGeometry : public FlatGeometry {
 public:
  explicit DistanceSampleGeometry(const Owned& rows) : FlatGeometry(rows) {}

  bool Feature(const Edge& edge) const override { return feature == edge; }
  std::optional<double> Chord(const std::array<Node, 2>& ends) override {
    return Feature(EdgeKey(ends)) ? std::optional<double>{2 * Limit()} : std::nullopt;
  }

  local::SurfaceSamples Samples(const SurfaceFace& face) override {
    local::SurfaceSamples result{};
    const auto key = Key(face.v);
    if (centroid == key) result.distances[0] = 2 * Limit();
    const auto row = errors.find(key);
    if (row != errors.end())
      for (size_t i = 0; i < face.v.size(); ++i) {
        const auto found = row->second.find(FaceEdge(face, i));
        if (found != row->second.end()) result.distances[i + 1] = found->second * Limit();
      }
    return result;
  }

  local::SurfacePlacement Place(const std::array<Node, 2>& ends, const std::array<SurfaceFace, 2>& faces,
                                Id id) override {
    const auto edge = EdgeKey(ends);
    attempts.push_back(edge);
    if (refuse == edge) {
      local::SurfacePlacement result;
      result.refusal = SurfaceRefusal::Walk;
      return result;
    }
    auto placement = FlatGeometry::Place(ends, faces, id);
    if (Feature(edge)) placement.parent = edge;
    return placement;
  }

  std::map<Face, std::map<Edge, double>> errors;
  std::optional<Face> centroid;
  std::optional<Edge> refuse;
  std::optional<Edge> feature;
  std::vector<Edge> attempts;
};

constexpr size_t CylinderSectors = 8, CylinderLayers = 2, CylinderStride = CylinderSectors + 1;
const double CylinderLayerHeight = 2.4 * 2 * std::sin(std::acos(-1.) / 8);

class LayeredCylinderGeometry : public CylinderGeometry {
 public:
  explicit LayeredCylinderGeometry(const Owned& rows)
      : CylinderGeometry(rows, CylinderLayers * CylinderLayerHeight, false) {}

  local::SurfacePlacement Place(const std::array<Node, 2>& ends, const std::array<SurfaceFace, 2>& faces,
                                Id id) override {
    attempts.push_back(EdgeKey(ends));
    return CylinderGeometry::Place(ends, faces, id);
  }

  bool IsCap(const SurfaceFace& face) const {
    return std::all_of(face.v.begin(), face.v.end(), [&](Id id) { return At(id).z == 0; }) ||
           std::all_of(face.v.begin(), face.v.end(),
                       [&](Id id) { return At(id).z == CylinderLayers * CylinderLayerHeight; });
  }

  std::vector<Edge> attempts;
};

size_t CountCaps(const std::vector<SurfaceFace>& faces, const LayeredCylinderGeometry& geometry) {
  return std::count_if(faces.begin(), faces.end(), [&](const auto& face) { return geometry.IsCap(face); });
}

std::string S0DistanceFields(const local::SurfaceCounters& counters) {
  std::ostringstream fields;
  local::PrintSurfaceFields(fields, counters);
  std::istringstream input(fields.str());
  std::string token, result;
  while (input >> token)
    if (token.rfind("seconds_", 0) != 0 && token.rfind("d_", 0) != 0) result += " " + token;
  return result;
}

void SetSkewedErrors(DistanceSampleGeometry& geometry) {
  geometry.errors[AskingFace][BoxDiagonal] = 2;
  geometry.errors[LockedFace][LockedSide] = 3;
  geometry.errors[{2, 3, 6}][FirstTerminal] = 4;
}

constexpr Edge RadialA{0, 1}, RadialB{0, 2}, RadialC{0, 3};
constexpr Face FanAB{0, 1, 2}, FanBC{0, 2, 3}, FanCA{0, 1, 3};

Owned DistanceFan() {
  return BoundMesh({{0, 0, 0}, {1, 0, 0}, {0, 1, 0}, {0, 0, 1}}, {{0, 1, 2, 3}}, 1);
}
}  // namespace

TEST_CASE("FP2-D cuts fewer edges on the FP2-A cylinder", "[.][NativeFP2D]") {
  const auto rows = Cylinder();
  CylinderGeometry ordinary(rows), directional(rows);
  const auto off = Run(rows, ordinary, true);
  REQUIRE(off.stats.surface.splits_distance == 27);
  CHECK(off.stats.surface.left_distance == 1);
  CHECK(off.stats.surface.left_refused == 1);
  CHECK(off.remaining.size() == 1);

  const auto options = ErrorCut();
  local::Mesh mesh(rows, false, options);
  local::IdSequence ids(MaxPointId(mesh));
  local::SurfaceChosenEdgeProbe probe(mesh, directional);
  local::PassStats report;
  std::map<Face, SurfaceRemaining> remaining;
  report.sink = [&](const local::SweepRow& sweep) { probe.EndSweep(sweep); };
  report.surface_remaining_sink = [&](Face face, SurfaceRemaining reason) {
    REQUIRE(remaining.emplace(Key(face), reason).second);
  };
  const auto stats = mesh.RunPass(ids, SIZE_MAX, std::move(report), options, &probe);
  const auto& counts = stats.surface;
  // The refused rim cannot be cut; a side child still spans its full 45-degree chord.
  // Other midpoint errors can end directly, while chord requests retain the parent rule.
  CHECK(probe.Checked() == counts.splits_distance);
  CHECK(probe.Violations() == 0);
  CHECK(counts.splits_distance < off.stats.surface.splits_distance);
  CHECK(counts.left_distance == counts.left_refused);
  CHECK(counts.left_refused > 0);
  size_t above = 0;
  for (const auto& face : Surface(mesh)) {
    const auto samples = directional.Samples(face).distances;
    if (*std::max_element(samples.begin(), samples.end()) <= directional.Limit()) continue;
    ++above;
    REQUIRE(remaining.count(Key(face.v)) == 1);
    CHECK(remaining.at(Key(face.v)) == SurfaceRemaining::Refused);
  }
  CHECK(above == remaining.size());
  CHECK(above == counts.left_distance);
  std::cout << "FP2_D cylinder off_splits=" << off.stats.surface.splits_distance
            << " on_splits=" << counts.splits_distance << " sweeps=" << counts.sweeps_distance
            << " left_refused=" << counts.left_refused << '\n';
}

TEST_CASE("FP2-D layered cylinder cuts diagonals followed by across edges", "[.][NativeFP2D]") {
  const auto rows = Cylinder(CylinderLayers, CylinderLayerHeight);
  local::Mesh mesh(rows, false, ErrorCut());
  LayeredCylinderGeometry geometry(rows);
  const auto initial = Surface(mesh);
  const auto initial_caps = CountCaps(initial, geometry);
  const auto [counts, remaining, sweeps, refusals] = RunDistanceFixture(mesh, geometry);

  std::set<Edge> diagonals, across;
  for (size_t layer = 0; layer <= CylinderLayers; ++layer) {
    const Id base = layer * CylinderStride;
    for (Id i = 1; i <= CylinderSectors; ++i) {
      const Id j = i % CylinderSectors + 1;
      const Id a = base + std::min(i, j), b = base + std::max(i, j);
      across.insert({a, b});
      if (layer < CylinderLayers) diagonals.insert({a, b + CylinderStride});
    }
  }
  // The original 45-degree across edge and diagonal have the identical midpoint radius.
  // The diagonal is longer and each diagonal ring is confined to its own sector and layer.
  // In the second sweep no cell contains two complete across edges, so their rings are disjoint.
  // A diagonal splits two side faces; an across split adds two side faces, except a rim adds
  // one side and one cap. That adds 4nm side faces and 2n cap faces. All remaining side angles
  // between vertices are at most 22.5 degrees; their chord sagitta is below Limit().
  REQUIRE(counts.sweeps_distance == 2);
  REQUIRE(sweeps.size() == 2);
  REQUIRE(sweeps[0].counters.splits == CylinderSectors * CylinderLayers);
  REQUIRE(sweeps[1].counters.splits == CylinderSectors * (CylinderLayers + 1));
  REQUIRE(geometry.attempts.size() == diagonals.size() + across.size());
  const auto boundary = geometry.attempts.begin() + diagonals.size();
  CHECK((std::set<Edge>(geometry.attempts.begin(), boundary) == diagonals));
  CHECK((std::set<Edge>(boundary, geometry.attempts.end()) == across));
  CHECK(counts.d_marks_error == diagonals.size() + across.size());
  CHECK(counts.d_marks_longest == 0);
  CHECK(remaining.empty());
  CHECK(refusals.empty());

  const auto final = Surface(mesh);
  const auto final_caps = CountCaps(final, geometry);
  CHECK(initial_caps == 2 * CylinderSectors);
  CHECK(final_caps == 4 * CylinderSectors);
  CHECK(initial.size() - initial_caps == 2 * CylinderSectors * CylinderLayers);
  CHECK(final.size() - final_caps == 6 * CylinderSectors * CylinderLayers);
}

TEST_CASE("FP2-D never falls back from a refused edge carrying the error", "[.][NativeFP2D]") {
  const auto rows = Box();
  constexpr Edge selected{0, 1};
  for (const bool enabled : {false, true}) {
    CAPTURE(enabled);
    local::Mesh mesh(rows, false, ErrorCut(enabled));
    DistanceSampleGeometry geometry(rows);
    geometry.errors[AskingFace][selected] = 2;
    geometry.refuse = selected;
    const auto before = Snapshot(mesh);
    const auto result = RunDistanceFixture(mesh, geometry);
    if (enabled) {
      CHECK((geometry.attempts == std::vector<Edge>{selected}));
      CHECK((result.refusals == std::vector<DistanceRefusal>{{selected, SurfaceRefusal::Walk}}));
      CHECK((result.remaining == std::map<Face, SurfaceRemaining>{{AskingFace, SurfaceRemaining::Refused}}));
      CHECK(Snapshot(mesh) == before);
      CHECK(result.counts.splits_distance == 0);
    } else {
      CHECK((geometry.attempts == std::vector<Edge>{BoxDiagonal}));
      CHECK(result.remaining.empty());
      CHECK(result.counts.splits_distance == 1);
    }
  }
}

TEST_CASE("FP2-D preserves the skewed box fallback with only centroid errors", "[.][NativeFP2D]") {
  constexpr std::array<std::pair<bool, bool>, 3> cases{{{false, false}, {true, false}, {false, true}}};
  for (const auto& [border, feature] : cases) {
    CAPTURE(border, feature);
    const auto rows = SkewedBox(border);
    local::Mesh off_mesh(rows, border, ErrorCut(false)), on_mesh(rows, border, ErrorCut());
    LockedBoxGeometry off_geometry(rows, feature), on_geometry(rows, feature);
    if (!border && !feature) {
      for (const auto* mesh : {&off_mesh, &on_mesh}) {
        const auto allowed = local::SurfaceFixtureAccess::DistanceChoice(*mesh, on_geometry, AskingFace, {});
        const auto refused = local::SurfaceFixtureAccess::DistanceChoice(*mesh, on_geometry, AskingFace, {LockedSide});
        CHECK(allowed.first == FirstTerminal);
        CHECK_FALSE(allowed.second.has_value());
        CHECK_FALSE(refused.first.has_value());
        CHECK(refused.second == SurfaceRemaining::NeighborLocked);
      }
    }
    const auto off = RunDistanceFixture(off_mesh, off_geometry);
    const auto on = RunDistanceFixture(on_mesh, on_geometry);
    CHECK(Snapshot(off_mesh) == Snapshot(on_mesh));
    CHECK(off_geometry.attempts == on_geometry.attempts);
    CHECK(off_geometry.chord_queries == on_geometry.chord_queries);
    CHECK(off.remaining == on.remaining);
    CHECK(off.refusals == on.refusals);
    CHECK(S0DistanceFields(off.counts) == S0DistanceFields(on.counts));
    CHECK(on.counts.d_error_edges == 0);
    CHECK(on.counts.d_marks_error == 0);
    CHECK(on.counts.d_marks_longest == on.counts.splits_distance);
    CHECK(on.counts.d_centroid_only > 0);
    CHECK(on.counts.d_nonfinite == 0);
  }
}

TEST_CASE("FP2-D follows pooled errors to the terminal marked by its own face", "[.][NativeFP2D]") {
  const auto rows = SkewedBox(false);
  local::Mesh mesh(rows, false, ErrorCut());
  DistanceSampleGeometry geometry(rows);
  SetSkewedErrors(geometry);
  const auto [counts, remaining, sweeps, refusals] = RunDistanceFixture(mesh, geometry);
  // All three edges carry an error and meet in tetrahedron 0-2-3-6. Their lengths
  // are sqrt(5), 2 and sqrt(2) in attempt order. Each cut stamps the common ring
  // and removes its asking face; the other original requests survive and defer.
  CHECK((geometry.attempts == std::vector<Edge>{FirstTerminal, LockedSide, BoxDiagonal}));
  REQUIRE(sweeps.size() == 3);
  for (size_t i = 0; i < sweeps.size(); ++i) {
    CHECK(sweeps[i].candidates == 3 - i);
    CHECK(sweeps[i].counters.deferred == 2 - i);
    CHECK(sweeps[i].counters.splits == 1);
  }
  CHECK(counts.sweeps_distance == 3);
  CHECK(counts.path_max == 0);
  CHECK(counts.waits_neighbor == 0);
  CHECK(counts.path_splits == 0);
  CHECK(counts.d_marks_error == 3);
  CHECK(remaining.empty());
  CHECK(refusals.empty());
}

TEST_CASE("FP2-D border locks both a path and its asking neighbor", "[.][NativeFP2D]") {
  const auto rows = SkewedBox(true);
  local::Mesh mesh(rows, true, ErrorCut());
  DistanceSampleGeometry geometry(rows);
  SetSkewedErrors(geometry);
  const auto result = RunDistanceFixture(mesh, geometry);
  // The longer 2-3 is tried first and refused on the missing physical face
  // 2-3-6. The 0-2 ring is complete; its cut removes both asking faces.
  CHECK((geometry.attempts == std::vector<Edge>{BoxDiagonal}));
  CHECK((result.refusals == std::vector<DistanceRefusal>{{LockedSide, SurfaceRefusal::Border}}));
  CHECK(result.remaining.empty());
  CHECK(result.counts.splits_distance == 1);
  REQUIRE(result.sweeps.size() == 1);
  CHECK(result.sweeps.front().candidates == 2);
}

TEST_CASE("FP2-D uses the longest edge for an unshared centroid error", "[.][NativeFP2D]") {
  const auto rows = Box();
  for (const bool enabled : {false, true}) {
    CAPTURE(enabled);
    local::Mesh mesh(rows, false, ErrorCut(enabled));
    DistanceSampleGeometry geometry(rows);
    geometry.centroid = AskingFace;
    const auto result = RunDistanceFixture(mesh, geometry);
    CHECK((geometry.attempts == std::vector<Edge>{BoxDiagonal}));
    CHECK(result.counts.splits_distance == 1);
    CHECK(result.counts.d_error_edges == 0);
    CHECK(result.counts.d_marks_longest == static_cast<size_t>(enabled));
    CHECK(result.counts.d_centroid_only == static_cast<size_t>(enabled));
    CHECK(result.remaining.empty());
  }
}

TEST_CASE("FP2-D pools the cyclic face preferences in a tetrahedron fan", "[.][NativeFP2D]") {
  const auto rows = DistanceFan();
  local::Mesh mesh(rows, false, ErrorCut());
  DistanceSampleGeometry geometry(rows);
  geometry.errors[FanAB][RadialB] = 2;
  geometry.errors[FanBC][RadialC] = 3;
  geometry.errors[FanCA][RadialA] = 4;
  const auto [counts, remaining, sweeps, refusals] = RunDistanceFixture(mesh, geometry);
  // AB and CA mark pa, and BC marks pc. Both edges have unit length, so pa goes
  // first by ID and stamps the single tetrahedron. BC survives and marks pc again.
  CHECK((geometry.attempts == std::vector<Edge>{RadialA, RadialC}));
  REQUIRE(sweeps.size() == 2);
  CHECK(sweeps[0].counters.splits == 1);
  CHECK(sweeps[1].counters.splits == 1);
  CHECK(sweeps[0].candidates == 2);
  CHECK(sweeps[0].counters.deferred == 1);
  CHECK(sweeps[1].candidates == 1);
  CHECK(sweeps[1].counters.deferred == 0);
  CHECK(counts.waits_neighbor == 0);
  CHECK(counts.path_max == 0);
  CHECK(counts.d_marks_error == 2);
  CHECK(remaining.empty());
  CHECK(refusals.empty());
}

TEST_CASE("FP2-D uses a shared edge error across the limit", "[.][NativeFP2D]") {
  const auto rows = DistanceFan();
  local::Mesh mesh(rows, false, ErrorCut());
  DistanceSampleGeometry geometry(rows);
  geometry.errors[FanAB][RadialA] = .99;
  geometry.errors[FanCA][RadialA] = 1.01;
  const auto [counts, remaining, sweeps, refusals] = RunDistanceFixture(mesh, geometry);
  CHECK((geometry.attempts == std::vector<Edge>{RadialA}));
  REQUIRE(sweeps.size() == 1);
  CHECK(sweeps[0].counters.splits == 1);
  CHECK(counts.d_error_edges == 1);
  CHECK(counts.d_marks_error == 1);
  CHECK(counts.path_max == 0);
  CHECK(remaining.empty());
  CHECK(refusals.empty());
}

TEST_CASE("FP2-D enforces the length floor after advancing a path", "[.][NativeFP2D]") {
  const auto rows = BoundMesh({{0, 0, 0}, {1, 0, 0}, {.5, .2, 0}, {0, 0, .02}}, {{0, 1, 2, 3}}, 1);
  local::Mesh mesh(rows, false, ErrorCut());
  DistanceSampleGeometry geometry(rows);
  // pa is longer than either side incident to b. Its other face chooses pc
  // by the pooled error, but pc = .02 fails the .028 floor before the early stop.
  geometry.centroid = FanAB;
  geometry.errors[FanCA][RadialC] = 3;
  const auto result = RunDistanceFixture(mesh, geometry);
  CHECK(geometry.attempts.empty());
  CHECK(result.sweeps.empty());
  CHECK((result.remaining == std::map<Face, SurfaceRemaining>{
                                 {FanAB, SurfaceRemaining::NeighborLocked}, {FanCA, SurfaceRemaining::Short}}));
  CHECK(result.counts.path_max == 1);
  CHECK(result.counts.left_neighbor_locked == 1);
  CHECK(result.counts.left_short == 1);
}

TEST_CASE("FP2-D breaks an exact midpoint tie by length with either work list option", "[.][NativeFP2D]") {
  const auto rows = Cylinder(CylinderLayers, CylinderLayerHeight);
  constexpr Edge across{1, 2}, diagonal{1, CylinderStride + 2};
  const Face witness{1, 2, CylinderStride + 2};
  LayeredCylinderGeometry off_geometry(rows), on_geometry(rows);
  const auto initial = Surface(rows);
  const auto face = std::find_if(initial.begin(), initial.end(), [&](const auto& f) { return Key(f.v) == witness; });
  REQUIRE(face != initial.end());
  const auto samples = on_geometry.Samples(*face).distances;
  std::map<Edge, double> sampled;
  for (size_t i = 0; i < face->v.size(); ++i) sampled[FaceEdge(*face, i)] = samples[i + 1];
  REQUIRE(sampled.at(across) > on_geometry.Limit());
  REQUIRE(sampled.at(across) == sampled.at(diagonal));

  local::Mesh off_mesh(rows, false, ErrorCut(true, false)), on_mesh(rows, false, ErrorCut());
  const auto off = RunDistanceFixture(off_mesh, off_geometry);
  const auto on = RunDistanceFixture(on_mesh, on_geometry);
  REQUIRE(on.sweeps.size() == 2);
  REQUIRE(on_geometry.attempts.size() >= on.sweeps[0].counters.splits);
  const auto first_ends = on_geometry.attempts.begin() + on.sweeps[0].counters.splits;
  CHECK(std::find(on_geometry.attempts.begin(), first_ends, diagonal) != first_ends);
  CHECK(std::find(on_geometry.attempts.begin(), first_ends, across) == first_ends);
  CHECK(off_geometry.attempts == on_geometry.attempts);
  CHECK(off.remaining == on.remaining);
  CHECK(Snapshot(off_mesh) == Snapshot(on_mesh));
}

TEST_CASE("FP2-D preserves the altitude guard for a flat child", "[.][NativeFP2D]") {
  constexpr double limit = .014, height = 1.25 * limit;
  const auto rows = BoundMesh({{0, 0, 0}, {4, 0, 0}, {1, height, 0}, {2, -.8, 0}, {2, 0, .8}},
                              {{0, 1, 2, 4}, {0, 3, 1, 4}}, 1);
  local::Mesh mesh(rows, false, ErrorCut());
  DistanceSampleGeometry geometry(rows);
  geometry.errors[AskingFace][ChainLongest] = 2;
  const auto before = Snapshot(mesh);
  const auto result = RunDistanceFixture(mesh, geometry);
  CHECK((geometry.attempts == std::vector<Edge>{ChainLongest}));
  CHECK((result.refusals == std::vector<DistanceRefusal>{{ChainLongest, SurfaceRefusal::Near}}));
  CHECK((result.remaining == std::map<Face, SurfaceRemaining>{{AskingFace, SurfaceRemaining::Refused}}));
  CHECK(result.counts.refused_near == 1);
  CHECK(result.counts.splits_distance == 0);
  CHECK(geometry.created.empty());
  CHECK(Snapshot(mesh) == before);
}

TEST_CASE("FP2-D splits independent strip diagonals in one sweep", "[.][NativeFP2D]") {
  for (const size_t layers : {4, 8}) {
    CAPTURE(layers);
    std::vector<Point> points;
    std::vector<Tet> tets;
    for (size_t layer = 0; layer <= layers; ++layer) {
      const double z = CylinderLayerHeight * layer;
      points.insert(points.end(), {{0, 0, z}, {1, 0, z}, {std::sqrt(.5), std::sqrt(.5), z}});
    }
    for (size_t layer = 0; layer < layers; ++layer) {
      const Id p = 3 * layer;
      tets.insert(tets.end(), {{p, p + 1, p + 2, p + 5}, {p, p + 1, p + 4, p + 5},
                               {p, p + 3, p + 4, p + 5}});
    }
    const auto rows = BoundMesh(points, tets, 1);
    local::Mesh mesh(rows, false, ErrorCut());
    DistanceSampleGeometry geometry(rows);
    std::vector<Edge> diagonals;
    for (size_t layer = 0; layer < layers; ++layer) {
      const Id p = 3 * layer;
      const Face lower{p + 1, p + 2, p + 5}, upper{p + 1, p + 4, p + 5};
      const Edge diagonal{p + 1, p + 5}, across{p + 4, p + 5};
      diagonals.push_back(diagonal);
      geometry.errors[lower][diagonal] = geometry.errors[upper][diagonal] = 2 * layer + 2;
      if (layer + 1 < layers) {
        const Face next_lower{p + 4, p + 5, p + 8};
        geometry.errors[upper][across] = geometry.errors[next_lower][across] = 2 * layer + 3;
      }
    }
    // Along the strip the increasing keys alternate diagonal and across; the old walk
    // ends at the last diagonal for every request. The disjoint diagonal rings are
    // longer than across edges, so they split first and stamp every interior across ring.
    const auto result = RunDistanceFixture(mesh, geometry);
    REQUIRE(result.sweeps.size() == 1);
    CHECK(result.sweeps.front().candidates == 2 * layers - 1);
    CHECK(result.sweeps.front().counters.deferred == layers - 1);
    CHECK(result.sweeps.front().counters.splits == layers);
    CHECK(result.counts.path_max == 0);
    CHECK(result.counts.waits_neighbor == 0);
    std::sort(geometry.attempts.begin(), geometry.attempts.end());
    CHECK(geometry.attempts == diagonals);
    CHECK(result.remaining.empty());
  }
}

TEST_CASE("FP2-D retains the centroid walk until reaching an error edge", "[.][NativeFP2D]") {
  const auto rows = SkewedBox(false);
  for (const bool midpoint : {false, true}) {
    CAPTURE(midpoint);
    local::Mesh mesh(rows, false, ErrorCut());
    DistanceSampleGeometry geometry(rows);
    geometry.centroid = AskingFace;
    if (midpoint) geometry.errors[{2, 3, 6}][LockedSide] = 2;
    const auto result = RunDistanceFixture(mesh, geometry);
    // All three seed lengths rise. With error on 2-3, it ends the path
    // after the first advance and is also marked by its own asking face.
    REQUIRE_FALSE(geometry.attempts.empty());
    CHECK(geometry.attempts.front() == (midpoint ? LockedSide : FirstTerminal));
    CHECK(result.sweeps.front().candidates == 1);
    CHECK(result.counts.path_max == (midpoint ? 1 : 2));
    CHECK(result.remaining.empty());
  }
}

TEST_CASE("FP2-D walks through a feature carrying midpoint error", "[.][NativeFP2D]") {
  const auto rows = SkewedBox(false);
  local::Mesh mesh(rows, false, ErrorCut());
  DistanceSampleGeometry geometry(rows);
  geometry.centroid = AskingFace;
  geometry.feature = LockedSide;
  geometry.errors[{2, 3, 6}] = {{LockedSide, 2}, {FirstTerminal, 3}};
  // Both the centroid route and the chord request advance through the feature
  // to 3-6, which is longer and preferred by the other parent of 2-3.
  const auto result = RunDistanceFixture(mesh, geometry);
  REQUIRE_FALSE(geometry.attempts.empty());
  REQUIRE_FALSE(result.sweeps.empty());
  CHECK(geometry.attempts.front() == FirstTerminal);
  CHECK(result.sweeps.front().candidates == 1);
  CHECK(result.sweeps.front().counters.splits == 1);
  CHECK(result.sweeps.front().counters.deferred == 0);
}

namespace {
class PersistentLineGeometry final : public DistanceSampleGeometry {
 public:
  explicit PersistentLineGeometry(const Owned& rows) : DistanceSampleGeometry(rows) {}

  double Tolerance() const override { return .018; }

  local::SurfaceSamples Samples(const SurfaceFace& face) override {
    local::SurfaceSamples samples{};
    for (size_t i = 0; i < face.v.size(); ++i) {
      const auto a = At(face.v[i]), b = At(face.v[(i + 1) % face.v.size()]);
      if (a.y == 0 && a.z == 0 && b.y == 0 && b.z == 0) samples.distances[i + 1] = 2 * Limit();
    }
    return samples;
  }
};
}  // namespace

TEST_CASE("FP2-D screens persistent midpoint errors at the metric seed floor", "[.][NativeFP2D]") {
  const auto rows = BoundMesh({{-.5, 0, 0}, {.5, 0, 0}, {0, 1, 0}, {0, 0, 1}},
                              {{0, 1, 2, 3}}, .0025);
  for (const bool enabled : {true, false}) {
    CAPTURE(enabled);
    local::Mesh mesh(rows, false, ErrorCut(enabled));
    PersistentLineGeometry geometry(rows);
    REQUIRE(mesh.Length({SlotOf(mesh, 0), SlotOf(mesh, 1)}) == Approx(.05));
    const auto result = RunDistanceFixture(mesh, geometry);
    const auto edges = mesh.Edges();
    const auto below = std::count_if(edges.begin(), edges.end(),
                                     [&](const auto& row) { return mesh.Length(row.edge) < local::HandoverMetricEdge; });
    CHECK(result.counts.d_refused_metric == 0);
    if (enabled) {
      // Metric lengths .05 and .025 pass; .0125 stops. The halves occupy
      // distinct tetrahedra, so both are cut in sweep 2 and leave eight Short faces.
      CHECK(result.counts.splits_distance == 3);
      CHECK(result.counts.sweeps_distance == 2);
      CHECK(result.counts.left_short == 8);
      CHECK(result.counts.left_short == result.remaining.size());
      CHECK(result.counts.d_short_metric == 8);
      CHECK(result.refusals.empty());
      CHECK(below == 0);
    } else {
      // With the option off, the spatial-only longest-edge walk starts on a
      // sqrt(1.25) spoke rather than the unit line. Both requests lead to 2-3,
      // length sqrt(2), before the test ever cuts the error line itself.
      REQUIRE_FALSE(geometry.attempts.empty());
      CHECK((geometry.attempts.front() == Edge{2, 3}));
      CHECK(result.counts.d_short_metric == 0);
      CHECK(below > 0);
      std::cout << "FP2_D chain_control splits=" << result.counts.splits_distance
                << " sweeps=" << result.counts.sweeps_distance << " left=" << result.remaining.size()
                << " left_short=" << result.counts.left_short << " below_metric=" << below << '\n';
    }
  }
}

TEST_CASE("FP2-D checks the inner spoke before committing a split", "[.][NativeFP2D]") {
  const auto rows = BoundMesh({{-1, 0, 0}, {1, 0, 0}, {0, 1, 0}, {0, 0, 1}, {0, .001, .001}},
                              {{0, 1, 2, 4}, {0, 1, 4, 3}}, 1);
  for (const double floor : {local::HandoverMetricEdge, 0.0}) {
    CAPTURE(floor);
    FlatGeometry geometry(rows);
    local::Mesh mesh(rows);
    const auto before = Snapshot(mesh);
    local::IdSequence ids(4);
    local::SurfaceCounters stats;
    std::string row;
    // The physical children have unit-height third vertices C and D. The
    // interior spoke to X has identity-metric length sqrt(2) * .001.
    const auto result = mesh.SplitSurface({SlotOf(mesh, 0), SlotOf(mesh, 1)}, 0, geometry, ids, 1, SIZE_MAX,
                                          stats, false, true, &row, floor);
    if (floor > 0) {
      CHECK(result == SurfaceRefusal::Near);
      CHECK(stats.refused_near == 1);
      CHECK(stats.d_refused_metric == 1);
      CHECK(Snapshot(mesh) == before);
      CHECK(ids.Peek() == 5);
      CHECK(geometry.created.empty());
      CHECK(row.find("outcome=refused reason=metric_floor m=5 blocker=4 m_xyz=0,0,0 length=") == 0);
    } else {
      CHECK(result == SurfaceRefusal::None);
      CHECK(stats.d_refused_metric == 0);
      CHECK(ids.Peek() == 6);
      CHECK(mesh.Length({SlotOf(mesh, 4), SlotOf(mesh, 5)}) == Approx(std::sqrt(2.) * .001));
    }
    REQUIRE_NOTHROW(mesh.Check());
  }
}

TEST_CASE("FP2-D screens a feature seed without face requests", "[.][NativeFP2D]") {
  const auto rows = Box(1e-4);
  local::Mesh mesh(rows, false, ErrorCut());
  DistanceSampleGeometry geometry(rows);
  geometry.feature = BoxDiagonal;
  // Diagonal 0-2 is the longest edge of both parents, but its metric length
  // is sqrt(2) * .01. Only its chord asks, so exactly one metric check stops it.
  CHECK(mesh.Length(BottomEdge(mesh)) == Approx(std::sqrt(2.) * .01));
  const auto result = RunDistanceFixture(mesh, geometry);
  CHECK(result.sweeps.empty());
  CHECK(result.counts.sweeps_distance == 0);
  CHECK(result.counts.d_short_metric == 1);
  CHECK(result.counts.d_refused_metric == 0);
  CHECK(geometry.attempts.empty());
  CHECK(result.remaining.empty());
}
