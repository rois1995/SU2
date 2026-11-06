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
#include "../../../../Common/include/toolboxes/geometry_toolbox.hpp"
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

/*--- Moved test space (VISC_P1_ENTROPY_MOVED, P1_VISCOUS_MOVED.md): the finite volume of the element fluxes on the moved
 * (MOD_CENTROIDS) dual of a tetrahedron is R_a = F_T . nu_a, nu_a = (|T| I + Y_T^T) g_a (Lemma MV1), and
 *   -sum_a w_a . R_a = -D^gal_T + P^v_T,  D^gal_T = |T| G:K G >= 0,  P^v_T = -(G Y_T):K G        (Lemma MV2),
 * which can be positive whenever Y_T != 0 (Theorem MV3). The element blend pays it in one budget with the convective
 * defect: E_T = P^c_T + P^v_T - D^gal_T (+ W_T, the production of the weak wall terms), theta_v = min(1, E_T/P^v_T)
 * first, theta_c = (E_T - theta_v P^v_T)^+ / D^c_T, test vectors (|T| I + (1 - theta_v) Y_T^T) g_a. Then theta_v,
 * theta_c are in [0,1] and the element production is min(E_T, 0) <= 0 (Theorem MV6, MV13). ---*/

namespace P1ViscousEntropy {

constexpr unsigned short MAXNODE = 4;
constexpr unsigned short MAXNVAR = 5;
constexpr double NOSLIP_SIPG = 1.0;     /*!< \brief s of the weak no-slip wall, SIPG. */
constexpr double NOSLIP_PENALTY = 2.5;  /*!< \brief c of the weak no-slip wall, the maximum of lambda(c)/c (P1_NOSLIP.md, 3.3). */

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
  bool moved = false;                           /*!< \brief Moved test vectors (SetMovedTest), otherwise |T| g_a. */
  su2double test[MAXNODE][3] = {{0.0}};         /*!< \brief Moved test vectors (|T| I + keep Y^T) g_a. */

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
    moved = false;
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
   * \brief Moved test vectors test_a = |T| g_a + keep Y^T g_a (3D), used by Residual and Jacobian from now on.
   * \param[in] Y - Y_T of the moved dual, row major Y[3 i + j] = Y_ij (CEdge::GetBlendY).
   * \param[in] keep - 1 - theta_v, the part of Y_T kept by the element blend.
   */
  void SetMovedTest(const su2double* Y, const su2double& keep) {
    moved = true;
    for (unsigned short a = 0; a < nNodes; ++a) {
      for (unsigned short jDim = 0; jDim < nDim; ++jDim) {
        su2double sum = 0.0;
        for (unsigned short iDim = 0; iDim < nDim; ++iDim) sum += Y[3 * iDim + jDim] * grad[a][iDim];
        test[a][jDim] = size * grad[a][jDim] + keep * sum;
      }
    }
  }

  /*!
   * \brief Gradient G = grad(w_h) of the entropy variables, rows like the conservative variables (mass row 0, the
   *        flux has no mass row).
   */
  void Gradient(su2double G[][3]) const {
    for (unsigned short iVar = 0; iVar < nVar; ++iVar)
      for (unsigned short jDim = 0; jDim < 3; ++jDim) G[iVar][jDim] = 0.0;
    for (unsigned short a = 0; a < nNodes; ++a) {
      for (unsigned short jDim = 0; jDim < nDim; ++jDim) {
        for (unsigned short iDim = 0; iDim < nDim; ++iDim) G[iDim + 1][jDim] += wMom[a][iDim] * grad[a][jDim];
        G[nDim + 1][jDim] -= thetaNode[a] * grad[a][jDim];
      }
    }
  }

  /*!
   * \brief D^gal_T = |T| G:F_T = |T| G:K G >= 0, the entropy dissipation of the Galerkin (V-A) operator.
   */
  su2double GalerkinDissipation() const {
    su2double G[MAXNVAR][3], sum = 0.0;
    Gradient(G);
    for (unsigned short iVar = 0; iVar < nVar; ++iVar)
      for (unsigned short jDim = 0; jDim < nDim; ++jDim) sum += G[iVar][jDim] * flux[iVar][jDim];
    return size * sum;
  }

