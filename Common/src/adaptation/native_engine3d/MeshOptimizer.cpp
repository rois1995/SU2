// ---- DC-12 (OPTIMIZE_PLAN sect. 4/6): mesh-wide metric optimization and the serial build-then-optimize driver
// ----------
namespace {
// Hashes the original coordinate bits used by the tensor memo.
struct CoordinateHash {
  size_t operator()(const std::array<double, 3>& k) const {
    uint64_t h = 1469598103934665603ULL;
    for (const double v : k) {
      uint64_t b = 0;
      std::memcpy(&b, &v, sizeof b);
      h = (h ^ b) * 1099511628211ULL;
    }
    return size_t(h);
  }
};
// Returns the outward face order indexed by opposite vertex.
std::array<Face, 4> OppositeFaces(const Cell& c) {
  const auto& v = c.v;
  return {{{v[1].id, v[2].id, v[3].id},
           {v[0].id, v[3].id, v[2].id},
           {v[0].id, v[1].id, v[3].id},
           {v[0].id, v[2].id, v[1].id}}};
}
// Reports whether a cell contains the indicated identity.
bool CellHas(const Cell& c, Id v) {
  for (const auto& n : c.v)
    if (n.id == v) return true;
  return false;
}
// Holds the measured shape scores and the original validity sentinels.
struct CellQuality {
  double q = -1, J = -1;
  double Score() const { return std::min(q / MinimumMeanRatio, J / MinimumScaledJacobian); }
  bool Valid() const { return std::isfinite(q) && std::isfinite(J) && q >= 0 && J >= 0; }
};
// ---- DC-13 placement engine (DC13_PLAN sect. 1.1): proposals only, every acceptance re-measures exactly ----------
// Upper Cholesky factor R of an SPD tensor (M = R^T R); ok=false if not SPD in double.
// Holds the upper factor of the symmetric positive definite metric.
struct Factor3 {
  double r00 = 0, r01 = 0, r02 = 0, r11 = 0, r12 = 0, r22 = 0;
  bool ok = false;
};
// Factors the point tensor used to place a proposal.
Factor3 FactorOf(const Tensor& m) {
  Factor3 c;
  if (!(m.xx > 0)) return c;
  c.r00 = std::sqrt(m.xx);
  c.r01 = m.xy / c.r00;
  c.r02 = m.xz / c.r00;
  const double p11 = m.yy - c.r01 * c.r01;
  if (!(p11 > 0)) return c;
  c.r11 = std::sqrt(p11);
  c.r12 = (m.yz - c.r01 * c.r02) / c.r11;
  const double p22 = m.zz - c.r02 * c.r02 - c.r12 * c.r12;
  if (!(p22 > 0)) return c;
  c.r22 = std::sqrt(p22);
  c.ok = true;
  return c;
}
// Approximate minimum score or negative summed hinge debt in a constant tensor (same q/J as Measure);
// -INFINITY for a non-positive (determinant <= 0) or degenerate cell.
double FastScore(const std::array<Point, 4>& p, const Factor3& f, bool debt = false) {
  double y[4][3];
  for (unsigned k = 0; k < 4; ++k) {
    const double d0 = p[k].x - p[0].x, d1 = p[k].y - p[0].y, d2 = p[k].z - p[0].z;
    y[k][0] = f.r00 * d0 + f.r01 * d1 + f.r02 * d2;
    y[k][1] = f.r11 * d1 + f.r12 * d2;
    y[k][2] = f.r22 * d2;
  }
  auto e = [&](unsigned i, unsigned j, unsigned c) { return y[j][c] - y[i][c]; };
  const double u[3] = {e(0, 1, 0), e(0, 1, 1), e(0, 1, 2)}, v[3] = {e(0, 2, 0), e(0, 2, 1), e(0, 2, 2)},
               w[3] = {e(0, 3, 0), e(0, 3, 1), e(0, 3, 2)};
  const double det =
      u[0] * (v[1] * w[2] - v[2] * w[1]) - u[1] * (v[0] * w[2] - v[2] * w[0]) + u[2] * (v[0] * w[1] - v[1] * w[0]);
  if (!(det > 0) || !std::isfinite(det)) return -INFINITY;
  double l2[4][4] = {{0}}, sum = 0;
  for (unsigned i = 0; i < 4; ++i)
    for (unsigned j = i + 1; j < 4; ++j) {
      double s = 0;
      for (unsigned c = 0; c < 3; ++c) {
        const double d = e(i, j, c);
        s += d * d;
      }
      l2[i][j] = l2[j][i] = s;
      sum += s;
    }
  const double root = std::cbrt(det / 2), q = 12 * root * root / sum;
  double J = 1;
  for (unsigned k = 0; k < 4; ++k) {
    double den = 1;
    for (unsigned j = 0; j < 4; ++j)
      if (j != k) den *= std::sqrt(l2[k][j]);
    J = std::min(J, den > 0 ? std::sqrt(2.) * det / den : 0.);
  }
  return debt ? -double(NormalizedShapeHingeDebt(q, J)) : std::min(q / MinimumMeanRatio, J / MinimumScaledJacobian);
}
// Boundary faces of a cell set (faces used once inside the set), oriented as in their cell (opposite vertex inside).
std::vector<Face> CavityBoundary(const std::vector<Cell>& cells) {
  std::map<Face, std::pair<int, Face>> uses;
  for (const auto& c : cells)
    for (auto f : OppositeFaces(c)) {
      auto k = f;
      std::sort(k.begin(), k.end());
      auto& u = uses[k];
      ++u.first;
      u.second = f;
    }
  std::vector<Face> out;
  for (const auto& u : uses)
    if (u.second.first == 1) out.push_back(u.second.second);
  return out;
}
// Builds one constant proposal metric and coordinate base per face.
bool FillConeBases(const std::vector<Face>& faces, const std::function<Point(Id)>& point, const Tensor& t0,
                   const std::map<Id, Tensor>& vt, std::vector<std::array<Point, 3>>& base,
                   std::vector<Factor3>& factor) {
  for (const auto& f : faces) {
    Tensor m = t0;
    for (Id v : f) {
      const auto& t = vt.at(v);
      m.xx += t.xx;
      m.xy += t.xy;
      m.xz += t.xz;
      m.yy += t.yy;
      m.yz += t.yz;
      m.zz += t.zz;
    }
    m.xx /= 4;
    m.xy /= 4;
    m.xz /= 4;
    m.yy /= 4;
    m.yz /= 4;
    m.zz /= 4;
    const auto c = FactorOf(m);
    if (!c.ok) return false;
    factor.push_back(c);
    base.push_back({point(f[0]), point(f[1]), point(f[2])});
  }
  return true;
}
// Placement of one new point in a star-shaped cavity, using minimum score or summed deficit. The tensor of a
// cone cell is the arithmetic mean of the point tensors at its three face vertices and at the start point (an SPD
// proposal metric); deterministic best-improvement compass search in the metric frame of the start point (6 axes and
// 8 diagonals, step .25 halving to 1e-3). `admissible` restricts the search region (e.g. the size-split region).
// Records the best approximate point and the number of evaluations.
struct Placement {
  bool ok = false;
  Point x;
  double score = -INFINITY;
  size_t evaluations = 0;
};
Placement PlaceCone(const std::vector<Face>& faces, const std::function<Point(Id)>& point,
                    const std::function<Tensor(Point)>& at, Point start, const std::function<bool(Point)>& admissible,
                    size_t budget = 160, bool debt = false) {
  Placement out;
  if (faces.empty()) return out;
  // a start outside the target's coverage (e.g. a smoothing proposal beyond the domain) is no proposal, not an error
  Tensor t0;
  std::map<Id, Tensor> vt;
  try {
    t0 = at(start);
    for (const auto& f : faces)
      for (Id v : f)
        if (!vt.count(v)) vt.emplace(v, at(point(v)));
  } catch (const std::exception&) {
    return out;
  }
  std::vector<std::array<Point, 3>> base;
  std::vector<Factor3> factor;
  if (!FillConeBases(faces, point, t0, vt, base, factor)) return out;
  auto F = [&](Point x) -> double {
    ++out.evaluations;
    if (admissible && !admissible(x)) return -INFINITY;
    double s = debt ? 0 : INFINITY;
    for (size_t k = 0; k < base.size() && s > -INFINITY; ++k) {
      const auto part = FastScore({x, base[k][0], base[k][1], base[k][2]}, factor[k], debt);
      s = debt ? s + part : std::min(s, part);
    }
    return s;
  };
  // metric frame of the start: d = R0^{-1} e (unit metric length)
  const auto R = FactorOf(t0);
  if (!R.ok) return out;
  auto frame = [&](double e0, double e1, double e2) -> Point {
    const double z = e2 / R.r22, y = (e1 - R.r12 * z) / R.r11, x = (e0 - R.r01 * y - R.r02 * z) / R.r00;
    return {x, y, z};
  };
  std::vector<Point> dirs;
  const double r3 = 1 / std::sqrt(3.);
  for (int a = 0; a < 3; ++a)
    for (int s : {-1, 1}) {
      double e[3] = {0, 0, 0};
      e[a] = s;
      dirs.push_back(frame(e[0], e[1], e[2]));
    }
  for (int sx : {-1, 1})
    for (int sy : {-1, 1})
      for (int sz : {-1, 1}) dirs.push_back(frame(sx * r3, sy * r3, sz * r3));
  Point x = start;
  double best = F(x), h = .25;
  while (h >= 1e-3 && out.evaluations < budget) {
    Point bx = x;
    double bs = best;
    for (const auto& d : dirs) {
      const Point y{x.x + h * d.x, x.y + h * d.y, x.z + h * d.z};
      const double s = F(y);
      if (s > bs) {
        bs = s;
        bx = y;
      }
      if (out.evaluations >= budget) break;
    }
    if (bs > best) {
      best = bs;
      x = bx;
    } else
      h /= 2;
  }
  out.ok = std::isfinite(best);
  out.x = x;
  out.score = best;
  return out;
}
// Owns one metric optimizer invocation and its local topology and metric caches.
class MeshOptimizer {
 public:
  MeshOptimizer(Owned& mesh, const RegionMetric& target, Id& next_node, Id& next_cell, const OptimizeOptions& options,
                OptimizeReport& report)
      : mesh(mesh),
        target(target),
        next_node(next_node),
        next_cell(next_cell),
        options(options),
        report(report),
        started(Clock::now()) {}
  bool Run();

