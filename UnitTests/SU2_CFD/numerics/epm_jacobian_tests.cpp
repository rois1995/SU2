/*!
 * \file epm_jacobian_tests.cpp
 * \brief Unit tests for the tangent Jacobian of the classical EPM stress, UQ_JACOBIAN= TANGENT.
 * \author SU2 Contributors
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

#include "catch.hpp"
#include "../../../Common/include/CConfig.hpp"
#include "epm_test_setup.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <memory>
#include <sstream>
#include <string>

/*--- Case modes as in the other EPM tests: 0 1C_KEEP, 1 1C_SWAP, 2 2C_KEEP, 3 2C_SWAP, 4 3C. ---*/

namespace {
using epm_test::PointState;

/*--- Deterministic generator of values in [-1, 1) (portable, unlike the standard distributions). ---*/
class Lcg {
  uint64_t state;

 public:
  explicit Lcg(uint64_t seed) : state(seed) {}
  double operator()() {
    state = state * 6364136223846793005ULL + 1442695040888963407ULL;
    return static_cast<double>(state >> 11) * (1.0 / 9007199254740992.0) * 2.0 - 1.0;
  }
};

/*--- Constants of the test states. ---*/
constexpr passivedouble kRho = 1.2, kMuLam = 0.01, kMuT = 0.3, kTke = 0.8, kDist = 0.7, kArea = 1.3;

/*--- Random unit normal, velocity and velocity gradient (du_i/dx_j) of a seed; always 3 + 3 + 9 draws. ---*/
struct RandomState {
  passivedouble n[3] = {}, u[3] = {}, g[3][3] = {};
  RandomState(unsigned short nDim, uint64_t seed) {
    Lcg r(seed);
    passivedouble n3[3], norm = 0;
    for (auto& x : n3) x = r();
    for (int i = 0; i < nDim; ++i) norm += n3[i] * n3[i];
    for (int i = 0; i < nDim; ++i) n[i] = n3[i] / std::sqrt(norm);
    for (auto& x : u) x = r();
    for (auto& row : g)
      for (auto& x : row) x = r();
    for (int i = nDim; i < 3; ++i) u[i] = 0;
    for (int i = 0; i < 3; ++i)
      for (int j = 0; j < 3; ++j)
        if (i >= nDim || j >= nDim) g[i][j] = 0;
  }
};

/*--- "+" runs get the pair term (KEEP with A > 0, SWAP with A < 0); the "-" runs and 3C get none. ---*/
bool PlusRun(int mode, double urlx) { return mode != 4 && ((urlx > 0) != (mode % 2 == 1)); }

bool Same(passivedouble a, passivedouble b) { return std::memcmp(&a, &b, sizeof(passivedouble)) == 0; }

std::string Number(double value) {
  std::stringstream text;
  text << std::setprecision(17) << value;
  return text.str();
}

/*--- Classical UQ (SST_OPTIONS UQ) of the case mode at UQ_URLX= urlx, with UQ_JACOBIAN= jacobian. ---*/
std::string UQText(int mode, double urlx, const std::string& jacobian, const std::string& extra = "") {
  return "SOLVER= RANS\nREYNOLDS_NUMBER= 1e6\nKIND_TURB_MODEL= SST\nSST_OPTIONS= (V2003m, UQ)\nUQ_COMPONENT= " +
         std::to_string(mode / 2 + 1) + "\nUQ_PERMUTE= " + (mode % 2 ? "YES" : "NO") +
         "\nUQ_DELTA_B= 1\nUQ_URLX= " + Number(urlx) + "\nUQ_JACOBIAN= " + jacobian + "\n" + extra;
}

/*--- Eigen data and tangent term of the passive classical perturbation of a velocity gradient. ---*/
template <class Mat>
CNumerics::EPMSpectrum Spectrum(unsigned short nDim, int mode, double urlx, const Mat& velgrad, passivedouble rho,
                                passivedouble mut, passivedouble k, passivedouble (*rsm)[3] = nullptr) {
  passivedouble grad[3][3] = {}, stress[3][3];
  for (int i = 0; i < nDim; ++i)
    for (int j = 0; j < nDim; ++j) grad[i][j] = SU2_TYPE::GetValue(velgrad[i][j]);
  CNumerics::EPMSpectrum spectrum;
  CNumerics::ComputePerturbedRSM(nDim, mode / 2 + 1, mode % 2 == 1, 1.0, urlx, grad, rho, mut, k, stress, 0.0, 0.0,
                                 nullptr, 3, &spectrum);
  if (rsm)
    for (int i = 0; i < 3; ++i)
      for (int j = 0; j < 3; ++j) rsm[i][j] = stress[i][j];
  return spectrum;
}

/*--- Edge state of the scalar flux: V_i = V_j (so the thin-layer J_j is the exact derivative at the state, and
 *    dU_j/du_c = (0, rho e_c, rho u_c) uses the mean velocity), Coord_j - Coord_i = d n, same node gradients. ---*/
struct EdgeState {
  su2double vi[12] = {}, vj[12] = {}, grad[4][3] = {}, xi[3] = {}, xj[3] = {}, normal[3] = {};
  RandomState random;
  EdgeState(unsigned short nDim, uint64_t seed) : random(nDim, seed) {
    const passivedouble v[] = {1, 0, 0, 0, 1, kRho, 10, 10, kMuLam, kMuT, .2, 1};
    /*--- T, velocity (nDim), p, rho, h, c, mu_lam, mu_t, kappa, cp. ---*/
    vi[0] = v[0];
    for (int i = 0; i < nDim; ++i) vi[1 + i] = random.u[i];
    for (int i = 0; i < 8; ++i) vi[nDim + 1 + i] = v[4 + i];
    for (int i = 0; i < 12; ++i) vj[i] = vi[i];
    for (int i = 0; i < nDim; ++i) {
      grad[0][i] = .1 * (i + 1);
      for (int j = 0; j < nDim; ++j) grad[1 + i][j] = random.g[i][j];
      xj[i] = kDist * random.n[i];
      normal[i] = kArea * random.n[i];
    }
  }
  EdgeState(const EdgeState&) = delete;
  EdgeState& operator=(const EdgeState&) = delete;
};

struct EdgeOutput {
  passivedouble flux[5] = {}, jac_i[5][5] = {}, jac_j[5][5] = {};
};

/*--- Scalar CAvgGrad_Flow (corrected gradient, implicit) at the edge state; the numerics keep pointers into it. ---*/
class EdgeRun {
  std::unique_ptr<CConfig> config;
  std::unique_ptr<CAvgGrad_Flow> numerics;
  unsigned short nVar;

