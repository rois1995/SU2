/*!
 * \file ConservativeKernels.cpp
 * \brief Kernels of the conservative P1 projection shared by the serial and the distributed projection.
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

#include "../../include/adaptation/ConservativeKernels.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

#include "../../../Common/include/adaptation/CDistributedSearch.hpp"
#include "../../../Common/include/geometry/CGeometry.hpp"
#include "../../../Common/include/parallelization/CPassiveComm.hpp"
#include "../../include/adaptation/ConvexClipping.hpp"

namespace conservative {

namespace {

constexpr passivedouble kInf = std::numeric_limits<passivedouble>::infinity();

/*--- Convex polygon (2D) or polyhedron (3D). ---*/
class Poly {
 public:
  explicit Poly(unsigned short nDim) : nDim(nDim) {}

  void InitSimplex(const passivedouble (*y)[3]) {
    if (nDim == 2) {
      p2.InitTriangle(y[0], y[1], y[2]);
    } else {
      const passivedouble* p[4] = {y[0], y[1], y[2], y[3]};
      p3.InitTetrahedron(p);
    }
  }

  /*--- Keep g.x + d >= 0. ---*/
  void Clip(const passivedouble* g, passivedouble d) {
    const int n = (nDim == 2) ? p2.Clip(g[0], g[1], d) : p3.Clip(g[0], g[1], g[2], d);
    if (n < 0) SU2_MPI::Error("Vertex capacity of the polytope clipping exceeded.", CURRENT_FUNCTION);
  }

  /*--- Copy of the vertices in use only (the polytopes have fixed capacities). ---*/
  void CopyFrom(const Poly& other) {
    if (nDim == 2) {
      p2.n = other.p2.n;
      std::copy(&other.p2.x[0][0], &other.p2.x[0][0] + 2 * std::max(other.p2.n, 0), &p2.x[0][0]);
    } else {
      p3.n = other.p3.n;
      std::copy(&other.p3.x[0][0], &other.p3.x[0][0] + 3 * std::max(other.p3.n, 0), &p3.x[0][0]);
      std::copy(&other.p3.nbr[0][0], &other.p3.nbr[0][0] + 3 * std::max(other.p3.n, 0), &p3.nbr[0][0]);
    }
  }

  int Size() const { return nDim == 2 ? p2.n : p3.n; }
  bool Empty() const { return Size() < nDim + 1; }
  const passivedouble* Vertex(int v) const { return nDim == 2 ? p2.x[v] : p3.x[v]; }

  void Moments(passivedouble& volume, passivedouble* centroid) const {
    centroid[2] = 0.0;
    if (nDim == 2) {
      p2.Moments(volume, centroid);
    } else {
      p3.Moments(volume, centroid);
    }
  }

 private:
  unsigned short nDim;
  convex_clip::Polygon p2;
  convex_clip::Polyhedron p3;
};

}  // namespace

passivedouble SimplexPlanes(unsigned short nDim, const passivedouble (*y)[3], passivedouble (*G)[3], passivedouble* a) {
  passivedouble E[3][3] = {}, inv[3][3] = {}, det = 0.0;
  for (unsigned short c = 0; c < nDim; ++c)
    for (unsigned short r = 0; r < nDim; ++r) E[r][c] = y[c + 1][r] - y[0][r];
  if (nDim == 2) {
    det = E[0][0] * E[1][1] - E[0][1] * E[1][0];
    if (det == 0.0) return 0.0;
    inv[0][0] = E[1][1] / det;
    inv[0][1] = -E[0][1] / det;
    inv[1][0] = -E[1][0] / det;
    inv[1][1] = E[0][0] / det;
  } else {
    const passivedouble c00 = E[1][1] * E[2][2] - E[1][2] * E[2][1];
    const passivedouble c01 = -(E[1][0] * E[2][2] - E[1][2] * E[2][0]);
    const passivedouble c02 = E[1][0] * E[2][1] - E[1][1] * E[2][0];
    det = E[0][0] * c00 + E[0][1] * c01 + E[0][2] * c02;
    if (det == 0.0) return 0.0;
    const passivedouble c10 = -(E[0][1] * E[2][2] - E[0][2] * E[2][1]);
    const passivedouble c11 = E[0][0] * E[2][2] - E[0][2] * E[2][0];
    const passivedouble c12 = -(E[0][0] * E[2][1] - E[0][1] * E[2][0]);
    const passivedouble c20 = E[0][1] * E[1][2] - E[0][2] * E[1][1];
    const passivedouble c21 = -(E[0][0] * E[1][2] - E[0][2] * E[1][0]);
    const passivedouble c22 = E[0][0] * E[1][1] - E[0][1] * E[1][0];
    /*--- Inverse = adjugate / det, adjugate = transposed cofactors. ---*/
    const passivedouble cof[3][3] = {{c00, c01, c02}, {c10, c11, c12}, {c20, c21, c22}};
    for (int i = 0; i < 3; ++i)
      for (int j = 0; j < 3; ++j) inv[i][j] = cof[j][i] / det;
  }
  passivedouble sumA = 0.0;
  for (unsigned short k = 1; k <= nDim; ++k) {
    a[k] = 0.0;
    for (unsigned short r = 0; r < nDim; ++r) {
      G[k][r] = inv[k - 1][r];
      a[k] -= G[k][r] * y[0][r];
    }
    sumA += a[k];
  }
  for (unsigned short r = 0; r < nDim; ++r) {
    G[0][r] = 0.0;
    for (unsigned short k = 1; k <= nDim; ++k) G[0][r] -= G[k][r];
  }
  a[0] = 1.0 - sumA;
  return det;
}

