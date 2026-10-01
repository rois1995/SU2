/*!
 * \file CMemoryMeshReaderFVM_tests.cpp
 * \brief Unit tests for the geometry built from a simplex mesh in memory (mesh adaptation).
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

#include <array>
#include <cstdio>
#include <map>
#include <set>

#include "../../../Common/include/CConfig.hpp"
#include "../../../Common/include/adaptation/CMMGInterface.hpp"
#include "../../../Common/include/geometry/CPhysicalGeometry.hpp"
#include "../../../Common/include/geometry/meshreader/CMemoryMeshReaderFVM.hpp"
#include "../../../SU2_CFD/include/drivers/CDriver.hpp"
#include "../adaptation/SimplexMeshTestCase.hpp"

namespace {

/*!
 * \brief Geometry of all multigrid levels and the marker names; the marker data of the config describe the last
 *        geometry built only.
 */
struct Geometry {
  std::vector<std::unique_ptr<CGeometry>> levels;
  std::vector<string> tags;

  const CGeometry& Fine() const { return *levels[MESH_0]; }
};

/*!
 * \brief Geometry preprocessing of the driver for one zone: CDriver::BuildGeometryFVM (partitioning, connectivity,
 *        orientation, edges, control volumes, multigrid, ...), reference area and wall distance.
 */
Geometry Preprocess(CConfig* config, CGeometry* geometry_aux) {
  CGeometry** geometry = nullptr;
  CDriver::BuildGeometryFVM(config, geometry_aux, geometry);
  geometry[MESH_0]->SetPositive_ZArea(config);

  CGeometry** instances[] = {geometry};
  CGeometry*** zones[] = {instances};
  const CConfig* configs[] = {config};
  CGeometry::ComputeWallDistance(configs, zones);

  for (unsigned short iMesh = 0; iMesh <= config->GetnMGLevels(); ++iMesh) geometry[iMesh]->SetElemVolume();

  Geometry result;
  for (unsigned short iMesh = 0; iMesh <= config->GetnMGLevels(); ++iMesh) result.levels.emplace_back(geometry[iMesh]);
  for (unsigned short iMarker = 0; iMarker < geometry[MESH_0]->GetnMarker(); ++iMarker)
    result.tags.push_back(config->GetMarker_All_TagBound(iMarker));
  delete[] geometry;
  return result;
}

/*!
 * \brief Config of the test cases: viscous walls for the wall distance, small multigrid levels so that the
 *        coarsest ones are dropped.
 */
std::unique_ptr<CConfig> MakeConfig(unsigned short nDim, const string& meshFile) {
  const string markers = (nDim == 2) ? "MARKER_FAR= (left, right, upper)\n"
                                       "MARKER_HEATFLUX= (lower_b, 0.0, lower_a, 0.0)\n"
                                     : "MARKER_FAR= (x_minus, x_plus, y_minus, y_plus, z_plus)\n"
                                       "MARKER_HEATFLUX= (z_minus_b, 0.0, z_minus_a, 0.0)\n";
  stringstream options("SOLVER= RANS\nKIND_TURB_MODEL= SA\nREYNOLDS_NUMBER= 1e6\nMESH_FORMAT= SU2\n"
                       "MESH_FILENAME= " + meshFile + "\n" + markers +
                       "MGLEVEL= 3\nMG_MIN_MESHSIZE= " + (nDim == 2 ? "20" : "50") + "\n");
  auto origBuf = cout.rdbuf();
  cout.rdbuf(nullptr);
  auto config = std::unique_ptr<CConfig>(new CConfig(options, SU2_COMPONENT::SU2_CFD, false));
  cout.rdbuf(origBuf);
  return config;
}

/*--- Geometry from the mesh file of the config, as in the driver. ---*/
Geometry FromFile(CConfig* config, unsigned short nMGLevels) {
  auto origBuf = cout.rdbuf();
  cout.rdbuf(nullptr);
  config->SetMGLevels(nMGLevels);
  auto geometry = Preprocess(config, new CPhysicalGeometry(config, 0, 1));
  cout.rdbuf(origBuf);
  return geometry;
}

