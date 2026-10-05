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

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <limits>
#include <memory>
#include <set>
#include <sstream>
#include <stdexcept>

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

CSimplexMesh::Marker& MarkerOf(CSimplexMesh& mesh, const string& name) {
  for (auto& marker : mesh.markers)
    if (marker.name == name) return marker;
  throw std::runtime_error("no marker " + name);
}

/*--- Mesh with its points renumbered in reverse order (elements and markers follow). ---*/
CSimplexMesh Renumbered(const CSimplexMesh& mesh) {
  auto out = mesh;
  const auto n = mesh.GetnPoint();
  for (unsigned long i = 0; i < n; ++i)
    for (int d = 0; d < 2; ++d) out.coord[2 * (n - 1 - i) + d] = mesh.coord[2 * i + d];
  for (auto& v : out.elem) v = n - 1 - v;
  for (auto& marker : out.markers)
    for (auto& v : marker.elem) v = n - 1 - v;
  return out;
}

const string blOptions = "ADAP_HMIN= 1e-5\nADAP_HMAX= 1\nADAP_ARMAX= 1e4\nADAP_BL_MARKER= (wall)\n"
                         "ADAP_BL_FIRST_HEIGHT= (1e-3)\nADAP_BL_GROWTH= (1.2)\nADAP_BL_THICKNESS= (0.05)\n"
                         "ADAP_BL_METHOD= TWO_PASS\nADAP_BL_REFERENCE= bl_twopass_test.dat\n";

string ReadFile(const string& name) {
  std::ifstream in(name);
  std::stringstream text;
  text << in.rdbuf();
  return text.str();
}

void WriteFile(const string& name, const string& text) {
  std::ofstream out(name);
  out << text;
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
      CHECK(report.attempts == 1);
      CHECK(report.nG2 + report.nG3 + report.nG4 + report.nG5 == 0);
      CHECK(report.maxArcDeviation <= 0.25 * h0);
      CHECK(report.maxSizeRatio <= 2.0);
    }
    /*--- Reports of the boundary-layer pass: the metric check ran, nothing coarser; the uniform sensor (0.1) is never
     *    finer than the boundary-layer normal size. ---*/
    CHECK(report.nMetricChecked > 0);
    CHECK(report.nMetricViolations == 0);
    CHECK(report.nSensorLoss == 0);
  }
}

TEST_CASE("Two-pass remesh: injected gate failures (retry, fallback, immediate fallback)", "[Adaptation][MMG]") {
  auto mesh = Annulus(24, 12, 0.5, 3.0);
  const auto nPoint = mesh.GetnPoint();
  mesh.metric.assign(nPoint * 3, 0.0);
  for (unsigned long i = 0; i < nPoint; ++i) mesh.metric[3 * i] = mesh.metric[3 * i + 2] = 100.0;
  CReferenceWall reference(mesh, {"wall"}, 45.0);
  auto config = MakeConfig(blOptions + "ADAP_SURFACE= YES\n");

  auto run = [&](const CBoundaryLayerRemesher::PassAHook& hook, CBoundaryLayerRemesher::Report& report) {
    std::unique_ptr<Mute> mute(getenv("SU2_TEST_VERBOSE") == nullptr ? new Mute : nullptr);
    return CBoundaryLayerRemesher::TwoPass(*config, mesh, reference, report, hook);
  };
  /*--- An interior triangle turned over (G4). ---*/
  auto invert = [](CSimplexMesh& meshA) {
    std::set<unsigned long> boundary;
    for (const auto& marker : meshA.markers) boundary.insert(marker.elem.begin(), marker.elem.end());
    for (unsigned long e = 0; e < meshA.GetnElem(); ++e) {
      auto* v = &meshA.elem[3 * e];
      if (boundary.count(v[0]) || boundary.count(v[1]) || boundary.count(v[2])) continue;
      std::swap(v[1], v[2]);
      return;
    }
  };
  CBoundaryLayerRemesher::Report report;

  SECTION("G4 once: retry accepted") {
    const auto adapted = run([&](CSimplexMesh& m, unsigned short attempt) { if (attempt == 0) invert(m); }, report);
    CHECK(report.passA);
    CHECK(report.attempts == 2);
    CMMGInterface::ValidateMesh(adapted, &mesh, "retry");
  }
  SECTION("G4 twice: fallback after the retry") {
    run([&](CSimplexMesh& m, unsigned short) { invert(m); }, report);
    CHECK_FALSE(report.passA);
    CHECK(report.attempts == 2);
    CHECK_FALSE(report.immediateFallback);
    CHECK(report.failedGate.find("G4 1") != string::npos);
  }
  SECTION("G1 point moved: immediate fallback") {
    const auto adapted = run([](CSimplexMesh& m, unsigned short) { m.coord[2 * m.FindMarker("farfield")->elem[0]] += 1e-3; },
                             report);
    CHECK_FALSE(report.passA);
    CHECK(report.attempts == 1);
    CHECK(report.immediateFallback);
    CHECK(report.failedGate.rfind("G1", 0) == 0);
    CMMGInterface::ValidateMesh(adapted, &mesh, "fallback");
  }
  SECTION("G1 edge flipped: immediate fallback") {
    run([](CSimplexMesh& m, unsigned short) {
          auto& far = MarkerOf(m, "farfield").elem;
          for (unsigned long j = 1; 2 * j + 1 < far.size(); ++j) {
            const std::set<unsigned long> first = {far[0], far[1]};
            if (first.count(far[2 * j]) || first.count(far[2 * j + 1])) continue;
            std::swap(far[1], far[2 * j]);
            return;
          }
        },
        report);
    CHECK_FALSE(report.passA);
    CHECK(report.attempts == 1);
    CHECK(report.failedGate.find("lines of marker farfield") != string::npos);
  }
}

TEST_CASE("Two-pass remesh: corner-free retry preserves the legacy pass A", "[Adaptation][MMG]") {
  /*--- Only wall points lie in the band; constant size 1e-3, sample spacing about 1.2e-4. ---*/
  const unsigned long nTheta = 128;
  auto mesh = Annulus(nTheta, 4, 0.01, 0.03);
  mesh.metric.assign(mesh.GetnPoint() * 3, 0.0);
  for (unsigned long p = 0; p < mesh.GetnPoint(); ++p) mesh.metric[3 * p] = mesh.metric[3 * p + 2] = 2.5e5;
  CReferenceWall reference(mesh, {"wall"}, 45.0);
  REQUIRE(BLWallRule::FindCorners(reference).empty());
  auto config = MakeConfig("ADAP_HMIN= 1e-5\nADAP_HMAX= 1e-3\nADAP_ARMAX= 1e4\nADAP_BL_MARKER= (wall)\n"
                           "ADAP_BL_FIRST_HEIGHT= (1e-4)\nADAP_BL_GROWTH= (1.2)\nADAP_BL_THICKNESS= (0.002)\n"
                           "ADAP_BL_METHOD= TWO_PASS\nADAP_BL_CURVATURE_FACTOR= 1\nADAP_SURFACE= YES\n");
  auto broken = mesh;
  std::swap(broken.elem[1], broken.elem[2]);
  std::vector<std::array<passivedouble, 2>> failures;
  REQUIRE(CBoundaryLayerRemesher::CountInverted(broken, &failures) == 1);
  const auto failure = reference.Project(failures[0].data(), "wall");
  REQUIRE(failure.distance < 0.002);

  /*--- Legacy retry: scale only samples within 2 t_w of the failure, without grading afterwards. ---*/
  BLWallRule::SizeRule rule;
  rule.h0 = 1e-4;
  rule.hmin = 1e-5;
  rule.hmax = 1e-3;
  rule.curvatureFactor = 1.0;
  auto samples = BLWallRule::SampleSize(reference, 0, rule,
                                       [](const passivedouble*, const passivedouble*) { return 2e-3; });
  const auto base = samples;
  for (unsigned long j = 0; j < samples.s.size(); ++j)
    if (fabs(samples.s[j] - failure.s) <= 2.0 * BLWallRule::SizeAt(base, samples.s[j]))
      samples.size[j] = std::max(samples.tmin, samples.size[j] * 0.7);
  auto graded = samples;
  BLWallRule::Grade(graded, rule.gradation, true);
  REQUIRE(graded.size != samples.size);  // this case detects the extra grading
  auto legacy = mesh;
  for (unsigned long p = 0; p < nTheta; ++p) {
    const auto proj = reference.Project(&mesh.coord[2 * p], "wall");
    const auto h = BLWallRule::SizeAt(samples, proj.s);
    legacy.metric[3 * p] = legacy.metric[3 * p + 2] = 1.0 / (h * h);
  }
  std::unique_ptr<Mute> mute(getenv("SU2_TEST_VERBOSE") == nullptr ? new Mute : nullptr);
  CMMGInterface mmg(*config);
  auto& params = mmg.GetParameters();
  params.boundaryLayer = false;
  params.swap = 1;
  params.localWallHmax = false;
  params.requiredMarkers = {"farfield"};
  CMMGInterface::LocalParameter local;
  local.ref = mesh.FindMarker("wall")->ref;
  local.hmin = 1e-4;
  local.hmax = 1.5 * *std::max_element(samples.size.begin(), samples.size.end());
  local.hausd = 0.5 * rule.h0;
  mmg.SetLocalParameters({local});
  const auto expected = mmg.Adapt(legacy);
  CBoundaryLayerRemesher::Report report;
  bool compared = false;
  CBoundaryLayerRemesher::TwoPass(*config, mesh, reference, report, [&](CSimplexMesh& m, unsigned short attempt) {
    if (attempt == 0) {
      m = broken;  // G4 forces a local retry
    } else {
      REQUIRE(m.coord.size() == expected.coord.size());
      REQUIRE(m.metric.size() == expected.metric.size());
      CHECK(std::memcmp(m.coord.data(), expected.coord.data(), m.coord.size() * sizeof(passivedouble)) == 0);
      CHECK(std::memcmp(m.metric.data(), expected.metric.data(), m.metric.size() * sizeof(passivedouble)) == 0);
      CHECK(m.elem == expected.elem);
      compared = true;
      m = mesh;  // valid wall and cells, so the gate accepts the retry
    }
  });
  CHECK(compared);
  CHECK(report.passA);
  CHECK(report.attempts == 2);
  CHECK(report.nCorner == 0);
}
#endif