passivedouble SimplexMeasure(unsigned short nDim, const passivedouble* const* nodes) {
  passivedouble y[4][3] = {}, G[4][3], a[4];
  for (unsigned short k = 0; k <= nDim; ++k)
    for (unsigned short iDim = 0; iDim < nDim; ++iDim) y[k][iDim] = nodes[k][iDim] - nodes[0][iDim];
  return std::fabs(SimplexPlanes(nDim, y, G, a)) / Factorial(nDim);
}

void SetFrame(unsigned short nDim, const passivedouble* const* nodes, Frame& frame) {
  const unsigned short nNode = nDim + 1;
  for (unsigned short iDim = 0; iDim < 3; ++iDim) frame.origin[iDim] = (iDim < nDim) ? nodes[0][iDim] : 0.0;
  for (unsigned short k = 0; k < nNode; ++k)
    for (unsigned short iDim = 0; iDim < 3; ++iDim)
      frame.y[k][iDim] = (iDim < nDim) ? nodes[k][iDim] - frame.origin[iDim] : 0.0;
  frame.volume = std::fabs(SimplexPlanes(nDim, frame.y, frame.G, frame.a)) / Factorial(nDim);
  for (unsigned short iDim = 0; iDim < nDim; ++iDim) {
    frame.bbMin[iDim] = kInf;
    frame.bbMax[iDim] = -kInf;
    for (unsigned short k = 0; k < nNode; ++k) {
      frame.bbMin[iDim] = std::min(frame.bbMin[iDim], frame.y[k][iDim]);
      frame.bbMax[iDim] = std::max(frame.bbMax[iDim], frame.y[k][iDim]);
    }
  }
}

