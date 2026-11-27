#include "catch.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <random>
#include <vector>

#include "../../SU2_CFD/include/numerics/flow/jst_entropy_fourth.hpp"
#include "../../SU2_CFD/include/numerics/flow/p1_pair_jst.hpp"

namespace {
namespace es4 = JSTEntropyFourth;
namespace pbc = P1BoundaryClosure;
namespace jst = P1PairJST;

constexpr double GasGamma = 1.4;
using Row = std::array<su2double, pbc::ES_NVAR>;

Row Entropy(unsigned short dim, const pbc::EsState& state) {
  Row w{};
  pbc::EsEntropyVariables(dim, GasGamma, state, w.data());
  return w;
}

Row Enthalpy(unsigned short dim, const pbc::EsState& state) {
  Row u{};
  pbc::EsConservative(dim, GasGamma, state, u.data());
  u[dim + 1] += state.p;
  return u;
}

Row Action(unsigned short dim, const pbc::EsState& state, const Row& x) {
  Row q{};
  es4::EnthalpyStateAction(dim, GasGamma, state, x.data(), q.data());
  return q;
}

double Dot(const Row& a, const Row& b) {
  double sum = 0.0;
  for (auto v = 0u; v < a.size(); ++v) sum += a[v] * b[v];
  return sum;
}

pbc::EsState State(unsigned short dim, std::mt19937& rng) {
  std::uniform_real_distribution<double> logState(-3.0, 3.0), dir(-1.0, 1.0);
  pbc::EsState s;
  s.rho = std::exp(logState(rng));
  s.p = std::exp(logState(rng));
  const auto speed = 3.0 * std::sqrt(GasGamma * s.p / (dim * s.rho));
  for (auto d = 0u; d < dim; ++d) s.vel[d] = speed * dir(rng);
  return s;
}

struct Edge {
  size_t i, k;
  double a, s, t;
};

struct Result {
  std::vector<Row> x, q, res;
  Row fluxMagnitude{};
};

Result Apply(unsigned short dim, const std::vector<pbc::EsState>& states, const std::vector<Edge>& edges) {
  const auto n = states.size();
  const unsigned short nVar = dim + 2;
  std::vector<Row> w(n);
  Result out{std::vector<Row>(n), std::vector<Row>(n), std::vector<Row>(n), {}};
  for (size_t i = 0; i < n; ++i) w[i] = Entropy(dim, states[i]);
  for (const auto& e : edges) {
    es4::AddStencilDifference(nVar, e.a * e.s, w[e.i].data(), w[e.k].data(), out.x[e.i].data());
    es4::AddStencilDifference(nVar, e.a * e.t, w[e.k].data(), w[e.i].data(), out.x[e.k].data());
  }
  for (size_t i = 0; i < n; ++i) out.q[i] = Action(dim, states[i], out.x[i]);
  for (const auto& e : edges) {
    Row f{};
    es4::AddCouplingFlux(nVar, e.a, e.s, e.t, out.q[e.i], out.q[e.k], f);
    for (auto v = 0u; v < nVar; ++v) {
      out.res[e.i][v] += f[v];
      out.res[e.k][v] -= f[v];
      out.fluxMagnitude[v] += std::abs(f[v]);
    }
  }
  return out;
}

Row FrozenFlux(const Edge& edge, double Si, double Sk, const pbc::EsState& state_i,
               const pbc::EsState& state_k, double step, bool perturbI, const Row& dw) {
  Row xi{}, xk{}, flux{};
  for (auto v = 0u; v < dw.size(); ++v) {
    xi[v] = (perturbI ? Si : -edge.s * edge.a) * step * dw[v];
    xk[v] = (perturbI ? -edge.t * edge.a : Sk) * step * dw[v];
  }
  es4::AddCouplingFlux(4, edge.a, edge.s, edge.t, Action(2, state_i, xi), Action(2, state_k, xk), flux);
  return flux;
}
}  // namespace