TEST_CASE("Robust orientation predicate (gate G4)", "[Adaptation]") {
  const passivedouble a[2] = {0.0, 0.0}, b[2] = {1.0, 1.0}, c[2] = {2.0, 2.0}, d[2] = {0.0, 1.0};
  CHECK(BLWallRule::Orientation(a, b, c) == 0);
  CHECK(BLWallRule::Orientation(a, b, d) == 1);
  CHECK(BLWallRule::Orientation(a, d, b) == -1);
  /*--- (1 + e)(1 - e + e^2) - 1 = e^3 with e = 2^-20: rounded products cancel to 0 (without fused multiply-add), the
   *    exact sign is +. ---*/
  const passivedouble e = std::ldexp(1.0, -20);
  for (const passivedouble shift : {0.0, 1.0, -3.0}) {
    const passivedouble p[2] = {shift, shift}, q[2] = {shift + 1.0 + e, shift + 1.0},
                        r[2] = {shift + 1.0, shift + 1.0 - e + e * e};
    CHECK(BLWallRule::Orientation(p, q, r) == 1);
    CHECK(BLWallRule::Orientation(p, r, q) == -1);
  }
}

TEST_CASE("Reference wall: guaranteed arc-to-chord bound (gate G3)", "[Adaptation]") {
  /*--- Dense sampling of the distance from the arc between s0 and s1 to the finite chord [a, b]. ---*/
  auto sampled = [](const CReferenceWall& wall, const CReferenceWall::Segment& seg, passivedouble s0, passivedouble s1,
                    const passivedouble* a, const passivedouble* b) {
    passivedouble worst = 0.0;
    for (int k = 0; k <= 20000; ++k) {
      passivedouble x[2];
      wall.Evaluate(seg, s0 + (s1 - s0) * k / 20000.0, x);
      const passivedouble u[2] = {b[0] - a[0], b[1] - a[1]};
      auto t = ((x[0] - a[0]) * u[0] + (x[1] - a[1]) * u[1]) / (u[0] * u[0] + u[1] * u[1]);
      t = std::max(0.0, std::min(1.0, t));
      worst = std::max(worst, std::hypot(a[0] + t * u[0] - x[0], a[1] + t * u[1] - x[1]));
    }
    return worst;
  };

  /*--- Circle: chords across several spline pieces, also across the period; the bound is the sagitta. ---*/
  CReferenceWall circle(PolylineMesh(Circle(64, 1.0), true), {"wall"}, 45.0);
  const auto& seg = circle.GetSegments()[0];
  const auto length = seg.s.back();
  for (const auto span : std::vector<std::array<passivedouble, 2>>{{0.3, 0.9}, {length - 0.2, length + 0.25}}) {
    passivedouble a[2], b[2];
    circle.Evaluate(seg, span[0], a);
    circle.Evaluate(seg, span[1], b);
    const auto bound = circle.ArcChordDistance(seg, span[0], span[1], a, b);
    const auto exact = sampled(circle, seg, span[0], span[1], a, b);
    CHECK(bound >= exact);
    CHECK(bound == Approx(exact).epsilon(1e-6));
    CHECK(bound == Approx(1.0 - cos(0.5 * (span[1] - span[0]))).epsilon(1e-3));  // sagitta of the arc
  }

  /*--- S-shaped reference (inflection inside the line): a = start, b = end of one wave; the line midpoint lies on the
   *    reference, the arc is 1e-3 away from the line in two places. ---*/
  std::vector<std::array<passivedouble, 2>> wave;
  for (int k = 0; k <= 100; ++k) wave.push_back({k / 100.0, 1e-3 * sin(4.0 * M_PI * k / 100.0)});
  CReferenceWall sWall(PolylineMesh(wave, false), {"wall"}, 45.0);
  const auto& sSeg = sWall.GetSegments()[0];
  const passivedouble a[2] = {0.0, 0.0}, b[2] = {0.5, 0.0}, mid[2] = {0.25, 0.0};
  CHECK(sWall.Project(mid, "wall").distance < 1e-6);
  const auto sEnd = sWall.Project(b, "wall").s;
  const auto bound = sWall.ArcChordDistance(sSeg, 0.0, sEnd, a, b);
  CHECK(bound >= sampled(sWall, sSeg, 0.0, sEnd, a, b));
  CHECK(bound == Approx(1e-3).epsilon(0.01));

  /*--- The arc beyond the chord ends counts (distance to the finite chord, not to its line). ---*/
  CReferenceWall line(PolylineMesh({{0, 0}, {0.5, 0}, {1, 0}}, false), {"wall"}, 45.0);
  const passivedouble c[2] = {0.1, 0.0}, d[2] = {0.9, 0.0};
  CHECK(line.ArcChordDistance(line.GetSegments()[0], 0.0, 1.0, c, d) == Approx(0.1).epsilon(1e-9));
  /*--- Translated far from the origin: no floor from the absolute coordinates (geometric deviation 0 or the sagitta). ---*/
  for (const passivedouble shift : {1e3, 1e6}) {
    INFO(shift);
    CReferenceWall far(PolylineMesh({{shift, 0}, {shift + 0.5, 0}, {shift + 1, 0}}, false), {"wall"}, 45.0);
    const passivedouble e[2] = {shift + 0.2, 0.0}, f[2] = {shift + 0.7, 0.0};
    CHECK(far.ArcChordDistance(far.GetSegments()[0], 0.2, 0.7, e, f) < 1e-7);  // margin 64 eps x 1e6 = 1.4e-8
    auto ring = Circle(64, 1.0);
    for (auto& x : ring) x[0] += shift;
    CReferenceWall shifted(PolylineMesh(ring, true), {"wall"}, 45.0);
    const auto& rs = shifted.GetSegments()[0];
    passivedouble g[2], h[2];
    shifted.Evaluate(rs, 0.3, g);
    shifted.Evaluate(rs, 0.9, h);
    CHECK(shifted.ArcChordDistance(rs, 0.3, 0.9, g, h) == Approx(1.0 - cos(0.3)).epsilon(1e-3));
  }
  /*--- Degenerate chord. ---*/
  CHECK(std::isinf(line.ArcChordDistance(line.GetSegments()[0], 0.0, 1.0, c, c)));
}

TEST_CASE("Reference wall: creation and restart rules", "[Adaptation]") {
  const auto mesh = Annulus(24, 4, 0.5, 3.0);
  const string file = "bl_twopass_test.dat";
  auto config = MakeConfig(blOptions);
  std::string info;

  /*--- Fresh run: fitted and written; a restart reads it back. ---*/
  CReferenceWall created;
  REQUIRE(CBoundaryLayerRemesher::PrepareReference(*config, mesh, true, created, info).empty());
  const auto valid = ReadFile(file);
  CReferenceWall read;
  CHECK(CBoundaryLayerRemesher::PrepareReference(*config, mesh, false, read, info).empty());
  CHECK(read.Fingerprint() == created.Fingerprint());

  /*--- Creation only on a fresh run or with REBASE. ---*/
  CHECK(CBoundaryLayerRemesher::CreatesReference(*config));
  CHECK_FALSE(CBoundaryLayerRemesher::CreatesReference(*MakeConfig(blOptions + "RESTART_SOL= YES\n")));
  CHECK(CBoundaryLayerRemesher::CreatesReference(
      *MakeConfig(blOptions + "RESTART_SOL= YES\nADAP_BL_REFERENCE_REBASE= YES\n")));

  /*--- Missing, truncated, foreign (fingerprint), moved wall: an error without REBASE, a new reference with it. ---*/
  auto moved = mesh;
  for (const auto p : mesh.FindMarker("wall")->elem)
    for (int dd = 0; dd < 2; ++dd) moved.coord[2 * p + dd] = 1.01 * mesh.coord[2 * p + dd];
  std::string edited = valid;
  const auto knot = edited.find('\n', edited.find('\n', edited.find("\nSEGMENT") + 1) + 1) + 1;
  edited.insert(knot, "1");  // second knot changed, fingerprint kept
  struct Case {
    const char* name;
    string content;  // empty: no file
    const CSimplexMesh* mesh;
    const char* error;
  };
  const Case cases[] = {{"missing", "", &mesh, "does not exist"},
                        {"truncated", valid.substr(0, valid.size() / 2), &mesh, "truncated"},
                        {"fingerprint", edited, &mesh, "fingerprint"},
                        {"moved wall", valid, &moved, "does not match"}};
  for (const auto& test : cases) {
    INFO(test.name);
    std::remove(file.c_str());
    if (!test.content.empty()) WriteFile(file, test.content);
    CReferenceWall ref;
    const auto error = CBoundaryLayerRemesher::PrepareReference(*config, *test.mesh, false, ref, info);
    INFO(error);
    CHECK(error.find(test.error) != string::npos);
    CHECK(error.find("ADAP_BL_REFERENCE_REBASE") != string::npos);
    CHECK(ref.GetSegments().empty());
    /*--- With REBASE: fitted to the current wall, written, and then readable. ---*/
    CHECK(CBoundaryLayerRemesher::PrepareReference(*config, *test.mesh, true, ref, info).empty());
    CReferenceWall again;
    CHECK(CBoundaryLayerRemesher::PrepareReference(*config, *test.mesh, false, again, info).empty());
  }
  /*--- Feature flags edited with the fingerprint kept: closed, sharp start/end, marker name. ---*/
  {
    CReferenceWall corner(PolylineMesh({{0, 0}, {1, 0}, {1, 1}, {0.5, 1.2}, {0, 1}}, true), {"wall"}, 45.0);
    corner.Write(file);
    const auto text = ReadFile(file);
    const auto line = text.find("\nSEGMENT wall 0 1 1") + 1;
    REQUIRE(line > 0);
    for (const char* flags : {"SEGMENT wall 0 0 0", "SEGMENT wall 1 1 1", "SEGMENT wall 0 1 0", "SEGMENT ward 0 1 1"}) {
      INFO(flags);
      std::string changed = text;
      changed.replace(line, 18, flags);
      WriteFile(file, changed);
      CReferenceWall ref;
      std::string why;
      CHECK_FALSE(ref.Read(file, &why));
      CHECK(ref.GetSegments().empty());
    }
  }
  /*--- A reference made with another corner angle is foreign. ---*/
  WriteFile(file, valid);
  CReferenceWall other;
  CHECK(CBoundaryLayerRemesher::PrepareReference(*MakeConfig(blOptions + "ADAP_ANGLE= 30\n"), mesh, false, other, info)
            .find("corner angle") != string::npos);
  std::remove(file.c_str());
}

