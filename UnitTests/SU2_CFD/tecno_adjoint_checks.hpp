/*!
 * \file tecno_adjoint_checks.hpp
 * \brief Checks of EDGE_FORM_TECNO_ADJOINT and EDGE_FORM_EC_EXACT_JACOBIAN on any geometry with the CGeometry interface
 *        used by TeCNOAdjoint (serial: every node owned). Used by p1_flux_variants.cpp.
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

#include <Eigen/Dense>
#include <algorithm>
#include <array>
#include <random>
#include <vector>
#include "../../SU2_CFD/include/numerics_simd/flow/convection/common.hpp"
#include "../../SU2_CFD/include/numerics_simd/flow/variables.hpp"
#include "../../SU2_CFD/include/numerics/flow/tecno_adjoint.hpp"

namespace TeCNOAdjointChecks {

constexpr double GAMMA = 1.4;

/*--- Nodal states rho, u, p. ---*/
struct States {
  std::vector<double> rho, p;
  std::vector<std::array<double, 3>> u;
};

/*!
 * \brief Random states, rho and p in exp(+-spread), |u_d| <= 3c / sqrt(nDim).
 */
inline States RandomStates(unsigned long nPoint, unsigned short nDim, std::mt19937& gen, double spread) {
  std::uniform_real_distribution<double> r(-1.0, 1.0);
  States S;
  S.rho.resize(nPoint);
  S.p.resize(nPoint);
  S.u.resize(nPoint);
  for (auto i = 0ul; i < nPoint; ++i) {
    S.rho[i] = exp(spread * r(gen));
    S.p[i] = exp(spread * r(gen));
    const double c = sqrt(GAMMA * S.p[i] / S.rho[i]);
    S.u[i] = {0.0, 0.0, 0.0};
    for (unsigned short d = 0; d < nDim; ++d) S.u[i][d] = 3.0 * c * r(gen) / sqrt(double(nDim));
  }
  return S;
}

template <size_t nDim>
CCompressiblePrimitives<nDim, nDim + 4> Primitives(double rho, const double* u, double p) {
  CCompressiblePrimitives<nDim, nDim + 4> V;
  V.density() = rho;
  V.pressure() = p;
  V.temperature() = p / rho;
  double q2 = 0.0;
  for (size_t d = 0; d < nDim; ++d) {
    V.velocity(d) = u[d];
    q2 += u[d] * u[d];
  }
  V.enthalpy() = GAMMA / (GAMMA - 1.0) * p / rho + 0.5 * q2;
  return V;
}

template <size_t nDim>
CCompressiblePrimitives<nDim, nDim + 4> PrimitivesOfConservatives(const double* U) {
  double u[3] = {0.0, 0.0, 0.0}, q2 = 0.0;
  for (size_t d = 0; d < nDim; ++d) {
    u[d] = U[d + 1] / U[0];
    q2 += u[d] * u[d];
  }
  return Primitives<nDim>(U[0], u, (GAMMA - 1.0) * (U[nDim + 1] - 0.5 * U[0] * q2));
}

/*!
 * \brief TECNO_ADJOINT_GRADIENT= P1_AVERAGE data of a geometry (CPhysicalGeometry::SetTeCNOAdjointP1Weights).
 */
struct P1Average {
  su2activematrix weights; /*!< \brief a_{0,1}, a_{1,0} of each edge (iEdge, 2 nDim). */
  su2activevector volume;  /*!< \brief Vhat (iPoint). */
  template <class Geometry>
  explicit P1Average(Geometry& geometry) { geometry.SetTeCNOAdjointP1Weights(weights, volume); }
};

inline su2activematrix EntropyField(const States& S, unsigned short nDim) {
  su2activematrix w(S.rho.size(), nDim + 2);
  for (auto i = 0ul; i < S.rho.size(); ++i)
    TeCNOAdjoint::EntropyVariables(nDim, GAMMA, S.rho[i], S.u[i].data(), S.p[i], w[i]);
  return w;
}

/*!
 * \brief D_e of each edge as in the edge kernel: Roe average of the nodal states on nu (projected, or the dual normal),
 *        eigenvalue floor 0.001 (|u.n| + c), 1/2 |nu|, as dense nVar x nVar row major rows of an (nEdge, nVar^2) matrix.
 */
template <size_t nDim, class Geometry>
su2activematrix FrozenD(const Geometry& geometry, const States& S, bool projected) {
  constexpr size_t nVar = nDim + 2;
  su2activematrix D(geometry.GetnEdge(), nVar * nVar);
  for (auto e = 0ul; e < geometry.GetnEdge(); ++e) {
    const auto i = geometry.edges->GetNode(e, 0), k = geometry.edges->GetNode(e, 1);
    const auto* n = geometry.edges->GetNormal(e);
    double edge[3] = {0.0}, e2 = 0.0, ne = 0.0;
    for (size_t d = 0; d < nDim; ++d) {
      edge[d] = geometry.nodes->GetCoord(k)[d] - geometry.nodes->GetCoord(i)[d];
      e2 += edge[d] * edge[d];
      ne += n[d] * edge[d];
    }
    double nu[3] = {0.0}, area = 0.0;
    for (size_t d = 0; d < nDim; ++d) {
      nu[d] = projected ? ne / e2 * edge[d] : n[d];
      area += nu[d] * nu[d];
    }
    area = sqrt(area);
    VectorDbl<nDim> unit;
    for (size_t d = 0; d < nDim; ++d) unit(d) = nu[d] / area;
    CPair<CCompressiblePrimitives<nDim, nDim + 4>> V;
    V.i = Primitives<nDim>(S.rho[i], S.u[i].data(), S.p[i]);
    V.j = Primitives<nDim>(S.rho[k], S.u[k].data(), S.p[k]);
    const auto avg = roeAveragedVariables(Double(GAMMA), V, unit);
    const Double lamMin = 0.001 * (abs(avg.projVel) + avg.speedSound);
    const Double lamM = fmax(abs(avg.projVel - avg.speedSound), lamMin);
    const Double lam0 = fmax(abs(avg.projVel), lamMin);
    const Double lamP = fmax(abs(avg.projVel + avg.speedSound), lamMin);
    for (size_t col = 0; col < nVar; ++col) {
      VectorDbl<nVar> v;
      for (size_t r = 0; r < nVar; ++r) v(r) = (r == col) ? 1.0 : 0.0;
      const auto q = tecnoDissipation(Double(GAMMA), avg, unit, lamM, lam0, lamP, Double(0.5 * area), v);
      for (size_t r = 0; r < nVar; ++r) D(e, r * nVar + col) = q(r)[0];
    }
  }
  return D;
}

