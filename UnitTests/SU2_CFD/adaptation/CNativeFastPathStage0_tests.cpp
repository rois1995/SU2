/*!
 * \file CNativeFastPathStage0_tests.cpp
 * \brief Test-only FP0 measurements of native remeshing on a frozen ONERA M6 target.
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md).
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */
#include "TransferTestCase.hpp"
#include "../../../Common/include/adaptation/CNativeBoundaryParameters3D.hpp"
#include "../../../Common/include/adaptation/CNativeCurvedAtlas3D.hpp"
#include "../../../Common/include/adaptation/CNativeCurvedReference.hpp"
#include "../../../Common/include/adaptation/CNativeImport3D.hpp"
#include "../../../Common/include/adaptation/CNativeMetric3D.hpp"
#include "../../../Common/include/adaptation/CNativeOptimize3D.hpp"
#include "../../../Common/include/adaptation/CNativeReferenceIO3D.hpp"
#include "../../../SU2_CFD/include/drivers/CSinglezoneDriver.hpp"
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <limits>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

namespace {
using namespace SU2Native3D;
using Clock = std::chrono::steady_clock;
using Index = uint32_t;
using TetIndices = std::array<Index, 4>;
using PairIndices = std::array<Index, 2>;

struct FlatPoint {
  Node node;
  Tensor metric;
  bool surface = false;
};

struct FlatMesh {
  std::vector<FlatPoint> points;
  std::vector<TetIndices> cells;
  std::vector<TetIndices> neighbors;
  std::vector<std::array<int, 4>> markers;
};

struct LoadedMesh {
  ImportedGeometry imported;
  ReferenceState reference;
  double import_seconds = 0;
};

double Seconds(Clock::time_point start) { return std::chrono::duration<double>(Clock::now() - start).count(); }

void Output(const char* name) { std::cout << std::setprecision(17) << "FP0 " << name << " {"; }

void ReportError(const char* row, Clock::time_point start, const std::exception& error) {
  std::string message(error.what());
  std::replace(message.begin(), message.end(), '\n', ' ');
  std::replace(message.begin(), message.end(), '\r', ' ');
  Output(row);
  std::cout << "\"seconds\":" << Seconds(start) << ",\"error\":" << std::quoted(message) << "}" << std::endl;
}

class FP0Driver : public CSinglezoneDriver {
 public:
  using CSinglezoneDriver::CSinglezoneDriver;
  CConfig& Config() { return *config_container[ZONE_0]; }
  CGeometry& Geometry() { return *geometry_container[ZONE_0][INST_0][MESH_0]; }
};

su2activematrix ReadFrozenSensor(CGeometry& geometry, const char* path) {
  std::ifstream input(path, std::ios::binary);
  uint64_t count = 0;
  input.read(reinterpret_cast<char*>(&count), sizeof(count));
  REQUIRE(count == 146676);
  REQUIRE(count == geometry.GetnPointDomain());
  std::vector<std::array<double, 12>> rows(count);
  for (uint64_t i = 0; i < count; ++i) {
    uint64_t id = count;
    input.read(reinterpret_cast<char*>(&id), sizeof(id));
    input.read(reinterpret_cast<char*>(rows[i].data()), sizeof(rows[i]));
    if (!input || id != i) throw std::runtime_error("Invalid FP0 frozen sensor record.");
  }
  REQUIRE(input.peek() == std::char_traits<char>::eof());
  constexpr std::array<size_t, 6> upper{0, 1, 2, 4, 5, 8};
  su2activematrix result(count, upper.size());
  for (size_t i = 0; i < count; ++i) {
    const auto& row = rows.at(geometry.nodes->GetGlobalIndex(i));
    for (size_t d = 0; d < 3; ++d)
      if (row[d] != SU2_TYPE::GetValue(geometry.nodes->GetCoord(i, d)))
        throw std::runtime_error("FP0 frozen coordinate differs from the CFD point.");
    for (size_t k = 0; k < upper.size(); ++k) result(i, k) = row[3 + upper[k]];
  }
  return result;
}

LoadedMesh LoadMesh() {
  REQUIRE(SU2_MPI::GetSize() == 1);
  const char* config = std::getenv("SU2_NATIVE_FROZEN_CONFIG");
  const char* sensor = std::getenv("SU2_NATIVE_FROZEN_SENSOR3D");
  REQUIRE(config);
  REQUIRE(sensor);
  std::string config_path(config);
  FP0Driver driver(config_path.data(), 1, SU2_MPI::GetComm());
  auto& geometry = driver.Geometry();
  auto metric = ReadFrozenSensor(geometry, sensor);
  LoadedMesh result;
  std::string reason;
  bool loaded = false;
  size_t reference_bytes = 0;
  const auto ceiling = driver.Config().GetAdap_Native_MemoryLimit();
  const auto path = driver.Config().GetMesh_FileName() + ".native_ref";
  const bool restored = LoadReference(path, result.reference, loaded, reason, ceiling, 0, reference_bytes);
  INFO(reason);
  REQUIRE(restored);
  REQUIRE(loaded);
  REQUIRE(result.reference.original);
  REQUIRE(result.reference.original->EntityGeometry());
  BridgeStats stats;
  const auto start = Clock::now();
  const bool imported =
      ImportGeometry(driver.Config(), geometry, &metric, result.reference, result.imported, reason, stats, ceiling);
  result.import_seconds = Seconds(start);
  INFO(reason);
  REQUIRE(imported);
  REQUIRE(result.imported.cells.size() == 726442);
  driver.Finalize();
  return result;
}

template <class T>
size_t Bytes(const std::vector<T>& values) {
  return values.capacity() * sizeof(T);
}

std::array<double, 6> TensorValues(const Tensor& m) { return {m.xx, m.xy, m.xz, m.yy, m.yz, m.zz}; }

void CopyPoints(FlatMesh& mesh, const Owned& cells) {
  mesh.points.reserve(146676);
  mesh.cells.reserve(cells.size());
  std::unordered_map<Id, Index> index;
  index.reserve(146676);
  for (const auto& [id, owned] : cells) {
    (void)id;
    TetIndices indices{};
    if (!owned.sensor_present) throw std::runtime_error("FP0 import is missing an owned sensor.");
    for (size_t k = 0; k < 4; ++k) {
      const auto& node = owned.cell.v[k];
      const auto [where, inserted] = index.emplace(node.id, mesh.points.size());
      indices[k] = where->second;
      if (inserted) {
        mesh.points.push_back({node, owned.sensor[k], false});
      } else {
        const auto& old = mesh.points[where->second];
        if (std::tie(old.node.p.x, old.node.p.y, old.node.p.z) != std::tie(node.p.x, node.p.y, node.p.z) ||
            TensorValues(old.metric) != TensorValues(owned.sensor[k]))
          throw std::runtime_error("FP0 import has inconsistent nodal copies.");
      }
    }
    mesh.cells.push_back(indices);
  }
  REQUIRE(mesh.points.size() == 146676);
}

struct FaceUse {
  std::array<Index, 3> key;
  Index cell;
  unsigned slot;
};

size_t CopyFaces(FlatMesh& mesh, const Owned& cells, const PlanarReference& reference) {
  constexpr auto outside = std::numeric_limits<Index>::max();
  mesh.neighbors.resize(cells.size(), {outside, outside, outside, outside});
  mesh.markers.resize(cells.size(), {-1, -1, -1, -1});
  std::vector<FaceUse> faces;
  faces.reserve(4 * cells.size());
  Index cell = 0;
  size_t surface = 0;
  for (const auto& entry : cells) {
    for (unsigned slot = 0; slot < 4; ++slot) {
      std::array<Index, 3> key{};
      for (unsigned k = 0, j = 0; k < 4; ++k)
        if (k != slot) key[j++] = mesh.cells[cell][k];
      std::sort(key.begin(), key.end());
      const auto& binding = entry.second.physical[slot];
      if (binding.present) {
        mesh.markers[cell][slot] = reference.Facets().at(binding.facet).marker;
        for (const auto point : key) mesh.points[point].surface = true;
        ++surface;
      }
      faces.push_back({key, cell, slot});
    }
    ++cell;
  }
  std::sort(faces.begin(), faces.end(), [](const auto& a, const auto& b) { return a.key < b.key; });
  for (size_t i = 0, j = 0; i < faces.size(); i = j) {
    j = i + 1;
    while (j < faces.size() && faces[i].key == faces[j].key) ++j;
    const auto& a = faces[i];
    if (j == i + 2) {
      const auto& b = faces[i + 1];
      mesh.neighbors[a.cell][a.slot] = b.cell;
      mesh.neighbors[b.cell][b.slot] = a.cell;
    } else if (j != i + 1 || mesh.markers[a.cell][a.slot] < 0) {
      throw std::runtime_error("FP0 face incidence has a non-manifold or unmarked outside face.");
    }
  }
  REQUIRE(surface == 67170);
  return surface;
}

FlatMesh Flatten(const LoadedMesh& input) {
  FlatMesh mesh;
  const auto start = Clock::now();
  CopyPoints(mesh, input.imported.cells);
  const auto points_seconds = Seconds(start);
  const auto faces_start = Clock::now();
  const auto surfaces = CopyFaces(mesh, input.imported.cells, *input.reference.original);
  const auto faces_seconds = Seconds(faces_start);
  const auto bytes = Bytes(mesh.points) + Bytes(mesh.cells) + Bytes(mesh.neighbors) + Bytes(mesh.markers);
  Output("A1");
  std::cout << "\"import_seconds\":" << input.import_seconds << ",\"point_cell_seconds\":" << points_seconds
            << ",\"face_seconds\":" << faces_seconds << ",\"points\":" << mesh.points.size()
            << ",\"cells\":" << mesh.cells.size() << ",\"surface_faces\":" << surfaces << ",\"bytes\":" << bytes
            << ",\"point_bytes\":" << Bytes(mesh.points) << ",\"cell_bytes\":" << Bytes(mesh.cells)
            << ",\"neighbor_bytes\":" << Bytes(mesh.neighbors) << ",\"marker_bytes\":" << Bytes(mesh.markers)
            << ",\"bytes_per_cell\":" << double(bytes) / mesh.cells.size() << "}" << std::endl;
  return mesh;
}

template <class F>
void EachEdge(const TetIndices& tet, F visit) {
  for (size_t i = 0; i < 4; ++i)
    for (size_t j = i + 1; j < 4; ++j) visit(PairIndices{std::min(tet[i], tet[j]), std::max(tet[i], tet[j])});
}

struct EdgeUse {
  PairIndices edge;
  Index cell;
};
struct EdgeRange {
  PairIndices edge;
  size_t first, last;
};
struct Rings {
  std::vector<EdgeUse> uses;
  std::vector<EdgeRange> edges;
};

Rings BuildRings(const FlatMesh& mesh) {
  Rings rings;
  rings.uses.reserve(6 * mesh.cells.size());
  for (Index i = 0; i < mesh.cells.size(); ++i)
    EachEdge(mesh.cells[i], [&](PairIndices edge) { rings.uses.push_back({edge, i}); });
  std::sort(rings.uses.begin(), rings.uses.end(),
            [](const auto& a, const auto& b) { return std::tie(a.edge, a.cell) < std::tie(b.edge, b.cell); });
  for (size_t i = 0, j = 0; i < rings.uses.size(); i = j) {
    const auto edge = rings.uses[i].edge;
    j = i + 1;
    while (j < rings.uses.size() && rings.uses[j].edge == edge) ++j;
    rings.edges.push_back({edge, i, j});
  }
  return rings;
}

Rings Neighborhoods(const FlatMesh& mesh) {
  auto start = Clock::now();
  std::vector<size_t> offsets(mesh.points.size() + 1);
  for (const auto& tet : mesh.cells)
    for (Index point : tet) ++offsets[point + 1];
  for (size_t i = 1; i < offsets.size(); ++i) offsets[i] += offsets[i - 1];
  auto cursor = offsets;
  std::vector<Index> stars(offsets.back());
  for (Index i = 0; i < mesh.cells.size(); ++i)
    for (Index point : mesh.cells[i]) stars[cursor[point]++] = i;
  const double stars_seconds = Seconds(start);
  start = Clock::now();
  auto rings = BuildRings(mesh);
  const double rings_seconds = Seconds(start);
  size_t max_star = 0, max_ring = 0;
  for (size_t i = 1; i < offsets.size(); ++i) max_star = std::max(max_star, offsets[i] - offsets[i - 1]);
  for (const auto& range : rings.edges) max_ring = std::max(max_ring, range.last - range.first);
  Output("A6");
  std::cout << "\"points\":" << mesh.points.size() << ",\"point_cell_incidence\":" << stars.size()
            << ",\"max_star\":" << max_star << ",\"edges\":" << rings.edges.size() << ",\"max_ring\":" << max_ring
            << ",\"stars_seconds\":" << stars_seconds << ",\"rings_seconds\":" << rings_seconds
            << ",\"seconds\":" << stars_seconds + rings_seconds
            << ",\"ns_per_ring\":" << (1e9 * rings_seconds / rings.edges.size()) << "}" << std::endl;
  return rings;
}

Tetrahedron Tet(const FlatMesh& mesh, const TetIndices& cell) {
  return {mesh.points[cell[0]].node.p, mesh.points[cell[1]].node.p, mesh.points[cell[2]].node.p,
          mesh.points[cell[3]].node.p};
}

Tensor MeanTensor(const FlatMesh& mesh, const TetIndices& cell) {
  std::array<double, 6> mean{};
  for (const auto i : cell) {
    const auto tensor = TensorValues(mesh.points[i].metric);
    for (size_t k = 0; k < mean.size(); ++k) mean[k] += .25 * tensor[k];
  }
  return {mean[0], mean[1], mean[2], mean[3], mean[4], mean[5]};
}

Point Difference(Point b, Point a) { return {b.x - a.x, b.y - a.y, b.z - a.z}; }

double FloatOrientation(Point a, Point b, Point c, Point d) {
  const auto u = Difference(b, a), v = Difference(c, a), w = Difference(d, a);
  return u.x * (v.y * w.z - v.z * w.y) + u.y * (v.z * w.x - v.x * w.z) + u.z * (v.x * w.y - v.y * w.x);
}

double NormSquared(Point a, Point b, const Tensor& m) {
  const auto d = Difference(b, a);
  return d.x * (m.xx * d.x + m.xy * d.y + m.xz * d.z) + d.y * (m.xy * d.x + m.yy * d.y + m.yz * d.z) +
         d.z * (m.xz * d.x + m.yz * d.y + m.zz * d.z);
}

template <class T>
T ClosedLength(T a, T b) {
  const auto larger = std::max(a, b), ratio = std::min(a, b) / larger;
  return larger * (T(2) / T(3)) * (1 + ratio + ratio * ratio) / (1 + ratio);
}

double FloatLength(const FlatPoint& a, const FlatPoint& b) {
  return ClosedLength(std::sqrt(NormSquared(a.node.p, b.node.p, a.metric)),
                      std::sqrt(NormSquared(a.node.p, b.node.p, b.metric)));
}

long double StrictLength(const FlatPoint& a, const FlatPoint& b) {
  return ClosedLength(MetricNorm(a.node.p, b.node.p, a.metric), MetricNorm(a.node.p, b.node.p, b.metric));
}

std::array<double, 2> FloatShape(const Tetrahedron& tet, const Tensor& m) {
  const double det =
      m.xx * (m.yy * m.zz - m.yz * m.yz) - m.xy * (m.xy * m.zz - m.xz * m.yz) + m.xz * (m.xy * m.yz - m.xz * m.yy);
  const double volume = FloatOrientation(tet[0], tet[1], tet[2], tet[3]) * std::sqrt(det);
  const auto root = std::cbrt(std::abs(volume) / 2);
  double sum = 0;
  std::array<double, 4> product{1, 1, 1, 1};
  for (size_t i = 0; i < 4; ++i)
    for (size_t j = i + 1; j < 4; ++j) {
      const auto squared = NormSquared(tet[i], tet[j], m);
      sum += squared;
      product[i] *= squared;
      product[j] *= squared;
    }
  const auto sign = std::copysign(1., volume);
  const double largest = *std::max_element(product.begin(), product.end());
  return {sign * 12 * root * root / sum, sign * std::min(1., std::sqrt(2.) * std::abs(volume) / std::sqrt(largest))};
}

void KernelMeasurements(const FlatMesh& mesh, const Rings& rings) {
  std::vector<bool> strict_sign(mesh.cells.size());
  std::vector<MetricMeasures> strict_shape(mesh.cells.size());
  std::vector<long double> strict_length(rings.edges.size());
  KernelStats stats;
  auto start = Clock::now();
  for (size_t i = 0; i < mesh.cells.size(); ++i) {
    const auto p = Tet(mesh, mesh.cells[i]);
    strict_sign[i] = Orientation(p[0], p[1], p[2], p[3], &stats) > 0;
  }
  const double strict_orientation_seconds = Seconds(start);
  start = Clock::now();
  for (size_t i = 0; i < mesh.cells.size(); ++i)
    strict_shape[i] = Measure(Tet(mesh, mesh.cells[i]), MeanTensor(mesh, mesh.cells[i]));
  const double strict_shape_seconds = Seconds(start);
  start = Clock::now();
  for (size_t i = 0; i < rings.edges.size(); ++i) {
    const auto& edge = rings.edges[i].edge;
    strict_length[i] = StrictLength(mesh.points[edge[0]], mesh.points[edge[1]]);
  }
  const double strict_edge_seconds = Seconds(start);
  Output("A2");
  std::cout << "\"cells\":" << mesh.cells.size() << ",\"edges\":" << rings.edges.size()
            << ",\"orientation_seconds\":" << strict_orientation_seconds
            << ",\"shape_seconds\":" << strict_shape_seconds << ",\"edge_seconds\":" << strict_edge_seconds
            << ",\"filtered\":" << stats.filtered_orientations << ",\"exact\":" << stats.exact_orientations << "}"
            << std::endl;

  size_t signs = 0;
  double max_q = 0, max_j = 0;
  long double max_length = 0;
  start = Clock::now();
  for (size_t i = 0; i < mesh.cells.size(); ++i) {
    const auto p = Tet(mesh, mesh.cells[i]);
    signs += strict_sign[i] != (FloatOrientation(p[0], p[1], p[2], p[3]) > 0);
  }
  const double float_orientation_seconds = Seconds(start);
  start = Clock::now();
  for (size_t i = 0; i < mesh.cells.size(); ++i) {
    const auto m = FloatShape(Tet(mesh, mesh.cells[i]), MeanTensor(mesh, mesh.cells[i]));
    max_q = std::max(max_q, std::abs(m[0] - strict_shape[i].mean_ratio));
    max_j = std::max(max_j, std::abs(m[1] - strict_shape[i].minimum_scaled_jacobian));
  }
  const double float_shape_seconds = Seconds(start);
  start = Clock::now();
  for (size_t i = 0; i < rings.edges.size(); ++i) {
    const auto& e = rings.edges[i].edge;
    max_length = std::max(max_length, std::abs(FloatLength(mesh.points[e[0]], mesh.points[e[1]]) - strict_length[i]));
  }
  const double float_edge_seconds = Seconds(start);
  Output("A3");
  std::cout << "\"cells\":" << mesh.cells.size() << ",\"edges\":" << rings.edges.size()
            << ",\"orientation_seconds\":" << float_orientation_seconds << ",\"shape_seconds\":" << float_shape_seconds
            << ",\"edge_seconds\":" << float_edge_seconds << ",\"sign_differences\":" << signs
            << ",\"max_q_difference\":" << max_q << ",\"max_j_difference\":" << max_j
            << ",\"max_length_difference\":" << max_length << "}" << std::endl;
}

DonorCell Donor(const FlatMesh& mesh, const TetIndices& cell, Id id) {
  DonorCell donor;
  donor.cell.id = id;
  for (size_t k = 0; k < cell.size(); ++k) {
    donor.cell.v[k] = mesh.points[cell[k]].node;
    donor.sensor[k] = mesh.points[cell[k]].metric;
  }
  return donor;
}

void CompareSampledEdges(const FlatMesh& mesh, const Rings& rings) {
  constexpr size_t count = 1000;
  REQUIRE(rings.edges.size() >= count);
  long double max_relative = 0;
  const auto start = Clock::now();
  for (size_t i = 0; i < count; ++i) {
    const auto& range = rings.edges[i * rings.edges.size() / count];
    const Index cell = rings.uses[range.first].cell;
    FrozenField field({Donor(mesh, mesh.cells[cell], cell)}, {}, false, {}, {}, {}, true, MaximumDonors, true);
    const auto& a = mesh.points[range.edge[0]];
    const auto& b = mesh.points[range.edge[1]];
    const auto expected = field.TargetEdgeLength(a.node.p, b.node.p);
    max_relative = std::max(max_relative, std::abs(StrictLength(a, b) - expected) / expected);
  }
  Output("A4");
  std::cout << "\"samples\":" << count << ",\"seconds\":" << Seconds(start)
            << ",\"max_relative_difference\":" << max_relative << "}" << std::endl;
}

struct SurfaceCount {
  size_t surface = 0, interior = 0;
  void Add(bool touches) {
    if (touches)
      ++surface;
    else
      ++interior;
  }
};

void ScanMesh(const char* name, const FlatMesh& mesh, const Rings& rings, Clock::time_point start) {
  std::array<SurfaceCount, 3> edge_counts{};
  std::array<SurfaceCount, 4> cell_counts{};
  std::vector<bool> long_cells(mesh.cells.size());
  for (const auto& range : rings.edges) {
    const auto& a = mesh.points[range.edge[0]];
    const auto& b = mesh.points[range.edge[1]];
    const bool surface = a.surface || b.surface;
    const double length = FloatLength(a, b);
    if (length < .9) edge_counts[1].Add(surface);
    if (length < .5) edge_counts[2].Add(surface);
    if (length <= 1.8) continue;
    edge_counts[0].Add(surface);
    for (size_t i = range.first; i < range.last; ++i) long_cells[rings.uses[i].cell] = true;
  }
  for (size_t i = 0; i < mesh.cells.size(); ++i) {
    const auto& cell = mesh.cells[i];
    const auto shape = FloatShape(Tet(mesh, cell), MeanTensor(mesh, cell));
    const bool surface = std::any_of(cell.begin(), cell.end(), [&](auto v) { return mesh.points[v].surface; });
    const bool q = shape[0] < .20, j = shape[1] < .05;
    if (q) cell_counts[0].Add(surface);
    if (j) cell_counts[1].Add(surface);
    if (q || j) cell_counts[2].Add(surface);
    if (long_cells[i]) cell_counts[3].Add(surface);
  }
  const auto seconds = Seconds(start);
  auto print_count = [](const char* key, const SurfaceCount& value) {
    std::cout << ",\"" << key << "\":[" << value.surface + value.interior << ',' << value.surface << ','
              << value.interior << ']';
  };
  Output(name);
  std::cout << "\"points\":" << mesh.points.size() << ",\"cells\":" << mesh.cells.size()
            << ",\"edges\":" << rings.edges.size() << ",\"seconds\":" << seconds;
  print_count("long_edges", edge_counts[0]);
  print_count("short_edges_09", edge_counts[1]);
  print_count("short_edges_05", edge_counts[2]);
  print_count("q_bad", cell_counts[0]);
  print_count("j_bad", cell_counts[1]);
  print_count("shape_bad", cell_counts[2]);
  print_count("long_cells", cell_counts[3]);
  std::cout << "}" << std::endl;
}

void PrintScan(const char* name, const FlatMesh& mesh) {
  const auto start = Clock::now();
  const auto rings = BuildRings(mesh);
  ScanMesh(name, mesh, rings, start);
}

size_t CountLongCells(const std::vector<Cell>& batch, const PatchMetric& measurement) {
  size_t result = 0;
  for (const auto& cell : batch) {
    bool has_long = false;
    for (size_t a = 0; a < 4; ++a)
      for (size_t b = a + 1; b < 4; ++b) {
        const auto u = cell.v[a].id, v = cell.v[b].id;
        has_long |= measurement.edges.at(Edge{std::min(u, v), std::max(u, v)}).length > 1.8L;
      }
    result += has_long;
  }
  return result;
}

void StrictCompleteMesh(const Owned& owned) {
  const auto start = Clock::now();
  size_t measured = 0, q_bad = 0, j_bad = 0, long_cells = 0;
  try {
    for (auto it = owned.begin(); it != owned.end();) {
      std::vector<Cell> batch;
      std::vector<DonorCell> donors;
      batch.reserve(128);
      donors.reserve(128);
      while (it != owned.end() && batch.size() < 128) {
        const auto& row = it++->second;
        batch.push_back(row.cell);
        donors.push_back({row.cell, row.sensor});
      }
      FrozenField field(std::move(donors), {}, false, {}, {}, {}, true, MaximumDonors, true);
      const auto value = MeasureMetricCells(batch, field);
      measured += value.shape_cells;
      q_bad += value.q_bad_cells;
      j_bad += value.j_bad_cells;
      long_cells += CountLongCells(batch, value);
    }
  } catch (const std::exception& error) {
    Output("A10_progress");
    std::cout << "\"cells\":" << measured << ",\"seconds\":" << Seconds(start) << "}" << std::endl;
    ReportError("A10", start, error);
    return;
  }
  Output("A10");
  std::cout << "\"cells\":" << measured << ",\"seconds\":" << Seconds(start) << ",\"q_bad\":" << q_bad
            << ",\"j_bad\":" << j_bad << ",\"long_cells\":" << long_cells << "}" << std::endl;
}

#include "fixtures/native_fast_path_edits.inc"
#include "fixtures/native_fast_path_proofs.inc"
}  // namespace

