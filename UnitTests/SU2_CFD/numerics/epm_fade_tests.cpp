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
#include "../../../Common/include/CEPMFieldSites.hpp"
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

/*--- Extraction and direct mode (Task 15): the change of the case is weighted by the fade factor f of the site,
 *    the f of the classical path at the same state. ---*/

TEST_CASE("UQ_FADE_GAP in the extraction helpers: c = 0 unchanged, c > 0 weights the change by f", "[EPMFade]") {
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
      for (int mode = 0; mode < 5; ++mode) {
        const unsigned short comp = mode / 2 + 1;
        const bool permute = mode % 2 == 1;
        /*--- c = 0: default argument and explicit 0 give the same bits and f = 1. ---*/
        passivedouble t0[3][3], tZero[3][3], c0[3][3], cZero[3][3];
        bool a0 = false, aZero = false, b0 = false, bZero = false;
        CHECK(CNumerics::EPMCaseTarget(nDim, g, kRho, kMuT, kTke, comp, permute, 0.0, 3, t0, a0) == 1.0);
        CHECK(CNumerics::EPMCaseTarget(nDim, g, kRho, kMuT, kTke, comp, permute, 0.0, 3, tZero, aZero, 0.0) == 1.0);
        CHECK(CNumerics::EPMCaseStressCorrection(nDim, g, kRho, kMuT, kTke, comp, permute, 0.0, 3, c0, b0) == 1.0);
        CHECK(CNumerics::EPMCaseStressCorrection(nDim, g, kRho, kMuT, kTke, comp, permute, 0.0, 3, cZero, bZero,
                                                 0.0) == 1.0);
        CHECK(a0 == aZero);
        CHECK(b0 == bZero);
        for (int i = 0; i < 3; ++i)
          for (int j = 0; j < 3; ++j) {
            CHECK(Same(tZero[i][j], t0[i][j]));
            CHECK(Same(cZero[i][j], c0[i][j]));
          }

        for (passivedouble c : {.05, .3}) {
          /*--- The target itself is not faded; f is the fade of the classical path at the same state. ---*/
          passivedouble tc[3][3], cc[3][3], rsm[3][3], unused[3][3];
          bool ac = false, bc = false;
          const passivedouble f = CNumerics::EPMCaseTarget(nDim, g, kRho, kMuT, kTke, comp, permute, 0.0, 3, tc, ac,
                                                           c);
          const auto classical = Perturbed(nDim, mode, .1, g, kRho, kMuT, kTke, c, rsm);
          const auto unfaded = Perturbed(nDim, mode, .1, g, kRho, kMuT, kTke, 0.0, unused);
          CHECK(f >= 0.0);
          CHECK(f <= 1.0);
          CHECK(f == Approx(classical.fade).epsilon(0).margin(1e-15));
          CHECK(CNumerics::EPMSpectrumFade(unfaded, c) == Approx(classical.fade).epsilon(0).margin(1e-15));
          if (mode == 4) CHECK(f == 1.0);
          CHECK(ac == a0);
          for (int i = 0; i < 3; ++i)
            for (int j = 0; j < 3; ++j) CHECK(Same(tc[i][j], t0[i][j]));

          /*--- The correction is f times the unfaded one, bit for bit. ---*/
          CHECK(Same(CNumerics::EPMCaseStressCorrection(nDim, g, kRho, kMuT, kTke, comp, permute, 0.0, 3, cc, bc, c),
                     f));
          CHECK(bc == b0);
          for (int i = 0; i < nDim; ++i)
            for (int j = 0; j < nDim; ++j) CHECK(Same(cc[i][j], c0[i][j] * f));

          /*--- Derivation check: the faded classical stress change per unit A, -rho (R_pert(A=1) - R), is
           *    f (tau_m - tau_0) + f 2/3 rho k I: the extracted change plus the isotropic term of the classical
           *    baseline (2/3 rho k I, absent from plain SST, #797), weighted by the same f. ---*/
          passivedouble r1[3][3], r0[3][3];
          Perturbed(nDim, mode, 1.0, g, kRho, kMuT, kTke, c, r1);
          Perturbed(nDim, mode, 0.0, g, kRho, kMuT, kTke, 0.0, r0);
          const passivedouble scale = kRho * (MaxAbs(r1) + MaxAbs(r0)) + MaxAbs(c0);
          for (int i = 0; i < nDim; ++i)
            for (int j = 0; j < nDim; ++j)
              CHECK(-kRho * (r1[i][j] - r0[i][j]) ==
                    Approx(cc[i][j] + (i == j ? f * 2.0 / 3.0 * kRho * kTke : 0.0)).epsilon(0).margin(1e-13 * scale));
        }
      }
    }
  /*--- Without an eigensystem (k = mu_t = 0) there is no fade information: f = 1 (the change is zero there). ---*/
  CNumerics::EPMSpectrum invalid;
  CHECK(CNumerics::EPMSpectrumFade(invalid, .05) == 1.0);
}

