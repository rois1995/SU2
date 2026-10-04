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

TEST_CASE("MMG interface: metric floor at fixed boundary faces", "[MMG]") {
  /*--- Metric of sizes hx along x and hy along y at every point. After the floor, every boundary edge is at most 1 long
   *    in the metric of its points (edges of length h: sizes along the boundary at least h), interior points and
   *    points whose boundary edges were already short enough are unchanged, and on a boundary point whose edges
   *    are all along x (not a corner) the size along y is kept. ---*/
  for (const unsigned short nDim : {2, 3}) {
    const unsigned long n = 4;
    const passivedouble h = 1.0 / n;
    auto mesh = simplex_test::MakeSimplexMesh(nDim, n, nDim == 2 ? simplex_test::Marker2D : simplex_test::Marker3D);
    const auto nMetric = CSimplexMesh::GetnMetric(nDim);
    const passivedouble hx = 0.1 * h, hy = 0.01 * h;  // finer than the edges along x and y
    mesh.metric.assign(mesh.GetnPoint() * nMetric, 0.0);
    for (unsigned long iPoint = 0; iPoint < mesh.GetnPoint(); ++iPoint) {
      auto* m = &mesh.metric[iPoint * nMetric];
      m[0] = 1.0 / (hx * hx);                              // xx
      m[nDim] = 1.0 / (hy * hy);                           // yy: index 2 (2D) or 3 (3D)
      if (nDim == 3) m[5] = 1.0 / (h * h);                 // zz: as long as the edges along z
    }
    const auto original = mesh.metric;
    CMMGInterface::FloorFixedBoundaryMetric(mesh);

    std::vector<bool> onBoundary(mesh.GetnPoint(), false);
    for (const auto& marker : mesh.markers)
      for (const auto iPoint : marker.elem) onBoundary[iPoint] = true;

    auto length = [&](unsigned long iPoint, const passivedouble* e) {
      passivedouble M[3][3] = {{0.0}}, l2 = 0.0;
      for (unsigned short i = 0, k = 0; i < nDim; ++i)
        for (unsigned short j = i; j < nDim; ++j, ++k) M[i][j] = M[j][i] = mesh.metric[iPoint * nMetric + k];
      for (unsigned short i = 0; i < nDim; ++i)
        for (unsigned short j = 0; j < nDim; ++j) l2 += e[i] * M[i][j] * e[j];
      return sqrt(l2);
    };
    passivedouble maxLength = 0.0;
    for (const auto& marker : mesh.markers) {
      for (unsigned long iFace = 0; iFace < marker.GetnElem(nDim); ++iFace) {
        const auto* face = &marker.elem[iFace * nDim];
        for (unsigned short a = 0; a < nDim; ++a)
          for (unsigned short b = 0; b < nDim; ++b) {
            if (a == b) continue;
            passivedouble e[3] = {0.0};
            for (unsigned short iDim = 0; iDim < nDim; ++iDim)
              e[iDim] = mesh.coord[face[b] * nDim + iDim] - mesh.coord[face[a] * nDim + iDim];
            maxLength = std::max(maxLength, length(face[a], e));
          }
      }
    }
    CHECK(maxLength <= 1.0 + 1e-10);

    unsigned long nChecked = 0;
    for (unsigned long iPoint = 0; iPoint < mesh.GetnPoint(); ++iPoint) {
      const auto* x = &mesh.coord[iPoint * nDim];
      const auto* m = &mesh.metric[iPoint * nMetric];
      if (!onBoundary[iPoint]) {
        for (unsigned short k = 0; k < nMetric; ++k) CHECK(m[k] == original[iPoint * nMetric + k]);
        continue;
      }
      /*--- 2D: points of y = 0 and y = 1 away from the corners have edges along x only. ---*/
      if (nDim == 2 && (x[1] < 1e-12 || x[1] > 1.0 - 1e-12) && x[0] > 1e-12 && x[0] < 2.0 - 1e-12) {
        CHECK(m[0] == Approx(1.0 / (h * h)));
        CHECK(fabs(m[1]) <= 1e-9 * m[2]);
        CHECK(m[2] == Approx(1.0 / (hy * hy)));
        ++nChecked;
      }
    }
    if (nDim == 2) CHECK(nChecked == 2 * (2 * n - 1));
  }
}

