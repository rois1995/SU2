/*!
 * \file CMeshOutput_tests.cpp
 * \brief Unit tests for the mesh output from memory (adapted meshes of the mesh adaptation loop).
 * \version 8.5.0 "Harrier"
 *
 * SU2 Project Website: https://su2code.github.io
 *
 * The SU2 Project is maintained by the SU2 Foundation
 * (http://su2foundation.org)
 *
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md)
 *
 * SU2 is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * SU2 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with SU2. If not, see <http://www.gnu.org/licenses/>.
 */

#include "catch.hpp"

#include <algorithm>
#include <array>
#include <cstdio>
#include <fstream>
#include <map>
#include <set>

#include "../../../Common/include/CConfig.hpp"
#include "../../../Common/include/parallelization/CPassiveComm.hpp"
#include "../../../Common/include/geometry/CPhysicalGeometry.hpp"
#include "../../../SU2_CFD/include/drivers/CDriver.hpp"
#include "../../../SU2_CFD/include/output/CMeshOutput.hpp"
#include "../../Common/adaptation/SimplexMeshTestCase.hpp"

namespace {

/*!
 * \brief A mesh by global point index: coordinates, volume elements and boundary elements of each marker (node
 *        lists sorted, so that the orientation and the first node do not matter).
 */
struct GlobalMesh {
  std::map<unsigned long, std::vector<passivedouble>> coord;
  std::multiset<std::vector<unsigned long>> elem;
  std::map<unsigned long, std::vector<unsigned long>> elemByIndex;
  std::map<string, std::vector<std::vector<unsigned long>>> markerOrder;
  std::map<string, std::multiset<std::vector<unsigned long>>> markers;
};

/*--- Config of the tests: Euler, all markers far field, no multigrid. ---*/
std::unique_ptr<CConfig> MakeConfig(unsigned short nDim, const string& meshFile, const string& meshFormat,
                                    const string& outFormat, const string& markerOptions = "") {
  const string markers =
      !markerOptions.empty()
          ? markerOptions
          : (nDim == 2) ? "MARKER_FAR= (left, right, upper, lower_a, lower_b)\n"
                        : "MARKER_FAR= (x_minus, x_plus, y_minus, y_plus, z_plus, z_minus_a, z_minus_b)\n";
  std::stringstream options("SOLVER= EULER\nMGLEVEL= 0\nMESH_FORMAT= " + meshFormat + "\nMESH_FILENAME= " + meshFile +
                            "\n" + (outFormat.empty() ? "" : "MESH_OUT_FORMAT= " + outFormat + "\n") + markers);
  auto* origBuf = std::cout.rdbuf(nullptr);
  auto config = std::make_unique<CConfig>(options, SU2_COMPONENT::SU2_CFD, false);
  std::cout.rdbuf(origBuf);
  return config;
}

/*--- Geometry from the mesh file of the config, as in the driver, and its mesh by global index. ---*/
GlobalMesh ReadMesh(CConfig* config) {
  auto* origBuf = std::cout.rdbuf(nullptr);
  CGeometry** geometry = nullptr;
  CDriver::BuildGeometryFVM(config, new CPhysicalGeometry(config, 0, 1), geometry);
  std::cout.rdbuf(origBuf);

  const auto* fine = geometry[MESH_0];
  const auto nDim = fine->GetnDim();
  GlobalMesh mesh;
  // Test-only gather: compare complete global meshes, independently of the CFD partition and its halos.
  std::vector<unsigned long> pointIds;
  std::vector<std::array<passivedouble, 3>> coordinates;
  for (auto iPoint = 0ul; iPoint < fine->GetnPointDomain(); ++iPoint) {
    pointIds.push_back(fine->nodes->GetGlobalIndex(iPoint));
    std::array<passivedouble, 3> x{};
    for (unsigned short iDim = 0; iDim < nDim; ++iDim)
      x[iDim] = SU2_TYPE::GetValue(fine->nodes->GetCoord(iPoint, iDim));
    coordinates.push_back(x);
  }
  pointIds = CPassiveComm::Allgatherv(pointIds, nullptr);
  coordinates = CPassiveComm::Allgatherv(coordinates, nullptr);
  REQUIRE(pointIds.size() == coordinates.size());
  for (size_t i = 0; i < pointIds.size(); ++i)
    REQUIRE(mesh.coord.emplace(pointIds[i], std::vector<passivedouble>(coordinates[i].begin(),
                                                                    coordinates[i].begin() + nDim)).second);

  using Record = std::array<unsigned long, SU2_CONN_SIZE>; // Global identity, node count, sorted global nodes.
  auto record = [&](const CPrimalGrid* element, unsigned long id) {
    Record result{};
    result[0] = id;
    result[1] = element->GetnNodes();
    for (unsigned short iNode = 0; iNode < element->GetnNodes(); ++iNode)
      result[iNode + 2] = fine->nodes->GetGlobalIndex(element->GetNode(iNode));
    std::sort(result.begin() + 2, result.begin() + 2 + result[1]);
    return result;
  };
  auto nodes = [](const Record& value) {
    return std::vector<unsigned long>(value.begin() + 2, value.begin() + 2 + value[1]);
  };
  std::vector<Record> localElements;
  for (auto iElem = 0ul; iElem < fine->GetnElem(); ++iElem)
    localElements.push_back(record(fine->elem[iElem], fine->elem[iElem]->GetGlobalIndex()));
  for (const auto& value : CPassiveComm::Allgatherv(localElements, nullptr)) {
    const auto row = nodes(value);
    const auto inserted = mesh.elemByIndex.emplace(value[0], row);
    CHECK(inserted.second || inserted.first->second == row); // Halo copies must agree.
  }
  for (const auto& value : mesh.elemByIndex) mesh.elem.insert(value.second);

  const auto* physical = dynamic_cast<const CPhysicalGeometry*>(fine);
  REQUIRE(physical != nullptr);
  for (unsigned short iCfg = 0; iCfg < config->GetnMarker_CfgFile(); ++iCfg) {
    const auto tag = config->GetMarker_CfgFile_TagBound(iCfg);
    if (config->GetMarker_CfgFile_KindBC(tag) == SEND_RECEIVE) continue;
    std::vector<Record> localFaces;
    for (unsigned short iMarker = 0; iMarker < fine->GetnMarker(); ++iMarker) {
      if (config->GetMarker_All_TagBound(iMarker) != tag) continue;
      for (auto iElem = 0ul; iElem < fine->GetnElem_Bound(iMarker); ++iElem) {
        const auto* face = fine->bound[iMarker][iElem];
        const auto id = physical->BoundaryGlobalIndex.find(face);
        REQUIRE(id != physical->BoundaryGlobalIndex.end());
        localFaces.push_back(record(face, id->second));
      }
    }
    std::map<unsigned long, std::vector<unsigned long>> facesByIndex;
    for (const auto& value : CPassiveComm::Allgatherv(localFaces, nullptr)) {
      const auto row = nodes(value);
      const auto inserted = facesByIndex.emplace(value[0], row);
      CHECK(inserted.second || inserted.first->second == row);
    }
    for (const auto& face : facesByIndex) {
      mesh.markers[tag].insert(face.second);
      mesh.markerOrder[tag].push_back(face.second);
    }
  }
  for (unsigned short iMesh = 0; iMesh <= config->GetnMGLevels(); ++iMesh) delete geometry[iMesh];
  delete[] geometry;
  return mesh;
}

/*!
 * \brief Write a simplex mesh in SU2 format, read it as in the driver, write it from memory in the format outFormat
 *        with CMeshOutput::WriteMesh, read that file, and compare both meshes by global point index (the output keeps
 *        the numbering of the geometry). The coordinates must be read back exactly in every format.
 */
void CheckWriteAndRead(const CSimplexMesh& simplexMesh, const string& outFormat,
                       const string& markerOptions = "", bool scramble = false) {
  const auto nDim = simplexMesh.nDim;
  const string inputFile = "mesh_output_test_input.su2";
  const string outputName = "mesh_output_test_output";
  if (SU2_MPI::GetRank() == MASTER_NODE) simplex_test::WriteSU2Mesh(simplexMesh, inputFile);
  SU2_MPI::Barrier(SU2_MPI::GetComm());

  /*--- Read the mesh and write it from memory. ---*/

  auto config = MakeConfig(nDim, inputFile, "SU2", outFormat, markerOptions);
  const auto extension = config->GetMesh_Out_FileExtension();
  CGeometry** geometry = nullptr;
  auto* origBuf = std::cout.rdbuf(nullptr);
  CDriver::BuildGeometryFVM(config.get(), new CPhysicalGeometry(config.get(), 0, 1), geometry);
  if (scramble) {
    auto* fine = geometry[MESH_0];
    std::reverse(fine->elem, fine->elem + fine->GetnElem());
    for (unsigned short iMarker = 0; iMarker < fine->GetnMarker(); ++iMarker)
      std::reverse(fine->bound[iMarker], fine->bound[iMarker] + fine->GetnElem_Bound(iMarker));
  }
  CMeshOutput::WriteMesh(config.get(), geometry[MESH_0], outputName);
  std::cout.rdbuf(origBuf);
  delete geometry[MESH_0];
  delete[] geometry;

  const auto input = ReadMesh(config.get());

  /*--- Read the written mesh in its own format. ---*/

  auto outputConfig = MakeConfig(nDim, outputName + extension, outFormat, "", markerOptions);
  const auto output = ReadMesh(outputConfig.get());

  REQUIRE(output.coord.size() == simplexMesh.GetnPoint());
  REQUIRE(output.coord.size() == input.coord.size());
  passivedouble maxDiff = 0.0;
  for (const auto& entry : input.coord) {
    const auto& x = output.coord.at(entry.first);
    for (unsigned short iDim = 0; iDim < nDim; ++iDim) maxDiff = std::max(maxDiff, fabs(x[iDim] - entry.second[iDim]));
  }
  CHECK(maxDiff == 0.0);

  CHECK(output.elem.size() == simplexMesh.GetnElem());
  CHECK(output.elem == input.elem);
  if (outFormat != "CGNS") {
    CHECK(output.elemByIndex == input.elemByIndex);
    CHECK(output.markerOrder == input.markerOrder);
  }

  /*--- Every marker of the mesh, with its own elements (coplanar markers of the same type are not merged). ---*/
  REQUIRE(output.markers.size() == simplexMesh.markers.size());
  for (const auto& marker : simplexMesh.markers) {
    REQUIRE(output.markers.count(marker.name) == 1);
    CHECK(output.markers.at(marker.name).size() == marker.GetnElem(nDim));
    CHECK(output.markers.at(marker.name) == input.markers.at(marker.name));
  }

  SU2_MPI::Barrier(SU2_MPI::GetComm());
  if (SU2_MPI::GetRank() == MASTER_NODE) {
    std::remove(inputFile.c_str());
    std::remove((outputName + extension).c_str());
  }
  SU2_MPI::Barrier(SU2_MPI::GetComm());
}

/*--- Rectangle [100, 100 + 2e-4] x [0, 1]: cells of 2.5e-5 x 0.25 near x = 100, where single precision has a
 * resolution of 7.6e-6, so the coordinates must be written in double precision. ---*/
CSimplexMesh AnisotropicRectangle() {
  auto mesh = simplex_test::MakeSimplexMesh(2, 4, simplex_test::Marker2D);
  for (auto iPoint = 0ul; iPoint < mesh.GetnPoint(); ++iPoint) mesh.coord[2 * iPoint] = 100.0 + 1e-4 * mesh.coord[2 * iPoint];
  return mesh;
}

}  // namespace