/*!
 * \brief Adjoint dissipation residual for frozen D and phi, any nodal field w: the Delta part of the edge loop
 *        -(1 - ramp kappa) q (+ at node 0, - at node 1 of the flux convention, i.e. R_i -= , R_k +=) and the transpose
 *        loop of the solver. Returns R (nPoint, nVar), and sum_e L_e^T D_e L_e. With p1, the P1 average gradient
 *        (TECNO_ADJOINT_GRADIENT= P1_AVERAGE) and its transpose, else Green-Gauss.
 */
template <size_t nDim, class Geometry>
su2activematrix Residual(const Geometry& geometry, const su2activematrix& D, const su2activevector& phi,
                         const su2activematrix& w, double ramp, double kappa, double& sumLDL,
                         const P1Average* p1 = nullptr) {
  constexpr size_t nVar = nDim + 2;
  const auto nPoint = w.rows();
  CVectorOfMatrix grad(nPoint, nVar, nDim, 0.0);
  if (p1) {
    TeCNOAdjoint::GradientP1(geometry, nDim, nVar, p1->weights, p1->volume, w, grad);
  } else {
    TeCNOAdjoint::Gradient(geometry, nDim, nVar, w, grad);
  }
  su2activematrix Q(geometry.GetnEdge(), nVar);
  su2activematrix R(nPoint, nVar);
  R = su2double(0.0);
  sumLDL = 0.0;
  const double coefDelta = 1.0 - ramp * kappa;
  for (auto e = 0ul; e < geometry.GetnEdge(); ++e) {
    const auto i = geometry.edges->GetNode(e, 0), k = geometry.edges->GetNode(e, 1);
    Vector<su2double, nVar> dw, L;
    Vector<su2double, nDim> edge;
    for (size_t v = 0; v < nVar; ++v) dw(v) = w(k, v) - w(i, v);
    for (size_t d = 0; d < nDim; ++d) edge(d) = geometry.nodes->GetCoord(k)[d] - geometry.nodes->GetCoord(i)[d];
    TeCNOAdjoint::EdgeJump(nDim, nVar, dw, grad[i], grad[k], phi(i), phi(k), edge, ramp, kappa, L);
    for (size_t r = 0; r < nVar; ++r) {
      double q = 0.0;
      for (size_t c = 0; c < nVar; ++c) q += D(e, r * nVar + c) * L(c);
      Q(e, r) = q;
      sumLDL += L(r) * q;
      R(i, r) -= coefDelta * q;
      R(k, r) += coefDelta * q;
    }
  }
  CVectorOfMatrix Y(nPoint, nVar, nDim, 0.0);
  const su2double gam = TeCNOAdjoint::GradientCoefficient(ramp, kappa);
  if (p1) {
    TeCNOAdjoint::NodalMomentP1(geometry, nDim, nVar, Q, phi, p1->volume, Y);
    TeCNOAdjoint::AddTransposeP1(geometry, nDim, nVar, p1->weights, Y, gam, R);
  } else {
    TeCNOAdjoint::NodalMoment(geometry, nDim, nVar, Q, phi, Y);
    TeCNOAdjoint::AddTranspose(geometry, nDim, nVar, Y, gam, R);
  }
  return R;
}

/*--- Theorem A1 and conservation for random states and weights. ---*/
struct EntropyResult {
  double identity = 0.0;    /*!< \brief max |sum w.R - sum L^T D L| / sum |w R|. */
  double production = -1e300; /*!< \brief max of -sum L^T D L / sum |w R| (must be <= 0). */
  double conservation = 0.0;  /*!< \brief max |sum_m R_m| / max |R|. */
};

template <size_t nDim, class Geometry>
EntropyResult EntropyCheck(const Geometry& geometry, unsigned long nPoint, int nTrials, unsigned seed,
                           const P1Average* p1 = nullptr) {
  constexpr size_t nVar = nDim + 2;
  std::mt19937 gen(seed);
  std::uniform_real_distribution<double> r01(0.0, 1.0);
  EntropyResult res;
  for (int t = 0; t < nTrials; ++t) {
    const auto S = RandomStates(nPoint, nDim, gen, 2.0);
    const auto D = FrozenD<nDim>(geometry, S, t % 2);
    su2activevector phi(nPoint);
    for (auto i = 0ul; i < nPoint; ++i) phi(i) = -0.5 + 2.0 * r01(gen);
    const double ramp = (t % 3 == 0) ? 0.6 : 1.0, kappa = (t % 4 == 1) ? 1.0 / 3.0 : 0.0;
    const auto w = EntropyField(S, nDim);
    double sumLDL = 0.0;
    const auto R = Residual<nDim>(geometry, D, phi, w, ramp, kappa, sumLDL, p1);
    double wR = 0.0, absWR = 0.0, maxR = 0.0, cons[nVar] = {0.0};
    for (auto i = 0ul; i < nPoint; ++i) {
      for (size_t v = 0; v < nVar; ++v) {
        wR += w(i, v) * R(i, v);
        absWR += fabs(w(i, v) * R(i, v));
        maxR = std::max(maxR, fabs(R(i, v)));
        cons[v] += R(i, v);
      }
    }
    res.identity = std::max(res.identity, fabs(wR - sumLDL) / absWR);
    res.production = std::max(res.production, -sumLDL / absWR);
    for (size_t v = 0; v < nVar; ++v) res.conservation = std::max(res.conservation, fabs(cons[v]) / maxR);
  }
  return res;
}