namespace {
using Record = CEPMFieldSites::Record;

/*--- Field-extraction record of one site (scalar flux: EDGE, SST source: NODE) at the epm_test point state, with
 *    UQ_FADE_GAP= c (set after parsing, as SST_EPM_POSTPROCESS cannot be parsed in a primal build). ---*/
Record FieldRecord(bool flux, unsigned short nDim, bool degenerate, passivedouble c) {
  std::stringstream options(epm_test::Options(0, false));
  CConfig config(options, SU2_COMPONENT::SU2_CFD, false);
  config.SetUQ_FadeGap(c);
  auto& epm = config.GetEPMExtraction();
  epm.active = true;
  epm.mode = CEPMExtraction::Mode::FIELD;
  epm.field = std::make_shared<CEPMFieldSites>();
  epm_test::PointState s(100, nDim, degenerate);
  const auto numerics = epm_test::MakeNumerics(flux, nDim, config, s);
  epm.cursor.kind = flux ? CEPMExtraction::SiteKind::EDGE : CEPMExtraction::SiteKind::NODE;
  epm.cursor.a = 3;
  epm.cursor.b = flux ? 7 : 0;
  epm.cursor.set = true;
  numerics->ComputeResidual(&config);
  REQUIRE(epm.field->Size() == 1);
  return epm.field->Data(0);
}

/*--- Fade factor of the case mode at the stored state of a record. ---*/
passivedouble RecordFade(unsigned short nDim, const Record& record, int mode, passivedouble c) {
  passivedouble grad[3][3], target[3][3];
  for (int i = 0; i < 3; ++i)
    for (int j = 0; j < 3; ++j) grad[i][j] = record.grad[i * 3 + j];
  bool ambiguous = false;
  return CNumerics::EPMCaseTarget(nDim, grad, record.rho, record.mu_t, record.k, mode / 2 + 1, mode % 2 == 1, 0.0, 3,
                                  target, ambiguous, c);
}

/*--- d/dA at A = 0 of sum (i+1) R_i of the scalar-amplitude replay (reverse AD; 0 in primal builds). ---*/
passivedouble ReplayDerivative(bool flux, unsigned short nDim, int mode, passivedouble c) {
  std::stringstream options(epm_test::Options(mode, false));
  CConfig config(options, SU2_COMPONENT::SU2_CFD, false);
  config.SetUQ_FadeGap(c);
  auto& epm = config.GetEPMExtraction();
  epm.mode = CEPMExtraction::Mode::SCALAR_REPLAY;
  epm.active = true;
  epm.component = mode / 2 + 1;
  epm.permute = mode % 2;
  epm.amplitude = 0.0;
  epm_test::PointState s(100, nDim);
  const auto numerics = epm_test::MakeNumerics(flux, nDim, config, s);
  AD::Reset();
#ifdef CODI_REVERSE_TYPE
  AD::PreaccEnabled = true;
#endif
  AD::StartRecording();
  AD::RegisterInput(epm.amplitude);
  const auto residual = numerics->ComputeResidual(&config);
  su2double objective = 0;
  for (int i = 0; i < (flux ? nDim + 2 : 2); ++i) objective += (i + 1) * residual[i];
  AD::RegisterOutput(objective);
  AD::StopRecording();
  SU2_TYPE::SetDerivative(objective, 1);
  AD::ComputeAdjoint();
  const passivedouble derivative = SU2_TYPE::GetDerivative(epm.amplitude);
  AD::ClearAdjoints();
  return derivative;
}
}  // namespace

