/*!
 * \file p1_viscous_entropy.hpp
 * \brief P1 Galerkin viscous operator in entropy variables on one simplex (VISC_P1_ENTROPY) and the weak
 *        no-slip wall closure built on it (BC_WALL_ENTROPY_P1).
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

#include "../../../../Common/include/basic_types/datatype_structure.hpp"
#include "../../../../Common/include/toolboxes/p1_simplex.hpp"
#include "../CNumerics.hpp"

/*--- Ideal gas, entropy U = -rho s/(gamma-1), entropy variables w = (w_0, theta v, -theta), theta = rho/p = 1/(R T).
 * Only w_m = theta v and w_E = -theta enter the viscous flux, its mass row is zero.
 *
 * Operator V-A (P1 viscous notes, section 3): on a simplex T with shape gradients g_a,
 *   G_theta = sum_a theta_a g_a,  G_m = sum_a theta_a v_a (x) g_a  (the gradient of w_h on T),
 *   frozen state = mean of the nodal entropy variables: theta* = mean theta_a, v* = sum theta_a v_a / sum theta_a,
 *   F_T = K(wbar_T) grad(w_h) = F^v(v*, mu_T, kappa_T; grad v_eff, grad T_eff)            (Lemma V5)
 *   grad v_eff = (G_m - v* (x) G_theta)/theta*,  grad T_eff = -G_theta/(R theta*^2),
 *   residual of node a (the convention of the edge scheme, added to LinSysRes): R_a = |T| F_T . g_a.
 * Then sum_a w_a . R_a = |T| grad(w_h) : K grad(w_h) >= 0 for any state (Theorem V6), so the operator dissipates
 * entropy. mu_T, kappa_T are the means of the nodal mu + mu_t and kappa + cp mu_t/Pr_t, mu_t clipped at 0. ---*/

namespace P1ViscousEntropy {

constexpr unsigned short MAXNODE = 4;
constexpr unsigned short MAXNVAR = 5;

/*!
 * \brief Nodal data of the operator: density, velocity, pressure, total viscosity and total conductivity.
 */
struct NodeState {
  su2double rho = 1.0, vel[3] = {0.0, 0.0, 0.0}, p = 1.0, mu = 0.0, kappa = 0.0;
};

/*!
 * \brief Gather the nodal data, mu = mu_lam + mu_t, kappa = kappa_lam + cp mu_t / Pr_t, with mu_t clipped at 0
 *        (K is positive semidefinite only for mu_t >= 0).
 */
template <class Nodes>
inline void GatherNode(const Nodes& nodes, unsigned long iPoint, unsigned short nDim, const su2double& prandtlTurb,
                       NodeState& S) {
  S.rho = nodes.GetDensity(iPoint);
  S.p = nodes.GetPressure(iPoint);
  for (unsigned short iDim = 0; iDim < nDim; ++iDim) S.vel[iDim] = nodes.GetVelocity(iPoint, iDim);
  su2double eddy = nodes.GetEddyViscosity(iPoint);
  if (eddy < 0.0) eddy = 0.0;
  S.mu = nodes.GetLaminarViscosity(iPoint) + eddy;
  S.kappa = nodes.GetThermalConductivity(iPoint) + nodes.GetSpecificHeatCp(iPoint) * eddy / prandtlTurb;
}

/*!
 * \brief dw/du at one node, rows w_m (1..nDim) and w_E (nDim+1); the w_0 row is not used and stays 0.
 *        dp/du = (gamma-1)(|v|^2/2, -v, 1), dw_m/du = e_m/p - (rho v/p^2) dp/du, dw_E/du = -e_rho/p + (rho/p^2) dp/du.
 */
inline void EntropyVariableJacobian(unsigned short nDim, const su2double& gamma, const NodeState& S,
                                    su2double W[][MAXNVAR]) {
  const unsigned short nVar = nDim + 2;
  su2double vel2 = 0.0, dpdu[MAXNVAR] = {0.0};
  for (unsigned short iDim = 0; iDim < nDim; ++iDim) vel2 += S.vel[iDim] * S.vel[iDim];
  dpdu[0] = 0.5 * (gamma - 1.0) * vel2;
  for (unsigned short iDim = 0; iDim < nDim; ++iDim) dpdu[iDim + 1] = -(gamma - 1.0) * S.vel[iDim];
  dpdu[nDim + 1] = gamma - 1.0;

  const su2double invP = 1.0 / S.p, rhoOnP2 = S.rho * invP * invP;
  for (unsigned short jVar = 0; jVar < nVar; ++jVar) {
    W[0][jVar] = 0.0;
    for (unsigned short iDim = 0; iDim < nDim; ++iDim) W[iDim + 1][jVar] = -rhoOnP2 * S.vel[iDim] * dpdu[jVar];
    W[nDim + 1][jVar] = rhoOnP2 * dpdu[jVar];
  }
  for (unsigned short iDim = 0; iDim < nDim; ++iDim) W[iDim + 1][iDim + 1] += invP;
  W[nDim + 1][0] -= invP;
}

/*!
 * \brief One simplex: shape gradients, nodal entropy variables, frozen state and the element flux F_T.
 */
class Element {
 public:
  unsigned short nDim = 2, nNodes = 3, nVar = 4;
  su2double size = 0.0;                         /*!< \brief |T|. */
  su2double grad[MAXNODE][3] = {{0.0}};         /*!< \brief Shape gradients g_a. */
  su2double thetaNode[MAXNODE] = {0.0};         /*!< \brief theta_a = rho_a/p_a = -w_E. */
  su2double wMom[MAXNODE][3] = {{0.0}};         /*!< \brief w_m = theta_a v_a. */
  su2double theta = 1.0, vel[3] = {0.0, 0.0, 0.0}, mu = 0.0, kappa = 0.0; /*!< \brief Frozen state. */
  su2double gasConst = 1.0;
  su2double flux[MAXNVAR][3] = {{0.0}};         /*!< \brief F_T, flux[iVar][iDim]. */

