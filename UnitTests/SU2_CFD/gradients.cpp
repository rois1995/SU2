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
#include <array>
#include "../../Common/include/geometry/CPhysicalGeometry.hpp"
#include "../../Common/include/parallelization/CPassiveComm.hpp"
#include <map>
#include "../../Common/include/containers/container_decorators.hpp"
#include "../../SU2_CFD/include/solvers/CSolver.hpp"
#include "../../SU2_CFD/include/adaptation/CAdapSensors.hpp"
#include "../../SU2_CFD/include/variables/CPrimitiveIndices.hpp"
#include "../../SU2_CFD/include/solvers/CSolverFactory.hpp"
#include "../../SU2_CFD/include/gradients/computeGradientsGreenGauss.hpp"
#include "../../SU2_CFD/include/gradients/computeGradientsLeastSquares.hpp"
#include "../../SU2_CFD/include/gradients/computeHessians.hpp"
#include "../../SU2_CFD/include/gradients/computeHessiansQuadratic.hpp"
#include "adaptation/TransferTestCase.hpp"

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
      aux_geometry->SetColorGrid_Parallel(config.get());
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

template <size_t nDim, bool periodic>
void testLeastSquaresScaling() {
  const su2double reference[3] = {1.0, -2.0, 3.0};
  for (const auto size : {1e-12, 1.0, 1e12}) {
    for (const auto aspect : {1.0, 1e6}) {
      for (const bool weighted : {false, true}) {
        CAPTURE(nDim, periodic, size, aspect, weighted);
        C3DDoubleMatrix R(1, nDim, nDim, 0.0), gradient(1, 1, nDim, 0.0);
        /*--- Opposite edges of a stretched stencil; assemble the actual LS normal equations. ---*/
        for (size_t axis = 0; axis < nDim; ++axis) {
          const su2double edge = size * (axis == 0 ? 1.0 / aspect : 1.0);
          const su2double weight = weighted ? 1.0 / (edge * edge) : 1.0;
          R(0, axis, axis) = 2 * edge * edge * weight;
          gradient(0, 0, axis) = R(0, axis, axis) * reference[axis];
        }
        detail::solveLeastSquares<nDim, periodic>(0, 0, 1, R, gradient);
        for (size_t axis = 0; axis < nDim; ++axis)
          CHECK(SU2_TYPE::GetValue(gradient(0, 0, axis)) == Approx(reference[axis]).margin(1e-12));
      }
    }
  }
  /*--- A stencil missing a coordinate direction must still be rejected. ---*/
  C3DDoubleMatrix R(1, nDim, nDim, 0.0), gradient(1, 1, nDim, 1.0);
  R(0, 0, 0) = 1.0;
  detail::solveLeastSquares<nDim, periodic>(0, 0, 1, R, gradient);
  for (size_t axis = 0; axis < nDim; ++axis) CHECK(gradient(0, 0, axis) == 0.0);
}

TEST_CASE("Least-squares coordinate scaling", "[Gradients][MetricRobustness]") {
  testLeastSquaresScaling<2, false>();
  testLeastSquaresScaling<3, false>();
  testLeastSquaresScaling<2, true>();
  testLeastSquaresScaling<3, true>();
}

TEST_CASE("Hessian stencil conditioning is independent of coordinate scales", "[HessianReliability]") {
  for (const unsigned short nDim : {2, 3}) {
    C3DDoubleMatrix R(1, nDim, nDim, 0.0);
    const passivedouble scale[3] = {1e-12, 1e6, 1.0};
    for (unsigned short i = 0; i < nDim; ++i) R(0, i, i) = scale[i] * scale[i];
    CHECK(detail::hessianStencilReciprocalCondition(nDim, 0, R) == Approx(1.0));
    R(0, 0, 1) = (1.0 - 1e-10) * scale[0] * scale[1];
    const auto poor = detail::hessianStencilReciprocalCondition(nDim, 0, R);
    CHECK(poor > 64 * std::numeric_limits<passivedouble>::epsilon());
    CHECK(poor < sqrt(std::numeric_limits<passivedouble>::epsilon()));
    for (unsigned short i = 0; i < nDim; ++i)
      for (unsigned short j = i; j < nDim; ++j) R(0, i, j) = scale[i] * scale[j];
    CHECK(detail::hessianStencilReciprocalCondition(nDim, 0, R) <= 64 * std::numeric_limits<passivedouble>::epsilon());
    R(0, 0, 1) = std::numeric_limits<passivedouble>::quiet_NaN();
    CHECK(detail::hessianStencilReciprocalCondition(nDim, 0, R) == 0.0);
    R(0, 0, 0) = 0.0;
    CHECK(detail::hessianStencilReciprocalCondition(nDim, 0, R) == 0.0);
  }
}

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