TEST_CASE("Mesh output from memory, SU2", "[Adaptation]") {
  CheckWriteAndRead(simplex_test::MakeSimplexMesh(2, 4, simplex_test::Marker2D), "SU2");
  CheckWriteAndRead(simplex_test::MakeSimplexMesh(3, 2, simplex_test::Marker3D), "SU2");
  CheckWriteAndRead(AnisotropicRectangle(), "SU2");
}

TEST_CASE("Mesh output from memory, SU2 binary", "[Adaptation]") {
  CheckWriteAndRead(simplex_test::MakeSimplexMesh(2, 4, simplex_test::Marker2D), "SU2B");
  CheckWriteAndRead(simplex_test::MakeSimplexMesh(3, 2, simplex_test::Marker3D), "SU2B");
  CheckWriteAndRead(AnisotropicRectangle(), "SU2B");
}

TEST_CASE("Mesh output preserves global element and boundary order", "[Adaptation][MeshOrder]") {
  for (const auto& format : {"SU2", "SU2B"}) {
    CheckWriteAndRead(simplex_test::MakeSimplexMesh(2, 4, simplex_test::Marker2D), format, "", true);
    CheckWriteAndRead(simplex_test::MakeSimplexMesh(3, 2, simplex_test::Marker3D), format, "", true);
  }
}

