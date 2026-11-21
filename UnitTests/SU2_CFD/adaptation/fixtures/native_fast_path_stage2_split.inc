// FP2-0b surface-only edits. Production sees the candidate only through the strict measurement calls.
constexpr double SplitTolerance = 0.7 * Tol;
constexpr size_t SplitPassLimit = 40;
using SU2NativeGeometry::AtlasBudget;
using SU2NativeGeometry::BoundaryCoverage;
using SU2NativeGeometry::EdgeTrace;

Edge EdgeOf(Id a, Id b) { return {std::min(a, b), std::max(a, b)}; }

std::array<Edge, 3> FaceEdges(const SurfaceFace& face) {
  const auto& v = face.v;
  return {EdgeOf(v[0], v[1]), EdgeOf(v[1], v[2]), EdgeOf(v[2], v[0])};
}

Point Midpoint(Point a, Point b) { return {0.5 * (a.x + b.x), 0.5 * (a.y + b.y), 0.5 * (a.z + b.z)}; }

double MetricLength(const MetricPoint& a, const MetricPoint& b) {
  const auto p = a.point.node.p, q = b.point.node.p;
  const Point d{q.x - p.x, q.y - p.y, q.z - p.z};
  const auto norm = [&](const Tensor& m) {
    return std::sqrt(d.x * (m.xx * d.x + m.xy * d.y + m.xz * d.z) +
                     d.y * (m.xy * d.x + m.yy * d.y + m.yz * d.z) +
                     d.z * (m.xz * d.x + m.yz * d.y + m.zz * d.z));
  };
  const double first = norm(a.tensor), last = norm(b.tensor);
  const double larger = std::max(first, last), ratio = std::min(first, last) / larger;
  return larger * (2.0 / 3) * (1 + ratio + ratio * ratio) / (1 + ratio);
}

struct Calls {
  size_t count = 0;
  double seconds = 0;

  void Record(Clock::time_point start) {
    ++count;
    seconds += Seconds(start);
  }

  void Add(const Calls& other) {
    count += other.count;
    seconds += other.seconds;
  }

  void Print(const char* name) const {
    std::cout << ",\"" << name << "_calls\":" << count << ",\"" << name << "_seconds\":" << seconds
              << ",\"" << name << "_us_per_call\":";
    JsonNumber(count ? 1e6 * seconds / count : std::numeric_limits<double>::quiet_NaN());
  }
};

struct SplitStats {
  size_t scans = 0, passes = 0, marked = 0, walk_splits = 0, feature_splits = 0, blocked = 0;
  size_t dominated = 0, dominated_new = 0, fold_all = 0, all_feature = 0;
  size_t samples = 0, unconverged_samples = 0, capped_samples = 0;
  Calls propose, rebuild, strict;
  std::map<std::string, size_t> refusals;
  std::map<std::string, std::string> first_error;

  void Refuse(const char* kind, const std::string& reason = {}) {
    ++refusals[kind];
    if (!reason.empty()) first_error.try_emplace(kind, reason);
  }

  void Add(const SplitStats& pass) {
    scans += pass.scans;
    passes += pass.passes;
    marked += pass.marked;
    walk_splits += pass.walk_splits;
    feature_splits += pass.feature_splits;
    blocked += pass.blocked;
    dominated += pass.dominated;
    dominated_new += pass.dominated_new;
    fold_all += pass.fold_all;
    all_feature += pass.all_feature;
    samples += pass.samples;
    unconverged_samples += pass.unconverged_samples;
    capped_samples += pass.capped_samples;
    propose.Add(pass.propose);
    rebuild.Add(pass.rebuild);
    strict.Add(pass.strict);
    for (const auto& [kind, count] : pass.refusals) refusals[kind] += count;
    for (const auto& [kind, reason] : pass.first_error) first_error.try_emplace(kind, reason);
  }
};

