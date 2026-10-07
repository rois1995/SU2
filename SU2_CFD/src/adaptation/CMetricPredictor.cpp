/*!
 * \file CMetricPredictor.cpp
 * \brief Prediction of the adaptation metric over the next time window (optical flow on the mesh, transport,
 *        intersection).
 * \version 8.5.0 "Harrier"
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

#include "../../include/adaptation/CMetricPredictor.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>
#include <utility>

#include "../../../Common/include/geometry/CGeometry.hpp"
#include "../../../Common/include/adaptation/CMeshGather.hpp"
#include "../../../Common/include/linear_algebra/blas_structure.hpp"
#include "../../include/adaptation/CBarycentricTransfer.hpp"
#include "../../include/solvers/CSolver.hpp"

namespace {

using Mat3 = su2double[3][3];

/*--- Parameters of the optical flow (see the class note). ---*/
constexpr passivedouble kSmoothingLevels[] = {2.0, 1.0, 0.5, 0.0}; /*!< Heat-kernel lengths in units of Ls. */
constexpr int kOuterIterations = 3;      /*!< Gauss-Newton (warping) iterations per level. */
constexpr passivedouble kMuRelative = 0.1;  /*!< mu^2 / (99th percentile of |grad s|^2) of the level. */
constexpr passivedouble kDecay = 1e-4;   /*!< eps: the motion decays to zero over about l / sqrt(eps) without data. */
constexpr passivedouble kPercentile = 0.99; /*!< Percentile of |grad s|^2 for Ls and mu. */
constexpr passivedouble kFlowTolerance = 1e-3, kSmoothTolerance = 1e-4; /*!< Relative CG tolerances (the
                                                                                warping iterations correct). */
constexpr unsigned long kMaxCG = 5000;
constexpr passivedouble kWarpTolerance = 0.01; /*!< Largest update of the warping, in units of Ls, to stop. */

/*--- Value at a fraction (0..1) of the sorted values (nearest rank). ---*/
su2double Quantile(std::vector<su2double> values, passivedouble fraction) {
  if (values.empty()) return 0.0;
  const auto k = static_cast<size_t>(fraction * (values.size() - 1) + 0.5);
  std::nth_element(values.begin(), values.begin() + k, values.end());
  return values[k];
}

/*--- Median of values with weights. ---*/
su2double WeightedMedian(const std::vector<su2double>& values, const std::vector<su2double>& weights) {
  std::vector<std::pair<su2double, su2double>> pairs(values.size());
  for (size_t i = 0; i < values.size(); ++i) pairs[i] = {values[i], weights[i]};
  std::sort(pairs.begin(), pairs.end(),
            [](const std::pair<su2double, su2double>& a, const std::pair<su2double, su2double>& b) {
              return a.first < b.first;
            });
  su2double total = 0.0;
  for (const auto& p : pairs) total += p.second;
  su2double sum = 0.0;
  for (const auto& p : pairs) {
    sum += p.second;
    if (sum >= 0.5 * total) return p.first;
  }
  return pairs.empty() ? su2double(0.0) : pairs.back().first;
}

/*--- Symmetric matrix from its upper triangle and back. ---*/
void Unpack(unsigned short nDim, const su2double* row, Mat3& M) {
  for (unsigned short i = 0, k = 0; i < nDim; ++i)
    for (unsigned short j = i; j < nDim; ++j, ++k) M[i][j] = M[j][i] = row[k];
}
void Pack(unsigned short nDim, const Mat3& M, su2double* row) {
  for (unsigned short i = 0, k = 0; i < nDim; ++i)
    for (unsigned short j = i; j < nDim; ++j, ++k) row[k] = 0.5 * (M[i][j] + M[j][i]);
}

void MatMul(unsigned short n, const Mat3& A, const Mat3& B, Mat3& C) {
  Mat3 T;
  for (unsigned short i = 0; i < n; ++i)
    for (unsigned short j = 0; j < n; ++j) {
      T[i][j] = 0.0;
      for (unsigned short k = 0; k < n; ++k) T[i][j] += A[i][k] * B[k][j];
    }
  for (unsigned short i = 0; i < n; ++i)
    for (unsigned short j = 0; j < n; ++j) C[i][j] = T[i][j];
}