TEST_CASE("Gate G1: other boundaries, required corners and wall chains", "[Adaptation]") {
  const auto mesh = Annulus(24, 4, 0.5, 3.0);
  const auto tol = 1e-9 * 6.0;
  std::set<unsigned long> breaks;
  const auto wall0 = mesh.FindMarker("wall")->elem[0];

  auto same = mesh;
  CHECK(CBoundaryLayerRemesher::CheckOtherBoundaries(mesh, same, {"wall"}, {wall0}, tol, breaks).empty());
  CHECK(breaks == std::set<unsigned long>{wall0});

  /*--- Renumbered points: still a bijection with the same edges; tiny offsets are restored exactly. ---*/
  auto renumbered = Renumbered(mesh);
  const auto far0 = renumbered.FindMarker("farfield")->elem[0];
  renumbered.coord[2 * far0] += 0.5 * tol;
  CHECK(CBoundaryLayerRemesher::CheckOtherBoundaries(mesh, renumbered, {"wall"}, {wall0}, tol, breaks).empty());
  CHECK(renumbered.coord[2 * far0] == mesh.coord[2 * (mesh.GetnPoint() - 1 - far0)]);
  CHECK(breaks == std::set<unsigned long>{mesh.GetnPoint() - 1 - wall0});

  /*--- Boundary points spaced far below the tolerance (finely graded boundaries): exact or near-exact matches are
   *    unique, also for a required corner. ---*/
  {
    CSimplexMesh fine;
    fine.nDim = 2;
    fine.coord = {0, 0, 1e-12, 0, 1, 0, 1, 1, 0, 1, 2, 0, 2, 1e-12, 2, 1, 3, 1};
    CSimplexMesh::Marker wall, farfield;
    wall.name = "wall";
    wall.ref = 1;
    wall.elem = {0, 1, 1, 2, 2, 3, 3, 4, 4, 0};
    farfield.name = "farfield";
    farfield.ref = 2;
    farfield.elem = {5, 6, 6, 7, 7, 8};
    fine.markers = {wall, farfield};
    for (const passivedouble offset : {0.0, 1e-14}) {
      INFO(offset);
      auto out = Renumbered(fine);
      for (auto& x : out.coord) x += offset;
      CHECK(CBoundaryLayerRemesher::CheckOtherBoundaries(fine, out, {"wall"}, {1}, 1e-9, breaks).empty());
      CHECK(breaks == std::set<unsigned long>{fine.GetnPoint() - 2});
    }
  }

  /*--- A far-field point moved. ---*/
  auto movedPoint = mesh;
  movedPoint.coord[2 * mesh.FindMarker("farfield")->elem[0]] += 1e-3;
  CHECK(CBoundaryLayerRemesher::CheckOtherBoundaries(mesh, movedPoint, {"wall"}, {}, tol, breaks).find("moved") !=
        string::npos);

  /*--- Far-field lines flipped: the same points and number of lines, other edges. ---*/
  auto flipped = mesh;
  auto& far = MarkerOf(flipped, "farfield").elem;
  std::swap(far[1], far[4]);  // (a, b), (c, d) -> (a, c), (b, d)
  CHECK(CBoundaryLayerRemesher::CheckOtherBoundaries(mesh, flipped, {"wall"}, {}, tol, breaks).find("lines") !=
        string::npos);

  /*--- A required corner lost. ---*/
  auto lost = mesh;
  lost.coord[2 * wall0 + 1] += 1e-3;
  CHECK(CBoundaryLayerRemesher::CheckOtherBoundaries(mesh, lost, {"wall"}, {wall0}, tol, breaks).find("required") !=
        string::npos);

  /*--- The wall split into two chains (a line removed). ---*/
  auto split = mesh;
  auto& wallLines = MarkerOf(split, "wall").elem;
  wallLines.erase(wallLines.begin(), wallLines.begin() + 2);
  CHECK(CBoundaryLayerRemesher::CheckOtherBoundaries(mesh, split, {"wall"}, {}, tol, breaks).find("chain") !=
        string::npos);
}

TEST_CASE("Gates G3 and G5 on wall pieces", "[Adaptation]") {
  CReferenceWall reference(PolylineMesh(Circle(64, 1.0), true), {"wall"}, 45.0);
  const unsigned long n = 256;
  const auto L = 2.0 * M_PI / n;
  auto check = [&](const CSimplexMesh& wall, passivedouble tw, CBoundaryLayerRemesher::Report& report) {
    auto pieces = CBoundaryLayerRemesher::WallPieces(wall, "wall", {});
    CBoundaryLayerRemesher::AssignSegments(wall, reference, "wall", pieces);
    report = CBoundaryLayerRemesher::Report();
    std::vector<std::array<passivedouble, 2>> failures;
    CBoundaryLayerRemesher::CheckPieces(wall, reference, "wall", pieces, 1e-4,
                                        [tw](long, passivedouble) { return tw; }, report, failures);
    return pieces;
  };
  CBoundaryLayerRemesher::Report report;

  /*--- Points on the circle: one closed piece on segment 0, G3 and G5 hold. ---*/
  const auto good = PolylineMesh(Circle(n, 1.0, 0.01), true);
  const auto pieces = check(good, L, report);
  REQUIRE(pieces.size() == 1);
  CHECK(pieces[0].closed);
  CHECK(pieces[0].segment == 0);
  CHECK(report.nG3 == 0);
  CHECK(report.nG5 == 0);
  CHECK(report.maxArcDeviation == Approx(1.0 - cos(0.5 * L)).epsilon(0.01));

  /*--- A point off the reference; two points in the wrong order. ---*/
  auto off = good;
  off.coord[0] *= 1.0 + 2e-4;
  check(off, L, report);
  CHECK(report.nG3 >= 1);
  auto swapped = good;
  for (int d = 0; d < 2; ++d) std::swap(swapped.coord[2 * 5 + d], swapped.coord[2 * 6 + d]);
  check(swapped, L, report);
  CHECK(report.nG3 == 3);  // backwards, and the two lines over two intervals deviate by 3e-4

  /*--- G5: lines of 3 t_w fail; lines of t_w / 3 are reported only. ---*/
  check(good, L / 3.0, report);
  CHECK(report.nG5 == n);
  CHECK(report.maxSizeRatio == Approx(3.0).epsilon(1e-3));
  check(good, 3.0 * L, report);
  CHECK(report.nG5 == 0);
  CHECK(report.nUndersized == n);

  /*--- Open piece of a straight wall: lines within one t_w of its ends are exempt from G5. ---*/
  std::vector<std::array<passivedouble, 2>> straight;
  for (int k = 0; k <= 10; ++k) straight.push_back({0.1 * k, 0.0});
  CReferenceWall flat(PolylineMesh(straight, false), {"wall"}, 45.0);
  const auto flatMesh = PolylineMesh(straight, false);
  auto flatPieces = CBoundaryLayerRemesher::WallPieces(flatMesh, "wall", {});
  CBoundaryLayerRemesher::AssignSegments(flatMesh, flat, "wall", flatPieces);
  REQUIRE(flatPieces.size() == 1);
  CHECK_FALSE(flatPieces[0].closed);
  report = CBoundaryLayerRemesher::Report();
  std::vector<std::array<passivedouble, 2>> failures;
  CBoundaryLayerRemesher::CheckPieces(flatMesh, flat, "wall", flatPieces, 1e-6,
                                      [](long, passivedouble) { return 0.04; }, report, failures);
  CHECK(report.nG3 == 0);
  CHECK(report.nG5 == 8);  // 0.1 / 0.04 = 2.5 > 2; the two lines that touch the ends are exempt

  /*--- Far from the origin (x ~ 1e8): short lines of a straight wall pass G3 and G5 (no origin-dependent guards). ---*/
  {
    std::vector<std::array<passivedouble, 2>> farWall;
    for (int k = 0; k <= 20; ++k) farWall.push_back({1e8 + 5e-5 * k, 0.0});
    const auto farMesh = PolylineMesh(farWall, false);
    CReferenceWall farRef(PolylineMesh({farWall.front(), farWall[10], farWall.back()}, false), {"wall"}, 45.0);
    auto farPieces = CBoundaryLayerRemesher::WallPieces(farMesh, "wall", {});
    CBoundaryLayerRemesher::AssignSegments(farMesh, farRef, "wall", farPieces);
    report = CBoundaryLayerRemesher::Report();
    CBoundaryLayerRemesher::CheckPieces(farMesh, farRef, "wall", farPieces, 2.5e-6,
                                        [](long, passivedouble) { return 5e-5; }, report, failures);
    CHECK(report.nG3 == 0);
    CHECK(report.nG5 == 0);
  }

  /*--- Coverage: a thin lens (two segments between two sharp tips); both sides of the wall projected onto the upper
   *    segment pass every per-line check, but the upper segment is covered twice and the lower one not at all. ---*/
  {
    std::vector<std::array<passivedouble, 2>> lens;
    for (int k = 0; k <= 20; ++k) {
      const passivedouble x = -1.0 + 0.1 * k;
      lens.push_back({x, 0.2 * (1.0 - x * x)});
    }
    for (int k = 19; k >= 1; --k) {
      const passivedouble x = -1.0 + 0.1 * k;
      lens.push_back({x, -0.2 * (1.0 - x * x)});
    }
    CReferenceWall lensWall(PolylineMesh(lens, true), {"wall"}, 45.0);
    REQUIRE(lensWall.GetSegments().size() == 2);
    auto collapsed = lens;
    for (auto& x : collapsed) x[1] = std::fabs(x[1]);
    for (const bool collapse : {false, true}) {
      INFO(collapse);
      const auto lensMesh = PolylineMesh(collapse ? collapsed : lens, true);
      auto lensPieces = CBoundaryLayerRemesher::WallPieces(lensMesh, "wall", {0, 20});
      CBoundaryLayerRemesher::AssignSegments(lensMesh, lensWall, "wall", lensPieces);
      REQUIRE(lensPieces.size() == 2);
      report = CBoundaryLayerRemesher::Report();
      CBoundaryLayerRemesher::CheckPieces(lensMesh, lensWall, "wall", lensPieces, 1e-2, nullptr, report, failures);
      if (collapse) {
        CHECK(lensPieces[0].segment == lensPieces[1].segment);
        CHECK(report.nG3 == 2);  // one segment twice, the other not at all
      } else {
        CHECK(lensPieces[0].segment != lensPieces[1].segment);
        CHECK(report.nG3 == 0);
      }
    }
  }

  /*--- An S-shaped reference: the line over one wave has its end points and midpoint on the reference, G3 fails. ---*/
  std::vector<std::array<passivedouble, 2>> wave;
  for (int k = 0; k <= 100; ++k) wave.push_back({k / 100.0, 1e-3 * sin(4.0 * M_PI * k / 100.0)});
  CReferenceWall sWall(PolylineMesh(wave, false), {"wall"}, 45.0);
  const auto coarse = PolylineMesh({{0.0, 0.0}, {0.5, 0.0}, {1.0, 0.0}}, false);
  auto sPieces = CBoundaryLayerRemesher::WallPieces(coarse, "wall", {});
  CBoundaryLayerRemesher::AssignSegments(coarse, sWall, "wall", sPieces);
  report = CBoundaryLayerRemesher::Report();
  CBoundaryLayerRemesher::CheckPieces(coarse, sWall, "wall", sPieces, 1e-4, nullptr, report, failures);
  CHECK(report.maxResidual < 1e-6);
  CHECK(report.nG3 == 2);
}