TEST_CASE("Hessian from analytic gradients on stretched meshes", "[Gradients][MetricRobustness]") {
  for (const auto method : {LEAST_SQUARES, WEIGHTED_LEAST_SQUARES}) {
    for (const auto size : {1e-7, 1.0, 1e7}) {
      for (const auto angle : {0.0, 0.37}) {
        CAPTURE(method, size, angle);
        QuadraticFunction field;
        const auto nPoint = field.geometry->GetnPoint();
        CHECK(field.geometry->GetnPointDomain() > 0);
        if (SU2_MPI::GetSize() > 1) CHECK(nPoint > field.geometry->GetnPointDomain());
        C3DDoubleMatrix R(nPoint, 3, 3), gradient(nPoint, 1, 3), hessian(nPoint, 1, 6), gradGrad(nPoint, 3, 3);
        su2activematrix gradientField(nPoint, 3);
        for (auto point = 0ul; point < nPoint; ++point) {
          /*--- Rotate a 1000:1 stencil, including halo coordinates and exact input gradients. ---*/
          const su2double x = size * field.geometry->nodes->GetCoord(point, 0);
          const su2double y = size * 1e-3 * field.geometry->nodes->GetCoord(point, 1);
          const su2double z = size * field.geometry->nodes->GetCoord(point, 2);
          field.geometry->nodes->SetCoord(point, 0, cos(angle) * x - sin(angle) * y);
          field.geometry->nodes->SetCoord(point, 1, sin(angle) * x + cos(angle) * y);
          field.geometry->nodes->SetCoord(point, 2, z);
          const auto coord = field.geometry->nodes->GetCoord(point);
          for (auto i = 0u; i < 3; ++i) gradient(point, 0, i) = GeometryToolbox::DotProduct(3, field.hess[i], coord);
        }
        computeHessians(nullptr, method, *field.geometry, *field.config, gradient, 0, 1, gradientField, gradGrad, R,
                        hessian);
        su2double error = 0.0;
        for (auto point = 0ul; point < field.geometry->GetnPointDomain(); ++point) {
          unsigned short component = 0;
          for (auto i = 0u; i < 3; ++i)
            for (auto j = i; j < 3; ++j, ++component)
              error = max(error, abs(hessian(point, 0, component) - field.hess[i][j]));
        }
        CHECK(error < 1e-7);
      }
    }
  }
}

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

  AdaptBoxTest(const string& markers, const string& method, const string& adaptOptions = "ADAP_SENSOR= (MACH)\n",
               unsigned long boxSize = 8, const string& solverKind = "EULER",
               passivedouble angle = 0.0, bool fullMirror = false) {
    const string configOptions =
        "SOLVER= " + solverKind + "\n"
        "MATH_PROBLEM= DIRECT\n"
        "MESH_FORMAT= BOX\n"
        "INIT_OPTION= TD_CONDITIONS\n" + markers +
        "MESH_BOX_SIZE= " + std::to_string(boxSize) + "," + std::to_string(boxSize) + "," + std::to_string(boxSize) + "\n"
        "MESH_BOX_LENGTH= " + (fullMirror ? string("1,1,2\nMESH_BOX_OFFSET= 0,0,-1\n") : string("1,1,1\nMESH_BOX_OFFSET= 0,0,0\n")) +
        "COMPUTE_METRIC= YES\n" + adaptOptions +
        "NUM_METHOD_HESS= " + method + "\n";

    auto origBuf = cout.rdbuf();
    cout.rdbuf(nullptr);
    stringstream ss(configOptions);
    config = std::unique_ptr<CConfig>(new CConfig(ss, SU2_COMPONENT::SU2_CFD, false));
    {
      auto aux_geometry = std::unique_ptr<CGeometry>(new CPhysicalGeometry(config.get(), 0, 1));
      aux_geometry->SetColorGrid_Parallel(config.get());
      geometry = std::unique_ptr<CGeometry>(new CPhysicalGeometry(aux_geometry.get(), config.get()));
    }
    if (angle != 0.0) {
      for (unsigned long point = 0; point < geometry->GetnPoint(); ++point) {
        const auto x = geometry->nodes->GetCoord(point, 0), z = geometry->nodes->GetCoord(point, 2);
        geometry->nodes->SetCoord(point, 0, cos(angle)*x - sin(angle)*z);
        geometry->nodes->SetCoord(point, 2, sin(angle)*x + cos(angle)*z);
      }
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
    if (solver != nullptr) {
      for (unsigned short iSol = 0; iSol < MAX_SOLS; ++iSol) {
        CSolverFactory::ClearSolverMeta(solver[iSol]);
        delete solver[iSol];
      }
    }
    delete[] solver;
  }
};

TEST_CASE("Hessian stencil diagnostics count owners across MPI partitions", "[HessianReliability]") {
  AdaptBoxTest test("MARKER_FAR= (x_minus, x_plus, y_minus, y_plus, z_minus, z_plus)\n", "WEIGHTED_LEAST_SQUARES");
  const auto c = cos(0.37), s = sin(0.37);
  auto* flow = test.solver[FLOW_SOL];
  for (auto point = 0ul; point < test.geometry->GetnPoint(); ++point) {
    const auto coord = test.geometry->nodes->GetCoord(point);
    const su2double x = coord[0] + 0.4 * coord[1], y = 1e-10 * coord[1], z = coord[2];
    test.geometry->nodes->SetCoord(point, 0, c * x - s * y);
    test.geometry->nodes->SetCoord(point, 1, s * x + c * y);
    flow->GetNodes()->SetAuxVar_Adapt(point, 0, x * x + y * y + z * z);
  }
  stringstream out;
  auto* buffer = cout.rdbuf(out.rdbuf());
  flow->SetHessian_Adapt(test.geometry.get(), test.config.get());
  cout.rdbuf(buffer);
  if (SU2_MPI::GetRank() == MASTER_NODE) {
    const auto expected = std::to_string(test.geometry->GetGlobal_nPointDomain()) + " numerically deficient";
    CHECK(out.str().find(expected) != string::npos);
    CHECK(out.str().find("Hessians may be inaccurate even when finite") != string::npos);
  }
}

TEST_CASE("Quadratic Hessians preserve signed curvature and report failed fits", "[HessianReliability]") {
  AdaptBoxTest test("MARKER_FAR= (x_minus, x_plus, y_minus, y_plus, z_minus, z_plus)\n", "QUADRATIC_LEAST_SQUARES",
                    "ADAP_SENSOR= (MACH, PRESSURE, DENSITY)\n");
  auto* flow = test.solver[FLOW_SOL];
  auto* nodes = flow->GetNodes();
  const passivedouble exact[6] = {1.0, 0.5, 0.0, 2.0, 0.0, 3.0};
  for (auto point = 0ul; point < test.geometry->GetnPoint(); ++point) {
    const auto x = test.geometry->nodes->GetCoord(point);
    const auto value = 0.5 * (x[0] * x[0] + x[0] * x[1] + 2 * x[1] * x[1] + 3 * x[2] * x[2]);
    nodes->SetAuxVar_Adapt(point, 0, value);
    nodes->SetAuxVar_Adapt(point, 1, -2 * value + 4 * x[0]);
    nodes->SetAuxVar_Adapt(point, 2, 7.0);
  }
  for (const bool invalidSensor : {false, true}) {
    if (invalidSensor) {
      for (auto point = 0ul; point < test.geometry->GetnPointDomain(); ++point)
        if (test.geometry->nodes->GetGlobalIndex(point) == 0)
          nodes->SetAuxVar_Adapt(point, 2, std::numeric_limits<passivedouble>::quiet_NaN());
    }
    stringstream out;
    auto* buffer = cout.rdbuf(out.rdbuf());
    flow->SetHessian_Adapt(test.geometry.get(), test.config.get());
    cout.rdbuf(buffer);
    for (auto point = 0ul; point < test.geometry->GetnPointDomain(); ++point) {
      for (unsigned short component = 0; component < 6; ++component) {
        CHECK(SU2_TYPE::GetValue(nodes->GetHessian()(point, 0, component)) == Approx(exact[component]).margin(1e-10));
        CHECK(SU2_TYPE::GetValue(nodes->GetHessian()(point, 1, component)) ==
              Approx(-2 * exact[component]).margin(1e-10));
        if (!invalidSensor)
          CHECK(SU2_TYPE::GetValue(nodes->GetHessian()(point, 2, component)) == Approx(0).margin(1e-10));
      }
    }
    if (SU2_MPI::GetRank() == MASTER_NODE) {
      if (invalidSensor)
        CHECK(out.str().find("WARNING: Quadratic") != string::npos);
      else
        CHECK(out.str().find("0 WLS fallbacks") != string::npos);
    }
  }

  /*--- A planar stencil cannot recover 3D curvature: preserve the supplied fallback, never silently zero it. ---*/
  for (auto point = 0ul; point < test.geometry->GetnPoint(); ++point) {
    test.geometry->nodes->SetCoord(point, 2, 0.0);
    for (unsigned short v = 0; v < 3; ++v) {
      for (unsigned short d = 0; d < 3; ++d) nodes->GetGradient_Adapt()(point, v, d) = 11.0;
      for (unsigned short k = 0; k < 6; ++k) nodes->GetHessian()(point, v, k) = 13.0;
    }
  }
  {
    transfer_test::Mute mute;
    computeHessiansQuadratic(*test.geometry, 3, nodes->GetAuxVar_Adapt(), nodes->GetGradient_Adapt(),
                             nodes->GetHessian());
  }
  for (auto point = 0ul; point < test.geometry->GetnPointDomain(); ++point)
    for (unsigned short v = 0; v < 3; ++v) {
      for (unsigned short d = 0; d < 3; ++d) CHECK(nodes->GetGradient_Adapt()(point, v, d) == 11.0);
      for (unsigned short k = 0; k < 6; ++k) CHECK(nodes->GetHessian()(point, v, k) == 13.0);
    }
}

