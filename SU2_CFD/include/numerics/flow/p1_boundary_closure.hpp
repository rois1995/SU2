/*!
 * \file p1_boundary_closure.hpp
 * \brief Closure C of BC_ENTROPY_P1 on static meshes (P1_BOUNDARY.md, Thm B9; P1_NOSLIP.md, 11): the face residual of
 *        Euler walls, symmetry planes, far field, inlets and outlets, and its exact Jacobian, ideal gas, scalar code.
 * \version 8.6.0 "Harrier"
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

#pragma once

#include <cmath>
#include <string>
#include "../../../../Common/include/basic_types/datatype_structure.hpp"
#include "../../../../Common/include/option_structure.hpp"
#include "../../../../Common/include/parallelization/mpi_structure.hpp"
#include "../../../../Common/include/toolboxes/geometry_toolbox.hpp"
#include "p1_boundary_ghost.hpp"
#include "tecno_adjoint.hpp"

/*--- The residual part was the anonymous namespace of CSolver.cpp (Weak_BC_Residual and WeakNoSlipWallResidual call
 * it), moved here unchanged so that the unit tests reach it. ---*/

namespace P1BoundaryClosure {

/*--- BC_ENTROPY_P1: closure C of the weak boundary (P1_BOUNDARY.md, Thm B9), ideal gas, scalar code.
 * Entropy U = -rho s/(gamma-1), s = ln(p rho^-gamma), entropy variables
 * w = ((gamma-s)/(gamma-1) - beta|u|^2, 2 beta u, -2 beta), beta = rho/(2p). ---*/

constexpr unsigned short ES_NVAR = 5;

struct EsState {
  su2double rho = 1.0, vel[3] = {0.0, 0.0, 0.0}, p = 1.0;
};

/*--- Logarithmic mean (a-b)/(ln a - ln b), stable form of Ismail and Roe (2009), as logMean of the vectorized
 * numerics: f = (a-b)/(a+b), u = f^2, mean = (a+b)/(2F), F = ln(a/b)/(2f) = 1 + u/3 + u^2/5 + u^3/7 + ... ---*/
inline su2double EsLogMean(const su2double& a, const su2double& b) {
  const su2double f = (a - b) / (a + b);
  const su2double u = f * f;
  if (u < 1e-2) return (a + b) / (2.0 * (1.0 + u * (1.0/3.0 + u * (1.0/5.0 + u * (1.0/7.0)))));
  return (a + b) * f / log(a / b);
}

/*--- Ismail-Roe entropy conservative flux projected on "normal", the same formulas as ismailRoeProjFlux of the
 * vectorized numerics. Symmetric in the two states, equal to the physical flux for equal states. ---*/
inline void EsIsmailRoeFlux(unsigned short nDim, const su2double& gamma, const EsState& L, const EsState& R,
                     const su2double* normal, su2double* flux) {
  const su2double zL = sqrt(L.rho / L.p), zR = sqrt(R.rho / R.p);
  const su2double yL = zL * L.p, yR = zR * R.p;
  const su2double zAvg = 0.5 * (zL + zR), yAvg = 0.5 * (yL + yR);
  const su2double zLog = EsLogMean(zL, zR), yLog = EsLogMean(yL, yR);

  const su2double density = zAvg * yLog;
  su2double vel[3] = {0.0, 0.0, 0.0}, vel2 = 0.0, velN = 0.0;
  for (unsigned short iDim = 0; iDim < nDim; ++iDim) {
    vel[iDim] = 0.5 * (zL * L.vel[iDim] + zR * R.vel[iDim]) / zAvg;
    vel2 += vel[iDim] * vel[iDim];
    velN += vel[iDim] * normal[iDim];
  }
  const su2double pressure1 = yAvg / zAvg;
  const su2double pressure2 = ((gamma + 1.0) * yLog / zLog + (gamma - 1.0) * pressure1) / (2.0 * gamma);
  const su2double enthalpy = gamma * pressure2 / ((gamma - 1.0) * density) + 0.5 * vel2;

  const su2double mdot = density * velN;
  flux[0] = mdot;
  for (unsigned short iDim = 0; iDim < nDim; ++iDim) flux[iDim+1] = mdot * vel[iDim] + normal[iDim] * pressure1;
  flux[nDim+1] = mdot * enthalpy;
}

inline void EsEntropyVariables(unsigned short nDim, const su2double& gamma, const EsState& S, su2double* w) {
  const su2double beta = 0.5 * S.rho / S.p;
  su2double vel2 = 0.0;
  for (unsigned short iDim = 0; iDim < nDim; ++iDim) vel2 += S.vel[iDim] * S.vel[iDim];
  const su2double entropy = log(S.p) - gamma * log(S.rho);
  w[0] = (gamma - entropy) / (gamma - 1.0) - beta * vel2;
  for (unsigned short iDim = 0; iDim < nDim; ++iDim) w[iDim+1] = 2.0 * beta * S.vel[iDim];
  w[nDim+1] = -2.0 * beta;
}

/*--- Inverse map u(w). It needs only w[nDim+1] < 0, which a convex combination of entropy variables keeps, so the
 * projected state is admissible (rho, p > 0): beta = -w_E/2, u = w_mom/(2 beta), s = gamma - (gamma-1)(w_0 + beta|u|^2),
 * p rho^-gamma = e^s with p = rho/(2 beta), i.e. rho^(1-gamma) = 2 beta e^s. ---*/
inline void EsStateFromEntropyVariables(unsigned short nDim, const su2double& gamma, const su2double* w, EsState& S) {
  const su2double beta = -0.5 * w[nDim+1];
  su2double vel2 = 0.0;
  for (unsigned short iDim = 0; iDim < nDim; ++iDim) {
    S.vel[iDim] = 0.5 * w[iDim+1] / beta;
    vel2 += S.vel[iDim] * S.vel[iDim];
  }
  const su2double entropy = gamma - (gamma - 1.0) * (w[0] + beta * vel2);
  S.rho = exp((log(2.0 * beta) + entropy) / (1.0 - gamma));
  S.p = 0.5 * S.rho / beta;
}

inline void EsConservative(unsigned short nDim, const su2double& gamma, const EsState& S, su2double* U) {
  su2double vel2 = 0.0;
  U[0] = S.rho;
  for (unsigned short iDim = 0; iDim < nDim; ++iDim) {
    U[iDim+1] = S.rho * S.vel[iDim];
    vel2 += S.vel[iDim] * S.vel[iDim];
  }
  U[nDim+1] = S.p / (gamma - 1.0) + 0.5 * S.rho * vel2;
}

/*--- Characteristic far-field ghost of CEulerSolver::BC_Far_Field_Residual (Riemann invariants, no grid motion),
 * built from the state S at a face point instead of the nodal state (P1BoundaryGhost::FarField). ---*/
inline void EsFarFieldGhost(unsigned short nDim, const su2double& gamma, const EsState& S, const EsState& Inf,
                     const su2double* unitNormal, EsState& G) {
  P1BoundaryGhost::FarFieldState ghost;
  P1BoundaryGhost::FarField(nDim, gamma, S.rho, S.vel, S.p, Inf.rho, Inf.vel, Inf.p, unitNormal, nullptr, ghost);
  G.rho = ghost.Density;
  G.p = ghost.Pressure;
  for (unsigned short iDim = 0; iDim < nDim; ++iDim) G.vel[iDim] = ghost.Velocity[iDim];
}

/*--- BC_ENTROPY_P1 (P1_NOSLIP.md, 11): the marker of a boundary face and its data at the face nodes, in the order of
 * the face states of EsFaceResidual. Far field, inlet and outlet get the ghost of the weak boundary residuals of
 * CEulerSolver at every face point, from the projected state u~_q and the data interpolated to the point; walls and
 * symmetry planes the pressure flux. ---*/
struct EsBoundaryData {
  unsigned short kind = EULER_WALL;   /*!< \brief KindBC of the marker. */
  std::string tag;                       /*!< \brief Marker tag, for the messages. */
  EsState inf;                        /*!< \brief Free stream (far field). */
  bool massFlow = false;              /*!< \brief Inlet: mass flow, otherwise total conditions. */
  bool useNormal = false;             /*!< \brief Inlet: flow along the negative face normal. */
  su2double gasConst = 1.0, refT = 1.0, refP = 1.0;   /*!< \brief Inlet: gas constant, references of the data. */
  su2double dataT[3] = {0.0, 0.0, 0.0};               /*!< \brief Inlet: Inlet_Ttotal of the face nodes. */
  su2double dataP[3] = {0.0, 0.0, 0.0};               /*!< \brief Inlet: Inlet_Ptotal of the face nodes. */
  su2double flowDir[3][3] = {{0.0}};                  /*!< \brief Inlet: flow direction of the face nodes. */
  su2double pExit[3] = {0.0, 0.0, 0.0};               /*!< \brief Outlet: non-dimensional back pressure of the face nodes. */
};

/*--- Ghost g_q at a face point with phi_k(x_q) = phiq[k]. False for walls and symmetry planes (pressure flux). The
 * ghost must be admissible (Lemma B7 needs rho, p > 0), otherwise the run stops. ---*/
inline bool EsBoundaryGhost(unsigned short nDim, unsigned short nNodes, const su2double& gamma, const EsBoundaryData& data,
                     const su2double* phiq, const EsState& sq, const su2double* unitNormal, EsState& gq) {
  switch (data.kind) {
    case FAR_FIELD:
      EsFarFieldGhost(nDim, gamma, sq, data.inf, unitNormal, gq);
      break;
    case INLET_FLOW: {
      su2double dataT = 0.0, dataP = 0.0, flowDir[3] = {0.0, 0.0, 0.0}, vel2 = 0.0;
      for (unsigned short k = 0; k < nNodes; ++k) {
        dataT += phiq[k] * data.dataT[k];
        dataP += phiq[k] * data.dataP[k];
        for (unsigned short iDim = 0; iDim < nDim; ++iDim) flowDir[iDim] += phiq[k] * data.flowDir[k][iDim];
      }
      for (unsigned short iDim = 0; iDim < nDim; ++iDim) vel2 += sq.vel[iDim] * sq.vel[iDim];
      const su2double enthalpy = gamma * sq.p / ((gamma - 1.0) * sq.rho) + 0.5 * vel2;
      P1BoundaryGhost::InletState ghost;
      P1BoundaryGhost::Inlet(nDim, data.massFlow, gamma, data.gasConst, sq.rho, sq.vel, sq.p, enthalpy, dataT, dataP,
                             flowDir, data.useNormal, unitNormal, data.refT, data.refP, ghost);
      gq.rho = ghost.Density;
      gq.p = ghost.Pressure;
      for (unsigned short iDim = 0; iDim < nDim; ++iDim) gq.vel[iDim] = ghost.Velocity[iDim];
      break;
    }
    case OUTLET_FLOW: {
      su2double pExit = 0.0;
      for (unsigned short k = 0; k < nNodes; ++k) pExit += phiq[k] * data.pExit[k];
      P1BoundaryGhost::OutletState ghost;
      P1BoundaryGhost::Outlet(nDim, gamma, sq.rho, sq.vel, sq.p, pExit, unitNormal, ghost);
      gq.rho = ghost.Density;
      gq.p = ghost.Pressure;
      for (unsigned short iDim = 0; iDim < nDim; ++iDim) gq.vel[iDim] = ghost.Velocity[iDim];
      break;
    }
    default:
      return false;
  }
  if (!P1BoundaryGhost::Admissible(gq.rho, gq.p)) {
    SU2_MPI::Error("BC_ENTROPY_P1: inadmissible ghost state (rho or p not positive) on marker " + data.tag + ".",
                   CURRENT_FUNCTION);
  }
  return true;
}

/*--- Closure C on one boundary simplex face (segment: 2 nodes, triangle: 3 nodes) with the outward area vector
 * faceNormal = A_f nu_f. Points x_q, weights a_q A_f: 2-point Gauss on segments (a_q = 1/2), barycentric
 * (2/3,1/6,1/6) on triangles (a_q = 1/3), both exact to degree 2. With N_q = a_q A_f nu_f,
 * w~_q = sum_k phi_k(x_q) w_k, u~_q = u(w~_q) and G_qk = F_IR(u~_q, u_k; N_q):
 *   b_i = sum_q phi_i(x_q) [ G_qi - sum_k phi_k(x_q) G_qk + F*_q ],
 * F*_q = (0, p~_q N_q, 0) at walls and symmetry planes (entropy conservative, also for data = nullptr), and at the
 * far field, inlets and outlets (EsBoundaryGhost)
 * F*_q = F_IR(u~_q, g_q; N_q) - lam/2 |N_q| (U(g_q) - U(u~_q)), lam = max(|u.nu| + c) over u~_q and the ghost g_q,
 * Tadmor entropy stable for any admissible ghost (Lemma B7). ---*/
inline void EsFaceResidual(unsigned short nDim, unsigned short nNodes, const su2double& gamma, const EsBoundaryData* data,
                    const su2double* faceNormal, const EsState* nodeState, su2double (*res)[ES_NVAR]) {
  const unsigned short nVar = nDim + 2;

  su2double phi[3][3] = {{0.0}}, weight = 0.0;
  if (nNodes == 2) {
    const su2double g = 0.5 / sqrt(3.0);
    phi[0][0] = 0.5 + g; phi[0][1] = 0.5 - g;
    phi[1][0] = 0.5 - g; phi[1][1] = 0.5 + g;
    weight = 0.5;
  } else {
    for (unsigned short q = 0; q < 3; ++q)
      for (unsigned short k = 0; k < 3; ++k) phi[q][k] = (q == k) ? 2.0 / 3.0 : 1.0 / 6.0;
    weight = 1.0 / 3.0;
  }

  const su2double area = GeometryToolbox::Norm(nDim, faceNormal);
  su2double unitNormal[3] = {0.0, 0.0, 0.0}, pointNormal[3] = {0.0, 0.0, 0.0};
  for (unsigned short iDim = 0; iDim < nDim; ++iDim) {
    unitNormal[iDim] = faceNormal[iDim] / area;
    pointNormal[iDim] = weight * faceNormal[iDim];
  }

  su2double w[3][ES_NVAR] = {{0.0}};
  for (unsigned short k = 0; k < nNodes; ++k) EsEntropyVariables(nDim, gamma, nodeState[k], w[k]);

  for (unsigned short i = 0; i < nNodes; ++i)
    for (unsigned short iVar = 0; iVar < nVar; ++iVar) res[i][iVar] = 0.0;

  for (unsigned short q = 0; q < nNodes; ++q) {
    su2double wq[ES_NVAR] = {0.0};
    for (unsigned short k = 0; k < nNodes; ++k)
      for (unsigned short iVar = 0; iVar < nVar; ++iVar) wq[iVar] += phi[q][k] * w[k][iVar];
    EsState sq;
    EsStateFromEntropyVariables(nDim, gamma, wq, sq);

    /*--- G_qk once per node: F_IR is symmetric, so it serves both F_ec(u_i, u~_q) and F_ec(u~_q, u_k). ---*/
    su2double G[3][ES_NVAR] = {{0.0}}, Gbar[ES_NVAR] = {0.0};
    for (unsigned short k = 0; k < nNodes; ++k) {
      EsIsmailRoeFlux(nDim, gamma, sq, nodeState[k], pointNormal, G[k]);
      for (unsigned short iVar = 0; iVar < nVar; ++iVar) Gbar[iVar] += phi[q][k] * G[k][iVar];
    }

    su2double Fstar[ES_NVAR] = {0.0};
    EsState gq;
    const bool ghost = (data != nullptr) && EsBoundaryGhost(nDim, nNodes, gamma, *data, phi[q], sq, unitNormal, gq);
    if (!ghost) {
      for (unsigned short iDim = 0; iDim < nDim; ++iDim) Fstar[iDim+1] = sq.p * pointNormal[iDim];
    } else {
      EsIsmailRoeFlux(nDim, gamma, sq, gq, pointNormal, Fstar);
      su2double velNq = 0.0, velNg = 0.0;
      for (unsigned short iDim = 0; iDim < nDim; ++iDim) {
        velNq += sq.vel[iDim] * unitNormal[iDim];
        velNg += gq.vel[iDim] * unitNormal[iDim];
      }
      const su2double lamQ = fabs(velNq) + sqrt(gamma * sq.p / sq.rho);
      const su2double lamG = fabs(velNg) + sqrt(gamma * gq.p / gq.rho);
      const su2double lam = fmax(lamQ, lamG);
      su2double Uq[ES_NVAR] = {0.0}, Ug[ES_NVAR] = {0.0};
      EsConservative(nDim, gamma, sq, Uq);
      EsConservative(nDim, gamma, gq, Ug);
      for (unsigned short iVar = 0; iVar < nVar; ++iVar) Fstar[iVar] -= 0.5 * lam * weight * area * (Ug[iVar] - Uq[iVar]);
    }

    for (unsigned short i = 0; i < nNodes; ++i)
      for (unsigned short iVar = 0; iVar < nVar; ++iVar) res[i][iVar] += phi[q][i] * (G[i][iVar] - Gbar[iVar] + Fstar[iVar]);
  }
}

/*--- Exact Jacobian of closure C (P1_BOUNDARY.md, 11). With M_qj = dU~_q/dU_j = phi_j(x_q) C_q W_j, C_q = dU/dw at
 * u~_q (the derivative of EsStateFromEntropyVariables) and W_j = dw/dU at u_j (TeCNOAdjoint::EntropyHessian),
 * A_qk, B_qk the derivatives of G_qk = F_IR(u~_q, u_k; N_q) in its first and second state and D_q = dF*_q/dU~_q,
 *   d b_i/d U_j = sum_q phi_i(x_q) [ (A_qi - sum_k phi_k(x_q) A_qk + D_q) M_qj + (delta_ij - phi_j(x_q)) B_qj ].
 * Pressure flux: D_q = (0, N_q dp/dU, 0). Ghost flux: with Gam_q = dU(g_q)/dU~_q (P1BoundaryGhost::*Jacobian),
 *   D_q = A^g_q + B^g_q Gam_q - lam/2 |N_q| (Gam_q - I) - |N_q|/2 (U(g_q) - U~_q) (x) dlam/dU~_q,
 * lam differentiated on its active branch (u~_q when lam_Q >= lam_G, as fmax), |u.nu| with the sign of u.nu. All
 * derivatives are those of the residual code, exact where it is differentiable. ---*/

/*--- C = A B for nVar x nVar blocks. ---*/
inline void EsMatMul(unsigned short nVar, const su2double (*A)[ES_NVAR], const su2double (*B)[ES_NVAR],
                     su2double (*C)[ES_NVAR]) {
  for (unsigned short i = 0; i < nVar; ++i) {
    for (unsigned short j = 0; j < nVar; ++j) {
      su2double sum = 0.0;
      for (unsigned short k = 0; k < nVar; ++k) sum += A[i][k] * B[k][j];
      C[i][j] = sum;
    }
  }
}

/*--- dP/dU of the primitive variables P = (rho, u, p) at S. ---*/
inline void EsPrimitiveJacobian(unsigned short nDim, const su2double& gamma, const EsState& S, su2double (*J)[ES_NVAR]) {
  const unsigned short nVar = nDim + 2;
  for (unsigned short a = 0; a < nVar; ++a)
    for (unsigned short b = 0; b < nVar; ++b) J[a][b] = 0.0;
  su2double vel2 = 0.0;
  J[0][0] = 1.0;
  for (unsigned short iDim = 0; iDim < nDim; ++iDim) {
    J[iDim+1][0] = -S.vel[iDim] / S.rho;
    J[iDim+1][iDim+1] = 1.0 / S.rho;
    J[nDim+1][iDim+1] = -(gamma - 1.0) * S.vel[iDim];
    vel2 += S.vel[iDim] * S.vel[iDim];
  }
  J[nDim+1][0] = 0.5 * (gamma - 1.0) * vel2;
  J[nDim+1][nDim+1] = gamma - 1.0;
}

/*--- dU/dP at S. ---*/
inline void EsConservativeJacobian(unsigned short nDim, const su2double& gamma, const EsState& S,
                                   su2double (*J)[ES_NVAR]) {
  const unsigned short nVar = nDim + 2;
  for (unsigned short a = 0; a < nVar; ++a)
    for (unsigned short b = 0; b < nVar; ++b) J[a][b] = 0.0;
  su2double vel2 = 0.0;
  J[0][0] = 1.0;
  for (unsigned short iDim = 0; iDim < nDim; ++iDim) {
    J[iDim+1][0] = S.vel[iDim];
    J[iDim+1][iDim+1] = S.rho;
    J[nDim+1][iDim+1] = S.rho * S.vel[iDim];
    vel2 += S.vel[iDim] * S.vel[iDim];
  }
  J[nDim+1][0] = 0.5 * vel2;
  J[nDim+1][nDim+1] = 1.0 / (gamma - 1.0);
}

/*--- dP/dw of EsStateFromEntropyVariables at its result S: d beta = -dw_E/2, du = (dw_mom/2 - u d beta)/beta,
 * ds = -(gamma-1)(dw_0 + |u|^2 d beta + 2 beta u.du), d rho = rho (d beta/beta + ds)/(1-gamma),
 * dp = p (d rho/rho - d beta/beta). ---*/
inline void EsProjectionJacobian(unsigned short nDim, const su2double& gamma, const EsState& S,
                                 su2double (*J)[ES_NVAR]) {
  const unsigned short nVar = nDim + 2;
  const su2double beta = 0.5 * S.rho / S.p;
  su2double vel2 = 0.0;
  for (unsigned short iDim = 0; iDim < nDim; ++iDim) vel2 += S.vel[iDim] * S.vel[iDim];
  for (unsigned short c = 0; c < nVar; ++c) {
    su2double dw[ES_NVAR] = {0.0};
    dw[c] = 1.0;
    const su2double dBeta = -0.5 * dw[nDim+1];
    su2double velDVel = 0.0;
    for (unsigned short iDim = 0; iDim < nDim; ++iDim) {
      J[iDim+1][c] = (0.5 * dw[iDim+1] - S.vel[iDim] * dBeta) / beta;
      velDVel += S.vel[iDim] * J[iDim+1][c];
    }
    const su2double dEntropy = -(gamma - 1.0) * (dw[0] + vel2 * dBeta + 2.0 * beta * velDVel);
    J[0][c] = S.rho * (dBeta / beta + dEntropy) / (1.0 - gamma);
    J[nDim+1][c] = S.p * (J[0][c] / S.rho - dBeta / beta);
  }
}

/*--- EsLogMean and its partial derivatives, scalar form of logMeanDerivatives (numerics_simd common.hpp) with the
 * branch of EsLogMean: mean = (a+b)/(2F(f)), f = (a-b)/(a+b), F the series or atanh(f)/f. ---*/
inline void EsLogMeanDerivatives(const su2double& a, const su2double& b, su2double& mean, su2double& dMean_a,
                                 su2double& dMean_b) {
  const su2double sum = a + b;
  const su2double f = (a - b) / sum;
  const su2double u = f * f;
  su2double F, dF;
  if (u < 1e-2) {
    F = 1.0 + u * (1.0/3.0 + u * (1.0/5.0 + u * (1.0/7.0)));
    dF = 2.0 * f * (1.0/3.0 + u * (2.0/5.0 + u * (3.0/7.0)));
  } else {
    F = log(a / b) / (2.0 * f);
    dF = (1.0 / (1.0 - u) - F) / f;
  }
  mean = sum / (2.0 * F);
  const su2double factor = dF / (sum * F * F);
  dMean_a = 0.5 / F - b * factor;
  dMean_b = 0.5 / F + a * factor;
}

/*--- Exact Jacobians jacL = dF/dU_L, jacR = dF/dU_R of EsIsmailRoeFlux, the scalar form of ismailRoeProjJac
 * (EDGE_FORM_EC_EXACT_JACOBIAN, numerics_simd common.hpp): chain rule through z = sqrt(rho/p), y = sqrt(rho p) and
 * the velocity of each state. ---*/
inline void EsIsmailRoeJacobian(unsigned short nDim, const su2double& gamma, const EsState& L, const EsState& R,
                                const su2double* normal, su2double (*jacL)[ES_NVAR], su2double (*jacR)[ES_NVAR]) {
  const unsigned short nVar = nDim + 2;
  const su2double zL = sqrt(L.rho / L.p), zR = sqrt(R.rho / R.p);
  const su2double yL = zL * L.p, yR = zR * R.p;
  const su2double zAvg = 0.5 * (zL + zR), yAvg = 0.5 * (yL + yR);
  su2double zLog, dzLogL, dzLogR, yLog, dyLogL, dyLogR;
  EsLogMeanDerivatives(zL, zR, zLog, dzLogL, dzLogR);
  EsLogMeanDerivatives(yL, yR, yLog, dyLogL, dyLogR);

  const su2double density = zAvg * yLog;
  su2double vel[3] = {0.0, 0.0, 0.0}, vel2 = 0.0, velN = 0.0;
  for (unsigned short iDim = 0; iDim < nDim; ++iDim) {
    vel[iDim] = 0.5 * (zL * L.vel[iDim] + zR * R.vel[iDim]) / zAvg;
    vel2 += vel[iDim] * vel[iDim];
    velN += vel[iDim] * normal[iDim];
  }
  const su2double pressure1 = yAvg / zAvg;
  const su2double pressure2 = ((gamma + 1.0) * yLog / zLog + (gamma - 1.0) * pressure1) / (2.0 * gamma);
  const su2double enthalpy = gamma * pressure2 / ((gamma - 1.0) * density) + 0.5 * vel2;
  const su2double mdot = density * velN;
  const su2double hFactor = gamma / ((gamma - 1.0) * density);

  const EsState* sides[2] = {&L, &R};
  const su2double zSide[2] = {zL, zR}, ySide[2] = {yL, yR};
  const su2double dzLogSide[2] = {dzLogL, dzLogR}, dyLogSide[2] = {dyLogL, dyLogR};

  for (unsigned short side = 0; side < 2; ++side) {
    const EsState& V = *sides[side];
    const su2double& z = zSide[side];
    const su2double& y = ySide[side];
    su2double (*jac)[ES_NVAR] = (side == 0) ? jacL : jacR;

    /*--- Partial derivatives with respect to z, y and the velocity of this state. ---*/
    const su2double dRho_z = 0.5 * yLog;
    const su2double dRho_y = zAvg * dyLogSide[side];
    su2double dVel_z[3] = {0.0, 0.0, 0.0}, velDVel = 0.0, normalDVel = 0.0;
    for (unsigned short iDim = 0; iDim < nDim; ++iDim) {
      dVel_z[iDim] = (V.vel[iDim] - vel[iDim]) / (2.0 * zAvg);
      velDVel += vel[iDim] * dVel_z[iDim];
      normalDVel += normal[iDim] * dVel_z[iDim];
    }
    const su2double dVel_u = z / (2.0 * zAvg);
    const su2double dP1_z = -pressure1 / (2.0 * zAvg);
    const su2double dP1_y = 1.0 / (2.0 * zAvg);
    const su2double dP2_z = (-(gamma + 1.0) * yLog * dzLogSide[side] / (zLog * zLog) + (gamma - 1.0) * dP1_z) / (2.0 * gamma);
    const su2double dP2_y = ((gamma + 1.0) * dyLogSide[side] / zLog + (gamma - 1.0) * dP1_y) / (2.0 * gamma);
    const su2double dH_z = hFactor * (dP2_z - pressure2 / density * dRho_z) + velDVel;
    const su2double dH_y = hFactor * (dP2_y - pressure2 / density * dRho_y);
    const su2double dMdot_z = dRho_z * velN + density * normalDVel;
    const su2double dMdot_y = dRho_y * velN;

    su2double dF_z[ES_NVAR] = {0.0}, dF_y[ES_NVAR] = {0.0}, dF_u[ES_NVAR][3] = {{0.0}};
    dF_z[0] = dMdot_z;
    dF_y[0] = dMdot_y;
    for (unsigned short iDim = 0; iDim < nDim; ++iDim) {
      dF_z[iDim+1] = dMdot_z * vel[iDim] + mdot * dVel_z[iDim] + normal[iDim] * dP1_z;
      dF_y[iDim+1] = dMdot_y * vel[iDim] + normal[iDim] * dP1_y;
    }
    dF_z[nDim+1] = dMdot_z * enthalpy + mdot * dH_z;
    dF_y[nDim+1] = dMdot_y * enthalpy + mdot * dH_y;
    for (unsigned short jDim = 0; jDim < nDim; ++jDim) {
      const su2double dMdot_u = density * normal[jDim] * dVel_u;
      dF_u[0][jDim] = dMdot_u;
      for (unsigned short iDim = 0; iDim < nDim; ++iDim) dF_u[iDim+1][jDim] = dMdot_u * vel[iDim];
      dF_u[jDim+1][jDim] += mdot * dVel_u;
      dF_u[nDim+1][jDim] = dMdot_u * enthalpy + mdot * vel[jDim] * dVel_u;
    }

    /*--- Chain rule to the conservative variables of this state. ---*/
    su2double sideVel2 = 0.0;
    for (unsigned short iDim = 0; iDim < nDim; ++iDim) sideVel2 += V.vel[iDim] * V.vel[iDim];
    su2double dp[ES_NVAR] = {0.0}, dz[ES_NVAR] = {0.0}, dy[ES_NVAR] = {0.0};
    dp[0] = 0.5 * (gamma - 1.0) * sideVel2;
    for (unsigned short iDim = 0; iDim < nDim; ++iDim) dp[iDim+1] = -(gamma - 1.0) * V.vel[iDim];
    dp[nDim+1] = gamma - 1.0;
    for (unsigned short iVar = 0; iVar < nVar; ++iVar) {
      dz[iVar] = -z / (2.0 * V.p) * dp[iVar];
      dy[iVar] = y / (2.0 * V.p) * dp[iVar];
    }
    dz[0] += z / (2.0 * V.rho);
    dy[0] += y / (2.0 * V.rho);
    for (unsigned short iVar = 0; iVar < nVar; ++iVar) {
      su2double d0 = dF_z[iVar] * dz[0] + dF_y[iVar] * dy[0];
      for (unsigned short kDim = 0; kDim < nDim; ++kDim) d0 -= dF_u[iVar][kDim] * V.vel[kDim] / V.rho;
      jac[iVar][0] = d0;
      for (unsigned short jVar = 1; jVar < nVar; ++jVar) jac[iVar][jVar] = dF_z[iVar] * dz[jVar] + dF_y[iVar] * dy[jVar];
      for (unsigned short kDim = 0; kDim < nDim; ++kDim) jac[iVar][kDim+1] += dF_u[iVar][kDim] / V.rho;
    }
  }
}

static_assert(ES_NVAR == P1BoundaryGhost::JAC_NVAR, "closure C and the ghost Jacobians use the same block size");

/*--- dG[a][b] = dP(g_q)[a]/dP(u~_q)[b] of the ghost of EsBoundaryGhost, the same marker data at the point. False for
 * walls and symmetry planes. ---*/
inline bool EsBoundaryGhostJacobian(unsigned short nDim, unsigned short nNodes, const su2double& gamma,
                                    const EsBoundaryData& data, const su2double* phiq, const EsState& sq,
                                    const su2double* unitNormal, su2double (*dG)[ES_NVAR]) {
  switch (data.kind) {
    case FAR_FIELD:
      P1BoundaryGhost::FarFieldJacobian(nDim, gamma, sq.rho, sq.vel, sq.p, data.inf.rho, data.inf.vel, data.inf.p,
                                        unitNormal, dG);
      return true;
    case INLET_FLOW: {
      su2double dataT = 0.0, dataP = 0.0, flowDir[3] = {0.0, 0.0, 0.0}, vel2 = 0.0;
      for (unsigned short k = 0; k < nNodes; ++k) {
        dataT += phiq[k] * data.dataT[k];
        dataP += phiq[k] * data.dataP[k];
        for (unsigned short iDim = 0; iDim < nDim; ++iDim) flowDir[iDim] += phiq[k] * data.flowDir[k][iDim];
      }
      for (unsigned short iDim = 0; iDim < nDim; ++iDim) vel2 += sq.vel[iDim] * sq.vel[iDim];
      const su2double enthalpy = gamma * sq.p / ((gamma - 1.0) * sq.rho) + 0.5 * vel2;
      P1BoundaryGhost::InletJacobian(nDim, data.massFlow, gamma, data.gasConst, sq.rho, sq.vel, sq.p, enthalpy, dataT,
                                     dataP, flowDir, data.useNormal, unitNormal, data.refT, data.refP, dG);
      return true;
    }
    case OUTLET_FLOW: {
      su2double pExit = 0.0;
      for (unsigned short k = 0; k < nNodes; ++k) pExit += phiq[k] * data.pExit[k];
      P1BoundaryGhost::OutletJacobian(nDim, gamma, sq.rho, sq.vel, sq.p, pExit, unitNormal, dG);
      return true;
    }
    default:
      return false;
  }
}

/*!
 * \brief Exact Jacobian of EsFaceResidual: jac[i][j][iVar][jVar] = d res[i][iVar] / d U_j[jVar], U_j the conservative
 *        variables of face node j. Same arguments as EsFaceResidual; the residual itself is not changed.
 */
inline void EsFaceJacobian(unsigned short nDim, unsigned short nNodes, const su2double& gamma,
                           const EsBoundaryData* data, const su2double* faceNormal, const EsState* nodeState,
                           su2double (*jac)[3][ES_NVAR][ES_NVAR]) {
  const unsigned short nVar = nDim + 2;

  /*--- Face rule and normals of EsFaceResidual. ---*/
  su2double phi[3][3] = {{0.0}}, weight = 0.0;
  if (nNodes == 2) {
    const su2double g = 0.5 / sqrt(3.0);
    phi[0][0] = 0.5 + g; phi[0][1] = 0.5 - g;
    phi[1][0] = 0.5 - g; phi[1][1] = 0.5 + g;
    weight = 0.5;
  } else {
    for (unsigned short q = 0; q < 3; ++q)
      for (unsigned short k = 0; k < 3; ++k) phi[q][k] = (q == k) ? 2.0 / 3.0 : 1.0 / 6.0;
    weight = 1.0 / 3.0;
  }

  const su2double area = GeometryToolbox::Norm(nDim, faceNormal);
  su2double unitNormal[3] = {0.0, 0.0, 0.0}, pointNormal[3] = {0.0, 0.0, 0.0};
  for (unsigned short iDim = 0; iDim < nDim; ++iDim) {
    unitNormal[iDim] = faceNormal[iDim] / area;
    pointNormal[iDim] = weight * faceNormal[iDim];
  }
  const su2double pointArea = weight * area;

  su2double w[3][ES_NVAR] = {{0.0}};
  TeCNOAdjoint::Block W[3];
  for (unsigned short k = 0; k < nNodes; ++k) {
    EsEntropyVariables(nDim, gamma, nodeState[k], w[k]);
    TeCNOAdjoint::EntropyHessian(nDim, gamma, nodeState[k].rho, nodeState[k].vel, nodeState[k].p, W[k]);
  }

  for (unsigned short i = 0; i < nNodes; ++i)
    for (unsigned short j = 0; j < nNodes; ++j)
      for (unsigned short a = 0; a < nVar; ++a)
        for (unsigned short b = 0; b < nVar; ++b) jac[i][j][a][b] = 0.0;

  for (unsigned short q = 0; q < nNodes; ++q) {
    su2double wq[ES_NVAR] = {0.0};
    for (unsigned short k = 0; k < nNodes; ++k)
      for (unsigned short iVar = 0; iVar < nVar; ++iVar) wq[iVar] += phi[q][k] * w[k][iVar];
    EsState sq;
    EsStateFromEntropyVariables(nDim, gamma, wq, sq);

    /*--- C_q = dU~/dw~ and dP/dU at u~_q. ---*/
    su2double dPdU[ES_NVAR][ES_NVAR], dUdP[ES_NVAR][ES_NVAR], dPdw[ES_NVAR][ES_NVAR], C[ES_NVAR][ES_NVAR];
    EsPrimitiveJacobian(nDim, gamma, sq, dPdU);
    EsConservativeJacobian(nDim, gamma, sq, dUdP);
    EsProjectionJacobian(nDim, gamma, sq, dPdw);
    EsMatMul(nVar, dUdP, dPdw, C);

    /*--- A_qk, B_qk and their phi average. ---*/
    su2double A[3][ES_NVAR][ES_NVAR], B[3][ES_NVAR][ES_NVAR], Abar[ES_NVAR][ES_NVAR] = {{0.0}};
    for (unsigned short k = 0; k < nNodes; ++k) {
      EsIsmailRoeJacobian(nDim, gamma, sq, nodeState[k], pointNormal, A[k], B[k]);
      for (unsigned short a = 0; a < nVar; ++a)
        for (unsigned short b = 0; b < nVar; ++b) Abar[a][b] += phi[q][k] * A[k][a][b];
    }

    /*--- D_q = dF*_q/dU~_q. ---*/
    su2double D[ES_NVAR][ES_NVAR] = {{0.0}};
    EsState gq;
    const bool ghost = (data != nullptr) && EsBoundaryGhost(nDim, nNodes, gamma, *data, phi[q], sq, unitNormal, gq);
    if (!ghost) {
      for (unsigned short iDim = 0; iDim < nDim; ++iDim)
        for (unsigned short b = 0; b < nVar; ++b) D[iDim+1][b] = pointNormal[iDim] * dPdU[nDim+1][b];
    } else {
      su2double dG[ES_NVAR][ES_NVAR], dGdU[ES_NVAR][ES_NVAR], dUdPg[ES_NVAR][ES_NVAR], Gam[ES_NVAR][ES_NVAR];
      su2double Ag[ES_NVAR][ES_NVAR], Bg[ES_NVAR][ES_NVAR], BgGam[ES_NVAR][ES_NVAR];
      EsBoundaryGhostJacobian(nDim, nNodes, gamma, *data, phi[q], sq, unitNormal, dG);
      EsMatMul(nVar, dG, dPdU, dGdU);
      EsConservativeJacobian(nDim, gamma, gq, dUdPg);
      EsMatMul(nVar, dUdPg, dGdU, Gam);
      EsIsmailRoeJacobian(nDim, gamma, sq, gq, pointNormal, Ag, Bg);
      EsMatMul(nVar, Bg, Gam, BgGam);

      /*--- lam and its gradient, the expressions of EsFaceResidual. ---*/
      su2double velNq = 0.0, velNg = 0.0;
      for (unsigned short iDim = 0; iDim < nDim; ++iDim) {
        velNq += sq.vel[iDim] * unitNormal[iDim];
        velNg += gq.vel[iDim] * unitNormal[iDim];
      }
      const su2double cQ = sqrt(gamma * sq.p / sq.rho);
      const su2double cG = sqrt(gamma * gq.p / gq.rho);
      const su2double lamQ = fabs(velNq) + cQ;
      const su2double lamG = fabs(velNg) + cG;
      const su2double lam = fmax(lamQ, lamG);
      su2double dLamP[ES_NVAR] = {0.0}, dLam[ES_NVAR] = {0.0};
      if (lamQ >= lamG) {
        su2double sign = 1.0;
        if (velNq < 0.0) sign = -1.0;
        dLamP[0] = -0.5 * cQ / sq.rho;
        dLamP[nDim+1] = 0.5 * cQ / sq.p;
        for (unsigned short iDim = 0; iDim < nDim; ++iDim) dLamP[iDim+1] = sign * unitNormal[iDim];
        for (unsigned short b = 0; b < nVar; ++b)
          for (unsigned short a = 0; a < nVar; ++a) dLam[b] += dLamP[a] * dPdU[a][b];
      } else {
        su2double sign = 1.0;
        if (velNg < 0.0) sign = -1.0;
        dLamP[0] = -0.5 * cG / gq.rho;
        dLamP[nDim+1] = 0.5 * cG / gq.p;
        for (unsigned short iDim = 0; iDim < nDim; ++iDim) dLamP[iDim+1] = sign * unitNormal[iDim];
        for (unsigned short b = 0; b < nVar; ++b)
          for (unsigned short a = 0; a < nVar; ++a) dLam[b] += dLamP[a] * dGdU[a][b];
      }

      su2double Uq[ES_NVAR] = {0.0}, Ug[ES_NVAR] = {0.0};
      EsConservative(nDim, gamma, sq, Uq);
      EsConservative(nDim, gamma, gq, Ug);
      for (unsigned short a = 0; a < nVar; ++a) {
        for (unsigned short b = 0; b < nVar; ++b) {
          const su2double delta = (a == b) ? 1.0 : 0.0;
          D[a][b] = Ag[a][b] + BgGam[a][b] - 0.5 * lam * pointArea * (Gam[a][b] - delta) -
                    0.5 * pointArea * (Ug[a] - Uq[a]) * dLam[b];
        }
      }
    }

    /*--- Assembly to the face node blocks. ---*/
    for (unsigned short i = 0; i < nNodes; ++i) {
      su2double E[ES_NVAR][ES_NVAR], K[ES_NVAR][ES_NVAR], KW[ES_NVAR][ES_NVAR];
      for (unsigned short a = 0; a < nVar; ++a)
        for (unsigned short b = 0; b < nVar; ++b) E[a][b] = A[i][a][b] - Abar[a][b] + D[a][b];
      EsMatMul(nVar, E, C, K);
      for (unsigned short j = 0; j < nNodes; ++j) {
        EsMatMul(nVar, K, W[j].v, KW);
        const su2double delta = (i == j) ? 1.0 : 0.0;
        const su2double coefK = phi[q][i] * phi[q][j];
        const su2double coefB = phi[q][i] * (delta - phi[q][j]);
        for (unsigned short a = 0; a < nVar; ++a)
          for (unsigned short b = 0; b < nVar; ++b) jac[i][j][a][b] += coefK * KW[a][b] + coefB * B[j][a][b];
      }
    }
  }
}

}  // namespace P1BoundaryClosure