TEST_CASE("Pass-B donor of a point outside the input mesh: closest face of its own marker", "[Adaptation]") {
  CSimplexMesh square;
  square.nDim = 2;
  square.coord = {0, 0, 1, 0, 1, 1, 0, 1};
  square.elem = {0, 1, 2, 0, 2, 3};
  square.elemRef = {0, 0};
  const char* names[4] = {"wall", "right", "top", "left"};
  for (unsigned long k = 0; k < 4; ++k) {
    CSimplexMesh::Marker marker;
    marker.name = names[k];
    marker.ref = k + 1;
    marker.elem = {k, (k + 1) % 4};
    square.markers.push_back(marker);
  }
  CBarycentricLocator locator(square);
  /*--- (-0.01, 0.001): the closest face of any marker is "left", the closest face of "wall" its own. ---*/
  const su2double x[3] = {-0.01, 0.001, 0.0};
  const auto any = CBoundaryLayerRemesher::DonorStencil(locator, x, {});
  CHECK_FALSE(any.inside);
  bool usesLeft = false;
  for (unsigned short k = 0; k < any.nPoint; ++k) usesLeft = usesLeft || (any.point[k] == 3 && any.weight[k] > 0.0);
  CHECK(usesLeft);
  const auto own = CBoundaryLayerRemesher::DonorStencil(locator, x, {"wall"});
  for (unsigned short k = 0; k < own.nPoint; ++k) CHECK((own.point[k] == 0 || own.point[k] == 1));
  /*--- A point inside keeps its element. ---*/
  const su2double y[3] = {0.6, 0.2, 0.0};
  CHECK(CBoundaryLayerRemesher::DonorStencil(locator, y, {"wall"}).inside);
}

namespace {

/*--- Symmetric biconvex lens of chord 1 (tips at x = -0.5 and 0.5) with the wedge angle omega at both tips: points
 *    counterclockwise from the trailing edge (0.5, 0) over the upper arc, the leading edge, and back over the lower arc
 *    (cosine spacing, n intervals per side). ---*/
std::vector<std::array<passivedouble, 2>> Lens(passivedouble omega, unsigned long n) {
  const auto phi = 0.5 * omega, R = 0.5 / sin(phi), yc = R * cos(phi);
  std::vector<std::array<passivedouble, 2>> points;
  for (unsigned long k = 0; k < 2 * n; ++k) {
    const bool upper = k < n;
    const auto x = 0.5 * cos(M_PI * (upper ? k : k - n) / n) * (upper ? 1.0 : -1.0);
    const auto y = sqrt(R * R - x * x) - yc;
    points.push_back({x, upper ? y : -y});
  }
  return points;
}

/*--- O-mesh around a convex wall polygon containing the origin: rays to a far-field circle of radius rFar, nR
 *    layers graded by 1.3; quads split by the diagonal that gives two positive triangles. Wall markers: points
 *    [0, split] -> "wall", [split, end] -> wallB (if split > 0), else one marker "wall". ---*/
CSimplexMesh OMesh(const std::vector<std::array<passivedouble, 2>>& wallPoints, unsigned long nR, passivedouble rFar,
                   unsigned long split = 0, const string& wallB = "wall2") {
  CSimplexMesh mesh;
  mesh.nDim = 2;
  const auto n = wallPoints.size();
  for (unsigned long j = 0; j <= nR; ++j) {
    const auto f = (pow(1.3, j) - 1.0) / (pow(1.3, nR) - 1.0);
    for (const auto& p : wallPoints) {
      const auto r = std::hypot(p[0], p[1]);
      mesh.coord.push_back(p[0] + f * (rFar * p[0] / r - p[0]));
      mesh.coord.push_back(p[1] + f * (rFar * p[1] / r - p[1]));
    }
  }
  auto id = [&](unsigned long i, unsigned long j) { return (i % n) + j * n; };
  auto positive = [&](unsigned long a, unsigned long b, unsigned long c) {
    return BLWallRule::Orientation(&mesh.coord[2 * a], &mesh.coord[2 * b], &mesh.coord[2 * c]) > 0;
  };
  for (unsigned long j = 0; j < nR; ++j)
    for (unsigned long i = 0; i < n; ++i) {
      const auto a = id(i, j), b = id(i + 1, j), c = id(i + 1, j + 1), d = id(i, j + 1);
      /*--- i runs counterclockwise along the wall and j outwards: (a, c, b) and (a, d, c) are counterclockwise. ---*/
      if (positive(a, c, b) && positive(a, d, c)) {
        mesh.elem.insert(mesh.elem.end(), {a, c, b, a, d, c});
      } else {
        mesh.elem.insert(mesh.elem.end(), {a, d, b, b, d, c});
      }
    }
  mesh.elemRef.assign(mesh.GetnElem(), 0);
  CSimplexMesh::Marker far, wall, other;
  far.name = "farfield";
  far.ref = 1;
  wall.name = "wall";
  wall.ref = 2;
  other.name = wallB;
  other.ref = 3;
  for (unsigned long i = 0; i < n; ++i) {
    far.elem.insert(far.elem.end(), {id(i, nR), id(i + 1, nR)});
    auto& target = (split > 0 && i >= split) ? other : wall;
    target.elem.insert(target.elem.end(), {id(i + 1, 0), id(i, 0)});
  }
  mesh.markers = {far, wall};
  if (split > 0) mesh.markers.push_back(other);
  return mesh;
}

std::unique_ptr<CConfig> LensConfig(bool split, const string& extra = "") {
  const bool quiet = getenv("SU2_TEST_VERBOSE") == nullptr;
  std::unique_ptr<Mute> mute(quiet ? new Mute : nullptr);
  const string walls = split ? "wall, wall2" : "wall";
  stringstream ss("SOLVER= NAVIER_STOKES\nREYNOLDS_NUMBER= 1e6\nMACH_NUMBER= 0.5\nMESH_FORMAT= SU2\n"
                  "MESH_FILENAME= unused.su2\nMGLEVEL= 0\nCOMPUTE_METRIC= YES\nADAP_SENSOR= (MACH)\n"
                  "MARKER_HEATFLUX= (" + string(split ? "wall, 0.0, wall2, 0.0" : "wall, 0.0") + ")\n"
                  "MARKER_FAR= (farfield)\nADAP_HMIN= 1e-5\nADAP_HMAX= 1\nADAP_ARMAX= 1e4\nADAP_BL_MARKER= (" + walls +
                  ")\nADAP_BL_FIRST_HEIGHT= (1e-4)\nADAP_BL_GROWTH= (1.2)\nADAP_BL_THICKNESS= (0.02)\n"
                  "ADAP_BL_METHOD= TWO_PASS\nADAP_BL_REFERENCE= bl_twopass_test.dat\nADAP_SURFACE= YES\n" + extra);
  return std::unique_ptr<CConfig>(new CConfig(ss, SU2_COMPONENT::SU2_CFD, false));
}

/*--- Interior angle of the lens polygon at a tip (the wedge of the discrete wall). ---*/
passivedouble TipWedge(const std::vector<std::array<passivedouble, 2>>& p, unsigned long i) {
  const auto n = p.size();
  const auto& a = p[(i + n - 1) % n];
  const auto& b = p[(i + 1) % n];
  const passivedouble u[2] = {a[0] - p[i][0], a[1] - p[i][1]}, w[2] = {b[0] - p[i][0], b[1] - p[i][1]};
  return atan2(fabs(u[0] * w[1] - u[1] * w[0]), u[0] * w[0] + u[1] * w[1]);
}

}  // namespace