/*--- TECNO_ADJOINT_LIMITER= EDGE_ENTROPY_VA: the edge function of the kernel (tecnoAdjointEdgeVA) on every edge, the
 * Delta part scattered as in the edge loop, the per-end B^T q through NodalMomentEnds and the transpose of the solver.
 * Theorem A1 with the state-dependent per-edge-end Phi: sum_m w_m.R_m = sum_e L_e^T D_e L_e, the right side from R^T L
 * and the amplitudes (L^T q = (R^T L).c). Serial. ---*/
struct EntropyVAResult {
  double identity = 0.0, production = -1e300, conservation = 0.0;
  double phiMin = 1e300, phiMax = -1e300; /*!< \brief Range of the limiter values (unclipped). */
};

template <size_t nDim, class Geometry>
EntropyVAResult EntropyCheckVA(const Geometry& geometry, unsigned long nPoint, int nTrials, unsigned seed,
                               const P1Average* p1 = nullptr) {
  constexpr size_t nVar = nDim + 2;
  std::mt19937 gen(seed);
  EntropyVAResult res;
  for (int t = 0; t < nTrials; ++t) {
    const auto S = RandomStates(nPoint, nDim, gen, (t % 2) ? 0.3 : 2.0);
    const bool projected = (t % 2);
    const double ramp = (t % 3 == 0) ? 0.6 : 1.0, kappa = (t % 4 == 1) ? 1.0 / 3.0 : 0.0;
    const auto w = EntropyField(S, nDim);
    CVectorOfMatrix grad(nPoint, nVar, nDim, 0.0);
    if (p1) {
      TeCNOAdjoint::GradientP1(geometry, nDim, nVar, p1->weights, p1->volume, w, grad);
    } else {
      TeCNOAdjoint::Gradient(geometry, nDim, nVar, w, grad);
    }
    su2activematrix Q(geometry.GetnEdge(), 2 * nVar), R(nPoint, nVar);
    R = su2double(0.0);
    double sumLDL = 0.0;
    const double coefDelta = 1.0 - ramp * kappa;
    for (auto e = 0ul; e < geometry.GetnEdge(); ++e) {
      const auto i = geometry.edges->GetNode(e, 0), k = geometry.edges->GetNode(e, 1);
      const auto* n = geometry.edges->GetNormal(e);
      double edge[3] = {0.0}, e2 = 0.0, ne = 0.0;
      for (size_t d = 0; d < nDim; ++d) {
        edge[d] = geometry.nodes->GetCoord(k)[d] - geometry.nodes->GetCoord(i)[d];
        e2 += edge[d] * edge[d];
        ne += n[d] * edge[d];
      }
      double nu[3] = {0.0}, area = 0.0;
      for (size_t d = 0; d < nDim; ++d) {
        nu[d] = projected ? ne / e2 * edge[d] : n[d];
        area += nu[d] * nu[d];
      }
      area = sqrt(area);
      VectorDbl<nDim> unit;
      for (size_t d = 0; d < nDim; ++d) unit(d) = nu[d] / area;
      CPair<CCompressiblePrimitives<nDim, nDim + 4>> V;
      V.i = Primitives<nDim>(S.rho[i], S.u[i].data(), S.p[i]);
      V.j = Primitives<nDim>(S.rho[k], S.u[k].data(), S.p[k]);
      const auto avg = roeAveragedVariables(Double(GAMMA), V, unit);
      const Double lamMin = 0.001 * (abs(avg.projVel) + avg.speedSound);
      const Double lamM = fmax(abs(avg.projVel - avg.speedSound), lamMin);
      const Double lam0 = fmax(abs(avg.projVel), lamMin);
      const Double lamP = fmax(abs(avg.projVel + avg.speedSound), lamMin);
      const Double scale = 0.5 * area;
      VectorDbl<nVar> dw, p_i, p_k, q, q_i, q_k;
      for (size_t v = 0; v < nVar; ++v) {
        dw(v) = w(k, v) - w(i, v);
        p_i(v) = 0.0;
        p_k(v) = 0.0;
        for (size_t d = 0; d < nDim; ++d) {
          p_i(v) += grad(i, v, d) * edge[d];
          p_k(v) += grad(k, v, d) * edge[d];
        }
      }
      Double phi[8], yL[nDim + 3];
      tecnoAdjointEdgeVA(Double(GAMMA), avg, unit, lamM, lam0, lamP, scale, dw, p_i, p_k, V.i, V.j, ramp, kappa, q, q_i,
                         q_k, phi, yL);
      for (int a = 0; a < 8; ++a) {
        res.phiMin = std::min(res.phiMin, phi[a][0]);
        res.phiMax = std::max(res.phiMax, phi[a][0]);
      }
      /*--- L^T D L = (R^T L).c, c the amplitudes of tecnoDissipationFromY. ---*/
      const double rho = avg.density[0], c = avg.speedSound[0], sc = scale[0];
      double ldl = sc * lamM[0] * rho / (2.0 * GAMMA) * yL[0][0] * yL[0][0] +
                   sc * lam0[0] * rho * (GAMMA - 1.0) / GAMMA * yL[1][0] * yL[1][0] +
                   sc * lamP[0] * rho / (2.0 * GAMMA) * yL[2][0] * yL[2][0];
      for (size_t d = 0; d < nDim; ++d) ldl += sc * lam0[0] * rho * c * c / GAMMA * yL[3 + d][0] * yL[3 + d][0];
      sumLDL += ldl;
      for (size_t v = 0; v < nVar; ++v) {
        R(i, v) -= coefDelta * q(v)[0];
        R(k, v) += coefDelta * q(v)[0];
        Q(e, v) = q_i(v)[0];
        Q(e, nVar + v) = q_k(v)[0];
      }
    }
    CVectorOfMatrix Y(nPoint, nVar, nDim, 0.0);
    const su2double gam = TeCNOAdjoint::GradientCoefficient(ramp, kappa);
    if (p1) {
      TeCNOAdjoint::NodalMomentEnds(geometry, nDim, nVar, Q, p1->volume, Y);
      TeCNOAdjoint::AddTransposeP1(geometry, nDim, nVar, p1->weights, Y, gam, R);
    } else {
      auto volume = [&](unsigned long iPoint) { return geometry.nodes->GetVolume(iPoint); };
      TeCNOAdjoint::NodalMomentEnds(geometry, nDim, nVar, Q, volume, Y);
      TeCNOAdjoint::AddTranspose(geometry, nDim, nVar, Y, gam, R);
    }
    double wR = 0.0, absWR = 0.0, maxR = 0.0, cons[nVar] = {0.0};
    for (auto i = 0ul; i < nPoint; ++i) {
      for (size_t v = 0; v < nVar; ++v) {
        wR += w(i, v) * R(i, v);
        absWR += fabs(w(i, v) * R(i, v));
        maxR = std::max(maxR, fabs(R(i, v)));
        cons[v] += R(i, v);
      }
    }
    res.identity = std::max(res.identity, fabs(wR - sumLDL) / absWR);
    res.production = std::max(res.production, -sumLDL / absWR);
    for (size_t v = 0; v < nVar; ++v) res.conservation = std::max(res.conservation, fabs(cons[v]) / maxR);
  }
  return res;
}

