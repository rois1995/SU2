/*!
 * \file p1_turb_galerkin.hpp
 * \brief TURB_VISC_P1_GALERKIN: P1 Galerkin diffusion of the turbulence equations (TURB_GALERKIN_DIFFUSION.md).
 * \note On every simplex T the diffusion of a turbulence variable phi is R_a = D sum_b S_ab phi_b, S_ab = |T| g_a.g_b
 *       (g_a the shape gradients), the sign of the viscous residual (R_a = |T| F.g_a, F = D grad phi_T). This is the
 *       finite volume flux of the element gradient grad phi_T through the dual facets of node a inside T (on the median
 *       dual, and in 2D on any dual with edge-midpoint face points), so each facet gets a compact flux instead of the
 *       averaged nodal gradients of the edge scheme. S is the cotangent Laplacian: zero row sums, symmetric positive
 *       semi-definite, off-diagonals <= 0 on non-obtuse simplices, zero across the diagonal of a right-angle split.
 *       SST: k and omega with the vertex means of mu + sigma(F1) mu_t, and the cross diffusion of SU2's divergence form
 *       with the element gradient. SA: the diffusion (1/sigma) div((nu + fn nu_t) grad nu_t) and the cb2 term
 *       (cb2/sigma) |grad nu_t|^2 as a lumped source. The coefficients are frozen in the Picard Jacobian.
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

#include <vector>
#include "../../../../Common/include/basic_types/datatype_structure.hpp"
#include "../../../../Common/include/parallelization/mpi_structure.hpp"
#include "../../../../Common/include/toolboxes/p1_simplex.hpp"
#include "../../../../Common/include/toolboxes/p1_mixed.hpp"
#include "../flow/p1_sst_matched.hpp"

namespace P1TurbGalerkin {

constexpr unsigned short MAXNODE = 4; /*!< \brief Nodes of a tetrahedron. */

/*--- SST constants of the omega diffusion and of the cross diffusion, those of CScalarFlux_SST (checked there by a
 * static_assert). The k constants are SSTEnergy::sigma_k1 and sigma_k2 (P1SSTMatched::ElementKineDiffusivity). ---*/
constexpr passivedouble sigma_om1 = 0.5;
constexpr passivedouble sigma_om2 = 0.856;

/*--- SA constants of the diffusion, those of CScalarFlux_SA. ---*/
constexpr passivedouble sa_sigma = 2.0 / 3.0;
constexpr passivedouble sa_cb2 = 0.622;
constexpr passivedouble sa_cn1 = 16.0;

/*!
 * \brief Whether the fine-grid edge loop of a turbulence solver leaves out the diffusion (TURB_VISC_P1_GALERKIN), and
 *        then opt.viscous = false; the options are not touched otherwise.
 * \param[in] config - CConfig (GetTurb_Visc_P1_Galerkin).
 * \param[in] iMesh - Grid level, the element operator runs on the fine grid (level 0) only.
 * \param[in,out] opt - ScalarFluxOptions of the interior edge loop.
 * \return True if the element operator replaces the edge diffusion.
 */
template <class Config, class Options>
inline bool ElementDiffusionReplacesEdges(const Config& config, unsigned short iMesh, Options& opt) {
  if (!config.GetTurb_Visc_P1_Galerkin() || (iMesh != 0)) return false;
  opt.viscous = false;
  return true;
}

/*!
 * \brief Calls visit(point, coord, weight) for every simplex of the rank that holds an owned node: triangles and
 *        tetrahedra with weight 1 and, with EDGE_FORM_P1_MIXED, the weighted sub-simplices of the split average of the
 *        other cells (as CNSSolver::MixedViscousResidual). point and coord have nDim + 1 entries. Each rank adds to its
 *        owned rows only (the caller checks GetDomain), so every row gets the elements around it exactly once.
 */
