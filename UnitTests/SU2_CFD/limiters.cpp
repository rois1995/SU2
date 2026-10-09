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
#include "../../SU2_CFD/include/solvers/CTurbSolver.hpp"
#include "../../SU2_CFD/include/limiters/computeLimiters.hpp"
#include "../../SU2_CFD/include/numerics/util.hpp"
#include "../../SU2_CFD/include/numerics/turbulent/turb_sa_edge_flux.hpp"
#include "../../SU2_CFD/include/variables/CTurbSAVariable.hpp"
#include "adaptation/TransferTestCase.hpp"
#include "../UnitQuadTestCase.hpp"
#include "../../SU2_CFD/include/variables/CPrimitiveIndices.hpp"

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

TEST_CASE("Flow limiters preserve the physical result across reference units", "[Limiters][FlowLimiterScaling]") {
  for (const unsigned short nDim : {2, 3}) {
    const CEulerVariable::CIndices<unsigned short> indices(nDim, 0);
    for (const std::string flux : {"ROE", "HLLC"}) {
      for (const std::string kind : {"VENKATAKRISHNAN", "NISHIKAWA_R3", "NISHIKAWA_R4", "NISHIKAWA_R5"}) {
        su2activematrix reference;
        for (const std::string units : {"DIMENSIONAL", "FREESTREAM_VEL_EQ_MACH", "FREESTREAM_VEL_EQ_ONE"}) {
          CAPTURE(nDim, flux, kind, units);
          auto config = transfer_test::MakeConfig(nDim, "SOLVER= EULER\nMUSCL_FLOW= YES\n"
              "CONV_NUM_METHOD_FLOW= " + flux + "\nSLOPE_LIMITER_FLOW= " + kind +
              "\nREF_DIMENSIONALIZATION= " + units + "\n");
          transfer_test::MeshSolution domain(config.get(), transfer_test::BoxMesh(nDim, 2, false), 0);
          auto& geometry = domain.Fine();
          auto* flow = static_cast<CEulerSolver*>(domain.solver[0][FLOW_SOL]);
          auto* nodes = flow->GetNodes();
          auto& gradient = nodes->GetGradient_Reconstruction();
          const auto nPrim = flow->GetnPrimVarGrad();
          if (units == "DIMENSIONAL") reference.resize(geometry.GetnPointDomain(), nPrim);
          su2double scale[CEulerVariable::MAXNVAR] = {};
          scale[indices.Temperature()] = 1.0 / config->GetTemperature_Ref();
          scale[indices.Pressure()] = 1.0 / config->GetPressure_Ref();
          scale[indices.Density()] = 1.0 / config->GetDensity_Ref();
          scale[indices.Enthalpy()] = 1.0 / config->GetEnergy_Ref();
          for (unsigned short dim = 0; dim < nDim; ++dim) {
            scale[indices.Velocity() + dim] = 1.0 / config->GetVelocity_Ref();
            // Stagnant freestream exercises zero component references without a dimensional constant floor.
            flow->GetVelocity_Inf()[dim] = 0.0;
          }
          for (auto point = 0ul; point < geometry.GetnPoint(); ++point) {
            const auto x = geometry.nodes->GetCoord(point, 0);
            for (unsigned short var = 0; var < nPrim; ++var) {
              const bool velocity = var >= indices.Velocity() && var < indices.Velocity() + nDim;
              nodes->GetPrimitive(point)[var] = scale[var] * (velocity ? 200.0 * (x - 0.5) : 2.0 + x * x);
              for (unsigned short dim = 0; dim < nDim; ++dim)
                gradient(point, var, dim) = dim == 0 ? scale[var] * (velocity ? 800.0 : 4.0 + 2.0 * x) : 0.0;
            }
          }
          SU2_OMP_PARALLEL { flow->SetPrimitive_Limiter(&geometry, config.get()); }
          for (auto point = 0ul; point < geometry.GetnPointDomain(); ++point)
            for (unsigned short var = 0; var < nPrim; ++var) {
              const auto value = nodes->GetLimiter_Primitive(point, var);
              CHECK(std::isfinite(value));
              CHECK(value >= 0.0);
              CHECK(value <= 1.0);
              if (units == "DIMENSIONAL") reference(point, var) = value;
              else CHECK(value == Approx(reference(point, var)).margin(1e-12));
            }
        }
      }
    }
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

TEST_CASE("Bounded scalar residual and Jacobian preserve a constant field with varying density",
          "[Limiters][BoundedTransport]") {
  for (const std::string model : {"SA", "SST"}) {
    CAPTURE(model);
    auto config = transfer_test::MakeConfig(2, "SOLVER= RANS\nREYNOLDS_NUMBER= 1e6\nKIND_TURB_MODEL= " + model +
        "\nCONV_NUM_METHOD_TURB= BOUNDED_SCALAR\nMUSCL_TURB= NO\nTIME_DISCRE_TURB= EULER_IMPLICIT\n");
    transfer_test::MeshSolution domain(config.get(), transfer_test::BoxMesh(2, 2, false), 0);
    auto& geometry = domain.Fine();
    auto* flow = domain.solver[0][FLOW_SOL];
    auto* scalar = domain.solver[0][TURB_SOL];
    const auto nVar = scalar->GetnVar();
    const CPrimitiveIndices<unsigned short> indices(false, false, 2, config->GetnSpecies());
    auto* mass = const_cast<su2activevector*>(flow->GetEdgeMassFluxes());
    REQUIRE(mass != nullptr);
    for (auto edge = 0ul; edge < geometry.GetnEdge(); ++edge) (*mass)[edge] = edge % 2 ? -3.0 : 2.0;
    CSysVector<su2mixedfloat> constant(geometry.GetnPoint(), geometry.GetnPointDomain(), nVar, 0.0), product(constant);
    for (auto point = 0ul; point < geometry.GetnPoint(); ++point) {
      const auto density = 2.0 + geometry.nodes->GetCoord(point, 0);
      flow->GetNodes()->SetSolution(point, 0, density);
      flow->GetNodes()->GetPrimitive(point)[indices.Density()] = density;
      for (unsigned short var = 0; var < nVar; ++var) {
        const su2double value = 0.7 + 0.3 * var;
        scalar->GetNodes()->SetSolution(point, var, value);
        constant(point, var) = (model == "SST" ? density : 1.0) * value;
        for (unsigned short dim = 0; dim < 2; ++dim) scalar->GetNodes()->GetGradient(point)[var][dim] = 0.0;
      }
    }
    config->SetGlobalParam(config->GetKind_Solver(), RUNTIME_TURB_SYS);
    scalar->LinSysRes.SetValZero();
    scalar->Jacobian.SetValZero();
    SU2_OMP_PARALLEL {
      scalar->Upwind_Residual(&geometry, domain.solver[0], nullptr, config.get(), 0);
      scalar->Jacobian.MatrixVectorProduct(constant, product, &geometry, config.get());
    }
    for (auto point = 0ul; point < geometry.GetnPointDomain(); ++point)
      for (unsigned short var = 0; var < nVar; ++var) {
        CHECK(fabs(scalar->LinSysRes(point, var)) < 1e-11);
        CHECK(fabs(product(point, var)) < 1e-11);
      }
  }
}

TEST_CASE("Positive SA updates recover from the floor without removing relative relaxation",
          "[Limiters][UnderRelaxation]") {
  auto config = transfer_test::MakeConfig(2, "SOLVER= RANS\nREYNOLDS_NUMBER= 1e6\nKIND_TURB_MODEL= SA\n");
  transfer_test::MeshSolution domain(config.get(), transfer_test::BoxMesh(2, 2, false), 0);
  auto* solver = static_cast<CTurbSolver*>(domain.solver[0][TURB_SOL]);
  auto* nodes = solver->GetNodes();
  for (const su2double update : {-1e-4, 0.0, 1e-4}) {
    for (auto point = 0ul; point < domain.Fine().GetnPointDomain(); ++point) {
      nodes->SetSolution(point, 0, EPS);
      nodes->SetSolution_Old(point, 0, EPS);
      solver->LinSysSol(point, 0) = update;
    }
    SU2_OMP_PARALLEL {
      solver->ComputeUnderRelaxationFactorHelper(domain.solver[0], config->GetMaxUpdateFractionSA());
    }
    for (auto point = 0ul; point < domain.Fine().GetnPointDomain(); ++point) {
      const auto relaxation = nodes->GetUnderRelaxation(point);
      if (update > 0) {
        CHECK(relaxation > 0.0);
        CHECK(relaxation < 1e-10);
        CHECK(relaxation * update == Approx(config->GetMaxUpdateFractionSA() * 2 * EPS).margin(1e-30));
        nodes->AddClippedSolution(point, 0, relaxation * update, EPS, 1.0);
        CHECK(nodes->GetSolution(point, 0) > EPS);
      } else {
        CHECK(relaxation == (update == 0.0 ? 1.0 : 0.0));
      }
    }
  }
}

TEST_CASE("SST bound-active updates do not freeze the other variable", "[Limiters][UnderRelaxation]") {
  auto config = transfer_test::MakeConfig(2, "SOLVER= RANS\nREYNOLDS_NUMBER= 1e6\nKIND_TURB_MODEL= SST\n");
  transfer_test::MeshSolution domain(config.get(), transfer_test::BoxMesh(2, 2, false), 0);
  auto* solver = static_cast<CTurbSolver*>(domain.solver[0][TURB_SOL]);
  auto* nodes = solver->GetNodes();
  auto* flow = domain.solver[0][FLOW_SOL];
  const CPrimitiveIndices<unsigned short> indices(false, false, 2, 0);
  const auto kMin = solver->GetLowerLimit(0), wMin = solver->GetLowerLimit(1);
  for (auto point = 0ul; point < domain.Fine().GetnPoint(); ++point) {
    flow->GetNodes()->SetSolution(point, 0, 2.0);
    flow->GetNodes()->GetPrimitive(point)[indices.Density()] = 2.0;
  }
  for (const auto scenario : {0, 1, 2, 3, 4}) {
    CAPTURE(scenario);
    const auto k = scenario == 1 ? 2.0 : (scenario == 3 ? 2 * kMin : kMin);
    const auto w = scenario == 1 || scenario == 2 ? wMin : 10.0;
    const auto dk = scenario == 4 ? 0.01 : -0.8;
    for (auto point = 0ul; point < domain.Fine().GetnPoint(); ++point) {
      nodes->SetSolution(point, 0, k);
      nodes->SetSolution(point, 1, w);
      nodes->SetSolution_Old(point, 0, k);
      nodes->SetSolution_Old(point, 1, w);
      solver->LinSysSol(point, 0) = dk;
      solver->LinSysSol(point, 1) = -4.0;
    }
    SU2_OMP_PARALLEL {
      solver->CompleteImplicitIteration(&domain.Fine(), domain.solver[0], config.get());
    }
    for (auto point = 0ul; point < domain.Fine().GetnPointDomain(); ++point) {
      const auto alpha = nodes->GetUnderRelaxation(point);
      if (scenario == 0) {
        CHECK(alpha == 1.0);
        CHECK(nodes->GetSolution(point, 0) == kMin);
        CHECK(nodes->GetSolution(point, 1) == Approx(8.0));
      } else if (scenario == 1) {
        CHECK(alpha == 1.0);
        CHECK(nodes->GetSolution(point, 0) == Approx(1.6));
        CHECK(nodes->GetSolution(point, 1) == wMin);
      } else if (scenario == 2) {
        CHECK(alpha == 0.0);
        CHECK(nodes->GetSolution(point, 0) == kMin);
        CHECK(nodes->GetSolution(point, 1) == wMin);
      } else {
        CHECK(alpha > 0.0);
        CHECK(alpha < 1e-6);
        CHECK(nodes->GetSolution(point, 1) == Approx(10.0).epsilon(1e-6));
        CHECK(nodes->GetSolution(point, 0) >= kMin);
        if (scenario == 4) CHECK(nodes->GetSolution(point, 0) > kMin);
      }
    }
  }
}

TEST_CASE("Native SA-negative diffusion is consistent with the published nonlinear operator", "[Turbulence][SANegDiffusion]") {
  auto config = transfer_test::MakeConfig(2, "SOLVER= RANS\nREYNOLDS_NUMBER= 1e6\nKIND_TURB_MODEL= SA\n"
                                           "SA_OPTIONS= (NEGATIVE, WITHFT2)\n");
  const su2double velocity[2] = {0.0, 0.0};
  CEulerVariable flow(1.0, velocity, 1.0, 3, 2, 4, config.get());
  CTurbSAVariable scalar(0.0, 0.0, 3, 2, 1, config.get());
  const CEulerVariable::CIndices<unsigned short> indices(2, 0);
  for (auto point = 0ul; point < 3; ++point) {
    flow.GetPrimitive(point)[indices.Density()] = 1.0;
    flow.GetPrimitive(point)[indices.LaminarViscosity()] = 1.0;
  }
  const EdgeSide<CTurbSAVariable> side{scalar, &flow, {}, {}};
  CScalarFlux_SA<su2double, decltype(indices), 2, 1> flux(*config);
  const CPair<su2double> rho{1.0, 1.0};
  for (const su2double phi : {-2.0, 2.0}) {
    CAPTURE(phi);
    su2double lastError = 1e30;
    for (const su2double h : {0.01, 0.001, 0.0001}) {
      scalar.SetSolution(0, 0, phi);
      scalar.SetSolution(1, 0, phi + h);
      scalar.SetSolution(2, 0, phi - h);
      const auto right = flux.coefficients(indices, 0ul, side, 1ul, side, rho);
      const auto left = flux.coefficients(indices, 0ul, side, 2ul, side, rho);
      const auto diffusion = (right.i(0) - left.i(0)) / h;
      // For nu=1, nu_tilde=phi+x: [fn+chi*fn'+cb2]/sigma from ICCFD7-1902 Eq.14.
      const auto exact = phi < 0 ? -0.567 : 2.433;
      const auto error = fabs(diffusion - exact);
      if (phi < 0) CHECK(error < lastError);
      else CHECK(error < 2e-8);
      lastError = error;
      if (h == 0.0001) CHECK(diffusion == Approx(exact).margin(2e-8));
    }
  }
  // The two row coefficients may differ, but their edge energy contribution must dissipate.
  for (const su2double left : {-20.0, -2.0, -0.1, 0.1, 2.0, 20.0})
    for (const su2double right : {-20.0, -2.0, -0.1, 0.1, 2.0, 20.0}) {
      if (left == right) continue;
      scalar.SetSolution(0, 0, left);
      scalar.SetSolution(1, 0, right);
      const auto coefficient = flux.coefficients(indices, 0ul, side, 1ul, side, rho);
      CHECK((left * coefficient.i(0) - right * coefficient.j(0)) / (left - right) > 0.0);
    }
}
