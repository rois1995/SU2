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
/*--- EDGE_FORM_P1_MIXED_DISS_MEAN: the JST term as the mean over the conforming splits s of the non-simplex cells, as the
 * central part (the assembled pair coefficient n_ik is the mean of the split coefficients, zero where s lacks the pair).
 * With pi_ik the share of the splits that hold the pair (1 on true edges, CEdge::GetSplitShare on split pairs), a split
 * that holds it has the coefficient n_ik/pi_ik, and Phi lambda (stretching correction times radius) is homogeneous of
 * degree 1 - STRETCH_PARAM = 0.7 in lambda, so
 *     mean_s [Phi(lambda/pi) lambda/pi] = pi Phi(lambda/pi) lambda/pi = pi^0.3 Phi(lambda) lambda   (ShareFactor).
 * The nodal data of the splits are replaced by their pi-weighted forms on the pair graph (true edges and split pairs):
 * the neighbour count n_i = sum_k pi_ik (NeighborWeights), the undivided Laplacian and the pressure sensor with the
 * weights pi_ik (ForEachOwnedPairEnd, the boundary-neighbour rule of the true edges), the nodal spectral radius on the
 * form normals of the true edges and the coefficients of the split pairs. eps2 >= 0 is kept. ---*/
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
 * \brief EDGE_FORM_P1_MIXED_DISS_MEAN: factor pi^STRETCH_PARAM of the stretching-corrected radius of a pair held by the
 *        share pi of the splits; exactly 1 for pi = 1 (true edges, or the option off).
 */
