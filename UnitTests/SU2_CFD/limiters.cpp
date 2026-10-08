/*!
 * \file limiters.cpp
 * \brief Unit tests for the limiter functions.
 * \author A. Rausa
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
#include "../../SU2_CFD/include/solvers/CSolver.hpp"
#include "../../SU2_CFD/include/limiters/computeLimiters.hpp"
#include "../../SU2_CFD/include/numerics/util.hpp"
#include "adaptation/TransferTestCase.hpp"
#include "../UnitQuadTestCase.hpp"

/*--- The limiters are evaluated with the projection and the difference to the neighbor maximum (both >= 0)
 * and with the projection and the difference to the neighbor minimum (both <= 0). A limiter function must
 * then give the same value for (proj, delta) and (-proj, -delta). ---*/

TEST_CASE("Nishikawa limiters are symmetric", "[Limiters]") {
  using Helpers = LimiterHelpers<>;
  const su2double eps = 1e-3;

  for (const su2double proj : {0.1, 0.5, 1.0, 2.0}) {
    for (const su2double delta : {0.1, 0.5, 1.0, 1.9}) {
      CHECK(Helpers::r3Function(-proj, -delta, eps) == Approx(Helpers::r3Function(proj, delta, eps)));
      CHECK(Helpers::r4Function(-proj, -delta, eps) == Approx(Helpers::r4Function(proj, delta, eps)));
      CHECK(Helpers::r5Function(-proj, -delta, eps) == Approx(Helpers::r5Function(proj, delta, eps)));
    }
  }

  /*--- Nishikawa (AIAA 2022-1374): with a = |delta| = 1 and b = |proj| = 1, S4 = 6 and R4 = 7/8. ---*/
  CHECK(Helpers::r4Function(1.0, 1.0, 0.0) == Approx(0.875));
  CHECK(Helpers::r4Function(-1.0, -1.0, 0.0) == Approx(0.875));
}

TEST_CASE("Scalar reconstruction selects finite admissible lanes", "[Limiters][ScalarBounds]") {
  const su2double first = 0.25;
  for (const su2double value : {-2.0, 2.0, std::numeric_limits<double>::infinity(),
                                -std::numeric_limits<double>::infinity(), std::numeric_limits<double>::quiet_NaN()})
    CHECK(boundedReconstruction(value, first, -1.0, 1.0) == first);
  for (const su2double value : {-1.0, -0.5, 0.0, 1.0})
    CHECK(boundedReconstruction(value, first, -1.0, 1.0) == value);
  simd::Array<su2double, 4> face, cell(first);
  face[0] = -0.5; face[1] = 2.0; face[2] = std::numeric_limits<double>::infinity(); face[3] = 1.0;
  const auto result = boundedReconstruction(face, cell, -1.0, 1.0);
  CHECK(result[0] == -0.5);
  CHECK(result[1] == first);
  CHECK(result[2] == first);
  CHECK(result[3] == 1.0);
}

