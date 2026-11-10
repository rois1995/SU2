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
#include <map>
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

/*--- TECNO_ADJOINT_LIMITER= EDGE_ENTROPY_TVD (ROE_TECNO_PLAN.md 6.6): the solver passes of TeCNOAdjoint (the kernel's
 * ingredients function, TVDEdgeClip, TVDNodeFactor, TVDEdgePhi) on a geometry, an independent reference of the model-4
 * formulas (P1_TECNO_ADJ_LIMITER.md, fourth candidate (s); the shear block projected on t), the frozen-field box of the
 * final values, Theorem A1 with the stored Phi and FIRST_RING_JUMPS with the stored Phi. ---*/

template <size_t nDim>
struct TVDEdgeState {
  CRoeVariables<nDim> avg;
  VectorDbl<nDim> unit;
  Double lamM, lam0, lamP, scale;
  CPair<CCompressiblePrimitives<nDim, nDim + 4>> V;
};

/*!
 * \brief Frozen edge states as in the kernel: Roe average of the nodal states on nu (projected, or the dual normal),
 *        eigenvalue floor 0.001 (|u.n| + c), scale 1/2 |nu|.
 */
template <size_t nDim, class Geometry>
std::vector<TVDEdgeState<nDim>> TVDEdgeStates(const Geometry& geometry, const States& S, bool projected) {
  std::vector<TVDEdgeState<nDim>> es(geometry.GetnEdge());
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
    auto& st = es[e];
    for (size_t d = 0; d < nDim; ++d) st.unit(d) = nu[d] / area;
    st.V.i = Primitives<nDim>(S.rho[i], S.u[i].data(), S.p[i]);
    st.V.j = Primitives<nDim>(S.rho[k], S.u[k].data(), S.p[k]);
    st.avg = roeAveragedVariables(Double(GAMMA), st.V, st.unit);
    const Double lamMin = 0.001 * (abs(st.avg.projVel) + st.avg.speedSound);
    st.lamM = fmax(abs(st.avg.projVel - st.avg.speedSound), lamMin);
    st.lam0 = fmax(abs(st.avg.projVel), lamMin);
    st.lamP = fmax(abs(st.avg.projVel + st.avg.speedSound), lamMin);
    st.scale = 0.5 * area;
  }
  return es;
}

/*!
 * \brief D_e (dense, row major) of the frozen edge states.
 */
template <size_t nDim>
su2activematrix TVDFrozenD(const std::vector<TVDEdgeState<nDim>>& es) {
  constexpr size_t nVar = nDim + 2;
  su2activematrix D(es.size(), nVar * nVar);
  for (size_t e = 0; e < es.size(); ++e) {
    const auto& st = es[e];
    for (size_t col = 0; col < nVar; ++col) {
      VectorDbl<nVar> v;
      for (size_t r = 0; r < nVar; ++r) v(r) = (r == col) ? 1.0 : 0.0;
      const auto q = tecnoDissipation(Double(GAMMA), st.avg, st.unit, st.lamM, st.lam0, st.lamP, st.scale, v);
      for (size_t r = 0; r < nVar; ++r) D(e, r * nVar + col) = q(r)[0];
    }
  }
  return D;
}

template <size_t nDim, class Geometry>
CVectorOfMatrix TVDGradientOf(const Geometry& geometry, const su2activematrix& w, const P1Average* p1) {
  CVectorOfMatrix grad(w.rows(), nDim + 2, nDim, 0.0);
  if (p1) {
    TeCNOAdjoint::GradientP1(geometry, nDim, nDim + 2, p1->weights, p1->volume, w, grad);
  } else {
    TeCNOAdjoint::Gradient(geometry, nDim, nDim + 2, w, grad);
  }
  return grad;
}

/*!
 * \brief Ingredients of every local edge from w, its gradient (halo values included) and the frozen edge states.
 */
template <size_t nDim, class Geometry>
su2activematrix TVDIngredientsOf(const Geometry& geometry, const su2activematrix& w, const CVectorOfMatrix& grad,
                                 const std::vector<TVDEdgeState<nDim>>& es) {
  constexpr size_t nVar = nDim + 2;
  const unsigned short width = TeCNOAdjoint::TVDWidth(nDim);
  su2activematrix ing(geometry.GetnEdge(), width);
  for (auto e = 0ul; e < geometry.GetnEdge(); ++e) {
    const auto i = geometry.edges->GetNode(e, 0), k = geometry.edges->GetNode(e, 1);
    VectorDbl<nVar> dw, p_i, p_k;
    for (size_t v = 0; v < nVar; ++v) {
      dw(v) = w(k, v) - w(i, v);
      p_i(v) = 0.0;
      p_k(v) = 0.0;
      for (size_t d = 0; d < nDim; ++d) {
        const double ed = geometry.nodes->GetCoord(k)[d] - geometry.nodes->GetCoord(i)[d];
        p_i(v) += grad(i, v, d) * ed;
        p_k(v) += grad(k, v, d) * ed;
      }
    }
    const auto& st = es[e];
    Double out[TeCNOAdjoint::TVD_SHEAR + 9];
    tecnoAdjointTVDIngredients(Double(GAMMA), st.avg, st.unit, st.lamM, st.lam0, st.lamP, st.scale, dw, p_i, p_k,
                               st.V.i, st.V.j, out);
    for (unsigned short c = 0; c < width; ++c) ing(e, c) = out[c][0];
  }
  return ing;
}

struct TVDData {
  su2activematrix ing, clip, rho, phi;
};

/*!
 * \brief The solver passes (B), (C), (D) on given ingredients (serial: all nodes owned, or rho halos filled by the
 *        caller through haloCopy(rho) between (C) and (D)).
 */
template <class Geometry, class WeightType, class VolumeType, class HaloCopy>
void TVDPasses(const Geometry& geometry, unsigned short nDim, const WeightType& weights, const VolumeType& volume,
               TVDData& d, HaloCopy&& haloCopy) {
  d.clip.resize(geometry.GetnEdge(), TeCNOAdjoint::TVD_CLIP_WIDTH);
  d.rho.resize(geometry.GetnPoint(), TeCNOAdjoint::TVD_NFIELD);
  d.phi.resize(geometry.GetnEdge(), 2 * TeCNOAdjoint::TVD_NFIELD);
  d.rho = su2double(1.0);
  TeCNOAdjoint::TVDEdgeClip(geometry, nDim, d.ing, d.clip);
  TeCNOAdjoint::TVDNodeFactor(geometry, nDim, weights, volume, d.ing, d.clip, d.rho);
  haloCopy(d.rho);
  TeCNOAdjoint::TVDEdgePhi(geometry, d.clip, d.rho, d.phi);
}

/*--- Independent reference: canonical data of each edge built from the coordinates and the ingredient columns. ---*/
struct TVDRefEdge {
  unsigned long n[2] = {0, 0};
  bool flip = false, noise = false;
  double e[3] = {0, 0, 0}, dy[4] = {0, 0, 0, 0}, s[2][4] = {{0}}, D[4] = {0, 0, 0, 0}, d[4] = {0, 0, 0, 0};
  double dyS[3] = {0, 0, 0}, sS[2][3] = {{0}}, t[3] = {0, 0, 0}, B[4] = {0, 0, 0, 0}, P1[2][4] = {{0}};
};

template <class Geometry>
std::vector<TVDRefEdge> TVDRefEdges(const Geometry& geometry, unsigned short nDim, const su2activematrix& ing) {
  namespace ta = TeCNOAdjoint;
  std::vector<TVDRefEdge> ed(geometry.GetnEdge());
  for (auto e = 0ul; e < geometry.GetnEdge(); ++e) {
    auto& r = ed[e];
    const unsigned long a = geometry.edges->GetNode(e, 0), b = geometry.edges->GetNode(e, 1);
    std::array<double, 3> xa = {0, 0, 0}, xb = {0, 0, 0};
    for (unsigned short d = 0; d < nDim; ++d) {
      xa[d] = geometry.nodes->GetCoord(a)[d];
      xb[d] = geometry.nodes->GetCoord(b)[d];
    }
    r.flip = std::lexicographical_compare(xb.begin(), xb.begin() + nDim, xa.begin(), xa.begin() + nDim);
    r.n[0] = r.flip ? b : a;
    r.n[1] = r.flip ? a : b;
    for (unsigned short d = 0; d < nDim; ++d)
      r.e[d] = geometry.nodes->GetCoord(r.n[1])[d] - geometry.nodes->GetCoord(r.n[0])[d];
    const double sg = r.flip ? -1.0 : 1.0;
    /*--- Canonical field M is the local field P of a flipped edge (opposite normal). Local columns: M 0, 0 1, P 2. ---*/
    const int scalarCol[4] = {r.flip ? 2 : 0, 1, -1, r.flip ? 0 : 2};
    const int dCol[4] = {r.flip ? 2 : 0, 1, 3, r.flip ? 0 : 2};
    for (int f = 0; f < 4; ++f) {
      r.D[f] = ing(e, ta::TVD_D + dCol[f]);
      r.d[f] = ing(e, ta::TVD_GUARD + ((f == 2) ? 1 : 0));
      if (f == 2) continue;
      r.dy[f] = sg * ing(e, ta::TVD_DY + scalarCol[f]);
      r.s[0][f] = sg * ing(e, (r.flip ? ta::TVD_S1 : ta::TVD_S0) + scalarCol[f]);
      r.s[1][f] = sg * ing(e, (r.flip ? ta::TVD_S0 : ta::TVD_S1) + scalarCol[f]);
    }
    double norm2 = 0.0;
    for (unsigned short d = 0; d < nDim; ++d) {
      r.dyS[d] = sg * ing(e, ta::TVD_SHEAR + d);
      r.sS[0][d] = sg * ing(e, ta::TVD_SHEAR + (r.flip ? 2 : 1) * nDim + d);
      r.sS[1][d] = sg * ing(e, ta::TVD_SHEAR + (r.flip ? 1 : 2) * nDim + d);
      norm2 += r.dyS[d] * r.dyS[d];
    }
    const double norm = std::sqrt(norm2);
    r.noise = (norm <= (nDim + 2) * r.d[2]);
    for (unsigned short d = 0; d < nDim; ++d) r.t[d] = r.noise ? 0.0 : r.dyS[d] / norm;
    r.dy[2] = r.noise ? 0.0 : norm;
    for (int x = 0; x < 2; ++x) {
      double st = 0.0, ss = 0.0;
      for (unsigned short d = 0; d < nDim; ++d) {
        st += r.sS[x][d] * r.t[d];
        ss += r.sS[x][d] * r.sS[x][d];
      }
      r.s[x][2] = r.noise ? std::sqrt(ss) : st;
    }
    for (int f = 0; f < 4; ++f) {
      double B = 0.0;
      if (!((f == 2) && r.noise)) {
        const double c[3] = {r.dy[f], 2 * r.s[0][f] - r.dy[f], 2 * r.s[1][f] - r.dy[f]};
        const bool pos = (c[0] > 0) && (c[1] > 0) && (c[2] > 0), neg = (c[0] < 0) && (c[1] < 0) && (c[2] < 0);
        const double m = std::min(std::fabs(c[0]), std::min(std::fabs(c[1]), std::fabs(c[2])));
        B = 2.0 * (pos ? m : (neg ? -m : 0.0));
      }
      r.B[f] = B;
      const double lo = std::min(0.0, B) - r.d[f], hi = std::max(0.0, B) + r.d[f];
      for (int x = 0; x < 2; ++x) {
        const double sv = r.s[x][f];
        r.P1[x][f] = (sv > hi) ? hi / sv : ((sv < lo) ? lo / sv : 1.0);
      }
    }
  }
  return ed;
}