inline su2double ShareFactor(su2double share) {
  return (share == 1.0) ? su2double(1.0) : su2double(pow(share, STRETCH_PARAM));
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

/*--- chi = eps4 lambda, with the share contribution pi^0.3 already in lambda. ---*/
inline su2double SharedRadius(su2double radius, const Node& Ni, const Node& Nk, su2double share,
                             const su2double* shareFactor) {
  su2double lambda = CorrectedRadius(radius, Ni, Nk);
  if (share != 1.0) {
    const su2double factor = shareFactor ? *shareFactor : ShareFactor(share);
    lambda *= factor;
  }
  return lambda;
}

inline su2double FourthCoefficient(const Params& par, const Node& Ni, const Node& Nk, su2double radius,
                                  su2double share = 1.0, const su2double* shareFactor = nullptr) {
  if (radius == 0.0) return 0.0;
  return DissipationCoefficients(par, Ni, Nk).eps4 * SharedRadius(radius, Ni, Nk, share, shareFactor);
}

/*--- For true edges, the SIMD projection uses the signed area n.e/|e|. ---*/
inline su2double TrueEdgeRadius(unsigned short nDim, const Node& Ni, const Node& Nk, const su2double* normal,
                               bool projected, const su2double* coord_i, const su2double* coord_k) {
  if (!projected) return SpectralRadius(nDim, Ni, Nk, normal);
  su2double edge[3], dot = 0.0, edge2 = 0.0;
  for (auto d = 0u; d < nDim; ++d) {
    edge[d] = coord_k[d] - coord_i[d];
    dot += normal[d] * edge[d];
    edge2 += edge[d] * edge[d];
  }
  const su2double invLength = 1.0 / sqrt(edge2);
  const su2double area = dot * invLength;
  const su2double scale = area * invLength;
  su2double projVel = 0.0;
  for (auto d = 0u; d < nDim; ++d) projVel += 0.5 * (Ni.vel[d] + Nk.vel[d]) * (scale * edge[d]);
  return fabs(projVel) + 0.5 * (Ni.soundSpeed + Nk.soundSpeed) * area;
}

/*!
 * \brief Adds the JST term of the pair (i, k) to its flux (from i to k) and, if implicit, the surrogate blocks to
 *        d flux/dU_i and d flux/dU_k.
 * \param[in] normal - Dissipation normal: the pair coefficient n_ik (SplitNormal) or its edge projection
 *                     (DissipationNormal).
 * \param[in] shareFactor - Optional precomputed ShareFactor(share) for this pair.
 */
template <class Matrix>
inline void AddPairDissipation(unsigned short nDim, const Params& par, const Node& Ni, const Node& Nk,
                               const su2double* normal, su2double* flux, bool implicit, Matrix& jac_i, Matrix& jac_k,
                               su2double share = 1.0, const su2double* shareFactor = nullptr,
                               bool laplacianFourth = true) {
  const unsigned short nVar = nDim + 2;
  /*--- Only an edge projection can vanish (n_ik perpendicular to e_ik): no term, and the stretching factor
   * (Lambda/lambda)^0.3 is not defined at lambda = 0. EDGE_FORM_P1_MIXED_DISS_MEAN scales the corrected radius by
   * pi^0.3 (ShareFactor); the nodal data of Ni and Nk are already the pi-weighted ones. ---*/
  const su2double radius = SpectralRadius(nDim, Ni, Nk, normal);
  if (radius == 0.0) return;
  const su2double lambda = SharedRadius(radius, Ni, Nk, share, shareFactor);
  const Coefficients eps = DissipationCoefficients(par, Ni, Nk);

  if (!laplacianFourth) {
    for (auto v = 0u; v < nVar; ++v) flux[v] += (eps.eps2 * (Ni.diss[v] - Nk.diss[v])) * lambda;
    if (implicit) {
      AddSurrogateJacobian(nDim, par.gamma, Ni.vel, par.fixFactor * eps.eps2Jac * lambda, jac_i);
      AddSurrogateJacobian(nDim, par.gamma, Nk.vel, -par.fixFactor * eps.eps2Jac * lambda, jac_k);
    }
    return;
  }

  for (unsigned short iVar = 0; iVar < nVar; ++iVar) {
    flux[iVar] += (eps.eps2 * (Ni.diss[iVar] - Nk.diss[iVar]) - eps.eps4 * (Ni.lapl[iVar] - Nk.lapl[iVar])) * lambda;
  }
  if (!implicit) return;

  /*--- Fourth-difference count n_i + 1 of the edge kernel: |dL_i/dU_i| + |dL_k/dU_i| = n_i + 1 on the true-edge
   * graph. With the pi-weighted Laplacians (EDGE_FORM_P1_MIXED_DISS_MEAN) it is n_i + pi_ik; share = 1 otherwise. ---*/
  const su2double dissip_i = par.fixFactor * (eps.eps2Jac + eps.eps4 * (Ni.nNeighbor + share)) * lambda;
  const su2double dissip_k = -par.fixFactor * (eps.eps2Jac + eps.eps4 * (Nk.nNeighbor + share)) * lambda;
  AddSurrogateJacobian(nDim, par.gamma, Ni.vel, dissip_i, jac_i);
  AddSurrogateJacobian(nDim, par.gamma, Nk.vel, dissip_k, jac_k);
}

/*!
 * \brief EDGE_FORM_P1_MIXED_DISS_MEAN: calls add(a, b, pi_ab) for each end a of every split pair (a, b) that is owned
 *        by this rank, b the other end, pi_ab its share (CEdge::GetSplitShare). With boundaryRule the end is skipped
 *        if a is on a physical boundary and b is not, the neighbour rule of the undivided Laplacian and of the sensor.
 * \note An owned node has all its split pairs on its rank (CPhysicalGeometry::SetEdgeFormP1), so the nodal sums of the
 *       owned nodes are complete; the halo values come from the exchange that follows each nodal pass.
 */
template <class Geometry, class AddTerm>
void ForEachOwnedPairEnd(const Geometry& geometry, bool boundaryRule, const AddTerm& add) {
  const auto& pairs = geometry.edges->GetSplitNodes();
  const auto& share = geometry.edges->GetSplitShare();
  for (unsigned long iSplit = 0; iSplit < pairs.size(); ++iSplit) {
    for (unsigned short end = 0; end < 2; ++end) {
      const auto a = pairs[iSplit][end], b = pairs[iSplit][1 - end];
      if (!geometry.nodes->GetDomain(a)) continue;
      if (boundaryRule && geometry.nodes->GetPhysicalBoundary(a) && !geometry.nodes->GetPhysicalBoundary(b)) continue;
      add(a, b, share[iSplit]);
    }
  }
}

/*!
 * \brief EDGE_FORM_P1_MIXED_DISS_MEAN: neighbour weight n_i = sum_k pi_ik of each node on the pair graph, the number
 *        of true-edge neighbours plus the shares of its split pairs; on the owned nodes (the halo nodes keep their
 *        true-edge count and need the exchange). A mesh without split pairs gives exactly GetnNeighbor.
 * \param[out] weights - Resized to the number of points of the rank.
 */
template <class Geometry, class Vector>
void NeighborWeights(const Geometry& geometry, Vector& weights) {
  const unsigned long nPoint = geometry.GetnPoint();
  weights.resize(nPoint);
  for (unsigned long iPoint = 0; iPoint < nPoint; ++iPoint) weights[iPoint] = geometry.nodes->GetnNeighbor(iPoint);
  ForEachOwnedPairEnd(geometry, false, [&](unsigned long a, unsigned long, su2double share) { weights[a] += share; });
}

}  // namespace P1PairJST