/*--- Linearization with frozen data: symmetry, PSD, and the Lemma A4 majorant 2 M# - C^T D C >= 0. ---*/
struct LinearizationResult {
  double asymmetry = 0.0;   /*!< \brief |A - A^T| / |A|. */
  double negative = 0.0;    /*!< \brief max(0, -lambda_min / lambda_max) of A. */
  double majorant = 0.0;    /*!< \brief max(0, -lambda_min(2 M# - A) / lambda_max(A)). */
  double firstRing = 0.0;   /*!< \brief max(0, -lambda_min(2 J - A) / lambda_max(A)), J of TECNO_ADJOINT_JACOBIAN= FIRST_RING. */
  double firstRingEdge = 0.0; /*!< \brief |J_edge - sum_e a_e a_e^T D_e| / |.|, reference beta from EdgeJump. */
  double firstRingSpread = 0.0; /*!< \brief lambda_max(A)/lambda_max(J), for information. */
  double jumps = 0.0;        /*!< \brief max(0, -lambda_min(J - A) / lambda_max(A)), J of FIRST_RING_JUMPS (J >= A). */
  double jumpsRef = 0.0;     /*!< \brief |J - J_ref| / |J_ref|, J_ref assembled pair by pair from the definition (exact
                                  tau, nu; J carries the roundoff margin, O(sqrt(n eps)) where tau vanishes). */
  double jumpsAlpha = 0.0;   /*!< \brief max |alpha_e - alpha_ref| / (|alpha_ref| + 1), alpha_ref from EdgeJump. */
  double jumpsRowSum = 0.0;  /*!< \brief max_m |sum_n J(m, n)| / |J| (J 1 = 0). */
  double jumpsAsym = 0.0;    /*!< \brief |J - J^T| / |J|. */
  bool jumpsFinite = true;   /*!< \brief all entries of J and the eigensolves finite. */
};