/*--- Inverse of a small matrix (adjugate), returns the determinant. ---*/
su2double Inverse(unsigned short n, const Mat3& A, Mat3& Ainv) {
  if (n == 2) {
    const su2double det = A[0][0] * A[1][1] - A[0][1] * A[1][0];
    Ainv[0][0] = A[1][1] / det;
    Ainv[0][1] = -A[0][1] / det;
    Ainv[1][0] = -A[1][0] / det;
    Ainv[1][1] = A[0][0] / det;
    return det;
  }
  const su2double c00 = A[1][1] * A[2][2] - A[1][2] * A[2][1];
  const su2double c01 = A[1][2] * A[2][0] - A[1][0] * A[2][2];
  const su2double c02 = A[1][0] * A[2][1] - A[1][1] * A[2][0];
  const su2double det = A[0][0] * c00 + A[0][1] * c01 + A[0][2] * c02;
  Ainv[0][0] = c00 / det;
  Ainv[1][0] = c01 / det;
  Ainv[2][0] = c02 / det;
  Ainv[0][1] = (A[0][2] * A[2][1] - A[0][1] * A[2][2]) / det;
  Ainv[1][1] = (A[0][0] * A[2][2] - A[0][2] * A[2][0]) / det;
  Ainv[2][1] = (A[0][1] * A[2][0] - A[0][0] * A[2][1]) / det;
  Ainv[0][2] = (A[0][1] * A[1][2] - A[0][2] * A[1][1]) / det;
  Ainv[1][2] = (A[0][2] * A[1][0] - A[0][0] * A[1][2]) / det;
  Ainv[2][2] = (A[0][0] * A[1][1] - A[0][1] * A[1][0]) / det;
  return det;
}

/*--- Matrix exponential (scaling and squaring, Taylor series to 12th order). ---*/
void Exponential(unsigned short n, const Mat3& A, Mat3& E) {
  su2double norm = 0.0;
  for (unsigned short i = 0; i < n; ++i)
    for (unsigned short j = 0; j < n; ++j) norm += fabs(A[i][j]);
  int squarings = 0;
  while (norm > 0.5 && squarings < 60) {
    norm *= 0.5;
    ++squarings;
  }
  const su2double scale = pow(0.5, squarings);
  Mat3 B, term;
  for (unsigned short i = 0; i < n; ++i)
    for (unsigned short j = 0; j < n; ++j) {
      B[i][j] = scale * A[i][j];
      E[i][j] = term[i][j] = (i == j) ? 1.0 : 0.0;
    }
  for (int k = 1; k <= 12; ++k) {
    MatMul(n, term, B, term);
    for (unsigned short i = 0; i < n; ++i)
      for (unsigned short j = 0; j < n; ++j) {
        term[i][j] /= k;
        E[i][j] += term[i][j];
      }
  }
  for (int s = 0; s < squarings; ++s) MatMul(n, E, E, E);
}

/*--- Preconditioned conjugate gradients for an SPD operator. ---*/
template <class Op, class Prec>
unsigned long ConjugateGradient(const Op& A, const Prec& P, const std::vector<su2double>& b, std::vector<su2double>& x,
                                passivedouble tolerance, bool& converged) {
  const auto n = b.size();
  std::vector<su2double> r(n), z(n), p(n), q(n);
  A(x, q);
  su2double bNorm = 0.0, rNorm = 0.0;
  for (size_t i = 0; i < n; ++i) {
    r[i] = b[i] - q[i];
    bNorm += b[i] * b[i];
    rNorm += r[i] * r[i];
  }
  bNorm = sqrt(bNorm);
  converged = true;
  if (bNorm == 0.0) {
    std::fill(x.begin(), x.end(), 0.0);
    return 0;
  }
  if (sqrt(rNorm) <= tolerance * bNorm) return 0;
  P(r, z);
  p = z;
  su2double rz = std::inner_product(r.begin(), r.end(), z.begin(), su2double(0.0));
  for (unsigned long it = 1; it <= kMaxCG; ++it) {
    A(p, q);
    const su2double alpha = rz / std::inner_product(p.begin(), p.end(), q.begin(), su2double(0.0));
    rNorm = 0.0;
    for (size_t i = 0; i < n; ++i) {
      x[i] += alpha * p[i];
      r[i] -= alpha * q[i];
      rNorm += r[i] * r[i];
    }
    if (sqrt(rNorm) <= tolerance * bNorm) return it;
    P(r, z);
    const su2double rzNew = std::inner_product(r.begin(), r.end(), z.begin(), su2double(0.0));
    const su2double beta = rzNew / rz;
    rz = rzNew;
    for (size_t i = 0; i < n; ++i) p[i] = z[i] + beta * p[i];
  }
  converged = false;
  return kMaxCG;
}

}  // namespace

namespace {
/*--- The mesh of a geometry without halo points (one rank). ---*/
CSimplexMesh SerialMesh(const CGeometry& geometry) {
  if (geometry.GetnPointDomain() != geometry.GetnPoint()) {
    SU2_MPI::Error("The metric prediction of a geometry works on one rank (mesh without halo points); with MPI it "
                   "works on the mesh gathered on one rank.", CURRENT_FUNCTION);
  }
  return CMeshGather::LocalMesh(geometry, std::vector<std::string>(geometry.GetnMarker(), ""), false);
}
}  // namespace

