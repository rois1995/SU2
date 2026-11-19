
namespace {
// Stores one original M29 vertex and its nodal metric in tensor field order.
struct M29FixtureVertex {
  Id id;
  Point point;
  Tensor metric;
};

// Stores one original M29 cell and its four vertices in volume orientation.
struct M29FixtureCell {
  Id id;
  std::array<Id, 4> vertices;
};

// Exact coordinates and nodal target tensors from the two closed M29 vertex stars.
constexpr std::array<M29FixtureVertex, 10> M29FixtureVertices{{
    {8621,
     {0.92351367803404871, 0.41704633037663519, 0.00051237105587099974},
     {63286.906419970212, -15024.243184298866, -3390.1548478321738, 7466.9593388987414, 1735.6441340275564,
      60882.345546264121}},
    {50267,
     {0.92265576260516702, 0.41239010657838182, 0.0038954823154100479},
     {48625.873986830993, -11787.260840403671, -5438.1934139552905, 5770.8964658120212, 2281.6243718955798,
      47798.249615865963}},
    {189651626,
     {0.91955467453943696, 0.40152411171635222, 0.0027529119244888763},
     {68837.592609416533, -19063.271962843293, -8570.5882079373878, 7706.9495405674543, 3155.1202337300369,
      71101.807199866307}},
    {192316237,
     {0.92119636334663257, 0.40658907691583845, 0.0041816720259268874},
     {51999.691055855961, -13491.080432825996, -5846.7917663707503, 5976.0369808297437, 2523.6091222113791,
      52630.391189790775}},
    {194449197,
     {0.92045704951367413, 0.40727437615797468, 0.00055091344431307493},
     {71805.80128784712, -19059.542580513724, -8510.1440182671522, 8091.4121619019634, 3408.3578937624179,
      72936.875151950924}},
    {200812890,
     {0.92068832659753419, 0.40717156429676965, 0.00051871026064978466},
     {74416.058393359795, -19729.291860495599, -8959.5264550130232, 8276.6993857121997, 3388.5581009601583,
      75283.304082375427}},
    {200812891,
     {0.92043806710020437, 0.40517918591893143, 0.0027797025469369348},
     {62406.823749090538, -16660.449692317507, -7410.2223358038182, 7053.2799004638955, 2932.5110818314988,
      63609.517614672877}},
    {200812895,
     {0.92132241547199878, 0.40892747833593829, 0.0013012764535616042},
     {67945.305808667807, -17655.784448149305, -7850.460220316525, 7659.6574132823116, 3038.0903927170352,
      68216.619276653961}},
    {201464727,
     {0.92126588810669585, 0.40821430781106921, 0.00048348007542790421},
     {74316.216742398552, -19456.459659522701, -8238.5995981802571, 8306.784444628971, 3159.3873360625817,
      74616.768157129816}},
    {202634035,
     {0.92010193094547399, 0.40500644022073673, 0.002474340758810293},
     {63274.485444887679, -16940.427976226321, -7543.5082047791002, 7160.4075078837523, 2993.0037926902355,
      64580.457142442421}},
}};

// Original IDs and stored corner orders, shared by both stars.
constexpr std::array<M29FixtureCell, 16> M29FixtureCells{{
    {152845586, {200812891, 201464727, 50267, 192316237}},
    {152845587, {200812891, 189651626, 201464727, 192316237}},
    {152845588, {200812891, 189651626, 200812890, 201464727}},
    {152845589, {200812895, 201464727, 8621, 50267}},
    {152845590, {200812895, 200812890, 8621, 201464727}},
    {152845591, {200812895, 200812891, 201464727, 50267}},
    {152845592, {200812895, 200812891, 200812890, 201464727}},
    {153731208, {200812891, 192316237, 50267, 202634035}},
    {153731209, {200812891, 192316237, 202634035, 189651626}},
    {153731210, {200812891, 200812890, 189651626, 202634035}},
    {153731211, {200812895, 194449197, 202634035, 50267}},
    {153731212, {200812895, 200812891, 50267, 202634035}},
    {153731213, {200812895, 194449197, 200812890, 202634035}},
    {153731214, {200812895, 200812891, 202634035, 200812890}},
    {178709767, {200812895, 194449197, 50267, 8621}},
    {178709768, {200812895, 194449197, 8621, 200812890}},
}};

// Builds a ten-cell star with coordinates A*x and metric A^-T*M*A^-1.
std::vector<DonorCell> FineMoveStar(Id seed, double z_scale) {
  std::vector<DonorCell> donors;
  for (const auto& row : M29FixtureCells) {
    if (std::find(row.vertices.begin(), row.vertices.end(), seed) == row.vertices.end()) continue;
    DonorCell donor;
    donor.cell.id = row.id;
    for (size_t corner = 0; corner < row.vertices.size(); ++corner) {
      const Id vertex = row.vertices[corner];
      const auto original =
          std::find_if(M29FixtureVertices.begin(), M29FixtureVertices.end(),
                       [vertex](const M29FixtureVertex& candidate) { return candidate.id == vertex; });
      REQUIRE(original != M29FixtureVertices.end());
      auto point = original->point;
      auto metric = original->metric;
      point.z *= z_scale;
      metric.xz /= z_scale;
      metric.yz /= z_scale;
      metric.zz /= z_scale * z_scale;
      donor.cell.v[corner] = {vertex, point};
      donor.sensor[corner] = metric;
    }
    donors.push_back(donor);
  }
  REQUIRE(donors.size() == 10);
  return donors;
}

// Holds the original-field debt and minimum joint q/J score across the current cavity.
struct FineFixtureShape {
  long double debt = 0;
  double minimum = INFINITY;
};

// Re-measures each cell in the same current-nodal field at its actual centroid.
FineFixtureShape FineShape(const Owned& mesh, FrozenField& field) {
  FineFixtureShape shape;
  for (const auto& row : mesh) {
    const auto& cell = row.second.cell;
    const auto& vertices = cell.v;
    const auto measures =
        Measure({vertices[0].p, vertices[1].p, vertices[2].p, vertices[3].p}, field.Query(Centroid(cell)).target);
    shape.debt += NormalizedShapeHingeDebt(measures.mean_ratio, measures.minimum_scaled_jacobian);
    shape.minimum = std::min({shape.minimum, measures.mean_ratio / MinimumMeanRatio,
                              measures.minimum_scaled_jacobian / MinimumScaledJacobian});
  }
  return shape;
}

// Reports before/after shape and the optimizer's search counters for one frozen cavity.
struct FineFixtureResult {
  bool changed = false;
  FineFixtureShape before, after;
  OptimizeReport report;
};

// Restricts movement to the star center and runs one smoothing search using the cavity P1 target.
FineFixtureResult RunFineFixture(const char* fixture, std::vector<DonorCell> donors, Id seed, double z_scale,
                                 bool fine) {
  Owned mesh;
  Id next_node = 1, next_cell = 1;
  OptimizeOptions options;
  options.fine_placement = fine;
  options.maximum_sweeps = 1;
  options.swaps = options.collapses = options.insertions = options.boundary = false;
  for (const auto& donor : donors) {
    const auto& cell = donor.cell;
    OwnedCell row;
    row.cell = cell;
    mesh.emplace(cell.id, row);
    next_cell = std::max(next_cell, cell.id + 1);
    for (const auto& node : cell.v) {
      next_node = std::max(next_node, node.id + 1);
      if (node.id != seed) options.frozen_vertices.insert(node.id);
    }
  }
  FrozenField field(std::move(donors), {}, false, {}, {}, {}, true, MaximumDonors, true);
  const RegionMetric target(field);
  // Keeps this fixture on the optimizer's production debt path and restores other tests' setting.
  struct RestoreShapeMode {
    ShapeAdmission saved = GetShapeAdmission();
    ~RestoreShapeMode() { SetShapeAdmission(saved); }
  } restore;
  SetShapeAdmission(ShapeAdmission::ValidityOnly);

  FineFixtureResult result;
  result.before = FineShape(mesh, field);
  result.changed = OptimizeMetricMesh(mesh, target, next_node, next_cell, options, result.report);
  result.after = FineShape(mesh, field);
  std::printf(
      "NATIVE3D_FINE_FIXTURE fixture=%s vertex=%llu z_scale=%.17g fine=%d changed=%d cells=%zu "
      "placements=%zu evaluations=%llu moves=%zu debt_before=%.17Lg debt_after=%.17Lg "
      "minimum_before=%.17g minimum_after=%.17g\n",
      fixture, static_cast<unsigned long long>(seed), z_scale, int(fine), int(result.changed), mesh.size(),
      result.report.placements, static_cast<unsigned long long>(result.report.placement_evaluations),
      result.report.interior_moves, result.before.debt, result.after.debt, result.before.minimum, result.after.minimum);
  return result;
}
}  // namespace

