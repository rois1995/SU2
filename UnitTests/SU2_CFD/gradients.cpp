/*!
 * \file gradients.cpp
 * \brief Unit tests for gradient calculation.
 * \author P. Gomes, T. Albring
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
#include "../../Common/include/geometry/CPhysicalGeometry.hpp"
#include "../../Common/include/containers/container_decorators.hpp"
#include "../../SU2_CFD/include/solvers/CSolver.hpp"
#include "../../SU2_CFD/include/solvers/CSolverFactory.hpp"
#include "../../SU2_CFD/include/gradients/computeGradientsGreenGauss.hpp"
#include "../../SU2_CFD/include/gradients/computeGradientsLeastSquares.hpp"
#include "../../SU2_CFD/include/gradients/computeHessians.hpp"

/*!
 * \brief Base class for gradient tests using a unit cube geometry.
 * Derived classes should implement operator (i,j), returning the value
 * of the test function, and method grad(i,j,k), returning the known
 * gradient.
 */
struct GradientTestBase {
  const string configOptions =
      "SOLVER= NAVIER_STOKES\n"
      "MESH_FORMAT= BOX\n"
      "INIT_OPTION= TD_CONDITIONS\n"
      "MARKER_HEATFLUX= (y_minus, 0.0, y_plus, 0.0)\n"
      "MARKER_FAR= (x_minus, x_plus, z_plus, z_minus)\n"
      "MESH_BOX_SIZE= 10,10,10\n"
      "MESH_BOX_LENGTH= 1,1,1\n"
      "MESH_BOX_OFFSET= 0,0,0\n";

  std::unique_ptr<CConfig> config;
  std::unique_ptr<CGeometry> geometry;

  GradientTestBase() {
    initConfig();
    initGeometry();
  }

  /*!
   * \brief Initialize the config structure
   */
  void initConfig() {
    auto origBuf = cout.rdbuf();
    cout.rdbuf(nullptr);
    stringstream ss(configOptions);
    config = std::unique_ptr<CConfig>(new CConfig(ss, SU2_COMPONENT::SU2_CFD, false));
    cout.rdbuf(origBuf);
  }

  /*!
   * \brief Initialize the geometry
   */
  void initGeometry() {
    auto origBuf = cout.rdbuf();
    cout.rdbuf(nullptr);
    {
      auto aux_geometry = std::unique_ptr<CGeometry>(new CPhysicalGeometry(config.get(), 0, 1));
      geometry = std::unique_ptr<CGeometry>(new CPhysicalGeometry(aux_geometry.get(), config.get()));
    }
    geometry->SetSendReceive(config.get());
    geometry->SetBoundaries(config.get());
    geometry->SetPoint_Connectivity();
    geometry->SetElement_Connectivity();
    geometry->SetBoundVolume();
    geometry->Check_IntElem_Orientation(config.get());
    geometry->Check_BoundElem_Orientation(config.get());
    geometry->SetEdges();
    geometry->SetVertex(config.get());
    geometry->SetControlVolume(config.get(), ALLOCATE);
    geometry->SetBoundControlVolume(config.get(), ALLOCATE);
    geometry->FindNormal_Neighbor(config.get());
    geometry->SetGlobal_to_Local_Point();
    geometry->PreprocessP2PComms(geometry.get(), config.get());

    cout.rdbuf(origBuf);
  }
};

struct LinearFunction : public GradientTestBase {
  const unsigned long nVar = 1;
  const su2double constant = -1.0;
  const su2double slope[3] = {1.0, 2.0, 3.0};

  /*!
   * \brief Return manufactured value.
   */
  su2double operator()(unsigned long iPoint, unsigned long) const {
    const auto coord = geometry->nodes->GetCoord(iPoint);
    return constant + GeometryToolbox::DotProduct(geometry->GetnDim(), slope, coord);
  }

  /*!
   * \brief Return reference value.
   */
  su2double grad(unsigned long, unsigned long, unsigned long iDim) const { return slope[iDim]; }
};

template <class T, class U>
void check(const T& ref, const U& calc, su2double tol = 1e-9) {
  su2double err = 0.0;
  for (auto iPoint = 0ul; iPoint < calc.length(); ++iPoint) {
    for (auto iVar = 0ul; iVar < calc.rows(); ++iVar)
      for (auto iDim = 0ul; iDim < calc.cols(); ++iDim)
        err = max(err, abs(calc(iPoint, iVar, iDim) - ref.grad(iPoint, iVar, iDim)));
  }
  CHECK(err < tol);
}

