/*!
 * \file CNativeFastPathStage2_tests.cpp
 * \brief Test-only surface measurements on frozen ONERA M6 sidecars.
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md).
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */
#include "TransferTestCase.hpp"
#include "../../../Common/include/adaptation/CNativeBoundaryParameters3D.hpp"
#include "../../../Common/include/adaptation/CNativeCurvedAtlas3D.hpp"
#include "../../../Common/include/adaptation/CNativeCurvedReference.hpp"
#include "../../../Common/include/adaptation/CNativeGeometryBackend3D.hpp"
#include "../../../Common/include/adaptation/CNativeReferenceIO3D.hpp"
#include "../../../Common/include/adaptation/CNativeRuntimeProfile3D.hpp"
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>
#include <memory>
#include <numeric>
#include <optional>
#include <set>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

// Preserve finite/nonfinite diagnostics in this fixture under the test target's fast-math flags.
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC push_options
#pragma GCC optimize("no-fast-math", "no-unsafe-math-optimizations", "fp-contract=off")
#endif

namespace {
using namespace SU2Native3D;
using State = BoundaryParameterState;
using Clock = std::chrono::steady_clock;
using Triangle = std::array<BoundaryPoint, 3>;
constexpr double Tol = 1e-4;
constexpr size_t Ceiling = size_t{12288} << 20;

double Seconds(Clock::time_point start) { return std::chrono::duration<double>(Clock::now() - start).count(); }

void JsonString(const std::string& value) {
  constexpr char digits[] = "0123456789abcdef";
  std::cout << '"';
  for (unsigned char c : value) {
    if (c == '"' || c == '\\')
      std::cout << '\\' << static_cast<char>(c);
    else if (c < 0x20)
      std::cout << "\\u00" << digits[c / 16] << digits[c % 16];
    else
      std::cout << static_cast<char>(c);
  }
  std::cout << '"';
}

void JsonNumber(double value) {
  if (std::isfinite(value))
    std::cout << value;
  else
    std::cout << "null";
}

void Output(const char* row) { std::cout << std::setprecision(17) << "FP2_0 " << row << " {"; }

void ReportError(const char* row, Clock::time_point start, const std::exception& error) {
  Output(row);
  std::cout << "\"seconds\":" << Seconds(start) << ",\"error\":";
  JsonString(error.what());
  std::cout << '}' << std::endl;
}

void EntityJson(State::EntityKey entity) {
  const auto [body, sheet, marker] = entity;
  std::cout << '[' << body << ',' << sheet << ',' << marker << ']';
}

Triangle GetTriangle(const BoundaryPoints& points, const SurfaceFace& face) {
  return {points.at(face.v[0]), points.at(face.v[1]), points.at(face.v[2])};
}

SU2NativeGeometry::AtlasBudget Fp2Budget(size_t cells, size_t caller, Clock::time_point start, double seconds) {
  using SU2NativeGeometry::CurvedAtlasWorkspaceBytes;
  constexpr size_t scratch = size_t{16} << 20;
  size_t low = 0, high = scratch * 8;
  while (low < high) {
    const size_t middle = low + (high - low + 1) / 2;
    if (2 * CurvedAtlasWorkspaceBytes(middle) <= scratch / 2)
      low = middle;
    else
      high = middle - 1;
  }
  SU2NativeGeometry::AtlasBudget budget;
  budget.maximum_work =
      (cells + std::max(size_t{4096}, cells * 64)) * SU2NativeGeometry::MappedFacetReference::ClearanceWork;
  budget.maximum_bytes = Ceiling;
  budget.maximum_integer_bits = low;
  budget.caller_bytes = caller;
  budget.within_time = [start, seconds] { return Seconds(start) < seconds; };
  return budget;
}

struct EntityCounts {
  size_t faces = 0;
  std::set<Id> points;
};

void CountFace(std::map<State::EntityKey, EntityCounts>& counts, const State& state, const SurfaceFace& face) {
  auto& entity = counts[state.Entity(face.facet)];
  ++entity.faces;
  entity.points.insert(face.v.begin(), face.v.end());
}

void EntityCountsJson(const std::map<State::EntityKey, EntityCounts>& counts) {
  std::cout << '[';
  bool first = true;
  for (const auto& [entity, count] : counts) {
    if (!first) std::cout << ',';
    first = false;
    std::cout << "{\"entity\":";
    EntityJson(entity);
    std::cout << ",\"faces\":" << count.faces << ",\"points\":" << count.points.size() << '}';
  }
  std::cout << ']';
}

const char* Input(const char* name) {
  const auto* path = std::getenv(name);
  if (!path || !*path) throw std::invalid_argument(std::string("Missing input path ") + name);
  return path;
}

ReferenceState LoadSurface(bool output) {
  REQUIRE(SU2_MPI::GetSize() == 1);
  const auto start = Clock::now();
  size_t peak = 0;
  bool loaded = false;
  std::string reason;
  ReferenceState result;
  const bool restored =
      LoadReference(Input(output ? "SU2_FP2_OUT" : "SU2_FP2_IN"), result, loaded, reason, Ceiling, 0, peak);
  INFO(reason);
  REQUIRE(restored);
  REQUIRE(loaded);
  REQUIRE(result.original);
  const auto* state = result.original->Parameters();
  REQUIRE(state);
  REQUIRE(state->EntityGeometry() == output);
  REQUIRE(state->Records().size() == (output ? 33587 : 24479));
  REQUIRE(state->Surface().size() == (output ? 67170 : 48954));
  if (output) REQUIRE(state->GeometryTolerance() == Tol);
  const double seconds = Seconds(start);
  Output(output ? "OUT1_load" : "IN1_load");
  std::cout << "\"seconds\":" << seconds << ",\"modeled_peak_bytes\":" << peak
            << ",\"faces\":" << state->Surface().size() << ",\"points\":" << state->Records().size();
  if (output) {
    std::map<State::EntityKey, EntityCounts> counts;
    for (const auto& row : state->Surface()) CountFace(counts, *state, row.second);
    std::cout << ",\"entities\":";
    EntityCountsJson(counts);
  }
  std::cout << '}' << std::endl;
  return result;
}

void PrepareInput(ReferenceState& reference) {
  const auto start = Clock::now();
  const auto& state = *reference.original->Parameters();
  REQUIRE((reference.marker_names == std::vector<std::string>{"FARFIELD", "SYMMETRY", "WING3D"}));
  REQUIRE(state.Mobility().identity == 0);
  const size_t baseline = reference.original->ResidentBytes();
  if (baseline >= Ceiling) throw std::length_error("Curved mobility setup exceeds storage admission.");
  SU2NativeGeometry::AtlasBudget budget;
  budget.maximum_bytes = Ceiling;
  budget.caller_bytes = baseline;
  budget.maximum_integer_bits = transfer_memory::Mul(Ceiling - baseline, 8);
  budget.maximum_work = transfer_memory::Mul(
      transfer_memory::Add(state.Geometry().Reference().Facets().size(), state.Surface().size()),
      SU2NativeGeometry::MappedFacetReference::ClearanceWork);
  if (budget.maximum_work == SIZE_MAX || budget.maximum_integer_bits == SIZE_MAX)
    throw std::length_error("Curved mobility setup reservation overflows.");
  auto mobility = state.WithMobilityPolicy(60, {{0, 2}, {1, 0}, {2, 2}}, budget, Ceiling, baseline);
  auto entity = mobility->WithEntityGeometry(Tol, budget, Ceiling, transfer_memory::Add(baseline, mobility->ResidentBytes()));
  reference.original = reference.original->WithParameterState(entity);
  Output("IN1_setup");
  std::cout << "\"seconds\":" << Seconds(start) << ",\"caller_bytes\":" << baseline
            << ",\"modeled_peak_bytes\":" << budget.peak_bytes << ",\"work\":" << budget.work
            << ",\"protected_edges\":" << entity->Mobility().features.size()
            << ",\"traces\":" << entity->Coverage().traces.size() << '}' << std::endl;
}

using Bits = std::array<uint64_t, 3>;
Bits CoordinateBits(Point p) {
  static_assert(sizeof(double) == sizeof(uint64_t), "The frozen sensor uses binary64 coordinates.");
  Bits bits;
  std::memcpy(&bits[0], &p.x, sizeof(double));
  std::memcpy(&bits[1], &p.y, sizeof(double));
  std::memcpy(&bits[2], &p.z, sizeof(double));
  return bits;
}

struct MetricPoint {
  BoundaryPoint point;
  Tensor tensor;
};

std::vector<MetricPoint> ReadSurfaceSensor(const State& state) {
  const auto start = Clock::now();
  std::vector<MetricPoint> points;
  points.reserve(state.Records().size());
  std::map<Bits, size_t> boundary;
  for (const auto& [id, record] : state.Records()) {
    REQUIRE(record.point.node.id == id);
    REQUIRE(record.point.binding.dimension == 0);
    REQUIRE(boundary.emplace(CoordinateBits(record.point.node.p), points.size()).second);
    points.push_back({record.point, {}});
  }
  std::ifstream input(Input("SU2_FP2_SENSOR"), std::ios::binary);
  uint64_t count = 0;
  input.read(reinterpret_cast<char*>(&count), sizeof(count));
  REQUIRE(count == 61330);
  std::vector<bool> matched(points.size());
  for (uint64_t i = 0; i < count; ++i) {
    uint64_t id = count;
    std::array<double, 12> row;
    input.read(reinterpret_cast<char*>(&id), sizeof(id));
    input.read(reinterpret_cast<char*>(row.data()), sizeof(row));
    if (!input || id != i) throw std::invalid_argument("Invalid FP0 frozen sensor row.");
    const auto found = boundary.find(CoordinateBits({row[0], row[1], row[2]}));
    if (found == boundary.end()) continue;
    const size_t slot = found->second;
    if (matched[slot]) throw std::invalid_argument("Frozen sensor has multiple rows at one boundary coordinate.");
    points[slot].tensor = {row[3], row[4], row[5], row[7], row[8], row[11]};
    matched[slot] = true;
  }
  REQUIRE(input.peek() == std::char_traits<char>::eof());
  REQUIRE(std::count(matched.begin(), matched.end(), true) == static_cast<std::ptrdiff_t>(points.size()));
  Output("IN1_sensor");
  std::cout << "\"seconds\":" << Seconds(start) << ",\"sensor_records\":" << count << ",\"matched\":" << points.size()
            << '}' << std::endl;
  return points;
}

#include "fixtures/native_fast_path_stage2_walk.inc"
#include "fixtures/native_fast_path_stage2_strict.inc"

Edge EdgeOf(Id a, Id b) { return {std::min(a, b), std::max(a, b)}; }

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

struct SplitStats {
  size_t passes = 0, splits = 0, first_requests = 0, first_wing_requests = 0;
  size_t four_sample_calls = 0, unconverged_samples = 0, capped_samples = 0, candidates_blocked = 0;
  std::map<std::string, size_t> refusals;
  std::string first_error;
};

class Prototype {
 public:
  Prototype(const State& state, std::vector<MetricPoint> input)
      : state(state), walker(state), points(std::move(input)) {
    for (size_t i = 0; i < points.size(); ++i) {
      const auto& point = points[i].point;
      point_ids.emplace(point.node.id, i);
      coordinates.insert(Coordinate(point.node.p));
      bindings.insert(Support(point.binding));
      next_id = std::max(next_id, point.node.id + 1);
    }
    faces.reserve(state.Surface().size());
    for (const auto& row : state.Surface()) faces.push_back(NewFace(row.second, true));
  }