template <size_t nDim, class Geometry>
LinearizationResult LinearizationCheck(const Geometry& geometry, unsigned long nPoint, int nTrials, unsigned seed,
                                       const P1Average* p1 = nullptr) {
  constexpr size_t nVar = nDim + 2;
  std::mt19937 gen(seed);
  std::uniform_real_distribution<double> r01(0.0, 1.0);
  LinearizationResult res;
  const auto nDof = nVar * nPoint;
  for (int t = 0; t < nTrials; ++t) {
    const auto S = RandomStates(nPoint, nDim, gen, 1.0);
    auto D = FrozenD<nDim>(geometry, S, t % 2);
    su2activevector phi(nPoint);
    for (auto i = 0ul; i < nPoint; ++i) phi(i) = -0.5 + 2.0 * r01(gen);
    /*--- Last two trials: an intermediate ramp, some phi = 0 and one D_e = 0 (zero weights, Codex S3b review). ---*/
    const double ramp = (t == nTrials - 1) ? 0.4 : 1.0, kappa = (t % 2) ? 1.0 / 3.0 : 0.0;
    if (t == nTrials - 2) {
      for (auto i = 0ul; i < nPoint; i += 3) phi(i) = 0.0;
      for (size_t k = 0; k < nVar * nVar; ++k) D(0, k) = 0.0;
    }

    /*--- The residual is linear in w for frozen D and phi: its matrix column by column. ---*/
    Eigen::MatrixXd A(nDof, nDof);
    for (auto col = 0ul; col < nDof; ++col) {
      su2activematrix w(nPoint, nVar);
      w = su2double(0.0);
      w(col / nVar, col % nVar) = 1.0;
      double sumLDL = 0.0;
      const auto R = Residual<nDim>(geometry, D, phi, w, ramp, kappa, sumLDL, p1);
      for (auto i = 0ul; i < nPoint; ++i)
        for (size_t v = 0; v < nVar; ++v) A(nVar * i + v, col) = R(i, v);
    }
    res.asymmetry = std::max(res.asymmetry, (A - A.transpose()).norm() / A.norm());
    const Eigen::MatrixXd As = 0.5 * (A + A.transpose());
    Eigen::SelfAdjointEigenSolver<Eigen::MatrixXd> eigA(As);
    const double lamMax = eigA.eigenvalues().maxCoeff();
    res.negative = std::max(res.negative, -eigA.eigenvalues().minCoeff() / lamMax);

    /*--- M#: the Delta blocks of the edge loop and the diagonal blocks of the solver. ---*/
    const double coefDelta = 1.0 - ramp * kappa;
    const su2double gam = TeCNOAdjoint::GradientCoefficient(ramp, kappa);
    Eigen::MatrixXd M = Eigen::MatrixXd::Zero(nDof, nDof);
    for (auto e = 0ul; e < geometry.GetnEdge(); ++e) {
      const auto i = geometry.edges->GetNode(e, 0), k = geometry.edges->GetNode(e, 1);
      for (size_t a = 0; a < nVar; ++a) {
        for (size_t b = 0; b < nVar; ++b) {
          const double blk = 1.5 * coefDelta * coefDelta * D(e, a * nVar + b);
          M(nVar * i + a, nVar * i + b) += blk;
          M(nVar * k + a, nVar * k + b) += blk;
          M(nVar * i + a, nVar * k + b) -= blk;
          M(nVar * k + a, nVar * i + b) -= blk;
        }
      }
    }
    su2activematrix P(nPoint, nVar * nVar);
    if (p1) {
      TeCNOAdjoint::MajorantNodalP1(geometry, nDim, nVar, p1->weights, p1->volume, D, phi, gam, P);
    } else {
      TeCNOAdjoint::MajorantNodal(geometry, nDim, nVar, D, phi, gam, P);
    }
    for (auto i = 0ul; i < nPoint; ++i) {
      TeCNOAdjoint::Block blk;
      if (p1) {
        TeCNOAdjoint::MajorantDiagonalP1(geometry, nDim, nVar, p1->weights, P, i, blk);
      } else {
        TeCNOAdjoint::MajorantDiagonal(geometry, nDim, nVar, P, i, blk);
      }
      for (size_t a = 0; a < nVar; ++a)
        for (size_t b = 0; b < nVar; ++b) M(nVar * i + a, nVar * i + b) += blk(a, b);
    }
    Eigen::SelfAdjointEigenSolver<Eigen::MatrixXd> eigM(2.0 * M - As);
    res.majorant = std::max(res.majorant, -eigM.eigenvalues().minCoeff() / lamMax);

    /*--- TECNO_ADJOINT_JACOBIAN= FIRST_RING: J from the solver functions (the same calls as CEulerSolver), its edge
     * part against sum_e a_e a_e^T D_e with beta_{e,end} from EdgeJump of nodal indicator fields (independent of the
     * FirstRing formulas), and 2 J - A >= 0. ---*/
    auto firstRing = [&](const auto& weights, const auto& volume) {
      su2activematrix Sn(nPoint, nDim + 1 + nDim * nDim), Pn(nPoint, nVar * nVar), P0(nPoint, nVar * nVar);
      P0 = su2double(0.0);
      TeCNOAdjoint::FirstRingNodal(geometry, nDim, weights, volume, Sn);
      TeCNOAdjoint::FirstRingNodalP(geometry, nDim, nVar, D, Sn, phi, gam, Pn);
      auto assemble = [&](const su2activematrix& P) {
        Eigen::MatrixXd J = Eigen::MatrixXd::Zero(nDof, nDof);
        for (auto i = 0ul; i < nPoint; ++i) {
          TeCNOAdjoint::Block diag;
          TeCNOAdjoint::FirstRingRow(geometry, nDim, nVar, weights, Sn, P, D, phi, coefDelta, gam, i, diag,
                                     [&](unsigned long k, const TeCNOAdjoint::Block& off) {
            for (size_t a = 0; a < nVar; ++a)
              for (size_t b = 0; b < nVar; ++b) J(nVar * i + a, nVar * k + b) += off(a, b);
          });
          for (size_t a = 0; a < nVar; ++a)
            for (size_t b = 0; b < nVar; ++b) J(nVar * i + a, nVar * i + b) += diag(a, b);
        }
        return J;
      };
      su2activematrix beta(geometry.GetnEdge(), 2);
      for (auto m = 0ul; m < nPoint; ++m) {
        su2activematrix w(nPoint, nVar);
        w = su2double(0.0);
        for (size_t v = 0; v < nVar; ++v) w(m, v) = 1.0;
        CVectorOfMatrix grad(nPoint, nVar, nDim, 0.0);
        if (p1) {
          TeCNOAdjoint::GradientP1(geometry, nDim, nVar, p1->weights, p1->volume, w, grad);
        } else {
          TeCNOAdjoint::Gradient(geometry, nDim, nVar, w, grad);
        }
        for (auto e : geometry.nodes->GetEdges(m)) {
          const auto i = geometry.edges->GetNode(e, 0), k = geometry.edges->GetNode(e, 1);
          Vector<su2double, nVar> dw, L;
          Vector<su2double, nDim> edge;
          for (size_t v = 0; v < nVar; ++v) dw(v) = w(k, v) - w(i, v);
          for (size_t d = 0; d < nDim; ++d) edge(d) = geometry.nodes->GetCoord(k)[d] - geometry.nodes->GetCoord(i)[d];
          TeCNOAdjoint::EdgeJump(nDim, nVar, dw, grad[i], grad[k], phi(i), phi(k), edge, ramp, kappa, L);
          beta(e, (i == m) ? 0 : 1) = L(0);
        }
      }
      Eigen::MatrixXd Jref = Eigen::MatrixXd::Zero(nDof, nDof);
      for (auto e = 0ul; e < geometry.GetnEdge(); ++e) {
        const unsigned long node[2] = {geometry.edges->GetNode(e, 0), geometry.edges->GetNode(e, 1)};
        for (int x = 0; x < 2; ++x)
          for (int y = 0; y < 2; ++y)
            for (size_t a = 0; a < nVar; ++a)
              for (size_t b = 0; b < nVar; ++b)
                Jref(nVar * node[x] + a, nVar * node[y] + b) += beta(e, x) * beta(e, y) * D(e, a * nVar + b);
      }
      const Eigen::MatrixXd Jedge = assemble(P0);
      res.firstRingEdge = std::max(res.firstRingEdge, (Jedge - Jref).norm() / Jref.norm());
      const Eigen::MatrixXd J = assemble(Pn);
      const Eigen::MatrixXd Js = 0.5 * (J + J.transpose());
      Eigen::SelfAdjointEigenSolver<Eigen::MatrixXd> eigJ(2.0 * Js - As);
      res.firstRing = std::max(res.firstRing, -eigJ.eigenvalues().minCoeff() / lamMax);
      Eigen::SelfAdjointEigenSolver<Eigen::MatrixXd> eigJs(Js);
      res.firstRingSpread = std::max(res.firstRingSpread, lamMax / eigJs.eigenvalues().maxCoeff());

      /*--- TECNO_ADJOINT_JACOBIAN= FIRST_RING_JUMPS: J from the solver functions (edge weights w = trace(D), H = I
       * here), against J_ref assembled pair by pair from the definition with independent loops (E_x from the
       * coordinates, tau and nu as direct sums over the pairs, alpha from the EdgeJump coefficients beta), and
       * J - A >= 0, J 1 = 0, J = J^T. ---*/
      auto edgeW = [&](unsigned long e) {
        su2double tr = 0.0;
        for (size_t a = 0; a < nVar; ++a) tr += D(e, a * nVar + a);
        return tr;
      };
      su2activematrix Sj(nPoint, 3 + 2 * nDim * nDim), Qj(nPoint, nVar * nVar);
      TeCNOAdjoint::JumpNodal(geometry, nDim, weights, volume, edgeW, Sj);
      TeCNOAdjoint::JumpNodalQ(geometry, nDim, nVar, weights, Sj, phi, coefDelta, gam, D, Qj);
      Eigen::MatrixXd Jj = Eigen::MatrixXd::Zero(nDof, nDof);
      for (auto i = 0ul; i < nPoint; ++i) {
        TeCNOAdjoint::Block diag;
        TeCNOAdjoint::JumpRow(geometry, nDim, nVar, weights, Sj, Qj, D, phi, coefDelta, gam, i, diag,
                              [&](unsigned long k, const TeCNOAdjoint::Block& off) {
          for (size_t a = 0; a < nVar; ++a)
            for (size_t b = 0; b < nVar; ++b) Jj(nVar * i + a, nVar * k + b) += off(a, b);
        });
        for (size_t a = 0; a < nVar; ++a)
          for (size_t b = 0; b < nVar; ++b) Jj(nVar * i + a, nVar * i + b) += diag(a, b);
      }
      /*--- Reference: per node the pairs (m, a_xm, edge), E_x, mu_xm. ---*/
      struct Pair { unsigned long m, edge; double a[3]; };
      std::vector<std::vector<Pair>> pairs(nPoint);
      for (auto e = 0ul; e < geometry.GetnEdge(); ++e) {
        const unsigned long node[2] = {geometry.edges->GetNode(e, 0), geometry.edges->GetNode(e, 1)};
        for (int x = 0; x < 2; ++x) {
          Pair pr{node[1 - x], e, {0.0, 0.0, 0.0}};
          for (size_t d = 0; d < nDim; ++d) pr.a[d] = weights(e, x * nDim + d);
          pairs[node[x]].push_back(pr);
        }
      }
      std::vector<Eigen::MatrixXd> Ex(nPoint, Eigen::MatrixXd::Zero(nDim, nDim));
      std::vector<std::vector<double>> mu(nPoint);
      for (auto x = 0ul; x < nPoint; ++x) {
        double sw = 0.0;
        for (const auto& pr : pairs[x]) sw += edgeW(pr.edge);
        for (const auto& pr : pairs[x]) {
          Eigen::VectorXd f(nDim);
          for (size_t d = 0; d < nDim; ++d) f(d) = geometry.nodes->GetCoord(pr.m)[d] - geometry.nodes->GetCoord(x)[d];
          Ex[x] += ((sw > 0.0) ? edgeW(pr.edge) / sw : 1.0 / pairs[x].size()) * f * f.transpose();
        }
        for (const auto& pr : pairs[x]) {
          Eigen::VectorXd av(nDim);
          for (size_t d = 0; d < nDim; ++d) av(d) = pr.a[d];
          mu[x].push_back(std::sqrt(std::max(0.0, av.dot(Ex[x] * av))));
        }
      }
      Eigen::MatrixXd Jr = Eigen::MatrixXd::Zero(nDof, nDof);
      auto addPair = [&](unsigned long p, unsigned long q, double w, unsigned long e) {
        for (size_t a = 0; a < nVar; ++a) {
          for (size_t b = 0; b < nVar; ++b) {
            const double blk = w * D(e, a * nVar + b);
            Jr(nVar * p + a, nVar * p + b) += blk;
            Jr(nVar * q + a, nVar * q + b) += blk;
            Jr(nVar * p + a, nVar * q + b) -= blk;
            Jr(nVar * q + a, nVar * p + b) -= blk;
          }
        }
      };
      for (auto e = 0ul; e < geometry.GetnEdge(); ++e) {
        const unsigned long node[2] = {geometry.edges->GetNode(e, 0), geometry.edges->GetNode(e, 1)};
        double ev[3] = {0.0, 0.0, 0.0};
        for (size_t d = 0; d < nDim; ++d) ev[d] = geometry.nodes->GetCoord(node[1])[d] - geometry.nodes->GetCoord(node[0])[d];
        double t[2] = {0.0, 0.0}, r[2] = {0.0, 0.0}, sumOther[2] = {0.0, 0.0};
        for (int x = 0; x < 2; ++x) {
          const auto p = node[x];
          const double g = std::fabs(gam * phi(p) / volume(p));
          double tau = 0.0, nu = 0.0;
          for (size_t j = 0; j < pairs[p].size(); ++j) {
            const auto& pr = pairs[p][j];
            double ae = 0.0;
            for (size_t d = 0; d < nDim; ++d) ae += pr.a[d] * ev[d];
            if (pr.m == node[1 - x]) continue;
            sumOther[x] += gam * phi(p) / volume(p) * ae;
            if (mu[p][j] > 0.0) tau += ae * ae / mu[p][j];
            nu += mu[p][j];
          }
          t[x] = g * std::sqrt(tau * nu);
          r[x] = (nu > 0.0) ? g * std::sqrt(tau / nu) : 0.0;
        }
        /*--- alpha from beta of the end 1: beta_{e,1} = alpha - sum_{m != 0} c^1_m. ---*/
        const double alpha = beta(e, 1) + sumOther[1];
        su2double alphaCode, sumT, rc[2], muc[2];
        TeCNOAdjoint::JumpEdge(geometry, nDim, weights, Sj, phi, coefDelta, gam, e, alphaCode, sumT, rc, muc);
        res.jumpsAlpha = std::max(res.jumpsAlpha, std::fabs(alphaCode - alpha) / (std::fabs(alpha) + 1.0));
        const double S = std::fabs(alpha) + t[0] + t[1];
        addPair(node[0], node[1], S * std::fabs(alpha), e);
        for (int x = 0; x < 2; ++x) {
          const auto p = node[x];
          for (size_t j = 0; j < pairs[p].size(); ++j)
            if (pairs[p][j].m != node[1 - x]) addPair(p, pairs[p][j].m, S * r[x] * mu[p][j], e);
        }
      }
      res.jumpsFinite = res.jumpsFinite && Jj.allFinite() && Jr.allFinite();
      res.jumpsRef = std::max(res.jumpsRef, (Jj - Jr).norm() / Jr.norm());
      res.jumpsAsym = std::max(res.jumpsAsym, (Jj - Jj.transpose()).norm() / Jj.norm());
      for (auto i = 0ul; i < nPoint; ++i) {
        for (size_t a = 0; a < nVar; ++a) {
          for (size_t b = 0; b < nVar; ++b) {
            double sum = 0.0;
            for (auto k = 0ul; k < nPoint; ++k) sum += Jj(nVar * i + a, nVar * k + b);
            res.jumpsRowSum = std::max(res.jumpsRowSum, std::fabs(sum) / Jj.norm());
          }
        }
      }
      const Eigen::MatrixXd Jjs = 0.5 * (Jj + Jj.transpose());
      Eigen::SelfAdjointEigenSolver<Eigen::MatrixXd> eigJj(Jjs - As);
      res.jumpsFinite = res.jumpsFinite && (eigJj.info() == Eigen::Success) && std::isfinite(lamMax);
      res.jumps = std::max(res.jumps, -eigJj.eigenvalues().minCoeff() / lamMax);
    };
    if (p1) {
      firstRing(p1->weights, p1->volume);
    } else {
      auto volume = [&](unsigned long i) { return geometry.nodes->GetVolume(i); };
      firstRing(TeCNOAdjoint::GreenGaussWeights<Geometry>{geometry, nDim}, volume);
    }
  }
  return res;
}