template <class TestField>
void testGreenGauss() {
  TestField field;
  C3DDoubleMatrix gradient(field.geometry->GetnPoint(), field.nVar, field.geometry->GetnDim());

  computeGradientsGreenGauss(nullptr, MPI_QUANTITIES::SOLUTION, PERIODIC_NONE, *field.geometry.get(),
                             *field.config.get(), field, 0, field.nVar, -1, gradient);
  check(field, gradient);
}

template <class TestField>
void testLeastSquares(bool weighted) {
  TestField field;
  const auto nDim = field.geometry->GetnDim();
  C3DDoubleMatrix R(field.geometry->GetnPoint(), nDim, nDim);
  C3DDoubleMatrix gradient(field.geometry->GetnPoint(), field.nVar, nDim);

  computeGradientsLeastSquares(nullptr, MPI_QUANTITIES::SOLUTION, PERIODIC_NONE, *field.geometry.get(),
                               *field.config.get(), weighted, field, 0, field.nVar, -1, gradient, R);
  check(field, gradient);
}

TEST_CASE("GG", "[Gradients]") { testGreenGauss<LinearFunction>(); }

TEST_CASE("LS", "[Gradients]") { testLeastSquares<LinearFunction>(false); }

TEST_CASE("WLS", "[Gradients]") { testLeastSquares<LinearFunction>(true); }

struct QuadraticFunction : public GradientTestBase {
  const unsigned long nVar = 1;
  const su2double slope[3] = {1.0, -2.0, 3.0};
  const su2double hess[3][3] = {{2.0, 0.5, -0.3}, {0.5, 4.0, 0.2}, {-0.3, 0.2, 6.0}};

  /*!
   * \brief Return manufactured value, 0.5 x^T H x + b^T x.
   */
  su2double operator()(unsigned long iPoint, unsigned long) const {
    const auto coord = geometry->nodes->GetCoord(iPoint);
    su2double val = 0.0;
    for (auto iDim = 0u; iDim < 3; ++iDim) {
      val += slope[iDim] * coord[iDim];
      for (auto jDim = 0u; jDim < 3; ++jDim) val += 0.5 * coord[iDim] * hess[iDim][jDim] * coord[jDim];
    }
    return val;
  }

  /*!
   * \brief Whether the point is at least two layers away from the boundaries, where both passes are exact.
   */
  bool interior(unsigned long iPoint) const {
    const auto coord = geometry->nodes->GetCoord(iPoint);
    for (auto iDim = 0u; iDim < 3; ++iDim)
      if (coord[iDim] < 0.2 - 1e-6 || coord[iDim] > 0.8 + 1e-6) return false;
    return true;
  }
};

template <class TestField>
void testHessian(ENUM_FLOW_GRADIENT method) {
  TestField field;
  const auto nDim = field.geometry->GetnDim();
  const auto nPoint = field.geometry->GetnPoint();
  C3DDoubleMatrix R(nPoint, nDim, nDim);
  C3DDoubleMatrix gradient(nPoint, field.nVar, nDim);
  C3DDoubleMatrix hessian(nPoint, field.nVar, 3 * (nDim - 1));

  if (method == GREEN_GAUSS) {
    computeGradientsGreenGauss(nullptr, MPI_QUANTITIES::SOLUTION, PERIODIC_NONE, *field.geometry.get(),
                               *field.config.get(), field, 0, field.nVar, -1, gradient);
  } else {
    computeGradientsLeastSquares(nullptr, MPI_QUANTITIES::SOLUTION, PERIODIC_NONE, *field.geometry.get(),
                                 *field.config.get(), true, field, 0, field.nVar, -1, gradient, R);
  }
  su2activematrix gradientField(nPoint, nDim);
  C3DDoubleMatrix gradGrad(nPoint, nDim, nDim);
  computeHessians(nullptr, method, *field.geometry.get(), *field.config.get(), gradient, 0, field.nVar,
                  gradientField, gradGrad, R, hessian);

  su2double err = 0.0;
  unsigned long nChecked = 0;
  for (auto iPoint = 0ul; iPoint < field.geometry->GetnPointDomain(); ++iPoint) {
    if (!field.interior(iPoint)) continue;
    ++nChecked;
    unsigned long iMet = 0;
    for (auto iDim = 0u; iDim < nDim; ++iDim)
      for (auto jDim = iDim; jDim < nDim; ++jDim, ++iMet)
        err = max(err, abs(hessian(iPoint, 0, iMet) - field.hess[iDim][jDim]));
  }
  CHECK(nChecked > 0);
  CHECK(err < 1e-9);
}

