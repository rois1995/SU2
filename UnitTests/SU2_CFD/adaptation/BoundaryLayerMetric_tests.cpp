/*!
 * \file BoundaryLayerMetric_tests.cpp
 * \brief Unit tests for the boundary-layer metric of wall markers (mesh adaptation).
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
#include <cmath>
#include <memory>
#include <map>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <cstdlib>
#include <chrono>

#include "../../../Common/include/adaptation/CComplexityNormalization.hpp"
#include "../../../Common/include/adaptation/CNativeChordEnvelope.hpp"
#include "../../../Common/include/CConfig.hpp"
#include "../../../Common/include/adaptation/CNativeImport2D.hpp"
#include "../../../Common/include/adaptation/CNativeField3D.hpp"
#include "../../../Common/include/adaptation/CNativeImport3D.hpp"
#include "../../../Common/include/geometry/CPhysicalGeometry.hpp"
#include "../../../Common/include/geometry/meshreader/CMemoryMeshReaderFVM.hpp"
#include "../../../Common/include/linear_algebra/blas_structure.hpp"
#include "../../../Common/include/adaptation/CNativeMetric3D.hpp"
#include "../../../Common/include/toolboxes/geometry_toolbox.hpp"
#include "../../../SU2_CFD/include/adaptation/CBoundaryLayerMetric.hpp"
#include "../../../SU2_CFD/include/adaptation/CNativeComplexity3D.hpp"
#include "../../../SU2_CFD/include/drivers/CDriver.hpp"
#include "../../../SU2_CFD/include/solvers/CSolver.hpp"
#include "../../../SU2_CFD/include/solvers/CSolverFactory.hpp"
#include "../../Common/adaptation/SimplexMeshTestCase.hpp"

namespace {

using Tensor = CBoundaryLayerMetric::Tensor;

/*--- Wall-normal size of geometric rows at a distance from the wall. ---*/
su2double NormalSize(su2double h0, su2double growth, su2double d) {
  return max(h0, 2.0 * (h0 + (growth - 1.0) * d) / (growth + 1.0));
}

/*--- Smoothstep weight of the fade. ---*/
su2double Weight(su2double d, su2double h0, su2double thickness) {
  const su2double full = max(h0, 0.9 * thickness), width = thickness - full;
  const su2double s = min(max((d - full) / width, su2double(0.0)), su2double(1.0));
  return 1.0 - s * s * (3.0 - 2.0 * s);
}

/*--- t t^T / ht^2 + n n^T / hn^2 in 2D. ---*/
Tensor Metric2D(const su2double* n, su2double ht, su2double hn) {
  Tensor M;
  const su2double t[2] = {-n[1], n[0]};
  for (int i = 0; i < 2; ++i)
    for (int j = 0; j < 2; ++j) M.m[i][j] = t[i] * t[j] / (ht * ht) + n[i] * n[j] / (hn * hn);
  return M;
}

/*--- Eigenvalues of M^w * core^(1-w) (log interpolation towards an isotropic metric). ---*/
Tensor Fade(unsigned short nDim, const Tensor& M, su2double w, su2double core) {
  su2double vec[3][3], val[3], work[3];
  CBlasStructure::EigenDecomposition(M.m, vec, val, nDim, work);
  for (unsigned short i = 0; i < nDim; ++i) val[i] = pow(val[i], w) * pow(core, 1.0 - w);
  Tensor F;
  CBlasStructure::EigenRecomposition(F.m, vec, val, nDim);
  return F;
}

void CheckTensor(unsigned short nDim, const Tensor& A, const Tensor& B, su2double tol = 1e-10) {
  su2double scale = 0.0;
  for (unsigned short i = 0; i < nDim; ++i)
    for (unsigned short j = 0; j < nDim; ++j) scale = max(scale, fabs(B.m[i][j]));
  for (unsigned short i = 0; i < nDim; ++i)
    for (unsigned short j = 0; j < nDim; ++j) CHECK(fabs(A.m[i][j] - B.m[i][j]) <= tol * scale);
}

/*--- Straight wall y = 0 from x = 0 to 2 with points at the given x. ---*/
CBoundaryLayerMetric::Wall FlatWall2D(const std::vector<su2double>& xs, su2double h0, su2double growth,
                                      su2double thickness) {
  CBoundaryLayerMetric::Wall wall;
  wall.layer.marker = "wall";
  wall.layer.firstHeight = h0;
  wall.layer.growth = growth;
  wall.layer.thickness = thickness;
  for (const auto x : xs) {
    wall.coord.push_back(x);
    wall.coord.push_back(0.0);
  }
  for (unsigned long i = 0; i + 1 < xs.size(); ++i) {
    wall.conn.push_back(i);
    wall.conn.push_back(i + 1);
  }
  return wall;
}

/*--- Silence the console in a scope. ---*/
struct Mute {
  std::streambuf* buffer = cout.rdbuf();
  Mute() { cout.rdbuf(nullptr); }
  ~Mute() { cout.rdbuf(buffer); }
};

/*!
 * \brief Geometry and flow solver of a mesh in memory, with a constant Hessian of one sensor (MACH).
 */
struct MetricTest {
  std::unique_ptr<CConfig> config;
  CGeometry** geometry = nullptr;
  CSolver** solver = nullptr;

  MetricTest(const CSimplexMesh& mesh, const string& options, const std::array<su2double, 6>& hessian) {
    Mute mute;
    stringstream ss("SOLVER= NAVIER_STOKES\nREYNOLDS_NUMBER= 1e6\nMACH_NUMBER= 0.5\nMESH_FORMAT= SU2\n"
                    "MESH_FILENAME= unused.su2\nMGLEVEL= 0\nCOMPUTE_METRIC= YES\nADAP_SENSOR= (MACH)\nADAP_NORM= 2\n"
                    "NUM_METHOD_HESS= GREEN_GAUSS\n" + options);
    config = std::unique_ptr<CConfig>(new CConfig(ss, SU2_COMPONENT::SU2_CFD, false));
    CMemoryMeshReaderFVM reader(config.get(), mesh, 0, 1);
    CDriver::BuildGeometryFVM(config.get(), new CPhysicalGeometry(config.get(), reader, 1), geometry, true);
    solver = CSolverFactory::CreateSolverContainer(config->GetKind_Solver(), config.get(), geometry[MESH_0], 0);

    auto& H = solver[FLOW_SOL]->GetNodes()->GetHessian();
    const auto nMet = (mesh.nDim == 2) ? 3u : 6u;
    for (auto iPoint = 0ul; iPoint < Geometry().GetnPoint(); ++iPoint)
      for (auto iMet = 0u; iMet < nMet; ++iMet) H(iPoint, 0, iMet) = hessian[iMet];
  }

  ~MetricTest() {
    for (unsigned short iSol = 0; iSol < MAX_SOLS; ++iSol) {
      CSolverFactory::ClearSolverMeta(solver[iSol]);
      delete solver[iSol];
    }
    delete[] solver;
    delete geometry[MESH_0];
    delete[] geometry;
  }

  CGeometry& Geometry() const { return *geometry[MESH_0]; }

  string ComputeMetric() {
    stringstream out;
    auto buffer = cout.rdbuf();
    cout.rdbuf(out.rdbuf());
    solver[FLOW_SOL]->ComputeMetric(geometry[MESH_0], config.get());
    cout.rdbuf(buffer);
    return out.str();
  }

  Tensor Metric(unsigned long iPoint) const {
    Tensor M;
    solver[FLOW_SOL]->GetNodes()->GetMetricMat(iPoint, M.m);
    return M;
  }
};

}  // namespace

TEST_CASE("Boundary-layer metric of a flat wall 2D", "[Adaptation]") {
  /*--- Wall y = 0, x in [0, 2], edges of 0.1; h0 1e-3, growth 1.2, thickness 0.1 (full strength up to 0.09, fade
   *    from 0.09 to 0.1). ---*/
  const su2double h0 = 1e-3, growth = 1.2, thickness = 0.1, core = 1.0;
  std::vector<su2double> xs;
  for (int i = 0; i <= 20; ++i) xs.push_back(0.1 * i);
  CBoundaryLayerMetric metric(2, {FlatWall2D(xs, h0, growth, thickness)});

  const su2double up[2] = {0.0, 1.0};
  for (const su2double y : {0.0, 5e-4, 1e-2, 5e-2, 0.09, 0.095, 0.0975, 0.1, 0.15}) {
    const su2double x[2] = {1.05, y};
    const auto sample = metric.Evaluate(0, x, core);
    CHECK(sample.distance == Approx(y).margin(1e-15));
    const su2double w = Weight(y, h0, thickness);
    CHECK(sample.weight == Approx(w).margin(1e-14));
    if (y >= thickness) {
      CHECK(sample.weight == 0.0);
      continue;
    }
    CHECK(sample.hn == Approx(NormalSize(h0, growth, y)));
    CHECK(fabs(sample.normal[1]) == Approx(1.0));
    const auto expected = Metric2D(up, 0.1, NormalSize(h0, growth, y));
    CheckTensor(2, sample.full, expected);
    CheckTensor(2, sample.metric, (w < 1.0) ? Fade(2, expected, w, core) : expected);
  }
  /*--- Half way through the fade the eigenvalues are the geometric means with the core eigenvalue. ---*/
  {
    const su2double x[2] = {1.05, 0.095};
    const auto sample = metric.Evaluate(0, x, core);
    CHECK(sample.weight == Approx(0.5));
    CHECK(sample.metric.m[0][0] == Approx(sqrt(100.0)));
  }

  /*--- On a wall point: distance 0, normal of the (straight) vertex, hn = h0. ---*/
  {
    const su2double x[2] = {1.0, 0.0};
    const auto sample = metric.Evaluate(0, x, core);
    CHECK(sample.distance == 0.0);
    CHECK(sample.hn == h0);
    CheckTensor(2, sample.full, Metric2D(up, 0.1, h0));
  }

  /*--- Beyond the end of the wall: radial around the end point (2, 0). ---*/
  {
    const su2double x[2] = {2.03, 0.04};
    const auto sample = metric.Evaluate(0, x, core);
    CHECK(sample.distance == Approx(0.05));
    CHECK(sample.normal[0] == Approx(0.6));
    CHECK(sample.normal[1] == Approx(0.8));
    const su2double n[2] = {0.6, 0.8};
    CheckTensor(2, sample.full, Metric2D(n, 0.1, NormalSize(h0, growth, 0.05)));
  }

  /*--- Edges of different lengths: the tangential size is the length of the closest edge. ---*/
  {
    CBoundaryLayerMetric uneven(2, {FlatWall2D({0.0, 0.1, 0.3, 0.35}, h0, growth, thickness)});
    const su2double a[2] = {0.2, 0.01}, b[2] = {0.05, 0.01}, c[2] = {0.34, 0.02};
    CheckTensor(2, uneven.Evaluate(0, a, core).full, Metric2D(up, 0.2, NormalSize(h0, growth, 0.01)));
    CheckTensor(2, uneven.Evaluate(0, b, core).full, Metric2D(up, 0.1, NormalSize(h0, growth, 0.01)));
    CheckTensor(2, uneven.Evaluate(0, c, core).full, Metric2D(up, 0.05, NormalSize(h0, growth, 0.02)));
  }
}

TEST_CASE("Boundary-layer metric of a circle 2D", "[Adaptation]") {
  /*--- Regular polygon of 40 edges inscribed in the unit circle. Every vertex is a corner where the wall turns by
   *    2 pi / 40: cos of half the angle between its two edges = sin(pi / 40). ---*/
  const int N = 40;
  const su2double h0 = 1e-3, growth = 1.1, thickness = 0.2, core = 4.0;
  const su2double pi = PI_NUMBER, L = 2.0 * sin(pi / N), turn = sin(pi / N);
  CBoundaryLayerMetric::Wall wall;
  wall.layer = {"circle", h0, growth, thickness};
  for (int i = 0; i < N; ++i) {
    wall.coord.push_back(cos(2 * pi * i / N));
    wall.coord.push_back(sin(2 * pi * i / N));
    wall.conn.push_back(i);
    wall.conn.push_back((i + 1) % N);
  }
  CBoundaryLayerMetric metric(2, {wall});

  /*--- On the bisector of an edge, outside and inside: normal radial, tangential size the edge length (the corners
   *    are farther than half an edge). ---*/
  const su2double angle = pi / N, apothem = cos(pi / N);
  for (const su2double offset : {0.01, -0.01, 0.15}) {
    const su2double r = apothem + offset;
    const su2double x[2] = {r * cos(angle), r * sin(angle)};
    const auto sample = metric.Evaluate(0, x, core);
    CHECK(sample.distance == Approx(fabs(offset)));
    const su2double n[2] = {cos(angle), sin(angle)};
    CHECK(fabs(sample.normal[0] * n[0] + sample.normal[1] * n[1]) == Approx(1.0));
    const auto expected = Metric2D(n, L, NormalSize(h0, growth, fabs(offset)));
    CheckTensor(2, sample.full, expected, 1e-9);
    const su2double w = Weight(fabs(offset), h0, thickness);
    CHECK(sample.weight == Approx(w));
    if (w < 1.0) CheckTensor(2, sample.metric, Fade(2, expected, w, core), 1e-9);
  }

  /*--- At a vertex: the tangential size is limited to max(h0, 2 r / turn), r the distance to the vertex. ---*/
  for (const su2double r : {0.0, 1e-4, 2e-3, 0.03}) {
    const su2double x[2] = {1.0 + r, 0.0};
    const auto sample = metric.Evaluate(0, x, core);
    CHECK(sample.distance == Approx(r).margin(1e-15));
    CHECK(fabs(sample.normal[0]) == Approx(1.0));
    const su2double ht = (r <= 0.5 * L) ? min(L, max(h0, 2.0 * r / turn)) : L;
    const su2double n[2] = {1.0, 0.0};
    CheckTensor(2, sample.full, Metric2D(n, ht, NormalSize(h0, growth, r)), 1e-9);
  }
}

TEST_CASE("Boundary-layer metric at sharp corners 2D", "[Adaptation]") {
  /*--- With a corner angle of 45 degrees (ADAP_ANGLE), the vertices of the 40-gon (turn of 9 degrees) do not limit the
   *    tangential size: at a vertex it is the length of the edge. A wedge (trailing edge) of half angle 10 degrees,
   *    tip at the origin, edges of length 0.1: the tip turns by 160 degrees, cos of half the angle between its edges
   *    is cos(10 deg); around it the tangential size is max(h0, 2 r / cos(10 deg)). ---*/
  const int N = 40;
  const su2double h0 = 1e-3, growth = 1.1, thickness = 0.2, core = 4.0, pi = PI_NUMBER, L = 2.0 * sin(pi / N);
  CBoundaryLayerMetric::Wall circle;
  circle.layer = {"circle", h0, growth, thickness};
  for (int i = 0; i < N; ++i) {
    circle.coord.push_back(cos(2 * pi * i / N));
    circle.coord.push_back(sin(2 * pi * i / N));
    circle.conn.push_back(i);
    circle.conn.push_back((i + 1) % N);
  }
  CBoundaryLayerMetric smooth(2, {circle}, 45.0);
  const su2double n[2] = {1.0, 0.0};
  for (const su2double r : {0.0, 2e-3}) {
    const su2double x[2] = {1.0 + r, 0.0};
    CheckTensor(2, smooth.Evaluate(0, x, core).full, Metric2D(n, L, NormalSize(h0, growth, r)), 1e-9);
  }

  CBoundaryLayerMetric::Wall wedge;
  wedge.layer = {"wedge", h0, growth, thickness};
  const su2double half = 10.0 * pi / 180.0;
  for (int i = 2; i >= -2; --i) {
    const su2double s = 0.1 * abs(i);
    wedge.coord.push_back(-s * cos(half));
    wedge.coord.push_back((i > 0 ? 1.0 : -1.0) * s * sin(half));
  }
  for (unsigned long i = 0; i < 4; ++i) {
    wedge.conn.push_back(i);
    wedge.conn.push_back(i + 1);
  }
  CBoundaryLayerMetric sharp(2, {wedge}, 45.0);
  for (const su2double r : {0.0, 1e-4, 2e-3, 0.02, 0.06}) {
    const su2double x[2] = {r, 0.0};  // downstream of the tip, on its bisector
    const auto sample = sharp.Evaluate(0, x, core);
    CHECK(sample.distance == Approx(r).margin(1e-15));
    const su2double ht = (r <= 0.05) ? min(0.1, max(h0, 2.0 * r / cos(half))) : 0.1;
    CheckTensor(2, sample.full, Metric2D(n, ht, NormalSize(h0, growth, r)), 1e-9);
  }
}

TEST_CASE("Boundary-layer metric of a flat wall 3D", "[Adaptation]") {
  /*--- Wall z = 0, [0,1]^2 with 4 cells per side, each cell split along its (1,1) diagonal into two right isosceles
   *    triangles of legs L. Size tensor of every face: 2/3 (L^2 ex ex + L^2 ey ey + 2 L^2 d d), d = (1,1)/sqrt(2):
   *    (2/3) L^2 [[2,1],[1,2]] on the plane, the same at every vertex, so the metric on the plane is its inverse,
   *    (1 / (2 L^2)) [[2,-1],[-1,2]], everywhere. ---*/
  const int m = 4;
  const su2double L = 1.0 / m, h0 = 1e-3, growth = 1.2, thickness = 0.1, core = 1.0;
  CBoundaryLayerMetric::Wall wall;
  wall.layer = {"floor", h0, growth, thickness};
  auto index = [&](int i, int j) { return static_cast<unsigned long>(i + (m + 1) * j); };
  for (int j = 0; j <= m; ++j)
    for (int i = 0; i <= m; ++i) {
      wall.coord.push_back(i * L);
      wall.coord.push_back(j * L);
      wall.coord.push_back(0.0);
    }
  for (int j = 0; j < m; ++j)
    for (int i = 0; i < m; ++i) {
      const unsigned long t1[4] = {index(i, j), index(i + 1, j), index(i + 1, j + 1), index(i + 1, j + 1)};
      const unsigned long t2[4] = {index(i, j), index(i + 1, j + 1), index(i, j + 1), index(i, j + 1)};
      wall.conn.insert(wall.conn.end(), t1, t1 + 4);
      wall.conn.insert(wall.conn.end(), t2, t2 + 4);
    }
  CBoundaryLayerMetric metric(3, {wall});

  auto expected = [&](su2double hn) {
    Tensor M;
    const su2double f = 1.0 / (2 * L * L);
    M.m[0][0] = M.m[1][1] = 2 * f;
    M.m[0][1] = M.m[1][0] = -f;
    M.m[2][2] = 1.0 / (hn * hn);
    return M;
  };

  for (const su2double z : {0.0, 2e-4, 0.02, 0.08, 0.095}) {
    for (const auto& xy : {std::array<su2double, 2>{0.4, 0.45}, std::array<su2double, 2>{0.5, 0.5},
                           std::array<su2double, 2>{0.0, 0.3}, std::array<su2double, 2>{1.0, 1.0}}) {
      const su2double x[3] = {xy[0], xy[1], z};
      const auto sample = metric.Evaluate(0, x, core);
      CHECK(sample.distance == Approx(z).margin(1e-15));
      CHECK(fabs(sample.normal[2]) == Approx(1.0));
      CHECK(sample.hn == Approx(NormalSize(h0, growth, z)));
      CheckTensor(3, sample.full, expected(NormalSize(h0, growth, z)), 1e-9);
      const su2double w = Weight(z, h0, thickness);
      CHECK(sample.weight == Approx(w));
      if (w < 1.0) CheckTensor(3, sample.metric, Fade(3, expected(NormalSize(h0, growth, z)), w, core), 1e-9);
    }
  }

  /*--- Off the side of the wall: normal towards the closest point (1, 0.5, 0) of the edge x = 1; the tangential
   *    tensor is rotated with it. ---*/
  {
    const su2double x[3] = {1.03, 0.5, 0.04};
    const auto sample = metric.Evaluate(0, x, core);
    CHECK(sample.distance == Approx(0.05));
    CHECK(sample.normal[0] == Approx(0.6));
    CHECK(sample.normal[2] == Approx(0.8));
    su2double n[3] = {0.6, 0.0, 0.8};
    su2double Mn[3] = {0.0};
    for (int i = 0; i < 3; ++i)
      for (int j = 0; j < 3; ++j) Mn[i] += sample.full.m[i][j] * n[j];
    for (int i = 0; i < 3; ++i)
      CHECK(Mn[i] == Approx(n[i] / pow(NormalSize(h0, growth, 0.05), 2)).margin(1e-12 / pow(NormalSize(h0, growth, 0.05), 2)));
  }
}

TEST_CASE("Boundary-layer metric intersected with the adaptation metric 2D", "[Adaptation]") {
  /*--- Rectangle [0,2]x[0,1], 16 cells per unit length (wall edges L = 1/16), wall at y = 0. Hessian diag(400, 1):
   *    the sensor metric is diag(lx, ly) with lx / ly = 400 and complexity 200, i.e. ly = 5, lx = 2000 (size 0.022,
   *    finer than L in x). h0 2e-3, growth 1.2, thickness 0.2. Near the wall the result is diag(lx, 1/hn^2): the
   *    finer direction of each metric is kept; on the wall points (within max(h0, 0.05 L) of the wall) the tangential
   *    floor gives diag(1/L^2, 1/h0^2). ---*/
  auto markerOf = [](const passivedouble* x) { return std::string(x[1] < 1e-12 ? "wall" : "far"); };
  const auto mesh = simplex_test::MakeSimplexMesh(2, 16, markerOf);
  const string options =
      "MARKER_HEATFLUX= (wall, 0.0)\nMARKER_FAR= (far)\nADAP_COMPLEXITY= 200\nADAP_HMIN= 1e-4\nADAP_HMAX= 10\n"
      "ADAP_ARMAX= 1e6\nADAP_BL_MARKER= (wall)\nADAP_BL_FIRST_HEIGHT= (2e-3)\nADAP_BL_GROWTH= (1.2)\n"
      "ADAP_BL_THICKNESS= (0.2)\n";
  const std::array<su2double, 6> hessian = {400.0, 0.0, 1.0, 0.0, 0.0, 0.0};
  MetricTest test(mesh, options, hessian);
  const auto output = test.ComputeMetric();
  CHECK(output.find("Boundary-layer metric wall: first height 0.002") != string::npos);
  CHECK(output.find("Mesh complexity with the boundary-layer metric") != string::npos);
  CHECK(output.find("WARNING: The boundary-layer metric") == string::npos);

  const su2double lx = 2000.0, ly = 5.0, h0 = 2e-3, growth = 1.2, thickness = 0.2, L = 1.0 / 16;
  unsigned long nWall = 0, nFade = 0, nOut = 0, nFloor = 0;
  for (auto iPoint = 0ul; iPoint < test.Geometry().GetnPointDomain(); ++iPoint) {
    const su2double y = test.Geometry().nodes->GetCoord(iPoint, 1);
    const auto M = test.Metric(iPoint);
    CHECK(fabs(M.m[0][1]) <= 1e-9 * lx);
    const su2double w = Weight(y, h0, thickness);
    /*--- Next to the wall (up to max(h0, 0.05 L)) the tangential size is at least the wall edge length L. ---*/
    const bool floor = y <= max(h0, CBoundaryLayerMetric::floorFraction * L);
    CHECK(M.m[0][0] == Approx(floor ? 1.0 / (L * L) : lx).epsilon(1e-9));
    nFloor += floor;
    su2double expected = ly;
    if (w == 1.0) {
      expected = 1.0 / pow(NormalSize(h0, growth, y), 2);
      ++nWall;
    } else if (w > 0.0) {
      expected = max(ly, pow(1.0 / pow(NormalSize(h0, growth, y), 2), w) * pow(ly, 1.0 - w));
      ++nFade;
    } else {
      ++nOut;
    }
    CHECK(M.m[1][1] == Approx(expected).epsilon(1e-9));
  }
  CHECK(nWall > 0);
  CHECK(nFade > 0);
  CHECK(nOut > 0);
  CHECK(nFloor == 33);
  CHECK(output.find("tangential floor next to the wall on 33") != string::npos);

  /*--- The wall metric has aspect ratio L / h0 = 31 at the wall: above ADAP_ARMAX= 10, with a warning. ---*/
  MetricTest low(mesh, "MARKER_HEATFLUX= (wall, 0.0)\nMARKER_FAR= (far)\nADAP_COMPLEXITY= 200\nADAP_HMIN= 1e-4\n"
                       "ADAP_HMAX= 10\nADAP_ARMAX= 10\nADAP_BL_MARKER= (wall)\nADAP_BL_FIRST_HEIGHT= (2e-3)\n"
                       "ADAP_BL_GROWTH= (1.2)\nADAP_BL_THICKNESS= (0.2)\n", hessian);
  CHECK(low.ComputeMetric().find("WARNING: The boundary-layer metric of wall has aspect ratios up to") != string::npos);
}

