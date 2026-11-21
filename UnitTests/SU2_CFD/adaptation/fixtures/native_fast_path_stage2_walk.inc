// Projection measurements and the float walk shared with the split fixture.
using WalkWeights = std::array<double, 3>;
using WalkNet = std::array<WalkWeights, 10>;
using SU2NativeGeometry::MappedFacetReference;
using SU2NativeGeometry::SurfaceBinding;

struct WalkStart {
  Id facet = UINT64_MAX;
  double u = 0, v = 0;
  bool fallback = false;
};

struct WalkResult {
  Id facet = UINT64_MAX, start_facet = UINT64_MAX;
  double u = 0, v = 0, distance = std::numeric_limits<double>::infinity();
  Point query, foot;
  size_t steps = 0, changes = 0, nonconverged = 0;
  bool converged = false, capped = false, fallback = false;
};

double WalkDistance(Point a, Point b) { return std::hypot(a.x - b.x, a.y - b.y, a.z - b.z); }

class Walker {
 public:
  explicit Walker(const State& state) : state(state), geometry(state.Geometry()) {
    std::map<Edge, std::pair<Id, unsigned>> first;
    for (const auto& [id, face] : geometry.Reference().Facets()) {
      auto& row = facets[id];
      row.entity = state.Entity(id);
      for (unsigned k = 0; k < 3; ++k) row.vertices[k] = face.v[k].id;
      const auto& net = geometry.Curves().at(id);
      for (unsigned k = 0; k < net.size(); ++k) row.net[k] = {net[k].x, net[k].y, net[k].z};
      for (unsigned k = 0; k < 3; ++k) {
        Edge edge{row.vertices[(k + 1) % 3], row.vertices[(k + 2) % 3]};
        if (edge[0] > edge[1]) std::swap(edge[0], edge[1]);
        if (state.Mobility().features.count(edge)) continue;
        const auto [where, inserted] = first.emplace(edge, std::make_pair(id, k));
        if (inserted) continue;
        const auto [other, opposite] = where->second;
        if (facets.at(other).entity != row.entity) continue;
        row.neighbors[k] = other;
        facets.at(other).neighbors[opposite] = id;
      }
    }
  }

  WalkStart Start(const SurfaceFace& face, const Triangle& triangle, Point query, const WalkWeights& weights) const {
    if (std::all_of(triangle.begin(), triangle.end(),
                    [&](const auto& point) { return geometry.Supports(face.facet, point.binding); }))
      return Interpolated(face.facet, triangle, weights);

    const auto nearest = std::min_element(triangle.begin(), triangle.end(), [&](const auto& a, const auto& b) {
      const auto left = WalkDistance(a.node.p, query), right = WalkDistance(b.node.p, query);
      return left == right ? a.node.id < b.node.id : left < right;
    });
    Id oriented = UINT64_MAX, equal = UINT64_MAX;
    const auto entity = facets.at(face.facet).entity;
    const std::array<Point, 3> corners{triangle[0].node.p, triangle[1].node.p, triangle[2].node.p};
    geometry.ForFacets(nearest->binding, [&](Id side) {
      if (facets.at(side).entity != entity) return;
      equal = std::min(equal, side);
      if (geometry.OrientedFace(side, nearest->binding, corners)) oriented = std::min(oriented, side);
    });
    const Id side = oriented == UINT64_MAX ? equal : oriented;
    if (side == UINT64_MAX) return {side, 0, 0, true};
    const auto uv = geometry.SurfaceCoordinates(side, nearest->binding);
    return {side, uv.y, uv.z, oriented == UINT64_MAX};
  }

  WalkStart EdgeStart(const SurfaceFace& face, const Triangle& triangle, unsigned first, unsigned second,
                      Point query) const {
    WalkWeights weights{};
    weights.at(first) = weights.at(second) = .5;
    const auto& a = triangle.at(first).binding;
    const auto& b = triangle.at(second).binding;
    if (geometry.Supports(face.facet, a) && geometry.Supports(face.facet, b))
      return Interpolated(face.facet, triangle, weights);
    Id side = UINT64_MAX;
    const auto entity = facets.at(face.facet).entity;
    geometry.ForFacets(a, [&](Id candidate) {
      if (facets.at(candidate).entity == entity && geometry.Supports(candidate, b)) side = std::min(side, candidate);
    });
    return side == UINT64_MAX ? Start(face, triangle, query, weights) : Interpolated(side, triangle, weights);
  }