void ReportSplit(const char* row, const char* step, size_t pass, const SplitStats& stats, double seconds) {
  Output(row);
  std::cout << "\"step\":\"" << step << "\",\"pass\":" << pass << ",\"seconds\":" << seconds
            << ",\"scans\":" << stats.scans << ",\"passes\":" << stats.passes << ",\"marked\":" << stats.marked
            << ",\"walk_splits\":" << stats.walk_splits << ",\"feature_splits\":" << stats.feature_splits
            << ",\"blocked_by_pass_conflict\":" << stats.blocked << ",\"feature_dominated\":" << stats.dominated
            << ",\"unique_feature_dominated\":" << stats.dominated_new << ",\"fold_all\":" << stats.fold_all
            << ",\"all_feature_edges\":" << stats.all_feature << ",\"sample_calls\":" << stats.samples
            << ",\"sample_unconverged\":" << stats.unconverged_samples << ",\"sample_capped\":" << stats.capped_samples;
  stats.propose.Print("propose");
  stats.rebuild.Print("rebuild");
  stats.strict.Print("strict_children");
  std::cout << ",\"refusals\":{";
  bool first = true;
  for (const auto& [kind, count] : stats.refusals) {
    if (!first) std::cout << ',';
    first = false;
    JsonString(kind);
    std::cout << ':' << count;
  }
  std::cout << "},\"first_errors\":{";
  first = true;
  for (const auto& [kind, reason] : stats.first_error) {
    if (!first) std::cout << ',';
    first = false;
    JsonString(kind);
    std::cout << ':';
    JsonString(reason);
  }
  std::cout << "}}" << std::endl;
}

enum class LeftReason { Cap, Dominated, FoldAll, AllFeature, Refused, Blocked };

const char* ReasonName(LeftReason reason) {
  switch (reason) {
    case LeftReason::Cap: return "cap";
    case LeftReason::Dominated: return "feature_dominated";
    case LeftReason::FoldAll: return "fold_all";
    case LeftReason::AllFeature: return "all_feature_edges";
    case LeftReason::Refused: return "refused";
    case LeftReason::Blocked: return "blocked";
  }
  throw std::logic_error("Unknown split reason.");
}

struct PrototypeFace {
  FaceMeasurement measurement;
  std::array<double, 4> samples{};
  LeftReason last = LeftReason::Cap;
  bool dominated = false;
};

PrototypeFace NewFace(SurfaceFace face, bool original) {
  PrototypeFace result{};
  result.measurement.face = face;
  result.measurement.original = original;
  result.measurement.sample = -1;
  return result;
}

bool PassesFold(const State& state, Id facet, const Triangle& triangle) {
  const Point normal = FlatNormal(triangle);
  if (normal.x == 0 && normal.y == 0 && normal.z == 0) return false;
  const auto entity = state.Entity(facet);
  for (const auto& corner : triangle) {
    bool passed = false;
    state.Geometry().ForFacets(corner.binding, [&](Id side) {
      if (passed || state.Entity(side) != entity) return;
      try { passed = ChildNormalDot(state, side, corner, normal) > 0.1; }
      catch (const std::exception&) { }
    });
    if (!passed) return false;
  }
  return true;
}

class Prototype {
 public:
  Prototype(const PlanarReference& reference, std::vector<MetricPoint> input, AtlasBudget& budget, bool strict,
            local::StateSurfaceGeometry* surface = nullptr)
      : reference(reference), state(*reference.Parameters()), budget(budget), strict_children(strict), surface(surface),
        points(std::move(input)) {
    if (!surface) {
      walker.emplace(state);
      traces = state.Coverage().traces;
    }
    for (size_t i = 0; i < points.size(); ++i) {
      const auto& point = points[i].point;
      point_ids.emplace(point.node.id, i);
      if (!surface) {
        coordinates.insert(Coordinate(point.node.p));
        bindings.insert(Support(point.binding));
      }
      next_id = std::max(next_id, point.node.id + 1);
    }
    faces.reserve(state.Surface().size());
    for (const auto& row : state.Surface()) faces.push_back(NewFace(row.second, true));
  }

  std::vector<FaceMeasurement> Measurements() const {
    std::vector<FaceMeasurement> result;
    result.reserve(faces.size());
    for (const auto& row : faces) result.push_back(row.measurement);
    return result;
  }

  BoundaryPoints Records() const {
    BoundaryPoints result;
    for (const auto& point : points) result.emplace(point.point.node.id, point.point);
    return result;
  }