TEST_CASE("N20 F1 exact fine placement in the two M29 ten-cell stars", "[NativeFinePlacement3D]") {
  constexpr Id closes = 200812891, improves = 200812895;
  RunFineFixture("F1", FineMoveStar(closes, 1), closes, 1, false);
  RunFineFixture("F1", FineMoveStar(improves, 1), improves, 1, false);

  const auto closed = RunFineFixture("F1", FineMoveStar(closes, 1), closes, 1, true);
  REQUIRE(closed.before.debt > 0);
  CHECK(closed.after.debt == 0);
  CHECK(closed.after.minimum >= 1.003);
  CHECK(closed.report.placement_evaluations <= 512);

  const auto improved = RunFineFixture("F1", FineMoveStar(improves, 1), improves, 1, true);
  REQUIRE(improved.before.debt > 0);
  CHECK(improved.after.debt < 0.5L * improved.before.debt);
  CHECK(improved.report.placement_evaluations <= 512);
}

TEST_CASE("N20 F2 exact fine placement in the anisotropically transformed M29 star", "[NativeFinePlacement3D]") {
  constexpr Id seed = 200812891;
  constexpr double scale = 1000;
  RunFineFixture("F2", FineMoveStar(seed, scale), seed, scale, false);
  const auto closed = RunFineFixture("F2", FineMoveStar(seed, scale), seed, scale, true);
  REQUIRE(closed.before.debt > 0);
  CHECK(closed.after.debt == 0);
  CHECK(closed.after.minimum >= 1.003);
  CHECK(closed.report.placement_evaluations <= 512);
}