/*--- Projection of a source quantity of edge k on the frame of the target (f != 2: the scalar; f = 2: the vector
 * dotted with t of the target). which: 0 dy, 1 slope of the canonical end x. ---*/
inline double TVDRefValue(const TVDRefEdge& src, const TVDRefEdge& tgt, int f, int which, int x, unsigned short nDim) {
  if (f != 2) return (which == 0) ? src.dy[f] : src.s[x][f];
  double v = 0.0;
  for (unsigned short d = 0; d < nDim; ++d) v += ((which == 0) ? src.dyS[d] : src.sS[x][d]) * tgt.t[d];
  return v;
}

/*!
 * \brief Reference Phi (local layout of the store) from the model-4 formulas; t_up by bisection on the nondecreasing
 *        majorant (not the closed form). activity: fraction of (edge end, field) with Phi < 1.
 */
template <class Geometry, class WeightType, class VolumeType>
su2activematrix TVDReference(const Geometry& geometry, unsigned short nDim, const su2activematrix& ing,
                             const WeightType& weights, const VolumeType& volume, double* activity = nullptr) {
  const auto ed = TVDRefEdges(geometry, nDim, ing);
  const auto nP = geometry.GetnPoint();
  std::vector<std::vector<std::pair<unsigned long, int>>> at(nP);
  for (unsigned long e = 0; e < ed.size(); ++e)
    for (int x = 0; x < 2; ++x) at[ed[e].n[x]].push_back({e, x});
  std::vector<std::array<double, 4>> rho(nP, {1.0, 1.0, 1.0, 1.0});
  auto majorant = [](double a, double q, double r) {
    /*--- m(r) of f(r) = a r + q r^2: f up to its peak (concave case), then the line a r / 2; max_{s<=r} f(s) else. ---*/
    if ((q < 0.0) && (a > 0.0)) {
      const double rs = -a / (2.0 * q);
      return (r <= rs) ? a * r + q * r * r : 0.5 * a * r;
    }
    return std::max(0.0, a * r + q * r * r);
  };
  for (unsigned long a = 0; a < nP; ++a) {
    for (const auto& h : at[a]) {
      const auto& th = ed[h.first];
      const int x = h.second;
      const unsigned short lend = (geometry.edges->GetNode(h.first, 0) == a) ? 0 : 1;
      double g[3] = {0, 0, 0};
      for (unsigned short d = 0; d < nDim; ++d) g[d] = weights(h.first, lend * nDim + d) / volume(a);
      const double sigma = (x == 0) ? 1.0 : -1.0;
      for (int f = 0; f < 4; ++f) {
        if (!(th.D[f] > 0.0) || ((f == 2) && th.noise)) continue;
        double A = 0.0, Q = 0.0, Tp = 0.0, Tm = 0.0;
        for (const auto& k : at[a]) {
          const auto& sk = ed[k.first];
          const int xk = k.second;
          double pi = 0.0;
          for (unsigned short d = 0; d < nDim; ++d) pi += g[d] * sk.e[d];
          const double w = sigma * pi / th.D[f];
          const double a1 = sk.P1[xk][f], ao = sk.P1[1 - xk][f];
          A += w * a1 * sk.D[f] * TVDRefValue(sk, th, f, 0, 0, nDim);
          Q += -0.5 * w * a1 * a1 * sk.D[f] * TVDRefValue(sk, th, f, 1, xk, nDim);
          const double tc = -0.5 * w * a1 * sk.D[f] * ao * TVDRefValue(sk, th, f, 1, 1 - xk, nDim);
          Tp += std::max(0.0, tc);
          Tm += std::min(0.0, tc);
        }
        const double own0 = th.P1[x][f] * th.s[x][f];
        const double sg = (th.B[f] >= 0.0) ? 1.0 : -1.0;
        const double own = sg * own0, Ar = sg * A, Qr = sg * Q;
        const double tp = (sg > 0) ? Tp : -Tm, tm = (sg > 0) ? Tm : -Tp;
        const double hi = std::fabs(th.B[f]) + th.d[f];
        const double U1 = own + Ar + tp;
        double tUp = 1.0;
        if (majorant(U1, Qr, 1.0) > hi) {
          double lo = 0.0, up = 1.0;
          for (int it = 0; it < 200; ++it) {
            const double mid = 0.5 * (lo + up);
            if (majorant(U1, Qr, mid) <= hi) lo = mid; else up = mid;
          }
          tUp = lo;
        }
        const double md = -(Ar + tm + std::min(0.0, Qr));
        const double v = std::min(1.0, std::max(0.0, md) / std::max(own + th.d[f], std::numeric_limits<double>::min()));
        rho[a][f] *= tUp * (1.0 - v) * (1.0 - v) * (1.0 + 2.0 * v);
      }
    }
  }
  su2activematrix phi(ed.size(), 8);
  unsigned long active = 0;
  for (unsigned long e = 0; e < ed.size(); ++e) {
    for (int x = 0; x < 2; ++x) {
      for (int f = 0; f < 4; ++f) {
        const int y = ed[e].flip ? 1 - x : x;
        const int fl = ed[e].flip ? ((f == 0) ? 3 : ((f == 3) ? 0 : f)) : f;
        phi(e, 4 * y + fl) = ed[e].P1[x][f] * rho[ed[e].n[x]][f];
        active += (phi(e, 4 * y + fl) < 1.0 - 1e-12);
      }
    }
  }
  if (activity) *activity = double(active) / (8.0 * ed.size());
  return phi;
}

/*!
 * \brief Largest relative violation of the frozen-field box by the final values phi (local layout) over the
 *        constrained half-edges and fields (D > 0, not a noise shear edge).
 */
template <class Geometry, class WeightType, class VolumeType>
double TVDBoxViolation(const Geometry& geometry, unsigned short nDim, const su2activematrix& ing,
                       const su2activematrix& phi, const WeightType& weights, const VolumeType& volume) {
  const auto ed = TVDRefEdges(geometry, nDim, ing);
  std::vector<std::vector<std::pair<unsigned long, int>>> at(geometry.GetnPoint());
  for (unsigned long e = 0; e < ed.size(); ++e)
    for (int x = 0; x < 2; ++x) at[ed[e].n[x]].push_back({e, x});
  auto Phi = [&](unsigned long e, int x, int f) {
    const int y = ed[e].flip ? 1 - x : x;
    const int fl = ed[e].flip ? ((f == 0) ? 3 : ((f == 3) ? 0 : f)) : f;
    return double(phi(e, 4 * y + fl));
  };
  double worst = 0.0;
  for (unsigned long a = 0; a < at.size(); ++a) {
    for (const auto& h : at[a]) {
      const auto& th = ed[h.first];
      const int x = h.second;
      const unsigned short lend = (geometry.edges->GetNode(h.first, 0) == a) ? 0 : 1;
      double g[3] = {0, 0, 0};
      for (unsigned short d = 0; d < nDim; ++d) g[d] = weights(h.first, lend * nDim + d) / volume(a);
      const double sigma = (x == 0) ? 1.0 : -1.0;
      for (int f = 0; f < 4; ++f) {
        if (!(th.D[f] > 0.0) || ((f == 2) && th.noise)) continue;
        double sFin = Phi(h.first, x, f) * th.s[x][f], size = std::fabs(sFin);
        for (const auto& k : at[a]) {
          const auto& sk = ed[k.first];
          double pi = 0.0;
          for (unsigned short d = 0; d < nDim; ++d) pi += g[d] * sk.e[d];
          const double l = TVDRefValue(sk, th, f, 0, 0, nDim) -
                           0.5 * (Phi(k.first, 0, f) * TVDRefValue(sk, th, f, 1, 0, nDim) +
                                  Phi(k.first, 1, f) * TVDRefValue(sk, th, f, 1, 1, nDim));
          const double term = sigma * pi / th.D[f] * Phi(k.first, k.second, f) * sk.D[f] * l;
          sFin += term;
          size += std::fabs(term);
        }
        const double lo = std::min(0.0, th.B[f]) - th.d[f], hi = std::max(0.0, th.B[f]) + th.d[f];
        const double scale = std::fabs(th.B[f]) + std::fabs(th.dy[f]) + std::fabs(th.s[x][f]) + th.d[f] + size;
        worst = std::max(worst, std::max(0.0, std::max(sFin - hi, lo - sFin)) / scale);
      }
    }
  }
  return worst;
}

/*!
 * \brief Adjoint dissipation residual with per-edge-end values phi (local layout) and frozen edge states, any w:
 *        tecnoAdjointEdgePhi per edge, the Delta part scattered as the edge loop, NodalMomentEnds and the transpose.
 *        Returns R and sum_e L^T D L formed from the ingredient coefficients D (ing) and R^T L of the edge function.
 */