  BoundaryCoverage Coverage() const { return {traces, {}}; }

  bool Exhausted() const { return budget.work_exhausted || budget.time_exhausted; }

  void CheckInputFeatures() const {
    const auto edges = Edges();
    size_t old_features = 0, new_features = 0, differences = 0;
    for (const auto& [edge, incident] : edges) {
      (void)incident;
      const auto& a = At(edge[0]).point.binding;
      const auto& b = At(edge[1]).point.binding;
      const bool old = a.dimension == 0 && b.dimension == 0 && state.Mobility().features.count(EdgeOf(a.feature, b.feature));
      const bool current = Feature(edge);
      old_features += old;
      new_features += current;
      differences += old != current;
    }
    Output("P_feature_start");
    std::cout << "\"edges\":" << edges.size() << ",\"traces\":" << traces.size() << ",\"old_features\":" << old_features
              << ",\"new_features\":" << new_features << ",\"differences\":" << differences
              << ",\"strict_children\":" << (strict_children ? "true" : "false") << '}' << std::endl;
  }

  void SampleFaces(SplitStats& stats) {
    for (auto& row : faces) {
      if (row.measurement.sample >= 0) continue;
      const auto& face = row.measurement.face;
      if (surface) {
        const auto samples = surface->Samples(face);
        row.samples = samples.distances;
        stats.unconverged_samples += samples.unconverged;
      } else {
        const auto samples = walker->Samples(face, Corners(face));
        for (size_t i = 0; i < samples.size(); ++i) {
          row.samples[i] = samples[i].distance;
          stats.unconverged_samples += !samples[i].converged;
          stats.capped_samples += samples[i].capped;
        }
      }
      if (!std::all_of(row.samples.begin(), row.samples.end(), [](double value) { return std::isfinite(value); }))
        throw std::runtime_error("Float walk produced a nonfinite sample.");
      stats.samples += row.samples.size();
      row.measurement.sample = *std::max_element(row.samples.begin(), row.samples.end());
    }
  }

  void Run(bool distance) {
    const char* step = distance ? "D" : "L";
    const auto start = Clock::now();
    SplitStats totals;
    for (size_t pass = 1; pass <= SplitPassLimit && !Exhausted(); ++pass) {
      const auto pass_start = Clock::now();
      auto edges = Edges();
      SplitStats stats;
      stats.scans = 1;
      std::map<Edge, Candidate> marks;
      std::vector<std::optional<Edge>> choices;
      if (distance) {
        SampleFaces(stats);
        MarkFeatures(edges, marks, stats);
        if (!Exhausted()) choices = MarkFaces(marks, stats);
      } else {
        for (const auto& [edge, incident] : edges) {
          (void)incident;
          if (refused.count(edge)) continue;
          const double length = MetricLength(At(edge[0]), At(edge[1]));
          if (length > 1.8) marks.emplace(edge, Candidate{length});
        }
      }
      stats.marked = marks.size();
      stats.passes = !marks.empty() && !Exhausted();
      if (stats.passes) ApplyMarks(edges, marks, stats);
      if (distance)
        for (size_t i = 0; i < choices.size(); ++i)
          if (choices[i] && faces[i].measurement.sample > SplitTolerance) faces[i].last = marks.at(*choices[i]).outcome;
      ReportSplit("P_PASS", step, pass, stats, Seconds(pass_start));
      totals.Add(stats);
      if (!stats.passes) break;
    }
    if (distance && !Exhausted()) SampleFaces(totals);
    ReportSplit(distance ? "P_D" : "P_L", step, 0, totals, Seconds(start));
  }

  std::vector<SurfaceFace> SurfaceFaces() const {
    std::vector<SurfaceFace> result;
    result.reserve(faces.size());
    for (const auto& row : faces) result.push_back(row.measurement.face);
    return result;
  }