/*!
 * \brief A simplex mesh case: the mesh file, a config that reads it, and the requested number of multigrid levels.
 */
struct MemoryMeshCase {
  string meshFile;
  CSimplexMesh mesh;
  std::unique_ptr<CConfig> config;
  unsigned short nMGLevels = 0;

  MemoryMeshCase(unsigned short nDim, unsigned long n) {
    meshFile = "memory_mesh_reader_test_" + std::to_string(nDim) + "d.su2";
    mesh = simplex_test::MakeSimplexMesh(nDim, n, nDim == 2 ? simplex_test::Marker2D : simplex_test::Marker3D);
    simplex_test::WriteSU2Mesh(mesh, meshFile);
    config = MakeConfig(nDim, meshFile);
    nMGLevels = config->GetnMGLevels();
  }
  ~MemoryMeshCase() { std::remove(meshFile.c_str()); }

  Geometry FromFile() { return ::FromFile(config.get(), nMGLevels); }

  /*--- Geometry from a mesh in memory, with the same config (MGLEVEL reset as the driver must do). ---*/
  Geometry FromMemory(const CSimplexMesh& memoryMesh) {
    auto origBuf = cout.rdbuf();
    cout.rdbuf(nullptr);
    config->SetMGLevels(nMGLevels);
    CMemoryMeshReaderFVM reader(config.get(), memoryMesh, 0, 1);
    auto geometry = Preprocess(config.get(), new CPhysicalGeometry(config.get(), reader, 1));
    cout.rdbuf(origBuf);
    return geometry;
  }
};

/*--- Config marker data refer to the last geometry built, check that they match it. ---*/
void CheckConfigMarkers(const CConfig& config, const Geometry& geometry) {
  REQUIRE(config.GetnMarker_All() == geometry.Fine().GetnMarker());
  for (unsigned short iMarker = 0; iMarker < geometry.Fine().GetnMarker(); ++iMarker) {
    const auto tag = geometry.tags[iMarker];
    CHECK(config.GetMarker_All_TagBound(iMarker) == tag);
    CHECK(config.GetMarker_All_KindBC(iMarker) == config.GetMarker_CfgFile_KindBC(tag));
    CHECK(geometry.Fine().GetMarker_Tag(config.GetMarker_CfgFile_TagBound(tag)) == tag);
  }
}

/*--- Side of the rectangle or cube of each marker: axis of the outward direction, its sign, and the area. ---*/
struct Side {
  unsigned short axis;
  passivedouble sign, area;
};
Side SideOf(const string& tag) {
  static const std::map<string, Side> sides = {
      {"left", {0, -1.0, 1.0}},    {"right", {0, 1.0, 1.0}},     {"upper", {1, 1.0, 2.0}},
      {"lower_a", {1, -1.0, 1.0}}, {"lower_b", {1, -1.0, 1.0}},  {"x_minus", {0, -1.0, 1.0}},
      {"x_plus", {0, 1.0, 1.0}},   {"y_minus", {1, -1.0, 1.0}},  {"y_plus", {1, 1.0, 1.0}},
      {"z_plus", {2, 1.0, 1.0}},   {"z_minus_a", {2, -1.0, 0.5}}, {"z_minus_b", {2, -1.0, 0.5}}};
  return sides.at(tag);
}

/*--- Boundary normals point into the domain (SU2 convention, the boundary conditions use -normal as the outward
 * normal) on every vertex; the sum over a flat marker is its area times the inward normal. ---*/
