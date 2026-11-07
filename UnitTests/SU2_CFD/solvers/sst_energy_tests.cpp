/*!
 * \file sst_energy_tests.cpp
 * \brief SST thermal-energy relaxation and matrix dissipation regressions.
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
#include "../../UnitQuadTestCase.hpp"

TEST_CASE("SST relaxation limits thermal energy when k dominates", "[SST][UnderRelaxation]") {
  UnitQuadTestCase test;
  test.config_options =
      "SOLVER= RANS\nMACH_NUMBER= 0.5\nREYNOLDS_NUMBER= 1e6\nKIND_TURB_MODEL= SST\nMESH_FORMAT= RECTANGLE\nMESH_BOX_SIZE= 5,5,5\n"
      "MARKER_HEATFLUX= (x_minus,0,x_plus,0,y_minus,0,y_plus,0)\n"
      "TIME_DISCRE_FLOW= EULER_IMPLICIT\nMAX_UPDATE_FLOW= 0.2\n";
  std::stringstream options(test.config_options);
  test.config = std::make_unique<CConfig>(options, SU2_COMPONENT::SU2_CFD, false);
  test.InitGeometry();
  test.solver = CSolverFactory::CreateSolverContainer(test.config->GetKind_Solver(), test.config.get(),
                                                      test.geometry.get(), 0);
  auto* flow = static_cast<CEulerSolver*>(test.solver[FLOW_SOL]);
  auto* nodes = flow->GetNodes();
  auto* turb = test.solver[TURB_SOL]->GetNodes();
  for (auto p = 0ul; p < test.geometry->GetnPoint(); ++p) {
    nodes->SetSolution(p, 0, 2.0);
    nodes->SetSolution(p, 1, 0.0);
    nodes->SetSolution(p, 2, 0.0);
    nodes->SetSolution(p, 3, 201.0);
    turb->SetSolution(p, 0, 100.0);
    for (auto v = 0u; v < 4; ++v) flow->LinSysSol(p, v) = 0.0;
    flow->LinSysSol(p, 3) = -10.0;
  }
  flow->ComputeUnderRelaxationFactor(test.config.get(), test.solver);
  CHECK(nodes->GetUnderRelaxation(0) == Approx(0.02));
  CHECK(201.0 - 10.0 * nodes->GetUnderRelaxation(0) - 200.0 > 0.0);
  delete test.solver[TURB_SOL];
}

#include "../../../SU2_CFD/include/variables/CNSVariable.hpp"
#include "../../../SU2_CFD/include/numerics_simd/flow/diffusion/viscous_fluxes.hpp"
#include "../../../SU2_CFD/include/numerics_simd/flow/convection/centered.hpp"

TEST_CASE("JST matrix fourth-order k dissipation does not create pressure waves", "[SST][JST]") {
  UnitQuadTestCase test;
  test.config_options =
      "SOLVER= RANS\nMACH_NUMBER= 0.5\nREYNOLDS_NUMBER= 1e6\nKIND_TURB_MODEL= SST\nMESH_FORMAT= RECTANGLE\nMESH_BOX_SIZE= 5,5,5\n"
      "MARKER_HEATFLUX= (x_minus,0,x_plus,0,y_minus,0,y_plus,0)\n"
      "CONV_NUM_METHOD_FLOW= JST_MAT\nENTROPY_FIX_COEFF= 0\n";
  std::stringstream options(test.config_options);
  test.config = std::make_unique<CConfig>(options, SU2_COMPONENT::SU2_CFD, false);
  test.InitGeometry();
  test.solver = CSolverFactory::CreateSolverContainer(test.config->GetKind_Solver(), test.config.get(),
                                                      test.geometry.get(), 0);
  auto* flow = static_cast<CEulerSolver*>(test.solver[FLOW_SOL]);
  auto& nodes = *static_cast<CEulerVariable*>(flow->GetNodes());
  const CVariable* turb = test.solver[TURB_SOL]->GetNodes();
  for (auto p = 0ul; p < test.geometry->GetnPoint(); ++p) {
    nodes.SetSensor(p, 0.0);
    for (auto v = 0u; v < 5; ++v) nodes.SetUnd_Lapl(p, v, 0.0);
  }
  nodes.SetUnd_Lapl(0, 3, 2.0);
  nodes.SetUnd_Lapl(0, 4, 2.0);  // the energy Laplacian comes entirely from rho*k
  int iMesh = MESH_0;  // the centered schemes of this branch take the grid level explicitly
  CJSTmatScheme<CNoViscousFlux<2>> scheme(*test.config, iMesh, turb);
  CPair<CCompressiblePrimitives<2, 7>> V;
  V.i.all = Double(0.0);
  V.i.density() = 1.0;
  V.i.pressure() = 1.0;
  V.i.speedSound() = sqrt(1.4);
  V.i.enthalpy() = 3.5;
  V.j = V.i;
  VectorDbl<4> flux, diff;
  flux = Double(0.0);
  diff = Double(0.0);
  VectorDbl<2> normal;
  normal(0) = 1.0;
  normal(1) = 0.0;
  MatrixDbl<4> ji, jj;
  scheme.finalizeFlux(flux, ji, jj, false, Double(1.0), Double(0.0), V.i, V, diff, Int(0ul), Int(1ul), *test.geometry,
                      nodes, normal);
  for (size_t lane = 0; lane < Double::Size; ++lane)
    for (size_t v = 0; v < 4; ++v) CHECK(flux(v)[lane] == Approx(0.0).margin(1e-12));
  delete test.solver[TURB_SOL];
}

/*--- SST_ENERGY_CONSISTENT (P1_SST_COUPLED.md, section 4): the energy flux gets the k diffusion of the SST k row, and
 * the k production is the work of the turbulent stress. ---*/