CMetricPredictor::CMetricPredictor(const CGeometry& geometry) : CMetricPredictor(SerialMesh(geometry)) {}

CMetricPredictor::CMetricPredictor(const CSimplexMesh& mesh) {
  nDim = mesh.nDim;
  nPoint = mesh.GetnPoint();
  nElem = mesh.GetnElem();

  coord.resize(nPoint * nDim);
  for (auto i = 0ul; i < coord.size(); ++i) coord[i] = mesh.coord[i];

  /*--- Element gradients of the hat functions: x = x0 + A lambda', grad lambda_a = row a of A^-1, grad lambda_0 =
   *    -sum of the others; volume |det A| / nDim!. ---*/

  const unsigned short nNode = nDim + 1;
  elemNode.resize(nElem * nNode);
  elemGrad.resize(nElem * nNode * nDim);
  elemVolume.resize(nElem);
  mass.assign(nPoint, 0.0);

  if (mesh.elem.size() != nElem * nNode) {
    SU2_MPI::Error("The metric prediction needs a mesh of triangles (2D) or tetrahedra (3D).", CURRENT_FUNCTION);
  }
  for (auto iElem = 0ul; iElem < nElem; ++iElem) {
    for (unsigned short a = 0; a < nNode; ++a) elemNode[iElem * nNode + a] = mesh.elem[iElem * nNode + a];
    const auto* x0 = &coord[elemNode[iElem * nNode] * nDim];
    Mat3 A = {{0.0}}, Ainv = {{0.0}};
    for (unsigned short a = 1; a < nNode; ++a)
      for (unsigned short iDim = 0; iDim < nDim; ++iDim)
        A[iDim][a - 1] = coord[elemNode[iElem * nNode + a] * nDim + iDim] - x0[iDim];
    const su2double det = Inverse(nDim, A, Ainv);
    if (!(fabs(det) > 0.0)) SU2_MPI::Error("Degenerate element in the metric prediction.", CURRENT_FUNCTION);
    elemVolume[iElem] = fabs(det) / (nDim == 2 ? 2.0 : 6.0);
    auto* grad = &elemGrad[iElem * nNode * nDim];
    for (unsigned short iDim = 0; iDim < nDim; ++iDim) grad[iDim] = 0.0;
    for (unsigned short a = 1; a < nNode; ++a)
      for (unsigned short iDim = 0; iDim < nDim; ++iDim) {
        grad[a * nDim + iDim] = Ainv[a - 1][iDim];
        grad[iDim] -= Ainv[a - 1][iDim];
      }
    for (unsigned short a = 0; a < nNode; ++a) mass[elemNode[iElem * nNode + a]] += elemVolume[iElem] / nNode;
  }

  /*--- Node graph (all point pairs of an element, and the diagonal) and the P1 stiffness matrix on it. ---*/

  std::vector<std::vector<unsigned long>> rows(nPoint);
  for (auto iElem = 0ul; iElem < nElem; ++iElem)
    for (unsigned short a = 0; a < nNode; ++a)
      for (unsigned short b = 0; b < nNode; ++b) rows[elemNode[iElem * nNode + a]].push_back(elemNode[iElem * nNode + b]);
  rowPtr.assign(nPoint + 1, 0);
  for (auto iPoint = 0ul; iPoint < nPoint; ++iPoint) {
    rows[iPoint].push_back(iPoint);
    std::sort(rows[iPoint].begin(), rows[iPoint].end());
    rows[iPoint].erase(std::unique(rows[iPoint].begin(), rows[iPoint].end()), rows[iPoint].end());
    rowPtr[iPoint + 1] = rowPtr[iPoint] + rows[iPoint].size();
  }
  colInd.reserve(rowPtr[nPoint]);
  diagPos.resize(nPoint);
  for (auto iPoint = 0ul; iPoint < nPoint; ++iPoint) {
    for (const auto j : rows[iPoint]) {
      if (j == iPoint) diagPos[iPoint] = colInd.size();
      colInd.push_back(j);
    }
  }
  stiffness.assign(colInd.size(), 0.0);
  for (auto iElem = 0ul; iElem < nElem; ++iElem) {
    const auto* grad = &elemGrad[iElem * nNode * nDim];
    for (unsigned short a = 0; a < nNode; ++a) {
      const auto i = elemNode[iElem * nNode + a];
      for (unsigned short b = 0; b < nNode; ++b) {
        const auto j = elemNode[iElem * nNode + b];
        su2double dot = 0.0;
        for (unsigned short iDim = 0; iDim < nDim; ++iDim) dot += grad[a * nDim + iDim] * grad[b * nDim + iDim];
        const auto pos = std::lower_bound(colInd.data() + rowPtr[i], colInd.data() + rowPtr[i + 1], j) - colInd.data();
        stiffness[pos] += elemVolume[iElem] * dot;
      }
    }
  }

  locator = std::make_unique<CBarycentricLocator>(mesh);
}