  void ReportEntities(const BoundaryPoints& in_points, const std::vector<FaceMeasurement>& in_faces) const {
    std::map<State::EntityKey, EntityCounts> before, after;
    std::map<State::EntityKey, std::map<LeftReason, size_t>> left;
    for (const auto& row : in_faces) CountFace(before, state, row.face);
    for (const auto& row : faces) {
      const auto& face = row.measurement.face;
      CountFace(after, state, face);
      if (row.measurement.sample > SplitTolerance) ++left[state.Entity(face.facet)][row.last];
    }
    Output("P_entities");
    std::cout << "\"IN1_faces\":" << in_faces.size() << ",\"IN1_points\":" << in_points.size()
              << ",\"PD_faces\":" << faces.size() << ",\"PD_points\":" << points.size()
              << ",\"traces\":" << traces.size() << ",\"IN1\":";
    EntityCountsJson(before);
    std::cout << ",\"PD\":";
    EntityCountsJson(after);
    std::cout << ",\"PD_above_0p7_tol\":[";
    bool first = true;
    for (const auto& [entity, count] : after) {
      (void)count;
      if (!first) std::cout << ',';
      first = false;
      const auto& reasons = left[entity];
      size_t total = 0;
      for (const auto& [reason, count] : reasons) total += count;
      std::cout << "{\"entity\":";
      EntityJson(entity);
      std::cout << ",\"faces\":" << total << ",\"reasons\":{";
      bool first_reason = true;
      for (const auto& [reason, count] : reasons) {
        if (!first_reason) std::cout << ',';
        first_reason = false;
        JsonString(ReasonName(reason));
        std::cout << ':' << count;
      }
      std::cout << "}}";
    }
    std::cout << "]}" << std::endl;
  }

  void Save() const {
    std::ofstream point_file("points.jsonl"), face_file("faces.jsonl");
    point_file.exceptions(std::ios::badbit | std::ios::failbit);
    face_file.exceptions(std::ios::badbit | std::ios::failbit);
    point_file << std::setprecision(17);
    face_file << std::setprecision(17);
    for (const auto& row : points) {
      const auto& node = row.point.node;
      const auto& b = row.point.binding;
      point_file << "{\"id\":" << node.id << ",\"coordinates\":[" << node.p.x << ',' << node.p.y << ',' << node.p.z
                 << "],\"binding\":{\"dimension\":" << b.dimension << ",\"feature\":" << b.feature
                 << ",\"version\":" << b.version << ",\"u\":" << b.u << ",\"v\":" << b.v << "}}\n";
    }
    for (const auto& row : faces) {
      const auto& face = row.measurement.face;
      const auto [body, sheet, marker] = state.Entity(face.facet);
      face_file << '{';
      WriteFaceIds(face_file, face);
      face_file << ",\"entity\":[" << body << ',' << sheet << ',' << marker << "],\"samples\":["
                << row.samples[0] << ',' << row.samples[1] << ',' << row.samples[2] << ',' << row.samples[3]
                << "],\"sampled\":" << (row.measurement.sample >= 0 ? "true" : "false") << "}\n";
    }
    point_file.close();
    face_file.close();
  }

 private:
  using Incident = std::array<size_t, 2>;
  using Binding = std::tuple<unsigned, Id, double, double>;

  struct Candidate {
    double length;
    LeftReason outcome = LeftReason::Cap;
  };

  static std::array<double, 3> Coordinate(Point p) { return {p.x, p.y, p.z}; }
  static Binding Support(const SurfaceBinding& b) { return {b.dimension, b.feature, b.u, b.v}; }

  const MetricPoint& At(Id id) const { return points.at(point_ids.at(id)); }

  Triangle Corners(const SurfaceFace& face) const {
    return {At(face.v[0]).point, At(face.v[1]).point, At(face.v[2]).point};
  }

  std::map<Edge, Incident> Edges() const {
    std::map<Edge, Incident> result;
    for (size_t i = 0; i < faces.size(); ++i) {
      for (const auto& edge : FaceEdges(faces[i].measurement.face)) {
        auto [entry, inserted] = result.emplace(edge, Incident{i, SIZE_MAX});
        if (inserted) continue;
        if (entry->second[1] != SIZE_MAX) throw std::invalid_argument("Prototype edge has more than two faces.");
        entry->second[1] = i;
      }
    }
    for (const auto& entry : result)
      if (entry.second[1] == SIZE_MAX) throw std::invalid_argument("Prototype surface has an open edge.");
    return result;
  }