 public:
  EdgeRun(unsigned short nDim, const std::string& text, EdgeState& s) : nVar(nDim + 2) {
    std::stringstream options(text);
    config.reset(new CConfig(options, SU2_COMPONENT::SU2_CFD, false));
    config->SetKind_TimeIntScheme(EULER_IMPLICIT);
    numerics.reset(new CAvgGrad_Flow(nDim, nDim + 2, true, config.get()));
    numerics->SetPrimitive(s.vi, s.vj);
    numerics->SetPrimVarGradient({&s.grad[0][0], 3}, {&s.grad[0][0], 3});
    numerics->SetTurbKineticEnergy(kTke, kTke);
    numerics->SetCoord(s.xi, s.xj);
    numerics->SetNormal(s.normal);
  }
  EdgeOutput operator()() {
    const auto residual = numerics->ComputeResidual(config.get());
    EdgeOutput out;
    for (int i = 0; i < nVar; ++i) {
      out.flux[i] = SU2_TYPE::GetValue(residual[i]);
      for (int j = 0; j < nVar; ++j) {
        out.jac_i[i][j] = SU2_TYPE::GetValue(residual.jacobian_i[i][j]);
        out.jac_j[i][j] = SU2_TYPE::GetValue(residual.jacobian_j[i][j]);
      }
    }
    return out;
  }
};

/*--- Eigen data at the face of the edge state: the corrected mean gradient of CAvgGrad_Flow (V_i = V_j). ---*/
CNumerics::EPMSpectrum EdgeSpectrum(unsigned short nDim, const EdgeState& s, int mode, double urlx) {
  passivedouble edge[3] = {}, dist2 = 0, g[3][3] = {};
  for (int i = 0; i < nDim; ++i) {
    edge[i] = SU2_TYPE::GetValue(s.xj[i] - s.xi[i]);
    dist2 += edge[i] * edge[i];
  }
  for (int v = 0; v < nDim; ++v) {
    passivedouble proj = 0;
    for (int j = 0; j < nDim; ++j) proj += SU2_TYPE::GetValue(s.grad[v + 1][j]) * edge[j];
    for (int j = 0; j < nDim; ++j) g[v][j] = SU2_TYPE::GetValue(s.grad[v + 1][j]) - proj * edge[j] / dist2;
  }
  return Spectrum(nDim, mode, urlx, g, kRho, kMuT, kTke);
}

/*--- Change of Jacobian_i of the scalar flux for a pair term dM (times the distance), see AddTauJacobianUQ. ---*/
void JacobianChange(unsigned short nDim, const passivedouble (&dM)[3][3], const passivedouble* u,
                    passivedouble (&change)[5][5]) {
  for (auto& row : change)
    for (auto& x : row) x = 0;
  const passivedouble factor = kArea / (kRho * kDist);
  passivedouble dMu[3] = {}, udMu = 0;
  for (int i = 0; i < nDim; ++i) {
    for (int j = 0; j < nDim; ++j) dMu[i] += dM[i][j] * u[j];
    udMu += u[i] * dMu[i];
  }
  for (int i = 0; i < nDim; ++i) {
    change[1 + i][0] = factor * dMu[i];
    for (int j = 0; j < nDim; ++j) change[1 + i][1 + j] = -factor * dM[i][j];
    change[nDim + 1][1 + i] = -factor * dMu[i];
  }
  change[nDim + 1][0] = factor * udMu;
}

passivedouble MaxAbs(const passivedouble (&m)[5][5]) {
  passivedouble x = 0;
  for (const auto& row : m)
    for (auto y : row) x = std::max(x, std::fabs(y));
  return x;
}
}  // namespace

