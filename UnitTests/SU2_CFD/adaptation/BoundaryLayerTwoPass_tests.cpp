/*!
 * \file BoundaryLayerTwoPass_tests.cpp
 * \brief Unit tests of the reference wall, the wall size rule and the two-pass boundary-layer remesh (2D).
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

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <set>

#include "../../../Common/include/CConfig.hpp"
#include "../../../Common/include/adaptation/CMMGInterface.hpp"
#include "../../../SU2_CFD/include/adaptation/CBoundaryLayerRemesher.hpp"
#include "../../../SU2_CFD/include/adaptation/CReferenceWall.hpp"

namespace {

/*--- Points on a closed polyline (the "wall" marker) only: the reference wall needs points and boundary lines. ---*/
CSimplexMesh PolylineMesh(const std::vector<std::array<passivedouble, 2>>& points, bool closed) {
  CSimplexMesh mesh;
  mesh.nDim = 2;
  for (const auto& x : points) mesh.coord.insert(mesh.coord.end(), x.begin(), x.end());
  CSimplexMesh::Marker wall;
  wall.name = "wall";
  wall.ref = 1;
  const auto n = points.size();
  for (unsigned long i = 0; i + (closed ? 0 : 1) < n; ++i) {
    wall.elem.push_back(i);
    wall.elem.push_back((i + 1) % n);
  }
  mesh.markers.push_back(wall);
  return mesh;
}

std::vector<std::array<passivedouble, 2>> Circle(unsigned long n, passivedouble radius, passivedouble phase = 0.0) {
  std::vector<std::array<passivedouble, 2>> points;
  for (unsigned long i = 0; i < n; ++i) {
    const auto a = phase + 2.0 * M_PI * i / n;
    points.push_back({radius * cos(a), radius * sin(a)});
  }
  return points;
}

/*!
 * \brief Annulus mesh: wall circle radius r0 (marker "wall"), far-field circle radius r1 ("farfield"), nTheta x nR
 *        quads split into positively oriented triangles; radii graded geometrically.
 */
CSimplexMesh Annulus(unsigned long nTheta, unsigned long nR, passivedouble r0, passivedouble r1) {
  CSimplexMesh mesh;
  mesh.nDim = 2;
  for (unsigned long j = 0; j <= nR; ++j) {
    const auto r = r0 * pow(r1 / r0, static_cast<passivedouble>(j) / nR);
    for (unsigned long i = 0; i < nTheta; ++i) {
      const auto a = 2.0 * M_PI * i / nTheta;
      mesh.coord.push_back(r * cos(a));
      mesh.coord.push_back(r * sin(a));
    }
  }
  auto id = [&](unsigned long i, unsigned long j) { return (i % nTheta) + j * nTheta; };
  for (unsigned long j = 0; j < nR; ++j)
    for (unsigned long i = 0; i < nTheta; ++i) {
      mesh.elem.insert(mesh.elem.end(), {id(i, j), id(i + 1, j + 1), id(i + 1, j)});
      mesh.elem.insert(mesh.elem.end(), {id(i, j), id(i, j + 1), id(i + 1, j + 1)});
    }
  mesh.elemRef.assign(mesh.GetnElem(), 0);
  CSimplexMesh::Marker far, wall;
  far.name = "farfield";
  far.ref = 1;
  wall.name = "wall";
  wall.ref = 2;
  for (unsigned long i = 0; i < nTheta; ++i) {
    wall.elem.insert(wall.elem.end(), {id(i + 1, 0), id(i, 0)});
    far.elem.insert(far.elem.end(), {id(i, nR), id(i + 1, nR)});
  }
  mesh.markers = {far, wall};
  return mesh;
}

struct Mute {
  std::streambuf* buffer = cout.rdbuf();
  Mute() { cout.rdbuf(nullptr); }
  ~Mute() { cout.rdbuf(buffer); }
};

