/*!
 * \file CNewtonCoupling.cpp
 * \brief Assembled flow/turbulence cross Jacobians of a coupled assembled Newton group.
 * \author SU2 Contributors
 */

#include "../../include/integration/CNewtonCoupling.hpp"
#include "../../include/variables/CFlowVariable.hpp"

CNewtonCoupling::CNewtonCoupling(CGeometry* g, CConfig* c, CSolver** s, CNumerics** n, unsigned short flow,
                                 unsigned short turb, unsigned short turbVariables)
    : geometry(g),
      config(c),
      solvers(s),
      turbNumerics(n),
      nDim(g->GetnDim()),
      nFlow(s[FLOW_SOL]->GetnVar()),
      nTurb(turbVariables),
      flowOffset(flow),
      turbOffset(turb),
      sst(c->GetKind_Turb_Model() == TURB_MODEL::SST),
      negativeSA(c->GetSAParsedOptions().version == SA_OPTIONS::NEG) {
  if (nFlow > MaxVariables || nTurb > MaxVariables)
    SU2_MPI::Error("Assembled Newton coupling supports at most nDim+2 flow and turbulence variables", CURRENT_FUNCTION);
  const auto nPoint = geometry->GetnPoint(), nEdge = geometry->GetnEdge();
  tfDiagonal.assign(nPoint * nTurb * nFlow, 0.0);
  ftDiagonal.assign(nPoint * nFlow * nTurb, 0.0);
  tfEdge.assign(2 * nEdge * nTurb * nFlow, 0.0);
  ftEdge.assign(2 * nEdge * nFlow * nTurb, 0.0);
  eddyDerivative.assign(nPoint * nTurb, 0.0);
  vorticityDerivative.assign(nPoint * nTurb, 0.0);
  strainDerivative.assign(nPoint * nTurb, 0.0);
  densityDerivative.assign(nPoint * nTurb, 0.0);
  savedResidual.assign(nPoint * nTurb, 0.0);
  savedResidualBase.assign(nPoint * nTurb, 0.0);
  savedValue.assign(nPoint * 3, 0.0);
}