TEST_CASE("MMG interface: metric floor at a curved fixed boundary", "[MMG]") {
  /*--- Disk with a boundary-layer-like metric at its boundary points: size 1e-4 along the radius, 1 along the
   *    circle. The chords are inclined to the circle: after the floor every boundary edge is at most 1 long in the
   *    metric of its points, the metric is nowhere finer than before (M_old - M_new positive semi-definite), and the
   *    radial size is raised to the normal extent of the chords (about |e| sin(turn / 2), here between 1e-3 and 1e-1;
   *    the polygon of 32 chords turns by about 11 degrees per point). ---*/
  auto mesh = simplex_test::MakeRoundMesh(2, 8, 1.0);
  const passivedouble hn = 1e-4;
  mesh.metric.assign(mesh.GetnPoint() * 3, 0.0);
  for (unsigned long iPoint = 0; iPoint < mesh.GetnPoint(); ++iPoint) {
    const auto* x = &mesh.coord[2 * iPoint];
    const passivedouble r = sqrt(x[0] * x[0] + x[1] * x[1]);
    const passivedouble n[2] = {r > 0 ? x[0] / r : 1.0, r > 0 ? x[1] / r : 0.0};
    const passivedouble t[2] = {-n[1], n[0]};
    for (int k = 0, i = 0; i < 2; ++i)
      for (int j = i; j < 2; ++j, ++k) mesh.metric[3 * iPoint + k] = n[i] * n[j] / (hn * hn) + t[i] * t[j];
  }
  const auto original = mesh.metric;
  CMMGInterface::FloorFixedBoundaryMetric(mesh);

  passivedouble maxLength = 0.0;
  unsigned long nBoundary = 0;
  std::vector<bool> onBoundary(mesh.GetnPoint(), false);
  for (const auto& marker : mesh.markers)
    for (unsigned long iFace = 0; iFace < marker.GetnElem(2); ++iFace) {
      const unsigned long ends[2] = {marker.elem[2 * iFace], marker.elem[2 * iFace + 1]};
      const passivedouble e[2] = {mesh.coord[2 * ends[1]] - mesh.coord[2 * ends[0]],
                                  mesh.coord[2 * ends[1] + 1] - mesh.coord[2 * ends[0] + 1]};
      for (const auto p : ends) {
        onBoundary[p] = true;
        const auto* m = &mesh.metric[3 * p];
        maxLength = std::max(maxLength, sqrt(m[0] * e[0] * e[0] + 2 * m[1] * e[0] * e[1] + m[2] * e[1] * e[1]));
      }
    }
  CHECK(maxLength <= 1.0 + 1e-10);
  for (unsigned long iPoint = 0; iPoint < mesh.GetnPoint(); ++iPoint) {
    const auto* m = &mesh.metric[3 * iPoint];
    const auto* m0 = &original[3 * iPoint];
    if (!onBoundary[iPoint]) {
      for (int k = 0; k < 3; ++k) CHECK(m[k] == m0[k]);
      continue;
    }
    ++nBoundary;
    /*--- M_old - M_new >= 0 (2x2: trace and determinant, relative to the scale of M_old). ---*/
    const passivedouble d[3] = {m0[0] - m[0], m0[1] - m[1], m0[2] - m[2]};
    const passivedouble scale = m0[0] + m0[2];
    CHECK(d[0] + d[2] >= -1e-10 * scale);
    CHECK(d[0] * d[2] - d[1] * d[1] >= -1e-10 * scale * scale);
    const auto* x = &mesh.coord[2 * iPoint];
    const passivedouble r = sqrt(x[0] * x[0] + x[1] * x[1]), n[2] = {x[0] / r, x[1] / r};
    const passivedouble radialSize = 1.0 / sqrt(m[0] * n[0] * n[0] + 2 * m[1] * n[0] * n[1] + m[2] * n[1] * n[1]);
    CHECK(radialSize > 1e-3);
    CHECK(radialSize < 1e-1);
  }
  CHECK(nBoundary == 32);
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


TEST_CASE("MMG interface: coarsening ratio of two metrics", "[MMG]") {
  const passivedouble in2[3] = {1.0, 0.0, 4.0};
  const passivedouble same2[3] = {1.0, 0.0, 4.0};
  const passivedouble coarse2[3] = {1.0, 0.0, 1.0};   // 2x coarser along y
  const passivedouble finer2[3] = {9.0, 0.5, 16.0};
  CHECK(CMMGInterface::CoarseningRatio(2, in2, same2) == Approx(1.0).epsilon(1e-12));
  CHECK(CMMGInterface::CoarseningRatio(2, in2, coarse2) == Approx(0.25).epsilon(1e-12));
  CHECK(CMMGInterface::CoarseningRatio(2, in2, finer2) > 1.0);
  /*--- Strongly anisotropic (a boundary-layer metric, aspect ratio 2e4) against MMG's isotropic hmax 0.5. ---*/
  const passivedouble bl[3] = {1.0 / (0.04 * 0.04), 0.0, 1.0 / (2e-6 * 2e-6)};
  const passivedouble iso[3] = {4.0, 0.0, 4.0};
  CHECK(CMMGInterface::CoarseningRatio(2, bl, iso) == Approx(4.0 * 4e-12).epsilon(1e-6));
  const passivedouble in3[6] = {1.0, 0.0, 0.0, 2.0, 0.0, 3.0};
  const passivedouble coarse3[6] = {1.0, 0.0, 0.0, 2.0, 0.0, 0.3};
  CHECK(CMMGInterface::CoarseningRatio(3, in3, coarse3) == Approx(0.1).epsilon(1e-12));
}

namespace {

/*!
 * \brief Boundary-layer metric (first height h0 2e-6, growth 1.15 per row, tangential size 0.25) on the walls z = 0
 *        (3D) or y = 0 (2D) of the coarse box, hmax 0.5: hmax / h0 = 2.5e5, the regime where MMG 5.6 replaced the
 *        metric at the wall points by its isotropic geometric metric (MMG issue #331). Returns the mesh adapted with
 *        a fixed surface and the face heights of the wall faces.
 */
CSimplexMesh WallMetricCase(unsigned short nDim, bool localHmax, CMMGInterface::MetricCheck& check,
                            std::vector<passivedouble>& heights) {
  SimplexMeshCase test(nDim, 2);
  const passivedouble h0 = 2e-6, t = 0.25;
  const auto normal = nDim - 1;
  su2activematrix metric(test.geometry->GetnPoint(), CSimplexMesh::GetnMetric(nDim));
  for (auto iPoint = 0ul; iPoint < test.geometry->GetnPoint(); ++iPoint) {
    const passivedouble d = SU2_TYPE::GetValue(test.geometry->nodes->GetCoord(iPoint, normal));
    const passivedouble hn = std::min(h0 + 0.15 * d, t);
    for (unsigned short iDim = 0, iMet = 0; iDim < nDim; ++iDim)
      for (unsigned short jDim = iDim; jDim < nDim; ++jDim, ++iMet)
        metric(iPoint, iMet) = (iDim != jDim) ? 0.0 : (iDim == normal ? 1.0 / (hn * hn) : 1.0 / (t * t));
  }
  const auto mesh = CMMGInterface::ExtractMesh(*test.config, *test.geometry, metric);
  CMMGInterface mmg(*test.config);
  auto& params = mmg.GetParameters();
  params.surface = false;
  params.hmin = 1e-6;
  params.hmax = 0.5;
  params.swap = 1;
  params.localWallHmax = localHmax;
  params.boundaryLayerMarkers = (nDim == 2) ? std::vector<string>{"lower_a", "lower_b"}
                                            : std::vector<string>{"z_minus_a", "z_minus_b"};
  if (localHmax) {
    const auto local = mmg.WallLocalParameters(mesh);
    REQUIRE(local.size() == 2);
    for (const auto& param : local) CHECK(param.hmax == Approx(0.5));  // 2 x the longest wall edge (0.25 / 0.354)
  }
  auto adapted = mmg.Adapt(mesh);
  check = mmg.GetMetricCheck();

  /*--- Face heights of the wall faces: distance of the opposite vertex of their element from the wall. ---*/
  std::set<std::vector<unsigned long>> wallFaces;
  for (const auto& marker : adapted.markers) {
    if (std::find(params.boundaryLayerMarkers.begin(), params.boundaryLayerMarkers.end(), marker.name) ==
        params.boundaryLayerMarkers.end())
      continue;
    for (auto iFace = 0ul; iFace < marker.GetnElem(nDim); ++iFace) {
      std::vector<unsigned long> face(&marker.elem[iFace * nDim], &marker.elem[iFace * nDim] + nDim);
      std::sort(face.begin(), face.end());
      wallFaces.insert(face);
    }
  }
  heights.clear();
  for (auto iElem = 0ul; iElem < adapted.GetnElem(); ++iElem) {
    const auto* elem = &adapted.elem[iElem * (nDim + 1)];
    for (unsigned short k = 0; k <= nDim; ++k) {
      std::vector<unsigned long> face;
      for (unsigned short j = 0; j <= nDim; ++j)
        if (j != k) face.push_back(elem[j]);
      std::sort(face.begin(), face.end());
      if (wallFaces.count(face)) heights.push_back(adapted.coord[elem[k] * nDim + normal]);
    }
  }
  return adapted;
}

}  // namespace

TEST_CASE("MMG interface: boundary-layer metric kept at the fixed wall points (MMG issue 331)", "[MMG]") {
  for (const bool localHmax : {false, true}) {
    CMMGInterface::MetricCheck check;
    std::vector<passivedouble> heights;
    const auto adapted = WallMetricCase(2, localHmax, check, heights);
    CHECK(check.nChecked > 0);
    CHECK(check.nViolations == 0);   // fails with stock MMG 5.6 without the local parameters
    CHECK(check.worstRatio > 1.0 - CMMGInterface::metricCheckTolerance);
    REQUIRE(!heights.empty());
    std::sort(heights.begin(), heights.end());
    /*--- MMG builds the near-wall cells from the coarse input: first cells about h0. ---*/
    CHECK(heights[heights.size() / 2] < 2.0 * 2e-6);
    CHECK(heights.back() < 4.0 * 2e-6);
  }
}

TEST_CASE("MMG interface: boundary-layer metric at the fixed wall points in 3D", "[MMG]") {
  CMMGInterface::MetricCheck check;
  std::vector<passivedouble> heights;
  const auto adapted = WallMetricCase(3, true, check, heights);
  CHECK(check.nChecked > 0);
  CHECK(check.nViolations == 0);
}

#endif