TEST_CASE("Boundary-layer metric of a sphere 3D", "[Adaptation]") {
  /*--- Ball of radius 1 (8 cells per side of the mapped cube), the sphere is two no-slip walls round_a (x < 0) and
   *    round_b (x > 0) with the same layer. Isotropic sensor metric of complexity 20 (size about 0.59, coarser than
   *    the wall metric everywhere in the layer): there the result is exactly the wall metric of the nearest wall,
   *    elsewhere the sensor metric. Checked against a brute-force closest point of the boundary triangles, away from
   *    the junction of the two walls. ---*/
  const auto mesh = simplex_test::MakeRoundMesh(3, 8, 1.0);
  const string options =
      "MARKER_HEATFLUX= (round_a, 0.0, round_b, 0.0)\nADAP_COMPLEXITY= 20\nADAP_HMIN= 1e-4\nADAP_HMAX= 10\n"
      "ADAP_ARMAX= 1e6\nADAP_BL_MARKER= (round_b, round_a)\nADAP_BL_FIRST_HEIGHT= (0.01)\nADAP_BL_GROWTH= (1.3)\n"
      "ADAP_BL_THICKNESS= (0.3)\n";
  const std::array<su2double, 6> hessian = {1.0, 0.0, 0.0, 1.0, 0.0, 1.0};  // xx, xy, xz, yy, yz, zz
  MetricTest test(mesh, options, hessian);
  test.ComputeMetric();

  /*--- Boundary triangles of the mesh. ---*/
  std::vector<std::array<std::array<su2double, 3>, 3>> faces;
  for (const auto& marker : mesh.markers)
    for (auto iFace = 0ul; iFace < marker.elem.size() / 3; ++iFace) {
      std::array<std::array<su2double, 3>, 3> face;
      for (int k = 0; k < 3; ++k)
        for (int iDim = 0; iDim < 3; ++iDim) face[k][iDim] = mesh.coord[marker.elem[3 * iFace + k] * 3 + iDim];
      faces.push_back(face);
    }

  /*--- Closest point of a triangle: projection on its plane if it falls inside, else the closest point of its edges
   *    (independent of the Voronoi-region code of the class). ---*/
  auto closest = [](const std::array<std::array<su2double, 3>, 3>& t, const su2double* p, su2double* q) {
    auto dot = [](const su2double* a, const su2double* b) { return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]; };
    su2double e1[3], e2[3], w[3];
    for (int d = 0; d < 3; ++d) {
      e1[d] = t[1][d] - t[0][d];
      e2[d] = t[2][d] - t[0][d];
      w[d] = p[d] - t[0][d];
    }
    const su2double a = dot(e1, e1), b = dot(e1, e2), c = dot(e2, e2), r1 = dot(w, e1), r2 = dot(w, e2);
    const su2double det = a * c - b * b, u = (r1 * c - r2 * b) / det, v = (r2 * a - r1 * b) / det;
    if (u >= 0.0 && v >= 0.0 && u + v <= 1.0) {
      for (int d = 0; d < 3; ++d) q[d] = t[0][d] + u * e1[d] + v * e2[d];
    } else {
      su2double best = 1e300;
      for (int k = 0; k < 3; ++k) {
        const auto& A = t[k];
        const auto& B = t[(k + 1) % 3];
        su2double ab[3], ap[3];
        for (int d = 0; d < 3; ++d) {
          ab[d] = B[d] - A[d];
          ap[d] = p[d] - A[d];
        }
        const su2double s = min(max(dot(ap, ab) / dot(ab, ab), su2double(0.0)), su2double(1.0));
        su2double c2[3], dist = 0.0;
        for (int d = 0; d < 3; ++d) {
          c2[d] = A[d] + s * ab[d];
          dist += pow(p[d] - c2[d], 2);
        }
        if (dist < best) {
          best = dist;
          for (int d = 0; d < 3; ++d) q[d] = c2[d];
        }
      }
    }
    return sqrt(pow(p[0] - q[0], 2) + pow(p[1] - q[1], 2) + pow(p[2] - q[2], 2));
  };

  /*--- Sensor metric: the metric at the centre of the ball. ---*/
  unsigned long centre = 0;
  for (auto iPoint = 0ul; iPoint < test.Geometry().GetnPointDomain(); ++iPoint) {
    if (GeometryToolbox::Norm(3, test.Geometry().nodes->GetCoord(iPoint)) <
        GeometryToolbox::Norm(3, test.Geometry().nodes->GetCoord(centre)))
      centre = iPoint;
  }
  const su2double sensor = test.Metric(centre).m[0][0];
  CHECK(sensor < 4.0);

  const su2double h0 = 0.01, growth = 1.3, thickness = 0.3;
  unsigned long nLayer = 0, nOut = 0;
  for (auto iPoint = 0ul; iPoint < test.Geometry().GetnPointDomain(); ++iPoint) {
    const auto* x = test.Geometry().nodes->GetCoord(iPoint);
    const su2double r = sqrt(x[0] * x[0] + x[1] * x[1] + x[2] * x[2]);
    const auto M = test.Metric(iPoint);
    if (r < 1.0 - 0.32) {
      /*--- Beyond the thickness (the polyhedron is within 0.02 of the sphere here): the sensor metric. ---*/
      ++nOut;
      for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j) CHECK(M.m[i][j] == Approx(i == j ? sensor : 0.0).margin(1e-9 * sensor));
      continue;
    }
    if (fabs(x[0]) < 0.35 || r > 1.0 - 1e-6) continue;  // junction of the two walls, or a wall point

    su2double best = 1e300, q[3] = {0.0};
    std::vector<std::pair<su2double, std::array<su2double, 3>>> candidates;
    for (const auto& face : faces) {
      su2double c[3];
      const su2double d = closest(face, x, c);
      candidates.push_back({d, {c[0], c[1], c[2]}});
      if (d < best) {
        best = d;
        for (int k = 0; k < 3; ++k) q[k] = c[k];
      }
    }
    /*--- Skip points with two different closest points (on the medial axis of the concave wall, the normal is not
     *    unique). ---*/
    bool tie = false;
    for (const auto& c : candidates)
      tie |= c.first < best + 1e-9 && GeometryToolbox::Distance(3, c.second.data(), q) > 1e-6;
    if (tie) continue;
    if (Weight(best, h0, thickness) < 1.0) continue;
    ++nLayer;

    /*--- The normal (towards the closest point) is an eigenvector with the size hn of the distance. ---*/
    su2double n[3], Mn[3] = {0.0};
    for (int k = 0; k < 3; ++k) n[k] = (x[k] - q[k]) / best;
    for (int i = 0; i < 3; ++i)
      for (int j = 0; j < 3; ++j) Mn[i] += M.m[i][j] * n[j];
    const su2double expected = 1.0 / pow(NormalSize(h0, growth, best), 2);
    for (int i = 0; i < 3; ++i) CHECK(Mn[i] == Approx(expected * n[i]).margin(1e-7 * expected));
  }
  CHECK(nLayer > 20);
  CHECK(nOut > 20);
}

TEST_CASE("Boundary layer point queries share the batch intersection and floor", "[BoundaryLayerMetric][NativeComposite2D]") {
  CBoundaryLayerMetric layers(2, {FlatWall2D({0., 1., 2.}, 1e-5, 1.2, .02)}, 45);
  std::vector<su2double> coord{.5, 0, .5, .001, .5, .019, .5, .1};
  std::vector<Tensor> batch(4), points(4);
  for (auto* field : {&batch, &points})
    for (auto& m : *field) {m.m[0][0]=100; m.m[0][1]=m.m[1][0]=1; m.m[1][1]=400;}
  layers.Apply(coord, batch, 1);
  for (size_t k=0; k<points.size(); ++k) {
    layers.ApplyPoint(&coord[2*k], points[k], 1);
    CheckTensor(2, points[k], batch[k]);
  }
  CHECK(points[0].m[1][1] == Approx(1e10));
  CHECK(points[1].m[1][1] < 1e8);
  CHECK(points[3].m[1][1] == 400);
}

TEST_CASE("Native BL query preserves sensor intersection across the legacy floor switch", "[BoundaryLayerMetric][NativeComposite2D]") {
  const double h0=1e-5,growth=1.2,L=.01,band=CBoundaryLayerMetric::floorFraction*L;
  CBoundaryLayerMetric layers(2,{FlatWall2D({0,L},h0,growth,.02)});
  auto at=[&](double distance,bool floor) {
    const su2double coord[2]={L*.5,distance};Tensor m;m.m[0][0]=1e8;m.m[1][1]=1;
    layers.ApplyPoint(coord,m,1,nullptr,floor);return m;
  };
  const double below=band-1e-12,above=band+1e-12;
  const auto legacyBelow=at(below,true),legacyAbove=at(above,true);
  // A smooth sensor and flat wall previously gave a 10,000-fold jump.
  CHECK(legacyAbove.m[0][0]>legacyBelow.m[0][0]*1000);
  const auto nativeBelow=at(below,false),nativeAbove=at(above,false);
  for (const auto d:{below,above}) {
    const auto m=at(d,false);
    CHECK(m.m[0][0]==Approx(1e8));
    CHECK(m.m[1][1]==Approx(1/std::pow(NormalSize(h0,growth,d),2)));
    CHECK(m.m[0][0]>=1/(L*L));CHECK(m.m[1][1]>=1);
    CHECK(m.m[0][0]*m.m[1][1]-m.m[0][1]*m.m[0][1]>0);
  }
  CHECK(nativeAbove.m[0][0]==Approx(nativeBelow.m[0][0]).epsilon(1e-8));
  CHECK(nativeAbove.m[1][1]==Approx(nativeBelow.m[1][1]).epsilon(1e-8));
}

TEST_CASE("Native cached geometric composition preserves finer sensors and the point fade", "[MetricRobustness][NativeComposite2D]") {
  CBoundaryLayerMetric layers(2, {FlatWall2D({0.,1.,2.}, 1e-5, 1.2, .02)}, 45);
  const std::vector<su2double> coord{.5,0., .5,.001, .5,.019, .5,.03};
  Tensor sensor; sensor.m[0][0]=1e8; sensor.m[1][1]=1e12; sensor.m[0][1]=sensor.m[1][0]=1e8;
  const auto samples=layers.SamplePoints(coord);
  std::vector<Tensor> cached(coord.size()/2,sensor);
  for (const auto& item:samples) layers.ApplySample(item.wall,item.sample,cached[item.point],1.,nullptr,false);
  for (unsigned k=0;k<cached.size();++k) {
    Tensor actual=sensor;
    layers.ApplyPoint(&coord[2*k],actual,1.,nullptr,false);
    CheckTensor(2,cached[k],actual);
    su2double delta[3][3]={}, vec[3][3], val[3], work[3];
    for(unsigned i=0;i<2;++i) for(unsigned j=0;j<2;++j) delta[i][j]=actual.m[i][j]-sensor.m[i][j];
    CBlasStructure::EigenDecomposition(delta,vec,val,2,work);
    CHECK(std::min(val[0],val[1]) >= -1e-3);
    CHECK(actual.m[1][1] >= sensor.m[1][1]*(1-1e-12));
  }
}

TEST_CASE("Geometric complexity resolves a microscopic BL inside a coarse donor triangle", "[MetricRobustness][NativeComposite2D]") {
  const double h0=1e-6, thickness=.01;
  CBoundaryLayerMetric layers(2,{FlatWall2D({0.,1.,2.},h0,1.,thickness)},45);
  const std::array<std::array<double,2>,3> cell{{{{.2,0.}},{{.8,0.}},{{.5,1.}}}};
  Tensor sensor; sensor.m[0][0]=sensor.m[1][1]=1;
  auto integrate=[&](double ratio) {
    double integral=0,area=0;
    const auto rule=layers.IntegrationRule2D(cell,ratio);
    REQUIRE(rule.size()<2000);
    for(const auto& point:rule) {
      Tensor m=sensor;
      for(const auto& sample:point.samples) layers.ApplySample(sample.wall,sample.sample,m,1.,nullptr,false);
      integral+=point.weight*sqrt(m.m[0][0]*m.m[1][1]-m.m[0][1]*m.m[0][1]);
      area+=point.weight;
    }
    CHECK(area==Approx(.3).epsilon(1e-12));
    return integral;
  };
  // Independent one-dimensional Simpson integral: triangle width .6*(1-y).
  double reference=.3*(1-thickness)*(1-thickness);
  const unsigned n=10000;
  for(unsigned k=0;k<=n;++k) {
    const double y=thickness*k/n, t=std::max(0.,std::min(1.,(y-.9*thickness)/(.1*thickness)));
    const double weight=1-t*t*(3-2*t), rho=pow(h0,-weight);
    reference+=thickness/n/3*(k==0||k==n?1:(k%2?4:2))*.6*(1-y)*rho;
  }
  CHECK(integrate(1.5)==Approx(reference).epsilon(5e-4));
  CHECK(integrate(1.25)==Approx(reference).epsilon(5e-4));
  Tensor center=sensor;const su2double x[2]={.5,1./3};layers.ApplyPoint(x,center,1.,nullptr,false);
  CHECK(.3*sqrt(center.m[0][0]*center.m[1][1]) < reference/100);
}

TEST_CASE("Geometric BL quadrature resolves a far vertex closest to another face", "[MetricRobustness][NativeComposite2D]") {
  CBoundaryLayerMetric::Wall wall;
  wall.layer={"wall",1e-4,1.2,.01};wall.coord={0,0,1,0,1,1,0,1};wall.conn={0,1,1,2,2,3,3,0};
  CBoundaryLayerMetric layers(2,{wall},45);
  const std::array<std::array<double,2>,3> cell{{{{.2,0.}},{{.8,0.}},{{.2,.4}}}};
  // The top vertex is closest to the left wall, although the thin active band is at the bottom.
  // Distance interpolation without resolving that switch puts fade bands at the wrong physical height.
  auto density=[](double y) {
    const double hn=std::max(1e-4,2*(1e-4+.2*y)/2.2);
    const double t=std::clamp((y-.009)/.001,0.,1.),weight=1-t*t*(3-2*t);
    const double normal=std::exp(weight*std::log(1/(hn*hn))+(1-weight)*std::log(4.));
    return .6*(1-y/.4)*2*std::sqrt(std::max(9.,normal));
  };
  double reference=0;
  const double intervals[]={0.,.5e-4,.009,.01,.4};
  const unsigned n=10000;
  for(unsigned band=1;band<5;++band) {
    const double a=intervals[band-1],step=(intervals[band]-a)/n;
    for(unsigned k=0;k<=n;++k) reference+=step/3*(k==0||k==n?1:(k%2?4:2))*density(a+k*step);
  }
  for(const double ratio:{1.5,1.25}) {
    double total=0,area=0;
    const auto rule=layers.IntegrationRule2D(cell,ratio);
    for(const auto& point:rule) {
      Tensor m;m.m[0][0]=4;m.m[1][1]=9;
      for(const auto& sample:point.samples) layers.ApplySample(sample.wall,sample.sample,m,4.,nullptr,false);
      total+=point.weight*std::sqrt(m.m[0][0]*m.m[1][1]-m.m[0][1]*m.m[0][1]);area+=point.weight;
    }
    CHECK(area==Approx(.12).epsilon(1e-12));
    CHECK(total==Approx(reference).epsilon(5e-4));
  }
}

TEST_CASE("Geometric integration detects a curved BL missed by every donor vertex", "[MetricRobustness][NativeComposite2D]") {
  CBoundaryLayerMetric::Wall wall;wall.layer={"wall",1e-5,1.2,.01};
  const unsigned count=40;
  for(unsigned k=0;k<count;++k) {
    const double angle=2*acos(-1.)*k/count;
    wall.coord.push_back(cos(angle));wall.coord.push_back(sin(angle));
    wall.conn.push_back(k);wall.conn.push_back((k+1)%count);
  }
  CBoundaryLayerMetric layers(2,{wall},45);
  const std::array<std::array<double,2>,3> cell{{{{-.2,1.000001}},{{.2,1.000001}},{{0.,1.2}}}};
  for(const auto& p:cell) {const su2double x[2]={p[0],p[1]};CHECK(layers.Evaluate(0,x,1.).weight==0);}
  double area=0,integral=0;
  for(const auto& point:layers.IntegrationRule2D(cell)) {
    Tensor m;m.m[0][0]=m.m[1][1]=1;
    for(const auto& sample:point.samples)layers.ApplySample(sample.wall,sample.sample,m,1.,nullptr,false);
    area+=point.weight;integral+=point.weight*sqrt(m.m[0][0]*m.m[1][1]-m.m[0][1]*m.m[0][1]);
  }
  CHECK(area==Approx(.2*.199999).epsilon(1e-10));
  CHECK(integral>2*area);
}

TEST_CASE("Symmetric eigen decomposition handles subnormal off-diagonal scales", "[LinearAlgebra][MetricRobustness]") {
  for (volatile unsigned short dimension = 2; dimension <= 3; ++dimension) {
    const auto dim = dimension; // Keep the shared kernel's runtime dimension, as in CSolver.
    for (const auto offDiagonal : {1e-300, 1e-310, 1e-320}) {
      su2double matrix[3][3] = {{.005,offDiagonal,offDiagonal},
                              {offDiagonal,.006,offDiagonal},{offDiagonal,offDiagonal,.007}};
      su2double vec[3][3], val[3], work[3], recovered[3][3];
      CBlasStructure::EigenDecomposition(matrix, vec, val, dim, work);
      CBlasStructure::EigenRecomposition(recovered, vec, val, dim);
      for (unsigned i = 0; i < dim; ++i) {
        REQUIRE(std::isfinite(val[i]));
        CHECK(val[i] > 0);
        for (unsigned j = 0; j < dim; ++j) {
          REQUIRE(std::isfinite(vec[i][j]));
          CHECK(recovered[i][j] == Approx(matrix[i][j]).margin(1e-16));
        }
      }
    }
  }
}

TEST_CASE("Native full tensor gradation includes complexity and crosses MPI partitions in 2D and 3D", "[MetricRobustness]") {
  for (const unsigned short dim : {2, 3}) {
    INFO("Dimension " << dim);
    const auto mesh = simplex_test::MakeSimplexMesh(dim, dim == 2 ? 12 : 6, [](const passivedouble*) { return std::string("far"); });
    MetricTest test(mesh, "MARKER_FAR= (far)\nADAP_REMESHER= NATIVE_CAVITY\nADAP_HGRAD= 1.3\n"
                          "ADAP_HMIN= .002\nADAP_HMAX= 1\nADAP_ARMAX= 100\n"
                          "ADAP_COMPLEXITY= 300\nADAP_ISO_CORNER= NO\n", {1,0,1,0,0,0});
    auto& H = test.solver[FLOW_SOL]->GetNodes()->GetHessian();
    for (auto point = 0ul; point < test.Geometry().GetnPoint(); ++point) {
      const auto* x = test.Geometry().nodes->GetCoord(point);
      su2double radius2 = 0;
      for (unsigned i = 0; i < dim; ++i) radius2 += (x[i]-.5)*(x[i]-.5);
      const auto bump = 200*exp(-radius2/.0025);
      const su2double coupling[3][3] = {{1,.9,.2},{.9,1,.4},{.2,.4,1}};
      for (unsigned i = 0, k = 0; i < dim; ++i)
        for (unsigned j = i; j < dim; ++j, ++k) H(point,0,k) = (i == j ? 1 : 0) + bump*coupling[i][j];
    }
    const auto output = test.ComputeMetric();
    INFO(output);
    REQUIRE(std::isfinite(test.solver[FLOW_SOL]->GetMetricComplexityFinal()));
    CHECK(test.solver[FLOW_SOL]->GetMetricComplexityFinal() == Approx(300).epsilon(2e-6));
    CHECK(test.solver[FLOW_SOL]->GetMetricComplexityBracketed());
    if (SU2_MPI::GetRank() == MASTER_NODE) CHECK(output.find("iteration limit reached") == string::npos);
    for (auto point = 0ul; point < test.Geometry().GetnPointDomain(); ++point) {
      const auto M = test.Metric(point);
      su2double vec[3][3], val[3], work[3];
      CBlasStructure::EigenDecomposition(M.m, vec, val, dim, work);
      for (unsigned i = 0; i < dim; ++i) CHECK(val[i] > 0);
      for (const auto neighbor : test.Geometry().nodes->GetPoints(point)) {
        const auto other = test.Metric(neighbor);
        const auto* x = test.Geometry().nodes->GetCoord(point);
        const auto* y = test.Geometry().nodes->GetCoord(neighbor);
        su2double dx[3] = {};
        for (unsigned i = 0; i < dim; ++i) dx[i] = x[i]-y[i];
        su2double distance2 = 0, B[3][3] = {};
        for (unsigned i = 0; i < dim; ++i)
          for (unsigned j = 0; j < dim; ++j) distance2 += dx[i]*other.m[i][j]*dx[j];
        const auto factor = 1/pow(1+log(1.3)*sqrt(distance2),2);
        for (unsigned i = 0; i < dim; ++i)
          for (unsigned j = 0; j < dim; ++j)
            for (unsigned a = 0; a < dim; ++a)
              for (unsigned b = 0; b < dim; ++b)
                B[i][j] += vec[a][i]*factor*other.m[a][b]*vec[b][j]/sqrt(val[i]*val[j]);
        su2double Q[3][3], lambda[3];
        CBlasStructure::EigenDecomposition(B, Q, lambda, dim, work);
        CHECK(*std::max_element(lambda, lambda+dim) <= 1+2e-5);
      }
    }
  }
}

TEST_CASE("Native overlapping walls intersect without overwriting a finer demand", "[MetricRobustness][NativeComposite2D]") {
  auto bottom=FlatWall2D({0.,1.,2.},1e-3,1.2,.1), top=bottom;
  bottom.layer.marker="a";top.layer.marker="z";top.layer.firstHeight=2e-3;
  for(size_t k=1;k<top.coord.size();k+=2) top.coord[k]=.03;
  CBoundaryLayerMetric layers(2,{bottom,top});
  Tensor sensor;sensor.m[0][0]=4;sensor.m[1][1]=1e8;
  for(const auto y:{0.,.015,.03}) {
    auto combined=sensor;const su2double x[2]={.5,y};
    layers.ApplyPoint(x,combined,1.,nullptr,false);
    CHECK(combined.m[1][1]>=1e8*(1-1e-12));
  }
}

TEST_CASE("Native BL spatial exchange preserves closest-face normals", "[MetricRobustness]") {
  auto mesh = simplex_test::MakeSimplexMesh(2, 12,
      [](const passivedouble* x) { return std::string(x[1] < 1e-12 ? "wall" : "far"); },
      [](const passivedouble* x) { return x[0] < .75 || x[0] > 1.25; });
  for (size_t k = 0; k < mesh.coord.size(); k += 2) if (mesh.coord[k] > 1) mesh.coord[k] += 100;
  const string options = "MARKER_HEATFLUX= (wall,0)\nMARKER_FAR= (far)\nADAP_HMIN= 1e-4\nADAP_HMAX= 10\n"
                         "ADAP_BL_MARKER= (wall)\nADAP_BL_FIRST_HEIGHT= (.001)\n"
                         "ADAP_BL_GROWTH= (1.2)\nADAP_BL_THICKNESS= (.15)\n";
  MetricTest native(mesh, options+"ADAP_REMESHER= NATIVE_CAVITY\n", {1,0,1,0,0,0});
  MetricTest legacy(mesh, options+"ADAP_REMESHER= MMG\n", {1,0,1,0,0,0});
  CBoundaryLayerMetric local(native.Geometry(), *native.config), global(legacy.Geometry(), *legacy.config);
  for (auto point = 0ul; point < native.Geometry().GetnPointDomain(); ++point) {
    const auto* x = native.Geometry().nodes->GetCoord(point);
    if (x[1] > .1) continue;
    const auto a = local.Evaluate(0,x,1), b = global.Evaluate(0,x,1);
    CHECK(a.weight == Approx(b.weight));
    CHECK(a.distance == Approx(b.distance).margin(1e-12));
    CheckTensor(2, a.full, b.full, 1e-9);
  }
}