void CNewtonCoupling::SourceSensitivities() {
  auto* flow = su2staticcast_p<CFlowVariable*>(solvers[FLOW_SOL]->GetNodes());
  auto* turbSolver = solvers[TURB_SOL];
  auto& strain = flow->GetStrainMag();
  const auto nPoint = geometry->GetnPoint();
  const unsigned short density = nDim + 2; /*--- Compressible primitive layout (T, u, p, rho, ...). ---*/
  const double relative = 1e-6;
  SU2_OMP_FOR_STAT(1024)
  for (auto p = 0ul; p < nPoint; ++p)
    for (auto v = 0u; v < nTurb; ++v) savedResidual[p * nTurb + v] = turbSolver->LinSysRes(p, v);
  END_SU2_OMP_FOR
  /*--- Explicit source only: no Jacobian contributions. The evaluation's settings guard restores the config. ---*/
  SU2_OMP_SAFE_GLOBAL_ACCESS(config->SetResidualOnly(true);
                             config->SetGlobalParam(config->GetKind_Solver(), RUNTIME_TURB_SYS);)
  auto source = [&](std::vector<passivedouble>& result) {
    turbSolver->LinSysRes.SetValZero();
    SU2_OMP_BARRIER
    turbSolver->Source_Residual(geometry, solvers, turbNumerics, config, MESH_0);
    SU2_OMP_BARRIER
    SU2_OMP_FOR_STAT(1024)
    for (auto p = 0ul; p < geometry->GetnPointDomain(); ++p)
      for (auto v = 0u; v < nTurb; ++v) result[p * nTurb + v] = SU2_TYPE::GetValue(turbSolver->LinSysRes(p, v));
    END_SU2_OMP_FOR
  };
  /*--- Perturbed passes first: the last (base) pass leaves the stored source outputs (e.g. intermittency) intact. ---*/
  /*--- value(p, d) is the d-th perturbed quantity of point p; originals are restored exactly. ---*/
  auto perturbed = [&](std::vector<passivedouble>& result, unsigned short count, const auto& value) {
    SU2_OMP_FOR_STAT(1024)
    for (auto p = 0ul; p < nPoint; ++p)
      for (auto d = 0u; d < count; ++d) {
        savedValue[p * 3 + d] = value(p, d);
        value(p, d) *= 1 + relative;
      }
    END_SU2_OMP_FOR
    source(result);
    SU2_OMP_FOR_STAT(1024)
    for (auto p = 0ul; p < nPoint; ++p)
      for (auto d = 0u; d < count; ++d) value(p, d) = savedValue[p * 3 + d];
    END_SU2_OMP_FOR
  };
  perturbed(vorticityDerivative, 3, [&](unsigned long p, unsigned short d) -> su2double& {
    return flow->GetVorticity(p)[d];
  });
  perturbed(strainDerivative, 1, [&](unsigned long p, unsigned short) -> su2double& { return strain(p); });
  if (!sst)
    perturbed(densityDerivative, 1, [&](unsigned long p, unsigned short) -> su2double& {
      return flow->GetPrimitive(p)[density];
    });
  std::vector<passivedouble>& base = savedResidualBase;
  source(base);
  SU2_OMP_FOR_STAT(1024)
  for (auto p = 0ul; p < geometry->GetnPointDomain(); ++p) {
    const double omega = SU2_TYPE::GetValue(GeometryToolbox::Norm(3, flow->GetVorticity(p)));
    const double s = SU2_TYPE::GetValue(strain(p)), rho = SU2_TYPE::GetValue(flow->GetDensity(p));
    for (auto v = 0u; v < nTurb; ++v) {
      const auto n = p * nTurb + v;
      vorticityDerivative[n] = omega > 1e-18 ? (vorticityDerivative[n] - base[n]) / (relative * omega) : 0;
      strainDerivative[n] = s > 1e-18 ? (strainDerivative[n] - base[n]) / (relative * s) : 0;
      densityDerivative[n] = sst ? 0 : (densityDerivative[n] - base[n]) / (relative * rho);
    }
  }
  END_SU2_OMP_FOR
  SU2_OMP_FOR_STAT(1024)
  for (auto p = 0ul; p < nPoint; ++p)
    for (auto v = 0u; v < nTurb; ++v) turbSolver->LinSysRes(p, v) = savedResidual[p * nTurb + v];
  END_SU2_OMP_FOR
}

