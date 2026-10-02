/*!
 * \file CMMGInterface_tests.cpp
 * \brief Unit tests for the conversion of SU2 meshes to MMG and for the remeshing.
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
#include <limits>
#include <set>

#include "../../../Common/include/CConfig.hpp"
#include "../../../Common/include/adaptation/CMMGInterface.hpp"
#include "../../../Common/include/geometry/CPhysicalGeometry.hpp"
#include "../../../Common/include/geometry/meshreader/CMemoryMeshReaderFVM.hpp"
#include "SimplexMeshTestCase.hpp"

namespace {

using simplex_test::Marker2D;
using simplex_test::Marker3D;

/*--- Region of each marker, used to check that the adapted markers stay where they were. ---*/
bool OnMarker(unsigned short nDim, const string& name, const passivedouble* x) {
  const passivedouble tol = 1e-10;
  if (nDim == 2) {
    if (name == "lower_a") return fabs(x[1]) < tol && x[0] < 1.0 + tol;
    if (name == "lower_b") return fabs(x[1]) < tol && x[0] > 1.0 - tol;
    if (name == "upper") return fabs(x[1] - 1.0) < tol;
    if (name == "left") return fabs(x[0]) < tol;
    if (name == "right") return fabs(x[0] - 2.0) < tol;
    return false;
  }
  if (name == "z_minus_a") return fabs(x[2]) < tol && x[0] < 0.5 + tol;
  if (name == "z_minus_b") return fabs(x[2]) < tol && x[0] > 0.5 - tol;
  if (name == "z_plus") return fabs(x[2] - 1.0) < tol;
  if (name == "x_minus") return fabs(x[0]) < tol;
  if (name == "x_plus") return fabs(x[0] - 1.0) < tol;
  if (name == "y_minus") return fabs(x[1]) < tol;
  if (name == "y_plus") return fabs(x[1] - 1.0) < tol;
  return false;
}

/*--- Length (2D) or area (3D) of a marker. ---*/
passivedouble MarkerMeasure(const CSimplexMesh& mesh, const CSimplexMesh::Marker& marker) {
  const auto nDim = mesh.nDim;
  passivedouble sum = 0.0;
  for (auto iElem = 0ul; iElem < marker.GetnElem(nDim); ++iElem) {
    const auto* a = &mesh.coord[marker.elem[iElem * nDim] * nDim];
    const auto* b = &mesh.coord[marker.elem[iElem * nDim + 1] * nDim];
    if (nDim == 2) {
      sum += sqrt(pow(b[0] - a[0], 2) + pow(b[1] - a[1], 2));
    } else {
      const auto* c = &mesh.coord[marker.elem[iElem * nDim + 2] * nDim];
      const passivedouble u[] = {b[0] - a[0], b[1] - a[1], b[2] - a[2]};
      const passivedouble v[] = {c[0] - a[0], c[1] - a[1], c[2] - a[2]};
      sum += 0.5 * sqrt(pow(u[1] * v[2] - u[2] * v[1], 2) + pow(u[2] * v[0] - u[0] * v[2], 2) +
                        pow(u[0] * v[1] - u[1] * v[0], 2));
    }
  }
  return sum;
}

/*!
 * \brief Mesh file, config and geometry of a test case; the files are removed by the destructor.
 */
struct SimplexMeshCase {
  string meshFile;
  std::unique_ptr<CConfig> config;
  std::unique_ptr<CGeometry> geometry;

  SimplexMeshCase(unsigned short nDim, unsigned long n)
      : SimplexMeshCase(simplex_test::MakeSimplexMesh(nDim, n, nDim == 2 ? Marker2D : Marker3D),
                        (nDim == 2) ? "MARKER_FAR= (left, right, upper)\nMARKER_EULER= (lower_b, lower_a)\n"
                                    : "MARKER_FAR= (x_minus, x_plus, y_minus, y_plus, z_plus)\n"
                                      "MARKER_EULER= (z_minus_b, z_minus_a)\n") {}