TEST_CASE("Corner rule: corners of the reference, floor, symmetry (SERIAL_BL_FIX_PLAN 11.7)", "[Adaptation]") {
  const auto omega = 16.0 * M_PI / 180.0;
  const auto lens = Lens(omega, 40);

  /*--- Corners: the two tips of a lens; the four corners of a square; one per tip of a lens split into two markers at
   *    its tips; none on an open polyline with free ends; one (a segment with itself) on a teardrop. ---*/
  {
    CReferenceWall wall(PolylineMesh(lens, true), {"wall"}, 45.0);
    REQUIRE(wall.GetSegments().size() == 2);
    CHECK(BLWallRule::FindCorners(wall).size() == 2);
    const std::vector<std::array<passivedouble, 2>> square = {{0, 0}, {1, 0}, {1, 1}, {0, 1}};
    CHECK(BLWallRule::FindCorners(CReferenceWall(PolylineMesh(square, true), {"wall"}, 45.0)).size() == 4);
    CHECK(BLWallRule::FindCorners(CReferenceWall(PolylineMesh({{0, 0}, {1, 0.1}, {2, 0}}, false), {"wall"}, 45.0))
              .empty());
    /*--- Teardrop: the tip (1, 0) and the arc of the circle r = 0.3 between the two tangent points. ---*/
    std::vector<std::array<passivedouble, 2>> teardrop = {{1.0, 0.0}};
    const auto tangent = acos(0.3);
    for (int k = 0; k < 40; ++k) {
      const auto a = tangent + (2.0 * M_PI - 2.0 * tangent) * k / 39.0;
      teardrop.push_back({0.3 * cos(a), 0.3 * sin(a)});
    }
    const auto drop = BLWallRule::FindCorners(CReferenceWall(PolylineMesh(teardrop, true), {"wall"}, 45.0));
    REQUIRE(drop.size() == 1);
    CHECK(drop[0].seg[0] == drop[0].seg[1]);
    CHECK(drop[0].atEnd[0] != drop[0].atEnd[1]);
    CHECK(drop[0].x[0] == 1.0);
    const auto mesh = OMesh(lens, 4, 5.0, 40);
    CHECK(BLWallRule::FindCorners(CReferenceWall(mesh, {"wall", "wall2"}, 45.0)).size() == 2);
    CHECK(BLWallRule::FindCorners(CReferenceWall(mesh, {"wall"}, 45.0)).empty());  // its ends meet a non-BL marker
  }

  /*--- Effective floor: clamps by hmax and by the segment length (support within half of it); none at or below
   *    t_min. ---*/
  CHECK(BLWallRule::CornerFloor(1e-3, 10.0, 2.0, 2e-4, 0.15) == 1e-3);
  CHECK(BLWallRule::CornerFloor(1e-3, 5e-4, 2.0, 2e-4, 0.15) == 5e-4);
  const auto clamped = BLWallRule::CornerFloor(1.0, 10.0, 0.01, 1e-4, 0.15);
  CHECK(clamped == Approx((0.15 * 0.005 + 1e-4) / 1.15));
  CHECK(clamped + (clamped - 1e-4) / 0.15 == Approx(0.005));
  CHECK(BLWallRule::CornerFloor(1.0, 10.0, 1e-4, 1e-4, 0.15) == 0.0);  // (0.15 5e-5 + 1e-4) / 1.15 < t_min
  CHECK(BLWallRule::CornerFloor(0.0, 10.0, 2.0, 1e-4, 0.15) == 0.0);

  /*--- Floor and symmetry on the lens: an asymmetric sensor (finer on the upper side), h0 1e-5 (t_min 2e-5). ---*/
  CReferenceWall wall(PolylineMesh(lens, true), {"wall"}, 45.0);
  BLWallRule::SizeRule rule;
  rule.h0 = 1e-5;
  rule.hmin = 1e-6;
  rule.hmax = 1.0;
  auto sensor = [](const passivedouble* x, const passivedouble*) { return x[1] > 0.0 ? 2e-3 : 8e-3; };
  std::vector<BLWallRule::SizeSamples> base;
  for (unsigned long iSeg = 0; iSeg < 2; ++iSeg) base.push_back(BLWallRule::SampleSize(wall, iSeg, rule, sensor, 20));
  auto corners = BLWallRule::FindCorners(wall);
  for (auto& c : corners) {
    c.convex = true;
    c.h0 = rule.h0;
    c.tmin = 2e-5;
    c.requested = 2.0 * c.h0 / sin(0.5 * omega);
    c.floor = BLWallRule::CornerFloor(c.requested, 1.0, 1.0, c.tmin, rule.gradation);
    REQUIRE(c.floor == Approx(c.requested));
  }
  auto samples = base;
  const auto changes = BLWallRule::ApplyCorners(wall, corners, rule.gradation, samples);
  CHECK(changes.nFloorRaised > 0);
  CHECK(changes.nSymmetry > 0);
  for (const auto& c : corners) {
    const auto reach = BLWallRule::CornerReach(c, rule.gradation);
    for (unsigned short k = 0; k < 2; ++k) {
      const auto& smp = samples[c.seg[k]];
      const auto& other = samples[c.seg[1 - k]];
      for (unsigned long j = 0; j < smp.s.size(); ++j) {
        const auto r = c.atEnd[k] ? smp.s.back() - smp.s[j] : smp.s[j] - smp.s.front();
        /*--- At least the floor profile; equal on both sides near the corner. ---*/
        CHECK(smp.size[j] >= std::max(c.tmin, c.floor - rule.gradation * std::max(0.0, r - c.floor)) * (1 - 1e-12));
        if (r <= reach) {
          const auto s = c.atEnd[1 - k] ? other.s.back() - r : other.s.front() + r;
          CHECK(smp.size[j] == Approx(BLWallRule::SizeAt(other, s)).epsilon(1e-6));
        }
      }
    }
  }
  /*--- Graded; away from the corners (beyond the reach of the floor and of the symmetry) unchanged. ---*/
  for (unsigned long iSeg = 0; iSeg < 2; ++iSeg) {
    const auto& smp = samples[iSeg];
    for (unsigned long j = 1; j < smp.s.size(); ++j)
      CHECK(fabs(smp.size[j] - smp.size[j - 1]) <= rule.gradation * (smp.s[j] - smp.s[j - 1]) * (1 + 1e-9) + 1e-15);
    const auto& b = base[iSeg];
    const auto len = b.s.back() - b.s.front();
    for (unsigned long j = 0; j < b.s.size(); ++j) {
      const auto r = std::min(b.s[j] - b.s.front(), b.s.back() - b.s[j]);
      if (r > 0.2 * len) CHECK(BLWallRule::SizeAt(smp, b.s[j]) == b.size[j]);
    }
    CHECK(smp.s.size() > b.s.size());  // the break points of the floor profile were added
  }
  /*--- Order independence: the corners in the other order give the same sizes. ---*/
  auto reversed = corners;
  std::reverse(reversed.begin(), reversed.end());
  auto samples2 = base;
  BLWallRule::ApplyCorners(wall, reversed, rule.gradation, samples2);
  for (unsigned long iSeg = 0; iSeg < 2; ++iSeg) CHECK(samples2[iSeg].size == samples[iSeg].size);

  /*--- Different first heights on the two sides: the symmetry keeps each side's own t_min. ---*/
  /*--- t_min 2e-5 on one side, 8e-5 on the other (h0 1e-5 and 4e-5), with and without the floor. ---*/
  for (const bool floorOn : {false, true}) {
    auto unequal = base;
    unequal[1].tmin = 8e-5;
    for (auto& t : unequal[1].size) t = std::max(t, 8e-5);
    BLWallRule::Grade(unequal[1], rule.gradation, false);
    auto cs = corners;
    for (auto& c : cs) if (!floorOn) c.floor = 0.0;
    const auto ch = BLWallRule::ApplyCorners(wall, cs, rule.gradation, unequal);
    for (const auto t : unequal[1].size) CHECK(t >= 8e-5 * (1 - 1e-12));
    bool lowered = false;
    for (const auto t : unequal[0].size) lowered = lowered || (t < 8e-5);
    if (!floorOn) CHECK(lowered);  // the other side keeps its own smaller sizes (the floor would raise them)
    for (const auto& smp : unequal)
      for (unsigned long j = 1; j < smp.s.size(); ++j)
        CHECK(fabs(smp.size[j] - smp.size[j - 1]) <= rule.gradation * (smp.s[j] - smp.s[j - 1]) * (1 + 1e-9) + 1e-15);
    REQUIRE(ch.changed.size() == cs.size());
    for (const auto d : ch.changed) CHECK((floorOn ? d > 0.0 : d == 0.0));  // without the floor nothing changes
  }
}