/*!
 * \brief Largest |L_e| (relative to |w_k - w_i| if relative) over the edges whose two nodes satisfy interior(i), for
 *        phi = 1, ramp 1, kappa 0 and a field w(x) given at the nodes.
 */
template <size_t nDim, class Geometry, class Interior>
double MaxJump(const Geometry& geometry, const su2activematrix& w, const Interior& interior, bool relative) {
  constexpr size_t nVar = nDim + 2;
  const auto nPoint = w.rows();
  CVectorOfMatrix grad(nPoint, nVar, nDim, 0.0);
  TeCNOAdjoint::Gradient(geometry, nDim, nVar, w, grad);
  double worst = 0.0;
  for (auto e = 0ul; e < geometry.GetnEdge(); ++e) {
    const auto i = geometry.edges->GetNode(e, 0), k = geometry.edges->GetNode(e, 1);
    if (!interior(i) || !interior(k)) continue;
    Vector<su2double, nVar> dw, L;
    Vector<su2double, nDim> edge;
    for (size_t v = 0; v < nVar; ++v) dw(v) = w(k, v) - w(i, v);
    for (size_t d = 0; d < nDim; ++d) edge(d) = geometry.nodes->GetCoord(k)[d] - geometry.nodes->GetCoord(i)[d];
    TeCNOAdjoint::EdgeJump(nDim, nVar, dw, grad[i], grad[k], su2double(1.0), su2double(1.0), edge, 1.0, 0.0, L);
    for (size_t v = 0; v < nVar; ++v) {
      worst = std::max(worst, relative ? fabs(L(v)) / std::max(fabs(dw(v)), 1e-300) : fabs(L(v)));
    }
  }
  return worst;
}