TEST_CASE("Native complexity retains the correct bounded tensor for infeasible targets", "[MetricRobustness]") {
  for (const unsigned short dim : {2, 3}) {
    const auto mesh = simplex_test::MakeSimplexMesh(dim, 4, [](const passivedouble*) { return std::string("far"); });
    for (const bool upper : {false, true}) {
      INFO("Dimension " << dim << ", upper endpoint " << upper);
      const auto target = upper ? "1000000" : "1";
      const std::array<su2double,6> identity = dim == 2 ? std::array<su2double,6>{1,0,1,0,0,0}
                                                       : std::array<su2double,6>{1,0,0,1,0,1};
      MetricTest test(mesh, string("MARKER_FAR= (far)\nADAP_REMESHER= NATIVE_CAVITY\nADAP_HGRAD= 1.3\n") +
                          "ADAP_HMIN= .05\nADAP_HMAX= .5\nADAP_ISO_CORNER= NO\nADAP_COMPLEXITY= " + target + "\n", identity);
      const auto output = test.ComputeMetric();
      INFO(output);
      CHECK_FALSE(test.solver[FLOW_SOL]->GetMetricComplexityBracketed());
      su2double localVolume = 0, volume = 0;
      for (auto point = 0ul; point < test.Geometry().GetnPointDomain(); ++point)
        localVolume += test.Geometry().nodes->GetVolume(point);
      SU2_MPI::Allreduce(&localVolume, &volume, 1, MPI_DOUBLE, MPI_SUM, SU2_MPI::GetComm());
      const auto eigenvalue = upper ? 400. : 4.;
      CHECK(test.solver[FLOW_SOL]->GetMetricComplexityFinal() == Approx(volume*pow(eigenvalue,dim/2.)).epsilon(2e-6));
      for (auto point = 0ul; point < test.Geometry().GetnPoint(); ++point) {
        const auto M = test.Metric(point);
        for (unsigned i = 0; i < dim; ++i)
          for (unsigned j = 0; j < dim; ++j) CHECK(M.m[i][j] == Approx(i == j ? eigenvalue : 0.).margin(1e-9));
      }
    }
  }
}

TEST_CASE("Native sparse gradation reaches a remote demand beyond eighty rounds", "[MetricRobustness]") {
  constexpr unsigned n = 48;
  const auto mesh = simplex_test::MakeSimplexMesh(2, n, [](const passivedouble*) { return std::string("far"); });
  MetricTest test(mesh, "MARKER_FAR= (far)\nADAP_REMESHER= NATIVE_CAVITY\nADAP_HGRAD= 1.01\n"
                        "ADAP_HMIN= 1e-4\nADAP_HMAX= 10\nADAP_ARMAX= 100\n"
                        "ADAP_COMPLEXITY= 300\nADAP_ISO_CORNER= NO\n", {1,0,1,0,0,0});
  auto& H = test.solver[FLOW_SOL]->GetNodes()->GetHessian();
  const auto seedX = 4./n;
  for (auto point = 0ul; point < test.Geometry().GetnPoint(); ++point) {
    const auto* x = test.Geometry().nodes->GetCoord(point);
    if (fabs(x[0]-seedX)<1e-12 && fabs(x[1]-.5)<1e-12) H(point,0,0)=H(point,0,2)=10000;
  }
  const auto output = test.ComputeMetric();
  INFO(output);
  CHECK(test.solver[FLOW_SOL]->GetMetricComplexityFinal() == Approx(300).epsilon(2e-6));
  CHECK(test.solver[FLOW_SOL]->GetMetricComplexityBracketed());
  if (SU2_MPI::GetRank() == MASTER_NODE) {
    const auto position = output.find("Native sensor gradation: ");
    REQUIRE(position != string::npos);
    CHECK(std::stoul(output.substr(position+string("Native sensor gradation: ").size())) > 80);
    CHECK(output.find("fixed point reached") != string::npos);
    CHECK(output.find("iteration limit reached") == string::npos);
  }
  su2double localSeedSize = 0, seedSize = 0;
  for (auto point = 0ul; point < test.Geometry().GetnPointDomain(); ++point) {
    const auto* x = test.Geometry().nodes->GetCoord(point);
    if (fabs(x[0]-seedX)<1e-12 && fabs(x[1]-.5)<1e-12) localSeedSize = 1/sqrt(test.Metric(point).m[0][0]);
  }
  SU2_MPI::Allreduce(&localSeedSize, &seedSize, 1, MPI_DOUBLE, MPI_MAX, SU2_MPI::GetComm());
  REQUIRE(seedSize>0);
  // On the horizontal row, the graph shortest path is exactly its Euclidean distance from the unique fine seed.
  for (auto point = 0ul; point < test.Geometry().GetnPoint(); ++point) {
    const auto* x = test.Geometry().nodes->GetCoord(point);
    if (fabs(x[1]-.5)<1e-12 && x[0]>=seedX) {
      const auto expected = seedSize+log(1.01)*(x[0]-seedX);
      CHECK(1/sqrt(test.Metric(point).m[0][0]) == Approx(expected).epsilon(5e-6));
    }
  }
}

TEST_CASE("Native 3D composed edges resolve a microscopic BL without spreading donor tensors",
          "[NativeComposite3D]") {
  using namespace SU2Native3D;
  const double h0=1e-6, thickness=.01;
  CBoundaryLayerMetric::Wall wall;
  wall.layer={"wall",h0,1.,thickness};
  wall.coord={-2,-2,0, 2,-2,0, 2,2,0, -2,2,0};
  wall.conn={0,1,2,2, 0,2,3,3};
  CBoundaryLayerMetric layers(3,{wall});
  DonorCell donor;
  donor.cell={0,{{{0,{0,0,0}},{1,{1,0,0}},{2,{0,1,0}},{3,{0,0,1}}}}};
  for(auto& m:donor.sensor) m={1,0,0,1,0,1};
  const auto compose=[&](Point p,SU2Native3D::Tensor sensor) {
    CBoundaryLayerMetric::Tensor m;
    m.m[0][0]=sensor.xx; m.m[1][1]=sensor.yy; m.m[2][2]=sensor.zz;
    const su2double x[3]={p.x,p.y,p.z};
    layers.ApplyPoint(x,m,1.,nullptr,false);
    return SU2Native3D::Tensor{SU2_TYPE::GetValue(m.m[0][0]),SU2_TYPE::GetValue(m.m[0][1]),SU2_TYPE::GetValue(m.m[0][2]),
        SU2_TYPE::GetValue(m.m[1][1]),SU2_TYPE::GetValue(m.m[1][2]),SU2_TYPE::GetValue(m.m[2][2])};
  };
  const auto bands=[&](Point a,Point b) {return layers.EdgeBreakpoints({a.x,a.y,a.z},{b.x,b.y,b.z});};
  const Point a{.125,.125,0},b{.125,.125,.6};
  FrozenField field({donor},compose,true,{},bands);
  // Independent one-dimensional reference, retaining the original smoothstep/log fade.
  double expected=.6-thickness+.9*thickness/h0;
  const unsigned n=10000;
  for(unsigned k=0;k<=n;++k) {
    const double t=double(k)/n, w=1-t*t*(3-2*t);
    expected+=.1*thickness/n/3*(k==0||k==n?1:(k%2?4:2))*std::pow(h0,-w);
  }
  for(const double z:{0.,.005,.009,.0091,.0095,.0099,.01}) {
    const su2double x[3]={.125,.125,z};
    const auto sample=layers.Evaluate(0,x,1.);
    const double t=std::clamp((z-.9*thickness)/(.1*thickness),0.,1.),w=1-t*t*(3-2*t);
    INFO("z=" << z << " weight=" << sample.weight << " fullzz=" << sample.full.m[2][2]);
    if(z<thickness) CHECK(SU2_TYPE::GetValue(sample.metric.m[2][2])==Approx(std::pow(h0,-2*w)).epsilon(1e-8));
    CHECK(compose({.125,.125,z},{1,0,0,1,0,1}).zz==Approx(std::pow(h0,-2*w)).epsilon(1e-8));
  }
  CHECK(double(field.TargetEdgeLength(a,b))==Approx(expected).epsilon(2e-9));
  CHECK(double(field.TargetEdgeLength(b,a))==Approx(expected).epsilon(2e-9));
  CHECK(double(field.SensorEdgeLength(a,b))==Approx(.6).epsilon(1e-14));
  const auto balanced=field.BalancedSplitPoint(a,b);
  CHECK(balanced.z<thickness);
  CHECK(double(field.TargetEdgeLength(a,balanced))==Approx(double(field.TargetEdgeLength(balanced,b))).epsilon(3e-8));
  CHECK(field.Query({.125,.125,.1}).sensor.zz==1);
  CHECK(field.Query({.125,.125,.1}).target.zz==Approx(1));
  CHECK(field.Statistics().compositions<10000);
  CHECK(field.Statistics().edge_compositions>0);
  CHECK(field.Statistics().edge_geometry_seconds>0);
  CHECK(field.Statistics().edge_composition_seconds>0);
  for(auto& m:donor.sensor) m.zz=1e14;
  FrozenField finer({donor},compose,false,{},bands);
  CHECK(double(finer.TargetEdgeLength(a,b))==Approx(.6e7).epsilon(1e-12));
  FrozenField missing({donor},compose);
  CHECK_THROWS(missing.TargetEdgeLength(a,b));
}

TEST_CASE("Geometric 3D edge bands detect walls missed by endpoints and midpoint", "[NativeComposite3D]") {
  CBoundaryLayerMetric::Wall wall;
  wall.layer={"wall",1e-6,1.2,1e-4};
  wall.coord={.29,.09,0, .31,.09,0, .30,.12,0};wall.conn={0,1,2,2};
  CBoundaryLayerMetric layers(3,{wall});
  const std::array<double,3> a{.1,.1,2e-6},b{.9,.1,2e-6};
  for(const double t:{0.,.5,1.}) {
    const su2double x[3]={(1-t)*a[0]+t*b[0],.1,2e-6};
    CHECK(layers.Evaluate(0,x,1.).weight==0);
  }
  const auto cuts=layers.EdgeBreakpoints(a,b);
  REQUIRE(cuts.size()>2);
  bool active=false;
  for(size_t k=1;k<cuts.size();++k) {
    const double t=.5*(cuts[k-1]+cuts[k]);
    const su2double x[3]={(1-t)*a[0]+t*b[0],.1,2e-6};
    active=active || layers.Evaluate(0,x,1.).weight>0;
  }
  CHECK(active);
  CHECK_THROWS(layers.EdgeBreakpoints(a,b,1.));
  CHECK_THROWS(layers.EdgeBreakpoints(a,a));
}

TEST_CASE("Native 3D wall normal size survives rotated extreme layer aspect ratios", "[NativeComposite3D]") {
  const double c=std::cos(.37),s=std::sin(.37);
  for(const double h0:{1e-3,1e-6,1e-8}) {
    CBoundaryLayerMetric::Wall wall;wall.layer={"wall",h0,1.,.01};
    // Rotate both the reference and actual query; the normal eigenvalue is independently known.
    for(const auto xy:std::vector<std::array<double,2>>{{-2,-2},{2,-2},{2,2},{-2,2}}) {
      wall.coord.push_back(c*xy[0]);wall.coord.push_back(xy[1]);wall.coord.push_back(-s*xy[0]);
    }
    wall.conn={0,1,2,2,0,2,3,3};CBoundaryLayerMetric layers(3,{wall});
    for(const double z:{0.,.005,.0095}) {
      const su2double x[3]={s*z,0,c*z};const auto sample=layers.Evaluate(0,x,1.);
      double normal=0;
      const double n[3]={s,0,c};
      for(unsigned i=0;i<3;++i)for(unsigned j=0;j<3;++j)
        normal+=n[i]*SU2_TYPE::GetValue(sample.metric.m[i][j])*n[j];
      const double t=std::clamp((z-.009)/.001,0.,1.),w=1-t*t*(3-2*t);
      INFO("h0=" << h0 << " z=" << z);
      CHECK(normal==Approx(std::pow(h0,-2*w)).epsilon(2e-12));
      const auto& m=sample.full.m;
      CHECK(SU2_TYPE::GetValue(m[0][0]+m[2][2])>0);
    }
  }
}

TEST_CASE("Thin faceted BL queries preserve face and radial feature directions after translation", "[NativeComposite3D]") {
  using V=std::array<long double,3>;
  const V ab{.015625L,0,.0078125L},ac{0,.03125L,.015625L};
  auto dot=[](const V& a,const V& b){return a[0]*b[0]+a[1]*b[1]+a[2]*b[2];};
  auto unit=[&](V v){const auto length=std::sqrt(dot(v,v));for(auto& x:v)x/=length;return v;};
  const auto n=unit(V{-1,-1,2}),e=unit(ab),inward=unit(V{-1,5,2});
  for(double origin:{0.,7.25,1000000.}) {
    const V a{origin+.5L,origin+1.L,origin+2.L};
    CBoundaryLayerMetric::Wall wall;wall.layer={"wall",2e-6,1.2,.01};wall.conn={0,1,2,2};
    for(const auto& delta:std::array<V,3>{V{0,0,0},ab,ac})for(unsigned j=0;j<3;++j)wall.coord.push_back(double(a[j]+delta[j]));
    CBoundaryLayerMetric layers(3,{wall});
    for(unsigned k=0;k<129;++k) {
      su2double p[3];V offset{};
      for(unsigned j=0;j<3;++j){p[j]=double(a[j]+(.2L+k*.0005L)*ab[j]+.2L*ac[j]+2e-6L*n[j]);offset[j]=(long double)p[j]-a[j];}
      const auto sample=layers.Evaluate(0,p,1.);const auto distance=dot(offset,n);
      INFO("origin="<<origin<<" point="<<k);
      CHECK(std::abs((long double)sample.distance-distance)<8e-19L);
      for(unsigned j=0;j<3;++j)CHECK(std::abs((long double)sample.normal[j]-n[j])<4e-15L);
    }
    for(bool vertex:{false,true}) {
      su2double p[3];V offset{},expected{};
      for(unsigned j=0;j<3;++j) {
        const auto local=vertex ? -2e-6L*(e[j]+unit(ac)[j])+2e-6L*n[j]
                                : .5L*ab[j]-2e-6L*inward[j]+2e-6L*n[j];
        p[j]=double(a[j]+local);offset[j]=(long double)p[j]-a[j];
      }
      expected=offset;
      if(!vertex){const auto t=dot(offset,ab)/dot(ab,ab);for(unsigned j=0;j<3;++j)expected[j]-=t*ab[j];}
      const auto distance=std::sqrt(dot(expected,expected));expected=unit(expected);
      const auto sample=layers.Evaluate(0,p,1.);
      CHECK(std::abs((long double)sample.distance-distance)<8e-19L);
      for(unsigned j=0;j<3;++j)CHECK(std::abs((long double)sample.normal[j]-expected[j])<4e-15L);
      CHECK(std::abs(dot(expected,n))<.9L); // Off-feature normals must retain their actual radial direction.
    }
  }
}

TEST_CASE("Strict faceted edge integration resolves the existing on-wall normal classification band", "[NativeComposite3D]") {
  using namespace SU2Native3D;
  CBoundaryLayerMetric::Wall wall;wall.layer={"wall",2e-6,1.2,.01};
  wall.coord={1,1,1, 1.03125,1,1, 1,1.03125,1, 1,1,1.03125};wall.conn={0,1,2,2, 1,0,3,3};
  CBoundaryLayerMetric layers(3,{wall});
  const Point a{1,1,1},b{1.015625,1.0078125,1.000002};
  const auto cuts=layers.EdgeBreakpoints({a.x,a.y,a.z},{b.x,b.y,b.z});
  CHECK(std::any_of(cuts.begin(),cuts.end(),[](double t){return t>0 && t<1e-6;}));
  DonorCell donor;donor.cell={0,{{{0,{1,1,1}},{1,{1.125,1,1}},{2,{1,1.125,1}},{3,{1,1,1.125}}}}};
  donor.sensor.fill({1,0,0,1,0,1});
  const auto compose=[&](Point p,SU2Native3D::Tensor input) {
    CBoundaryLayerMetric::Tensor m;m.m[0][0]=input.xx;m.m[1][1]=input.yy;m.m[2][2]=input.zz;
    const su2double x[3]={p.x,p.y,p.z};layers.ApplyPoint(x,m,1.,nullptr,false);
    return SU2Native3D::Tensor{m.m[0][0],m.m[0][1],m.m[0][2],m.m[1][1],m.m[1][2],m.m[2][2]};
  };
  FrozenField target({donor},compose,false,{},[&](Point p,Point q){return layers.EdgeBreakpoints({p.x,p.y,p.z},{q.x,q.y,q.z});});
  for(const Point endpoint:std::array<Point,3>{b,Point{1,1.03125,1},Point{1.03125,1,1}}) {
    const auto forward=target.TargetEdgeLength(a,endpoint),reverse=target.TargetEdgeLength(endpoint,a);
    CHECK(std::abs(forward-reverse)<1e-9L*forward+1e-12L);
  }
}

TEST_CASE("Native geometric volume quadrature resolves microscopic BL in coarse tetrahedra", "[NativeVolumeBL3D]") {
  const double h0=1e-6,thickness=.01;
  CBoundaryLayerMetric::Wall wall;wall.layer={"wall",h0,1.,thickness};
  wall.coord={-2,-2,0, 2,-2,0, 2,2,0, -2,2,0};wall.conn={0,1,2,2,0,2,3,3};
  CBoundaryLayerMetric layers(3,{wall});
  const std::array<std::array<double,3>,4> cell{{{{.2,.2,0}},{{.8,.2,0}},{{.2,.8,0}},{{.2,.2,1}}}};
  double reference=.06*std::pow(1-thickness,3);
  const unsigned n=10000;
  for(unsigned k=0;k<=n;++k) {
    const double z=thickness*k/n,t=std::clamp((z-.9*thickness)/(.1*thickness),0.,1.),w=1-t*t*(3-2*t);
    reference+=thickness/n/3*(k==0||k==n?1:(k%2?4:2))*.18*(1-z)*(1-z)*std::pow(h0,-w);
  }
  for(const double ratio:{1.5,1.25}) {
    double volume=0,integral=0,mx=0,mz=0,mz2=0;
    bool dependent=true;unsigned densityCalls=0;
    const auto rule=layers.IntegrationRule3D(cell,ratio,[&](const auto&){++densityCalls;return 1.;},&dependent);
    CHECK(dependent);CHECK(densityCalls>0);
    REQUIRE(rule.size()<10000);
    for(const auto& point:rule) {
      REQUIRE(point.weight>0);double sum=0;
      std::array<double,3> x{};
      for(unsigned k=0;k<4;++k) {
        CHECK(point.barycentric[k]>=-1e-14);sum+=point.barycentric[k];
        for(unsigned j=0;j<3;++j)x[j]+=point.barycentric[k]*cell[k][j];
      }
      CHECK(sum==Approx(1).epsilon(1e-13));
      Tensor m;m.m[0][0]=m.m[1][1]=m.m[2][2]=1;
      for(const auto& sample:point.samples)layers.ApplySample(sample.wall,sample.sample,m,1.,nullptr,false);
      volume+=point.weight;mx+=point.weight*x[0];mz+=point.weight*x[2];mz2+=point.weight*x[2]*x[2];
      integral+=point.weight*std::sqrt(SU2_TYPE::GetValue(m.m[2][2]));
    }
    CHECK(volume==Approx(.06).epsilon(2e-12));CHECK(mx==Approx(.021).epsilon(2e-12));
    CHECK(mz==Approx(.015).epsilon(2e-12));CHECK(mz2==Approx(.006).epsilon(2e-12));
    CHECK(integral==Approx(reference).epsilon(5e-4));
  }
  auto reversed=cell;std::swap(reversed[0],reversed[1]);double reversedVolume=0;
  for(const auto& p:layers.IntegrationRule3D(reversed))reversedVolume+=p.weight;
  CHECK(reversedVolume==Approx(.06).epsilon(2e-12));
  CHECK_THROWS(layers.IntegrationRule3D(cell,1.));
  auto degenerate=cell;degenerate[3]=degenerate[0];CHECK_THROWS(layers.IntegrationRule3D(degenerate));
}

TEST_CASE("Native3D complexity integrates the geometric BL with sensor-only donors", "[NativeVolumeBL3D]") {
  const double h0=1e-4,T=.02;
  const auto mesh=simplex_test::MakeSimplexMesh(3,2,[](const passivedouble* x){return x[2]==0?"wall":"far";});
  MetricTest test(mesh,"MARKER_HEATFLUX= (wall,0)\nMARKER_FAR= (far)\nADAP_REMESHER= NATIVE_CAVITY\n"
                       "ADAP_SURFACE= YES\nADAP_ISO_CORNER= NO\nADAP_HGRAD= 1.3\nADAP_HMIN= 1e-4\nADAP_HMAX= 1\n"
                       "ADAP_ARMAX= 100\nADAP_COMPLEXITY= 100\nADAP_BL_MARKER= (wall)\nADAP_BL_FIRST_HEIGHT= (1e-4)\n"
                       "ADAP_BL_GROWTH= (1)\nADAP_BL_THICKNESS= (.02)\n",{1,0,0,1,0,1});
  const auto output=test.ComputeMetric();INFO(output);
  // The wall triangles have tangent eigenvalues2 and6; sensor is the bounded identity.
  const double fullDensity=std::sqrt(12.)/h0;
  double reference=1-T+.9*T*fullDensity;const unsigned n=10000;
  for(unsigned k=0;k<=n;++k) {
    const double t=double(k)/n,w=1-t*t*(3-2*t);
    reference+=.1*T/n/3*(k==0||k==n?1:(k%2?4:2))*std::pow(fullDensity,w);
  }
  CHECK(test.solver[FLOW_SOL]->GetMetricComplexityFinal()==Approx(reference).epsilon(5e-4));
  for(auto p=0ul;p<test.Geometry().GetnPointDomain();++p) {
    const auto m=test.Metric(p);CHECK(SU2_TYPE::GetValue(m.m[0][0])==Approx(1));
    CHECK(SU2_TYPE::GetValue(m.m[1][1])==Approx(1));CHECK(SU2_TYPE::GetValue(m.m[2][2])==Approx(1));
  }
}

TEST_CASE("Native3D complexity reference does not rebase the wall on the adapted surface", "[NativeVolumeBL3D]") {
  using namespace SU2Native3D;
  auto mesh=simplex_test::MakeSimplexMesh(3,2,[](const passivedouble* x){return x[2]==0?"wall":"far";});
  MetricTest test(mesh,"MARKER_HEATFLUX= (wall,0)\nMARKER_FAR= (far)\nADAP_REMESHER= NATIVE_CAVITY\n"
                       "ADAP_SURFACE= YES\nADAP_ISO_CORNER= NO\nADAP_HMIN= .001\nADAP_HMAX= 1\n"
                       "ADAP_ARMAX= 100\nADAP_COMPLEXITY= 100\nADAP_BL_MARKER= (wall)\nADAP_BL_FIRST_HEIGHT= (.001)\n"
                       "ADAP_BL_GROWTH= (1.2)\nADAP_BL_THICKNESS= (.1)\n",{1,0,0,1,0,1});
  ReferenceState state;const auto imported=ImportGeometry(*test.config,test.Geometry(),nullptr,state);
  REQUIRE(state.original);auto layers=CBoundaryLayerMetric::FromNativeReference(state,*test.config);
  const su2double x[3]={.25,.25,0};const auto original=layers->Evaluate(0,x,1.);
  CBoundaryLayerMetric::Wall shifted;shifted.layer={"wall",.001,1.2,.1};
  shifted.coord={0,0,.025,1,0,.025,1,1,.025,0,1,.025};shifted.conn={0,1,2,2,0,2,3,3};
  CBoundaryLayerMetric current(3,{shifted});
  CHECK(original.distance==Approx(0).margin(1e-14));CHECK(current.Evaluate(0,x,1.).distance==Approx(.025));
  CHECK(CBoundaryLayerMetric::FromNativeReference(state,*test.config)->Evaluate(0,x,1.).hn==Approx(.001));
  CHECK_FALSE(imported.original.empty());
}

TEST_CASE("Native3D volume integration resolves a far vertex closest to another wall face", "[NativeVolumeBL3D]") {
  CBoundaryLayerMetric::Wall wall;wall.layer={"wall",.005,1.2,.03};
  wall.coord={0,0,0,1,0,0,1,1,0,0,1,0,0,0,1,0,1,1};
  wall.conn={0,1,2,2,0,2,3,3,0,3,5,5,0,5,4,4};
  CBoundaryLayerMetric layers(3,{wall});
  const std::array<std::array<double,3>,4> cell{{{{.2,.2,0}},{{.8,.2,0}},{{.2,.8,0}},{{.2,.2,.4}}}};
  const su2double top[3]={.2,.2,.4};CHECK(layers.Evaluate(0,top,1.).distance==Approx(.2));
  double integrals[2]{};unsigned index=0;
  for(const double ratio:{1.5,1.25}) {
    double volume=0;
    for(const auto& point:layers.IntegrationRule3D(cell,ratio)) {
      Tensor m;m.m[0][0]=m.m[1][1]=m.m[2][2]=1;
      for(const auto& sample:point.samples)layers.ApplySample(sample.wall,sample.sample,m,1.,nullptr,false);
      const double determinant=SU2_TYPE::GetValue(m.m[0][0]*(m.m[1][1]*m.m[2][2]-m.m[1][2]*m.m[2][1])
          -m.m[0][1]*(m.m[1][0]*m.m[2][2]-m.m[1][2]*m.m[2][0])
          +m.m[0][2]*(m.m[1][0]*m.m[2][1]-m.m[1][1]*m.m[2][0]));
      REQUIRE(determinant>0);volume+=point.weight;integrals[index]+=point.weight*std::sqrt(determinant);
    }
    CHECK(volume==Approx(.024).epsilon(1e-11));++index;
  }
  CHECK(integrals[0]>10*.024);CHECK(integrals[0]==Approx(integrals[1]).epsilon(5e-3));
}