TEST_CASE("U1 ES4 full enthalpy Jacobian action and sharp entropy bound", "[ES4][JSTEntropyFourth]") {
  std::mt19937 rng(4027);
  std::uniform_real_distribution<double> sample(-1.0, 1.0);
  const auto bound = (GasGamma + 1 - std::sqrt(GasGamma * (GasGamma - 1))) / 2;
  for (const unsigned short dim : {2, 3}) {
    const auto nVar = dim + 2;
    for (auto i = 0u; i < 80; ++i) {
      const auto state = State(dim, rng);
      const auto w = Entropy(dim, state);
      Row x{}, Hx{}, Jx{};
      for (auto v = 0; v < nVar; ++v) x[v] = sample(rng);
      const auto actual = Action(dim, state, x);
      su2double P_w[5][5], U_P[5][5];
      pbc::EsProjectionJacobian(dim, GasGamma, state, P_w);
      pbc::EsConservativeJacobian(dim, GasGamma, state, U_P);
      for (auto v = 0; v < nVar; ++v) {
        for (auto p = 0; p < nVar; ++p)
          for (auto j = 0; j < nVar; ++j) Hx[v] += U_P[v][p] * P_w[p][j] * x[j];
        Jx[v] = Hx[v];
      }
      for (auto j = 0; j < nVar; ++j) Jx[nVar - 1] += P_w[nVar - 1][j] * x[j];
      const auto step = 1e-5 * std::min(1.0, std::abs(w[nVar - 1]));
      Row wp = w, wm = w;
      for (auto v = 0; v < nVar; ++v) {
        wp[v] += step * x[v];
        wm[v] -= step * x[v];
      }
      pbc::EsState sp, sm;
      pbc::EsStateFromEntropyVariables(dim, GasGamma, wp.data(), sp);
      pbc::EsStateFromEntropyVariables(dim, GasGamma, wm.data(), sm);
      const auto up = Enthalpy(dim, sp), um = Enthalpy(dim, sm);
      for (auto v = 0; v < nVar; ++v) {
        CHECK(actual[v] == Approx(Jx[v]).margin(1e-10).epsilon(1e-12));
        CHECK((up[v] - um[v]) / (2 * step) == Approx(Jx[v]).margin(2e-5).epsilon(2e-6));
      }
      CHECK(Dot(x, actual) >= (bound - 1e-11) * Dot(x, Hx));
    }
  }
  pbc::EsState rest;
  const Row energy{0, 0, 0, 1}, density{1, 0, 0, 0};
  const auto Jenergy = Action(2, rest, energy);
  const auto Jdensity = Action(2, rest, density);
  CHECK(Jenergy[0] != Approx(Jdensity[3]));
  const Row minimum{-(1 + GasGamma - 2 * bound) / (2 * (1 - bound) * (GasGamma - 1)), 0, 0, 1};
  const auto q = Action(2, rest, minimum);
  Row Hx = q;
  Hx[3] /= GasGamma;
  CHECK(Dot(minimum, q) / Dot(minimum, Hx) == Approx(bound).epsilon(1e-12));
}

TEST_CASE("U2 ES4 random asymmetric graphs conserve and dissipate entropy", "[ES4][JSTEntropyFourth]") {
  std::mt19937 rng(80);
  std::uniform_real_distribution<double> logChi(-6.0, 6.0);
  constexpr size_t n = 40;
  for (const unsigned short dim : {2, 3}) {
    for (auto sample = 0u; sample < 24; ++sample) {
      std::vector<pbc::EsState> states(n);
      double boundary[n];
      for (size_t i = 0; i < n; ++i) {
        states[i] = State(dim, rng);
        boundary[i] = (rng() % 4 == 0);
      }
      std::vector<Edge> edges;
      for (size_t i = 0; i < n; ++i) {
        for (size_t k = i + 1; k < n; ++k) {
          if (rng() % 7 != 0 && k != i + 1) continue;
          const double chi = (rng() % 7 == 0) ? 0.0 : std::exp(logChi(rng));
          const double share = std::array<double, 3>{1.0, 0.5, 0.25}[rng() % 3];
          edges.push_back({i, k, es4::CouplingWeight(chi, share), es4::StencilFlag(boundary[i], boundary[k]),
                           es4::StencilFlag(boundary[k], boundary[i])});
        }
      }
      const auto out = Apply(dim, states, edges);
      double rate = 0.0, production = 0.0, magnitude = 0.0;
      Row total{};
      for (size_t i = 0; i < n; ++i) {
        const double wr = Dot(Entropy(dim, states[i]), out.res[i]);
        rate -= wr;
        production -= Dot(out.x[i], out.q[i]);
        magnitude += std::abs(wr);
        for (auto v = 0u; v < total.size(); ++v) total[v] += out.res[i][v];
      }
      CAPTURE(dim, sample, std::abs(rate - production) / magnitude);
      CHECK(std::abs(rate - production) <= 1e-13 * magnitude);
      CHECK(production <= 0.0);
      for (auto v = 0; v < dim + 2; ++v) {
        CAPTURE(v, total[v] / out.fluxMagnitude[v]);
        CHECK(std::abs(total[v]) <= 1e-14 * out.fluxMagnitude[v]);
      }
    }
  }
}

