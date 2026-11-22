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
#include "../../../Common/include/adaptation/CNativeLocalSurface3D.hpp"
#include "../../../Common/include/adaptation/CNativeReferenceIO3D.hpp"
#include "../../../Common/include/adaptation/CNativeRuntimeProfile3D.hpp"
#include "fixtures/native_fast_path_stage2_setup.hpp"
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
namespace local = SU2NativeLocal3D;
using State = BoundaryParameterState;
using Clock = std::chrono::steady_clock;
using Triangle = std::array<BoundaryPoint, 3>;
constexpr double Tol = native_fp2_fixture::Tolerance;
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
  return native_fp2_fixture::Budget(cells, caller, start, seconds, Ceiling);
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
  REQUIRE((reference.marker_names == std::vector<std::string>{"FARFIELD", "SYMMETRY", "WING3D"}));
  REQUIRE(reference.original->Parameters()->Mobility().identity == 0);
  const auto budget = native_fp2_fixture::PrepareIN1(reference, Ceiling);
  const auto* entity = reference.original->Parameters();
  Output("IN1_setup");
  std::cout << "\"seconds\":" << Seconds(start) << ",\"caller_bytes\":" << budget.caller_bytes
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

Point FlatNormal(const Triangle& corners) {
  const auto p = corners[0].node.p, q = corners[1].node.p, r = corners[2].node.p;
  const Point a{q.x - p.x, q.y - p.y, q.z - p.z}, b{r.x - p.x, r.y - p.y, r.z - p.z};
  const Point cross{a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
  const double length = std::hypot(cross.x, cross.y, cross.z);
  if (!(length > 0) || !std::isfinite(length)) return {};
  return {cross.x / length, cross.y / length, cross.z / length};
}

double ChildNormalDot(const State& state, Id side, const BoundaryPoint& corner, Point unit_normal) {
  const auto n = state.Geometry().EvaluateSurface(side, corner.binding).normal;
  const double length = std::hypot(n.x, n.y, n.z);
  if (!(length > 0) || !std::isfinite(length)) return 0;
  return unit_normal.x * (n.x / length) + unit_normal.y * (n.y / length) + unit_normal.z * (n.z / length);
}

#include "fixtures/native_fast_path_stage2_strict.inc"
#include "fixtures/native_fast_path_stage2_split.inc"

struct SheetPlacementPin {
  Edge edge;
  Bits position, parameters;
  Id feature;
  uint64_t version;
  unsigned dimension;
};

// Build638 used the same ordered IN1 sweep before the projector bound was added.
constexpr std::array<SheetPlacementPin, 3> SheetPlacementPins{{
    {{0, 834}, {13827030213679295113ULL, 4396545604192088565ULL, 13859865237332765445ULL},
     {4602678819172646894ULL, 0ULL, 0ULL}, 2, 18, 1},
    {{0, 1095}, {4613790540759143725ULL, 4397073084710767572ULL, 13859920413322671983ULL},
     {4602678819172646917ULL, 0ULL, 0ULL}, 3, 18, 1},
    {{0, 13978}, {4394268158308896713ULL, 4617203892881609772ULL, 13860092385972589726ULL},
     {4602678819172646912ULL, 0ULL, 0ULL}, 4, 18, 1},
}};

void PrintSheetPlacement(const char* kind, const Edge& edge, Point query, const BoundaryPoint& point) {
  const auto p = point.node.p;
  const auto& binding = point.binding;
  const auto xyz = CoordinateBits(p);
  const auto uv = CoordinateBits({binding.u, binding.v, 0.});
  std::cout << std::setprecision(17) << "FP2_B_C3 " << kind << " {\"edge\":[" << edge[0] << ',' << edge[1]
            << "],\"q\":[" << query.x << ',' << query.y << ',' << query.z << "],\"p\":[" << p.x << ',' << p.y
            << ',' << p.z << "],\"p_bits\":[" << xyz[0] << ',' << xyz[1] << ',' << xyz[2]
            << "],\"binding\":{\"dimension\":" << binding.dimension << ",\"feature\":" << binding.feature
            << ",\"version\":" << binding.version << ",\"u\":" << binding.u << ",\"v\":" << binding.v
            << ",\"uv_bits\":[" << uv[0] << ',' << uv[1] << "]}}" << std::endl;
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

TEST_CASE("FP2-A production surface geometry on IN1", "[.][NativeFastPathStage2Surface]") {
  auto reference = LoadSurface(false);
  CheckProductionSurface(reference);
}

TEST_CASE("FP2-B correction 3 IN1 sheet-placement bound", "[.][NativeFastPathStage2SheetPlacement]") {
  auto reference = LoadSurface(false);
  PrepareInput(reference);
  auto budget = Fp2Budget(726442, reference.original->ResidentBytes(), Clock::now(), 5400);
  local::StateSurfaceGeometry surface(reference.original, budget);
  const auto& state = surface.State();
  const Id fresh = state.Records().rbegin()->first + 1;
  std::map<Edge, std::vector<SurfaceFace>> incident;
  for (const auto& entry : state.Surface())
    for (const auto& edge : FaceEdges(entry.second)) incident[edge].push_back(entry.second);

  using Walk = local::SurfaceWalk;
  std::array<size_t, 5> walks{};
  size_t accepted_ordinary = 0, accepted_tolerance = 0;
  for (const auto& [edge, faces] : incident) {
    if (faces.size() != 2 || surface.Feature(edge)) continue;
    const auto& a = surface.Find(edge[0])->node;
    const auto& b = surface.Find(edge[1])->node;
    const auto placement = surface.Place({a, b}, {faces[0], faces[1]}, fresh);
    ++walks[static_cast<size_t>(placement.walk)];
    const auto query = Midpoint(a.p, b.p);
    if (placement.walk == Walk::Far) {
      CAPTURE(edge);
      CHECK(placement.refusal == local::SurfaceRefusal::Walk);
      std::cout << std::setprecision(17) << "FP2_B_C3 far {\"edge\":[" << edge[0] << ',' << edge[1]
                << "],\"q\":[" << query.x << ',' << query.y << ',' << query.z << "]}" << std::endl;
    }
    if (!placement.Accepted()) continue;
    const auto& binding = placement.point.binding;
    const auto point = placement.point.node.p;
    if (placement.walk == Walk::None) {
      const auto ordinal = accepted_ordinary++;
      if (ordinal >= SheetPlacementPins.size()) continue;
      const auto& pin = SheetPlacementPins[ordinal];
      CAPTURE(edge);
      CHECK(edge == pin.edge);
      CHECK(CoordinateBits(point) == pin.position);
      CHECK(CoordinateBits({binding.u, binding.v, 0.}) == pin.parameters);
      CHECK(binding.feature == pin.feature);
      CHECK(binding.version == pin.version);
      CHECK(binding.dimension == pin.dimension);
      PrintSheetPlacement("ordinary", edge, query, placement.point);
    } else if (placement.walk == Walk::Tolerance) {
      if (++accepted_tolerance > 3) continue;
      CAPTURE(edge);
      const auto& model = state.Geometry();
      const auto foot = model.Realize(binding);
      CHECK(CoordinateBits(point) == CoordinateBits(foot));
      const auto distance = std::hypot(static_cast<long double>(query.x) - point.x,
                                       static_cast<long double>(query.y) - point.y,
                                       static_cast<long double>(query.z) - point.z);
      const auto upper = std::nextafter(static_cast<double>(distance + model.ErrorBound(binding, point)),
                                       std::numeric_limits<double>::infinity());
      CHECK(upper <= surface.Tolerance() / 8);
      CHECK(model.MobilityDimension(binding, state.Mobility()) == 2);
      for (const auto& parent : faces)
        for (Id end : edge) {
          auto child = parent;
          *std::find(child.v.begin(), child.v.end(), end) = fresh;
          CHECK(surface.OnEntity(child, placement.point));
        }
      PrintSheetPlacement("tolerance", edge, query, placement.point);
    }
  }
  std::cout << "FP2_B_C3 counts {\"walk_none\":" << walks[static_cast<size_t>(Walk::None)]
            << ",\"walk_tol8\":" << walks[static_cast<size_t>(Walk::Tolerance)]
            << ",\"walk_far\":" << walks[static_cast<size_t>(Walk::Far)]
            << ",\"walk_no_start\":" << walks[static_cast<size_t>(Walk::NoStart)]
            << ",\"walk_exception\":" << walks[static_cast<size_t>(Walk::Exception)]
            << ",\"accepted_ordinary\":" << accepted_ordinary
            << ",\"accepted_tolerance\":" << accepted_tolerance << "}" << std::endl;
  REQUIRE(accepted_ordinary >= SheetPlacementPins.size());
}

#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC pop_options
#endif
