/*!
 * \file epm_fade_tests.cpp
 * \brief Unit tests for the diagnostic fade of the classical EPM perturbation, UQ_FADE_GAP.
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
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <limits>
#include <memory>
#include <sstream>
#include <string>

/*--- Case modes as in the other EPM tests: 0 1C_KEEP, 1 1C_SWAP, 2 2C_KEEP, 3 2C_SWAP, 4 3C. ---*/

namespace {

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

constexpr passivedouble kRho = 1.2, kMuLam = 0.01, kMuT = 0.3, kTke = 0.8;

/*--- Random unit normal and velocity gradient (du_i/dx_j) of a seed, zero outside nDim. ---*/
struct RandomState {
  passivedouble n[3] = {}, g[3][3] = {};
  RandomState(unsigned short nDim, uint64_t seed) {
    Lcg r(seed);
    passivedouble norm = 0;
    for (auto& x : n) x = r();
    for (int i = nDim; i < 3; ++i) n[i] = 0;
    for (int i = 0; i < nDim; ++i) norm += n[i] * n[i];
    for (int i = 0; i < nDim; ++i) n[i] /= std::sqrt(norm);
    for (auto& row : g)
      for (auto& x : row) x = r();
    for (int i = 0; i < 3; ++i)
      for (int j = 0; j < 3; ++j)
        if (i >= nDim || j >= nDim) g[i][j] = 0;
  }
};

bool Same(passivedouble a, passivedouble b) { return std::memcmp(&a, &b, sizeof(passivedouble)) == 0; }

std::string Number(double value) {
  std::stringstream text;
  text << std::setprecision(17) << value;
  return text.str();
}

/*--- Classical UQ (SST_OPTIONS UQ) of the case mode at UQ_URLX= urlx. ---*/
std::string UQText(int mode, double urlx, const std::string& extra = "") {
  return "SOLVER= RANS\nREYNOLDS_NUMBER= 1e6\nKIND_TURB_MODEL= SST\nSST_OPTIONS= (V2003m, UQ)\nUQ_COMPONENT= " +
         std::to_string(mode / 2 + 1) + "\nUQ_PERMUTE= " + (mode % 2 ? "YES" : "NO") +
         "\nUQ_DELTA_B= 1\nUQ_URLX= " + Number(urlx) + "\n" + extra;
}

/*--- Passive perturbed stress with UQ_FADE_GAP= fade, and its eigen data. ---*/
template <class Mat>
CNumerics::EPMSpectrum Perturbed(unsigned short nDim, int mode, passivedouble urlx, const Mat& velgrad,
                                 passivedouble rho, passivedouble mut, passivedouble k, passivedouble fade,
                                 passivedouble (&rsm)[3][3]) {
  passivedouble grad[3][3] = {};
  for (int i = 0; i < nDim; ++i)
    for (int j = 0; j < nDim; ++j) grad[i][j] = SU2_TYPE::GetValue(velgrad[i][j]);
  CNumerics::EPMSpectrum spectrum;
  CNumerics::ComputePerturbedRSM(nDim, mode / 2 + 1, mode % 2 == 1, 1.0, urlx, grad, rho, mut, k, rsm, 0.0, 0.0,
                                 nullptr, 3, &spectrum, fade);
  return spectrum;
}

/*--- Gap that decides the target direction, from the (sorted) baseline anisotropy eigenvalues: the singled-out
 *    target is the largest baseline eigenvalue for 1C_KEEP and 2C_SWAP, the smallest for 1C_SWAP and 2C_KEEP. ---*/
passivedouble ExpectedFade(int mode, const CNumerics::EPMSpectrum& spectrum, passivedouble c) {
  if (mode == 4) return 1.0;
  passivedouble b[3] = {spectrum.baseline[0], spectrum.baseline[1], spectrum.baseline[2]};
  std::sort(b, b + 3);
  const bool top = mode == 0 || mode == 3;
  const passivedouble g = top ? b[2] - b[1] : b[1] - b[0];
  return g * g / (g * g + c * c);
}

passivedouble MaxAbs(const passivedouble (&m)[3][3]) {
  passivedouble x = 0;
  for (const auto& row : m)
    for (auto y : row) x = std::max(x, std::fabs(y));
  return x;
}

/*--- Velocity gradients with tied baseline eigenvalues: zero strain, pure rotation (2D: all three tied), the 2D
 *    out-of-plane crossing s = theta/2 (two tied), and the 3D axisymmetric strain (two tied). ---*/
void Degenerate(unsigned short nDim, int which, passivedouble (&g)[3][3]) {
  for (auto& row : g)
    for (auto& x : row) x = 0;
  if (which == 1) { g[0][1] = 2; g[1][0] = -2; }
  if (which == 2) g[0][0] = 1;
  if (which == 3 && nDim == 3) { g[0][0] = .5; g[1][1] = .5; g[2][2] = -1; g[0][1] = .3; g[1][0] = -.3; }
}
}  // namespace