/*--- Complete pressure sensor -> gradient -> Hessian chain on triangles/tetrahedra.
 *     Smooth grading and distortion vary the stencil with refinement. Error comparisons are made in
 *     local layer coordinates, so the physical normal curvature (O(aspect^2)) cannot hide tangential errors. ---*/
std::array<passivedouble, 5> pressureHessianErrors(unsigned short nDim, unsigned long n, bool distorted,
                                                   passivedouble aspect, passivedouble angle) {
  auto mesh = simplex_test::MakeSimplexMesh(
      nDim, n, [](const passivedouble* x) { return std::string(x[1] < 1e-12 ? "wall" : "far"); });
  const auto parameter = mesh.coord;
  const passivedouble c = cos(angle), s = sin(angle), pi = acos(-1.0);
  const passivedouble bend = distorted ? 0.02 : 0.0;
  for (auto point = 0ul; point < mesh.GetnPoint(); ++point) {
    auto* x = &mesh.coord[point * nDim];
    const auto eta = distorted ? pow(x[1], 1.3) : x[1];
    const auto u = x[0] + (distorted ? 0.1 * sin(pi * x[0]) * sin(pi * eta) : 0.0);
    const auto v = (eta + bend * sin(pi * u)) / aspect;
    x[0] = c * u - s * v;
    x[1] = s * u + c * v;
  }
  std::unique_ptr<CConfig> config;
  {
    transfer_test::Mute mute;
    stringstream options(
        "SOLVER= EULER\nMESH_FORMAT= SU2\nMESH_FILENAME= unused.su2\n"
        "MARKER_FAR= (far)\nMARKER_EULER= (wall)\nCOMPUTE_METRIC= YES\n"
        "ADAP_SENSOR= (PRESSURE)\nNUM_METHOD_HESS= QUADRATIC_LEAST_SQUARES\n");
    config = std::make_unique<CConfig>(options, SU2_COMPONENT::SU2_CFD, false);
  }
  transfer_test::MeshSolution state(config.get(), mesh, 0);
  auto& geometry = state.Fine();
  auto* flow = state.solver[MESH_0][FLOW_SOL];
  const auto idx = CPrimitiveIndices<unsigned short>(false, false, nDim, 0);
  const passivedouble H[3][3] = {{2.0, 0.5, -0.3}, {0.5, 4.0, 0.2}, {-0.3, 0.2, 6.0}};
  auto layerCoord = [&](unsigned long point, passivedouble* q) {
    const auto* x = geometry.nodes->GetCoord(point);
    q[0] = c * SU2_TYPE::GetValue(x[0]) + s * SU2_TYPE::GetValue(x[1]);
    q[1] = aspect * (-s * SU2_TYPE::GetValue(x[0]) + c * SU2_TYPE::GetValue(x[1])) - bend * sin(pi * q[0]);
    q[2] = nDim == 3 ? SU2_TYPE::GetValue(x[2]) : 0.0;
  };
  for (auto point = 0ul; point < geometry.GetnPoint(); ++point) {
    passivedouble q[3], pressure = 1.0;
    layerCoord(point, q);
    for (unsigned short i = 0; i < nDim; ++i)
      for (unsigned short j = 0; j < nDim; ++j) pressure += 0.5 * q[i] * H[i][j] * q[j];
    flow->GetNodes()->SetPrimitive(point, idx.Pressure(), pressure);
  }
  {
    transfer_test::Mute mute;
    flow->SetAuxVar_Adapt(&geometry, config.get(), state.solver[MESH_0]);
    flow->SetHessian_Adapt(&geometry, config.get());
  }
  std::array<passivedouble, 5> error = {};  // gradient, interior Hessian, wall normal Hessian, eigenvalues, direction
  unsigned long nInterior = 0, nWall = 0;
  for (auto point = 0ul; point < geometry.GetnPointDomain(); ++point) {
    passivedouble q[3], g[3] = {}, ref[3][3] = {};
    layerCoord(point, q);
    for (unsigned short i = 0; i < nDim; ++i) {
      for (unsigned short j = 0; j < nDim; ++j) {
        g[i] += H[i][j] * q[j];
        ref[i][j] = H[i][j];
      }
    }
    const auto first = bend * pi * cos(pi * q[0]), second = -bend * pi * pi * sin(pi * q[0]);
    ref[0][0] -= g[1] * second;
    const passivedouble T[3][3] = {
        {c - s * first / aspect, -s / aspect, 0.0}, {s + c * first / aspect, c / aspect, 0.0}, {0.0, 0.0, 1.0}};
    su2double tensor[3][3];
    flow->GetNodes()->GetHessianMat(point, 0, tensor);
    passivedouble recovered[3][3] = {};
    for (unsigned short i = 0; i < nDim; ++i) {
      passivedouble gradient = 0.0;
      for (unsigned short a = 0; a < nDim; ++a)
        gradient += T[a][i] * SU2_TYPE::GetValue(flow->GetNodes()->GetGradient_Adapt()(point, 0, a));
      error[0] = max(error[0], fabs(gradient - g[i]));
      for (unsigned short j = 0; j < nDim; ++j)
        for (unsigned short a = 0; a < nDim; ++a)
          for (unsigned short b = 0; b < nDim; ++b)
            recovered[i][j] += T[a][i] * SU2_TYPE::GetValue(tensor[a][b]) * T[b][j];
    }
    const auto* raw = &parameter[geometry.nodes->GetGlobalIndex(point) * nDim];
    if (raw[1] < 1e-12) {
      ++nWall;
      error[2] = max(error[2], fabs(recovered[1][1] / H[1][1] - 1.0));
    }
    bool interior = raw[0] >= 0.4 && raw[0] <= (nDim == 2 ? 1.6 : 0.6) && raw[1] >= 0.4 && raw[1] <= 0.6;
    if (nDim == 3) interior &= raw[2] >= 0.4 && raw[2] <= 0.6;
    if (!interior) continue;
    ++nInterior;
    for (unsigned short i = 0; i < nDim; ++i)
      for (unsigned short j = 0; j < nDim; ++j) error[1] = max(error[1], fabs(recovered[i][j] - ref[i][j]));
    passivedouble rvec[3][3], rval[3], cvec[3][3], cval[3], work[3], dot = 0.0;
    CBlasStructure::EigenDecomposition(ref, rvec, rval, nDim, work);
    CBlasStructure::EigenDecomposition(recovered, cvec, cval, nDim, work);
    for (unsigned short i = 0; i < nDim; ++i) {
      error[3] = max(error[3], fabs(cval[i] / rval[i] - 1.0));
      dot += rvec[i][nDim - 1] * cvec[i][nDim - 1];
    }
    error[4] = max(error[4], sqrt(max(0.0, 1.0 - dot * dot)));
  }
  auto globalError = error;
  CPassiveComm::Allreduce(error.data(), globalError.data(), error.size(), CPassiveComm::Op::MAX);
  CHECK(CPassiveComm::AllreduceSum(nInterior) > 0);
  CHECK(CPassiveComm::AllreduceSum(nWall) > 0);
  return globalError;
}