TEST_CASE("EPM spectrum output does not change the perturbed stress", "[EPMJacobian]") {
  for (unsigned short nDim : {2, 3})
    for (uint64_t seed = 1; seed <= 6; ++seed) {
      const RandomState s(nDim, seed);
      for (int mode = 0; mode < 5; ++mode)
        for (double urlx : {.1, -.1, 1.0}) {
          passivedouble plain[3][3], with[3][3];
          CNumerics::ComputePerturbedRSM(nDim, mode / 2 + 1, mode % 2 == 1, 1.0, urlx, s.g, kRho, kMuT, kTke, plain);
          const auto spectrum = Spectrum(nDim, mode, urlx, s.g, kRho, kMuT, kTke, with);
          CHECK(spectrum.valid);
          CHECK(spectrum.permute == (mode % 2 == 1));
          for (int i = 0; i < 3; ++i)
            for (int j = 0; j < 3; ++j) CHECK(Same(plain[i][j], with[i][j]));
        }
    }
  /*--- Smooth wall (k = mu_t = 0): no eigensystem, no term. ---*/
  const RandomState s(3, 1);
  const auto wall = Spectrum(3, 0, .1, s.g, kRho, 0.0, 0.0);
  CHECK_FALSE(wall.valid);
  passivedouble dM[3][3];
  CHECK_FALSE(CNumerics::EPMTangentStressJacobian(3, wall, .1, 0.0, kMuLam, s.n, dM));
  for (auto& row : dM)
    for (auto x : row) CHECK(x == 0.0);
}