CMetricPredictor::~CMetricPredictor() = default;

su2double CMetricPredictor::Invariant(unsigned short nDim, const su2double* metric) {
  Mat3 M = {{0.0}};
  Unpack(nDim, metric, M);

  /*--- log det M = sum log M_ii + log det S, S = D^-1/2 M D^-1/2 (unit diagonal), det S from its Cholesky factor.
   *    The expanded determinant cancels for anisotropic metrics that are not aligned with the axes (for
   *    I + 1e10 ones(3,3) it is negative); the scaled factorization keeps the small eigenvalues (as the
   *    positive-definiteness check of the remesher, CMMGInterface::IsFinitePositiveDefinite). A metric that is not
   *    positive definite gets the floor of the invariant. ---*/
  constexpr passivedouble floorLog10 = -300.0;
  su2double logDet = 0.0;
  su2double scale[3];
  for (unsigned short i = 0; i < nDim; ++i) {
    if (!(M[i][i] > 0.0)) return 0.5 * floorLog10;
    scale[i] = 1.0 / sqrt(M[i][i]);
    logDet += log10(M[i][i]);
  }
  Mat3 L = {{0.0}};
  for (unsigned short j = 0; j < nDim; ++j) {
    su2double pivot = 1.0;
    for (unsigned short k = 0; k < j; ++k) pivot -= L[j][k] * L[j][k];
    if (!(pivot > 0.0)) return 0.5 * floorLog10;
    L[j][j] = sqrt(pivot);
    logDet += log10(pivot);
    for (unsigned short i = j + 1; i < nDim; ++i) {
      su2double sum = M[i][j] * scale[i] * scale[j];
      for (unsigned short k = 0; k < j; ++k) sum -= L[i][k] * L[j][k];
      L[i][j] = sum / L[j][j];
    }
  }
  return 0.5 * fmax(logDet, su2double(floorLog10));
}

CMetricPredictor::Sample CMetricPredictor::Locate(const su2double* x) {
  const auto stencil = locator->Locate(x);
  Sample sample;
  sample.nPoint = stencil.nPoint;
  sample.inside = stencil.inside;
  for (unsigned short k = 0; k < stencil.nPoint; ++k) {
    sample.point[k] = stencil.point[k];
    sample.weight[k] = stencil.weight[k];
  }
  return sample;
}

void CMetricPredictor::MultiplyStiffness(const std::vector<su2double>& x, unsigned short nComp,
                                         std::vector<su2double>& y) const {
  y.assign(nPoint * nComp, 0.0);
  for (auto i = 0ul; i < nPoint; ++i)
    for (auto pos = rowPtr[i]; pos < rowPtr[i + 1]; ++pos) {
      const auto j = colInd[pos];
      for (unsigned short c = 0; c < nComp; ++c) y[i * nComp + c] += stiffness[pos] * x[j * nComp + c];
    }
}

std::vector<su2double> CMetricPredictor::Gradient(const std::vector<su2double>& field, unsigned short stride,
                                                  unsigned short component) const {
  const unsigned short nNode = nDim + 1;
  std::vector<su2double> grad(nPoint * nDim, 0.0);
  for (auto iElem = 0ul; iElem < nElem; ++iElem) {
    const auto* g = &elemGrad[iElem * nNode * nDim];
    su2double ge[3] = {0.0, 0.0, 0.0};
    for (unsigned short a = 0; a < nNode; ++a) {
      const su2double value = field[elemNode[iElem * nNode + a] * stride + component];
      for (unsigned short iDim = 0; iDim < nDim; ++iDim) ge[iDim] += value * g[a * nDim + iDim];
    }
    const su2double weight = elemVolume[iElem] / nNode;
    for (unsigned short a = 0; a < nNode; ++a)
      for (unsigned short iDim = 0; iDim < nDim; ++iDim)
        grad[elemNode[iElem * nNode + a] * nDim + iDim] += weight * ge[iDim];
  }
  for (auto iPoint = 0ul; iPoint < nPoint; ++iPoint)
    for (unsigned short iDim = 0; iDim < nDim; ++iDim) grad[iPoint * nDim + iDim] /= mass[iPoint];
  return grad;
}