TEST_CASE("UQ_FADE_GAP weights the field-extraction changes and gradients by f per site", "[EPMFade]") {
  for (unsigned short nDim : {2, 3})
    for (bool flux : {true, false})
      for (bool degenerate : {false, true}) {
        const auto plain = FieldRecord(flux, nDim, degenerate, 0.0);
        for (passivedouble c : {.05, .1}) {
          const auto faded = FieldRecord(flux, nDim, degenerate, c);
          /*--- Same stored state; only the changes are weighted. ---*/
          CHECK(Same(faded.rho, plain.rho));
          CHECK(Same(faded.mu_t, plain.mu_t));
          CHECK(Same(faded.k, plain.k));
          for (int k = 0; k < 9; ++k) CHECK(Same(faded.grad[k], plain.grad[k]));
          if (!flux) {
            CHECK(Same(faded.pk0, plain.pk0));
            CHECK(Same(faded.pw0, plain.pw0));
          }
          /*--- Sensitivities of the site (any values): the gradient of the faded field is f times the unfaded one,
           *    for the canonical case and the orientation probes. ---*/
          auto fs = faded, ps = plain;
          for (int k = 0; k < 9; ++k) fs.S_tau[k] = ps.S_tau[k] = .1 * (k + 1) - .35;
          fs.S_pk = ps.S_pk = .7;
          fs.S_pw = ps.S_pw = -.2;

          for (int mode = 0; mode < 5; ++mode) {
            const passivedouble f = RecordFade(nDim, plain, mode, c);
            CHECK(f >= 0.0);
            CHECK(f <= 1.0);
            if (mode == 4) CHECK(f == 1.0);
            /*--- The f of the classical path at the stored state. ---*/
            passivedouble grad[3][3], rsm[3][3];
            for (int i = 0; i < 3; ++i)
              for (int j = 0; j < 3; ++j) grad[i][j] = plain.grad[i * 3 + j];
            CHECK(f == Approx(Perturbed(nDim, mode, .1, grad, plain.rho, plain.mu_t, plain.k, c, rsm).fade)
                           .epsilon(0).margin(1e-15));
            for (int k = 0; k < 9; ++k) CHECK(Same(faded.dtau[mode][k], plain.dtau[mode][k] * f));
            CHECK(Same(faded.dpk[mode], plain.dpk[mode] * f));
            CHECK(Same(faded.dpw[mode], plain.dpw[mode] * f));

            passivedouble scale = std::fabs(ps.S_pk * ps.dpk[mode]) + std::fabs(ps.S_pw * ps.dpw[mode]);
            for (int k = 0; k < 9; ++k) scale += std::fabs(ps.S_tau[k] * ps.dtau[mode][k]);
            for (unsigned short probe = 0; probe < 4; ++probe) {
              bool a0 = false, ac = false;
              const auto g0 = CNumerics::EPMFieldSiteGradient(nDim, ps, mode, probe, a0);
              const auto gc = CNumerics::EPMFieldSiteGradient(nDim, fs, mode, probe, ac, c);
              CHECK(ac == a0);
              CHECK(gc == Approx(f * g0).epsilon(0).margin(1e-12 * (scale + std::fabs(g0))));
              /*--- c = 0 passed explicitly: bit for bit the default. ---*/
              bool aZero = false;
              CHECK(Same(CNumerics::EPMFieldSiteGradient(nDim, ps, mode, probe, aZero, 0.0), g0));
            }
          }
        }
      }
}

TEST_CASE("UQ_FADE_GAP: scalar-replay derivative is f times the unfaded one", "[EPMFade]") {
  for (unsigned short nDim : {2, 3})
    for (bool flux : {true, false}) {
      const auto record = FieldRecord(flux, nDim, false, 0.0);
      for (int mode = 0; mode < 5; ++mode) {
        const passivedouble plain = ReplayDerivative(flux, nDim, mode, 0.0);
        for (passivedouble c : {.05, .1}) {
          const passivedouble f = RecordFade(nDim, record, mode, c);
          const passivedouble faded = ReplayDerivative(flux, nDim, mode, c);
          CHECK(faded == Approx(f * plain).epsilon(0).margin(1e-12 * std::fabs(plain) + 1e-15));
        }
#ifdef CODI_REVERSE_TYPE
        /*--- The unfaded derivative is not trivially zero (so the check above means something). ---*/
        CHECK(std::fabs(plain) > 0.0);
#endif
      }
    }
}

/*--- SST_EPM_DIRECT is rejected in builds with OpenMP (see epm_direct_tests.cpp). ---*/
#ifndef HAVE_OMP
namespace {
/*--- Plain SST primal in EPM direct mode of the case at the amplitude; UQ_FADE_GAP= c unless c < 0 (unset). The
 *    options are parsed, so the direct and fade guards run. ---*/
std::string DirectText(int mode, double amplitude, double c) {
  std::string text =
      "SOLVER= RANS\nMATH_PROBLEM= DIRECT\nREYNOLDS_NUMBER= 1e6\nKIND_TURB_MODEL= SST\nSST_OPTIONS= V2003m\n"
      "SLOPE_LIMITER_FLOW= NONE\nMGLEVEL= 0\nCFL_ADAPT= NO\nUQ_DELTA_B= 1\nUQ_URLX= 1\nUQ_COMPONENT= " +
      std::to_string(mode / 2 + 1) + "\nUQ_PERMUTE= " + (mode % 2 ? "YES" : "NO") +
      "\nSST_EPM_DIRECT= YES\nSST_EPM_DIRECT_AMPLITUDE= " + Number(amplitude) + "\n";
  if (c >= 0) text += "UQ_FADE_GAP= " + Number(c) + "\n";
  return text;
}

std::array<passivedouble, 5> DirectResidual(bool flux, unsigned short nDim, const std::string& text) {
  std::stringstream options(text);
  CConfig config(options, SU2_COMPONENT::SU2_CFD, false);
  REQUIRE(config.GetEPMExtraction().active);
  epm_test::PointState s(100, nDim);
  const auto numerics = epm_test::MakeNumerics(flux, nDim, config, s);
  const auto residual = numerics->ComputeResidual(&config);
  std::array<passivedouble, 5> out{};
  for (int i = 0; i < (flux ? nDim + 2 : 2); ++i) out[i] = SU2_TYPE::GetValue(residual[i]);
  return out;
}
}  // namespace