TEST_CASE("Hessian GG", "[Gradients]") { testHessian<QuadraticFunction>(GREEN_GAUSS); }

TEST_CASE("Hessian WLS", "[Gradients]") { testHessian<QuadraticFunction>(WEIGHTED_LEAST_SQUARES); }

TEST_CASE("Metric intersection", "[Adaptation]") {
  const su2double c = cos(0.3), s = sin(0.3);
  const su2double R[3][3] = {{c, -s, 0.0}, {s, c, 0.0}, {0.0, 0.0, 1.0}};
  const su2double eigA[3] = {1.0, 9.0, 4.0}, eigB[3] = {16.0, 4.0, 1.0}, eigC[3] = {16.0, 9.0, 4.0};

  /*--- Metrics with common eigenvectors R: the intersection takes the largest eigenvalues. ---*/
  su2double A[3][3], B[3][3], ref[3][3], C[3][3];
  CBlasStructure::EigenRecomposition(A, R, eigA, 3);
  CBlasStructure::EigenRecomposition(B, R, eigB, 3);
  CBlasStructure::EigenRecomposition(ref, R, eigC, 3);

  CSolver::IntersectMetrics(3, A, B, C);
  su2double err = 0.0;
  for (auto i = 0u; i < 3; ++i)
    for (auto j = 0u; j < 3; ++j) err = max(err, abs(C[i][j] - ref[i][j]));
  CHECK(err < 1e-12);

  /*--- In 2D, intersecting with a smaller metric changes nothing. ---*/
  su2double small[3][3] = {{0.5, 0.1, 0.0}, {0.1, 0.5, 0.0}, {0.0, 0.0, 0.0}};
  CSolver::IntersectMetrics(2, A, small, C);
  err = 0.0;
  for (auto i = 0u; i < 2; ++i)
    for (auto j = 0u; j < 2; ++j) err = max(err, abs(C[i][j] - A[i][j]));
  CHECK(err < 1e-12);
}

/*!
 * \brief Unit cube (8^3 hexahedra) with the given markers and a compressible flow solver, which computes the
 *        adaptation Hessians (MACH sensor) and provides the periodic communications.
 */
struct AdaptBoxTest {
  static constexpr su2double h = 0.125;
  std::unique_ptr<CConfig> config;
  std::unique_ptr<CGeometry> geometry;
  CSolver** solver = nullptr;

  AdaptBoxTest(const string& markers, const string& method) {
    const string configOptions =
        "SOLVER= EULER\n"
        "MESH_FORMAT= BOX\n"
        "INIT_OPTION= TD_CONDITIONS\n" + markers +
        "MESH_BOX_SIZE= 8,8,8\n"
        "MESH_BOX_LENGTH= 1,1,1\n"
        "MESH_BOX_OFFSET= 0,0,0\n"
        "COMPUTE_METRIC= YES\n"
        "ADAP_SENSOR= (MACH)\n"
        "NUM_METHOD_HESS= " + method + "\n";

    auto origBuf = cout.rdbuf();
    cout.rdbuf(nullptr);
    stringstream ss(configOptions);
    config = std::unique_ptr<CConfig>(new CConfig(ss, SU2_COMPONENT::SU2_CFD, false));
    {
      auto aux_geometry = std::unique_ptr<CGeometry>(new CPhysicalGeometry(config.get(), 0, 1));
      geometry = std::unique_ptr<CGeometry>(new CPhysicalGeometry(aux_geometry.get(), config.get()));
    }
    geometry->SetSendReceive(config.get());
    geometry->SetBoundaries(config.get());
    geometry->SetPoint_Connectivity();
    geometry->SetElement_Connectivity();
    geometry->SetBoundVolume();
    geometry->Check_IntElem_Orientation(config.get());
    geometry->Check_BoundElem_Orientation(config.get());
    geometry->SetEdges();
    geometry->SetVertex(config.get());
    geometry->SetControlVolume(config.get(), ALLOCATE);
    geometry->SetBoundControlVolume(config.get(), ALLOCATE);
    geometry->FindNormal_Neighbor(config.get());
    geometry->SetGlobal_to_Local_Point();
    geometry->PreprocessP2PComms(geometry.get(), config.get());
    geometry->MatchPeriodic(config.get(), 1);
    geometry->PreprocessPeriodicComms(geometry.get(), config.get());
    solver = CSolverFactory::CreateSolverContainer(config->GetKind_Solver(), config.get(), geometry.get(), 0);
    cout.rdbuf(origBuf);
  }