std::vector<su2double> CMetricPredictor::Smooth(const std::vector<su2double>& field, su2double sigma,
                                                unsigned long* iterations, bool* converged) const {
  std::vector<su2double> u = field;
  if (!(sigma > 0.0)) return u;
  const su2double tau = 0.5 * sigma * sigma;
  std::vector<su2double> b(nPoint);
  for (auto i = 0ul; i < nPoint; ++i) b[i] = mass[i] * field[i];
  auto A = [&](const std::vector<su2double>& x, std::vector<su2double>& y) {
    MultiplyStiffness(x, 1, y);
    for (auto i = 0ul; i < nPoint; ++i) y[i] = mass[i] * x[i] + tau * y[i];
  };
  auto P = [&](const std::vector<su2double>& r, std::vector<su2double>& z) {
    z.resize(nPoint);
    for (auto i = 0ul; i < nPoint; ++i) z[i] = r[i] / (mass[i] + tau * stiffness[diagPos[i]]);
  };
  bool ok = true;
  const auto its = ConjugateGradient(A, P, b, u, kSmoothTolerance, ok);
  if (iterations) *iterations += its;
  if (converged) *converged = *converged && ok;
  return u;
}

void CMetricPredictor::SolveFlow(const std::vector<FlowLevel>& levels, size_t first, size_t last,
                                 su2double featureLength, su2double smoothLength, std::vector<su2double>& D,
                                 MotionReport& report) {
  const su2double ell2 = smoothLength * smoothLength;
  std::vector<su2double> J(nPoint * nDim * nDim), rhs(nPoint * nDim), dD(nPoint * nDim), KD;

  auto A = [&](const std::vector<su2double>& x, std::vector<su2double>& y) {
    MultiplyStiffness(x, nDim, y);
    for (auto i = 0ul; i < nPoint; ++i)
      for (unsigned short a = 0; a < nDim; ++a) {
        su2double sum = ell2 * y[i * nDim + a] + kDecay * mass[i] * x[i * nDim + a];
        for (unsigned short b = 0; b < nDim; ++b) sum += J[(i * nDim + a) * nDim + b] * x[i * nDim + b];
        y[i * nDim + a] = fixedPoint[i] ? x[i * nDim + a] : sum;
      }
  };
  /*--- Block Jacobi: the nDim x nDim diagonal block of each point. ---*/
  std::vector<su2double> blockInv(nPoint * nDim * nDim);
  auto P = [&](const std::vector<su2double>& r, std::vector<su2double>& z) {
    z.resize(nPoint * nDim);
    for (auto i = 0ul; i < nPoint; ++i)
      for (unsigned short a = 0; a < nDim; ++a) {
        su2double sum = 0.0;
        for (unsigned short b = 0; b < nDim; ++b) sum += blockInv[(i * nDim + a) * nDim + b] * r[i * nDim + b];
        z[i * nDim + a] = sum;
      }
  };

  for (size_t iLevel = first; iLevel < last; ++iLevel) {
    const auto& level = levels[iLevel];
    const auto& sjs = level.sj;
    const auto& sks = level.sk;
    const auto& gj = level.gj;
    const auto& gk = level.gk;
    const su2double mu2 = level.mu2;
    if (!(mu2 > 0.0)) continue;

    /*--- Gauss-Newton with warping. Each iteration samples s_j at x - D; a step is limited so that no point moves by
     *    more than the feature width (the linearization holds over that distance). Points sampled outside the domain
     *    carry no data. ---*/
    for (int it = 0; it < kOuterIterations; ++it) {
      for (auto i = 0ul; i < nPoint; ++i) {
        su2double x[3] = {0.0, 0.0, 0.0}, g[3] = {0.0, 0.0, 0.0}, gg = 0.0;
        for (unsigned short iDim = 0; iDim < nDim; ++iDim) x[iDim] = coord[i * nDim + iDim] - D[i * nDim + iDim];
        const auto sample = Locate(x);
        su2double sjw = 0.0;
        for (unsigned short k = 0; k < sample.nPoint; ++k) {
          sjw += sample.weight[k] * sjs[sample.point[k]];
          for (unsigned short iDim = 0; iDim < nDim; ++iDim)
            g[iDim] += 0.5 * sample.weight[k] * gj[sample.point[k] * nDim + iDim];
        }
        for (unsigned short iDim = 0; iDim < nDim; ++iDim) {
          g[iDim] += 0.5 * gk[i * nDim + iDim];
          gg += g[iDim] * g[iDim];
        }
        const su2double r = sjw - sks[i];
        const su2double w = sample.inside ? mass[i] / (gg + mu2) : su2double(0.0);
        for (unsigned short a = 0; a < nDim; ++a) {
          rhs[i * nDim + a] = w * g[a] * r;
          for (unsigned short b = 0; b < nDim; ++b) J[(i * nDim + a) * nDim + b] = w * g[a] * g[b];
        }
      }
      MultiplyStiffness(D, nDim, KD);
      for (auto i = 0ul; i < nPoint; ++i) {
        Mat3 block = {{0.0}}, inv = {{0.0}};
        for (unsigned short a = 0; a < nDim; ++a) {
          rhs[i * nDim + a] -= ell2 * KD[i * nDim + a] + kDecay * mass[i] * D[i * nDim + a];
          for (unsigned short b = 0; b < nDim; ++b) block[a][b] = J[(i * nDim + a) * nDim + b];
          block[a][a] += ell2 * stiffness[diagPos[i]] + kDecay * mass[i];
        }
        Inverse(nDim, block, inv);
        for (unsigned short a = 0; a < nDim; ++a)
          for (unsigned short b = 0; b < nDim; ++b) {
            blockInv[(i * nDim + a) * nDim + b] = fixedPoint[i] ? su2double(a == b) : inv[a][b];
            if (fixedPoint[i]) rhs[i * nDim + a] = 0.0;
          }
      }
      std::fill(dD.begin(), dD.end(), 0.0);
      bool ok = true;
      report.linearIterations += ConjugateGradient(A, P, rhs, dD, kFlowTolerance, ok);
      report.converged = report.converged && ok;
      su2double change = 0.0;
      for (auto i = 0ul; i < nPoint; ++i) {
        su2double norm = 0.0;
        for (unsigned short iDim = 0; iDim < nDim; ++iDim) norm += pow(dD[i * nDim + iDim], 2);
        change = fmax(change, sqrt(norm));
      }
      const su2double scale = change > featureLength ? featureLength / change : su2double(1.0);
      for (auto k = 0ul; k < nPoint * nDim; ++k) D[k] += scale * dD[k];
      /*--- Converged on this level when no point moves by more than 1% of the feature width. ---*/
      if (scale * change < kWarpTolerance * featureLength) break;
    }
  }

}