  /*!
   * \brief P^v_T = -(G Y):F_T, the entropy production of the moved part Y^T g_a of the test vectors (Lemma MV2).
   * \param[in] Y - Row major Y_T, as SetMovedTest.
   */
  su2double MovedProduction(const su2double* Y) const {
    su2double G[MAXNVAR][3], sum = 0.0;
    Gradient(G);
    for (unsigned short iVar = 0; iVar < nVar; ++iVar) {
      for (unsigned short jDim = 0; jDim < nDim; ++jDim) {
        su2double GY = 0.0;
        for (unsigned short kDim = 0; kDim < nDim; ++kDim) GY += G[iVar][kDim] * Y[3 * kDim + jDim];
        sum += GY * flux[iVar][jDim];
      }
    }
    return -sum;
  }

  /*!
   * \brief Residual of every node, R_a = |T| F_T . g_a (sign of the edge scheme, R_a = -sum_k F_v . n_ak),
   *        or R_a = F_T . test_a with moved test vectors.
   */
  void Residual(su2double res[][MAXNVAR]) const {
    for (unsigned short a = 0; a < nNodes; ++a) {
      for (unsigned short iVar = 0; iVar < nVar; ++iVar) {
        su2double sum = 0.0;
        if (moved) {
          for (unsigned short jDim = 0; jDim < nDim; ++jDim) sum += flux[iVar][jDim] * test[a][jDim];
          res[a][iVar] = sum;
          continue;
        }
        for (unsigned short jDim = 0; jDim < nDim; ++jDim) sum += flux[iVar][jDim] * grad[a][jDim];
        res[a][iVar] = size * sum;
      }
    }
  }

  /*!
   * \brief Picard Jacobian (K frozen): dR_a/du_b = |T| sum_jk g_a,j K_jk g_b,k W_b, W_b = dw/du at node b. With moved
   *        test vectors sum_jk test_a,j K_jk g_b,k W_b (not symmetric, theta_v frozen).
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
            if (moved) {
              for (unsigned short jDim = 0; jDim < nDim; ++jDim) sum += F[iVar][jDim] * test[a][jDim];
              jac[a][b][iVar][jVar] = sum;
              continue;
            }
            for (unsigned short jDim = 0; jDim < nDim; ++jDim) sum += F[iVar][jDim] * grad[a][jDim];
            jac[a][b][iVar][jVar] = size * sum;
          }
        }
      }
    }
  }

  /*!
   * \brief VISC_P1_TIME_STEP: Lambda_b of Lemma T1 (P1_TIME_STEP.md), the bound sup_x x.K(t,t)x / (|t|^2 x.H_b x) of
   *        the frozen K of this element against the entropy Hessian H_b = du/dw of node b.
   * \param[in] S - Nodal data of node b, as given to Set.
   * \param[in] gamma - Ratio of specific heats.
   * \param[in] full - false: momentum rows only (projector of the adiabatic wall at rest), then delta = v_b.
   */
  su2double SpectralFactor(const NodeState& S, const su2double& gamma, bool full = true) const;

  /*!
   * \brief VISC_P1_TIME_STEP: add the block row bounds of Jacobian (Theorem T3 of P1_TIME_STEP.md),
   *        rows[a] += |t_a| sqrt(Lambda_a) sum_b |g_b| sqrt(Lambda_b), t_a = |T| g_a or the moved test vector.
   * \param[in] node - Nodal data given to Set.
   * \param[in] gamma - Ratio of specific heats.
   * \param[in,out] rows - Row bounds of the element nodes, added to.
   */
  void RowBound(const NodeState* node, const su2double& gamma, su2double* rows) const;
};

/*!
 * \brief VISC_P1_TIME_STEP: largest eigenvalue of L S L^T (Lemma T1 of P1_TIME_STEP.md),
 *        (a + b)/2 + sqrt(((a - b)/2)^2 + c^2), a = alphaM (1/p + beta |delta|^2), b = alphaE beta,
 *        c^2 = alphaM alphaE beta^2 |delta|^2, beta = (gamma - 1) rho/p^2 at the node.
 */
inline su2double SpectralPencil(const su2double& alphaM, const su2double& alphaE, const su2double& delta2,
                                const NodeState& S, const su2double& gamma) {
  const su2double invP = 1.0 / S.p, beta = (gamma - 1.0) * S.rho * invP * invP;
  const su2double a = alphaM * (invP + beta * delta2), b = alphaE * beta;
  const su2double half = 0.5 * (a - b), disc = half * half + alphaM * alphaE * beta * beta * delta2;
  su2double root = 0.0;
  if (disc > 0.0) root = sqrt(disc);
  return 0.5 * (a + b) + root;
}