  std::vector<FaceMeasurement>& Faces() { return faces; }

  BoundaryPoints Records() const {
    BoundaryPoints result;
    for (const auto& point : points) result.emplace(point.point.node.id, point.point);
    return result;
  }

  void SampleFaces(SplitStats& stats) {
    for (auto& item : faces) {
      if (item.sample >= 0) continue;
      const auto samples = walker.Samples(item.face, Corners(item.face));
      item.sample = 0;
      for (const auto& sample : samples) {
        if (!std::isfinite(sample.distance)) throw std::runtime_error("Float walk produced a nonfinite sample.");
        item.sample = std::max(item.sample, sample.distance);
        ++stats.four_sample_calls;
        if (!sample.converged) ++stats.unconverged_samples;
        if (sample.capped) ++stats.capped_samples;
      }
    }
  }

  SplitStats Run(bool distance) {
    SplitStats stats;
    for (size_t pass = 0; pass < 20; ++pass) {
      auto edges = Edges();
      std::map<Edge, double> marks;
      size_t requested = 0;
      if (distance) {
        SampleFaces(stats);
        for (const auto& face : faces) {
          if (face.sample <= 0.8 * Tol) continue;
          ++requested;
          const auto edges_of_face = FaceEdges(face.face);
          std::optional<Edge> best;
          double longest = 0;
          for (const auto& edge : edges_of_face) {
            if (Feature(edge)) continue;
            const double length = WalkDistance(At(edge[0]).point.node.p, At(edge[1]).point.node.p);
            if (!best || length > longest || (length == longest && edge < *best)) {
              best = edge;
              longest = length;
            }
          }
          if (best)
            marks.emplace(*best, longest);
          else
            ++stats.refusals["all_feature_edges"];
        }
      } else {
        for (const auto& [edge, incident] : edges) {
          if (Feature(edge)) continue;
          const double length = MetricLength(At(edge[0]), At(edge[1]));
          if (length <= 1.8) continue;
          ++requested;
          if (!pass && std::get<2>(state.Entity(faces[incident[0]].face.facet)) == 2) ++stats.first_wing_requests;
          marks.emplace(edge, length);
        }
      }
      if (!pass) stats.first_requests = requested;
      if (!requested) break;
      ++stats.passes;
      std::vector<std::pair<Edge, double>> ordered(marks.begin(), marks.end());
      std::sort(ordered.begin(), ordered.end(), [](const auto& a, const auto& b) {
        return a.second == b.second ? a.first < b.first : a.second > b.second;
      });
      std::vector<bool> changed(faces.size());
      for (const auto& [edge, length] : ordered) {
        (void)length;
        const auto incident = edges.at(edge);
        if (changed[incident[0]] || changed[incident[1]]) {
          ++stats.candidates_blocked;
          continue;
        }
        if (Split(edge, incident, stats)) {
          changed[incident[0]] = changed[incident[1]] = true;
          ++stats.splits;
        }
      }
    }
    if (distance) SampleFaces(stats);
    return stats;
  }