template <class Geometry, class Visitor>
void ForEachSimplex(const Geometry& geometry, unsigned short nDim, bool mixed, Visitor&& visit) {
  const unsigned short nNodes = nDim + 1;

  /*--- A simplex is its own list with weight 1. ---*/
  P1Mixed::Simplex self;
  for (unsigned short a = 0; a < nNodes; ++a) self.nodes[a] = a;
  self.weight = 1.0;
  const std::vector<P1Mixed::Simplex> selfList(1, self);

  for (unsigned long iElem = 0; iElem < geometry.GetnElem(); ++iElem) {
    const auto* element = geometry.elem[iElem];
    const unsigned short nCellNodes = element->GetnNodes();
    if ((nCellNodes != nNodes) && !mixed) {
      SU2_MPI::Error("TURB_VISC_P1_GALERKIN: needs triangles or tetrahedra, or EDGE_FORM_P1_MIXED= YES.", CURRENT_FUNCTION);
    }

    bool owned = false;
    for (unsigned short a = 0; a < nCellNodes; ++a) owned |= geometry.nodes->GetDomain(element->GetNode(a));
    if (!owned) continue;

    const auto& subs = (nCellNodes == nNodes) ? selfList : geometry.GetMixedSimplices(element->GetVTK_Type());
    for (const auto& entry : subs) {
      unsigned long point[MAXNODE];
      const su2double* coord[MAXNODE];
      for (unsigned short a = 0; a < nNodes; ++a) {
        point[a] = element->GetNode(entry.nodes[a]);
        coord[a] = geometry.nodes->GetCoord(point[a]);
      }
      visit(point, coord, su2double(entry.weight));
    }
  }
}

/*!
 * \brief Stiffness matrix of the P1 Laplacian on a simplex, S_ab = |T| g_a.g_b (the cotangent weights: in 2D
 *        -S_ab = cot(theta_ab)/2 with theta_ab the angle opposite the edge ab).
 * \param[in] nDim - Dimension, nDim + 1 nodes.
 * \param[in] size - |T|.
 * \param[in] grad - Shape gradients g_a (P1Simplex::ShapeGradients).
 * \param[out] S - S[a][b].
 */
inline void Stiffness(unsigned short nDim, const su2double& size, const su2double (*grad)[3], su2double (*S)[MAXNODE]) {
  const unsigned short nNodes = nDim + 1;
  for (unsigned short a = 0; a < nNodes; ++a) {
    for (unsigned short b = 0; b < nNodes; ++b) {
      su2double dot = 0.0;
      for (unsigned short iDim = 0; iDim < nDim; ++iDim) dot += grad[a][iDim] * grad[b][iDim];
      S[a][b] = size * dot;
    }
  }
}

/*!
 * \brief Gradient of the P1 interpolant of the nodal values phi, sum_b phi_b g_b.
 */
inline void ElementGradient(unsigned short nDim, const su2double (*grad)[3], const su2double* phi, su2double* gradPhi) {
  for (unsigned short iDim = 0; iDim < nDim; ++iDim) {
    gradPhi[iDim] = 0.0;
    for (unsigned short b = 0; b <= nDim; ++b) gradPhi[iDim] += phi[b] * grad[b][iDim];
  }
}

/*!
 * \brief Row a of S times phi, sum_b S_ab phi_b = |T| grad phi_T . g_a.
 */
inline su2double StiffnessRow(unsigned short nNodes, const su2double (*S)[MAXNODE], unsigned short a, const su2double* phi) {
  su2double sum = 0.0;
  for (unsigned short b = 0; b < nNodes; ++b) sum += S[a][b] * phi[b];
  return sum;
}

inline su2double VertexMean(unsigned short nNodes, const su2double* value) {
  su2double mean = 0.0;
  for (unsigned short a = 0; a < nNodes; ++a) mean += value[a];
  return mean / nNodes;
}

/*--- SST. ---*/

/*!
 * \brief Element coefficients of the SST diffusion, the vertex means of the edge averages of CScalarFlux_SST.
 */
struct SSTCoefficients {
  su2double diffKine = 0.0;  /*!< \brief Mean of mu + sigma_k(F1) mu_t (P1SSTMatched::ElementKineDiffusivity). */
  su2double diffOmega = 0.0; /*!< \brief Mean of mu + sigma_om(F1) mu_t. */
  su2double lambda = 0.0;    /*!< \brief Mean of 2 (1 - F1) rho sigma_om2, the cross-diffusion coefficient. */
  su2double omegaMean = 0.0; /*!< \brief Mean of omega, the face omega of the cross diffusion. */
};

/*!
 * \brief SSTCoefficients of a simplex from its nodal values.
 */