inline su2double Element::SpectralFactor(const NodeState& S, const su2double& gamma, bool full) const {
  const su2double alphaM = 4.0 * mu / (3.0 * theta);
  const su2double alphaE = full ? su2double(kappa / (gasConst * theta * theta)) : su2double(0.0);
  su2double delta2 = 0.0;
  for (unsigned short iDim = 0; iDim < nDim; ++iDim) {
    const su2double delta = full ? su2double(S.vel[iDim] - vel[iDim]) : S.vel[iDim];
    delta2 += delta * delta;
  }
  return SpectralPencil(alphaM, alphaE, delta2, S, gamma);
}

inline void Element::RowBound(const NodeState* node, const su2double& gamma, su2double* rows) const {
  su2double rootLambda[MAXNODE] = {0.0}, sum = 0.0;
  for (unsigned short a = 0; a < nNodes; ++a) {
    rootLambda[a] = sqrt(SpectralFactor(node[a], gamma));
    sum += GeometryToolbox::Norm(nDim, grad[a]) * rootLambda[a];
  }
  for (unsigned short a = 0; a < nNodes; ++a) {
    const su2double testNorm =
        moved ? GeometryToolbox::Norm(nDim, test[a]) : su2double(size * GeometryToolbox::Norm(nDim, grad[a]));
    rows[a] += testNorm * rootLambda[a] * sum;
  }
}

/*!
 * \brief Projector of the moving adiabatic no-slip wall (P1_ALE.md, Lemma A17, Theorem A18): v = v_w is the linear,
 *        solution independent condition C w = w_m + w_E v_w = 0, C = [0 | I | v_w], and
 *        P_C x = C^T (C C^T)^-1 C x with (C C^T)^-1 = I - v_w v_w^T / (1 + |v_w|^2): with y = x_m + x_E v_w and
 *        z = y - v_w (v_w.y)/(1 + |v_w|^2), P_C x = (0, z, v_w.z). Symmetric and idempotent; v_w = 0 gives the
 *        momentum rows of the static wall.
 */
inline void MovingWallProjector(unsigned short nDim, const su2double* wallVel, const su2double* x, su2double* Px) {
  su2double y[3] = {0.0, 0.0, 0.0}, vel2 = 0.0, velY = 0.0;
  for (unsigned short iDim = 0; iDim < nDim; ++iDim) {
    y[iDim] = x[iDim + 1] + wallVel[iDim] * x[nDim + 1];
    vel2 += wallVel[iDim] * wallVel[iDim];
    velY += wallVel[iDim] * y[iDim];
  }
  const su2double factor = velY / (1.0 + vel2);
  su2double velZ = 0.0;
  Px[0] = 0.0;
  for (unsigned short iDim = 0; iDim < nDim; ++iDim) {
    Px[iDim + 1] = y[iDim] - wallVel[iDim] * factor;
    velZ += wallVel[iDim] * Px[iDim + 1];
  }
  Px[nDim + 1] = velZ;
}

/*!
 * \brief Projector of the viscous symmetry plane (P1_NOSLIP.md, 11.3): u.n = 0 is nu.w_m = 0, the constant symmetric
 *        projector P_s = blockdiag(0, nu nu^T, 0), P_s x = (0, (nu.x_m) nu, 0), g = 0. The tangential traction and the
 *        energy flux stay natural.
 */