  /*--- Any simplex mesh, with the marker (and other) options of the config. ---*/
  SimplexMeshCase(const CSimplexMesh& mesh, const string& markers) {
    meshFile = "mmg_interface_test_" + std::to_string(mesh.nDim) + "d.su2";
    simplex_test::WriteSU2Mesh(mesh, meshFile);
    stringstream options("SOLVER= EULER\nMESH_FORMAT= SU2\nMESH_FILENAME= " + meshFile + "\n" + markers +
                         "ADAP_HMIN= 1e-4\nADAP_HMAX= 10\n");
    auto origBuf = cout.rdbuf();
    cout.rdbuf(nullptr);
    config = std::unique_ptr<CConfig>(new CConfig(options, SU2_COMPONENT::SU2_CFD, false));
    {
      auto aux_geometry = std::unique_ptr<CGeometry>(new CPhysicalGeometry(config.get(), 0, 1));
      geometry = std::unique_ptr<CGeometry>(new CPhysicalGeometry(aux_geometry.get(), config.get()));
    }
    geometry->SetSendReceive(config.get());
    geometry->SetBoundaries(config.get());
    cout.rdbuf(origBuf);
  }
  ~SimplexMeshCase() { std::remove(meshFile.c_str()); }

  /*--- Uniform isotropic metric for the size h. ---*/
  su2activematrix UniformMetric(passivedouble h) const {
    const auto nDim = geometry->GetnDim();
    su2activematrix metric(geometry->GetnPoint(), CSimplexMesh::GetnMetric(nDim));
    for (auto iPoint = 0ul; iPoint < geometry->GetnPoint(); ++iPoint)
      for (unsigned short iDim = 0, iMet = 0; iDim < nDim; ++iDim)
        for (unsigned short jDim = iDim; jDim < nDim; ++jDim, ++iMet)
          metric(iPoint, iMet) = (iDim == jDim) ? 1.0 / (h * h) : 0.0;
    return metric;
  }
};

void CheckExtraction(unsigned short nDim, unsigned long n) {
  SimplexMeshCase test(nDim, n);
  CMMGInterface::CheckSupport(*test.config, *test.geometry);
  const auto mesh = CMMGInterface::ExtractMesh(*test.config, *test.geometry, test.UniformMetric(0.1));

  const unsigned long nPoint = (nDim == 2) ? (2 * n + 1) * (n + 1) : (n + 1) * (n + 1) * (n + 1);
  const unsigned long nElem = (nDim == 2) ? 4 * n * n : 6 * n * n * n;
  CHECK(mesh.GetnPoint() == nPoint);
  CHECK(mesh.GetnElem() == nElem);
  CHECK(mesh.metric.size() == nPoint * CSimplexMesh::GetnMetric(nDim));
  CHECK(mesh.markers.size() == (nDim == 2 ? 5u : 7u));

  /*--- References follow the alphabetical order of the marker names, not the order in the config or mesh file. ---*/
  const std::vector<string> names = (nDim == 2) ? std::vector<string>{"left", "lower_a", "lower_b", "right", "upper"}
                                                : std::vector<string>{"x_minus", "x_plus", "y_minus", "y_plus",
                                                                      "z_minus_a", "z_minus_b", "z_plus"};
  for (const auto& marker : mesh.markers) {
    const auto it = std::find(names.begin(), names.end(), marker.name);
    REQUIRE(it != names.end());
    CHECK(marker.ref == 1 + (it - names.begin()));
    CHECK(marker.ref == CMMGInterface::GetMarkerReference(*test.config, marker.name));
    CHECK(marker.GetnElem(nDim) > 0);
    for (const auto iPoint : marker.elem) CHECK(OnMarker(nDim, marker.name, &mesh.coord[iPoint * nDim]));
  }
  /*--- Markers in the config order (the order of the mesh output), not the alphabetical order of the mesh file. ---*/
  for (auto iMarker = 1ul; iMarker < mesh.markers.size(); ++iMarker) {
    CHECK(test.config->GetMarker_CfgFile_TagBound(mesh.markers[iMarker - 1].name) <
          test.config->GetMarker_CfgFile_TagBound(mesh.markers[iMarker].name));
  }
  CHECK(mesh.markers.front().name != (nDim == 2 ? "left" : "x_minus"));

  /*--- The split markers have half of the face each. ---*/
  const auto* a = mesh.FindMarker(nDim == 2 ? "lower_a" : "z_minus_a");
  const auto* b = mesh.FindMarker(nDim == 2 ? "lower_b" : "z_minus_b");
  REQUIRE(a != nullptr);
  REQUIRE(b != nullptr);
  CHECK(MarkerMeasure(mesh, *a) == Approx(nDim == 2 ? 1.0 : 0.5));
  CHECK(MarkerMeasure(mesh, *b) == Approx(nDim == 2 ? 1.0 : 0.5));

  /*--- Positive volumes (half of the input elements were negatively oriented) and a closed boundary. ---*/
  CMMGInterface::ValidateMesh(mesh, nullptr, "test");
}