TEST_CASE("Pressure Hessian end-to-end convergence on boundary-layer meshes", "[HessianReliability]") {
  for (const unsigned short nDim : {2, 3}) {
    for (const bool distorted : {false, true}) {
      for (const auto aspect : {1.0, 1000.0}) {
        const auto angle = aspect == 1.0 ? 0.0 : 0.37;
        CAPTURE(nDim, distorted, aspect);
        const auto coarse = pressureHessianErrors(nDim, 8, distorted, aspect, angle);
        const auto fine = pressureHessianErrors(nDim, 16, distorted, aspect, angle);
        if (SU2_MPI::GetRank() == MASTER_NODE) {
          cout << "Pressure Hessian " << nDim << "D, distorted=" << distorted << ", AR=" << aspect
               << ": coarse [grad,H,wall,eigen,direction]=";
          for (const auto e : coarse) cout << " " << e;
          cout << "; fine=";
          for (const auto e : fine) cout << " " << e;
          cout << endl;
        }
        for (const auto e : fine) CHECK(std::isfinite(e));
        CHECK(fine[0] < 0.75 * coarse[0] + 1e-9);
        CHECK(fine[1] < 0.6 * coarse[1] + 1e-8);
        CHECK(fine[3] < 0.6 * coarse[3] + 1e-8);
        CHECK(fine[4] < 0.03);
        if (!distorted)
          CHECK(fine[2] < 1e-8);
        else
          CHECK(fine[2] < 0.75 * coarse[2] + 1e-8);
      }
    }
  }
}

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

/*!
 * \brief Unit cube with constant sensor Hessians (set directly, not computed) and the given adaptation options.
 */
struct ConstantHessianBoxTest : public AdaptBoxTest {
  ConstantHessianBoxTest(const string& adaptOptions, const vector<std::array<su2double, 6>>& hessians)
      : AdaptBoxTest("MARKER_FAR= (x_minus, x_plus, y_minus, y_plus, z_minus, z_plus)\n", "GREEN_GAUSS",
                     adaptOptions) {
    auto& H = solver[FLOW_SOL]->GetNodes()->GetHessian();
    for (auto iPoint = 0ul; iPoint < geometry->GetnPoint(); ++iPoint)
      for (auto iSensor = 0ul; iSensor < hessians.size(); ++iSensor)
        for (auto iMet = 0u; iMet < 6; ++iMet) H(iPoint, iSensor, iMet) = hessians[iSensor][iMet];
  }

  /*!
   * \brief Compute the metric, return the screen output.
   */
  string ComputeMetric() {
    stringstream out;
    auto origBuf = cout.rdbuf();
    cout.rdbuf(out.rdbuf());
    solver[FLOW_SOL]->ComputeMetric(geometry.get(), config.get());
    cout.rdbuf(origBuf);
    return out.str();
  }

  /*!
   * \brief Sorted eigenvalues of the metric at a point.
   */
  void Eigenvalues(unsigned long iPoint, su2double (&val)[3]) const {
    su2double M[3][3], vec[3][3], work[3];
    solver[FLOW_SOL]->GetNodes()->GetMetricMat(iPoint, M);
    CBlasStructure::EigenDecomposition(M, vec, val, 3, work);
  }

  /*!
   * \brief Complexity of the metric, integral of sqrt(det(M)).
   */
  su2double Complexity() const {
    su2double complexity = 0.0;
    for (auto iPoint = 0ul; iPoint < geometry->GetnPointDomain(); ++iPoint) {
      su2double val[3];
      Eigenvalues(iPoint, val);
      complexity += sqrt(val[0] * val[1] * val[2]) * geometry->nodes->GetVolume(iPoint);
    }
    return complexity;
  }

  /*!
   * \brief Largest relative difference between the metric eigenvalues and the reference, over all points.
   */
  su2double EigenvalueError(const su2double (&ref)[3]) const {
    su2double err = 0.0;
    for (auto iPoint = 0ul; iPoint < geometry->GetnPoint(); ++iPoint) {
      su2double val[3];
      Eigenvalues(iPoint, val);
      for (auto i = 0u; i < 3; ++i) err = max(err, abs(val[i] - ref[i]) / ref[i]);
    }
    return err;
  }
};