TEST_CASE("Native3D volume support detects a thin finite wall missed by all cell vertices", "[NativeVolumeBL3D]") {
  const bool tiny=GENERATE(false,true),shifted=GENERATE(false,true);
  const double h0=tiny?.002:.02,thickness=tiny?.008:.08,side=tiny?.024:.24;
  const double x0=(tiny?.338:.28)+(shifted?.065:0),y0=(tiny?.238:.18)+(shifted?.037:0),z0=tiny?.019:0,gap=.02-z0;
  const double strongerCore=100*std::pow(.24/side,2),strongerBase=std::pow(strongerCore,1.5);
  CBoundaryLayerMetric::Wall wall;wall.layer={"wall",h0,1.2,thickness};
  wall.coord={x0,y0,z0,x0+side,y0,z0,x0,y0+side,z0};wall.conn={0,1,2,2};
  CBoundaryLayerMetric layers(3,{wall});
  const std::array<std::array<double,3>,4> cell{{{{.1,.1,.02}},{{.9,.1,.02}},{{.1,.9,.02}},{{.1,.1,.8}}}};
  for(const auto& p:cell){const su2double x[3]={p[0],p[1],p[2]};CHECK(layers.Evaluate(0,x,1.).weight==0);}
  auto density=[=](double distance) {
    const double hn=std::max(h0,2*(h0+.2*distance)/2.2);
    const double t=std::clamp((distance-.9*thickness)/(.1*thickness),0.,1.),w=1-t*t*(3-2*t);
    return std::pow(std::sqrt(3.)/(2*side*side*hn),w);
  };
  double volume=0,active=0,integral=0,mz=0,mz2=0,analyticIntegral=0,maximumDensityMismatch=0;
  bool dependent=false;
  const auto rule=layers.IntegrationRule3D(cell,1.5,{},&dependent);
  CHECK(dependent);
  for(const auto& point:rule) {
    volume+=point.weight;
    if(!point.samples.empty())active+=point.weight;
    double z=0;for(unsigned k=0;k<4;++k)z+=point.barycentric[k]*cell[k][2];
    mz+=point.weight*z;mz2+=point.weight*z*z;
    Tensor m;m.m[0][0]=m.m[1][1]=m.m[2][2]=1;
    for(const auto& sample:point.samples)layers.ApplySample(sample.wall,sample.sample,m,1.,nullptr,false);
    const double det=SU2_TYPE::GetValue(m.m[0][0]*(m.m[1][1]*m.m[2][2]-m.m[1][2]*m.m[2][1])
        -m.m[0][1]*(m.m[1][0]*m.m[2][2]-m.m[1][2]*m.m[2][0])
        +m.m[0][2]*(m.m[1][0]*m.m[2][1]-m.m[1][1]*m.m[2][0]));
    REQUIRE(det>0);integral+=point.weight*std::sqrt(det);
    const double analytic=point.samples.empty()?1:density(SU2_TYPE::GetValue(point.samples.front().sample.distance));
    analyticIntegral+=point.weight*analytic;maximumDensityMismatch=std::max(maximumDensityMismatch,std::abs(std::sqrt(det)/analytic-1));
  }
  const double expectedVolume=.8*.8*.78/6;
  CHECK(volume==Approx(expectedVolume).epsilon(1e-10));CHECK(active>0);
  CHECK(mz==Approx(expectedVolume*(.02+.78/4)).epsilon(1e-10));
  CHECK(mz2==Approx(expectedVolume*(.02*.02+2*.02*.78/4+.78*.78/10)).epsilon(1e-10));
  // Independent parallel-body integral: area=A+P*r+pi*r^2 outside a convex triangle.
  // Its whole active support above z=.02 lies inside this source tetrahedron.
  auto simpson=[](auto f,double a,double b) {
    double value=0;const unsigned n=256;
    for(unsigned k=0;k<=n;++k)value+=(b-a)/n/3*(k==0||k==n?1:(k%2?4:2))*f(a+(b-a)*k/n);
    return value;
  };
  auto section=[&](double z) {
    const double radius=std::sqrt(std::max(0.,thickness*thickness-z*z)),full=std::sqrt(std::max(0.,.81*thickness*thickness-z*z));
    auto rings=[&](double r){return (side*(2+std::sqrt(2.))+2*std::acos(-1.)*r)*(density(std::hypot(z,r))-1);};
    return side*side/2*(density(z)-1)+simpson(rings,0,full)+simpson(rings,full,radius);
  };
  const double reference=expectedVolume+simpson(section,gap,.9*thickness)+simpson(section,.9*thickness,thickness);
  INFO("points=" << rule.size() << " integral=" << integral << " analytic_on_rule=" << analyticIntegral
       << " pointwise_mismatch=" << maximumDensityMismatch << " reference=" << reference);
  CHECK(maximumDensityMismatch<1e-6);
  CHECK(integral==Approx(reference).epsilon(5e-4));
  // A finer isotropic sensor controls both tangents; test the full-target callback against a second parallel-body oracle.
  auto strongerDensity=[&](const CBoundaryLayerMetric::IntegrationPoint3D& point) {
    Tensor m;for(unsigned j=0;j<3;++j)m.m[j][j]=strongerCore;
    for(const auto& sample:point.samples)layers.ApplySample(sample.wall,sample.sample,m,strongerCore,nullptr,false);
    const double det=SU2_TYPE::GetValue(m.m[0][0]*(m.m[1][1]*m.m[2][2]-m.m[1][2]*m.m[2][1])
        -m.m[0][1]*(m.m[1][0]*m.m[2][2]-m.m[1][2]*m.m[2][0])
        +m.m[0][2]*(m.m[1][0]*m.m[2][1]-m.m[1][1]*m.m[2][0]));
    return std::sqrt(det);
  };
  const auto strongerRule=layers.IntegrationRule3D(cell,1.5,strongerDensity);
  double strongerIntegral=0;for(const auto& point:strongerRule)strongerIntegral+=point.weight*strongerDensity(point);
  auto strongerRadial=[=](double distance) {
    const double hn=std::max(h0,2*(h0+.2*distance)/2.2);
    const double t=std::clamp((distance-.9*thickness)/(.1*thickness),0.,1.),w=1-t*t*(3-2*t);
    return strongerCore*std::pow(1/hn,w)*std::pow(std::sqrt(strongerCore),1-w);
  };
  auto strongerSection=[&](double z) {
    const double radius=std::sqrt(std::max(0.,thickness*thickness-z*z)),full=std::sqrt(std::max(0.,.81*thickness*thickness-z*z));
    auto rings=[&](double r){return (side*(2+std::sqrt(2.))+2*std::acos(-1.)*r)*(strongerRadial(std::hypot(z,r))-strongerBase);};
    return side*side/2*(strongerRadial(z)-strongerBase)+simpson(rings,0,full)+simpson(rings,full,radius);
  };
  const double strongerReference=strongerBase*expectedVolume+simpson(strongerSection,gap,.9*thickness)+simpson(strongerSection,.9*thickness,thickness);
  INFO("stronger points=" << strongerRule.size() << " integral=" << strongerIntegral << " reference=" << strongerReference);
  CHECK(strongerIntegral==Approx(strongerReference).epsilon(5e-4));
  std::ostringstream summary;summary << std::setprecision(17);
  summary << "NativeBL3DQuadrature {\"tiny\":" << (tiny?"true":"false") << ",\"shifted\":" << (shifted?"true":"false") << ",\"points\":" << rule.size() << ",\"integral\":" << integral
          << ",\"reference\":" << reference << ",\"analytic_on_rule\":" << analyticIntegral
          << ",\"pointwise_mismatch\":" << maximumDensityMismatch << ",\"stronger_points\":" << strongerRule.size()
          << ",\"stronger_integral\":" << strongerIntegral << ",\"stronger_reference\":" << strongerReference << "}";
  std::cout << summary.str() << '\n';
  CHECK_THROWS(layers.IntegrationRule3D(cell,1.5,[](const auto&){return -1.;}));
  CHECK_THROWS(layers.IntegrationRule3D(cell,1.5,[](const auto&){return std::numeric_limits<double>::quiet_NaN();}));
}


TEST_CASE("Native3D parallel chords preserve source volume and moments on rotated and sheared tetrahedra", "[NativeParallelBL3D]") {
  for(bool shear:{false,true}) {
    auto rotate=[](const std::array<double,3>& x) {
      return std::array<double,3>{10+(x[0]-2*x[1]+2*x[2])/3,-7+(2*x[0]+2*x[1]+x[2])/3,3+(-2*x[0]+x[1]+2*x[2])/3};
    };
    std::array<std::array<double,3>,4> cell{{{{.1,.1,.02}},{{.9,.1,.02}},{{.1,.9,.02}},{{.1,.1,.8}}}};
    if(shear)cell[3]={.7,.5,.8};for(auto& x:cell)x=rotate(x);if(shear)std::swap(cell[0],cell[1]);
    CBoundaryLayerMetric::Wall wall;wall.layer={"wall",.02,1.2,.08};wall.conn={0,1,2,2};
    for(const auto& x:std::array<std::array<double,3>,3>{{{{.28,.18,0}},{{.52,.18,0}},{{.28,.42,0}}}}) {
      const auto p=rotate(x);for(auto value:p)wall.coord.push_back(value);
    }
    CBoundaryLayerMetric layers(3,{wall});
    auto polynomial=[&](const CBoundaryLayerMetric::IntegrationPoint3D& point) {
      std::array<double,3> x{};for(unsigned k=0;k<4;++k)for(unsigned j=0;j<3;++j)x[j]+=point.barycentric[k]*cell[k][j];
      return 1+x[0]*x[0]+.2*x[1]*x[1]+.3*x[2]*x[2];
    };
    INFO("shear=" << shear);
    const auto rule=layers.IntegrationRule3D(cell,1.5,polynomial);
    double volume=0,integral=0;bool sampled=false;
    for(const auto& point:rule) {
      REQUIRE(point.weight>0);double sum=0;for(auto w:point.barycentric){REQUIRE(w>=-1e-14);sum+=w;}
      CHECK(sum==Approx(1).epsilon(1e-12));volume+=point.weight;integral+=point.weight*polynomial(point);
      sampled=sampled || !point.samples.empty();
    }
    const double expectedVolume=.8*.8*.78/6;double expected=1;
    for(unsigned j=0;j<3;++j) {
      double sum=0,squares=0;for(const auto& x:cell){sum+=x[j];squares+=x[j]*x[j];}
      expected+=(j==0?1:(j==1?.2:.3))*(sum*sum+squares)/20;
    }
    INFO("shear=" << shear << " points=" << rule.size());
    CHECK(sampled);CHECK(volume==Approx(expectedVolume).epsilon(1e-10));
    CHECK(integral==Approx(expectedVolume*expected).epsilon(1e-10));
  }
}

TEST_CASE("Native edge breakpoint admission counts physical roots on a densely tiled wall", "[NativeComposite3D]") {
  CBoundaryLayerMetric::Wall wall;wall.layer={"wall",1e-6,1.2,.05};
  constexpr unsigned n=64;
  for(unsigned j=0;j<=n;++j)for(unsigned i=0;i<=n;++i)wall.coord.insert(wall.coord.end(),{double(i)/n,double(j)/n,0});
  for(unsigned j=0;j<n;++j)for(unsigned i=0;i<n;++i) {
    const unsigned a=j*(n+1)+i,b=a+1,d=a+n+1,c=d+1;
    wall.conn.insert(wall.conn.end(),{a,b,c,c,a,c,d,d});
  }
  CBoundaryLayerMetric layers(3,{wall});
  const auto cuts=layers.EdgeBreakpoints({.1,.53,5e-6},{.9,.53,5e-6});
  // A chord wholly inside a planar band has no physical level crossings, despite many feature roots.
  REQUIRE(cuts.size()==2);CHECK(cuts.front()==0);CHECK(cuts.back()==1);
}

namespace {
CBoundaryLayerMetric::Wall AnalyticSphereWall(double h0=.01,double thickness=.03,double radius=1,SU2Native3D::Point origin={}) {
  using namespace SU2Native3D;
  const std::array<Point,6> points{{{origin.x+radius,origin.y,origin.z},{origin.x-radius,origin.y,origin.z},
      {origin.x,origin.y+radius,origin.z},{origin.x,origin.y-radius,origin.z},{origin.x,origin.y,origin.z+radius},{origin.x,origin.y,origin.z-radius}}};
  CBoundaryLayerMetric::Wall wall;wall.layer={"sphere",h0,1.2,thickness};wall.surface_tolerance=.01;
  std::vector<Facet> facets;std::map<Id,AnalyticSurface> curves;AnalyticSurface g;g.origin=origin;g.extent={radius,0,0};
  for(unsigned x=0;x<2;++x)for(unsigned y=2;y<4;++y)for(unsigned z=4;z<6;++z) {
    std::array<unsigned,3> v{x,y,z};
    if(Orientation(origin,points[v[0]],points[v[1]],points[v[2]])>0)std::swap(v[1],v[2]);
    Facet f;f.id=facets.size();f.marker=1;
    for(unsigned k=0;k<3;++k)f.v[k]={v[k],points[v[k]]};facets.push_back(f);curves[f.id]=g;
    wall.conn.insert(wall.conn.end(),{v[0],v[1],v[2],v[2]});
  }
  for(auto p:points)wall.coord.insert(wall.coord.end(),{p.x,p.y,p.z});
  wall.authority=std::make_shared<PlanarReference>(facets,SIZE_MAX,curves);wall.authority_facet=wall.authority->Canonical(0);
  return wall;
}
}

TEST_CASE("Smooth native BL preserves actual radial queries, fine sensors and hidden sphere edge bands", "[NativeCurvedBL3D]") {
  const auto wall=AnalyticSphereWall();CBoundaryLayerMetric layers(3,{wall});
  for(double offset:{-1e-7,1e-7,.005,.02}) {
    const su2double x[3]={1+offset,0,0};const auto sample=layers.Evaluate(0,x,4.);
    CHECK(SU2_TYPE::GetValue(sample.distance)==Approx(std::abs(offset)).margin(2e-15));
    double norm=0;for(auto value:sample.normal)norm+=SU2_TYPE::GetValue(value*value);
    CHECK(norm==Approx(1.).margin(2e-12));
    Tensor sensor;for(unsigned j=0;j<3;++j)sensor.m[j][j]=1e8;
    layers.ApplyPoint(x,sensor,4.,nullptr,false);
    for(unsigned i=0;i<3;++i)for(unsigned j=0;j<3;++j)CHECK(SU2_TYPE::GetValue(sensor.m[i][j])==Approx(i==j?1e8:0.).margin(2e-6));
  }
  const std::array<double,3> a{-2,.9,0},b{2,.9,0};
  for(auto p:{a,b,std::array<double,3>{0,.9,0}}) {
    const su2double x[3]={p[0],p[1],p[2]};CHECK(layers.Evaluate(0,x,4.).weight==0);
  }
  const auto cuts=layers.EdgeBreakpoints(a,b),reverse=layers.EdgeBreakpoints(b,a);
  REQUIRE(cuts.size()>20);REQUIRE(cuts.size()==reverse.size());
  const double crossing=.5-std::sqrt(1-.9*.9)/4;
  CHECK(*std::min_element(cuts.begin(),cuts.end(),[&](auto x,auto y){return std::abs(x-crossing)<std::abs(y-crossing);})==Approx(crossing).margin(2e-12));
  for(size_t i=0;i<cuts.size();++i)CHECK(cuts[i]==Approx(1-reverse[cuts.size()-1-i]).margin(2e-12));
}

TEST_CASE("Smooth spherical BL thin volume rule resolves an enclosed layer against a radial integral", "[.][NativeCurvedBLVolume3D]") {
  const auto wall=AnalyticSphereWall();CBoundaryLayerMetric layers(3,{wall});
  const std::array<std::array<double,3>,4> cell{{{{0,0,0}},{{4,0,0}},{{0,4,0}},{{0,0,4}}}};
  auto density=[&](const CBoundaryLayerMetric::IntegrationPoint3D& p) {
    Tensor metric;for(unsigned j=0;j<3;++j)metric.m[j][j]=4;
    for(const auto& sample:p.samples)layers.ApplySample(sample.wall,sample.sample,metric,4.,nullptr,false);
    su2double v[3][3],e[3],w[3];CBlasStructure::EigenDecomposition(metric.m,v,e,3,w);
    return SU2_TYPE::GetValue(sqrt(e[0]*e[1]*e[2]));
  };
  // Whole spherical octant shell is inside this coarse tetrahedron.
  const double epsilon=.005,chord=2*std::sqrt(epsilon*(2-epsilon)),curvature=std::pow(1.8/chord,2);
  auto radial=[&](double distance) {
    const double hn=std::max(.01,2*(.01+.2*distance)/2.2);
    const double t=std::clamp((distance-.027)/.003,0.,1.),weight=1-t*t*(3-2*t);
    return std::pow(curvature,weight)*std::pow(4.,1-weight)*std::pow(1/hn,weight)*std::pow(2.,1-weight);
  };
  auto integral=[&](double left,double right) {
    constexpr unsigned n=4096;const double h=(right-left)/n;double total=0;
    for(unsigned k=0;k<=n;++k){const double d=left+h*k;total+=(k==0||k==n?1:k%2?4:2)*(1+d*d)*(radial(d)-8);}
    return h*total/3;
  };
  const double expected=8*64/6+std::acos(-1.)*(integral(0,.005)+integral(.005,.027)+integral(.027,.03));
  bool dependent=false;const auto rule=layers.IntegrationRule3D(cell,1.5,density,&dependent);
  double volume=0,measured=0;bool sampled=false;
  for(const auto& p:rule){CHECK(p.weight>0);volume+=p.weight;measured+=p.weight*density(p);sampled=sampled||!p.samples.empty();}
  REQUIRE(sampled);CHECK(dependent);CHECK(volume==Approx(64./6).epsilon(2e-10));
  INFO("points="<<rule.size()<<" measured="<<measured<<" radial="<<expected);
  CHECK(measured==Approx(expected).epsilon(8e-4));
}

TEST_CASE("Smooth first-row clearance separates curved geometry blockage from exact facet altitude", "[NativeCurvedBL3D]") {
  using namespace SU2Native3D;const auto wall=AnalyticSphereWall();const auto& reference=*wall.authority;
  const auto face=reference.Get(0);const auto a=face.v[0].p,b=face.v[1].p,c=face.v[2].p;
  Point center{(a.x+b.x+c.x)/3,(a.y+b.y+c.y)/3,(a.z+b.z+c.z)/3};
  const double radius=std::hypot(center.x,center.y,center.z);
  for(double height:{.1,.6}) {
    Node apex{10,{center.x*(1+height/radius),center.y*(1+height/radius),center.z*(1+height/radius)}};
    Cell cell;cell.id=100;cell.v={face.v[0],face.v[2],face.v[1],apex};
    REQUIRE(Orientation(cell.v[0].p,cell.v[1].p,cell.v[2].p,cell.v[3].p)>0);
    Patch patch{{cell},{{{face.v[0].id,face.v[1].id,face.v[2].id},0}}};
    FirstHeightReport report;std::string reason;const bool valid=CheckFirstHeight(patch,{{1,height}},reference,report,reason);
    CHECK(report.height_failures==0);CHECK(report.footprint_failures==0);
    CHECK(report.minimum_reference_clearance_ratio==Approx((radius+height-1)/height).epsilon(2e-12));
    CHECK(valid==(height>.5));
    if(!valid){CHECK(report.reference_clearance_failures==1);CHECK(reason.find("geometry-height incompatibility")!=std::string::npos);CHECK(report.reference_clearance_cell==100);}
  }
  const PlanarReference partial({face},SIZE_MAX,{{0,reference.Analytic().at(0)}});
  CHECK_FALSE(partial.CompleteAnalytic(0));CHECK_THROWS(partial.ClosestAnalytic(0,{1.1,0,0}));
}

namespace {
CBoundaryLayerMetric::Wall AnalyticCylinderWall(double height=.1,double thickness=.3,double tolerance=.01) {
  using namespace SU2Native3D;constexpr unsigned n=16;
  CBoundaryLayerMetric::Wall wall;wall.layer={"cylinder",height,1.2,thickness};wall.surface_tolerance=tolerance;
  std::vector<Node> points;std::vector<Facet> facets;std::map<Id,AnalyticSurface> curves;
  AnalyticSurface g;g.kind=AnalyticSurface::Kind::Cylinder;g.extent={1,1,0};
  for(unsigned z=0;z<2;++z)for(unsigned i=0;i<n;++i) {
    const double angle=2*std::acos(-1.)*i/n;Point p{std::cos(angle),std::sin(angle),double(z)};
    points.push_back({points.size(),p});wall.coord.insert(wall.coord.end(),{p.x,p.y,p.z});
  }
  for(unsigned i=0;i<n;++i) {
    const unsigned j=(i+1)%n;
    for(auto v:{std::array<unsigned,3>{i,n+j,j},std::array<unsigned,3>{i,n+i,n+j}}) {
      Facet f;f.id=facets.size();f.marker=1;for(unsigned k=0;k<3;++k)f.v[k]=points[v[k]];
      facets.push_back(f);curves[f.id]=g;wall.conn.insert(wall.conn.end(),{v[0],v[1],v[2],v[2]});
    }
  }
  wall.authority=std::make_shared<PlanarReference>(facets,SIZE_MAX,curves);wall.authority_facet=wall.authority->Canonical(0);
  return wall;
}
}

TEST_CASE("Finite cylinder BL queries and edge bands include the actual end rims", "[NativeCurvedBL3D]") {
  const auto wall=AnalyticCylinderWall();REQUIRE(wall.authority->CompleteAnalytic(0));CBoundaryLayerMetric layers(3,{wall});
  for(double z:{-.1,.5,1.1}) {
    const su2double x[3]={1.05,0,z};const auto sample=layers.Evaluate(0,x,1.);
    const double dz=std::max({0.,-z,z-1});CHECK(SU2_TYPE::GetValue(sample.distance)==Approx(std::hypot(.05,dz)).epsilon(2e-12));
    double norm=0;for(auto value:sample.normal)norm+=SU2_TYPE::GetValue(value*value);CHECK(norm==Approx(1.).margin(2e-12));
  }
  const su2double rim[3]={1,0,1.05};const auto sample=layers.Evaluate(0,rim,1.);
  CHECK(SU2_TYPE::GetValue(sample.full.m[1][1])>=std::pow(1.8/(2*std::sqrt(.01*1.99)),2)*(1-1e-10));
  const auto cuts=layers.EdgeBreakpoints({1.05,0,-1},{1.05,0,2});
  const double crossing=(1-std::sqrt(.3*.3-.05*.05))/3;
  CHECK(*std::min_element(cuts.begin(),cuts.end(),[&](auto x,auto y){return std::abs(x-crossing)<std::abs(y-crossing);})==Approx(crossing).margin(2e-10));
}