std::unique_ptr<CConfig> MakeConfig(const string& options) {
  const bool quiet = getenv("SU2_TEST_VERBOSE") == nullptr;
  std::unique_ptr<Mute> mute(quiet ? new Mute : nullptr);
  stringstream ss("SOLVER= NAVIER_STOKES\nREYNOLDS_NUMBER= 1e6\nMACH_NUMBER= 0.5\nMESH_FORMAT= SU2\n"
                  "MESH_FILENAME= unused.su2\nMGLEVEL= 0\nCOMPUTE_METRIC= YES\nADAP_SENSOR= (MACH)\n"
                  "MARKER_HEATFLUX= (wall, 0.0)\nMARKER_FAR= (farfield)\n" + options);
  return std::unique_ptr<CConfig>(new CConfig(ss, SU2_COMPONENT::SU2_CFD, false));
}

}  // namespace

TEST_CASE("Reference wall: periodic spline through a circle", "[Adaptation]") {
  const passivedouble R = 1.0;
  CReferenceWall wall(PolylineMesh(Circle(64, R), true), {"wall"}, 45.0);
  REQUIRE(wall.GetSegments().size() == 1);
  const auto& seg = wall.GetSegments()[0];
  CHECK(seg.closed);
  /*--- Spline vs circle: far better than the polyline (sagitta R (1 - cos(pi/64)) = 1.2e-3). ---*/
  passivedouble maxDev = 0.0, maxCurvErr = 0.0;
  for (int k = 0; k < 1000; ++k) {
    const auto s = seg.s.back() * (k + 0.5) / 1000;
    passivedouble x[2];
    wall.Evaluate(seg, s, x);
    maxDev = std::max(maxDev, fabs(std::hypot(x[0], x[1]) - R));
    maxCurvErr = std::max(maxCurvErr, fabs(wall.Curvature(seg, s) * R - 1.0));
  }
  CHECK(maxDev < 1e-5);
  CHECK(maxCurvErr < 0.01);

  /*--- Projection of points off the circle. ---*/
  for (const auto r : {0.9, 1.1, 1.5}) {
    for (int k = 0; k < 7; ++k) {
      const auto a = 0.3 + 0.9 * k;
      const passivedouble p[2] = {r * cos(a), r * sin(a)};
      const auto proj = wall.Project(p, "wall");
      REQUIRE(proj.segment == 0);
      CHECK(proj.x[0] == Approx(cos(a)).margin(2e-5));
      CHECK(proj.x[1] == Approx(sin(a)).margin(2e-5));
      CHECK(proj.distance == Approx(fabs(r - 1.0)).margin(2e-5));
      CHECK(fabs(proj.tangent[0] * cos(a) + proj.tangent[1] * sin(a)) < 1e-3);  // tangent normal to the radius
    }
  }
  CHECK(wall.Project(std::array<passivedouble, 2>{2.0, 0.0}.data(), "other").segment < 0);
}