/*--- Two sensors with eigenvalues (100, 1, 1) and different principal directions (the second rotated by 45 degrees
 *    about z): each sensor metric has eigenvalues (1, 1, 100) for ADAP_COMPLEXITY= 10, their intersection
 *    (1, 30.4, 167.7) has a complexity of 71.4, a size of 0.077 and an aspect ratio of 12.95. ---*/
const std::array<su2double, 6> hessianX = {100.0, 0.0, 0.0, 1.0, 0.0, 1.0};
const std::array<su2double, 6> hessianXY = {50.5, 49.5, 0.0, 50.5, 0.0, 1.0};

TEST_CASE("Non-finite Hessian diagnostics", "[Adaptation][MetricRobustness]") {
  ConstantHessianBoxTest test("ADAP_SENSOR= (MACH, PRESSURE)\nADAP_COMPLEXITY= 10\n", {hessianX, hessianXY});
  auto& H = test.solver[FLOW_SOL]->GetNodes()->GetHessian();
  /*--- Corrupt two owned points per rank, including their halo copies and multiple components at one point.
   *     Halo copies must not inflate the count; repeated metric evaluations must not either. ---*/
  const unsigned long local[2] = {test.geometry->nodes->GetGlobalIndex(0), test.geometry->nodes->GetGlobalIndex(1)};
  vector<unsigned long> corrupt(2 * SU2_MPI::GetSize());
  SU2_MPI::Allgather(local, 2, MPI_UNSIGNED_LONG, corrupt.data(), 2, MPI_UNSIGNED_LONG, SU2_MPI::GetComm());
  for (auto point = 0ul; point < test.geometry->GetnPoint(); ++point) {
    const auto global = test.geometry->nodes->GetGlobalIndex(point);
    const auto found = find(corrupt.begin(), corrupt.end(), global);
    if (found == corrupt.end()) continue;
    H(point, 0, 5) = -std::numeric_limits<passivedouble>::infinity();
    if ((found - corrupt.begin()) % 2 == 0) {
      H(point, 0, 0) = std::numeric_limits<passivedouble>::quiet_NaN();
      H(point, 0, 3) = std::numeric_limits<passivedouble>::infinity();
    }
  }
  for (unsigned short repeat = 0; repeat < 2; ++repeat) {
    const auto output = test.ComputeMetric();
    if (SU2_MPI::GetRank() == MASTER_NODE) {
      const string expected =
          "sensor 1: replaced non-finite Hessians by zero at " + std::to_string(corrupt.size()) + " owned mesh points";
      CHECK(output.find(expected) != string::npos);
      CHECK(output.find("sensor 2: replaced") == string::npos);
    }
    for (auto point = 0ul; point < test.geometry->GetnPoint(); ++point) {
      su2double val[3];
      test.Eigenvalues(point, val);
      for (const auto eigenvalue : val) {
        CHECK(std::isfinite(SU2_TYPE::GetValue(eigenvalue)));
        CHECK(eigenvalue > 0.0);
      }
    }
  }
}

TEST_CASE("Metric bounds after the intersection", "[Adaptation]") {
  /*--- Reference eigenvalues of the final metric: bounded intersection, scaled to the complexity. ---*/
  const string sensors = "ADAP_SENSOR= (MACH, PRESSURE)\nADAP_COMPLEXITY= 10\n";
  const struct {
    string bounds;
    su2double hmin, hmax, armax, ref[3];
  } cases[] = {{"ADAP_HMIN= 0.1\nADAP_HMAX= 10\nADAP_ARMAX= 10\n", 0.1, 10.0, 10.0, {0.3806839, 6.90034786, 38.06838978}},
               {"ADAP_HMIN= 0.2\nADAP_HMAX= 10\nADAP_ARMAX= 10\n", 0.2, 10.0, 10.0, {0.36273708, 11.02727091, 25.0}}};

  for (const auto& c : cases) {
    ConstantHessianBoxTest test(sensors + c.bounds, {hessianX, hessianXY});
    const auto output = test.ComputeMetric();
    CHECK(output.find("WARNING") == string::npos);

    const su2double tol = 1e-12;
    bool inBounds = true;
    for (auto iPoint = 0ul; iPoint < test.geometry->GetnPoint(); ++iPoint) {
      su2double val[3];
      test.Eigenvalues(iPoint, val);
      inBounds &= val[2] <= (1 + tol) / pow(c.hmin, 2) && val[0] >= (1 - tol) / pow(c.hmax, 2);
      inBounds &= sqrt(val[2] / val[0]) <= (1 + tol) * c.armax;
    }
    CHECK(inBounds);
    CHECK(test.EigenvalueError(c.ref) < 1e-6);
    CHECK(test.Complexity() == Approx(10.0).epsilon(1e-6));
  }
}

TEST_CASE("Metric complexity", "[Adaptation]") {
  const string loose = "ADAP_HMIN= 0.01\nADAP_HMAX= 10\nADAP_ARMAX= 1000\n";

  /*--- One sensor: the Lp-optimal metric, C^(2/n) det(|H|)^(-1/n) |H| for a constant Hessian on a unit volume. ---*/
  {
    ConstantHessianBoxTest test("ADAP_SENSOR= (MACH)\nADAP_COMPLEXITY= 10\n" + loose, {hessianXY});
    test.ComputeMetric();
    const su2double factor = pow(10.0, 2.0 / 3.0) * pow(100.0, -1.0 / 3.0);
    su2double err = 0.0;
    for (auto iPoint = 0ul; iPoint < test.geometry->GetnPoint(); ++iPoint)
      for (auto iMet = 0u; iMet < 6; ++iMet)
        err = max(err, abs(test.solver[FLOW_SOL]->GetNodes()->GetMetric(iPoint, iMet) - factor * hessianXY[iMet]));
    CHECK(err < 1e-12);
    CHECK(test.Complexity() == Approx(10.0).epsilon(1e-12));
  }

  /*--- Two sensors with inactive bounds: the final metric, not each sensor metric, has the target complexity
   *    (it was 71.4, and 100 for orthogonal principal directions). ---*/
  const string sensors = "ADAP_SENSOR= (MACH, PRESSURE)\nADAP_COMPLEXITY= 10\n";
  {
    ConstantHessianBoxTest test(sensors + loose, {hessianX, hessianXY});
    const auto output = test.ComputeMetric();
    CHECK(output.find("WARNING") == string::npos);
    CHECK(test.Complexity() == Approx(10.0).epsilon(1e-6));
    const su2double ref[3] = {0.26968167, 8.1983702, 45.22942297};
    CHECK(test.EigenvalueError(ref) < 1e-6);
  }
  {
    const std::array<su2double, 6> hessianY = {1.0, 0.0, 0.0, 100.0, 0.0, 1.0};
    ConstantHessianBoxTest test(sensors + loose, {hessianX, hessianY});
    test.ComputeMetric();
    CHECK(test.Complexity() == Approx(10.0).epsilon(1e-6));
  }

  /*--- Bounds that do not allow the target: ADAP_HMAX= 0.2 gives at least 125, ADAP_HMIN= 1 at most 1. The metric
   *    uses the limiting size everywhere and a warning reports it. ---*/
  const struct {
    string bounds;
    su2double complexity, eigenvalue;
  } infeasible[] = {{"ADAP_HMIN= 0.01\nADAP_HMAX= 0.2\nADAP_ARMAX= 1000\n", 125.0, 25.0},
                    {"ADAP_HMIN= 1\nADAP_HMAX= 10\nADAP_ARMAX= 1000\n", 1.0, 1.0}};
  for (const auto& c : infeasible) {
    ConstantHessianBoxTest test(sensors + c.bounds, {hessianX, hessianXY});
    const auto output = test.ComputeMetric();
    CHECK(output.find("WARNING") != string::npos);
    CHECK(output.find("cannot be reached") != string::npos);
    CHECK(test.Complexity() == Approx(c.complexity).epsilon(1e-12));
    const su2double ref[3] = {c.eigenvalue, c.eigenvalue, c.eigenvalue};
    CHECK(test.EigenvalueError(ref) < 1e-12);
  }
}

