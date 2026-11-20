/*!
 * \file CNativeLocalCore3DResumeOption_tests.cpp
 * \brief FP1 wired interior edits, border shifts and repeatable publication.
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md).
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */
#include "TransferTestCase.hpp"
#include "../../../Common/include/adaptation/CNativeReferenceIO3D.hpp"
#include "../../../Common/include/adaptation/CNativeRemesher3D.hpp"
#include "../../../SU2_CFD/include/adaptation/CConservativeTransfer.hpp"

#include <algorithm>
#include <array>
#include <cstring>
#include <iostream>
#include <map>
#include <sstream>
#include <tuple>
#include <utility>

namespace {
using namespace SU2Native3D;
using namespace transfer_test;

CSimplexMesh InteriorControls() {
  constexpr unsigned long n = 6;
  auto mesh = BoxMesh(3, n, false);
  const auto original_points = mesh.GetnPoint();
  std::vector<unsigned long> cells;
  for (unsigned long e = 0; e < mesh.GetnElem(); ++e) {
    const auto first = mesh.elem.begin() + 4 * e;
    const auto box = e / 6;
    const auto x = box % n, y = box / n % n, z = box / (n * n);
    if (e % 6 != 0 || x == 0 || x == n - 1 || y == 0 || y == n - 1 || (z != 0 && z != 3)) {
      cells.insert(cells.end(), first, first + 4);
      continue;
    }
    // The deep stars can collapse; the bottom stars are wall-near and leave thin cells for quality work.
    const auto center = mesh.GetnPoint();
    for (unsigned d = 0; d < 3; ++d) {
      const auto coord = [&](unsigned i) { return mesh.coord[3 * first[i] + d]; };
      mesh.coord.push_back((61 * coord(0) + coord(1) + coord(2) + coord(3)) / 64);
    }
    std::array<unsigned long, 4> v{};
    std::copy_n(first, 4, v.begin());
    for (unsigned i = 0; i < 4; ++i) {
      auto child = v;
      child[i] = center;
      cells.insert(cells.end(), child.begin(), child.end());
    }
  }
  mesh.elem = std::move(cells);
  mesh.elemRef.assign(mesh.GetnElem(), 0);
  const auto new_points = mesh.GetnPoint() - original_points;
  // Give inserted stars the first IDs so the core considers their centers before their ordinary neighbors.
  std::rotate(mesh.coord.begin(), mesh.coord.begin() + 3 * original_points, mesh.coord.end());
  const auto renumber = [&](auto& connectivity) {
    for (auto& id : connectivity) id = id < original_points ? id + new_points : id - original_points;
  };
  renumber(mesh.elem);
  for (auto& marker : mesh.markers) renumber(marker.elem);
  return mesh;
}

enum Count { Collapse, Removal, Swap, Move, LowerSent, LowerReceived, HigherSent, HigherReceived, CountSize };

void CheckCoreRows(World& world, const std::string& captured) {
  std::array<uint64_t, CountSize> local{}, total{};
  std::istringstream lines(captured);
  for (std::string line; std::getline(lines, line);) {
    std::istringstream words(line);
    std::string tag;
    words >> tag;
    if (tag != "NATIVE3D_LOCAL_CORE" && tag != "NATIVE3D_LOCAL_SHIFT") continue;
    std::cout << line << std::endl;
    std::map<std::string, std::string> fields;
    for (std::string word; words >> word;) {
      const auto separator = word.find('=');
      if (separator != std::string::npos) fields.emplace(word.substr(0, separator), word.substr(separator + 1));
    }
    const auto number = [&](const char* key) -> uint64_t { return std::stoull(fields.at(key)); };
    if (tag == "NATIVE3D_LOCAL_CORE") {
      CHECK(fields.at("applicable") == "1");
      if (fields.at("applicable") != "1") continue;
      local[Collapse] += number("collapses");
      local[Removal] += number("edge_removals");
      local[Swap] += number("face_swaps");
      local[Move] += number("moves");
    } else {
      const auto& direction = fields.at("direction");
      CHECK((direction == "lower" || direction == "higher"));
      local[direction == "lower" ? LowerSent : HigherSent] += number("sent");
      local[direction == "lower" ? LowerReceived : HigherReceived] += number("received");
    }
  }
  CPassiveComm::Allreduce(local.data(), total.data(), total.size(), CPassiveComm::Op::SUM, world.comm);
  CHECK(total[Collapse] > 0);
  CHECK(total[Removal] + total[Swap] + total[Move] > 0);
  if (world.size > 1) {
    CHECK(total[LowerSent] > 0);
    CHECK(total[LowerReceived] > 0);
    CHECK(total[HigherSent] > 0);
    CHECK(total[HigherReceived] > 0);
  }
}

struct Published {
  CReaderSlices mesh;
  std::vector<std::string> markers;
  std::string reference;
  std::vector<passivedouble> flow;
};

Published WiredRun(World& world, const CSimplexMesh& seed) {
  const std::string options = "SOLVER= EULER\nCOMPUTE_METRIC= YES\nADAP_SENSOR= PRESSURE\n"
                              "ADAP_REMESHER= NATIVE_CAVITY\nADAP_SURFACE= YES\nADAP_HESSIAN_NOISE= 0\n"
                              "ADAP_NATIVE_LOCAL_CORE= YES\nWRT_PERFORMANCE= YES\nADAP_NATIVE_REPARTITION= " +
                              std::string(world.size > 1 ? "YES\n" : "NO\n");
  const auto config = MakeConfig(3, options);
  MeshSolution donor(config.get(), seed, 0);
  const Field field = [](const su2double*, su2double* state) {
    const std::array<double, 5> values{1.2, 400., -30., 12., 2.5e5};
    std::copy(values.begin(), values.end(), state);
  };
  donor.SetField(FLOW_SOL, field);
  su2activematrix sensor(donor.Fine().GetnPointDomain(), 6);
  for (size_t p = 0; p < sensor.rows(); ++p) {
    const auto* x = donor.Fine().nodes->GetCoord(p);
    const auto scale = 35. + .5 * (x[0] + x[1] + x[2]);
    const std::array<su2double, 6> metric{scale, .125 * x[2], 0., scale, 0., scale};
    for (size_t k = 0; k < metric.size(); ++k) sensor(p, k) = metric[k];
  }
  const auto reference = std::make_shared<ReferenceState>();
  CNativeRemesher3D remesher(reference);
  CRemeshResult result;
  std::ostringstream captured;
  {
    struct Capture {
      std::streambuf* previous;
      explicit Capture(std::ostream& sink) : previous(std::cout.rdbuf(sink.rdbuf())) {}
      ~Capture() { std::cout.rdbuf(previous); }
    } capture(captured);
    result = remesher.Remesh(*config, donor.Fine(), sensor);
  }
  const auto log = captured.str();
  INFO(log);
  REQUIRE(result.status == CRemeshResult::Status::COMPLETE);
  REQUIRE(result.onAccepted);
  CHECK(remesher.Statistics().workers == world.size);
  CheckCoreRows(world, log);

  MeshSolution target(config.get(), result.slices, 0);
  CConservativeTransfer transfer;
  {
    Mute mute;
    transfer.Transfer(config.get(), donor.Donor(), target.geometry, target.solver);
  }
  result.onAccepted();
  REQUIRE(reference->original);
  std::vector<passivedouble> flow;
  passivedouble error = 0;
  const auto* nodes = target.solver[MESH_0][FLOW_SOL]->GetNodes();
  for (unsigned long p = 0; p < target.Fine().GetnPointDomain(); ++p) {
    su2double expected[MAXVAR]{};
    field(target.Fine().nodes->GetCoord(p), expected);
    for (unsigned short k = 0; k < 5; ++k) {
      const auto value = nodes->GetSolution(p, k);
      flow.push_back(SU2_TYPE::GetValue(value));
      error = std::max(error, RelDiff(value, expected[k]));
    }
  }
  CHECK(CPassiveComm::Allreduce(error, CPassiveComm::Op::MAX, world.comm) < 2e-10);
  return {std::move(result.slices), std::move(result.markers), EncodeReference(*reference), std::move(flow)};
}

bool SameBits(const std::vector<passivedouble>& a, const std::vector<passivedouble>& b) {
  return a.size() == b.size() && (a.empty() || std::memcmp(a.data(), b.data(), a.size() * sizeof(a.front())) == 0);
}
}  // namespace