TEST_CASE("Corner rule: symmetry between samples with the floor disabled", "[Adaptation]") {
  const struct {
    passivedouble length;
    bool largerAtEnd;
  } cases[] = {{0.01, false}, {3e-4, false}, {0.02, true}};
  for (const auto& test : cases) {
    const auto length = test.length;
    CReferenceWall wall(PolylineMesh({{-length, 0.0}, {0.0, 0.0}, {0.0, 0.01}}, false), {"wall"}, 45.0);
    const auto corners = BLWallRule::FindCorners(wall);
    REQUIRE(corners.size() == 1);
    REQUIRE(corners[0].floor == 0.0);
    BLWallRule::SizeRule rule;
    rule.h0 = 1e-4;
    rule.hmin = 1e-6;
    rule.hmax = 1.0;
    auto sensor = [&](const passivedouble* x, const passivedouble*) {
      return (test.largerAtEnd ? x[1] <= 0.0 : x[1] > 0.0) ? 1e-3 : 2e-4;
    };
    std::vector<BLWallRule::SizeSamples> samples;
    for (unsigned long iSeg = 0; iSeg < 2; ++iSeg)
      samples.push_back(test.largerAtEnd ? BLWallRule::SampleSize(wall, iSeg, rule, sensor)
                                        : BLWallRule::SampleSize(wall, iSeg, rule, sensor, 10));
    BLWallRule::ApplyCorners(wall, corners, rule.gradation, samples);
    /*--- Reach 4e-4 falls between the coarse samples, clipped to the shorter side. ---*/
    const auto reach = std::min(4e-4, length);
    const auto& c = corners[0];
    for (const passivedouble fraction : {0.0, 0.25, 0.5, 0.975, 1.0}) {
      const auto r = fraction * reach;
      passivedouble t[2];
      for (unsigned short k = 0; k < 2; ++k) {
        const auto& smp = samples[c.seg[k]];
        t[k] = BLWallRule::SizeAt(smp, c.atEnd[k] ? smp.s.back() - r : smp.s.front() + r);
      }
      CHECK(t[0] == Approx(t[1]).epsilon(1e-12));
    }
  }
}

TEST_CASE("Corner rule: overlapping symmetry windows with the floor disabled", "[Adaptation]") {
  auto mesh = PolylineMesh({{-0.01, 0.0}, {0.0, 0.0}, {0.0, 0.001}, {0.01, 0.001}}, false);
  auto marker = mesh.markers.front();
  mesh.markers.clear();
  const std::vector<std::string> names = {"wall0", "wall1", "wall2"};
  for (unsigned long i = 0; i < 3; ++i) {
    marker.name = names[i];
    marker.ref = i + 1;
    marker.elem = {i, i + 1};
    mesh.markers.push_back(marker);
  }
  CReferenceWall wall(mesh, names, 45.0);
  REQUIRE(wall.GetSegments().size() == 3);
  const auto corners = BLWallRule::FindCorners(wall);
  REQUIRE(corners.size() == 2);
  BLWallRule::SizeRule rule;
  rule.h0 = 1e-4;
  const passivedouble sensorSize[] = {1e-3, 1e-3, 2e-4};
  std::vector<BLWallRule::SizeSamples> base;
  for (unsigned long iSeg = 0; iSeg < 3; ++iSeg) {
    REQUIRE(wall.GetSegments()[iSeg].marker == names[iSeg]);
    base.push_back(BLWallRule::SampleSize(wall, iSeg, rule,
                                        [&](const passivedouble*, const passivedouble*) { return sensorSize[iSeg]; }));
  }
  auto samples = base;
  BLWallRule::ApplyCorners(wall, corners, rule.gradation, samples);
  for (const auto& c : corners) {
    REQUIRE(c.floor == 0.0);
    /*--- Both initial windows reach 0.002; their common part is the whole 0.001 middle segment. ---*/
    for (int i = 0; i <= 100; ++i) {
      const auto r = 0.001 * i / 100.0;
      passivedouble t[2];
      for (unsigned short k = 0; k < 2; ++k) {
        const auto& smp = samples[c.seg[k]];
        t[k] = BLWallRule::SizeAt(smp, c.atEnd[k] ? smp.s.back() - r : smp.s.front() + r);
      }
      CHECK(t[0] == Approx(t[1]).epsilon(1e-12));
      CHECK(t[0] == Approx(2e-4).epsilon(1e-12));
    }
  }
  auto reversed = corners;
  std::reverse(reversed.begin(), reversed.end());
  auto samples2 = base;
  BLWallRule::ApplyCorners(wall, reversed, rule.gradation, samples2);
  for (unsigned long iSeg = 0; iSeg < 3; ++iSeg) {
    CHECK(samples2[iSeg].s == samples[iSeg].s);
    CHECK(samples2[iSeg].size == samples[iSeg].size);
    for (const auto t : samples[iSeg].size) CHECK(t >= samples[iSeg].tmin * (1 - 1e-12));
    for (unsigned long j = 1; j < samples[iSeg].s.size(); ++j)
      CHECK(fabs(samples[iSeg].size[j] - samples[iSeg].size[j - 1]) <=
            rule.gradation * (samples[iSeg].s[j] - samples[iSeg].s[j - 1]) * (1 + 1e-9) + 1e-15);
  }
}

TEST_CASE("Corner rule: shared samples agree through overlapping symmetry windows", "[Adaptation]") {
  /*--- One knot interval on the first two markers, ten on the third; its sensor has a minimum at r = 0.001.
   *    A single grid pass need not mirror the second corner's new break points through the first corner. ---*/
  std::vector<std::array<passivedouble, 2>> points = {{-0.01, 0.0}, {0.0, 0.0}, {0.0, 0.0025}};
  for (int i = 1; i <= 10; ++i) points.push_back({0.001 * i, 0.0025});
  auto mesh = PolylineMesh(points, false);
  auto marker = mesh.markers.front();
  mesh.markers.clear();
  const std::vector<std::string> names = {"wall0", "wall1", "wall2"};
  for (unsigned long i = 0; i < 3; ++i) {
    marker.name = names[i];
    marker.ref = i + 1;
    marker.elem = {i, i + 1};
    if (i == 2)
      for (unsigned long j = 3; j + 1 < points.size(); ++j) marker.elem.insert(marker.elem.end(), {j, j + 1});
    mesh.markers.push_back(marker);
  }
  CReferenceWall wall(mesh, names, 45.0);
  REQUIRE(wall.GetSegments().size() == 3);
  auto corners = BLWallRule::FindCorners(wall);
  REQUIRE(corners.size() == 2);
  BLWallRule::SizeRule rule;
  rule.h0 = 1e-4;
  std::vector<BLWallRule::SizeSamples> base;
  for (unsigned long iSeg = 0; iSeg < 3; ++iSeg) {
    REQUIRE(wall.GetSegments()[iSeg].marker == names[iSeg]);
    base.push_back(BLWallRule::SampleSize(wall, iSeg, rule, [&](const passivedouble* x, const passivedouble*) {
      return iSeg == 2 ? 2e-4 + 0.15 * fabs(x[0] - 0.001) : 1e-3;
    }));
  }
  for (const bool reverse : {false, true}) {
    if (reverse) std::reverse(corners.begin(), corners.end());
    auto samples = base;
    BLWallRule::ApplyCorners(wall, corners, rule.gradation, samples);
    for (const auto& c : corners) {
      REQUIRE(c.floor == 0.0);
      /*--- Both initial windows reach 0.002. Symmetry is guaranteed only at shared sample distances. ---*/
      const auto& a = samples[c.seg[0]];
      const auto& b = samples[c.seg[1]];
      const auto tol = 8.0 * std::numeric_limits<passivedouble>::epsilon() *
                       std::max(a.s.back() - a.s.front(), b.s.back() - b.s.front());
      unsigned long shared = 0;
      for (unsigned long j = 0; j < a.s.size(); ++j) {
        const auto r = c.atEnd[0] ? a.s.back() - a.s[j] : a.s[j] - a.s.front();
        if (r > 0.002 + tol) continue;
        const auto s = c.atEnd[1] ? b.s.back() - r : b.s.front() + r;
        for (unsigned long k = 0; k < b.s.size(); ++k)
          if (fabs(b.s[k] - s) <= tol) {
            CHECK(a.size[j] == Approx(b.size[k]).epsilon(1e-12));
            ++shared;
          }
      }
      CHECK(shared > 1);
    }
  }
}

TEST_CASE("Corner rule: cyclic square windows have bounded sample growth", "[Adaptation]") {
  for (const passivedouble y : {0.007, 0.0070000001}) {
    CAPTURE(y);
    CReferenceWall wall(PolylineMesh({{0.0, 0.0}, {0.007, 0.0}, {0.007, 0.007}, {0.0, y}}, true), {"wall"}, 45.0);
    const auto corners = BLWallRule::FindCorners(wall);
    REQUIRE(corners.size() == 4);
    BLWallRule::SizeRule rule;
    rule.h0 = 0.001;
    std::vector<BLWallRule::SizeSamples> samples;
    unsigned long initial = 0;
    for (unsigned long iSeg = 0; iSeg < wall.GetSegments().size(); ++iSeg) {
      samples.push_back(BLWallRule::SampleSize(wall, iSeg, rule,
                                             [](const passivedouble*, const passivedouble*) { return 0.005; }));
      initial += samples.back().s.size();
      for (const auto t : samples.back().size) REQUIRE(t == Approx(0.002));
    }
    for (const auto& c : corners) REQUIRE(c.floor == 0.0);
    const auto start = std::chrono::steady_clock::now();
    BLWallRule::ApplyCorners(wall, corners, rule.gradation, samples);
    CHECK(std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count() < 5.0);
    unsigned long total = 0;
    for (const auto& smp : samples) {
      total += smp.s.size();
      for (const auto t : smp.size) CHECK(t >= smp.tmin * (1 - 1e-12));
      for (unsigned long j = 1; j < smp.s.size(); ++j)
        CHECK(fabs(smp.size[j] - smp.size[j - 1]) <=
              rule.gradation * (smp.s[j] - smp.s[j - 1]) * (1 + 1e-9) + 1e-15);
    }
    CHECK(total <= 10 * initial);
  }
}