  bool Feature(const Edge& edge) const { return surface ? surface->Feature(edge) : traces.count(edge) != 0; }

  double Length(const Edge& edge) const { return WalkDistance(At(edge[0]).point.node.p, At(edge[1]).point.node.p); }

  bool Propose(const Edge& edge, Point query, BoundaryPoint& output, std::string& reason, SplitStats& stats) {
    const auto start = Clock::now();
    bool passed = false;
    try {
      passed = SU2NativeGeometry::ProposeFeaturePoint(state.Geometry(), state.Mobility(), {traces.at(edge)},
          At(edge[0]).point.node.p, At(edge[1]).point.node.p, {next_id, query}, output, budget, reason);
    } catch (const std::exception& error) { reason = error.what(); }
    stats.propose.Record(start);
    return passed;
  }

  void MarkFeatures(const std::map<Edge, Incident>& edges, std::map<Edge, Candidate>& marks, SplitStats& stats) {
    for (const auto& [edge, incident] : edges) {
      (void)incident;
      if (!Feature(edge)) continue;
      if (refused.count(edge)) continue;
      auto [where, inserted] = chords.try_emplace(edge);
      if (inserted) {
        if (surface) {
          const auto start = Clock::now();
          where->second = surface->Chord({At(edge[0]).point.node, At(edge[1]).point.node});
          stats.propose.Record(start);
          if (!where->second) stats.Refuse("feature_chord_unknown");
        } else {
          const Point query = Midpoint(At(edge[0]).point.node.p, At(edge[1]).point.node.p);
          BoundaryPoint point;
          std::string reason;
          if (Propose(edge, query, point, reason, stats))
            where->second = WalkDistance(query, point.node.p);
          else
            stats.Refuse("feature_chord_unknown", reason);
        }
      }
      if (Exhausted()) break;
      if (where->second && *where->second > SplitTolerance) marks.emplace(edge, Candidate{Length(edge)});
    }
  }

  std::vector<std::optional<Edge>> MarkFaces(std::map<Edge, Candidate>& marks, SplitStats& stats) {
    std::vector<std::optional<Edge>> choices(faces.size());
    for (size_t i = 0; i < faces.size(); ++i) {
      auto& row = faces[i];
      if (row.measurement.sample <= SplitTolerance) continue;
      const auto edges = FaceEdges(row.measurement.face);
      auto& choice = choices[i];
      for (const auto& edge : edges)
        if (Feature(edge) && marks.count(edge)) { choice = edge; break; }
      if (choice) continue;

      const auto excessive = std::count_if(row.samples.begin(), row.samples.end(),
                                           [](double sample) { return sample > SplitTolerance; });
      if (excessive == 1 && row.samples[0] <= SplitTolerance) {
        const auto midpoint = std::max_element(row.samples.begin() + 1, row.samples.end()) - row.samples.begin();
        if (Feature(edges[midpoint - 1])) {
          row.last = LeftReason::Dominated;
          ++stats.dominated;
          if (!row.dominated) ++stats.dominated_new;
          row.dominated = true;
          continue;
        }
      }

      bool has_nonfeature = false;
      double longest = 0;
      for (const auto& edge : edges) {
        if (Feature(edge)) continue;
        has_nonfeature = true;
        if (refused.count(edge)) continue;
        const auto length = Length(edge);
        if (!choice || length > longest || (length == longest && edge < *choice)) {
          choice = edge;
          longest = length;
        }
      }
      if (choice)
        marks.emplace(*choice, Candidate{longest});
      else if (has_nonfeature) {
        row.last = LeftReason::FoldAll;
        ++stats.fold_all;
      } else {
        row.last = LeftReason::AllFeature;
        ++stats.all_feature;
      }
    }
    return choices;
  }