  ~AdaptBoxTest() {
    if (solver != nullptr) delete solver[FLOW_SOL];
    delete[] solver;
  }
};

/*!
 * \brief Unit cube with rotational periodicity: y_minus is mapped to x_minus by a rotation of 90 degrees
 *        about the z axis, the other faces are far-field.
 */
struct PeriodicBoxTest : public AdaptBoxTest {
  explicit PeriodicBoxTest(const string& method)
      : AdaptBoxTest("MARKER_PERIODIC= (y_minus, x_minus, 0.0, 0.0, 0.0, 0.0, 0.0, 90.0, 0.0, 0.0, 0.0)\n"
                     "MARKER_FAR= (x_plus, y_plus, z_minus, z_plus)\n",
                     method) {}

  /*!
   * \brief Points where the Hessian of a quadratic field is exact: two layers away from the far-field faces
   *        and from the rotation axis (where more than two partial control volumes meet).
   */
  bool exact(unsigned long iPoint) const {
    const auto x = geometry->nodes->GetCoord(iPoint);
    const su2double tol = 1e-6;
    if (x[0] > 1 - 2 * h + tol || x[1] > 1 - 2 * h + tol) return false;
    if (x[2] < 2 * h - tol || x[2] > 1 - 2 * h + tol) return false;
    return x[0] > 2 * h - tol || x[1] > 2 * h - tol;
  }
};

void testPeriodicHessian(const string& method) {
  PeriodicBoxTest test(method);
  auto* geometry = test.geometry.get();
  auto* config = test.config.get();
  auto* flow = test.solver[FLOW_SOL];
  auto* nodes = flow->GetNodes();

  /*--- Field invariant under the periodic rotation, its gradient is rotated across the periodic faces. ---*/
  const su2double a = 1.5, b = -0.7, c = 2.0;
  for (auto iPoint = 0ul; iPoint < geometry->GetnPoint(); ++iPoint) {
    const auto x = geometry->nodes->GetCoord(iPoint);
    nodes->SetAuxVar_Adapt(iPoint, 0, a * (x[0] * x[0] + x[1] * x[1]) + b * x[2] * x[2] + c * x[2]);
  }
  flow->SetHessian_Adapt(geometry, config);

  const su2double ref[6] = {2 * a, 0.0, 0.0, 2 * a, 0.0, 2 * b};
  su2double err = 0.0;
  unsigned long nChecked = 0, nPeriodicChecked = 0;
  for (auto iPoint = 0ul; iPoint < geometry->GetnPointDomain(); ++iPoint) {
    if (!test.exact(iPoint)) continue;
    ++nChecked;
    if (geometry->nodes->GetPeriodicBoundary(iPoint)) ++nPeriodicChecked;
    for (auto iMet = 0u; iMet < 6; ++iMet) err = max(err, abs(nodes->GetHessian(iPoint, 0, iMet) - ref[iMet]));
  }
  CHECK(nPeriodicChecked > 0);
  CHECK(nChecked > nPeriodicChecked);
  CHECK(err < 1e-9);

  /*--- The metric is finite and positive definite. ---*/
  auto origBuf = cout.rdbuf();
  cout.rdbuf(nullptr);
  flow->ComputeMetric(geometry, config);
  cout.rdbuf(origBuf);
  bool positive = true;
  for (auto iPoint = 0ul; iPoint < geometry->GetnPoint(); ++iPoint) {
    su2double M[3][3], vec[3][3], val[3], work[3];
    nodes->GetMetricMat(iPoint, M);
    CBlasStructure::EigenDecomposition(M, vec, val, 3, work);
    positive = positive && std::isfinite(SU2_TYPE::GetValue(val[0])) && val[0] > 0.0;
  }
  CHECK(positive);
}

TEST_CASE("Periodic Hessian GG", "[Adaptation]") { testPeriodicHessian("GREEN_GAUSS"); }