inline void SymmetryProjector(unsigned short nDim, const su2double* nu, const su2double* x, su2double* Px) {
  su2double normal = 0.0;
  for (unsigned short iDim = 0; iDim < nDim; ++iDim) normal += nu[iDim] * x[iDim + 1];
  Px[0] = 0.0;
  for (unsigned short iDim = 0; iDim < nDim; ++iDim) Px[iDim + 1] = normal * nu[iDim];
  Px[nDim + 1] = 0.0;
}

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
 * \param[in] shareFrac - Moved wall shares a_a/A_f of the face nodes (VISC_P1_ENTROPY_MOVED, Lemma MV11), in the
 *            order of faceNodes; nullptr for the lumped rule 1/d. They replace A_f/d in all three terms.
 * \param[in] wallVel - Wall velocities v_w of the face nodes, in the order of faceNodes (GRID_ENTROPY_P1, P1_ALE.md,
 *            Theorem A18), nullptr for a wall at rest. Adiabatic: P_a = MovingWallProjector(v_w,a), g = 0, so
 *            e_a = P_a w_a = (0, z, v_w.z) with z from y = theta_a (v_a - v_w); the consistency term P_a F_T nu then
 *            has the traction work of the wall in the energy row. Isothermal: P unchanged, g = (0, theta_w v_w, -theta_w).
 * \param[out] faceFlux - P1_WALL_FORCES: faceFlux[k][iVar] = -(weight_k) (P F_T nu - sigma_f M e_k), the consistency and
 *            penalty terms of face node faceNodes[k] with the weight of the face rule or of the shares, without the
 *            adjoint term (it sums to zero over the element, sum_c g_c = 0). Its momentum rows are the force of the wall
 *            traction on the body, the energy row the heat flow into the wall. nullptr if not needed.
 * \param[in] symmetry - Viscous symmetry plane (P1_NOSLIP.md, 11.3): P = SymmetryProjector (u.n = 0 only), g = 0, the
 *            same terms and sigma_f (Theorem S1); isothermal and wallVel are ignored.
 */
