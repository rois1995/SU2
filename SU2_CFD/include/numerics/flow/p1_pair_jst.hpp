/*!
 * \file p1_pair_jst.hpp
 * \brief EDGE_FORM_P1_MIXED_DISS: the scalar JST dissipation of the true edges (CJSTScheme::finalizeFlux) on the split
 *        pairs of EDGE_FORM_P1_MIXED, the face and body diagonals of non-simplex cells.
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
#include "../../../../Common/include/basic_types/datatype_structure.hpp"

/*--- Derivation (MIXED_DIAG_SOURCE_CHECK.md, section 3.3): every pair of the central stencil gets the JST term
 *     d_ik = lambda_ik [eps2 (U_i - U_k) - eps4 (L_i - L_k)],  lambda_ik = |ubar.n_ik| + cbar |n_ik|,
 * on a virtual pair with n_ik its assembled split-average coefficient (SplitNormal). Everything else is read as on the
 * true edges: the arithmetic means ubar, cbar, the stretching correction with the nodal spectral radii of the true edges,
 * the nodal pressure sensor, eps2 and eps4 with the true-edge neighbour counts, the undivided Laplacians of the true
 * edges, the dissipated variables (rho, rho u, rho H; hat states with matched k) and SU2's surrogate Jacobian. With
 * eps2 = 1/2 every pair is a Rusanov pair (pair positivity), and the eps2 part is entropy dissipative for any lambda >= 0.
 * The functions below are the scalar transcription of CJSTScheme::finalizeFlux and its helpers in
 * numerics_simd/flow/convection/common.hpp, in the same order of operations. ---*/
/*--- EDGE_FORM_P1_MIXED_DISS_ALIGNED, variant (d) (ONE_GEOMETRY_DISS.md, section 5): lambda_ik on the edge projection
 *     nu_ik = (n_ik.e_ik / |e_ik|^2) e_ik,  e_ik = x_k - x_i,
 * instead of n_ik; everything else as above, the central flux keeps n_ik. Reason (P1_OPTIMAL_DUAL.md, Lemma G4, the D1
 * rule of the true edges): with lambda = s|nu| the dissipation tensor is K = (1/4V) sum_k lambda_ik e_ik e_ik^T, and a
 * normal at angle theta to its pair inflates the term by sec(theta). The face diagonals of BL prisms have theta ~ 90 deg
 * (n_ik wall normal, e_ik along the wall, sec(theta) ~ L/h), so lambda(n_ik) inflates the dissipation along the wall.
 * With nu_ik the term is that of the projection, n.e is kept: nu.e = n.e. The eps2 part stays entropy dissipative
 * (any lambda >= 0); pair positivity (lambda >= rho(A(n_ik))) is given up, as on BL tetrahedra. A pair with n.e = 0 has
 * nu = 0, lambda = 0 and no dissipation. ---*/