#include "../../../SU2_CFD/include/numerics/turbulent/turb_sst_edge_flux.hpp"
#include "../../../SU2_CFD/include/numerics/turbulent/sst_energy.hpp"
#include "../../../SU2_CFD/include/numerics/turbulent/turb_sources.hpp"
#include "../../../SU2_CFD/include/numerics/flow/flow_diffusion.hpp"
#include "../../../SU2_CFD/include/variables/CTurbSSTVariable.hpp"
#include "../../../SU2_CFD/include/variables/CNSVariable.hpp"

namespace {
std::unique_ptr<CConfig> SSTEnergyConfig(bool on) {
  std::stringstream options;
  options << "SOLVER= RANS\nKIND_TURB_MODEL= SST\nMACH_NUMBER= 0.3\nREYNOLDS_NUMBER= 1e6\n"
          << "SST_ENERGY_CONSISTENT= " << (on ? "YES" : "NO") << "\n";
  return std::make_unique<CConfig>(options, SU2_COMPONENT::SU2_CFD, false);
}
const su2double sstConstants[11] = {0.85, 1.0, 0.5, 0.856, 0.075, 0.0828, 0.09, 0.31, 0.5532, 0.4403, 20.0};
}  // namespace

TEST_CASE("SST_ENERGY_CONSISTENT: the energy k diffusion equals the diffusive flux of the SST k row", "[SSTEnergy]") {
  constexpr unsigned short nDim = 2;
  auto config = SSTEnergyConfig(true);
  const su2double vel0[2] = {0.0, 0.0};
  CNSVariable flow(1.0, vel0, 2.5, 2, nDim, nDim + 2, config.get());
  CTurbSSTVariable turb(0.0, 1.0, 0.0, 2, nDim, 2, sstConstants, config.get());
  const CEulerVariable::CIndices<unsigned short> idx(nDim, 0);

  /*--- Two points with different states, a skew edge and a normal not along it. ---*/
  const su2double rho[2] = {1.1, 0.9}, mu[2] = {1.8e-3, 2.1e-3}, mut[2] = {0.03, 0.07}, F1[2] = {0.8, 0.3};
  const su2double k[2] = {0.02, 0.05}, grad[2][2] = {{0.3, -0.2}, {0.1, 0.4}};
  su2activematrix coord(2, nDim);
  coord(0, 0) = 0.1; coord(0, 1) = 0.2; coord(1, 0) = 0.45; coord(1, 1) = 0.33;
  for (unsigned long p = 0; p < 2; ++p) {
    flow.SetPrimitive(p, idx.Density(), rho[p]);
    flow.SetPrimitive(p, idx.LaminarViscosity(), mu[p]);
    flow.SetPrimitive(p, idx.EddyViscosity(), mut[p]);
    turb.SetSolution(p, 0, k[p]);
    turb.SetF1blending(p, F1[p]);
    for (unsigned short iDim = 0; iDim < nDim; ++iDim) turb.GetGradient(p)[0][iDim] = grad[p][iDim];
  }

  /*--- The SST kernel, diffusion only (BoundaryDiffusive: no convection, row of i, corrected gradient). ---*/
  using Model = CScalarFlux_SST<su2double, CEulerVariable::CIndices<unsigned short>, 2, 2>;
  const Model model(*config);
  const EdgeSide<CTurbSSTVariable> side{turb, &flow, CMatrixView<const su2double>(coord), CMatrixView<const su2double>()};
  Vector<su2double, 2> normal;
  normal(0) = 0.7; normal(1) = 0.25;
  const auto opt = ScalarFluxOptions::BoundaryDiffusive(*config, true);
  const auto res = model.ComputeFlux(opt, 0ul, side, 1ul, side, normal, su2double(0.0));

  /*--- The shared functions used by the flow fluxes. ---*/
  const su2double e[2] = {coord(1, 0) - coord(0, 0), coord(1, 1) - coord(0, 1)}, n[2] = {0.7, 0.25};
  const su2double D = SSTEnergy::EdgeDiffusivity(mu[0], mut[0], F1[0], mu[1], mut[1], F1[1]);
  const su2double g = SSTEnergy::CorrectedProjection(int(nDim), grad[0], grad[1], k[0], k[1], e, n);
  CHECK(res.flux_i(0) == Approx(-D * g).epsilon(1e-14));

  /*--- Consistency: for a linear k with its exact gradient, g is grad k . n. ---*/
  const su2double gl[2] = {0.3, -0.2};
  const su2double kl1 = k[0] + gl[0] * e[0] + gl[1] * e[1];
  CHECK(SSTEnergy::CorrectedProjection(int(nDim), gl, gl, k[0], kl1, e, n) ==
        Approx(gl[0] * n[0] + gl[1] * n[1]).epsilon(1e-14));

  /*--- Non-SIMD viscous flux: the energy component changes by +D g with the edge flag (the physical flux is
   * subtracted from the residual of i), the other components do not change. ---*/
  {
    CAvgGrad_Flow visc(nDim, nDim + 2, VISCOUS_GRAD_CORR::NONE, config.get());
    su2double V[2][nDim + 9];
    for (unsigned long p = 0; p < 2; ++p) {
      for (auto& v : V[p]) v = 0.0;
      V[p][0] = 300.0; V[p][1] = 10.0 * (p + 1); V[p][2] = -3.0;
      V[p][nDim + 1] = 1e5; V[p][nDim + 2] = rho[p]; V[p][nDim + 3] = 3e5; V[p][nDim + 4] = 340.0;
      V[p][nDim + 5] = mu[p]; V[p][nDim + 6] = mut[p]; V[p][nDim + 7] = 0.025; V[p][nDim + 8] = 1005.0;
    }
    su2activematrix gradPrim(nDim + 1, nDim);
    gradPrim = su2double(0.0);
    gradPrim(1, 1) = 0.5;  // some shear so that the stress is not zero
    su2activematrix gradK_i(2, nDim), gradK_j(2, nDim);
    gradK_i = su2double(0.0); gradK_j = su2double(0.0);
    for (unsigned short iDim = 0; iDim < nDim; ++iDim) { gradK_i(0, iDim) = grad[0][iDim]; gradK_j(0, iDim) = grad[1][iDim]; }
    visc.SetPrimitive(V[0], V[1]);
    visc.SetPrimVarGradient(CMatrixView<const su2double>(gradPrim), CMatrixView<const su2double>(gradPrim));
    visc.SetCoord(coord[0], coord[1]);
    visc.SetNormal(n);
    visc.SetTurbKineticEnergy(k[0], k[1]);
    visc.SetTau_Wall(-1.0, -1.0);
    visc.SetScalarVarGradient(CMatrixView<const su2double>(gradK_i), CMatrixView<const su2double>(gradK_j));
    visc.SetF1blending(F1[0], F1[1]);
    su2double off[nDim + 2], on[nDim + 2];
    visc.SetSSTEnergyKineDiffusion(false);
    auto r0 = visc.ComputeResidual(config.get());
    for (unsigned short iVar = 0; iVar < nDim + 2; ++iVar) off[iVar] = r0[iVar];
    visc.SetSSTEnergyKineDiffusion(true);
    auto r1 = visc.ComputeResidual(config.get());
    for (unsigned short iVar = 0; iVar < nDim + 2; ++iVar) on[iVar] = r1[iVar];
    for (unsigned short iVar = 0; iVar < nDim + 1; ++iVar) CHECK(on[iVar] == off[iVar]);
    CHECK(on[nDim + 1] - off[nDim + 1] == Approx(D * g).epsilon(1e-10));
  }
}