TEST_CASE("Tangent pair term: analytic values", "[EPMJacobian]") {
  /*--- 2D shear du/dy = G (s = G/2), n = e_y: the in-plane pair stiffens the normal velocity only. ---*/
  const passivedouble rho = 1, k = 1e-3, mut = 1e-4, urlx = .1, G = 1e-2, ey[3] = {0, 1, 0};
  for (int mode : {0, 2}) {
    const passivedouble grad[2][2] = {{0, G}, {0, 0}};
    const auto spectrum = Spectrum(2, mode, urlx, grad, rho, mut, k);
    passivedouble dM[3][3], exact[3][3];
    REQUIRE(CNumerics::EPMTangentStressJacobian(2, spectrum, urlx, mut, 0.0, ey, dM));
    /*--- mu_rot = rho A k / (2 s) for 1C, rho A k / (4 s) for 2C. ---*/
    const passivedouble muRot = rho * urlx * k / ((mode == 0 ? 2 : 4) * G / 2);
    CHECK(dM[1][1] == Approx(muRot - urlx * mut).epsilon(1e-10));
    CHECK(std::fabs(dM[0][0]) < 1e-12 * muRot);
    CHECK(std::fabs(dM[0][1]) < 1e-12 * muRot);
    /*--- Exact mode: (1-A) mu_t in place of mu_t, plus the signed pair term. ---*/
    CNumerics::EPMTangentStressJacobian(2, spectrum, urlx, mut, 0.0, ey, exact, true);
    CHECK(exact[0][0] == Approx(-urlx * mut).epsilon(1e-10));
    CHECK(exact[1][1] == Approx(muRot - urlx * mut * 4 / 3).epsilon(1e-10));
  }
  /*--- Wake centreline, S_xx = e dominates: the pair stiffens the tangential velocity, rho A k / (2 |S_xx|). ---*/
  for (passivedouble shear : {0.0, 1e-4}) {
    const passivedouble e = 1e-2, grad[2][2] = {{e, shear}, {0, -e}};
    const auto spectrum = Spectrum(2, 0, urlx, grad, rho, mut, k);
    passivedouble dM[3][3];
    REQUIRE(CNumerics::EPMTangentStressJacobian(2, spectrum, urlx, mut, 0.0, ey, dM));
    CHECK(dM[0][0] == Approx(rho * urlx * k / (2 * e) - urlx * mut).epsilon(shear == 0 ? 1e-10 : 1e-2));
    CHECK(std::fabs(dM[1][1]) < 1e-3 * dM[0][0]);
  }
  /*--- 3C: tied targets, no term. ---*/
  {
    const passivedouble grad[2][2] = {{0, G}, {0, 0}};
    const auto spectrum = Spectrum(2, 4, urlx, grad, rho, mut, k);
    passivedouble dM[3][3];
    CHECK_FALSE(CNumerics::EPMTangentStressJacobian(2, spectrum, urlx, mut, 0.0, ey, dM));
    for (auto& row : dM)
      for (auto x : row) CHECK(x == 0.0);
  }
}

TEST_CASE("Tangent pair term is one-sided, symmetric and finite", "[EPMJacobian]") {
  for (unsigned short nDim : {2, 3})
    for (int mode = 0; mode < 5; ++mode)
      for (double urlx : {.1, 1.0, -.1, -1.0}) {
        int nonzero = 0;
        for (uint64_t seed = 1; seed <= 20; ++seed) {
          const RandomState s(nDim, seed);
          const auto spectrum = Spectrum(nDim, mode, urlx, s.g, kRho, kMuT, kTke);
          passivedouble dM[3][3];
          const bool any = CNumerics::EPMTangentStressJacobian(nDim, spectrum, urlx, kMuT, kMuLam, s.n, dM);
          nonzero += any;
          if (!PlusRun(mode, urlx)) {
            /*--- "-" runs and 3C: exactly nothing. ---*/
            CHECK_FALSE(any);
            for (auto& row : dM)
              for (auto x : row) CHECK(x == 0.0);
            continue;
          }
          passivedouble scale = 0;
          for (int i = 0; i < 3; ++i)
            for (int j = 0; j < 3; ++j) {
              CHECK(std::isfinite(dM[i][j]));
              CHECK(Same(dM[i][j], dM[j][i]));
              if (i >= nDim || j >= nDim) CHECK(dM[i][j] == 0.0);
              scale = std::max(scale, std::fabs(dM[i][j]));
            }
          /*--- Positive semidefinite. ---*/
          Lcg r(100 + seed);
          for (int trial = 0; trial < 10; ++trial) {
            passivedouble x[3] = {r(), r(), r()}, q = 0;
            for (int i = 0; i < nDim; ++i)
              for (int j = 0; j < nDim; ++j) q += x[i] * dM[i][j] * x[j];
            CHECK(q >= -1e-14 * scale);
          }
        }
        /*--- The term is active in the "+" runs. ---*/
        if (PlusRun(mode, urlx)) CHECK(nonzero > 0);
      }
}