TEST_CASE("U3 ES4 zero weight contributes exactly zero", "[ES4][JSTEntropyFourth]") {
  std::vector<pbc::EsState> states(2);
  states[1].p = 1.2;
  const auto out = Apply(2, states, {{0, 1, es4::CouplingWeight(0.0), 1.0, 1.0}});
  for (auto i : {0, 1}) {
    CHECK(out.x[i] == Row{});
    CHECK(out.q[i] == Row{});
    CHECK(out.res[i] == Row{});
  }
}

TEST_CASE("U4 ES4 transpose includes the incoming entry at a boundary", "[ES4][JSTEntropyFourth]") {
  const double a = 2.0, s = es4::StencilFlag(1.0, 0.0), t = es4::StencilFlag(0.0, 1.0);
  CHECK(s == 0.0);
  CHECK(t == 1.0);
  std::vector<pbc::EsState> states(2);
  states[1].rho = 2.0;
  states[1].p = 2.0;
  const auto out = Apply(2, states, {{0, 1, a, s, t}});
  CHECK(out.x[0] == Row{});
  const auto drho = 2.0 * a * std::log(2.0);
  CHECK(out.res[0][0] == Approx(-a * drho));
  CHECK(out.res[1][0] == Approx(a * drho));
  CHECK(out.res[1][3] == Approx(a * drho * GasGamma / (GasGamma - 1.0)));
}

TEST_CASE("U5 ES4 linearized split pair has one factor of pi", "[ES4][JSTEntropyFourth]") {
  const double chi = 3.5, share = 0.25;
  const double a = es4::CouplingWeight(chi, share);
  pbc::EsState rest;
  const Row dw{0.1, 0.2, -0.1, 0.3};
  const auto model = FrozenFlux({0, 1, a, 1.0, 1.0}, a, a, rest, rest, 1.0, true, dw);
  const auto du = Action(2, rest, dw);
  for (auto v = 0u; v < 4; ++v) CHECK(model[v] == Approx(2 * share * chi * du[v]));
}