void CNewtonCoupling::EddyViscosityDerivatives() {
  const auto* flow = solvers[FLOW_SOL]->GetNodes();
  const auto* turb = solvers[TURB_SOL]->GetNodes();
  const bool v1994 = config->GetSSTParsedOptions().version == SST_OPTIONS::V1994;
  SU2_OMP_FOR_STAT(1024)
  for (auto p = 0ul; p < geometry->GetnPoint(); ++p) {
    auto* d = &eddyDerivative[p * nTurb];
    for (auto v = 0u; v < nTurb; ++v) d[v] = 0;
    const double rho = SU2_TYPE::GetValue(flow->GetDensity(p));
    const double mu = SU2_TYPE::GetValue(flow->GetLaminarViscosity(p));
    if (!sst) {
      /*--- SA (CTurbSASolver::Postprocessing): mu_t = rho fv1(chi) nu_hat. ---*/
      const double cv1_3 = 7.1 * 7.1 * 7.1, cR1 = 0.5, roughConst = 0.03;
      const double nuHat = SU2_TYPE::GetValue(turb->GetSolution(p, 0));
      if (negativeSA && nuHat < 0) continue;
      const double nu = mu / rho;
      const double roughness = SU2_TYPE::GetValue(geometry->nodes->GetRoughnessHeight(p));
      double chi = nuHat / nu;
      if (roughness > 1.0e-10)
        chi += cR1 * roughness /
               (SU2_TYPE::GetValue(geometry->nodes->GetWall_Distance(p)) + roughConst * roughness + EPS);
      const double chi3 = chi * chi * chi, fv1 = chi3 / (chi3 + cv1_3);
      const double dfv1 = 3 * chi * chi * cv1_3 / std::pow(chi3 + cv1_3, 2);
      d[0] = rho * fv1 + rho * nuHat * dfv1 / nu;
    } else {
      /*--- SST (CTurbSSTSolver::Postprocessing): mu_t = rho a1 k / max(a1 omega, S F2), unknowns (rho k, rho omega)
       *    at frozen density. The blending function F2 is frozen. ---*/
      const double a1 = 0.31;
      const double k = SU2_TYPE::GetValue(turb->GetSolution(p, 0)), omega = SU2_TYPE::GetValue(turb->GetSolution(p, 1));
      const double strain = v1994 ? std::max(SU2_TYPE::GetValue(GeometryToolbox::Norm(3, flow->GetVorticity(p))), 1e-12)
                                  : std::max(SU2_TYPE::GetValue(flow->GetStrainMag(p)), 1e-12);
      const double limiter = strain * SU2_TYPE::GetValue(turb->GetF2blending(p));
      const double denominator = std::max(a1 * omega, limiter);
      const double muT = rho * a1 * k / denominator;
      if (!(muT > 0)) continue;
      d[0] = a1 / denominator;
      if (a1 * omega >= limiter) d[1] = -muT / (rho * omega);
    }
  }
  END_SU2_OMP_FOR
}

