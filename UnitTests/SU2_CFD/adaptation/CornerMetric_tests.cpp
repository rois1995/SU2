/*!
 * \file CornerMetric_tests.cpp
 * \brief Unit tests for the isotropic adaptation metric at sharp wall corners (ADAP_ISO_CORNER).
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
#include "../../../Common/include/geometry/CPhysicalGeometry.hpp"
#include "../../../Common/include/geometry/meshreader/CMemoryMeshReaderFVM.hpp"
#include "../../../Common/include/linear_algebra/blas_structure.hpp"
#include "../../../SU2_CFD/include/drivers/CDriver.hpp"
#include "../../../SU2_CFD/include/solvers/CSolver.hpp"
#include "../../../SU2_CFD/include/solvers/CSolverFactory.hpp"
#include "../../Common/adaptation/SimplexMeshTestCase.hpp"

namespace {

/*--- Silence the console in a scope. ---*/
struct Mute {
  std::streambuf* buffer = cout.rdbuf();
  Mute() { cout.rdbuf(nullptr); }
  ~Mute() { cout.rdbuf(buffer); }
};

/*--- Rectangle [0,2]x[0,1] (16 cells per unit length) around a triangular obstacle with the corners A = (0.75, 0.5),
 *    B = (1, 0.5) and C = (1, 0.75), edges along mesh edges. The wall turns by 135 degrees at A and C (a wedge of
 *    45 degrees, like a trailing edge) and by 90 degrees at B. Markers "wall" (obstacle) and "far" (rectangle). ---*/
constexpr passivedouble corners2D[3][2] = {{0.75, 0.5}, {1.0, 0.5}, {1.0, 0.75}};

CSimplexMesh WedgeMesh() {
  auto markerOf = [](const passivedouble* x) {
    const passivedouble tol = 1e-12;
    const bool outer = x[0] < tol || x[0] > 2.0 - tol || x[1] < tol || x[1] > 1.0 - tol;
    return std::string(outer ? "far" : "wall");
  };
  auto keepElem = [](const passivedouble* x) { return !(x[1] > 0.5 && x[0] < 1.0 && x[1] - 0.5 < x[0] - 0.75); };
  return simplex_test::MakeSimplexMesh(2, 16, markerOf, keepElem);
}

/*!
 * \brief Geometry and flow solver of a mesh in memory, with a constant Hessian of one sensor (MACH).
 */
struct CornerTest {
  std::unique_ptr<CConfig> config;
  CGeometry** geometry = nullptr;
  CSolver** solver = nullptr;

  CornerTest(const CSimplexMesh& mesh, const string& options, const std::array<su2double, 6>& hessian) {
    Mute mute;
    const string solverOption = (options.find("SOLVER=") == string::npos) ? "SOLVER= EULER\n" : "";
    stringstream ss(solverOption + "MACH_NUMBER= 0.5\nMESH_FORMAT= SU2\nMESH_FILENAME= unused.su2\nMGLEVEL= 0\n"
                    "COMPUTE_METRIC= YES\nADAP_SENSOR= (MACH)\nADAP_NORM= 2\nNUM_METHOD_HESS= GREEN_GAUSS\n" +
                    options);
    config = std::unique_ptr<CConfig>(new CConfig(ss, SU2_COMPONENT::SU2_CFD, false));
    CMemoryMeshReaderFVM reader(config.get(), mesh, 0, 1);
    CDriver::BuildGeometryFVM(config.get(), new CPhysicalGeometry(config.get(), reader, 1), geometry, true);
    solver = CSolverFactory::CreateSolverContainer(config->GetKind_Solver(), config.get(), geometry[MESH_0], 0);

    auto& H = solver[FLOW_SOL]->GetNodes()->GetHessian();
    const auto nMet = (mesh.nDim == 2) ? 3u : 6u;
    for (auto iPoint = 0ul; iPoint < Geometry().GetnPoint(); ++iPoint)
      for (auto iMet = 0u; iMet < nMet; ++iMet) H(iPoint, 0, iMet) = hessian[iMet];
  }