TEST_CASE("Wired native local core repeats collapse, quality, shift and transfer on a nonuniform box",
          "[.][NativeLocalCore3DResumeOption]") {
  World world;
  const auto seed = InteriorControls();
  const auto first = WiredRun(world, seed);
  const auto second = WiredRun(world, seed);
  const auto count = [](const CReaderSlices& r) {
    return std::tie(r.nDim, r.nPointGlobal, r.nElemGlobal, r.firstPoint, r.nPointLocal, r.nElemLocal, r.nMetric);
  };
  CHECK(count(first.mesh) == count(second.mesh));
  REQUIRE(first.mesh.coord.size() == second.mesh.coord.size());
  for (size_t d = 0; d < first.mesh.coord.size(); ++d) CHECK(SameBits(first.mesh.coord[d], second.mesh.coord[d]));
  CHECK(SameBits(first.mesh.metric, second.mesh.metric));
  CHECK(first.mesh.elemRows == second.mesh.elemRows);
  CHECK(first.mesh.boundaryRows == second.mesh.boundaryRows);
  CHECK(first.mesh.markerNames == second.mesh.markerNames);
  CHECK(first.mesh.markersWithElements == second.mesh.markersWithElements);
  CHECK(first.markers == second.markers);
  CHECK(first.reference == second.reference);
  CHECK(SameBits(first.flow, second.flow));
}