bool Overlap(unsigned short nDim, const Frame& frame, const passivedouble* const* donorNodes, passivedouble donorVolume,
             PieceFunction piece, void* context, bool* clipped) {
  const unsigned short nNode = nDim + 1;
  *clipped = false;

  /*--- Donor simplex in the frame of the target element; bounding boxes. ---*/
  passivedouble z[4][3] = {};
  for (unsigned short k = 0; k < nNode; ++k)
    for (unsigned short iDim = 0; iDim < nDim; ++iDim) z[k][iDim] = donorNodes[k][iDim] - frame.origin[iDim];
  for (unsigned short iDim = 0; iDim < nDim; ++iDim) {
    passivedouble zMin = kInf, zMax = -kInf;
    for (unsigned short k = 0; k < nNode; ++k) {
      zMin = std::min(zMin, z[k][iDim]);
      zMax = std::max(zMax, z[k][iDim]);
    }
    if (zMax < frame.bbMin[iDim] || zMin > frame.bbMax[iDim]) return false;
  }
  *clipped = true;

  /*--- Overlap: the donor simplex clipped by lambda_k >= 0 of the target element. ---*/
  Poly overlap(nDim);
  overlap.InitSimplex(z);
  for (unsigned short k = 0; k < nNode; ++k) {
    overlap.Clip(frame.G[k], frame.a[k]);
    if (overlap.Empty()) return false;
  }
  passivedouble volume0 = 0.0, centroid0[3] = {};
  overlap.Moments(volume0, centroid0);
  if (!(volume0 > kOverlapFraction * std::min(frame.volume, donorVolume))) return false;

  /*--- Donor field in the frame. ---*/
  passivedouble GD[4][3] = {}, aD[4] = {};
  SimplexPlanes(nDim, z, GD, aD);
  auto call = [&](unsigned short i, passivedouble volume, const passivedouble* centroid) {
    passivedouble mu[4] = {};
    for (unsigned short k = 0; k < nNode; ++k) {
      mu[k] = aD[k];
      for (unsigned short iDim = 0; iDim < nDim; ++iDim) mu[k] += GD[k][iDim] * centroid[iDim];
    }
    piece(context, i, volume, centroid, mu);
  };

  /*--- Split among the dual pieces of the target vertices: piece i = {lambda_i >= lambda_j for all j}. If every vertex
   *    of the overlap is in the same piece, the overlap is that piece's part. ---*/
  for (unsigned short i = 0; i < nNode; ++i) {
    bool all = true;
    for (int v = 0; v < overlap.Size() && all; ++v) {
      const auto* x = overlap.Vertex(v);
      passivedouble lambdaI = frame.a[i];
      for (unsigned short iDim = 0; iDim < nDim; ++iDim) lambdaI += frame.G[i][iDim] * x[iDim];
      for (unsigned short j = 0; j < nNode && all; ++j) {
        if (j == i) continue;
        passivedouble lambdaJ = frame.a[j];
        for (unsigned short iDim = 0; iDim < nDim; ++iDim) lambdaJ += frame.G[j][iDim] * x[iDim];
        all = lambdaI >= lambdaJ;
      }
    }
    if (all) {
      call(i, volume0, centroid0);
      return true;
    }
  }
  Poly part(nDim);
  for (unsigned short i = 0; i < nNode; ++i) {
    part.CopyFrom(overlap);
    for (unsigned short j = 0; j < nNode && !part.Empty(); ++j) {
      if (j == i) continue;
      passivedouble g[3] = {};
      for (unsigned short iDim = 0; iDim < nDim; ++iDim) g[iDim] = frame.G[i][iDim] - frame.G[j][iDim];
      part.Clip(g, frame.a[i] - frame.a[j]);
    }
    if (part.Empty()) continue;
    passivedouble volume = 0.0, centroid[3] = {};
    part.Moments(volume, centroid);
    if (volume > 0.0) call(i, volume, centroid);
  }
  return true;
}

void PieceMoments(unsigned short nDim, const Frame& frame, unsigned short i, passivedouble& volume,
                  passivedouble* centroid) {
  Poly poly(nDim);
  poly.InitSimplex(frame.y);
  for (unsigned short j = 0; j <= nDim; ++j) {
    if (j == i) continue;
    passivedouble g[3] = {};
    for (unsigned short iDim = 0; iDim < nDim; ++iDim) g[iDim] = frame.G[i][iDim] - frame.G[j][iDim];
    poly.Clip(g, frame.a[i] - frame.a[j]);
  }
  poly.Moments(volume, centroid);
}

void SliverCentroid(unsigned short nDim, const passivedouble* const* donorNodes, passivedouble volume,
                    passivedouble missing, const passivedouble* moment, passivedouble* x, passivedouble* mu) {
  const unsigned short nNode = nDim + 1;
  passivedouble y[4][3] = {}, G[4][3] = {}, a[4] = {}, centroid[3] = {}, c[3] = {};
  for (unsigned short k = 0; k < nNode; ++k)
    for (unsigned short iDim = 0; iDim < nDim; ++iDim) {
      y[k][iDim] = donorNodes[k][iDim] - donorNodes[0][iDim];
      centroid[iDim] += y[k][iDim] / nNode;
    }
  SimplexPlanes(nDim, y, G, a);
  for (unsigned short iDim = 0; iDim < nDim; ++iDim) {
    c[iDim] = (missing > kCentroidFraction * volume) ? (volume * centroid[iDim] - moment[iDim]) / missing
                                                     : centroid[iDim];
    x[iDim] = c[iDim] + donorNodes[0][iDim];
  }
  for (unsigned short k = 0; k < nNode; ++k) {
    mu[k] = a[k];
    for (unsigned short iDim = 0; iDim < nDim; ++iDim) mu[k] += G[k][iDim] * c[iDim];
  }
}

