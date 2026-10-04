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
#include <fstream>
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