TEST_CASE("U6 ES4 linearization recovers JST on a triangular torus", "[ES4][JSTEntropyFourth]") {
  constexpr int n = 16;
  constexpr double chi = 2.3, step = 1e-6;
  std::vector<pbc::EsState> plus(n * n), minus(n * n);
  std::vector<Edge> edges;
  std::vector<Row> du(n * n), lapl(n * n), expected(n * n);
  const auto w = Entropy(2, {});
  for (auto i = 0; i < n; ++i) {
    for (auto j = 0; j < n; ++j) {
      const size_t a = i * n + j, b = ((i + 1) % n) * n + j;
      const size_t c = i * n + (j + 1) % n, d = ((i + 1) % n) * n + (j + 1) % n;
      for (auto k : {b, c, d}) edges.push_back({a, k, es4::CouplingWeight(chi), 1.0, 1.0});
      const Row dw{std::sin(2 * std::acos(-1.0) * i / n) + ((i + j) % 2 ? -1.0 : 1.0), 0.1, 0.0, 0.0};
      du[a] = Action(2, {}, dw);
      Row wp = w, wm = w;
      for (auto v = 0u; v < 4; ++v) {
        wp[v] += step * dw[v];
        wm[v] -= step * dw[v];
      }
      pbc::EsStateFromEntropyVariables(2, GasGamma, wp.data(), plus[a]);
      pbc::EsStateFromEntropyVariables(2, GasGamma, wm.data(), minus[a]);
    }
  }
  for (const auto& e : edges) {
    for (auto v = 0u; v < 4; ++v) {
      const auto jump = du[e.k][v] - du[e.i][v];
      lapl[e.i][v] += jump;
      lapl[e.k][v] -= jump;
    }
  }
  for (const auto& e : edges) {
    for (auto v = 0u; v < 4; ++v) {
      const auto flux = -chi * (lapl[e.i][v] - lapl[e.k][v]);
      expected[e.i][v] += flux;
      expected[e.k][v] -= flux;
    }
  }
  const auto rp = Apply(2, plus, edges), rm = Apply(2, minus, edges);
  double error2 = 0.0, scale2 = 0.0;
  for (size_t i = 0; i < plus.size(); ++i) {
    Row error{};
    for (auto v = 0u; v < 4; ++v) error[v] = (rp.res[i][v] - rm.res[i][v]) / (2 * step) - expected[i][v];
    error2 += Dot(error, error);
    scale2 += Dot(expected[i], expected[i]);
  }
  CHECK(std::sqrt(error2 / scale2) < 1e-8);
}

TEST_CASE("U7 ES4 uniform state is exactly zero on an asymmetric graph", "[ES4][JSTEntropyFourth]") {
  std::mt19937 rng(23);
  for (const auto& row : Apply(3, std::vector<pbc::EsState>(3, State(3, rng)),
                               {{0, 1, 2.0, 0.0, 1.0}, {1, 2, 1.3, 1.0, 1.0}}).res)
    CHECK(row == Row{});
}

TEST_CASE("U8 ES4 kappa4 zero switches off the operator", "[ES4][JSTEntropyFourth]") {
  jst::Params par;
  par.kappa4 = 0.0;
  jst::Node i, k;
  i.lambda = 2.0;
  k.lambda = 2.5;
  i.sensor = 0.02;
  const auto chi = jst::FourthCoefficient(par, i, k, 0.8, 0.5);
  CHECK(chi == 0.0);
  const auto a = es4::CouplingWeight(chi, 0.5);
  CHECK(a == 0.0);
  std::mt19937 rng(234);
  for (const auto& row : Apply(2, {State(2, rng), State(2, rng)}, {{0, 1, a, 1.0, 1.0}}).res)
    CHECK(row == Row{});
}

TEST_CASE("U9 ES4 frozen derivative and one-ring Jacobian surrogate", "[ES4][JSTEntropyFourth]") {
  const auto a = es4::CouplingWeight(2.0);
  double di, dk;
  es4::JacobianCoefficients(a, 1.0, 1.0, 6 * a, 6 * a, di, dk);
  CHECK(di == Approx(14.0));
  CHECK(dk == Approx(14.0));

  pbc::EsState i, k;
  k.rho = 1.7;
  k.p = 0.6;
  k.vel[0] = 0.5;
  const Row dw{0.2, -0.3, 0.1, 0.7};
  const auto Ji = Action(2, i, dw);
  constexpr double h = 1e-6;
  for (const bool asymmetric : {false, true}) {
    const double weight = asymmetric ? es4::CouplingWeight(2.0, 0.25) : a;
    const Edge edge{0, 1, weight, asymmetric ? 0.0 : 1.0, 1.0};
    const double Si = asymmetric ? 0.9 : 6 * a, Sk = asymmetric ? weight + 2.8 : 6 * a;
    const auto& stateK = asymmetric ? k : i;
    const auto Jk = Action(2, stateK, dw);
    es4::JacobianCoefficients(weight, edge.s, edge.t, Si, Sk, di, dk);
    for (const bool endI : {true, false}) {
      const auto fp = FrozenFlux(edge, Si, Sk, i, stateK, h, endI, dw);
      const auto fm = FrozenFlux(edge, Si, Sk, i, stateK, -h, endI, dw);
      for (auto v = 0u; v < 4; ++v) {
        const auto exact = endI ? weight * (edge.s * Si * Ji[v] + edge.t * weight * Jk[v])
                                : -weight * (edge.s * weight * Ji[v] + edge.t * Sk * Jk[v]);
        const auto surrogate = endI ? di * Ji[v] : -dk * Jk[v];
        const auto fd = (fp[v] - fm[v]) / (2 * h);
        CHECK(fd == Approx(exact));
        if (!asymmetric || !endI) CHECK(fd == Approx(surrogate));
        else CHECK(fd - surrogate == Approx(weight * weight * (Jk[v] - Ji[v])).margin(1e-14));
      }
      if (asymmetric && endI) CHECK((fp[0] - fm[0]) / (2 * h) != Approx(di * Ji[0]));
    }
  }
}