/*------------------------------------------------------------------------------------------------------------------*/
/*--- Mass matrix and conjugate gradients                                                                          ---*/
/*------------------------------------------------------------------------------------------------------------------*/

void MassMatrix::Assemble(unsigned short nDim, unsigned long nRowIn, unsigned long nLocalIn,
                          const std::vector<unsigned long>& elements, const std::vector<passivedouble>& volumes) {
  const unsigned short nNode = nDim + 1;
  nRow = nRowIn;
  nLocal = nLocalIn;
  const auto nElem = volumes.size();
  const passivedouble diagCoef = (nDim == 2) ? 11.0 / 54.0 : 25.0 / 192.0;
  const passivedouble offCoef = (1.0 / nNode - diagCoef) / nDim;
  std::vector<std::vector<unsigned long>> adjacency(nRow);
  for (auto iElem = 0ul; iElem < nElem; ++iElem)
    for (unsigned short k = 0; k < nNode; ++k) {
      const auto row = elements[iElem * nNode + k];
      if (row >= nRow) continue;
      for (unsigned short l = 0; l < nNode; ++l) adjacency[row].push_back(elements[iElem * nNode + l]);
    }
  rowPtr.assign(nRow + 1, 0);
  for (auto iRow = 0ul; iRow < nRow; ++iRow) {
    auto& list = adjacency[iRow];
    list.push_back(iRow);
    std::sort(list.begin(), list.end());
    list.erase(std::unique(list.begin(), list.end()), list.end());
    rowPtr[iRow + 1] = rowPtr[iRow] + list.size();
  }
  col.resize(rowPtr[nRow]);
  for (auto iRow = 0ul; iRow < nRow; ++iRow) std::copy(adjacency[iRow].begin(), adjacency[iRow].end(), col.begin() + rowPtr[iRow]);
  adjacency = std::vector<std::vector<unsigned long>>();
  value.assign(col.size(), 0.0);
  for (auto iElem = 0ul; iElem < nElem; ++iElem) {
    for (unsigned short k = 0; k < nNode; ++k) {
      const auto row = elements[iElem * nNode + k];
      if (row >= nRow) continue;
      for (unsigned short l = 0; l < nNode; ++l) {
        const auto column = elements[iElem * nNode + l];
        const auto begin = col.begin() + rowPtr[row], end = col.begin() + rowPtr[row + 1];
        const auto pos = std::lower_bound(begin, end, column) - col.begin();
        value[pos] += (k == l ? diagCoef : offCoef) * volumes[iElem];
      }
    }
  }
  diag.assign(nRow, 0.0);
  rowVolume.assign(nRow, 0.0);
  for (auto iRow = 0ul; iRow < nRow; ++iRow) {
    for (auto p = rowPtr[iRow]; p < rowPtr[iRow + 1]; ++p) {
      rowVolume[iRow] += value[p];
      if (col[p] == iRow) diag[iRow] = value[p];
    }
  }
}

void MassMatrix::Multiply(const std::vector<passivedouble>& v, std::vector<passivedouble>& out) const {
  for (auto i = 0ul; i < nRow; ++i) {
    passivedouble sum = 0.0;
    for (auto p = rowPtr[i]; p < rowPtr[i + 1]; ++p) sum += value[p] * v[col[p]];
    out[i] = sum;
  }
}

passivedouble MassSolver::Sum(passivedouble value) const {
  return haloGeometry ? CPassiveComm::Allreduce(value, CPassiveComm::Op::SUM) : value;
}

void MassSolver::Sum2(passivedouble* values) const {
  if (!haloGeometry) return;
  passivedouble result[2];
  CPassiveComm::Allreduce(values, result, 2, CPassiveComm::Op::SUM);
  values[0] = result[0];
  values[1] = result[1];
}

passivedouble MassSolver::Max(passivedouble value) const {
  return haloGeometry ? CPassiveComm::Allreduce(value, CPassiveComm::Op::MAX) : value;
}

bool MassSolver::AnyOf(bool value) const {
  return haloGeometry ? CPassiveComm::AllreduceMax(value ? 1ul : 0ul) > 0 : value;
}

void MassSolver::Exchange(std::vector<passivedouble>& v) const {
  if (haloGeometry) CPassiveComm::ExchangeHalo(*haloGeometry, v.data(), sizeof(passivedouble));
}

