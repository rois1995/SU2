/*!
 * \file CGoalMetric.hpp
 * \brief Goal-oriented adaptation (ADAP_SENSOR= GOAL, stage G): point functions of the estimator of Loseille,
 *        Dervieux and Alauzet (JCP 2010), H_go = sum_{d,j} |d_d lambda_j| |H(F_{d,j})|, with the Euler fluxes F of
 *        the converged primal and the residual adjoint lambda; mirror rules of these vector/tensor fields at
 *        symmetry planes.
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

#pragma once

#include <array>
#include <string>
#include <vector>

#include "../../../Common/include/basic_types/datatype_structure.hpp"

class CGeometry;
class CConfig;

/*!
 * \namespace GoalMetric
 * \brief Fields of the goal-oriented estimator. Layout of the adaptation work columns (AuxVar_Adapt, Gradient_Adapt) of
 *        a GOAL run: the nVar = nDim + 2 components of lambda first (density, momentum, energy), then the flux fields,
 *        column nVar + d * nVar + j = component j of the flux in direction d. Conservative variables U = (rho, rho u,
 *        rho E), perfect gas, p = (gamma - 1) (rho E - |rho u|^2 / (2 rho)), F_d = (rho u_d, rho u_d u + p e_d,
 *        (rho E + p) u_d). The flux tensor Phi = [F_1 .. F_nDim] (nVar x nDim) mirrors as Phi(S x) = T Phi(x) S and
 *        lambda(S x) = T lambda(x), S = I - 2 n n^T, T = diag(1, S, 1).
 */