TEST_CASE("Corner rule: connected zigzag has bounded sample growth", "[Adaptation]") {
  const unsigned long n = 2000;
  std::vector<std::array<passivedouble, 2>> points;
  for (unsigned long i = 0; i <= n; ++i)
    points.push_back({0.0005 * i, 0.0005 * (i % 2) + 1e-10 * sin(1.2345 * i)});
  CReferenceWall wall(PolylineMesh(points, false), {"wall"}, 45.0);
  REQUIRE(wall.GetSegments().size() == n);
  const auto corners = BLWallRule::FindCorners(wall);
  REQUIRE(corners.size() == n - 1);
  for (const auto& c : corners) REQUIRE(c.floor == 0.0);
  BLWallRule::SizeRule rule;
  rule.h0 = 2e-4;
  std::vector<BLWallRule::SizeSamples> samples;
  unsigned long initial = 0;
  for (unsigned long iSeg = 0; iSeg < wall.GetSegments().size(); ++iSeg) {
    samples.push_back(BLWallRule::SampleSize(wall, iSeg, rule,
                                           [](const passivedouble*, const passivedouble*) { return 0.005; }));
    initial += samples.back().s.size();
  }
  const auto start = std::chrono::steady_clock::now();
  BLWallRule::ApplyCorners(wall, corners, rule.gradation, samples);
  const auto elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
  unsigned long total = 0;
  for (const auto& smp : samples) total += smp.s.size();
  CAPTURE(initial, total, elapsed);
  CHECK(total <= 4 * initial);
  CHECK(elapsed < 5.0);
}

TEST_CASE("Corner rule: convexity from the input mesh (fluid angle, fan, wedge)", "[Adaptation]") {
  const auto omega = 16.0 * M_PI / 180.0;
  const auto lens = Lens(omega, 40);
  auto config = LensConfig(false, "ADAP_BL_CORNER_FLOOR= 2\n");
  {
    const auto mesh = OMesh(lens, 6, 5.0);
    CReferenceWall reference(mesh, {"wall"}, 45.0);
    unsigned long nUnmatched = 0;
    const auto corners = CBoundaryLayerRemesher::Corners(*config, mesh, reference, nUnmatched);
    CHECK(nUnmatched == 0);
    REQUIRE(corners.size() == 2);
    for (const auto& c : corners) {
      CHECK(c.matched);
      CHECK(c.convex);
      const unsigned long tip = c.x[0] > 0.0 ? 0 : 40;
      CHECK(c.wedge == Approx(TipWedge(lens, tip)).epsilon(1e-12));
      CHECK(c.floor == Approx(2.0 * 1e-4 / sin(0.5 * c.wedge)));
    }
    /*--- A triangle at a tip turned over: the fan is not valid, no floor. ---*/
    auto broken = mesh;
    for (unsigned long e = 0; e < broken.GetnElem(); ++e) {
      auto* v = &broken.elem[3 * e];
      if (v[0] == 0 || v[1] == 0 || v[2] == 0) {
        std::swap(v[1], v[2]);
        break;
      }
    }
    const auto bad = CBoundaryLayerRemesher::Corners(*config, broken, reference, nUnmatched);
    CHECK(nUnmatched == 1);
    for (const auto& c : bad)
      if (c.x[0] > 0.0) CHECK_FALSE(c.matched);
  }
  /*--- Concave corner: the inside of a box with a wall on two sides (fluid angle 90 degrees). ---*/
  {
    CSimplexMesh box;
    box.nDim = 2;
    box.coord = {0, 0, 1, 0, 1, 1, 0, 1, 0.5, 0.5};
    box.elem = {0, 1, 4, 1, 2, 4, 2, 3, 4, 3, 0, 4};
    box.elemRef.assign(4, 0);
    CSimplexMesh::Marker wall, far;
    wall.name = "wall";
    wall.ref = 2;
    wall.elem = {3, 0, 0, 1};
    far.name = "farfield";
    far.ref = 1;
    far.elem = {1, 2, 2, 3};
    box.markers = {far, wall};
    CHECK(CBoundaryLayerRemesher::FluidAngle(box, 0) == Approx(0.5 * M_PI));
    CHECK(CBoundaryLayerRemesher::FluidAngle(box, 4) == Approx(2.0 * M_PI));
    CReferenceWall reference(box, {"wall"}, 45.0);
    unsigned long nUnmatched = 0;
    const auto corners = CBoundaryLayerRemesher::Corners(*config, box, reference, nUnmatched);
    REQUIRE(corners.size() == 1);
    CHECK(corners[0].matched);
    CHECK_FALSE(corners[0].convex);
    CHECK(corners[0].floor == 0.0);
  }
}

#ifdef HAVE_MMG
TEST_CASE("Two-pass remesh of a lens: sharp tips classified and reported (SERIAL_BL_FIX_PLAN 11.7)", "[Adaptation][MMG]") {
  const passivedouble h0 = 1e-4;
  for (const passivedouble degrees : {16.0, 40.0}) {
    for (const bool split : {false, true}) {
      if (split && degrees != 16.0) continue;
      const auto lens = Lens(degrees * M_PI / 180.0, 40);
      auto mesh = OMesh(lens, 14, 5.0, split ? 40 : 0);
      /*--- Sensor: size 0.05, isotropic and finer around the tips (2 h0 + log(1.3) r, as ADAP_ISO_CORNER). ---*/
      const auto nPoint = mesh.GetnPoint();
      mesh.metric.assign(3 * nPoint, 0.0);
      for (unsigned long i = 0; i < nPoint; ++i) {
        const auto x = mesh.coord[2 * i], y = mesh.coord[2 * i + 1];
        const auto r = std::min(std::hypot(x - 0.5, y), std::hypot(x + 0.5, y));
        const auto s = std::min(0.05, 2.0 * h0 + log(1.3) * r);
        mesh.metric[3 * i] = mesh.metric[3 * i + 2] = 1.0 / (s * s);
      }
      const std::vector<std::string> walls = split ? std::vector<std::string>{"wall", "wall2"}
                                                   : std::vector<std::string>{"wall"};
      CReferenceWall reference(mesh, walls, 45.0);
      auto config = LensConfig(split);
      CBoundaryLayerRemesher::Report report;
      CSimplexMesh adapted;
      {
        std::unique_ptr<Mute> mute(getenv("SU2_TEST_VERBOSE") == nullptr ? new Mute : nullptr);
        adapted = CBoundaryLayerRemesher::TwoPass(*config, mesh, reference, report);
      }
      INFO("wedge " << degrees << " split " << split);
      CMMGInterface::ValidateMesh(adapted, &mesh, "lens");
      CHECK(report.passA);
      CHECK(report.nG2 + report.nG3 + report.nG4 + report.nG5 == 0);
      CHECK(report.nCorner == 2);
      CHECK(report.nConvexCorner == 2);
      CHECK(report.nCornerUnmatched == 0);
      /*--- The first cells near the tips are reported, not required in [0.5, 2] h0: no tested rule achieves that on
       *    every lens (SERIAL_BL_FIX_PLAN.md 11.7, calibration); at least half of them are. ---*/
      CHECK(report.nCornerFace >= 8);
      CHECK(2 * report.nCornerFaceOut <= report.nCornerFace);
      CHECK(report.cornerFaceMax <= 4.0);
      CHECK(report.nMetricViolations == 0);
      REQUIRE(report.nFirstNode.size() == (split ? 2u : 1u));
      CHECK(report.nFirstNode[0] > 0);
    }
  }
}
#endif

#ifdef HAVE_MMG
TEST_CASE("Corner floor calibration sweep on lenses (developer, hidden)", "[.BLCornerSweep]") {
  /*--- Prints one line per lens variant: wedge, points per side, split, faces near the tips, outside [0.5, 2] h0,
   *    smallest and largest first cell / h0, effective floor / h0, points. The factor m_c comes from
   *    ADAP_BL_CORNER_FLOOR in SU2_TEST_CORNER_FLOOR (default the built-in value). ---*/
  const passivedouble h0 = 1e-4;
  const char* factor = getenv("SU2_TEST_CORNER_FLOOR");
  for (const passivedouble degrees : {10.0, 16.0, 24.0, 40.0})
    for (const unsigned long n : {30ul, 40ul, 55ul})
      for (const bool split : {false, true}) {
        const auto lens = Lens(degrees * M_PI / 180.0, n);
        auto mesh = OMesh(lens, 14, 5.0, split ? n : 0);
        const auto nPoint = mesh.GetnPoint();
        mesh.metric.assign(3 * nPoint, 0.0);
        for (unsigned long i = 0; i < nPoint; ++i) {
          const auto x = mesh.coord[2 * i], y = mesh.coord[2 * i + 1];
          const auto r = std::min(std::hypot(x - 0.5, y), std::hypot(x + 0.5, y));
          const auto s = std::min(0.05, 2.0 * h0 + log(1.3) * r);
          mesh.metric[3 * i] = mesh.metric[3 * i + 2] = 1.0 / (s * s);
        }
        const std::vector<std::string> walls = split ? std::vector<std::string>{"wall", "wall2"}
                                                     : std::vector<std::string>{"wall"};
        CReferenceWall reference(mesh, walls, 45.0);
        auto config = LensConfig(split, factor ? "ADAP_BL_CORNER_FLOOR= " + string(factor) + "\n" : "");
        CBoundaryLayerRemesher::Report report;
        CSimplexMesh adapted;
        {
          std::unique_ptr<Mute> mute(new Mute);
          adapted = CBoundaryLayerRemesher::TwoPass(*config, mesh, reference, report);
        }
        std::cerr << "SWEEP " << degrees << " " << n << " " << split << " " << report.passA << " "
                  << report.nCornerFace << " " << report.nCornerFaceOut << " " << report.cornerFaceMin << " "
                  << report.cornerFaceMax << " " << report.corners[0].floor / h0 << " " << adapted.GetnPoint() << endl;
      }
}
#endif

