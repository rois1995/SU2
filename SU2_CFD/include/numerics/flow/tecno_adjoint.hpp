/*!
 * \file tecno_adjoint.hpp
 * \brief Adjoint (dual consistent) entropy dissipation of EDGE_FORM_TECNO (EDGE_FORM_TECNO_ADJOINT): the linear
 *        U-MUSCL jump of the nodal entropy variables, its Green-Gauss gradient, the transpose of that gradient, and
 *        the Lemma A4 majorant of its linearization (P1_TECNO_CONVERGENCE.md, 3.2 and 5.1), and the first-ring
 *        Jacobians of TECNO_ADJOINT_JACOBIAN= FIRST_RING and FIRST_RING_JUMPS.
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

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <vector>
#include "../../../../Common/include/basic_types/datatype_structure.hpp"
#include "../../../../Common/include/containers/C2DContainer.hpp"
#include "../../../../Common/include/parallelization/mpi_structure.hpp"
#include "../../../../Common/include/parallelization/omp_structure.hpp"

/*--- Scheme (P1_TECNO_CONVERGENCE.md, 3.2). Ideal gas, w = ((gamma-s)/(gamma-1) - beta|u|^2/2, beta u, -beta) the
 * entropy variables of U = -rho s/(gamma-1), beta = rho/p. On the edge e = (i,k), e_ik = x_k - x_i,
 *
 *   L_e(w) = (1 - rho kappa)(w_k - w_i) - 1/2 rho (1-kappa) (phi_i G_i w + phi_k G_k w).e_ik = sum_m c_{e,m} w_m,
 *   q_e = D_e L_e,  D_e = 1/2 |nu| delta R|Lambda|T R^T (frozen, PSD),   R^diss_m = sum_e c_{e,m} q_e,
 *
 * rho the MUSCL ramp, kappa the U-MUSCL kappa, phi_i any nodal reals, G the gradient below. Then
 * sum_m w_m.R^diss_m = sum_e L_e^T D_e L_e >= 0 for any states and phi (Theorem A1). The Delta part of c_e is the edge
 * flux -(1 - rho kappa) q_e (scattered +/- by the edge loop), the gradient part is
 *   R_m += gam sum_i dG_i/dw_m : Z_i,  Z_i = phi_i sum_{e ni i} e_e (x) q_e,  gam = -1/2 rho (1-kappa),
 * applied here after the edge loop (the SIMD colouring does not allow scattering to second neighbours).
 *
 * Gradient: Green-Gauss on the dual in difference form, G_i w = V_i^-1 sum_k 1/2 (w_k - w_i) n_ik. With the closure
 * sum_k n_ik + beta_i = 0 of the dual cell (beta_i its outward boundary normal) it equals the usual
 * V_i^-1 [sum_k 1/2 (w_i + w_k) n_ik + w_i beta_i], so it is linear exact where that one is (interior nodes of the
 * median dual), and G 1 = 0 holds exactly, without the boundary normals. Its transpose is
 *   dG_i/dw_k = n_ik/(2 V_i),  dG_i/dw_i = -sum_k n_ik/(2 V_i),  so  R_m += -gam/2 sum_k n_mk.(Y_m + Y_k),  Y = Z/V.
 *
 * MPI: G, Y and phi are computed on the owned nodes, whose edges, neighbours, volumes and normals are all on this
 * rank, and copied to the halos. Z_i needs q_e on all edges of i, i.e. G and phi of its neighbours (halos after the
 * copy). Y is divided by the volume on the owner, halo volumes being incomplete. The residual of an owned node then
 * equals the serial one.
 *
 * TECNO_ADJOINT_GRADIENT= P1_AVERAGE (P1_TECNO_ADJ_GRADIENT.md, G2/G3): G_i w = Vhat_i^-1 sum_{T ni i} W_i^T grad(w_h)|_T,
 * W_i^T the dual share of node i in the simplex T (the sub-volumes of the residual's dual, moved with MOD_CENTROIDS),
 * Vhat_i = sum_T W_i^T. On simplices this is the difference form G_i w = Vhat_i^-1 sum_k a_ik (w_k - w_i),
 * a_ik = sum_{T ni i,k} W_i^T g_k^T (a_ik != a_ki), linear exact at every node (sum_k a_ik (x) (x_k - x_i) = Vhat_i I),
 * G 1 = 0. Its transpose: R_m += gam sum_k (a_km.Y_k - a_mk.Y_m), Y = Z/Vhat. Green-Gauss is the case a_ik = n_ik/2,
 * a_ki = -n_ik/2; its functions keep their own arithmetic. The weights are stored per edge, a_{0,1} in columns
 * [0, nDim) and a_{1,0} in [nDim, 2 nDim) (node 0 / 1 of the edge); they are complete on the edges of owned nodes,
 * and Vhat on owned nodes, since all cells of an owned node are on its rank. ---*/