TEST_CASE("Reference wall: sharp corners, marker ends and persistence", "[Adaptation]") {
  /*--- Square with 4 points per side: 4 sharp corners give 4 straight segments. ---*/
  std::vector<std::array<passivedouble, 2>> square;
  for (int side = 0; side < 4; ++side)
    for (int k = 0; k < 4; ++k) {
      const passivedouble t = k / 4.0;
      const std::array<passivedouble, 2> corners[4] = {{0, 0}, {1, 0}, {1, 1}, {0, 1}};
      const auto& a = corners[side];
      const auto& b = corners[(side + 1) % 4];
      square.push_back({a[0] + t * (b[0] - a[0]), a[1] + t * (b[1] - a[1])});
    }
  CReferenceWall wall(PolylineMesh(square, true), {"wall"}, 45.0);
  REQUIRE(wall.GetSegments().size() == 4);
  for (const auto& seg : wall.GetSegments()) {
    CHECK_FALSE(seg.closed);
    CHECK(seg.sharpStart);
    CHECK(seg.sharpEnd);
    CHECK(seg.x.size() == 5);
    CHECK(wall.Curvature(seg, 0.5 * seg.s.back()) < 1e-12);  // straight
  }
  const passivedouble p[2] = {0.3, -0.2};
  const auto proj = wall.Project(p, "wall");
  CHECK(proj.x[0] == Approx(0.3).margin(1e-12));
  CHECK(proj.x[1] == Approx(0.0).margin(1e-12));

  /*--- Open polyline: one segment with free (not sharp) ends. ---*/
  CReferenceWall open(PolylineMesh({{0, 0}, {1, 0.1}, {2, 0.15}, {3, 0.1}}, false), {"wall"}, 45.0);
  REQUIRE(open.GetSegments().size() == 1);
  CHECK_FALSE(open.GetSegments()[0].sharpStart);
  CHECK_FALSE(open.GetSegments()[0].sharpEnd);

  /*--- Write and read back: same fingerprint, same projection. ---*/
  const string file = "bl_reference_wall_test.dat";
  wall.Write(file);
  CReferenceWall copy;
  REQUIRE(copy.Read(file));
  CHECK(copy.Fingerprint() == wall.Fingerprint());
  const auto proj2 = copy.Project(p, "wall");
  CHECK(proj2.x[0] == proj.x[0]);
  CHECK(proj2.x[1] == proj.x[1]);
  std::remove(file.c_str());
  CReferenceWall missing;
  CHECK_FALSE(missing.Read("no_such_reference_wall.dat"));
}

TEST_CASE("Wall size rule: curvature cap, sensor, corners, gradation, discretization independence", "[Adaptation]") {
  BLWallRule::SizeRule rule;
  rule.h0 = 1e-4;
  rule.curvatureFactor = 0.7;
  rule.hmin = 1e-6;
  rule.hmax = 10.0;
  const auto cap = 0.7 * sqrt(2.0 * 1.0 * 1e-4);  // R = 1
  auto coarse = [](const passivedouble*, const passivedouble*) { return 1.0; };

  /*--- Circle: the curvature cap everywhere; the same for two discretizations of the same circle (the size does not
   *    depend on the wall edges, only on the reference geometry). ---*/
  for (const unsigned long n : {64ul, 100ul}) {
    CReferenceWall wall(PolylineMesh(Circle(n, 1.0, 0.1), true), {"wall"}, 45.0);
    const auto samples = BLWallRule::SampleSize(wall, 0, rule, coarse);
    for (const auto t : samples.size) CHECK(t == Approx(cap).epsilon(0.01));
    CHECK(samples.nConflict == 0);
    CHECK(samples.nCurvatureLimited == samples.size.size());
  }
  /*--- A sensor finer than the cap wins. ---*/
  {
    CReferenceWall wall(PolylineMesh(Circle(64, 1.0), true), {"wall"}, 45.0);
    auto fine = [](const passivedouble*, const passivedouble*) { return 1e-3; };
    for (const auto t : BLWallRule::SampleSize(wall, 0, rule, fine).size) CHECK(t == Approx(1e-3));
  }
  /*--- Conflict: 2 h0 above the cap (R = 1e-3, h0 = 1e-3: cap 9.9e-4 < 2e-3). ---*/
  {
    CReferenceWall wall(PolylineMesh(Circle(64, 1e-3), true), {"wall"}, 45.0);
    BLWallRule::SizeRule tight = rule;
    tight.h0 = 1e-3;
    const auto samples = BLWallRule::SampleSize(wall, 0, tight, coarse);
    CHECK(samples.nConflict == samples.size.size());
    for (const auto t : samples.size) CHECK(t == Approx(2e-3));
  }
  /*--- Square (straight sides): the sensor size away from the corners, max(t_min, 0.5 r) near them, gradation. ---*/
  {
    std::vector<std::array<passivedouble, 2>> square = {{0, 0}, {0.5, 0}, {1, 0}, {1, 0.5}, {1, 1}, {0.5, 1}, {0, 1},
                                                        {0, 0.5}};
    CReferenceWall wall(PolylineMesh(square, true), {"wall"}, 45.0);
    auto sensor = [](const passivedouble*, const passivedouble*) { return 0.05; };
    BLWallRule::SizeRule r2 = rule;
    r2.h0 = 1e-4;
    const auto samples = BLWallRule::SampleSize(wall, 0, r2, sensor, 50);
    const auto len = samples.s.back();
    for (unsigned long j = 0; j < samples.s.size(); ++j) {
      const auto r = std::min(samples.s[j], len - samples.s[j]);
      const auto expected = std::min(0.05, std::max(2e-4, 0.5 * r));
      CHECK(samples.size[j] <= expected + 1e-12);
      if (j > 0)
        CHECK(fabs(samples.size[j] - samples.size[j - 1]) <= r2.gradation * (samples.s[j] - samples.s[j - 1]) + 1e-12);
    }
    CHECK(BLWallRule::SizeAt(samples, 0.5 * len) == Approx(0.05));
    CHECK(samples.size.front() == Approx(2e-4));
  }
}