 private:
  using Incident = std::array<size_t, 2>;
  using Binding = std::tuple<unsigned, Id, double, double>;

  static std::array<double, 3> Coordinate(Point p) { return {p.x, p.y, p.z}; }
  static Binding Support(const SU2NativeGeometry::SurfaceBinding& b) { return {b.dimension, b.feature, b.u, b.v}; }

  static FaceMeasurement NewFace(SurfaceFace face, bool original) {
    FaceMeasurement result{};
    result.face = face;
    result.original = original;
    result.sample = -1;
    return result;
  }

  const MetricPoint& At(Id id) const { return points.at(point_ids.at(id)); }

  Triangle Corners(const SurfaceFace& face) const {
    return {At(face.v[0]).point, At(face.v[1]).point, At(face.v[2]).point};
  }

  static std::array<Edge, 3> FaceEdges(const SurfaceFace& face) {
    const auto& v = face.v;
    return {EdgeOf(v[0], v[1]), EdgeOf(v[1], v[2]), EdgeOf(v[2], v[0])};
  }

  std::map<Edge, Incident> Edges() const {
    std::map<Edge, Incident> result;
    for (size_t i = 0; i < faces.size(); ++i) {
      for (const auto& edge : FaceEdges(faces[i].face)) {
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

  bool Feature(const Edge& edge) const {
    const auto& a = At(edge[0]).point.binding;
    const auto& b = At(edge[1]).point.binding;
    return a.dimension == 0 && b.dimension == 0 && state.Mobility().features.count(EdgeOf(a.feature, b.feature));
  }

  bool Split(const Edge& edge, const Incident& incident, SplitStats& stats) {
    const auto& a = At(edge[0]);
    const auto& b = At(edge[1]);
    const auto& first_face = faces[incident[0]].face;
    const auto corners = Corners(first_face);
    unsigned first = 0, second = 0;
    for (unsigned k = 0; k < 3; ++k) {
      if (first_face.v[k] == edge[0]) first = k;
      if (first_face.v[k] == edge[1]) second = k;
    }
    const Point query{0.5 * (a.point.node.p.x + b.point.node.p.x), 0.5 * (a.point.node.p.y + b.point.node.p.y),
                      0.5 * (a.point.node.p.z + b.point.node.p.z)};
    BoundaryPoint point;
    try {
      const auto closest = walker.Run(query, walker.EdgeStart(first_face, corners, first, second, query));
      if (!closest.converged || closest.capped) {
        ++stats.refusals[closest.capped ? "walk_cap" : "walk_nonconvergence"];
        return false;
      }
      point.binding = walker.Binding(closest);
      if (state.Geometry().MobilityDimension(point.binding, state.Mobility()) != 2) {
        ++stats.refusals["protected_binding"];
        return false;
      }
      point.node = {next_id, state.Geometry().Realize(point.binding)};
    } catch (const std::exception& error) {
      ++stats.refusals["projection_error"];
      if (stats.first_error.empty()) stats.first_error = error.what();
      return false;
    }
    const auto support = Support(point.binding);
    const auto coord = Coordinate(point.node.p);
    if (bindings.count(support)) {
      ++stats.refusals["duplicate_binding"];
      return false;
    }
    if (coordinates.count(coord)) {
      ++stats.refusals["duplicate_coordinates"];
      return false;
    }
    const Tensor m{0.5 * (a.tensor.xx + b.tensor.xx), 0.5 * (a.tensor.xy + b.tensor.xy),
                   0.5 * (a.tensor.xz + b.tensor.xz), 0.5 * (a.tensor.yy + b.tensor.yy),
                   0.5 * (a.tensor.yz + b.tensor.yz), 0.5 * (a.tensor.zz + b.tensor.zz)};
    bindings.insert(support);
    coordinates.insert(coord);
    point_ids.emplace(next_id, points.size());
    points.push_back({point, m});
    for (size_t index : incident) {
      const auto parent = faces[index].face;
      const auto edges = FaceEdges(parent);
      const auto k = std::find(edges.begin(), edges.end(), edge) - edges.begin();
      const Id left = parent.v[k], right = parent.v[(k + 1) % 3], other = parent.v[(k + 2) % 3];
      faces[index] = NewFace({{left, next_id, other}, parent.facet}, false);
      faces.push_back(NewFace({{next_id, right, other}, parent.facet}, false));
    }
    ++next_id;
    return true;
  }

  const State& state;
  Walker walker;
  std::vector<MetricPoint> points;
  std::unordered_map<Id, size_t> point_ids;
  std::set<std::array<double, 3>> coordinates;
  std::set<Binding> bindings;
  std::vector<FaceMeasurement> faces;
  Id next_id = 0;
};

void ReportSplit(const char* row, const SplitStats& stats, double seconds) {
  Output(row);
  std::cout << "\"seconds\":" << seconds << ",\"passes\":" << stats.passes << ",\"splits\":" << stats.splits
            << ",\"microseconds_per_split\":";
  JsonNumber(stats.splits ? 1e6 * seconds / stats.splits : std::numeric_limits<double>::quiet_NaN());
  std::cout << ",\"initial_requests\":" << stats.first_requests
            << ",\"initial_wing_requests\":" << stats.first_wing_requests
            << ",\"blocked_by_pass_conflict\":" << stats.candidates_blocked
            << ",\"sample_calls\":" << stats.four_sample_calls << ",\"sample_unconverged\":" << stats.unconverged_samples
            << ",\"sample_capped\":" << stats.capped_samples << ",\"refusals\":{";
  bool first = true;
  for (const auto& [reason, count] : stats.refusals) {
    if (!first) std::cout << ',';
    first = false;
    JsonString(reason);
    std::cout << ':' << count;
  }
  std::cout << "},\"first_projection_error\":";
  JsonString(stats.first_error);
  std::cout << '}' << std::endl;
}

void MeasurePrototype(ReferenceState& reference) {
  auto start = Clock::now();
  PrepareInput(reference);
  const auto& state = *reference.original->Parameters();
  Prototype mesh(state, ReadSurfaceSensor(state));
  const auto in_points = mesh.Records();
  start = Clock::now();
  SplitStats initial_samples;
  mesh.SampleFaces(initial_samples);
  Output("P_initial_samples");
  std::cout << "\"seconds\":" << Seconds(start) << ",\"samples\":" << initial_samples.four_sample_calls
            << ",\"unconverged\":" << initial_samples.unconverged_samples << ",\"capped\":"
            << initial_samples.capped_samples << '}' << std::endl;
  auto in_faces = mesh.Faces();
  start = Clock::now();
  const auto length = mesh.Run(false);
  ReportSplit("P_L", length, Seconds(start));
  start = Clock::now();
  const auto distance = mesh.Run(true);
  ReportSplit("P_D", distance, Seconds(start));

  std::map<State::EntityKey, EntityCounts> before, after;
  std::map<State::EntityKey, size_t> above;
  for (const auto& row : in_faces) CountFace(before, state, row.face);
  for (const auto& row : mesh.Faces()) {
    CountFace(after, state, row.face);
    if (row.sample > 0.8 * Tol) ++above[state.Entity(row.face.facet)];
  }
  auto final_points = mesh.Records();
  Output("P_entities");
  std::cout << "\"IN1_faces\":" << in_faces.size() << ",\"IN1_points\":" << in_points.size()
            << ",\"PD_faces\":" << mesh.Faces().size() << ",\"PD_points\":" << final_points.size() << ",\"IN1\":";
  EntityCountsJson(before);
  std::cout << ",\"PD\":";
  EntityCountsJson(after);
  std::cout << ",\"PD_above_0p8_tol\":[";
  bool first = true;
  for (const auto& [entity, count] : after) {
    (void)count;
    if (!first) std::cout << ',';
    first = false;
    std::cout << "{\"entity\":";
    EntityJson(entity);
    std::cout << ",\"faces\":" << above[entity] << '}';
  }
  std::cout << "]}" << std::endl;
  if (std::getenv("SU2_FP2_SPLITS_ONLY")) return;

  MeasureStrict(state, in_points, in_faces, final_points, mesh.Faces());
  MeasureProof(state, final_points, mesh.Faces());
}
}  // namespace

TEST_CASE("FP2-0 surface state rebuild and exact local checks", "[.][NativeFastPathStage2][NativeFastPathStage2Bounds]") {
  const auto reference = LoadSurface(true);
  MeasureBulkAndLocal(*reference.original->Parameters());
}

TEST_CASE("FP2-0 float wing walk and strict projection", "[.][NativeFastPathStage2][NativeFastPathStage2Projection]") {
  const auto reference = LoadSurface(true);
  MeasureProjection(*reference.original->Parameters());
}

TEST_CASE("FP2-0 input surface split and strict proof", "[.][NativeFastPathStage2][NativeFastPathStage2Split]") {
  auto reference = LoadSurface(false);
  const auto start = Clock::now();
  try {
    MeasurePrototype(reference);
  } catch (const std::exception& error) {
    ReportError("P", start, error);
  }
}

#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC pop_options
#endif