/*--- Packed metric (upper triangle) R diag(eig) R^T, R a rotation by the angles a, b, c (about z, y, x). ---*/
std::vector<passivedouble> RotatedMetric(unsigned short nDim, const passivedouble* eig, passivedouble a,
                                         passivedouble b = 0.0, passivedouble c = 0.0) {
  const passivedouble Rz[3][3] = {{cos(a), -sin(a), 0.0}, {sin(a), cos(a), 0.0}, {0.0, 0.0, 1.0}};
  const passivedouble Ry[3][3] = {{cos(b), 0.0, sin(b)}, {0.0, 1.0, 0.0}, {-sin(b), 0.0, cos(b)}};
  const passivedouble Rx[3][3] = {{1.0, 0.0, 0.0}, {0.0, cos(c), -sin(c)}, {0.0, sin(c), cos(c)}};
  passivedouble Ryx[3][3] = {{0.0}}, R[3][3] = {{0.0}};
  for (int i = 0; i < 3; ++i)
    for (int j = 0; j < 3; ++j)
      for (int k = 0; k < 3; ++k) Ryx[i][j] += Ry[i][k] * Rx[k][j];
  for (int i = 0; i < 3; ++i)
    for (int j = 0; j < 3; ++j)
      for (int k = 0; k < 3; ++k) R[i][j] += Rz[i][k] * Ryx[k][j];

  std::vector<passivedouble> metric;
  for (unsigned short i = 0; i < nDim; ++i)
    for (unsigned short j = i; j < nDim; ++j) {
      passivedouble value = 0.0;
      for (unsigned short k = 0; k < nDim; ++k) value += R[i][k] * eig[k] * R[j][k];
      metric.push_back(value);
    }
  return metric;
}

}  // namespace

TEST_CASE("MMG interface: extraction of a 2D simplex mesh", "[MMG]") { CheckExtraction(2, 4); }

TEST_CASE("MMG interface: extraction of a 3D simplex mesh", "[MMG]") { CheckExtraction(3, 4); }