/*--- The custom path uses the same cached primitives and scalar differentiation as built-in sensors. ---*/
TEST_CASE("Custom pressure and current density identity", "[Adaptation][CustomSensors]") {
  for (const auto* method : {"GREEN_GAUSS", "WEIGHTED_LEAST_SQUARES"}) {
    AdaptBoxTest test("MARKER_FAR= (x_minus, x_plus, y_minus, y_plus, z_minus, z_plus)\n", method,
                      "ADAP_SENSOR= (PRESSURE, P, DENSITY, D)\nADAP_CUSTOM_SENSORS= 'P : PRESSURE; D : DENSITY'\n");
    const auto idx = CPrimitiveIndices<unsigned short>(false, false, 3, 0);
    auto* flow = test.solver[FLOW_SOL];
    auto* nodes = flow->GetNodes();
    std::vector<su2double> solution;
    for (unsigned long point = 0; point < test.geometry->GetnPoint(); ++point) {
      const auto x = test.geometry->nodes->GetCoord(point);
      nodes->SetPrimitive(point, idx.Pressure(), 1 + x[0]*x[0] + 2*x[1]*x[1]);
      nodes->SetPrimitive(point, idx.Density(), 123.0);  // Deliberately stale density cache.
      nodes->SetSolution(point, 0, 2 + x[0]*x[0]);
      for (unsigned short var = 0; var < flow->GetnVar(); ++var) solution.push_back(nodes->GetSolution(point, var));
    }
    flow->SetAuxVar_Adapt(test.geometry.get(), test.config.get(), test.solver);
    flow->SetHessian_Adapt(test.geometry.get(), test.config.get());
    for (unsigned long point = 0; point < test.geometry->GetnPoint(); ++point) {
      CHECK(nodes->GetAuxVar_Adapt(point, 0) == nodes->GetAuxVar_Adapt(point, 1));
      CHECK(nodes->GetAuxVar_Adapt(point, 2) == nodes->GetAuxVar_Adapt(point, 3));
      CHECK(nodes->GetAuxVar_Adapt(point, 2) != 123.0);
      for (unsigned short dim = 0; dim < 3; ++dim) {
        CHECK(nodes->GetGradient_Adapt()(point, 0, dim) == nodes->GetGradient_Adapt()(point, 1, dim));
        CHECK(nodes->GetGradient_Adapt()(point, 2, dim) == nodes->GetGradient_Adapt()(point, 3, dim));
      }
      for (unsigned short component = 0; component < 6; ++component) {
        CHECK(nodes->GetHessian(point, 0, component) == nodes->GetHessian(point, 1, component));
        CHECK(nodes->GetHessian(point, 2, component) == nodes->GetHessian(point, 3, component));
      }
      for (unsigned short var = 0; var < flow->GetnVar(); ++var)
        CHECK(nodes->GetSolution(point, var) == solution[point * flow->GetnVar() + var]);
      CHECK(nodes->GetPrimitive(point, idx.Density()) == 123.0);
    }
  }
}

TEST_CASE("Custom input gradient linear and cubic fields", "[Adaptation][CustomSensors]") {
  for (const auto* method : {"GREEN_GAUSS", "WEIGHTED_LEAST_SQUARES"}) {
    AdaptBoxTest test("MARKER_FAR= (x_minus, x_plus, y_minus, y_plus, z_minus, z_plus)\n", method,
                      "ADAP_SENSOR= (L, C)\nADAP_CUSTOM_SENSORS= 'L : GRAD_PRESSURE_X; C : GRAD_TEMPERATURE_X'\n");
    const auto idx = CPrimitiveIndices<unsigned short>(false, false, 3, 0);
    auto* flow = test.solver[FLOW_SOL];
    auto* nodes = flow->GetNodes();
    for (unsigned long point = 0; point < test.geometry->GetnPoint(); ++point) {
      const auto x = test.geometry->nodes->GetCoord(point);
      nodes->SetPrimitive(point, idx.Pressure(), 1 + 2*x[0] - x[1] + 3*x[2]);
      nodes->SetPrimitive(point, idx.Temperature(), x[0]*x[0]*x[0]);
    }
    flow->SetAuxVar_Adapt(test.geometry.get(), test.config.get(), test.solver);
    flow->SetHessian_Adapt(test.geometry.get(), test.config.get());
    unsigned long checked = 0;
    for (unsigned long point = 0; point < test.geometry->GetnPointDomain(); ++point) {
      const auto x = test.geometry->nodes->GetCoord(point);
      if (x[0] < 0.4 || x[0] > 0.6 || x[1] < 0.4 || x[1] > 0.6 || x[2] < 0.4 || x[2] > 0.6) continue;
      ++checked;
      CHECK(SU2_TYPE::GetValue(nodes->GetAuxVar_Adapt(point, 0)) == Approx(2.0).margin(1e-10));
      CHECK(SU2_TYPE::GetValue(nodes->GetHessian(point, 1, 0)) == Approx(6.0).margin(1e-8));
      for (unsigned short comp = 1; comp < 6; ++comp)
        CHECK(SU2_TYPE::GetValue(nodes->GetHessian(point, 1, comp)) == Approx(0.0).margin(1e-8));
    }
    CHECK(CPassiveComm::AllreduceSum(checked) > 0);
  }
}