TEST_CASE("Visible wall samples cannot hide an enclosed thin spherical second wall", "[.][NativeInteractingBLVolume3D]") {
  CBoundaryLayerMetric::Wall visible;visible.layer={"a_visible",2.,1.,2.};
  visible.coord={-2,-2,0,2,-2,0,2,2,0,-2,2,0};visible.conn={0,1,2,2,0,2,3,3};
  auto hidden=AnalyticSphereWall(1e-5,3e-5,.001,{.35,.35,.02});hidden.layer.marker="b_hidden";
  CBoundaryLayerMetric layers(3,{visible,hidden});
  const std::array<std::array<double,3>,4> cell{{{{0,0,0}},{{1,0,0}},{{0,1,0}},{{0,0,1}}}};
  auto density=[&](const CBoundaryLayerMetric::IntegrationPoint3D& p) {
    Tensor metric;for(unsigned j=0;j<3;++j)metric.m[j][j]=4;
    for(const auto& sample:p.samples)layers.ApplySample(sample.wall,sample.sample,metric,4.,nullptr,false);
    su2double v[3][3],e[3],w[3];CBlasStructure::EigenDecomposition(metric.m,v,e,3,w);
    return SU2_TYPE::GetValue(sqrt(e[0]*e[1]*e[2]));
  };
  const double r=.001,epsilon=5e-6,curvature=std::pow(1.8/(2*std::sqrt(epsilon*(2*r-epsilon))),2);
  auto integrand=[&](double distance) {
    const double hn=std::max(1e-5,2*(1e-5+.2*distance)/2.2),t=std::clamp((distance-2.7e-5)/3e-6,0.,1.),weight=1-t*t*(3-2*t);
    const double full=std::pow(curvature,weight)*std::pow(4.,1-weight)*std::pow(1/hn,weight)*std::pow(2.,1-weight);
    return (r*r+distance*distance)*(full-8);
  };
  auto integral=[&](double left,double right) {
    constexpr unsigned n=4096;const double h=(right-left)/n;double sum=0;
    for(unsigned k=0;k<=n;++k)sum+=(k==0||k==n?1:k%2?4:2)*integrand(left+h*k);
    return h*sum/3;
  };
  // The enclosed sphere has both shells: (r-d)^2+(r+d)^2 = 2*(r*r+d*d).
  const double expected=8./6+8*std::acos(-1.)*(integral(0,5e-6)+integral(5e-6,2.7e-5)+integral(2.7e-5,3e-5));
  const auto rule=layers.IntegrationRule3D(cell,1.5,density);double volume=0,measured=0;bool found=false;
  for(const auto& p:rule) {
    volume+=p.weight;measured+=p.weight*density(p);
    CHECK(std::any_of(p.samples.begin(),p.samples.end(),[](const auto& s){return s.wall==0;}));
    found=found||std::any_of(p.samples.begin(),p.samples.end(),[](const auto& s){return s.wall==1;});
  }
  REQUIRE(found);CHECK(volume==Approx(1./6).epsilon(2e-10));
  INFO("points="<<rule.size()<<" measured="<<measured<<" radial="<<expected);
  CHECK(measured==Approx(expected).epsilon(8e-4));
}

TEST_CASE("Curved BL gradation diagnosis keeps sensor and composed fields distinct", "[NativeCurvedBL3D]") {
  const auto wall=AnalyticSphereWall();CBoundaryLayerMetric layers(3,{wall});
  const double points[]={1.027,1.028,1.029,1.03};double maximum=1;
  auto metric=[&](double position){const su2double x[3]={position,0,0};Tensor m;for(unsigned j=0;j<3;++j)m.m[j][j]=4;layers.ApplyPoint(x,m,4.,nullptr,false);return m;};
  for(unsigned i=0;i<3;++i)for(bool reversed:{false,true}) {
    const auto a=metric(points[i+unsigned(reversed)]),b=metric(points[i+unsigned(!reversed)]);
    const double distance=(points[i+1]-points[i])*std::sqrt(SU2_TYPE::GetValue(b.m[0][0]));
    const double factor=std::pow(1+std::log(1.3)*distance,-2);
    for(unsigned j=0;j<3;++j)maximum=std::max(maximum,factor*SU2_TYPE::GetValue(b.m[j][j]/a.m[j][j]));
  }
  // These finite fade probes diagnose the target; they do not regularize it during reconstruction.
  CHECK(maximum>1);std::cout<<"CURVED_BL_GRADATION sensor_ratio=1 composed_ratio="<<std::setprecision(17)<<maximum<<'\n';
}


TEST_CASE("Cylindrical annulus integration resolves coarse prism cells against independent polar integration", "[.][NativeCylinderBLVolume3D]") {
  const long double delta=std::ldexp(1.L,-40),det=std::ldexp(1.L,-80);
  CHECK(SU2Native3D::CompensatedCross2D(1,1+delta,1-delta,1)==det);
  CHECK(SU2Native3D::CompensatedCross2D(1-delta,1,1,1+delta)==-det);
  CHECK(SU2Native3D::CompensatedCross2D(1,2,2,4)==0);
  const auto wall=AnalyticCylinderWall(.05,.2,.04);CBoundaryLayerMetric layers(3,{wall});
  const double angle=std::acos(-1.)/4,c=std::cos(angle),s=std::sin(angle);
  using P=std::array<double,3>;using T=std::array<P,4>;
  const P a{0,0,0},b{1,0,0},d{c,s,0},A{0,0,1},B{1,0,1},D{c,s,1};
  auto density=[&](const CBoundaryLayerMetric::IntegrationPoint3D& p) {
    Tensor metric;for(unsigned j=0;j<3;++j)metric.m[j][j]=1;
    for(const auto& sample:p.samples)layers.ApplySample(sample.wall,sample.sample,metric,1.,nullptr,false);
    su2double v[3][3],e[3],w[3];CBlasStructure::EigenDecomposition(metric.m,v,e,3,w);
    return SU2_TYPE::GetValue(sqrt(e[0]*e[1]*e[2]));
  };
  double measured=0;size_t points=0;
  for(const T& cell:std::array<T,3>{{{a,b,d,D},{a,b,B,D},{a,B,A,D}}}) {
    bool dependent=false;const auto rule=layers.IntegrationRule3D(cell,1.5,density,&dependent);CHECK(dependent);CHECK(rule.size()<=65536);
    double volume=0;for(const auto& p:rule){REQUIRE(p.weight>0);for(double v:p.barycentric)REQUIRE(v>=-1e-14);volume+=p.weight;measured+=p.weight*density(p);}
    CHECK(volume==Approx(s/6).epsilon(2e-10));points+=rule.size();
  }
  // Independent prism measure r dr dtheta dz. The retained wall tangential tensor varies with theta/z.
  auto tangentLog=[&](double theta,double z) {
    const double ct=std::cos(theta),st=std::sin(theta);const su2double x[3]={.975*ct,.975*st,z};
    const auto sample=layers.Evaluate(0,x,1.);const auto& m=sample.full.m;
    const double tx=-st,ty=ct;
    const double aa=SU2_TYPE::GetValue(tx*tx*m[0][0]+2*tx*ty*m[0][1]+ty*ty*m[1][1]);
    const double bb=SU2_TYPE::GetValue(tx*m[0][2]+ty*m[1][2]),cc=SU2_TYPE::GetValue(m[2][2]);
    const double gap=std::hypot(aa-cc,2*bb),largest=.5*(aa+cc+gap),smallest=(aa*cc-bb*bb)/largest;
    return .5*std::log(std::max(1.,largest)*std::max(1.,smallest));
  };
  auto radial=[&](double r,double tangent) {const double distance=1-r,hn=std::max(.05,2*(.05+.2*distance)/2.2);
    const double t=std::clamp((distance-.18)/.02,0.,1.),weight=distance>=.2?0:1-t*t*(3-2*t);
    return std::exp(weight*(tangent-std::log(hn)));};
  for(double theta:{.2*angle,.8*angle})for(double z:{.2,.7})for(double r:{.81,.9,.99}) {
    Tensor metric;for(unsigned j=0;j<3;++j)metric.m[j][j]=1;const su2double x[3]={r*std::cos(theta),r*std::sin(theta),z};
    layers.ApplyPoint(x,metric,1.,nullptr,false);su2double v[3][3],e[3],w[3];CBlasStructure::EigenDecomposition(metric.m,v,e,3,w);
    CHECK(radial(r,tangentLog(theta,z))==Approx(SU2_TYPE::GetValue(sqrt(e[0]*e[1]*e[2]))).epsilon(2e-11));
  }
  auto reference=[&](unsigned n) {double total=0;const double h=angle/n;
    for(unsigned j=0;j<=n;++j){const double theta=h*j,end=std::cos(angle/2)/std::cos(theta-angle/2);double axial=0;
      for(unsigned z=0;z<=n;++z){const double tangent=tangentLog(theta,double(z)/n);double value=0;
        const double cuts[5]={0,.8,.82,.975,end};
        for(unsigned band=1;band<5;++band){const double left=std::min(cuts[band-1],end),right=std::min(cuts[band],end);if(!(right>left))continue;
          constexpr unsigned nr=128;const double step=(right-left)/nr;double sum=0;
          for(unsigned k=0;k<=nr;++k){const double r=left+step*k;sum+=(k==0||k==nr?1:k%2?4:2)*r*radial(r,tangent);}value+=step*sum/3;}
        axial+=(z==0||z==n?1:z%2?4:2)*value;}
      total+=(j==0||j==n?1:j%2?4:2)*axial/(3*n);}
    return h*total/3;};
  // Exercise the actual cyclic coarse-mesh sectors, including the periodic seam and reversed source orientation.
  for(unsigned sector=1;sector<8;++sector) {
    const double theta=sector*angle,next=((sector+1)%8)*angle;
    const P b{std::cos(theta),std::sin(theta),0},d{std::cos(next),std::sin(next),0};
    const P B{b[0],b[1],1},D{d[0],d[1],1};
    for(T cell:std::array<T,3>{{{a,b,d,D},{a,b,B,D},{a,B,A,D}}}) {
      std::swap(cell[0],cell[1]);  // A wall vertex first exercises source permutations and projection-frame independence.
      const auto rule=layers.IntegrationRule3D(cell,1.5,[](const auto&){return 1.;});double volume=0;
      for(const auto& p:rule){REQUIRE(p.weight>0);for(double v:p.barycentric)REQUIRE(v>=-1e-14);volume+=p.weight;}
      CHECK(volume==Approx(s/6).epsilon(2e-10));
    }
  }
  const double coarse=reference(64),fine=reference(128);CHECK(coarse==Approx(fine).epsilon(2e-6));
  INFO("points="<<points<<" native="<<measured<<" independent_polar="<<fine);CHECK(measured==Approx(fine).epsilon(8e-4));
  std::cout<<"CYLINDER_VOLUME points="<<points<<" native="<<std::setprecision(17)<<measured<<" independent_polar="<<fine<<" reference_delta="<<std::abs(coarse-fine)<<'\n';

}

TEST_CASE("Planar wall preserves a finer rotated tangent field across its source diagonal", "[NativePlanarTangentEdge3D]") {
  using namespace SU2Native3D;
  using Tensor = SU2Native3D::Tensor;
  CBoundaryLayerMetric::Wall wall;wall.layer={"wall",.1,1.2,.3};
  wall.coord={0,0,0,1,0,0,1,1,0,0,1,0};wall.conn={0,1,2,2,0,2,3,3};
  CBoundaryLayerMetric layers(3,{wall});
  const Tensor sensor{193,.25*std::sqrt(3.)*252,0,67,0,4};
  const Tensor expected{sensor.xx,sensor.xy,0,sensor.yy,0,100};
  const Point a{.235294117708690464497,0,0},b{0,.299999999930150806904,0};
  double maximum_error=0;size_t queries=0;Point worst{};Tensor worst_tensor{};
  const auto compose=[&](Point p,Tensor input) {
    ++queries;CBoundaryLayerMetric::Tensor m;
    m.m[0][0]=input.xx;m.m[0][1]=m.m[1][0]=input.xy;m.m[0][2]=m.m[2][0]=input.xz;
    m.m[1][1]=input.yy;m.m[1][2]=m.m[2][1]=input.yz;m.m[2][2]=input.zz;
    const su2double x[3]={p.x,p.y,p.z};layers.ApplyPoint(x,m,4,nullptr,false);
    const Tensor result{SU2_TYPE::GetValue(m.m[0][0]),SU2_TYPE::GetValue(m.m[0][1]),SU2_TYPE::GetValue(m.m[0][2]),
        SU2_TYPE::GetValue(m.m[1][1]),SU2_TYPE::GetValue(m.m[1][2]),SU2_TYPE::GetValue(m.m[2][2])};
    const double error=std::max({std::abs(result.xx-expected.xx),std::abs(result.xy-expected.xy),std::abs(result.xz),
        std::abs(result.yy-expected.yy),std::abs(result.yz),std::abs(result.zz-expected.zz)});
    if(error>maximum_error){maximum_error=error;worst=p;worst_tensor=result;}
    return result;
  };
  const double crossing=a.x/(a.x+b.y);
  for(unsigned k=0;k<64;++k)for(double sign:{-1.,1.}) {
    const double t=crossing+sign*std::ldexp(.1,-int(k));
    compose({(1-t)*a.x+t*b.x,(1-t)*a.y+t*b.y,0},sensor);
  }
  DonorCell donor;donor.cell={0,{{{0,{0,0,0}},{1,{1,0,0}},{2,{0,1,0}},{3,{0,0,1}}}}};donor.sensor.fill(sensor);
  FrozenField field({donor},compose,false,{},[&](Point p,Point q){return layers.EdgeBreakpoints({p.x,p.y,p.z},{q.x,q.y,q.z});});
  long double length=0;std::string failure;
  try {length=field.TargetEdgeLength(a,b);}catch(const std::exception& e){failure=e.what();}
  std::cout<<std::setprecision(21)<<"PLANAR_TANGENT_EDGE length="<<length<<" exact="<<MetricNorm(a,b,expected)
      <<" queries="<<queries<<" maximum_tensor_error="<<maximum_error<<" worst=("<<worst.x<<','<<worst.y<<','<<worst.z
      <<") tensor="<<worst_tensor.xx<<','<<worst_tensor.xy<<','<<worst_tensor.xz<<','<<worst_tensor.yy<<','<<worst_tensor.yz<<','<<worst_tensor.zz<<std::endl;
  INFO(failure);CHECK(failure.empty());CHECK(maximum_error<2e-10);
  if(failure.empty())CHECK(std::abs(length-MetricNorm(a,b,expected))<1e-9L);
}


#include "NativeCorrectionReplay3D.hpp"
#include "../../../Common/include/adaptation/CNativeSupport3D.hpp"