TEST_CASE("Tangent pair term at degenerate states", "[EPMJacobian]") {
  const passivedouble cap = CNumerics::EPMTangentBiMax * (kMuLam + kMuT);
  /*--- Tied baseline eigenvalues (2D pure rotation: all tied; 3D: the two lowest tied): finite, at most the
   *    overflow guard per pair, nothing for the "-" runs. ---*/
  for (unsigned short nDim : {2, 3}) {
    const PointState p(100, nDim, true);
    passivedouble grad[3][3] = {}, n[3] = {};
    for (int i = 0; i < nDim; ++i) {
      n[i] = 1 / std::sqrt(passivedouble(nDim));
      for (int j = 0; j < nDim; ++j) grad[i][j] = SU2_TYPE::GetValue(p.grad[i + 1][j]);
    }
    for (int mode = 0; mode < 5; ++mode)
      for (double urlx : {.1, -.1}) {
        const auto spectrum = Spectrum(nDim, mode, urlx, grad, kRho, kMuT, kTke);
        passivedouble dM[3][3];
        const bool any = CNumerics::EPMTangentStressJacobian(nDim, spectrum, urlx, kMuT, kMuLam, n, dM);
        if (!PlusRun(mode, urlx)) CHECK_FALSE(any);
        for (auto& row : dM)
          for (auto x : row) {
            CHECK(std::isfinite(x));
            CHECK(std::fabs(x) <= 3 * cap);
          }
      }
  }
  /*--- 2D, du_y/dy = b only: the out-of-plane eigenvalue ties with the x one, as the two largest, whose targets differ
   *    in 1C_KEEP (zero gap: the overflow guard if the pair were kept). The e_z pairs are skipped, so at most the
   *    in-plane pair (y, x) acts, with mu_rot = rho A k / b. ---*/
  const passivedouble b = 1, grad[2][2] = {{0, 0}, {0, b}}, n[3] = {.6, .8, 0}, urlx = .1;
  const auto spectrum = Spectrum(2, 0, urlx, grad, kRho, kMuT, kTke);
  CHECK(spectrum.baseline[1] == Approx(spectrum.baseline[2]).margin(1e-14));
  passivedouble dM[3][3];
  CNumerics::EPMTangentStressJacobian(2, spectrum, urlx, kMuT, kMuLam, n, dM);
  const passivedouble muRot = kRho * urlx * kTke / b;
  for (auto& row : dM)
    for (auto x : row) {
      CHECK(std::isfinite(x));
      CHECK(std::fabs(x) <= muRot * (1 + 1e-9));
    }
}

TEST_CASE("Scalar viscous flux: tangent Jacobian against finite differences", "[EPMJacobian]") {
  for (unsigned short nDim : {2, 3}) {
    const int nVar = nDim + 2;
    for (int mode = 0; mode < 5; ++mode)
      for (double urlx : {.1, 1.0, -.1}) {
        for (uint64_t seed = 1; seed <= 4; ++seed) {
          EdgeState s(nDim, seed);
          EdgeRun none(nDim, UQText(mode, urlx, "NONE"), s);
          EdgeRun tangent(nDim, UQText(mode, urlx, "TANGENT"), s);
          const auto base = none();
          const auto withTerm = tangent();
          const auto spectrum = EdgeSpectrum(nDim, s, mode, urlx);
          const passivedouble* u = s.random.u;

          /*--- The residual is never changed. ---*/
          for (int i = 0; i < nVar; ++i) CHECK(Same(base.flux[i], withTerm.flux[i]));

          /*--- Tangent minus NONE is the one-sided pair term: J_i gains the change, J_j loses it. "-" runs and 3C
           *    are bitwise identical. ---*/
          passivedouble dM[3][3], change[5][5];
          const bool any = CNumerics::EPMTangentStressJacobian(nDim, spectrum, urlx, kMuT, kMuLam, s.random.n, dM);
          if (!PlusRun(mode, urlx)) CHECK_FALSE(any);
          JacobianChange(nDim, dM, u, change);
          const passivedouble margin = 1e-12 * std::max({1.0, MaxAbs(base.jac_i), MaxAbs(change)});
          for (int i = 0; i < nVar; ++i)
            for (int j = 0; j < nVar; ++j) {
              if (!any) {
                CHECK(Same(withTerm.jac_i[i][j], base.jac_i[i][j]));
                CHECK(Same(withTerm.jac_j[i][j], base.jac_j[i][j]));
              } else {
                CHECK(withTerm.jac_i[i][j] - base.jac_i[i][j] == Approx(change[i][j]).margin(margin));
                CHECK(withTerm.jac_j[i][j] - base.jac_j[i][j] == Approx(-change[i][j]).margin(margin));
              }
            }

          /*--- Central differences in the velocity of point j (T, p, rho fixed) against J_j(NONE) dU/du_c plus the
           *    exact pair term, area [0; dMex/d; (dMex u)/d] (the exact Jacobian with k, mu_t, rho and the node
           *    gradients frozen; it equals the thin-layer one because Coord_j - Coord_i = d n). ---*/
          passivedouble exact[3][3];
          CNumerics::EPMTangentStressJacobian(nDim, spectrum, urlx, kMuT, kMuLam, s.random.n, exact, true);
          const passivedouble h = 1e-6;
          for (int c = 0; c < nDim; ++c) {
            passivedouble fd[5];
            s.vj[1 + c] = u[c] + h;
            const auto plus = none();
            s.vj[1 + c] = u[c] - h;
            const auto minus = none();
            s.vj[1 + c] = u[c];
            passivedouble scale = 0;
            for (int i = 0; i < nVar; ++i) {
              fd[i] = (plus.flux[i] - minus.flux[i]) / (2 * h);
              scale = std::max(scale, std::fabs(fd[i]));
            }
            for (int i = 0; i < nVar; ++i) {
              passivedouble linear = kRho * base.jac_j[i][1 + c] + kRho * u[c] * base.jac_j[i][nDim + 1];
              if (i >= 1 && i <= nDim) linear += kArea * exact[i - 1][c] / kDist;
              if (i == nDim + 1)
                for (int m = 0; m < nDim; ++m) linear += kArea * exact[m][c] * u[m] / kDist;
              CHECK(fd[i] == Approx(linear).margin(1e-6 * scale));
            }
          }
        }
      }
  }
}