  /*!
   * \brief Set the element from the coordinates and the nodal data of its nDim+1 nodes.
   * \return |T|, zero for a degenerate simplex (then nothing else is valid).
   */
  template <class Coords>
  su2double Set(unsigned short nDim_, const Coords& coord, const NodeState* node, const su2double& gasConst_) {
    nDim = nDim_;
    nNodes = nDim + 1;
    nVar = nDim + 2;
    gasConst = gasConst_;
    size = fabs(P1Simplex::ShapeGradients(nDim, coord, grad));
    if (size == 0.0) return size;

    su2double sumTheta = 0.0, sumMom[3] = {0.0, 0.0, 0.0};
    mu = 0.0;
    kappa = 0.0;
    for (unsigned short a = 0; a < nNodes; ++a) {
      thetaNode[a] = node[a].rho / node[a].p;
      sumTheta += thetaNode[a];
      for (unsigned short iDim = 0; iDim < nDim; ++iDim) {
        wMom[a][iDim] = thetaNode[a] * node[a].vel[iDim];
        sumMom[iDim] += wMom[a][iDim];
      }
      mu += node[a].mu / nNodes;
      kappa += node[a].kappa / nNodes;
    }
    theta = sumTheta / nNodes;
    for (unsigned short iDim = 0; iDim < nDim; ++iDim) vel[iDim] = sumMom[iDim] / sumTheta;

    su2double gradTheta[3] = {0.0, 0.0, 0.0}, gradMom[3][3] = {{0.0}};
    for (unsigned short a = 0; a < nNodes; ++a) {
      for (unsigned short jDim = 0; jDim < nDim; ++jDim) {
        gradTheta[jDim] += thetaNode[a] * grad[a][jDim];
        for (unsigned short iDim = 0; iDim < nDim; ++iDim) gradMom[iDim][jDim] += wMom[a][iDim] * grad[a][jDim];
      }
    }
    Flux(gradTheta, gradMom, flux);
    return size;
  }

