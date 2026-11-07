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

#include <cmath>
#include <limits>
#include "../../../../Common/include/basic_types/datatype_structure.hpp"
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
 * Regrouped by mesh edges f = (m, j): K <= 2 J = sum_f W_f (x) (e_m - e_j)(e_m - e_j)^T,
 *   W_f = S_f |alpha_f| D_f + mu_mj (Q_m - S_f r_m^f D_f) + mu_jm (Q_j - S_f r_j^f D_f),   Q_x = sum_{e ni x} S_e r_x^e D_e,
 * so 2 J >= K (C4 (ii)), J symmetric, J 1 = 0, first-neighbour blocks only. The exclusions of the own pair in tau and
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
 * \brief Row of the owned node m of J (in w, FIRST_RING_JUMPS): for each edge f = (m, j) the block W_f / 2 on the
 *        diagonal and -W_f / 2 passed to offBlock(j, block). S, phi, Q on halos.
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
        diag(iVar, jVar) += 0.5 * w;
        off(iVar, jVar) = -0.5 * w;
      }
    }
    offBlock(jPoint, off);
  }
}

}  // namespace TeCNOAdjoint