TEST_CASE("Mesh output preserves interleaved element types", "[Adaptation][MeshOrder]") {
  const string inputFile = "mesh_output_mixed_input.su2";
  const string outputName = "mesh_output_mixed_output";
  if (SU2_MPI::GetRank() == MASTER_NODE) {
    std::ofstream file(inputFile);
    file << "NDIME= 2\nNELEM= 3\n5 1 2 5 0\n9 0 1 4 3 1\n5 1 5 4 2\n"
            "NPOIN= 6\n0 0 0\n1 0 1\n2 0 2\n0 1 3\n1 1 4\n2 1 5\n"
            "NMARK= 1\nMARKER_TAG= wall\nMARKER_ELEMS= 6\n"
            "3 0 1\n3 1 2\n3 2 5\n3 5 4\n3 4 3\n3 3 0\n";
  }
  SU2_MPI::Barrier(SU2_MPI::GetComm());
  for (const auto& format : {"SU2", "SU2B"}) {
    auto config = MakeConfig(2, inputFile, "SU2", format, "MARKER_FAR= (wall)\n");
    const auto input = ReadMesh(config.get());
    CGeometry** geometry = nullptr;
    auto* origBuf = std::cout.rdbuf(nullptr);
    CDriver::BuildGeometryFVM(config.get(), new CPhysicalGeometry(config.get(), 0, 1), geometry);
    CMeshOutput::WriteMesh(config.get(), geometry[MESH_0], outputName);
    std::cout.rdbuf(origBuf);
    delete geometry[MESH_0];
    delete[] geometry;
    const auto outputFile = outputName + config->GetMesh_Out_FileExtension();
    auto outputConfig = MakeConfig(2, outputFile, format, "", "MARKER_FAR= (wall)\n");
    const auto output = ReadMesh(outputConfig.get());
    CHECK(output.elemByIndex == input.elemByIndex);
    CHECK(output.markerOrder == input.markerOrder);
    SU2_MPI::Barrier(SU2_MPI::GetComm());
    if (SU2_MPI::GetRank() == MASTER_NODE) std::remove(outputFile.c_str());
    SU2_MPI::Barrier(SU2_MPI::GetComm());
  }
  if (SU2_MPI::GetRank() == MASTER_NODE) std::remove(inputFile.c_str());
  SU2_MPI::Barrier(SU2_MPI::GetComm());
}