/*!
 * \brief Largest relative error of ismailRoeProjJac against central differences of ismailRoeProjFlux in the
 *        conservative variables, random states at relative distance spread, both node blocks.
 */
template <size_t nDim>
double IsmailRoeJacobianError(std::mt19937& gen, double spread) {
  constexpr size_t nVar = nDim + 2;
  std::uniform_real_distribution<double> r(-1.0, 1.0);
  auto random = [&](double* U) {
    const double rho = exp(r(gen)), p = exp(r(gen));
    double q2 = 0.0;
    U[0] = rho;
    for (size_t d = 0; d < nDim; ++d) {
      const double u = 0.8 * r(gen);
      U[d + 1] = rho * u;
      q2 += u * u;
    }
    U[nDim + 1] = p / (GAMMA - 1.0) + 0.5 * rho * q2;
  };
  double U_i[nVar], U_k[nVar], other[nVar];
  random(U_i);
  random(other);
  for (size_t v = 0; v < nVar; ++v) U_k[v] = U_i[v] + spread * (other[v] - U_i[v]);
  VectorDbl<nDim> normal;
  for (size_t d = 0; d < nDim; ++d) normal(d) = r(gen);
  const auto V_i = PrimitivesOfConservatives<nDim>(U_i);
  const auto V_k = PrimitivesOfConservatives<nDim>(U_k);
  MatrixDbl<nVar> jac_i, jac_k;
  ismailRoeProjJac(Double(GAMMA), V_i, V_k, normal, jac_i, jac_k);

  double err = 0.0, ref = 0.0;
  for (int side = 0; side < 2; ++side) {
    const double* U0 = side ? U_k : U_i;
    for (size_t c = 0; c < nVar; ++c) {
      double Up[nVar], Um[nVar];
      for (size_t v = 0; v < nVar; ++v) Up[v] = Um[v] = U0[v];
      const double h = 1e-6 * std::max(1.0, fabs(U0[c]));
      Up[c] += h;
      Um[c] -= h;
      const auto Vp = PrimitivesOfConservatives<nDim>(Up), Vm = PrimitivesOfConservatives<nDim>(Um);
      const auto Fp = side ? ismailRoeProjFlux(Double(GAMMA), V_i, Vp, normal) : ismailRoeProjFlux(Double(GAMMA), Vp, V_k, normal);
      const auto Fm = side ? ismailRoeProjFlux(Double(GAMMA), V_i, Vm, normal) : ismailRoeProjFlux(Double(GAMMA), Vm, V_k, normal);
      for (size_t v = 0; v < nVar; ++v) {
        const double fd = (Fp(v)[0] - Fm(v)[0]) / (2.0 * h);
        const double exact = side ? jac_k(v, c)[0] : jac_i(v, c)[0];
        err = std::max(err, fabs(fd - exact));
        ref = std::max(ref, fabs(fd));
      }
    }
  }
  return err / ref;
}

