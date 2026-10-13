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

#include "../../../Common/include/CConfig.hpp"
#include "../../../Common/include/adaptation/CNativeImport2D.hpp"
#include "../../../Common/include/adaptation/CNativeField3D.hpp"
#include "../../../Common/include/adaptation/CNativeImport3D.hpp"
#include "../../../Common/include/geometry/CPhysicalGeometry.hpp"
#include "../../../Common/include/geometry/meshreader/CMemoryMeshReaderFVM.hpp"
#include "../../../Common/include/linear_algebra/blas_structure.hpp"
#include "../../../Common/include/toolboxes/geometry_toolbox.hpp"
#include "../../../SU2_CFD/include/adaptation/CBoundaryLayerMetric.hpp"
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
    const auto rule=layers.IntegrationRule3D(cell,ratio);
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
                       "ADAP_SURFACE= YES\nADAP_ISO_CORNER= NO\nADAP_HGRAD= 1.3\nADAP_HMIN= .5\nADAP_HMAX= 1\n"
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
                       "ADAP_SURFACE= YES\nADAP_ISO_CORNER= NO\nADAP_HMIN= .5\nADAP_HMAX= 1\n"
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
  CBoundaryLayerMetric::Wall wall;wall.layer={"wall",.02,1.2,.08};
  wall.coord={.28,.18,0,.52,.18,0,.28,.42,0};wall.conn={0,1,2,2};
  CBoundaryLayerMetric layers(3,{wall});
  const std::array<std::array<double,3>,4> cell{{{{.1,.1,.02}},{{.9,.1,.02}},{{.1,.9,.02}},{{.1,.1,.8}}}};
  for(const auto& p:cell){const su2double x[3]={p[0],p[1],p[2]};CHECK(layers.Evaluate(0,x,1.).weight==0);}
  double volume=0,active=0;
  for(const auto& point:layers.IntegrationRule3D(cell)) {
    volume+=point.weight;
    if(!point.samples.empty())active+=point.weight;
  }
  CHECK(volume==Approx(.8*.8*.78/6).epsilon(1e-10));CHECK(active>0);
}