TEST_CASE("Wall normal extent L sin(turn/2) of a polygonal circle", "[Adaptation]") {
  const unsigned long n = 40;
  const auto mesh = PolylineMesh(Circle(n, 2.0), true);
  const auto extent = BLWallRule::NormalExtent(mesh, "wall", 45.0);
  REQUIRE(extent.size() == n);
  const auto L = 2.0 * 2.0 * sin(M_PI / n);
  for (const auto e : extent) CHECK(e == Approx(L * sin(M_PI / n)).epsilon(1e-10));
  /*--- A sharp corner (above the corner angle) does not count. ---*/
  const auto square = PolylineMesh({{0, 0}, {1, 0}, {1, 1}, {0, 1}}, true);
  for (const auto e : BLWallRule::NormalExtent(square, "wall", 45.0)) CHECK(e == 0.0);
}

TEST_CASE("Log-Euclidean mean and bounds of metrics", "[Adaptation]") {
  const passivedouble a[3] = {1.0, 0.0, 1.0}, b[3] = {4.0, 0.0, 0.25};
  const passivedouble* metrics[2] = {a, b};
  const passivedouble half[2] = {0.5, 0.5};
  passivedouble mean[3];
  CBoundaryLayerRemesher::LogEuclideanMean(2, 2, metrics, half, mean);
  CHECK(mean[0] == Approx(2.0));
  CHECK(mean[1] == Approx(0.0).margin(1e-14));
  CHECK(mean[2] == Approx(0.5));
  const passivedouble* same[2] = {b, b};
  CBoundaryLayerRemesher::LogEuclideanMean(2, 2, same, half, mean);
  CHECK(mean[0] == Approx(4.0));
  CHECK(mean[2] == Approx(0.25));

  passivedouble m[3] = {1e10, 0.0, 1e-6};  // sizes 1e-5 and 1e3
  CBoundaryLayerRemesher::BoundMetric(2, 1e-4, 10.0, 100.0, m);
  CHECK(m[0] == Approx(1e8));               // hmin 1e-4
  CHECK(m[2] == Approx(1e8 / 1e4));          // aspect ratio 100 from the finest size
}