TEST_CASE("U10 ES4 split coefficient and legacy second difference", "[ES4][JSTEntropyFourth]") {
  constexpr auto roundoff = 8 * std::numeric_limits<su2double>::epsilon();
  std::mt19937 rng(72);
  std::uniform_real_distribution<double> sample(0.1, 0.9);
  const double normal[] = {0.14, -0.325};
  for (auto n = 0u; n < 64; ++n) {
    jst::Node i, k;
    jst::Params par;
    par.weakWall = true;
    i.wall = (n % 5 == 0);
    i.sensor = 0.02 * sample(rng);
    k.sensor = 0.03 * sample(rng);
    i.lambda = sample(rng);
    k.lambda = sample(rng);
    i.soundSpeed = 1.2;
    k.soundSpeed = 0.9;
    for (auto v = 0u; v < 4; ++v) {
      i.diss[v] = sample(rng);
      k.diss[v] = sample(rng);
      i.lapl[v] = sample(rng);
      k.lapl[v] = sample(rng);
    }
    const auto share = (n % 2 == 0) ? 0.25 : 0.5;
    const auto radius = jst::SpectralRadius(2, i, k, normal);
    const auto lambda = jst::CorrectedRadius(radius, i, k) * jst::ShareFactor(share);
    REQUIRE(std::isfinite(lambda));
    const auto eps = jst::DissipationCoefficients(par, i, k);
    CHECK(jst::FourthCoefficient(par, i, k, radius, share) == eps.eps4 * lambda);
    for (const bool legacy : {true, false}) {
      CAPTURE(n, legacy);
      Row flux{};
      su2double ji[5][5]{}, jk[5][5]{}, ei[5][5]{}, ek[5][5]{};
      if (legacy) jst::AddPairDissipation(2, par, i, k, normal, flux.data(), true, ji, jk, share);
      else jst::AddPairDissipation(2, par, i, k, normal, flux.data(), true, ji, jk, share, nullptr, false);
      for (auto v = 0u; v < 4; ++v) {
        CAPTURE(v);
        const auto second = eps.eps2 * (i.diss[v] - k.diss[v]);
        const auto fourth = legacy ? eps.eps4 * (i.lapl[v] - k.lapl[v]) : 0.0;
        const auto expected = (second - fourth) * lambda;
        const auto scale = std::abs(second * lambda) + std::abs(fourth * lambda);
        CHECK(std::abs(flux[v] - expected) <= roundoff * scale);
      }
      const auto shift = [&](const jst::Node& node) {
        return legacy ? eps.eps2Jac + eps.eps4 * (node.nNeighbor + share) : eps.eps2Jac;
      };
      jst::AddSurrogateJacobian(2, par.gamma, i.vel, par.fixFactor * shift(i) * lambda, ei);
      jst::AddSurrogateJacobian(2, par.gamma, k.vel, -par.fixFactor * shift(k) * lambda, ek);
      for (auto row = 0u; row < 5; ++row) {
        for (auto col = 0u; col < 5; ++col) {
          CAPTURE(row, col);
          CHECK(std::abs(ji[row][col] - ei[row][col]) <= roundoff * std::abs(ei[row][col]));
          CHECK(std::abs(jk[row][col] - ek[row][col]) <= roundoff * std::abs(ek[row][col]));
        }
      }
    }
  }
}