inline void NoSlipWall(const Element& E, unsigned short nFaceNodes, const unsigned short* faceNodes,
                       const su2double* faceNormal, bool isothermal, const su2double& wallTheta,
                       unsigned short nWallFaces, const su2double& s, const su2double& c, su2double res[][MAXNVAR],
                       const su2double (*dwdu)[MAXNVAR][MAXNVAR] = nullptr,
                       su2double (*jac)[MAXNODE][MAXNVAR][MAXNVAR] = nullptr, const su2double* shareFrac = nullptr,
                       const su2double (*wallVel)[3] = nullptr, su2double (*faceFlux)[MAXNVAR] = nullptr,
                       bool symmetry = false) {
  const unsigned short nDim = E.nDim, nVar = E.nVar, nNodes = E.nNodes;

  su2double area = 0.0;
  for (unsigned short iDim = 0; iDim < nDim; ++iDim) area += faceNormal[iDim] * faceNormal[iDim];
  area = sqrt(area);
  su2double nu[3] = {0.0, 0.0, 0.0};
  for (unsigned short iDim = 0; iDim < nDim; ++iDim) nu[iDim] = faceNormal[iDim] / area;

  /*--- A_f/d, lumped face rule, or the moved shares a_a. ---*/
  su2double weight[3] = {0.0, 0.0, 0.0};
  for (unsigned short k = 0; k < nFaceNodes; ++k) {
    if (shareFrac != nullptr) weight[k] = area * shareFrac[k];
    else weight[k] = area / nDim;
  }
  const su2double sigma = c * (1.0 + s) * (1.0 + s) * nWallFaces * area / (4.0 * E.size);  /*--- c(1+s)^2 d m_T/(4 h_f). ---*/

  if (symmetry) {
    isothermal = false;
    wallVel = nullptr;
  }

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

  /*--- Moving adiabatic wall: P_a of face node k instead of the mask (the isothermal P does not move). Symmetry plane:
   * P_s of the face. ---*/
  const bool movingProjector = (wallVel != nullptr) && !isothermal;
  const bool fullProjector = movingProjector || symmetry;
  auto applyProjector = [&](unsigned short k, const su2double* x, su2double* Px) {
    if (symmetry) SymmetryProjector(nDim, nu, x, Px);
    else MovingWallProjector(nDim, wallVel[k], x, Px);
  };
  auto projectNormalAt = [&](unsigned short k, const su2double (*F)[3], su2double* Fn) {
    if (!fullProjector) return projectNormal(F, Fn);
    su2double raw[MAXNVAR] = {0.0};
    for (unsigned short iVar = 0; iVar < nVar; ++iVar)
      for (unsigned short iDim = 0; iDim < nDim; ++iDim) raw[iVar] += F[iVar][iDim] * nu[iDim];
    applyProjector(k, raw, Fn);
  };

  /*--- e_k = P_k (w_a - g_k) at the face node a = faceNodes[k]. ---*/
  auto wallError = [&](unsigned short k, su2double* e) {
    const auto a = faceNodes[k];
    for (unsigned short iVar = 0; iVar < nVar; ++iVar) e[iVar] = 0.0;
    if (fullProjector) {
      su2double w[MAXNVAR] = {0.0};
      for (unsigned short iDim = 0; iDim < nDim; ++iDim) w[iDim + 1] = E.wMom[a][iDim];
      w[nDim + 1] = -E.thetaNode[a];
      applyProjector(k, w, e);
      return;
    }
    for (unsigned short iDim = 0; iDim < nDim; ++iDim) e[iDim + 1] = E.wMom[a][iDim];
    if (wallVel != nullptr)
      for (unsigned short iDim = 0; iDim < nDim; ++iDim) e[iDim + 1] -= wallTheta * wallVel[k][iDim];
    if (isothermal) e[nDim + 1] = wallTheta - E.thetaNode[a];
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
    wallError(k, e);
    if (fullProjector) projectNormalAt(k, E.flux, FTn);
    for (unsigned short iVar = 0; iVar < nVar; ++iVar) ebar[iVar] += weight[k] * e[iVar];

    E.FluxOuter(e, nu, F);
    projectNormalAt(k, F, Me);
    for (unsigned short iVar = 0; iVar < nVar; ++iVar) res[a][iVar] -= weight[k] * (FTn[iVar] - sigma * Me[iVar]);
    if (faceFlux != nullptr)
      for (unsigned short iVar = 0; iVar < nVar; ++iVar) faceFlux[k][iVar] = -weight[k] * (FTn[iVar] - sigma * Me[iVar]);
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
      for (unsigned short k = 0; k < nFaceNodes; ++k) {
        if (fullProjector) projectNormalAt(k, F, Fn);
        for (unsigned short iVar = 0; iVar < nVar; ++iVar) jac[faceNodes[k]][b][iVar][jVar] -= weight[k] * Fn[iVar];
      }
    }
  }

  /*--- Penalty (row a, column a) and adjoint (rows c, column a): de_a/du_a = P W_a (g does not depend on u). ---*/
  for (unsigned short k = 0; k < nFaceNodes; ++k) {
    const auto a = faceNodes[k];
    for (unsigned short jVar = 0; jVar < nVar; ++jVar) {
      su2double de[MAXNVAR] = {0.0}, F[MAXNVAR][3] = {{0.0}}, Fn[MAXNVAR] = {0.0};
      if (fullProjector) {
        su2double dw[MAXNVAR] = {0.0};
        for (unsigned short iVar = 0; iVar < nVar; ++iVar) dw[iVar] = dwdu[a][iVar][jVar];
        applyProjector(k, dw, de);
      } else {
        for (unsigned short iVar = 0; iVar < nVar; ++iVar) de[iVar] = mask[iVar] * dwdu[a][iVar][jVar];
      }
      E.FluxOuter(de, nu, F);
      projectNormalAt(k, F, Fn);
      for (unsigned short iVar = 0; iVar < nVar; ++iVar) jac[a][a][iVar][jVar] += weight[k] * sigma * Fn[iVar];
      if (s == 0.0) continue;
      for (unsigned short b = 0; b < nNodes; ++b) {
        for (unsigned short iVar = 0; iVar < nVar; ++iVar) {
          su2double sum = 0.0;
          for (unsigned short iDim = 0; iDim < nDim; ++iDim) sum += F[iVar][iDim] * E.grad[b][iDim];
          jac[b][a][iVar][jVar] -= s * weight[k] * sum;
        }
      }
    }
  }
}

/*!
 * \brief Exact entropy production of the viscous wall terms of NoSlipWall on one face, without the terms of the wall
 *        data g (physical wall entropy flux, Corollary N7): W_f = (1 + s) sum_a a_a e_a . P F_T nu - sigma_f X_f,
 *        X_f = sum_a a_a e_a . M e_a (Theorem MV13, with a_a = A_f/d for shareFrac = nullptr). W_f <= D^gal_T / c.
 *        For an adiabatic wall at rest and for a symmetry plane it is -sum_c w_c . res_c of NoSlipWall. Same arguments
 *        as NoSlipWall.
 */