  void ApplyMarks(const std::map<Edge, Incident>& edges, std::map<Edge, Candidate>& marks, SplitStats& stats) {
    std::vector<Edge> ordered;
    ordered.reserve(marks.size());
    for (const auto& entry : marks) ordered.push_back(entry.first);
    std::sort(ordered.begin(), ordered.end(), [&](const Edge& a, const Edge& b) {
      const double left = marks.at(a).length, right = marks.at(b).length;
      return left == right ? a < b : left > right;
    });
    std::vector<bool> changed(faces.size());
    for (const auto& edge : ordered) {
      if (Exhausted()) break;
      const auto incident = edges.at(edge);
      if (changed[incident[0]] || changed[incident[1]]) {
        marks.at(edge).outcome = LeftReason::Blocked;
        ++stats.blocked;
        continue;
      }
      const bool feature = Feature(edge);
      if (Split(edge, incident, feature, stats)) {
        changed[incident[0]] = changed[incident[1]] = true;
        if (feature) ++stats.feature_splits;
        else ++stats.walk_splits;
      } else {
        refused.insert(edge);
        marks.at(edge).outcome = LeftReason::Refused;
      }
    }
  }

  bool WalkPoint(const Edge& edge, const SurfaceFace& first_face, Point query, BoundaryPoint& point, SplitStats& stats) {
    const auto corners = Corners(first_face);
    unsigned first = 0, second = 0;
    for (unsigned k = 0; k < 3; ++k) {
      if (first_face.v[k] == edge[0]) first = k;
      if (first_face.v[k] == edge[1]) second = k;
    }
    try {
      const auto closest = walker->Run(query, walker->EdgeStart(first_face, corners, first, second, query));
      if (!closest.converged || closest.capped) {
        stats.Refuse(closest.capped ? "walk_cap" : "walk_nonconvergence");
        return false;
      }
      point.binding = walker->Binding(closest);
      if (state.Geometry().MobilityDimension(point.binding, state.Mobility()) != 2) {
        stats.Refuse("protected_binding");
        return false;
      }
      point.node = {next_id, state.Geometry().Realize(point.binding)};
      return true;
    } catch (const std::exception& error) {
      stats.Refuse("projection_error", error.what());
      return false;
    }
  }

  std::array<SurfaceFace, 4> Children(const Edge& edge, const Incident& incident) const {
    std::array<SurfaceFace, 4> children;
    for (size_t i = 0; i < incident.size(); ++i) {
      const auto& parent = faces[incident[i]].measurement.face;
      const auto edges = FaceEdges(parent);
      const auto k = std::find(edges.begin(), edges.end(), edge) - edges.begin();
      if (k == 3) throw std::logic_error("Marked edge is absent from its face.");
      const Id left = parent.v[k], right = parent.v[(k + 1) % 3], other = parent.v[(k + 2) % 3];
      children[2 * i] = {{left, next_id, other}, parent.facet};
      children[2 * i + 1] = {{next_id, right, other}, parent.facet};
    }
    return children;
  }

  bool Rebuild(const Edge& edge, const Incident& incident, const std::array<SurfaceFace, 4>& children,
               const BoundaryPoint& point, std::map<Edge, EdgeTrace>& result, SplitStats& stats) {
    Patch old;
    for (size_t index : incident) {
      old.surface.push_back(faces[index].measurement.face);
      for (Id id : old.surface.back().v) old.parameters.emplace(id, At(id).point);
    }
    old.coverage.traces.emplace(edge, traces.at(edge));
    auto updated = old.parameters;
    updated.emplace(next_id, point);
    const std::vector<SurfaceFace> replacement(children.begin(), children.end());
    const auto start = Clock::now();
    std::string reason;
    bool passed = false;
    try {
      passed = SU2NativeGeometry::RebuildFeatureTraces(old, replacement, updated, reference,
                                                     result, budget, reason);
    } catch (const std::exception& error) { reason = error.what(); }
    stats.rebuild.Record(start);
    if (passed && result.count(EdgeOf(edge[0], next_id)) && result.count(EdgeOf(edge[1], next_id))) return true;
    if (passed) reason = "Production trace reconstruction omitted a child edge.";
    stats.Refuse("trace_refused", reason);
    return false;
  }