  /*!
   * \brief F = K(wbar_T) G for the entropy-variable gradient G = (G_theta = grad theta, G_m = grad(theta v)),
   *        with SU2's stress tensor of grad v_eff and the heat flux kappa grad T_eff (Lemma V5). Linear in G.
   */
  void Flux(const su2double* gradTheta, const su2double (*gradMom)[3], su2double F[][3]) const {
    su2double gradVel[3][3] = {{0.0}}, tau[3][3] = {{0.0}}, gradT[3] = {0.0, 0.0, 0.0};
    const su2double invTheta = 1.0 / theta;
    for (unsigned short iDim = 0; iDim < nDim; ++iDim) {
      gradT[iDim] = -gradTheta[iDim] * invTheta * invTheta / gasConst;
      for (unsigned short jDim = 0; jDim < nDim; ++jDim)
        gradVel[iDim][jDim] = (gradMom[iDim][jDim] - vel[iDim] * gradTheta[jDim]) * invTheta;
    }
    CNumerics::ComputeStressTensor(nDim, tau, gradVel, mu);

    for (unsigned short jDim = 0; jDim < nDim; ++jDim) {
      F[0][jDim] = 0.0;
      su2double work = 0.0;
      for (unsigned short iDim = 0; iDim < nDim; ++iDim) {
        F[iDim + 1][jDim] = tau[iDim][jDim];
        work += tau[jDim][iDim] * vel[iDim];
      }
      F[nDim + 1][jDim] = work + kappa * gradT[jDim];
    }
  }

  /*!
   * \brief Flux of the gradient field G = dw (x) y, i.e. d_j w = y_j dw (dw indexed like the conservative variables).
   */
  void FluxOuter(const su2double* dw, const su2double* y, su2double F[][3]) const {
    su2double gradTheta[3] = {0.0, 0.0, 0.0}, gradMom[3][3] = {{0.0}};
    for (unsigned short jDim = 0; jDim < nDim; ++jDim) {
      gradTheta[jDim] = -dw[nDim + 1] * y[jDim];
      for (unsigned short iDim = 0; iDim < nDim; ++iDim) gradMom[iDim][jDim] = dw[iDim + 1] * y[jDim];
    }
    Flux(gradTheta, gradMom, F);
  }

  /*!
   * \brief Residual of every node, R_a = |T| F_T . g_a (sign of the edge scheme, R_a = -sum_k F_v . n_ak).
   */
  void Residual(su2double res[][MAXNVAR]) const {
    for (unsigned short a = 0; a < nNodes; ++a) {
      for (unsigned short iVar = 0; iVar < nVar; ++iVar) {
        su2double sum = 0.0;
        for (unsigned short jDim = 0; jDim < nDim; ++jDim) sum += flux[iVar][jDim] * grad[a][jDim];
        res[a][iVar] = size * sum;
      }
    }
  }