#ifdef HAVE_MMG
namespace {
/*--- 2D SU2 ASCII mesh (triangles, quadrilaterals split; line markers) into a CSimplexMesh. ---*/
CSimplexMesh ReadSU2(const string& file) {
  std::ifstream in(file);
  if (!in) throw std::runtime_error("cannot open " + file);
  CSimplexMesh mesh;
  mesh.nDim = 2;
  string line;
  auto value = [](const string& l) { return std::stoul(l.substr(l.find('=') + 1)); };
  short ref = 0;
  while (std::getline(in, line)) {
    if (line.rfind("NELEM", 0) == 0) {
      const auto n = value(line);
      for (unsigned long i = 0; i < n; ++i) {
        std::getline(in, line);
        std::istringstream ss(line);
        unsigned long type, a, b, c, d;
        ss >> type >> a >> b >> c;
        if (type == 5) {
          mesh.elem.insert(mesh.elem.end(), {a, b, c});
        } else {
          ss >> d;
          mesh.elem.insert(mesh.elem.end(), {a, b, c, a, c, d});
        }
      }
    } else if (line.rfind("NPOIN", 0) == 0) {
      const auto n = value(line);
      for (unsigned long i = 0; i < n; ++i) {
        std::getline(in, line);
        std::istringstream ss(line);
        passivedouble x, y;
        ss >> x >> y;
        mesh.coord.insert(mesh.coord.end(), {x, y});
      }
    } else if (line.rfind("MARKER_TAG", 0) == 0) {
      CSimplexMesh::Marker marker;
      auto name = line.substr(line.find('=') + 1);
      name.erase(std::remove(name.begin(), name.end(), ' '), name.end());
      marker.name = name;
      marker.ref = ++ref;
      std::getline(in, line);
      const auto n = value(line);
      for (unsigned long i = 0; i < n; ++i) {
        std::getline(in, line);
        std::istringstream ss(line);
        unsigned long type, a, b;
        ss >> type >> a >> b;
        marker.elem.insert(marker.elem.end(), {a, b});
      }
      mesh.markers.push_back(marker);
    }
  }
  mesh.elemRef.assign(mesh.GetnElem(), 0);
  /*--- Positive orientation. ---*/
  for (unsigned long e = 0; e < mesh.GetnElem(); ++e) {
    auto* v = &mesh.elem[3 * e];
    if (BLWallRule::Orientation(&mesh.coord[2 * v[0]], &mesh.coord[2 * v[1]], &mesh.coord[2 * v[2]]) < 0)
      std::swap(v[1], v[2]);
  }
  return mesh;
}
}  // namespace

TEST_CASE("Fixed analytic metric, repeated two-pass remeshes (developer, M3.3, hidden)", "[.BLFixedMetric]") {
  /*--- SU2_TEST_MESH: plate (marker wall) or NACA (marker airfoil) input mesh; SU2_TEST_CASE: plate or naca. ---*/
  const char* file = getenv("SU2_TEST_MESH");
  const char* kind = getenv("SU2_TEST_CASE");
  if (file == nullptr || kind == nullptr) {
    WARN("SU2_TEST_MESH and SU2_TEST_CASE not set: nothing to do.");
    return;
  }
  const bool plate = string(kind) == "plate";
  const string wallName = plate ? "wall" : "airfoil";
  const passivedouble h0 = plate ? 2e-6 : 4e-6;
  auto mesh = ReadSU2(file);
  auto metricAt = [&](CSimplexMesh& m) {
    m.metric.assign(3 * m.GetnPoint(), 0.0);
    for (unsigned long i = 0; i < m.GetnPoint(); ++i) {
      const auto x = m.coord[2 * i], y = m.coord[2 * i + 1];
      auto* M = &m.metric[3 * i];
      if (plate) {
        const auto hn = std::min(0.05, 1e-4 + 0.1 * fabs(y));
        M[0] = 1.0 / (0.02 * 0.02);
        M[2] = 1.0 / (hn * hn);
      } else {
        const auto d = std::hypot(x - std::min(1.0, std::max(0.0, x)), y);
        const auto s = std::min(50.0, 2e-3 + 0.1 * d);
        M[0] = M[2] = 1.0 / (s * s);
      }
    }
  };
  const bool quiet = getenv("SU2_TEST_VERBOSE") == nullptr;
  std::unique_ptr<CConfig> config;
  {
    std::unique_ptr<Mute> mute(quiet ? new Mute : nullptr);
    stringstream ss(string("SOLVER= RANS\nKIND_TURB_MODEL= SA\nREYNOLDS_NUMBER= 6e6\nMACH_NUMBER= 0.15\n") +
                    "MESH_FORMAT= SU2\nMESH_FILENAME= unused.su2\nMGLEVEL= 0\nCOMPUTE_METRIC= YES\nADAP_SENSOR= (MACH)\n" +
                    (plate ? "MARKER_HEATFLUX= (wall, 0.0)\nMARKER_FAR= (farfield, inlet, outlet)\nMARKER_SYM= (symmetry)\n"
                                 "ADAP_HMAX= 0.5\nADAP_BL_FIRST_HEIGHT= (2e-6)\n"
                           : "MARKER_HEATFLUX= (airfoil, 0.0)\nMARKER_FAR= (farfield)\nADAP_HMAX= 50\n"
                             "ADAP_BL_FIRST_HEIGHT= (4e-6)\n") +
                    "ADAP_HMIN= 1e-6\nADAP_ARMAX= 1e4\nADAP_BL_MARKER= (" + wallName + ")\nADAP_BL_GROWTH= (1.15)\n"
                    "ADAP_BL_THICKNESS= (0.05)\nADAP_BL_METHOD= TWO_PASS\nADAP_SURFACE= YES\n" +
                    (getenv("SU2_TEST_EXTRA") ? getenv("SU2_TEST_EXTRA") : ""));
    config.reset(new CConfig(ss, SU2_COMPONENT::SU2_CFD, false));
  }
  CReferenceWall reference(mesh, {wallName}, SU2_TYPE::GetValue(config->GetAdap_Angle()));
  for (int cycle = 1; cycle <= 3; ++cycle) {
    metricAt(mesh);
    CBoundaryLayerRemesher::Report report;
    {
      std::unique_ptr<Mute> mute(quiet ? new Mute : nullptr);
      mesh = CBoundaryLayerRemesher::TwoPass(*config, mesh, reference, report);
    }
    /*--- Face heights of the wall: in-band fraction (count). ---*/
    const auto* wall = mesh.FindMarker(wallName);
    std::set<std::pair<unsigned long, unsigned long>> faces;
    for (unsigned long i = 0; i < wall->GetnElem(2); ++i) faces.insert(std::minmax(wall->elem[2 * i], wall->elem[2 * i + 1]));
    unsigned long nFace = 0, nIn = 0;
    std::vector<passivedouble> heights;
    for (unsigned long e = 0; e < mesh.GetnElem(); ++e) {
      const auto* v = &mesh.elem[3 * e];
      for (int k = 0; k < 3; ++k) {
        const auto a = v[(k + 1) % 3], b = v[(k + 2) % 3];
        if (!faces.count(std::minmax(a, b))) continue;
        const passivedouble* pa = &mesh.coord[2 * a];
        const passivedouble* pb = &mesh.coord[2 * b];
        const passivedouble* pc = &mesh.coord[2 * v[k]];
        const auto len = std::hypot(pb[0] - pa[0], pb[1] - pa[1]);
        const auto h = fabs((pb[0] - pa[0]) * (pc[1] - pa[1]) - (pb[1] - pa[1]) * (pc[0] - pa[0])) / len / h0;
        heights.push_back(h);
        nFace++;
        nIn += (h >= 0.5 && h <= 2.0);
      }
    }
    std::sort(heights.begin(), heights.end());
    std::cerr << "FIXEDMETRIC " << kind << " cycle " << cycle << " points " << mesh.GetnPoint() << " passA "
              << report.passA << " attempts " << report.attempts << " wallFaces " << nFace << " inBand "
              << 100.0 * nIn / std::max(1ul, nFace) << "% median " << heights[heights.size() / 2] << " cornerFacesOut "
              << report.nCornerFaceOut << "/" << report.nCornerFace << " cornerMin " << report.cornerFaceMin
              << " firstNode " << (report.nFirstNode.empty() ? 0 : report.nFirstNode[0]) << " below "
              << (report.nFirstNodeBelow.empty() ? 0 : report.nFirstNodeBelow[0]) << " above "
              << (report.nFirstNodeAbove.empty() ? 0 : report.nFirstNodeAbove[0]) << " sensorLoss "
              << report.nSensorLoss << endl;
  }
}
#endif