TEST_CASE("Local limiter length and relative scaling in 2D and 3D", "[Limiters][LimiterScaling]") {
  using Helpers = LimiterHelpers<>;
  for (const unsigned short nDim : {2, 3}) {
    auto config = transfer_test::MakeConfig(nDim,
        "SOLVER= EULER\nLIMITER_LOCAL_LENGTH= YES\nVENKAT_LIMITER_COEFF= 2.0\n");
    auto global = transfer_test::MakeConfig(nDim, "SOLVER= EULER\nVENKAT_LIMITER_COEFF= 2.0\n");
    CHECK_FALSE(global->GetLimiterLocalLength());
    transfer_test::MeshSolution domain(config.get(), transfer_test::BoxMesh(nDim, 2, false), 0);
    auto& geometry = domain.Fine();
    const auto nPoint = geometry.GetnPoint();
    su2activematrix field(nPoint, 1), fieldMin(nPoint, 1), fieldMax(nPoint, 1), limiter(nPoint, 1), reference(nPoint, 1);
    C3DDoubleMatrix gradient(nPoint, 1, nDim);
    for (const auto kind : {LIMITER::VENKATAKRISHNAN, LIMITER::NISHIKAWA_R3,
                           LIMITER::NISHIKAWA_R4, LIMITER::NISHIKAWA_R5}) {
      for (const su2double volume : {nDim == 2 ? PI_NUMBER / 4.0 : PI_NUMBER / 6.0, 1e-24, 0.0}) {
        CAPTURE(nDim, kind, volume);
        const su2double amplitude = volume == 1e-24 ? 1e-8 : 1.0;
        for (auto point = 0ul; point < nPoint; ++point) {
          geometry.nodes->SetVolume(point, 0.75 * volume);
          geometry.nodes->SetPeriodicVolume(point, 0.25 * volume);
          const auto x = geometry.nodes->GetCoord(point, 0);
          field(point, 0) = amplitude * (2.0 + x * x);
          for (unsigned short dim = 0; dim < nDim; ++dim)
            gradient(point, 0, dim) = dim == 0 ? amplitude * (4.0 + 2.0 * x) : 0.0;
        }
        SU2_OMP_PARALLEL {
          computeLimiters(kind, nullptr, MPI_QUANTITIES::SOLUTION, PERIODIC_NONE, PERIODIC_NONE,
                          geometry, *config, 0, 1, 0.0, field, gradient, fieldMin, fieldMax, limiter);
        }
        // A unit-diameter circle/sphere is the dimensional reference; tiny volumes hit the epsilon floor.
        const su2double length = volume > 1e-24 ? 1.0 : 0.0;
        su2double eps1 = length * config->GetVenkat_LimiterCoeff();
        for (const bool local : {true, false}) {
          if (!local) {
            SU2_OMP_PARALLEL {
              computeLimiters(kind, nullptr, MPI_QUANTITIES::SOLUTION, PERIODIC_NONE, PERIODIC_NONE,
                              geometry, *global, 0, 1, 0.0, field, gradient, fieldMin, fieldMax, limiter);
            }
            eps1 = global->GetRefElemLength() * global->GetVenkat_LimiterCoeff();
          }
          for (auto point = 0ul; point < geometry.GetnPointDomain(); ++point) {
            su2double pmin = 0.0, pmax = 0.0, dmin = 0.0, dmax = 0.0;
            for (auto neighbor : geometry.nodes->GetPoints(point)) {
              const auto proj = 0.5 * (geometry.nodes->GetCoord(neighbor, 0) - geometry.nodes->GetCoord(point, 0)) *
                                gradient(point, 0, 0);
              const auto delta = field(neighbor, 0) - field(point, 0);
              pmin = min(pmin, proj); pmax = max(pmax, proj);
              dmin = min(dmin, delta); dmax = max(dmax, delta);
            }
            auto expected = [&](su2double proj, su2double delta) {
              if (kind == LIMITER::VENKATAKRISHNAN)
                return Helpers::venkatFunction(proj, delta, max(pow(eps1, 3), Helpers::epsilon()));
              if (kind == LIMITER::NISHIKAWA_R3)
                return Helpers::r3Function(proj, delta, max(pow(eps1, 4), Helpers::epsilon()));
              if (kind == LIMITER::NISHIKAWA_R4)
                return Helpers::r4Function(proj, delta, max(pow(eps1, 5), Helpers::epsilon()));
              return Helpers::r5Function(proj, delta, max(pow(eps1, 6), Helpers::epsilon()));
            };
            CHECK(limiter(point, 0) == Approx(min(expected(pmin, dmin), expected(pmax, dmax))));
          }
        }
        for (const su2double refMagnitude : {2.0, 10.0}) {
          for (const su2double scale : {1.0, 1e-5, 1e8}) {
            const su2double ref = refMagnitude * scale * (scale == 1e8 ? -1.0 : 1.0);
            for (auto point = 0ul; point < nPoint; ++point) {
              const auto x = geometry.nodes->GetCoord(point, 0);
              field(point, 0) = scale * (2.0 + x * x);
              for (unsigned short dim = 0; dim < nDim; ++dim)
                gradient(point, 0, dim) = dim == 0 ? scale * (4.0 + 2.0 * x) : 0.0;
            }
            SU2_OMP_PARALLEL {
              computeLimiters(kind, nullptr, MPI_QUANTITIES::SOLUTION, PERIODIC_NONE, PERIODIC_NONE,
                              geometry, *config, 0, 1, 0.0, field, gradient, fieldMin, fieldMax, limiter, &ref);
            }
            for (auto point = 0ul; point < geometry.GetnPointDomain(); ++point) {
              if (scale == 1.0) reference(point, 0) = limiter(point, 0);
              else CHECK(limiter(point, 0) == Approx(reference(point, 0)));
            }
          }
        }
      }
    }
    field = su2double(0.0);
    for (auto point = 0ul; point < nPoint; ++point)
      for (unsigned short dim = 0; dim < nDim; ++dim) gradient(point, 0, dim) = 0.0;
    const su2double zeroReference = 0.0;
    SU2_OMP_PARALLEL {
      computeLimiters(LIMITER::VENKATAKRISHNAN, nullptr, MPI_QUANTITIES::SOLUTION, PERIODIC_NONE, PERIODIC_NONE,
                      geometry, *config, 0, 1, 0.0, field, gradient, fieldMin, fieldMax, limiter, &zeroReference);
    }
    for (auto point = 0ul; point < geometry.GetnPointDomain(); ++point) CHECK(limiter(point, 0) == 1.0);
  }
}

TEST_CASE("Compressible update relaxation uses internal energy density", "[Limiters][UnderRelaxation]") {
  UnitQuadTestCase domain;
  domain.AddOption("MGLEVEL= 0");
  domain.InitConfig(); domain.InitGeometry(); domain.InitSolver();
  auto* solver = static_cast<CEulerSolver*>(domain.solver[FLOW_SOL]);
  auto* nodes = solver->GetNodes();
  const auto nVar = solver->GetnVar();
  for (const su2double deltaRho : {0.0, 1.0, 1.5}) {
    for (auto point = 0ul; point < domain.geometry->GetnPointDomain(); ++point) {
      for (unsigned short var = 0; var < nVar; ++var) {
        nodes->SetSolution(point, var, 0.0);
        solver->LinSysSol(point, var) = 0.0;
      }
      nodes->SetSolution(point, 0, 10.0);
      nodes->SetSolution(point, 1, 20.0);
      nodes->SetSolution(point, nVar - 1, 30.0);
      solver->LinSysSol(point, 0) = deltaRho;
      solver->LinSysSol(point, nVar - 1) = 0.1;
    }
    SU2_OMP_PARALLEL { solver->ComputeUnderRelaxationFactor(domain.config.get()); }
    const auto ratio = max(fabs(deltaRho) / 10.0, fabs(30.1 - 200.0 / (10.0 + deltaRho) - 10.0) / 10.0);
    const auto expected = min(domain.config->GetMaxUpdateFractionFlow() / ratio, 1.0);
    for (auto point = 0ul; point < domain.geometry->GetnPointDomain(); ++point)
      CHECK(nodes->GetUnderRelaxation(point) == Approx(expected));
  }
}