inline SSTCoefficients MakeSSTCoefficients(unsigned short nNodes, const su2double* mu, const su2double* muT,
                                           const su2double* F1, const su2double* rho, const su2double* omega) {
  SSTCoefficients C;
  C.diffKine = P1SSTMatched::ElementKineDiffusivity(nNodes, mu, muT, F1);
  for (unsigned short a = 0; a < nNodes; ++a) {
    const su2double sigmaOmega = F1[a] * sigma_om1 + (1.0 - F1[a]) * sigma_om2;
    C.diffOmega += (mu[a] + sigmaOmega * muT[a]) / nNodes;
    C.lambda += 2.0 * (1.0 - F1[a]) * rho[a] * sigma_om2 / nNodes;
  }
  C.omegaMean = VertexMean(nNodes, omega);
  return C;
}

/*!
 * \brief Cross-diffusion weight of row a, lambda (1 - omega_a / omegaMean). It is SU2's divergence form of
 *        (lambda/omega) grad k . grad omega, sum over the faces of lambda (1 - omega_a/omega_face) grad k . n
 *        (CScalarFlux_SST::coefficients), with the element gradient of k and the element mean as the face omega.
 */
inline su2double CrossWeight(const SSTCoefficients& C, const su2double& omega_a) {
  return C.lambda * (1.0 - omega_a / C.omegaMean);
}

/*!
 * \brief SST rows of a simplex: resK[a] = Dk (S k)_a, resOmega[a] = Domega (S omega)_a + CrossWeight_a (S k)_a.
 * \param[in] kineRow - Whether the k row is assembled here (not with SST_ENERGY_CONSISTENT and VISC_P1_ENTROPY,
 *            where CTurbSSTSolver::P1KineDiffusion has it); resK is then 0.
 */
inline void SSTResidual(unsigned short nNodes, const su2double (*S)[MAXNODE], const su2double* k, const su2double* omega,
                        const SSTCoefficients& C, bool kineRow, su2double* resK, su2double* resOmega) {
  for (unsigned short a = 0; a < nNodes; ++a) {
    const su2double Sk = StiffnessRow(nNodes, S, a, k);
    resK[a] = 0.0;
    if (kineRow) resK[a] = C.diffKine * Sk;
    resOmega[a] = C.diffOmega * StiffnessRow(nNodes, S, a, omega) + CrossWeight(C, omega[a]) * Sk;
  }
}

/*!
 * \brief Jacobian of SSTResidual w.r.t. the conserved variables (rho k, rho omega) of node b (the unknowns of the SST
 *        linear system, k = (rho k)/rho with the flow density frozen), jac[a][b][row][column]. The coefficients are
 *        frozen (mu_t, F1, rho, the means), as in the edge scheme. With accurate (USE_ACCURATE_TURB_JACOBIANS) the omega
 *        dependence of CrossWeight is added: d/d omega_b of -lambda omega_a/omegaMean = -lambda (delta_ab - omega_a/
 *        (n omegaMean)) / omegaMean, as CScalarFlux_SST::coefficientJacobians does for the edge.
 */
inline void SSTJacobian(unsigned short nNodes, const su2double (*S)[MAXNODE], const su2double* k, const su2double* omega,
                        const su2double* rho, const SSTCoefficients& C, bool kineRow, bool accurate,
                        su2double (*jac)[MAXNODE][2][2]) {
  for (unsigned short a = 0; a < nNodes; ++a) {
    const su2double cross = CrossWeight(C, omega[a]);
    const su2double Sk = accurate ? StiffnessRow(nNodes, S, a, k) : su2double(0.0);
    for (unsigned short b = 0; b < nNodes; ++b) {
      jac[a][b][0][0] = 0.0;
      if (kineRow) jac[a][b][0][0] = C.diffKine * S[a][b] / rho[b];
      jac[a][b][0][1] = 0.0;
      jac[a][b][1][0] = cross * S[a][b] / rho[b];
      jac[a][b][1][1] = C.diffOmega * S[a][b] / rho[b];
      if (accurate) {
        const su2double delta = (a == b) ? 1.0 : 0.0;
        const su2double dCross = -C.lambda * (delta - omega[a] / (nNodes * C.omegaMean)) / C.omegaMean;
        jac[a][b][1][1] += dCross * Sk / rho[b];
      }
    }
  }
}

/*--- SA. ---*/

