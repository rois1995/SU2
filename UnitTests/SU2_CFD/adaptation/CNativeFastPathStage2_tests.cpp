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