  /*!
   * \brief Picard Jacobian (K frozen): dR_a/du_b = |T| sum_jk g_a,j K_jk g_b,k W_b, W_b = dw/du at node b.
   * \param[in] dwdu - dw/du of each node, from EntropyVariableJacobian.
   * \param[out] jac - jac[a][b][iVar][jVar].
   */
  void Jacobian(const su2double (*dwdu)[MAXNVAR][MAXNVAR], su2double (*jac)[MAXNODE][MAXNVAR][MAXNVAR]) const {
    for (unsigned short b = 0; b < nNodes; ++b) {
      for (unsigned short jVar = 0; jVar < nVar; ++jVar) {
        su2double dw[MAXNVAR] = {0.0}, F[MAXNVAR][3] = {{0.0}};
        for (unsigned short iVar = 0; iVar < nVar; ++iVar) dw[iVar] = dwdu[b][iVar][jVar];
        FluxOuter(dw, grad[b], F);
        for (unsigned short a = 0; a < nNodes; ++a) {
          for (unsigned short iVar = 0; iVar < nVar; ++iVar) {
            su2double sum = 0.0;
            for (unsigned short jDim = 0; jDim < nDim; ++jDim) sum += F[iVar][jDim] * grad[a][jDim];
            jac[a][b][iVar][jVar] = size * sum;
          }
        }
      }
    }
  }
};

/*!
 * \brief Weak no-slip wall on the face f of the element E (P1 no-slip notes, section 3.1), SIPG/IIPG/NIPG family.
 *
 * Wall data P w = g: P = momentum rows (adiabatic), momentum + energy rows (isothermal), g_E = -1/(R T_w).
 * With N_f = A_f nu (outward), e_a = P (w_a - g) at the face nodes, M = P K_nunu P of the frozen K of E,
 * h_f = d |T| / A_f and sigma_f = c (1+s)^2 d m_T / (4 h_f):
 *   face nodes a:      res_a -= (A_f/d) (P F_T nu - sigma_f M e_a)        (consistency + penalty)
 *   all nodes c of E:  res_c -= s F(nu (x) ebar_f) . g_c,  ebar_f = (A_f/d) sum_a e_a   (adjoint term)
 * Theorem N6: with sigma_f >= (1+s)^2 d m_T/(4 h_f) the interior operator plus this closure dissipate entropy
 * (exergy for isothermal walls), the bound depends only on the wall-normal height.
 *
 * \param[in] E - The wall element, set with Element::Set.
 * \param[in] nFaceNodes - nDim.
 * \param[in] faceNodes - Element-local indices of the face nodes.
 * \param[in] faceNormal - Outward area vector A_f nu.
 * \param[in] isothermal - Energy row prescribed (T = T_w) or free (adiabatic).
 * \param[in] wallTheta - 1/(R T_w), used if isothermal.
 * \param[in] nWallFaces - m_T, number of wall faces of E (>= 1).
 * \param[in] s - Adjoint factor, 1 SIPG.
 * \param[in] c - Penalty factor (> 1).
 * \param[out] res - res[c][iVar] for the element nodes, overwritten.
 * \param[in] dwdu - dw/du of the element nodes, or nullptr.
 * \param[out] jac - jac[a][b][iVar][jVar] (Picard: K and sigma frozen), overwritten if dwdu is given.
 */
inline void NoSlipWall(const Element& E, unsigned short nFaceNodes, const unsigned short* faceNodes,
                       const su2double* faceNormal, bool isothermal, const su2double& wallTheta,
                       unsigned short nWallFaces, const su2double& s, const su2double& c, su2double res[][MAXNVAR],
                       const su2double (*dwdu)[MAXNVAR][MAXNVAR] = nullptr,
                       su2double (*jac)[MAXNODE][MAXNVAR][MAXNVAR] = nullptr) {
  const unsigned short nDim = E.nDim, nVar = E.nVar, nNodes = E.nNodes;

  su2double area = 0.0;
  for (unsigned short iDim = 0; iDim < nDim; ++iDim) area += faceNormal[iDim] * faceNormal[iDim];
  area = sqrt(area);
  su2double nu[3] = {0.0, 0.0, 0.0};
  for (unsigned short iDim = 0; iDim < nDim; ++iDim) nu[iDim] = faceNormal[iDim] / area;

  const su2double weight = area / nDim;                        /*--- A_f/d, lumped face rule. ---*/
  const su2double sigma = c * (1.0 + s) * (1.0 + s) * nWallFaces * area / (4.0 * E.size);  /*--- c(1+s)^2 d m_T/(4 h_f). ---*/

  su2double mask[MAXNVAR] = {0.0};
  for (unsigned short iDim = 0; iDim < nDim; ++iDim) mask[iDim + 1] = 1.0;
  mask[nDim + 1] = isothermal ? 1.0 : 0.0;

  auto projectNormal = [&](const su2double (*F)[3], su2double* Fn) {
    for (unsigned short iVar = 0; iVar < nVar; ++iVar) {
      su2double sum = 0.0;
      for (unsigned short iDim = 0; iDim < nDim; ++iDim) sum += F[iVar][iDim] * nu[iDim];
      Fn[iVar] = mask[iVar] * sum;
    }
  };

  for (unsigned short a = 0; a < nNodes; ++a)
    for (unsigned short iVar = 0; iVar < nVar; ++iVar) res[a][iVar] = 0.0;

  /*--- Consistency term P F_T nu. ---*/
  su2double FTn[MAXNVAR] = {0.0};
  projectNormal(E.flux, FTn);

  su2double ebar[MAXNVAR] = {0.0};
  for (unsigned short k = 0; k < nFaceNodes; ++k) {
    const auto a = faceNodes[k];
    su2double e[MAXNVAR] = {0.0}, F[MAXNVAR][3] = {{0.0}}, Me[MAXNVAR] = {0.0};
    for (unsigned short iDim = 0; iDim < nDim; ++iDim) e[iDim + 1] = E.wMom[a][iDim];
    if (isothermal) e[nDim + 1] = wallTheta - E.thetaNode[a];
    for (unsigned short iVar = 0; iVar < nVar; ++iVar) ebar[iVar] += weight * e[iVar];

    E.FluxOuter(e, nu, F);
    projectNormal(F, Me);
    for (unsigned short iVar = 0; iVar < nVar; ++iVar) res[a][iVar] -= weight * (FTn[iVar] - sigma * Me[iVar]);
  }

  /*--- Adjoint term. ---*/
  if (s != 0.0) {
    su2double F[MAXNVAR][3] = {{0.0}};
    E.FluxOuter(ebar, nu, F);
    for (unsigned short a = 0; a < nNodes; ++a) {
      for (unsigned short iVar = 0; iVar < nVar; ++iVar) {
        su2double sum = 0.0;
        for (unsigned short iDim = 0; iDim < nDim; ++iDim) sum += F[iVar][iDim] * E.grad[a][iDim];
        res[a][iVar] -= s * sum;
      }
    }
  }

  if (dwdu == nullptr || jac == nullptr) return;

  for (unsigned short a = 0; a < nNodes; ++a)
    for (unsigned short b = 0; b < nNodes; ++b)
      for (unsigned short iVar = 0; iVar < nVar; ++iVar)
        for (unsigned short jVar = 0; jVar < nVar; ++jVar) jac[a][b][iVar][jVar] = 0.0;

  /*--- Consistency: d(P F_T nu)/du_b = P K(nu, g_b) W_b, for the face rows. ---*/
  for (unsigned short b = 0; b < nNodes; ++b) {
    for (unsigned short jVar = 0; jVar < nVar; ++jVar) {
      su2double dw[MAXNVAR] = {0.0}, F[MAXNVAR][3] = {{0.0}}, Fn[MAXNVAR] = {0.0};
      for (unsigned short iVar = 0; iVar < nVar; ++iVar) dw[iVar] = dwdu[b][iVar][jVar];
      E.FluxOuter(dw, E.grad[b], F);
      projectNormal(F, Fn);
      for (unsigned short k = 0; k < nFaceNodes; ++k)
        for (unsigned short iVar = 0; iVar < nVar; ++iVar) jac[faceNodes[k]][b][iVar][jVar] -= weight * Fn[iVar];
    }
  }

  /*--- Penalty (row a, column a) and adjoint (rows c, column a): de_a/du_a = P W_a. ---*/
  for (unsigned short k = 0; k < nFaceNodes; ++k) {
    const auto a = faceNodes[k];
    for (unsigned short jVar = 0; jVar < nVar; ++jVar) {
      su2double de[MAXNVAR] = {0.0}, F[MAXNVAR][3] = {{0.0}}, Fn[MAXNVAR] = {0.0};
      for (unsigned short iVar = 0; iVar < nVar; ++iVar) de[iVar] = mask[iVar] * dwdu[a][iVar][jVar];
      E.FluxOuter(de, nu, F);
      projectNormal(F, Fn);
      for (unsigned short iVar = 0; iVar < nVar; ++iVar) jac[a][a][iVar][jVar] += weight * sigma * Fn[iVar];
      if (s == 0.0) continue;
      for (unsigned short b = 0; b < nNodes; ++b) {
        for (unsigned short iVar = 0; iVar < nVar; ++iVar) {
          su2double sum = 0.0;
          for (unsigned short iDim = 0; iDim < nDim; ++iDim) sum += F[iVar][iDim] * E.grad[b][iDim];
          jac[b][a][iVar][jVar] -= s * weight * sum;
        }
      }
    }
  }
}

}  // namespace P1ViscousEntropy