 private:
  static constexpr double Gain = 1e-6;
  static constexpr long double EstimatePass = 1.4L;  // new edges estimated at most this long are not integrated
  Owned& mesh;
  const RegionMetric& target;
  Id& next_node;
  Id& next_cell;
  const OptimizeOptions& options;
  OptimizeReport& report;
  Clock::time_point started;
  std::unique_ptr<PlanarReference> moved_geometry;
  bool changed = false, limited = false, coupled = true;
  size_t stage_work_end = SIZE_MAX;
  std::unordered_map<Id, std::vector<Id>> star;  // vertex -> incident cells
  std::unordered_map<Id, Point> points;
  std::map<Face, Id> physical;  // Current physical faces for first-height deficits.
  std::set<Id> fixed;           // skin vertices: only geometry-certified movement may change them
  std::map<Id, std::array<double, 3>>
      sliding;        // TEST-ONLY diagnostic sliding planes (options.diagnostic_sliding, follows moves)
  std::set<Id> keep;  // options.keep_vertices, following moves (never collapsed)
  std::unordered_map<Id, double> height_scores;  // Current cells are immutable by identity.
  std::unordered_map<std::array<double, 3>, Tensor, CoordinateHash> tensors;
  std::map<std::array<Id, 4>, CellQuality> qualities;  // ordered cell vertex ids (an identity keeps its coordinates)
  std::map<std::array<double, 6>, long double> exact;
  // Target/options are fixed; pure-shape rejections depend on the complete vertex star.
  std::set<Id> rejected_smoothing;
  // Checks the sticky invocation deadline and the current stage's work allotment.
  bool Limited() {
    if (limited) return true;
    if (report.trials >= options.maximum_work) limited = true;
    if (options.maximum_seconds > 0 &&
        std::chrono::duration<double>(Clock::now() - started).count() >= options.maximum_seconds)
      limited = true;
    return limited || report.trials >= stage_work_end;
  }
  // Clears each cache at its original configured entry bound.
  void Bound() {
    if (height_scores.size() >= options.cache_entries) height_scores.clear();
    if (tensors.size() >= options.cache_entries) tensors.clear();
    if (qualities.size() >= options.cache_entries) qualities.clear();
    if (exact.size() >= options.cache_entries) exact.clear();
  }
  // Rebuilds topology and boundary constraints from the current mesh.
  void Index() {
    star.clear();
    points.clear();
    fixed.clear();
    physical.clear();
    std::map<Face, int> uses;
    for (const auto& row : mesh) {
      for (const auto& n : row.second.cell.v) {
        star[n.id].push_back(row.first);
        points[n.id] = n.p;
      }
      const auto faces = OppositeFaces(row.second.cell);
      for (unsigned k = 0; k < 4; ++k) {
        const auto f = Key(faces[k]);
        ++uses[f];
        if (row.second.physical[k].present) physical.emplace(f, row.second.physical[k].facet);
      }
    }
    for (const auto& u : uses)
      if (u.second == 1) fixed.insert(u.first.begin(), u.first.end());
    sliding = options.diagnostic_sliding;
    keep = options.keep_vertices;
  }
  // Looks up or measures a point tensor and accounts for a miss.
  Tensor At(Point p) {
    const std::array<double, 3> key{p.x, p.y, p.z};
    const auto found = tensors.find(key);
    if (found != tensors.end()) return found->second;
    ++report.point_queries;
    const auto m = target.point(p);
    if (tensors.size() >= options.cache_entries) tensors.clear();
    tensors.emplace(key, m);
    return m;
  }
  // q, J in the metric at the actual centroid; q = J = -1 for a non-positive or unmeasurable cell
  CellQuality Quality(const Cell& c, bool memo = true) {
    if (!(Orientation(c.v[0].p, c.v[1].p, c.v[2].p, c.v[3].p) > 0)) return {};
    const std::array<Id, 4> key{c.v[0].id, c.v[1].id, c.v[2].id, c.v[3].id};
    if (memo) {
      const auto found = qualities.find(key);
      if (found != qualities.end()) return found->second;
    }
    CellQuality r;
    try {
      const auto m = Measure({c.v[0].p, c.v[1].p, c.v[2].p, c.v[3].p}, At(Centroid(c)));
      r.q = m.mean_ratio;
      r.J = m.minimum_scaled_jacobian;
    } catch (const std::exception&) {
      r = CellQuality{};
    }
    if (memo) {
      if (qualities.size() >= options.cache_entries) qualities.clear();
      qualities.emplace(key, r);
    }
    return r;
  }
  // Returns the exact cell shape score.
  double Score(const Cell& c) { return Quality(c).Score(); }
  // DC-17: score of a current mesh cell, from the caller's identity memo when given (same exact measure)
  Patch GeometryPatch(const std::vector<Cell>& cells) const {
    Patch patch;
    patch.cells = cells;
    for (const auto& c : cells)
      for (auto f : OppositeFaces(c)) {
        const auto b = physical.find(Key(f));
        if (b != physical.end()) patch.surface.push_back({f, b->second});
      }
    if (options.geometry) {
      patch.parameters = report.parameters;
      patch.coverage = report.coverage;
      RetainPhysicalCoverage(patch);
      const auto& geometry = moved_geometry ? *moved_geometry : *options.geometry;
      geometry.ExportParameters(patch);
    }
    return patch;
  }
  // Measures requested first height with the matching private parameters.
  double Height(const Patch& patch) const {
    if (!options.geometry || options.wall_heights.empty()) return INFINITY;
    const auto& base = moved_geometry ? *moved_geometry : *options.geometry;
    auto parameters = base.PrivateParameters(patch);
    return HeightScore(patch, options.wall_heights, parameters ? *parameters : base);
  }
  // Reads the caller's cached shape if available, then the local memo.
  CellQuality RowQuality(Id id, const Cell& c) {
    CellQuality q;
    if (!options.mesh_quality || !options.mesh_quality(id, q.q, q.J)) q = Quality(c);
    return q;
  }
  // Memoizes the current cell's requested-height score.
  double RowHeight(Id id, const Cell& c) {
    if (options.wall_heights.empty()) return INFINITY;
    auto found = height_scores.find(id);
    if (found == height_scores.end()) {
      if (height_scores.size() >= options.cache_entries) height_scores.clear();
      found = height_scores.emplace(id, Height(GeometryPatch({c}))).first;
    }
    return found->second;
  }
  // Returns the lesser of the current shape and height scores.
  double RowScore(Id id, const Cell& c) { return std::min(RowQuality(id, c).Score(), RowHeight(id, c)); }
  // Ranks cells by the active merit without certifying proposals.
  long double RowPriority(Id id, const Cell& c) {
    if (GetShapeAdmission() != ShapeAdmission::ValidityOnly) return RowScore(id, c);
    const auto q = RowQuality(id, c);
    long double debt = NormalizedShapeHingeDebt(q.q, q.J);
    const auto h = RowHeight(id, c);
    if (h < 1) {
      const auto deficit = 1 - static_cast<long double>(h);
      debt += deficit * deficit;
    }
    return -debt;
  }
  // Finds the minimum score in the listed current cells.
  double MinScore(const std::vector<Id>& ids) {
    double m = INFINITY;
    for (Id id : ids) m = std::min(m, RowScore(id, mesh.at(id).cell));
    return m;
  }
  // Minimum score of finite, exact-positive fresh cells.
  double FreshScore(const std::vector<Cell>& cells, bool memo = true, const Patch* patch = nullptr) {
    double m = INFINITY;
    for (const auto& c : cells) {
      const auto r = Quality(c, memo);
      if (!r.Valid()) return -INFINITY;
      m = std::min(m, r.Score());
    }
    return options.wall_heights.empty() ? m : std::min(m, Height(patch ? *patch : GeometryPatch(cells)));
  }
  // Height repair retains its existing merit until the actual requested constraints close.
  bool DebtMode(const std::vector<Id>& ids, const Patch* patch = nullptr) const {
    if (GetShapeAdmission() != ShapeAdmission::ValidityOnly) return false;
    if (options.wall_heights.empty()) return true;
    if (patch) return Height(*patch) >= 1;
    std::vector<Cell> cells;
    for (Id id : ids) cells.push_back(mesh.at(id).cell);
    return Height(GeometryPatch(cells)) >= 1;
  }
  // Measures the current cavity under its original timer boundary.
  long double OldMerit(const std::vector<Id>& ids, bool debt) {
    SU2NativeTiming3D::Scope old_cavity_scope(SU2NativeTiming3D::Operation::OldCavityMeasurement);
    if (!debt) return MinScore(ids);
    long double sum = 0;
    for (Id id : ids) {
      const auto q = RowQuality(id, mesh.at(id).cell);
      if (!q.Valid()) return -INFINITY;
      sum += NormalizedShapeHingeDebt(q.q, q.J);
    }
    return -sum;
  }
  // Measures candidate shape and rejects a new requested-height failure.
  long double FreshMerit(const std::vector<Cell>& cells, bool debt, bool memo = true, const Patch* patch = nullptr) {
    if (!debt) return FreshScore(cells, memo, patch);
    if (cells.empty()) return -INFINITY;
    long double sum = 0;
    for (const auto& c : cells) {
      const auto q = Quality(c, memo);
      if (!q.Valid()) return -INFINITY;
      sum += NormalizedShapeHingeDebt(q.q, q.J);
    }
    if (!options.wall_heights.empty() && !(Height(patch ? *patch : GeometryPatch(cells)) >= 1)) return -INFINITY;
    return -sum;
  }
  // Applies the active comparison to the measured candidate and original merit.
  bool Progress(long double after, long double before, bool debt, bool length = false) const {
    if (!std::isfinite(after)) return false;
    if (debt) return std::isfinite(before) && (length ? after >= before : ShapeDebtDecreases(-before, -after));
    const double floor = length ? std::min(double(before), 1.) - 1e-12 : double(before) + Gain;
    return double(after) > floor;
  }
  // Estimates a target edge length and charges the surrogate query.
  long double Estimate(Point a, Point b) {
    const Point mid{double(((long double)a.x + b.x) / 2), double(((long double)a.y + b.y) / 2),
                    double(((long double)a.z + b.z) / 2)};
    ++report.edge_surrogates;
    return (MetricNorm(a, b, At(a)) + 4 * MetricNorm(a, b, At(mid)) + MetricNorm(a, b, At(b))) / 6;
  }
  // Caches the integrated edge length under the undirected coordinate key.
  long double Exact(Point a, Point b) {
    std::array<double, 6> key{a.x, a.y, a.z, b.x, b.y, b.z};
    if (std::make_tuple(b.x, b.y, b.z) < std::make_tuple(a.x, a.y, a.z)) key = {b.x, b.y, b.z, a.x, a.y, a.z};
    const auto found = exact.find(key);
    if (found != exact.end()) return found->second;
    ++report.edge_integrations;
    const auto l = target.edge(a, b);
    if (exact.size() >= options.cache_entries) exact.clear();
    exact.emplace(key, l);
    return l;
  }
  // Edge guard of an accepted candidate: the exact composed length when the estimate exceeds EstimatePass.
  bool Certified(Point a, Point b, long double limit) {
    try {
      const auto e = Estimate(a, b);
      if (e <= EstimatePass) return true;
      if (e > 1.45L * limit) return false;
      return Exact(a, b) <= limit;
    } catch (const std::exception&) {
      return false;
    }
  }
  // Checks whether topology changes can touch the named current cell.
  bool Frozen(Id cell) const {  // no topology change (DC-17: also cells holding a structure vertex)
    if (options.frozen_cells.count(cell)) return true;
    if (options.structure_vertices.empty()) return false;
    const auto f = mesh.find(cell);
    if (f == mesh.end()) return false;
    for (const auto& n : f->second.cell.v)
      if (options.structure_vertices.count(n.id)) return true;
    return false;
  }
  // Checks the fixed-vertex rule and the bounded, unfrozen incident star.
  bool Movable(Id v) const {
    if ((fixed.count(v) && !sliding.count(v)) || options.frozen_vertices.count(v)) return false;
    const auto s = star.find(v);
    if (s == star.end() || s->second.empty() || s->second.size() > MaximumCavityCells) return false;
    for (Id id : s->second)
      if (options.frozen_cells.count(id)) return false;  // moves keep the topology of structure cells
    return true;
  }
  // Intersects two incident-cell lists in canonical order.
  static std::vector<Id> Common(std::vector<Id> x, std::vector<Id> y) {
    std::vector<Id> out;
    std::sort(x.begin(), x.end());
    std::sort(y.begin(), y.end());
    std::set_intersection(x.begin(), x.end(), y.begin(), y.end(), std::back_inserter(out));
    return out;
  }
  // Checks for a current edge from the first vertex to the second.
  bool Adjacent(Id a, Id b) const {
    const auto s = star.find(a);
    if (s == star.end()) return false;
    for (Id id : s->second)
      if (CellHas(mesh.at(id).cell, b)) return true;
    return false;
  }
  // Exact interface/ball-topology certificate of a replacement (temporary identities from next_cell).
  bool Interface(const std::vector<Id>& old, std::vector<Cell>& fresh) {
    std::vector<Cell> before;
    before.reserve(old.size());
    for (Id id : old) before.push_back(mesh.at(id).cell);
    for (size_t i = 0; i < fresh.size(); ++i) fresh[i].id = next_cell + i;
    std::string reason;
    bool ok = false;
    try {
      ok = ValidateFixedInterface(before, fresh, reason, nullptr, std::max<size_t>(MaximumCavityCells, before.size()));
    } catch (const std::exception&) {
      ok = false;
    }
    if (!ok) ++report.rejected_interface;
    return ok;
  }
  // Replace old cells by fresh ones: every physical binding of an old face that survives is carried by its face key;
  // `added` binds the boundary faces a boundary operation creates (the faces it retires disappear with their cells).
  void Commit(const std::vector<Id>& old, std::vector<Cell> fresh, Id removed = UINT64_MAX,
              const std::map<Face, FacetBinding>& added = {}) {
    std::map<Face, FacetBinding> bindings = added;
    std::set<Face> oldPhysical;
    for (Id id : old) {
      const auto& row = mesh.at(id);
      const auto faces = OppositeFaces(row.cell);
      for (unsigned k = 0; k < 4; ++k)
        if (row.physical[k].present) {
          auto f = faces[k];
          std::sort(f.begin(), f.end());
          bindings.emplace(f, row.physical[k]);
          oldPhysical.insert(f);
          physical.erase(f);
        }
      for (const auto& n : row.cell.v) {
        rejected_smoothing.erase(n.id);
        auto& s = star[n.id];
        s.erase(std::remove(s.begin(), s.end(), id), s.end());
      }
      mesh.erase(id);
    }
    std::set<Face> used;
    for (auto& c : fresh) {
      c.id = next_cell++;
      OwnedCell row;
      row.cell = c;
      row.version = 1;
      const auto faces = OppositeFaces(c);
      for (unsigned k = 0; k < 4; ++k) {
        auto f = faces[k];
        std::sort(f.begin(), f.end());
        const auto b = bindings.find(f);
        if (b != bindings.end()) {
          row.physical[k] = b->second;
          used.insert(f);
          physical.insert_or_assign(f, b->second.facet);
        }
      }
      if (!mesh.emplace(c.id, row).second) throw std::logic_error("Native3D optimizer repeated a cell identity.");
      for (const auto& n : c.v) {
        rejected_smoothing.erase(n.id);
        star[n.id].push_back(c.id);
        points[n.id] = n.p;
      }
    }
    for (const auto& a : added)
      if (!used.count(a.first)) throw std::logic_error("Native3D optimizer lost a created boundary face.");
    if (added.empty())
      for (const auto& f : oldPhysical)
        if (!used.count(f)) throw std::logic_error("Native3D optimizer lost a physical face binding.");
    if (removed != UINT64_MAX) {
      const auto s = star.find(removed);
      if (s != star.end() && !s->second.empty()) throw std::logic_error("Native3D optimizer removed a used vertex.");
      star.erase(removed);
      points.erase(removed);
    }
    changed = true;
    Bound();
  }
  // Caches a pure-shape refusal up to the existing entry bound.
  void RememberRejectedSmooth(Id p) {
    if (!options.cache_entries) return;
    if (rejected_smoothing.size() >= options.cache_entries) rejected_smoothing.clear();
    rejected_smoothing.insert(p);
  }
  bool EdgeRemoval(Id a, Id b, bool lengthDriven = false);
  bool FaceSwap(const Face& f);
  bool Collapse(Id p, Id q);
  bool Smooth(Id p);
  bool SmoothBoundary(Id p);
  bool CoupledBoundary();
  void AccountNested(const OptimizeReport& nested, bool selected);
  bool ShellBoundary(Id cell);
  bool TryCell(Id id, bool enrichment);
  // DC-13 (DC13_PLAN sect. 1): cone re-meshing of a star-shaped cavity from one new placed vertex
  enum class Insert { Shell, Neighbourhood, Length };
  bool ConeReplace(const std::vector<Id>& old, Point start, Insert kind, const std::pair<Id, Id>* split = nullptr,
                   long double removed = 0);
  bool ShellInsert(Id a, Id b, bool lengthDriven = false);
  bool NeighbourhoodInsert(Id cell);
  // boundary operations (no new point; every face stays inside its facet): O5 flip of a boundary edge between two faces
  // bound to the same facet, O6 contraction of a boundary vertex interior to one facet along a boundary edge
  // Carries the oriented mesh face and its physical binding.
  struct BoundaryFace {
    Face oriented{};
    Id cell = 0;
    FacetBinding binding;
  };
  // Collects each matching physical face in cell order.
  std::vector<BoundaryFace> BoundaryFacesOf(const std::vector<Id>& cells, Id v) const {
    std::vector<BoundaryFace> out;
    for (Id id : cells) {
      const auto& row = mesh.at(id);
      const auto faces = OppositeFaces(row.cell);
      for (unsigned k = 0; k < 4; ++k)
        if (row.physical[k].present && (v == UINT64_MAX || faces[k][0] == v || faces[k][1] == v || faces[k][2] == v))
          out.push_back({faces[k], id, row.physical[k]});
    }
    return out;
  }
  // Measures the distance between two points.
  static long double Norm3(Point a, Point b) {
    const long double x = (long double)a.x - b.x, y = (long double)a.y - b.y, z = (long double)a.z - b.z;
    return std::sqrt(x * x + y * y + z * z);
  }
  // Returns twice the triangle area and its unnormalized normal.
  static long double Area2(Point a, Point b, Point c, long double n[3]) {
    const long double u[3] = {(long double)b.x - a.x, (long double)b.y - a.y, (long double)b.z - a.z},
                      w[3] = {(long double)c.x - a.x, (long double)c.y - a.y, (long double)c.z - a.z};
    n[0] = u[1] * w[2] - u[2] * w[1];
    n[1] = u[2] * w[0] - u[0] * w[2];
    n[2] = u[0] * w[1] - u[1] * w[0];
    return std::sqrt(n[0] * n[0] + n[1] * n[1] + n[2] * n[2]);
  }
  // fresh boundary = old boundary - retired + created, with the created faces inside the retired faces' plane: same
  // facet, unfolded (normals along the retired ones) and the same total area (no gap or overlap)
  bool BoundaryReplacement(const std::vector<Id>& old, std::vector<Cell>& fresh,
                           const std::vector<BoundaryFace>& retired, FacetBinding binding,
                           std::map<Face, FacetBinding>& added) {
    std::vector<Cell> before;
    for (Id id : old) before.push_back(mesh.at(id).cell);
    for (size_t i = 0; i < fresh.size(); ++i) fresh[i].id = next_cell + i;
    std::vector<Face> oldSkin, newSkin;
    try {
      oldSkin = Incidence(before).Boundary();
      newSkin = Incidence(fresh).Boundary();
    } catch (const std::exception&) {
      ++report.rejected_interface;
      return false;
    }
    auto canon = [](Face f) {
      std::rotate(f.begin(), std::min_element(f.begin(), f.end()), f.end());
      return f;
    };
    std::set<Face> retiredSet, oldSet, newSet;
    for (const auto& r : retired) retiredSet.insert(canon(r.oriented));
    for (const auto& f : oldSkin) oldSet.insert(canon(f));
    for (const auto& f : newSkin) newSet.insert(canon(f));
    for (const auto& f : oldSet)
      if (!retiredSet.count(f) && !newSet.count(f)) {
        ++report.rejected_interface;
        return false;
      }
    long double n0[3] = {0, 0, 0}, area0 = 0, area1 = 0;
    for (const auto& r : retired) {
      long double n[3];
      area0 += Area2(points.at(r.oriented[0]), points.at(r.oriented[1]), points.at(r.oriented[2]), n);
      for (int i = 0; i < 3; ++i) n0[i] += n[i];
    }
    added.clear();
    for (const auto& f : newSet) {
      if (oldSet.count(f)) continue;
      Point q[3];
      for (int i = 0; i < 3; ++i) {
        bool found = false;
        for (const auto& c : fresh)
          for (const auto& v : c.v)
            if (v.id == f[i]) {
              q[i] = v.p;
              found = true;
            }
        if (!found) return false;
      }
      long double n[3];
      const auto a = Area2(q[0], q[1], q[2], n);
      area1 += a;
      if (!(n[0] * n0[0] + n[1] * n0[1] + n[2] * n0[2] > 0)) {
        ++report.rejected_interface;
        return false;
      }
      auto key = f;
      std::sort(key.begin(), key.end());
      added.emplace(key, binding);
    }
    if (!(std::abs(area1 - area0) <= 1e-10L * area0)) {
      ++report.rejected_interface;
      return false;
    }
    std::string reason;
    bool ok = false;
    try {
      ok = ValidatePrescribedInterface(before, fresh, newSkin, reason, nullptr,
                                       std::max<size_t>(MaximumCavityCells, before.size()));
    } catch (const std::exception&) {
      ok = false;
    }
    if (!ok) ++report.rejected_interface;
    return ok;
  }
  bool BoundaryFlip(Id a, Id b);
  bool BoundaryCollapse(Id p, Id q);

  // Captures the four cumulative counters before a star search.
  struct SmoothRejections {
    size_t validity, shape, edge, geometry;
  };
  // Holds one measured interior trial coordinate and score.
  struct SmoothTrial {
    long double score;
    Point x;
  };
  enum class BoundaryDecision { Next, Admit, Stop, Fail };

  // Keeps the original atlas accounting and borrowed geometry alive for the whole star search.
  struct BoundaryMoveResources {
    SU2NativeGeometry::AtlasBudget* atlas_budget;
    // Restores the exact borrowed atlas byte charge on every exit.
    struct CallerByteRestore {
      SU2NativeGeometry::AtlasBudget* budget;
      size_t original;
      ~CallerByteRestore() {
        if (budget) budget->caller_bytes = original;
      }
    } caller_restore;
    GeometryGuidanceMemo guidance_memo;
    bool guidance_memo_ready = false;
    std::unique_ptr<PreparedVertexMove> prepared_move;
    bool preparation_attempted = false;
    std::unique_ptr<PlanarReference> projected_context;
    const PlanarReference* projected_geometry;
    SU2NativeGeometry::CurvedAtlas prepared_atlas;
    bool prepared_atlas_ok = false;

    explicit BoundaryMoveResources(const PlanarReference& geometry)
        : atlas_budget(geometry.AtlasReservation()),
          caller_restore{atlas_budget, atlas_budget ? atlas_budget->caller_bytes : 0},
          projected_geometry(&geometry) {}
    bool Charge(const PlanarReference* candidate_context = nullptr);
  };
  // Borrows the exported star and tracks its current merit and guidance.
  struct BoundarySmoothSearch {
    Id p;
    const Patch& patch;
    const PlanarReference& geometry;
    bool debt;
    long double best_score;
    long double geometric_before;
    bool geometry_measured;
  };

  long double ShapeDebt(const std::vector<Cell>& cells);
  bool CollectBoundarySmoothPatch(Id p, std::vector<Id>& old, Patch& patch, std::set<Face>& old_physical);
  bool BoundaryTargets(Id p, const Patch& patch, const PlanarReference& geometry, Point origin,
                       std::vector<Point>& targets, std::set<Id>& neighbors);
  bool FallbackBoundaryTargets(Point origin, const std::set<Id>& neighbors, std::vector<Point>& targets);
  bool PrepareBoundaryGeometry(Id p, const Patch& patch, const PlanarReference& geometry,
                               BoundaryMoveResources& resources);
  bool BoundaryProjection(BoundarySmoothSearch& search, Point target_point, BoundaryMoveResources& resources,
                          std::string& reason);
  BoundaryDecision BuildBoundaryMove(BoundarySmoothSearch& search, Point target_point, BoundaryMoveResources& resources,
                                     Patch& candidate, std::string& reason);
  BoundaryDecision BoundaryGuidance(BoundarySmoothSearch& search, const Patch& candidate,
                                    const PlanarReference& candidate_geometry, const PlanarReference* candidate_context,
                                    BoundaryMoveResources& resources, SU2NativeD9::SmoothChain& d9_chain);
  BoundaryDecision TryBoundaryMove(BoundarySmoothSearch& search, Point target_point, BoundaryMoveResources& resources,
                                   Patch& best, SU2NativeD9::SmoothChain& d9_chain);
  bool SearchBoundarySmooth(BoundarySmoothSearch& search, const std::vector<Id>& old, const std::vector<Point>& targets,
                            long double old_shape_debt, const SmoothRejections& baseline);
  bool CommitBoundarySmooth(BoundarySmoothSearch& search, const std::vector<Id>& old, const Patch& best,
                            long double old_shape_debt, SU2NativeD9::SmoothChain& d9_chain);