#ifdef HAVE_MMG
TEST_CASE("Two-pass boundary-layer remesh of a disk wall (2D)", "[Adaptation][MMG]") {
  /*--- Wall circle R 0.5 with 24 points (L = 0.13, far above the cap 0.7 sqrt(2 R h0) = 0.022 for h0 = 1e-3), far
   *    field R 3; uniform sensor metric of size 0.1. ---*/
  const passivedouble h0 = 1e-3;
  auto mesh = Annulus(24, 12, 0.5, 3.0);
  const auto nPoint = mesh.GetnPoint();
  mesh.metric.assign(nPoint * 3, 0.0);
  for (unsigned long i = 0; i < nPoint; ++i) {
    mesh.metric[3 * i] = mesh.metric[3 * i + 2] = 100.0;
  }
  const string options = "ADAP_HMIN= 1e-5\nADAP_HMAX= 1\nADAP_ARMAX= 1e4\nADAP_BL_MARKER= (wall)\n"
                         "ADAP_BL_FIRST_HEIGHT= (1e-3)\nADAP_BL_GROWTH= (1.2)\nADAP_BL_THICKNESS= (0.05)\n"
                         "ADAP_BL_METHOD= TWO_PASS\nADAP_BL_REFERENCE= bl_twopass_test.dat\n";
  CReferenceWall reference(mesh, {"wall"}, 45.0);

  for (const bool surface : {true, false}) {
    auto config = MakeConfig(options + (surface ? "ADAP_SURFACE= YES\n" : "ADAP_SURFACE= NO\n"));
    CBoundaryLayerRemesher::Report report;
    CSimplexMesh adapted;
    {
      std::unique_ptr<Mute> mute(getenv("SU2_TEST_VERBOSE") == nullptr ? new Mute : nullptr);
      adapted = CBoundaryLayerRemesher::TwoPass(*config, mesh, reference, report);
    }
    CMMGInterface::ValidateMesh(adapted, &mesh, "two-pass result");
    CHECK(report.passA);
    CHECK(report.failedGate.empty());
    const auto* wall = adapted.FindMarker("wall");
    REQUIRE(wall != nullptr);
    const auto nWall = wall->GetnElem(2);

    /*--- The far field is unchanged (bitwise). ---*/
    std::set<std::array<passivedouble, 2>> farIn, farOut;
    for (const auto p : mesh.FindMarker("farfield")->elem) farIn.insert({mesh.coord[2 * p], mesh.coord[2 * p + 1]});
    for (const auto p : adapted.FindMarker("farfield")->elem)
      farOut.insert({adapted.coord[2 * p], adapted.coord[2 * p + 1]});
    CHECK(farIn == farOut);

    /*--- Wall: refined to the curvature cap and on the reference (surface), or unchanged (fixed). ---*/
    if (surface) {
      CHECK(nWall > 4 * 24);
      CHECK(report.maxResidual <= 0.25 * h0);
      for (const auto e : BLWallRule::NormalExtent(adapted, "wall", 45.0)) CHECK(e <= h0);
    } else {
      CHECK(nWall == 24);
      CHECK(report.maxExtentRatio > 1.0);  // the coarse fixed wall cannot carry h0 (reported)
    }

    /*--- First cells: heights of the wall triangles over their wall faces. ---*/
    std::set<std::pair<unsigned long, unsigned long>> faces;
    for (unsigned long i = 0; i < nWall; ++i)
      faces.insert(std::minmax(wall->elem[2 * i], wall->elem[2 * i + 1]));
    std::vector<passivedouble> heights;
    for (unsigned long e = 0; e < adapted.GetnElem(); ++e) {
      const auto* v = &adapted.elem[3 * e];
      for (int k = 0; k < 3; ++k) {
        const auto a = v[(k + 1) % 3], b = v[(k + 2) % 3];
        if (!faces.count(std::minmax(a, b))) continue;
        const passivedouble* pa = &adapted.coord[2 * a];
        const passivedouble* pb = &adapted.coord[2 * b];
        const passivedouble* pc = &adapted.coord[2 * v[k]];
        const auto len = std::hypot(pb[0] - pa[0], pb[1] - pa[1]);
        heights.push_back(fabs((pb[0] - pa[0]) * (pc[1] - pa[1]) - (pb[1] - pa[1]) * (pc[0] - pa[0])) / len);
      }
    }
    REQUIRE(heights.size() == nWall);
    std::sort(heights.begin(), heights.end());
    const auto median = heights[heights.size() / 2];
    if (surface) {
      CHECK(median > 0.5 * h0);
      CHECK(median < 2.0 * h0);
    }
  }
}
#endif