su2double CMetricPredictor::Mismatch(const std::vector<su2double>& sj, const std::vector<su2double>& sk,
                                     const std::vector<su2double>& D) {
  su2double sum = 0.0, volume = 0.0;
  for (auto i = 0ul; i < nPoint; ++i) {
    su2double x[3] = {0.0, 0.0, 0.0};
    for (unsigned short iDim = 0; iDim < nDim; ++iDim) x[iDim] = coord[i * nDim + iDim] - D[i * nDim + iDim];
    const auto sample = Locate(x);
    su2double sjw = 0.0;
    for (unsigned short k = 0; k < sample.nPoint; ++k) sjw += sample.weight[k] * sj[sample.point[k]];
    sum += mass[i] * pow(sjw - sk[i], 2);
    volume += mass[i];
  }
  return sqrt(sum / volume);
}

std::vector<su2double> CMetricPredictor::MotionField(const std::vector<su2double>& sjIn,
                                                     const std::vector<su2double>& sk, su2double separation,
                                                     const std::vector<su2double>* guess, su2double regularization,
                                                     MotionReport& report, const std::vector<bool>* fixed) {
  report = MotionReport();
  std::vector<su2double> motion(nPoint * nDim, 0.0);
  if (!(separation > 0.0)) return motion;
  fixedPoint.assign(nPoint, false);
  if (fixed != nullptr && fixed->size() == nPoint) fixedPoint = *fixed;

  /*--- Remove the global offset of s_k - s_j (the complexity scaling of each snapshot), so that the flow only sees
   *    the motion. ---*/
  std::vector<su2double> diff(nPoint), sj(nPoint);
  for (auto i = 0ul; i < nPoint; ++i) diff[i] = sk[i] - sjIn[i];
  report.offset = WeightedMedian(diff, mass);
  su2double sum = 0.0, volume = 0.0;
  for (auto i = 0ul; i < nPoint; ++i) {
    sj[i] = sjIn[i] + report.offset;
    sum += mass[i] * pow(sj[i] - sk[i], 2);
    volume += mass[i];
  }
  report.mismatchBefore = sqrt(sum / volume);

  /*--- Width of the steepest features: range of s over its gradient (robust: percentiles). ---*/
  std::vector<su2double> both(sj);
  both.insert(both.end(), sk.begin(), sk.end());
  const su2double range = Quantile(both, 0.995) - Quantile(both, 0.005);
  const auto gk = Gradient(sk);
  std::vector<su2double> g2(nPoint, 0.0);
  for (auto i = 0ul; i < nPoint; ++i)
    for (unsigned short iDim = 0; iDim < nDim; ++iDim) g2[i] += pow(gk[i * nDim + iDim], 2);
  const su2double g2Ref = Quantile(g2, kPercentile);
  if (!(range > 0.0) || !(g2Ref > 0.0)) {
    report.mismatchAfter = report.mismatchBefore;
    return motion;
  }
  report.featureLength = range / sqrt(g2Ref);
  report.smoothLength = regularization * report.featureLength;

  /*--- Levels of the coarse-to-fine iteration: both snapshots smoothed, their gradients, mu. ---*/
  std::vector<FlowLevel> levels;
  for (const auto factor : kSmoothingLevels) {
    FlowLevel level;
    const su2double sigma = factor * report.featureLength;
    level.sj = Smooth(sj, sigma, &report.linearIterations, &report.converged);
    level.sk = Smooth(sk, sigma, &report.linearIterations, &report.converged);
    level.gj = Gradient(level.sj);
    level.gk = Gradient(level.sk);
    std::vector<su2double> g2Level(nPoint, 0.0);
    for (auto i = 0ul; i < nPoint; ++i)
      for (unsigned short iDim = 0; iDim < nDim; ++iDim) g2Level[i] += pow(level.gk[i * nDim + iDim], 2);
    level.mu2 = kMuRelative * Quantile(g2Level, kPercentile);
    levels.push_back(std::move(level));
  }

  /*--- Starts: zero, and the guess if given; the one with the smaller final mismatch is kept (the heavily smoothed
   *    snapshots of the coarse levels do not tell them apart reliably). ---*/
  std::vector<su2double> D(nPoint * nDim, 0.0);
  SolveFlow(levels, 0, levels.size(), report.featureLength, report.smoothLength, D, report);
  report.mismatchAfter = Mismatch(sj, sk, D);
  if (guess != nullptr && guess->size() == nPoint * nDim) {
    std::vector<su2double> Dg(nPoint * nDim);
    for (auto k = 0ul; k < nPoint * nDim; ++k) Dg[k] = fixedPoint[k / nDim] ? su2double(0.0) : (*guess)[k] * separation;
    SolveFlow(levels, 0, levels.size(), report.featureLength, report.smoothLength, Dg, report);
    const su2double mismatch = Mismatch(sj, sk, Dg);
    report.guessKept = mismatch < report.mismatchAfter;
    report.mismatchOther = report.guessKept ? report.mismatchAfter : mismatch;
    if (report.guessKept) {
      report.mismatchAfter = mismatch;
      D = std::move(Dg);
    }
  }

  /*--- The motion must explain the change better than no motion at all, else there is none (e.g. features that
   *    appear or vanish rather than move). ---*/
  if (!(report.mismatchAfter < report.mismatchBefore)) {
    report.noMotion = true;
    report.mismatchAfter = report.mismatchBefore;
    std::fill(D.begin(), D.end(), 0.0);
  }

  su2double weightSum = 0.0, speedSum = 0.0;
  for (auto i = 0ul; i < nPoint; ++i) {
    su2double speed = 0.0;
    for (unsigned short iDim = 0; iDim < nDim; ++iDim) {
      motion[i * nDim + iDim] = D[i * nDim + iDim] / separation;
      speed += pow(motion[i * nDim + iDim], 2);
    }
    speed = sqrt(speed);
    report.maxSpeed = fmax(report.maxSpeed, speed);
    speedSum += mass[i] * g2[i] * speed;
    weightSum += mass[i] * g2[i];
  }
  report.meanSpeed = speedSum / weightSum;
  return motion;
}