  bool SplitWithProductionGeometry(const Edge& edge, const Incident& incident, SplitStats& stats) {
    const std::array<SurfaceFace, 2> parents{faces[incident[0]].measurement.face, faces[incident[1]].measurement.face};
    const auto start = Clock::now();
    auto placement = surface->Place({At(edge[0]).point.node, At(edge[1]).point.node}, parents, next_id);
    stats.propose.Record(start);
    if (!placement.Accepted()) {
      stats.Refuse("production_placement", placement.reason);
      return false;
    }
    const auto children = Children(edge, incident);
    for (const auto& child : children) {
      const auto check = Clock::now();
      const bool valid = surface->OnEntity(child, placement.point);
      stats.strict.Record(check);
      if (!valid) {
        stats.Refuse("strict_entity");
        return false;
      }
    }
    const auto point = placement.point;
    surface->Accept(std::move(placement));
    CommitPoint(edge, incident, children, point);
    chords.erase(edge);
    return true;
  }

  bool Split(const Edge& edge, const Incident& incident, bool feature, SplitStats& stats) {
    if (surface) return SplitWithProductionGeometry(edge, incident, stats);
    const auto& a = At(edge[0]);
    const auto& b = At(edge[1]);
    const auto query = Midpoint(a.point.node.p, b.point.node.p);
    BoundaryPoint point;
    if (feature) {
      std::string reason;
      if (!Propose(edge, query, point, reason, stats)) {
        stats.Refuse("feature_refused", reason);
        return false;
      }
      try {
        if (state.Geometry().MobilityDimension(point.binding, state.Mobility()) != 1) {
          stats.Refuse("feature_refused", "Proposed feature binding did not have physical dimension 1.");
          return false;
        }
      } catch (const std::exception& error) {
        stats.Refuse("feature_refused", error.what());
        return false;
      }
    } else if (!WalkPoint(edge, faces[incident[0]].measurement.face, query, point, stats)) {
      return false;
    }

    const auto support = Support(point.binding);
    const auto coord = Coordinate(point.node.p);
    if (bindings.count(support)) {
      stats.Refuse(feature ? "feature_refused" : "duplicate_binding", "Candidate binding already exists.");
      return false;
    }
    if (coordinates.count(coord)) {
      stats.Refuse(feature ? "feature_refused" : "duplicate_coordinates", "Candidate coordinates already exist.");
      return false;
    }

    const auto children = Children(edge, incident);
    std::array<Triangle, 4> triangles;
    for (size_t i = 0; i < children.size(); ++i) {
      for (size_t k = 0; k < 3; ++k) {
        const Id id = children[i].v[k];
        triangles[i][k] = id == next_id ? point : At(id).point;
      }
      if (!PassesFold(state, children[i].facet, triangles[i])) {
        stats.Refuse("fold");
        return false;
      }
    }
    std::map<Edge, EdgeTrace> rebuilt;
    if (feature && !Rebuild(edge, incident, children, point, rebuilt, stats)) return false;
    if (strict_children) {
      for (size_t i = 0; i < children.size(); ++i) {
        const auto start = Clock::now();
        bool passed = false;
        std::string reason;
        try { passed = state.ContainsEntityFace(children[i].facet, triangles[i]); }
        catch (const std::exception& error) { reason = error.what(); }
        stats.strict.Record(start);
        if (!passed) {
          stats.Refuse("strict_entity", reason);
          return false;
        }
      }
    }

    bindings.insert(support);
    coordinates.insert(coord);
    CommitPoint(edge, incident, children, point);
    if (feature) {
      traces.erase(edge);
      chords.erase(edge);
      traces.insert(rebuilt.extract(EdgeOf(edge[0], point.node.id)));
      traces.insert(rebuilt.extract(EdgeOf(edge[1], point.node.id)));
    }
    return true;
  }

  void CommitPoint(const Edge& edge, const Incident& incident, const std::array<SurfaceFace, 4>& children,
                   const BoundaryPoint& point) {
    const auto& a = At(edge[0]).tensor;
    const auto& b = At(edge[1]).tensor;
    const Tensor m{0.5 * (a.xx + b.xx), 0.5 * (a.xy + b.xy), 0.5 * (a.xz + b.xz),
                   0.5 * (a.yy + b.yy), 0.5 * (a.yz + b.yz), 0.5 * (a.zz + b.zz)};
    point_ids.emplace(next_id, points.size());
    points.push_back({point, m});
    for (size_t i = 0; i < incident.size(); ++i) {
      faces[incident[i]] = NewFace(children[2 * i], false);
      faces.push_back(NewFace(children[2 * i + 1], false));
    }
    ++next_id;
  }