TEST_CASE("UQ_FADE_GAP= 0 leaves the perturbed stress unchanged", "[EPMFade]") {
  for (unsigned short nDim : {2, 3})
    for (int state = 0; state < 12; ++state) {
      passivedouble g[3][3];
      if (state < 4) {
        Degenerate(nDim, state, g);
      } else {
        const RandomState s(nDim, state);
        for (int i = 0; i < 3; ++i)
          for (int j = 0; j < 3; ++j) g[i][j] = s.g[i][j];
      }
      for (int mode = 0; mode < 5; ++mode)
        for (double urlx : {.1, -.1, 1.0}) {
          /*--- Passive: default argument, explicit 0, and 0 with the spectrum output. ---*/
          passivedouble plain[3][3], zero[3][3], with[3][3];
          CNumerics::ComputePerturbedRSM(nDim, mode / 2 + 1, mode % 2 == 1, 1.0, urlx, g, kRho, kMuT, kTke, plain);
          CNumerics::ComputePerturbedRSM(nDim, mode / 2 + 1, mode % 2 == 1, 1.0, urlx, g, kRho, kMuT, kTke, zero,
                                         0.0, 0.0, nullptr, 3, nullptr, 0.0);
          const auto spectrum = Perturbed(nDim, mode, urlx, g, kRho, kMuT, kTke, 0.0, with);
          CHECK(spectrum.fade == 1.0);
          /*--- The solver type (su2double, active in AD builds). ---*/
          su2double ga[3][3], plainAD[3][3], zeroAD[3][3];
          for (int i = 0; i < 3; ++i)
            for (int j = 0; j < 3; ++j) ga[i][j] = g[i][j];
          CNumerics::ComputePerturbedRSM(nDim, mode / 2 + 1, mode % 2 == 1, su2double(1.0), su2double(urlx), ga,
                                         su2double(kRho), su2double(kMuT), su2double(kTke), plainAD);
          CNumerics::ComputePerturbedRSM(nDim, mode / 2 + 1, mode % 2 == 1, su2double(1.0), su2double(urlx), ga,
                                         su2double(kRho), su2double(kMuT), su2double(kTke), zeroAD, 0.0, 0.0,
                                         nullptr, 3, nullptr, 0.0);
          for (int i = 0; i < 3; ++i)
            for (int j = 0; j < 3; ++j) {
              CHECK(Same(zero[i][j], plain[i][j]));
              CHECK(Same(with[i][j], plain[i][j]));
              CHECK(Same(SU2_TYPE::GetValue(zeroAD[i][j]), SU2_TYPE::GetValue(plainAD[i][j])));
            }
        }
    }
}