TEST_CASE("Replay a complete faceted wall volume-integration input without CFD", "[.][NativeFacetedVolumeReplay3D]") {
  const char* path=std::getenv("SU2_NATIVE_BL_REPLAY");REQUIRE(path!=nullptr);
  std::ifstream input(path);std::string magic;unsigned version=0;size_t points=0,faces=0;
  input>>magic>>version>>points>>faces;
  REQUIRE(magic=="NATIVE_BL_REPLAY_3D");REQUIRE((version==1 || version==2));
  REQUIRE(points>0);REQUIRE(points<=1000000);REQUIRE(faces>0);REQUIRE(faces<=1000000);
  CBoundaryLayerMetric::Wall wall;wall.layer.marker="replay_wall";
  input>>wall.layer.firstHeight>>wall.layer.growth>>wall.layer.thickness;
  wall.coord.resize(3*points);for(auto& x:wall.coord)input>>x;
  for(size_t k=0;k<faces;++k) {
    unsigned long a,b,c;input>>a>>b>>c;REQUIRE(a<points);REQUIRE(b<points);REQUIRE(c<points);
    wall.conn.insert(wall.conn.end(),{a,b,c,c});
  }
  std::array<std::array<double,3>,4> cell;
  for(auto& p:cell)for(auto& x:p)input>>x;
  double core=1;std::array<Tensor,4> sensor{};
  if(version==2) {
    input>>core;REQUIRE(std::isfinite(core));REQUIRE(core>0);
    for(auto& m:sensor) {
      for(auto& row:m.m)for(auto& value:row)input>>value;
      double scale=0;for(const auto& row:m.m)for(auto value:row)scale=std::max(scale,std::abs(SU2_TYPE::GetValue(value)));
      for(unsigned i=0;i<3;++i)for(unsigned j=0;j<3;++j) {
        REQUIRE(std::isfinite(SU2_TYPE::GetValue(m.m[i][j])));
        // Preserve recorded tensors, including spectral-recomposition roundoff asymmetry.
        REQUIRE(std::abs(m.m[i][j]-m.m[j][i])<=64*std::numeric_limits<double>::epsilon()*scale);
      }
      REQUIRE_NOTHROW(SU2Native3D::ValidateTensor({m.m[0][0],m.m[0][1],m.m[0][2],m.m[1][1],m.m[1][2],m.m[2][2]}));
    }
  }
  REQUIRE(bool(input));input>>std::ws;REQUIRE(input.eof());
  CBoundaryLayerMetric layers(3,{std::move(wall)});
  if(const char* countText=std::getenv("SU2_NATIVE_BL_PREPARE_CORRECTION")) {
    REQUIRE(SU2_MPI::GetSize()==1);
    const size_t count=std::stoull(countText);const uint64_t seed=std::getenv("SU2_NATIVE_BL_PROPOSAL_SEED")?std::stoull(std::getenv("SU2_NATIVE_BL_PROPOSAL_SEED")):2641;
    auto sampler=layers.PrepareCorrectionSupport3D(size_t(2)<<30,80000000);const auto queries=sampler->Draw(count,seed);
    std::ofstream output("native_queries.bin",std::ios::binary),detail("native_queries.csv");REQUIRE(bool(output));REQUIRE(bool(detail));
    const uint64_t magic=0x4e41545155455259ULL,n=queries.size();output.write(reinterpret_cast<const char*>(&magic),8);output.write(reinterpret_cast<const char*>(&n),8);
    detail<<std::setprecision(17)<<"x,y,z,inverse_pdf,kind\n";
    for(const auto& q:queries) {
      const double record[4]={q.point.x,q.point.y,q.point.z,q.inverse_pdf};output.write(reinterpret_cast<const char*>(record),sizeof(record));
      detail<<q.point.x<<','<<q.point.y<<','<<q.point.z<<','<<q.inverse_pdf<<','<<q.kind<<'\n';
    }
    output.close();detail.close();REQUIRE(bool(output));REQUIRE(bool(detail));
    const auto& stats=sampler->Statistics();std::ofstream result("native_proposal.json");result<<std::setprecision(17)
      <<"{\"samples\":"<<queries.size()<<",\"seed\":"<<seed<<",\"components\":["<<stats.components[0]<<','<<stats.components[1]<<','<<stats.components[2]
      <<"],\"preparation_work\":"<<stats.preparation_work<<",\"pdf_candidates\":"<<stats.pdf_candidates<<",\"retained_bytes\":"<<stats.retained_bytes
      <<",\"preparation_seconds\":"<<stats.preparation_seconds<<",\"pdf_seconds\":"<<stats.pdf_seconds<<",\"proven_error_bound\":false}\n";REQUIRE(bool(result));return;
  }
  if(const char* domain=std::getenv("SU2_NATIVE_BL_CELL_CARRIER")) {
    const uint64_t count=std::getenv("SU2_NATIVE_BL_CARRIER_SAMPLES")?std::stoull(std::getenv("SU2_NATIVE_BL_CARRIER_SAMPLES")):8192;
    const uint64_t seed=std::getenv("SU2_NATIVE_BL_PROPOSAL_SEED")?std::stoull(std::getenv("SU2_NATIVE_BL_PROPOSAL_SEED")):20261017;
    REQUIRE_NOTHROW(SU2NativeBLReplay::PrepareCellCarrier(layers,domain,count,seed));return;
  }
  if(const char* domain=std::getenv("SU2_NATIVE_BL_DYNAMIC_DOMAIN")) {
    REQUIRE_NOTHROW(SU2NativeBLReplay::DynamicCorrection(layers,domain));return;
  }
  if(const char* domain=std::getenv("SU2_NATIVE_BL_CORRECTION_DOMAIN")) {
    const char* queries=std::getenv("SU2_NATIVE_BL_CORRECTION_QUERIES");REQUIRE(queries);
    REQUIRE_NOTHROW(SU2NativeBLReplay::Correction(layers,domain,queries));return;
  }
  if(const char* queries=std::getenv("SU2_NATIVE_BL_WALL_PRISMS")) {
    // Disjoint original-cell subsets give a lower mass; this fixture never normalizes or certifies a mesh.
    std::ifstream stream(queries,std::ios::binary|std::ios::ate);REQUIRE(bool(stream));
    const auto bytes=static_cast<std::streamoff>(stream.tellg());stream.seekg(0);uint64_t count=0;
    stream.read(reinterpret_cast<char*>(&count),8);REQUIRE(count>0);REQUIRE(count<=262144);
    REQUIRE(bytes==std::streamoff(8+count*(8+16*8)));
    const auto rank=SU2_MPI::GetRank(),ranks=SU2_MPI::GetSize();
    std::ofstream values("wall_prism_rank"+std::to_string(rank)+".csv");REQUIRE(bool(values));
    values<<std::setprecision(17)<<"row,cell,inside,strong,density,inverse_pdf,log_spectral_relative_difference\n";
    SU2Native3D::KernelStats predicates;
    const char* capText=std::getenv("SU2_NATIVE_BL_PRISM_CAP");
    const double cap=capText?std::stod(capText):1.;REQUIRE(std::isfinite(cap));REQUIRE(cap>0);
    long double localMass[2]={},globalMass[2]={};int localFailure=0,globalFailure=0;
    try {for(uint64_t row=0;row<count;++row) {
      uint64_t id=0;double data[16];stream.read(reinterpret_cast<char*>(&id),8);
      stream.read(reinterpret_cast<char*>(data),sizeof(data));REQUIRE(bool(stream));
      if(row%ranks!=uint64_t(rank))continue;
      for(double x:data)REQUIRE(std::isfinite(x));REQUIRE(data[15]>0);
      SU2Native3D::Tetrahedron source;
      for(unsigned k=0;k<4;++k)source[k]={data[3*k],data[3*k+1],data[3*k+2]};
      const auto orientation=SU2Native3D::Orientation(source[0],source[1],source[2],source[3],&predicates);
      REQUIRE(orientation!=0);if(orientation<0)std::swap(source[0],source[1]);
      const SU2Native3D::Point point{data[12],data[13],data[14]};std::array<long double,4> bary{};
      const bool inside=SU2Native3D::Barycentric(source,point,bary,&predicates);
      bool strong=false;double density=0,difference=0;
      if(inside) {
        const auto sample=layers.Evaluate(0,data+12,core);strong=sample.weight==1;
        if(strong) {
          double trace=0;for(unsigned k=0;k<3;++k)trace+=SU2_TYPE::GetValue(sample.logarithm.m[k][k]);
          density=std::exp(.5*trace);REQUIRE(std::isfinite(density));REQUIRE(density>0);
          su2double vectors[3][3],eigenvalues[3],workspace[3];auto metric=sample.full;
          CBlasStructure::EigenDecomposition(metric.m,vectors,eigenvalues,3,workspace);
          double logDet=0;for(auto v:eigenvalues){REQUIRE(v>0);logDet+=std::log(SU2_TYPE::GetValue(v));}
          difference=std::abs(std::expm1(.5*logDet-.5*trace));
        }
      }
      const long double raw=(long double)density*data[15];REQUIRE(std::isfinite(raw));
      localMass[0]+=std::min(raw,(long double)cap);++localMass[1];
      values<<row<<','<<id<<','<<inside<<','<<strong<<','<<density<<','<<data[15]<<','<<difference<<'\n';
    }
    values.close();REQUIRE(bool(values));
    }catch(...){localFailure=1;}
    SU2_MPI::Allreduce(&localFailure,&globalFailure,1,MPI_INT,MPI_MAX,SU2_MPI::GetComm());REQUIRE(globalFailure==0);
    if(capText) {
      const char* deltaText=std::getenv("SU2_NATIVE_BL_PRISM_DELTA");REQUIRE(deltaText);
      const char* targetText=std::getenv("SU2_NATIVE_BL_PRISM_TARGET");REQUIRE(targetText);
      const double delta=std::stod(deltaText),target=std::stod(targetText);
      REQUIRE(delta>0);REQUIRE(delta<1);REQUIRE(std::isfinite(target));REQUIRE(target>0);
      SU2_MPI::Allreduce(localMass,globalMass,2,MPI_LONG_DOUBLE,MPI_SUM,SU2_MPI::GetComm());
      REQUIRE(uint64_t(globalMass[1])==count);
      const double mean=double(globalMass[0]/count),radius=cap*std::sqrt(std::log(1/delta)/(2*count));
      const double minimum=std::max(0.,mean-radius);
      // Only the strong BL subset is known here. Remaining positive mass stays explicitly unknown.
      const auto normal=SU2Complexity::Normalize(target,-std::log(64.),std::log(64.),0,.01,24,
          [&](double,double){return SU2Complexity::Estimate{mean,INFINITY,minimum};});
      if(!rank) {
        const char* statuses[]={"estimated_tolerance","integration_limited","outside_bracket","trial_limit"};
        std::ofstream out("prism_global_controller.json");out<<std::setprecision(17)
          <<"{\"samples\":"<<count<<",\"ranks\":"<<ranks<<",\"mean_lower_contribution\":"<<mean
          <<",\"statistical_radius\":"<<radius<<",\"minimum\":"<<minimum<<",\"target\":"<<target
          <<",\"status\":\""<<statuses[unsigned(normal.status)]<<"\",\"log_scale\":"<<normal.logScale
          <<",\"scale_trials\":"<<normal.trials<<",\"delta\":"<<delta
          <<",\"upper_mass_unknown\":true,\"full_mass_integrated\":false,\"new_queries_during_scale_search\":0"
          <<",\"scale_independent_strong_BL_subset_only\":true,\"machine_bound_proven\":false,\"production_normalization_changed\":false}\n";
        REQUIRE(bool(out));
      }
      CHECK(normal.status!=SU2Complexity::Status::EstimatedTolerance);
    }
    std::cout<<"WALL_PRISM_REPLAY rank="<<rank<<" samples="<<count<<" filtered="<<predicates.filtered_orientations
             <<" exact="<<predicates.exact_orientations<<" mesh_acceptance=false\n";
    return;
  }
  auto targetDensity=[&](const CBoundaryLayerMetric::IntegrationPoint3D& point) {
    if(version==1) {
      double value=1;
      for(const auto& sample:point.samples)value=std::max(value,std::pow(SU2_TYPE::GetValue(sample.sample.hn),-SU2_TYPE::GetValue(sample.sample.weight)));
      return value;
    }
    Tensor metric;
    for(unsigned k=0;k<4;++k)for(unsigned i=0;i<3;++i)for(unsigned j=0;j<3;++j)
      metric.m[i][j]+=point.barycentric[k]*sensor[k].m[i][j];
    for(const auto& sample:point.samples)layers.ApplySample(sample.wall,sample.sample,metric,core,nullptr,false);
    su2double vectors[3][3],values[3],workspace[3];
    CBlasStructure::EigenDecomposition(metric.m,vectors,values,3,workspace);
    return SU2_TYPE::GetValue(std::sqrt(values[0]*values[1]*values[2]));
  };
  if(const char* edgePath=std::getenv("SU2_NATIVE_BL_EDGE")) {
    using namespace SU2Native3D;
    REQUIRE(version==2);
    const char* originalPath=std::getenv("SU2_NATIVE_BL_EDGE_SENSOR");REQUIRE(originalPath);
    std::ifstream original(originalPath,std::ios::binary);uint64_t count=0;
    original.read(reinterpret_cast<char*>(&count),8);REQUIRE(count>0);REQUIRE(count<=1000000);
    struct OriginalNode { Point p; SU2Native3D::Tensor sensor; };
    std::map<Id,OriginalNode> nodes;
    double originalCore=std::numeric_limits<double>::max();
    for(uint64_t k=0;k<count;++k) {
      uint64_t id;double x[3],m[9];original.read(reinterpret_cast<char*>(&id),8);
      original.read(reinterpret_cast<char*>(x),sizeof(x));original.read(reinterpret_cast<char*>(m),sizeof(m));
      REQUIRE(bool(original));
      for(double v:x)REQUIRE(std::isfinite(v));for(double v:m)REQUIRE(std::isfinite(v));
      REQUIRE(nodes.emplace(id,OriginalNode{{x[0],x[1],x[2]},{m[0],m[1],m[2],m[4],m[5],m[8]}}).second);
      su2double matrix[3][3]={{m[0],m[1],m[2]},{m[1],m[4],m[5]},{m[2],m[5],m[8]}},vectors[3][3],values[3],workspace[3];
      CBlasStructure::EigenDecomposition(matrix,vectors,values,3,workspace);
      const double smallest=SU2_TYPE::GetValue(*std::min_element(values,values+3));
      REQUIRE(std::isfinite(smallest));REQUIRE(smallest>0);originalCore=std::min(originalCore,smallest);
    }
    REQUIRE(original.peek()==std::char_traits<char>::eof());
    // Match the remesher's owner-UPPER6 core, rather than the volume replay's raw9 host representation.
    std::ifstream edge(edgePath);std::string edgeMagic;Point a,b;size_t candidates=0;
    edge>>edgeMagic>>a.x>>a.y>>a.z>>b.x>>b.y>>b.z>>candidates;
    REQUIRE(edgeMagic=="NATIVE_BL_EDGE_3D");REQUIRE(candidates>0);REQUIRE(candidates<=MaximumDonors);
    std::vector<DonorCell> donors;
    for(size_t k=0;k<candidates;++k) {
      DonorCell donor;edge>>donor.cell.id;
      for(unsigned j=0;j<4;++j) {
        Id id;edge>>id;const auto found=nodes.find(id);REQUIRE(found!=nodes.end());
        donor.cell.v[j]={id,found->second.p};donor.sensor[j]=found->second.sensor;
      }
      auto& v=donor.cell.v;const auto orientation=Orientation(v[0].p,v[1].p,v[2].p,v[3].p);
      REQUIRE(orientation!=0);
      if(orientation<0){std::swap(v[0],v[1]);std::swap(donor.sensor[0],donor.sensor[1]);}
      if(IntersectsSegment({v[0].p,v[1].p,v[2].p,v[3].p},a,b))donors.push_back(donor);
    }
    REQUIRE_FALSE(donors.empty());
    size_t sampleCount=0;edge>>sampleCount;REQUIRE(sampleCount<=65536);
    std::vector<double> samples(sampleCount);for(auto& t:samples){edge>>t;REQUIRE(std::isfinite(t));REQUIRE(t>=0);REQUIRE(t<=1);}
    REQUIRE(bool(edge));edge>>std::ws;REQUIRE(edge.eof());
    std::ofstream support("edge_donors.csv");support<<"cell,node0,node1,node2,node3\n";
    for(const auto& d:donors){support<<d.cell.id;for(const auto& n:d.cell.v)support<<','<<n.id;support<<'\n';}
    REQUIRE(bool(support));support.close();
    const auto compose=[&](Point p,SU2Native3D::Tensor input) {
      CBoundaryLayerMetric::Tensor m;
      m.m[0][0]=input.xx;m.m[0][1]=m.m[1][0]=input.xy;m.m[0][2]=m.m[2][0]=input.xz;
      m.m[1][1]=input.yy;m.m[1][2]=m.m[2][1]=input.yz;m.m[2][2]=input.zz;
      const su2double x[3]={p.x,p.y,p.z};layers.ApplyPoint(x,m,originalCore,nullptr,false);
      return SU2Native3D::Tensor{m.m[0][0],m.m[0][1],m.m[0][2],m.m[1][1],m.m[1][2],m.m[2][2]};
    };
    std::sort(donors.begin(),donors.end(),[](const DonorCell& a,const DonorCell& b){return a.Key()<b.Key();});
    std::vector<const Cell*> source;for(const auto& donor:donors)source.push_back(&donor.cell);
    const auto intervals=TraceSegment(source,a,b);
    std::vector<double> cuts;
    FrozenField target(donors,compose,true,{},[&](Point p,Point q) {
      cuts=layers.EdgeBreakpoints({p.x,p.y,p.z},{q.x,q.y,q.z});return cuts;
    });
    long double length=0;std::string failure;const auto start=std::chrono::steady_clock::now();
    try{length=target.TargetEdgeLength(a,b);}catch(const std::exception& e){failure=e.what();}
    const auto seconds=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();
    const auto stats=target.Statistics();
    std::ofstream cutFile("edge_cuts.txt");cutFile<<std::setprecision(17);for(double t:cuts)cutFile<<t<<'\n';REQUIRE(bool(cutFile));
    std::ofstream values("edge_samples.csv");values<<std::setprecision(17)<<"t,norm,xx,xy,xz,yy,yz,zz,distance,nx,ny,nz\n";
    for(double t:samples) {
      const long double u=t;const Point p{double((1-u)*a.x+u*b.x),double((1-u)*a.y+u*b.y),double((1-u)*a.z+u*b.z)};
      // Match certified edge-trace semantics: a rounded coordinate can leave the original volume near a wall.
      const auto interval=std::find_if(intervals.begin(),intervals.end(),[&](const SegmentPiece& piece){return u>=piece.begin && u<=piece.end;});
      REQUIRE(interval!=intervals.end());const auto along=(u-interval->begin)/interval->width;
      std::array<long double,6> components{};
      for(unsigned k=0;k<4;++k) {
        const auto& tensor=donors[interval->donor].sensor[k];
        const double entry[6]={tensor.xx,tensor.xy,tensor.xz,tensor.yy,tensor.yz,tensor.zz};
        const auto weight=(1-along)*interval->begin_weights[k]+along*interval->end_weights[k];
        for(unsigned j=0;j<6;++j)components[j]+=weight*entry[j];
      }
      const auto m=compose(p,{double(components[0]),double(components[1]),double(components[2]),double(components[3]),double(components[4]),double(components[5])});
      const su2double x[3]={p.x,p.y,p.z};const auto wallSample=layers.Evaluate(0,x,originalCore);
      values<<t<<','<<MetricNorm(a,b,m)<<','<<m.xx<<','<<m.xy<<','<<m.xz<<','<<m.yy<<','<<m.yz<<','<<m.zz
          <<','<<wallSample.distance<<','<<wallSample.normal[0]<<','<<wallSample.normal[1]<<','<<wallSample.normal[2]<<'\n';
    }
    REQUIRE(bool(values));
    std::cout<<std::setprecision(21)<<"FROZEN_EDGE length="<<length<<" failure="<<std::quoted(failure)
        <<" core="<<originalCore<<" core_raw9="<<core<<" candidates="<<candidates<<" donors="<<stats.maximum_edge_candidates
        <<" pieces="<<stats.edge_pieces<<" cuts="<<cuts.size()<<" compositions="<<stats.edge_compositions
        <<" seconds="<<seconds<<" geometry_seconds="<<stats.edge_geometry_seconds
        <<" composition_seconds="<<stats.edge_composition_seconds<<std::endl;
    INFO(failure);CHECK(failure.empty());return;
  }
  struct ReplayEstimate {
    double value=0,indicator=0,preparation=0,density=0;
    size_t evaluations=0,panels=0,limited=0,unresolved=0,subcells=0,chords=0;
  };
  const double absoluteTolerance=std::getenv("SU2_NATIVE_BL_ABSOLUTE")?std::stod(std::getenv("SU2_NATIVE_BL_ABSOLUTE")):-1;
  auto estimateSubcells=[&](size_t budget,unsigned depth) {
    if(depth>6)throw std::invalid_argument("Frozen integration subdivision depth exceeds6.");
    using Bary=std::array<double,4>;using Subcell=std::array<Bary,4>;
    std::vector<Subcell> parts{Subcell{{{{1,0,0,0}},{{0,1,0,0}},{{0,0,1,0}},{{0,0,0,1}}}}};
    auto coordinates=[&](const Subcell& part) {
      std::array<std::array<double,3>,4> x{};
      for(unsigned k=0;k<4;++k)for(unsigned j=0;j<3;++j) {
        long double v=cell[0][j];for(unsigned a=1;a<4;++a)v+=(long double)part[k][a]*((long double)cell[a][j]-cell[0][j]);x[k][j]=double(v);
      }
      return x;
    };
    for(unsigned d=0;d<depth;++d) {
      std::vector<Subcell> next;
      for(const auto& part:parts) {
        const auto x=coordinates(part);unsigned a=0,b=1;double longest=0;
        for(unsigned i=0;i<4;++i)for(unsigned j=i+1;j<4;++j) {
          double length=0;for(unsigned k=0;k<3;++k)length+=(x[i][k]-x[j][k])*(x[i][k]-x[j][k]);
          if(length>longest){longest=length;a=i;b=j;}
        }
        Bary mid{};for(unsigned k=0;k<4;++k)mid[k]=.5*(part[a][k]+part[b][k]);
        auto left=part,right=part;left[a]=mid;right[b]=mid;next.push_back(left);next.push_back(right);
      }
      parts=std::move(next);
    }
    ReplayEstimate total;total.subcells=parts.size();
    for(const auto& part:parts) {
      const auto x=coordinates(part);
      auto originalDensity=[&](const CBoundaryLayerMetric::IntegrationPoint3D& query) {
        auto original=query;original.barycentric={};
        for(unsigned k=0;k<4;++k)for(unsigned j=0;j<4;++j)original.barycentric[k]+=query.barycentric[j]*part[j][k];
        return targetDensity(original);
      };
      const auto result=layers.EstimateComplexity3D(x,originalDensity,budget,absoluteTolerance);
      total.value+=result.value;total.indicator+=result.indicator;total.evaluations+=result.evaluations;total.panels+=result.panels;
      total.limited+=result.limited;total.unresolved+=result.unresolvedSupport;
      total.preparation+=result.preparationSeconds;total.density+=result.densitySeconds;total.chords+=result.preparedChords;
    }
    return total;
  };
  if(const char* records=std::getenv("SU2_NATIVE_BL_FIELD")) {
    REQUIRE(version==2);std::ifstream field(records,std::ios::binary|std::ios::ate);REQUIRE(bool(field));
    const auto bytes=static_cast<std::streamoff>(field.tellg());field.seekg(0);uint64_t count=0;
    field.read(reinterpret_cast<char*>(&count),sizeof(count));REQUIRE(count>0);REQUIRE(count<=1000000);
    REQUIRE(bytes==std::streamoff(8+count*(8+48*8)));
    const size_t budget=std::getenv("SU2_NATIVE_BL_COMPLEXITY")?std::stoul(std::getenv("SU2_NATIVE_BL_COMPLEXITY")):8192;
    const auto rank=SU2_MPI::GetRank(),ranks=SU2_MPI::GetSize();
    if(const char* coarseValue=std::getenv("SU2_NATIVE_BL_VOLUME_SEED")) {
      // Cheap complete-field audit: agreement between rules is not certification of thin-support coverage.
      const double coarseEigenvalue=std::stod(coarseValue);REQUIRE(std::isfinite(coarseEigenvalue));REQUIRE(coarseEigenvalue>0);
      struct SeedPoint {std::array<double,4> bary;double weight;};
      std::vector<SeedPoint> coarseRule,fineRule;
      const double a=(5+3*std::sqrt(5.))/20,b=(5-std::sqrt(5.))/20;
      for(unsigned i=0;i<4;++i){std::array<double,4> beta{{b,b,b,b}};beta[i]=a;coarseRule.push_back({beta,.25});}
      const double x3[3]={.5*(1-.77459666924148337704),.5,.5*(1+.77459666924148337704)},w3[3]={5./18,4./9,5./18};
      const double x2[2]={.5*(1-.57735026918962576451),.5*(1+.57735026918962576451)};
      for(unsigned i=0;i<3;++i)for(unsigned j=0;j<3;++j)for(unsigned k=0;k<2;++k) {
        const double u=x3[i],v=x3[j],w=x2[k];
        fineRule.push_back({{1-u,u*(1-v),u*v*(1-w),u*v*w},3*u*u*v*w3[i]*w3[j]});
      }
      std::set<uint64_t> watched;
      if(const char* list=std::getenv("SU2_NATIVE_BL_SEED_WATCH")) {
        std::istringstream ids(list);uint64_t id=0;while(ids>>id){watched.insert(id);if(ids.peek()==',')ids.get();}
        REQUIRE(ids.eof());
      }
      std::ofstream detail("seed_watch_rank"+std::to_string(rank)+".csv");
      detail<<std::setprecision(17)<<"cell,sensor_q4,composed_q4,sensor_q18,composed_q18,absolute_rule_difference,minimum_q4,minimum_q18\n";
      long double local[10]={},global[10]={};int failed=0,globalFailed=0;
      const auto begin=std::chrono::steady_clock::now();
      for(uint64_t row=0;row<count;++row) {
        uint64_t id=0;double coefficients[48];field.read(reinterpret_cast<char*>(&id),8);field.read(reinterpret_cast<char*>(coefficients),sizeof(coefficients));
        if(!field)failed=1;
        if(row%ranks==uint64_t(rank) && !failed)try {
          for(double x:coefficients)if(!std::isfinite(x))throw std::invalid_argument("Nonfinite original seed-field coefficient.");
          unsigned at=0;for(auto& p:cell)for(auto& x:p)x=coefficients[at++];
          for(auto& m:sensor)for(auto& r:m.m)for(auto& x:r)x=coefficients[at++];
          SU2Native3D::Tetrahedron source;for(unsigned k=0;k<4;++k)source[k]={cell[k][0],cell[k][1],cell[k][2]};
          const auto orientation=SU2Native3D::Orientation(source[0],source[1],source[2],source[3]);
          if(orientation==0)throw std::runtime_error("Degenerate original seed-field tetrahedron.");
          const long double volume=std::abs(orientation)/6;
          const bool reversed=orientation<0;if(reversed)std::swap(source[1],source[2]);
          double sums[2][3]={};
          for(unsigned order=0;order<2;++order)for(const auto& q:order?fineRule:coarseRule) {
            std::array<double,3> position{};
            for(unsigned j=0;j<3;++j) {
              long double x=cell[0][j];for(unsigned k=1;k<4;++k)x+=(long double)q.bary[k]*((long double)cell[k][j]-cell[0][j]);
              position[j]=double(x);
            }
            std::array<long double,4> actual;
            if(!SU2Native3D::Barycentric(source,{position[0],position[1],position[2]},actual))
              throw std::runtime_error("Rounded seed query leaves original donor.");
            if(reversed)std::swap(actual[1],actual[2]);
            CBoundaryLayerMetric::IntegrationPoint3D query{};for(unsigned k=0;k<4;++k)query.barycentric[k]=double(actual[k]);
            query.samples=layers.SamplePoints({position[0],position[1],position[2]});
            Tensor background,minimum;
            for(unsigned i=0;i<3;++i){minimum.m[i][i]=coarseEigenvalue;for(unsigned j=0;j<3;++j)
              for(unsigned k=0;k<4;++k)background.m[i][j]+=query.barycentric[k]*sensor[k].m[i][j];}
            auto density=[](Tensor metric) {
              su2double vectors[3][3],val[3],work[3];CBlasStructure::EigenDecomposition(metric.m,vectors,val,3,work);
              double logarithm=0;for(auto x:val){if(!(x>0))throw std::runtime_error("Nonpositive seed density eigenvalue.");logarithm+=std::log(SU2_TYPE::GetValue(x));}
              const double value=std::exp(.5*logarithm);if(!(value>0) || !std::isfinite(value))throw std::runtime_error("Invalid seed density.");return value;
            };
            for(const auto& sample:query.samples)layers.ApplySample(sample.wall,sample.sample,minimum,coarseEigenvalue,nullptr,false);
            sums[order][0]+=q.weight*density(background);sums[order][1]+=q.weight*targetDensity(query);
            sums[order][2]+=q.weight*density(minimum);++local[8];local[9]+=query.samples.size();
          }
          local[0]+=volume*sums[0][0];local[1]+=volume*sums[0][1];local[2]+=volume*sums[1][0];local[3]+=volume*sums[1][1];
          local[4]+=volume*std::abs(sums[1][1]-sums[0][1]);local[5]+=volume*sums[0][2];local[6]+=volume*sums[1][2];++local[7];
          if(watched.count(id))detail<<id<<','<<double(volume*sums[0][0])<<','<<double(volume*sums[0][1])<<','
            <<double(volume*sums[1][0])<<','<<double(volume*sums[1][1])<<','<<double(volume*std::abs(sums[1][1]-sums[0][1]))
            <<','<<double(volume*sums[0][2])<<','<<double(volume*sums[1][2])<<'\n';
        }catch(const std::exception& error){failed=1;std::cerr<<"VOLUME_SEED_FAILURE cell="<<id<<" rank="<<rank<<" reason="<<error.what()<<std::endl;}
        if((row+1)%4096==0 || row+1==count) {
          SU2_MPI::Allreduce(&failed,&globalFailed,1,MPI_INT,MPI_MAX,SU2_MPI::GetComm());if(globalFailed)break;
          if(!rank && ((row+1)%32768==0 || row+1==count))std::cout<<"VOLUME_SEED_PROGRESS completed_input_rows="<<row+1<<'/'<<count<<std::endl;
        }
      }
      detail.close();if(!detail)failed=1;
      SU2_MPI::Allreduce(&failed,&globalFailed,1,MPI_INT,MPI_MAX,SU2_MPI::GetComm());REQUIRE(globalFailed==0);
      SU2_MPI::Allreduce(local,global,10,MPI_LONG_DOUBLE,MPI_SUM,SU2_MPI::GetComm());
      REQUIRE(uint64_t(global[7])==count);REQUIRE(uint64_t(global[8])==22*count);
      if(!rank) {
        const char* keys[]={"sensor_q4","composed_q4","sensor_q18","composed_q18","sum_absolute_cell_rule_difference","minimum_q4","minimum_q18","cells","query_points","wall_samples"};
        std::ofstream out("volume_seed_global.json");out<<std::setprecision(21)<<'{';
        for(unsigned k=0;k<10;++k)out<<(k?",":"")<<'\"'<<keys[k]<<"\":"<<global[k];
        out<<",\"coarse_sensor_eigenvalue\":"<<coarseEigenvalue<<",\"ranks\":"<<ranks
           <<",\"seconds\":"<<std::chrono::duration<double>(std::chrono::steady_clock::now()-begin).count()
           <<",\"complete_field_coverage\":true,\"thin_BL_accuracy_certified\":false,\"production_normalization_changed\":false}\n";
        REQUIRE(bool(out));
      }
      return;
    }
    if(const char* requestedTarget=std::getenv("SU2_NATIVE_BL_NORMALIZE")) {
      struct FrozenCell {
        uint64_t id=0;
        decltype(cell) coordinates;
        decltype(sensor) originalSensor;
        std::shared_ptr<CBoundaryLayerMetric::ComplexityWorkspace3D> workspace;
        CBoundaryLayerMetric::ComplexityEstimate3D estimate;
        bool spent=false;
      };
      std::vector<FrozenCell> owned;
      const auto slots=std::max<uint64_t>(1,(count+ranks-1)/ranks);
      const auto bytesPerCell=std::min<size_t>(size_t(4)<<20,(size_t(512)<<20)/slots);
      int failure=0,globalFailure=0;unsigned reductions=0;
      for(uint64_t row=0;row<count;++row) {
        uint64_t id;double coefficients[48];field.read(reinterpret_cast<char*>(&id),8);
        field.read(reinterpret_cast<char*>(coefficients),sizeof(coefficients));REQUIRE(bool(field));
        if(row%ranks!=uint64_t(rank))continue;
        FrozenCell item;item.id=id;unsigned at=0;
        for(auto& p:item.coordinates)for(auto& x:p)x=coefficients[at++];
        for(auto& m:item.originalSensor)for(auto& r:m.m)for(auto& v:r)v=coefficients[at++];
        try {item.workspace=layers.PrepareComplexity3D(item.coordinates,bytesPerCell);}
        catch(const std::exception& e){failure=1;std::cerr<<"NORMALIZATION_INPUT cell="<<id<<" reason="<<e.what()<<std::endl;}
        owned.push_back(std::move(item));
      }
      SU2_MPI::Allreduce(&failure,&globalFailure,1,MPI_INT,MPI_MAX,SU2_MPI::GetComm());++reductions;
      REQUIRE(globalFailure==0);
      const double originalCore=core,target=std::stod(requestedTarget);
      const double rtol=std::getenv("SU2_NATIVE_BL_NORMALIZE_RTOL")?std::stod(std::getenv("SU2_NATIVE_BL_NORMALIZE_RTOL")):.01;
      // Empirical guard for the saved reference samples, NOT a proven global error multiplier.
      constexpr double guard=4;
      double lastScale=NAN,localStats[6]={};
      std::ofstream trace;
      if(rank==0){trace.open("normalization_trace.csv");trace<<std::setprecision(17)<<"log_scale,requested_absolute,value,raw_indicator,priority_rounds\n";}
      auto assess=[&](double logScale,double requested) {
        const bool fresh=logScale!=lastScale;
        const double scale=std::exp(logScale);core=originalCore*scale;
        auto update=[&](FrozenCell& item,double allocation) {
          cell=item.coordinates;sensor=item.originalSensor;
          for(auto& m:sensor)for(auto& r:m.m)for(auto& v:r)v*=scale;
          try {
            item.estimate=layers.EstimateComplexity3D(cell,targetDensity,budget,allocation,item.workspace.get());
            const auto& e=item.estimate;
            localStats[0]+=e.evaluations;localStats[1]+=e.preparationSeconds;localStats[2]+=e.densitySeconds;
            ++localStats[3];localStats[4]+=e.preparedChords;localStats[5]+=e.reusedChords;
          }catch(const std::exception& error) {
            failure=1;std::cerr<<"NORMALIZATION_FIELD cell="<<item.id<<" reason="<<error.what()<<std::endl;
          }
        };
        if(fresh) {
          lastScale=logScale;
          for(auto& item:owned){update(item,1e300);item.spent=item.estimate.limited;}
        }
        auto reduce=[&]() {
          SU2_MPI::Allreduce(&failure,&globalFailure,1,MPI_INT,MPI_MAX,SU2_MPI::GetComm());++reductions;
          if(globalFailure)throw std::runtime_error("Collective frozen-field normalization evaluation failed.");
          double local[2]={},global[2];
          for(const auto& item:owned){local[0]+=item.estimate.value;local[1]+=item.estimate.indicator;}
          SU2_MPI::Allreduce(local,global,2,MPI_DOUBLE,MPI_SUM,SU2_MPI::GetComm());++reductions;
          return SU2Complexity::Estimate{global[0],global[1]};
        };
        auto global=reduce();unsigned rounds=0;
        // A coarse interval suffices to bracket the target; only ambiguous trials need accuracy work.
        while(guard*global.uncertainty>requested &&
              std::abs(global.value-target)<=guard*global.uncertainty && rounds<8) {
          double maximum=0,globalMaximum=0;
          for(const auto& item:owned)if(!item.spent)maximum=std::max(maximum,item.estimate.indicator);
          SU2_MPI::Allreduce(&maximum,&globalMaximum,1,MPI_DOUBLE,MPI_MAX,SU2_MPI::GetComm());++reductions;
          if(!(globalMaximum>0) || (std::isinf(global.uncertainty) && !std::isinf(globalMaximum)))break;
          for(auto& item:owned)if(!item.spent && SU2Complexity::SelectError(item.estimate.indicator,globalMaximum)) {
            update(item,0);item.spent=true;
          }
          ++rounds;global=reduce();
        }
        if(rank==0)trace<<logScale<<','<<requested<<','<<global.value<<','<<global.uncertainty<<','<<rounds<<std::endl;
        return SU2Complexity::Estimate{global.value,guard*global.uncertainty};
      };
      const auto start=std::chrono::steady_clock::now();
      const auto result=SU2Complexity::Normalize(target,-std::log(64.),std::log(64.),0,rtol,24,assess);
      if(rank==0){trace.flush();if(!trace)failure=1;}
      SU2_MPI::Allreduce(&failure,&globalFailure,1,MPI_INT,MPI_MAX,SU2_MPI::GetComm());++reductions;
      REQUIRE(globalFailure==0);
      double globalStats[6];SU2_MPI::Allreduce(localStats,globalStats,6,MPI_DOUBLE,MPI_SUM,SU2_MPI::GetComm());++reductions;
      unsigned long localBytes=0,globalBytes=0;
      for(const auto& item:owned)localBytes+=item.estimate.cachedBytes;
      SU2_MPI::Allreduce(&localBytes,&globalBytes,1,MPI_UNSIGNED_LONG,MPI_SUM,SU2_MPI::GetComm());++reductions;
      if(rank==0) {
        const char* statuses[]={"estimated_tolerance","integration_limited","outside_bracket","trial_limit"};
        std::ofstream out("normalization.json");out<<std::setprecision(17)
          <<"{\"status\":\""<<statuses[unsigned(result.status)]<<"\",\"log_scale\":"<<result.logScale
          <<",\"value\":"<<result.estimate.value<<",\"uncertainty\":";
        if(std::isfinite(result.estimate.uncertainty))out<<result.estimate.uncertainty;else out<<"null";
        out<<",\"target\":"<<target<<",\"relative_tolerance\":"<<rtol<<",\"trials\":"<<result.trials
          <<",\"cells\":"<<count<<",\"ranks\":"<<ranks<<",\"evaluations\":"<<globalStats[0]
          <<",\"preparation_seconds_sum\":"<<globalStats[1]<<",\"density_seconds_sum\":"<<globalStats[2]
          <<",\"cell_estimates\":"<<globalStats[3]<<",\"prepared_chords\":"<<globalStats[4]
          <<",\"reused_chords\":"<<globalStats[5]<<",\"cached_bytes_sum\":"<<globalBytes
          <<",\"reductions_per_rank\":"<<reductions<<",\"indicator_guard\":"<<guard
          <<",\"seconds\":"<<std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count()
          <<",\"proven_error_bound\":false,\"production_regradation\":false,\"CFD_steps\":0,\"mesh_constructed\":false}\n";
        REQUIRE(bool(out));
      }
      CHECK(result.trials<=24);
      if(result.status==SU2Complexity::Status::EstimatedTolerance)
        CHECK(std::abs(result.estimate.value-target)+result.estimate.uncertainty<=rtol*target);
      return;
    }
    std::ofstream out("field_estimates_rank"+std::to_string(rank)+".csv");
    out<<std::setprecision(17)<<"cell,value,indicator,evaluations,panels,limited,unresolved_support,seconds,preparation_seconds,density_seconds,prepared_chords\n";
    double local[3]={},global[3]={};int localFailure=0,globalFailure=0;
    for(uint64_t row=0;row<count;++row) {
      uint64_t id=0;double coefficients[48];field.read(reinterpret_cast<char*>(&id),8);field.read(reinterpret_cast<char*>(coefficients),sizeof(coefficients));
      REQUIRE(bool(field));
      if(row%ranks==uint64_t(rank) && !localFailure) {
      unsigned at=0;
      for(auto& p:cell)for(auto& x:p)x=coefficients[at++];
      for(auto& m:sensor)for(auto& r:m.m)for(auto& x:r)x=coefficients[at++];
      const auto start=std::chrono::steady_clock::now();
      const unsigned depth=std::getenv("SU2_NATIVE_BL_SUBDIVISIONS")?std::stoul(std::getenv("SU2_NATIVE_BL_SUBDIVISIONS")):0;
      ReplayEstimate e;
      try {e=estimateSubcells(budget,depth);}
      catch(const std::exception& error) {
        localFailure=1;std::cerr<<"FROZEN_FIELD_FAILURE rank="<<rank<<" cell="<<id<<" reason="<<error.what()<<std::endl;
      }
      if(!localFailure) {
      out<<id<<','<<e.value<<','<<e.indicator<<','<<e.evaluations<<','<<e.panels<<','<<e.limited<<','<<e.unresolved<<','
         <<std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count()<<','<<e.preparation<<','<<e.density<<','<<e.chords<<std::endl;
      REQUIRE(e.evaluations<=budget*e.subcells);REQUIRE(bool(out));
      local[0]+=e.value;local[1]+=e.indicator;local[2]+=e.evaluations;
      }
      }
      if((row+1)%64==0 || row+1==count) {
        SU2_MPI::Allreduce(&localFailure,&globalFailure,1,MPI_INT,MPI_MAX,SU2_MPI::GetComm());
        if(globalFailure)break;
      }
    }
    REQUIRE(globalFailure==0);
    SU2_MPI::Allreduce(local,global,3,MPI_DOUBLE,MPI_SUM,SU2_MPI::GetComm());
    if(rank==0) {
      std::ofstream summary("field_global.json");summary<<std::setprecision(17)<<"{\"value\":"<<global[0]<<",\"indicator\":";
      if(std::isfinite(global[1]))summary<<global[1];else summary<<"null";
      summary<<",\"evaluations\":"<<global[2]<<",\"ranks\":"<<ranks<<"}\n";
    }
    return;
  }
  if(std::getenv("SU2_NATIVE_BL_CORRECTION")) {
    REQUIRE(version==2);
    auto sensorDensity=[&](const std::array<double,4>& bary) {
      Tensor metric;
      for(unsigned k=0;k<4;++k)for(unsigned i=0;i<3;++i)for(unsigned j=0;j<3;++j)
        metric.m[i][j]+=bary[k]*sensor[k].m[i][j];
      su2double vectors[3][3],values[3],work[3];
      CBlasStructure::EigenDecomposition(metric.m,vectors,values,3,work);
      return SU2_TYPE::GetValue(std::sqrt(values[0]*values[1]*values[2]));
    };
    const long double volume=std::abs(SU2Native3D::Orientation(
        {cell[0][0],cell[0][1],cell[0][2]},{cell[1][0],cell[1][1],cell[1][2]},
        {cell[2][0],cell[2][1],cell[2][2]},{cell[3][0],cell[3][1],cell[3][2]}))/6;
    REQUIRE(volume>0);
    const auto begin=std::chrono::steady_clock::now();
    const double a=(5+3*std::sqrt(5.))/20,b=(5-std::sqrt(5.))/20;
    long double backgroundLow=0,background=0;
    for(unsigned k=0;k<4;++k){std::array<double,4> beta{{b,b,b,b}};beta[k]=a;backgroundLow+=volume*.25*sensorDensity(beta);}
    const double x3[3]={.5*(1-.77459666924148337704),.5,.5*(1+.77459666924148337704)},w3[3]={5./18,4./9,5./18};
    const double x2[2]={.5*(1-.57735026918962576451),.5*(1+.57735026918962576451)};
    for(unsigned i=0;i<3;++i)for(unsigned j=0;j<3;++j)for(unsigned k=0;k<2;++k) {
      const double u=x3[i],v=x3[j],w=x2[k];
      background+=volume*3*u*u*v*w3[i]*w3[j]*sensorDensity({1-u,u*(1-v),u*v*(1-w),u*v*w});
    }
    const double backgroundSeconds=std::chrono::duration<double>(std::chrono::steady_clock::now()-begin).count();
    const double shift=std::pow(core,1.5);REQUIRE(shift>0);
    size_t negativeRoundoff=0;
    auto correction=[&](const auto& q) {
      const double full=targetDensity(q),sensorValue=sensorDensity(q.barycentric),delta=full-sensorValue;
      if(delta < -128*std::numeric_limits<double>::epsilon()*std::max(full,sensorValue))
        throw std::runtime_error("BL correction weakens the original sensor density.");
      if(delta<0)++negativeRoundoff;
      // Strictly positive scalar API: integrate delta+constant, then subtract its exact volume contribution.
      return std::max(0.,delta)+shift;
    };
    std::ofstream out("correction.json");out<<std::setprecision(17)
        <<"{\"background\":"<<double(background)<<",\"background_indicator\":"<<double(std::abs(background-backgroundLow))
        <<",\"background_seconds\":"<<backgroundSeconds<<",\"background_evaluations\":22,\"runs\":[";
    unsigned at=0;
    for(bool decomposed:{false,true}) {
      auto workspace=layers.PrepareComplexity3D(cell,4u<<20);
      for(bool warm:{false,true}) {
        const auto e=layers.EstimateComplexity3D(cell,decomposed?CBoundaryLayerMetric::IntegrationDensity3D(correction):targetDensity,
            8192,.001,workspace.get());
        const double value=decomposed?double(background+e.value-shift*volume):e.value;
        const double error=e.indicator+(decomposed?double(std::abs(background-backgroundLow)):0);
        CHECK(value>0);CHECK(e.evaluations<=8192);
        out<<(at++?",":"")<<"{\"decomposed\":"<<(decomposed?"true":"false")<<",\"warm\":"<<(warm?"true":"false")
          <<",\"value\":"<<value<<",\"indicator\":";
        if(std::isfinite(error))out<<error;else out<<"null";
        out<<",\"evaluations\":"<<e.evaluations<<",\"seconds\":"<<e.seconds
          <<",\"preparation_seconds\":"<<e.preparationSeconds<<",\"density_seconds\":"<<e.densitySeconds
          <<",\"prepared_chords\":"<<e.preparedChords<<",\"reused_chords\":"<<e.reusedChords
          <<",\"cached_bytes\":"<<e.cachedBytes<<",\"limited\":"<<(e.limited?"true":"false")
          <<",\"unknown_support\":"<<(e.unresolvedSupport?"true":"false")<<"}";
      }
    }
    out<<"],\"negative_roundoff_queries\":"<<negativeRoundoff<<",\"original_P1\":true,\"indicator_is_proven\":false}\n";
    REQUIRE(bool(out));return;
  }
  if(std::getenv("SU2_NATIVE_BL_CACHE")) {
    REQUIRE(version==2);
    auto cache=layers.PrepareComplexity3D(cell,4u<<20);
    const auto initial=layers.EstimateComplexity3D(cell,targetDensity,8192,-1,cache.get());
    const auto repeated=layers.EstimateComplexity3D(cell,targetDensity,8192,-1,cache.get());
    CHECK(repeated.value==initial.value);CHECK(repeated.indicator==initial.indicator);
    CHECK(repeated.evaluations==initial.evaluations);CHECK(repeated.reusedChords>0);
    // Change both sensor strength and the physical fade core; geometry preparation must remain reusable.
    for(auto& m:sensor)for(auto& row:m.m)for(auto& v:row)v*=4;
    core*=4;
    const auto changed=layers.EstimateComplexity3D(cell,targetDensity,8192,-1,cache.get());
    const auto cold=layers.EstimateComplexity3D(cell,targetDensity);
    CHECK(changed.value==cold.value);CHECK(changed.indicator==cold.indicator);
    CHECK(changed.evaluations==cold.evaluations);
    std::ofstream out("geometry_reuse.json");out<<std::setprecision(17)<<"[";
    unsigned index=0;
    for(const auto* e:{&initial,&repeated,&changed,&cold}) {
      out<<(index++?",":"")<<"{\"value\":"<<e->value<<",\"indicator\":";
      if(std::isfinite(e->indicator))out<<e->indicator;else out<<"null";
      out<<",\"evaluations\":"<<e->evaluations<<",\"panels\":"<<e->panels
         <<",\"seconds\":"<<e->seconds<<",\"preparation_seconds\":"<<e->preparationSeconds
         <<",\"density_seconds\":"<<e->densitySeconds<<",\"prepared_chords\":"<<e->preparedChords
         <<",\"reused_chords\":"<<e->reusedChords<<",\"cached_bytes\":"<<e->cachedBytes<<"}";
    }
    out<<"]\n";REQUIRE(bool(out));return;
  }
  if(const char* control=std::getenv("SU2_NATIVE_BL_COMPLEXITY")) {
    const size_t budget=std::stoul(control);
    const unsigned depth=std::getenv("SU2_NATIVE_BL_SUBDIVISIONS")?std::stoul(std::getenv("SU2_NATIVE_BL_SUBDIVISIONS")):0;
    const auto start=std::chrono::steady_clock::now();const auto total=estimateSubcells(budget,depth);
    std::ofstream out("complexity_estimate.json");out<<std::setprecision(17)
      <<"{\"value\":"<<total.value<<",\"indicator\":";
    if(std::isfinite(total.indicator))out<<total.indicator;else out<<"null";
    out<<",\"evaluations\":"<<total.evaluations<<",\"panels\":"<<total.panels<<",\"limited_cells\":"<<total.limited
       <<",\"unresolved_support\":"<<total.unresolved<<",\"subcells\":"<<total.subcells<<",\"budget_per_cell\":"<<budget
       <<",\"seconds\":"<<std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count()
       <<",\"indicator_is_proven_bound\":false}\n";
    REQUIRE(bool(out));CHECK(total.value>0);CHECK(total.evaluations<=budget*total.subcells);return;
  }
  if(const char* queries=std::getenv("SU2_NATIVE_BL_QUERY_POINTS")) {
    std::ifstream points(queries,std::ios::binary|std::ios::ate);REQUIRE(bool(points));
    const auto bytes=static_cast<std::streamoff>(points.tellg());
    REQUIRE(bytes>0);REQUIRE(bytes%std::streamoff(4*sizeof(double))==0);REQUIRE(bytes<=32000000);
    const size_t count=size_t(bytes)/(4*sizeof(double));points.seekg(0);
    std::ofstream values("point_values.f64",std::ios::binary);REQUIRE(bool(values));
    SU2Native3D::Tetrahedron source;
    for(unsigned k=0;k<4;++k)source[k]={cell[k][0],cell[k][1],cell[k][2]};
    const bool reversed=SU2Native3D::Orientation(source[0],source[1],source[2],source[3])<0;
    if(reversed)std::swap(source[1],source[2]);
    for(size_t row=0;row<count;++row) {
      std::array<double,4> requested;points.read(reinterpret_cast<char*>(requested.data()),sizeof(requested));
      double sum=0;for(auto b:requested) {
        if(!std::isfinite(b) || b<0 || b>1)throw std::invalid_argument("Invalid frozen point-query barycentrics.");
        sum+=b;
      }
      if(std::abs(sum-1)>2e-12)throw std::invalid_argument("Frozen point-query weights do not sum to one.");
      std::array<double,3> coordinates;
      for(unsigned j=0;j<3;++j) {
        long double x=cell[0][j];
        for(unsigned k=1;k<4;++k)x+=(long double)requested[k]*((long double)cell[k][j]-cell[0][j]);
        coordinates[j]=double(x);
      }
      const SU2Native3D::Point position{coordinates[0],coordinates[1],coordinates[2]};
      std::array<long double,4> actual;
      if(!SU2Native3D::Barycentric(source,position,actual))throw std::invalid_argument("Rounded frozen query leaves its original source cell.");
      if(reversed)std::swap(actual[1],actual[2]);
      CBoundaryLayerMetric::IntegrationPoint3D query{};query.weight=1;
      for(unsigned k=0;k<4;++k)query.barycentric[k]=double(actual[k]);
      query.samples=layers.SamplePoints({position.x,position.y,position.z});
      double output[6];for(unsigned k=0;k<4;++k)output[k]=query.barycentric[k];
      output[4]=targetDensity(query);output[5]=-1;
      if(!(output[4]>0) || !std::isfinite(output[4]))throw std::runtime_error("Invalid frozen point-query density.");
      for(const auto& sample:query.samples)output[5]=SU2_TYPE::GetValue(sample.sample.distance);
      values.write(reinterpret_cast<const char*>(output),sizeof(output));
      if(!points || !values)throw std::runtime_error("Frozen point-query I/O failed.");
    }
    values.close();REQUIRE(bool(values));
    std::cout<<"FACETED_POINT_REPLAY count="<<count<<" target_version="<<version<<" columns=b0,b1,b2,b3,target_density,active_wall_distance\n";
    return;
  }
  std::array<double,4> nodal{};
  for(unsigned k=0;k<4;++k)nodal[k]=SU2_TYPE::GetValue(layers.Evaluate(0,cell[k].data(),1.).distance);
  std::ofstream samples("replay_samples.csv");samples<<std::setprecision(17)<<"mask,x,y,z,actual_distance,linear_distance,error\n";
  double worst=0;
  for(unsigned mask=1;mask<16;++mask) {
    unsigned count=0;for(unsigned k=0;k<4;++k)count+=(mask>>k)&1u;
    std::array<double,3> p{};double linear=0;
    for(unsigned k=0;k<4;++k)if(mask&(1u<<k)) {
      linear+=nodal[k]/count;for(unsigned j=0;j<3;++j)p[j]+=cell[k][j]/count;
    }
    const double actual=SU2_TYPE::GetValue(layers.Evaluate(0,p.data(),1.).distance);
    worst=std::max(worst,std::abs(actual-linear));
    samples<<mask<<','<<p[0]<<','<<p[1]<<','<<p[2]<<','<<actual<<','<<linear<<','<<std::abs(actual-linear)<<'\n';
  }
  std::array<long double,3> a{},b{},c{};
  for(unsigned j=0;j<3;++j){a[j]=(long double)cell[1][j]-cell[0][j];b[j]=(long double)cell[2][j]-cell[0][j];c[j]=(long double)cell[3][j]-cell[0][j];}
  const long double expectedVolume=std::abs(a[0]*(b[1]*c[2]-b[2]*c[1])+a[1]*(b[2]*c[0]-b[0]*c[2])+a[2]*(b[0]*c[1]-b[1]*c[0]))/6;
  REQUIRE(expectedVolume>0);
  for(const double ratio:version==2?std::vector<double>{1.5}:std::vector<double>{1.5,1.25}) {
    const auto begin=std::chrono::steady_clock::now();unsigned densityCalls=0;bool dependent=false,complete=false;
    std::string reason;size_t count=0;long double volume=0,normalIntegral=0,fullIntegral=0,composedIntegral=0;
    long double first[4]={},second[4][4]={};
    std::ofstream output(ratio==1.5?"replay_rule_150.csv":"replay_rule_125.csv");
    output<<std::setprecision(17)<<"b0,b1,b2,b3,weight,distance,normal_size,normal_density,full_unit_density,target_density\n";
    try {
      const auto rule=layers.IntegrationRule3D(cell,ratio,[&](const auto& point) {
        ++densityCalls;return targetDensity(point);
      },&dependent);
      CHECK(dependent);CHECK(densityCalls>0);
      count=rule.size();
      for(const auto& point:rule) {
        REQUIRE(point.weight>0);volume+=point.weight;double sum=0;
        for(unsigned i=0;i<4;++i) {
          REQUIRE(point.barycentric[i]>=-1e-14);sum+=point.barycentric[i];first[i]+=point.weight*point.barycentric[i];
          for(unsigned j=0;j<4;++j)second[i][j]+=point.weight*point.barycentric[i]*point.barycentric[j];
        }
        CHECK(sum==Approx(1.).epsilon(1e-12));
        Tensor metric;for(unsigned j=0;j<3;++j)metric.m[j][j]=1;
        double normalDensity=1,distance=0,hn=1;
        for(const auto& sample:point.samples) {
          layers.ApplySample(sample.wall,sample.sample,metric,1.,nullptr,false);
          distance=SU2_TYPE::GetValue(sample.sample.distance);hn=SU2_TYPE::GetValue(sample.sample.hn);
          normalDensity=std::max(normalDensity,std::pow(hn,-SU2_TYPE::GetValue(sample.sample.weight)));
        }
        su2double vectors[3][3],eigenvalues[3],workspace[3];
        CBlasStructure::EigenDecomposition(metric.m,vectors,eigenvalues,3,workspace);
        const double density=SU2_TYPE::GetValue(std::sqrt(eigenvalues[0]*eigenvalues[1]*eigenvalues[2]));
        REQUIRE(std::isfinite(density));REQUIRE(density>0);
        normalIntegral+=point.weight*normalDensity;fullIntegral+=point.weight*density;
        const double target=targetDensity(point);REQUIRE(std::isfinite(target));REQUIRE(target>0);composedIntegral+=point.weight*target;
        for(auto value:point.barycentric)output<<value<<',';
        output<<point.weight<<','<<distance<<','<<hn<<','<<normalDensity<<','<<density<<','<<target<<'\n';
      }
      CHECK(double(volume/expectedVolume)==Approx(1.).epsilon(2e-10));
      for(unsigned i=0;i<4;++i) {
        CHECK(double(first[i]/expectedVolume)==Approx(.25).epsilon(2e-10));
        for(unsigned j=0;j<4;++j)CHECK(double(second[i][j]/expectedVolume)==Approx(i==j?.1:.05).epsilon(2e-10));
      }
      complete=true;
    }catch(const std::exception& e){reason=e.what();}
    std::cout<<std::setprecision(17)<<"FACETED_VOLUME_REPLAY ratio="<<ratio<<" complete="<<complete<<" points="<<count
        <<" volume="<<double(volume)<<" normal_integral="<<double(normalIntegral)<<" full_unit_integral="<<double(fullIntegral)
        <<" target_version="<<version<<" composed_integral="<<double(composedIntegral)
        <<" density_calls="<<densityCalls<<" dependent="<<dependent<<" largest_initial_distance_defect="<<worst
        <<" seconds="<<std::chrono::duration<double>(std::chrono::steady_clock::now()-begin).count()<<" reason="<<reason<<'\n';
    INFO(reason);CHECK(complete);
  }
}