template <size_t nDim, class Geometry>
su2activematrix TVDResidual(const Geometry& geometry, const std::vector<TVDEdgeState<nDim>>& es,
                            const su2activematrix& phi, const su2activematrix& w, double ramp, const P1Average* p1,
                            const su2activematrix* ing, double& sumLDL) {
  constexpr size_t nVar = nDim + 2;
  const auto nPoint = w.rows();
  const auto grad = TVDGradientOf<nDim>(geometry, w, p1);
  su2activematrix Q(geometry.GetnEdge(), 2 * nVar), R(nPoint, nVar);
  R = su2double(0.0);
  sumLDL = 0.0;
  for (auto e = 0ul; e < geometry.GetnEdge(); ++e) {
    const auto i = geometry.edges->GetNode(e, 0), k = geometry.edges->GetNode(e, 1);
    VectorDbl<nVar> dw, p_i, p_k, q, q_i, q_k;
    for (size_t v = 0; v < nVar; ++v) {
      dw(v) = w(k, v) - w(i, v);
      p_i(v) = 0.0;
      p_k(v) = 0.0;
      for (size_t d = 0; d < nDim; ++d) {
        const double ed = geometry.nodes->GetCoord(k)[d] - geometry.nodes->GetCoord(i)[d];
        p_i(v) += grad(i, v, d) * ed;
        p_k(v) += grad(k, v, d) * ed;
      }
    }
    Double ph[8], yL[nDim + 3];
    for (int c = 0; c < 8; ++c) ph[c] = phi(e, c);
    const auto& st = es[e];
    tecnoAdjointEdgePhi(Double(GAMMA), st.avg, st.unit, st.lamM, st.lam0, st.lamP, st.scale, dw, p_i, p_k, ramp, 0.0,
                        ph, q, q_i, q_k, yL);
    if (ing) {
      const auto& g = *ing;
      double ldl = g(e, TeCNOAdjoint::TVD_D + 0) * yL[0][0] * yL[0][0] + g(e, TeCNOAdjoint::TVD_D + 1) * yL[1][0] * yL[1][0] +
                   g(e, TeCNOAdjoint::TVD_D + 2) * yL[2][0] * yL[2][0];
      for (size_t d = 0; d < nDim; ++d) ldl += g(e, TeCNOAdjoint::TVD_D + 3) * yL[3 + d][0] * yL[3 + d][0];
      sumLDL += ldl;
    }
    for (size_t v = 0; v < nVar; ++v) {
      R(i, v) -= q(v)[0];
      R(k, v) += q(v)[0];
      Q(e, v) = q_i(v)[0];
      Q(e, nVar + v) = q_k(v)[0];
    }
  }
  CVectorOfMatrix Y(nPoint, nVar, nDim, 0.0);
  const su2double gam = TeCNOAdjoint::GradientCoefficient(ramp, 0.0);
  if (p1) {
    TeCNOAdjoint::NodalMomentEnds(geometry, nDim, nVar, Q, p1->volume, Y);
    TeCNOAdjoint::AddTransposeP1(geometry, nDim, nVar, p1->weights, Y, gam, R);
  } else {
    auto volume = [&](unsigned long iPoint) { return geometry.nodes->GetVolume(iPoint); };
    TeCNOAdjoint::NodalMomentEnds(geometry, nDim, nVar, Q, volume, Y);
    TeCNOAdjoint::AddTranspose(geometry, nDim, nVar, Y, gam, R);
  }
  return R;
}

struct TVDJumpsResult {
  double jumps = 0.0, rowSum = 0.0, asym = 0.0, ones = 0.0;
  bool finite = true;
};

/*!
 * \brief FIRST_RING_JUMPS with per-edge-end values phi: J from JumpNodal / JumpNodalQEnds / JumpRowEnds against the
 *        frozen linear residual matrix K (column by column), lambda_min(J - K) >= 0, J 1 = 0, J = J^T; and, for
 *        phi = 1 everywhere, JumpEdgeEnds = JumpEdge with nodal phi = 1 (ones).
 */
template <size_t nDim, class Geometry>
TVDJumpsResult TVDJumpsCheck(const Geometry& geometry, const std::vector<TVDEdgeState<nDim>>& es,
                             const su2activematrix& phi, double ramp, const P1Average* p1) {
  constexpr size_t nVar = nDim + 2;
  const auto nPoint = geometry.GetnPoint();
  const auto nDof = nVar * nPoint;
  TVDJumpsResult res;
  Eigen::MatrixXd K(nDof, nDof);
  for (auto col = 0ul; col < nDof; ++col) {
    su2activematrix w(nPoint, nVar);
    w = su2double(0.0);
    w(col / nVar, col % nVar) = 1.0;
    double dummy = 0.0;
    const auto R = TVDResidual<nDim>(geometry, es, phi, w, ramp, p1, nullptr, dummy);
    for (auto i = 0ul; i < nPoint; ++i)
      for (size_t v = 0; v < nVar; ++v) K(nVar * i + v, col) = R(i, v);
  }
  const Eigen::MatrixXd Ks = 0.5 * (K + K.transpose());
  Eigen::SelfAdjointEigenSolver<Eigen::MatrixXd> eigK(Ks);
  const double lamMax = std::max(eigK.eigenvalues().maxCoeff(), 1e-300);
  const auto D = TVDFrozenD<nDim>(es);
  const su2double gam = TeCNOAdjoint::GradientCoefficient(ramp, 0.0);
  const double coefDelta = 1.0;
  auto check = [&](const auto& weights, const auto& volume) {
    auto edgeW = [&](unsigned long e) {
      su2double tr = 0.0;
      for (size_t a = 0; a < nVar; ++a) tr += D(e, a * nVar + a);
      return tr;
    };
    su2activematrix Sj(nPoint, 3 + 2 * nDim * nDim), Qj(nPoint, nVar * nVar);
    TeCNOAdjoint::JumpNodal(geometry, nDim, weights, volume, edgeW, Sj);
    TeCNOAdjoint::JumpNodalQEnds(geometry, nDim, nVar, weights, Sj, phi, coefDelta, gam, D, Qj);
    Eigen::MatrixXd J = Eigen::MatrixXd::Zero(nDof, nDof);
    for (auto i = 0ul; i < nPoint; ++i) {
      TeCNOAdjoint::Block diag;
      TeCNOAdjoint::JumpRowEnds(geometry, nDim, nVar, weights, Sj, Qj, D, phi, coefDelta, gam, i, diag,
                                [&](unsigned long k, const TeCNOAdjoint::Block& off) {
        for (size_t a = 0; a < nVar; ++a)
          for (size_t b = 0; b < nVar; ++b) J(nVar * i + a, nVar * k + b) += off(a, b);
      });
      for (size_t a = 0; a < nVar; ++a)
        for (size_t b = 0; b < nVar; ++b) J(nVar * i + a, nVar * i + b) += diag(a, b);
    }
    res.finite = res.finite && J.allFinite() && K.allFinite();
    res.asym = std::max(res.asym, (J - J.transpose()).norm() / J.norm());
    for (auto i = 0ul; i < nPoint; ++i) {
      for (size_t a = 0; a < nVar; ++a) {
        for (size_t b = 0; b < nVar; ++b) {
          double sum = 0.0;
          for (auto k = 0ul; k < nPoint; ++k) sum += J(nVar * i + a, nVar * k + b);
          res.rowSum = std::max(res.rowSum, std::fabs(sum) / J.norm());
        }
      }
    }
    const Eigen::MatrixXd Js = 0.5 * (J + J.transpose());
    Eigen::SelfAdjointEigenSolver<Eigen::MatrixXd> eig(Js - Ks);
    res.finite = res.finite && (eig.info() == Eigen::Success);
    res.jumps = std::max(res.jumps, -eig.eigenvalues().minCoeff() / lamMax);
    /*--- Phi = 1: the edge quantities of JumpEdge with the nodal phi = 1. ---*/
    su2activematrix ones(geometry.GetnEdge(), 8);
    ones = su2double(1.0);
    su2activevector phiNode(nPoint);
    phiNode = su2double(1.0);
    for (auto e = 0ul; e < geometry.GetnEdge(); ++e) {
      su2double a0, s0, r0[2], m0[2], a1, s1, r1[2], m1[2];
      TeCNOAdjoint::JumpEdge(geometry, nDim, weights, Sj, phiNode, coefDelta, gam, e, a0, s0, r0, m0);
      TeCNOAdjoint::JumpEdgeEnds(geometry, nDim, weights, Sj, ones, coefDelta, gam, e, a1, s1, r1, m1);
      double dev = std::fabs(std::fabs(a0) - a1) + std::fabs(s0 - s1);
      for (int x = 0; x < 2; ++x) dev += std::fabs(r0[x] - r1[x]) + std::fabs(m0[x] - m1[x]);
      res.ones = std::max(res.ones, dev / (std::fabs(s0) + 1e-300));
    }
  };
  if (p1) {
    check(p1->weights, p1->volume);
  } else {
    auto volume = [&](unsigned long i) { return geometry.nodes->GetVolume(i); };
    check(TeCNOAdjoint::GreenGaussWeights<Geometry>{geometry, nDim}, volume);
  }
  return res;
}

/*!
 * \brief Copies the owned rows of data (nPoint x width) to the halo rows of every rank through an Allgatherv keyed by
 *        the global index (an independent stand-in for the solver's CommPointData in the unit tests).
 */
template <class Geometry>
void TVDHaloCopy(const Geometry& geometry, su2activematrix& data) {
  const unsigned long width = data.cols(), nDomain = geometry.GetnPointDomain();
  std::vector<double> send;
  for (auto i = 0ul; i < nDomain; ++i) {
    send.push_back(double(geometry.nodes->GetGlobalIndex(i)));
    for (auto c = 0ul; c < width; ++c) send.push_back(data(i, c));
  }
  const int size = SU2_MPI::GetSize();
  int count = static_cast<int>(send.size());
  std::vector<int> counts(size), displs(size, 0);
  SU2_MPI::Allgather(&count, 1, MPI_INT, counts.data(), 1, MPI_INT, SU2_MPI::GetComm());
  for (int r = 1; r < size; ++r) displs[r] = displs[r - 1] + counts[r - 1];
  std::vector<double> recv(displs[size - 1] + counts[size - 1]);
  SU2_MPI::Allgatherv(send.data(), count, MPI_DOUBLE, recv.data(), counts.data(), displs.data(), MPI_DOUBLE,
                      SU2_MPI::GetComm());
  std::map<unsigned long, size_t> where;
  for (size_t p = 0; p < recv.size(); p += width + 1) where[static_cast<unsigned long>(recv[p])] = p + 1;
  for (auto i = nDomain; i < geometry.GetnPoint(); ++i) {
    const auto it = where.find(geometry.nodes->GetGlobalIndex(i));
    if (it == where.end()) continue;
    for (auto c = 0ul; c < width; ++c) data(i, c) = recv[it->second + c];
  }
}