void CNewtonCoupling::Assemble() {
  EddyViscosityDerivatives();
  SourceSensitivities();
  const auto* flow = solvers[FLOW_SOL]->GetNodes();
  const auto* turb = solvers[TURB_SOL]->GetNodes();
  const double prandtlTurb = SU2_TYPE::GetValue(config->GetPrandtl_Turb());
  /*--- QCR, stress perturbation and wall functions change the eddy-viscosity dependence: keep only convection. ---*/
  const bool viscousCoupling = !config->GetSAParsedOptions().qcr2000 && !config->GetSSTParsedOptions().uq &&
                               !config->GetWall_Functions();
  const auto nVar = nDim + 1;
  const auto& flowJacobian = solvers[FLOW_SOL]->Jacobian;
  const auto* constants = sst ? solvers[TURB_SOL]->GetConstants() : nullptr;
  const auto production = config->GetSSTParsedOptions().production;

  SU2_OMP_FOR_DYN(256)
  for (auto i = 0ul; i < geometry->GetnPointDomain(); ++i) {
    auto* tfI = TF(true, i);
    auto* ftI = FT(true, i);
    for (auto n = 0u; n < nTurb * nFlow; ++n) tfI[n] = ftI[n] = 0;
    if (sst) {
      /*--- Diagonal native rho E column, see the edge loop. ---*/
      const auto* native = flowJacobian.GetBlock(i, i);
      if (native)
        for (auto r = 0u; r < nFlow; ++r) ftI[r * nTurb] -= SU2_TYPE::GetValue(native[r * nFlow + nDim + 1]);
      /*--- At frozen specific k, omega and strain, every k source term (production through mu_t ~ rho, its limiter,
       *    dissipation) is linear in rho: dS_k/drho = S_k/rho, R_k -= S_k V (CSourcePieceWise_TurbSST).
       *    The omega row's rho column (alpha rho S^2 + beta rho omega^2) is orders of magnitude above its diagonal
       *    near walls and is left out, as the strain chain of the omega row. ---*/
      const double betaStar = SU2_TYPE::GetValue(constants[6]), limit = SU2_TYPE::GetValue(constants[10]);
      const double rho = SU2_TYPE::GetValue(flow->GetDensity(i));
      const double k = SU2_TYPE::GetValue(turb->GetSolution(i, 0)), omega = SU2_TYPE::GetValue(turb->GetSolution(i, 1));
      const double vorticity = SU2_TYPE::GetValue(GeometryToolbox::Norm(3, flow->GetVorticity(i)));
      const double strain = SU2_TYPE::GetValue(flow->GetStrainMag(i));
      const double base = production == SST_OPTIONS::V ? vorticity
                          : production == SST_OPTIONS::KL ? std::sqrt(strain * vorticity) : strain;
      const double muT = SU2_TYPE::GetValue(turb->GetmuT(i));
      const double pk = std::max(0.0, std::min(muT * base * base, limit * betaStar * rho * omega * k));
      const double dk = betaStar * rho * omega * k;
      const double volume = SU2_TYPE::GetValue(geometry->nodes->GetVolume(i));
      if (SU2_TYPE::GetValue(geometry->nodes->GetWall_Distance(i)) > 1e-10) tfI[0] -= (pk - dk) * volume / rho;
    }
    /*--- Source chain through |omega| and S: dR/du_k of a column point X with Green-Gauss weight vector G
     *    (grad u_i = sum_X G_X u_X): d|omega|/du_k = w.(G x e_k), w = omega/|omega|; dS/du_k = 2 S_kl G_l / S. ---*/
    const unsigned short chainRows = sst ? 1 : nTurb;
    double unitVorticity[3] = {}, strainTensor[MaxDim][MaxDim] = {}, strainMagnitude = 0;
    {
      const auto* vorticity = flow->GetVorticity(i);
      const double magnitude = SU2_TYPE::GetValue(GeometryToolbox::Norm(3, vorticity));
      for (auto d = 0u; d < 3; ++d)
        unitVorticity[d] = magnitude > 1e-18 ? SU2_TYPE::GetValue(vorticity[d]) / magnitude : 0;
      for (auto a = 0u; a < nDim; ++a)
        for (auto b = 0u; b < nDim; ++b)
          strainTensor[a][b] = 0.5 * SU2_TYPE::GetValue(flow->GetGradient_Primitive(i, 1 + a, b) +
                                                        flow->GetGradient_Primitive(i, 1 + b, a));
      strainMagnitude = SU2_TYPE::GetValue(flow->GetStrainMag(i));
    }
    const double volumeI = SU2_TYPE::GetValue(geometry->nodes->GetVolume(i));
    double weightSum[MaxDim] = {};
    auto sourceChain = [&](passivedouble* block, unsigned long point, const double* weight) {
      const double rhoX = SU2_TYPE::GetValue(flow->GetDensity(point));
      double gradient[3] = {};
      for (auto d = 0u; d < nDim; ++d) gradient[d] = weight[d];
      for (auto v = 0u; v < chainRows; ++v) {
        const double dOmega = vorticityDerivative[i * nTurb + v], dStrain = strainDerivative[i * nTurb + v];
        if (dOmega == 0 && dStrain == 0) continue;
        for (auto k = 0u; k < nDim; ++k) {
          double e[3] = {}, cross[3];
          e[k] = 1;
          cross[0] = gradient[1] * e[2] - gradient[2] * e[1];
          cross[1] = gradient[2] * e[0] - gradient[0] * e[2];
          cross[2] = gradient[0] * e[1] - gradient[1] * e[0];
          double dRdu = dOmega * (unitVorticity[0] * cross[0] + unitVorticity[1] * cross[1] +
                                  unitVorticity[2] * cross[2]);
          if (strainMagnitude > 1e-18) {
            double dS = 0;
            for (auto l = 0u; l < nDim; ++l) dS += 2 * strainTensor[k][l] * gradient[l] / strainMagnitude;
            dRdu += dStrain * dS;
          }
          /*--- du_k/drho = -u_k/rho, du_k/d(rho u_k) = 1/rho. ---*/
          block[v * nFlow] -= dRdu * SU2_TYPE::GetValue(flow->GetVelocity(point, k)) / rhoX;
          block[v * nFlow + 1 + k] += dRdu / rhoX;
        }
      }
    };

    for (auto neighbour = 0u; neighbour < geometry->nodes->GetnPoint(i); ++neighbour) {
      const auto e = geometry->nodes->GetEdge(i, neighbour);
      const auto a = geometry->edges->GetNode(e, 0), b = geometry->edges->GetNode(e, 1);
      const bool first = i == a;
      const auto j = first ? b : a;
      auto* tfJ = TF(false, 2 * e + (first ? 0 : 1));
      auto* ftJ = FT(false, 2 * e + (first ? 0 : 1));
      for (auto n = 0u; n < nTurb * nFlow; ++n) tfJ[n] = ftJ[n] = 0;
      /*--- Row i is node 0 (sign +1) or node 1 (sign -1) of the edge. ---*/
      const double sign = first ? 1 : -1;

      double normal[MaxDim] = {}, u[2][MaxDim] = {}, rho[2], qn[2] = {};
      for (auto d = 0u; d < nDim; ++d) normal[d] = SU2_TYPE::GetValue(geometry->edges->GetNormal(e)[d]);
      const unsigned long node[2] = {i, j};
      double q = 0;
      for (auto side = 0u; side < 2; ++side) {
        rho[side] = SU2_TYPE::GetValue(flow->GetDensity(node[side]));
        for (auto d = 0u; d < nDim; ++d) {
          u[side][d] = SU2_TYPE::GetValue(flow->GetVelocity(node[side], d));
          qn[side] += u[side][d] * normal[d];
        }
        q += 0.5 * qn[side];
      }
      /*--- Convection (CUpwScalarBase, first order): with the edge normal n from node 0 to node 1,
       *    F = q+ w_0 + q- w_1, q = (u_0 + u_1).n / 2, w = nu_hat (SA) or rho phi (SST) at frozen specific phi.
       *    R_0 += F, R_1 -= F, and dq/dU_X = 0.5 [-(u_X.n)/rho_X, n/rho_X, 0]. ---*/
      const unsigned short node0 = first ? 0 : 1;
      for (auto v = 0u; v < nTurb; ++v) {
        double phi[2], w[2];
        for (auto side = 0u; side < 2; ++side) {
          phi[side] = SU2_TYPE::GetValue(turb->GetSolution(node[side], v));
          w[side] = sst ? rho[side] * phi[side] : phi[side];
        }
        const double upwind = q > 0 ? w[node0] : q < 0 ? w[1 - node0] : 0.5 * (w[0] + w[1]);
        for (auto side = 0u; side < 2; ++side) {
          auto* row = (side == 0 ? tfI : tfJ) + v * nFlow;
          const double scale = sign * 0.5 * upwind / rho[side];
          row[0] -= scale * qn[side];
          for (auto d = 0u; d < nDim; ++d) row[1 + d] += scale * normal[d];
          /*--- Density weighting of the conservative flux of the same endpoint. ---*/
          if (sst) row[0] += sign * (side == node0 ? std::max(q, 0.0) : std::min(q, 0.0)) * phi[side];
        }
      }

      {
        /*--- Green-Gauss weight of the neighbour in grad u_i: n_ij / (2 V_i), with n_ij outward from i. ---*/
        double weight[MaxDim];
        for (auto d = 0u; d < nDim; ++d) {
          weight[d] = 0.5 * sign * normal[d] / volumeI;
          weightSum[d] += weight[d];
        }
        sourceChain(tfJ, j, weight);
      }

      if (viscousCoupling) {
        /*--- Viscous and heat fluxes through mu_t,avg = (mu_t,i + mu_t,j)/2, frozen corrected mean gradients
         *    (CCompressibleViscousFluxBase): dFv/dmu_t = [0, tau1.n, (tau1.n).u + cp/Pr_t grad(T).n]. ---*/
        double grad[MaxDim + 1][MaxDim] = {}, edge[MaxDim] = {}, dist2 = 0;
        for (auto d = 0u; d < nDim; ++d) {
          edge[d] = SU2_TYPE::GetValue(geometry->nodes->GetCoord(j, d) - geometry->nodes->GetCoord(i, d));
          dist2 += edge[d] * edge[d];
        }
        for (auto var = 0u; var < nVar; ++var) {
          double projected = 0;
          for (auto d = 0u; d < nDim; ++d) {
            grad[var][d] = 0.5 * SU2_TYPE::GetValue(flow->GetGradient_Primitive(i, var, d) +
                                                     flow->GetGradient_Primitive(j, var, d));
            projected += grad[var][d] * edge[d];
          }
          const double delta = var == 0 ? SU2_TYPE::GetValue(flow->GetTemperature(j) - flow->GetTemperature(i))
                                        : u[1][var - 1] - u[0][var - 1];
          const double correction = dist2 > 0 ? (projected - delta) / dist2 : 0;
          for (auto d = 0u; d < nDim; ++d) grad[var][d] -= correction * edge[d];
        }
        double divergence = 0;
        for (auto d = 0u; d < nDim; ++d) divergence += grad[d + 1][d];
        /*--- R_0 -= Fv, R_1 += Fv: dFv carries the row sign, the minus is applied below. ---*/
        double dFv[MaxVariables] = {};
        const double cp = 0.5 * SU2_TYPE::GetValue(flow->GetSpecificHeatCp(i) + flow->GetSpecificHeatCp(j));
        for (auto d = 0u; d < nDim; ++d) {
          double tauN = 0;
          for (auto k = 0u; k < nDim; ++k) {
            double tau = grad[k + 1][d] + grad[d + 1][k];
            if (k == d) tau -= 2.0 / 3.0 * divergence;
            tauN += tau * normal[k] * sign;
          }
          dFv[1 + d] = tauN;
          dFv[nDim + 1] += tauN * 0.5 * (u[0][d] + u[1][d]);
          dFv[nDim + 1] += cp / prandtlTurb * grad[0][d] * normal[d] * sign;
        }
        for (auto side = 0u; side < 2; ++side) {
          const auto* dMu = &eddyDerivative[node[side] * nTurb];
          auto* block = side == 0 ? ftI : ftJ;
          for (auto r = 0u; r < nFlow; ++r)
            for (auto v = 0u; v < nTurb; ++v) block[r * nTurb + v] -= 0.5 * dFv[r] * dMu[v];
        }
      }

      if (sst) {
        /*--- The static energy is e = E - u^2/2 - k, so rho k acts on the flow residual as -rho E does, except for the
         *    explicit rho E of the energy convection, whose central part is (rho E)_X (u_X.n)/2 (see below). ---*/
        for (auto side = 0u; side < 2; ++side) (side == 0 ? ftI : ftJ)[(nDim + 1) * nTurb] += sign * 0.5 * qn[side];
        /*--- Off-diagonal native column of this edge side. ---*/
        const auto* native = flowJacobian.GetBlock(i, j);
        if (native)
          for (auto r = 0u; r < nFlow; ++r) ftJ[r * nTurb] -= SU2_TYPE::GetValue(native[r * nFlow + nDim + 1]);
      }
    }
    {
      /*--- Own weight of a closed control volume: minus the sum of the neighbour weights. ---*/
      double weight[MaxDim];
      for (auto d = 0u; d < nDim; ++d) weight[d] = -weightSum[d];
      sourceChain(tfI, i, weight);
      /*--- SA source through nu = mu/rho at the point (the velocity part of the density column is chained above). ---*/
      if (!sst)
        for (auto v = 0u; v < nTurb; ++v) tfI[v * nFlow] += densityDerivative[i * nTurb + v];
    }
  }
  END_SU2_OMP_FOR
  ZeroPrescribedRows();
  ProjectSymmetryRows();
}