TEST_CASE("Bounded complexity keeps complete positive estimates and explicit uncertainty", "[NativeComplexityEstimate3D]") {
  CBoundaryLayerMetric::Wall wall;wall.layer.marker="wall";wall.layer.firstHeight=.01;wall.layer.growth=1.2;wall.layer.thickness=.1;
  wall.coord={-2,0,-2,2,0,-2,2,0,2,-2,0,2};wall.conn={0,1,2,3};
  CBoundaryLayerMetric layers(3,{wall});
  const std::array<std::array<double,3>,4> near{{{{0,0,0}},{{.1,0,0}},{{0,.05,0}},{{0,0,.1}}}};
  const double volume=.1*.05*.1/6;
  auto polynomial=[](const auto& q){double v=1;for(auto b:q.barycentric)v+=b*b;return v;};
  const auto result=layers.EstimateComplexity3D(near,polynomial);
  CHECK(result.value==Approx(1.4*volume).epsilon(1e-10));CHECK(result.indicator<1e-10*result.value);
  CHECK_FALSE(result.unresolvedSupport);CHECK(result.evaluations<=8192);
  auto far=near;for(auto& p:far)p[1]+=1;
  const auto outside=layers.EstimateComplexity3D(far,polynomial);
  CHECK(outside.value==Approx(1.4*volume).epsilon(1e-10));CHECK(outside.evaluations==22);
  const auto limited=layers.EstimateComplexity3D(near,polynomial,64);
  CHECK(limited.limited);CHECK(limited.value>0);CHECK(std::isinf(limited.indicator));CHECK(limited.evaluations==64);
  auto workspace=layers.PrepareComplexity3D(near,1u<<20);
  const auto first=layers.EstimateComplexity3D(near,polynomial,8192,-1,workspace.get());
  size_t callbackCount=0;
  auto doubled=[&](const auto& q){++callbackCount;return 2*polynomial(q);};
  const auto reused=layers.EstimateComplexity3D(near,doubled,8192,-1,workspace.get());
  CHECK(reused.value==2*first.value);CHECK(callbackCount==reused.evaluations);
  CHECK(first.preparedChords>0);CHECK(reused.preparedChords==0);CHECK(reused.reusedChords>0);
  CHECK(reused.cachedBytes<=1u<<20);CHECK(reused.cachedBytes==first.cachedBytes);
  CHECK(reused.seconds>=reused.densitySeconds);CHECK(reused.seconds>=reused.preparationSeconds);
  auto changing=[](const auto& q){return std::exp(5*q.barycentric[0]);};
  const auto warm=layers.EstimateComplexity3D(near,changing,8192,0,workspace.get());
  const auto cold=layers.EstimateComplexity3D(near,changing,8192,0);
  CHECK(warm.value==cold.value);CHECK(warm.indicator==cold.indicator);CHECK(warm.evaluations==cold.evaluations);
  const auto loose=layers.EstimateComplexity3D(near,changing,8192,1e99);
  CHECK(loose.evaluations<warm.evaluations);CHECK(warm.evaluations<=8192);
  auto smallCache=layers.PrepareComplexity3D(near,1024);
  const auto small=layers.EstimateComplexity3D(near,polynomial,8192,-1,smallCache.get());
  CHECK(small.value==result.value);CHECK(small.cachedBytes<=1024);
  CHECK_THROWS(layers.EstimateComplexity3D(far,polynomial,8192,-1,workspace.get()));
  CBoundaryLayerMetric different(3,{wall});
  CHECK_THROWS(different.EstimateComplexity3D(near,polynomial,8192,-1,workspace.get()));
  CHECK_THROWS(layers.PrepareComplexity3D(near,0));
  CHECK_THROWS(layers.EstimateComplexity3D(near,polynomial,8192,-2));
  CHECK_THROWS(layers.EstimateComplexity3D(near,polynomial,8192,std::numeric_limits<double>::quiet_NaN()));
  CHECK_THROWS(layers.EstimateComplexity3D(near,polynomial,63));
  size_t beforeGeometryLimit=0;
  const auto geometryLimited=layers.EstimateComplexity3D(near,[&](const auto&) {
    if(++beforeGeometryLimit>4)throw SU2Native3D::ChordEnvelope::WorkLimit("Test geometry work exhaustion.");
    return 1.;
  });
  CHECK(geometryLimited.value==Approx(volume).epsilon(1e-12));
  CHECK(geometryLimited.limited);CHECK(geometryLimited.unresolvedSupport);CHECK(std::isinf(geometryLimited.indicator));
  CHECK_THROWS(layers.EstimateComplexity3D(near,[](const auto&){return -1.;}));
  CHECK_THROWS(layers.EstimateComplexity3D(near,[](const auto&){return std::numeric_limits<double>::quiet_NaN();}));
}