/*--- TECNO_ADJOINT_LIMITER= EDGE_ENTROPY_FCT (ROE_TECNO_PLAN.md 6.9): the passes of TeCNOAdjoint on a geometry (the
 * kernel's ingredient functions, FCTSetupRows/Finish, FCTPasses), an independent reference of the model-8 formulas
 * (explicit eigenvectors, brute-force upwind elements, global pair lists), the frozen-field box of the final values,
 * Theorem A1' with the stored values and FIRST_RING_JUMPS with the stored phi_c. ---*/

/*!
 * \brief Ingredients with the FCT columns of every local edge from w, its gradient and the frozen edge states.
 */
template <size_t nDim, class Geometry>
su2activematrix FCTIngredientsOf(const Geometry& geometry, const su2activematrix& w, const CVectorOfMatrix& grad,
                                 const std::vector<TVDEdgeState<nDim>>& es) {
  const auto tvd = TVDIngredientsOf<nDim>(geometry, w, grad, es);
  const unsigned short width = TeCNOAdjoint::FCTWidth(nDim), X = TeCNOAdjoint::TVDWidth(nDim);
  su2activematrix ing(geometry.GetnEdge(), width);
  for (auto e = 0ul; e < geometry.GetnEdge(); ++e) {
    for (unsigned short c = 0; c < X; ++c) ing(e, c) = tvd(e, c);
    const auto& st = es[e];
    Double out[8 + 2 * 3];
    tecnoAdjointFCTIngredients(st.avg, st.unit, st.lamM, st.lam0, st.lamP, out);
    for (unsigned short c = 0; c < width - X; ++c) ing(e, X + c) = out[c][0];
  }
  return ing;
}

struct FCTData {
  su2activematrix ing, store;
  TeCNOAdjoint::FCTSetup setup;
  TeCNOAdjoint::FCTWork work;
};

/*!
 * \brief Setup (rows copied to the halos by haloCopy) and the passes on given ingredients and entropy variables.
 */
template <class Geometry, class WeightType, class VolumeType, class HaloCopy>
void FCTRun(const Geometry& geometry, unsigned short nDim, const WeightType& weights, const VolumeType& volume,
            const su2activematrix& W, FCTData& d, HaloCopy&& haloCopy, unsigned short nPasses = 1,
            bool nodeInput = false) {
  su2activematrix rows;
  TeCNOAdjoint::FCTSetupRows(geometry, d.setup, rows);
  haloCopy(rows);
  TeCNOAdjoint::FCTSetupFinish(geometry, nDim, rows, d.setup);
  TeCNOAdjoint::FCTWorkResize(geometry, d.setup, d.work, nodeInput);
  d.store.resize(geometry.GetnEdge(), TeCNOAdjoint::FCT_STORE_WIDTH);
  d.store = su2double(-1.0);
  TeCNOAdjoint::FCTPasses(geometry, nDim, weights, volume, d.setup, d.ing, W, d.work, d.store, haloCopy, nPasses,
                          nodeInput);
}

/*--- Independent reference (serial): canonical edge data from the coordinates and the ingredient columns. ---*/
struct FCTRefEdge {
  unsigned long n[2] = {0, 0};
  bool flip = false;
  double e[3] = {0, 0, 0}, dy[4] = {0}, s[2][4] = {{0}}, D[4] = {0}, d[4] = {0}, lam[4] = {0}, lamFix[4] = {0};
  double dyS[3] = {0}, sS[2][3] = {{0}}, t[3] = {0}, c = 0, H = 0, u[3] = {0}, nrm[3] = {0};
  /*--- Results: A of the canonical ends, valid, cap, o, sigma, phi_c, M_c, the shear moment, Pm, R, room, a_e, rho,
   * alpha, gamma. ---*/
  double A[2][4] = {{0}}, cap[4] = {0}, o[4] = {0}, sigma[4] = {0}, phic[4] = {0}, Mc[4] = {0}, Mv[3] = {0};
  double Pm[4] = {0}, R[4] = {0}, room[4] = {0}, aown[4] = {0}, rho[4] = {1, 1, 1, 1}, alpha[4] = {0}, gam[4] = {0};
  double rowCap[2][4] = {{0}};  /*!< \brief NODE_INPUT: cap of the row of canonical end x. */
  bool valid[4] = {false, false, false, false};
};

struct FCTRefResult {
  std::vector<FCTRefEdge> ed;
  std::vector<std::vector<std::pair<unsigned long, int>>> at;  /*!< \brief (edge, canonical end) at each node. */
  std::vector<std::vector<std::array<double, 3>>> g;           /*!< \brief Gradient weight of the other end, same order. */
  /*!< \brief NODE_INPUT: s_a = sum of the gradient weights of node a and sum of their absolute values, summed in the star
   *   order of the geometry with the arithmetic of TeCNOAdjoint::FCTLoadStar (w * (1/V)): at interior nodes of the median
   *   dual s_a is a rounding residue whose guard decision depends on the summation order (ROE_TECNO_PLAN.md 6.13). */
  std::vector<std::array<double, 3>> s, sAbs;
};

/*!
 * \brief Characteristic values of z in the canonical fields of the edge from the explicit eigenvectors
 *        r_M = (1, u - c n, H - u.n c), r_0 = (1, u, |u|^2/2), r_P = (1, u + c n, H + u.n c), r_t = (0, t, u.t).
 */
inline void FCTRefProject(unsigned short nDim, const FCTRefEdge& r, const double* z, double* y) {
  double un = 0, q2 = 0;
  for (unsigned short d = 0; d < nDim; ++d) {
    un += r.u[d] * r.nrm[d];
    q2 += r.u[d] * r.u[d];
  }
  double rM[5], r0[5], rP[5], rt[5];
  rM[0] = r0[0] = rP[0] = 1.0;
  rt[0] = 0.0;
  for (unsigned short d = 0; d < nDim; ++d) {
    rM[1 + d] = r.u[d] - r.c * r.nrm[d];
    r0[1 + d] = r.u[d];
    rP[1 + d] = r.u[d] + r.c * r.nrm[d];
    rt[1 + d] = r.t[d];
  }
  double ut = 0;
  for (unsigned short d = 0; d < nDim; ++d) ut += r.u[d] * r.t[d];
  rM[nDim + 1] = r.H - un * r.c;
  r0[nDim + 1] = 0.5 * q2;
  rP[nDim + 1] = r.H + un * r.c;
  rt[nDim + 1] = ut;
  double yM = 0, y0 = 0, yP = 0, yt = 0;
  for (unsigned short v = 0; v < nDim + 2; ++v) {
    yM += rM[v] * z[v];
    y0 += r0[v] * z[v];
    yP += rP[v] * z[v];
    yt += rt[v] * z[v];
  }
  y[0] = r.flip ? yP : yM;
  y[1] = y0;
  y[2] = yt;
  y[3] = r.flip ? yM : yP;
}

/*!
 * \brief Self piece of the h-th (edge, end) at node a (TECNO_ADJOINT_FCT_BOUND= NODE_INPUT): the pair formula with the
 *        weight replaced by s_a = sum_k g_k, contributor = the edge itself; 0 within the rounding bound of s_a . e_h.
 */
inline double FCTRefSelf(const FCTRefResult& res, unsigned short nDim, unsigned long a, size_t h, int f) {
  const auto& r = res.ed[res.at[a][h].first];
  const auto& s = res.s[a];
  const auto& sa = res.sAbs[a];
  double pi = 0, size = 0;
  for (unsigned short d = 0; d < nDim; ++d) {
    pi += s[d] * r.e[d];
    size += sa[d] * std::fabs(r.e[d]);
  }
  if (!(std::fabs(pi) > double(res.at[a].size() + nDim + 2) * std::numeric_limits<double>::epsilon() * size)) return 0.0;
  pi *= (res.at[a][h].second == 0) ? 1.0 : -1.0;
  double m = r.Mc[f];
  if (f == 2) {
    m = 0;
    double ms = 0;
    for (unsigned short d = 0; d < nDim; ++d) {
      m += r.t[d] * r.Mv[d];
      ms += std::fabs(r.t[d] * r.Mv[d]);
    }
    if (!(std::fabs(m) > (nDim + 2) * std::numeric_limits<double>::epsilon() * ms)) return 0.0;
  }
  return 0.5 * ((r.dy[f] >= 0) ? 1.0 : -1.0) * pi * m / r.D[f];
}

/*!
 * \brief NODE_INPUT selection of the reference (ROE_TECNO_PLAN.md 6.13): rows (edge, canonical end x); the row of x gets the
 *        own term, the pairs collected at the other end n[1 - x] (all contributors), the pairs at n[x] of the invalid
 *        contributors and the edge's own self piece at n[x] (valid). swapRows (test only) exchanges far and near.
 */