inline su2double NoSlipWallProduction(const Element& E, unsigned short nFaceNodes, const unsigned short* faceNodes,
                                      const su2double* faceNormal, bool isothermal, const su2double& wallTheta,
                                      unsigned short nWallFaces, const su2double& s, const su2double& c,
                                      const su2double* shareFrac = nullptr, const su2double (*wallVel)[3] = nullptr,
                                      bool symmetry = false) {
  const unsigned short nDim = E.nDim, nVar = E.nVar;

  su2double area = 0.0;
  for (unsigned short iDim = 0; iDim < nDim; ++iDim) area += faceNormal[iDim] * faceNormal[iDim];
  area = sqrt(area);
  su2double nu[3] = {0.0, 0.0, 0.0};
  for (unsigned short iDim = 0; iDim < nDim; ++iDim) nu[iDim] = faceNormal[iDim] / area;
  const su2double sigma = c * (1.0 + s) * (1.0 + s) * nWallFaces * area / (4.0 * E.size);

  if (symmetry) {
    isothermal = false;
    wallVel = nullptr;
  }

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

  su2double FTn[MAXNVAR] = {0.0};
  projectNormal(E.flux, FTn);

  /*--- Moving walls as in NoSlipWall. With e_a = P_a e_a, e_a.P_a F_T nu = e_a.F_T nu. ---*/
  const bool movingProjector = (wallVel != nullptr) && !isothermal;

  su2double production = 0.0;
  for (unsigned short k = 0; k < nFaceNodes; ++k) {
    const auto a = faceNodes[k];
    su2double weight = area / nDim;
    if (shareFrac != nullptr) weight = area * shareFrac[k];
    su2double e[MAXNVAR] = {0.0}, F[MAXNVAR][3] = {{0.0}}, Me[MAXNVAR] = {0.0};
    if (symmetry) {
      su2double w[MAXNVAR] = {0.0};
      for (unsigned short iDim = 0; iDim < nDim; ++iDim) w[iDim + 1] = E.wMom[a][iDim];
      SymmetryProjector(nDim, nu, w, e);
      su2double raw[MAXNVAR] = {0.0};
      for (unsigned short iVar = 0; iVar < nVar; ++iVar)
        for (unsigned short iDim = 0; iDim < nDim; ++iDim) raw[iVar] += E.flux[iVar][iDim] * nu[iDim];
      SymmetryProjector(nDim, nu, raw, FTn);
    } else if (movingProjector) {
      su2double w[MAXNVAR] = {0.0};
      for (unsigned short iDim = 0; iDim < nDim; ++iDim) w[iDim + 1] = E.wMom[a][iDim];
      w[nDim + 1] = -E.thetaNode[a];
      MovingWallProjector(nDim, wallVel[k], w, e);
      su2double raw[MAXNVAR] = {0.0};
      for (unsigned short iVar = 0; iVar < nVar; ++iVar)
        for (unsigned short iDim = 0; iDim < nDim; ++iDim) raw[iVar] += E.flux[iVar][iDim] * nu[iDim];
      MovingWallProjector(nDim, wallVel[k], raw, FTn);
    } else {
      for (unsigned short iDim = 0; iDim < nDim; ++iDim) e[iDim + 1] = E.wMom[a][iDim];
      if (wallVel != nullptr)
        for (unsigned short iDim = 0; iDim < nDim; ++iDim) e[iDim + 1] -= wallTheta * wallVel[k][iDim];
      if (isothermal) e[nDim + 1] = wallTheta - E.thetaNode[a];
    }
    E.FluxOuter(e, nu, F);
    if (symmetry || movingProjector) {
      su2double raw[MAXNVAR] = {0.0};
      for (unsigned short iVar = 0; iVar < nVar; ++iVar)
        for (unsigned short iDim = 0; iDim < nDim; ++iDim) raw[iVar] += F[iVar][iDim] * nu[iDim];
      if (symmetry) SymmetryProjector(nDim, nu, raw, Me);
      else MovingWallProjector(nDim, wallVel[k], raw, Me);
    } else {
      projectNormal(F, Me);
    }
    su2double eFTn = 0.0, eMe = 0.0;
    for (unsigned short iVar = 0; iVar < nVar; ++iVar) {
      eFTn += e[iVar] * FTn[iVar];
      eMe += e[iVar] * Me[iVar];
    }
    production += weight * ((1.0 + s) * eFTn - sigma * eMe);
  }
  return production;
}

/*!
 * \brief VISC_P1_TIME_STEP, moving adiabatic wall: bound of lambda_max((L P) S (L P)^T) by its Frobenius norm
 *        (P1_TIME_STEP.md, Lemma T2), P = MovingWallProjector(v_w), rows of L: sqrt(alphaM) (0, e_i, v*_i) and
 *        sqrt(alphaE) (0, 0, 1), S = dw/du at the node.
 */