TEST_CASE("Custom translational periodic staging exceeds primitive storage", "[Adaptation][CustomSensors]") {
  AdaptBoxTest test("MARKER_PERIODIC= (x_minus, x_plus, 0,0,0, 0,0,0, 1,0,0)\n"
                    "MARKER_FAR= (y_minus, y_plus, z_minus, z_plus)\n", "WEIGHTED_LEAST_SQUARES",
                    "ADAP_SENSOR= (S)\nADAP_CUSTOM_SENSORS= 'S : GRAD_VELOCITY_X_Y+GRAD_TEMPERATURE_Y"
                    "+GRAD_PRESSURE_Y+GRAD_DENSITY_Y+GRAD_ENTHALPY_Y+GRAD_SOUND_SPEED_Y"
                    "+GRAD_LAMINAR_VISCOSITY_Y+GRAD_EDDY_VISCOSITY_Y+GRAD_THERMAL_CONDUCTIVITY_Y"
                    "+GRAD_CP_TOTAL_Y+GRAD_TURB[0]_Y'\nKIND_TURB_MODEL= SA\n", 8, "RANS");
  const auto idx = CPrimitiveIndices<unsigned short>(false, false, 3, 0);
  auto* flow = test.solver[FLOW_SOL];
  auto* nodes = flow->GetNodes();
  for (unsigned long point = 0; point < test.geometry->GetnPoint(); ++point) {
    const auto y = test.geometry->nodes->GetCoord(point, 1);
    for (const auto var : {idx.Velocity(), idx.Temperature(), idx.Pressure(), idx.Enthalpy(), idx.SoundSpeed(),
                           idx.LaminarViscosity(), idx.EddyViscosity(), idx.ThermalConductivity(), idx.CpTotal()})
      nodes->SetPrimitive(point, var, 2 + y);
    nodes->SetSolution(point, 0, 2+y);
    test.solver[TURB_SOL]->GetNodes()->SetSolution(point, 0, 2+y);
  }
  CAdapSensors sensors(*test.config, *test.geometry, test.solver);
  CHECK(sensors.GetnStaged() == 13);
  CHECK(sensors.GetnWork() > flow->GetnVar());
  CHECK(sensors.GetnWork() > flow->GetnPrimVar());
  sensors.Sample(*flow, *test.geometry, *test.config, test.solver);
  for (unsigned long point = 0; point < test.geometry->GetnPointDomain(); ++point) {
    CHECK(SU2_TYPE::GetValue(nodes->GetAuxVar_Adapt(point, 0)) == Approx(11.0).margin(1e-10));
    for (unsigned short pad = 1; pad < sensors.GetnWork(); ++pad) CHECK(nodes->GetAuxVar_Adapt(point, pad) == 0.0);
  }
}

TEST_CASE("Custom rotational periodic cascade is rejected", "[Adaptation][CustomSensors]") {
  AdaptBoxTest test("MARKER_PERIODIC= (y_minus, x_minus, 0,0,0, 0,0,90, 0,0,0)\n"
                    "MARKER_FAR= (x_plus, y_plus, z_minus, z_plus)\n", "WEIGHTED_LEAST_SQUARES",
                    "ADAP_SENSOR= (S)\nADAP_CUSTOM_SENSORS= 'V : VELOCITY_X; S : V*V'\n");
  CHECK_THROWS_AS(CAdapSensors(*test.config, *test.geometry, test.solver), std::invalid_argument);
}

TEST_CASE("Custom pressure metric is bitwise identical to built-in", "[Adaptation][CustomSensors]") {
  const string markers = "MARKER_FAR= (x_minus, x_plus, y_minus, y_plus, z_minus, z_plus)\n";
  for (const auto* method : {"GREEN_GAUSS", "WEIGHTED_LEAST_SQUARES"}) {
    AdaptBoxTest legacy(markers, method, "ADAP_SENSOR= (PRESSURE)\n");
    AdaptBoxTest custom(markers, method, "ADAP_SENSOR= (P)\nADAP_CUSTOM_SENSORS= 'G : GRAD_PRESSURE_X; P : PRESSURE'\n");
    const auto idx = CPrimitiveIndices<unsigned short>(false, false, 3, 0);
    for (auto* test : {&legacy, &custom}) {
      auto* flow = test->solver[FLOW_SOL];
      for (unsigned long point = 0; point < test->geometry->GetnPoint(); ++point) {
        const auto x = test->geometry->nodes->GetCoord(point);
        flow->GetNodes()->SetPrimitive(point, idx.Pressure(), 1 + x[0]*x[0] + 2*x[1]*x[1] + 3*x[2]*x[2]);
      }
      flow->SetAuxVar_Adapt(test->geometry.get(), test->config.get(), test->solver);
      flow->SetHessian_Adapt(test->geometry.get(), test->config.get());
      flow->ComputeMetric(test->geometry.get(), test->config.get());
    }
    auto gather = [](const AdaptBoxTest& test) {
      vector<unsigned long> ids;
      vector<std::array<passivedouble, 6>> rows;
      for (unsigned long p = 0; p < test.geometry->GetnPointDomain(); ++p) {
        ids.push_back(test.geometry->nodes->GetGlobalIndex(p));
        std::array<passivedouble, 6> row{};
        for (unsigned short c = 0; c < 6; ++c) row[c] = SU2_TYPE::GetValue(test.solver[FLOW_SOL]->GetNodes()->GetMetric(p,c));
        rows.push_back(row);
      }
      ids = CPassiveComm::Allgatherv(ids, nullptr);
      rows = CPassiveComm::Allgatherv(rows, nullptr);
      REQUIRE(ids.size() == rows.size());
      std::map<unsigned long, std::array<passivedouble, 6>> result;
      for (size_t p = 0; p < ids.size(); ++p) REQUIRE(result.emplace(ids[p],rows[p]).second);
      return result;
    };
    CHECK(gather(legacy) == gather(custom));
  }
}

