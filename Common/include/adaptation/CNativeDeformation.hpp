/*!
 * \file CNativeDeformation.hpp
 * \brief Dimension-generic (2D triangles / 3D tetrahedra) mesh deformation for the native remeshers: a P1 vector
 *        Laplacian with Jacobian stiffening and sliding constraints solved by projected PCG, and the pseudo-time move
 *        with validity freezing of the closed advancing-layer method (Alauzet, Loseille, Marcum, AIAA 2017-0585, Alg. 2).
 *        Independent of the engines' mesh classes: the caller passes local vertex indices, coordinates and constraints,
 *        and a per-cell validity predicate (its dimension-specific kernel). Header-only, no global state.
 *        Parallel use: the operator is a sparse SPD system (halo CG); freezing is per cell and converges by iteration;
 *        the caller commits per owned cell.
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md).
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */
#pragma once
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <functional>
#include <stdexcept>
#include <vector>

namespace SU2NativeDeform {

template <int D>
using Vec = std::array<double, D>;

/*! Free: solved and moved. Fixed: Dirichlet value (prescribed displacement, 0 = clamped); never moved by the move step.
 *  Slide: solved and moved with the displacement orthogonal to every given unit normal (symmetry planes, lines). */
enum class Constraint : unsigned char { Free = 0, Fixed = 1, Slide = 2 };

template <int D>
struct Problem {
  std::vector<Vec<D>> x;                          // vertex positions
  std::vector<std::array<uint32_t, D + 1>> cells;  // positively oriented simplices (local vertex indices)
  std::vector<Constraint> kind;                   // per vertex
  std::vector<Vec<D>> value;                      // Fixed: prescribed displacement
  std::vector<std::vector<Vec<D>>> normals;       // Slide: orthonormal constraint normals (1..D-1)
  std::vector<double> weight;                     // optional per-cell stiffness multiplier (empty: 1)
};

struct SolveOptions {
  double stiffening = 1;      // w_K = (mean |K| / |K|)^stiffening: small cells move nearly rigidly
  size_t max_iterations = 4000;
  double tolerance = 1e-10;   // relative preconditioned residual
};
struct SolveReport {
  size_t unknowns = 0, iterations = 0;
  double residual0 = 0, residual = 0, seconds = 0;
  bool converged = false;
};

namespace detail {
/*! Gradients of the D+1 P1 basis functions and the simplex measure (signed, from the corner determinant). */
template <int D>
inline bool Gradients(const std::array<Vec<D>, D + 1>& p, std::array<std::array<long double, D>, D + 1>& g,
                      long double& measure) {
  long double J[D][D], inv[D][D];
  for (int i = 0; i < D; ++i)
    for (int j = 0; j < D; ++j) J[i][j] = (long double)p[j + 1][i] - p[0][i];  // columns p_j - p_0
  long double det;
  if (D == 2) {
    det = J[0][0] * J[1][1] - J[0][1] * J[1][0];
    if (!(det != 0)) return false;
    inv[0][0] = J[1][1] / det; inv[0][1] = -J[0][1] / det; inv[1][0] = -J[1][0] / det; inv[1][1] = J[0][0] / det;
  } else {
    det = J[0][0] * (J[1][1] * J[2][2] - J[1][2] * J[2][1]) - J[0][1] * (J[1][0] * J[2][2] - J[1][2] * J[2][0]) +
          J[0][2] * (J[1][0] * J[2][1] - J[1][1] * J[2][0]);
    if (!(det != 0)) return false;
    inv[0][0] = (J[1][1] * J[2][2] - J[1][2] * J[2][1]) / det; inv[0][1] = (J[0][2] * J[2][1] - J[0][1] * J[2][2]) / det;
    inv[0][2] = (J[0][1] * J[1][2] - J[0][2] * J[1][1]) / det; inv[1][0] = (J[1][2] * J[2][0] - J[1][0] * J[2][2]) / det;
    inv[1][1] = (J[0][0] * J[2][2] - J[0][2] * J[2][0]) / det; inv[1][2] = (J[0][2] * J[1][0] - J[0][0] * J[1][2]) / det;
    inv[2][0] = (J[1][0] * J[2][1] - J[1][1] * J[2][0]) / det; inv[2][1] = (J[0][1] * J[2][0] - J[0][0] * J[2][1]) / det;
    inv[2][2] = (J[0][0] * J[1][1] - J[0][1] * J[1][0]) / det;
  }
  // grad phi_{j+1} = row j of J^{-1}; grad phi_0 = -sum
  for (int k = 0; k < D; ++k) g[0][k] = 0;
  for (int j = 0; j < D; ++j)
    for (int k = 0; k < D; ++k) { g[j + 1][k] = inv[j][k]; g[0][k] -= inv[j][k]; }
  measure = det / (D == 2 ? 2 : 6);
  return true;
}
template <int D>
inline void Project(const std::vector<Vec<D>>& normals, Vec<D>& v) {
  for (const auto& n : normals) {
    long double s = 0;
    for (int k = 0; k < D; ++k) s += (long double)v[k] * n[k];
    for (int k = 0; k < D; ++k) v[k] = double(v[k] - s * n[k]);
  }
}
}  // namespace detail

/*! Displacement d of every vertex: d = value on Fixed, solution of the stiffened P1 vector Laplacian elsewhere, with d
 *  orthogonal to the normals of Slide vertices. Throws on malformed input or a degenerate cell. */
template <int D>
inline bool SolveDisplacement(const Problem<D>& p, std::vector<Vec<D>>& d, const SolveOptions& o, SolveReport& r) {
  static_assert(D == 2 || D == 3, "SU2NativeDeform supports 2D and 3D simplices");
  const auto t0 = std::chrono::steady_clock::now();
  const size_t n = p.x.size();
  if (p.kind.size() != n || p.value.size() != n || p.normals.size() != n) throw std::invalid_argument("Deformation problem: per-vertex arrays differ in size.");
  if (!p.weight.empty() && p.weight.size() != p.cells.size()) throw std::invalid_argument("Deformation problem: per-cell weights differ in size.");
  // element matrices, mean measure for the stiffening
  std::vector<std::array<std::array<long double, D>, D + 1>> grad(p.cells.size());
  std::vector<long double> meas(p.cells.size());
  long double mean = 0;
  for (size_t c = 0; c < p.cells.size(); ++c) {
    std::array<Vec<D>, D + 1> q;
    for (int k = 0; k <= D; ++k) { const auto v = p.cells[c][k]; if (v >= n) throw std::invalid_argument("Deformation problem: cell index out of range."); q[k] = p.x[v]; }
    if (!detail::Gradients<D>(q, grad[c], meas[c])) throw std::invalid_argument("Deformation problem: degenerate cell.");
    meas[c] = std::abs(meas[c]);
    mean += meas[c];
  }
  if (!p.cells.empty()) mean /= p.cells.size();
  // scalar stiffness K (CSR), operator K (x) I_D
  std::vector<std::vector<std::pair<uint32_t, long double>>> rows(n);
  for (size_t c = 0; c < p.cells.size(); ++c) {
    const long double w = (o.stiffening != 0 ? std::pow(mean / meas[c], (long double)o.stiffening) : 1.L) * (p.weight.empty() ? 1.L : p.weight[c]);
    for (int a = 0; a <= D; ++a)
      for (int b = 0; b <= D; ++b) {
        long double s = 0;
        for (int k = 0; k < D; ++k) s += grad[c][a][k] * grad[c][b][k];
        rows[p.cells[c][a]].push_back({p.cells[c][b], w * meas[c] * s});
      }
  }
  std::vector<size_t> start(n + 1, 0);
  std::vector<uint32_t> col;
  std::vector<long double> val, diag(n, 0);
  for (size_t i = 0; i < n; ++i) {
    auto& row = rows[i];
    std::sort(row.begin(), row.end(), [](const std::pair<uint32_t, long double>& a, const std::pair<uint32_t, long double>& b) { return a.first < b.first; });
    for (size_t k = 0; k < row.size();) {
      size_t j = k; long double s = 0;
      while (j < row.size() && row[j].first == row[k].first) s += row[j++].second;
      col.push_back(row[k].first); val.push_back(s);
      if (row[k].first == i) diag[i] = s;
      k = j;
    }
    start[i + 1] = col.size();
    std::vector<std::pair<uint32_t, long double>>().swap(row);
  }
  auto apply = [&](const std::vector<Vec<D>>& u, std::vector<Vec<D>>& out) {
    for (size_t i = 0; i < n; ++i) {
      long double s[D] = {};
      for (size_t k = start[i]; k < start[i + 1]; ++k)
        for (int a = 0; a < D; ++a) s[a] += val[k] * u[col[k]][a];
      for (int a = 0; a < D; ++a) out[i][a] = double(s[a]);
    }
  };
  auto project = [&](std::vector<Vec<D>>& u) {
    for (size_t i = 0; i < n; ++i) {
      if (p.kind[i] == Constraint::Fixed) u[i].fill(0);
      else if (p.kind[i] == Constraint::Slide) detail::Project<D>(p.normals[i], u[i]);
    }
  };
  auto dot = [&](const std::vector<Vec<D>>& a, const std::vector<Vec<D>>& b) {
    long double s = 0;
    for (size_t i = 0; i < n; ++i) for (int k = 0; k < D; ++k) s += (long double)a[i][k] * b[i][k];
    return s;
  };
  d.assign(n, Vec<D>{});
  for (size_t i = 0; i < n; ++i) if (p.kind[i] == Constraint::Fixed) d[i] = p.value[i];
  for (size_t i = 0; i < n; ++i) r.unknowns += p.kind[i] == Constraint::Free ? D : p.kind[i] == Constraint::Slide ? D - p.normals[i].size() : 0;
  std::vector<Vec<D>> res(n), z(n), dir(n), q(n);
  apply(d, res);
  for (auto& v : res) for (auto& c : v) c = -c;
  project(res);
  auto precondition = [&](const std::vector<Vec<D>>& a, std::vector<Vec<D>>& out) {
    for (size_t i = 0; i < n; ++i) { const long double m = diag[i] > 0 ? diag[i] : 1; for (int k = 0; k < D; ++k) out[i][k] = double(a[i][k] / m); }
    project(out);
  };
  precondition(res, z);
  long double rz = dot(res, z);
  r.residual0 = std::sqrt(double(std::max(0.L, rz)));
  r.residual = r.residual0;
  if (!(r.residual0 > 0)) { r.converged = true; r.seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count(); return true; }
  dir = z;
  for (r.iterations = 0; r.iterations < o.max_iterations; ++r.iterations) {
    apply(dir, q);
    project(q);
    const long double pq = dot(dir, q);
    if (!(pq > 0)) break;
    const long double alpha = rz / pq;
    for (size_t i = 0; i < n; ++i) for (int k = 0; k < D; ++k) { d[i][k] = double(d[i][k] + alpha * dir[i][k]); res[i][k] = double(res[i][k] - alpha * q[i][k]); }
    precondition(res, z);
    const long double rz1 = dot(res, z);
    r.residual = std::sqrt(double(std::max(0.L, rz1)));
    if (r.residual <= o.tolerance * r.residual0) { r.converged = true; ++r.iterations; break; }
    const long double beta = rz1 / rz;
    rz = rz1;
    for (size_t i = 0; i < n; ++i) for (int k = 0; k < D; ++k) dir[i][k] = double(z[i][k] + beta * dir[i][k]);
  }
  for (size_t i = 0; i < n; ++i) if (p.kind[i] == Constraint::Slide) detail::Project<D>(p.normals[i], d[i]);
  r.seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
  return r.converged;
}

struct MoveOptions {
  std::vector<double> fractions{.25, .5, .75, 1.};  // pseudo-time stations of the displacement
  size_t max_rounds = 256;                           // freeze rounds per station (then the station is cancelled)
};
struct MoveReport {
  size_t movable = 0, moved = 0, frozen = 0, rounds = 0, checks = 0, failed_checks = 0, cancelled_stations = 0;
  double reached = 0, seconds = 0;  // last fully checked station
};
/*! Caller kernel: is the cell (index into Problem::cells) valid with these corner positions? */
template <int D>
using CellCheck = std::function<bool(size_t, const std::array<Vec<D>, D + 1>&)>;

/*! Pseudo-time move: at each station tau the Free/Slide vertices that are not frozen go to x + tau d; every cell touching a
 *  vertex that changed is checked; the movable vertices of a failing cell are frozen at their last accepted position and
 *  the cells touching them are re-checked, until no checked cell fails. The input state (y = x) must satisfy `valid`;
 *  every accepted state does. `y` returns the final positions, `frozen` the frozen flags. */
template <int D>
inline void PseudoTimeMove(const Problem<D>& p, const std::vector<Vec<D>>& d, const CellCheck<D>& valid,
                           std::vector<Vec<D>>& y, std::vector<char>& frozen, const MoveOptions& o, MoveReport& r) {
  const auto t0 = std::chrono::steady_clock::now();
  const size_t n = p.x.size();
  if (d.size() != n) throw std::invalid_argument("Deformation move: displacement size differs.");
  y = p.x;
  frozen.assign(n, 0);
  std::vector<std::vector<uint32_t>> star(n);
  for (size_t c = 0; c < p.cells.size(); ++c) for (auto v : p.cells[c]) star[v].push_back(uint32_t(c));
  std::vector<char> movable(n, 0);
  for (size_t i = 0; i < n; ++i) {
    bool nonzero = false;
    for (int k = 0; k < D; ++k) nonzero = nonzero || d[i][k] != 0;
    movable[i] = p.kind[i] != Constraint::Fixed && nonzero;
    r.movable += movable[i];
  }
  std::vector<uint32_t> mark(p.cells.size(), 0);
  uint32_t stamp = 0;
  auto corners = [&](const std::vector<Vec<D>>& at, size_t c) { std::array<Vec<D>, D + 1> q; for (int k = 0; k <= D; ++k) q[k] = at[p.cells[c][k]]; return q; };
  for (double tau : o.fractions) {
    std::vector<Vec<D>> z = y;
    std::vector<uint32_t> changed;
    for (size_t i = 0; i < n; ++i)
      if (movable[i] && !frozen[i]) {
        Vec<D> to;
        for (int k = 0; k < D; ++k) to[k] = double((long double)p.x[i][k] + (long double)tau * d[i][k]);
        if (to != y[i]) { z[i] = to; changed.push_back(uint32_t(i)); }
      }
    bool done = false;
    std::vector<uint32_t> newly;
    for (size_t round = 0; round < o.max_rounds && !done; ++round) {
      ++r.rounds;
      ++stamp;
      std::vector<uint32_t> failing;
      for (auto v : changed)
        for (auto c : star[v]) {
          if (mark[c] == stamp) continue;
          mark[c] = stamp;
          ++r.checks;
          if (!valid(c, corners(z, c))) failing.push_back(c);
        }
      if (failing.empty()) { done = true; break; }
      r.failed_checks += failing.size();
      changed.clear();
      for (auto c : failing)
        for (auto v : p.cells[c])
          if (movable[v] && !frozen[v]) { frozen[v] = 1; newly.push_back(v); if (z[v] != y[v]) { z[v] = y[v]; changed.push_back(v); } }
      if (changed.empty()) break;  // a failing cell with no movable vertex left: cannot happen from a valid state
    }
    if (!done) {  // cancel the station: keep the last accepted state, undo its freezes
      ++r.cancelled_stations;
      for (auto v : newly) frozen[v] = 0;
      break;
    }
    y.swap(z);
    r.reached = tau;
  }
  for (size_t i = 0; i < n; ++i) { r.frozen += frozen[i]; r.moved += y[i] != p.x[i]; }
  r.seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
}

}  // namespace SU2NativeDeform