  ~CornerTest() {
    for (unsigned short iSol = 0; iSol < MAX_SOLS; ++iSol) {
      CSolverFactory::ClearSolverMeta(solver[iSol]);
      delete solver[iSol];
    }
    delete[] solver;
    delete geometry[MESH_0];
    delete[] geometry;
  }

  CGeometry& Geometry() const { return *geometry[MESH_0]; }

  /*!
   * \brief Compute the metric, return the screen output.
   */
  string ComputeMetric() {
    stringstream out;
    auto buffer = cout.rdbuf();
    cout.rdbuf(out.rdbuf());
    solver[FLOW_SOL]->ComputeMetric(geometry[MESH_0], config.get());
    cout.rdbuf(buffer);
    return out.str();
  }

  void Metric(unsigned long iPoint, su2double (&M)[3][3]) const {
    for (auto i = 0u; i < 3; ++i)
      for (auto j = 0u; j < 3; ++j) M[i][j] = 0.0;
    solver[FLOW_SOL]->GetNodes()->GetMetricMat(iPoint, M);
  }

  /*!
   * \brief Sorted eigenvalues of the metric at a point.
   */
  void Eigenvalues(unsigned long iPoint, su2double (&val)[3]) const {
    su2double M[3][3], vec[3][3], work[3];
    Metric(iPoint, M);
    CBlasStructure::EigenDecomposition(M, vec, val, Geometry().GetnDim(), work);
  }

  su2double Complexity() const {
    const auto nDim = Geometry().GetnDim();
    su2double complexity = 0.0;
    for (auto iPoint = 0ul; iPoint < Geometry().GetnPointDomain(); ++iPoint) {
      su2double val[3];
      Eigenvalues(iPoint, val);
      su2double det = 1.0;
      for (auto i = 0u; i < nDim; ++i) det *= val[i];
      complexity += sqrt(det) * Geometry().nodes->GetVolume(iPoint);
    }
    return complexity;
  }

  /*!
   * \brief Sharp wall points, as coordinates.
   */
  vector<std::array<su2double, 3>> SharpPoints() const {
    vector<std::array<su2double, 3>> points;
    for (const auto iPoint : CSolver::FindSharpWallPoints(geometry[MESH_0], config.get())) {
      std::array<su2double, 3> x = {0.0, 0.0, 0.0};
      for (auto iDim = 0u; iDim < Geometry().GetnDim(); ++iDim) x[iDim] = Geometry().nodes->GetCoord(iPoint, iDim);
      points.push_back(x);
    }
    return points;
  }

  su2double CornerDistance(unsigned long iPoint) const {
    su2double distance = 1e10;
    for (const auto& corner : corners2D)
      distance = min(distance, GeometryToolbox::Distance(2, Geometry().nodes->GetCoord(iPoint), corner));
    return distance;
  }
};

/*--- Hessian with eigenvalues 25 and 1, principal directions rotated by 30 degrees (aspect ratio 5). ---*/
const std::array<su2double, 6> hessian2D = {19.0, 10.392304845413264, 7.0, 0.0, 0.0, 0.0};

const string wallOptions = "MARKER_EULER= (wall)\nMARKER_FAR= (far)\nADAP_COMPLEXITY= 2000\nADAP_HMIN= 1e-4\n"
                           "ADAP_HMAX= 10\nADAP_ARMAX= 1000\n";

}  // namespace