void CNewtonCoupling::ZeroPrescribedRows() {
  const auto& flowJacobian = solvers[FLOW_SOL]->Jacobian;
  const auto& turbJacobian = solvers[TURB_SOL]->Jacobian;
  SU2_OMP_FOR_DYN(256)
  for (auto i = 0ul; i < geometry->GetnPointDomain(); ++i) {
    std::vector<passivedouble*> tf{TF(true, i)}, ft{FT(true, i)};
    for (auto neighbour = 0u; neighbour < geometry->nodes->GetnPoint(i); ++neighbour) {
      const auto e = geometry->nodes->GetEdge(i, neighbour);
      const auto slot = 2 * e + (geometry->edges->GetNode(e, 0) == i ? 0 : 1);
      tf.push_back(TF(false, slot));
      ft.push_back(FT(false, slot));
    }
    for (auto v = 0u; v < nTurb; ++v)
      if (turbJacobian.IsPrescribedRow(i, v))
        for (auto* block : tf)
          for (auto c = 0u; c < nFlow; ++c) block[v * nFlow + c] = 0;
    for (auto r = 0u; r < nFlow; ++r)
      if (flowJacobian.IsPrescribedRow(i, r))
        for (auto* block : ft)
          for (auto c = 0u; c < nTurb; ++c) block[r * nTurb + c] = 0;
  }
  END_SU2_OMP_FOR
}