SolveResult MassSolver::Solve(const std::vector<passivedouble>& b, std::vector<passivedouble>& x) const {
  SolveResult result;
  const auto n = matrix.nRow;
  x.assign(n, 0.0);

  /*--- Nonfinite right-hand side: a separate reduced flag before any shortcut (a maximum is not a NaN detector). ---*/
  bool bad = false;
  passivedouble amax = 0.0;
  for (auto i = 0ul; i < n; ++i) {
    bad |= !std::isfinite(b[i]);
    amax = std::max(amax, std::fabs(b[i]));
  }
  if (AnyOf(bad)) {
    result.nonfiniteRHS = true;
    result.failed = true;
    return result;
  }
  amax = Max(amax);
  if (amax == 0.0) return result;

  /*--- Exact power-of-two scaling: |bt| < 1, no overflow in the inner products (frexp: no infinite scale even for
   *    amax near the largest double). ---*/
  int exponent = 0;
  std::frexp(amax, &exponent);
  std::vector<passivedouble> bt(n), xt(matrix.nLocal, 0.0), r(n), z(n), p(matrix.nLocal, 0.0), q(n);
  for (auto i = 0ul; i < n; ++i) {
    bt[i] = std::ldexp(b[i], -exponent);
    xt[i] = bt[i] / matrix.rowVolume[i];
  }
  Exchange(xt);
  passivedouble bbLocal = 0.0;
  for (auto i = 0ul; i < n; ++i) bbLocal += bt[i] * bt[i];
  const passivedouble bb = Sum(bbLocal);

  matrix.Multiply(xt, q);
  passivedouble sums[2] = {0.0, 0.0};
  for (auto i = 0ul; i < n; ++i) {
    r[i] = bt[i] - q[i];
    z[i] = r[i] / matrix.diag[i];
    p[i] = z[i];
    sums[0] += r[i] * z[i];
    sums[1] += r[i] * r[i];
  }
  Sum2(sums);
  passivedouble rz = sums[0], rr = sums[1];
  if (!(std::isfinite(bb) && bb > 0.0) || !std::isfinite(rz) || !std::isfinite(rr) || !(rz > 0.0 || rr == 0.0)) {
    result.breakdown = true;
    result.failed = true;
    return result;
  }

  unsigned long it = 0;
  while (std::sqrt(rr / bb) > tolerance && it < maxIterations) {
    Exchange(p);
    matrix.Multiply(p, q);
    passivedouble pqLocal = 0.0;
    for (auto i = 0ul; i < n; ++i) pqLocal += p[i] * q[i];
    const passivedouble pq = Sum(pqLocal);
    if (!(std::isfinite(pq) && pq > 0.0)) break;
    const passivedouble alpha = rz / pq;
    sums[0] = sums[1] = 0.0;
    for (auto i = 0ul; i < n; ++i) {
      xt[i] += alpha * p[i];
      r[i] -= alpha * q[i];
      z[i] = r[i] / matrix.diag[i];
      sums[0] += r[i] * r[i];
      sums[1] += r[i] * z[i];
    }
    Sum2(sums);
    rr = sums[0];
    const passivedouble rzNew = sums[1];
    it++;
    if (!(std::isfinite(rr) && std::isfinite(rzNew))) break;
    if (rr == 0.0) break;
    if (!(rzNew > 0.0)) break;
    const passivedouble beta = rzNew / rz;
    rz = rzNew;
    for (auto i = 0ul; i < n; ++i) p[i] = z[i] + beta * p[i];
  }

  /*--- True residual. ---*/
  Exchange(xt);
  matrix.Multiply(xt, q);
  passivedouble trLocal = 0.0;
  for (auto i = 0ul; i < n; ++i) trLocal += (bt[i] - q[i]) * (bt[i] - q[i]);
  result.iterations = it;
  result.trueResidual = std::sqrt(Sum(trLocal) / bb);
  if (!std::isfinite(result.trueResidual) || result.trueResidual > 1e-10) {
    result.failed = true;
  } else if (result.trueResidual > tolerance) {
    result.warning = true;
  }
  for (auto i = 0ul; i < n; ++i) x[i] = std::ldexp(xt[i], exponent);
  return result;
}