TEST_CASE("MMG interface: metric validity check", "[MMG]") {
  /*--- Rotated anisotropic 3D metric with eigenvalues of about (9998, 10000.25, 1e16) (aspect ratio 1e6): its
   *    expanded determinant is negative in floating point. ---*/
  const passivedouble anisotropic[] = {876094525110343.2,  -2193580787387677.8, 1783705963237083.0,
                                       5492325922541279.0, -4466074172598621.5, 3631579552368379.0};
  CHECK(CMMGInterface::IsFinitePositiveDefinite(3, anisotropic));

  /*--- Aspect ratio 1e6 at many orientations and scales, in 2D and 3D. ---*/
  bool accepted = true;
  for (int i = 0; i < 50; ++i) {
    const passivedouble big = pow(10.0, -4 + i % 21), a = 0.37 * i, b = 0.61 * i + 0.1, c = 1.13 * i + 0.2;
    const passivedouble eig3[] = {big, big * 1e-12, (i % 2) ? big : big * 1e-6};
    const passivedouble eig2[] = {big * 1e-12, big};
    accepted &= CMMGInterface::IsFinitePositiveDefinite(3, RotatedMetric(3, eig3, a, b, c).data());
    accepted &= CMMGInterface::IsFinitePositiveDefinite(2, RotatedMetric(2, eig2, a).data());
  }
  CHECK(accepted);

  /*--- Indefinite, negative definite, singular (rotated, round-off only) and non-finite tensors. ---*/
  const passivedouble inf = std::numeric_limits<passivedouble>::infinity();
  const passivedouble nan = std::numeric_limits<passivedouble>::quiet_NaN();
  const std::vector<std::vector<passivedouble>> invalid3 = {
      {1e4, 1e-2, -1e-2}, {1e4, 1e4, -1.0}, {-1.0, -2.0, -3.0}, {1e8, 0.0, 0.0}, {1e8, 1e8, 0.0}, {0.0, 0.0, 0.0}};
  bool rejected = true;
  for (int i = 0; i < 50; ++i) {
    const passivedouble a = 0.37 * i, b = 0.61 * i + 0.1, c = 1.13 * i + 0.2;
    for (const auto& eig : invalid3) {
      rejected &= !CMMGInterface::IsFinitePositiveDefinite(3, RotatedMetric(3, eig.data(), a, b, c).data());
      rejected &= !CMMGInterface::IsFinitePositiveDefinite(2, RotatedMetric(2, eig.data() + 1, a).data());
    }
  }
  CHECK(rejected);
  const passivedouble nonFinite3[][6] = {{nan, 0, 0, 1, 0, 1}, {1, 0, 0, inf, 0, 1}, {1, 0, inf, 1, 0, 1}};
  const passivedouble nonFinite2[][3] = {{nan, 0, 1}, {1, inf, 1}, {1, 0, -inf}};
  for (const auto* m : nonFinite3) CHECK_FALSE(CMMGInterface::IsFinitePositiveDefinite(3, m));
  for (const auto* m : nonFinite2) CHECK_FALSE(CMMGInterface::IsFinitePositiveDefinite(2, m));
  const passivedouble indefinite2[] = {1.0, 2.0, 1.0}, unit2[] = {1.0, 0.0, 1.0};
  CHECK_FALSE(CMMGInterface::IsFinitePositiveDefinite(2, indefinite2));
  CHECK(CMMGInterface::IsFinitePositiveDefinite(2, unit2));

  /*--- The anisotropic metric is extracted with the mesh. ---*/
  SimplexMeshCase test(3, 2);
  su2activematrix metric(test.geometry->GetnPoint(), 6);
  for (auto iPoint = 0ul; iPoint < test.geometry->GetnPoint(); ++iPoint)
    for (unsigned short iMet = 0; iMet < 6; ++iMet) metric(iPoint, iMet) = anisotropic[iMet];
  const auto mesh = CMMGInterface::ExtractMesh(*test.config, *test.geometry, metric);
  CHECK(mesh.metric.size() == 6 * test.geometry->GetnPoint());
}

#ifdef HAVE_MMG