TEST_CASE("UQ_FADE_GAP fade factor", "[EPMFade]") {
  for (passivedouble c : {1e-3, 1e-2, 5e-2, 1.0}) {
    CHECK(CNumerics::EPMFadeFactor(passivedouble(0), c) == 0.0);
    CHECK(CNumerics::EPMFadeFactor(c * c, c) == Approx(0.5).epsilon(1e-15));
    passivedouble previous = 0;
    for (passivedouble g = 1e-12; g < 1e6; g *= 1.7) {
      const passivedouble f = CNumerics::EPMFadeFactor(g * g, c);
      CHECK(std::isfinite(f));
      CHECK(f >= 0.0);
      CHECK(f <= 1.0);
      CHECK(f >= previous - 1e-15);  // monotone up to rounding
      previous = f;
    }
    /*--- f -> 1 for a large gap: 1 - f = c^2 / (g^2 + c^2). ---*/
    const passivedouble large = 1e3 * c;
    CHECK(1.0 - CNumerics::EPMFadeFactor(large * large, c) == Approx(c * c / (large * large + c * c)).epsilon(1e-8));
    CHECK(CNumerics::EPMFadeFactor(passivedouble(1e100), c) == 1.0);
  }
}

TEST_CASE("UQ_FADE_GAP blends with UQ_URLX f, f from the direction gap", "[EPMFade]") {
  for (unsigned short nDim : {2, 3})
    for (uint64_t seed = 1; seed <= 10; ++seed) {
      const RandomState s(nDim, seed);
      for (int mode = 0; mode < 5; ++mode)
        for (double urlx : {.1, -.1, 1.0})
          for (passivedouble c : {1e-3, 5e-2, .3}) {
            passivedouble base[3][3], target[3][3], plain[3][3], faded[3][3];
            Perturbed(nDim, mode, 0.0, s.g, kRho, kMuT, kTke, 0.0, base);
            Perturbed(nDim, mode, 1.0, s.g, kRho, kMuT, kTke, 0.0, target);
            Perturbed(nDim, mode, urlx, s.g, kRho, kMuT, kTke, 0.0, plain);
            const auto spectrum = Perturbed(nDim, mode, urlx, s.g, kRho, kMuT, kTke, c, faded);
            const passivedouble f = spectrum.fade;
            CHECK(f >= 0.0);
            CHECK(f <= 1.0);
            CHECK(f == Approx(ExpectedFade(mode, spectrum, c)).epsilon(0).margin(1e-14));
            const passivedouble scale = MaxAbs(base) + MaxAbs(target);
            for (int i = 0; i < 3; ++i)
              for (int j = 0; j < 3; ++j) {
                CHECK(std::isfinite(faded[i][j]));
                CHECK(faded[i][j] ==
                      Approx(base[i][j] + urlx * f * (target[i][j] - base[i][j])).epsilon(0).margin(1e-13 * scale));
                /*--- 3C has no direction: no fade, bit for bit. ---*/
                if (mode == 4) CHECK(Same(faded[i][j], plain[i][j]));
              }
          }
    }
}