SolveResult MassSolver::SolveActive(const std::vector<su2double>& b, std::vector<su2double>& x) const {
  /*--- M is passive (geometry), x = M^-1 b is linear in b: the values are solved as passive numbers and, in forward
   *    mode (DIRECT_DIFF), the derivatives as well, x' = M^-1 b', with the same guards (never the iterations of the
   *    conjugate gradients differentiated). Nothing is recorded on a reverse-mode tape. ---*/
  const auto n = matrix.nRow;
  std::vector<passivedouble> bValue(n), xValue;
  for (auto i = 0ul; i < n; ++i) bValue[i] = SU2_TYPE::GetValue(b[i]);
  auto result = Solve(bValue, xValue);
  x.resize(n);
  for (auto i = 0ul; i < n; ++i) x[i] = xValue[i];
#ifdef CODI_FORWARD_TYPE
  std::vector<passivedouble> bDerivative(n), xDerivative;
  bool seeded = false;
  for (auto i = 0ul; i < n; ++i) {
    bDerivative[i] = SU2_TYPE::GetDerivative(b[i]);
    seeded |= (bDerivative[i] != 0.0) || !std::isfinite(bDerivative[i]);
  }
  /*--- Solved if any rank has a nonzero (or nonfinite) derivative right-hand side. ---*/
  if (AnyOf(seeded)) {
    const auto derivative = Solve(bDerivative, xDerivative);
    result.derivativeSolved = true;
    result.nonfiniteRHS |= derivative.nonfiniteRHS;
    result.breakdown |= derivative.breakdown;
    result.failed |= derivative.failed;
    result.warning |= derivative.warning;
    result.trueResidual = std::max(result.trueResidual, derivative.trueResidual);
    if (!derivative.failed)
      for (auto i = 0ul; i < n; ++i) SU2_TYPE::SetDerivative(x[i], xDerivative[i]);
  }
#endif
  return result;
}

/*------------------------------------------------------------------------------------------------------------------*/
/*--- Bounded redistribution                                                                                        ---*/
/*------------------------------------------------------------------------------------------------------------------*/