TEST_CASE("Periodic Hessian WLS", "[Adaptation]") { testPeriodicHessian("WEIGHTED_LEAST_SQUARES"); }

TEST_CASE("Periodic Hessian and metric copy", "[Adaptation]") {
  PeriodicBoxTest test("GREEN_GAUSS");
  auto* geometry = test.geometry.get();
  auto* config = test.config.get();
  auto* flow = test.solver[FLOW_SOL];
  auto* nodes = flow->GetNodes();

  /*--- Arbitrary symmetric tensors, different at each point. ---*/
  for (auto iPoint = 0ul; iPoint < geometry->GetnPoint(); ++iPoint) {
    su2double T[3][3];
    for (auto i = 0u; i < 3; ++i)
      for (auto j = i; j < 3; ++j) T[i][j] = T[j][i] = sin(1.0 + iPoint + 3 * i + 7 * j);
    nodes->SetMetricMat(iPoint, T);
    for (auto iMet = 0u; iMet < 6; ++iMet) nodes->GetHessian()(iPoint, 0, iMet) = nodes->GetMetric(iPoint, iMet);
  }
  flow->InitiatePeriodicComms(geometry, config, 1, PERIODIC_HESSIAN);
  flow->CompletePeriodicComms(geometry, config, 1, PERIODIC_HESSIAN);
  flow->InitiatePeriodicComms(geometry, config, 1, PERIODIC_METRIC);
  flow->CompletePeriodicComms(geometry, config, 1, PERIODIC_METRIC);

  /*--- On each pair of periodic points, T_x_minus = R T_y_minus R^T (rotation of 90 degrees about z). ---*/
  const su2double R[3][3] = {{0.0, -1.0, 0.0}, {1.0, 0.0, 0.0}, {0.0, 0.0, 1.0}};
  auto rotated = [&](const su2double (&T)[3][3], unsigned short i, unsigned short j) {
    su2double val = 0.0;
    for (auto k = 0u; k < 3; ++k)
      for (auto l = 0u; l < 3; ++l) val += R[i][k] * T[k][l] * R[j][l];
    return val;
  };
  const auto iMarker = config->GetMarker_All_TagBound("y_minus");
  REQUIRE(iMarker >= 0);
  su2double errHessian = 0.0, errMetric = 0.0;
  unsigned long nPairs = 0;
  for (auto iVertex = 0ul; iVertex < geometry->GetnVertex(iMarker); ++iVertex) {
    const auto iPoint = geometry->vertex[iMarker][iVertex]->GetNode();
    const auto jPoint = geometry->vertex[iMarker][iVertex]->GetDonorPoint();
    if (jPoint < 0 || static_cast<unsigned long>(jPoint) == iPoint) continue;
    ++nPairs;
    su2double Mi[3][3], Mj[3][3], Hi[3][3], Hj[3][3];
    nodes->GetMetricMat(iPoint, Mi);
    nodes->GetMetricMat(jPoint, Mj);
    nodes->GetHessianMat(iPoint, 0, Hi);
    nodes->GetHessianMat(jPoint, 0, Hj);
    for (auto i = 0u; i < 3; ++i) {
      for (auto j = 0u; j < 3; ++j) {
        errMetric = max(errMetric, abs(Mj[i][j] - rotated(Mi, i, j)));
        errHessian = max(errHessian, abs(Hj[i][j] - rotated(Hi, i, j)));
      }
    }
  }
  CHECK(nPairs > 0);
  CHECK(errHessian < 1e-12);
  CHECK(errMetric < 1e-12);
}

/*!
 * \brief Set the sensor to 0.5 x^T H x + b^T x on all points, compute its gradient and Hessian.
 */
void setQuadraticSensor(AdaptBoxTest& test, const su2double (&H)[3][3], const su2double (&b)[3]) {
  auto* nodes = test.solver[FLOW_SOL]->GetNodes();
  for (auto iPoint = 0ul; iPoint < test.geometry->GetnPoint(); ++iPoint) {
    const auto x = test.geometry->nodes->GetCoord(iPoint);
    su2double val = 0.0;
    for (auto iDim = 0u; iDim < 3; ++iDim) {
      val += b[iDim] * x[iDim];
      for (auto jDim = 0u; jDim < 3; ++jDim) val += 0.5 * x[iDim] * H[iDim][jDim] * x[jDim];
    }
    nodes->SetAuxVar_Adapt(iPoint, 0, val);
  }
  test.solver[FLOW_SOL]->SetHessian_Adapt(test.geometry.get(), test.config.get());
}

