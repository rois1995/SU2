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

#include "../../../Common/include/CConfig.hpp"
#include "../../../Common/include/adaptation/CMMGInterface.hpp"
#include "../../../Common/include/geometry/CPhysicalGeometry.hpp"
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

  SimplexMeshCase(unsigned short nDim, unsigned long n) {
    meshFile = "mmg_interface_test_" + std::to_string(nDim) + "d.su2";
    simplex_test::WriteSU2Mesh(simplex_test::MakeSimplexMesh(nDim, n, nDim == 2 ? Marker2D : Marker3D), meshFile);

    const string markers = (nDim == 2) ? "MARKER_FAR= (left, right, upper)\nMARKER_EULER= (lower_b, lower_a)\n"
                                       : "MARKER_FAR= (x_minus, x_plus, y_minus, y_plus, z_plus)\n"
                                         "MARKER_EULER= (z_minus_b, z_minus_a)\n";
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

}  // namespace

TEST_CASE("MMG interface: extraction of a 2D simplex mesh", "[MMG]") { CheckExtraction(2, 4); }

TEST_CASE("MMG interface: extraction of a 3D simplex mesh", "[MMG]") { CheckExtraction(3, 4); }

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

}  // namespace

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