namespace {

/*--- Element connectivity with the nodes of each element sorted. ---*/
std::vector<unsigned long> SortedElements(unsigned short nNode, std::vector<unsigned long> elem) {
  for (auto it = elem.begin(); it != elem.end(); it += nNode) std::sort(it, it + nNode);
  return elem;
}

void CheckRoundTrip(unsigned short nDim, unsigned long n) {
  SimplexMeshCase test(nDim, n);
  const auto mesh = CMMGInterface::ExtractMesh(*test.config, *test.geometry, test.UniformMetric(0.1));

  CMMGInterface mmg(*test.config);
  mmg.SetMesh(mesh);
  const auto back = mmg.GetMesh();

  CHECK(back.nDim == mesh.nDim);
  CHECK(back.coord == mesh.coord);
  CHECK(back.metric == mesh.metric);
  CHECK(back.elem == mesh.elem);
  CHECK(back.elemRef == mesh.elemRef);
  REQUIRE(back.markers.size() == mesh.markers.size());
  for (auto iMarker = 0ul; iMarker < mesh.markers.size(); ++iMarker) {
    CHECK(back.markers[iMarker].name == mesh.markers[iMarker].name);
    CHECK(back.markers[iMarker].ref == mesh.markers[iMarker].ref);
    /*--- MMG may reorder the nodes of boundary elements (orientation is not prescribed). ---*/
    CHECK(SortedElements(nDim, back.markers[iMarker].elem) == SortedElements(nDim, mesh.markers[iMarker].elem));
  }
  CMMGInterface::ValidateMesh(back, &mesh, "round trip");
}

/*--- Mean edge length measured in a uniform isotropic metric of size h. ---*/
passivedouble MeanEdgeLength(const CSimplexMesh& mesh, passivedouble h) {
  const auto nDim = mesh.nDim;
  passivedouble sum = 0.0;
  unsigned long count = 0;
  for (auto iElem = 0ul; iElem < mesh.GetnElem(); ++iElem)
    for (unsigned short i = 0; i <= nDim; ++i)
      for (unsigned short j = i + 1; j <= nDim; ++j) {
        const auto* a = &mesh.coord[mesh.elem[iElem * (nDim + 1) + i] * nDim];
        const auto* b = &mesh.coord[mesh.elem[iElem * (nDim + 1) + j] * nDim];
        passivedouble len2 = 0.0;
        for (unsigned short iDim = 0; iDim < nDim; ++iDim) len2 += pow(b[iDim] - a[iDim], 2);
        sum += sqrt(len2) / h;
        ++count;
      }
  return sum / count;
}

void CheckUniformRemesh(unsigned short nDim, unsigned long n, passivedouble factor) {
  SimplexMeshCase test(nDim, n);
  const passivedouble h0 = 1.0 / n, h = factor * h0;
  const auto mesh = CMMGInterface::ExtractMesh(*test.config, *test.geometry, test.UniformMetric(h));

  CMMGInterface mmg(*test.config);
  const auto adapted = mmg.Adapt(mesh);  // validates positive volumes, closed boundary, no lost marker

  /*--- Size: unit mean edge length in the metric, element count of a regular mesh of size h. ---*/
  const passivedouble volume = (nDim == 2) ? 2.0 : 1.0;
  const passivedouble elemVolume = (nDim == 2) ? sqrt(3.0) / 4.0 * h * h : h * h * h / (6.0 * sqrt(2.0));
  const passivedouble nExpected = volume / elemVolume;
  CHECK(MeanEdgeLength(adapted, h) > 0.75);
  CHECK(MeanEdgeLength(adapted, h) < 1.35);
  CHECK(adapted.GetnElem() > 0.5 * nExpected);
  CHECK(adapted.GetnElem() < 2.5 * nExpected);  // regular tetrahedra do not tile space, MMG gives about 1.5x
  if (factor < 1.0) CHECK(adapted.GetnPoint() > mesh.GetnPoint());
  if (factor > 1.0) CHECK(adapted.GetnPoint() < mesh.GetnPoint());

  /*--- Markers keep their names, references, positions and lengths/areas (flat boundaries). ---*/
  REQUIRE(adapted.markers.size() == mesh.markers.size());
  for (const auto& marker : adapted.markers) {
    const auto* input = mesh.FindMarker(marker.name);
    REQUIRE(input != nullptr);
    CHECK(marker.ref == input->ref);
    CHECK(marker.GetnElem(nDim) > 0);
    bool onMarker = true;
    for (const auto iPoint : marker.elem) onMarker &= OnMarker(nDim, marker.name, &adapted.coord[iPoint * nDim]);
    CHECK(onMarker);
    CHECK(MarkerMeasure(adapted, marker) == Approx(MarkerMeasure(mesh, *input)).epsilon(1e-10));
  }
}

/*--- Coordinates of the boundary points of a mesh. ---*/
std::set<std::array<passivedouble, 3>> BoundaryCoordinates(const CSimplexMesh& mesh) {
  std::set<std::array<passivedouble, 3>> points;
  for (const auto& marker : mesh.markers)
    for (const auto iPoint : marker.elem) {
      std::array<passivedouble, 3> x = {0.0, 0.0, 0.0};
      for (unsigned short iDim = 0; iDim < mesh.nDim; ++iDim) x[iDim] = mesh.coord[iPoint * mesh.nDim + iDim];
      points.insert(x);
    }
  return points;
}

/*!
 * \brief Volume-only adaptation (ADAP_SURFACE= NO, MMG -nosurf) of a disk/ball whose boundary points lie on the
 *        circle/sphere: refined twice in the volume, the boundary points stay bitwise the same and the boundary faces
 *        the same, also after the adapted mesh is built as an SU2 geometry from memory and extracted again. With
 *        the surface adapted (the default) MMG inserts points on its reconstruction of the curved boundary.
 */
void CheckFixedSurface(unsigned short nDim, unsigned long n) {
  SimplexMeshCase test(simplex_test::MakeRoundMesh(nDim, n, 1.0), "MARKER_EULER= (round_a, round_b)\nADAP_SURFACE= NO\n");
  const passivedouble h = 1.0 / n;  // half of the input size (diameter 2, n cells)
  const auto mesh = CMMGInterface::ExtractMesh(*test.config, *test.geometry, test.UniformMetric(h));
  const auto inputBoundary = BoundaryCoordinates(mesh);

  CMMGInterface mmg(*test.config);
  REQUIRE_FALSE(mmg.GetParameters().surface);
  const auto adapted = mmg.Adapt(mesh);  // includes CheckSameBoundary
  CMMGInterface::CheckSameBoundary(adapted, mesh, "fixed surface");
  CHECK(adapted.GetnPoint() > 2 * mesh.GetnPoint());
  CHECK(BoundaryCoordinates(adapted) == inputBoundary);
  for (const auto& marker : mesh.markers) {
    const auto* adaptedMarker = adapted.FindMarker(marker.name);
    REQUIRE(adaptedMarker != nullptr);
    CHECK(adaptedMarker->ref == marker.ref);
    CHECK(MarkerMeasure(adapted, *adaptedMarker) == Approx(MarkerMeasure(mesh, marker)).epsilon(1e-14));
  }

  /*--- Geometry built from the adapted mesh in memory (as the driver does), extracted again. ---*/
  {
    auto origBuf = cout.rdbuf();
    cout.rdbuf(nullptr);
    CMemoryMeshReaderFVM reader(test.config.get(), adapted, 0, 1);
    CPhysicalGeometry aux(test.config.get(), reader, 1);
    CPhysicalGeometry geometry(&aux, test.config.get());
    geometry.SetSendReceive(test.config.get());
    geometry.SetBoundaries(test.config.get());
    cout.rdbuf(origBuf);
    su2activematrix metric(geometry.GetnPoint(), CSimplexMesh::GetnMetric(nDim));
    for (auto iPoint = 0ul; iPoint < geometry.GetnPoint(); ++iPoint)
      for (unsigned short iDim = 0, iMet = 0; iDim < nDim; ++iDim)
        for (unsigned short jDim = iDim; jDim < nDim; ++jDim, ++iMet) metric(iPoint, iMet) = (iDim == jDim) ? 1.0 : 0.0;
    const auto rebuilt = CMMGInterface::ExtractMesh(*test.config, geometry, metric);
    CMMGInterface::CheckSameBoundary(rebuilt, mesh, "fixed surface, geometry from memory");
    CHECK(rebuilt.GetnPoint() == adapted.GetnPoint());
  }

  /*--- The surface adapted: new boundary points on MMG's curved reconstruction of the boundary. ---*/
  CMMGInterface mmgSurface(*test.config);
  mmgSurface.GetParameters().surface = true;
  const auto withSurface = BoundaryCoordinates(mmgSurface.Adapt(mesh));
  CHECK(withSurface.size() > inputBoundary.size());
  unsigned long nNew = 0;
  for (const auto& x : withSurface) nNew += inputBoundary.count(x) == 0;
  CHECK(nNew > 0);
}

}  // namespace

TEST_CASE("MMG interface: volume-only adaptation (fixed surface)", "[MMG]") {
  CheckFixedSurface(2, 4);
  CheckFixedSurface(3, 4);
}

TEST_CASE("MMG interface: 2D round trip", "[MMG]") { CheckRoundTrip(2, 4); }

TEST_CASE("MMG interface: 3D round trip", "[MMG]") { CheckRoundTrip(3, 4); }

TEST_CASE("MMG interface: 2D uniform refinement and coarsening", "[MMG]") {
  CheckUniformRemesh(2, 8, 0.5);
  CheckUniformRemesh(2, 8, 2.0);
}

TEST_CASE("MMG interface: 3D uniform refinement and coarsening", "[MMG]") {
  CheckUniformRemesh(3, 4, 0.5);
  CheckUniformRemesh(3, 8, 2.0);
}

#endif