TEST_CASE("SST_EPM_DIRECT with UQ_FADE_GAP is the direct mode at amplitude A f", "[EPMFade]") {
  for (unsigned short nDim : {2, 3})
    for (bool flux : {true, false}) {
      const int n = flux ? nDim + 2 : 2;
      const auto record = FieldRecord(flux, nDim, false, 0.0);
      for (int mode = 0; mode < 5; ++mode) {
        /*--- c = 0 (explicit) is the direct mode without the option, bit for bit. ---*/
        const auto unset = DirectResidual(flux, nDim, DirectText(mode, .5, -1));
        const auto off = DirectResidual(flux, nDim, DirectText(mode, .5, 0));
        for (int i = 0; i < n; ++i) CHECK(Same(off[i], unset[i]));

        const auto base = DirectResidual(flux, nDim, DirectText(mode, 0, -1));
        for (passivedouble c : {.05, .1}) {
          const passivedouble f = RecordFade(nDim, record, mode, c);
          /*--- A = 0 stays the plain SST residual, bit for bit. ---*/
          const auto zero = DirectResidual(flux, nDim, DirectText(mode, 0, c));
          for (int i = 0; i < n; ++i) CHECK(Same(zero[i], base[i]));
          for (double amplitude : {-.5, .25, 1.0}) {
            /*--- F0 + A f (Fm - F0) (source: pk0 + A f (clip(P_m) - pk0)) = the unfaded direct mode at A f. ---*/
            const auto faded = DirectResidual(flux, nDim, DirectText(mode, amplitude, c));
            const auto reference = DirectResidual(flux, nDim, DirectText(mode, amplitude * f, -1));
            const auto full = DirectResidual(flux, nDim, DirectText(mode, amplitude, -1));
            passivedouble scale = 1;
            for (int i = 0; i < n; ++i) scale = std::max({scale, std::fabs(full[i]), std::fabs(base[i])});
            for (int i = 0; i < n; ++i)
              CHECK(faded[i] == Approx(reference[i]).epsilon(0).margin(1e-12 * scale));
            if (mode == 4)
              for (int i = 0; i < n; ++i) CHECK(Same(faded[i], full[i]));
          }
        }
      }
    }
}
#endif

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
  /*--- 0 < c < 1e-8: c^2 may underflow, giving 0/0 at an exact tie. ---*/
  for (double tiny : {1e-9, 1e-200, std::numeric_limits<double>::denorm_min()})
    CHECK(error(UQText(0, .1), tiny).find("at least 1e-8") != std::string::npos);
  CHECK(error(UQText(0, .1), 1e-8).empty());
  /*--- No classical EPM. ---*/
  const std::string noUQ = "SOLVER= RANS\nREYNOLDS_NUMBER= 1e6\nKIND_TURB_MODEL= SST\nSST_OPTIONS= V2003m\n";
  CHECK(error(noUQ, .05).find("classical EPM") != std::string::npos);
  CHECK(error(noUQ, 0.0).empty());
  /*--- The EPM extraction and the direct mode take the fade without the classical EPM (their flags are set directly:
   *    parsing SST_EPM_POSTPROCESS aborts in a primal build). The value checks still apply. ---*/
  {
    std::stringstream options(noUQ);
    CConfig config(options, SU2_COMPONENT::SU2_CFD, false);
    config.SetUQ_FadeGap(.05);
    auto& epm = config.GetEPMExtraction();
    epm.enabled = true;
    CHECK(config.GetUQFadeGapError().empty());
    epm.enabled = false;
    epm.direct = true;
    CHECK(config.GetUQFadeGapError().empty());
    config.SetUQ_FadeGap(-.05);
    CHECK(config.GetUQFadeGapError().find("UQ_FADE_GAP must be") != std::string::npos);
    epm.direct = false;
    config.SetUQ_FadeGap(.05);
    CHECK(config.GetUQFadeGapError().find("SST_EPM_POSTPROCESS= YES or SST_EPM_DIRECT= YES") != std::string::npos);
  }
  CHECK(error("SOLVER= RANS\nREYNOLDS_NUMBER= 1e6\nKIND_TURB_MODEL= SA\n", .05).find("classical EPM") !=
        std::string::npos);
}