template <class PairC>
void FCTReferenceNodeInput(FCTRefResult& res, unsigned short nDim, unsigned short nPasses, bool swapRows,
                           const std::vector<std::vector<unsigned long>>& nbr, PairC&& pairC) {
  auto& ed = res.ed;
  const size_t nE = ed.size();
  using Rows = std::vector<std::array<std::array<double, 4>, 2>>;
  auto zero = [&]() { return Rows(nE, {{{0, 0, 0, 0}, {0, 0, 0, 0}}}); };
  Rows Rr = zero();
  for (unsigned short pass = 0; pass < nPasses; ++pass) {
    const bool first = (pass == 0);
    Rows P = zero(), S = zero(), Sz = zero();
    for (unsigned long a = 0; a < res.at.size(); ++a)
      for (size_t h = 0; h < res.at[a].size(); ++h) {
        const auto eh = res.at[a][h].first;
        const int xh = res.at[a][h].second;
        const int far = swapRows ? xh : 1 - xh, near = swapRows ? 1 - xh : xh;
        for (int f = 0; f < 4; ++f) {
          if (!ed[eh].valid[f]) continue;
          for (size_t k = 0; k < res.at[a].size(); ++k) {
            const auto ek = res.at[a][k].first;
            const double c = pairC(a, h, k, f), al = first ? 0.0 : ed[ek].alpha[f];
            P[eh][far][f] += std::max(0.0, c) * (1 - al);
            S[eh][far][f] += al * c;
            Sz[eh][far][f] += std::fabs(al * c);
            if (!ed[ek].valid[f]) {
              P[eh][near][f] += std::max(0.0, c) * (1 - al);
              S[eh][near][f] += al * c;
              Sz[eh][near][f] += std::fabs(al * c);
            }
          }
          const double cs = FCTRefSelf(res, nDim, a, h, f), al = first ? 0.0 : ed[eh].alpha[f];
          P[eh][near][f] += std::max(0.0, cs) * (1 - al);
          S[eh][near][f] += al * cs;
          Sz[eh][near][f] += std::fabs(al * cs);
        }
      }
    std::vector<std::array<double, 4>> ar(nE, {1, 1, 1, 1});
    for (unsigned long e = 0; e < nE; ++e) {
      const auto& r = ed[e];
      const double nPairs = double(nbr[r.n[0]].size() + nbr[r.n[1]].size());
      for (int f = 0; f < 4; ++f) {
        const double al = first ? 0.0 : r.alpha[f], own = r.phic[f] * r.o[f], den = own * (1 - al);
        for (int x = 0; x < 2; ++x) {
          double Q = r.rowCap[x][f];
          if (!first) {
            const double tau = (nPairs + 3) * std::numeric_limits<double>::epsilon() *
                               (r.rowCap[x][f] + std::fabs(al * own) + Sz[e][x][f]);
            Q = r.valid[f] ? std::max(0.0, r.rowCap[x][f] - al * own - S[e][x][f] - tau) : 0.0;
          }
          Rr[e][x][f] = (r.valid[f] && P[e][x][f] > Q) ? Q / P[e][x][f] : 1.0;
          const double room = std::max(0.0, Q - Rr[e][x][f] * P[e][x][f]);
          if (r.valid[f] && den > 0) ar[e][f] = std::min(ar[e][f], std::min(1.0, room / den));
        }
      }
    }
    std::vector<std::array<double, 4>> rho(nE, {1, 1, 1, 1});
    for (unsigned long a = 0; a < res.at.size(); ++a)
      for (size_t k = 0; k < res.at[a].size(); ++k) {
        const auto ek = res.at[a][k].first;
        const int xk = res.at[a][k].second;
        for (int f = 0; f < 4; ++f) {
          if (!first && !(ed[ek].alpha[f] < 1.0)) continue;
          for (size_t h = 0; h < res.at[a].size(); ++h) {
            const auto eh = res.at[a][h].first;
            const int xh = res.at[a][h].second;
            if (!ed[eh].valid[f] || !(pairC(a, h, k, f) > 0)) continue;
            rho[ek][f] = std::min(rho[ek][f], Rr[eh][swapRows ? xh : 1 - xh][f]);
            if (!ed[ek].valid[f]) rho[ek][f] = std::min(rho[ek][f], Rr[eh][swapRows ? 1 - xh : xh][f]);
          }
          if (ed[ek].valid[f] && FCTRefSelf(res, nDim, a, k, f) > 0)
            rho[ek][f] = std::min(rho[ek][f], Rr[ek][swapRows ? 1 - xk : xk][f]);
        }
      }
    for (unsigned long e = 0; e < nE; ++e)
      for (int f = 0; f < 4; ++f)
        ed[e].alpha[f] = first ? std::min(rho[e][f], ar[e][f])
                               : std::min(1.0, ed[e].alpha[f] + (1 - ed[e].alpha[f]) * std::min(rho[e][f], ar[e][f]));
  }
  for (auto& r : ed)
    for (int f = 0; f < 4; ++f) r.gam[f] = 1 - r.alpha[f];
}

/*!
 * \brief The model-8 limiter on a serial geometry: everything from global lists (adjacency, all elements of the mesh
 *        containing the end, Eigen inverses), in the canonical orientation of TVDRefEdges.
 */