  WalkResult Run(Point query, WalkStart start) const {
    if (start.facet == UINT64_MAX) {
      WalkResult missing;
      missing.query = query;
      missing.fallback = start.fallback;
      return missing;
    }
    auto best = Minimize(query, start.facet, start.u, start.v);
    std::vector<Id> visited{start.facet};
    size_t steps = 1, changes = 0, nonconverged = !best.converged;
    constexpr size_t maximum_changes = 64;
    while (changes < maximum_changes) {
      const auto& facet = facets.at(best.facet);
      const WalkWeights weights{1 - best.u - best.v, best.u, best.v};
      WalkResult next;
      next.distance = best.distance;
      auto consider = [&](Id candidate) {
        if (candidate == UINT64_MAX || std::find(visited.begin(), visited.end(), candidate) != visited.end()) return;
        visited.push_back(candidate);
        const auto& vertices = facets.at(candidate).vertices;
        WalkWeights mapped{};
        for (unsigned i = 0; i < 3; ++i)
          for (unsigned j = 0; j < 3; ++j)
            if (facet.vertices[i] == vertices[j]) mapped[j] = weights[i];
        auto hit = Minimize(query, candidate, mapped[1], mapped[2]);
        ++steps;
        nonconverged += !hit.converged;
        if (hit.distance < next.distance) next = hit;
      };

      const auto zero = std::count(weights.begin(), weights.end(), 0.);
      if (zero == 0) break;
      if (zero == 1) {
        const auto opposite = std::find(weights.begin(), weights.end(), 0.) - weights.begin();
        consider(facet.neighbors[opposite]);
      } else {
        const Id vertex = facet.vertices[std::max_element(weights.begin(), weights.end()) - weights.begin()];
        for (Id candidate : Fan(best.facet, vertex)) consider(candidate);
      }
      if (next.facet == UINT64_MAX) break;
      best = next;
      ++changes;
    }
    best.query = query;
    best.start_facet = start.facet;
    best.steps = steps;
    best.changes = changes;
    best.nonconverged = nonconverged;
    best.capped = changes == maximum_changes;
    best.converged = best.converged && !best.capped;
    best.fallback = start.fallback;
    return best;
  }

  SurfaceBinding Binding(const WalkResult& result) const {
    const auto& f = facets.at(result.facet);
    const auto u = result.u, v = result.v;
    auto edge = [&](unsigned a, unsigned b, double t) {
      return f.vertices[a] < f.vertices[b] ? geometry.EdgePoint({f.vertices[a], f.vertices[b]}, t)
                                           : geometry.EdgePoint({f.vertices[b], f.vertices[a]}, 1 - t);
    };
    if (u == 1) return geometry.Vertex(f.vertices[1]);
    if (v == 1) return geometry.Vertex(f.vertices[2]);
    if (u == 0 && v == 0) return geometry.Vertex(f.vertices[0]);
    if (v == 0) return edge(0, 1, u);
    if (u == 0) return edge(0, 2, v);
    if (1 - u - v == 0) return edge(1, 2, v);
    return geometry.FacetPoint(result.facet, u, v);
  }

  std::array<WalkResult, 4> Samples(const SurfaceFace& face, const Triangle& triangle) const {
    constexpr std::array<WalkWeights, 4> weights{
        {{1. / 3., 1. / 3., 1. / 3.}, {.5, .5, 0}, {0, .5, .5}, {.5, 0, .5}}};
    std::array<WalkResult, 4> results;
    for (size_t i = 0; i < weights.size(); ++i) {
      Point query;
      for (unsigned k = 0; k < 3; ++k) {
        query.x += weights[i][k] * triangle[k].node.p.x;
        query.y += weights[i][k] * triangle[k].node.p.y;
        query.z += weights[i][k] * triangle[k].node.p.z;
      }
      results[i] = Run(query, Start(face, triangle, query, weights[i]));
    }
    return results;
  }

