/*!
 * \file CNativeImport2D_tests.cpp
 * \brief Native physical association across actual SU2 repartitioning and reader renumbering.
 * \version 8.5.0 "Harrier"
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md)
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */

#include "TransferTestCase.hpp"
#include "../../../Common/include/adaptation/CNativeImport2D.hpp"
#include "../../../Common/include/geometry/meshreader/CDistributedMemoryMeshReaderFVM.hpp"

using namespace SU2NativeBoundary2D;
using namespace transfer_test;

TEST_CASE("Native MPI import: physical labels and immutable features survive SU2 rebuild", "[NativeImport2D]") {
  World world;
  const auto input = BoxMesh(2, 3, true);
  auto config = MakeConfig(2, "SOLVER= EULER\n");
  std::map<Id, Cell> owned;
  std::vector<BoundaryLabel> labels;
  std::vector<std::string> names;
  for (const auto& marker : input.markers) names.push_back(marker.name);
  const int owners = std::max(1, world.size - 1);
  auto node = [&](unsigned long id) {
    return Node{(Id(1) << 40) + id * 97, {input.coord[2 * id], input.coord[2 * id + 1]}, 0};
  };
  for (unsigned long e = 0; e < input.GetnElem(); ++e)
    if (int(e % owners) == world.rank) {
      Cell cell;
      cell.t = triangle(node(input.elem[3 * e]), node(input.elem[3 * e + 1]), node(input.elem[3 * e + 2]));
      cell.t.id = 9000 + e;
      for (int k = 0; k < 3; ++k) {
        const auto p = cell.t.v[k].p;
        cell.nodal_target[k] = {4 + p.x + 2 * p.y, .1, 8 + 3 * p.x + p.y};
      }
      owned.emplace(cell.t.id, cell);
    }
  for (size_t m = 0; m < input.markers.size(); ++m) {
    const auto& marker = input.markers[m];
    for (unsigned long e = 0; e < marker.GetnElem(2); ++e)
      if (int((e + m + 1) % owners) == world.rank)
        labels.push_back({node(marker.elem[2 * e]).id, node(marker.elem[2 * e + 1]).id, int(m)});
  }
  ReferenceState state;
  Associate(world, owned, labels, names, 45, state);
  REQUIRE(state.original != nullptr);
  CHECK(state.original->IsFeature(Point{0, 0}));
  CHECK(state.accepted_edges.size() == 6 * 3);  // Rectangle perimeter: six units, three faces per unit.
  const auto original = state.original;
  const auto initialBindings = state.accepted_edges;
  const auto slices = ReaderSlices(owned, state);
  CGeometry** geometry = nullptr;
  {
    Mute mute;
    config->SetMGLevels(0);
    CDistributedMemoryMeshReaderFVM reader(config.get(), slices, 0, 1);
    CDriver::BuildGeometryFVM(config.get(), new CPhysicalGeometry(config.get(), reader, 1), geometry, true);
  }
  const auto metric = slices.FetchPointMetric(*geometry[0]);
  const auto triangles = OwnedSimplices(*geometry[0]);
  std::vector<std::string> tags;
  for (unsigned short m = 0; m < geometry[0]->GetnMarker(); ++m) tags.push_back(config->GetMarker_All_TagBound(m));
  const auto faces = OwnedBoundaryFaces(*geometry[0], tags, *config);
  std::map<Id, Cell> rebuilt;
  const auto offset = CPassiveComm::ExscanSum(triangles.size());
  auto realNode = [&](unsigned long p) {
    return Node{geometry[0]->nodes->GetGlobalIndex(p),
                {SU2_TYPE::GetValue(geometry[0]->nodes->GetCoord(p, 0)),
                 SU2_TYPE::GetValue(geometry[0]->nodes->GetCoord(p, 1))},
                0};
  };
  for (unsigned long e = 0; e < triangles.size(); ++e) {
    Cell cell;
    cell.t.id = offset + e;
    for (int k = 0; k < 3; ++k) {
      const auto p = triangles.nodes[3 * e + k];
      cell.t.v[k] = realNode(p);
      cell.nodal_target[k] = {metric[3 * p], metric[3 * p + 1], metric[3 * p + 2]};
    }
    if (area(cell.t) < 0) {
      std::swap(cell.t.v[1], cell.t.v[2]);
      std::swap(cell.nodal_target[1], cell.nodal_target[2]);
    }
    rebuilt.emplace(cell.t.id, cell);
  }
  labels.clear();
  for (unsigned long f = 0; f < faces.size(); ++f) {
    const auto name = config->GetMarker_CfgFile_TagBound(faces.markerId[f]);
    const auto position = std::find(names.begin(), names.end(), name) - names.begin();
    labels.push_back({realNode(faces.nodes[2 * f]).id, realNode(faces.nodes[2 * f + 1]).id, int(position)});
  }
  Associate(world, rebuilt, labels, names, 45, state);
  CHECK(state.original == original);
  CHECK(state.accepted_edges == initialBindings);
  CHECK(Bindings(world, rebuilt) == initialBindings);
  for (const auto& cell : rebuilt)
    for (const auto& v : cell.second.t.v) {
      CHECK(v.id < (Id(1) << 40));
      if (v.p.x == 0 && v.p.y == 0) CHECK((v.fixed & FEATURE) != 0);
    }
  const auto second = ReaderSlices(rebuilt, state);
  CHECK(second.nPointGlobal == input.GetnPoint());
  CHECK(second.nElemGlobal == input.GetnElem());
  delete geometry[0];
  delete[] geometry;
}