template <class Geometry, class WeightType, class VolumeType>
FCTRefResult FCTReference(const Geometry& geometry, unsigned short nDim, const su2activematrix& ing,
                          const su2activematrix& W, const WeightType& weights, const VolumeType& volume,
                          unsigned short nPasses = 1, bool nodeInput = false, bool swapRows = false) {
  namespace ta = TeCNOAdjoint;
  FCTRefResult res;
  auto& ed = res.ed;
  const auto nP = geometry.GetnPoint(), nE = geometry.GetnEdge();
  const unsigned short nVar = nDim + 2, X = ta::TVDWidth(nDim);
  ed.resize(nE);
  for (unsigned long e = 0; e < nE; ++e) {
    auto& r = ed[e];
    const unsigned long a = geometry.edges->GetNode(e, 0), b = geometry.edges->GetNode(e, 1);
    std::array<double, 3> xa = {0, 0, 0}, xb = {0, 0, 0};
    for (unsigned short d = 0; d < nDim; ++d) {
      xa[d] = geometry.nodes->GetCoord(a)[d];
      xb[d] = geometry.nodes->GetCoord(b)[d];
    }
    r.flip = std::lexicographical_compare(xb.begin(), xb.begin() + nDim, xa.begin(), xa.begin() + nDim);
    r.n[0] = r.flip ? b : a;
    r.n[1] = r.flip ? a : b;
    for (unsigned short d = 0; d < nDim; ++d) r.e[d] = geometry.nodes->GetCoord(r.n[1])[d] - geometry.nodes->GetCoord(r.n[0])[d];
    const double sg = r.flip ? -1.0 : 1.0;
    const int scalarCol[4] = {r.flip ? 2 : 0, 1, -1, r.flip ? 0 : 2};
    const int dCol[4] = {r.flip ? 2 : 0, 1, 3, r.flip ? 0 : 2};
    const int lamCol[4] = {r.flip ? 2 : 0, 1, 1, r.flip ? 0 : 2};
    for (int f = 0; f < 4; ++f) {
      r.D[f] = ing(e, ta::TVD_D + dCol[f]);
      r.d[f] = ing(e, ta::TVD_GUARD + ((f == 2) ? 1 : 0));
      r.lam[f] = sg * ing(e, X + ta::FCT_LAM + lamCol[f]);
      r.lamFix[f] = ing(e, X + ta::FCT_LAMFIX + lamCol[f]);
      if (f == 2) continue;
      r.dy[f] = sg * ing(e, ta::TVD_DY + scalarCol[f]);
      r.s[0][f] = sg * ing(e, (r.flip ? ta::TVD_S1 : ta::TVD_S0) + scalarCol[f]);
      r.s[1][f] = sg * ing(e, (r.flip ? ta::TVD_S0 : ta::TVD_S1) + scalarCol[f]);
    }
    double n2 = 0;
    for (unsigned short d = 0; d < nDim; ++d) {
      r.dyS[d] = sg * ing(e, ta::TVD_SHEAR + d);
      r.sS[0][d] = sg * ing(e, ta::TVD_SHEAR + (r.flip ? 2 : 1) * nDim + d);
      r.sS[1][d] = sg * ing(e, ta::TVD_SHEAR + (r.flip ? 1 : 2) * nDim + d);
      n2 += r.dyS[d] * r.dyS[d];
    }
    const double nn = std::sqrt(n2);
    r.dy[2] = nn;
    for (unsigned short d = 0; d < nDim; ++d) r.t[d] = (nn > 0) ? r.dyS[d] / nn : 0.0;
    for (int x = 0; x < 2; ++x) {
      r.s[x][2] = 0;
      for (unsigned short d = 0; d < nDim; ++d) r.s[x][2] += r.sS[x][d] * r.t[d];
    }
    r.c = ing(e, X + ta::FCT_SOUND);
    r.H = ing(e, X + ta::FCT_ENTHALPY);
    for (unsigned short d = 0; d < nDim; ++d) {
      r.u[d] = ing(e, X + ta::FCT_VEL + d);
      r.nrm[d] = ing(e, X + ta::FCT_VEL + nDim + d);
    }
  }
  /*--- Adjacency (from the edge list), degrees, gradient weights of the pairs. ---*/
  res.at.assign(nP, {});
  res.g.assign(nP, {});
  std::vector<std::vector<unsigned long>> nbr(nP);
  for (unsigned long e = 0; e < nE; ++e) {
    for (int x = 0; x < 2; ++x) {
      const auto a = ed[e].n[x];
      res.at[a].push_back({e, x});
      nbr[a].push_back(ed[e].n[1 - x]);
      const unsigned short lend = (geometry.edges->GetNode(e, 0) == a) ? 0 : 1;
      std::array<double, 3> gg = {0, 0, 0};
      const double invVol = 1.0 / volume(a);  /*--- the arithmetic of TeCNOAdjoint::FCTLoadStar ---*/
      for (unsigned short d = 0; d < nDim; ++d) gg[d] = weights(e, lend * nDim + d) * invVol;
      res.g[a].push_back(gg);
    }
  }
  res.s.assign(nP, {0, 0, 0});
  res.sAbs.assign(nP, {0, 0, 0});
  for (unsigned long a = 0; a < nP; ++a) {
    const double invVol = 1.0 / volume(a);
    for (auto iEdge : geometry.nodes->GetEdges(a)) {
      const unsigned short lend = (geometry.edges->GetNode(iEdge, 0) == a) ? 0 : 1;
      for (unsigned short d = 0; d < nDim; ++d) {
        const double gd = weights(iEdge, lend * nDim + d) * invVol;
        res.s[a][d] += gd;
        res.sAbs[a][d] += std::fabs(gd);
      }
    }
  }
  /*--- (1) A of every canonical end. ---*/
  for (unsigned long e = 0; e < nE; ++e) {
    auto& r = ed[e];
    for (int x = 0; x < 2; ++x) {
      const auto a = r.n[x];
      const double sx = (x == 0) ? 1.0 : -1.0;
      double sgn[4], beta[4] = {0, 0, 0, 0}, z[5], y[4];
      for (int f = 0; f < 4; ++f) sgn[f] = (r.dy[f] >= 0) ? 1.0 : -1.0;
      for (auto v : nbr[a]) {
        for (unsigned short k = 0; k < nVar; ++k) z[k] = sx * (W(a, k) - W(v, k));
        FCTRefProject(nDim, r, z, y);
        for (int f = 0; f < 4; ++f) beta[f] = std::max(beta[f], sgn[f] * y[f]);
      }
      /*--- Upwind element: all elements of the mesh with the node a, direction x_a - x_other. ---*/
      double dir[3] = {0, 0, 0}, best = -1e300, bestMu[3] = {0, 0, 0};
      unsigned long bestV[3] = {0, 0, 0};
      for (unsigned short d = 0; d < nDim; ++d) dir[d] = geometry.nodes->GetCoord(a)[d] - geometry.nodes->GetCoord(r.n[1 - x])[d];
      for (unsigned long iElem = 0; iElem < geometry.GetnElem(); ++iElem) {
        const auto* el = geometry.elem[iElem];
        bool has = false;
        for (unsigned short k = 0; k < el->GetnNodes(); ++k) has = has || (el->GetNode(k) == a);
        if (!has) continue;
        Eigen::MatrixXd M(nDim, nDim);
        unsigned long vv[3] = {0, 0, 0};
        int c = 0;
        for (unsigned short k = 0; k < el->GetnNodes(); ++k) {
          const auto p = el->GetNode(k);
          if (p == a) continue;
          vv[c] = p;
          for (unsigned short d = 0; d < nDim; ++d) M(d, c) = geometry.nodes->GetCoord(p)[d] - geometry.nodes->GetCoord(a)[d];
          ++c;
        }
        Eigen::VectorXd rhs(nDim);
        for (unsigned short d = 0; d < nDim; ++d) rhs(d) = dir[d];
        const Eigen::VectorXd mu = M.inverse() * rhs;
        const double score = mu.minCoeff() / std::max(mu.cwiseAbs().sum(), 1e-300);
        if (score > best) {
          best = score;
          for (int k = 0; k < c; ++k) {
            bestMu[k] = mu(k);
            bestV[k] = vv[k];
          }
        }
      }
      double ub[4];
      if (best >= -1e-10) {
        for (unsigned short k = 0; k < nVar; ++k) {
          z[k] = 0;
          for (unsigned short m = 0; m < nDim; ++m) z[k] += bestMu[m] * sx * (W(a, k) - W(bestV[m], k));
        }
        FCTRefProject(nDim, r, z, y);
        for (int f = 0; f < 4; ++f) ub[f] = sgn[f] * y[f];
      } else {
        for (int f = 0; f < 4; ++f) ub[f] = sgn[f] * (2 * r.s[x][f] - r.dy[f]);
      }
      for (int f = 0; f < 4; ++f) {
        const double ratio = (r.lamFix[f] > 0) ? r.lam[f] / r.lamFix[f] : 0.0;
        const double ownJ = 0.5 * (1 - sx * ratio) * std::fabs(r.dy[f]);
        r.A[x][f] = nodeInput ? std::max(beta[f], ub[f]) + ownJ : std::max(std::max(beta[f], ub[f]), ownJ);
      }
    }
    /*--- (2) Own values. ---*/
    const double nmax = double(std::max(nbr[r.n[0]].size(), nbr[r.n[1]].size()));
    for (int f = 0; f < 4; ++f) {
      const double delta = (nmax + 2) * r.d[f];
      r.valid[f] = (std::fabs(r.dy[f]) > delta) && (r.D[f] > 0);
      const double sgn = (r.dy[f] >= 0) ? 1.0 : -1.0;
      const double B = 2 * sgn * std::min(r.A[0][f], r.A[1][f]);
      r.cap[f] = std::fabs(B) + delta;
      r.sigma[f] = 0.5 * (r.s[0][f] + r.s[1][f]);
      r.o[f] = sgn * r.sigma[f];
      r.phic[f] = (r.valid[f] && r.o[f] > 0) ? std::min(1.0, r.cap[f] / r.o[f]) : 1.0;
      if (nodeInput) {  /*--- S4f (ROE_TECNO_PLAN.md 6.13, D3): |l_c| <= |dy| on every valid entry. ---*/
        if (r.valid[f]) r.phic[f] = (r.o[f] > 0) ? std::min(r.phic[f], 2 * std::fabs(r.dy[f]) / r.o[f]) : 0.0;
        for (int x = 0; x < 2; ++x) r.rowCap[x][f] = 2 * r.A[x][f] + delta;
      }
      r.Mc[f] = r.phic[f] * r.D[f] * (r.dy[f] - r.phic[f] * r.sigma[f]);
    }
    for (unsigned short d = 0; d < nDim; ++d)
      r.Mv[d] = r.phic[2] * r.D[2] * (r.dyS[d] - r.phic[2] * 0.5 * (r.sS[0][d] + r.sS[1][d]));
  }
  /*--- (3) Pairs at every node: c of the contributor k seen by the observer h. ---*/
  auto pairC = [&](unsigned long a, size_t h, size_t k, int f) {
    const auto& rh = ed[res.at[a][h].first];
    const auto& rk = ed[res.at[a][k].first];
    double pi = 0, size = 0;
    for (unsigned short d = 0; d < nDim; ++d) {
      pi += res.g[a][h][d] * rk.e[d];
      size += std::fabs(res.g[a][h][d] * rk.e[d]);
    }
    if (!(std::fabs(pi) > (nDim + 2) * std::numeric_limits<double>::epsilon() * size)) return 0.0;  /*--- rev 4 ---*/
    pi *= (res.at[a][h].second == 0) ? 1.0 : -1.0;
    double m = rk.Mc[f];
    if (f == 2) {
      m = 0;
      double ms = 0;
      for (unsigned short d = 0; d < nDim; ++d) {
        m += rh.t[d] * rk.Mv[d];
        ms += std::fabs(rh.t[d] * rk.Mv[d]);
      }
      if (!(std::fabs(m) > (nDim + 2) * std::numeric_limits<double>::epsilon() * ms)) return 0.0;  /*--- rev 4 ---*/
    }
    return 0.5 * ((rh.dy[f] >= 0) ? 1.0 : -1.0) * pi * m / rh.D[f];
  };
  if (nodeInput) {
    FCTReferenceNodeInput(res, nDim, nPasses, swapRows, nbr, pairC);
    return res;
  }
  for (unsigned long a = 0; a < nP; ++a)
    for (size_t h = 0; h < res.at[a].size(); ++h)
      for (int f = 0; f < 4; ++f)
        if (ed[res.at[a][h].first].valid[f])
          for (size_t k = 0; k < res.at[a].size(); ++k) ed[res.at[a][h].first].Pm[f] += std::max(0.0, pairC(a, h, k, f));
  for (auto& r : ed) {
    for (int f = 0; f < 4; ++f) {
      r.R[f] = (r.valid[f] && r.Pm[f] > r.cap[f]) ? r.cap[f] / r.Pm[f] : 1.0;
      r.room[f] = std::max(0.0, r.cap[f] - r.R[f] * r.Pm[f]);
      const double den = r.phic[f] * r.o[f];
      r.aown[f] = (r.valid[f] && den > 0) ? std::min(1.0, r.room[f] / den) : 1.0;
    }
  }
  /*--- (4) Observer minima and the final values. ---*/
  for (unsigned long a = 0; a < nP; ++a)
    for (size_t k = 0; k < res.at[a].size(); ++k)
      for (int f = 0; f < 4; ++f)
        for (size_t h = 0; h < res.at[a].size(); ++h)
          if (ed[res.at[a][h].first].valid[f] && (pairC(a, h, k, f) > 0))
            ed[res.at[a][k].first].rho[f] = std::min(ed[res.at[a][k].first].rho[f], ed[res.at[a][h].first].R[f]);
  for (auto& r : ed)
    for (int f = 0; f < 4; ++f) r.alpha[f] = std::min(r.rho[f], r.aown[f]);
  /*--- (5) S4e (ROE_TECNO_PLAN.md 6.11): further passes on the remaining amounts with Q = cap - S(alpha). ---*/
  for (unsigned short pass = 1; pass < nPasses; ++pass) {
    std::vector<std::array<double, 4>> S(nE, {0, 0, 0, 0}), Ss(nE, {0, 0, 0, 0}), Pr(nE, {0, 0, 0, 0}), rho(nE, {1, 1, 1, 1});
    for (unsigned long a = 0; a < nP; ++a)
      for (size_t h = 0; h < res.at[a].size(); ++h)
        for (int f = 0; f < 4; ++f) {
          const auto eh = res.at[a][h].first;
          if (!ed[eh].valid[f]) continue;
          for (size_t k = 0; k < res.at[a].size(); ++k) {
            const double c = pairC(a, h, k, f), al = ed[res.at[a][k].first].alpha[f];
            S[eh][f] += al * c;
            Ss[eh][f] += std::fabs(al * c);
            Pr[eh][f] += std::max(0.0, c) * (1.0 - al);
          }
        }
    std::vector<std::array<double, 4>> Rr(nE, {1, 1, 1, 1}), ar(nE, {1, 1, 1, 1});
    for (unsigned long e = 0; e < nE; ++e) {
      const auto& r = ed[e];
      for (int f = 0; f < 4; ++f) {
        const double own = r.phic[f] * r.o[f];
        const double raw = r.cap[f] - r.alpha[f] * own - S[e][f];
        const double nPairs = double(nbr[r.n[0]].size() + nbr[r.n[1]].size());
        const double tau = (nPairs + 3) * std::numeric_limits<double>::epsilon() * (r.cap[f] + std::fabs(r.alpha[f] * own) + Ss[e][f]);
        const double Q = r.valid[f] ? std::max(0.0, raw - tau) : 0.0;  /*--- less the rounding bound of the sum. ---*/
        Rr[e][f] = (r.valid[f] && Pr[e][f] > Q) ? Q / Pr[e][f] : 1.0;
        const double room = std::max(0.0, Q - Rr[e][f] * Pr[e][f]);
        const double den = own * (1.0 - r.alpha[f]);
        ar[e][f] = (r.valid[f] && den > 0) ? std::min(1.0, room / den) : 1.0;
      }
    }
    for (unsigned long a = 0; a < nP; ++a)
      for (size_t k = 0; k < res.at[a].size(); ++k)
        for (int f = 0; f < 4; ++f) {
          const auto ek = res.at[a][k].first;
          if (!(ed[ek].alpha[f] < 1.0)) continue;
          for (size_t h = 0; h < res.at[a].size(); ++h)
            if (ed[res.at[a][h].first].valid[f] && (pairC(a, h, k, f) > 0))
              rho[ek][f] = std::min(rho[ek][f], Rr[res.at[a][h].first][f]);
        }
    for (unsigned long e = 0; e < nE; ++e)
      for (int f = 0; f < 4; ++f)
        ed[e].alpha[f] = std::min(1.0, ed[e].alpha[f] + (1.0 - ed[e].alpha[f]) * std::min(rho[e][f], ar[e][f]));
  }
  for (auto& r : ed)
    for (int f = 0; f < 4; ++f) r.gam[f] = 1 - r.alpha[f];  /*--- S4d (ROE_TECNO_PLAN.md 6.10): no zero term, valid and invalid entries. ---*/
  return res;
}