  bool OnChain(const WalkResult& hit, Id chain) const {
    if (hit.facet == UINT64_MAX) return false;
    const auto binding = Binding(hit);
    const auto& features = state.Mobility().features;
    if (binding.dimension == 0) {
      bool match = false;
      geometry.ForFacets(binding, [&](Id side) {
        const auto& vertices = facets.at(side).vertices;
        for (Id vertex : vertices) {
          if (vertex == binding.feature) continue;
          Edge edge{std::min(vertex, binding.feature), std::max(vertex, binding.feature)};
          const auto found = features.find(edge);
          if (found != features.end() && found->second == chain) match = true;
        }
      });
      return match;
    }
    if (binding.dimension != 1) return false;
    const auto& f = facets.at(hit.facet);
    const WalkWeights weights{1 - hit.u - hit.v, hit.u, hit.v};
    const auto opposite = unsigned(std::find(weights.begin(), weights.end(), 0.) - weights.begin());
    Edge edge{f.vertices[(opposite + 1) % 3], f.vertices[(opposite + 2) % 3]};
    if (edge[0] > edge[1]) std::swap(edge[0], edge[1]);
    const auto found = features.find(edge);
    return found != features.end() && found->second == chain;
  }

 private:
  struct Stencil {
    WalkNet net;
    std::array<Id, 3> vertices;
    std::array<Id, 3> neighbors{UINT64_MAX, UINT64_MAX, UINT64_MAX};
    State::EntityKey entity;
  };

  WalkStart Interpolated(Id side, const Triangle& triangle, const WalkWeights& weights) const {
    WalkStart result{side};
    for (size_t k = 0; k < weights.size(); ++k) {
      if (weights[k] == 0) continue;
      const auto uv = geometry.SurfaceCoordinates(side, triangle[k].binding);
      result.u += weights[k] * uv.y;
      result.v += weights[k] * uv.z;
    }
    return result;
  }

  WalkResult Minimize(Point query, Id side, double u, double v) const {
    using namespace SU2NativeCurved::detail;
    const auto& original = facets.at(side).net;
    auto normalized = original;
    const auto target = NormalizeTriangleProjection(normalized, {query.x, query.y, query.z});
    ClampSimplex(u, v);
    const bool converged = MinimizeTriangle(
        target, u, v, [&](double a, double b) { return TrianglePatch(normalized, 1 - a - b, a, b); },
        [&](double a, double b, WalkWeights& first, WalkWeights& second) {
          TriangleDerivatives(normalized, a, b, first, second);
        });
    constexpr double tiny = 1e-13;
    if (u < tiny) u = 0;
    if (v < tiny) v = 0;
    if (1 - u - v < tiny) {
      const double sum = u + v;
      u /= sum;
      v = 1 - u;
    }
    const auto foot = TrianglePatch(original, 1 - u - v, u, v);
    WalkResult result;
    result.facet = side;
    result.u = u;
    result.v = v;
    result.foot = {foot[0], foot[1], foot[2]};
    result.distance = WalkDistance(query, result.foot);
    result.converged = converged;
    return result;
  }

  std::vector<Id> Fan(Id start, Id vertex) const {
    std::vector<Id> fan{start};
    for (size_t i = 0; i < fan.size(); ++i) {
      const auto& row = facets.at(fan[i]);
      for (unsigned k = 0; k < 3; ++k) {
        const auto next = row.neighbors[k];
        if (row.vertices[k] == vertex || next == UINT64_MAX) continue;
        if (std::find(fan.begin(), fan.end(), next) == fan.end()) fan.push_back(next);
      }
    }
    std::sort(fan.begin(), fan.end());
    return fan;
  }

  const State& state;
  const MappedFacetReference& geometry;
  std::map<Id, Stencil> facets;
};

struct WalkEntityMetrics {
  size_t faces = 0, samples = 0, above_half = 0, above_eighty = 0, above = 0;
  double sum_distance = 0, max_distance = 0, seconds = 0;
};

struct ProjectionMetrics {
  size_t samples = 0, successes = 0, failures = 0, trailing_edge = 0, tip = 0, distinct_feet = 0;
  double maximum_difference = 0, seconds = 0;
  std::string first_failure;
};

