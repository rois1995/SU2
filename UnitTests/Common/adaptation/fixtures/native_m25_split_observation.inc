// N13/M25: saved492 measurement only; all geometry calls retain production arithmetic.
#include "../../../../Common/include/adaptation/CTransferMemory.hpp"
namespace {
auto M25PointKey(Point p) { return std::array<double, 3>{p.x, p.y, p.z}; }
struct M25Input {
  std::array<std::string, 9> raw;
  SU2Native3D::Edge edge{};
  std::string verdict;
  unsigned group = 0;
};
struct M25Row {
  M25Input input;
  std::string status = "not_attempted_time_limit";
  SU2Native3D::GeometrySplitPrediction prediction;
  double seconds = 0, length = 0, distance = 0;
  size_t work = 0;
  SU2Native3D::Point midpoint{}, bound{};
  bool selfcheck = false;
  std::array<const SU2Native3D::SurfaceFace*, 2> faces{};
  std::array<double, 2> parent{};
  std::array<std::array<double, 2>, 2> child{};
  bool point_valid = false, midpoint_valid = false, distance_valid = false, length_valid = false;
  std::array<bool, 2> parent_valid{};
  std::array<std::array<bool, 2>, 2> child_valid{};
  bool law_rows_valid = false;
  bool supplemental_bounds = false;
  std::string bounds_error;
  std::array<std::array<SU2Native3D::WallEdgeEstimate, 3>, 2> law{};
  std::array<std::array<SU2Native3D::Edge, 3>, 2> law_edges{};
};
std::vector<M25Input> M25Read(const char* path) {
  std::ifstream in(path);
  if (!in) throw std::runtime_error("cannot open SU2_M25_EDGES");
  std::string line;
  std::getline(in, line);
  if (!line.empty() && line.back() == '\r') line.pop_back();
  if (line !=
      "edge_a,edge_b,law_pick,n11_first_verdict,failing_faces,limiting_faces,limiting_failing_faces,need,estimate")
    throw std::runtime_error("M25 input header mismatch");
  std::vector<M25Input> rows;
  while (std::getline(in, line)) {
    if (!line.empty() && line.back() == '\r') line.pop_back();
    std::stringstream s(line);
    M25Input r;
    for (auto& x : r.raw)
      if (!std::getline(s, x, ',')) throw std::runtime_error("short M25 row");
    std::string extra;
    if (std::getline(s, extra, ',')) throw std::runtime_error("long M25 row");
    r.edge = {std::stoull(r.raw[0]), std::stoull(r.raw[1])};
    if (r.edge[0] >= r.edge[1]) throw std::runtime_error("noncanonical M25 edge");
    r.verdict = r.raw[3];
    r.group = unsigned(std::stoul(r.raw[6]));
    rows.push_back(std::move(r));
  }
  if (rows.size() != 2408) throw std::runtime_error("M25 input row count mismatch");
  return rows;
}
AtlasBudget M25Budget(size_t held, const std::function<bool()>& within) {
  auto b = GeometryAtlasBudget();
  b.maximum_work = D9WorkLimit;
  b.maximum_bytes = D9WorkspaceLimit;
  b.maximum_integer_bits = D9IntegerLimit;
  b.caller_bytes = held;
  b.within_time = within;
  return b;
}
#pragma GCC push_options
#pragma GCC optimize("no-fast-math", "no-unsafe-math-optimizations", "fp-contract=off")
__attribute__((noinline)) bool M25Score(const PlanarReference& c, Id facet, const std::array<Point, 3>& tri, double tau,
                                        long double& sum, size_t& count, double& bound) {
  if (!c.ContainsFace(facet, tri)) return false;
  bound = c.SurfaceDeviation(facet, tri, SurfaceQueryNeed::Magnitude(), true);
  if (!std::isfinite(bound)) return false;
  const long double excess = std::max(0.L, (long double)bound - tau);
  sum += excess * excess;
  ++count;
  return std::isfinite(sum);
}
#pragma GCC pop_options
void M25Rotate(std::array<Point, 3>& tri, std::array<Id, 3> ids) {
  const auto n = std::min_element(ids.begin(), ids.end()) - ids.begin();
  std::rotate(tri.begin(), tri.begin() + n, tri.end());
}
void M25Prediction(M25Row& r, const D9Loaded& loaded, double tau, const std::function<bool()>& within) {
  auto budget = M25Budget(loaded.held, within);
  auto geometry = loaded.base->WithAtlasReservation(budget);
  const auto* a = loaded.parameters->Find(r.input.edge[0]);
  const auto* b = loaded.parameters->Find(r.input.edge[1]);
  std::array<Point, 2> endpoints{};
  if (a) endpoints[0] = a->node.p;
  if (b) endpoints[1] = b->node.p;
  const auto start = D9Clock::now();
  const size_t work = budget.work;
  try {
    r.prediction = PredictGeometrySplit(r.input.edge, endpoints, *geometry, tau);
  } catch (const std::length_error&) {
    r.seconds = std::chrono::duration<double>(D9Clock::now() - start).count();
    r.work = budget.work - work;
    if (!within()) {
      r.status = "incomplete_time_limit";
      return;
    }
    throw;
  }
  r.seconds = std::chrono::duration<double>(D9Clock::now() - start).count();
  r.work = budget.work - work;
  r.status = r.prediction.complete ? "complete" : "incomplete";
}
#pragma GCC push_options
#pragma GCC optimize("no-fast-math", "no-unsafe-math-optimizations", "fp-contract=off")
__attribute__((noinline)) bool M25Face(M25Row& r, const BoundaryParameterState& state_ref,
                                       const PlanarReference& context_ref, size_t f, double tau, long double& before,
                                       long double& after, size_t& count) {
  const auto* state = &state_ref;
  const auto* context = &context_ref;
  const auto& face = *r.faces[f];
  std::array<Point, 3> parent;
  unsigned ia = 3, ib = 3;
  for (unsigned k = 0; k < 3; ++k) {
    const auto* p = state->Find(face.v[k]);
    if (!p) return false;
    parent[k] = p->node.p;
    if (face.v[k] == r.input.edge[0]) ia = k;
    if (face.v[k] == r.input.edge[1]) ib = k;
  }
  if (ia == 3 || ib == 3 || !M25Score(*context, face.facet, parent, tau, before, count, r.parent[f])) return false;
  r.parent_valid[f] = true;
  auto left = parent, right = parent;
  left[ia] = r.bound;
  right[ib] = r.bound;
  auto li = face.v, ri = face.v;
  li[ia] = UINT64_MAX;
  ri[ib] = UINT64_MAX;
  M25Rotate(left, li);
  M25Rotate(right, ri);
  if (!M25Score(*context, face.facet, left, tau, after, count, r.child[f][0])) return false;
  r.child_valid[f][0] = true;
  if (!M25Score(*context, face.facet, right, tau, after, count, r.child[f][1])) return false;
  r.child_valid[f][1] = true;
  return true;
}
__attribute__((noinline)) bool M25Reconstruct(M25Row& r, const D9Loaded& loaded, double tau,
                                              const std::function<bool()>& within) {
  auto& pred = r.prediction;
  long double before = 0, after = 0;
  size_t count = 0;
  auto budget = M25Budget(loaded.held, within);
  auto geometry = loaded.base->WithAtlasReservation(budget);
  const auto* state = geometry->Parameters();
  if (r.input.edge[0] >= r.input.edge[1] || !std::isfinite(tau) || tau < 0) return false;
  if (!state || !geometry->EntityGeometry()) goto verify;
  {
    const auto* a = state->Find(r.input.edge[0]);
    const auto* b = state->Find(r.input.edge[1]);
    if (!a || !b || state->Geometry().MobilityDimension(a->binding, state->Mobility()) != 2 ||
        state->Geometry().MobilityDimension(b->binding, state->Mobility()) != 2)
      goto verify;
    if (state->Coverage().traces.count(r.input.edge)) goto verify;
    const auto incident = state->IncidentFaces(r.input.edge);
    if (!incident[0] || !incident[1] || state->Entity(incident[0]->facet) != state->Entity(incident[1]->facet))
      goto verify;
    r.faces = {incident[0], incident[1]};
    // AtlasStorage is private to the production TU; mirror its admitted live reservation on this local budget.
    const size_t live = transfer_memory::Add(budget.caller_bytes, geometry->ParameterOverlayBytes(), sizeof(Patch),
                                             6 * sizeof(std::array<Point, 3>), 8 * 512, 4 * 128);
    if (live == SIZE_MAX || live > budget.maximum_bytes) throw std::length_error("M25 atlas scratch exceeds admission");
    budget.caller_bytes = live;
    budget.peak_bytes = std::max(budget.peak_bytes, live);
    try {
      auto context = geometry->PrivateParameters(Patch{}, &budget);
      if (!context) goto verify;
      const std::set<Id> facets{incident[0]->facet, incident[1]->facet};
      r.midpoint = {double(((long double)a->node.p.x + b->node.p.x) / 2),
                    double(((long double)a->node.p.y + b->node.p.y) / 2),
                    double(((long double)a->node.p.z + b->node.p.z) / 2)};
      r.midpoint_valid = true;
      r.length = std::hypot(b->node.p.x - a->node.p.x, b->node.p.y - a->node.p.y, b->node.p.z - a->node.p.z);
      r.length_valid = true;
      const auto bound = context->BindNewPoint(Node{UINT64_MAX, r.midpoint}, facets, &r.input.edge);
      r.bound = bound.p;
      r.point_valid = true;
      r.distance = std::hypot(r.bound.x - r.midpoint.x, r.bound.y - r.midpoint.y, r.bound.z - r.midpoint.z);
      r.distance_valid = true;
      if (M25PointKey(bound.p) == M25PointKey(a->node.p) || M25PointKey(bound.p) == M25PointKey(b->node.p)) goto verify;
      for (size_t f = 0; f < 2; ++f)
        if (!M25Face(r, *state, *context, f, tau, before, after, count)) goto verify;
    } catch (const std::invalid_argument&) {
      if (budget.work_exhausted || !budget.geometry_failure.empty()) throw;
    }
  }
verify:
  r.selfcheck = count == pred.evaluated_faces && before == pred.before && after == pred.after;
  return r.selfcheck;
}
__attribute__((noinline)) void M25Supplement(M25Row& r, const D9Loaded& loaded, double tau,
                                             const std::function<bool()>& within) {
  if (r.prediction.evaluated_faces || !r.faces[0] || !r.faces[1]) return;
  r.supplemental_bounds = true;
  auto budget = M25Budget(loaded.held, within);
  auto geometry = loaded.base->WithAtlasReservation(budget);
  const size_t live = transfer_memory::Add(budget.caller_bytes, geometry->ParameterOverlayBytes(), sizeof(Patch),
                                           6 * sizeof(std::array<Point, 3>), 8 * 512, 4 * 128);
  if (live == SIZE_MAX || live > budget.maximum_bytes)
    throw std::length_error("M25 supplemental scratch exceeds admission");
  budget.caller_bytes = live;
  budget.peak_bytes = std::max(budget.peak_bytes, live);
  try {
    auto context = geometry->PrivateParameters(Patch{}, &budget);
    const auto* a = loaded.parameters->Find(r.input.edge[0]);
    const auto* b = loaded.parameters->Find(r.input.edge[1]);
    if (!a || !b) throw std::invalid_argument("M25 supplemental endpoint missing");
    r.midpoint = {double(((long double)a->node.p.x + b->node.p.x) / 2),
                  double(((long double)a->node.p.y + b->node.p.y) / 2),
                  double(((long double)a->node.p.z + b->node.p.z) / 2)};
    r.midpoint_valid = true;
    r.length = std::hypot(b->node.p.x - a->node.p.x, b->node.p.y - a->node.p.y, b->node.p.z - a->node.p.z);
    r.length_valid = true;
    const auto bound =
        context->BindNewPoint(Node{UINT64_MAX, r.midpoint}, {r.faces[0]->facet, r.faces[1]->facet}, &r.input.edge);
    r.bound = bound.p;
    r.point_valid = true;
    r.distance = std::hypot(r.bound.x - r.midpoint.x, r.bound.y - r.midpoint.y, r.bound.z - r.midpoint.z);
    r.distance_valid = true;
    long double before = 0, after = 0;
    size_t count = 0;
    for (size_t face = 0; face < 2; ++face)
      if (!M25Face(r, *loaded.parameters, *context, face, tau, before, after, count))
        r.bounds_error = "ContainsFace or finite-bound refusal";
  } catch (const std::invalid_argument& e) {
    if (budget.work_exhausted || !budget.geometry_failure.empty()) throw;
    r.bounds_error = e.what();
  }
}
#pragma GCC pop_options
std::array<Edge, 3> M25EdgesForFace(const M25Row& r, size_t face_index) {
  const auto& face = *r.faces[face_index];
  std::array<Edge, 3> out{};
  size_t n = 1;
  bool found = false;
  for (unsigned k = 0; k < 3; ++k) {
    Edge edge{face.v[k], face.v[(k + 1) % 3]};
    std::sort(edge.begin(), edge.end());
    if (edge == r.input.edge) {
      out[0] = edge;
      found = true;
    } else
      out[n++] = edge;
  }
  if (!found || n != 3) throw std::runtime_error("incident face does not contain M25 edge");
  return out;
}
void M25Laws(M25Row& r, const D9Loaded& loaded) {
  r.law_rows_valid = true;
  for (size_t f = 0; f < 2; ++f) {
    r.law_edges[f] = M25EdgesForFace(r, f);
    for (size_t e = 0; e < 3; ++e) {
      const auto& edge = r.law_edges[f][e];
      const auto* a = loaded.parameters->Find(edge[0]);
      const auto* b = loaded.parameters->Find(edge[1]);
      if (a && b) r.law[f][e] = EstimateWallEdge(*loaded.parameters, a->node, b->node, r.faces[f]->facet);
    }
  }
}
std::vector<std::string> M25Columns() {
  const std::string names =
      "edge_a,edge_b,law_pick,n11_first_verdict,failing_faces,limiting_faces,limiting_failing_faces,need,estimate,"
      "attempt_status,complete,before,after,evaluated_faces,seconds,atlas_work,selfcheck,point_valid,bound_x,bound_y,"
      "bound_z,midpoint_valid,midpoint_x,midpoint_y,midpoint_z,distance_valid,midpoint_distance,length_valid,edge_"
      "length,parent0_valid,parent0,child00_valid,child00,child01_valid,child01,parent1_valid,parent1,child10_valid,"
      "child10,child11_valid,child11,law_rows_valid";
  std::stringstream stream(names);
  std::string name;
  std::vector<std::string> columns;
  while (std::getline(stream, name, ',')) columns.push_back(name);
  for (size_t f = 0; f < 2; ++f) {
    columns.push_back("parent" + std::to_string(f) + "_facet");
    for (size_t v = 0; v < 3; ++v) columns.push_back("parent" + std::to_string(f) + "_v" + std::to_string(v));
  }
  for (size_t f = 0; f < 2; ++f)
    for (size_t e = 0; e < 3; ++e) {
      const auto prefix = "law_f" + std::to_string(f) + "_e" + std::to_string(e);
      for (const auto suffix : {"_edge", "_supported", "_value"}) columns.push_back(prefix + suffix);
    }
  columns.push_back("supplemental_bounds");
  columns.push_back("bounds_error");
  return columns;
}
template <class T>
std::string M25Text(T value) {
  std::ostringstream stream;
  stream << std::setprecision(std::numeric_limits<long double>::max_digits10) << value;
  return stream.str();
}
std::vector<std::string> M25Fields(const M25Row& r, size_t columns) {
  std::vector<std::string> out(r.input.raw.begin(), r.input.raw.end());
  out.push_back(r.status);
  if (r.status == "not_attempted_time_limit" || r.status == "incomplete_time_limit") {
    out.resize(columns);
    return out;
  }
  auto add = [&](auto value) { out.push_back(M25Text(value)); };
  auto measured = [&](bool valid, auto value) { out.push_back(valid ? M25Text(value) : ""); };
  add(r.prediction.complete);
  add(r.prediction.before);
  add(r.prediction.after);
  add(r.prediction.evaluated_faces);
  add(r.seconds);
  add(r.work);
  add(r.selfcheck);
  add(r.point_valid);
  measured(r.point_valid, r.bound.x);
  measured(r.point_valid, r.bound.y);
  measured(r.point_valid, r.bound.z);
  add(r.midpoint_valid);
  measured(r.midpoint_valid, r.midpoint.x);
  measured(r.midpoint_valid, r.midpoint.y);
  measured(r.midpoint_valid, r.midpoint.z);
  add(r.distance_valid);
  measured(r.distance_valid, r.distance);
  add(r.length_valid);
  measured(r.length_valid, r.length);
  for (size_t f = 0; f < 2; ++f) {
    add(r.parent_valid[f]);
    measured(r.parent_valid[f], r.parent[f]);
    for (size_t c = 0; c < 2; ++c) {
      add(r.child_valid[f][c]);
      measured(r.child_valid[f][c], r.child[f][c]);
    }
  }
  add(r.law_rows_valid);
  for (size_t f = 0; f < 2; ++f) {
    measured(r.faces[f] != nullptr, r.faces[f] ? r.faces[f]->facet : 0);
    for (size_t v = 0; v < 3; ++v) measured(r.faces[f] != nullptr, r.faces[f] ? r.faces[f]->v[v] : 0);
  }
  for (size_t f = 0; f < 2; ++f)
    for (size_t e = 0; e < 3; ++e) {
      out.push_back(r.law_rows_valid ? M25Text(r.law_edges[f][e][0]) + ":" + M25Text(r.law_edges[f][e][1]) : "");
      measured(r.law_rows_valid, r.law[f][e].supported);
      measured(r.law_rows_valid && r.law[f][e].supported, r.law[f][e].value);
    }
  add(r.supplemental_bounds);
  out.push_back(r.bounds_error);
  if (out.size() != columns) throw std::runtime_error("M25 output column count mismatch");
  return out;
}
void M25Write(const std::vector<M25Row>& rows) {
  std::ofstream out("m25_split_outcomes.csv");
  if (!out) throw std::runtime_error("cannot create M25 CSV");
  const auto columns = M25Columns();
  auto write = [&](const std::vector<std::string>& fields) {
    for (size_t i = 0; i < fields.size(); ++i) out << (i ? "," : "") << D9Csv(fields[i]);
    out << '\n';
  };
  write(columns);
  for (const auto& r : rows) write(M25Fields(r, columns.size()));
  if (!out) throw std::runtime_error("M25 CSV write failed");
}

D9Loaded M25Load(double tau) {
  const char* ref_path = std::getenv("SU2_D9_REFERENCE");
  REQUIRE(ref_path);
  const char* param_path = std::getenv("SU2_D9_PARAMETERS");
  D9Loaded loaded;
  loaded.bytes = D9ReadBytes(ref_path);
  loaded.reference = DecodeReference(std::string(loaded.bytes.begin(), loaded.bytes.end()), D9HeldMemoryLimit);
  REQUIRE(loaded.reference.original);
  if (param_path) {
    const auto* initial = loaded.reference.original->Parameters();
    const auto authority = initial ? initial->Authority() : std::shared_ptr<const MappedFacetReference>{};
    REQUIRE(authority);
    const auto bytes = D9ReadBytes(param_path);
    loaded.parameters = BoundaryParameterState::FromPayload(authority, bytes, D9HeldMemoryLimit,
                                                            loaded.reference.original->ResidentBytes());
  } else if (loaded.reference.original->Parameters()) {
    loaded.parameters = std::make_shared<BoundaryParameterState>(*loaded.reference.original->Parameters());
  }
  REQUIRE(loaded.parameters);
  loaded.authority = loaded.parameters->Authority();
  REQUIRE(loaded.authority);
  loaded.entity_geometry = loaded.parameters->EntityGeometry();
  loaded.query_tolerance = loaded.parameters->GeometryTolerance();
  REQUIRE(loaded.query_tolerance == tau);
  loaded.base = loaded.reference.original->WithParameterState(loaded.parameters);
  loaded.held = D9HeldReferenceBytes(loaded.reference, loaded.parameters, loaded.bytes, *loaded.base);
  REQUIRE(loaded.held <= D9HeldMemoryLimit);
  return loaded;
}
std::vector<M25Row> M25Run(const std::vector<M25Input>& inputs, const D9Loaded& loaded, double tau,
                           D9Clock::time_point started, const std::function<bool()>& within, bool& reprioritized) {
  std::vector<M25Row> rows(inputs.size());
  for (size_t i = 0; i < inputs.size(); ++i) rows[i].input = inputs[i];
  std::vector<size_t> order(inputs.size());
  for (size_t i = 0; i < order.size(); ++i) order[i] = i;
  size_t done = 0;
  for (size_t at = 0; at < order.size() && within(); ++at) {
    const auto i = order[at];
    const auto incident = loaded.parameters->IncidentFaces(inputs[i].edge);
    rows[i].faces = {incident[0], incident[1]};
    if (rows[i].faces[0] && rows[i].faces[1]) M25Laws(rows[i], loaded);
    try {
      M25Prediction(rows[i], loaded, tau, within);
      if (rows[i].status != "incomplete_time_limit") {
        if (!M25Reconstruct(rows[i], loaded, tau, within))
          throw std::runtime_error("M25 partial/complete sum self-check failed at edge " + inputs[i].raw[0] + ":" +
                                   inputs[i].raw[1]);
        M25Supplement(rows[i], loaded, tau, within);
        if (rows[i].prediction.complete) rows[i].status = "complete";
      }
    } catch (const std::length_error&) {
      if (!within())
        rows[i].status = "incomplete_time_limit";
      else
        throw;
    }
    ++done;
    if (done % 100 == 0) std::cout << "M25 completed " << done << "/" << inputs.size() << std::endl;
    if (done == 100 && within()) {
      const double elapsed = std::chrono::duration<double>(D9Clock::now() - started).count();
      if (elapsed * double(inputs.size()) / 100.0 > 3600.0) {
        std::stable_sort(order.begin() + 100, order.end(), [&](size_t a, size_t b) {
          return (inputs[a].verdict != "none") > (inputs[b].verdict != "none");
        });
        reprioritized = true;
      }
    }
  }
  return rows;
}
void M25Summary(const std::vector<M25Row>& rows, bool reprioritized) {
  const auto done =
      std::count_if(rows.begin(), rows.end(), [](const M25Row& r) { return r.status != "not_attempted_time_limit"; });
  std::ofstream meta("m25_fixture_summary.json");
  REQUIRE(bool(meta));
  const auto complete = std::count_if(rows.begin(), rows.end(), [](const M25Row& r) { return r.status == "complete"; });
  meta << "{\"schema\":\"m25_fixture_v1\",\"rows\":" << rows.size() << ",\"attempted\":" << done
       << ",\"complete\":" << complete << ",\"interrupted_time_limit\":"
       << std::count_if(rows.begin(), rows.end(), [](const M25Row& r) { return r.status == "incomplete_time_limit"; })
       << ",\"reprioritized_after_100\":" << (reprioritized ? "true" : "false")
       << ",\"time_limit_seconds\":3600,\"production_workflow_launched\":false}\n";
  REQUIRE(bool(meta));
  REQUIRE(done > 0);
}
}  // namespace
TEST_CASE("M25 saved492 geometry split outcomes", "[.][NativeM25Split]") {
  D9CheckNeedScaling();
  World world;
  REQUIRE(world.size == 1);
  const char* input_path = std::getenv("SU2_M25_EDGES");
  REQUIRE(input_path);
  const auto started = D9Clock::now();
  // Keep CSV output and teardown inside the 3600-second process cap.
  const auto within = [started] { return D9Clock::now() - started < std::chrono::seconds(3540); };
  const double tau = 1e-4;
  const auto inputs = M25Read(input_path);
  auto loaded = M25Load(tau);
  bool reprioritized = false;
  const auto rows = M25Run(inputs, loaded, tau, started, within, reprioritized);
  M25Write(rows);
  M25Summary(rows, reprioritized);
}