inline su2double MovingWallSpectralFactor(const Element& E, const NodeState& S, const su2double& gamma,
                                          const su2double* wallVel) {
  const unsigned short nDim = E.nDim, nVar = E.nVar;
  const su2double rootM = sqrt(4.0 * E.mu / (3.0 * E.theta));
  const su2double rootE = sqrt(E.kappa / (E.gasConst * E.theta * E.theta));
  su2double W[MAXNVAR][MAXNVAR], rows[MAXNODE][MAXNVAR];
  EntropyVariableJacobian(nDim, gamma, S, W);
  for (unsigned short r = 0; r <= nDim; ++r) {
    su2double x[MAXNVAR] = {0.0};
    if (r < nDim) {
      x[r + 1] = rootM;
      x[nDim + 1] = rootM * E.vel[r];
    } else {
      x[nDim + 1] = rootE;
    }
    MovingWallProjector(nDim, wallVel, x, rows[r]);
  }
  /*--- The rows have no mass entry, so the w_0 row of W (zero in EntropyVariableJacobian) is not needed. ---*/
  su2double frobenius = 0.0;
  for (unsigned short i = 0; i <= nDim; ++i) {
    for (unsigned short j = 0; j <= nDim; ++j) {
      su2double entry = 0.0;
      for (unsigned short p = 1; p < nVar; ++p)
        for (unsigned short q = 1; q < nVar; ++q) entry += rows[i][p] * W[p][q] * rows[j][q];
      frobenius += entry * entry;
    }
  }
  su2double bound = 0.0;
  if (frobenius > 0.0) bound = sqrt(frobenius);
  return bound;
}

/*!
 * \brief VISC_P1_TIME_STEP: block row bounds of the Picard Jacobian of NoSlipWall on one face (P1_TIME_STEP.md,
 *        Theorem T3), with r_b = |g_b| sqrt(Lambda_b) and pi_k the bound of the projected normal block P K_nunu P at
 *        face node a_k (adiabatic at rest: momentum rows; isothermal: Lambda; moving adiabatic: Lemma T2):
 *          face node a_k:     rows += weight_k (sqrt(pi_k) sum_b r_b + sigma_f pi_k)     (consistency, penalty)
 *          every node c of E: rows += |s| r_c sum_k weight_k sqrt(pi_k)                   (adjoint term)
 *        weight_k = A_f/d or the moved shares, sigma_f as in NoSlipWall.
 * \param[in] node - Nodal data of the element nodes, as given to Element::Set.
 * \param[in] gamma - Ratio of specific heats.
 * \param[in,out] rows - Row bounds of the element nodes, added to.
 * \note The other arguments are those of NoSlipWall.
 */
inline void NoSlipWallRowBound(const Element& E, const NodeState* node, const su2double& gamma,
                               unsigned short nFaceNodes, const unsigned short* faceNodes, const su2double* faceNormal,
                               bool isothermal, unsigned short nWallFaces, const su2double& s, const su2double& c,
                               su2double* rows, const su2double* shareFrac = nullptr,
                               const su2double (*wallVel)[3] = nullptr) {
  const unsigned short nDim = E.nDim, nNodes = E.nNodes;
  const su2double area = GeometryToolbox::Norm(nDim, faceNormal);
  const su2double sigma = c * (1.0 + s) * (1.0 + s) * nWallFaces * area / (4.0 * E.size);

  su2double rootGrad[MAXNODE] = {0.0}, sum = 0.0;
  for (unsigned short b = 0; b < nNodes; ++b) {
    rootGrad[b] = GeometryToolbox::Norm(nDim, E.grad[b]) * sqrt(E.SpectralFactor(node[b], gamma));
    sum += rootGrad[b];
  }

  const bool movingProjector = (wallVel != nullptr) && !isothermal;
  su2double adjoint = 0.0;
  for (unsigned short k = 0; k < nFaceNodes; ++k) {
    const auto a = faceNodes[k];
    const su2double weight = (shareFrac != nullptr) ? su2double(area * shareFrac[k]) : su2double(area / nDim);
    const su2double pi = movingProjector ? MovingWallSpectralFactor(E, node[a], gamma, wallVel[k])
                                         : E.SpectralFactor(node[a], gamma, isothermal);
    su2double rootPi = 0.0;
    if (pi > 0.0) rootPi = sqrt(pi);
    rows[a] += weight * (rootPi * sum + sigma * pi);
    adjoint += weight * rootPi;
  }
  for (unsigned short b = 0; b < nNodes; ++b) rows[b] += fabs(s) * adjoint * rootGrad[b];
}