void MeasureProjection(const State& state) {
  const auto& geometry = state.Geometry();
  const double tolerance = state.GeometryTolerance();
  constexpr size_t comparison_samples = 10000;
  constexpr Id main_wing = 10154, trailing_strip = 27868, trailing_chain = 908;
  std::map<State::EntityKey, WalkEntityMetrics> entities;
  size_t wing_faces = 0;
  for (const auto& [key, face] : state.Surface()) {
    const auto entity = state.Entity(face.facet);
    ++entities[entity].faces;
    wing_faces += std::get<2>(entity) == 2;
  }
  const auto table_started = Clock::now();
  const Walker walker(state);
  const double table_seconds = Seconds(table_started);
  struct ComparisonSample {
    State::EntityKey entity;
    WalkResult hit;
  };
  std::vector<ComparisonSample> sample;
  sample.reserve(comparison_samples);
  const auto started = Clock::now();
  size_t ordinal = 0, steps = 0, max_steps = 0, changes = 0, max_changes = 0, nonconverged = 0;
  size_t capped = 0, fallback = 0, failed_walks = 0;
  for (const auto& [key, face] : state.Surface()) {
    const auto entity = state.Entity(face.facet);
    if (std::get<2>(entity) != 2) continue;
    const Triangle triangle{*state.Find(face.v[0]), *state.Find(face.v[1]), *state.Find(face.v[2])};
    auto& output = entities.at(entity);
    double max_face = 0;
    const auto face_started = Clock::now();
    const auto hits = walker.Samples(face, triangle);
    output.seconds += Seconds(face_started);
    for (const auto& hit : hits) {
      if (sample.size() < comparison_samples && ordinal == sample.size() * 4 * wing_faces / comparison_samples)
        sample.push_back({entity, hit});
      ++ordinal;
      ++output.samples;
      output.sum_distance += hit.distance;
      output.max_distance = std::max(output.max_distance, hit.distance);
      max_face = std::max(max_face, hit.distance);
      steps += hit.steps;
      max_steps = std::max(max_steps, hit.steps);
      changes += hit.changes;
      max_changes = std::max(max_changes, hit.changes);
      nonconverged += hit.nonconverged;
      capped += hit.capped;
      fallback += hit.fallback;
      failed_walks += !hit.converged;
    }
    output.above_half += max_face > .5 * tolerance;
    output.above_eighty += max_face > .8 * tolerance;
    output.above += max_face > tolerance;
  }
  const double seconds = Seconds(started);
  if (ordinal != 4 * wing_faces || sample.size() != comparison_samples)
    throw std::logic_error("FP2 G1 fixed-stride sample population is incomplete.");
  Output("G1");
  std::cout << "\"seconds\":" << seconds << ",\"neighbor_table_seconds\":" << table_seconds
            << ",\"samples\":" << ordinal << ",\"faces\":" << wing_faces
            << ",\"microseconds_per_sample\":" << seconds * 1e6 / ordinal
            << ",\"mean_steps\":" << double(steps) / ordinal << ",\"max_steps\":" << max_steps
            << ",\"mean_facet_changes\":" << double(changes) / ordinal << ",\"max_facet_changes\":" << max_changes
            << ",\"minimizer_nonconvergences\":" << nonconverged << ",\"failed_walks\":" << failed_walks
            << ",\"capped_walks\":" << capped << ",\"fallback_samples\":" << fallback << ",\"entities\":[";
  bool comma = false;
  for (const auto& [entity, metric] : entities) {
    const auto [body, sheet, marker] = entity;
    if (comma) std::cout << ',';
    comma = true;
    std::cout << "{\"body\":" << body << ",\"sheet\":" << sheet << ",\"marker\":" << marker
              << ",\"faces\":" << metric.faces << ",\"samples\":" << metric.samples;
    if (metric.samples) {
      std::cout << ",\"seconds\":" << metric.seconds
                << ",\"microseconds_per_sample\":" << metric.seconds * 1e6 / metric.samples
                << ",\"mean_distance\":";
      JsonNumber(metric.sum_distance / metric.samples);
      std::cout << ",\"max_distance\":";
      JsonNumber(metric.max_distance);
      std::cout << ",\"above_half_tolerance\":" << metric.above_half
                << ",\"above_eighty_percent_tolerance\":" << metric.above_eighty
                << ",\"above_tolerance\":" << metric.above;
    }
    std::cout << '}';
  }
  std::cout << "]}" << std::endl;

  const auto tip_span = geometry.Reference().Get(trailing_strip).v[0].p.y;
  const SU2NativeGeometry::NativeMappedGeometryBackend3D backend(state.Authority(), state.Mobility());
  const std::array<double, 2> errors{tolerance / 8, 1e-12};
  for (size_t pass = 0; pass < errors.size(); ++pass) {
    const auto start = Clock::now();
    auto budget = Fp2Budget(726442, state.ResidentBytes() + state.Authority()->ResidentBytes(), start, 5400);
    std::map<State::EntityKey, ProjectionMetrics> by_entity;
    ProjectionMetrics total;
    for (const auto& [entity, hit] : sample) {
      auto& stats = by_entity[entity];
      SU2NativeGeometry::GeometryProjection projected;
      const auto [body, sheet, marker] = entity;
      std::string reason = "No same-entity G1 start facet.";
      double duration = 0;
      bool success = false;
      if (hit.start_facet != UINT64_MAX) {
        const SU2NativeGeometry::GeometryProjectionScope scope{
            body, hit.start_facet, geometry.FacetPoint(hit.start_facet, .25, .25), sheet, std::make_pair(2u, sheet),
            errors[pass]};
        const auto before = Clock::now();
        success = backend.Project(hit.query, scope, projected, budget, reason);
        duration = Seconds(before);
      }
      const bool trailing = !success &&
          (sheet == trailing_strip || (sheet == main_wing && walker.OnChain(hit, trailing_chain)));
      const bool tip = !success && hit.query.y >= tip_span;
      const auto difference = success ? std::abs(WalkDistance(hit.query, projected.evaluation.point) - hit.distance) : 0;
      const bool distinct = success && WalkDistance(hit.foot, projected.evaluation.point) > 1e-9;
      for (auto* row : {&total, &stats}) {
        ++row->samples;
        row->seconds += duration;
        if (success) {
          ++row->successes;
          row->maximum_difference = std::max(row->maximum_difference, difference);
          row->distinct_feet += distinct;
        } else {
          ++row->failures;
          row->trailing_edge += trailing;
          row->tip += tip;
          if (row->first_failure.empty()) row->first_failure = reason;
        }
      }
      if (budget.work_exhausted || budget.time_exhausted) break;
    }
    auto write = [](const ProjectionMetrics& row) {
      std::cout << "\"seconds\":" << row.seconds << ",\"samples\":" << row.samples
                << ",\"microseconds_per_sample\":" << (row.samples ? row.seconds * 1e6 / row.samples : 0)
                << ",\"failures\":" << row.failures << ",\"first_failure\":";
      JsonString(row.first_failure);
      std::cout << ",\"trailing_edge_failures\":" << row.trailing_edge << ",\"tip_failures\":" << row.tip
                << ",\"successful_comparisons\":" << row.successes << ",\"max_distance_difference\":";
      JsonNumber(row.successes ? row.maximum_difference : std::numeric_limits<double>::quiet_NaN());
      std::cout << ",\"feet_over_1e_9\":" << row.distinct_feet;
    };
    Output("G2");
    std::cout << "\"pass\":" << pass + 1 << ",\"absolute_error\":" << errors[pass]
              << ",\"planned_samples\":" << comparison_samples
              << ",\"tip_span\":" << tip_span << ",\"trailing_edge_chain\":" << trailing_chain
              << ",\"work\":" << budget.work << ",\"work_exhausted\":" << (budget.work_exhausted ? "true" : "false")
              << ",\"time_exhausted\":" << (budget.time_exhausted ? "true" : "false") << ',';
    write(total);
    std::cout << ",\"entities\":[";
    comma = false;
    for (const auto& [entity, stats] : by_entity) {
      const auto [body, sheet, marker] = entity;
      if (comma) std::cout << ',';
      comma = true;
      std::cout << "{\"body\":" << body << ",\"sheet\":" << sheet << ",\"marker\":" << marker << ',';
      write(stats);
      std::cout << '}';
    }
    std::cout << "]}" << std::endl;
  }
}