#ifdef HAVE_CGNS
TEST_CASE("Mesh output from memory, CGNS", "[Adaptation]") {
  CheckWriteAndRead(simplex_test::MakeSimplexMesh(2, 4, simplex_test::Marker2D), "CGNS");
  CheckWriteAndRead(simplex_test::MakeSimplexMesh(3, 2, simplex_test::Marker3D), "CGNS");
  CheckWriteAndRead(AnisotropicRectangle(), "CGNS");
}

TEST_CASE("Mesh output from memory, CGNS, marker names of other CGNS nodes", "[Adaptation]") {
  /*--- Markers named like the volume sections (Triangles, Tetrahedra, ...), the zone and the solution node of the
   *    writer: they share the parent node with them in a CGNS file (sections and the solution in the zone, families and
   *    the zone in the base). The file must be written and read back with the same marker names. ---*/
  auto names2D = [](const passivedouble* x) -> std::string {
    if (x[1] < 1e-12) return x[0] < 1.0 ? "Triangles" : "Quadrilaterals";
    if (x[1] > 1.0 - 1e-12) return "Zone";
    return x[0] < 1e-12 ? "Fields" : "Lines";
  };
  CheckWriteAndRead(simplex_test::MakeSimplexMesh(2, 4, names2D), "CGNS",
                    "MARKER_FAR= (Triangles, Quadrilaterals, Zone, Fields, Lines)\n");
  auto names3D = [](const passivedouble* x) -> std::string {
    if (x[2] < 1e-12) return x[0] < 0.5 ? "Tetrahedra" : "Triangles";
    if (x[2] > 1.0 - 1e-12) return "Zone";
    if (x[0] < 1e-12) return "Fields";
    if (x[0] > 1.0 - 1e-12) return "Prisms";
    return x[1] < 1e-12 ? "Hexahedra" : "Pyramids";
  };
  CheckWriteAndRead(simplex_test::MakeSimplexMesh(3, 2, names3D), "CGNS",
                    "MARKER_FAR= (Tetrahedra, Triangles, Zone, Fields, Prisms, Hexahedra, Pyramids)\n");
}