RedistributeResult BoundedRedistribute(std::vector<su2double>& v, const std::vector<passivedouble>& cv, su2double total,
                                       const std::vector<su2double>& lo, const std::vector<su2double>& hi,
                                       const std::vector<bool>* frozen, passivedouble countTolerance,
                                       passivedouble range, bool distributed) {
  RedistributeResult result;
  const auto n = v.size();
  auto isFree = [&](unsigned long i) { return frozen == nullptr || !(*frozen)[i]; };
  auto infinite = [](const su2double& value) { return std::isinf(SU2_TYPE::GetValue(value)); };
  auto reduce = [distributed](CAccurateSumBatch& batch) {
    if (distributed) {
      batch.Reduce();
    } else {
      batch.ReduceLocal();
    }
  };
  auto weighted = [&](std::vector<su2double>& terms) {
    terms.resize(n);
    for (auto i = 0ul; i < n; ++i) terms[i] = v[i] * cv[i];
  };

  /*--- Clip to the bounds (count beyond the tolerance). ---*/
  unsigned long nClipped = 0;
  for (auto i = 0ul; i < n; ++i) {
    if (!isFree(i)) continue;
    if (v[i] < lo[i] || v[i] > hi[i]) {
      const su2double bound = (v[i] < lo[i]) ? lo[i] : hi[i];
      if (fabs(v[i] - bound) > countTolerance) nClipped++;
      v[i] = bound;
    }
  }
  result.nClipped = distributed ? CPassiveComm::AllreduceSum(nClipped) : nClipped;

  /*--- Scale of the residual tests. ---*/
  passivedouble scale = 0.0;
  {
    std::vector<passivedouble> terms(n);
    for (auto i = 0ul; i < n; ++i) terms[i] = std::fabs(SU2_TYPE::GetValue(v[i])) * cv[i];
    CAccurateSumBatch batch;
    batch.Add(terms);
    reduce(batch);
    scale = std::max(batch.Get(0), std::fabs(SU2_TYPE::GetValue(total)));
  }

  /*--- Redistribute the defect over the free values with room, in proportion to the room (iterated). ---*/
  std::vector<su2double> terms;
  std::vector<passivedouble> capacityTerms(n), unboundedTerms(n);
  su2double r = 0.0;
  bool done = false;
  for (int iter = 0; iter < 100 && !done; ++iter) {
    weighted(terms);
    CAccurateSumBatch batch;
    batch.AddActive(terms);
    reduce(batch);
    r = total - batch.GetActive(0);
    if (!(fabs(r) > 1e-15 * scale)) {
      done = true;
      break;
    }
    std::vector<su2double> capacity(n, 0.0);
    for (auto i = 0ul; i < n; ++i) {
      unboundedTerms[i] = 0.0;
      if (!isFree(i)) continue;
      const su2double room = (r > 0.0) ? hi[i] - v[i] : v[i] - lo[i];
      if (infinite(room)) {
        unboundedTerms[i] = cv[i];
      } else if (room > 0.0) {
        capacity[i] = room * cv[i];
      }
    }
    CAccurateSumBatch roomBatch;
    roomBatch.AddActive(capacity);
    roomBatch.Add(unboundedTerms);
    reduce(roomBatch);
    const su2double totalCapacity = roomBatch.GetActive(0);
    const passivedouble unboundedVolume = roomBatch.Get(roomBatch.Size() - 1);
    if (unboundedVolume > 0.0) {
      for (auto i = 0ul; i < n; ++i) {
        if (!isFree(i)) continue;
        const su2double room = (r > 0.0) ? hi[i] - v[i] : v[i] - lo[i];
        if (infinite(room)) v[i] += r / unboundedVolume;
      }
      continue;
    }
    if (!(totalCapacity > 0.0)) break;
    su2double fraction = fabs(r) / totalCapacity;
    if (fraction > 1.0) fraction = 1.0;
    for (auto i = 0ul; i < n; ++i) {
      if (!isFree(i)) continue;
      su2double room = (r > 0.0) ? hi[i] - v[i] : v[i] - lo[i];
      if (!(room > 0.0)) room = 0.0;
      v[i] += (r > 0.0 ? fraction : su2double(-fraction)) * room;
    }
  }

  if (!done) {
    /*--- The bounds cannot hold the total: residual first, then the rest over the free values by volume. ---*/
    weighted(terms);
    std::vector<passivedouble> freeVolume(n, 0.0);
    for (auto i = 0ul; i < n; ++i)
      if (isFree(i)) freeVolume[i] = cv[i];
    CAccurateSumBatch batch;
    batch.AddActive(terms);
    batch.Add(freeVolume);
    reduce(batch);
    r = total - batch.GetActive(0);
    const passivedouble volume = batch.Get(batch.Size() - 1);
    if (fabs(r) > 1e-15 * scale) {
      if (volume > 0.0) {
        for (auto i = 0ul; i < n; ++i)
          if (isFree(i)) v[i] += r / volume;
        /*--- Reported as relaxed bounds beyond round-off of the total. ---*/
        result.relaxed = fabs(r) > 1e-12 * scale;
      } else {
        result.error = true;
        result.reason = "a nonzero correction of the total is required but no value is free to take it";
      }
    }
  }

  /*--- Recomputed residual and bound violations. ---*/
  weighted(terms);
  unsigned long nViolations = 0;
  passivedouble maxViolation = 0.0;
  const passivedouble rangeScale = (range > 0.0) ? range : 1.0;
  for (auto i = 0ul; i < n; ++i) {
    if (!isFree(i)) continue;
    const passivedouble value = SU2_TYPE::GetValue(v[i]);
    const passivedouble excess = std::max(value - SU2_TYPE::GetValue(hi[i]), SU2_TYPE::GetValue(lo[i]) - value);
    if (excess > countTolerance) {
      nViolations++;
      maxViolation = std::max(maxViolation, excess / rangeScale);
    }
  }
  CAccurateSumBatch batch;
  batch.AddActive(terms);
  reduce(batch);
  result.residual = std::fabs(SU2_TYPE::GetValue(total) - batch.Get(0)) / std::max(scale, 1e-300);
  result.totalExact = result.residual <= 1e-15;
  if (!(result.residual <= 1e-12) && !result.error) {
    result.error = true;
    result.reason = "the total is not restored (relative residual " + std::to_string(result.residual) + ")";
  }
  if (distributed) {
    result.nViolations = CPassiveComm::AllreduceSum(nViolations);
    result.maxViolation = CPassiveComm::Allreduce(maxViolation, CPassiveComm::Op::MAX);
  } else {
    result.nViolations = nViolations;
    result.maxViolation = maxViolation;
  }
  return result;
}

}  // namespace conservative