void CNewtonCoupling::ProjectSymmetryRows() {
  /*--- Native BC_Sym_Plane (also Euler walls) keeps only the tangential momentum rows: J <- (I - n n^T) J.
   *    Corner points can belong to several markers, so the projection runs on one thread. ---*/
  BEGIN_SU2_OMP_SAFE_GLOBAL_ACCESS {
    for (auto marker = 0u; marker < config->GetnMarker_All(); ++marker) {
      const auto kind = config->GetMarker_All_KindBC(marker);
      if (kind != SYMMETRY_PLANE && kind != EULER_WALL) continue;
      for (auto vertex = 0ul; vertex < geometry->nVertex[marker]; ++vertex) {
        const auto i = geometry->vertex[marker][vertex]->GetNode();
        if (!geometry->nodes->GetDomain(i)) continue;
        double unit[MaxDim] = {};
        const auto it = geometry->symmetryNormals[marker].find(vertex);
        if (it != geometry->symmetryNormals[marker].end()) {
          for (auto d = 0u; d < nDim; ++d) unit[d] = SU2_TYPE::GetValue(it->second[d]);
        } else {
          const auto* normal = geometry->vertex[marker][vertex]->GetNormal();
          const double area = SU2_TYPE::GetValue(GeometryToolbox::Norm(nDim, normal));
          for (auto d = 0u; d < nDim; ++d) unit[d] = SU2_TYPE::GetValue(normal[d]) / area;
        }
        auto project = [&](passivedouble* block) {
          for (auto c = 0u; c < nTurb; ++c) {
            double normalPart = 0;
            for (auto d = 0u; d < nDim; ++d) normalPart += block[(1 + d) * nTurb + c] * unit[d];
            for (auto d = 0u; d < nDim; ++d) block[(1 + d) * nTurb + c] -= normalPart * unit[d];
          }
        };
        project(FT(true, i));
        for (auto neighbour = 0u; neighbour < geometry->nodes->GetnPoint(i); ++neighbour) {
          const auto e = geometry->nodes->GetEdge(i, neighbour);
          project(FT(false, 2 * e + (geometry->edges->GetNode(e, 0) == i ? 0 : 1)));
        }
      }
    }
  }
  END_SU2_OMP_SAFE_GLOBAL_ACCESS
}