  void AddHeightTargets(Id p, const std::vector<Id>& old, Point x0, std::vector<Point>& targets);
  void AddMetricMeanTarget(Point x0, const std::set<Id>& nbr, std::vector<Point>& targets);
  void AddHingeTarget(Id p, const std::vector<Id>& old, Point x0, long double shortest, std::vector<Point>& targets);
  bool PlaceInteriorTargets(Id p, const std::vector<Id>& old, Point x0, bool slides, const std::array<double, 3>& plane,
                            bool debt, std::vector<Point>& targets);
  std::vector<SmoothTrial> CollectInteriorTrials(Id p, const std::vector<Id>& old, Point x0, bool debt,
                                                 long double oldMin, bool placed, const std::vector<Point>& targets,
                                                 Patch& constraints);
  void SlidingBindings(Id p, Id replacement, const std::vector<Id>& old, std::map<Face, FacetBinding>& added) const;
  bool CommitInteriorSmooth(Id p, const std::vector<Id>& old, const std::set<Id>& nbr, long double limit, bool slides,
                            const std::array<double, 3>& plane, bool debt, long double old_shape_debt,
                            std::vector<SmoothTrial>& trials);
  bool SearchInteriorSmooth(Id p, const std::vector<Id>& old, const std::set<Id>& nbr, Point x0, bool debt,
                            long double oldMin, long double old_shape_debt, long double shortest, long double limit,
                            const SmoothRejections& baseline);
  void RememberInteriorFailure(Id p, long double oldMin, const SmoothRejections& baseline);

  bool RemoveInteriorRing(const std::vector<Id>& shell, const std::vector<Id>& order, Node A, Node B,
                          bool lengthDriven);
  bool OrientBoundaryRing(const std::vector<BoundaryFace>& faces, const std::vector<Id>& shell,
                          std::map<Id, std::vector<Id>>& ring, std::vector<Id>& order, Id& a, Id& b, Point& pa,
                          Point& pb);
  bool FlipBoundaryRing(const std::vector<Id>& shell, const std::vector<BoundaryFace>& faces,
                        const std::vector<Id>& order, Node A, Node B);

  bool PrepareCoupledBoundary(const std::vector<Id>& shell, Id a, Id b, Id core, const PlanarReference& geometry);
  bool CertifyConeReplacement(const std::vector<Id>& old, const std::vector<Face>& faces, const std::set<Id>& vertices,
                              const Placement& pl, Insert kind, const std::pair<Id, Id>* split, long double removed,
                              bool debt, long double oldMin);
  bool CommitShellBoundary(const std::vector<Id>& shell, const Patch& fresh, const PlanarReference& geometry,
                           const std::map<Face, FacetBinding>& bindings);
  static Patch CoupledPatch(const Owned& rows, const Patch& original, const Patch& refined);
  bool OptimizeCoupledBoundary(const std::vector<Id>& all_ids, const Patch& original, const Patch& refined,
                               Owned& pending, const PlanarReference& geometry, std::string& reason);
  std::array<size_t, 2> ShapeFailures(const std::vector<Cell>& cells);
  bool CertifyCoupledBoundary(const std::vector<Id>& all_ids, const Patch& original, const Patch& refined,
                              Owned& pending, const PlanarReference& geometry, std::string& reason, Id node, Id cell,
                              OptimizeReport& nested_report, std::set<Id>& final_keep,
                              std::map<Id, std::array<double, 3>>& final_sliding);
  bool CertifyCoupledInterface(const Patch& original, const Patch& candidate, std::set<Face>& old_physical,
                               std::map<Face, Face>& skin, std::vector<Face>& prescribed, std::string& reason);
  bool PublishCoupledBoundary(Owned& pending, Id node, Id cell, Patch& candidate,
                              std::unique_ptr<PlanarReference>& final_context, const OptimizeReport& nested_report,
                              std::set<Id>& final_keep, std::map<Id, std::array<double, 3>>& final_sliding);