namespace TeCNOAdjoint {

constexpr unsigned short MAXNVAR = 5;
constexpr unsigned long OMP_CHUNK = 512;

/*--- TECNO_ADJOINT_LIMITER= EDGE_ENTROPY_TVD: columns of the per-edge ingredients (tecnoAdjointTVDIngredients, local
 * orientation): dy, s_0, s_1 of the fields M, 0, P, the coefficients D of M, 0, P, S, the guards d_A, d_S, then the shear
 * vectors dy_S, s_0S, s_1S (nDim each). Fields of the limiter values: 0 = M, 1 = 0 (entropy), 2 = S (shear), 3 = P. ---*/
constexpr unsigned short TVD_DY = 0, TVD_S0 = 3, TVD_S1 = 6, TVD_D = 9, TVD_GUARD = 13, TVD_SHEAR = 15;
constexpr unsigned short TVD_NFIELD = 4;
inline unsigned short TVDWidth(unsigned short nDim) { return TVD_SHEAR + 3 * nDim; }

/*!
 * \brief Small dense block, for the su2double users of the templates below.
 */
struct Block {
  su2double v[MAXNVAR][MAXNVAR] = {};
  su2double& operator()(unsigned short i, unsigned short j) { return v[i][j]; }
  const su2double& operator()(unsigned short i, unsigned short j) const { return v[i][j]; }
};

/*!
 * \brief Entropy variables w of U = -rho s/(gamma-1), s = ln p - gamma ln rho.
 */
inline void EntropyVariables(unsigned short nDim, su2double gamma, su2double density, const su2double* velocity,
                             su2double pressure, su2double* w) {
  const su2double beta = density / pressure;
  const su2double entropy = log(pressure) - gamma * log(density);
  su2double sqVel = 0.0;
  for (unsigned short iDim = 0; iDim < nDim; ++iDim) {
    sqVel += velocity[iDim] * velocity[iDim];
    w[iDim + 1] = beta * velocity[iDim];
  }
  w[0] = (gamma - entropy) / (gamma - 1.0) - 0.5 * beta * sqVel;
  w[nDim + 1] = -beta;
}

/*!
 * \brief dw/dU of those variables (the Hessian of U, symmetric positive definite), ideal gas:
 *        dw/dU = c0 z z^T + B,  c0 = (gamma-1) rho/p^2,  z = (-|u|^2/2, u, -1),
 *        B(0,0) = gamma/((gamma-1) rho), B(0,E) = B(E,0) = -1/p, B(m,m) = I/p, zero otherwise.
 * \note Templated on the scalar (su2double or a SIMD lane array) and on any matrix with operator()(i,j).
 */
template <class Scalar, class MatrixType>
inline void EntropyHessian(unsigned short nDim, const Scalar& gamma, const Scalar& density, const Scalar* velocity,
                           const Scalar& pressure, MatrixType& H) {
  const unsigned short nVar = nDim + 2;
  Scalar z[MAXNVAR];
  Scalar sqVel = 0.0;
  for (unsigned short iDim = 0; iDim < nDim; ++iDim) {
    sqVel += velocity[iDim] * velocity[iDim];
    z[iDim + 1] = velocity[iDim];
  }
  z[0] = -0.5 * sqVel;
  z[nDim + 1] = -1.0;
  const Scalar invP = 1.0 / pressure;
  const Scalar c0 = (gamma - 1.0) * density * invP * invP;
  for (unsigned short iVar = 0; iVar < nVar; ++iVar) {
    for (unsigned short jVar = 0; jVar < nVar; ++jVar) H(iVar, jVar) = c0 * z[iVar] * z[jVar];
  }
  H(0, 0) += gamma / ((gamma - 1.0) * density);
  H(0, nDim + 1) -= invP;
  H(nDim + 1, 0) -= invP;
  for (unsigned short iDim = 0; iDim < nDim; ++iDim) H(iDim + 1, iDim + 1) += invP;
}

/*!
 * \brief Linear U-MUSCL jump of the entropy variables on one edge,
 *        L = (1 - ramp kappa) dw - 1/2 ramp (1-kappa) (phi_i grad_i + phi_k grad_k).e.
 * \param[in] dw - Nodal jump w_k - w_i.
 * \param[in] grad_i, grad_k - Gradients of w at the two nodes, (iVar, iDim).
 * \param[in] e - Edge vector x_k - x_i.
 */
template <class Scalar, class VectorType, class GradType_i, class GradType_k, class EdgeType, class OutType>
inline void EdgeJump(unsigned short nDim, unsigned short nVar, const VectorType& dw, const GradType_i& grad_i,
                     const GradType_k& grad_k, const Scalar& phi_i, const Scalar& phi_k, const EdgeType& e,
                     su2double ramp, su2double kappa, OutType& L) {
  const su2double coefDelta = 1.0 - ramp * kappa;
  const su2double coefGrad = 0.5 * ramp * (1.0 - kappa);
  for (unsigned short iVar = 0; iVar < nVar; ++iVar) {
    Scalar proj_i = 0.0, proj_k = 0.0;
    for (unsigned short iDim = 0; iDim < nDim; ++iDim) {
      proj_i += grad_i(iVar, iDim) * e(iDim);
      proj_k += grad_k(iVar, iDim) * e(iDim);
    }
    L(iVar) = coefDelta * dw(iVar) - coefGrad * (phi_i * proj_i + phi_k * proj_k);
  }
}

/*!
 * \brief Coefficient gam = -1/2 ramp (1-kappa) of the gradient part of c_e.
 */
inline su2double GradientCoefficient(su2double ramp, su2double kappa) { return -0.5 * ramp * (1.0 - kappa); }

/*!
 * \brief Green-Gauss gradient of w in difference form on the owned nodes, G_i w = V_i^-1 sum_k 1/2 (w_k - w_i) n_ik.
 * \param[in] w - Field (iPoint, iVar), halos included.
 * \param[out] grad - (iPoint, iVar, iDim), owned nodes.
 */
template <class Geometry, class FieldType, class GradientType>
void Gradient(const Geometry& geometry, unsigned short nDim, unsigned short nVar, const FieldType& w,
              GradientType& grad) {
  SU2_OMP_FOR_STAT(OMP_CHUNK)
  for (unsigned long iPoint = 0; iPoint < geometry.GetnPointDomain(); ++iPoint) {
    su2double g[MAXNVAR][3] = {};
    for (auto iEdge : geometry.nodes->GetEdges(iPoint)) {
      const bool first = (geometry.edges->GetNode(iEdge, 0) == iPoint);
      const unsigned long jPoint = geometry.edges->GetNode(iEdge, first ? 1 : 0);
      const su2double sign = first ? 0.5 : -0.5;
      const auto* normal = geometry.edges->GetNormal(iEdge);
      for (unsigned short iVar = 0; iVar < nVar; ++iVar) {
        const su2double delta = sign * (w(jPoint, iVar) - w(iPoint, iVar));
        for (unsigned short iDim = 0; iDim < nDim; ++iDim) g[iVar][iDim] += delta * normal[iDim];
      }
    }
    const su2double invVol = 1.0 / geometry.nodes->GetVolume(iPoint);
    for (unsigned short iVar = 0; iVar < nVar; ++iVar)
      for (unsigned short iDim = 0; iDim < nDim; ++iDim) grad(iPoint, iVar, iDim) = g[iVar][iDim] * invVol;
  }
  END_SU2_OMP_FOR
}

/*!
 * \brief Y_i = (phi_i / V_i) sum_{e ni i} e_e (x) q_e on the owned nodes, e_e = x_{node 1} - x_{node 0} of the edge.
 * \param[in] edgeQ - q_e = D_e L_e of each edge (iEdge, iVar), from the edge loop.
 * \param[in] phi - Nodal weights.
 * \param[out] Y - (iPoint, iVar, iDim).
 */
template <class Geometry, class EdgeType, class PhiType, class MomentType>
void NodalMoment(const Geometry& geometry, unsigned short nDim, unsigned short nVar, const EdgeType& edgeQ,
                 const PhiType& phi, MomentType& Y) {
  SU2_OMP_FOR_STAT(OMP_CHUNK)
  for (unsigned long iPoint = 0; iPoint < geometry.GetnPointDomain(); ++iPoint) {
    su2double z[MAXNVAR][3] = {};
    for (auto iEdge : geometry.nodes->GetEdges(iPoint)) {
      const auto* x0 = geometry.nodes->GetCoord(geometry.edges->GetNode(iEdge, 0));
      const auto* x1 = geometry.nodes->GetCoord(geometry.edges->GetNode(iEdge, 1));
      for (unsigned short iVar = 0; iVar < nVar; ++iVar) {
        const su2double q = edgeQ(iEdge, iVar);
        for (unsigned short iDim = 0; iDim < nDim; ++iDim) z[iVar][iDim] += (x1[iDim] - x0[iDim]) * q;
      }
    }
    const su2double scale = phi(iPoint) / geometry.nodes->GetVolume(iPoint);
    for (unsigned short iVar = 0; iVar < nVar; ++iVar)
      for (unsigned short iDim = 0; iDim < nDim; ++iDim) Y(iPoint, iVar, iDim) = scale * z[iVar][iDim];
  }
  END_SU2_OMP_FOR
}

/*!
 * \brief P1 average gradient (TECNO_ADJOINT_GRADIENT= P1_AVERAGE) on the owned nodes,
 *        G_i w = Vhat_i^-1 sum_k a_ik (w_k - w_i).
 * \param[in] weights - a of each edge (iEdge, 2 nDim), see above.
 * \param[in] volume - Vhat (iPoint).
 */
template <class Geometry, class WeightType, class VolumeType, class FieldType, class GradientType>
void GradientP1(const Geometry& geometry, unsigned short nDim, unsigned short nVar, const WeightType& weights,
                const VolumeType& volume, const FieldType& w, GradientType& grad) {
  SU2_OMP_FOR_STAT(OMP_CHUNK)
  for (unsigned long iPoint = 0; iPoint < geometry.GetnPointDomain(); ++iPoint) {
    su2double g[MAXNVAR][3] = {};
    for (auto iEdge : geometry.nodes->GetEdges(iPoint)) {
      const bool first = (geometry.edges->GetNode(iEdge, 0) == iPoint);
      const unsigned long jPoint = geometry.edges->GetNode(iEdge, first ? 1 : 0);
      const unsigned short offset = first ? 0 : nDim;
      for (unsigned short iVar = 0; iVar < nVar; ++iVar) {
        const su2double delta = w(jPoint, iVar) - w(iPoint, iVar);
        for (unsigned short iDim = 0; iDim < nDim; ++iDim) g[iVar][iDim] += delta * weights(iEdge, offset + iDim);
      }
    }
    const su2double invVol = 1.0 / volume(iPoint);
    for (unsigned short iVar = 0; iVar < nVar; ++iVar)
      for (unsigned short iDim = 0; iDim < nDim; ++iDim) grad(iPoint, iVar, iDim) = g[iVar][iDim] * invVol;
  }
  END_SU2_OMP_FOR
}

/*!
 * \brief NodalMoment with the volumes Vhat of the P1 average, Y_i = (phi_i / Vhat_i) sum_{e ni i} e_e (x) q_e.
 */
template <class Geometry, class EdgeType, class PhiType, class VolumeType, class MomentType>
void NodalMomentP1(const Geometry& geometry, unsigned short nDim, unsigned short nVar, const EdgeType& edgeQ,
                   const PhiType& phi, const VolumeType& volume, MomentType& Y) {
  SU2_OMP_FOR_STAT(OMP_CHUNK)
  for (unsigned long iPoint = 0; iPoint < geometry.GetnPointDomain(); ++iPoint) {
    su2double z[MAXNVAR][3] = {};
    for (auto iEdge : geometry.nodes->GetEdges(iPoint)) {
      const auto* x0 = geometry.nodes->GetCoord(geometry.edges->GetNode(iEdge, 0));
      const auto* x1 = geometry.nodes->GetCoord(geometry.edges->GetNode(iEdge, 1));
      for (unsigned short iVar = 0; iVar < nVar; ++iVar) {
        const su2double q = edgeQ(iEdge, iVar);
        for (unsigned short iDim = 0; iDim < nDim; ++iDim) z[iVar][iDim] += (x1[iDim] - x0[iDim]) * q;
      }
    }
    const su2double scale = phi(iPoint) / volume(iPoint);
    for (unsigned short iVar = 0; iVar < nVar; ++iVar)
      for (unsigned short iDim = 0; iDim < nDim; ++iDim) Y(iPoint, iVar, iDim) = scale * z[iVar][iDim];
  }
  END_SU2_OMP_FOR
}

/*!
 * \brief Gradient part of the residual for the P1 average, the transpose of GradientP1 applied to Y:
 *        R_m += gam sum_k (a_km.Y_k - a_mk.Y_m) on the owned nodes, Y needed on the halos too.
 */
template <class Geometry, class WeightType, class MomentType, class ResidualType>
void AddTransposeP1(const Geometry& geometry, unsigned short nDim, unsigned short nVar, const WeightType& weights,
                    const MomentType& Y, su2double gam, ResidualType& residual) {
  SU2_OMP_FOR_STAT(OMP_CHUNK)
  for (unsigned long iPoint = 0; iPoint < geometry.GetnPointDomain(); ++iPoint) {
    su2double res[MAXNVAR] = {};
    for (auto iEdge : geometry.nodes->GetEdges(iPoint)) {
      const bool first = (geometry.edges->GetNode(iEdge, 0) == iPoint);
      const unsigned long jPoint = geometry.edges->GetNode(iEdge, first ? 1 : 0);
      const unsigned short own = first ? 0 : nDim;   /*--- a_mk, m = iPoint. ---*/
      const unsigned short other = first ? nDim : 0; /*--- a_km. ---*/
      for (unsigned short iVar = 0; iVar < nVar; ++iVar) {
        su2double flux = 0.0;
        for (unsigned short iDim = 0; iDim < nDim; ++iDim)
          flux += weights(iEdge, other + iDim) * Y(jPoint, iVar, iDim) - weights(iEdge, own + iDim) * Y(iPoint, iVar, iDim);
        res[iVar] += gam * flux;
      }
    }
    for (unsigned short iVar = 0; iVar < nVar; ++iVar) residual(iPoint, iVar) += res[iVar];
  }
  END_SU2_OMP_FOR
}

/*!
 * \brief Gradient part of the residual, the transpose of Gradient applied to Y:
 *        R_m += -gam/2 sum_k n_mk.(Y_m + Y_k) on the owned nodes, Y needed on the halos too.
 */
template <class Geometry, class MomentType, class ResidualType>
void AddTranspose(const Geometry& geometry, unsigned short nDim, unsigned short nVar, const MomentType& Y,
                  su2double gam, ResidualType& residual) {
  SU2_OMP_FOR_STAT(OMP_CHUNK)
  for (unsigned long iPoint = 0; iPoint < geometry.GetnPointDomain(); ++iPoint) {
    su2double res[MAXNVAR] = {};
    for (auto iEdge : geometry.nodes->GetEdges(iPoint)) {
      const bool first = (geometry.edges->GetNode(iEdge, 0) == iPoint);
      const unsigned long jPoint = geometry.edges->GetNode(iEdge, first ? 1 : 0);
      const su2double sign = first ? -0.5 * gam : 0.5 * gam;
      const auto* normal = geometry.edges->GetNormal(iEdge);
      for (unsigned short iVar = 0; iVar < nVar; ++iVar) {
        su2double flux = 0.0;
        for (unsigned short iDim = 0; iDim < nDim; ++iDim)
          flux += normal[iDim] * (Y(iPoint, iVar, iDim) + Y(jPoint, iVar, iDim));
        res[iVar] += sign * flux;
      }
    }
    for (unsigned short iVar = 0; iVar < nVar; ++iVar) residual(iPoint, iVar) += res[iVar];
  }
  END_SU2_OMP_FOR
}

/*--- Lemma A4 with a three-term split (P1_TECNO_CONVERGENCE.md, 3.2, adapted to a halo of one layer). Write
 * L_e = a_e + p_i + p_k, a_e = (1 - rho kappa) dw, p_i = gam phi_i sum_m (g_im.e) w_m, g_im = dG_i/dw_m. Then
 *   L^T D L <= 3 (a^T D a + p_i^T D p_i + p_k^T D p_k),
 *   p_i^T D p_i <= gam^2 phi_i^2 |b|_1 sum_m |b_m| w_m^T D w_m,  b_m = g_im.e   (Jensen, D PSD),
 *   |b|_1 <= |e| N_i / V_i,  |b_m| <= |e| |g_im|,  N_i = sum_k |n_ik|/2 + |sum_k n_ik|/2,
 * so that, summed over the edges, C^T D C <= 2 M#, with M# = 3/2 [sum_e a-blocks of D_e + diag_m(sum_i |g_im| P_i)],
 *   P_i = gam^2 phi_i^2 (N_i / V_i) S_i,  S_i = sum_{e ni i} |e|^2 D_e.
 * The a-blocks are the edge blocks (1-rho kappa)^2 [D, -D; -D, D] (edge loop). With Phat_i = P_i / V_i (owned nodes,
 * copied to the halos), the diagonal block of an owned node m is
 *   3/2 [sum_k |n_mk|/2 Phat_k + |sum_k n_mk|/2 Phat_m],
 * which needs only first neighbours. Lemma C4 (ii) then holds for the iteration with M# as the dissipation part. ---*/

/*!
 * \brief Phat_i = gam^2 phi_i^2 N_i S_i / V_i^2 on the owned nodes.
 * \param[in] edgeD - D_e of each edge, row major nVar x nVar per row (iEdge, iVar*nVar + jVar).
 * \param[out] P - (iPoint, iVar*nVar + jVar).
 */
template <class Geometry, class EdgeType, class PhiType, class NodeType>
void MajorantNodal(const Geometry& geometry, unsigned short nDim, unsigned short nVar, const EdgeType& edgeD,
                   const PhiType& phi, su2double gam, NodeType& P) {
  SU2_OMP_FOR_STAT(OMP_CHUNK)
  for (unsigned long iPoint = 0; iPoint < geometry.GetnPointDomain(); ++iPoint) {
    su2double S[MAXNVAR * MAXNVAR] = {};
    su2double sumNormal[3] = {}, sumArea = 0.0;
    for (auto iEdge : geometry.nodes->GetEdges(iPoint)) {
      const bool first = (geometry.edges->GetNode(iEdge, 0) == iPoint);
      const auto* x0 = geometry.nodes->GetCoord(geometry.edges->GetNode(iEdge, 0));
      const auto* x1 = geometry.nodes->GetCoord(geometry.edges->GetNode(iEdge, 1));
      const auto* normal = geometry.edges->GetNormal(iEdge);
      su2double length2 = 0.0, area2 = 0.0;
      for (unsigned short iDim = 0; iDim < nDim; ++iDim) {
        length2 += pow(x1[iDim] - x0[iDim], 2);
        area2 += normal[iDim] * normal[iDim];
        sumNormal[iDim] += (first ? 1.0 : -1.0) * normal[iDim];
      }
      sumArea += sqrt(area2);
      for (unsigned short k = 0; k < nVar * nVar; ++k) S[k] += length2 * edgeD(iEdge, k);
    }
    su2double normSum = 0.0;
    for (unsigned short iDim = 0; iDim < nDim; ++iDim) normSum += sumNormal[iDim] * sumNormal[iDim];
    const su2double N = 0.5 * sumArea + 0.5 * sqrt(normSum);
    const su2double vol = geometry.nodes->GetVolume(iPoint);
    const su2double coef = gam * gam * phi(iPoint) * phi(iPoint) * N / (vol * vol);
    for (unsigned short k = 0; k < nVar * nVar; ++k) P(iPoint, k) = coef * S[k];
  }
  END_SU2_OMP_FOR
}

/*!
 * \brief Diagonal block of M# (in w) of the owned node iPoint from Phat (needed on the halos), the factor 3/2 included.
 */
template <class Geometry, class NodeType, class MatrixType>
void MajorantDiagonal(const Geometry& geometry, unsigned short nDim, unsigned short nVar, const NodeType& P,
                      unsigned long iPoint, MatrixType& block) {
  for (unsigned short iVar = 0; iVar < nVar; ++iVar)
    for (unsigned short jVar = 0; jVar < nVar; ++jVar) block(iVar, jVar) = 0.0;
  su2double sumNormal[3] = {};
  for (auto iEdge : geometry.nodes->GetEdges(iPoint)) {
    const bool first = (geometry.edges->GetNode(iEdge, 0) == iPoint);
    const unsigned long jPoint = geometry.edges->GetNode(iEdge, first ? 1 : 0);
    const auto* normal = geometry.edges->GetNormal(iEdge);
    su2double area2 = 0.0;
    for (unsigned short iDim = 0; iDim < nDim; ++iDim) {
      area2 += normal[iDim] * normal[iDim];
      sumNormal[iDim] += (first ? 1.0 : -1.0) * normal[iDim];
    }
    const su2double weight = 0.75 * sqrt(area2);
    for (unsigned short iVar = 0; iVar < nVar; ++iVar)
      for (unsigned short jVar = 0; jVar < nVar; ++jVar) block(iVar, jVar) += weight * P(jPoint, iVar * nVar + jVar);
  }
  su2double normSum = 0.0;
  for (unsigned short iDim = 0; iDim < nDim; ++iDim) normSum += sumNormal[iDim] * sumNormal[iDim];
  const su2double weight = 0.75 * sqrt(normSum);
  for (unsigned short iVar = 0; iVar < nVar; ++iVar)
    for (unsigned short jVar = 0; jVar < nVar; ++jVar) block(iVar, jVar) += weight * P(iPoint, iVar * nVar + jVar);
}

/*--- TECNO_ADJOINT_LIMITER= EDGE_ENTROPY_VA (P1_TECNO_ADJ_LIMITER.md): per edge end a and characteristic entropy field j,
 *   phi_aj = (Delta_j^2 + alpha_aj Delta_j + tau_j) / (alpha_aj^2 + Delta_j^2 + tau_j),
 * Delta_j = (R^T (w_k - w_i))_j, alpha_aj = rho ((1-kappa) (R^T G_a w . e)_j + kappa Delta_j) the U-MUSCL slope, the shear
 * block as one field with inner products, tau_j = (eps S_j)^2 the roundoff floor. Unclipped: Theorem A1 holds for any
 * values because the residual applies the transpose B^T q = R Phi R^-1 q per edge end (L1). alpha = Delta and
 * Delta = alpha = 0 give phi = 1 exactly. The edge store then holds B^T q of end 0 in [0, nVar) and of end 1 in
 * [nVar, 2 nVar), the moment loop below takes the end of each node. ---*/

/*!
 * \brief Van Albada algebra of EDGE_ENTROPY_VA from the inner products dd = Delta.Delta, ad = alpha.Delta,
 *        aa = alpha.alpha (scalars or the shear block), guard tau > 0.
 */
template <class Scalar>
inline Scalar EdgeVanAlbada(const Scalar& dd, const Scalar& ad, const Scalar& aa, const Scalar& tau) {
  return (dd + ad + tau) / (aa + dd + tau);
}

/*!
 * \brief Y_a = V_a^-1 sum_{e ni a} e_e (x) (B^T q)_{e,a} on the owned nodes, (B^T q)_{e,a} read from the half of the
 *        edge store of the end a (EDGE_ENTROPY_VA). volume(i): the volumes of the gradient (V or Vhat).
 *        Not inlined, to keep the code generation of the default moment loops of the caller.
 */
template <class Geometry, class EdgeType, class VolumeType, class MomentType>
NEVERINLINE void NodalMomentEnds(const Geometry& geometry, unsigned short nDim, unsigned short nVar, const EdgeType& edgeQ,
                     const VolumeType& volume, MomentType& Y) {
  SU2_OMP_FOR_STAT(OMP_CHUNK)
  for (unsigned long iPoint = 0; iPoint < geometry.GetnPointDomain(); ++iPoint) {
    su2double z[MAXNVAR][3] = {};
    for (auto iEdge : geometry.nodes->GetEdges(iPoint)) {
      const unsigned short offset = (geometry.edges->GetNode(iEdge, 0) == iPoint) ? 0 : nVar;
      const auto* x0 = geometry.nodes->GetCoord(geometry.edges->GetNode(iEdge, 0));
      const auto* x1 = geometry.nodes->GetCoord(geometry.edges->GetNode(iEdge, 1));
      for (unsigned short iVar = 0; iVar < nVar; ++iVar) {
        const su2double q = edgeQ(iEdge, offset + iVar);
        for (unsigned short iDim = 0; iDim < nDim; ++iDim) z[iVar][iDim] += (x1[iDim] - x0[iDim]) * q;
      }
    }
    const su2double scale = 1.0 / volume(iPoint);
    for (unsigned short iVar = 0; iVar < nVar; ++iVar)
      for (unsigned short iDim = 0; iDim < nDim; ++iDim) Y(iPoint, iVar, iDim) = scale * z[iVar][iDim];
  }
  END_SU2_OMP_FOR
}

/*--- Lemma A4 for the P1 average (P1_TECNO_ADJ_GRADIENT.md, G3): g_im = a_im / Vhat_i (m != i), g_ii = -sum_k a_ik / Vhat_i,
 * so N_i = sum_k |a_ik| + |sum_k a_ik| and the diagonal block of m is 3/2 [sum_k |a_km| Phat_k + |sum_k a_mk| Phat_m],
 * Phat_i = gam^2 phi_i^2 N_i S_i / Vhat_i^2 (Green-Gauss: |a| = |n|/2). ---*/

/*!
 * \brief Phat_i of the P1 average on the owned nodes (MajorantNodal with the weights a and volumes Vhat).
 */
template <class Geometry, class WeightType, class VolumeType, class EdgeType, class PhiType, class NodeType>
void MajorantNodalP1(const Geometry& geometry, unsigned short nDim, unsigned short nVar, const WeightType& weights,
                     const VolumeType& volume, const EdgeType& edgeD, const PhiType& phi, su2double gam, NodeType& P) {
  SU2_OMP_FOR_STAT(OMP_CHUNK)
  for (unsigned long iPoint = 0; iPoint < geometry.GetnPointDomain(); ++iPoint) {
    su2double S[MAXNVAR * MAXNVAR] = {};
    su2double sumWeight[3] = {}, sumNorm = 0.0;
    for (auto iEdge : geometry.nodes->GetEdges(iPoint)) {
      const bool first = (geometry.edges->GetNode(iEdge, 0) == iPoint);
      const unsigned short own = first ? 0 : nDim;
      const auto* x0 = geometry.nodes->GetCoord(geometry.edges->GetNode(iEdge, 0));
      const auto* x1 = geometry.nodes->GetCoord(geometry.edges->GetNode(iEdge, 1));
      su2double length2 = 0.0, norm2 = 0.0;
      for (unsigned short iDim = 0; iDim < nDim; ++iDim) {
        length2 += pow(x1[iDim] - x0[iDim], 2);
        norm2 += pow(weights(iEdge, own + iDim), 2);
        sumWeight[iDim] += weights(iEdge, own + iDim);
      }
      sumNorm += sqrt(norm2);
      for (unsigned short k = 0; k < nVar * nVar; ++k) S[k] += length2 * edgeD(iEdge, k);
    }
    su2double normSum = 0.0;
    for (unsigned short iDim = 0; iDim < nDim; ++iDim) normSum += sumWeight[iDim] * sumWeight[iDim];
    const su2double N = sumNorm + sqrt(normSum);
    const su2double vol = volume(iPoint);
    const su2double coef = gam * gam * phi(iPoint) * phi(iPoint) * N / (vol * vol);
    for (unsigned short k = 0; k < nVar * nVar; ++k) P(iPoint, k) = coef * S[k];
  }
  END_SU2_OMP_FOR
}

/*!
 * \brief Diagonal block of M# (in w) of the owned node iPoint for the P1 average, Phat needed on the halos, 3/2 included.
 */
template <class Geometry, class WeightType, class NodeType, class MatrixType>
void MajorantDiagonalP1(const Geometry& geometry, unsigned short nDim, unsigned short nVar, const WeightType& weights,
                        const NodeType& P, unsigned long iPoint, MatrixType& block) {
  for (unsigned short iVar = 0; iVar < nVar; ++iVar)
    for (unsigned short jVar = 0; jVar < nVar; ++jVar) block(iVar, jVar) = 0.0;
  su2double sumWeight[3] = {};
  for (auto iEdge : geometry.nodes->GetEdges(iPoint)) {
    const bool first = (geometry.edges->GetNode(iEdge, 0) == iPoint);
    const unsigned long jPoint = geometry.edges->GetNode(iEdge, first ? 1 : 0);
    const unsigned short own = first ? 0 : nDim;
    const unsigned short other = first ? nDim : 0;
    su2double norm2 = 0.0;
    for (unsigned short iDim = 0; iDim < nDim; ++iDim) {
      norm2 += pow(weights(iEdge, other + iDim), 2);
      sumWeight[iDim] += weights(iEdge, own + iDim);
    }
    const su2double weight = 1.5 * sqrt(norm2);
    for (unsigned short iVar = 0; iVar < nVar; ++iVar)
      for (unsigned short jVar = 0; jVar < nVar; ++jVar) block(iVar, jVar) += weight * P(jPoint, iVar * nVar + jVar);
  }
  su2double normSum = 0.0;
  for (unsigned short iDim = 0; iDim < nDim; ++iDim) normSum += sumWeight[iDim] * sumWeight[iDim];
  const su2double weight = 1.5 * sqrt(normSum);
  for (unsigned short iVar = 0; iVar < nVar; ++iVar)
    for (unsigned short jVar = 0; jVar < nVar; ++jVar) block(iVar, jVar) += weight * P(iPoint, iVar * nVar + jVar);
}

/*--- TECNO_ADJOINT_JACOBIAN= FIRST_RING (ROE_TECNO_PLAN.md M4, J2): for frozen D_e and phi the dissipation has the
 * linearization K = sum_e beta_e beta_e^T (x) D_e in w, beta_{e,m} the scalar coefficients of L_e = sum_m beta_{e,m} w_m,
 *   beta_{e,m} = (1 - rho kappa)(delta_{m,1} - delta_{m,0}) + gam [phi_0 g_{0m}.e + phi_1 g_{1m}.e],  g_im = dG_i/dw_m,
 * (nodes 0, 1 of the edge, e = x_1 - x_0, gam = -1/2 rho (1-kappa)). Split beta_e = a_e + b_e with a_e the entries at the
 * two ends (Delta part and the gradient entries g_00, g_01, g_10, g_11) and b_e = b_e^0 + b_e^1 the entries at the other
 * stencil nodes, b^x_m = gam phi_x (a_xm.e)/V_x for m in N(x) (g_xm = a_xm/V_x). Young (eps = 1) twice:
 *   beta beta^T <= 2 a a^T + 2 b b^T,   b b^T <= 2 (b^0 b^0^T + b^1 b^1^T),
 * and Cauchy-Schwarz with D PSD and the weights c_m = |a_xm| (direction-aware, exact for one active neighbour):
 *   b^x b^x^T (x) D <= (sum_m (b^x_m)^2 / |a_xm|) diag_m(|a_xm|) (x) D,  sum_m (b^x_m)^2/|a_xm| <= gam^2 phi_x^2 V_x^-2 e^T T_x e,
 * T_x = sum_k a_xk a_xk^T / |a_xk| (a nodal tensor; on stretched cells e^T T e follows the cell shape, unlike |e|^2 |a|).
 * So K <= 2 J with
 *   J = sum_e a_e a_e^T (x) D_e  +  diag_m( sum_{i in N(m)} |a_im| P_i ),   P_i = 2 gam^2 phi_i^2 V_i^-2 sum_{e ni i} (e^T T_i e) D_e,
 * whose blocks are first neighbours only: the edge blocks of a_e a_e^T exactly (first-ring part of the consistent
 * linearization) and a diagonal majorant of the rest. g_ii = -s_i, s_i = V_i^-1 sum_k a_ik: the weights a of
 * TECNO_ADJOINT_GRADIENT (P1 average: the stored a, V = Vhat; Green-Gauss: a_{0,1} = n/2, a_{1,0} = -n/2, V the dual
 * volume). MPI: S = (s, 1/V, T) and P on the owned nodes, copied to the halos; rows of owned nodes. ---*/

/*!
 * \brief Edge weights of the Green-Gauss gradient in the layout of the P1 average weights: a_{0,1} = n/2 in columns
 *        [0, nDim), a_{1,0} = -n/2 in [nDim, 2 nDim).
 */
template <class Geometry>
struct GreenGaussWeights {
  const Geometry& geometry;
  unsigned short nDim;
  su2double operator()(unsigned long iEdge, unsigned short col) const {
    const auto* normal = geometry.edges->GetNormal(iEdge);
    return (col < nDim) ? 0.5 * normal[col] : -0.5 * normal[col - nDim];
  }
};

/*!
 * \brief S(i, 0:nDim) = s_i, S(i, nDim) = 1/V_i, S(i, nDim+1 + nDim*d + d') = T_i(d, d') on the owned nodes (FIRST_RING),
 *        width nDim + 1 + nDim^2.
 */
template <class Geometry, class WeightType, class VolumeType, class NodeType>
void FirstRingNodal(const Geometry& geometry, unsigned short nDim, const WeightType& weights, const VolumeType& volume,
                    NodeType& S) {
  SU2_OMP_FOR_STAT(OMP_CHUNK)
  for (unsigned long iPoint = 0; iPoint < geometry.GetnPointDomain(); ++iPoint) {
    su2double sum[3] = {}, T[3][3] = {};
    for (auto iEdge : geometry.nodes->GetEdges(iPoint)) {
      const unsigned short own = (geometry.edges->GetNode(iEdge, 0) == iPoint) ? 0 : nDim;
      su2double a[3] = {}, norm2 = 0.0;
      for (unsigned short iDim = 0; iDim < nDim; ++iDim) {
        a[iDim] = weights(iEdge, own + iDim);
        sum[iDim] += a[iDim];
        norm2 += a[iDim] * a[iDim];
      }
      if (norm2 == 0.0) continue;
      const su2double invNorm = 1.0 / sqrt(norm2);
      for (unsigned short iDim = 0; iDim < nDim; ++iDim)
        for (unsigned short jDim = 0; jDim < nDim; ++jDim) T[iDim][jDim] += a[iDim] * a[jDim] * invNorm;
    }
    const su2double invVol = 1.0 / volume(iPoint);
    for (unsigned short iDim = 0; iDim < nDim; ++iDim) S(iPoint, iDim) = sum[iDim] * invVol;
    S(iPoint, nDim) = invVol;
    for (unsigned short iDim = 0; iDim < nDim; ++iDim)
      for (unsigned short jDim = 0; jDim < nDim; ++jDim) S(iPoint, nDim + 1 + nDim * iDim + jDim) = T[iDim][jDim];
  }
  END_SU2_OMP_FOR
}

/*!
 * \brief beta of the two ends of an edge (FIRST_RING), S and phi needed at both nodes.
 */
template <class Geometry, class WeightType, class NodeType, class PhiType>
void FirstRingBeta(const Geometry& geometry, unsigned short nDim, const WeightType& weights, const NodeType& S,
                   const PhiType& phi, su2double coefDelta, su2double gam, unsigned long iEdge, su2double& beta0,
                   su2double& beta1) {
  const unsigned long i0 = geometry.edges->GetNode(iEdge, 0), i1 = geometry.edges->GetNode(iEdge, 1);
  const auto* x0 = geometry.nodes->GetCoord(i0);
  const auto* x1 = geometry.nodes->GetCoord(i1);
  su2double g00 = 0.0, g01 = 0.0, g10 = 0.0, g11 = 0.0;
  for (unsigned short iDim = 0; iDim < nDim; ++iDim) {
    const su2double e = x1[iDim] - x0[iDim];
    g00 -= S(i0, iDim) * e;
    g11 -= S(i1, iDim) * e;
    g01 += weights(iEdge, iDim) * S(i0, nDim) * e;
    g10 += weights(iEdge, nDim + iDim) * S(i1, nDim) * e;
  }
  beta0 = -coefDelta + gam * (phi(i0) * g00 + phi(i1) * g10);
  beta1 = coefDelta + gam * (phi(i0) * g01 + phi(i1) * g11);
}

/*!
 * \brief P_i = 2 gam^2 phi_i^2 V_i^-2 sum_{e ni i} (e^T T_i e) D_e on the owned nodes (FIRST_RING).
 * \param[in] edgeD - D_e of each edge, row major (iEdge, iVar*nVar + jVar).
 */
template <class Geometry, class EdgeType, class NodeType, class PhiType, class NodeTypeP>
void FirstRingNodalP(const Geometry& geometry, unsigned short nDim, unsigned short nVar, const EdgeType& edgeD,
                     const NodeType& S, const PhiType& phi, su2double gam, NodeTypeP& P) {
  SU2_OMP_FOR_STAT(OMP_CHUNK)
  for (unsigned long iPoint = 0; iPoint < geometry.GetnPointDomain(); ++iPoint) {
    su2double sum[MAXNVAR * MAXNVAR] = {};
    for (auto iEdge : geometry.nodes->GetEdges(iPoint)) {
      const auto* x0 = geometry.nodes->GetCoord(geometry.edges->GetNode(iEdge, 0));
      const auto* x1 = geometry.nodes->GetCoord(geometry.edges->GetNode(iEdge, 1));
      su2double eTe = 0.0;
      for (unsigned short iDim = 0; iDim < nDim; ++iDim)
        for (unsigned short jDim = 0; jDim < nDim; ++jDim)
          eTe += (x1[iDim] - x0[iDim]) * S(iPoint, nDim + 1 + nDim * iDim + jDim) * (x1[jDim] - x0[jDim]);
      for (unsigned short k = 0; k < nVar * nVar; ++k) sum[k] += eTe * edgeD(iEdge, k);
    }
    const su2double coef = 2.0 * pow(gam * phi(iPoint) * S(iPoint, nDim), 2);
    for (unsigned short k = 0; k < nVar * nVar; ++k) P(iPoint, k) = coef * sum[k];
  }
  END_SU2_OMP_FOR
}

/*!
 * \brief Row of the owned node m of J (in w, FIRST_RING): diag = sum_{e ni m} beta_{e,m}^2 D_e + sum_{k in N(m)} |a_km| P_k,
 *        and for each edge (m, k) the block beta_{e,m} beta_{e,k} D_e, passed to offBlock(k, block). S, phi, P on halos.
 */
template <class Geometry, class WeightType, class NodeType, class NodeTypeP, class EdgeType, class PhiType,
          class OffFunc>
void FirstRingRow(const Geometry& geometry, unsigned short nDim, unsigned short nVar, const WeightType& weights,
                  const NodeType& S, const NodeTypeP& P, const EdgeType& edgeD, const PhiType& phi, su2double coefDelta,
                  su2double gam, unsigned long iPoint, Block& diag, OffFunc&& offBlock) {
  for (unsigned short iVar = 0; iVar < nVar; ++iVar)
    for (unsigned short jVar = 0; jVar < nVar; ++jVar) diag(iVar, jVar) = 0.0;
  for (auto iEdge : geometry.nodes->GetEdges(iPoint)) {
    const bool first = (geometry.edges->GetNode(iEdge, 0) == iPoint);
    const unsigned long jPoint = geometry.edges->GetNode(iEdge, first ? 1 : 0);
    su2double beta0, beta1;
    FirstRingBeta(geometry, nDim, weights, S, phi, coefDelta, gam, iEdge, beta0, beta1);
    const su2double bm = first ? beta0 : beta1, bk = first ? beta1 : beta0;
    const unsigned short other = first ? nDim : 0; /*--- a_km, k = jPoint. ---*/
    su2double norm2 = 0.0;
    for (unsigned short iDim = 0; iDim < nDim; ++iDim) norm2 += pow(weights(iEdge, other + iDim), 2);
    const su2double aNorm = sqrt(norm2);
    Block off;
    for (unsigned short iVar = 0; iVar < nVar; ++iVar) {
      for (unsigned short jVar = 0; jVar < nVar; ++jVar) {
        const su2double d = edgeD(iEdge, iVar * nVar + jVar);
        diag(iVar, jVar) += bm * bm * d + aNorm * P(jPoint, iVar * nVar + jVar);
        off(iVar, jVar) = bm * bk * d;
      }
    }
    offBlock(jPoint, off);
  }
}

/*--- TECNO_ADJOINT_JACOBIAN= FIRST_RING_JUMPS (ROE_TECNO_PLAN.md 6.4): a first-ring majorant written in edge jumps, so
 * that J 1 = 0 as K 1 = 0 (FIRST_RING bounds the second-ring part by a diagonal, J 1 != 0, which acts like an O(1) CFL
 * pseudo-time term on all modes). With gamma_x = gam phi_x / V_x and the difference form of the gradient,
 *   L_e = alpha_e Delta_e + u_0 + u_1,   alpha_e = coefDelta + gamma_0 a_01.e - gamma_1 a_10.e,
 *   u_x = gamma_x sum_{m in N(x), m != x'} (a_xm.e)(w_m - w_x)        (x' the other end, e = x_1 - x_0),
 * all terms jumps over mesh edges. Pair weights (any positive values are admissible): mu_xm = sqrt(a_xm^T E_x a_xm),
 * E_x = sum_{f ni x} w_f f f^T / sum_f w_f, the w_f-weighted quadratic mean of |a_xm.f| over the edges f of x (the
 * per-edge optimum of the Cauchy-Schwarz weight is |a_xm.e|), with w_f = trace(D_f H_f) >= 0 the mean of
 * delta^T D_f delta over entropy-normalized jumps (E[delta delta^T] = H_f = dw/dU, mean of the two nodes); uniform
 * weights if all w_f = 0. If E_x a_xm = 0 then a_xm.f = 0 on every edge with w_f > 0, i.e. D_f != 0, so mu_xm = 0 only
 * where the pair carries no term. Cauchy-Schwarz with D PSD inside u_x, then the three-term weighted Cauchy-Schwarz
 * with t_0 = |alpha|, t_x = |gamma_x| sqrt(tau nu):
 *   L^T D L <= S_e [ |alpha| Delta^T D Delta + sum_x r_x sum_{m != x'} mu_xm delta_xm^T D delta_xm ],
 *   tau_x = e^T T_x e - (a_xx'.e)^2/mu_xx',  nu_x = M_x - mu_xx',  T_x = sum_m a_xm a_xm^T/mu_xm,  M_x = sum_m mu_xm,
 *   S_e = |alpha| + t_0 + t_1,  r_x = |gamma_x| sqrt(tau_x/nu_x)   (t_x = 0 means u_x = 0: no term).
 * Regrouped by mesh edges f = (m, j): K <= J = sum_f W_f (x) (e_m - e_j)(e_m - e_j)^T,
 *   W_f = S_f |alpha_f| D_f + mu_mj (Q_m - S_f r_m^f D_f) + mu_jm (Q_j - S_f r_j^f D_f),   Q_x = sum_{e ni x} S_e r_x^e D_e,
 * so J >= K, J symmetric, J 1 = 0, first-neighbour blocks only. J is the bound itself (not B/2): C4 (ii) (sufficient,
 * with its other hypotheses) holds with the margin 2 (S + J) - K >= J + 2 S, i.e. lambda(K, J + S) <= 1 for any S > 0; with J = B/2 only 2 J >= K
 * holds, lambda reaches 2 where the bound is tight (model T4 225x65: 1.33 P1, 1.99 GG; FIRST_RING 0.98), the margin is
 * the pseudo-time term alone, and T4 a10 at CFL 1000 diverged (ROE_TECNO_PLAN.md 6.4). The exclusions of the own pair in tau and
 * nu are subtractions (without them the model needs 1.4-1.8 x more iterations); their rounding error is at most
 * (n_x + 2) eps (e^T T e + own) and (n_x + 2) eps M (n_x terms in T_x, M_x, one product, one subtraction), and that
 * margin is added, so tau and nu are not below their exact values and the bound holds in floating point (the sqrt in
 * t and r turns the margin into a relative O(sqrt(n eps)) excess where the exact tau vanishes). MPI: S = (1/V, E, T, M) and Q on the owned nodes, copied to the halos; mu, tau of a halo end j from E_j,
 * T_j, M_j and the edge weights a_jm (complete on the edges of owned nodes). ---*/

/*!
 * \brief S(i, 0) = 1/V_i, S(i, 1 + nDim d + d') = E_i(d, d'), S(i, 1 + nDim^2 + nDim d + d') = T_i(d, d'),
 *        S(i, 1 + 2 nDim^2) = M_i, S(i, 2 + 2 nDim^2) = (n_i + 2) eps (the roundoff margin factor, n_i the number of
 *        neighbours) on the owned nodes (FIRST_RING_JUMPS), width 3 + 2 nDim^2.
 * \param[in] edgeWeight - w_f >= 0 of each edge (callable), trace(D_f H_f) in the solver.
 */
template <class Geometry, class WeightType, class VolumeType, class EdgeWeightType, class NodeType>
void JumpNodal(const Geometry& geometry, unsigned short nDim, const WeightType& weights, const VolumeType& volume,
               const EdgeWeightType& edgeWeight, NodeType& S) {
  const unsigned short offT = 1 + nDim * nDim, offM = 1 + 2 * nDim * nDim;
  SU2_OMP_FOR_STAT(OMP_CHUNK)
  for (unsigned long iPoint = 0; iPoint < geometry.GetnPointDomain(); ++iPoint) {
    su2double E[3][3] = {}, T[3][3] = {}, M = 0.0;
    const auto* xi = geometry.nodes->GetCoord(iPoint);
    su2double sumWeight = 0.0;
    for (auto iEdge : geometry.nodes->GetEdges(iPoint)) sumWeight += edgeWeight(iEdge);
    const bool uniform = !(sumWeight > 0.0);
    if (uniform) sumWeight = su2double(geometry.nodes->GetnPoint(iPoint));
    for (auto iEdge : geometry.nodes->GetEdges(iPoint)) {
      const auto jPoint = geometry.edges->GetNode(iEdge, (geometry.edges->GetNode(iEdge, 0) == iPoint) ? 1 : 0);
      const auto* xj = geometry.nodes->GetCoord(jPoint);
      const su2double w = (uniform ? su2double(1.0) : edgeWeight(iEdge)) / sumWeight;
      for (unsigned short iDim = 0; iDim < nDim; ++iDim)
        for (unsigned short jDim = 0; jDim < nDim; ++jDim)
          E[iDim][jDim] += w * (xj[iDim] - xi[iDim]) * (xj[jDim] - xi[jDim]);
    }
    for (auto iEdge : geometry.nodes->GetEdges(iPoint)) {
      const unsigned short own = (geometry.edges->GetNode(iEdge, 0) == iPoint) ? 0 : nDim;
      su2double a[3] = {}, aEa = 0.0;
      for (unsigned short iDim = 0; iDim < nDim; ++iDim) a[iDim] = weights(iEdge, own + iDim);
      for (unsigned short iDim = 0; iDim < nDim; ++iDim)
        for (unsigned short jDim = 0; jDim < nDim; ++jDim) aEa += a[iDim] * E[iDim][jDim] * a[jDim];
      const su2double mu = sqrt(fmax(aEa, su2double(0.0)));
      M += mu;
      if (mu == 0.0) continue;
      for (unsigned short iDim = 0; iDim < nDim; ++iDim)
        for (unsigned short jDim = 0; jDim < nDim; ++jDim) T[iDim][jDim] += a[iDim] * a[jDim] / mu;
    }
    S(iPoint, 0) = 1.0 / volume(iPoint);
    for (unsigned short iDim = 0; iDim < nDim; ++iDim) {
      for (unsigned short jDim = 0; jDim < nDim; ++jDim) {
        S(iPoint, 1 + nDim * iDim + jDim) = E[iDim][jDim];
        S(iPoint, offT + nDim * iDim + jDim) = T[iDim][jDim];
      }
    }
    S(iPoint, offM) = M;
    S(iPoint, offM + 1) = (geometry.nodes->GetnPoint(iPoint) + 2) * std::numeric_limits<su2double>::epsilon();
  }
  END_SU2_OMP_FOR
}

/*!
 * \brief Edge quantities of FIRST_RING_JUMPS: alpha_e, S_e, r_0, r_1 and the pair weights mu_01 (end 0), mu_10 (end 1).
 *        S and phi needed at both nodes.
 */
template <class Geometry, class WeightType, class NodeType, class PhiType>
void JumpEdge(const Geometry& geometry, unsigned short nDim, const WeightType& weights, const NodeType& S,
              const PhiType& phi, su2double coefDelta, su2double gam, unsigned long iEdge, su2double& alpha,
              su2double& sumT, su2double* r, su2double* mu) {
  const unsigned short offT = 1 + nDim * nDim, offM = 1 + 2 * nDim * nDim;
  const unsigned long node[2] = {geometry.edges->GetNode(iEdge, 0), geometry.edges->GetNode(iEdge, 1)};
  const auto* x0 = geometry.nodes->GetCoord(node[0]);
  const auto* x1 = geometry.nodes->GetCoord(node[1]);
  su2double e[3] = {};
  for (unsigned short iDim = 0; iDim < nDim; ++iDim) e[iDim] = x1[iDim] - x0[iDim];
  su2double t[2] = {}, ae[2] = {};
  for (unsigned short x = 0; x < 2; ++x) {
    const auto p = node[x];
    su2double a[3] = {}, aEa = 0.0, eTe = 0.0;
    for (unsigned short iDim = 0; iDim < nDim; ++iDim) {
      a[iDim] = weights(iEdge, x * nDim + iDim);
      ae[x] += a[iDim] * e[iDim];
    }
    for (unsigned short iDim = 0; iDim < nDim; ++iDim) {
      for (unsigned short jDim = 0; jDim < nDim; ++jDim) {
        aEa += a[iDim] * S(p, 1 + nDim * iDim + jDim) * a[jDim];
        eTe += e[iDim] * S(p, offT + nDim * iDim + jDim) * e[jDim];
      }
    }
    mu[x] = sqrt(fmax(aEa, su2double(0.0)));
    const su2double own = (mu[x] > 0.0) ? ae[x] * ae[x] / mu[x] : su2double(0.0);
    const su2double margin = S(p, offM + 1);
    const su2double tau = fmax(eTe - own, su2double(0.0)) + margin * (eTe + own);
    const su2double nu = fmax(S(p, offM) - mu[x], su2double(0.0)) + margin * S(p, offM);
    const su2double g = fabs(gam * phi(p) * S(p, 0));
    t[x] = g * sqrt(tau * nu);
    r[x] = (nu > 0.0) ? g * sqrt(tau / nu) : su2double(0.0);
  }
  alpha = coefDelta + gam * (phi(node[0]) * S(node[0], 0) * ae[0] - phi(node[1]) * S(node[1], 0) * ae[1]);
  sumT = fabs(alpha) + t[0] + t[1];
}

/*!
 * \brief Q_i = sum_{e ni i} S_e r_i^e D_e on the owned nodes (FIRST_RING_JUMPS).
 * \param[in] edgeD - D_e of each edge, row major (iEdge, iVar*nVar + jVar).
 */
template <class Geometry, class WeightType, class NodeType, class PhiType, class EdgeType, class NodeTypeQ>
void JumpNodalQ(const Geometry& geometry, unsigned short nDim, unsigned short nVar, const WeightType& weights,
                const NodeType& S, const PhiType& phi, su2double coefDelta, su2double gam, const EdgeType& edgeD,
                NodeTypeQ& Q) {
  SU2_OMP_FOR_STAT(OMP_CHUNK)
  for (unsigned long iPoint = 0; iPoint < geometry.GetnPointDomain(); ++iPoint) {
    su2double sum[MAXNVAR * MAXNVAR] = {};
    for (auto iEdge : geometry.nodes->GetEdges(iPoint)) {
      su2double alpha, sumT, r[2], mu[2];
      JumpEdge(geometry, nDim, weights, S, phi, coefDelta, gam, iEdge, alpha, sumT, r, mu);
      const su2double coef = sumT * r[(geometry.edges->GetNode(iEdge, 0) == iPoint) ? 0 : 1];
      for (unsigned short k = 0; k < nVar * nVar; ++k) sum[k] += coef * edgeD(iEdge, k);
    }
    for (unsigned short k = 0; k < nVar * nVar; ++k) Q(iPoint, k) = sum[k];
  }
  END_SU2_OMP_FOR
}

/*!
 * \brief Row of the owned node m of J (in w, FIRST_RING_JUMPS): for each edge f = (m, j) the block W_f on the
 *        diagonal and -W_f passed to offBlock(j, block) (J = B >= K). S, phi, Q on halos.
 */
template <class Geometry, class WeightType, class NodeType, class NodeTypeQ, class EdgeType, class PhiType,
          class OffFunc>
void JumpRow(const Geometry& geometry, unsigned short nDim, unsigned short nVar, const WeightType& weights,
             const NodeType& S, const NodeTypeQ& Q, const EdgeType& edgeD, const PhiType& phi, su2double coefDelta,
             su2double gam, unsigned long iPoint, Block& diag, OffFunc&& offBlock) {
  for (unsigned short iVar = 0; iVar < nVar; ++iVar)
    for (unsigned short jVar = 0; jVar < nVar; ++jVar) diag(iVar, jVar) = 0.0;
  for (auto iEdge : geometry.nodes->GetEdges(iPoint)) {
    const unsigned long node[2] = {geometry.edges->GetNode(iEdge, 0), geometry.edges->GetNode(iEdge, 1)};
    const unsigned long jPoint = (node[0] == iPoint) ? node[1] : node[0];
    su2double alpha, sumT, r[2], mu[2];
    JumpEdge(geometry, nDim, weights, S, phi, coefDelta, gam, iEdge, alpha, sumT, r, mu);
    Block off;
    for (unsigned short iVar = 0; iVar < nVar; ++iVar) {
      for (unsigned short jVar = 0; jVar < nVar; ++jVar) {
        const unsigned short k = iVar * nVar + jVar;
        const su2double d = edgeD(iEdge, k);
        su2double w = sumT * fabs(alpha) * d;
        for (unsigned short x = 0; x < 2; ++x) w += mu[x] * (Q(node[x], k) - sumT * r[x] * d);
        diag(iVar, jVar) += w;
        off(iVar, jVar) = -w;
      }
    }
    offBlock(jPoint, off);
  }
}

/*--- TECNO_ADJOINT_LIMITER= EDGE_ENTROPY_TVD (P1_TECNO_ADJ_LIMITER.md, fourth candidate (s) with the freeze path;
 * ROE_TECNO_PLAN.md 6.6). Half-edge h = (edge e, end a), characteristic field f of e (shear: the component along
 * t = dy_S/|dy_S|, one value for the block). In the canonical orientation of e (end 0 = the end with the
 * lexicographically smaller coordinates, normal and fields M/P relative to it), with dy, s_a = (R^T G_a w . e)_f,
 * b_a = 2 s_a - dy (the local far-side jump of the P1 slope), B = 2 minmod(dy, b_0, b_1), d = eps S (S2 guard):
 *   own clip   P1_h = clip of s_a into [min(0,B) - d, max(0,B) + d] (a factor in [0, 1]),
 *   moment     s_h(Phi) = Phi_h s_a + sum_{h' at a} pi_hh' Phi_h' D_h' l_e'(Phi) / D_h   (frozen field: field f of every
 *              edge e' of the node, shear vectors projected on t of e),  pi_hh' = sigma_h (g_a,k(h) . e_e'),
 *              l_e = dy - 1/2 (Phi_0 s_0 + Phi_1 s_1),  sigma_h = +1 / -1 at canonical end 0 / 1,  g_a,k = a_ak / V_a the
 *              gradient weight of the other end k in G_a (P1 average: the stored a, Vhat; Green-Gauss: n_ak/2, V),
 *   envelope   for every rho_a in [0, 1] and every other end Phi_o in [0, P1_o]:
 *              s_h in rho_a (P + A + [Tm, Tp]) + rho_a^2 Q   (P = P1_h s_a, the sums of model 4),
 *   factor     G_h = min(1, t_up) H(v), t_up the level |B| + d of the nondecreasing majorant of (P + A + Tp) rho + Q rho^2,
 *              H(v) = (1 - v)^2 (1 + 2 v) on [0, 1] (0 beyond), v = max(0, -(A + Tm + min(0, Q))) / (P + d), in the
 *              sign frame of B;  rho_a,f = prod_{h at a} G_h,f;  Phi_h = P1_h rho_a,f.
 * Then the frozen-field moment of every final value lies in the box (model 4 proof: rho_a <= G_h for every h at a).
 * Half-edges with D_h = 0 and the shear half-edges of an edge with |dy_S| <= (nDim + 2) d_S (noise: own clip with B = 0
 * on the magnitude, P1 = min(1, d_S / |s_S|)) have G = 1. Theorem A1 holds for any Phi (S2 machinery). MPI: the
 * ingredients and the clip on every local edge (halo gradients and primitives are current), rho on the owned nodes
 * (all their edges, weights and Vhat are local), copied to the halos, Phi on every local edge. ---*/

constexpr unsigned short TVD_CLIP_B = 0, TVD_CLIP_P1 = 4, TVD_CLIP_FLIP = 12, TVD_CLIP_NOISE = 13, TVD_CLIP_WIDTH = 14;

/*!
 * \brief Freeze rule of EDGE_ENTROPY_TVD: recompute while the store is not valid (first call of a run, e.g. after a
 *        restart) or while innerIter <= limiterIter (LIMITER_ITER, as the point limiters); frozen afterwards.
 */
inline bool TVDRecompute(bool valid, unsigned long innerIter, unsigned long limiterIter) {
  return !valid || (innerIter <= limiterIter);
}

/*!
 * \brief Canonical orientation of EDGE_ENTROPY_TVD: true if node 1 of the edge has the lexicographically smaller
 *        coordinates (x, then y, then z; the global index only on an exact tie), i.e. the canonical end 0 is node 1.
 *        Independent of the numbering and of the partition.
 */
template <class Geometry>
inline bool TVDFlip(const Geometry& geometry, unsigned short nDim, unsigned long iEdge) {
  const auto n0 = geometry.edges->GetNode(iEdge, 0), n1 = geometry.edges->GetNode(iEdge, 1);
  const auto* x0 = geometry.nodes->GetCoord(n0);
  const auto* x1 = geometry.nodes->GetCoord(n1);
  for (unsigned short iDim = 0; iDim < nDim; ++iDim) {
    if (x0[iDim] < x1[iDim]) return false;
    if (x0[iDim] > x1[iDim]) return true;
  }
  return geometry.nodes->GetGlobalIndex(n0) > geometry.nodes->GetGlobalIndex(n1);
}

/*!
 * \brief Local field index of the canonical field f (a flipped edge has the opposite normal: M <-> P).
 */
inline unsigned short TVDLocalField(unsigned short f, bool flip) {
  return flip ? ((f == 0) ? 3 : ((f == 3) ? 0 : f)) : f;
}

/*!
 * \brief Canonical data of one edge (EDGE_ENTROPY_TVD): ends, edge vector, dy, s of the canonical ends, D, guards of the
 *        four fields (shear: projected on t, or the magnitudes for a noise edge), the shear vectors and t.
 */
struct TVDEdgeView {
  unsigned long node[2] = {0, 0};
  bool flip = false, noise = false;
  su2double e[3] = {}, dy[TVD_NFIELD] = {}, s[2][TVD_NFIELD] = {}, D[TVD_NFIELD] = {}, d[TVD_NFIELD] = {};
  su2double dyS[3] = {}, sS[2][3] = {}, t[3] = {};
};

template <class Geometry, class StoreType>
inline void TVDLoad(const Geometry& geometry, unsigned short nDim, const StoreType& ing, unsigned long iEdge,
                    TVDEdgeView& v) {
  v.flip = TVDFlip(geometry, nDim, iEdge);
  const unsigned long n0 = geometry.edges->GetNode(iEdge, 0), n1 = geometry.edges->GetNode(iEdge, 1);
  v.node[0] = v.flip ? n1 : n0;
  v.node[1] = v.flip ? n0 : n1;
  const auto* x0 = geometry.nodes->GetCoord(v.node[0]);
  const auto* x1 = geometry.nodes->GetCoord(v.node[1]);
  for (unsigned short iDim = 0; iDim < nDim; ++iDim) v.e[iDim] = x1[iDim] - x0[iDim];
  const su2double sgn = v.flip ? -1.0 : 1.0;
  auto col = [](unsigned short fl) { return (fl == 3) ? 2 : fl; };                  /*--- M, 0, P columns. ---*/
  auto dcol = [](unsigned short fl) { return (fl == 3) ? 2 : ((fl == 2) ? 3 : fl); }; /*--- D columns. ---*/
  for (unsigned short f = 0; f < TVD_NFIELD; ++f) {
    const unsigned short fl = TVDLocalField(f, v.flip);
    v.D[f] = ing(iEdge, TVD_D + dcol(fl));
    if (f == 2) continue;
    v.dy[f] = sgn * ing(iEdge, TVD_DY + col(fl));
    for (unsigned short x = 0; x < 2; ++x) {
      const unsigned short y = v.flip ? 1 - x : x;
      v.s[x][f] = sgn * ing(iEdge, ((y == 0) ? TVD_S0 : TVD_S1) + col(fl));
    }
    v.d[f] = ing(iEdge, TVD_GUARD);
  }
  v.d[2] = ing(iEdge, TVD_GUARD + 1);
  su2double norm = 0.0;
  for (unsigned short iDim = 0; iDim < nDim; ++iDim) {
    v.dyS[iDim] = sgn * ing(iEdge, TVD_SHEAR + iDim);
    norm += v.dyS[iDim] * v.dyS[iDim];
    for (unsigned short x = 0; x < 2; ++x) {
      const unsigned short y = v.flip ? 1 - x : x;
      v.sS[x][iDim] = sgn * ing(iEdge, TVD_SHEAR + (1 + y) * nDim + iDim);
    }
  }
  norm = sqrt(norm);
  v.noise = !(norm > (nDim + 2) * v.d[2]);
  for (unsigned short x = 0; x < 2; ++x) {
    su2double st = 0.0, ss = 0.0;
    for (unsigned short iDim = 0; iDim < nDim; ++iDim) {
      v.t[iDim] = v.noise ? su2double(0.0) : v.dyS[iDim] / norm;
      st += v.sS[x][iDim] * v.t[iDim];
      ss += v.sS[x][iDim] * v.sS[x][iDim];
    }
    v.s[x][2] = v.noise ? su2double(sqrt(ss)) : st;
  }
  v.dy[2] = v.noise ? su2double(0.0) : norm;
}

/*!
 * \brief minmod of three values (0 unless all have the same strict sign).
 */
inline su2double TVDMinmod3(su2double a, su2double b, su2double c) {
  if ((a > 0.0) && (b > 0.0) && (c > 0.0)) return fmin(a, fmin(b, c));
  if ((a < 0.0) && (b < 0.0) && (c < 0.0)) return -fmin(-a, fmin(-b, -c));
  return 0.0;
}

/*!
 * \brief Own-slope clip of model 3: the factor that brings s into [lo, hi] (lo <= 0 <= hi), 1 inside.
 */
inline su2double TVDOwnClip(su2double s, su2double lo, su2double hi) {
  if (s > hi) return hi / s;
  if (s < lo) return lo / s;
  return 1.0;
}

/*!
 * \brief Smooth admissible factor G of one half-edge and field (model 4, factor_env), from the own limited slope
 *        P = P1 s, the envelope sums A, Q, Tp >= 0 >= Tm, the bound B and the guard d.
 */
inline su2double TVDFactor(su2double own, su2double A, su2double Q, su2double Tp, su2double Tm, su2double B,
                           su2double d) {
  const su2double sg = (B >= 0.0) ? 1.0 : -1.0;
  own *= sg;
  A *= sg;
  Q *= sg;
  const su2double tp = (sg > 0.0) ? Tp : -Tm;
  const su2double tm = (sg > 0.0) ? Tm : -Tp;
  const su2double hi = fabs(B) + d;
  const su2double U1 = own + A + tp;
  /*--- t_up: the largest t with m(rho) <= hi on [0, t], m the nondecreasing majorant of U1 rho + Q rho^2 (the
   * function up to its peak, then the line rho U1/2 through the peak value). ---*/
  su2double tUp = std::numeric_limits<su2double>::infinity();
  if (U1 > 0.0) {
    tUp = 2.0 * hi / (U1 + sqrt(fmax(su2double(0.0), U1 * U1 + 4.0 * Q * hi)));
  } else if (Q > 0.0) {
    tUp = (-U1 + sqrt(U1 * U1 + 4.0 * Q * hi)) / (2.0 * Q);
  }
  const su2double md = -(A + tm + fmin(su2double(0.0), Q));
  const su2double v = fmax(su2double(0.0), md) / fmax(own + d, std::numeric_limits<su2double>::min());
  const su2double vv = fmin(su2double(1.0), v);
  const su2double H = (1.0 - vv) * (1.0 - vv) * (1.0 + 2.0 * vv);
  return fmin(su2double(1.0), tUp) * H;
}

/*!
 * \brief Own clip of every local edge (EDGE_ENTROPY_TVD): clip(e, TVD_CLIP_B + f) = B, clip(e, TVD_CLIP_P1 + 4 x + f) =
 *        P1 of the canonical end x, flip and noise flags.
 * \param[in] ing - Ingredients (iEdge, TVDWidth(nDim)) of every local edge.
 */
template <class Geometry, class StoreType, class ClipType>
void TVDEdgeClip(const Geometry& geometry, unsigned short nDim, const StoreType& ing, ClipType& clip) {
  SU2_OMP_FOR_STAT(OMP_CHUNK)
  for (unsigned long iEdge = 0; iEdge < geometry.GetnEdge(); ++iEdge) {
    TVDEdgeView v;
    TVDLoad(geometry, nDim, ing, iEdge, v);
    for (unsigned short f = 0; f < TVD_NFIELD; ++f) {
      const su2double dy = v.dy[f];
      const su2double B = ((f == 2) && v.noise)
                              ? su2double(0.0)
                              : 2.0 * TVDMinmod3(dy, 2.0 * v.s[0][f] - dy, 2.0 * v.s[1][f] - dy);
      const su2double lo = fmin(su2double(0.0), B) - v.d[f], hi = fmax(su2double(0.0), B) + v.d[f];
      clip(iEdge, TVD_CLIP_B + f) = B;
      for (unsigned short x = 0; x < 2; ++x) clip(iEdge, TVD_CLIP_P1 + 4 * x + f) = TVDOwnClip(v.s[x][f], lo, hi);
    }
    clip(iEdge, TVD_CLIP_FLIP) = v.flip ? 1.0 : 0.0;
    clip(iEdge, TVD_CLIP_NOISE) = v.noise ? 1.0 : 0.0;
  }
  END_SU2_OMP_FOR
}

/*!
 * \brief Node factors rho(a, f) = prod_{h at a} G_h,f on the owned nodes (EDGE_ENTROPY_TVD).
 * \param[in] weights - Gradient weights a of each edge (iEdge, 2 nDim), a_{0,1} then a_{1,0} (P1 average, or
 *            GreenGaussWeights); volume(a) the volumes of the gradient (Vhat or V).
 * \param[in] ing, clip - Ingredients and own clip of every local edge.
 * \param[out] rho - (iPoint, TVD_NFIELD), owned nodes.
 */
template <class Geometry, class WeightType, class VolumeType, class StoreType, class ClipType, class RhoType>
void TVDNodeFactor(const Geometry& geometry, unsigned short nDim, const WeightType& weights, const VolumeType& volume,
                   const StoreType& ing, const ClipType& clip, RhoType& rho) {
  std::vector<TVDEdgeView> view;
  std::vector<unsigned short> end;
  std::vector<std::array<su2double, 3> > gam;
  SU2_OMP_FOR_STAT(OMP_CHUNK)
  for (unsigned long iPoint = 0; iPoint < geometry.GetnPointDomain(); ++iPoint) {
    const auto range = geometry.nodes->GetEdges(iPoint);
    const auto* edges = range.begin();
    const size_t nEdgeNode = range.end() - range.begin();
    view.resize(nEdgeNode);
    end.resize(nEdgeNode);
    gam.resize(nEdgeNode);
    const su2double invVol = 1.0 / volume(iPoint);
    for (size_t h = 0; h < nEdgeNode; ++h) {
      const auto iEdge = edges[h];
      TVDLoad(geometry, nDim, ing, iEdge, view[h]);
      end[h] = (view[h].node[0] == iPoint) ? 0 : 1;
      const unsigned short own = (geometry.edges->GetNode(iEdge, 0) == iPoint) ? 0 : nDim;
      gam[h] = {0.0, 0.0, 0.0};
      for (unsigned short iDim = 0; iDim < nDim; ++iDim) gam[h][iDim] = weights(iEdge, own + iDim) * invVol;
    }
    su2double rhoNode[TVD_NFIELD] = {1.0, 1.0, 1.0, 1.0};
    for (size_t h = 0; h < nEdgeNode; ++h) {
      const auto& vh = view[h];
      const auto eh = edges[h];
      const unsigned short x = end[h];
      const su2double sigma = (x == 0) ? 1.0 : -1.0;
      for (unsigned short f = 0; f < TVD_NFIELD; ++f) {
        const su2double Dh = vh.D[f];
        if (!(Dh > 0.0) || ((f == 2) && vh.noise)) continue;
        su2double A = 0.0, Q = 0.0, Tp = 0.0, Tm = 0.0;
        for (size_t k = 0; k < nEdgeNode; ++k) {
          const auto& vk = view[k];
          const auto ek = edges[k];
          su2double pi = 0.0;
          for (unsigned short iDim = 0; iDim < nDim; ++iDim) pi += gam[h][iDim] * vk.e[iDim];
          const su2double w = sigma * pi / Dh;
          const unsigned short xk = end[k], ok = 1 - xk;
          const su2double P1a = clip(ek, TVD_CLIP_P1 + 4 * xk + f), P1o = clip(ek, TVD_CLIP_P1 + 4 * ok + f);
          su2double dy = vk.dy[f], sa = vk.s[xk][f], so = vk.s[ok][f];
          if (f == 2) {
            dy = sa = so = 0.0;
            for (unsigned short iDim = 0; iDim < nDim; ++iDim) {
              dy += vk.dyS[iDim] * vh.t[iDim];
              sa += vk.sS[xk][iDim] * vh.t[iDim];
              so += vk.sS[ok][iDim] * vh.t[iDim];
            }
          }
          const su2double D = vk.D[f];
          const su2double fix = P1a * D * dy;
          const su2double tt = -0.5 * P1a * D * P1o * so;
          const su2double q2 = -0.5 * P1a * P1a * D * sa;
          A += w * fix;
          Q += w * q2;
          const su2double wt = w * tt;
          Tp += fmax(su2double(0.0), wt);
          Tm += fmin(su2double(0.0), wt);
        }
        const su2double own = clip(eh, TVD_CLIP_P1 + 4 * x + f) * vh.s[x][f];
        rhoNode[f] *= TVDFactor(own, A, Q, Tp, Tm, clip(eh, TVD_CLIP_B + f), vh.d[f]);
      }
    }
    for (unsigned short f = 0; f < TVD_NFIELD; ++f) rho(iPoint, f) = rhoNode[f];
  }
  END_SU2_OMP_FOR
}

/*!
 * \brief Final values Phi = P1 rho of every local edge (EDGE_ENTROPY_TVD), stored in the local orientation:
 *        phi(e, 4 y + f) for node y of the edge and the local field f (M, 0, S, P). rho needed on the halos.
 */
template <class Geometry, class ClipType, class RhoType, class PhiType>
void TVDEdgePhi(const Geometry& geometry, const ClipType& clip, const RhoType& rho, PhiType& phi) {
  SU2_OMP_FOR_STAT(OMP_CHUNK)
  for (unsigned long iEdge = 0; iEdge < geometry.GetnEdge(); ++iEdge) {
    const bool flip = (clip(iEdge, TVD_CLIP_FLIP) != 0.0);
    for (unsigned short x = 0; x < 2; ++x) {
      const unsigned short y = flip ? 1 - x : x;
      const auto node = geometry.edges->GetNode(iEdge, y);
      for (unsigned short f = 0; f < TVD_NFIELD; ++f) {
        phi(iEdge, 4 * y + TVDLocalField(f, flip)) = clip(iEdge, TVD_CLIP_P1 + 4 * x + f) * rho(node, f);
      }
    }
  }
  END_SU2_OMP_FOR
}

/*--- FIRST_RING_JUMPS with EDGE_ENTROPY_TVD (ROE_TECNO_PLAN.md 6.6, rev 3): with the per-edge-end, per-field values
 * Phi_xf (diagonal in the characteristic basis of the edge, D = R C R^T, C diagonal >= 0 in the same basis) the jump is,
 * per field f,  y_f(L) = alpha_f y_f(Delta) + Phi_0f y_f(u_0) + Phi_1f y_f(u_1),
 *   alpha_f = coefDelta + gam (Phi_0f a_01.e / V_0 - Phi_1f a_10.e / V_1),  u_x = (gam / V_x) sum_{m != x'} (a_xm.e) delta_xm,
 * and the three-term weighted Cauchy-Schwarz per field with the weights (alphaBar, phiBar_0 t'_0, phiBar_1 t'_1),
 * alphaBar = max_f |alpha_f|, phiBar_x = max_f |Phi_xf|, gives the bound of JumpEdge with |alpha| -> alphaBar and
 * |gam phi_x / V_x| -> phiBar_x |gam / V_x| (the inner Cauchy-Schwarz of u_x does not involve Phi). So J >= K(Phi),
 * J 1 = 0, symmetric, first ring, with the same regrouping (JumpNodalQEnds, JumpRowEnds). Phi = 1 everywhere gives
 * JumpEdge with phi = 1. ---*/

/*!
 * \brief JumpEdge with per-edge-end limiter values phiAt(x, f) (local orientation) and alphaBar >= alphaFloor
 *        (JumpEdgeEnds: the store of EDGE_ENTROPY_TVD and floor 0; JumpEdgeEndsFCT).
 */
template <class Geometry, class WeightType, class NodeType, class PhiAt>
void JumpEdgeEndsWith(const Geometry& geometry, unsigned short nDim, const WeightType& weights, const NodeType& S,
                      const PhiAt& phiAt, su2double alphaFloor, su2double coefDelta, su2double gam, unsigned long iEdge,
                      su2double& alpha, su2double& sumT, su2double* r, su2double* mu) {
  const unsigned short offT = 1 + nDim * nDim, offM = 1 + 2 * nDim * nDim;
  const unsigned long node[2] = {geometry.edges->GetNode(iEdge, 0), geometry.edges->GetNode(iEdge, 1)};
  const auto* x0 = geometry.nodes->GetCoord(node[0]);
  const auto* x1 = geometry.nodes->GetCoord(node[1]);
  su2double e[3] = {};
  for (unsigned short iDim = 0; iDim < nDim; ++iDim) e[iDim] = x1[iDim] - x0[iDim];
  su2double t[2] = {}, ae[2] = {}, phiBar[2] = {};
  for (unsigned short x = 0; x < 2; ++x) {
    for (unsigned short f = 0; f < TVD_NFIELD; ++f) phiBar[x] = fmax(phiBar[x], fabs(phiAt(x, f)));
  }
  for (unsigned short x = 0; x < 2; ++x) {
    const auto p = node[x];
    su2double a[3] = {}, aEa = 0.0, eTe = 0.0;
    for (unsigned short iDim = 0; iDim < nDim; ++iDim) {
      a[iDim] = weights(iEdge, x * nDim + iDim);
      ae[x] += a[iDim] * e[iDim];
    }
    for (unsigned short iDim = 0; iDim < nDim; ++iDim) {
      for (unsigned short jDim = 0; jDim < nDim; ++jDim) {
        aEa += a[iDim] * S(p, 1 + nDim * iDim + jDim) * a[jDim];
        eTe += e[iDim] * S(p, offT + nDim * iDim + jDim) * e[jDim];
      }
    }
    mu[x] = sqrt(fmax(aEa, su2double(0.0)));
    const su2double own = (mu[x] > 0.0) ? ae[x] * ae[x] / mu[x] : su2double(0.0);
    const su2double margin = S(p, offM + 1);
    const su2double tau = fmax(eTe - own, su2double(0.0)) + margin * (eTe + own);
    const su2double nu = fmax(S(p, offM) - mu[x], su2double(0.0)) + margin * S(p, offM);
    const su2double g = phiBar[x] * fabs(gam * S(p, 0));
    t[x] = g * sqrt(tau * nu);
    r[x] = (nu > 0.0) ? g * sqrt(tau / nu) : su2double(0.0);
  }
  alpha = alphaFloor;
  for (unsigned short f = 0; f < TVD_NFIELD; ++f) {
    const su2double af = coefDelta + gam * (phiAt(0, f) * S(node[0], 0) * ae[0] -
                                            phiAt(1, f) * S(node[1], 0) * ae[1]);
    alpha = fmax(alpha, fabs(af));
  }
  sumT = alpha + t[0] + t[1];
}

/*!
 * \brief JumpEdge with the per-edge-end limiter values of the edge store phi(e, 4 x + f) (local orientation).
 */
template <class Geometry, class WeightType, class NodeType, class PhiType>
void JumpEdgeEnds(const Geometry& geometry, unsigned short nDim, const WeightType& weights, const NodeType& S,
                  const PhiType& edgePhi, su2double coefDelta, su2double gam, unsigned long iEdge, su2double& alpha,
                  su2double& sumT, su2double* r, su2double* mu) {
  auto phiAt = [&](unsigned short x, unsigned short f) { return edgePhi(iEdge, 4 * x + f); };
  JumpEdgeEndsWith(geometry, nDim, weights, S, phiAt, 0.0, coefDelta, gam, iEdge, alpha, sumT, r, mu);
}

/*!
 * \brief JumpNodalQ with an edge function edgeFn(iEdge, alpha, sumT, r, mu) (JumpEdgeEnds, JumpEdgeEndsFCT).
 */
template <class Geometry, class EdgeType, class NodeTypeQ, class EdgeFunc>
void JumpNodalQEndsWith(const Geometry& geometry, unsigned short nVar, const EdgeType& edgeD, NodeTypeQ& Q,
                        const EdgeFunc& edgeFn) {
  SU2_OMP_FOR_STAT(OMP_CHUNK)
  for (unsigned long iPoint = 0; iPoint < geometry.GetnPointDomain(); ++iPoint) {
    su2double sum[MAXNVAR * MAXNVAR] = {};
    for (auto iEdge : geometry.nodes->GetEdges(iPoint)) {
      su2double alpha, sumT, r[2], mu[2];
      edgeFn(iEdge, alpha, sumT, r, mu);
      const su2double coef = sumT * r[(geometry.edges->GetNode(iEdge, 0) == iPoint) ? 0 : 1];
      for (unsigned short k = 0; k < nVar * nVar; ++k) sum[k] += coef * edgeD(iEdge, k);
    }
    for (unsigned short k = 0; k < nVar * nVar; ++k) Q(iPoint, k) = sum[k];
  }
  END_SU2_OMP_FOR
}

/*!
 * \brief JumpNodalQ with JumpEdgeEnds (EDGE_ENTROPY_TVD).
 */
template <class Geometry, class WeightType, class NodeType, class PhiType, class EdgeType, class NodeTypeQ>
void JumpNodalQEnds(const Geometry& geometry, unsigned short nDim, unsigned short nVar, const WeightType& weights,
                    const NodeType& S, const PhiType& edgePhi, su2double coefDelta, su2double gam, const EdgeType& edgeD,
                    NodeTypeQ& Q) {
  auto edgeFn = [&](unsigned long iEdge, su2double& alpha, su2double& sumT, su2double* r, su2double* mu) {
    JumpEdgeEnds(geometry, nDim, weights, S, edgePhi, coefDelta, gam, iEdge, alpha, sumT, r, mu);
  };
  JumpNodalQEndsWith(geometry, nVar, edgeD, Q, edgeFn);
}

/*!
 * \brief JumpRow with an edge function edgeFn(iEdge, alpha, sumT, r, mu): alpha is alphaBar >= 0 here.
 */
template <class Geometry, class NodeTypeQ, class EdgeType, class EdgeFunc, class OffFunc>
void JumpRowEndsWith(const Geometry& geometry, unsigned short nVar, const NodeTypeQ& Q, const EdgeType& edgeD,
                     const EdgeFunc& edgeFn, unsigned long iPoint, Block& diag, OffFunc&& offBlock) {
  for (unsigned short iVar = 0; iVar < nVar; ++iVar)
    for (unsigned short jVar = 0; jVar < nVar; ++jVar) diag(iVar, jVar) = 0.0;
  for (auto iEdge : geometry.nodes->GetEdges(iPoint)) {
    const unsigned long node[2] = {geometry.edges->GetNode(iEdge, 0), geometry.edges->GetNode(iEdge, 1)};
    const unsigned long jPoint = (node[0] == iPoint) ? node[1] : node[0];
    su2double alpha, sumT, r[2], mu[2];
    edgeFn(iEdge, alpha, sumT, r, mu);
    Block off;
    for (unsigned short iVar = 0; iVar < nVar; ++iVar) {
      for (unsigned short jVar = 0; jVar < nVar; ++jVar) {
        const unsigned short k = iVar * nVar + jVar;
        const su2double d = edgeD(iEdge, k);
        su2double w = sumT * alpha * d;
        for (unsigned short x = 0; x < 2; ++x) w += mu[x] * (Q(node[x], k) - sumT * r[x] * d);
        diag(iVar, jVar) += w;
        off(iVar, jVar) = -w;
      }
    }
    offBlock(jPoint, off);
  }
}

/*!
 * \brief JumpRow with JumpEdgeEnds (EDGE_ENTROPY_TVD): alpha is alphaBar >= 0 here.
 */
template <class Geometry, class WeightType, class NodeType, class NodeTypeQ, class EdgeType, class PhiType,
          class OffFunc>
void JumpRowEnds(const Geometry& geometry, unsigned short nDim, unsigned short nVar, const WeightType& weights,
                 const NodeType& S, const NodeTypeQ& Q, const EdgeType& edgeD, const PhiType& edgePhi,
                 su2double coefDelta, su2double gam, unsigned long iPoint, Block& diag, OffFunc&& offBlock) {
  auto edgeFn = [&](unsigned long iEdge, su2double& alpha, su2double& sumT, su2double* r, su2double* mu) {
    JumpEdgeEnds(geometry, nDim, weights, S, edgePhi, coefDelta, gam, iEdge, alpha, sumT, r, mu);
  };
  JumpRowEndsWith(geometry, nVar, Q, edgeD, edgeFn, iPoint, diag, offBlock);
}

/*--- TECNO_ADJOINT_LIMITER= EDGE_ENTROPY_FCT (ROE_TECNO_PLAN.md 6.9; P1_TECNO_ADJ_LIMITER.md, candidate 8 + freeze; model
 * Prove/solver_notes/tecno_adj_limiter_model8.py, limiter9). Per edge e and canonical field f (M, 0, S, P of
 * EDGE_ENTROPY_TVD, canonical orientation of TVDFlip; the shear block is one field along t_e = dy_S/|dy_S|, its moments
 * are vectors), frozen field, sg = sign dy (+1 at dy = 0):
 *  (1) far side of each canonical end a: beta_a = max(0, max_{v in star(a)} sg y(w_a - w_v)) at end 0 and
 *      max(0, max_v sg y(w_v - w_a)) at end 1, y = R_e^T of the edge (frozen); u_a = sg y(b_a), b_0 = sum mu_v (w_0 - w_v),
 *      b_1 = sum mu_v (w_v - w_1), mu >= 0 the coordinates of x_a - x_other in the element of the ball of a whose cone
 *      contains that direction (the upwind element through the edge extension), or u_a = sg (2 s_a - dy) where the
 *      direction leaves the mesh; A_0 = max(beta_0, u_0, (1 - lambda/|lambda|_fix)|dy|/2), A_1 = max(beta_1, u_1,
 *      (1 + lambda/|lambda|_fix)|dy|/2), the ratio replaced by 0 where |lambda|_fix = 0 (both ends attachable);
 *  (2) delta = (max(n_0, n_1) + 2) d (n the global node degrees, d the S2 guard), valid = |dy| > delta and D > 0,
 *      B = 2 sg min(A_0, A_1), cap = |B| + delta, sigma = (s_0 + s_1)/2, o = sg sigma, phi_c = min(1, cap/o) (valid, o > 0)
 *      else 1, M_c = phi_c D (dy - phi_c sigma) (shear: the vector phi_c D (dy_S - phi_c sigma_S));
 *  (3) pairs (observer e at a, contributor e' at a, self included): c = 1/2 sg_e pi M_c,e' / D_e, pi = sigma_a (g_a . e_e')
 *      the weights of EDGE_ENTROPY_TVD (shear: t_e . M_c,e'); Pm_e = sum over both ends of max(0, c) (valid observers),
 *      R_e = min(1, cap/Pm_e), room = max(0, cap - R_e Pm_e), a_e = min(1, room/(phi_c o)) (valid, phi_c o > 0) else 1;
 *  (4) rho_e' = min of R_e over the valid observers with c > 0 (1 if none), alpha = min(rho, a_e),
 *      gamma = max(0, 1 - alpha - (room - alpha phi_c o)/|dy|) (valid) else 0.
 * Residual (tecnoAdjointEdgeFCT): Delta part D (alpha_r l_c + gamma_r dy), moment alpha_r phi_c D l_c at both ends,
 * l_c = dy - phi_c sigma, (alpha_r, gamma_r) = (r alpha, 1 - r (1 - gamma)) with the MUSCL ramp r, transpose coefficient
 * GradientCoefficient(1, 0): sum_m w_m.R_m = sum_e sum_f D (alpha_r l_c^2 + gamma_r dy^2) >= 0 for any values (A1');
 * frozen field: S_e = sg (alpha phi_c sigma + (1 - alpha - gamma) dy) + sum_pairs alpha_e' c <= cap_e (box), also for
 * every ramp (S scales with r). MPI: A (1) needs the star and the ball of a, the halves of Pm and rho (3, 4) all edges of
 * a; they are computed by the owner of a for each edge of a and copied to the cut edges of the other ranks through the
 * star slots (FCTSetup: position of the global index of the other end in the sorted global neighbour list of a). Every
 * local edge then has the serial values. ---*/

/*--- Extra ingredient columns of EDGE_ENTROPY_FCT after the TVDWidth(nDim) columns of EDGE_ENTROPY_TVD (local orientation):
 * signed speeds of M, 0, P (u.n - c, u.n, u.n + c), their floored |lambda| (lamM, lam0, lamP of the kernel), the speed of
 * sound and the total enthalpy of the Roe average, its velocity (nDim) and the unit normal of the dissipation (nDim), so
 * that R^T of the edge can be applied to any vector (FCTProject, the algebra of tecnoProject). ---*/
constexpr unsigned short FCT_LAM = 0, FCT_LAMFIX = 3, FCT_SOUND = 6, FCT_ENTHALPY = 7, FCT_VEL = 8;
inline unsigned short FCTWidth(unsigned short nDim) { return TVDWidth(nDim) + 8 + 2 * nDim; }

/*--- Stored values of EDGE_ENTROPY_FCT (TeCNOAdjEdgePhi, local orientation and local fields M, 0, S, P). ---*/
constexpr unsigned short FCT_PHIC = 0, FCT_ALPHA = 4, FCT_GAMMA = 8, FCT_STORE_WIDTH = 12;

/*--- Per-edge work values (canonical fields): phi_c, cap, o, valid, M_c (scalar fields), the shear moment vector, R,
 * room and a_e. ---*/
constexpr unsigned short FCT_E_PHIC = 0, FCT_E_CAP = 4, FCT_E_O = 8, FCT_E_VALID = 12, FCT_E_MC = 16, FCT_E_MVEC = 20,
                         FCT_E_R = 23, FCT_E_ROOM = 27, FCT_E_AOWN = 31, FCT_E_WIDTH = 35;

/*!
 * \brief Static data of EDGE_ENTROPY_FCT on a partition: global node degrees, star slots of every (edge, local end) and
 *        the upwind element of every (edge, local end) of an owned node. Index of (iEdge, local end y): 2 iEdge + y.
 */
struct FCTSetup {
  bool ready = false;
  unsigned short maxDeg = 0;
  std::vector<unsigned long> degree;
  std::vector<unsigned short> slot;
  std::vector<unsigned long> ueNode;  /*!< \brief 3 per (edge, end). */
  std::vector<su2double> ueMu;        /*!< \brief 3 per (edge, end). */
  std::vector<char> ueOk;
};

/*!
 * \brief Work arrays of EDGE_ENTROPY_FCT: A, the halves of Pm and of rho per (edge, local end y, canonical field f) at
 *        column 4 y + f, the per-edge values (FCT_E_*) and the exchange buffer (nPoint, 4 maxDeg).
 */
struct FCTWork {
  su2activematrix A, Pm, rho, edge, buf;
};

/*!
 * \brief Rows of the setup exchange: (degree, sorted global indices of the neighbours) of every owned node, width
 *        1 + maxDeg (maxDeg the largest degree over all ranks). The caller copies them to the halos.
 */
template <class Geometry>
void FCTSetupRows(const Geometry& geometry, FCTSetup& setup, su2activematrix& rows) {
  unsigned long localMax = 0, globalMax = 0;
  for (unsigned long iPoint = 0; iPoint < geometry.GetnPointDomain(); ++iPoint)
    localMax = std::max<unsigned long>(localMax, geometry.nodes->GetnPoint(iPoint));
  SU2_MPI::Allreduce(&localMax, &globalMax, 1, MPI_UNSIGNED_LONG, MPI_MAX, SU2_MPI::GetComm());
  if (4 * globalMax + 1 > std::numeric_limits<unsigned short>::max())
    SU2_MPI::Error("TECNO_ADJOINT_LIMITER= EDGE_ENTROPY_FCT: node degree too large for the star exchange.", CURRENT_FUNCTION);
  setup.maxDeg = static_cast<unsigned short>(globalMax);
  rows.resize(geometry.GetnPoint(), 1 + setup.maxDeg);
  rows = su2double(0.0);
  std::vector<unsigned long> list;
  for (unsigned long iPoint = 0; iPoint < geometry.GetnPointDomain(); ++iPoint) {
    list.clear();
    for (auto jPoint : geometry.nodes->GetPoints(iPoint)) list.push_back(geometry.nodes->GetGlobalIndex(jPoint));
    std::sort(list.begin(), list.end());
    rows(iPoint, 0) = su2double(list.size());
    for (size_t k = 0; k < list.size(); ++k) rows(iPoint, 1 + k) = su2double(list[k]);
  }
}

/*!
 * \brief Coordinates mu of a direction in the cone of a simplex at its vertex a, sum_v mu_v (x_v - x_a) = dir (Cramer).
 * \return false if the simplex is degenerate.
 */
inline bool FCTConeCoordinates(unsigned short nDim, const su2double* const* xv, const su2double* xa,
                               const su2double* dir, su2double* mu) {
  su2double M[3][3] = {};
  for (unsigned short c = 0; c < nDim; ++c)
    for (unsigned short d = 0; d < nDim; ++d) M[d][c] = xv[c][d] - xa[d];
  auto det = [&](const su2double (&A)[3][3]) {
    if (nDim == 2) return A[0][0] * A[1][1] - A[0][1] * A[1][0];
    return A[0][0] * (A[1][1] * A[2][2] - A[1][2] * A[2][1]) - A[0][1] * (A[1][0] * A[2][2] - A[1][2] * A[2][0]) +
           A[0][2] * (A[1][0] * A[2][1] - A[1][1] * A[2][0]);
  };
  const su2double D = det(M);
  if (!(fabs(D) > 0.0)) return false;
  for (unsigned short c = 0; c < nDim; ++c) {
    su2double Mc[3][3];
    for (unsigned short i = 0; i < 3; ++i)
      for (unsigned short j = 0; j < 3; ++j) Mc[i][j] = (j == c) ? ((i < nDim) ? dir[i] : su2double(0.0)) : M[i][j];
    mu[c] = det(Mc) / D;
  }
  return true;
}

/*!
 * \brief Completes the setup after the exchange of the rows: degrees of all local nodes, star slots of every local
 *        (edge, end) (error if the other end is not in the list of the node), upwind elements of the ends at owned
 *        nodes: the element of the ball whose cone at a contains x_a - x_other with the largest min(mu)/sum|mu|,
 *        accepted if that score is >= -1e-10 (the containment test of the model, l8_geom.upwind_elements).
 */
template <class Geometry>
void FCTSetupFinish(const Geometry& geometry, unsigned short nDim, const su2activematrix& rows, FCTSetup& setup) {
  const auto nPoint = geometry.GetnPoint(), nEdge = geometry.GetnEdge();
  setup.degree.assign(nPoint, 0);
  for (unsigned long iPoint = 0; iPoint < nPoint; ++iPoint)
    setup.degree[iPoint] = static_cast<unsigned long>(SU2_TYPE::GetValue(rows(iPoint, 0)));
  setup.slot.assign(2 * nEdge, 0);
  setup.ueNode.assign(6 * nEdge, 0);
  setup.ueMu.assign(6 * nEdge, su2double(0.0));
  setup.ueOk.assign(2 * nEdge, 0);
  unsigned long missing = 0;
  for (unsigned long iEdge = 0; iEdge < nEdge; ++iEdge) {
    for (unsigned short y = 0; y < 2; ++y) {
      const auto a = geometry.edges->GetNode(iEdge, y), b = geometry.edges->GetNode(iEdge, 1 - y);
      const unsigned long target = geometry.nodes->GetGlobalIndex(b);
      unsigned long lo = 0, hi = setup.degree[a];
      while (lo < hi) {
        const unsigned long mid = (lo + hi) / 2;
        if (static_cast<unsigned long>(SU2_TYPE::GetValue(rows(a, 1 + mid))) < target) lo = mid + 1; else hi = mid;
      }
      if ((lo < setup.degree[a]) && (static_cast<unsigned long>(SU2_TYPE::GetValue(rows(a, 1 + lo))) == target)) {
        setup.slot[2 * iEdge + y] = static_cast<unsigned short>(lo);
      } else {
        ++missing;
      }
      if (a >= geometry.GetnPointDomain()) continue;
      /*--- Upwind element of the end a: direction x_a - x_b (through the edge extension). ---*/
      const auto* xa = geometry.nodes->GetCoord(a);
      const auto* xb = geometry.nodes->GetCoord(b);
      su2double dir[3] = {};
      for (unsigned short d = 0; d < nDim; ++d) dir[d] = xa[d] - xb[d];
      su2double best = -std::numeric_limits<su2double>::infinity();
      for (unsigned short j = 0; j < geometry.nodes->GetnElem(a); ++j) {
        const auto* elem = geometry.elem[geometry.nodes->GetElem(a, j)];
        if (elem->GetnNodes() != nDim + 1) continue;
        unsigned long vtx[3] = {};
        const su2double* xv[3] = {};
        unsigned short n = 0;
        for (unsigned short k = 0; k < elem->GetnNodes(); ++k) {
          const auto p = elem->GetNode(k);
          if ((p == a) || (n == nDim)) continue;
          vtx[n] = p;
          xv[n++] = geometry.nodes->GetCoord(p);
        }
        if (n != nDim) continue;
        su2double mu[3] = {};
        if (!FCTConeCoordinates(nDim, xv, xa, dir, mu)) continue;
        su2double mn = mu[0], sum = 0.0;
        for (unsigned short c = 0; c < nDim; ++c) {
          mn = fmin(mn, mu[c]);
          sum += fabs(mu[c]);
        }
        const su2double score = mn / fmax(sum, std::numeric_limits<su2double>::min());
        if (score > best) {
          best = score;
          for (unsigned short c = 0; c < 3; ++c) {
            setup.ueNode[3 * (2 * iEdge + y) + c] = (c < nDim) ? vtx[c] : 0;
            setup.ueMu[3 * (2 * iEdge + y) + c] = (c < nDim) ? mu[c] : su2double(0.0);
          }
        }
      }
      setup.ueOk[2 * iEdge + y] = (best >= -1e-10) ? 1 : 0;
    }
  }
  if (missing > 0)
    SU2_MPI::Error("TECNO_ADJOINT_LIMITER= EDGE_ENTROPY_FCT: an edge end is not in the star list of its node.", CURRENT_FUNCTION);
  setup.ready = true;
}

/*!
 * \brief Canonical data of one edge (EDGE_ENTROPY_FCT): as TVDEdgeView with the shear field |dy_S| along t = dy_S/|dy_S|
 *        (t = 0 where dy_S = 0), the canonical signed speeds and floored |lambda|, and the projection data of the edge.
 */
struct FCTEdgeView {
  unsigned long node[2] = {0, 0};
  bool flip = false;
  su2double e[3] = {}, dy[TVD_NFIELD] = {}, s[2][TVD_NFIELD] = {}, D[TVD_NFIELD] = {}, d[TVD_NFIELD] = {};
  su2double lam[TVD_NFIELD] = {}, lamFix[TVD_NFIELD] = {};
  su2double dyS[3] = {}, sS[2][3] = {}, t[3] = {};
  su2double c = 0.0, H = 0.0, u[3] = {}, n[3] = {};
};

template <class Geometry, class StoreType>
inline void FCTLoad(const Geometry& geometry, unsigned short nDim, const StoreType& ing, unsigned long iEdge,
                    FCTEdgeView& v) {
  v.flip = TVDFlip(geometry, nDim, iEdge);
  const unsigned long n0 = geometry.edges->GetNode(iEdge, 0), n1 = geometry.edges->GetNode(iEdge, 1);
  v.node[0] = v.flip ? n1 : n0;
  v.node[1] = v.flip ? n0 : n1;
  const auto* x0 = geometry.nodes->GetCoord(v.node[0]);
  const auto* x1 = geometry.nodes->GetCoord(v.node[1]);
  for (unsigned short iDim = 0; iDim < nDim; ++iDim) v.e[iDim] = x1[iDim] - x0[iDim];
  const su2double sgn = v.flip ? -1.0 : 1.0;
  const unsigned short X = TVDWidth(nDim);
  auto col = [](unsigned short fl) { return (fl == 3) ? 2 : fl; };                  /*--- M, 0, P columns. ---*/
  auto dcol = [](unsigned short fl) { return (fl == 3) ? 2 : ((fl == 2) ? 3 : fl); }; /*--- D columns. ---*/
  for (unsigned short f = 0; f < TVD_NFIELD; ++f) {
    const unsigned short fl = TVDLocalField(f, v.flip);
    v.D[f] = ing(iEdge, TVD_D + dcol(fl));
    v.d[f] = ing(iEdge, TVD_GUARD + ((f == 2) ? 1 : 0));
    const unsigned short lc = (f == 2) ? 1 : col(fl);  /*--- The shear waves travel with u.n. ---*/
    v.lam[f] = sgn * ing(iEdge, X + FCT_LAM + lc);
    v.lamFix[f] = ing(iEdge, X + FCT_LAMFIX + lc);
    if (f == 2) continue;
    v.dy[f] = sgn * ing(iEdge, TVD_DY + col(fl));
    for (unsigned short x = 0; x < 2; ++x) {
      const unsigned short y = v.flip ? 1 - x : x;
      v.s[x][f] = sgn * ing(iEdge, ((y == 0) ? TVD_S0 : TVD_S1) + col(fl));
    }
  }
  su2double norm = 0.0;
  for (unsigned short iDim = 0; iDim < nDim; ++iDim) {
    v.dyS[iDim] = sgn * ing(iEdge, TVD_SHEAR + iDim);
    norm += v.dyS[iDim] * v.dyS[iDim];
    for (unsigned short x = 0; x < 2; ++x) {
      const unsigned short y = v.flip ? 1 - x : x;
      v.sS[x][iDim] = sgn * ing(iEdge, TVD_SHEAR + (1 + y) * nDim + iDim);
    }
  }
  norm = sqrt(norm);
  for (unsigned short iDim = 0; iDim < nDim; ++iDim) v.t[iDim] = (norm > 0.0) ? v.dyS[iDim] / norm : su2double(0.0);
  v.dy[2] = norm;
  for (unsigned short x = 0; x < 2; ++x) {
    su2double st = 0.0;
    for (unsigned short iDim = 0; iDim < nDim; ++iDim) st += v.sS[x][iDim] * v.t[iDim];
    v.s[x][2] = st;
  }
  v.c = ing(iEdge, X + FCT_SOUND);
  v.H = ing(iEdge, X + FCT_ENTHALPY);
  for (unsigned short iDim = 0; iDim < nDim; ++iDim) {
    v.u[iDim] = ing(iEdge, X + FCT_VEL + iDim);
    v.n[iDim] = ing(iEdge, X + FCT_VEL + nDim + iDim);
  }
}

/*!
 * \brief Canonical characteristic values y[f] = (R_e^T z)_f of any vector z of entropy variables (tecnoProject on the
 *        stored average and normal; a flipped edge has M <-> P; the shear vector projected on t of the edge).
 */
inline void FCTProject(unsigned short nDim, const FCTEdgeView& v, const su2double* z, su2double* y) {
  su2double uv = 0.0, nv = 0.0, nq = 0.0, un = 0.0, sq = 0.0, yS[3] = {};
  for (unsigned short iDim = 0; iDim < nDim; ++iDim) {
    uv += v.u[iDim] * z[iDim + 1];
    nv += v.n[iDim] * z[iDim + 1];
    un += v.u[iDim] * v.n[iDim];
    sq += v.u[iDim] * v.u[iDim];
    yS[iDim] = z[iDim + 1] + v.u[iDim] * z[nDim + 1];
    nq += v.n[iDim] * yS[iDim];
  }
  su2double st = 0.0;
  for (unsigned short iDim = 0; iDim < nDim; ++iDim) st += v.t[iDim] * (yS[iDim] - nq * v.n[iDim]);
  const su2double yM = z[0] + uv - v.c * nv + (v.H - un * v.c) * z[nDim + 1];
  const su2double y0 = z[0] + uv + 0.5 * sq * z[nDim + 1];
  const su2double yP = z[0] + uv + v.c * nv + (v.H + un * v.c) * z[nDim + 1];
  y[0] = v.flip ? yP : yM;
  y[1] = y0;
  y[2] = st;
  y[3] = v.flip ? yM : yP;
}

/*!
 * \brief Pass (1) on the owned nodes: A of every (edge, end) at the node, column 4 y + f of A (y the local end).
 * \param[in] W - Entropy variables (iPoint, iVar), halos included.
 */
template <class Geometry, class StoreType, class FieldType>
void FCTEndCapacity(const Geometry& geometry, unsigned short nDim, const FCTSetup& setup, const StoreType& ing,
                    const FieldType& W, su2activematrix& A) {
  const unsigned short nVar = nDim + 2;
  SU2_OMP_FOR_STAT(OMP_CHUNK)
  for (unsigned long a = 0; a < geometry.GetnPointDomain(); ++a) {
    for (auto iEdge : geometry.nodes->GetEdges(a)) {
      FCTEdgeView v;
      FCTLoad(geometry, nDim, ing, iEdge, v);
      const unsigned short x = (v.node[0] == a) ? 0 : 1;
      const unsigned short y = (geometry.edges->GetNode(iEdge, 0) == a) ? 0 : 1;
      const su2double sx = (x == 0) ? 1.0 : -1.0;  /*--- end 0: w_a - w_v, end 1: w_v - w_a ---*/
      su2double sg[TVD_NFIELD], cap[TVD_NFIELD];
      for (unsigned short f = 0; f < TVD_NFIELD; ++f) {
        sg[f] = (v.dy[f] >= 0.0) ? 1.0 : -1.0;
        cap[f] = 0.0;
      }
      su2double z[MAXNVAR], yv[TVD_NFIELD];
      for (auto jEdge : geometry.nodes->GetEdges(a)) {
        const auto b = (geometry.edges->GetNode(jEdge, 0) == a) ? geometry.edges->GetNode(jEdge, 1)
                                                               : geometry.edges->GetNode(jEdge, 0);
        for (unsigned short iVar = 0; iVar < nVar; ++iVar) z[iVar] = sx * (W(a, iVar) - W(b, iVar));
        FCTProject(nDim, v, z, yv);
        for (unsigned short f = 0; f < TVD_NFIELD; ++f) cap[f] = fmax(cap[f], sg[f] * yv[f]);
      }
      const size_t h = 2 * iEdge + y;
      su2double ub[TVD_NFIELD];
      if (setup.ueOk[h]) {
        for (unsigned short iVar = 0; iVar < nVar; ++iVar) {
          z[iVar] = 0.0;
          for (unsigned short c = 0; c < nDim; ++c)
            z[iVar] += setup.ueMu[3 * h + c] * sx * (W(a, iVar) - W(setup.ueNode[3 * h + c], iVar));
        }
        FCTProject(nDim, v, z, yv);
        for (unsigned short f = 0; f < TVD_NFIELD; ++f) ub[f] = sg[f] * yv[f];
      } else {
        for (unsigned short f = 0; f < TVD_NFIELD; ++f) ub[f] = sg[f] * (2.0 * v.s[x][f] - v.dy[f]);
      }
      for (unsigned short f = 0; f < TVD_NFIELD; ++f) {
        const su2double ratio = (v.lamFix[f] > 0.0) ? v.lam[f] / v.lamFix[f] : su2double(0.0);
        const su2double own = 0.5 * (1.0 - sx * ratio) * fabs(v.dy[f]);
        A(iEdge, 4 * y + f) = fmax(cap[f], fmax(ub[f], own));
      }
    }
  }
  END_SU2_OMP_FOR
}

/*!
 * \brief Copies the (edge, end) values of the owned nodes to the cut edges of the halo nodes through the star slots;
 *        comm(buf) copies the rows of buf (nPoint, 4 maxDeg) from the owners to the halos. Called by all ranks.
 */
template <class Geometry, class Comm>
void FCTExchange(const Geometry& geometry, const FCTSetup& setup, su2activematrix& end, su2activematrix& buf,
                 Comm&& comm) {
  if (SU2_MPI::GetSize() == 1) return;
  SU2_OMP_FOR_STAT(OMP_CHUNK)
  for (unsigned long a = 0; a < geometry.GetnPointDomain(); ++a) {
    for (auto iEdge : geometry.nodes->GetEdges(a)) {
      const unsigned short y = (geometry.edges->GetNode(iEdge, 0) == a) ? 0 : 1;
      const unsigned short s = setup.slot[2 * iEdge + y];
      for (unsigned short f = 0; f < TVD_NFIELD; ++f) buf(a, 4 * s + f) = end(iEdge, 4 * y + f);
    }
  }
  END_SU2_OMP_FOR
  comm(buf);
  SU2_OMP_FOR_STAT(OMP_CHUNK)
  for (unsigned long iEdge = 0; iEdge < geometry.GetnEdge(); ++iEdge) {
    for (unsigned short y = 0; y < 2; ++y) {
      const auto a = geometry.edges->GetNode(iEdge, y);
      if (a < geometry.GetnPointDomain()) continue;
      const unsigned short s = setup.slot[2 * iEdge + y];
      for (unsigned short f = 0; f < TVD_NFIELD; ++f) end(iEdge, 4 * y + f) = buf(a, 4 * s + f);
    }
  }
  END_SU2_OMP_FOR
}

/*!
 * \brief Pass (2) on every local edge: delta, validity, cap, o, phi_c and the moments M_c (FCT_E_* columns).
 */
template <class Geometry, class StoreType>
void FCTEdgeOwn(const Geometry& geometry, unsigned short nDim, const FCTSetup& setup, const StoreType& ing,
                const su2activematrix& A, su2activematrix& ed) {
  SU2_OMP_FOR_STAT(OMP_CHUNK)
  for (unsigned long iEdge = 0; iEdge < geometry.GetnEdge(); ++iEdge) {
    FCTEdgeView v;
    FCTLoad(geometry, nDim, ing, iEdge, v);
    const su2double nmax = su2double(std::max(setup.degree[v.node[0]], setup.degree[v.node[1]]));
    const unsigned short y0 = v.flip ? 1 : 0;
    for (unsigned short f = 0; f < TVD_NFIELD; ++f) {
      const su2double delta = (nmax + 2.0) * v.d[f];
      const bool valid = (fabs(v.dy[f]) > delta) && (v.D[f] > 0.0);
      const su2double sg = (v.dy[f] >= 0.0) ? 1.0 : -1.0;
      const su2double cap = 2.0 * fmin(A(iEdge, 4 * y0 + f), A(iEdge, 4 * (1 - y0) + f)) + delta;
      const su2double sigma = 0.5 * (v.s[0][f] + v.s[1][f]);
      const su2double o = sg * sigma;
      const su2double phic = (valid && (o > 0.0)) ? fmin(su2double(1.0), cap / o) : su2double(1.0);
      ed(iEdge, FCT_E_PHIC + f) = phic;
      ed(iEdge, FCT_E_CAP + f) = cap;
      ed(iEdge, FCT_E_O + f) = o;
      ed(iEdge, FCT_E_VALID + f) = valid ? 1.0 : 0.0;
      ed(iEdge, FCT_E_MC + f) = (f == 2) ? su2double(0.0) : phic * v.D[f] * (v.dy[f] - phic * sigma);
    }
    const su2double phiS = ed(iEdge, FCT_E_PHIC + 2);
    for (unsigned short iDim = 0; iDim < 3; ++iDim) {
      ed(iEdge, FCT_E_MVEC + iDim) =
          (iDim < nDim) ? phiS * v.D[2] * (v.dyS[iDim] - phiS * 0.5 * (v.sS[0][iDim] + v.sS[1][iDim])) : su2double(0.0);
    }
  }
  END_SU2_OMP_FOR
}

/*!
 * \brief Edges of a node with their canonical views, ends and gradient weights g = a_{a,other}/V_a (passes 3 and 5).
 */
struct FCTNodeStar {
  std::vector<FCTEdgeView> view;
  std::vector<unsigned long> edge;
  std::vector<unsigned short> y;
  std::vector<std::array<su2double, 3> > g;
  std::vector<su2double> sigma;
};

template <class Geometry, class WeightType, class VolumeType, class StoreType>
inline void FCTLoadStar(const Geometry& geometry, unsigned short nDim, const WeightType& weights, const VolumeType& volume,
                        const StoreType& ing, unsigned long a, FCTNodeStar& st) {
  const auto range = geometry.nodes->GetEdges(a);
  const size_t n = range.end() - range.begin();
  st.view.resize(n);
  st.edge.resize(n);
  st.y.resize(n);
  st.g.resize(n);
  st.sigma.resize(n);
  const su2double invVol = 1.0 / volume(a);
  size_t h = 0;
  for (auto iEdge : range) {
    FCTLoad(geometry, nDim, ing, iEdge, st.view[h]);
    st.edge[h] = iEdge;
    st.y[h] = (geometry.edges->GetNode(iEdge, 0) == a) ? 0 : 1;
    st.sigma[h] = (st.view[h].node[0] == a) ? 1.0 : -1.0;
    st.g[h] = {0.0, 0.0, 0.0};
    for (unsigned short iDim = 0; iDim < nDim; ++iDim) st.g[h][iDim] = weights(iEdge, st.y[h] * nDim + iDim) * invVol;
    ++h;
  }
}

/*!
 * \brief Contribution c of the contributor k to the valid observer h at the node in field f (frozen field).
 */
inline su2double FCTPair(unsigned short nDim, const FCTNodeStar& st, size_t h, size_t k, unsigned short f,
                         const su2activematrix& ed) {
  const auto& vh = st.view[h];
  su2double pi = 0.0;
  for (unsigned short iDim = 0; iDim < nDim; ++iDim) pi += st.g[h][iDim] * st.view[k].e[iDim];
  pi *= st.sigma[h];
  const su2double sg = (vh.dy[f] >= 0.0) ? 1.0 : -1.0;
  su2double m = 0.0;
  if (f == 2) {
    for (unsigned short iDim = 0; iDim < nDim; ++iDim) m += vh.t[iDim] * ed(st.edge[k], FCT_E_MVEC + iDim);
  } else {
    m = ed(st.edge[k], FCT_E_MC + f);
  }
  return 0.5 * sg * pi * m / vh.D[f];
}

/*!
 * \brief Pass (3) on the owned nodes: the half of Pm of every valid observer at the node, column 4 y + f of Pm.
 */
template <class Geometry, class WeightType, class VolumeType, class StoreType>
void FCTPmHalf(const Geometry& geometry, unsigned short nDim, const WeightType& weights, const VolumeType& volume,
               const StoreType& ing, const su2activematrix& ed, su2activematrix& Pm) {
  FCTNodeStar st;
  SU2_OMP_FOR_STAT(OMP_CHUNK)
  for (unsigned long a = 0; a < geometry.GetnPointDomain(); ++a) {
    FCTLoadStar(geometry, nDim, weights, volume, ing, a, st);
    for (size_t h = 0; h < st.edge.size(); ++h) {
      for (unsigned short f = 0; f < TVD_NFIELD; ++f) {
        su2double sum = 0.0;
        if (ed(st.edge[h], FCT_E_VALID + f) != 0.0) {
          for (size_t k = 0; k < st.edge.size(); ++k) sum += fmax(su2double(0.0), FCTPair(nDim, st, h, k, f, ed));
        }
        Pm(st.edge[h], 4 * st.y[h] + f) = sum;
      }
    }
  }
  END_SU2_OMP_FOR
}

/*!
 * \brief Pass (4) on every local edge: R, room and a_e from Pm = sum of both halves.
 */
template <class Geometry>
void FCTRoom(const Geometry& geometry, const su2activematrix& Pm, su2activematrix& ed) {
  SU2_OMP_FOR_STAT(OMP_CHUNK)
  for (unsigned long iEdge = 0; iEdge < geometry.GetnEdge(); ++iEdge) {
    for (unsigned short f = 0; f < TVD_NFIELD; ++f) {
      const bool valid = (ed(iEdge, FCT_E_VALID + f) != 0.0);
      const su2double pm = Pm(iEdge, f) + Pm(iEdge, 4 + f);
      const su2double cap = ed(iEdge, FCT_E_CAP + f);
      const su2double R = (valid && (pm > cap)) ? cap / pm : su2double(1.0);
      const su2double room = fmax(su2double(0.0), cap - R * pm);
      const su2double den = ed(iEdge, FCT_E_PHIC + f) * ed(iEdge, FCT_E_O + f);
      ed(iEdge, FCT_E_R + f) = R;
      ed(iEdge, FCT_E_ROOM + f) = room;
      ed(iEdge, FCT_E_AOWN + f) = (valid && (den > 0.0)) ? fmin(su2double(1.0), room / den) : su2double(1.0);
    }
  }
  END_SU2_OMP_FOR
}

/*!
 * \brief Pass (5) on the owned nodes: the half of rho of every contributor at the node (min of R over the valid
 *        observers at the node with c > 0, 1 if none), column 4 y + f of rho.
 */
template <class Geometry, class WeightType, class VolumeType, class StoreType>
void FCTObserverMin(const Geometry& geometry, unsigned short nDim, const WeightType& weights, const VolumeType& volume,
                    const StoreType& ing, const su2activematrix& ed, su2activematrix& rho) {
  FCTNodeStar st;
  SU2_OMP_FOR_STAT(OMP_CHUNK)
  for (unsigned long a = 0; a < geometry.GetnPointDomain(); ++a) {
    FCTLoadStar(geometry, nDim, weights, volume, ing, a, st);
    for (size_t k = 0; k < st.edge.size(); ++k) {
      for (unsigned short f = 0; f < TVD_NFIELD; ++f) {
        su2double m = 1.0;
        for (size_t h = 0; h < st.edge.size(); ++h) {
          if (ed(st.edge[h], FCT_E_VALID + f) == 0.0) continue;
          if (FCTPair(nDim, st, h, k, f, ed) > 0.0) m = fmin(m, ed(st.edge[h], FCT_E_R + f));
        }
        rho(st.edge[k], 4 * st.y[k] + f) = m;
      }
    }
  }
  END_SU2_OMP_FOR
}

/*!
 * \brief Pass (6) on every local edge: alpha and gamma, stored with phi_c in the local layout (FCT_PHIC, FCT_ALPHA,
 *        FCT_GAMMA + local field).
 */
template <class Geometry, class StoreType, class PhiType>
void FCTFinal(const Geometry& geometry, unsigned short nDim, const StoreType& ing, const su2activematrix& ed,
              const su2activematrix& rho, PhiType& store) {
  SU2_OMP_FOR_STAT(OMP_CHUNK)
  for (unsigned long iEdge = 0; iEdge < geometry.GetnEdge(); ++iEdge) {
    const bool flip = TVDFlip(geometry, nDim, iEdge);
    for (unsigned short f = 0; f < TVD_NFIELD; ++f) {
      const bool valid = (ed(iEdge, FCT_E_VALID + f) != 0.0);
      const su2double phic = ed(iEdge, FCT_E_PHIC + f);
      const su2double r = fmin(rho(iEdge, f), rho(iEdge, 4 + f));
      const su2double alpha = fmin(r, ed(iEdge, FCT_E_AOWN + f));
      su2double gam = 0.0;
      if (valid) {
        /*--- |dy| of the canonical field (invariant under the orientation). ---*/
        su2double ady = 0.0;
        if (f == 2) {
          for (unsigned short iDim = 0; iDim < nDim; ++iDim) ady += pow(ing(iEdge, TVD_SHEAR + iDim), 2);
          ady = sqrt(ady);
        } else {
          const unsigned short fl = TVDLocalField(f, flip);
          ady = fabs(ing(iEdge, TVD_DY + ((fl == 3) ? 2 : fl)));
        }
        const su2double left = ed(iEdge, FCT_E_ROOM + f) - alpha * phic * ed(iEdge, FCT_E_O + f);
        gam = fmax(su2double(0.0), 1.0 - alpha - left / ady);
      }
      const unsigned short fl = TVDLocalField(f, flip);
      store(iEdge, FCT_PHIC + fl) = phic;
      store(iEdge, FCT_ALPHA + fl) = alpha;
      store(iEdge, FCT_GAMMA + fl) = gam;
    }
  }
  END_SU2_OMP_FOR
}

/*!
 * \brief All passes of EDGE_ENTROPY_FCT on given ingredients (all local edges, kernel columns FCTWidth(nDim)) and
 *        entropy variables W (halos included); comm(matrix) copies the rows of matrix from the owners to the halos.
 *        The setup must be complete and the work arrays sized (FCTWorkResize). Called by all threads and ranks.
 */
template <class Geometry, class WeightType, class VolumeType, class StoreType, class FieldType, class PhiType,
          class Comm>
void FCTPasses(const Geometry& geometry, unsigned short nDim, const WeightType& weights, const VolumeType& volume,
               const FCTSetup& setup, const StoreType& ing, const FieldType& W, FCTWork& work, PhiType& store,
               Comm&& comm) {
  FCTEndCapacity(geometry, nDim, setup, ing, W, work.A);
  FCTExchange(geometry, setup, work.A, work.buf, comm);
  FCTEdgeOwn(geometry, nDim, setup, ing, work.A, work.edge);
  FCTPmHalf(geometry, nDim, weights, volume, ing, work.edge, work.Pm);
  FCTExchange(geometry, setup, work.Pm, work.buf, comm);
  FCTRoom(geometry, work.Pm, work.edge);
  FCTObserverMin(geometry, nDim, weights, volume, ing, work.edge, work.rho);
  FCTExchange(geometry, setup, work.rho, work.buf, comm);
  FCTFinal(geometry, nDim, ing, work.edge, work.rho, store);
}

/*!
 * \brief Sizes the work arrays (serial: no exchange buffer).
 */
template <class Geometry>
void FCTWorkResize(const Geometry& geometry, const FCTSetup& setup, FCTWork& work) {
  const auto nEdge = geometry.GetnEdge();
  work.A.resize(nEdge, 8) = su2double(0.0);
  work.Pm.resize(nEdge, 8) = su2double(0.0);
  work.rho.resize(nEdge, 8) = su2double(1.0);
  work.edge.resize(nEdge, FCT_E_WIDTH) = su2double(0.0);
  if (SU2_MPI::GetSize() > 1) work.buf.resize(geometry.GetnPoint(), 4 * std::max<unsigned short>(setup.maxDeg, 1)) = su2double(0.0);
}

/*!
 * \brief Local sums of the activity statistics of the stored values (each edge once: its canonical end 0 owned), over
 *        the valid (edge, field) entries: [n, act, zeros, sum Phi_e, sum w min(Phi_e, 1), sum w, act_eq, zeros_eq,
 *        sum Phi_eq, sum w Phi_eq], Phi_e = alpha phi_c, Phi_eq = 1 - gamma - alpha (1 - phi_c), w = D |dy|; act: |x - 1| >
 *        1e-6, zeros: x < 1e-6 (the definitions of the model's activity9).
 */
template <class Geometry, class StoreType, class PhiType>
void FCTStatistics(const Geometry& geometry, unsigned short nDim, const StoreType& ing, const su2activematrix& ed,
                   const PhiType& store, su2double* sums) {
  for (unsigned short k = 0; k < 10; ++k) sums[k] = 0.0;
  for (unsigned long iEdge = 0; iEdge < geometry.GetnEdge(); ++iEdge) {
    FCTEdgeView v;
    FCTLoad(geometry, nDim, ing, iEdge, v);
    if (v.node[0] >= geometry.GetnPointDomain()) continue;
    for (unsigned short f = 0; f < TVD_NFIELD; ++f) {
      if (ed(iEdge, FCT_E_VALID + f) == 0.0) continue;
      const unsigned short fl = TVDLocalField(f, v.flip);
      const su2double phic = store(iEdge, FCT_PHIC + fl), alpha = store(iEdge, FCT_ALPHA + fl);
      const su2double gam = store(iEdge, FCT_GAMMA + fl);
      const su2double pe = alpha * phic, peq = 1.0 - gam - alpha * (1.0 - phic), w = v.D[f] * fabs(v.dy[f]);
      sums[0] += 1.0;
      sums[1] += (fabs(pe - 1.0) > 1e-6) ? 1.0 : 0.0;
      sums[2] += (pe < 1e-6) ? 1.0 : 0.0;
      sums[3] += pe;
      sums[4] += w * fmin(pe, su2double(1.0));
      sums[5] += w;
      sums[6] += (fabs(peq - 1.0) > 1e-6) ? 1.0 : 0.0;
      sums[7] += (peq < 1e-6) ? 1.0 : 0.0;
      sums[8] += peq;
      sums[9] += w * peq;
    }
  }
}

/*!
 * \brief FIRST_RING_JUMPS with EDGE_ENTROPY_FCT (ROE_TECNO_PLAN.md 6.9, item 3): per field f the blended linearization
 *        is K = alpha_r K(phi_c) + gamma_r K(0) at full ramp (coefDelta = 1, gam = GradientCoefficient(1, 0)); JumpEdgeEnds
 *        with Phi_0f = Phi_1f = phi_c,f and alphaBar' = max(1, alphaBar(phi_c)) majorizes K(phi_c) and K(0) per field
 *        (the bound is nondecreasing in alphaBar and phiBar), so K <= sum_f (alpha_r + gamma_r) J'_f <= J'.
 */
template <class Geometry, class WeightType, class NodeType, class PhiType>
void JumpEdgeEndsFCT(const Geometry& geometry, unsigned short nDim, const WeightType& weights, const NodeType& S,
                     const PhiType& store, unsigned long iEdge, su2double& alpha, su2double& sumT, su2double* r,
                     su2double* mu) {
  auto phiAt = [&](unsigned short /*x*/, unsigned short f) { return store(iEdge, FCT_PHIC + f); };
  JumpEdgeEndsWith(geometry, nDim, weights, S, phiAt, 1.0, 1.0, GradientCoefficient(1.0, 0.0), iEdge, alpha, sumT, r,
                   mu);
}

template <class Geometry, class WeightType, class NodeType, class PhiType, class EdgeType, class NodeTypeQ>
void JumpNodalQEndsFCT(const Geometry& geometry, unsigned short nDim, unsigned short nVar, const WeightType& weights,
                       const NodeType& S, const PhiType& store, const EdgeType& edgeD, NodeTypeQ& Q) {
  auto edgeFn = [&](unsigned long iEdge, su2double& alpha, su2double& sumT, su2double* r, su2double* mu) {
    JumpEdgeEndsFCT(geometry, nDim, weights, S, store, iEdge, alpha, sumT, r, mu);
  };
  JumpNodalQEndsWith(geometry, nVar, edgeD, Q, edgeFn);
}

template <class Geometry, class WeightType, class NodeType, class NodeTypeQ, class EdgeType, class PhiType,
          class OffFunc>
void JumpRowEndsFCT(const Geometry& geometry, unsigned short nDim, unsigned short nVar, const WeightType& weights,
                    const NodeType& S, const NodeTypeQ& Q, const EdgeType& edgeD, const PhiType& store,
                    unsigned long iPoint, Block& diag, OffFunc&& offBlock) {
  auto edgeFn = [&](unsigned long iEdge, su2double& alpha, su2double& sumT, su2double* r, su2double* mu) {
    JumpEdgeEndsFCT(geometry, nDim, weights, S, store, iEdge, alpha, sumT, r, mu);
  };
  JumpRowEndsWith(geometry, nVar, Q, edgeD, edgeFn, iPoint, diag, offBlock);
}

}  // namespace TeCNOAdjoint