TEST_CASE("SST_ENERGY_CONSISTENT: production is mu_t (2 S:S - 2/3 (div u)^2) before the limiter", "[SSTEnergy]") {
  constexpr unsigned short nDim = 2;
  const CEulerVariable::CIndices<unsigned short> idx(nDim, 0);
  su2double V[nDim + 9] = {0.0};
  V[idx.Density()] = 1.2; V[idx.LaminarViscosity()] = 1.8e-5; V[idx.EddyViscosity()] = 2e-3;
  V[idx.SoundSpeed()] = 340.0; V[idx.Velocity() + 1] = 0.0;
  /*--- Velocity gradient with dilatation: du/dx = 2, dv/dy = 1, du/dy = 0.5, dv/dx = -0.3. ---*/
  su2activematrix grad(nDim + 1, nDim);
  grad = su2double(0.0);
  grad(1, 0) = 2.0; grad(1, 1) = 0.5; grad(2, 0) = -0.3; grad(2, 1) = 1.0;
  const su2double S2 = 2.0 * (2.0 * 2.0 + 1.0 * 1.0) + pow(0.5 - 0.3, 2);  // 2 S:S
  const su2double div = 3.0, vort[3] = {0.0, 0.0, -0.8};
  /*--- k and omega with the limiter far away: c_l beta* rho omega k = 20*0.09*1.2*500*2 >> mu_t S^2. ---*/
  const su2double sol[2] = {2.0, 500.0};
  su2activematrix gradT(2, nDim);
  gradT = su2double(0.0);
  su2double res[2];
  for (bool on : {false, true}) {
    auto config = SSTEnergyConfig(on);
    CSourcePieceWise_TurbSST<CEulerVariable::CIndices<unsigned short>> src(nDim, 2, sstConstants, 1e-6, 1.0,
                                                                           config.get());
    src.SetPrimitive(V, nullptr);
    src.SetPrimVarGradient(CMatrixView<const su2double>(grad), CMatrixView<const su2double>());
    src.SetScalarVar(sol, nullptr);
    src.SetScalarVarGradient(CMatrixView<const su2double>(gradT), CMatrixView<const su2double>());
    src.SetVolume(1.0);
    src.SetDistance(1.0, 0.0);
    src.SetF1blending(1.0, 0.0);
    src.SetF2blending(0.0);
    src.SetVorticity(vort, nullptr);
    src.SetStrainMag(sqrt(S2), 0.0);
    src.SetCrossDiff(0.0);
    res[on] = src.ComputeResidual(config.get())[0];
  }
  /*--- The destruction is the same; the production differs by -2/3 mu_t div^2. ---*/
  CHECK(res[1] - res[0] == Approx(-TWO3 * V[idx.EddyViscosity()] * div * div).margin(1e-11));

  /*--- Pure shear (div u = 0): the same production with and without the option. ---*/
  grad(1, 0) = 0.0; grad(2, 1) = 0.0;
  const su2double S2shear = pow(0.5 - 0.3, 2);
  for (bool on : {false, true}) {
    auto config = SSTEnergyConfig(on);
    CSourcePieceWise_TurbSST<CEulerVariable::CIndices<unsigned short>> src(nDim, 2, sstConstants, 1e-6, 1.0,
                                                                           config.get());
    src.SetPrimitive(V, nullptr);
    src.SetPrimVarGradient(CMatrixView<const su2double>(grad), CMatrixView<const su2double>());
    src.SetScalarVar(sol, nullptr);
    src.SetScalarVarGradient(CMatrixView<const su2double>(gradT), CMatrixView<const su2double>());
    src.SetVolume(1.0);
    src.SetDistance(1.0, 0.0);
    src.SetF1blending(1.0, 0.0);
    src.SetF2blending(0.0);
    src.SetVorticity(vort, nullptr);
    src.SetStrainMag(sqrt(S2shear), 0.0);
    src.SetCrossDiff(0.0);
    res[on] = src.ComputeResidual(config.get())[0];
  }
  CHECK(res[1] == res[0]);
}