TEST_CASE("Custom Mach square analytic Hessian converges", "[Adaptation][CustomSensors]") {
  passivedouble errors[2] = {};
  for (unsigned short refinement = 0; refinement < 2; ++refinement) {
    const unsigned long size = refinement == 0 ? 12 : 24;
    AdaptBoxTest test("MARKER_FAR= (x_minus, x_plus, y_minus, y_plus, z_minus, z_plus)\n", "WEIGHTED_LEAST_SQUARES",
                      "ADAP_SENSOR= (S)\nADAP_CUSTOM_SENSORS= 'S : MACH*MACH'\n", size);
    const auto idx = CPrimitiveIndices<unsigned short>(false, false, 3, 0);
    auto* flow = test.solver[FLOW_SOL];
    auto* nodes = flow->GetNodes();
    for (unsigned long point = 0; point < test.geometry->GetnPoint(); ++point) {
      const auto x = test.geometry->nodes->GetCoord(point, 0);
      nodes->SetSolution(point, 0, 1.0);
      nodes->SetSolution(point, 1, exp(x));
      nodes->SetSolution(point, 2, 0.0);
      nodes->SetSolution(point, 3, 0.0);
      nodes->SetVelocity(point);
      nodes->SetPrimitive(point, idx.SoundSpeed(), 1.0);
    }
    flow->SetAuxVar_Adapt(test.geometry.get(), test.config.get(), test.solver);
    flow->SetHessian_Adapt(test.geometry.get(), test.config.get());
    unsigned long checked = 0;
    for (unsigned long point = 0; point < test.geometry->GetnPointDomain(); ++point) {
      const auto x = test.geometry->nodes->GetCoord(point);
      CHECK(SU2_TYPE::GetValue(nodes->GetAuxVar_Adapt(point, 0)) == Approx(SU2_TYPE::GetValue(exp(2*x[0]))));
      if (x[0] < 0.3 || x[0] > 0.7 || x[1] < 0.3 || x[1] > 0.7 || x[2] < 0.3 || x[2] > 0.7) continue;
      ++checked;
      errors[refinement] = max(errors[refinement], fabs(SU2_TYPE::GetValue(nodes->GetHessian(point, 0, 0) / (4*exp(2*x[0])) - 1)));
    }
    REQUIRE(CPassiveComm::AllreduceSum(checked) > 0);
    errors[refinement] = CPassiveComm::Allreduce(errors[refinement], CPassiveComm::Op::MAX);
  }
  cout << "Custom MACH*MACH Hessian maximum relative errors: coarse=" << errors[0]
       << " fine=" << errors[1] << endl;
  CHECK(errors[0] < 0.02);
  CHECK(errors[1] < 0.3*errors[0]);
}

TEST_CASE("Custom full velocity block on oblique symmetry plane", "[Adaptation][CustomSensors]") {
  const string definitions = "ADAP_SENSOR= (S)\nADAP_CUSTOM_SENSORS= 'S : GRAD_VELOCITY_X_X"
                             "+GRAD_PRESSURE_X*GRAD_PRESSURE_X+GRAD_PRESSURE_Y*GRAD_PRESSURE_Y"
                             "+GRAD_PRESSURE_Z*GRAD_PRESSURE_Z'\n";
  const passivedouble angle = PI_NUMBER/4;
  for (const auto* method : {"GREEN_GAUSS", "WEIGHTED_LEAST_SQUARES"}) {
    AdaptBoxTest half("MARKER_SYM= (z_minus)\nMARKER_FAR= (x_minus, x_plus, y_minus, y_plus, z_plus)\n",
                      method, definitions, 8, "EULER", angle);
    AdaptBoxTest full("MARKER_FAR= (x_minus, x_plus, y_minus, y_plus, z_minus, z_plus)\n",
                      method, definitions, 16, "EULER", angle, true);
    for (auto* test : {&half, &full}) {
      const auto idx = CPrimitiveIndices<unsigned short>(false, false, 3, 0);
      auto* nodes = test->solver[FLOW_SOL]->GetNodes();
      for (unsigned long point = 0; point < test->geometry->GetnPoint(); ++point) {
        const auto x = test->geometry->nodes->GetCoord(point);
        const auto t = cos(angle)*x[0]+sin(angle)*x[2], n = -sin(angle)*x[0]+cos(angle)*x[2];
        nodes->SetPrimitive(point, idx.Velocity(), 2*cos(angle)*t-3*sin(angle)*n);
        nodes->SetPrimitive(point, idx.Velocity()+1, 0.0);
        nodes->SetPrimitive(point, idx.Velocity()+2, 2*sin(angle)*t+3*cos(angle)*n);
        nodes->SetPrimitive(point, idx.Pressure(), 1+t+n*n);
      }
      CAdapSensors sensors(*test->config, *test->geometry, test->solver);
      CHECK(sensors.GetnStaged() == 4);
      sensors.Sample(*test->solver[FLOW_SOL], *test->geometry, *test->config, test->solver);
      unsigned long onPlane = 0, checked = 0;
      for (unsigned long point = 0; point < test->geometry->GetnPointDomain(); ++point) {
        const auto x = test->geometry->nodes->GetCoord(point);
        const auto t = cos(angle)*x[0]+sin(angle)*x[2], n = -sin(angle)*x[0]+cos(angle)*x[2];
        if (t < 0.2 || t > 0.8 || x[1] < 0.2 || x[1] > 0.8 || n < -1e-10 || n > 0.6) continue;
        ++checked;
        if (fabs(SU2_TYPE::GetValue(n)) < 1e-10) ++onPlane;
        CHECK(SU2_TYPE::GetValue(nodes->GetAuxVar_Adapt(point, 0)) == Approx(SU2_TYPE::GetValue(3.5+4*n*n)).margin(1e-9));
      }
      CHECK(CPassiveComm::AllreduceSum(checked) > 0);
      if (test == &half) CHECK(CPassiveComm::AllreduceSum(onPlane) > 0);
    }
  }
}

TEST_CASE("Custom scalar aliases with component-like names support rotational periodicity", "[Adaptation][CustomSensors]") {
  AdaptBoxTest test("MARKER_PERIODIC= (y_minus, x_minus, 0,0,0, 0,0,90, 0,0,0)\n"
                    "MARKER_FAR= (x_plus, y_plus, z_minus, z_plus)\n", "WEIGHTED_LEAST_SQUARES",
                    "ADAP_SENSOR= (S)\nADAP_CUSTOM_SENSORS= 'MyVELOCITY_X : PRESSURE; S : MyVELOCITY_X'\n");
  CHECK_NOTHROW(CAdapSensors(*test.config, *test.geometry, test.solver));
}
