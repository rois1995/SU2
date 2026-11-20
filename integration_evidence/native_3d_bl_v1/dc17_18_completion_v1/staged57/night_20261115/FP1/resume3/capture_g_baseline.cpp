#include "catch.hpp"
#include "/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated/Common/include/adaptation/CNativeLocalCore3D.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>

namespace {
using namespace SU2Native3D;
namespace local = SU2NativeLocal3D;

Tensor ResumeSensor(Point p) {
  const double scale = 16 + 6 * p.x + 2 * p.y + 4 * p.z;
  return {scale, 3 * p.z, 3 * p.y, .5 * (scale + 1), 3 * p.x, .5 * (scale + 2)};
}

Owned ResumeBox() {
  constexpr int n = 7;
  constexpr std::array<double, n + 1> x{0., .125, .1875, .25, .25 + 1. / 128., .375, .5, 1.};
  constexpr std::array<unsigned, 6> ring{1, 2, 3, 7, 4, 5};
  auto node = [&](int i, int j, int k) {
    return Node{(Id(1) << 56) + Id((k * (n + 1) + j) * (n + 1) + i), {x[i], j / 8., k / 8.}};
  };
  Owned rows;
  for (int k = 0; k < n; ++k)
    for (int j = 0; j < n; ++j)
      for (int i = 0; i < n; ++i) {
        const std::array<Node, 8> v{node(i, j, k), node(i + 1, j, k), node(i + 1, j + 1, k), node(i, j + 1, k),
                                    node(i, j, k + 1), node(i + 1, j, k + 1), node(i + 1, j + 1, k + 1),
                                    node(i, j + 1, k + 1)};
        for (size_t t = 0; t < ring.size(); ++t) {
          const Id id = (Id(1) << 60) + 13 * rows.size();
          OwnedCell row;
          row.cell = {id, {v[0], v[6], v[ring[t]], v[ring[(t + 1) % ring.size()]]}};
          row.version = 1 + rows.size() % 17;
          row.sensor_present = true;
          auto& tet = row.cell.v;
          if (Orientation(tet[0].p, tet[1].p, tet[2].p, tet[3].p) < 0) std::swap(tet[0], tet[1]);
          for (unsigned f = 0; f < 4; ++f) {
            std::array<Point, 3> face{};
            for (unsigned p = 0, q = 0; p < 4; ++p)
              if (p != f) face[q++] = tet[p].p;
            const auto a = face[0], b = face[1], c = face[2];
            const bool physical = (a.x == b.x && a.x == c.x && (a.x == 0 || a.x == x.back())) ||
                                  (a.y == b.y && a.y == c.y && (a.y == 0 || a.y == n / 8.)) ||
                                  (a.z == b.z && a.z == c.z && (a.z == 0 || a.z == n / 8.));
            row.physical[f] = {physical, 1000 + 4 * rows.size() + f};
            row.sensor[f] = ResumeSensor(tet[f].p);
          }
          rows.emplace(id, row);
        }
      }
  return rows;
}

Id MaximumPointId(const local::Mesh& mesh) {
  Id id = 0;
  for (const auto& point : mesh.Points()) id = std::max(id, point.node.id);
  return id;
}

}  // namespace

TEST_CASE("Capture exact G F4.1 owned bytes", "[.][NativeLocalCoreG609Baseline]") {
  auto rows = ResumeBox();
  local::Mesh mesh(rows);
  local::IdSequence point_ids(MaximumPointId(mesh));
  const auto stats = mesh.RunPass(point_ids, SIZE_MAX, true);
  REQUIRE(stats.cells_in == 2058);
  REQUIRE(stats.cells_out == 2600);
  REQUIRE(stats.collapses == 74);
  REQUIRE(stats.splits == 169);
  REQUIRE(stats.edge_removals == 380);
  REQUIRE(stats.face_swaps == 12);
  REQUIRE(stats.moves == 317);
  REQUIRE(stats.deferred == 2104);
  REQUIRE(stats.skipped_memory == 1134);
  local::IdSequence cell_ids(rows.rbegin()->first);
  mesh.WriteBack(rows, cell_ids);
  REQUIRE(rows.size() == 2600);
  std::vector<char> bytes;
  RecordStream stream(bytes);
  for (const auto& entry : rows) {
    auto id = entry.first;
    auto row = entry.second;
    stream(id, row);
  }
  std::ofstream output("f4_1_build609_owned.bin", std::ios::binary);
  output.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
  output.close();
  REQUIRE(output.good());
  std::cout << "FP1_G609_BASELINE rows=" << rows.size() << " wire_bytes=" << bytes.size() << std::endl;
}