TEST_CASE("N20 F3 exact fine placement leaves an already good movable star alone", "[NativeFinePlacement3D]") {
  const std::array<Node, 4> skin{{{1, {0, 0, 0}}, {2, {1, 0, 0}}, {3, {0, 1, 0}}, {4, {0, 0, 1}}}};
  const Node center{5, {0.25, 0.25, 0.25}};
  std::vector<DonorCell> donors;
  for (size_t omit = 0; omit < skin.size(); ++omit) {
    DonorCell donor;
    donor.cell = {omit + 1, {center}};
    size_t corner = 1;
    for (size_t i = 0; i < skin.size(); ++i)
      if (i != omit) donor.cell.v[corner++] = skin[i];
    auto& vertices = donor.cell.v;
    if (Orientation(vertices[0].p, vertices[1].p, vertices[2].p, vertices[3].p) < 0)
      std::swap(vertices[0], vertices[1]);
    donor.sensor.fill(Tensor{});
    donors.push_back(donor);
  }
  const auto result = RunFineFixture("F3", std::move(donors), center.id, 1, true);
  REQUIRE(result.before.debt == 0);
  REQUIRE(result.before.minimum >= 1);
  CHECK_FALSE(result.changed);
  CHECK(result.report.interior_moves == 0);
  CHECK(result.report.placement_evaluations == 0);
}