/*!
 * \brief fn of the negative SA model (Allmaras, Johnson & Spalart 2012), (cn1 + chi^3)/(cn1 - chi^3) for chi < 0 and 1
 *        otherwise (and always 1 without SA-neg), and its derivative dfn/dchi.
 */
inline su2double SAFn(bool negativeSA, const su2double& chi, su2double& dfn) {
  dfn = 0.0;
  if (!negativeSA || (chi >= 0.0)) return 1.0;
  const su2double chi3 = chi * chi * chi;
  const su2double den = sa_cn1 - chi3;
  dfn = 6.0 * sa_cn1 * chi * chi / (den * den);
  return (sa_cn1 + chi3) / den;
}

/*!
 * \brief Element diffusivity of SA, (nuMean + fn(chi) nuTildeMean)/sigma, chi = nuTildeMean/nuMean (the vertex means),
 *        and its derivative w.r.t. each nodal nu_tilde, dD = (fn + chi dfn/dchi)/(sigma n).
 */
inline su2double SADiffusivity(unsigned short nNodes, const su2double* nu, const su2double* nuTilde, bool negativeSA,
                               su2double& dD) {
  const su2double nuMean = VertexMean(nNodes, nu);
  const su2double nuTildeMean = VertexMean(nNodes, nuTilde);
  const su2double chi = nuTildeMean / nuMean;
  su2double dfn = 0.0;
  const su2double fn = SAFn(negativeSA, chi, dfn);
  dD = (fn + chi * dfn) / (sa_sigma * nNodes);
  return (nuMean + fn * nuTildeMean) / sa_sigma;
}

/*!
 * \brief SA rows of a simplex: res[a] = D (S nu_t)_a - (cb2/sigma) |grad nu_t,T|^2 |T|/(d+1). The second term is the
 *        cb2 part of the model as a lumped source (>= 0, it enters the residual with the minus sign of a source).
 */
inline void SAResidual(unsigned short nDim, const su2double& size, const su2double (*grad)[3],
                       const su2double (*S)[MAXNODE], const su2double* nuTilde, const su2double& D, su2double* res) {
  const unsigned short nNodes = nDim + 1;
  su2double gradNuTilde[3] = {0.0, 0.0, 0.0};
  ElementGradient(nDim, grad, nuTilde, gradNuTilde);
  su2double norm2 = 0.0;
  for (unsigned short iDim = 0; iDim < nDim; ++iDim) norm2 += gradNuTilde[iDim] * gradNuTilde[iDim];
  const su2double source = (sa_cb2 / sa_sigma) * norm2 * size / nNodes;
  for (unsigned short a = 0; a < nNodes; ++a) res[a] = D * StiffnessRow(nNodes, S, a, nuTilde) - source;
}

/*!
 * \brief Jacobian of SAResidual w.r.t. nu_tilde of node b, jac[a][b]. Picard: D S_ab (D frozen, the cb2 source
 *        explicit, it is >= 0). With accurate (USE_ACCURATE_TURB_JACOBIANS, not SA-neg, as CScalarFlux_SA) also
 *        dD/dnu_t,b (S nu_t)_a and the source derivative -(2 cb2/sigma) (|T|/(d+1)) grad nu_t,T . g_b.
 */
inline void SAJacobian(unsigned short nDim, const su2double& size, const su2double (*grad)[3],
                       const su2double (*S)[MAXNODE], const su2double* nuTilde, const su2double& D, const su2double& dD,
                       bool accurate, su2double (*jac)[MAXNODE]) {
  const unsigned short nNodes = nDim + 1;
  su2double gradNuTilde[3] = {0.0, 0.0, 0.0};
  ElementGradient(nDim, grad, nuTilde, gradNuTilde);
  for (unsigned short a = 0; a < nNodes; ++a) {
    const su2double Snu = accurate ? StiffnessRow(nNodes, S, a, nuTilde) : su2double(0.0);
    for (unsigned short b = 0; b < nNodes; ++b) {
      jac[a][b] = D * S[a][b];
      if (!accurate) continue;
      su2double gradDotG = 0.0;
      for (unsigned short iDim = 0; iDim < nDim; ++iDim) gradDotG += gradNuTilde[iDim] * grad[b][iDim];
      jac[a][b] += dD * Snu - 2.0 * (sa_cb2 / sa_sigma) * size / nNodes * gradDotG;
    }
  }
}

}  // namespace P1TurbGalerkin