/*!
 * \brief Equal states: both blocks are 1/2 A(n). Returns the largest relative deviation.
 */
template <size_t nDim>
double IsmailRoeJacobianEqualStates(std::mt19937& gen) {
  constexpr size_t nVar = nDim + 2;
  std::uniform_real_distribution<double> r(-1.0, 1.0);
  double u[3] = {0.0};
  for (size_t d = 0; d < nDim; ++d) u[d] = r(gen);
  const auto V = Primitives<nDim>(exp(r(gen)), u, exp(r(gen)));
  VectorDbl<nDim> normal;
  for (size_t d = 0; d < nDim; ++d) normal(d) = r(gen);
  MatrixDbl<nVar> jac_i, jac_k;
  ismailRoeProjJac(Double(GAMMA), V, V, normal, jac_i, jac_k);
  const auto U = compressibleConservatives(V);
  const auto A = inviscidProjJac(Double(GAMMA), V.velocity(), U.energy(), normal, Double(0.5));
  double err = 0.0, ref = 0.0;
  for (size_t a = 0; a < nVar; ++a) {
    for (size_t b = 0; b < nVar; ++b) {
      err = std::max(err, std::max(fabs(jac_i(a, b)[0] - A(a, b)[0]), fabs(jac_k(a, b)[0] - A(a, b)[0])));
      ref = std::max(ref, fabs(A(a, b)[0]));
    }
  }
  return err / ref;
}

/*!
 * \brief dw/dU of TeCNOAdjoint::EntropyHessian against central differences of EntropyVariables.
 */
inline double EntropyHessianError(std::mt19937& gen, unsigned short nDim) {
  std::uniform_real_distribution<double> r(-1.0, 1.0);
  const unsigned short nVar = nDim + 2;
  const double rho = exp(r(gen)), p = exp(r(gen));
  double u[3] = {0.0}, q2 = 0.0;
  for (unsigned short d = 0; d < nDim; ++d) {
    u[d] = r(gen);
    q2 += u[d] * u[d];
  }
  TeCNOAdjoint::Block H;
  TeCNOAdjoint::EntropyHessian(nDim, GAMMA, rho, u, p, H);
  double U[5] = {rho, 0.0, 0.0, 0.0, 0.0};
  for (unsigned short d = 0; d < nDim; ++d) U[d + 1] = rho * u[d];
  U[nDim + 1] = p / (GAMMA - 1.0) + 0.5 * rho * q2;
  auto entropy = [&](const double* Uc, double* w) {
    double uc[3] = {0.0}, s2 = 0.0;
    for (unsigned short d = 0; d < nDim; ++d) {
      uc[d] = Uc[d + 1] / Uc[0];
      s2 += uc[d] * uc[d];
    }
    TeCNOAdjoint::EntropyVariables(nDim, GAMMA, Uc[0], uc, (GAMMA - 1.0) * (Uc[nDim + 1] - 0.5 * Uc[0] * s2), w);
  };
  double err = 0.0, ref = 0.0;
  for (unsigned short c = 0; c < nVar; ++c) {
    double Up[5], Um[5], wp[5], wm[5];
    for (unsigned short v = 0; v < nVar; ++v) Up[v] = Um[v] = U[v];
    const double h = 1e-6 * std::max(1.0, fabs(U[c]));
    Up[c] += h;
    Um[c] -= h;
    entropy(Up, wp);
    entropy(Um, wm);
    for (unsigned short v = 0; v < nVar; ++v) {
      const double fd = (wp[v] - wm[v]) / (2.0 * h);
      err = std::max(err, fabs(fd - H(v, c)));
      ref = std::max(ref, fabs(fd));
    }
  }
  return err / ref;
}

}  // namespace TeCNOAdjointChecks