TEST_CASE("FP0 flat mesh and kernels on saved M6 metric", "[.][NativeFastPathStage0][NativeFP0Kernels]") {
  const auto loaded = LoadMesh();
  auto mesh = Flatten(loaded);
  {
    const auto rings = Neighborhoods(mesh);
    KernelMeasurements(mesh, rings);
    CompareSampledEdges(mesh, rings);
    ScanMesh("A5", mesh, rings, Clock::now());
  }
  RunPrototypeEdits(std::move(mesh));
}

TEST_CASE("FP0 full strict metric census", "[.][NativeFastPathStage0][NativeFP0Census]") {
  StrictCompleteMesh(LoadMesh().imported.cells);
}

TEST_CASE("FP0 existing optimizer around a bad M6 vertex", "[.][NativeFastPathStage0][NativeFP0Optimize]") {
  const auto* count = std::getenv("SU2_NATIVE_FP0_OPTIMIZER_CELLS");
  REQUIRE(count);
  const auto desired = std::stoull(count);
  REQUIRE((desired == 10000 || desired == 100000));
  const auto mesh = LoadMesh();
  Output("A9_begin");
  std::cout << "\"desired\":" << desired << "}" << std::endl;
  const auto start = Clock::now();
  try {
    RunOptimizer(mesh.imported, *mesh.reference.original, desired);
  } catch (const std::exception& error) {
    ReportError("A9", start, error);
  }
}

TEST_CASE("FP0 existing strict M6 surface proof", "[.][NativeFastPathStage0][NativeFP0Proof]") {
  const auto mesh = LoadMesh();
  const auto start = Clock::now();
  try {
    RunSurfaceProof(*mesh.reference.original);
  } catch (const std::exception& error) {
    ReportError("A11", start, error);
  }
}

TEST_CASE("FP0 double cubic patch evaluation on M6", "[.][NativeFastPathStage0][NativeFP0FloatSurface]") {
  const auto mesh = LoadMesh();
  RunFloatSurface(*mesh.reference.original);
}