TEST_CASE("Sharp wall points 2D", "[Adaptation]") {
  const auto mesh = WedgeMesh();

  /*--- All three corners of the obstacle with ADAP_ANGLE= 45 (default), only the two of 45 degrees with 100. Points
   *    where the wall meets the far field are not wall corners: none with the obstacle as far field. ---*/
  const struct {
    string options;
    vector<unsigned short> expected;
  } cases[] = {{wallOptions, {0, 1, 2}},
               {wallOptions + "ADAP_ANGLE= 100\n", {0, 2}},
               {"MARKER_FAR= (wall, far)\n", {}},
               {"MARKER_HEATFLUX= (wall, 0.0)\nMARKER_FAR= (far)\nSOLVER= NAVIER_STOKES\nREYNOLDS_NUMBER= 1e6\n", {0, 1, 2}}};

  for (const auto& c : cases) {
    CornerTest test(mesh, c.options, hessian2D);
    const auto points = test.SharpPoints();
    REQUIRE(points.size() == c.expected.size());
    for (const auto iCorner : c.expected) {
      bool found = false;
      for (const auto& x : points)
        found |= fabs(x[0] - corners2D[iCorner][0]) < 1e-12 && fabs(x[1] - corners2D[iCorner][1]) < 1e-12;
      CHECK(found);
    }
  }
}

TEST_CASE("Sharp wall points 3D", "[Adaptation]") {
  /*--- Unit cube, 4 cells per side, Euler walls everywhere: the points on the 12 edges are sharp, not those on the
   *    faces, nor those where the two coplanar markers z_minus_a and z_minus_b meet. ---*/
  const auto mesh = simplex_test::MakeSimplexMesh(3, 4, simplex_test::Marker3D);
  const string walls = "MARKER_EULER= (x_minus, x_plus, y_minus, y_plus, z_plus, z_minus_a, z_minus_b)\n";
  const std::array<su2double, 6> hessian = {25.0, 0.0, 0.0, 1.0, 0.0, 1.0};
  CornerTest test(mesh, walls + "ADAP_COMPLEXITY= 100\n", hessian);

  const auto points = test.SharpPoints();
  CHECK(points.size() == 12 * 3 + 8);
  bool onEdges = true;
  for (const auto& x : points) {
    int nOnFace = 0;
    for (auto iDim = 0u; iDim < 3; ++iDim) nOnFace += (x[iDim] < 1e-12 || x[iDim] > 1.0 - 1e-12);
    onEdges &= nOnFace >= 2;
  }
  CHECK(onEdges);

  /*--- The isotropic corner metric is applied in 2D only. ---*/
  const auto output = test.ComputeMetric();
  CHECK(output.find("2D only") != string::npos);
  CornerTest reference(mesh, walls + "ADAP_COMPLEXITY= 100\nADAP_ISO_CORNER= NO\n", hessian);
  reference.ComputeMetric();
  su2double diff = 0.0;
  for (auto iPoint = 0ul; iPoint < test.Geometry().GetnPoint(); ++iPoint)
    for (auto iMet = 0u; iMet < 6; ++iMet)
      diff = max(diff, fabs(test.solver[FLOW_SOL]->GetNodes()->GetMetric(iPoint, iMet) -
                            reference.solver[FLOW_SOL]->GetNodes()->GetMetric(iPoint, iMet)));
  CHECK(diff == 0.0);
}