void CheckBoundaryNormals(const Geometry& result) {
  const auto& geometry = result.Fine();
  const auto nDim = geometry.GetnDim();
  for (unsigned short iMarker = 0; iMarker < geometry.GetnMarker(); ++iMarker) {
    const auto side = SideOf(result.tags[iMarker]);
    std::array<passivedouble, 3> sum = {0.0, 0.0, 0.0};
    unsigned long nOutward = 0;
    for (auto iVertex = 0ul; iVertex < geometry.GetnVertex(iMarker); ++iVertex) {
      const auto* normal = geometry.vertex[iMarker][iVertex]->GetNormal();
      for (unsigned short iDim = 0; iDim < nDim; ++iDim) sum[iDim] += SU2_TYPE::GetValue(normal[iDim]);
      if (!(side.sign * normal[side.axis] < 0.0)) ++nOutward;
    }
    CHECK(nOutward == 0);
    CHECK(-side.sign * sum[side.axis] == Approx(side.area));
  }
}

/*!
 * \brief Same mesh with the same numbering: the geometries must be identical on every multigrid level.
 */
void CheckIdentical(const Geometry& a, const Geometry& b) {
  CHECK(a.tags == b.tags);
  REQUIRE(a.levels.size() == b.levels.size());
  for (size_t iMesh = 0; iMesh < a.levels.size(); ++iMesh) {
    const auto& ga = *a.levels[iMesh];
    const auto& gb = *b.levels[iMesh];
    const auto nDim = ga.GetnDim();
    REQUIRE(gb.GetnDim() == nDim);
    REQUIRE(ga.GetnPoint() == gb.GetnPoint());
    REQUIRE(ga.GetnPointDomain() == gb.GetnPointDomain());
    REQUIRE(ga.GetnElem() == gb.GetnElem());
    REQUIRE(ga.GetnEdge() == gb.GetnEdge());
    REQUIRE(ga.GetnMarker() == gb.GetnMarker());

    unsigned long nDiff = 0;
    for (auto iPoint = 0ul; iPoint < ga.GetnPoint(); ++iPoint) {
      for (unsigned short iDim = 0; iDim < nDim; ++iDim)
        nDiff += ga.nodes->GetCoord(iPoint, iDim) != gb.nodes->GetCoord(iPoint, iDim);
      nDiff += ga.nodes->GetVolume(iPoint) != gb.nodes->GetVolume(iPoint);
      nDiff += ga.nodes->GetWall_Distance(iPoint) != gb.nodes->GetWall_Distance(iPoint);
      nDiff += ga.nodes->GetnNeighbor(iPoint) != gb.nodes->GetnNeighbor(iPoint);
      nDiff += ga.nodes->GetGlobalIndex(iPoint) != gb.nodes->GetGlobalIndex(iPoint);
      nDiff += ga.nodes->GetnPoint(iPoint) != gb.nodes->GetnPoint(iPoint);
      for (unsigned short iNeigh = 0; iNeigh < ga.nodes->GetnPoint(iPoint); ++iNeigh)
        nDiff += ga.nodes->GetPoint(iPoint, iNeigh) != gb.nodes->GetPoint(iPoint, iNeigh);
    }
    CHECK(nDiff == 0);

    nDiff = 0;
    for (auto iEdge = 0ul; iEdge < ga.GetnEdge(); ++iEdge) {
      for (unsigned short iNode = 0; iNode < 2; ++iNode)
        nDiff += ga.edges->GetNode(iEdge, iNode) != gb.edges->GetNode(iEdge, iNode);
      for (unsigned short iDim = 0; iDim < nDim; ++iDim)
        nDiff += ga.edges->GetNormal(iEdge)[iDim] != gb.edges->GetNormal(iEdge)[iDim];
    }
    CHECK(nDiff == 0);

    nDiff = 0;
    for (auto iElem = 0ul; iElem < ga.GetnElem(); ++iElem) {
      nDiff += ga.elem[iElem]->GetnNodes() != gb.elem[iElem]->GetnNodes();
      for (unsigned short iNode = 0; iNode < ga.elem[iElem]->GetnNodes(); ++iNode)
        nDiff += ga.elem[iElem]->GetNode(iNode) != gb.elem[iElem]->GetNode(iNode);
      nDiff += ga.elem[iElem]->GetVolume() != gb.elem[iElem]->GetVolume();
    }
    CHECK(nDiff == 0);

    nDiff = 0;
    for (unsigned short iMarker = 0; iMarker < ga.GetnMarker(); ++iMarker) {
      REQUIRE(ga.GetnVertex(iMarker) == gb.GetnVertex(iMarker));
      for (auto iVertex = 0ul; iVertex < ga.GetnVertex(iMarker); ++iVertex) {
        const auto* va = ga.vertex[iMarker][iVertex];
        const auto* vb = gb.vertex[iMarker][iVertex];
        nDiff += va->GetNode() != vb->GetNode();
        nDiff += va->GetNormal_Neighbor() != vb->GetNormal_Neighbor();
        for (unsigned short iDim = 0; iDim < nDim; ++iDim) nDiff += va->GetNormal(iDim) != vb->GetNormal(iDim);
      }
    }
    CHECK(nDiff == 0);
  }
}