namespace P1PairJST {

constexpr unsigned short MAX_NVAR = 5;
constexpr su2double STRETCH_PARAM = 0.3; /*!< \brief CCenteredBase::stretchParam. */

/*!
 * \brief Nodal data of one end of a pair, as the edge kernel reads them.
 */
struct Node {
  su2double vel[3] = {0.0, 0.0, 0.0}; /*!< \brief Velocity. */
  su2double soundSpeed = 0.0;         /*!< \brief Speed of sound. */
  su2double diss[MAX_NVAR] = {};      /*!< \brief Dissipated variables: rho, rho u, rho H (rho Hhat with matched k). */
  su2double lapl[MAX_NVAR] = {};      /*!< \brief Undivided Laplacian of the same variables (true-edge neighbours). */
  su2double sensor = 0.0;             /*!< \brief Nodal pressure sensor. */
  su2double lambda = 0.0;             /*!< \brief Nodal spectral radius of the true edges (stretching correction). */
  su2double nNeighbor = 1.0;          /*!< \brief Number of true-edge neighbours. */
  su2double wall = 0.0;               /*!< \brief JST_WEAK_WALL_DISSIPATION: 1 on weak no-slip wall nodes. */
};

/*!
 * \brief Constants of the scheme (CONFIG: gamma, JST_SENSOR_COEFF, CENTRAL_JACOBIAN_FIX_FACTOR, weak wall option).
 */
struct Params {
  su2double gamma = 1.4;
  su2double kappa2 = 0.5;
  su2double kappa4 = 0.02;
  su2double fixFactor = 4.0;
  bool weakWall = false;
};

/*!
 * \brief Dissipation coefficients of a pair; eps2Jac is the unmasked eps2 of the Jacobian (weak walls).
 */
struct Coefficients {
  su2double eps2 = 0.0;
  su2double eps4 = 0.0;
  su2double eps2Jac = 0.0;
};

/*!
 * \brief |ubar.n| + cbar |n| with the arithmetic means of velocity and speed of sound (finalizeFlux, avgV).
 */
inline su2double SpectralRadius(unsigned short nDim, const Node& Ni, const Node& Nk, const su2double* normal) {
  su2double projVel = 0.0, area2 = 0.0;
  for (unsigned short iDim = 0; iDim < nDim; ++iDim) {
    projVel += 0.5 * (Ni.vel[iDim] + Nk.vel[iDim]) * normal[iDim];
    area2 += normal[iDim] * normal[iDim];
  }
  return fabs(projVel) + 0.5 * (Ni.soundSpeed + Nk.soundSpeed) * sqrt(area2);
}

/*!
 * \brief Normal of the pair dissipation: n_ik itself, or with aligned (EDGE_FORM_P1_MIXED_DISS_ALIGNED) its edge
 *        projection nu_ik = (n_ik.e_ik/|e_ik|^2) e_ik, e_ik = x_k - x_i, written to buffer.
 * \return normal (the same pointer) if not aligned, else buffer.
 */
inline const su2double* DissipationNormal(unsigned short nDim, bool aligned, const su2double* normal,
                                          const su2double* coord_i, const su2double* coord_k, su2double* buffer) {
  if (!aligned) return normal;
  su2double edge[3] = {0.0, 0.0, 0.0}, nDotE = 0.0, edge2 = 0.0;
  for (unsigned short iDim = 0; iDim < nDim; ++iDim) {
    edge[iDim] = coord_k[iDim] - coord_i[iDim];
    nDotE += normal[iDim] * edge[iDim];
    edge2 += edge[iDim] * edge[iDim];
  }
  for (unsigned short iDim = 0; iDim < nDim; ++iDim) buffer[iDim] = nDotE / edge2 * edge[iDim];
  return buffer;
}

/*!
 * \brief Stretching correction of a pair spectral radius (correctedSpectralRadius).
 */
inline su2double CorrectedRadius(su2double lambda, const Node& Ni, const Node& Nk) {
  const su2double phi_i = pow(0.25 * Ni.lambda / lambda, STRETCH_PARAM);
  const su2double phi_k = pow(0.25 * Nk.lambda / lambda, STRETCH_PARAM);
  return 4 * phi_i * phi_k / (phi_i + phi_k) * lambda;
}

/*!
 * \brief JST_WEAK_WALL_DISSIPATION factor: 0 if exactly one end is on a weak no-slip wall, else 1 (jstWallFactor).
 */
inline su2double WallKeep(const Node& Ni, const Node& Nk) { return 1.0 - fabs(Ni.wall - Nk.wall); }

/*!
 * \brief eps2 and eps4 of the pair from the nodal sensors and the true-edge neighbour counts.
 */
inline Coefficients DissipationCoefficients(const Params& par, const Node& Ni, const Node& Nk) {
  const su2double ni = Ni.nNeighbor, nk = Nk.nNeighbor;
  const su2double sc2 = 3 * (ni + nk) / (ni * nk);
  const su2double sc4 = 0.25 * pow(sc2, 2);

  Coefficients eps;
  eps.eps2 = par.kappa2 * 0.5 * (Ni.sensor + Nk.sensor) * sc2;
  eps.eps4 = fmax(su2double(0.0), par.kappa4 - eps.eps2) * sc4;
  /*--- Weak walls: eps4 from the unmasked eps2 first, the Jacobian keeps the unmasked eps2 (as the edges). ---*/
  eps.eps2Jac = eps.eps2;
  if (par.weakWall) {
    const su2double keep = WallKeep(Ni, Nk);
    eps.eps4 *= keep;
    eps.eps2 *= keep;
  }
  return eps;
}

/*!
 * \brief SU2's surrogate Jacobian of a scalar dissipation term on the jump of (rho, rho u, rho H), times dissip
 *        (scalarDissipationJacobian with k = 0: with EDGE_FORM_P1 an SST run always has matched k, whose transform the
 *        caller applies, as the edge kernel).
 */
template <class Matrix>
inline void AddSurrogateJacobian(unsigned short nDim, su2double gamma, const su2double* vel, su2double dissip,
                                 Matrix& jac) {
  const unsigned short nVar = nDim + 2;
  for (unsigned short iVar = 0; iVar < nVar - 1; ++iVar) jac[iVar][iVar] += dissip;
  jac[nVar - 1][nVar - 1] += dissip * gamma;

  dissip *= gamma - 1.0;
  for (unsigned short iDim = 0; iDim < nDim; ++iDim) {
    jac[nVar - 1][iDim + 1] -= dissip * vel[iDim];
    jac[nVar - 1][0] += 0.5 * dissip * pow(vel[iDim], 2);
  }
}

/*!
 * \brief Adds the JST term of the pair (i, k) to its flux (from i to k) and, if implicit, the surrogate blocks to
 *        d flux/dU_i and d flux/dU_k.
 * \param[in] normal - Dissipation normal: the pair coefficient n_ik (SplitNormal) or its edge projection
 *                     (DissipationNormal).
 */
template <class Matrix>
inline void AddPairDissipation(unsigned short nDim, const Params& par, const Node& Ni, const Node& Nk,
                               const su2double* normal, su2double* flux, bool implicit, Matrix& jac_i, Matrix& jac_k,
                               const su2double* ledNormal = nullptr) {
  const unsigned short nVar = nDim + 2;
  /*--- Only an edge projection can vanish (n_ik perpendicular to e_ik): no term, and the stretching factor
   * (Lambda/lambda)^0.3 is not defined at lambda = 0. EDGE_FORM_P1_DISS_LED: the second-difference term on ledNormal
   * (the full pair normal n_ik), the fourth-difference term on normal. ---*/
  const su2double radius = SpectralRadius(nDim, Ni, Nk, normal);
  const su2double radius2 = ledNormal ? SpectralRadius(nDim, Ni, Nk, ledNormal) : radius;
  if (radius2 == 0.0) return;
  const su2double lambda = (radius == 0.0) ? su2double(0.0) : CorrectedRadius(radius, Ni, Nk);
  const su2double lambda2 = ledNormal ? CorrectedRadius(radius2, Ni, Nk) : lambda;
  const Coefficients eps = DissipationCoefficients(par, Ni, Nk);

  for (unsigned short iVar = 0; iVar < nVar; ++iVar) {
    if (ledNormal) {
      flux[iVar] += eps.eps2 * (Ni.diss[iVar] - Nk.diss[iVar]) * lambda2 - eps.eps4 * (Ni.lapl[iVar] - Nk.lapl[iVar]) * lambda;
    } else {  // off: unchanged rounding
      flux[iVar] += (eps.eps2 * (Ni.diss[iVar] - Nk.diss[iVar]) - eps.eps4 * (Ni.lapl[iVar] - Nk.lapl[iVar])) * lambda;
    }
  }
  if (!implicit) return;

  const su2double dissip_i = ledNormal ? par.fixFactor * (eps.eps2Jac * lambda2 + eps.eps4 * (Ni.nNeighbor + 1) * lambda)
                                       : par.fixFactor * (eps.eps2Jac + eps.eps4 * (Ni.nNeighbor + 1)) * lambda;
  const su2double dissip_k = ledNormal ? -par.fixFactor * (eps.eps2Jac * lambda2 + eps.eps4 * (Nk.nNeighbor + 1) * lambda)
                                       : -par.fixFactor * (eps.eps2Jac + eps.eps4 * (Nk.nNeighbor + 1)) * lambda;
  AddSurrogateJacobian(nDim, par.gamma, Ni.vel, dissip_i, jac_i);
  AddSurrogateJacobian(nDim, par.gamma, Nk.vel, dissip_k, jac_k);
}

}  // namespace P1PairJST
