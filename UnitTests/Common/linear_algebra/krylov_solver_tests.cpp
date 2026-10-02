/*!
 * \file krylov_solver_tests.cpp
 * \brief Unit tests for edge cases of the Krylov solvers (FGCRODR recycling, restarted FGMRES).
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
#include "../../../Common/include/CConfig.hpp"
#include "../../../Common/include/linear_algebra/CSysSolve.hpp"
#include "../../../Common/include/linear_algebra/CMatrixVectorProduct.hpp"
#include "../../../Common/include/linear_algebra/CPreconditioner.hpp"

#include <cmath>
#include <functional>
#include <limits>
#include <memory>
#include <sstream>

namespace {
using Scalar = su2mixedfloat;
using Vec = CSysVector<Scalar>;

/*--- Serial matrix-free operator, counts the products and can inject a NaN in one of them. ---*/
class TestProduct final : public CMatrixVectorProduct<Scalar> {
 public:
  std::function<void(const Vec&, Vec&)> apply;
  mutable unsigned long calls = 0;
  unsigned long nanAtCall = 0;  // 1-based, 0 = never

  void operator()(const Vec& u, Vec& v) const override {
    ++calls;
    apply(u, v);
    if (calls == nanAtCall) v[0] = std::numeric_limits<Scalar>::quiet_NaN();
  }
};

/*--- Non-symmetric tridiagonal matrix (diagonal 4, lower -1, upper -2), its eigenvalues are real. ---*/
TestProduct Tridiagonal() {
  TestProduct p;
  p.apply = [](const Vec& u, Vec& v) {
    const auto n = u.GetLocSize();
    for (auto i = 0ul; i < n; ++i) {
      Scalar s = 4 * u[i];
      if (i > 0) s -= u[i - 1];
      if (i + 1 < n) s -= 2 * u[i + 1];
      v[i] = s;
    }
  };
  return p;
}

class Identity final : public CPreconditioner<Scalar> {
 public:
  void operator()(const Vec& u, Vec& v) const override { v = u; }
  bool IsIdentity() const override { return true; }
};

std::unique_ptr<CConfig> MakeConfig(const std::string& options) {
  std::stringstream ss("SOLVER= EULER\n" + options);
  return std::unique_ptr<CConfig>(new CConfig(ss, SU2_COMPONENT::SU2_CFD, false));
}

Vec MakeRhs(unsigned long n) {
  Vec b(n, n, 1, 0.0);
  for (auto i = 0ul; i < n; ++i) b[i] = 1 + 0.01 * i * i;
  return b;
}

/*--- Relative true residual |b - Ax| / |b|. ---*/
Scalar TrueResidual(const TestProduct& A, const Vec& b, const Vec& x) {
  Vec Ax(b.GetNBlk(), b.GetNBlkDomain(), 1, 0.0);
  A.apply(x, Ax);
  Scalar r2 = 0, b2 = 0;
  for (auto i = 0ul; i < b.GetLocSize(); ++i) {
    r2 += pow(b[i] - Ax[i], 2);
    b2 += pow(b[i], 2);
  }
  return sqrt(r2 / b2);
}
}  // namespace

TEST_CASE("FGCRODR SAME_MAT solves with decreasing subspace size", "[LinearAlgebra]") {
  /*--- Same pattern as the Newton-Krylov solver of CDiscAdjMultizoneDriver, the deflation vectors of a
   * solve were reused by the next one even if its subspace was smaller. ---*/
  auto config = MakeConfig("");
  const auto A = Tridiagonal();
  const auto b = MakeRhs(200);
  Vec x(200, 200, 1, 0.0);

  CSysSolve<Scalar> solver;
  for (const unsigned long maxIter : {18ul, 2ul, 1ul}) {
    Scalar res = -1;
    const auto iter = solver.FGCRODR_LinSolver(b, x, A, Identity(), 1e-30, maxIter, res, false, config.get(),
                                               FgcrodrMode::SAME_MAT, maxIter);
    CHECK(iter <= maxIter);
    CHECK(res < 1);
  }
}

TEST_CASE("FGCRODR orthogonalization failure on the first direction after a restart", "[LinearAlgebra]") {
  auto config = MakeConfig("LINEAR_SOLVER_RESTART_FREQUENCY= 5\nLINEAR_SOLVER_RESTART_DEFLATION= 2\n");
  auto A = Tridiagonal();
  /*--- Product 1 is the initial residual, 2-6 are the first cycle, 7 is the first new direction of the second. ---*/
  A.nanAtCall = 7;
  const auto b = MakeRhs(50);
  Vec x(50, 50, 1, 0.0);
  Scalar res = -1;

  CSysSolve<Scalar> solver;
  solver.FGCRODR_LinSolver(b, x, A, Identity(), 1e-14, 40, res, false, config.get());

  CHECK(A.calls == 7);
  /*--- The solution of the first cycle is kept. ---*/
  for (auto i = 0ul; i < x.GetLocSize(); ++i) CHECK(std::isfinite(x[i]));
  CHECK(TrueResidual(Tridiagonal(), b, x) < 1);
}

TEST_CASE("FGMRES monitoring frequency 0", "[LinearAlgebra]") {
  auto config = MakeConfig("");
  const auto A = Tridiagonal();
  const auto b = MakeRhs(50);
  Vec x(50, 50, 1, 0.0);
  Scalar res = -1;

  CSysSolve<Scalar> solver;
  /*--- 0 is treated as 1 (it used to be converted to bool and to cause a division by 0). ---*/
  solver.SetMonitoringFrequency(0);
  solver.FGMRES_LinSolver(b, x, A, Identity(), 1e-10, 100, res, true, config.get());

  CHECK(res < 1e-10);
}

TEST_CASE("Restarted FGMRES with relative tolerance", "[LinearAlgebra]") {
  /*--- The initial guess is close to the solution, so the tolerance relative to the initial residual
   * is much tighter than the same tolerance relative to |b|. ---*/
  auto config = MakeConfig("LINEAR_SOLVER_RESTART_FREQUENCY= 5\n");
  const auto A = Tridiagonal();
  constexpr unsigned long n = 50;

  Vec xExact(n, n, 1, 1.0), b(n, n, 1, 0.0), x(n, n, 1, 0.0);
  A.apply(xExact, b);
  for (auto i = 0ul; i < n; ++i) x[i] = xExact[i] + 1e-3 * sin(i);
  const auto res0 = TrueResidual(A, b, x);
  REQUIRE(res0 < 1e-2);

  CSysSolve<Scalar> solver;
  solver.SetToleranceType(LinearToleranceType::RELATIVE);
  Scalar res = -1;
  solver.RFGMRES_LinSolver(b, x, A, Identity(), 1e-6, 200, res, false, config.get());

  CHECK(res <= 1e-6);
  CHECK(TrueResidual(A, b, x) < 2e-6 * res0);
}