using Coord = std::array<passivedouble, 3>;

Coord PointCoord(const CGeometry& geometry, unsigned long iPoint) {
  Coord x = {0.0, 0.0, 0.0};
  for (unsigned short iDim = 0; iDim < geometry.GetnDim(); ++iDim)
    x[iDim] = SU2_TYPE::GetValue(geometry.nodes->GetCoord(iPoint, iDim));
  return x;
}

/*!
 * \brief Same mesh with another numbering: the fine grids must be equal up to the renumbering (points are matched
 *        by their coordinates, which are copied exactly).
 */
void CheckEquivalent(const Geometry& a, const Geometry& b) {
  const auto& ga = a.Fine();
  const auto& gb = b.Fine();
  const auto nDim = ga.GetnDim();
  REQUIRE(ga.GetnPoint() == gb.GetnPoint());
  REQUIRE(ga.GetnElem() == gb.GetnElem());
  REQUIRE(ga.GetnEdge() == gb.GetnEdge());
  REQUIRE(ga.GetnMarker() == gb.GetnMarker());

  /*--- Point of a that has the coordinates of each point of b. ---*/
  std::map<Coord, unsigned long> pointOfA;
  for (auto iPoint = 0ul; iPoint < ga.GetnPoint(); ++iPoint) pointOfA[PointCoord(ga, iPoint)] = iPoint;
  REQUIRE(pointOfA.size() == ga.GetnPoint());
  std::vector<unsigned long> toA(gb.GetnPoint());
  for (auto iPoint = 0ul; iPoint < gb.GetnPoint(); ++iPoint) {
    const auto it = pointOfA.find(PointCoord(gb, iPoint));
    REQUIRE(it != pointOfA.end());
    toA[iPoint] = it->second;
  }

  const passivedouble tol = 1e-12;
  unsigned long nDiff = 0;
  for (auto iPoint = 0ul; iPoint < gb.GetnPoint(); ++iPoint) {
    const auto jPoint = toA[iPoint];
    nDiff += fabs(gb.nodes->GetVolume(iPoint) - ga.nodes->GetVolume(jPoint)) > tol * ga.nodes->GetVolume(jPoint);
    nDiff += fabs(gb.nodes->GetWall_Distance(iPoint) - ga.nodes->GetWall_Distance(jPoint)) > tol;
    std::set<unsigned long> na, nb;
    for (unsigned short iNeigh = 0; iNeigh < ga.nodes->GetnPoint(jPoint); ++iNeigh)
      na.insert(ga.nodes->GetPoint(jPoint, iNeigh));
    for (unsigned short iNeigh = 0; iNeigh < gb.nodes->GetnPoint(iPoint); ++iNeigh)
      nb.insert(toA[gb.nodes->GetPoint(iPoint, iNeigh)]);
    nDiff += na != nb;
  }
  CHECK(nDiff == 0);

  /*--- Edge normals, oriented from the first to the second node of the edge of a. ---*/
  std::map<std::pair<unsigned long, unsigned long>, unsigned long> edgeOfA;
  for (auto iEdge = 0ul; iEdge < ga.GetnEdge(); ++iEdge)
    edgeOfA[{ga.edges->GetNode(iEdge, 0), ga.edges->GetNode(iEdge, 1)}] = iEdge;
  nDiff = 0;
  for (auto iEdge = 0ul; iEdge < gb.GetnEdge(); ++iEdge) {
    const auto i = toA[gb.edges->GetNode(iEdge, 0)], j = toA[gb.edges->GetNode(iEdge, 1)];
    auto it = edgeOfA.find({i, j});
    passivedouble sign = 1.0;
    if (it == edgeOfA.end()) {
      it = edgeOfA.find({j, i});
      sign = -1.0;
    }
    REQUIRE(it != edgeOfA.end());
    for (unsigned short iDim = 0; iDim < nDim; ++iDim)
      nDiff += fabs(sign * gb.edges->GetNormal(iEdge)[iDim] - ga.edges->GetNormal(it->second)[iDim]) > tol;
  }
  CHECK(nDiff == 0);

  /*--- Element volumes (as a sorted list) and the total volume. ---*/
  std::vector<passivedouble> va, vb;
  for (auto iElem = 0ul; iElem < ga.GetnElem(); ++iElem) va.push_back(SU2_TYPE::GetValue(ga.elem[iElem]->GetVolume()));
  for (auto iElem = 0ul; iElem < gb.GetnElem(); ++iElem) vb.push_back(SU2_TYPE::GetValue(gb.elem[iElem]->GetVolume()));
  std::sort(va.begin(), va.end());
  std::sort(vb.begin(), vb.end());
  nDiff = 0;
  for (size_t i = 0; i < va.size(); ++i) nDiff += fabs(va[i] - vb[i]) > tol;
  CHECK(nDiff == 0);
  CHECK(va.front() > 0.0);

  /*--- Markers by name, vertex normals by point. ---*/
  for (unsigned short iMarker = 0; iMarker < gb.GetnMarker(); ++iMarker) {
    unsigned short jMarker = 0;
    while (jMarker < ga.GetnMarker() && a.tags[jMarker] != b.tags[iMarker]) ++jMarker;
    REQUIRE(jMarker < ga.GetnMarker());
    REQUIRE(ga.GetnVertex(jMarker) == gb.GetnVertex(iMarker));
    std::map<unsigned long, const CVertex*> vertexOfA;
    for (auto iVertex = 0ul; iVertex < ga.GetnVertex(jMarker); ++iVertex)
      vertexOfA[ga.vertex[jMarker][iVertex]->GetNode()] = ga.vertex[jMarker][iVertex];
    nDiff = 0;
    for (auto iVertex = 0ul; iVertex < gb.GetnVertex(iMarker); ++iVertex) {
      const auto* vertex = gb.vertex[iMarker][iVertex];
      const auto it = vertexOfA.find(toA[vertex->GetNode()]);
      REQUIRE(it != vertexOfA.end());
      for (unsigned short iDim = 0; iDim < nDim; ++iDim)
        nDiff += fabs(vertex->GetNormal(iDim) - it->second->GetNormal(iDim)) > tol;
    }
    CHECK(nDiff == 0);
  }
}