namespace {
/*--- Edge state of the vectorized free functions, lane k with the random state of seed k + 1; with more than one
 *    lane, the last one is a smooth-wall lane (k = mu_t = 0). ---*/
template <size_t nDim>
struct SimdEdge {
  CCompressiblePrimitives<nDim, nDim + 9> v;
  MatrixDbl<nDim + 1, nDim> grad;
  VectorDbl<nDim> normal;
  Double turbKe, dist;
  passivedouble n[Double::Size][3] = {}, u[Double::Size][3] = {};

  static bool Wall(size_t lane) { return Double::Size > 1 && lane == Double::Size - 1; }

  SimdEdge() {
    for (size_t i = 0; i < v.nVar; ++i) v.all(i) = 0;
    for (size_t k = 0; k < Double::Size; ++k) {
      const RandomState s(nDim, k + 1);
      v.density()[k] = kRho;
      v.laminarVisc()[k] = kMuLam;
      v.eddyVisc()[k] = Wall(k) ? 0.0 : kMuT;
      turbKe[k] = Wall(k) ? 0.0 : kTke;
      dist[k] = kDist;
      for (size_t i = 0; i < nDim; ++i) {
        n[k][i] = s.n[i];
        u[k][i] = s.u[i];
        v.velocity(i)[k] = s.u[i];
        normal(i)[k] = s.n[i];
        grad(0, i)[k] = .1;
        for (size_t j = 0; j < nDim; ++j) grad(i + 1, j)[k] = s.g[i][j];
      }
    }
  }
};

/*--- Stress of the vectorized kernel: laminar plus classical EPM, with or without the tangent term. ---*/
template <size_t nDim>
MatrixDbl<nDim> SimdStress(const SimdEdge<nDim>& s, const MatrixDbl<nDim + 1, nDim>& grad, int mode, double urlx,
                           EPMTangentLanes* tangent) {
  auto tau = stressTensor(s.v.laminarVisc(), grad);
  if (tangent)
    addPerturbedRSMWithJacobian(s.v, grad, s.turbKe, tau, s.normal, mode / 2 + 1, mode % 2 == 1, su2double(1),
                                su2double(urlx), *tangent);
  else
    addPerturbedRSM(s.v, grad, s.turbKe, tau, size_t(mode / 2 + 1), mode % 2 == 1, su2double(1), su2double(urlx));
  return tau;
}

template <size_t nDim>
void CheckSimdTangent() {
  constexpr size_t nVar = nDim + 2;
  const SimdEdge<nDim> s;
  for (int mode = 0; mode < 5; ++mode)
    for (double urlx : {.1, 1.0, -.1}) {
      EPMTangentLanes tangent;
      const auto plain = SimdStress(s, s.grad, mode, urlx, nullptr);
      const auto tau = SimdStress(s, s.grad, mode, urlx, &tangent);
      const auto dtau = stressTensorJacobian<nVar>(s.v, s.normal, s.dist);
      auto dtauTangent = dtau;
      addTangentStressJacobian(s.v, tangent, s.dist, dtauTangent);

      for (size_t k = 0; k < Double::Size; ++k) {
        /*--- Same stress as addPerturbedRSM, bit for bit. ---*/
        for (size_t i = 0; i < nDim; ++i)
          for (size_t j = 0; j < nDim; ++j)
            CHECK(Same(SU2_TYPE::GetValue(tau(i, j)[k]), SU2_TYPE::GetValue(plain(i, j)[k])));

        /*--- Lane term: the helper on the eigen data of the lane. ---*/
        passivedouble velgrad[3][3] = {}, dM[3][3], exact[3][3];
        for (size_t i = 0; i < nDim; ++i)
          for (size_t j = 0; j < nDim; ++j) velgrad[i][j] = SU2_TYPE::GetValue(s.grad(i + 1, j)[k]);
        const passivedouble mut = SU2_TYPE::GetValue(s.v.eddyVisc()[k]), tke = SU2_TYPE::GetValue(s.turbKe[k]);
        const auto spectrum = Spectrum(nDim, mode, urlx, velgrad, kRho, mut, tke);
        const bool any = CNumerics::EPMTangentStressJacobian(nDim, spectrum, urlx, mut, kMuLam, s.n[k], dM);
        CNumerics::EPMTangentStressJacobian(nDim, spectrum, urlx, mut, kMuLam, s.n[k], exact, true);
        CHECK(tangent.nonzero[k] == any);
        if (!PlusRun(mode, urlx) || SimdEdge<nDim>::Wall(k)) CHECK_FALSE(tangent.nonzero[k]);

        /*--- Lanes without a term are untouched; otherwise momentum -dM/(rho d), density column +dM u/(rho d). ---*/
        const passivedouble factor = 1 / (kRho * kDist);
        passivedouble scale = 1;
        for (size_t i = 0; i < 3; ++i)
          for (size_t j = 0; j < 3; ++j) scale = std::max(scale, std::fabs(dM[i][j]) * factor);
        for (size_t i = 0; i < nDim; ++i) {
          passivedouble dMu = 0;
          for (size_t j = 0; j < nDim; ++j) dMu += dM[i][j] * s.u[k][j];
          for (size_t j = 0; j < nVar; ++j) {
            const passivedouble before = SU2_TYPE::GetValue(dtau(i, j)[k]);
            const passivedouble after = SU2_TYPE::GetValue(dtauTangent(i, j)[k]);
            if (!tangent.nonzero[k]) {
              CHECK(Same(after, before));
              continue;
            }
            passivedouble expected = 0;
            if (j == 0) expected = dMu * factor;
            if (j >= 1 && j <= nDim) expected = -dM[i][j - 1] * factor;
            CHECK(after - before == Approx(expected).margin(1e-12 * scale));
          }
        }

        /*--- Central differences of tau.n in the velocity jump along n (grad(c+1,:) += h n/d) against
         *    -rho dtau(NONE) + dMex/d (exact Jacobian with k, mu_t, rho frozen). ---*/
        const passivedouble h = 1e-6;
        for (size_t c = 0; c < nDim; ++c) {
          auto gradPlus = s.grad, gradMinus = s.grad;
          for (size_t j = 0; j < nDim; ++j) {
            gradPlus(c + 1, j)[k] += h * s.n[k][j] / kDist;
            gradMinus(c + 1, j)[k] -= h * s.n[k][j] / kDist;
          }
          EPMTangentLanes unused;
          const auto tauPlus = SimdStress(s, gradPlus, mode, urlx, &unused);
          const auto tauMinus = SimdStress(s, gradMinus, mode, urlx, &unused);
          passivedouble fd[3] = {}, fdScale = 0;
          for (size_t i = 0; i < nDim; ++i) {
            for (size_t j = 0; j < nDim; ++j)
              fd[i] += (SU2_TYPE::GetValue(tauPlus(i, j)[k]) - SU2_TYPE::GetValue(tauMinus(i, j)[k])) * s.n[k][j] /
                       (2 * h);
            fdScale = std::max(fdScale, std::fabs(fd[i]));
          }
          for (size_t i = 0; i < nDim; ++i) {
            const passivedouble linear = -kRho * SU2_TYPE::GetValue(dtau(i, c + 1)[k]) + exact[i][c] / kDist;
            CHECK(fd[i] == Approx(linear).margin(1e-6 * fdScale));
          }
        }
      }
    }
}
}  // namespace