  const PlanarReference& reference;
  const State& state;
  std::optional<Walker> walker;
  AtlasBudget& budget;
  bool strict_children;
  local::StateSurfaceGeometry* surface;
  std::vector<MetricPoint> points;
  std::map<Edge, EdgeTrace> traces;
  std::map<Edge, std::optional<double>> chords;
  std::set<Edge> refused;
  std::unordered_map<Id, size_t> point_ids;
  std::set<std::array<double, 3>> coordinates;
  std::set<Binding> bindings;
  std::vector<PrototypeFace> faces;
  Id next_id = 0;
};

void MeasurePrototype(ReferenceState& reference) {
  PrepareInput(reference);
  const auto& state = *reference.original->Parameters();
  auto points = ReadSurfaceSensor(state);
  const auto started = Clock::now();
  auto budget = Fp2Budget(726442, state.ResidentBytes() + state.Geometry().ResidentBytes(), started, 5400);
  const auto* mode = std::getenv("SU2_FP2_STRICT_CHILDREN");
  const bool strict = mode && std::string(mode) == "1";
  Prototype mesh(*reference.original, std::move(points), budget, strict);
  mesh.CheckInputFeatures();
  const auto in_points = mesh.Records();
  const auto initial_start = Clock::now();
  SplitStats initial;
  mesh.SampleFaces(initial);
  std::cout << std::setprecision(17) << "FP2_0 P_initial_samples {\"seconds\":" << Seconds(initial_start)
            << ",\"samples\":" << initial.samples << ",\"unconverged\":" << initial.unconverged_samples
            << ",\"capped\":" << initial.capped_samples << '}' << std::endl;
  auto in_faces = mesh.Measurements();
  mesh.Run(false);
  if (!mesh.Exhausted()) mesh.Run(true);
  mesh.ReportEntities(in_points, in_faces);
  mesh.Save();
  if (mesh.Exhausted()) {
    Output("P_budget_stop");
    std::cout << "\"seconds\":" << Seconds(started) << ",\"error\":\""
              << (budget.time_exhausted ? "time_exhausted" : "work_exhausted")
              << "\",\"work\":" << budget.work << '}' << std::endl;
    return;
  }
  const auto final_points = mesh.Records();
  auto final_faces = mesh.Measurements();
  MeasureStrict(state, in_points, in_faces, final_points, final_faces, !strict);
  MeasureProof(state, final_points, final_faces, mesh.Coverage());
}

void CheckProductionSurface(ReferenceState& reference) {
  PrepareInput(reference);
  const auto& state = *reference.original->Parameters();
  auto points = ReadSurfaceSensor(state);
  const auto caller = reference.original->ResidentBytes();
  auto budget = Fp2Budget(726442, caller, Clock::now(), 5400);
  local::StateSurfaceGeometry surface(reference.original, budget);
  Prototype mesh(*reference.original, std::move(points), budget, true, &surface);
  mesh.Run(false);
  mesh.Run(true);
  const auto faces = mesh.SurfaceFaces();
  const auto edits = surface.GatherEdits(faces);
  const auto result = local::BuildSurfaceState(surface, edits, Ceiling, caller);
  REQUIRE(result.state);
  REQUIRE(result.state->Revision() == state.Revision() + 1);
  CHECK(result.state->Surface().size() == faces.size());
  const auto points_out = result.state->Records().size();
  REQUIRE(points_out > state.Records().size());
  const auto splits = points_out - state.Records().size();
  CHECK(faces.size() == state.Surface().size() + 2 * splits);
  std::cout << "FP2_A IN1_STATE faces=" << faces.size() << " points=" << points_out
            << " traces=" << result.state->Coverage().traces.size() << " revision=" << result.state->Revision()
            << " splits=" << splits << " seconds_build=" << result.seconds_build
            << " modeled_peak=" << result.modeled_peak << std::endl;
}