TEST_CASE("Global complexity search respects integration uncertainty and bounded work", "[NativeComplexityNormalization]") {
  using namespace SU2Complexity;
  for(unsigned dimension:{2u,3u}) {
    std::vector<double> requests;
    const auto r=Normalize(20,-10,10,0,.001,32,[&](double x,double requested) {
      requests.push_back(requested);return Estimate{7+3*std::exp(.5*dimension*x),std::min(.2,.5*requested)};
    });
    CHECK(r.status==Status::EstimatedTolerance);
    CHECK(std::abs(r.estimate.value-20)+r.estimate.uncertainty<=.02);
    CHECK(*std::min_element(requests.begin(),requests.end())<requests.front());CHECK(r.trials<=32);
  }
  const auto limited=Normalize(10,-1,1,0,.001,16,[](double,double){return Estimate{10,1};});
  CHECK(limited.status==Status::IntegrationLimited);CHECK(limited.trials==2);
  const auto unknown=Normalize(10,-1,1,0,.001,16,[](double,double){return Estimate{10,INFINITY};});
  CHECK(unknown.status==Status::IntegrationLimited);CHECK(unknown.trials==2);
  const auto lowerOnly=Normalize(10,-10,10,0,.001,16,[](double,double){return Estimate{35,INFINITY,30};});
  CHECK(lowerOnly.status==Status::OutsideBracket);CHECK(lowerOnly.logScale==-10);
  CHECK(std::isinf(lowerOnly.estimate.uncertainty));CHECK(lowerOnly.estimate.minimum==30);
  const auto unresolvedUpper=Normalize(40,-10,10,0,.001,16,[](double,double){return Estimate{35,INFINITY,30};});
  CHECK(unresolvedUpper.status==Status::IntegrationLimited);CHECK(unresolvedUpper.trials==2);
  CHECK_THROWS(Normalize(10,-1,1,0,.001,16,[](double,double){return Estimate{10,1,12};}));
  CHECK_THROWS(Normalize(10,-1,1,0,.001,16,[](double,double){return Estimate{10,INFINITY,-1};}));
  CHECK_THROWS(Normalize(10,-1,1,0,.001,16,[](double,double){return Estimate{10,INFINITY,INFINITY};}));
  CHECK_THROWS(Normalize(10,-1,1,0,.001,16,[](double,double){return Estimate{10,INFINITY,NAN};}));
  const auto outside=Normalize(100,-10,-5,-8,.001,16,[](double x,double){return Estimate{5+std::exp(x),.001};});
  CHECK(outside.status==Status::OutsideBracket);
  const auto ceiling=Normalize(10,-1,1,0,.001,1,[](double x,double){return Estimate{std::exp(x),0};});
  CHECK(ceiling.status==Status::TrialLimit);CHECK(ceiling.trials==1);
  CHECK(SelectError(4,8));CHECK_FALSE(SelectError(3.99,8));CHECK_FALSE(SelectError(1,INFINITY));
  CHECK(SelectError(INFINITY,INFINITY));CHECK_FALSE(SelectError(0,0));CHECK_THROWS(SelectError(-1,1));
  CHECK_THROWS(Normalize(10,-1,1,0,.001,16,[](double,double){return Estimate{-1,1};}));
  CHECK_THROWS(Normalize(10,-1,1,0,.001,16,[](double,double){return Estimate{10,-1};}));
  CHECK_THROWS(Normalize(0,-1,1,0,.001,16,[](double,double){return Estimate{10,0};}));
  CHECK_THROWS(Normalize(std::numeric_limits<double>::denorm_min(),-1,1,0,.001,16,
                       [](double,double){return Estimate{10,0};}));
}


TEST_CASE("Finite feature envelopes retain global competition and distance crossings", "[NativeChordEnvelope]") {
  using E=SU2Native3D::ChordEnvelope;using P=std::array<double,3>;
  const std::array<P,3> triangle{{{{0,0,0}},{{1,0,0}},{{0,1,0}}}};
  E envelope({-.5,.25,.1},{1.5,.25,.1});
  for(unsigned k=0;k<3;++k){envelope.Vertex(triangle[k]);envelope.Segment(triangle[k],triangle[(k+1)%3]);}
  envelope.Face(triangle);
  for(unsigned k=0;k<=1000;++k) {
    const double t=k/1000.,x=-.5+2*t;
    const double expected=x<0?std::hypot(x,.1):x<=.75?.1:x<=1.25?std::hypot((x-.75)/std::sqrt(2.),.1):std::hypot(x-1,.25,.1);
    CHECK(double(envelope.Distance(t))==Approx(expected).epsilon(1e-13));
  }
  const auto cuts=envelope.Cuts({.1,.2,.5});
  CHECK(cuts.front()==0);CHECK(cuts.back()==1);CHECK(envelope.Features()==7);
  CHECK(envelope.Comparisons()>0);CHECK(envelope.Pieces()>1);
  for(const double level:{.2,.5}) {
    bool found=false;for(auto t:cuts)found=found || std::abs(envelope.Distance(t)-level)<1e-12;
    CHECK(found);
  }
  E competing({-2,0,0},{2,0,0});competing.Vertex({-1,0,0});competing.Vertex({1,0,0});
  for(unsigned k=0;k<=100;++k){const double t=k/100.,x=-2+4*t;CHECK(double(competing.Distance(t))==Approx(std::min(std::abs(x-1),std::abs(x+1))).margin(1e-14));}
  const auto ties=competing.Cuts({.25});CHECK(std::find(ties.begin(),ties.end(),.5)!=ties.end());
  const auto bandsOnly=competing.Cuts({.25},false);
  CHECK(std::find(bandsOnly.begin(),bandsOnly.end(),.5)==bandsOnly.end());
  CHECK(bandsOnly.size()==6);  // Four physical quarter-distance crossings and both endpoints.

  E bounded({0,0,0},{1,0,0},1);bounded.Vertex({0,1,0});bounded.Vertex({1,1,0});
  CHECK_THROWS(bounded.Vertex({.5,1,0}));CHECK_THROWS(envelope.Cuts({-1}));
  CHECK_THROWS(envelope.Face({{{{0,0,0}},{{0,0,0}},{{0,0,0}}}}));
  CHECK_THROWS(E({0,0,0},{0,0,0}));
}


TEST_CASE("Finite-feature chord support retains all active crossings and distant competition", "[NativeChordEnvelope]") {
  using E=SU2Native3D::ChordEnvelope;using P=std::array<double,3>;
  const std::array<P,3> triangle{{{{0,0,0}},{{1,0,0}},{{0,1,0}}}};
  E full({-.5,.25,.001},{1.5,.25,.001}),local({-.5,.25,.001},{1.5,.25,.001},1u<<20,.02);
  for(auto* e:{&full,&local}) {
    for(unsigned k=0;k<3;++k){e->Vertex(triangle[k]);e->Segment(triangle[k],triangle[(k+1)%3]);}
    e->Face(triangle);for(unsigned k=0;k<100;++k)e->Vertex({double(k),10,20});
  }
  CHECK(local.Features()<full.Features());CHECK(local.Comparisons()<full.Comparisons());
  for(unsigned k=0;k<=4096;++k) {
    const long double t=k/4096.L;
    CHECK(double(local.Distance(t))==Approx(double(std::min(full.Distance(t),.02L))).margin(1e-15));
  }
  const auto cuts=local.Cuts({.001,.002,.01,.019});
  for(double t:full.Cuts({.001,.002,.01,.019}))if(full.Distance(t)<.0199L) {
    CHECK(std::any_of(cuts.begin(),cuts.end(),[&](double s){return std::abs(t-s)<1e-13;}));
  }
  // Competing supports on disconnected wall pieces retain both narrow pockets.
  E pockets({-2,0,0},{2,0,0},1u<<20,.01);pockets.Vertex({-1,.001,0});pockets.Vertex({1,.001,0});
  for(double t:{.25,.75})CHECK(double(pockets.Distance(t))==Approx(.001).epsilon(1e-13));
  CHECK(pockets.Cuts({.002}).size()==10);
  CHECK_THROWS(E({0,0,0},{1,0,0},100,0));CHECK_THROWS(E({0,0,0},{1,0,0},100,NAN));
}


TEST_CASE("Retained complexity stencils use fresh nonhomogeneous trial tensors and cores", "[NativeDynamicComplexity3D]") {
  using namespace SU2Native3D;
  const auto mesh=simplex_test::MakeSimplexMesh(3,2,[](const passivedouble*){return "far";});
  const int rank=SU2_MPI::GetRank(),ranks=SU2_MPI::GetSize();
  std::vector<CNativeComplexity3D::SourceCell> source;
  for(size_t i=0;i<mesh.GetnElem();++i)if((i+1)%ranks==unsigned(rank)) {
    CNativeComplexity3D::SourceCell row;row.cell.id=i;
    for(unsigned k=0;k<4;++k) {const auto n=mesh.elem[4*i+k];row.nodes[k]=n;
      row.cell.v[k]={n,{mesh.coord[3*n],mesh.coord[3*n+1],mesh.coord[3*n+2]}};}
    source.push_back(row);
  }
  CBoundaryLayerMetric::Wall wall;wall.layer={"wall",.02,1.2,.15};
  wall.coord={0,0,.5,1,0,.5,1,1,.5,0,1,.5};wall.conn={0,1,2,2,0,2,3,3};
  CBoundaryLayerMetric layers(3,{wall});
  CNativeComplexity3D estimate(layers,source,mesh.GetnPoint(),size_t(256)<<20,2048,7519);
  std::vector<CBoundaryLayerMetric::Tensor> original(mesh.GetnPoint());
  for(auto& m:original)for(unsigned d=0;d<3;++d)m.m[d][d]=2;
  const auto a=estimate.Assess(original,2,true);
  CHECK(a.sensorLower==Approx(std::sqrt(8.)).epsilon(1e-13));CHECK(a.sensorUpper==Approx(std::sqrt(8.)).epsilon(1e-13));
  CHECK(a.directChecks==a.inside);CHECK(a.directChecks>100);CHECK(a.nonzero>100);
  CHECK(std::isfinite(a.global.uncertainty));CHECK(a.global.value>a.sensorUpper);
  auto trial=original;
  for(size_t n=0;n<trial.size();++n) {
    trial[n].m[0][0]=std::min(4.+20*mesh.coord[3*n],10.);
    trial[n].m[1][1]=3.+2*mesh.coord[3*n+1];trial[n].m[2][2]=1.+mesh.coord[3*n+2];
    trial[n].m[0][1]=trial[n].m[1][0]=.1*mesh.coord[3*n];
  }
  const auto b=estimate.Assess(trial,1,true),c=estimate.Assess(original,2,true);
  CHECK(b.directChecks==a.directChecks);CHECK(b.sensorLower<=b.sensorQuadrature);CHECK(b.sensorQuadrature<=b.sensorUpper);
  CHECK(b.sensorQuadrature>a.sensorQuadrature);CHECK(b.global.value!=a.global.value);
  CHECK(c.global.value==a.global.value);CHECK(c.global.uncertainty==a.global.uncertainty);
  CHECK(estimate.GetStatistics().assessments==3);
  auto anisotropic=original;
  const su2double frame[3][3]={{.6,-.8,0},{.8,.6,0},{0,0,1}},val[3]={1e10,1,4};
  for(auto& m:anisotropic)CBlasStructure::EigenRecomposition(m.m,frame,val,3);
  const auto extreme=estimate.Assess(anisotropic,1);
  CHECK(extreme.sensorLower==Approx(2e5).epsilon(1e-6));CHECK(extreme.sensorUpper==Approx(2e5).epsilon(1e-6));
  CHECK(extreme.numericalIndicator>1e-8*extreme.sensorLower);
  if(rank==0)trial.front().m[0][0]=-1;
  CHECK_THROWS(estimate.Assess(trial,1));
  CHECK_THROWS(CNativeComplexity3D(layers,source,mesh.GetnPoint(),size_t(256)<<20,ranks==1?0:2048,7519+rank));
}

TEST_CASE("Actual native normalization reports uncertainty at intersecting walls", "[NativeRuntimeComplexity3D]") {
  if(SU2_MPI::GetSize()!=1)return;
  const auto mesh=simplex_test::MakeSimplexMesh(3,2,[](const passivedouble* x) {
    return x[0]==0 || x[1]==0 || x[2]==0?"wall":"far";
  });
  MetricTest test(mesh,"MARKER_HEATFLUX= (wall,0)\nMARKER_FAR= (far)\nADAP_REMESHER= NATIVE_CAVITY\n"
      "ADAP_SURFACE= YES\nADAP_ISO_CORNER= NO\nADAP_HGRAD= 1.3\nADAP_HMIN= 1e-6\nADAP_HMAX= 1\n"
      "ADAP_ARMAX= 100\nADAP_COMPLEXITY= 100\nADAP_BL_MARKER= (wall)\nADAP_BL_FIRST_HEIGHT= (1e-6)\n"
      "ADAP_BL_GROWTH= (1.2)\nADAP_BL_THICKNESS= (.01)\n",{1,0,0,1,0,1});
  const auto output=test.ComputeMetric();INFO(output);
  CHECK(output.find("Native3D complexity normalization:")!=std::string::npos);
  CHECK(output.find("proven_error_bound=0")!=std::string::npos);
  CHECK(output.find("Native3D bounded complexity strategy: nonplanar faceted wall geometry.")!=std::string::npos);
  CHECK(output.find("0 strict density evaluations.")!=std::string::npos);
  CHECK(std::isfinite(test.solver[FLOW_SOL]->GetMetricComplexityFinal()));
  // The published donor is sensor-only even at a wall; BL composition remains a query-time operation.
  for(auto p=0ul;p<test.Geometry().GetnPointDomain();++p) {
    const auto m=test.Metric(p);for(unsigned d=0;d<3;++d)CHECK(SU2_TYPE::GetValue(m.m[d][d])<1e10);
  }
}


TEST_CASE("Wall coplanarity selects integration without changing geometric queries", "[NativeIntegrationPolicy3D]") {
  CBoundaryLayerMetric::Wall wall;wall.layer={"wall",.01,1.2,.2};
  wall.coord={0,0,0,1,0,1,1,1,1,0,1,0};wall.conn={0,1,2,2,0,2,3,3};
  CBoundaryLayerMetric planar(3,{wall});CHECK(planar.PlanarWalls3D());
  for(size_t i=0;i<wall.coord.size();i+=3) {wall.coord[i]+=1e8;wall.coord[i+1]-=2e8;wall.coord[i+2]+=3e8;}
  CBoundaryLayerMetric translated(3,{wall});CHECK(translated.PlanarWalls3D());
  wall.coord[11]+=.01;CBoundaryLayerMetric bent(3,{wall});CHECK_FALSE(bent.PlanarWalls3D());
  auto second=wall;second.layer.marker="second";
  CBoundaryLayerMetric together(3,{second,wall});CHECK_FALSE(together.PlanarWalls3D());
}

TEST_CASE("Coarsest-field preflight continues toward a reachable planar budget", "[NativeFeasibleComplexity3D]") {
  if(SU2_MPI::GetSize()!=1)return;
  const auto mesh=simplex_test::MakeSimplexMesh(3,2,[](const passivedouble* x){return x[2]==0?"wall":"far";});
  MetricTest test(mesh,"MARKER_HEATFLUX= (wall,0)\nMARKER_FAR= (far)\nADAP_REMESHER= NATIVE_CAVITY\n"
      "ADAP_SURFACE= YES\nADAP_ISO_CORNER= NO\nADAP_HGRAD= 1.3\nADAP_HMIN= 1e-4\nADAP_HMAX= 1\n"
      "ADAP_ARMAX= 100\nADAP_COMPLEXITY= 100\nADAP_BL_MARKER= (wall)\nADAP_BL_FIRST_HEIGHT= (.1)\n"
      "ADAP_BL_GROWTH= (1.2)\nADAP_BL_THICKNESS= (.2)\n",{1,0,0,1,0,1});
  const auto output=test.ComputeMetric();INFO(output);
  CHECK(output.find("outside_estimated_bounds")==std::string::npos);
  CHECK(test.solver[FLOW_SOL]->GetMetricComplexityFinal()==Approx(100).epsilon(.001));
  CHECK(SU2_TYPE::GetValue(test.Metric(0).m[0][0])>1);
}

TEST_CASE("Budget profiles scan ordered samples without a monotonicity assumption", "[NativeBLBudgetPolicy]") {
  using namespace SU2Complexity;
  const double nonmonotone[]={20,30,8,25,5};unsigned calls=0;
  auto chosen=SelectProfile(10,4,[&](double p){++calls;return Estimate{nonmonotone[unsigned(p*4)],.1};});
  CHECK(chosen.status==BudgetStatus::EstimatedAttainable);CHECK(chosen.parameter==.5);CHECK(calls==3);
  auto uncertain=SelectProfile(10,4,[](double p){return Estimate{p==0?10.:5.,p==0?1.:.1};});
  CHECK(uncertain.status==BudgetStatus::IntegrationLimited);CHECK(uncertain.parameter==0);CHECK(uncertain.trials==1);
  auto incompatible=SelectProfile(10,8,[](double p){return Estimate{30+std::sin(20*p),1};});
  CHECK(incompatible.status==BudgetStatus::EstimatedIncompatible);CHECK(incompatible.trials==9);
  CHECK_THROWS(SelectProfile(0,8,[](double){return Estimate{1,0};}));
  CHECK_THROWS(SelectProfile(1,33,[](double){return Estimate{1,0};}));
  CHECK_THROWS(SelectProfile(1,8,[](double){return Estimate{1,-1};}));
  CHECK(SelectProfile(1,8,[](double){return Estimate{1,INFINITY};}).status==BudgetStatus::IntegrationLimited);
}

TEST_CASE("Outer-normal profiles protect height tangents geometry fade and frozen sample identity", "[NativeBLBudgetProfile]") {
  const auto wall=AnalyticSphereWall(.01,.3,1.);CBoundaryLayerMetric strict(3,{wall}),layers(3,{wall});
  layers.ConfigureOuterNormal(.03,2.);
  std::vector<CBoundaryLayerMetric::Sample> original;
  const double distances[]={0.,.005,.01,.03,.04,.1,.275,.299,.31};
  for(auto d:distances) {
    const su2double p[]={1+d,0.,0.};const auto a=strict.Evaluate(0,p,.01),b=layers.Evaluate(0,p,.01);
    CHECK(a.hn==b.hn);CHECK(a.weight==b.weight);
    for(unsigned i=0;i<3;++i)for(unsigned j=0;j<3;++j)CHECK(a.full.m[i][j]==b.full.m[i][j]);
    original.push_back(b);
  }
  auto workspace=layers.PrepareComplexity3D({{{1,0,0},{1.4,0,0},{1,.1,0},{1,0,.1}}},1<<20);
  layers.SelectOuterNormal(.5);CHECK(layers.GetOuterNormal().version==2);
  for(size_t k=0;k<original.size();++k) {
    const auto d=distances[k];const su2double p[]={1+d,0.,0.};const auto sample=layers.Evaluate(0,p,.01);
    CHECK(sample.weight==original[k].weight);
    if(sample.weight==0)continue;
    CHECK(sample.hn>=original[k].hn);
    if(d<=.03)CHECK(sample.hn==original[k].hn);
    for(unsigned i=1;i<3;++i)for(unsigned j=1;j<3;++j)CHECK(sample.full.m[i][j]==original[k].full.m[i][j]);
    Tensor fine;fine.m[0][0]=1e8;fine.m[1][1]=2e8;fine.m[2][2]=3e8;
    layers.ApplyPoint(p,fine,.01,nullptr,false);
    CHECK(fine.m[0][0]>=1e8*(1-1e-12));CHECK(fine.m[1][1]>=2e8*(1-1e-12));CHECK(fine.m[2][2]>=3e8*(1-1e-12));
    Tensor target;for(unsigned i=0;i<3;++i)target.m[i][i]=1;
    CHECK_THROWS(layers.ApplySample(0,original[k],target,.01,nullptr,false));
  }
  CHECK_THROWS(layers.EstimateComplexity3D({{{1,0,0},{1.4,0,0},{1,.1,0},{1,0,.1}}},[](const auto&){return 1.;},8192,-1,workspace.get()));
  const auto cuts=layers.EdgeBreakpoints({1,0,0},{1.3,0,0});
  CHECK(std::any_of(cuts.begin(),cuts.end(),[](double t){return std::abs(t-.1)<1e-10;}));
  layers.FreezeOuterNormal();CHECK_THROWS(layers.SelectOuterNormal(.75));CHECK_THROWS(layers.ConfigureOuterNormal(.03,2.));
  CBoundaryLayerMetric invalid(3,{wall});CHECK_THROWS(invalid.ConfigureOuterNormal(.001,2.));CHECK_THROWS(invalid.ConfigureOuterNormal(.03,1.));
}

TEST_CASE("Interacting walls retain finer demands with one frozen outer-normal profile", "[NativeBLBudgetProfile]") {
  CBoundaryLayerMetric::Wall a;a.layer={"a",.01,1.2,.3};a.coord={0,0,0,1,0,0,1,1,0,0,1,0};a.conn={0,1,2,2,0,2,3,3};
  auto b=a;b.layer.marker="b";for(size_t i=2;i<b.coord.size();i+=3)b.coord[i]=.2;
  CBoundaryLayerMetric layers(3,{a,b});layers.ConfigureOuterNormal(.02,2.);layers.SelectOuterNormal(.75);
  const su2double p[]={.5,.5,.1};Tensor sensor;sensor.m[0][0]=1e5;sensor.m[1][1]=4;sensor.m[2][2]=1;
  auto direct=sensor;layers.ApplyPoint(p,direct,.1,nullptr,false);
  auto cached=sensor;const auto samples=layers.SamplePoints({p[0],p[1],p[2]});REQUIRE(samples.size()==2);
  for(const auto& sample:samples)layers.ApplySample(sample.wall,sample.sample,cached,.1,nullptr,false);
  for(unsigned i=0;i<3;++i)for(unsigned j=0;j<3;++j)CHECK(cached.m[i][j]==direct.m[i][j]);
  CHECK(direct.m[0][0]>=1e5*(1-1e-12));CHECK(direct.m[1][1]>=4*(1-1e-12));CHECK(direct.m[2][2]>=1*(1-1e-12));
  layers.FreezeOuterNormal();CHECK_THROWS(layers.SelectOuterNormal(1));
}