TEST_CASE("UQ_FADE_GAP at degenerate states", "[EPMFade]") {
  const passivedouble c = .05, urlx = .1;
  /*--- Zero strain (and pure rotation): no direction, f = 0, the stress is the finite baseline 2/3 k I. ---*/
  for (unsigned short nDim : {2, 3})
    for (int which : {0, 1}) {
      passivedouble g[3][3];
      Degenerate(nDim, which, g);
      for (int mode = 0; mode < 5; ++mode) {
        passivedouble faded[3][3], plain[3][3];
        const auto spectrum = Perturbed(nDim, mode, urlx, g, kRho, kMuT, kTke, c, faded);
        Perturbed(nDim, mode, urlx, g, kRho, kMuT, kTke, 0.0, plain);
        CHECK(spectrum.fade == (mode == 4 ? 1.0 : 0.0));
        for (int i = 0; i < 3; ++i)
          for (int j = 0; j < 3; ++j) {
            CHECK(std::isfinite(faded[i][j]));
            if (mode == 4) {
              CHECK(Same(faded[i][j], plain[i][j]));
            } else {
              CHECK(faded[i][j] == Approx(i == j ? 2.0 / 3.0 * kTke : 0.0).epsilon(0).margin(1e-15));
            }
          }
        /*--- No tangent term at f = 0. ---*/
        passivedouble dM[3][3];
        const passivedouble n[3] = {0, 1, 0};
        if (mode != 4) CHECK_FALSE(CNumerics::EPMTangentStressJacobian(nDim, spectrum, urlx, kMuT, kMuLam, n, dM));
      }
    }
  /*--- Smooth wall (k = mu_t = 0): zero stress, no eigensystem. ---*/
  {
    const RandomState s(3, 1);
    passivedouble wall[3][3];
    const auto spectrum = Perturbed(3, 0, urlx, s.g, kRho, 0.0, 0.0, c, wall);
    CHECK_FALSE(spectrum.valid);
    for (const auto& row : wall)
      for (auto x : row) CHECK(x == 0.0);
  }
  /*--- 2D shear du/dy = G: g = nu_t G / (2 k) for all 1C and 2C modes. ---*/
  {
    const passivedouble rho = 1, mut = 1e-4, k = 1e-3, G = 2, grad[2][2] = {{0, G}, {0, 0}};
    const passivedouble gap = mut / rho * G / (2 * k);
    for (int mode = 0; mode < 4; ++mode) {
      passivedouble faded[3][3];
      const auto spectrum = Perturbed(2, mode, urlx, grad, rho, mut, k, c, faded);
      CHECK(spectrum.fade == Approx(gap * gap / (gap * gap + c * c)).epsilon(1e-12));
    }
  }
  /*--- 2D out-of-plane crossing, du/dx = 1, dv/dy = +-h: the 1C_KEEP target jumps between e_y and e_z (R_yy, R_zz
   *    by about 2 A k); the fade (gap |a_yy - a_zz| = nu_t h / k) makes it continuous. 2C_KEEP singles out e_x, whose
   *    gap stays nu_t / k: almost no fade there. ---*/
  {
    const passivedouble h = 1e-7, plus[2][2] = {{1, 0}, {0, h}}, minus[2][2] = {{1, 0}, {0, -h}};
    passivedouble a[3][3], b[3][3], fa[3][3], fb[3][3];
    Perturbed(2, 0, urlx, plus, kRho, kMuT, kTke, 0.0, a);
    Perturbed(2, 0, urlx, minus, kRho, kMuT, kTke, 0.0, b);
    const auto sa = Perturbed(2, 0, urlx, plus, kRho, kMuT, kTke, c, fa);
    const auto sb = Perturbed(2, 0, urlx, minus, kRho, kMuT, kTke, c, fb);
    CHECK(std::fabs(a[2][2] - b[2][2]) > urlx * kTke);
    CHECK(sa.fade < 1e-9);
    CHECK(sb.fade < 1e-9);
    for (int i = 0; i < 3; ++i)
      for (int j = 0; j < 3; ++j) CHECK(std::fabs(fa[i][j] - fb[i][j]) < 1e-6 * kTke);
    passivedouble unused[3][3];
    CHECK(Perturbed(2, 2, urlx, plus, kRho, kMuT, kTke, c, unused).fade > .9);
  }
}