  using PoorCells = std::vector<std::pair<long double, Id>>;
  void QualityCensus(size_t& bad, double& minimum);
  void RunSweeps();
  void RunSweepStage(unsigned stage, const std::array<bool, 3>& enabled, size_t& accepted);
  void ImproveSmoothing(const PoorCells& bad, size_t& accepted);
  void ImproveTopology(const PoorCells& bad, bool enrichment, size_t& accepted);
  void ImproveLongEdges(unsigned stage, size_t& accepted);
};
// Scores the current ring under its frozen diagonal constraints and tie order.
template <class Triangle, class RingEdge>
void ScoreRingTriangulation(size_t n, bool debt, const std::set<std::pair<size_t, size_t>>& forbidden,
                            Triangle& triangle, RingEdge& ringEdge, std::vector<std::vector<long double>>& best,
                            std::vector<std::vector<size_t>>& arg) {
  for (size_t i = 0; i + 1 < n; ++i) best[i][i + 1] = debt ? 0 : INFINITY;
  for (size_t len = 2; len < n; ++len)
    for (size_t i = 0; i + len < n; ++i) {
      const size_t j = i + len;
      for (size_t k = i + 1; k < j; ++k) {
        if ((!ringEdge(i, k) && forbidden.count({i, k})) || (!ringEdge(k, j) && forbidden.count({k, j}))) continue;
        const auto v =
            debt ? best[i][k] + best[k][j] + triangle(i, k, j) : std::min({best[i][k], best[k][j], triangle(i, k, j)});
        if (v > best[i][j]) {
          best[i][j] = v;
          arg[i][j] = k;
        }
      }
    }
}
// Reads the chosen cells in the original depth-first order.
void ReadRingTriangulation(const std::vector<std::vector<size_t>>& arg, std::vector<std::array<size_t, 3>>& used,
                           std::vector<std::pair<size_t, size_t>>& stack) {
  while (!stack.empty()) {
    const auto e = stack.back();
    stack.pop_back();
    if (e.second <= e.first + 1) continue;
    const size_t k = arg[e.first][e.second];
    used.push_back({e.first, k, e.second});
    stack.push_back({e.first, k});
    stack.push_back({k, e.second});
  }
}
// O1: interior edge with a closed shell of n cells (3 <= n <= maximum_shell) replaced by the 2(n-2) cells of the best
// triangulation of its ring polygon (additive debt or legacy max-min dynamic programming; ring order with
// (a,b,r_i,r_i+1) positive, so (a,r_i,r_k,r_j) and (b,r_i,r_j,r_k) for i < k < j are the positive orientations).
// lengthDriven (O1L): the edge is longer than 1.8; accepted when no new edge exceeds 1.8 and the replaced cells'
// minimum score does not fall below min(old minimum, 1) (the long edge leaves the mesh without creating a final-gate
// failure).
bool MeshOptimizer::EdgeRemoval(Id a, Id b, bool lengthDriven) {
  if (Limited()) return false;
  const auto sa = star.find(a), sb = star.find(b);
  if (sa == star.end() || sb == star.end()) return false;
  const auto shell = Common(sa->second, sb->second);
  const size_t n = shell.size();
  if (n < 3 || n > options.maximum_shell) return false;
  for (Id id : shell)
    if (Frozen(id)) {
      ++report.rejected_frozen;
      return false;
    }
  std::map<Id, std::vector<Id>> ring;
  for (Id id : shell) {
    std::vector<Id> o;
    for (const auto& v : mesh.at(id).cell.v)
      if (v.id != a && v.id != b) o.push_back(v.id);
    if (o.size() != 2) return false;
    ring[o[0]].push_back(o[1]);
    ring[o[1]].push_back(o[0]);
  }
  if (ring.size() != n) return false;
  for (const auto& r : ring)
    if (r.second.size() != 2) return false;  // open (boundary) shell
  ++report.trials;
  const Point pa = points.at(a), pb = points.at(b);
  std::vector<Id> order{ring.begin()->first};
  {
    const Id r0 = order[0];
    Id next = UINT64_MAX;
    for (Id y : ring.at(r0))
      if (Orientation(pa, pb, points.at(r0), points.at(y)) > 0) next = y;
    if (next == UINT64_MAX) return false;
    Id prev = r0;
    Id cur = next;
    while (cur != r0) {
      if (order.size() > n) return false;
      order.push_back(cur);
      const auto& nb = ring.at(cur);
      const Id nx = nb[0] == prev ? nb[1] : nb[0];
      prev = cur;
      cur = nx;
    }
  }
  if (order.size() != n) return false;
  for (size_t i = 0; i < n; ++i)
    if (!(Orientation(pa, pb, points.at(order[i]), points.at(order[(i + 1) % n])) > 0)) return false;
  return RemoveInteriorRing(shell, order, {a, pa}, {b, pb}, lengthDriven);
}
// Attempts the closed-ring replacements with the original length and fixed-interface rules.
bool MeshOptimizer::RemoveInteriorRing(const std::vector<Id>& shell, const std::vector<Id>& order, Node A, Node B,
                                       bool lengthDriven) {
  const size_t n = shell.size();
  const Point pa = A.p, pb = B.p;
  const bool debt = DebtMode(shell);
  const auto oldMin = OldMerit(shell, debt);
  long double limit = MaximumMetricEdge;
  if (!lengthDriven) {
    try {
      const auto e = Estimate(pa, pb);
      limit = std::max<long double>(limit, e > EstimatePass ? Exact(pa, pb) : e);
    } catch (const std::exception&) {
    }
  }
  std::vector<Node> r(n);
  for (size_t i = 0; i < n; ++i) r[i] = {order[i], points.at(order[i])};
  std::map<std::array<size_t, 3>, long double> tri;
  auto triangle = [&](size_t i, size_t k, size_t j) {
    const std::array<size_t, 3> key{i, k, j};
    const auto f = tri.find(key);
    if (f != tri.end()) return f->second;
    const auto s = FreshMerit({Cell{0, {A, r[i], r[k], r[j]}}, Cell{0, {B, r[i], r[j], r[k]}}}, debt);
    tri.emplace(key, s);
    return s;
  };
  std::set<std::pair<size_t, size_t>> forbidden;
  auto ringEdge = [&](size_t i, size_t j) { return j == i + 1 || (i == 0 && j == n - 1); };
  for (unsigned attempt = 0; attempt < 4; ++attempt) {
    std::vector<std::vector<long double>> best(n, std::vector<long double>(n, -INFINITY));
    std::vector<std::vector<size_t>> arg(n, std::vector<size_t>(n, SIZE_MAX));
    ScoreRingTriangulation(n, debt, forbidden, triangle, ringEdge, best, arg);
    if (!(Progress(best[0][n - 1], oldMin, debt, lengthDriven))) {
      if (best[0][n - 1] == -INFINITY)
        ++report.rejected_validity;
      else
        ++report.rejected_quality;
      return false;
    }
    std::vector<std::array<size_t, 3>> used;
    std::vector<std::pair<size_t, size_t>> stack{{0, n - 1}};
    ReadRingTriangulation(arg, used, stack);
    bool ok = true;
    for (const auto& t : used)
      for (const auto& d : {std::make_pair(t[0], t[1]), std::make_pair(t[1], t[2]), std::make_pair(t[0], t[2])}) {
        if (ringEdge(d.first, d.second) || forbidden.count(d)) continue;
        if (!Certified(r[d.first].p, r[d.second].p, limit)) {
          forbidden.insert(d);
          ok = false;
          break;
        }
      }
    if (!ok) {
      ++report.rejected_edge;
      if (Limited()) return false;
      continue;
    }
    std::vector<Cell> fresh;
    for (const auto& t : used) {
      fresh.push_back(Cell{0, {A, r[t[0]], r[t[1]], r[t[2]]}});
      fresh.push_back(Cell{0, {B, r[t[0]], r[t[2]], r[t[1]]}});
    }
    if (!Progress(FreshMerit(fresh, debt), oldMin, debt, lengthDriven)) {
      ++report.rejected_quality;
      return false;
    }
    if (!Interface(shell, fresh)) return false;
    Commit(shell, fresh);
    ++(lengthDriven ? report.length_removals : report.edge_removals);
    return true;
  }
  return false;
}
// O2: interior face (x,y,z) of two unfrozen cells with apices d, e (not yet connected) -> three cells around (d,e).
bool MeshOptimizer::FaceSwap(const Face& f) {
  if (Limited()) return false;
  const auto both = Common(Common(star.at(f[0]), star.at(f[1])), star.at(f[2]));
  if (both.size() != 2) return false;
  for (Id id : both)
    if (Frozen(id)) {
      ++report.rejected_frozen;
      return false;
    }
  Id d = UINT64_MAX, e = UINT64_MAX;
  for (const auto& v : mesh.at(both[0]).cell.v)
    if (v.id != f[0] && v.id != f[1] && v.id != f[2]) d = v.id;
  for (const auto& v : mesh.at(both[1]).cell.v)
    if (v.id != f[0] && v.id != f[1] && v.id != f[2]) e = v.id;
  if (d == UINT64_MAX || e == UINT64_MAX || d == e || Adjacent(d, e)) return false;
  ++report.trials;
  const Node D{d, points.at(d)}, E{e, points.at(e)};
  std::array<Node, 3> r{Node{f[0], points.at(f[0])}, Node{f[1], points.at(f[1])}, Node{f[2], points.at(f[2])}};
  if (!(Orientation(D.p, E.p, r[0].p, r[1].p) > 0)) std::swap(r[1], r[2]);
  std::vector<Cell> fresh{Cell{0, {D, E, r[0], r[1]}}, Cell{0, {D, E, r[1], r[2]}}, Cell{0, {D, E, r[2], r[0]}}};
  const bool debt = DebtMode(both);
  const auto oldMin = OldMerit(both, debt), newMin = FreshMerit(fresh, debt);
  if (newMin == -INFINITY) {
    ++report.rejected_validity;
    return false;
  }
  if (!(Progress(newMin, oldMin, debt))) {
    ++report.rejected_quality;
    return false;
  }
  if (!Certified(D.p, E.p, MaximumMetricEdge)) {
    ++report.rejected_edge;
    return false;
  }
  if (!Interface(both, fresh)) return false;
  Commit(both, fresh);
  ++report.face_swaps;
  return true;
}
// O3: interior vertex p contracted to its neighbour q (cells of the edge disappear, the rest take q).
bool MeshOptimizer::Collapse(Id p, Id q) {
  if (Limited() || keep.count(p) || !Movable(p) || !Adjacent(p, q)) return false;
  const auto old = star.at(p);
  ++report.trials;
  const Node Q{q, points.at(q)};
  std::vector<Cell> fresh;
  std::set<Id> link;
  for (Id id : old) {
    const auto& c = mesh.at(id).cell;
    for (const auto& v : c.v)
      if (v.id != p) link.insert(v.id);
    if (CellHas(c, q)) continue;
    Cell x = c;
    for (auto& v : x.v)
      if (v.id == p) v = Q;
    fresh.push_back(x);
  }
  if (fresh.empty()) return false;
  const bool debt = DebtMode(old);
  const auto oldMin = OldMerit(old, debt), newMin = FreshMerit(fresh, debt);
  if (newMin == -INFINITY) {
    ++report.rejected_validity;
    return false;
  }
  if (!(Progress(newMin, oldMin, debt))) {
    ++report.rejected_quality;
    return false;
  }
  for (Id x : link) {
    if (x == q || Adjacent(q, x)) continue;
    if (!Certified(Q.p, points.at(x), MaximumMetricEdge)) {
      ++report.rejected_edge;
      return false;
    }
  }
  if (!Interface(old, fresh)) return false;
  Commit(old, fresh, p);
  ++report.collapses;
  return true;
}
// Physical stars move through their forward geometry bindings; artificial interfaces remain fixed.
// Measures the same cached shape debt for original and accepted stars.
long double MeshOptimizer::ShapeDebt(const std::vector<Cell>& cells) {
  long double sum = 0;
  for (const auto& c : cells) {
    const auto q = Quality(c);
    if (!q.Valid()) return std::numeric_limits<long double>::infinity();
    sum += NormalizedShapeHingeDebt(q.q, q.J);
  }
  return sum;
}
bool MeshOptimizer::SmoothBoundary(Id p) {
  SU2NativeD9::Star d9_star;
  D9QueryScope d9_query(SU2NativeD9::Site::SmoothBoundary, options.geometry);
  ++report.boundary_stars_searched;
  const auto* state = options.geometry ? options.geometry->Parameters() : nullptr;
  const auto* query_geometry = state ? &state->Geometry() : nullptr;
  const auto query_before = query_geometry ? query_geometry->GeometryStatsSnapshot()
                                           : SU2NativeGeometry::MappedFacetReference::GeometryStats{};
  struct MeasureScope {
    OptimizeReport& report;
    const SU2NativeGeometry::MappedFacetReference* geometry;
    SU2NativeGeometry::MappedFacetReference::GeometryStats before;
    Clock::time_point start = Clock::now();
    ~MeasureScope() {
      report.boundary_seconds += std::chrono::duration<double>(Clock::now() - start).count();
      if (geometry) {
        const auto after = geometry->GeometryStatsSnapshot();
        report.boundary_upper_queries += after.upper_calls - before.upper_calls;
      }
    }
  } measure{report, query_geometry, query_before};
  const auto validity_start = report.boundary_rejected_validity, shape_start = report.boundary_rejected_shape;
  const auto edge_start = report.boundary_rejected_edge, geometry_start = report.boundary_rejected_geometry;
  if (Limited() || !options.geometry || options.frozen_vertices.count(p)) return false;

  std::vector<Id> old;
  Patch patch;
  std::set<Face> old_physical;
  if (!CollectBoundarySmoothPatch(p, old, patch, old_physical)) return false;
  const auto& geometry = moved_geometry ? *moved_geometry : *options.geometry;
  try {
    SU2NativeD9::Measure(SU2NativeD9::Work::Export, [&] { geometry.ExportParameters(patch); });
  } catch (const std::exception&) {
    return false;
  }
  const bool debt = DebtMode(old, &patch);
  const auto before = OldMerit(old, debt);
  const auto origin = points.at(p);
  long double old_shape_debt;
  if (debt)
    old_shape_debt = -before;
  else {
    SU2NativeTiming3D::Scope old_cavity_scope(SU2NativeTiming3D::Operation::OldCavityMeasurement);
    old_shape_debt = ShapeDebt(patch.cells);
  }
  long double geometric_before = 0;
  bool geometry_measured = false;

  std::vector<Point> targets;
  std::set<Id> neighbors;
  if (!BoundaryTargets(p, patch, geometry, origin, targets, neighbors)) return false;
  BoundarySmoothSearch search{p, patch, geometry, debt, before, geometric_before, geometry_measured};
  return SearchBoundarySmooth(search, old, targets, old_shape_debt,
                              {validity_start, shape_start, edge_start, geometry_start});
}
// Collects a complete movable physical star while holding the artificial interface fixed.
bool MeshOptimizer::CollectBoundarySmoothPatch(Id p, std::vector<Id>& old, Patch& patch, std::set<Face>& old_physical) {
  const auto found = star.find(p);
  if (found == star.end() || found->second.empty() || found->second.size() > MaximumCavityCells) return false;
  old = found->second;
  for (Id id : old) {
    if (options.frozen_cells.count(id)) return false;
    const auto& row = mesh.at(id);
    patch.cells.push_back(row.cell);
    const auto faces = OppositeFaces(row.cell);
    for (unsigned k = 0; k < 4; ++k)
      if (row.physical[k].present) patch.surface.push_back({faces[k], row.physical[k].facet});
  }
  if (patch.surface.empty()) return false;
  for (const auto& f : patch.surface) old_physical.insert(Key(f.v));
  for (auto f : Incidence(patch.cells).Boundary())
    if (std::find(f.begin(), f.end(), p) != f.end() && !old_physical.count(Key(f))) return false;
  return true;
}
// Enumerates forward binding targets and keeps the existing fallback search.
bool MeshOptimizer::BoundaryTargets(Id p, const Patch& patch, const PlanarReference& geometry, Point origin,
                                    std::vector<Point>& targets, std::set<Id>& neighbors) {
  for (const auto& c : patch.cells)
    for (auto n : c.v)
      if (n.id != p) neighbors.insert(n.id);
  if (geometry.Mapped()) {
    const auto binding = patch.parameters.at(p).binding;
    const auto& authority = geometry.Parameters()->Geometry();
    const auto* mobility = geometry.Mobility();
    if (mobility && authority.MobilityDimension(binding, *mobility) == 0) return false;
    if (!mobility && binding.dimension == 0 && authority.PhysicalSupport(binding)) return false;
    if (!mobility && binding.dimension == 1 &&
        (!authority.ReferenceComponents() || authority.PhysicalSupport(binding))) {
      Edge support{};
      bool located = false;
      authority.ForFacets(binding, [&](Id facet) {
        if (located) return;
        const auto& f = geometry.Get(facet);
        for (unsigned k = 0; k < 3; ++k) {
          Edge e{f.v[k].id, f.v[(k + 1) % 3].id};
          std::sort(e.begin(), e.end());
          if (authority.EdgePoint(e, .5).feature == binding.feature) {
            support = e;
            located = true;
            break;
          }
        }
      });
      if (!located) return false;
      Point a{}, b{};
      authority.ForFacets(binding, [&](Id facet) {
        for (auto n : geometry.Get(facet).v) {
          if (n.id == support[0]) a = n.p;
          if (n.id == support[1]) b = n.p;
        }
      });
      // Search the forward parameter, not a chord-plane derivative of the already curved point.
      for (unsigned level = 1; level <= 4; ++level) {
        const unsigned denominator = 1u << level;
        for (unsigned k = 1; k < denominator; k += 2) {
          const long double t = static_cast<long double>(k) / denominator;
          targets.push_back({double(a.x + t * (b.x - (long double)a.x)), double(a.y + t * (b.y - (long double)a.y)),
                             double(a.z + t * (b.z - (long double)a.z))});
        }
      }
    } else if (!mobility && binding.dimension == 2 && !authority.ReferenceComponents()) {
      const auto& f = geometry.Get(binding.feature);
      for (double h : {.25, .125, .0625, .03125})
        for (const auto d :
             std::array<std::array<int, 2>, 6>{{{{1, 0}}, {{-1, 0}}, {{0, 1}}, {{0, -1}}, {{1, -1}}, {{-1, 1}}}}) {
          const long double u = binding.u + h * d[0], v = binding.v + h * d[1];
          if (u <= 0 || v <= 0 || u + v >= 1) continue;
          const auto a = f.v[0].p, b = f.v[1].p, c = f.v[2].p;
          targets.push_back({double(a.x + u * (b.x - (long double)a.x) + v * (c.x - (long double)a.x)),
                             double(a.y + u * (b.y - (long double)a.y) + v * (c.y - (long double)a.y)),
                             double(a.z + u * (b.z - (long double)a.z) + v * (c.z - (long double)a.z))});
        }
    }
  }
  return !targets.empty() || FallbackBoundaryTargets(origin, neighbors, targets);
}
// Adds the original metric mean and axis proposals when no forward target exists.
bool MeshOptimizer::FallbackBoundaryTargets(Point origin, const std::set<Id>& neighbors, std::vector<Point>& targets) {
  try {
    std::vector<std::pair<Point, Tensor>> samples;
    double radius = INFINITY;
    for (Id n : neighbors) {
      samples.emplace_back(points.at(n), At(points.at(n)));
      radius = std::min(radius, double(Norm3(origin, points.at(n))));
    }
    targets.push_back(MetricMeanPoint(origin, samples));
    for (double step : {.25, .125, .0625})
      for (unsigned axis = 0; axis < 3; ++axis)
        for (double sign : {-1., 1.}) {
          Point q = origin;
          if (axis == 0)
            q.x += sign * step * radius;
          else if (axis == 1)
            q.y += sign * step * radius;
          else
            q.z += sign * step * radius;
          targets.push_back(q);
        }
  } catch (const std::exception&) {
    return false;
  }
  return true;
}
// Charges exactly the retained overlay, atlas, context and memo storage.
bool MeshOptimizer::BoundaryMoveResources::Charge(const PlanarReference* candidate_context) {
  if (!atlas_budget) return false;
  size_t bytes = caller_restore.original;
  // One new binding and coordinate-map row can be allocated before the next refresh.
  if (projected_context) bytes = transfer_memory::Add(bytes, projected_context->ParameterOverlayBytes(), 512);
  if (prepared_atlas_ok) bytes = transfer_memory::Add(bytes, prepared_atlas.ResidentBytes());
  bytes = transfer_memory::Add(bytes, guidance_memo.ResidentBytes());
  if (prepared_move) bytes = transfer_memory::Add(bytes, prepared_move->ResidentBytes());
  if (candidate_context) bytes = transfer_memory::Add(bytes, candidate_context->ParameterOverlayBytes());
  atlas_budget->caller_bytes = bytes;
  atlas_budget->peak_bytes = std::max(atlas_budget->peak_bytes, bytes);
  return bytes <= atlas_budget->maximum_bytes;
}
// Prepares and charges the projected context and curved atlas when requested.
bool MeshOptimizer::PrepareBoundaryGeometry(Id p, const Patch& patch, const PlanarReference& geometry,
                                            BoundaryMoveResources& resources) {
  auto& projected_context = resources.projected_context;
  const auto*& projected_geometry = resources.projected_geometry;
  auto* atlas_budget = resources.atlas_budget;
  auto& prepared_atlas = resources.prepared_atlas;
  bool& prepared_atlas_ok = resources.prepared_atlas_ok;
  if (geometry.Mobility()) {
    projected_context = geometry.PrivateParameters(patch);
    if (projected_context) projected_geometry = projected_context.get();
    if (!resources.Charge()) {
      ++report.boundary_rejected_geometry;
      return false;
    }
    if (!projected_geometry->EntityGeometry()) {
      std::string prepare_reason;
      if (!atlas_budget || !SU2NativeGeometry::PrepareCurvedAtlas(patch, *projected_geometry, p, prepared_atlas,
                                                                  *atlas_budget, prepare_reason)) {
        ++report.boundary_rejected_geometry;
        return false;
      }
      prepared_atlas_ok = true;
      if (!resources.Charge()) {
        ++report.boundary_rejected_geometry;
        return false;
      }
    }
  }
  return true;
}
// Tests the projection and volume score before expensive geometry reconstruction.
bool MeshOptimizer::BoundaryProjection(BoundarySmoothSearch& search, Point target_point,
                                       BoundaryMoveResources& resources, std::string& reason) {
  const auto p = search.p;
  const auto& patch = search.patch;
  const auto& geometry = search.geometry;
  const bool debt = search.debt;
  const auto best_score = search.best_score;
  const auto* projected_geometry = resources.projected_geometry;
  auto& prepared_atlas = resources.prepared_atlas;
  const bool& prepared_atlas_ok = resources.prepared_atlas_ok;
  if (geometry.Mobility()) {
    // Projection and volume quality precede expensive surface reconstruction; height can only lower this score.
    long double upper = debt ? 0 : INFINITY;
    {
      auto* budget = projected_geometry->AtlasReservation();
      BoundaryPoint projected;
      {
        SU2NativeD9::WorkScope d9_projection(SU2NativeD9::Work::Projection);
        if (projected_geometry->EntityGeometry()) {
          try {
            std::set<Id> incident;
            for (const auto& face : patch.surface)
              if (std::find(face.v.begin(), face.v.end(), p) != face.v.end()) incident.insert(face.facet);
            projected.node = projected_geometry->BindMovedPoint(p, {next_node, target_point}, incident);
          } catch (const std::exception&) {
            ++report.rejected_interface;
            ++report.boundary_rejected_geometry;
            return false;
          }
          if (!resources.Charge()) {
            ++report.boundary_rejected_geometry;
            return false;
          }
        } else if (!budget || !prepared_atlas_ok ||
                   !SU2NativeGeometry::ProposeAtlasBinding(prepared_atlas, p, {next_node, target_point}, projected,
                                                           *budget, reason)) {
          ++report.rejected_interface;
          ++report.boundary_rejected_geometry;
          return false;
        }
      }
      for (auto cell : patch.cells) {
        for (auto& node : cell.v)
          if (node.id == p) node = projected.node;
        const auto measured = Quality(cell, false);
        if (!measured.Valid()) {
          upper = -INFINITY;
          break;
        }
        upper = debt ? upper - NormalizedShapeHingeDebt(measured.q, measured.J)
                     : std::min(upper, static_cast<long double>(measured.Score()));
      }
    }
    if (!std::isfinite(upper)) {
      ++report.rejected_validity;
      ++report.boundary_rejected_validity;
      return false;
    }
    if (!(Progress(upper, best_score, debt))) {
      ++report.rejected_quality;
      ++report.boundary_rejected_shape;
      return false;
    }
  }
  return true;
}
// Prepares the live move once and evaluates the original raw proposal.
MeshOptimizer::BoundaryDecision MeshOptimizer::BuildBoundaryMove(BoundarySmoothSearch& search, Point target_point,
                                                                 BoundaryMoveResources& resources, Patch& candidate,
                                                                 std::string& reason) {
  const auto p = search.p;
  const auto& patch = search.patch;
  const auto& geometry = search.geometry;
  auto& prepared_move = resources.prepared_move;
  bool& preparation_attempted = resources.preparation_attempted;
  auto* atlas_budget = resources.atlas_budget;
  // Use the raw proposal again: projecting the realized surface point need not be idempotent.
  if (!preparation_attempted) {
    preparation_attempted = true;
    try {
      prepared_move = PrepareVertexMove(patch, geometry);
    } catch (const std::exception&) {
      ++report.rejected_interface;
      ++report.boundary_rejected_geometry;
      return BoundaryDecision::Next;
    }
    if (prepared_move) ++report.boundary_move_contexts;
    if (atlas_budget && !resources.Charge()) {
      ++report.boundary_rejected_geometry;
      return BoundaryDecision::Stop;
    }
  }
  if (prepared_move) ++report.boundary_prepared_trials;
  if (!SU2NativeD9::Measure(SU2NativeD9::Work::Move, [&] {
        return prepared_move ? MoveVertex(*prepared_move, p, {next_node, target_point}, next_cell, candidate, reason)
                             : MoveVertex(patch, p, {next_node, target_point}, next_cell, geometry, candidate, reason);
      })) {
    ++report.rejected_interface;
    ++report.boundary_rejected_geometry;
    return BoundaryDecision::Next;
  }
  return BoundaryDecision::Admit;
}
// Measures old guidance lazily and rejects the candidate if its geometry regresses.
MeshOptimizer::BoundaryDecision MeshOptimizer::BoundaryGuidance(BoundarySmoothSearch& search, const Patch& candidate,
                                                                const PlanarReference& candidate_geometry,
                                                                const PlanarReference* candidate_context,
                                                                BoundaryMoveResources& resources,
                                                                SU2NativeD9::SmoothChain& d9_chain) {
  const auto p = search.p;
  const auto& patch = search.patch;
  const auto& geometry = search.geometry;
  auto& geometry_measured = search.geometry_measured;
  auto& geometric_before = search.geometric_before;
  auto& guidance_memo = resources.guidance_memo;
  auto& guidance_memo_ready = resources.guidance_memo_ready;
  if (!geometry_measured) {
    if (std::any_of(patch.surface.begin(), patch.surface.end(),
                    [&](const SurfaceFace& f) { return std::find(f.v.begin(), f.v.end(), p) == f.v.end(); }))
      guidance_memo_ready = guidance_memo.Prepare(patch, geometry, options.surface_tolerance);
    if (resources.atlas_budget && !resources.Charge(candidate_context)) {
      ++report.boundary_rejected_geometry;
      return BoundaryDecision::Fail;
    }
    ++report.boundary_guidance_calls;
    const auto guidance = SU2NativeD9::Measure(SU2NativeD9::Work::Guidance, [&] {
      return GuidanceOf(SU2NativeD9::Part::SmoothOld, patch, geometry, options.surface_tolerance, INFINITY,
                        guidance_memo_ready ? &guidance_memo : nullptr);
    });
    report.boundary_guidance_faces += guidance.evaluated_faces;
    report.boundary_guidance_reused_faces += guidance.reused_faces;
    geometric_before = guidance.excess;
    geometry_measured = true;
  }
  ++report.boundary_guidance_calls;
  if (SU2NativeD9::ObservingUnion()) ++d9_chain.checked;
  const auto guidance = SU2NativeD9::Measure(SU2NativeD9::Work::Guidance, [&] {
    return GuidanceOf(SU2NativeD9::Part::SmoothCandidate, candidate, candidate_geometry, options.surface_tolerance,
                      geometric_before, guidance_memo_ready ? &guidance_memo : nullptr);
  });
  report.boundary_guidance_faces += guidance.evaluated_faces;
  report.boundary_guidance_reused_faces += guidance.reused_faces;
  report.boundary_guidance_early_rejections += !guidance.complete;
  const auto after = guidance.excess;
  if (!guidance.complete || !std::isfinite(after) || after > geometric_before) {
    ++report.rejected_interface;
    ++report.boundary_rejected_geometry;
    return BoundaryDecision::Next;
  }
  return BoundaryDecision::Admit;
}
// Carries one candidate through exactly the original ordered gates and counter scope.
MeshOptimizer::BoundaryDecision MeshOptimizer::TryBoundaryMove(BoundarySmoothSearch& search, Point target_point,
                                                               BoundaryMoveResources& resources, Patch& best,
                                                               SU2NativeD9::SmoothChain& d9_chain) {
  const auto& geometry = search.geometry;
  const bool debt = search.debt;
  auto& best_score = search.best_score;
  auto* atlas_budget = resources.atlas_budget;
  ++report.trials;
  ++report.boundary_candidates;
  SU2NativeD9::Candidate d9_candidate;
  Patch candidate;
  std::string reason;
  if (atlas_budget && !resources.Charge()) {
    ++report.boundary_rejected_geometry;
    return BoundaryDecision::Stop;
  }

  if (!BoundaryProjection(search, target_point, resources, reason)) return BoundaryDecision::Next;
  if (const auto decision = BuildBoundaryMove(search, target_point, resources, candidate, reason);
      decision != BoundaryDecision::Admit)
    return decision;
  const auto score = FreshMerit(candidate.cells, debt, false, &candidate);
  if (!std::isfinite(score)) {
    ++report.rejected_validity;
    ++report.boundary_rejected_validity;
    return BoundaryDecision::Next;
  }
  if (!(Progress(score, best_score, debt))) {
    ++report.rejected_quality;
    ++report.boundary_rejected_shape;
    return BoundaryDecision::Next;
  }
  auto candidate_parameters = geometry.PrivateParameters(candidate);
  const auto& candidate_geometry = candidate_parameters ? *candidate_parameters : geometry;
  if (atlas_budget && !resources.Charge(candidate_parameters.get())) {
    ++report.boundary_rejected_geometry;
    return BoundaryDecision::Next;
  }
  FirstHeightReport height;
  if (GetShapeAdmission() == ShapeAdmission::Absolute &&
      !CheckFirstHeight(candidate, options.wall_heights, candidate_geometry, height, reason)) {
    ++report.rejected_frozen;
    ++report.boundary_rejected_geometry;
    return BoundaryDecision::Next;
  }
  bool sized = true;
  for (const auto& c : candidate.cells)
    for (unsigned i = 0; i < 4; ++i)
      for (unsigned j = i + 1; j < 4; ++j)
        if ((c.v[i].id == next_node || c.v[j].id == next_node) && !Certified(c.v[i].p, c.v[j].p, MaximumMetricEdge))
          sized = false;
  if (!sized) {
    ++report.rejected_edge;
    ++report.boundary_rejected_edge;
    return BoundaryDecision::Next;
  }

  if (const auto decision =
          BoundaryGuidance(search, candidate, candidate_geometry, candidate_parameters.get(), resources, d9_chain);
      decision != BoundaryDecision::Admit)
    return decision;
  best_score = score;
  best = std::move(candidate);
  if (SU2NativeD9::ObservingUnion()) ++d9_chain.passed;
  if (debt ? best_score >= 0 : best_score >= 1) return BoundaryDecision::Stop;
  return BoundaryDecision::Next;
}
// Keeps the prepared resources and candidate-chain scope through the winning publication.
bool MeshOptimizer::SearchBoundarySmooth(BoundarySmoothSearch& search, const std::vector<Id>& old,
                                         const std::vector<Point>& targets, long double old_shape_debt,
                                         const SmoothRejections& baseline) {
  const auto p = search.p;
  const auto& geometry = search.geometry;
  const auto before = search.best_score;
  BoundaryMoveResources resources(geometry);
  if (!PrepareBoundaryGeometry(p, search.patch, geometry, resources)) return false;
  Patch best;
  SU2NativeD9::SmoothChain d9_chain;
  for (Point target_point : targets) {
    if (Limited()) break;
    const auto decision = TryBoundaryMove(search, target_point, resources, best, d9_chain);
    if (decision == BoundaryDecision::Stop) break;
    if (decision == BoundaryDecision::Fail) return false;
  }
  if (best.cells.empty()) {
    auto* reservation = geometry.AtlasReservation();
    if (!Limited() && std::isfinite(before) && report.boundary_candidates &&
        report.boundary_rejected_shape > baseline.shape && report.boundary_rejected_validity == baseline.validity &&
        report.boundary_rejected_edge == baseline.edge && report.boundary_rejected_geometry == baseline.geometry &&
        (!reservation ||
         (!reservation->work_exhausted && !reservation->time_exhausted && reservation->geometry_failure.empty())))
      RememberRejectedSmooth(p);
    return false;
  }

  resources.prepared_atlas = {};
  resources.projected_context.reset();
  resources.prepared_move.reset();
  resources.guidance_memo = GeometryGuidanceMemo{};
  if (resources.atlas_budget) resources.atlas_budget->caller_bytes = resources.caller_restore.original;
  return CommitBoundarySmooth(search, old, best, old_shape_debt, d9_chain);
}
// Publishes the certified move with the original binding and relabel order.
bool MeshOptimizer::CommitBoundarySmooth(BoundarySmoothSearch& search, const std::vector<Id>& old, const Patch& best,
                                         long double old_shape_debt, SU2NativeD9::SmoothChain& d9_chain) {
  const auto p = search.p;
  const auto& geometry = search.geometry;
  const auto best_score = search.best_score;
  const auto debt = search.debt;
  const auto debt_reduction = old_shape_debt - (debt ? -best_score : ShapeDebt(best.cells));
  if (std::isfinite(old_shape_debt) && std::isfinite(debt_reduction)) report.boundary_debt_reduction += debt_reduction;
  ++report.boundary_moves;
  if (SU2NativeD9::ObservingUnion()) d9_chain.moved = true;
  std::map<Face, FacetBinding> added;
  for (const auto& f : best.surface) added.emplace(Key(f.v), FacetBinding{true, f.facet});
  const Id moved = next_node++;
  Commit(old, best.cells, p, added);
  report.parameters.erase(p);
  for (const auto& row : best.parameters) report.parameters.insert_or_assign(row.first, row.second);
  for (const auto& row : best.coverage.faces) report.coverage.faces.insert_or_assign(row.first, row.second);
  for (const auto& row : best.coverage.traces) report.coverage.traces.insert_or_assign(row.first, row.second);
  for (auto it = report.coverage.faces.begin(); it != report.coverage.faces.end();)
    if (!physical.count(Key(it->first)))
      it = report.coverage.faces.erase(it);
    else
      ++it;
  for (auto it = report.coverage.traces.begin(); it != report.coverage.traces.end();)
    if (!Adjacent(it->first[0], it->first[1]))
      it = report.coverage.traces.erase(it);
    else
      ++it;
  Patch overlay;
  overlay.parameters = report.parameters;
  overlay.coverage = report.coverage;
  overlay.surface = best.surface;
  auto parameters = geometry.PrivateParameters(overlay);
  if (parameters) moved_geometry = std::move(parameters);
  fixed.erase(p);
  fixed.insert(moved);
  if (keep.erase(p)) keep.insert(moved);
  Id first = p;
  for (const auto& row : report.relabel)
    if (row.second == p) {
      first = row.first;
      break;
    }
  report.relabel[first] = moved;
  ++report.moves;
  return true;
}
// Dispatches a movable star and keeps the original interior measurement scope.
bool MeshOptimizer::Smooth(Id p) {
  if (fixed.count(p) && !sliding.count(p) && options.geometry) {
    if (Limited()) return false;
    if (rejected_smoothing.count(p)) {
      ++report.trials;
      ++report.boundary_stars_reused;
      return false;
    }
    return SmoothBoundary(p);
  }
  if (Limited() || !Movable(p)) return false;
  if (rejected_smoothing.count(p)) {
    ++report.trials;
    ++report.interior_stars_reused;
    return false;
  }
  ++report.interior_stars_searched;
  const auto* state = options.geometry ? options.geometry->Parameters() : nullptr;
  const auto* query_geometry = state ? &state->Geometry() : nullptr;
  const auto query_before = query_geometry ? query_geometry->GeometryStatsSnapshot()
                                           : SU2NativeGeometry::MappedFacetReference::GeometryStats{};
  struct MeasureScope {
    OptimizeReport& report;
    const SU2NativeGeometry::MappedFacetReference* geometry;
    SU2NativeGeometry::MappedFacetReference::GeometryStats before;
    Clock::time_point start = Clock::now();
    ~MeasureScope() {
      report.interior_seconds += std::chrono::duration<double>(Clock::now() - start).count();
      if (geometry) {
        const auto after = geometry->GeometryStatsSnapshot();
        report.interior_upper_queries += after.upper_calls - before.upper_calls;
      }
    }
  } measure{report, query_geometry, query_before};
  const auto validity_start = report.interior_rejected_validity, shape_start = report.interior_rejected_shape;
  const auto edge_start = report.interior_rejected_edge, geometry_start = report.interior_rejected_geometry;
  const auto old = star.at(p);
  const Point x0 = points.at(p);
  std::set<Id> nbr;
  for (Id id : old)
    for (const auto& v : mesh.at(id).cell.v)
      if (v.id != p) nbr.insert(v.id);
  ++report.trials;
  const bool debt = DebtMode(old);
  const auto oldMin = OldMerit(old, debt);
  std::vector<Cell> old_cells;
  for (Id id : old) old_cells.push_back(mesh.at(id).cell);
  long double old_shape_debt;
  if (debt)
    old_shape_debt = -oldMin;
  else {
    SU2NativeTiming3D::Scope old_cavity_scope(SU2NativeTiming3D::Operation::OldCavityMeasurement);
    old_shape_debt = ShapeDebt(old_cells);
  }
  long double oldMax = 0, shortest = INFINITY;
  for (Id x : nbr) {
    const Point px = points.at(x);
    try {
      oldMax = std::max(oldMax, Estimate(x0, px));
    } catch (const std::exception&) {
      oldMax = INFINITY;
    }
    const long double dx = (long double)px.x - x0.x, dy = (long double)px.y - x0.y, dz = (long double)px.z - x0.z;
    shortest = std::min(shortest, std::sqrt(dx * dx + dy * dy + dz * dz));
  }
  const long double limit = std::max<long double>(MaximumMetricEdge, oldMax);

  return SearchInteriorSmooth(p, old, nbr, x0, debt, oldMin, old_shape_debt, shortest, limit,
                              {validity_start, shape_start, edge_start, geometry_start});
}
// Adds the existing requested-height proposals before metric proposals.
void MeshOptimizer::AddHeightTargets(Id p, const std::vector<Id>& old, Point x0, std::vector<Point>& targets) {
  if (options.geometry && !options.wall_heights.empty()) {
    std::vector<Cell> cells;
    for (Id id : old) cells.push_back(mesh.at(id).cell);
    const auto patch = GeometryPatch(cells);
    for (const auto& f : patch.surface) {
      const auto h = options.wall_heights.find(options.geometry->Get(f.facet).marker);
      if (h == options.wall_heights.end() || std::find(f.v.begin(), f.v.end(), p) != f.v.end()) continue;
      bool apex = false;
      for (const auto& c : patch.cells) {
        unsigned hits = 0;
        for (auto n : c.v) hits += std::find(f.v.begin(), f.v.end(), n.id) != f.v.end();
        apex = apex || hits == 3;
      }
      if (!apex) continue;
      const auto a = points.at(f.v[0]), b = points.at(f.v[1]), c = points.at(f.v[2]);
      long double normal[3];
      const auto area = Area2(a, b, c, normal);
      if (!(area > 0)) continue;
      const long double correction = (((long double)x0.x - a.x) * normal[0] + ((long double)x0.y - a.y) * normal[1] +
                                      ((long double)x0.z - a.z) * normal[2]) /
                                         area +
                                     h->second;
      targets.push_back({double(x0.x - correction * normal[0] / area), double(x0.y - correction * normal[1] / area),
                         double(x0.z - correction * normal[2] / area)});
      targets.push_back({double(((long double)a.x + b.x + c.x) / 3 - h->second * normal[0] / area),
                         double(((long double)a.y + b.y + c.y) / 3 - h->second * normal[1] / area),
                         double(((long double)a.z + b.z + c.z) / 3 - h->second * normal[2] / area)});
    }
  }
}
// Samples the neighbor tensors for the metric mean proposal.
void MeshOptimizer::AddMetricMeanTarget(Point x0, const std::set<Id>& nbr, std::vector<Point>& targets) {
  try {
    std::vector<std::pair<Point, Tensor>> samples;
    for (Id x : nbr) samples.push_back({points.at(x), At(points.at(x))});
    targets.push_back(MetricMeanPoint(x0, samples));
  } catch (const std::exception&) {
  }
}
// Adds the damped hinge step when its original solve succeeds.
void MeshOptimizer::AddHingeTarget(Id p, const std::vector<Id>& old, Point x0, long double shortest,
                                   std::vector<Point>& targets) {
  try {  // damped hinge Gauss-Newton on the failing cells of the star (LinearizeMetricShape residuals, 1% interior aim)
    long double g[3] = {0, 0, 0}, H[3][3] = {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}};
    bool any = false;
    for (Id id : old) {
      const auto& c = mesh.at(id).cell;
      if (Score(c) >= 1.01) continue;
      unsigned slot = 0;
      while (c.v[slot].id != p) ++slot;
      const auto L = LinearizeMetricShape({c.v[0].p, c.v[1].p, c.v[2].p, c.v[3].p}, At(Centroid(c)));
      for (unsigned r = 0; r < 2; ++r)
        if (L.residual[r] > 0) {
          any = true;
          const auto& d = L.derivative[r][slot];
          for (unsigned i = 0; i < 3; ++i) {
            g[i] += L.residual[r] * d[i];
            for (unsigned j = 0; j < 3; ++j) H[i][j] += (long double)d[i] * d[j];
          }
        }
    }
    if (any) {
      const long double trace = H[0][0] + H[1][1] + H[2][2];
      if (!(trace > 0)) throw std::runtime_error("flat hinge");
      for (unsigned i = 0; i < 3; ++i) H[i][i] += 1e-6L * trace;
      long double M[3][4];
      for (unsigned i = 0; i < 3; ++i) {
        for (unsigned j = 0; j < 3; ++j) M[i][j] = H[i][j];
        M[i][3] = -g[i];
      }
      for (unsigned c = 0; c < 3; ++c) {
        unsigned r = c;
        for (unsigned t = c + 1; t < 3; ++t)
          if (std::abs(M[t][c]) > std::abs(M[r][c])) r = t;
        if (!(std::abs(M[r][c]) > 0)) throw std::runtime_error("singular hinge");
        for (unsigned q = 0; q < 4; ++q) std::swap(M[c][q], M[r][q]);
        for (unsigned t = 0; t < 3; ++t)
          if (t != c) {
            const auto f = M[t][c] / M[c][c];
            for (unsigned q = c; q < 4; ++q) M[t][q] -= f * M[c][q];
          }
      }
      long double d[3];
      for (unsigned i = 0; i < 3; ++i) d[i] = M[i][3] / M[i][i];
      const long double norm = std::sqrt(d[0] * d[0] + d[1] * d[1] + d[2] * d[2]), cap = .5L * shortest;
      if (norm > cap && norm > 0)
        for (auto& v : d) v *= cap / norm;
      if (std::isfinite(norm) && norm > 0)
        targets.push_back({double(x0.x + d[0]), double(x0.y + d[1]), double(x0.z + d[2])});
    }
  } catch (const std::exception&) {
  }
}
// Projects proposals onto the existing diagnostic plane and runs the bounded link search.
bool MeshOptimizer::PlaceInteriorTargets(Id p, const std::vector<Id>& old, Point x0, bool slides,
                                         const std::array<double, 3>& plane, bool debt, std::vector<Point>& targets) {
  // DC-13 O4m: bounded placement over the star (approximate proposal, exact merit below), from p and each proposal
  bool placed = false;
  auto inPlane = [&](Point t) -> Point {
    if (!slides) return t;
    const auto& n = plane;
    const double d = (t.x - x0.x) * n[0] + (t.y - x0.y) * n[1] + (t.z - x0.z) * n[2];
    return {t.x - d * n[0], t.y - d * n[1], t.z - d * n[2]};
  };
  for (auto& t : targets) t = inPlane(t);
  {
    std::vector<Cell> cells;
    for (Id id : old) cells.push_back(mesh.at(id).cell);
    auto faces = CavityBoundary(cells);
    faces.erase(
        std::remove_if(faces.begin(), faces.end(), [&](const Face& f) { return f[0] == p || f[1] == p || f[2] == p; }),
        faces.end());  // the link of p
    std::vector<Point> starts{x0};
    for (const auto& t : targets) starts.push_back(t);
    Placement best;
    for (const auto& st : starts) {
      const auto pl = PlaceCone(
          faces, [&](Id v) { return points.at(v); }, [&](Point q) { return At(q); }, st, nullptr,
          std::max<size_t>(16, options.placement_budget / starts.size()), debt);
      report.placement_evaluations += pl.evaluations;
      if (pl.ok && pl.score > best.score) best = pl;
    }
    if (best.ok) best.x = inPlane(best.x);
    ++report.placements;
    if (best.ok && !(best.x.x == x0.x && best.x.y == x0.y && best.x.z == x0.z)) {
      targets.insert(targets.begin(), best.x);
      placed = true;
    }
  }
  return placed;
}
// Measures each damped position in proposal order without memoizing a trial identity.
std::vector<MeshOptimizer::SmoothTrial> MeshOptimizer::CollectInteriorTrials(Id p, const std::vector<Id>& old, Point x0,
                                                                             bool debt, long double oldMin, bool placed,
                                                                             const std::vector<Point>& targets,
                                                                             Patch& constraints) {
  if (debt && options.geometry && !options.wall_heights.empty()) {
    std::vector<Cell> cells;
    for (Id id : old) cells.push_back(mesh.at(id).cell);
    constraints = GeometryPatch(cells);
  }
  std::vector<SmoothTrial> trials;
  for (size_t ti = 0; ti < targets.size(); ++ti)
    for (long double w : {1.L, .5L, .25L}) {
      const Point t = targets[ti];
      if (ti == 0 && placed && w != 1.L) continue;  // the placed point is tried as is
      Point x{double(x0.x + w * ((long double)t.x - x0.x)), double(x0.y + w * ((long double)t.y - x0.y)),
              double(x0.z + w * ((long double)t.z - x0.z))};
      if (!constraints.cells.empty()) {
        const auto& geometry = moved_geometry ? *moved_geometry : *options.geometry;
        x = ConstrainMovePoint(constraints, p, {next_node, x}, options.wall_heights, geometry).p;
      }
      if (x.x == x0.x && x.y == x0.y && x.z == x0.z) continue;
      ++report.interior_candidates;
      const Node P{next_node, x};
      std::vector<Cell> fresh;
      for (Id id : old) {
        Cell c = mesh.at(id).cell;
        for (auto& v : c.v)
          if (v.id == p) v = P;
        fresh.push_back(c);
      }
      const auto s = FreshMerit(fresh, debt, false);  // the trial identity next_node is reused: never memoized
      if (s == -INFINITY) {
        ++report.rejected_validity;
        ++report.interior_rejected_validity;
      } else if (Progress(s, oldMin, debt))
        trials.push_back({s, x});
      else {
        ++report.rejected_quality;
        ++report.interior_rejected_shape;
      }
    }
  return trials;
}
// Carries the old binding when the diagnostic slide changes the skin vertex.
void MeshOptimizer::SlidingBindings(Id p, Id replacement, const std::vector<Id>& old,
                                    std::map<Face, FacetBinding>& added) const {
  for (Id id : old) {
    const auto& row = mesh.at(id);
    const auto faces = OppositeFaces(row.cell);  // diagnostic: bindings follow the moved skin vertex
    for (unsigned k = 0; k < 4; ++k)
      if (row.physical[k].present) {
        auto f = faces[k];
        if (f[0] != p && f[1] != p && f[2] != p) continue;
        for (auto& x : f)
          if (x == p) x = replacement;
        std::sort(f.begin(), f.end());
        added.emplace(f, row.physical[k]);
      }
  }
}
// Certifies scored positions in stable order and publishes the first admissible move.
bool MeshOptimizer::CommitInteriorSmooth(Id p, const std::vector<Id>& old, const std::set<Id>& nbr, long double limit,
                                         bool slides, const std::array<double, 3>& plane, bool debt,
                                         long double old_shape_debt, std::vector<SmoothTrial>& trials) {
  std::stable_sort(trials.begin(), trials.end(),
                   [](const SmoothTrial& u, const SmoothTrial& v) { return u.score > v.score; });
  for (const auto& t : trials) {
    bool ok = true;
    for (Id x : nbr)
      if (!Certified(t.x, points.at(x), limit)) {
        ok = false;
        break;
      }
    if (!ok) {
      ++report.rejected_edge;
      ++report.interior_rejected_edge;
      continue;
    }
    const Node P{next_node, t.x};
    std::vector<Cell> fresh;
    for (Id id : old) {
      Cell c = mesh.at(id).cell;
      for (auto& v : c.v)
        if (v.id == p) v = P;
      fresh.push_back(c);
    }
    std::map<Face, FacetBinding> added;

    if (slides)
      SlidingBindings(p, P.id, old, added);
    else if (!Interface(old, fresh)) {
      ++report.interior_rejected_geometry;
      continue;
    }
    const auto debt_reduction = old_shape_debt - (debt ? -t.score : ShapeDebt(fresh));
    if (std::isfinite(old_shape_debt) && std::isfinite(debt_reduction))
      report.interior_debt_reduction += debt_reduction;
    ++report.interior_moves;
    const Id moved = next_node++;
    Commit(old, fresh, p, added);
    if (slides) {
      sliding.erase(p);
      sliding[moved] = plane;
      fixed.insert(moved);
    }
    if (keep.erase(p)) keep.insert(moved);
    Id origin = p;
    for (const auto& r : report.relabel)
      if (r.second == p) {
        origin = r.first;
        break;
      }
    report.relabel[origin] = moved;
    ++report.moves;
    return true;
  }
  return false;
}
// Keeps the original pure-shape refusal cache predicate and budget exclusions.
void MeshOptimizer::RememberInteriorFailure(Id p, long double oldMin, const SmoothRejections& baseline) {
  auto* reservation = options.geometry ? options.geometry->AtlasReservation() : nullptr;
  if (!Limited() && std::isfinite(oldMin) && report.interior_candidates &&
      report.interior_rejected_shape > baseline.shape && report.interior_rejected_validity == baseline.validity &&
      report.interior_rejected_edge == baseline.edge && report.interior_rejected_geometry == baseline.geometry &&
      (!reservation ||
       (!reservation->work_exhausted && !reservation->time_exhausted && reservation->geometry_failure.empty())))
    RememberRejectedSmooth(p);
}
// Runs target generation, scoring, and ordered interior certification under the caller's sweep.
bool MeshOptimizer::SearchInteriorSmooth(Id p, const std::vector<Id>& old, const std::set<Id>& nbr, Point x0, bool debt,
                                         long double oldMin, long double old_shape_debt, long double shortest,
                                         long double limit, const SmoothRejections& baseline) {
  std::vector<Point> targets;
  AddHeightTargets(p, old, x0, targets);
  AddMetricMeanTarget(x0, nbr, targets);
  AddHingeTarget(p, old, x0, shortest, targets);
  const auto slideIt = sliding.find(p);
  const bool slides = slideIt != sliding.end();
  const std::array<double, 3> plane = slides ? slideIt->second : std::array<double, 3>{0, 0, 0};

  const bool placed = PlaceInteriorTargets(p, old, x0, slides, plane, debt, targets);
  Patch constraints;
  auto trials = CollectInteriorTrials(p, old, x0, debt, oldMin, placed, targets, constraints);
  if (trials.empty()) {
    RememberInteriorFailure(p, oldMin, baseline);
    return false;
  }
  if (CommitInteriorSmooth(p, old, nbr, limit, slides, plane, debt, old_shape_debt, trials)) return true;
  RememberInteriorFailure(p, oldMin, baseline);
  return false;
}
// O5: boundary edge (a,b) between two boundary faces bound to the same facet (coplanar within 1e-12 relative); its open
// shell (ring path r_0..r_m from one face to the other) is replaced by the best triangulation of the polygon r_0..r_m
// coned to a and b, the boundary pair (a,b,r_0),(a,b,r_m) becoming (a,r_0,r_m),(b,r_0,r_m).
bool MeshOptimizer::BoundaryFlip(Id a, Id b) {
  if (Limited() || options.frozen_vertices.count(a) || options.frozen_vertices.count(b)) return false;
  const auto sa = star.find(a), sb = star.find(b);
  if (sa == star.end() || sb == star.end()) return false;
  const auto shell = Common(sa->second, sb->second);
  const size_t n = shell.size();
  if (n < 2 || n + 1 > options.maximum_shell) return false;
  for (Id id : shell)
    if (Frozen(id)) {
      ++report.rejected_frozen;
      return false;
    }
  std::vector<BoundaryFace> faces;
  for (const auto& f : BoundaryFacesOf(shell, UINT64_MAX)) {
    int k = 0;
    for (Id x : f.oriented) k += x == a || x == b;
    if (k == 2) faces.push_back(f);
  }
  if (faces.size() != 2 || faces[0].binding.facet != faces[1].binding.facet) return false;

  std::map<Id, std::vector<Id>> ring;
  std::vector<Id> order;
  Point pa, pb;
  if (!OrientBoundaryRing(faces, shell, ring, order, a, b, pa, pb)) return false;
  return FlipBoundaryRing(shell, faces, order, {a, pa}, {b, pb});
}
// Builds and orients the open boundary ring before its original planarity test.
bool MeshOptimizer::OrientBoundaryRing(const std::vector<BoundaryFace>& faces, const std::vector<Id>& shell,
                                       std::map<Id, std::vector<Id>>& ring, std::vector<Id>& order, Id& a, Id& b,
                                       Point& pa, Point& pb) {
  const auto n = shell.size();
  for (Id id : shell) {
    std::vector<Id> o;
    for (const auto& v : mesh.at(id).cell.v)
      if (v.id != a && v.id != b) o.push_back(v.id);
    if (o.size() != 2) return false;
    ring[o[0]].push_back(o[1]);
    ring[o[1]].push_back(o[0]);
  }
  Id r0 = UINT64_MAX, rm = UINT64_MAX;
  for (Id x : faces[0].oriented)
    if (x != a && x != b) r0 = x;
  for (Id x : faces[1].oriented)
    if (x != a && x != b) rm = x;
  if (r0 == UINT64_MAX || rm == UINT64_MAX || r0 == rm || Adjacent(r0, rm) || ring.size() != n + 1 ||
      ring.at(r0).size() != 1 || ring.at(rm).size() != 1)
    return false;
  order = {r0};
  {
    Id prev = UINT64_MAX, cur = r0;
    while (cur != rm) {
      const auto& nb = ring.at(cur);
      Id nx = UINT64_MAX;
      for (Id y : nb)
        if (y != prev) nx = y;
      if (nx == UINT64_MAX || order.size() > n) return false;
      order.push_back(nx);
      prev = cur;
      cur = nx;
    }
  }
  if (order.size() != n + 1) return false;
  pa = points.at(a);
  pb = points.at(b);
  if (!(Orientation(pa, pb, points.at(order[0]), points.at(order[1])) > 0)) {
    std::swap(a, b);
    std::swap(pa, pb);
  }
  for (size_t i = 0; i + 1 < order.size(); ++i)
    if (!(Orientation(pa, pb, points.at(order[i]), points.at(order[i + 1])) > 0)) return false;
  {
    long double scale = 0;
    for (Point p : {pa, pb, points.at(r0), points.at(rm)})
      for (Point q : {pa, pb, points.at(r0), points.at(rm)})
        scale = std::max(scale, std::sqrt(((long double)p.x - q.x) * ((long double)p.x - q.x) +
                                          ((long double)p.y - q.y) * ((long double)p.y - q.y) +
                                          ((long double)p.z - q.z) * ((long double)p.z - q.z)));
    if (!(std::abs(Orientation(pa, pb, points.at(r0), points.at(rm))) <= 1e-12L * scale * scale * scale)) return false;
  }
  return true;
}
// Attempts a boundary ring replacement without changing its prescribed skin rules.
bool MeshOptimizer::FlipBoundaryRing(const std::vector<Id>& shell, const std::vector<BoundaryFace>& faces,
                                     const std::vector<Id>& order, Node A, Node B) {
  ++report.trials;
  const bool debt = DebtMode(shell);
  const auto oldMin = OldMerit(shell, debt);
  const size_t m = order.size();
  std::vector<Node> r(m);
  for (size_t i = 0; i < m; ++i) r[i] = {order[i], points.at(order[i])};
  std::map<std::array<size_t, 3>, long double> tri;
  auto triangle = [&](size_t i, size_t k, size_t j) {
    const std::array<size_t, 3> key{i, k, j};
    const auto f = tri.find(key);
    if (f != tri.end()) return f->second;
    const auto s = FreshMerit({Cell{0, {A, r[i], r[k], r[j]}}, Cell{0, {B, r[i], r[j], r[k]}}}, debt);
    tri.emplace(key, s);
    return s;
  };
  std::set<std::pair<size_t, size_t>> forbidden;
  auto ringEdge = [&](size_t i, size_t j) { return j == i + 1 || (i == 0 && j == m - 1); };
  if (!Certified(r[0].p, r[m - 1].p, MaximumMetricEdge)) {
    ++report.rejected_edge;
    return false;
  }
  for (unsigned attempt = 0; attempt < 4; ++attempt) {
    std::vector<std::vector<long double>> best(m, std::vector<long double>(m, -INFINITY));
    std::vector<std::vector<size_t>> arg(m, std::vector<size_t>(m, SIZE_MAX));
    ScoreRingTriangulation(m, debt, forbidden, triangle, ringEdge, best, arg);
    if (!(Progress(best[0][m - 1], oldMin, debt))) {
      if (best[0][m - 1] == -INFINITY)
        ++report.rejected_validity;
      else
        ++report.rejected_quality;
      return false;
    }
    std::vector<std::array<size_t, 3>> used;
    std::vector<std::pair<size_t, size_t>> stack{{0, m - 1}};
    ReadRingTriangulation(arg, used, stack);
    bool ok = true;
    for (const auto& t : used)
      for (const auto& d : {std::make_pair(t[0], t[1]), std::make_pair(t[1], t[2]), std::make_pair(t[0], t[2])}) {
        if (ringEdge(d.first, d.second) || forbidden.count(d)) continue;
        if (!Certified(r[d.first].p, r[d.second].p, MaximumMetricEdge)) {
          forbidden.insert(d);
          ok = false;
          break;
        }
      }
    if (!ok) {
      ++report.rejected_edge;
      if (Limited()) return false;
      continue;
    }
    std::vector<Cell> fresh;
    for (const auto& t : used) {
      fresh.push_back(Cell{0, {A, r[t[0]], r[t[1]], r[t[2]]}});
      fresh.push_back(Cell{0, {B, r[t[0]], r[t[2]], r[t[1]]}});
    }
    if (!Progress(FreshMerit(fresh, debt), oldMin, debt)) {
      ++report.rejected_quality;
      return false;
    }
    std::map<Face, FacetBinding> added;
    if (!BoundaryReplacement(shell, fresh, faces, faces[0].binding, added)) return false;
    Commit(shell, fresh, UINT64_MAX, added);
    ++report.boundary_flips;
    return true;
  }
  return false;
}
// O6: boundary vertex p, every incident boundary face bound to one facet (p interior to it), contracted to a neighbour
// q along a boundary edge of that facet; area and orientation of the facet patch are kept.
bool MeshOptimizer::BoundaryCollapse(Id p, Id q) {
  if (Limited() || !fixed.count(p) || options.frozen_vertices.count(p) || keep.count(p) || !Adjacent(p, q))
    return false;
  const auto sp = star.find(p);
  if (sp == star.end() || sp->second.size() > MaximumCavityCells) return false;
  const auto old = sp->second;
  for (Id id : old)
    if (Frozen(id)) {
      ++report.rejected_frozen;
      return false;
    }
  const auto faces = BoundaryFacesOf(old, p);
  if (faces.empty()) return false;
  for (const auto& f : faces)
    if (f.binding.facet != faces.front().binding.facet) return false;
  bool edge = false;
  for (const auto& f : faces) edge = edge || f.oriented[0] == q || f.oriented[1] == q || f.oriented[2] == q;
  if (!edge) return false;
  // p is interior to the facet patch: its boundary faces close a fan around it (every boundary edge at p used twice)
  {
    std::map<Id, int> uses;
    for (const auto& f : faces)
      for (Id x : f.oriented)
        if (x != p) ++uses[x];
    for (const auto& u : uses)
      if (u.second != 2) return false;
    // the fan (p and its boundary ring) is planar within 1e-12 relative: the contracted faces stay in the facet's plane
    const auto& f0 = faces.front().oriented;
    long double scale = 0;
    for (const auto& u : uses) scale = std::max(scale, (long double)Norm3(points.at(u.first), points.at(p)));
    for (const auto& u : uses)
      if (!(std::abs(Orientation(points.at(f0[0]), points.at(f0[1]), points.at(f0[2]), points.at(u.first))) <=
            1e-12L * scale * scale * scale))
        return false;
    if (!(std::abs(Orientation(points.at(f0[0]), points.at(f0[1]), points.at(f0[2]), points.at(p))) <=
          1e-12L * scale * scale * scale))
      return false;
  }
  ++report.trials;
  const Node Q{q, points.at(q)};
  std::vector<Cell> fresh;
  std::set<Id> link;
  for (Id id : old) {
    const auto& c = mesh.at(id).cell;
    for (const auto& v : c.v)
      if (v.id != p) link.insert(v.id);
    if (CellHas(c, q)) continue;
    Cell x = c;
    for (auto& v : x.v)
      if (v.id == p) v = Q;
    fresh.push_back(x);
  }
  if (fresh.empty()) return false;
  const bool debt = DebtMode(old);
  const auto oldMin = OldMerit(old, debt), newMin = FreshMerit(fresh, debt);
  if (newMin == -INFINITY) {
    ++report.rejected_validity;
    return false;
  }
  if (!(Progress(newMin, oldMin, debt))) {
    ++report.rejected_quality;
    return false;
  }
  for (Id x : link) {
    if (x == q || Adjacent(q, x)) continue;
    if (!Certified(Q.p, points.at(x), MaximumMetricEdge)) {
      ++report.rejected_edge;
      return false;
    }
  }
  std::map<Face, FacetBinding> added;
  if (!BoundaryReplacement(old, fresh, faces, faces.front().binding, added)) return false;
  Commit(old, fresh, p, added);
  ++report.boundary_collapses;
  return true;
}
// DC-13: replace the unfrozen cavity `old` by the cone from one new vertex placed by the merit search (proposal with
// vertex-mean tensors), accepted on the exact checks: every cavity vertex kept, exact positivity and validity of every
// fresh cell, patch-merit progress (or nonregression for Length), ValidateFixedInterface and
// the edge guard on every new edge (limit max(1.8, removed) or, for Length, max(1.8, .75 removed)).
bool MeshOptimizer::ConeReplace(const std::vector<Id>& old, Point start, Insert kind, const std::pair<Id, Id>* split,
                                long double removed) {
  if (Limited() || old.empty() || old.size() > MaximumCavityCells) return false;
  for (Id id : old)
    if (Frozen(id)) {
      ++report.rejected_frozen;
      return false;
    }
  ++report.trials;
  std::vector<Cell> cells;
  std::set<Id> vertices;
  for (Id id : old) {
    cells.push_back(mesh.at(id).cell);
    for (const auto& v : cells.back().v) vertices.insert(v.id);
  }
  const auto faces = CavityBoundary(cells);
  const bool debt = DebtMode(old);
  const auto oldMin = OldMerit(old, debt);
  std::function<bool(Point)> admissible;
  if (split) {
    const Point pa = points.at(split->first), pb = points.at(split->second);
    Tensor m = At(pa);
    const Tensor mb = At(pb);
    m.xx = (m.xx + mb.xx) / 2;
    m.xy = (m.xy + mb.xy) / 2;
    m.xz = (m.xz + mb.xz) / 2;
    m.yy = (m.yy + mb.yy) / 2;
    m.yz = (m.yz + mb.yz) / 2;
    m.zz = (m.zz + mb.zz) / 2;
    long double whole = 0;
    try {
      whole = MetricNorm(pa, pb, m);
    } catch (const std::exception&) {
      return false;
    }
    admissible = [pa, pb, m, whole](Point x) {
      try {
        return MetricNorm(x, pa, m) <= .6L * whole && MetricNorm(x, pb, m) <= .6L * whole;
      } catch (const std::exception&) {
        return false;
      }
    };
  }
  const auto pl = PlaceCone(
      faces, [&](Id v) { return points.at(v); }, [&](Point q) { return At(q); }, start, admissible,
      options.placement_budget, debt);
  ++report.placements;
  report.placement_evaluations += pl.evaluations;
  if (!pl.ok) {
    ++report.rejected_validity;
    return false;
  }

  return CertifyConeReplacement(old, faces, vertices, pl, kind, split, removed, debt, oldMin);
}
// Checks the cone with exact measures before replacing the old cells.
bool MeshOptimizer::CertifyConeReplacement(const std::vector<Id>& old, const std::vector<Face>& faces,
                                           const std::set<Id>& vertices, const Placement& pl, Insert kind,
                                           const std::pair<Id, Id>* split, long double removed, bool debt,
                                           long double oldMin) {
  const Node X{next_node, pl.x};
  std::vector<Cell> fresh;
  std::set<Id> kept;
  for (const auto& f : faces) {
    fresh.push_back(
        Cell{0, {X, Node{f[0], points.at(f[0])}, Node{f[1], points.at(f[1])}, Node{f[2], points.at(f[2])}}});
    for (Id v : f) kept.insert(v);
  }
  // Reject the replacement if a cavity vertex would vanish.
  if (kept.size() != vertices.size()) {
    ++report.placement_refusals;
    return false;
  }
  const auto newMin = FreshMerit(fresh, debt, false);  // trial identity next_node: never memoized
  if (newMin == -INFINITY) {
    ++report.rejected_validity;
    ++report.placement_refusals;
    return false;
  }
  if (!(Progress(newMin, oldMin, debt, kind == Insert::Length))) {
    ++report.rejected_quality;
    ++report.placement_refusals;
    return false;
  }
  const long double limit = kind == Insert::Length ? std::max<long double>(MaximumMetricEdge, .75L * removed)
                                                   : std::max<long double>(MaximumMetricEdge, removed);
  for (Id v : vertices)
    if (!Certified(X.p, points.at(v), limit)) {
      ++report.rejected_edge;
      ++report.placement_refusals;
      return false;
    }
  if (split) {  // O7L: both halves of the removed edge are integrated exactly and strictly shorter than it
    try {
      for (Id e : {split->first, split->second})
        if (!(Exact(X.p, points.at(e)) < removed)) {
          ++report.rejected_edge;
          ++report.placement_refusals;
          return false;
        }
    } catch (const std::exception&) {
      ++report.rejected_edge;
      ++report.placement_refusals;
      return false;
    }
  }
  if (!Interface(old, fresh)) {
    ++report.placement_refusals;
    return false;
  }
  ++next_node;
  Commit(old, fresh);
  ++(kind == Insert::Shell           ? report.shell_insertions
     : kind == Insert::Neighbourhood ? report.neighbourhood_insertions
                                     : report.length_insertions);
  return true;
}
// O7 / O7L: the shell of edge (a,b) (closed ring for O7L) re-coned from a vertex placed near the edge midpoint.
bool MeshOptimizer::ShellInsert(Id a, Id b, bool lengthDriven) {
  if (Limited() || !options.insertions) return false;
  const auto sa = star.find(a), sb = star.find(b);
  if (sa == star.end() || sb == star.end()) return false;
  auto shell = Common(sa->second, sb->second);
  if (shell.size() < 2 || shell.size() > 2 * options.maximum_shell) return false;
  if (!lengthDriven) {
    shell.erase(std::remove_if(shell.begin(), shell.end(), [&](Id id) { return Frozen(id); }), shell.end());
    if (shell.empty()) return false;
  }  // partial shell: the edge stays
  if (lengthDriven) {
    std::map<Id, int> ring;
    for (Id id : shell)
      for (const auto& v : mesh.at(id).cell.v)
        if (v.id != a && v.id != b) ++ring[v.id];
    if (ring.size() != shell.size()) return false;
    for (const auto& r : ring)
      if (r.second != 2) return false;
  }  // closed shell only
  const Point pa = points.at(a), pb = points.at(b);
  const Point mid{double(((long double)pa.x + pb.x) / 2), double(((long double)pa.y + pb.y) / 2),
                  double(((long double)pa.z + pb.z) / 2)};
  long double removed = MaximumMetricEdge;
  try {
    const auto e = Estimate(pa, pb);
    removed = e > EstimatePass ? Exact(pa, pb) : e;
  } catch (const std::exception&) {
    return false;
  }
  const std::pair<Id, Id> edge{a, b};
  return ConeReplace(shell, mid, lengthDriven ? Insert::Length : Insert::Shell, lengthDriven ? &edge : nullptr,
                     removed);
}
// O8: the bad cell and its face neighbours across interior faces re-coned from a vertex placed near its centroid.
bool MeshOptimizer::NeighbourhoodInsert(Id cell) {
  if (Limited() || !options.insertions || !mesh.count(cell)) return false;
  const auto& c = mesh.at(cell).cell;
  std::vector<Id> old{cell};
  for (auto f : OppositeFaces(c)) {
    const auto both = Common(Common(star.at(f[0]), star.at(f[1])), star.at(f[2]));
    for (Id id : both)
      if (id != cell && !Frozen(id)) old.push_back(id);
  }  // a frozen neighbour stays: its face bounds the cavity
  if (Frozen(cell)) return false;
  return ConeReplace(old, Centroid(c), Insert::Neighbourhood);
}
// Exposes a physical diagonal through a certified shell replacement.
bool MeshOptimizer::ShellBoundary(Id id) {
  D9QueryScope d9_query(SU2NativeD9::Site::ShellBoundary, options.geometry);
  if (Limited() || !options.geometry || Frozen(id)) return false;
  const auto& row = mesh.at(id);
  std::vector<unsigned> faces;
  for (unsigned k = 0; k < 4; ++k)
    if (row.physical[k].present) faces.push_back(k);
  if (faces.size() != 2) return false;
  // Opposite vertices form the new physical diagonal; its entire current ring is the dependency closure.
  const Id a = row.cell.v[faces[0]].id, b = row.cell.v[faces[1]].id;
  const auto shell = Common(star.at(a), star.at(b));
  if (shell.size() < 3 || shell.size() > options.maximum_shell) return false;
  for (Id c : shell)
    if (Frozen(c)) {
      ++report.rejected_frozen;
      return false;
    }
  ++report.trials;
  std::vector<Cell> cells;
  for (Id c : shell) cells.push_back(mesh.at(c).cell);
  const auto old = GeometryPatch(cells);
  const auto& geometry = moved_geometry ? *moved_geometry : *options.geometry;
  Patch fresh;
  std::string reason;
  if (!ShellBoundaryCell(old, id, next_cell, geometry, fresh, reason)) {
    ++report.rejected_interface;
    return false;
  }
  const bool debt = DebtMode(shell, &old);
  const auto score = FreshMerit(fresh.cells, debt, false, &fresh);
  if (!std::isfinite(score)) {
    ++report.rejected_validity;
    return false;
  }
  if (!(Progress(score, OldMerit(shell, debt), debt))) {
    ++report.rejected_quality;
    return false;
  }
  auto context = geometry.PrivateParameters(fresh);
  const auto& proposed = context ? *context : geometry;
  std::map<Face, FacetBinding> bindings;
  bool strict_geometry = true;
  for (const auto& face : fresh.surface) {
    const auto key = Key(face.v);
    if (!physical.count(key) && options.surface_tolerance > 0) {
      const std::array<Point, 3> p{points.at(face.v[0]), points.at(face.v[1]), points.at(face.v[2])};
      if (!(proposed.SurfaceDeviation(face.facet, p, SU2NativeGeometry::SurfaceQueryNeed::Classify(), true) <=
            options.surface_tolerance)) {
        strict_geometry = false;
      }
    }
    bindings.emplace(key, FacetBinding{true, face.facet});
  }
  if (!strict_geometry) {
    if (GetShapeAdmission() != ShapeAdmission::ValidityOnly) {
      ++report.rejected_interface;
      return false;
    }
    // As for boundary smoothing, a useful intermediate may retain non-increasing geometry debt.
    const auto before = GeometryGuidanceExcess(old, geometry, options.surface_tolerance);
    const auto after = GeometryGuidanceExcess(fresh, proposed, options.surface_tolerance, before.excess);
    if (!before.complete || !after.complete || !std::isfinite(after.excess) || after.excess > before.excess) {
      ++report.rejected_interface;
      return false;
    }
  }
  return CommitShellBoundary(shell, fresh, geometry, bindings);
}
// Publishes the certified boundary shell and refreshes the parameter overlay.
bool MeshOptimizer::CommitShellBoundary(const std::vector<Id>& shell, const Patch& fresh,
                                        const PlanarReference& geometry, const std::map<Face, FacetBinding>& bindings) {
  Commit(shell, fresh.cells, UINT64_MAX, bindings);
  for (const auto& p : fresh.parameters) report.parameters.insert_or_assign(p.first, p.second);
  for (const auto& trace : fresh.coverage.traces) report.coverage.traces.insert_or_assign(trace.first, trace.second);
  for (const auto& face : fresh.coverage.faces) report.coverage.faces.insert_or_assign(face.first, face.second);
  for (auto i = report.coverage.faces.begin(); i != report.coverage.faces.end();)
    if (!physical.count(Key(i->first)))
      i = report.coverage.faces.erase(i);
    else
      ++i;
  for (auto i = report.coverage.traces.begin(); i != report.coverage.traces.end();)
    if (!Adjacent(i->first[0], i->first[1]))
      i = report.coverage.traces.erase(i);
    else
      ++i;
  Patch overlay;
  overlay.parameters = report.parameters;
  overlay.coverage = report.coverage;
  overlay.surface = fresh.surface;
  auto parameters = geometry.PrivateParameters(overlay);
  if (parameters) moved_geometry = std::move(parameters);
  ++report.boundary_flips;
  return true;
}
// Tries the enabled operators in their original order on a current cell.
bool MeshOptimizer::TryCell(Id id, bool enrichment) {
  const Cell c = mesh.at(id).cell;
  if (!enrichment && options.swaps) {
    if (ShellBoundary(id)) return true;
    for (unsigned i = 0; i < 4; ++i)
      for (unsigned j = i + 1; j < 4; ++j) {
        if (!mesh.count(id)) return true;
        if (EdgeRemoval(std::min(c.v[i].id, c.v[j].id), std::max(c.v[i].id, c.v[j].id))) return true;
      }
    for (auto f : OppositeFaces(c)) {
      if (!mesh.count(id)) return true;
      std::sort(f.begin(), f.end());
      if (FaceSwap(f)) return true;
    }
    for (unsigned i = 0; i < 4; ++i)
      for (unsigned j = i + 1; j < 4; ++j) {
        if (!mesh.count(id)) return true;
        if (options.boundary && !options.geometry && fixed.count(c.v[i].id) && fixed.count(c.v[j].id) &&
            BoundaryFlip(c.v[i].id, c.v[j].id))
          return true;
      }
  }
  if (enrichment && options.insertions) {  // DC-13 O7 (longest edges first) and O8
    std::vector<std::pair<long double, std::pair<Id, Id>>> edges;
    for (unsigned i = 0; i < 4; ++i)
      for (unsigned j = i + 1; j < 4; ++j) {
        long double l = 0;
        try {
          l = Estimate(c.v[i].p, c.v[j].p);
        } catch (const std::exception&) {
        }
        edges.push_back({-l, {c.v[i].id, c.v[j].id}});
      }
    std::sort(edges.begin(), edges.end());
    for (const auto& e : edges) {
      if (!mesh.count(id)) return true;
      if (ShellInsert(e.second.first, e.second.second)) return true;
    }
    if (!mesh.count(id)) return true;
    if (NeighbourhoodInsert(id)) return true;
  }
  if (!enrichment && options.collapses) {
    std::vector<std::pair<long double, std::pair<Id, Id>>> edges;
    for (unsigned i = 0; i < 4; ++i)
      for (unsigned j = i + 1; j < 4; ++j) {
        long double l = INFINITY;
        try {
          l = Estimate(c.v[i].p, c.v[j].p);
        } catch (const std::exception&) {
        }
        edges.push_back({l, {c.v[i].id, c.v[j].id}});
      }
    std::sort(edges.begin(), edges.end());
    for (const auto& e : edges) {
      if (!mesh.count(id)) return true;
      if (Collapse(e.second.first, e.second.second) || Collapse(e.second.second, e.second.first)) return true;
    }
    if (options.boundary && !options.geometry)
      for (const auto& e : edges) {
        if (!mesh.count(id)) return true;
        if (BoundaryCollapse(e.second.first, e.second.second) || BoundaryCollapse(e.second.second, e.second.first))
          return true;
      }
  }
  return false;
}
// Charges nested work once and credits progress only on publication.
void MeshOptimizer::AccountNested(const OptimizeReport& n, bool selected) {
  // Charge attempted work once; credit progress only after the complete candidate is admitted.
  if (!selected) {
    report.sweeps += n.sweeps;
    report.trials += n.trials;
    report.rejected_orientation += n.rejected_orientation;
    report.rejected_interface += n.rejected_interface;
    report.rejected_quality += n.rejected_quality;
    report.rejected_edge += n.rejected_edge;
    report.rejected_frozen += n.rejected_frozen;
    report.rejected_validity += n.rejected_validity;
    report.placements += n.placements;
    report.placement_refusals += n.placement_refusals;
    report.placement_evaluations += n.placement_evaluations;
    report.long_edges_seen += n.long_edges_seen;
    report.point_queries += n.point_queries;
    report.edge_integrations += n.edge_integrations;
    report.edge_surrogates += n.edge_surrogates;
    report.boundary_stars_searched += n.boundary_stars_searched;
    report.boundary_stars_reused += n.boundary_stars_reused;
    report.boundary_candidates += n.boundary_candidates;
    report.boundary_rejected_validity += n.boundary_rejected_validity;
    report.boundary_rejected_shape += n.boundary_rejected_shape;
    report.boundary_rejected_edge += n.boundary_rejected_edge;
    report.boundary_rejected_geometry += n.boundary_rejected_geometry;
    report.boundary_guidance_calls += n.boundary_guidance_calls;
    report.boundary_upper_queries += n.boundary_upper_queries;
    report.boundary_move_contexts += n.boundary_move_contexts;
    report.boundary_prepared_trials += n.boundary_prepared_trials;
    report.boundary_guidance_faces += n.boundary_guidance_faces;
    report.boundary_guidance_reused_faces += n.boundary_guidance_reused_faces;
    report.boundary_guidance_early_rejections += n.boundary_guidance_early_rejections;
    report.boundary_seconds += n.boundary_seconds;
    report.interior_stars_searched += n.interior_stars_searched;
    report.interior_stars_reused += n.interior_stars_reused;
    report.interior_candidates += n.interior_candidates;
    report.interior_rejected_validity += n.interior_rejected_validity;
    report.interior_rejected_shape += n.interior_rejected_shape;
    report.interior_rejected_edge += n.interior_rejected_edge;
    report.interior_rejected_geometry += n.interior_rejected_geometry;
    report.interior_guidance_calls += n.interior_guidance_calls;
    report.interior_upper_queries += n.interior_upper_queries;
    report.interior_seconds += n.interior_seconds;
  } else {
    report.edge_removals += n.edge_removals;
    report.face_swaps += n.face_swaps;
    report.collapses += n.collapses;
    report.moves += n.moves;
    report.boundary_flips += n.boundary_flips;
    report.boundary_collapses += n.boundary_collapses;
    report.length_removals += n.length_removals;
    report.shell_insertions += n.shell_insertions;
    report.neighbourhood_insertions += n.neighbourhood_insertions;
    report.length_insertions += n.length_insertions;
    report.boundary_moves += n.boundary_moves;
    report.boundary_debt_reduction += n.boundary_debt_reduction;
    report.interior_moves += n.interior_moves;
    report.interior_debt_reduction += n.interior_debt_reduction;
  }
}
// Attempts the bounded shell and insertion recovery with its borrowed atlas.
bool MeshOptimizer::CoupledBoundary() {
  D9QueryScope d9_query(SU2NativeD9::Site::CoupledBoundary, options.geometry);
  if (!coupled || Limited() || !options.geometry || !options.swaps || !options.insertions ||
      GetShapeAdmission() != ShapeAdmission::ValidityOnly || mesh.size() > MaximumCavityCells ||
      options.maximum_work - report.trials < 3 || report.sweeps >= options.maximum_sweeps ||
      next_node >= UINT64_MAX - 1 || next_cell >= UINT64_MAX - MaximumReplacementCells)
    return false;
  std::vector<std::pair<long double, Id>> candidates;
  for (const auto& row : mesh)
    if (!Frozen(row.first) &&
        std::count_if(row.second.physical.begin(), row.second.physical.end(),
                      [](const auto& f) { return f.present; }) == 2 &&
        RowScore(row.first, row.second.cell) < 1)
      candidates.push_back({RowPriority(row.first, row.second.cell), row.first});
  if (candidates.empty()) return false;
  std::sort(candidates.begin(), candidates.end());
  const auto& core = mesh.at(candidates.front().second);
  std::vector<unsigned> faces;
  for (unsigned k = 0; k < 4; ++k)
    if (core.physical[k].present) faces.push_back(k);
  const Id a = core.cell.v[faces[0]].id, b = core.cell.v[faces[1]].id;
  const auto shell = Common(star.at(a), star.at(b));
  if (shell.size() < 3 || shell.size() > options.maximum_shell) return false;
  for (Id id : shell)
    if (Frozen(id)) return false;
  const auto& geometry = moved_geometry ? *moved_geometry : *options.geometry;
  auto* budget = geometry.AtlasReservation();
  if (!budget) return false;
  struct Restore {
    SU2NativeGeometry::AtlasBudget& budget;
    size_t caller;
    ~Restore() { budget.caller_bytes = caller; }
  } restore{*budget, budget->caller_bytes};
  const auto live = transfer_memory::Add(budget->caller_bytes, transfer_memory::Mul(2 * MaximumReplacementCells, 8192),
                                         geometry.ParameterOverlayBytes());
  if (live == SIZE_MAX || live > budget->maximum_bytes)
    throw std::length_error("Coupled boundary candidate exceeds storage admission.");
  budget->caller_bytes = live;
  budget->peak_bytes = std::max(budget->peak_bytes, live);
  return PrepareCoupledBoundary(shell, a, b, core.cell.id, geometry);
}
// Builds the speculative shell split and retains its original patch through certification.
bool MeshOptimizer::PrepareCoupledBoundary(const std::vector<Id>& shell, Id a, Id b, Id core,
                                           const PlanarReference& geometry) {
  std::vector<Id> all_ids;
  std::vector<Cell> all_cells, ring;
  for (const auto& row : mesh) {
    all_ids.push_back(row.first);
    all_cells.push_back(row.second.cell);
  }
  for (Id id : shell) ring.push_back(mesh.at(id).cell);
  const auto original = GeometryPatch(all_cells), old = GeometryPatch(ring);
  Patch exposed, refined;
  std::string reason;
  ++report.trials;
  if (!ShellBoundaryCell(old, core, next_cell, geometry, exposed, reason)) {
    ++report.rejected_interface;
    return false;
  }
  if (Limited()) return false;
  const auto pa = points.at(a), pb = points.at(b);
  const Node point{next_node,
                   {double(((long double)pa.x + pb.x) / 2), double(((long double)pa.y + pb.y) / 2),
                    double(((long double)pa.z + pb.z) / 2)}};
  auto exposed_context = geometry.PrivateParameters(exposed);
  const auto& exposed_geometry = exposed_context ? *exposed_context : geometry;
  ++report.trials;
  if (!SplitEdge(exposed, {a, b}, point, next_cell, exposed_geometry, refined, reason)) {
    ++report.rejected_interface;
    return false;
  }
  if (next_cell > UINT64_MAX - refined.cells.size()) return false;
  Owned pending = mesh;
  for (Id id : shell) pending.erase(id);
  std::map<Face, Id> bindings;
  for (const auto& face : refined.surface) bindings.emplace(Key(face.v), face.facet);
  for (const auto& cell : refined.cells) {
    OwnedCell row;
    row.cell = cell;
    row.version = 1;
    const auto skin = OppositeFaces(cell);
    for (unsigned k = 0; k < 4; ++k) {
      const auto found = bindings.find(Key(skin[k]));
      if (found != bindings.end()) row.physical[k] = {true, found->second};
    }
    if (!pending.emplace(cell.id, row).second)
      throw std::logic_error("Coupled boundary candidate repeats a cell identity.");
  }
  if (pending.size() > MaximumReplacementCells) return false;
  return OptimizeCoupledBoundary(all_ids, original, refined, pending, geometry, reason);
}
// Builds the private mesh patch with the original overlay and coverage order.
Patch MeshOptimizer::CoupledPatch(const Owned& rows, const Patch& original, const Patch& refined) {
  Patch result;
  result.parameters = original.parameters;
  result.coverage = original.coverage;
  for (const auto& row : refined.parameters) result.parameters.insert_or_assign(row.first, row.second);
  for (const auto& row : refined.coverage.faces) result.coverage.faces.insert_or_assign(row.first, row.second);
  for (const auto& row : refined.coverage.traces) result.coverage.traces.insert_or_assign(row.first, row.second);
  for (const auto& row : rows) {
    result.cells.push_back(row.second.cell);
    const auto skin = OppositeFaces(row.second.cell);
    for (unsigned k = 0; k < 4; ++k)
      if (row.second.physical[k].present) result.surface.push_back({skin[k], row.second.physical[k].facet});
  }
  RetainPhysicalCoverage(result);
  return result;
}
// Runs the nested optimizer with the original remaining time, work and ID bounds.
bool MeshOptimizer::OptimizeCoupledBoundary(const std::vector<Id>& all_ids, const Patch& original, const Patch& refined,
                                            Owned& pending, const PlanarReference& geometry, std::string& reason) {
  auto initial = CoupledPatch(pending, original, refined);
  auto context = geometry.PrivateParameters(initial);
  if (!context) return false;
  OptimizeOptions nested_options = options;
  nested_options.geometry = context.get();
  nested_options.keep_vertices = keep;
  nested_options.diagnostic_sliding = sliding;
  nested_options.maximum_work = options.maximum_work - report.trials;
  nested_options.maximum_sweeps = options.maximum_sweeps - report.sweeps;
  if (options.maximum_seconds > 0) {
    nested_options.maximum_seconds =
        options.maximum_seconds - std::chrono::duration<double>(Clock::now() - started).count();
    if (nested_options.maximum_seconds <= 0) return false;
  }
  Id node = next_node + 1, cell = next_cell + refined.cells.size();
  OptimizeReport nested_report;
  std::set<Id> final_keep;
  std::map<Id, std::array<double, 3>> final_sliding;
  {
    MeshOptimizer nested(pending, target, node, cell, nested_options, nested_report);
    nested.coupled = false;
    nested.Run();
    final_keep = std::move(nested.keep);
    final_sliding = std::move(nested.sliding);
  }
  if (nested_report.trials > nested_options.maximum_work || nested_report.sweeps > nested_options.maximum_sweeps ||
      node < next_node + 1 || cell < next_cell + refined.cells.size() || node == UINT64_MAX || cell == UINT64_MAX)
    throw std::logic_error("Coupled boundary candidate exceeded work/identity reservation.");
  AccountNested(nested_report, false);
  if (options.maximum_seconds > 0 &&
      std::chrono::duration<double>(Clock::now() - started).count() >= options.maximum_seconds) {
    limited = true;
    return false;
  }
  return CertifyCoupledBoundary(all_ids, original, refined, pending, geometry, reason, node, cell, nested_report,
                                final_keep, final_sliding);
}
// Counts the exact shape failures without memoizing provisional cell identities.
std::array<size_t, 2> MeshOptimizer::ShapeFailures(const std::vector<Cell>& cells) {
  std::array<size_t, 2> result{};
  for (const auto& c : cells) {
    const auto q = Quality(c, false);
    if (!q.Valid()) throw std::invalid_argument("Coupled boundary candidate has invalid quality.");
    result[0] += q.q < MinimumMeanRatio;
    result[1] += q.J < MinimumScaledJacobian;
  }
  return result;
}
// Checks the complete coupled patch before it changes authoritative optimizer state.
bool MeshOptimizer::CertifyCoupledBoundary(const std::vector<Id>& all_ids, const Patch& original, const Patch& refined,
                                           Owned& pending, const PlanarReference& geometry, std::string& reason,
                                           Id node, Id cell, OptimizeReport& nested_report, std::set<Id>& final_keep,
                                           std::map<Id, std::array<double, 3>>& final_sliding) {
  if (pending.size() > MaximumReplacementCells) return false;
  auto candidate = CoupledPatch(pending, original, refined);
  for (const auto& row : nested_report.parameters) candidate.parameters.insert_or_assign(row.first, row.second);
  for (const auto& row : nested_report.coverage.faces) candidate.coverage.faces.insert_or_assign(row.first, row.second);
  for (const auto& row : nested_report.coverage.traces)
    candidate.coverage.traces.insert_or_assign(row.first, row.second);
  RetainPhysicalCoverage(candidate);
  const bool debt = DebtMode(all_ids, &original);
  const auto before = OldMerit(all_ids, debt), after = FreshMerit(candidate.cells, debt, false, &candidate);
  if (!Progress(after, before, debt)) {
    ++report.rejected_quality;
    return false;
  }
  std::array<size_t, 2> old_bad;
  {
    SU2NativeTiming3D::Scope old_cavity_scope(SU2NativeTiming3D::Operation::OldCavityMeasurement);
    old_bad = ShapeFailures(original.cells);
  }
  const auto new_bad = ShapeFailures(candidate.cells);
  if (new_bad[0] > old_bad[0] || new_bad[1] > old_bad[1] || new_bad == old_bad) {
    ++report.rejected_quality;
    return false;
  }
  for (const auto& c : candidate.cells)
    for (unsigned i = 0; i < 4; ++i)
      for (unsigned j = i + 1; j < 4; ++j)
        if ((!points.count(c.v[i].id) || !points.count(c.v[j].id) || !Adjacent(c.v[i].id, c.v[j].id)) &&
            !Certified(c.v[i].p, c.v[j].p, MaximumMetricEdge)) {
          ++report.rejected_edge;
          return false;
        }
  auto final_context = geometry.PrivateParameters(candidate);
  if (!final_context) return false;
  const auto old_geometry = GeometryGuidanceExcess(original, geometry, options.surface_tolerance);
  const auto new_geometry =
      GeometryGuidanceExcess(candidate, *final_context, options.surface_tolerance, old_geometry.excess);
  if (!old_geometry.complete || !new_geometry.complete || !std::isfinite(new_geometry.excess) ||
      new_geometry.excess > old_geometry.excess) {
    ++report.rejected_interface;
    return false;
  }
  std::set<Face> old_physical;
  std::map<Face, Face> skin;
  std::vector<Face> prescribed;
  if (!CertifyCoupledInterface(original, candidate, old_physical, skin, prescribed, reason)) return false;
  if (options.maximum_seconds > 0 &&
      std::chrono::duration<double>(Clock::now() - started).count() >= options.maximum_seconds) {
    limited = true;
    return false;
  }
  return PublishCoupledBoundary(pending, node, cell, candidate, final_context, nested_report, final_keep,
                                final_sliding);
}
// Retains the artificial interface and replaces the physical skin with the candidate faces.
bool MeshOptimizer::CertifyCoupledInterface(const Patch& original, const Patch& candidate, std::set<Face>& old_physical,
                                            std::map<Face, Face>& skin, std::vector<Face>& prescribed,
                                            std::string& reason) {
  for (const auto& face : original.surface) old_physical.insert(Key(face.v));
  for (const auto& face : Incidence(original.cells).Boundary())
    if (!old_physical.count(Key(face))) skin.emplace(Key(face), face);
  for (auto face : candidate.surface) {
    std::rotate(face.v.begin(), std::min_element(face.v.begin(), face.v.end()), face.v.end());
    skin.emplace(Key(face.v), face.v);
  }
  for (const auto& face : skin) prescribed.push_back(face.second);
  if (!ValidatePrescribedInterface(original.cells, candidate.cells, prescribed, reason)) {
    report.stop = reason;
    ++report.rejected_interface;
    return false;
  }
  return true;
}
// Publishes the private mesh and gives credit only after certification.
bool MeshOptimizer::PublishCoupledBoundary(Owned& pending, Id node, Id cell, Patch& candidate,
                                           std::unique_ptr<PlanarReference>& final_context,
                                           const OptimizeReport& nested_report, std::set<Id>& final_keep,
                                           std::map<Id, std::array<double, 3>>& final_sliding) {
  mesh.swap(pending);
  next_node = node;
  next_cell = cell;
  Index();
  keep = std::move(final_keep);
  sliding = std::move(final_sliding);
  report.parameters = std::move(candidate.parameters);
  report.coverage = std::move(candidate.coverage);
  for (const auto& row : nested_report.relabel) {
    Id origin = row.first;
    for (const auto& prior : report.relabel)
      if (prior.second == row.first) {
        origin = prior.first;
        break;
      }
    report.relabel[origin] = row.second;
  }
  moved_geometry = std::move(final_context);
  AccountNested(nested_report, true);
  ++report.shell_insertions;
  ++report.boundary_flips;
  changed = true;
  rejected_smoothing.clear();
  Bound();
  return true;
}
// Runs the original metric optimization sweep and reports the final census.
bool MeshOptimizer::Run() {
  SU2NativeTiming3D::PhaseScope quality_phase(SU2NativeTiming3D::Phase::Quality);
  SU2NativeTiming3D::Scope optimization_timer(SU2NativeTiming3D::Operation::QualityOptimization);
  Index();

  QualityCensus(report.bad_before, report.minimum_before);
  RunSweeps();
  if (report.stop.empty()) report.stop = "sweep cap";
  QualityCensus(report.bad_after, report.minimum_after);
  report.seconds = std::chrono::duration<double>(Clock::now() - started).count();
  return changed;
}
// Reports the existing score census over cells excluded only by the frozen-cell set.
void MeshOptimizer::QualityCensus(size_t& bad, double& minimum) {
  bad = 0;
  minimum = INFINITY;
  for (const auto& row : mesh) {
    if (options.frozen_cells.count(row.first)) continue;
    const double s = RowScore(row.first, row.second.cell);
    minimum = std::min(minimum, s);
    bad += s < 1;
  }
  if (!std::isfinite(minimum)) minimum = 1;
}
// Drives the enabled optimizer stages within each existing sweep budget.
void MeshOptimizer::RunSweeps() {
  while (report.sweeps < options.maximum_sweeps) {
    ++report.sweeps;
    size_t accepted = 0;
    const std::array<bool, 3> enabled{options.smoothing, options.swaps || options.collapses, options.insertions};
    for (unsigned stage = 0; stage < enabled.size(); ++stage) {
      RunSweepStage(stage, enabled, accepted);
    }
    stage_work_end = options.maximum_work;
    if (Limited()) {
      report.stop = "limit";
      break;
    }
    if (!accepted) {
      const bool combined = CoupledBoundary();
      QualityCensus(report.bad_after, report.minimum_after);
      if (combined || report.stop.empty())
        report.stop = report.bad_after ? (combined ? "coupled progress" : "no progress") : "complete";
      break;
    }
  }
}
// Reserves the original stage work and invokes the existing ordered operators.
void MeshOptimizer::RunSweepStage(unsigned stage, const std::array<bool, 3>& enabled, size_t& accepted) {
  stage_work_end = options.maximum_work;
  if (!enabled[stage] || Limited()) return;
  const size_t remaining = std::count(enabled.begin() + stage, enabled.end(), true);
  // A phase refusal is temporary: reserve work for every later enabled phase, including enrichment.
  stage_work_end = report.trials + (options.maximum_work - report.trials) / remaining;
  std::vector<std::pair<long double, Id>> bad;
  for (const auto& row : mesh) {
    if (Limited()) break;
    if (options.frozen_cells.count(row.first)) continue;
    const double s = RowScore(row.first, row.second.cell);
    if (s < 1) bad.emplace_back(RowPriority(row.first, row.second.cell), row.first);
  }
  std::sort(bad.begin(), bad.end());

  if (stage == 0) {
    ImproveSmoothing(bad, accepted);
  } else {
    ImproveTopology(bad, stage == 2, accepted);
    ImproveLongEdges(stage, accepted);
  }
}
// Visits each bad-cell vertex once, maintaining its initial occurrence order.
void MeshOptimizer::ImproveSmoothing(const PoorCells& bad, size_t& accepted) {
  std::set<Id> seen;
  std::vector<Id> vertices;
  for (const auto& row : bad)
    for (const auto& v : mesh.at(row.second).cell.v)
      if (seen.insert(v.id).second) vertices.push_back(v.id);
  for (Id v : vertices) {
    if (Limited()) break;
    const auto s = star.find(v);
    if (s == star.end() || s->second.empty()) continue;
    if (MinScore(s->second) < 1 && Smooth(v)) ++accepted;
  }
}
// Tries topology changes on the still-bad cells in their saved priority order.
void MeshOptimizer::ImproveTopology(const PoorCells& bad, bool enrichment, size_t& accepted) {
  for (const auto& row : bad) {
    if (Limited()) break;
    if (!mesh.count(row.second)) continue;
    if (RowScore(row.second, mesh.at(row.second).cell) < 1 && TryCell(row.second, enrichment)) ++accepted;
  }
}
// Scans long edges and tries removal or insertion in the original stage order.
void MeshOptimizer::ImproveLongEdges(unsigned stage, size_t& accepted) {
  // Length repair follows the same topology/enrichment ordering as quality repair.
  if ((stage == 1 ? options.swaps : options.insertions) && !Limited()) {
    std::vector<std::pair<long double, std::pair<Id, Id>>> longs;
    std::set<std::pair<Id, Id>> seen;
    for (const auto& row : mesh) {
      if (Limited()) break;
      if (Frozen(row.first)) continue;
      const auto& c = row.second.cell;
      for (unsigned i = 0; i < 4; ++i)
        for (unsigned j = i + 1; j < 4; ++j) {
          const auto e = std::minmax(c.v[i].id, c.v[j].id);
          if (!seen.insert(e).second) continue;
          try {
            if (Estimate(c.v[i].p, c.v[j].p) <= 1.6L) continue;
            const auto l = Exact(c.v[i].p, c.v[j].p);
            if (l > MaximumMetricEdge) longs.push_back({-l, e});
          } catch (const std::exception&) {
          }
        }
    }
    std::sort(longs.begin(), longs.end());
    report.long_edges_seen += longs.size();
    for (const auto& e : longs) {
      if (Limited()) break;
      if (!Adjacent(e.second.first, e.second.second)) continue;
      if (stage == 1 ? EdgeRemoval(e.second.first, e.second.second, true)
                     : ShellInsert(e.second.first, e.second.second, true))
        ++accepted;
    }
  }
}
}  // namespace
// Checks the borrowed atlas admission and optimizes the caller's mesh in place.
bool OptimizeMetricMesh(Owned& mesh, const RegionMetric& target, Id& next_node, Id& next_cell,
                        const OptimizeOptions& options, OptimizeReport& report) {
  auto* reservation = options.geometry ? options.geometry->AtlasReservation() : nullptr;
  if (options.geometry && options.geometry->Mapped() &&
      (options.geometry->Mobility() || !options.geometry->Parameters()->Coverage().faces.empty() ||
       !options.geometry->Parameters()->Coverage().traces.empty()) &&
      !reservation) {
    report.stop = "atlas reservation missing";
    return false;
  }
  const auto before = reservation ? reservation->work : 0;
  MeshOptimizer optimizer(mesh, target, next_node, next_cell, options, report);
  const bool changed = optimizer.Run();
  if (reservation) {
    report.atlas_work = reservation->work - before;
    report.atlas_peak_bytes = reservation->peak_bytes;
  }
  return changed;
}