namespace GoalMetric {

constexpr unsigned short MAXDIM = 3;
constexpr unsigned short MAXVAR = 5;
constexpr unsigned short MAXFLUX = 15;
constexpr unsigned short MAXFIELD = 20;

/*! \brief Number of conservative variables (components of lambda). */
inline unsigned short NumVar(unsigned short nDim) { return nDim + 2; }

/*! \brief Number of flux fields F_{d,j}. */
inline unsigned short NumFlux(unsigned short nDim) { return nDim * (nDim + 2); }

/*! \brief Number of work fields of a GOAL run (lambda and the fluxes). */
inline unsigned short FieldCount(unsigned short nDim) { return (nDim + 1) * (nDim + 2); }

/*! \brief Work column of the flux field F_{d,j}. */
inline unsigned short FluxColumn(unsigned short nDim, unsigned short d, unsigned short j) {
  return NumVar(nDim) + d * NumVar(nDim) + j;
}

/*!
 * \brief Euler fluxes of a conservative state.
 * \param[in] nDim - 2 or 3.
 * \param[in] gamma - Ratio of specific heats.
 * \param[in] U - Conservative variables (nDim + 2).
 * \param[out] F - F[d * nVar + j], component j of the flux in direction d.
 */
void FluxFields(unsigned short nDim, su2double gamma, const su2double* U, su2double* F);

/*!
 * \brief Jacobians of the Euler fluxes, A[d][i][j] = dF_{d,i} / dU_j.
 */
void FluxJacobians(unsigned short nDim, su2double gamma, const su2double* U, su2double (&A)[MAXDIM][MAXVAR][MAXVAR]);

/*!
 * \brief Odd part, under the mirror of the unit normal n, of one scalar per field component: lambda_odd = (0, n n^T
 *        lambda_m, 0); Phi_odd = Tn Phi Pt + Te Phi Pn, Pn = n n^T, Pt = I - Pn, Tn = diag(0, Pn, 0), Te = I - Tn.
 *        The even part is the rest. Linear, so it applies to each derivative component of the fields.
 * \param[in] nDim - 2 or 3.
 * \param[in] n - Unit normal of the plane.
 * \param[in] lambda - nVar values of the lambda components, or nullptr (no lambda part).
 * \param[in] phi - nDim * nVar values of the flux components (phi[d * nVar + j]).
 * \param[out] lambdaOdd - Odd part of lambda (if lambda is given).
 * \param[out] phiOdd - Odd part of phi.
 */
void OddPart(unsigned short nDim, const su2double* n, const su2double* lambda, const su2double* phi,
             su2double* lambdaOdd, su2double* phiOdd);

/*!
 * \brief Mirror rule at a symmetry-plane point for the values of all fields (nField = FieldCount, layout of the work
 *        columns): the odd part is zero on the plane.
 */
void MirrorValues(unsigned short nDim, const su2double* n, su2double* values);

/*!
 * \brief Mirror rule for the gradients of all fields, grad[k * nDim + a]: even components keep their tangential
 *        gradient, odd components their normal gradient.
 */
void MirrorGradients(unsigned short nDim, const su2double* n, su2double* grad);

/*!
 * \brief Mirror rule for the Hessians of the flux fields, hess[(f * nDim + a) * nDim + b] (f = d * nVar + j, full
 *        symmetric matrices): even components lose their normal-tangential block, odd components keep only it.
 */
void MirrorHessians(unsigned short nDim, const su2double* n, su2double* hess);

/*!
 * \brief Adds |weight| |H| to Hgo and weight H to S (H full, row-major nDim x nDim).
 * \return false, and nothing added, if H has non-finite entries.
 */
bool AccumulateField(unsigned short nDim, su2double weight, const su2double* H, su2double (&Hgo)[MAXDIM][MAXDIM],
                     su2double (&S)[MAXDIM][MAXDIM]);

/*! \brief Sum of the absolute eigenvalues of a symmetric matrix. */
su2double NuclearNorm(unsigned short nDim, const su2double (&S)[MAXDIM][MAXDIM]);

/*!
 * \brief Interior weights of the improved estimate (AF21 eq. 33 without the objective and viscous terms):
 *        eq33Signed = sum_j |G_j|, G = sum_d A_d^T d_d lambda; eq33Abs = sum_j sum_{d,k} |A_d,kj d_d lambda_k|.
 */
void Eq33Terms(unsigned short nDim, const su2double* gradLambda, const su2double (&A)[MAXDIM][MAXVAR][MAXVAR],
               su2double& eq33Signed, su2double& eq33Abs);

/*!
 * \brief Estimate of one point.
 * \param[in] nDim - 2 or 3.
 * \param[in] gradLambda - gradLambda[j * nDim + d] = d_d lambda_j.
 * \param[in] hess - Hessians of the flux fields, as in MirrorHessians.
 * \param[in] A - Flux Jacobians of the point (FluxJacobians).
 * \param[out] Hgo - sum_{d,j} |d_d lambda_j| |H(F_{d,j})| (positive semidefinite, full matrix).
 * \param[out] signedNorm - Nuclear norm (sum of |eigenvalues|) of the signed sum sum_{d,j} d_d lambda_j H(F_{d,j}).
 * \param[out] eq33Signed - sum_j |G_j|, G = sum_d A_d^T d_d lambda (interior weights of the improved estimate).
 * \param[out] eq33Abs - sum_j sum_{d,k} |A_d,kj d_d lambda_k|.
 * \return Number of flux Hessians with non-finite entries (counted as zero).
 */
unsigned short PointEstimate(unsigned short nDim, const su2double* gradLambda, const su2double* hess,
                             const su2double (&A)[MAXDIM][MAXVAR][MAXVAR], su2double (&Hgo)[MAXDIM][MAXDIM],
                             su2double& signedNorm, su2double& eq33Signed, su2double& eq33Abs);

/*!
 * \brief Collective: the symmetry planes the mirror rules support, and their normals. Every SYMMETRY_PLANE marker must
 *        be planar (vertex unit normals within 1e-8 of the area-weighted normal), two symmetry markers meeting at a
 *        point must be orthogonal, and the problem must be mirror-invariant: freestream velocity normal to no plane,
 *        objectives DRAG, LIFT (directions as SU2 builds them from AoA and AoS), FORCE_k with e_k . n = 0, MOMENT_k
 *        with e_k parallel to n and every moment origin on the plane. SIDEFORCE and other objectives need an odd
 *        adjoint and are rejected when there is a symmetry plane.
 * \param[in] geometry - Partitioned geometry.
 * \param[in] config - Definition of the problem.
 * \param[out] normals - Unit plane normal of each marker (iMarker; zero for other markers).
 * \return Empty if supported, otherwise the reason.
 */
std::string CheckSymmetry(const CGeometry& geometry, const CConfig& config,
                          std::vector<std::array<su2double, MAXDIM>>& normals);

}  // namespace GoalMetric