passivedouble TotalVolume(const CGeometry& geometry) {
  passivedouble volume = 0.0;
  for (auto iPoint = 0ul; iPoint < geometry.GetnPointDomain(); ++iPoint)
    volume += SU2_TYPE::GetValue(geometry.nodes->GetVolume(iPoint));
  return volume;
}

void CheckMemoryGeometry(unsigned short nDim, unsigned long n) {
  MemoryMeshCase test(nDim, n);

  /*--- Reference: the geometry read from the mesh file. ---*/
  const auto fromFile = test.FromFile();
  const auto nLevels = fromFile.levels.size();
  CHECK(nLevels > 1);
  CHECK(nLevels < test.nMGLevels + 1u);  // the coarsest levels are dropped (MG_MIN_MESHSIZE)

  /*--- Same arrays in memory (same numbering, mixed orientations, unoriented boundary elements). ---*/
  const auto fromMemory = test.FromMemory(test.mesh);
  CheckConfigMarkers(*test.config, fromMemory);
  CheckIdentical(fromFile, fromMemory);
  CheckBoundaryNormals(fromMemory);

  const passivedouble volume = (nDim == 2) ? 2.0 : 1.0;
  for (const auto& level : fromMemory.levels) CHECK(TotalVolume(*level) == Approx(volume));

  /*--- Mesh extracted from the geometry (renumbered by SU2, positive elements), as in the adaptation. ---*/
  su2activematrix metric(fromFile.Fine().GetnPoint(), CSimplexMesh::GetnMetric(nDim));
  for (auto iPoint = 0ul; iPoint < metric.rows(); ++iPoint)
    for (unsigned short iDim = 0, iMet = 0; iDim < nDim; ++iDim)
      for (unsigned short jDim = iDim; jDim < nDim; ++jDim, ++iMet) metric(iPoint, iMet) = (iDim == jDim);
  const auto extracted = CMMGInterface::ExtractMesh(*test.config, fromFile.Fine(), metric);
  const auto fromExtracted = test.FromMemory(extracted);
  CheckConfigMarkers(*test.config, fromExtracted);
  CheckEquivalent(fromFile, fromExtracted);
  CheckBoundaryNormals(fromExtracted);
  CHECK(fromExtracted.levels.size() > 1);
  for (const auto& level : fromExtracted.levels) CHECK(TotalVolume(*level) == Approx(volume));
}