TEST_CASE("UQ_FADE_GAP scales the tangent term by the frozen fade factor", "[EPMFade]") {
  for (unsigned short nDim : {2, 3})
    for (uint64_t seed = 1; seed <= 8; ++seed) {
      const RandomState s(nDim, seed);
      for (int mode = 0; mode < 5; ++mode)
        for (double urlx : {.1, -.1, 1.0})
          for (passivedouble c : {5e-2, .3}) {
            passivedouble unused[3][3];
            const auto plain = Perturbed(nDim, mode, urlx, s.g, kRho, kMuT, kTke, 0.0, unused);
            const auto faded = Perturbed(nDim, mode, urlx, s.g, kRho, kMuT, kTke, c, unused);
            /*--- Same eigen data; only the fade factor differs. ---*/
            CHECK(faded.valid == plain.valid);
            CHECK(faded.permute == plain.permute);
            for (int i = 0; i < 3; ++i) {
              CHECK(Same(faded.baseline[i], plain.baseline[i]));
              CHECK(Same(faded.target[i], plain.target[i]));
              for (int j = 0; j < 3; ++j) CHECK(Same(faded.vec[i][j], plain.vec[i][j]));
            }
            /*--- The term of the faded spectrum is the term at UQ_URLX f, bit for bit, also in exact mode. ---*/
            auto reference = faded;
            reference.fade = 1.0;
            for (bool exact : {false, true}) {
              passivedouble dM[3][3], dMref[3][3];
              const bool any = CNumerics::EPMTangentStressJacobian(nDim, faded, urlx, kMuT, kMuLam, s.n, dM, exact);
              const bool anyRef = CNumerics::EPMTangentStressJacobian(nDim, reference, urlx * faded.fade, kMuT,
                                                                      kMuLam, s.n, dMref, exact);
              CHECK(any == anyRef);
              for (int i = 0; i < 3; ++i)
                for (int j = 0; j < 3; ++j) CHECK(Same(dM[i][j], dMref[i][j]));
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
  Double turbKe;
  passivedouble n[Double::Size][3] = {};

  static bool Wall(size_t lane) { return Double::Size > 1 && lane == Double::Size - 1; }

  SimdEdge() {
    for (size_t i = 0; i < v.nVar; ++i) v.all(i) = 0;
    for (size_t k = 0; k < Double::Size; ++k) {
      const RandomState s(nDim, k + 1);
      v.density()[k] = kRho;
      v.laminarVisc()[k] = kMuLam;
      v.eddyVisc()[k] = Wall(k) ? 0.0 : kMuT;
      turbKe[k] = Wall(k) ? 0.0 : kTke;
      for (size_t i = 0; i < nDim; ++i) {
        n[k][i] = s.n[i];
        normal(i)[k] = s.n[i];
        grad(0, i)[k] = .1;
        for (size_t j = 0; j < nDim; ++j) grad(i + 1, j)[k] = s.g[i][j];
      }
    }
  }
};

template <size_t nDim>
void CheckSimdFade() {
  const SimdEdge<nDim> s;
  for (int mode = 0; mode < 5; ++mode)
    for (double urlx : {.1, -.1})
      for (passivedouble c : {5e-2, .3}) {
        const size_t comp = mode / 2 + 1;
        const bool permute = mode % 2 == 1;
        auto plain = stressTensor(s.v.laminarVisc(), s.grad);
        auto faded = plain, withJac = plain, withJacOff = plain, withJacDefault = plain;
        addPerturbedRSM(s.v, s.grad, s.turbKe, plain, comp, permute, su2double(1), su2double(urlx));
        addPerturbedRSMFaded(s.v, s.grad, s.turbKe, faded, comp, permute, su2double(1), su2double(urlx), c);
        EPMTangentLanes lanes, lanesOff, lanesDefault;
        addPerturbedRSMWithJacobian(s.v, s.grad, s.turbKe, withJac, s.normal, comp, permute, su2double(1),
                                    su2double(urlx), lanes, c);
        addPerturbedRSMWithJacobian(s.v, s.grad, s.turbKe, withJacOff, s.normal, comp, permute, su2double(1),
                                    su2double(urlx), lanesOff, 0.0);
        addPerturbedRSMWithJacobian(s.v, s.grad, s.turbKe, withJacDefault, s.normal, comp, permute, su2double(1),
                                    su2double(urlx), lanesDefault);

        for (size_t k = 0; k < Double::Size; ++k) {
          passivedouble velgrad[3][3] = {}, rsm[3][3];
          for (size_t i = 0; i < nDim; ++i)
            for (size_t j = 0; j < nDim; ++j) velgrad[i][j] = SU2_TYPE::GetValue(s.grad(i + 1, j)[k]);
          const passivedouble mut = SU2_TYPE::GetValue(s.v.eddyVisc()[k]), tke = SU2_TYPE::GetValue(s.turbKe[k]);
          const auto spectrum = Perturbed(nDim, mode, urlx, velgrad, kRho, mut, tke, c, rsm);
          const passivedouble scale = MaxAbs(rsm) * kRho + 1;

          for (size_t i = 0; i < nDim; ++i)
            for (size_t j = 0; j < nDim; ++j) {
              const passivedouble lam = SU2_TYPE::GetValue(stressTensor(s.v.laminarVisc(), s.grad)(i, j)[k]);
              /*--- The faded lane stress is the passive faded stress of the lane. ---*/
              CHECK(SU2_TYPE::GetValue(faded(i, j)[k]) ==
                    Approx(lam - kRho * rsm[i][j]).epsilon(0).margin(1e-13 * scale));
              /*--- With the Jacobian: same stress as without (fade on and off, default argument = off). ---*/
              CHECK(Same(SU2_TYPE::GetValue(withJac(i, j)[k]), SU2_TYPE::GetValue(faded(i, j)[k])));
              CHECK(Same(SU2_TYPE::GetValue(withJacOff(i, j)[k]), SU2_TYPE::GetValue(plain(i, j)[k])));
              CHECK(Same(SU2_TYPE::GetValue(withJacDefault(i, j)[k]), SU2_TYPE::GetValue(plain(i, j)[k])));
            }

          /*--- Lane term: the helper on the faded eigen data of the lane; off and default agree bit for bit. ---*/
          passivedouble dM[3][3];
          const bool any = CNumerics::EPMTangentStressJacobian(nDim, spectrum, urlx, mut, kMuLam, s.n[k], dM);
          CHECK(lanes.nonzero[k] == any);
          if (SimdEdge<nDim>::Wall(k)) CHECK_FALSE(lanes.nonzero[k]);
          const passivedouble dScale = MaxAbs(dM) + kMuT;
          if (any)
            for (size_t i = 0; i < 3; ++i)
              for (size_t j = 0; j < 3; ++j)
                CHECK(lanes.dM[k][i][j] == Approx(dM[i][j]).epsilon(0).margin(1e-12 * dScale));
          CHECK(lanesOff.nonzero[k] == lanesDefault.nonzero[k]);
          if (lanesOff.nonzero[k])
            for (size_t i = 0; i < 3; ++i)
              for (size_t j = 0; j < 3; ++j) CHECK(Same(lanesOff.dM[k][i][j], lanesDefault.dM[k][i][j]));
        }
      }
}
}  // namespace

TEST_CASE("Vectorized viscous flux: UQ_FADE_GAP", "[EPMFade]") {
  CheckSimdFade<2>();
  CheckSimdFade<3>();
}

namespace {
/*--- Residual of the scalar viscous flux (CAvgGrad_Flow, explicit) or of the SST source at the 2D point state of
 *    epm_test (du/dy = 2, rho = 2, mu_t = 0.3, k = 1: anisotropy gap g = nu_t s / k = 0.15 for 1C and 2C). ---*/
std::array<passivedouble, 4> Residual(bool flux, const std::string& text) {
  std::stringstream options(text);
  CConfig config(options, SU2_COMPONENT::SU2_CFD, false);
  epm_test::PointState s(100, 2);
  const auto numerics = epm_test::MakeNumerics(flux, 2, config, s);
  const auto residual = numerics->ComputeResidual(&config);
  std::array<passivedouble, 4> out{};
  for (int i = 0; i < (flux ? 4 : 2); ++i) out[i] = SU2_TYPE::GetValue(residual[i]);
  return out;
}

/*--- Implicit scalar CAvgGrad_Flow at the same state: flux and Jacobian_i. ---*/
struct ImplicitOutput {
  passivedouble flux[4] = {}, jac[4][4] = {};
};
ImplicitOutput Implicit(const std::string& text) {
  std::stringstream options(text);
  CConfig config(options, SU2_COMPONENT::SU2_CFD, false);
  config.SetKind_TimeIntScheme(EULER_IMPLICIT);
  epm_test::PointState s(100, 2);
  CAvgGrad_Flow numerics(2, 4, false, &config);
  numerics.SetPrimitive(s.v, s.v);
  numerics.SetPrimVarGradient({&s.grad[0][0], 3}, {&s.grad[0][0], 3});
  numerics.SetTurbKineticEnergy(1, 1);
  numerics.SetCoord(s.x, s.y);
  numerics.SetNormal(s.normal);
  const auto residual = numerics.ComputeResidual(&config);
  ImplicitOutput out;
  for (int i = 0; i < 4; ++i) {
    out.flux[i] = SU2_TYPE::GetValue(residual[i]);
    for (int j = 0; j < 4; ++j) out.jac[i][j] = SU2_TYPE::GetValue(residual.jacobian_i[i][j]);
  }
  return out;
}
}  // namespace

TEST_CASE("UQ_FADE_GAP reaches the scalar flux, the SST source and the scalar tangent term", "[EPMFade]") {
  const double urlx = .1, c = .05, gap = .15;
  const double f = gap * gap / (gap * gap + c * c);  // 0.9
  for (bool flux : {true, false})
    for (int mode = 0; mode < 5; ++mode) {
      const auto plain = Residual(flux, UQText(mode, urlx));
      const auto off = Residual(flux, UQText(mode, urlx, "UQ_FADE_GAP= 0\n"));
      const auto faded = Residual(flux, UQText(mode, urlx, "UQ_FADE_GAP= " + Number(c) + "\n"));
      /*--- 1C/2C: the fade is the blend at UQ_URLX f; 3C: no fade. ---*/
      const auto reference = Residual(flux, UQText(mode, mode == 4 ? urlx : urlx * f));
      /*--- A very large gap scale removes the perturbation: the residual at UQ_URLX= 0. ---*/
      const auto removed = Residual(flux, UQText(mode, urlx, "UQ_FADE_GAP= 1e6\n"));
      const auto baseline = Residual(flux, UQText(mode, mode == 4 ? urlx : 0.0));
      passivedouble scale = 0;
      for (auto x : plain) scale = std::max(scale, std::fabs(x));
      for (int i = 0; i < 4; ++i) {
        CHECK(Same(off[i], plain[i]));
        CHECK(faded[i] == Approx(reference[i]).epsilon(0).margin(1e-12 * scale));
        CHECK(removed[i] == Approx(baseline[i]).epsilon(0).margin(1e-10 * scale));
        if (mode == 4) CHECK(Same(faded[i], plain[i]));
      }
    }

  /*--- Scalar tangent term (1C_KEEP and 2C_KEEP, A > 0: active pair): residual unchanged by TANGENT; the term is
   *    f times the unfaded one (cap not reached), and vanishes for a very large gap scale. ---*/
  for (int mode : {0, 2}) {
    const auto noneOff = Implicit(UQText(mode, urlx, "UQ_JACOBIAN= NONE\n"));
    const auto tangentOff = Implicit(UQText(mode, urlx, "UQ_JACOBIAN= TANGENT\n"));
    const auto noneFaded = Implicit(UQText(mode, urlx, "UQ_JACOBIAN= NONE\nUQ_FADE_GAP= " + Number(c) + "\n"));
    const auto tangentFaded = Implicit(UQText(mode, urlx, "UQ_JACOBIAN= TANGENT\nUQ_FADE_GAP= " + Number(c) + "\n"));
    const auto noneRemoved = Implicit(UQText(mode, urlx, "UQ_JACOBIAN= NONE\nUQ_FADE_GAP= 1e6\n"));
    const auto tangentRemoved = Implicit(UQText(mode, urlx, "UQ_JACOBIAN= TANGENT\nUQ_FADE_GAP= 1e6\n"));
    passivedouble termScale = 0;
    for (int i = 0; i < 4; ++i)
      for (int j = 0; j < 4; ++j)
        termScale = std::max(termScale, std::fabs(tangentOff.jac[i][j] - noneOff.jac[i][j]));
    REQUIRE(termScale > 0);
    for (int i = 0; i < 4; ++i) {
      CHECK(Same(tangentFaded.flux[i], noneFaded.flux[i]));
      for (int j = 0; j < 4; ++j) {
        const passivedouble term = tangentFaded.jac[i][j] - noneFaded.jac[i][j];
        CHECK(term == Approx(f * (tangentOff.jac[i][j] - noneOff.jac[i][j])).epsilon(0).margin(1e-10 * termScale));
        CHECK(tangentRemoved.jac[i][j] - noneRemoved.jac[i][j] == Approx(0.0).epsilon(0).margin(1e-10 * termScale));
      }
    }
  }
}

/*--- Last, so that an abort in CConfig cannot hide the numerical tests above. ---*/
TEST_CASE("UQ_FADE_GAP option and guards", "[EPMFade]") {
  {
    std::stringstream options(UQText(0, .1));
    CConfig config(options, SU2_COMPONENT::SU2_CFD, false);
    CHECK(SU2_TYPE::GetValue(config.GetUQ_FadeGap()) == 0.0);
    CHECK(config.GetUQFadeGapError().empty());
  }
  for (int mode = 0; mode < 5; ++mode)
    for (double c : {0.0, .01, .05}) {
      std::stringstream options(UQText(mode, .1, "UQ_FADE_GAP= " + Number(c) + "\n"));
      CConfig config(options, SU2_COMPONENT::SU2_CFD, false);
      CHECK(SU2_TYPE::GetValue(config.GetUQ_FadeGap()) == c);
      CHECK(config.GetUQFadeGapError().empty());
    }
  /*--- With UQ_JACOBIAN= TANGENT. ---*/
  {
    std::stringstream options(UQText(0, .1, "UQ_JACOBIAN= TANGENT\nUQ_FADE_GAP= 0.05\n"));
    CConfig config(options, SU2_COMPONENT::SU2_CFD, false);
    CHECK(config.GetUQFadeGapError().empty());
  }

  /*--- SU2_MPI::Error aborts, so the guard is checked on valid configurations with the value set afterwards. ---*/
  const auto error = [](const std::string& text, double c) {
    std::stringstream options(text);
    CConfig config(options, SU2_COMPONENT::SU2_CFD, false);
    config.SetUQ_FadeGap(c);
    return config.GetUQFadeGapError();
  };
  CHECK(error(UQText(0, .1), .05).empty());
  CHECK(error(UQText(0, .1), -.05).find("UQ_FADE_GAP must be") != std::string::npos);
  CHECK(error(UQText(0, .1), std::numeric_limits<double>::quiet_NaN()).find("UQ_FADE_GAP must be") !=
        std::string::npos);
  CHECK(error(UQText(0, .1), std::numeric_limits<double>::infinity()).find("UQ_FADE_GAP must be") !=
        std::string::npos);
  /*--- No classical EPM. ---*/
  const std::string noUQ = "SOLVER= RANS\nREYNOLDS_NUMBER= 1e6\nKIND_TURB_MODEL= SST\nSST_OPTIONS= V2003m\n";
  CHECK(error(noUQ, .05).find("classical EPM") != std::string::npos);
  CHECK(error(noUQ, 0.0).empty());
  CHECK(error("SOLVER= RANS\nREYNOLDS_NUMBER= 1e6\nKIND_TURB_MODEL= SA\n", .05).find("classical EPM") !=
        std::string::npos);
}