TEST_CASE("Mesh output from memory, CGNS, reserved and long marker names", "[Adaptation]") {
  /*--- Markers named like the standard (SIDS) nodes of a zone, and names longer than the 32 characters of a CGNS
   *    name (two of them equal in their first 32 characters): the sections get other names in the file, but the file
   *    must be read back with the original marker names and the unchanged config (a restart of the adaptation). ---*/
  const std::string long1 = "a_marker_name_longer_than_32_characters_one";
  const std::string long2 = "a_marker_name_longer_than_32_characters_two";
  auto names2D = [&](const passivedouble* x) -> std::string {
    if (x[1] < 1e-12) return x[0] < 1.0 ? "GridCoordinates" : "ZoneBC";
    if (x[1] > 1.0 - 1e-12) return "ZoneType";
    return x[0] < 1e-12 ? long1 : long2;
  };
  CheckWriteAndRead(simplex_test::MakeSimplexMesh(2, 4, names2D), "CGNS",
                    "MARKER_FAR= (GridCoordinates, ZoneBC, ZoneType, " + long1 + ", " + long2 + ")\n");
  auto names3D = [&](const passivedouble* x) -> std::string {
    if (x[2] < 1e-12) return x[0] < 0.5 ? "GridCoordinates" : "GridCoordinates_1";
    if (x[2] > 1.0 - 1e-12) return "ZoneBC";
    if (x[0] < 1e-12) return "ZoneType";
    if (x[0] > 1.0 - 1e-12) return long1;
    return x[1] < 1e-12 ? long2 : "Zone";
  };
  CheckWriteAndRead(simplex_test::MakeSimplexMesh(3, 2, names3D), "CGNS",
                    "MARKER_FAR= (GridCoordinates, GridCoordinates_1, ZoneBC, ZoneType, " + long1 + ", " + long2 +
                        ", Zone)\n");
}
#endif