TEST_CASE("Isotropic metric at sharp wall corners", "[Adaptation]") {
  const auto mesh = WedgeMesh();
  const su2double growth = log(1.3);

  /*--- Without the corner treatment the metric is the same everywhere. ---*/
  CornerTest reference(mesh, wallOptions + "ADAP_ISO_CORNER= NO\n", hessian2D);
  reference.ComputeMetric();
  su2double refVal[3];
  reference.Eigenvalues(0, refVal);
  CHECK(reference.Complexity() == Approx(2000.0).epsilon(1e-6));

  CornerTest test(mesh, wallOptions, hessian2D);
  const auto output = test.ComputeMetric();
  CHECK(output.find("WARNING") == string::npos);
  CHECK(output.find("Sharp wall corners: 3") != string::npos);

  /*--- The complexity is still the target. ---*/
  CHECK(test.Complexity() == Approx(2000.0).epsilon(1e-6));

  /*--- Metric away from the corners (the same everywhere there, scaled down by the global factor). ---*/
  const auto& geometry = test.Geometry();
  su2double farVal[3] = {0.0, 0.0, 0.0};
  unsigned long nFar = 0;
  for (auto iPoint = 0ul; iPoint < geometry.GetnPointDomain(); ++iPoint) {
    if (test.CornerDistance(iPoint) < 0.6) continue;
    su2double val[3];
    test.Eigenvalues(iPoint, val);
    if (nFar++ == 0) {
      for (auto i = 0u; i < 2; ++i) farVal[i] = val[i];
    }
    for (auto i = 0u; i < 2; ++i) CHECK(val[i] == Approx(farVal[i]).epsilon(1e-12));
  }
  REQUIRE(nFar > 0);
  CHECK(farVal[1] / farVal[0] == Approx(refVal[1] / refVal[0]).epsilon(1e-10));
  CHECK(farVal[1] < refVal[1]);

  /*--- At the corners: isotropic, with the smallest size of the metric (its largest eigenvalue). ---*/
  unsigned long nCorner = 0, nChanged = 0;
  for (auto iPoint = 0ul; iPoint < geometry.GetnPointDomain(); ++iPoint) {
    su2double val[3];
    test.Eigenvalues(iPoint, val);
    if (test.CornerDistance(iPoint) < 1e-12) {
      ++nCorner;
      CHECK(val[0] == Approx(farVal[1]).epsilon(1e-10));
      CHECK(val[1] == Approx(farVal[1]).epsilon(1e-10));
    }

    /*--- Everywhere: an intersection with an isotropic metric, i.e. the same eigenvectors as the anisotropic metric
     *    and eigenvalues that are not smaller. ---*/
    su2double M[3][3], A[3][3] = {{0.0}};
    test.Metric(iPoint, M);
    const su2double scale = farVal[1] / refVal[1];
    for (auto i = 0u; i < 2; ++i)
      for (auto j = 0u; j < 2; ++j) A[i][j] = scale * reference.solver[FLOW_SOL]->GetNodes()->GetMetric(0, i + j);
    su2double commutator = 0.0;
    for (auto i = 0u; i < 2; ++i)
      for (auto j = 0u; j < 2; ++j) {
        su2double c = 0.0;
        for (auto k = 0u; k < 2; ++k) c += M[i][k] * A[k][j] - A[i][k] * M[k][j];
        commutator = max(commutator, fabs(c));
      }
    CHECK(commutator < 1e-10 * farVal[1] * farVal[1]);
    CHECK(val[0] >= farVal[0] * (1 - 1e-10));
    CHECK(val[1] >= farVal[1] * (1 - 1e-10));
    nChanged += val[0] > farVal[0] * (1 + 1e-10);
  }
  CHECK(nCorner == 3);
  CHECK(nChanged > 10 * nCorner);

  /*--- Smooth blend: the largest size grows at most by log(ADAP_HGRAD) times the length of each edge (the gradation
   *    of the remesher); the smallest size stays that of the anisotropic metric. ---*/
  su2double worst = 0.0;
  for (auto iEdge = 0ul; iEdge < geometry.GetnEdge(); ++iEdge) {
    const auto iPoint = geometry.edges->GetNode(iEdge, 0), jPoint = geometry.edges->GetNode(iEdge, 1);
    su2double val_i[3], val_j[3];
    test.Eigenvalues(iPoint, val_i);
    test.Eigenvalues(jPoint, val_j);
    const su2double length = GeometryToolbox::Distance(2, geometry.nodes->GetCoord(iPoint), geometry.nodes->GetCoord(jPoint));
    worst = max(worst, fabs(1 / sqrt(val_i[0]) - 1 / sqrt(val_j[0])) / (growth * length));
  }
  CHECK(worst <= 1 + 1e-6);
  CHECK(worst > 0.5);
}