#ifdef HAVE_MMG
/*!
 * \brief Geometry of a mesh adapted by MMG (boundary elements without orientation) built from memory, against the
 *        geometry read from the same mesh written to a file.
 */
void CheckAdaptedGeometry(unsigned short nDim, unsigned long n) {
  MemoryMeshCase test(nDim, n);
  const auto fromFile = test.FromFile();

  const passivedouble h = 0.7 / n;
  su2activematrix metric(fromFile.Fine().GetnPoint(), CSimplexMesh::GetnMetric(nDim));
  for (auto iPoint = 0ul; iPoint < metric.rows(); ++iPoint)
    for (unsigned short iDim = 0, iMet = 0; iDim < nDim; ++iDim)
      for (unsigned short jDim = iDim; jDim < nDim; ++jDim, ++iMet) metric(iPoint, iMet) = (iDim == jDim) / (h * h);
  const auto mesh = CMMGInterface::ExtractMesh(*test.config, fromFile.Fine(), metric);
  CMMGInterface mmg(*test.config);
  const auto adapted = mmg.Adapt(mesh);

  const auto fromMemory = test.FromMemory(adapted);
  CheckConfigMarkers(*test.config, fromMemory);
  CheckBoundaryNormals(fromMemory);
  CHECK(fromMemory.Fine().GetnPoint() == adapted.GetnPoint());
  CHECK(fromMemory.Fine().GetnElem() == adapted.GetnElem());

  const string adaptedFile = "memory_mesh_reader_test_adapted.su2";
  simplex_test::WriteSU2Mesh(adapted, adaptedFile);
  auto config = MakeConfig(nDim, adaptedFile);
  const auto reloaded = FromFile(config.get(), test.nMGLevels);
  std::remove(adaptedFile.c_str());
  CheckIdentical(reloaded, fromMemory);
}
#endif

}  // namespace

#ifdef HAVE_MMG
TEST_CASE("Geometry from a 2D mesh adapted by MMG", "[Adaptation][MMG]") { CheckAdaptedGeometry(2, 8); }

TEST_CASE("Geometry from a 3D mesh adapted by MMG", "[Adaptation][MMG]") { CheckAdaptedGeometry(3, 6); }
#endif

TEST_CASE("Geometry from a 2D simplex mesh in memory", "[Adaptation]") { CheckMemoryGeometry(2, 8); }

TEST_CASE("Geometry from a 3D simplex mesh in memory", "[Adaptation]") { CheckMemoryGeometry(3, 6); }