TEST_CASE("Vectorized viscous flux: tangent Jacobian against finite differences", "[EPMJacobian]") {
  CheckSimdTangent<2>();
  CheckSimdTangent<3>();
}

/*--- Last, so that an abort in CConfig cannot hide the numerical tests above. ---*/
TEST_CASE("UQ_JACOBIAN option and guards", "[EPMJacobian]") {
  {
    std::stringstream options(UQText(0, .1, "NONE"));
    CConfig config(options, SU2_COMPONENT::SU2_CFD, false);
    CHECK(config.GetUQ_Jacobian() == UQ_JACOBIAN::NONE);
    CHECK(config.GetUQJacobianError().empty());
  }
  {
    std::stringstream options("SOLVER= RANS\nREYNOLDS_NUMBER= 1e6\nKIND_TURB_MODEL= SST\nSST_OPTIONS= (V2003m, UQ)\n");
    CConfig config(options, SU2_COMPONENT::SU2_CFD, false);
    CHECK(config.GetUQ_Jacobian() == UQ_JACOBIAN::NONE);
  }
  /*--- Accepted for all five cases and both signs of UQ_URLX, with the usual preconditioners. ---*/
  for (int mode = 0; mode < 5; ++mode)
    for (double urlx : {.1, -.1, 1.0}) {
      std::stringstream options(UQText(mode, urlx, "TANGENT"));
      CConfig config(options, SU2_COMPONENT::SU2_CFD, false);
      CHECK(config.GetUQ_Jacobian() == UQ_JACOBIAN::TANGENT);
      CHECK(config.GetUQJacobianError().empty());
    }
  for (const char* prec : {"ILU", "LU_SGS", "JACOBI"}) {
    std::stringstream options(UQText(0, .1, "TANGENT", std::string("LINEAR_SOLVER_PREC= ") + prec + "\n"));
    CConfig config(options, SU2_COMPONENT::SU2_CFD, false);
    CHECK(config.GetUQJacobianError().empty());
  }

  /*--- SU2_MPI::Error aborts, so the guard is checked on configurations that are valid with UQ_JACOBIAN= NONE, with
   *    TANGENT set afterwards. The string is the message CConfig raises at postprocessing. ---*/
  const auto error = [](const std::string& text) {
    std::stringstream options(text);
    CConfig config(options, SU2_COMPONENT::SU2_CFD, false);
    config.SetUQ_Jacobian(UQ_JACOBIAN::TANGENT);
    return config.GetUQJacobianError();
  };
  CHECK(error(UQText(0, .1, "NONE")).empty());
  /*--- No classical EPM. ---*/
  CHECK(error("SOLVER= RANS\nREYNOLDS_NUMBER= 1e6\nKIND_TURB_MODEL= SST\nSST_OPTIONS= V2003m\n").find("classical EPM") !=
        std::string::npos);
  CHECK(error("SOLVER= RANS\nREYNOLDS_NUMBER= 1e6\nKIND_TURB_MODEL= SA\n").find("classical EPM") != std::string::npos);
  /*--- The rejection of other solvers (e.g. INC_RANS) is not tested here: an incompressible SST UQ configuration
   *    that passes CConfig postprocessing could not be verified without running it. ---*/
  /*--- Target eigenvalues that depend on the baseline ones. ---*/
  {
    std::stringstream options(
        "SOLVER= RANS\nREYNOLDS_NUMBER= 1e6\nKIND_TURB_MODEL= SST\nSST_OPTIONS= (V2003m, UQ)\nUQ_DELTA_B= 0.5\n");
    CConfig config(options, SU2_COMPONENT::SU2_CFD, false);
    config.SetUQ_Jacobian(UQ_JACOBIAN::TANGENT);
    CHECK(config.GetUQJacobianError().find("UQ_DELTA_B") != std::string::npos);
  }
  /*--- Quantized (int8) off-diagonal storage. ---*/
  for (const char* prec : {"Q_LU_SGS", "Q_JACOBI", "Q_IDENTITY"})
    CHECK(error(UQText(0, .1, "NONE", std::string("LINEAR_SOLVER_PREC= ") + prec + "\n")).find("quantized") !=
          std::string::npos);
}