std::vector<su2double> CMetricPredictor::Predict(const std::vector<su2double>& metricK,
                                                 const std::vector<su2double>& motion,
                                                 const std::vector<su2double>& instants, const Options& options,
                                                 PredictionReport& report,
                                                 std::vector<std::vector<su2double>>* perInstant) {
  report = PredictionReport();
  const unsigned short nMet = nDim * (nDim + 1) / 2;
  const su2double eigMin = options.hmax > 0.0 ? su2double(1.0 / pow(options.hmax, 2)) : su2double(0.0);
  const su2double eigMax = options.hmin > 0.0 ? su2double(1.0 / pow(options.hmin, 2))
                                              : su2double(std::numeric_limits<passivedouble>::max());

  /*--- Gradient of the motion: gradMotion[(i * nDim + a) * nDim + b] = d W_a / d x_b. ---*/
  std::vector<su2double> gradMotion(nPoint * nDim * nDim);
  for (unsigned short a = 0; a < nDim; ++a) {
    const auto g = Gradient(motion, nDim, a);
    for (auto i = 0ul; i < nPoint; ++i)
      for (unsigned short b = 0; b < nDim; ++b) gradMotion[(i * nDim + a) * nDim + b] = g[i * nDim + b];
  }

  /*--- Backward trajectories: position y of the source of each point, the motion there, F^-1 = dy/dx. ---*/
  std::vector<su2double> y(coord), motionY(motion), Finv(nPoint * 9, 0.0);
  for (auto i = 0ul; i < nPoint; ++i)
    for (unsigned short a = 0; a < nDim; ++a) Finv[i * 9 + a * 3 + a] = 1.0;

  std::vector<su2double> result(nPoint * nMet, 0.0);
  su2double time = 0.0;
  std::vector<bool> outside(nPoint, false);

  for (const auto instant : instants) {
    const su2double dt = instant - time;
    if (dt > 0.0) {
      for (auto i = 0ul; i < nPoint; ++i) {
        su2double xm[3] = {0.0, 0.0, 0.0};
        for (unsigned short a = 0; a < nDim; ++a) xm[a] = y[i * nDim + a] - 0.5 * dt * motionY[i * nDim + a];
        const auto sample = Locate(xm);
        Mat3 G = {{0.0}}, E = {{0.0}}, F = {{0.0}};
        su2double Wm[3] = {0.0, 0.0, 0.0};
        for (unsigned short k = 0; k < sample.nPoint; ++k) {
          const auto j = sample.point[k];
          for (unsigned short a = 0; a < nDim; ++a) {
            Wm[a] += sample.weight[k] * motion[j * nDim + a];
            for (unsigned short b = 0; b < nDim; ++b)
              G[a][b] -= dt * sample.weight[k] * gradMotion[(j * nDim + a) * nDim + b];
          }
        }
        for (unsigned short a = 0; a < nDim; ++a) y[i * nDim + a] -= dt * Wm[a];
        Exponential(nDim, G, E);
        for (unsigned short a = 0; a < nDim; ++a)
          for (unsigned short b = 0; b < nDim; ++b) F[a][b] = Finv[i * 9 + a * 3 + b];
        MatMul(nDim, E, F, F);
        for (unsigned short a = 0; a < nDim; ++a)
          for (unsigned short b = 0; b < nDim; ++b) Finv[i * 9 + a * 3 + b] = F[a][b];
      }
      time = instant;
    }

    std::vector<su2double> current(nPoint * nMet);
    for (auto i = 0ul; i < nPoint; ++i) {
      const auto sample = Locate(&y[i * nDim]);
      outside[i] = !sample.inside;
      su2double row[6] = {0.0};
      for (unsigned short a = 0; a < nDim; ++a) motionY[i * nDim + a] = 0.0;
      for (unsigned short k = 0; k < sample.nPoint; ++k) {
        const auto j = sample.point[k];
        for (unsigned short m = 0; m < nMet; ++m) row[m] += sample.weight[k] * metricK[j * nMet + m];
        for (unsigned short a = 0; a < nDim; ++a) motionY[i * nDim + a] += sample.weight[k] * motion[j * nDim + a];
      }
      Mat3 M = {{0.0}}, vec = {{0.0}};
      su2double val[3] = {0.0}, work[3] = {0.0};
      Unpack(nDim, row, M);
      CBlasStructure::EigenDecomposition(M, vec, val, nDim, work);
      const su2double valMin = *std::min_element(val, val + nDim), valMax = *std::max_element(val, val + nDim);

      /*--- Reorientation (congruence) where anisotropic, else the metric is only moved. ---*/
      if (valMin > 0.0 && sqrt(valMax / valMin) > options.anisoThreshold) {
        Mat3 Fi = {{0.0}}, FiT = {{0.0}};
        for (unsigned short a = 0; a < nDim; ++a)
          for (unsigned short b = 0; b < nDim; ++b) {
            Fi[a][b] = Finv[i * 9 + a * 3 + b];
            FiT[b][a] = Fi[a][b];
          }
        MatMul(nDim, FiT, M, M);
        MatMul(nDim, M, Fi, M);
        for (unsigned short a = 0; a < nDim; ++a)
          for (unsigned short b = a + 1; b < nDim; ++b) M[a][b] = M[b][a] = 0.5 * (M[a][b] + M[b][a]);
        CBlasStructure::EigenDecomposition(M, vec, val, nDim, work);
        ++report.nCongruence;
      }
      for (unsigned short a = 0; a < nDim; ++a) val[a] = fmin(fmax(val[a], eigMin), eigMax);
      CBlasStructure::EigenRecomposition(M, vec, val, nDim);
      Pack(nDim, M, &current[i * nMet]);

      if (report.nInstant == 0) {
        for (unsigned short m = 0; m < nMet; ++m) result[i * nMet + m] = current[i * nMet + m];
      } else {
        Mat3 A = {{0.0}}, C = {{0.0}};
        Unpack(nDim, &result[i * nMet], A);
        CSolver::IntersectMetrics(nDim, A, M, C);
        Pack(nDim, C, &result[i * nMet]);
      }
    }
    if (perInstant) perInstant->push_back(std::move(current));
    ++report.nInstant;
  }
  report.nOutside = std::count(outside.begin(), outside.end(), true);
  return result;
}