/*!
 * \brief Canonical value (phi_c 0, alpha 1, gamma 2) of the stored local layout.
 */
inline double FCTStored(const su2activematrix& store, unsigned long e, bool flip, int which, int f) {
  const int fl = flip ? ((f == 0) ? 3 : ((f == 3) ? 0 : f)) : f;
  return store(e, 4 * which + fl);
}

/*!
 * \brief Largest relative excess of the frozen-field box by the stored values at the ramp r over the valid entries:
 *        S_e = r [sg (alpha phi_c sigma + (1 - alpha - gamma) dy) + sum_pairs alpha_e' c] <= cap_e + E_drop, all pairs
 *        unfiltered, E_drop the budget of the pairs that the selection drops (rev 4), relative to the size of the terms.
 */
inline double FCTBoxViolation(const FCTRefResult& ref, unsigned short nDim, const su2activematrix& store, double ramp) {
  const auto& ed = ref.ed;
  std::vector<std::array<double, 4>> S(ed.size(), {0, 0, 0, 0}), size(ed.size(), {0, 0, 0, 0}), drop(ed.size(), {0, 0, 0, 0});
  for (unsigned long a = 0; a < ref.at.size(); ++a) {
    for (size_t h = 0; h < ref.at[a].size(); ++h) {
      const auto eh = ref.at[a][h].first;
      const auto& rh = ed[eh];
      for (int f = 0; f < 4; ++f) {
        if (!rh.valid[f]) continue;
        for (size_t k = 0; k < ref.at[a].size(); ++k) {
          const auto ek = ref.at[a][k].first;
          const auto& rk = ed[ek];
          double pi = 0;
          for (unsigned short d = 0; d < nDim; ++d) pi += ref.g[a][h][d] * rk.e[d];
          pi *= (ref.at[a][h].second == 0) ? 1.0 : -1.0;
          /*--- M_c of the contributor with its stored phi_c. ---*/
          const double pk = FCTStored(store, ek, rk.flip, 0, f);
          double m = pk * rk.D[f] * (rk.dy[f] - pk * rk.sigma[f]);
          if (f == 2) {
            m = 0;
            for (unsigned short d = 0; d < nDim; ++d)
              m += rh.t[d] * pk * rk.D[2] * (rk.dyS[d] - pk * 0.5 * (rk.sS[0][d] + rk.sS[1][d]));
          }
          const double c = 0.5 * ((rh.dy[f] >= 0) ? 1.0 : -1.0) * pi * m / rh.D[f];
          const double term = ramp * FCTStored(store, ek, rk.flip, 1, f) * c;
          S[eh][f] += term;
          size[eh][f] += std::fabs(term);
          /*--- Budget of a pair that the selection drops (rev 4): |term| <= r alpha' tau_pi |m| / (2 D), or with the
           * shear projection dropped r alpha' |pi| tau_t / (2 D), tau the rounding bounds of the two dot products. ---*/
          const double eps = std::numeric_limits<double>::epsilon();
          double gs = 0;
          for (unsigned short d = 0; d < nDim; ++d) gs += std::fabs(ref.g[a][h][d] * rk.e[d]);
          const double al = ramp * FCTStored(store, ek, rk.flip, 1, f);
          if (!(std::fabs(pi) > (nDim + 2) * eps * gs)) drop[eh][f] += al * 0.5 * (nDim + 2) * eps * gs * std::fabs(m) / rh.D[f];
          if (f == 2) {
            double ms = 0;
            for (unsigned short d = 0; d < nDim; ++d)
              ms += std::fabs(rh.t[d] * pk * rk.D[2] * (rk.dyS[d] - pk * 0.5 * (rk.sS[0][d] + rk.sS[1][d])));
            if (!(std::fabs(m) > (nDim + 2) * eps * ms)) drop[eh][f] += al * 0.5 * std::fabs(pi) * (nDim + 2) * eps * ms / rh.D[f];
          }
        }
      }
    }
  }
  double worst = 0;
  for (unsigned long e = 0; e < ed.size(); ++e) {
    const auto& r = ed[e];
    for (int f = 0; f < 4; ++f) {
      if (!r.valid[f]) continue;
      const double pc = FCTStored(store, e, r.flip, 0, f), al = ramp * FCTStored(store, e, r.flip, 1, f);
      const double ga = 1.0 - ramp * (1.0 - FCTStored(store, e, r.flip, 2, f));
      const double sgn = (r.dy[f] >= 0) ? 1.0 : -1.0;
      const double own = sgn * (al * pc * r.sigma[f] + (1 - al - ga) * r.dy[f]);
      const double tot = S[e][f] + own;
      const double scale = r.cap[f] + std::fabs(r.dy[f]) + std::fabs(r.sigma[f]) + size[e][f];
      worst = std::max(worst, std::max(0.0, tot - r.cap[f] - drop[e][f]) / scale);
    }
  }
  return worst;
}

/*!
 * \brief NODE_INPUT checks on the reference data with stored values at the ramp r: (box) the largest relative excess of
 *        the row amounts (guarded terms as the selection) over the row caps; (identity) for every node and scalar field,
 *        with independent contributor weights w (unfiltered terms), the largest relative difference between the pushes
 *        sum_h D_h sg_h sig_h sum_k w_k c_hk (the near pairs at the node) and sum_h D_h sg_h sig_h (sum_{k invalid} w_k c_hk
 *        + [h valid] w_h c_self,h).
 */
template <class Gen>
void FCTNodeInputChecks(const FCTRefResult& ref, unsigned short nDim, const su2activematrix& store, double ramp,
                        Gen& gen, double& box, double& identity) {
  const auto& ed = ref.ed;
  const double eps = std::numeric_limits<double>::epsilon();
  auto mOf = [&](unsigned long eh, unsigned long ek, int f) {
    const auto& rh = ed[eh];
    const auto& rk = ed[ek];
    const double pk = FCTStored(store, ek, rk.flip, 0, f);
    if (f != 2) return pk * rk.D[f] * (rk.dy[f] - pk * rk.sigma[f]);
    double m = 0;
    for (unsigned short d = 0; d < nDim; ++d) m += rh.t[d] * pk * rk.D[2] * (rk.dyS[d] - pk * 0.5 * (rk.sS[0][d] + rk.sS[1][d]));
    return m;
  };
  auto pairRaw = [&](unsigned long a, size_t h, size_t k, int f, bool guard) {
    const auto eh = ref.at[a][h].first, ek = ref.at[a][k].first;
    double pi = 0, size = 0;
    for (unsigned short d = 0; d < nDim; ++d) {
      pi += ref.g[a][h][d] * ed[ek].e[d];
      size += std::fabs(ref.g[a][h][d] * ed[ek].e[d]);
    }
    if (guard && !(std::fabs(pi) > (nDim + 2) * eps * size)) return 0.0;
    pi *= (ref.at[a][h].second == 0) ? 1.0 : -1.0;
    return 0.5 * ((ed[eh].dy[f] >= 0) ? 1.0 : -1.0) * pi * mOf(eh, ek, f) / ed[eh].D[f];
  };
  auto selfRaw = [&](unsigned long a, size_t h, int f) {
    const auto eh = ref.at[a][h].first;
    double s[3] = {0, 0, 0}, pi = 0;
    for (const auto& g : ref.g[a])
      for (unsigned short d = 0; d < nDim; ++d) s[d] += g[d];
    for (unsigned short d = 0; d < nDim; ++d) pi += s[d] * ed[eh].e[d];
    pi *= (ref.at[a][h].second == 0) ? 1.0 : -1.0;
    return 0.5 * ((ed[eh].dy[f] >= 0) ? 1.0 : -1.0) * pi * mOf(eh, eh, f) / ed[eh].D[f];
  };
  std::uniform_real_distribution<double> U(0.0, 1.0);
  std::vector<std::array<double, 4>> wgt(ed.size());
  for (auto& w : wgt) for (auto& x : w) x = U(gen);
  std::vector<std::array<std::array<double, 4>, 2>> S(ed.size(), {{{0, 0, 0, 0}, {0, 0, 0, 0}}}), Z = S;
  for (unsigned long a = 0; a < ref.at.size(); ++a) {
    for (int f = 0; f < 4; ++f) {
      double lhs = 0, rhs = 0, scale = 0;
      for (size_t h = 0; h < ref.at[a].size(); ++h) {
        const auto eh = ref.at[a][h].first;
        const int xh = ref.at[a][h].second;
        const double conv = ed[eh].D[f] * ((ed[eh].dy[f] >= 0) ? 1.0 : -1.0) * ((xh == 0) ? 1.0 : -1.0);
        for (size_t k = 0; k < ref.at[a].size(); ++k) {
          const auto ek = ref.at[a][k].first;
          const double c = pairRaw(a, h, k, f, false), al = ramp * FCTStored(store, ek, ed[ek].flip, 1, f);
          if (f != 2) {
            lhs += conv * wgt[ek][f] * c;
            scale += std::fabs(conv * wgt[ek][f] * c);
            if (!ed[ek].valid[f]) rhs += conv * wgt[ek][f] * c;
          }
          if (!ed[eh].valid[f]) continue;
          const double cg = pairRaw(a, h, k, f, true);
          S[eh][1 - xh][f] += al * cg;
          Z[eh][1 - xh][f] += std::fabs(al * cg);
          if (!ed[ek].valid[f]) {
            S[eh][xh][f] += al * cg;
            Z[eh][xh][f] += std::fabs(al * cg);
          }
        }
        if (f != 2 && ed[eh].valid[f]) rhs += conv * wgt[eh][f] * selfRaw(a, h, f);
        if (ed[eh].valid[f]) {
          const double cs = FCTRefSelf(ref, nDim, a, h, f), al = ramp * FCTStored(store, eh, ed[eh].flip, 1, f);
          S[eh][xh][f] += al * cs;
          Z[eh][xh][f] += std::fabs(al * cs);
        }
      }
      if (f != 2 && scale > 0) identity = std::max(identity, std::fabs(lhs - rhs) / scale);
    }
  }
  for (unsigned long e = 0; e < ed.size(); ++e) {
    const auto& r = ed[e];
    for (int f = 0; f < 4; ++f) {
      if (!r.valid[f]) continue;
      const double pc = FCTStored(store, e, r.flip, 0, f), al = ramp * FCTStored(store, e, r.flip, 1, f);
      const double own = al * pc * ((r.dy[f] >= 0) ? 1.0 : -1.0) * r.sigma[f];
      for (int x = 0; x < 2; ++x) {
        const double tot = own + S[e][x][f];
        const double scale = r.rowCap[x][f] + std::fabs(r.dy[f]) + std::fabs(r.sigma[f]) + Z[e][x][f];
        box = std::max(box, std::max(0.0, tot - r.rowCap[x][f]) / scale);
      }
    }
  }
}