/*!
 * \brief Outward area vector A_f nu of the face of a simplex opposite node "opposite" (pointing away from it).
 * \param[in] coord - Coordinates of the simplex nodes.
 * \param[in] faceNodes - Element-local indices of the face nodes (the other nodes).
 */
template <class Coords>
inline void WallFaceNormal(unsigned short nDim, const Coords& coord, unsigned short opposite,
                           const unsigned short* faceNodes, su2double* faceNormal) {
  su2double orientation = 0.0;
  for (unsigned short iDim = 0; iDim < 3; ++iDim) faceNormal[iDim] = 0.0;
  const auto* x0 = coord[faceNodes[0]];
  const auto* x1 = coord[faceNodes[1]];
  if (nDim == 2) {
    faceNormal[0] = x1[1] - x0[1];
    faceNormal[1] = -(x1[0] - x0[0]);
  } else {
    const auto* x2 = coord[faceNodes[2]];
    const su2double a[3] = {x1[0] - x0[0], x1[1] - x0[1], x1[2] - x0[2]};
    const su2double b[3] = {x2[0] - x0[0], x2[1] - x0[1], x2[2] - x0[2]};
    GeometryToolbox::CrossProduct(a, b, faceNormal);
    for (unsigned short iDim = 0; iDim < nDim; ++iDim) faceNormal[iDim] *= 0.5;
  }
  for (unsigned short iDim = 0; iDim < nDim; ++iDim)
    orientation += faceNormal[iDim] * (x0[iDim] - coord[opposite][iDim]);
  if (orientation < 0.0)
    for (unsigned short iDim = 0; iDim < nDim; ++iDim) faceNormal[iDim] = -faceNormal[iDim];
}

/*!
 * \brief One-budget element blend of VISC_P1_ENTROPY_MOVED (P1_VISCOUS_MOVED.md, Definition MV5, Theorems MV6, MV13):
 *        E = P^c + P^v - D^v, theta_v = min(1, E/P^v) if E > 0 and P^v > 0 (else 0), theta_c = (E - theta_v P^v)^+/D^c.
 *        Both are in [0,1]; with P^c <= D^c (Lemma 3) and W_T <= D^gal (MV12) the element production
 *        P^c - theta_c D^c + (1 - theta_v) P^v - D^v is min(E, 0) <= 0.
 * \param[in] convProduction - P^c, production of the E part of the element.
 * \param[in] convDissipation - D^c >= 0, dissipation of the Lemma 3 floor of the element.
 * \param[in] viscProduction - P^v, production of the moved part of the viscous test vectors.
 * \param[in] viscDissipation - D^v = D^gal - W_T, Galerkin viscous dissipation minus the weak wall production.
 * \param[out] thetaV - Part of Y_T removed from the viscous test vectors.
 * \param[out] thetaC - Factor of the Lemma 3 floor.
 */
inline void OneBudgetBlend(const su2double& convProduction, const su2double& convDissipation,
                           const su2double& viscProduction, const su2double& viscDissipation, su2double& thetaV,
                           su2double& thetaC) {
  const su2double excess = convProduction + viscProduction - viscDissipation;
  thetaV = 0.0;
  thetaC = 0.0;
  if (excess <= 0.0) return;

  /*--- The V-A geometry pays first: consistent, no artificial dissipation. ---*/
  if ((viscProduction > 0.0) && (excess <= viscProduction)) {
    thetaV = excess / viscProduction;
    return;
  }
  if (viscProduction > 0.0) thetaV = 1.0;

  /*--- Rest <= P^c - D^gal + W_T <= P^c <= D^c, min(.,1) only guards round-off. ---*/
  const su2double rest = excess - thetaV * viscProduction;
  if ((rest > 0.0) && (convDissipation > 0.0)) {
    const su2double ratio = rest / convDissipation;
    thetaC = (ratio < 1.0) ? ratio : su2double(1.0);
  }
}

}  // namespace P1ViscousEntropy