void testSymmetryHessian(const string& method) {
  AdaptBoxTest test("MARKER_SYM= (z_minus)\nMARKER_FAR= (x_minus, x_plus, y_minus, y_plus, z_plus)\n", method);
  auto* nodes = test.solver[FLOW_SOL]->GetNodes();
  const auto h = AdaptBoxTest::h;

  /*--- Field symmetric about the plane z = 0, its Hessian is exact on the plane too. ---*/
  const su2double H[3][3] = {{2.0, 0.5, 0.0}, {0.5, 4.0, 0.0}, {0.0, 0.0, 6.0}};
  const su2double b[3] = {1.0, -2.0, 0.0};
  setQuadraticSensor(test, H, b);

  const su2double tol = 1e-6;
  su2double err = 0.0;
  unsigned long nChecked = 0, nPlaneChecked = 0;
  for (auto iPoint = 0ul; iPoint < test.geometry->GetnPointDomain(); ++iPoint) {
    const auto x = test.geometry->nodes->GetCoord(iPoint);
    if (x[0] < 2 * h - tol || x[0] > 1 - 2 * h + tol || x[1] < 2 * h - tol || x[1] > 1 - 2 * h + tol) continue;
    if (x[2] > 1 - 2 * h + tol) continue;
    ++nChecked;
    if (x[2] < tol) ++nPlaneChecked;
    su2double Hc[3][3];
    nodes->GetHessianMat(iPoint, 0, Hc);
    for (auto i = 0u; i < 3; ++i)
      for (auto j = 0u; j < 3; ++j) err = max(err, abs(Hc[i][j] - H[i][j]));
  }
  CHECK(nPlaneChecked > 0);
  CHECK(nChecked > nPlaneChecked);
  CHECK(err < 1e-9);
}

TEST_CASE("Symmetry plane Hessian GG", "[Adaptation]") { testSymmetryHessian("GREEN_GAUSS"); }

TEST_CASE("Symmetry plane Hessian WLS", "[Adaptation]") { testSymmetryHessian("WEIGHTED_LEAST_SQUARES"); }

void testEulerWallHessian(const string& method) {
  AdaptBoxTest wall("MARKER_EULER= (z_minus)\nMARKER_FAR= (x_minus, x_plus, y_minus, y_plus, z_plus)\n", method);
  AdaptBoxTest far("MARKER_FAR= (x_minus, x_plus, y_minus, y_plus, z_minus, z_plus)\n", method);

  /*--- n.grad(phi) = 0 does not hold on an Euler wall, the sensor gradient and Hessian are not corrected
   *    there, they are the same as with a far-field marker. ---*/
  const su2double H[3][3] = {{2.0, 0.5, -0.3}, {0.5, 4.0, 0.2}, {-0.3, 0.2, 6.0}};
  const su2double b[3] = {1.0, -2.0, 3.0};
  setQuadraticSensor(wall, H, b);
  setQuadraticSensor(far, H, b);

  const auto* nodesWall = wall.solver[FLOW_SOL]->GetNodes();
  const auto* nodesFar = far.solver[FLOW_SOL]->GetNodes();
  su2double diff = 0.0, minNormalGrad = 1e10;
  for (auto iPoint = 0ul; iPoint < wall.geometry->GetnPointDomain(); ++iPoint) {
    for (auto iMet = 0u; iMet < 6; ++iMet)
      diff = max(diff, abs(nodesWall->GetHessian(iPoint, 0, iMet) - nodesFar->GetHessian(iPoint, 0, iMet)));
    if (wall.geometry->nodes->GetCoord(iPoint, 2) < 1e-6)
      minNormalGrad = min(minNormalGrad, abs(nodesWall->GetGradient_Adapt()(iPoint, 0, 2)));
  }
  CHECK(minNormalGrad > 1.0);
  CHECK(diff < 1e-12);
}

TEST_CASE("Euler wall Hessian GG", "[Adaptation]") { testEulerWallHessian("GREEN_GAUSS"); }

TEST_CASE("Euler wall Hessian WLS", "[Adaptation]") { testEulerWallHessian("WEIGHTED_LEAST_SQUARES"); }