/*!
 * \brief Adjoint dissipation residual with the stored values (local layout) and frozen edge states, any w:
 *        tecnoAdjointEdgeFCT per edge, the Delta part scattered as the edge loop, NodalMomentEnds and the transpose with
 *        GradientCoefficient(1, 0). Returns R and sum_e sum_f D (alpha_r l_c^2 + gamma_r dy^2) from the ingredients' D.
 */
template <size_t nDim, class Geometry>
su2activematrix FCTResidual(const Geometry& geometry, const std::vector<TVDEdgeState<nDim>>& es,
                            const su2activematrix& store, const su2activematrix& w, double ramp, const P1Average* p1,
                            const su2activematrix* ing, double& sumProd) {
  constexpr size_t nVar = nDim + 2;
  const auto nPoint = w.rows();
  const auto grad = TVDGradientOf<nDim>(geometry, w, p1);
  su2activematrix Q(geometry.GetnEdge(), 2 * nVar), R(nPoint, nVar);
  R = su2double(0.0);
  sumProd = 0.0;
  for (auto e = 0ul; e < geometry.GetnEdge(); ++e) {
    const auto i = geometry.edges->GetNode(e, 0), k = geometry.edges->GetNode(e, 1);
    VectorDbl<nVar> dw, p_i, p_k, q, q_m;
    for (size_t v = 0; v < nVar; ++v) {
      dw(v) = w(k, v) - w(i, v);
      p_i(v) = 0.0;
      p_k(v) = 0.0;
      for (size_t d = 0; d < nDim; ++d) {
        const double ed = geometry.nodes->GetCoord(k)[d] - geometry.nodes->GetCoord(i)[d];
        p_i(v) += grad(i, v, d) * ed;
        p_k(v) += grad(k, v, d) * ed;
      }
    }
    Double fct[12], yL[nDim + 3], yD[nDim + 3];
    for (int c = 0; c < 12; ++c) fct[c] = store(e, c);
    const auto& st = es[e];
    tecnoAdjointEdgeFCT(Double(GAMMA), st.avg, st.unit, st.lamM, st.lam0, st.lamP, st.scale, dw, p_i, p_k, ramp, fct, q,
                        q_m, yL, yD);
    if (ing) {
      namespace ta = TeCNOAdjoint;
      const auto& g = *ing;
      /*--- Local fields M (0), 0 (1), P (2), shear (3 + d) of yL/yD; store fields M 0, 0 1, S 2, P 3. ---*/
      const int sf[3] = {0, 1, 3};
      for (int j = 0; j < 3; ++j) {
        const double ar = ramp * store(e, ta::FCT_ALPHA + sf[j]), gr = 1.0 - ramp * (1.0 - store(e, ta::FCT_GAMMA + sf[j]));
        sumProd += g(e, ta::TVD_D + j) * (ar * yL[j][0] * yL[j][0] + gr * yD[j][0] * yD[j][0]);
      }
      const double ar = ramp * store(e, ta::FCT_ALPHA + 2), gr = 1.0 - ramp * (1.0 - store(e, ta::FCT_GAMMA + 2));
      for (size_t d = 0; d < nDim; ++d)
        sumProd += g(e, ta::TVD_D + 3) * (ar * yL[3 + d][0] * yL[3 + d][0] + gr * yD[3 + d][0] * yD[3 + d][0]);
    }
    for (size_t v = 0; v < nVar; ++v) {
      R(i, v) -= q(v)[0];
      R(k, v) += q(v)[0];
      Q(e, v) = q_m(v)[0];
      Q(e, nVar + v) = q_m(v)[0];
    }
  }
  CVectorOfMatrix Y(nPoint, nVar, nDim, 0.0);
  const su2double gam = TeCNOAdjoint::GradientCoefficient(1.0, 0.0);
  if (p1) {
    TeCNOAdjoint::NodalMomentEnds(geometry, nDim, nVar, Q, p1->volume, Y);
    TeCNOAdjoint::AddTransposeP1(geometry, nDim, nVar, p1->weights, Y, gam, R);
  } else {
    auto volume = [&](unsigned long iPoint) { return geometry.nodes->GetVolume(iPoint); };
    TeCNOAdjoint::NodalMomentEnds(geometry, nDim, nVar, Q, volume, Y);
    TeCNOAdjoint::AddTranspose(geometry, nDim, nVar, Y, gam, R);
  }
  return R;
}

/*!
 * \brief FIRST_RING_JUMPS with the stored values: J from JumpNodal / JumpNodalQEndsFCT / JumpRowEndsFCT against the
 *        frozen linear residual matrix K of FCTResidual (column by column, ramp r), lambda_min(J - K) >= 0, J 1 = 0,
 *        J = J^T.
 */
template <size_t nDim, class Geometry>
TVDJumpsResult FCTJumpsCheck(const Geometry& geometry, const std::vector<TVDEdgeState<nDim>>& es,
                             const su2activematrix& store, double ramp, const P1Average* p1) {
  constexpr size_t nVar = nDim + 2;
  const auto nPoint = geometry.GetnPoint();
  const auto nDof = nVar * nPoint;
  TVDJumpsResult res;
  Eigen::MatrixXd K(nDof, nDof);
  for (auto col = 0ul; col < nDof; ++col) {
    su2activematrix w(nPoint, nVar);
    w = su2double(0.0);
    w(col / nVar, col % nVar) = 1.0;
    double dummy = 0.0;
    const auto R = FCTResidual<nDim>(geometry, es, store, w, ramp, p1, nullptr, dummy);
    for (auto i = 0ul; i < nPoint; ++i)
      for (size_t v = 0; v < nVar; ++v) K(nVar * i + v, col) = R(i, v);
  }
  const Eigen::MatrixXd Ks = 0.5 * (K + K.transpose());
  Eigen::SelfAdjointEigenSolver<Eigen::MatrixXd> eigK(Ks);
  const double lamMax = std::max(eigK.eigenvalues().maxCoeff(), 1e-300);
  const auto D = TVDFrozenD<nDim>(es);
  auto check = [&](const auto& weights, const auto& volume) {
    auto edgeW = [&](unsigned long e) {
      su2double tr = 0.0;
      for (size_t a = 0; a < nVar; ++a) tr += D(e, a * nVar + a);
      return tr;
    };
    su2activematrix Sj(nPoint, 3 + 2 * nDim * nDim), Qj(nPoint, nVar * nVar);
    TeCNOAdjoint::JumpNodal(geometry, nDim, weights, volume, edgeW, Sj);
    TeCNOAdjoint::JumpNodalQEndsFCT(geometry, nDim, nVar, weights, Sj, store, D, Qj);
    Eigen::MatrixXd J = Eigen::MatrixXd::Zero(nDof, nDof);
    for (auto i = 0ul; i < nPoint; ++i) {
      TeCNOAdjoint::Block diag;
      TeCNOAdjoint::JumpRowEndsFCT(geometry, nDim, nVar, weights, Sj, Qj, D, store, i, diag,
                                   [&](unsigned long k, const TeCNOAdjoint::Block& off) {
        for (size_t a = 0; a < nVar; ++a)
          for (size_t b = 0; b < nVar; ++b) J(nVar * i + a, nVar * k + b) += off(a, b);
      });
      for (size_t a = 0; a < nVar; ++a)
        for (size_t b = 0; b < nVar; ++b) J(nVar * i + a, nVar * i + b) += diag(a, b);
    }
    res.finite = res.finite && J.allFinite() && K.allFinite();
    res.asym = std::max(res.asym, (J - J.transpose()).norm() / J.norm());
    for (auto i = 0ul; i < nPoint; ++i) {
      for (size_t a = 0; a < nVar; ++a) {
        for (size_t b = 0; b < nVar; ++b) {
          double sum = 0.0;
          for (auto k = 0ul; k < nPoint; ++k) sum += J(nVar * i + a, nVar * k + b);
          res.rowSum = std::max(res.rowSum, std::fabs(sum) / J.norm());
        }
      }
    }
    const Eigen::MatrixXd Js = 0.5 * (J + J.transpose());
    Eigen::SelfAdjointEigenSolver<Eigen::MatrixXd> eig(Js - Ks);
    res.finite = res.finite && (eig.info() == Eigen::Success);
    res.jumps = std::max(res.jumps, -eig.eigenvalues().minCoeff() / lamMax);
  };
  if (p1) {
    check(p1->weights, p1->volume);
  } else {
    auto volume = [&](unsigned long i) { return geometry.nodes->GetVolume(i); };
    check(TeCNOAdjoint::GreenGaussWeights<Geometry>{geometry, nDim}, volume);
  }
  return res;
}

}  // namespace TeCNOAdjointChecks