TEST_CASE("SST_ENERGY_CONSISTENT: the vectorized JST viscous flux adds -D g of the k row to the energy row", "[SSTEnergy]") {
  auto options = [](bool on) {
    return std::string("SOLVER= RANS\nKIND_TURB_MODEL= SST\nMACH_NUMBER= 0.3\nREYNOLDS_NUMBER= 1e6\n"
                       "MESH_FORMAT= RECTANGLE\nMESH_BOX_SIZE= 5,5,5\n"
                       "MARKER_HEATFLUX= (x_minus,0,x_plus,0,y_minus,0,y_plus,0)\n"
                       "CONV_NUM_METHOD_FLOW= JST\nTIME_DISCRE_FLOW= EULER_EXPLICIT\nTIME_DISCRE_TURB= EULER_EXPLICIT\n"
                       "SST_ENERGY_CONSISTENT= ") + (on ? "YES\n" : "NO\n");
  };
  UnitQuadTestCase test;
  test.config_options = options(true);
  std::stringstream optOn(test.config_options), optOff(options(false));
  test.config = std::make_unique<CConfig>(optOn, SU2_COMPONENT::SU2_CFD, false);
  CConfig configOff(optOff, SU2_COMPONENT::SU2_CFD, false);
  test.InitGeometry();
  test.solver = CSolverFactory::CreateSolverContainer(test.config->GetKind_Solver(), test.config.get(),
                                                      test.geometry.get(), 0);
  auto& nodes = *static_cast<CNSVariable*>(test.solver[FLOW_SOL]->GetNodes());
  auto* turb = static_cast<CTurbSSTVariable*>(test.solver[TURB_SOL]->GetNodes());
  const auto nPoint = test.geometry->GetnPoint();
  for (auto p = 0ul; p < nPoint; ++p) {
    nodes.SetSensor(p, 0.0);
    nodes.SetLambda(p, 1.0);
    for (auto v = 0u; v < 4; ++v) nodes.SetUnd_Lapl(p, v, 0.0);
    nodes.SetLaminarViscosity(p, 1e-3 * (1.0 + 0.1 * p));
    nodes.SetEddyViscosity(p, 0.02 + 0.01 * p);
    turb->SetSolution(p, 0, 0.01 + 0.002 * p);
    turb->SetF1blending(p, 0.2 + 0.05 * (p % 5));
    for (unsigned short iDim = 0; iDim < 2; ++iDim) turb->GetGradient(p)[0][iDim] = 0.1 * (iDim + 1) + 0.01 * p;
  }
  int iMesh = MESH_0;
  const CVariable* turbVars = turb;
  const CJSTScheme<CCompressibleViscousFlux<2>> schemeOn(*test.config, iMesh, turbVars);
  const CJSTScheme<CCompressibleViscousFlux<2>> schemeOff(configOff, iMesh, turbVars);
  CSysVector<su2double> resOn(nPoint, test.geometry->GetnPointDomain(), 4), resOff(nPoint, test.geometry->GetnPointDomain(), 4);
  resOn.SetValZero();
  resOff.SetValZero();
  SparseMatrixType matrix;
  Double mask = 0.0;
  mask[0] = 1.0;  // lane 0 only: every lane holds the same edge
  const unsigned long iEdge = 3;
  schemeOn.ComputeFlux(Int(iEdge), *test.config, *test.geometry, nodes, UpdateType::COLORING, mask, resOn, matrix, nullptr);
  schemeOff.ComputeFlux(Int(iEdge), configOff, *test.geometry, nodes, UpdateType::COLORING, mask, resOff, matrix, nullptr);

  const auto iPoint = test.geometry->edges->GetNode(iEdge, 0), jPoint = test.geometry->edges->GetNode(iEdge, 1);
  const su2double* xi = test.geometry->nodes->GetCoord(iPoint);
  const su2double* xj = test.geometry->nodes->GetCoord(jPoint);
  const su2double e[2] = {xj[0] - xi[0], xj[1] - xi[1]};
  const su2double* n = test.geometry->edges->GetNormal(iEdge);
  const su2double gi[2] = {turb->GetGradient(iPoint)[0][0], turb->GetGradient(iPoint)[0][1]};
  const su2double gj[2] = {turb->GetGradient(jPoint)[0][0], turb->GetGradient(jPoint)[0][1]};
  const su2double g = SSTEnergy::CorrectedProjection(2, gi, gj, turb->GetSolution(iPoint, 0), turb->GetSolution(jPoint, 0), e, n);
  const su2double D = SSTEnergy::EdgeDiffusivity(nodes.GetLaminarViscosity(iPoint), nodes.GetEddyViscosity(iPoint),
                                                 turb->GetF1blending(iPoint), nodes.GetLaminarViscosity(jPoint),
                                                 nodes.GetEddyViscosity(jPoint), turb->GetF1blending(jPoint));
  REQUIRE(std::abs(D * g) > 1e-8);
  for (unsigned short iVar = 0; iVar < 3; ++iVar) {
    CHECK(resOn(iPoint, iVar) == resOff(iPoint, iVar));
    CHECK(resOn(jPoint, iVar) == resOff(jPoint, iVar));
  }
  CHECK(resOn(iPoint, 3) - resOff(iPoint, 3) == Approx(-D * g).epsilon(1e-9));
  CHECK(resOn(jPoint, 3) - resOff(jPoint, 3) == Approx(D * g).epsilon(1e-9));
  delete test.solver[TURB_SOL];
}
