/*!
 * \file CConservativeTransfer.cpp
 * \brief Conservative P1 transfer of the solution to a new mesh (supermesh of the two meshes, mesh adaptation).
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

#include "../../include/adaptation/CConservativeTransfer.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <limits>
#include <numeric>
#include <sstream>

#include "../../../Common/include/CConfig.hpp"
#include "../../../Common/include/geometry/CGeometry.hpp"
#include "../../include/adaptation/ConvexClipping.hpp"
#include "../../include/fluid/CFluidModel.hpp"
#include "../../include/solvers/CSolver.hpp"
#include "../../include/solvers/CTurbSolver.hpp"

namespace {

constexpr passivedouble kOverlapFraction = 1e-13; /*!< \brief Overlaps below this fraction of the smaller element are
                                                        ignored (round-off contacts at shared faces/edges/points). */
constexpr passivedouble kGapFraction = 1e-10;     /*!< \brief Uncovered parts above this fraction are filled/moved. */
constexpr passivedouble kCentroidFraction = 1e-8; /*!< \brief Below it, the centroid of a missing part is not
                                                        computed from the moments (cancellation), the whole cell's
                                                        centroid is used. */
constexpr unsigned long kSeedBudget = 512;        /*!< \brief Donor elements tested in the fallback seed search. */
constexpr passivedouble kInf = std::numeric_limits<passivedouble>::infinity();

/*--- Barycentric functions of the simplex y[0..nDim] (coordinates of a local frame): lambda_k(x) = a[k] + G[k].x.
 *    Returns the determinant of the edge matrix (nDim! x the signed measure), 0 for a degenerate simplex. ---*/
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

passivedouble Factorial(unsigned short nDim) { return nDim == 2 ? 2.0 : 6.0; }

std::string PointText(unsigned short nDim, const passivedouble* x) {
  std::ostringstream text;
  text << std::setprecision(10) << "(";
  for (unsigned short iDim = 0; iDim < nDim; ++iDim) text << (iDim ? ", " : "") << x[iDim];
  text << ")";
  return text.str();
}

}  // namespace

/*!
 * \brief A target element in a frame local to its first vertex: vertices, barycentric functions, bounding box.
 */
struct CConservativeProjection::Frame {
  unsigned long iElem = 0;
  passivedouble origin[3] = {};
  passivedouble y[4][3] = {};
  passivedouble G[4][3] = {}, a[4] = {};
  passivedouble volume = 0.0;
  passivedouble bbMin[3] = {}, bbMax[3] = {};
};

/*!
 * \brief Convex polygon (2D) or polyhedron (3D).
 */
class CConservativeProjection::Poly {
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

CConservativeProjection::CConservativeProjection(const CGeometry& donor, const std::vector<std::string>& donorTags,
                                                 const CGeometry& target, const std::vector<std::string>& targetTags,
                                                 const Options& options)
    : options(options), nDim(target.GetnDim()), nNode(target.GetnDim() + 1), targetGeometry(target) {
  const auto start = SU2_MPI::Wtime();
  if (donor.GetnDim() != nDim) SU2_MPI::Error("The two meshes have different dimensions.", CURRENT_FUNCTION);
  if (donor.GetnPoint() != donor.GetnPointDomain() || target.GetnPoint() != target.GetnPointDomain()) {
    SU2_MPI::Error("The conservative projection needs meshes without halo points (one rank).", CURRENT_FUNCTION);
  }
  const unsigned short elemType = (nDim == 2) ? TRIANGLE : TETRAHEDRON;

  auto readMesh = [&](const CGeometry& geometry, std::vector<passivedouble>& coord, std::vector<unsigned long>& elem,
                      std::vector<passivedouble>& volElem, std::vector<passivedouble>& cv) {
    const auto nPoint = geometry.GetnPoint();
    coord.resize(nPoint * nDim);
    cv.resize(nPoint);
    for (auto iPoint = 0ul; iPoint < nPoint; ++iPoint) {
      for (unsigned short iDim = 0; iDim < nDim; ++iDim)
        coord[iPoint * nDim + iDim] = SU2_TYPE::GetValue(geometry.nodes->GetCoord(iPoint, iDim));
      cv[iPoint] = SU2_TYPE::GetValue(geometry.nodes->GetVolume(iPoint));
    }
    elem.resize(geometry.GetnElem() * nNode);
    volElem.resize(geometry.GetnElem());
    for (auto iElem = 0ul; iElem < geometry.GetnElem(); ++iElem) {
      if (geometry.elem[iElem]->GetVTK_Type() != elemType) {
        SU2_MPI::Error("The conservative projection needs a mesh of triangles (2D) or tetrahedra (3D).",
                       CURRENT_FUNCTION);
      }
      passivedouble y[4][3] = {}, G[4][3], a[4];
      for (unsigned short k = 0; k < nNode; ++k) {
        elem[iElem * nNode + k] = geometry.elem[iElem]->GetNode(k);
        for (unsigned short iDim = 0; iDim < nDim; ++iDim)
          y[k][iDim] = coord[elem[iElem * nNode + k] * nDim + iDim] - coord[elem[iElem * nNode] * nDim + iDim];
      }
      volElem[iElem] = fabs(SimplexPlanes(nDim, y, G, a)) / Factorial(nDim);
    }
  };

  /*--- Donor: points, elements, neighbours across faces, boundary faces (faces with one element). ---*/

  readMesh(donor, coordD, elemD, volElemD, cvD);
  nPointD = donor.GetnPoint();
  nElemD = donor.GetnElem();

  {
    struct Face {
      std::array<unsigned long, 3> key;
      unsigned long elem;
      unsigned short k;
    };
    std::vector<Face> faces;
    faces.reserve(nElemD * nNode);
    for (auto iElem = 0ul; iElem < nElemD; ++iElem) {
      for (unsigned short k = 0; k < nNode; ++k) {
        Face face{{0, 0, 0}, iElem, k};
        unsigned short m = 0;
        for (unsigned short l = 0; l < nNode; ++l)
          if (l != k) face.key[m++] = elemD[iElem * nNode + l];
        std::sort(face.key.begin(), face.key.begin() + nDim);
        faces.push_back(face);
      }
    }
    std::sort(faces.begin(), faces.end(), [](const Face& f1, const Face& f2) { return f1.key < f2.key; });
    nbrD.assign(nElemD * nNode, -1);
    for (auto i = 0ul; i < faces.size();) {
      auto j = i + 1;
      while (j < faces.size() && faces[j].key == faces[i].key) j++;
      if (j - i == 2) {
        nbrD[faces[i].elem * nNode + faces[i].k] = faces[i + 1].elem;
        nbrD[faces[i + 1].elem * nNode + faces[i + 1].k] = faces[i].elem;
      } else if (j - i == 1) {
        freeFaces.emplace_back(std::vector<unsigned long>(faces[i].key.begin(), faces[i].key.begin() + nDim),
                               faces[i].elem);
      } else {
        SU2_MPI::Error("The donor mesh has a face shared by more than two elements.", CURRENT_FUNCTION);
      }
      i = j;
    }
    std::sort(freeFaces.begin(), freeFaces.end());
  }
  donorLocator = std::make_unique<CBarycentricLocator>(donor, donorTags, options.absoluteLimit);
  donorNames = donorLocator->GetMarkerNames();

  /*--- Target: points, elements, markers of each point. ---*/

  readMesh(target, coordT, elemT, volElemT, cvT);
  nPointT = target.GetnPoint();
  nElemT = target.GetnElem();
  pointMarkers.resize(nPointT);
  for (unsigned short iMarker = 0; iMarker < target.GetnMarker() && iMarker < targetTags.size(); ++iMarker)
    for (auto iVertex = 0ul; iVertex < target.GetnVertex(iMarker); ++iVertex)
      pointMarkers[target.vertex[iMarker][iVertex]->GetNode()].push_back(targetTags[iMarker]);
  if (options.sliverRule == SliverRule::BOUNDARY)
    targetLocator = std::make_unique<CBarycentricLocator>(target, targetTags);

  /*--- Mass matrix M[j][k] = integral over C_j of the hat function of k (exact for simplices: the median-dual piece of
   *    vertex i is {lambda_i >= lambda_j}; integral of lambda_i over it = |e| E[max of the barycentric coordinates] /
   *    (nDim+1), 11/54 |e| in 2D, 25/192 |e| in 3D; the rest of the row of the element is |e|/(nDim+1) - that). ---*/

  const passivedouble diagCoef = (nDim == 2) ? 11.0 / 54.0 : 25.0 / 192.0;
  const passivedouble offCoef = (1.0 / nNode - diagCoef) / nDim;
  {
    std::vector<std::vector<unsigned long>> adjacency(nPointT);
    for (auto iElem = 0ul; iElem < nElemT; ++iElem)
      for (unsigned short k = 0; k < nNode; ++k)
        for (unsigned short l = 0; l < nNode; ++l)
          adjacency[elemT[iElem * nNode + k]].push_back(elemT[iElem * nNode + l]);
    massRowPtr.assign(nPointT + 1, 0);
    for (auto iPoint = 0ul; iPoint < nPointT; ++iPoint) {
      auto& row = adjacency[iPoint];
      row.push_back(iPoint);
      std::sort(row.begin(), row.end());
      row.erase(std::unique(row.begin(), row.end()), row.end());
      massRowPtr[iPoint + 1] = massRowPtr[iPoint] + row.size();
    }
    massCol.resize(massRowPtr[nPointT]);
    for (auto iPoint = 0ul; iPoint < nPointT; ++iPoint)
      std::copy(adjacency[iPoint].begin(), adjacency[iPoint].end(), massCol.begin() + massRowPtr[iPoint]);
  }
  massValue.assign(massCol.size(), 0.0);
  for (auto iElem = 0ul; iElem < nElemT; ++iElem) {
    for (unsigned short k = 0; k < nNode; ++k) {
      const auto row = elemT[iElem * nNode + k];
      for (unsigned short l = 0; l < nNode; ++l) {
        const auto col = elemT[iElem * nNode + l];
        const auto begin = massCol.begin() + massRowPtr[row], end = massCol.begin() + massRowPtr[row + 1];
        const auto pos = std::lower_bound(begin, end, col) - massCol.begin();
        massValue[pos] += (k == l ? diagCoef : offCoef) * volElemT[iElem];
      }
    }
  }
  massDiag.assign(nPointT, 0.0);
  rowVolume.assign(nPointT, 0.0);
  for (auto iPoint = 0ul; iPoint < nPointT; ++iPoint) {
    for (auto p = massRowPtr[iPoint]; p < massRowPtr[iPoint + 1]; ++p) {
      rowVolume[iPoint] += massValue[p];
      if (massCol[p] == iPoint) massDiag[iPoint] = massValue[p];
    }
    if (!(massDiag[iPoint] > 0.0)) {
      SU2_MPI::Error("A point of the new mesh has no control volume (unused point or degenerate elements).",
                     CURRENT_FUNCTION);
    }
  }

  summary.nTargetElem = nElemT;
  summary.nDonorElem = nElemD;
  summary.timeSetup = SU2_TYPE::GetValue(SU2_MPI::Wtime() - start);
}

CConservativeProjection::~CConservativeProjection() = default;

void CConservativeProjection::SetFrame(unsigned long iElem, Frame& frame) const {
  frame.iElem = iElem;
  const auto* nodes = &elemT[iElem * nNode];
  for (unsigned short iDim = 0; iDim < nDim; ++iDim) frame.origin[iDim] = coordT[nodes[0] * nDim + iDim];
  for (unsigned short k = 0; k < nNode; ++k)
    for (unsigned short iDim = 0; iDim < 3; ++iDim)
      frame.y[k][iDim] = (iDim < nDim) ? coordT[nodes[k] * nDim + iDim] - frame.origin[iDim] : 0.0;
  frame.volume = fabs(SimplexPlanes(nDim, frame.y, frame.G, frame.a)) / Factorial(nDim);
  for (unsigned short iDim = 0; iDim < nDim; ++iDim) {
    frame.bbMin[iDim] = kInf;
    frame.bbMax[iDim] = -kInf;
    for (unsigned short k = 0; k < nNode; ++k) {
      frame.bbMin[iDim] = std::min(frame.bbMin[iDim], frame.y[k][iDim]);
      frame.bbMax[iDim] = std::max(frame.bbMax[iDim], frame.y[k][iDim]);
    }
  }
}

void CConservativeProjection::UpdateBounds(unsigned long iPoint, const unsigned long* donorPoints,
                                           unsigned short nDonor) {
  const auto& U = *donorField;
  for (unsigned short k = 0; k < nDonor; ++k) {
    for (unsigned short f = 0; f < nField; ++f) {
      const auto value = U[donorPoints[k] * nField + f];
      auto& lo = lower[iPoint * nField + f];
      auto& hi = upper[iPoint * nField + f];
      lo = std::min(lo, value);
      hi = std::max(hi, value);
    }
  }
}

bool CConservativeProjection::Overlap(const Frame& frame, unsigned long jElem, bool accumulate) {
  const auto* nodesD = &elemD[jElem * nNode];

  /*--- Donor simplex in the frame of the target element; bounding boxes. ---*/
  passivedouble z[4][3] = {};
  for (unsigned short k = 0; k < nNode; ++k)
    for (unsigned short iDim = 0; iDim < nDim; ++iDim)
      z[k][iDim] = coordD[nodesD[k] * nDim + iDim] - frame.origin[iDim];
  for (unsigned short iDim = 0; iDim < nDim; ++iDim) {
    passivedouble zMin = kInf, zMax = -kInf;
    for (unsigned short k = 0; k < nNode; ++k) {
      zMin = std::min(zMin, z[k][iDim]);
      zMax = std::max(zMax, z[k][iDim]);
    }
    if (zMax < frame.bbMin[iDim] || zMin > frame.bbMax[iDim]) return false;
  }
  summary.nTested++;

  /*--- Overlap: the donor simplex clipped by lambda_k >= 0 of the target element. ---*/
  Poly overlap(nDim);
  overlap.InitSimplex(z);
  for (unsigned short k = 0; k < nNode; ++k) {
    overlap.Clip(frame.G[k], frame.a[k]);
    if (overlap.Empty()) return false;
  }
  passivedouble volume0 = 0.0, centroid0[3] = {};
  overlap.Moments(volume0, centroid0);
  if (!(volume0 > kOverlapFraction * std::min(frame.volume, volElemD[jElem]))) return false;
  if (!accumulate) return true;
  summary.nPairs++;

  /*--- Donor field in the frame. ---*/
  passivedouble GD[4][3] = {}, aD[4] = {};
  SimplexPlanes(nDim, z, GD, aD);

  auto accumulatePiece = [&](unsigned short i, passivedouble volume, const passivedouble* centroid) {
    const auto iPoint = elemT[frame.iElem * nNode + i];
    passivedouble mu[4] = {};
    for (unsigned short k = 0; k < nNode; ++k) {
      mu[k] = aD[k];
      for (unsigned short iDim = 0; iDim < nDim; ++iDim) mu[k] += GD[k][iDim] * centroid[iDim];
    }
    const auto& U = *donorField;
    for (unsigned short f = 0; f < nField; ++f) {
      passivedouble value = 0.0;
      for (unsigned short k = 0; k < nNode; ++k) value += mu[k] * U[nodesD[k] * nField + f];
      rhs[iPoint * nField + f] += volume * value;
    }
    const auto piece = frame.iElem * nNode + i;
    covT[piece] += volume;
    covD[jElem] += volume;
    for (unsigned short iDim = 0; iDim < nDim; ++iDim) {
      momT[piece * 3 + iDim] += volume * centroid[iDim];
      momD[jElem * 3 + iDim] += volume * (centroid[iDim] + frame.origin[iDim] - coordD[nodesD[0] * nDim + iDim]);
    }
    UpdateBounds(iPoint, nodesD, nNode);
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
      accumulatePiece(i, volume0, centroid0);
      return true;
    }
  }
  Poly piece(nDim);
  for (unsigned short i = 0; i < nNode; ++i) {
    piece.CopyFrom(overlap);
    for (unsigned short j = 0; j < nNode && !piece.Empty(); ++j) {
      if (j == i) continue;
      passivedouble g[3] = {};
      for (unsigned short iDim = 0; iDim < nDim; ++iDim) g[iDim] = frame.G[i][iDim] - frame.G[j][iDim];
      piece.Clip(g, frame.a[i] - frame.a[j]);
    }
    if (piece.Empty()) continue;
    passivedouble volume = 0.0, centroid[3] = {};
    piece.Moments(volume, centroid);
    if (volume > 0.0) accumulatePiece(i, volume, centroid);
  }
  return true;
}

long CConservativeProjection::FindSeed(const Frame& frame, std::vector<unsigned long>& stamp,
                                       std::vector<unsigned long>& queue) {
  /*--- Donor elements that contain the centroid or points near the vertices of the target element. ---*/
  passivedouble centroid[3] = {};
  for (unsigned short k = 0; k < nNode; ++k)
    for (unsigned short iDim = 0; iDim < nDim; ++iDim) centroid[iDim] += frame.y[k][iDim] / nNode;

  long tried[5] = {-1, -1, -1, -1, -1};
  for (unsigned short k = 0; k <= nNode; ++k) {
    su2double x[3] = {};
    for (unsigned short iDim = 0; iDim < nDim; ++iDim) {
      const passivedouble local =
          (k == 0) ? centroid[iDim] : frame.y[k - 1][iDim] + 0.05 * (centroid[iDim] - frame.y[k - 1][iDim]);
      x[iDim] = local + frame.origin[iDim];
    }
    const long candidate = donorLocator->ContainingElement(x);
    tried[k] = candidate;
    if (candidate < 0 || std::find(tried, tried + k, candidate) != tried + k) continue;
    if (Overlap(frame, candidate, false)) return candidate;
  }

  /*--- Else (the element lies near or across the donor boundary): breadth-first search from the donor element at the
   *    boundary face nearest to the centroid. ---*/
  summary.nSeedFallback++;
  su2double x[3] = {};
  for (unsigned short iDim = 0; iDim < nDim; ++iDim) x[iDim] = centroid[iDim] + frame.origin[iDim];
  const auto stencil = donorLocator->Locate(x);
  long start = -1;
  if (stencil.onFace) {
    std::vector<unsigned long> key(stencil.point, stencil.point + nDim);
    std::sort(key.begin(), key.end());
    const auto it = std::lower_bound(freeFaces.begin(), freeFaces.end(), std::make_pair(key, 0ul));
    if (it != freeFaces.end() && it->first == key) start = it->second;
  } else {
    start = donorLocator->ContainingElement(x);
  }
  if (start < 0) return -1;

  const unsigned long tag = frame.iElem + 1;
  queue.clear();
  queue.push_back(start);
  stamp[start] = tag;
  for (auto head = 0ul; head < queue.size() && head < kSeedBudget; ++head) {
    const auto jElem = queue[head];
    if (Overlap(frame, jElem, false)) return jElem;
    for (unsigned short k = 0; k < nNode; ++k) {
      const auto next = nbrD[jElem * nNode + k];
      if (next >= 0 && stamp[next] != tag) {
        stamp[next] = tag;
        queue.push_back(next);
      }
    }
  }
  return -1;
}

void CConservativeProjection::SolveMass(const std::vector<passivedouble>& b, std::vector<passivedouble>& x,
                                        unsigned long& iterations, passivedouble& residual) const {
  /*--- Jacobi-preconditioned conjugate gradients from the control-volume means. ---*/
  const auto n = nPointT;
  auto multiply = [&](const std::vector<passivedouble>& v, std::vector<passivedouble>& out) {
    for (auto i = 0ul; i < n; ++i) {
      passivedouble sum = 0.0;
      for (auto p = massRowPtr[i]; p < massRowPtr[i + 1]; ++p) sum += massValue[p] * v[massCol[p]];
      out[i] = sum;
    }
  };
  auto dot = [n](const std::vector<passivedouble>& u, const std::vector<passivedouble>& v) {
    passivedouble sum = 0.0;
    for (auto i = 0ul; i < n; ++i) sum += u[i] * v[i];
    return sum;
  };

  x.resize(n);
  for (auto i = 0ul; i < n; ++i) x[i] = b[i] / rowVolume[i];
  const passivedouble normB = sqrt(dot(b, b));
  iterations = 0;
  residual = 0.0;
  if (!(normB > 0.0)) return;

  std::vector<passivedouble> r(n), z(n), p(n), q(n);
  multiply(x, q);
  for (auto i = 0ul; i < n; ++i) {
    r[i] = b[i] - q[i];
    z[i] = r[i] / massDiag[i];
  }
  p = z;
  passivedouble rz = dot(r, z);
  residual = sqrt(dot(r, r)) / normB;
  while (residual > options.solverTolerance && iterations < options.maxSolverIter) {
    multiply(p, q);
    const passivedouble pq = dot(p, q);
    if (!(pq > 0.0)) break;
    const passivedouble alpha = rz / pq;
    for (auto i = 0ul; i < n; ++i) {
      x[i] += alpha * p[i];
      r[i] -= alpha * q[i];
      z[i] = r[i] / massDiag[i];
    }
    iterations++;
    residual = sqrt(dot(r, r)) / normB;
    const passivedouble rzNew = dot(r, z);
    const passivedouble beta = rzNew / rz;
    rz = rzNew;
    for (auto i = 0ul; i < n; ++i) p[i] = z[i] + beta * p[i];
  }
}

bool CConservativeProjection::BoundedRedistribute(std::vector<passivedouble>& v, unsigned short iField,
                                                  passivedouble total, const std::vector<passivedouble>& lo,
                                                  const std::vector<passivedouble>& hi, const std::vector<bool>* frozen,
                                                  unsigned long* nClipped) const {
  const auto n = nPointT;
  auto isFree = [&](unsigned long i) { return frozen == nullptr || !(*frozen)[i]; };

  /*--- Clip to the bounds (count beyond round-off). ---*/
  if (nClipped != nullptr) *nClipped = 0;
  const passivedouble typical = summary.scale[iField] / std::max(summary.donorCV, passivedouble(1e-300));
  const passivedouble countTol = 1e-12 * std::max({range[iField], typical, passivedouble(1e-300)});
  for (auto i = 0ul; i < n; ++i) {
    if (!isFree(i)) continue;
    if (v[i] < lo[i] || v[i] > hi[i]) {
      const passivedouble bound = (v[i] < lo[i]) ? lo[i] : hi[i];
      if (nClipped != nullptr && fabs(v[i] - bound) > countTol) (*nClipped)++;
      v[i] = bound;
    }
  }

  /*--- Redistribute the defect over the nodes with room, in proportion to the room (iterated). ---*/
  passivedouble scale = 0.0;
  for (auto i = 0ul; i < n; ++i) scale += fabs(v[i]) * cvT[i];
  scale = std::max(scale, fabs(total));
  for (int iter = 0; iter < 100; ++iter) {
    passivedouble sum = 0.0;
    for (auto i = 0ul; i < n; ++i) sum += v[i] * cvT[i];
    const passivedouble r = total - sum;
    if (!(fabs(r) > 1e-15 * scale)) return true;

    passivedouble capacity = 0.0, unboundedVolume = 0.0;
    for (auto i = 0ul; i < n; ++i) {
      if (!isFree(i)) continue;
      const passivedouble room = (r > 0.0) ? hi[i] - v[i] : v[i] - lo[i];
      if (std::isinf(room)) {
        unboundedVolume += cvT[i];
      } else {
        capacity += std::max(room, passivedouble(0.0)) * cvT[i];
      }
    }
    if (unboundedVolume > 0.0) {
      for (auto i = 0ul; i < n; ++i) {
        if (!isFree(i)) continue;
        const passivedouble room = (r > 0.0) ? hi[i] - v[i] : v[i] - lo[i];
        if (std::isinf(room)) v[i] += r / unboundedVolume;
      }
      continue;
    }
    if (!(capacity > 0.0)) break;
    const passivedouble fraction = std::min(fabs(r) / capacity, passivedouble(1.0));
    for (auto i = 0ul; i < n; ++i) {
      if (!isFree(i)) continue;
      const passivedouble room = std::max((r > 0.0) ? hi[i] - v[i] : v[i] - lo[i], passivedouble(0.0));
      v[i] += (r > 0.0 ? fraction : -fraction) * room;
    }
  }

  /*--- The bounds cannot hold the total (or round-off is left): spread the rest over the free nodes by volume. ---*/
  passivedouble sum = 0.0, freeVolume = 0.0;
  for (auto i = 0ul; i < n; ++i) {
    sum += v[i] * cvT[i];
    if (isFree(i)) freeVolume += cvT[i];
  }
  const passivedouble r = total - sum;
  if (freeVolume > 0.0)
    for (auto i = 0ul; i < n; ++i)
      if (isFree(i)) v[i] += r / freeVolume;
  return !(fabs(r) > 1e-12 * scale);
}

void CConservativeProjection::Project(unsigned short nFieldIn, const std::vector<passivedouble>& donorValues,
                                      std::vector<passivedouble>& newValues) {
  SU2_ZONE_SCOPED

  nField = nFieldIn;
  if (donorValues.size() != nPointD * nField) SU2_MPI::Error("Wrong size of the donor values.", CURRENT_FUNCTION);
  donorField = &donorValues;
  const auto timeSetup = summary.timeSetup;
  summary = Summary();
  summary.timeSetup = timeSetup;
  summary.nField = nField;
  summary.nTargetElem = nElemT;
  summary.nDonorElem = nElemD;

  rhs.assign(nPointT * nField, 0.0);
  lower.assign(nPointT * nField, kInf);
  upper.assign(nPointT * nField, -kInf);
  covT.assign(nElemT * nNode, 0.0);
  momT.assign(nElemT * nNode * 3, 0.0);
  covD.assign(nElemD, 0.0);
  momD.assign(nElemD * 3, 0.0);

  /*--- Supermesh: every target element against the donor elements that overlap it (advancing front). ---*/

  auto start = SU2_MPI::Wtime();
  {
    std::vector<unsigned long> stamp(nElemD, 0), seedStamp(nElemD, 0), queue, seedQueue;
    Frame frame;
    for (auto iElem = 0ul; iElem < nElemT; ++iElem) {
      SetFrame(iElem, frame);
      summary.targetVolume += frame.volume;
      if (!(frame.volume > 0.0)) {
        summary.nDegenerate++;
        continue;
      }
      const long seed = FindSeed(frame, seedStamp, seedQueue);
      if (seed < 0) {
        summary.nElemOutside++;
        continue;
      }
      const unsigned long tag = iElem + 1;
      queue.clear();
      queue.push_back(seed);
      stamp[seed] = tag;
      for (auto head = 0ul; head < queue.size(); ++head) {
        const auto jElem = queue[head];
        if (!Overlap(frame, jElem, true)) continue;
        for (unsigned short k = 0; k < nNode; ++k) {
          const auto next = nbrD[jElem * nNode + k];
          if (next >= 0 && stamp[next] != tag) {
            stamp[next] = tag;
            queue.push_back(next);
          }
        }
      }
    }
  }
  for (auto jElem = 0ul; jElem < nElemD; ++jElem) {
    summary.donorVolume += volElemD[jElem];
    summary.overlapVolume += covD[jElem];
  }
  summary.timeSupermesh = SU2_TYPE::GetValue(SU2_MPI::Wtime() - start);
  start = SU2_MPI::Wtime();

  const auto& U = donorValues;
  summary.fill.assign(nField, 0.0);
  summary.sliver.assign(nField, 0.0);
  summary.fillOpen.assign(nField, 0.0);
  summary.sliverOpen.assign(nField, 0.0);

  /*--- New domain outside the donor (S_n): the missing part of every dual piece, with the donor field at its centroid
   *    by the closest-point rule of the barycentric transfer. ---*/
  {
    unsigned long nBeyond = 0;
    passivedouble worstRatio = 0.0, worstX[3] = {};
    std::vector<bool> filledNode(nPointT, false);
    Frame frame;
    for (auto iElem = 0ul; iElem < nElemT; ++iElem) {
      SetFrame(iElem, frame);
      if (!(frame.volume > 0.0)) continue;
      const passivedouble pieceVolume = frame.volume / nNode;
      for (unsigned short i = 0; i < nNode; ++i) {
        const auto piece = iElem * nNode + i;
        const passivedouble missing = pieceVolume - covT[piece];
        summary.maxUncovered = std::max(summary.maxUncovered, missing / pieceVolume);
        if (!(missing > kGapFraction * pieceVolume)) continue;

        /*--- Centroid of the piece (the element clipped by the piece planes), then of its missing part. ---*/
        Poly poly(nDim);
        poly.InitSimplex(frame.y);
        for (unsigned short j = 0; j < nNode; ++j) {
          if (j == i) continue;
          passivedouble g[3] = {};
          for (unsigned short iDim = 0; iDim < nDim; ++iDim) g[iDim] = frame.G[i][iDim] - frame.G[j][iDim];
          poly.Clip(g, frame.a[i] - frame.a[j]);
        }
        passivedouble volume = 0.0, centroid[3] = {};
        poly.Moments(volume, centroid);
        su2double x[3] = {};
        passivedouble xp[3] = {};
        for (unsigned short iDim = 0; iDim < nDim; ++iDim) {
          const passivedouble local = (missing > kCentroidFraction * pieceVolume)
                                          ? (volume * centroid[iDim] - momT[piece * 3 + iDim]) / missing
                                          : centroid[iDim];
          xp[iDim] = local + frame.origin[iDim];
          x[iDim] = xp[iDim];
        }

        const auto iPoint = elemT[piece];
        std::vector<std::string> names;
        for (const auto& name : pointMarkers[iPoint])
          if (donorLocator->HasMarker(name)) names.push_back(name);
        std::string markerName;
        const auto stencil =
            names.empty() ? donorLocator->Locate(x) : donorLocator->LocateOnBoundary(x, names, &markerName);
        /*--- The boundary this part lies at: the marker of its stencil, else that of the nearest donor face. ---*/
        if (markerName.empty()) donorLocator->LocateOnBoundary(x, donorNames, &markerName);
        const bool open =
            std::find(options.openMarkers.begin(), options.openMarkers.end(), markerName) != options.openMarkers.end();
        /*--- A tiny missing part (below kCentroidFraction of the piece) has no reliable centroid of its own: the value
         * at the piece centroid is used (its weight is negligible), without the distance statistics and limit. ---*/
        const bool tiny = !(missing > kCentroidFraction * pieceVolume);
        if (stencil.beyondLimit && !tiny) {
          nBeyond++;
          const passivedouble ratio =
              SU2_TYPE::GetValue(stencil.distance / donorLocator->GetDistanceLimit(stencil.faceSize));
          if (ratio > worstRatio) {
            worstRatio = ratio;
            std::copy(xp, xp + 3, worstX);
          }
        }
        if (stencil.onFace && !tiny) {
          summary.maxFillDistance = std::max(summary.maxFillDistance, SU2_TYPE::GetValue(stencil.distance));
          summary.maxFillRelDistance =
              std::max(summary.maxFillRelDistance, SU2_TYPE::GetValue(stencil.distance / stencil.faceSize));
        }
        for (unsigned short f = 0; f < nField; ++f) {
          passivedouble value = 0.0;
          for (unsigned short k = 0; k < stencil.nPoint; ++k)
            value += SU2_TYPE::GetValue(stencil.weight[k]) * U[stencil.point[k] * nField + f];
          rhs[iPoint * nField + f] += missing * value;
          summary.fill[f] += missing * value;
          if (open) summary.fillOpen[f] += missing * value;
        }
        UpdateBounds(iPoint, stencil.point, stencil.nPoint);
        summary.nFillPieces++;
        if (names.empty()) summary.nInteriorFill++;
        summary.fillVolume += missing;
        filledNode[iPoint] = true;
      }
    }
    summary.nFillNodes = std::count(filledNode.begin(), filledNode.end(), true);
    if (nBeyond > 0) {
      SU2_MPI::Error(std::to_string(nBeyond) +
                         " parts of control volumes of the new mesh are farther from the donor "
                         "mesh than accepted (max(face size, 2 ADAP_HAUSD, 1e-3 x domain size)), the farthest at " +
                         PointText(nDim, worstX) + ". The two meshes do not describe the same domain.",
                     CURRENT_FUNCTION);
    }
  }

  /*--- Donor domain outside the new one (S_d): content of the uncovered part of every donor element. ---*/
  for (auto jElem = 0ul; jElem < nElemD; ++jElem) {
    const passivedouble volume = volElemD[jElem];
    const passivedouble missing = volume - covD[jElem];
    if (!(volume > 0.0) || !(missing > kGapFraction * volume)) continue;
    const auto* nodesD = &elemD[jElem * nNode];
    passivedouble y[4][3] = {}, G[4][3] = {}, a[4] = {}, centroid[3] = {};
    for (unsigned short k = 0; k < nNode; ++k)
      for (unsigned short iDim = 0; iDim < nDim; ++iDim) {
        y[k][iDim] = coordD[nodesD[k] * nDim + iDim] - coordD[nodesD[0] * nDim + iDim];
        centroid[iDim] += y[k][iDim] / nNode;
      }
    SimplexPlanes(nDim, y, G, a);
    passivedouble c[3] = {}, mu[4] = {};
    su2double x[3] = {};
    for (unsigned short iDim = 0; iDim < nDim; ++iDim) {
      c[iDim] = (missing > kCentroidFraction * volume) ? (volume * centroid[iDim] - momD[jElem * 3 + iDim]) / missing
                                                       : centroid[iDim];
      x[iDim] = c[iDim] + coordD[nodesD[0] * nDim + iDim];
    }
    for (unsigned short k = 0; k < nNode; ++k) {
      mu[k] = a[k];
      for (unsigned short iDim = 0; iDim < nDim; ++iDim) mu[k] += G[k][iDim] * c[iDim];
    }
    /*--- The boundary this part lies at: the marker of the nearest donor face. ---*/
    std::string name;
    donorLocator->LocateOnBoundary(x, donorNames, &name);
    const bool open =
        std::find(options.openMarkers.begin(), options.openMarkers.end(), name) != options.openMarkers.end();
    std::vector<passivedouble> content(nField, 0.0);
    for (unsigned short f = 0; f < nField; ++f) {
      for (unsigned short k = 0; k < nNode; ++k) content[f] += missing * mu[k] * U[nodesD[k] * nField + f];
      summary.sliver[f] += content[f];
      if (open) summary.sliverOpen[f] += content[f];
    }
    summary.nSliverElems++;
    summary.sliverVolume += missing;

    if (options.sliverRule == SliverRule::BOUNDARY) {
      const auto stencil = (!name.empty() && targetLocator->HasMarker(name))
                               ? targetLocator->LocateOnBoundary(x, {name})
                               : targetLocator->Locate(x);
      if (stencil.onFace && missing > kCentroidFraction * volume) {
        summary.maxSliverDistance = std::max(summary.maxSliverDistance, SU2_TYPE::GetValue(stencil.distance));
        summary.maxSliverRelDistance =
            std::max(summary.maxSliverRelDistance, SU2_TYPE::GetValue(stencil.distance / stencil.faceSize));
      }
      for (unsigned short k = 0; k < stencil.nPoint; ++k) {
        const auto iPoint = stencil.point[k];
        const passivedouble w = SU2_TYPE::GetValue(stencil.weight[k]);
        for (unsigned short f = 0; f < nField; ++f) rhs[iPoint * nField + f] += w * content[f];
        UpdateBounds(iPoint, nodesD, nNode);
      }
    }
  }

  /*--- Exact totals: the donor total minus the right-hand side (the S_n content, the S_d content left to it, and
   *    round-off) is spread over the new domain by volume (a uniform shift of the solution, M 1 = |C|). ---*/

  passivedouble sumRowVolume = 0.0;
  for (auto iPoint = 0ul; iPoint < nPointT; ++iPoint) {
    sumRowVolume += rowVolume[iPoint];
    summary.targetCV += cvT[iPoint];
  }
  for (auto iPoint = 0ul; iPoint < nPointD; ++iPoint) summary.donorCV += cvD[iPoint];

  summary.donorTotal.assign(nField, 0.0);
  summary.targetTotal.assign(nField, 0.0);
  summary.scale.assign(nField, 0.0);
  summary.correction.assign(nField, 0.0);
  summary.supermeshDefect.assign(nField, 0.0);
  range.assign(nField, 0.0);
  for (unsigned short f = 0; f < nField; ++f) {
    passivedouble minValue = kInf, maxValue = -kInf, rhsTotal = 0.0;
    for (auto iPoint = 0ul; iPoint < nPointD; ++iPoint) {
      const auto value = U[iPoint * nField + f];
      summary.donorTotal[f] += value * cvD[iPoint];
      summary.scale[f] += fabs(value) * cvD[iPoint];
      minValue = std::min(minValue, value);
      maxValue = std::max(maxValue, value);
    }
    range[f] = maxValue - minValue;
    for (auto iPoint = 0ul; iPoint < nPointT; ++iPoint) rhsTotal += rhs[iPoint * nField + f];
    /*--- Total of the new field: the donor total, or with NONE the content of the common domain plus S_n. ---*/
    summary.targetTotal[f] = summary.donorTotal[f];
    if (options.sliverRule == SliverRule::NONE) summary.targetTotal[f] += summary.fill[f] - summary.sliver[f];
    if (options.sliverRule == SliverRule::CLOSED) summary.targetTotal[f] += summary.fillOpen[f] - summary.sliverOpen[f];
    summary.correction[f] = summary.targetTotal[f] - rhsTotal;
    passivedouble expected = 0.0;
    switch (options.sliverRule) {
      case SliverRule::BOUNDARY:
        expected = -summary.fill[f];
        break;
      case SliverRule::GLOBAL:
        expected = summary.sliver[f] - summary.fill[f];
        break;
      case SliverRule::NONE:
        expected = 0.0;
        break;
      case SliverRule::CLOSED:
        expected = (summary.sliver[f] - summary.sliverOpen[f]) - (summary.fill[f] - summary.fillOpen[f]);
        break;
    }
    const passivedouble scale = std::max(summary.scale[f], passivedouble(1e-300));
    summary.supermeshDefect[f] = (summary.correction[f] - expected) / scale;
    for (auto iPoint = 0ul; iPoint < nPointT; ++iPoint)
      rhs[iPoint * nField + f] += summary.correction[f] * rowVolume[iPoint] / sumRowVolume;
  }
  summary.timeSlivers = SU2_TYPE::GetValue(SU2_MPI::Wtime() - start);

  /*--- Solve M u = S per field, then limit with the exact total. ---*/

  newValues.assign(nPointT * nField, 0.0);
  summary.iterations.assign(nField, 0);
  summary.residual.assign(nField, 0.0);
  summary.nLimited.assign(nField, 0);
  summary.infeasible.assign(nField, false);
  summary.newTotal.assign(nField, 0.0);
  std::vector<passivedouble> b(nPointT), x(nPointT), lo(nPointT), hi(nPointT);
  passivedouble solveTime = 0.0, limiterTime = 0.0;
  for (unsigned short f = 0; f < nField; ++f) {
    start = SU2_MPI::Wtime();
    for (auto iPoint = 0ul; iPoint < nPointT; ++iPoint) b[iPoint] = rhs[iPoint * nField + f];
    SolveMass(b, x, summary.iterations[f], summary.residual[f]);
    solveTime += SU2_TYPE::GetValue(SU2_MPI::Wtime() - start);
    start = SU2_MPI::Wtime();

    const passivedouble widen = options.limiterTolerance * range[f];
    for (auto iPoint = 0ul; iPoint < nPointT; ++iPoint) {
      lo[iPoint] = options.limiter ? lower[iPoint * nField + f] - widen : -kInf;
      hi[iPoint] = options.limiter ? upper[iPoint * nField + f] + widen : kInf;
      if (std::isinf(lo[iPoint]) || std::isinf(hi[iPoint])) {
        lo[iPoint] = -kInf;
        hi[iPoint] = kInf;
      }
    }
    summary.infeasible[f] = !BoundedRedistribute(x, f, summary.targetTotal[f], lo, hi, nullptr, &summary.nLimited[f]);
    for (auto iPoint = 0ul; iPoint < nPointT; ++iPoint) newValues[iPoint * nField + f] = x[iPoint];
    summary.newTotal[f] = NewTotal(f, newValues);
    limiterTime += SU2_TYPE::GetValue(SU2_MPI::Wtime() - start);
  }
  summary.timeSolve = solveTime;
  summary.timeLimiter = limiterTime;
}

passivedouble CConservativeProjection::GetMean(unsigned long iPoint, unsigned short iField) const {
  return rhs[iPoint * nField + iField] / rowVolume[iPoint];
}

passivedouble CConservativeProjection::NewTotal(unsigned short iField, const std::vector<passivedouble>& values) const {
  passivedouble total = 0.0;
  for (auto iPoint = 0ul; iPoint < nPointT; ++iPoint) total += values[iPoint * nField + iField] * cvT[iPoint];
  return total;
}

bool CConservativeProjection::Redistribute(unsigned short iField, std::vector<passivedouble>& newValues,
                                           const std::vector<bool>& frozen) const {
  std::vector<passivedouble> v(nPointT), lo(nPointT), hi(nPointT);
  const passivedouble widen = options.limiterTolerance * range[iField];
  for (auto iPoint = 0ul; iPoint < nPointT; ++iPoint) {
    v[iPoint] = newValues[iPoint * nField + iField];
    lo[iPoint] = options.limiter ? lower[iPoint * nField + iField] - widen : -kInf;
    hi[iPoint] = options.limiter ? upper[iPoint * nField + iField] + widen : kInf;
    if (std::isinf(lo[iPoint]) || std::isinf(hi[iPoint])) {
      lo[iPoint] = -kInf;
      hi[iPoint] = kInf;
    }
    /*--- Values already outside their bounds are not pulled back here. ---*/
    lo[iPoint] = std::min(lo[iPoint], v[iPoint]);
    hi[iPoint] = std::max(hi[iPoint], v[iPoint]);
  }
  const bool ok = BoundedRedistribute(v, iField, summary.targetTotal[iField], lo, hi, &frozen, nullptr);
  for (auto iPoint = 0ul; iPoint < nPointT; ++iPoint) newValues[iPoint * nField + iField] = v[iPoint];
  return ok;
}

void CConservativeTransfer::Transfer(CConfig* config, const CMeshDonor& donor, CGeometry** geometry,
                                     CSolver*** solver) {
  SU2_ZONE_SCOPED

  const auto start = SU2_MPI::Wtime();
  const int rank = SU2_MPI::GetRank();
  summary = Summary();

  const auto arrays = CheckProblem("conservative", config, donor, geometry, solver);
  auto* donorGeometry = donor.geometry[MESH_0];
  auto* newGeometry = geometry[MESH_0];
  const auto nDim = newGeometry->GetnDim();
  const auto nPoint = newGeometry->GetnPoint();
  const auto nPointDonor = donorGeometry->GetnPoint();
  summary.nPoint = nPoint;
  summary.nVarFlow = nDim + 2;
  summary.interpolateTimeN1 = arrays.interpolateTimeN1;
  summary.nTimeLevels = arrays.hasTimeN + arrays.hasTimeN1;

  auto* flowSolver = solver[MESH_0][FLOW_SOL];
  const auto nVarFlow = flowSolver->GetnVar();
  auto* fluidModel = flowSolver->GetFluidModel();
  if (fluidModel == nullptr || nVarFlow != nDim + 2) {
    SU2_MPI::Error("The flow solver is not a compressible flow solver.", CURRENT_FUNCTION);
  }
  const auto* turbSolver = dynamic_cast<const CTurbSolver*>(solver[MESH_0][TURB_SOL]);
  if (solver[MESH_0][TURB_SOL] != nullptr && turbSolver == nullptr) {
    SU2_MPI::Error("Unexpected turbulence solver.", CURRENT_FUNCTION);
  }
  const unsigned short nVarTurb = turbSolver ? turbSolver->GetnVar() : 0;
  /*--- SST transports rho k, rho omega in conservation form (CTurbSSTSolver is "Conservative"), SA nu_tilde. ---*/
  const bool turbTimesDensity = turbSolver && TurbModelFamily(config->GetKind_Turb_Model()) == TURB_FAMILY::KW;

  /*--- Fields: per time level (U^n, and U^(n-1) for 2nd order) the flow variables and the turbulence variables. ---*/

  const unsigned short nLevel = arrays.interpolateTimeN1 ? 2 : 1;
  const unsigned short nPerLevel = nVarFlow + nVarTurb;
  const unsigned short nField = nLevel * nPerLevel;
  for (unsigned short iLevel = 0; iLevel < nLevel; ++iLevel) {
    const string suffix = iLevel ? " (n-1)" : "";
    summary.names.push_back("Density" + suffix);
    for (unsigned short iDim = 0; iDim < nDim; ++iDim)
      summary.names.push_back(string("Momentum-") + "xyz"[iDim] + suffix);
    summary.names.push_back("Energy" + suffix);
    for (unsigned short iVar = 0; iVar < nVarTurb; ++iVar) {
      if (!turbTimesDensity) {
        summary.names.push_back("Nu_Tilde" + suffix);
      } else {
        summary.names.push_back((iVar == 0 ? "rho*k" : "rho*omega") + suffix);
      }
    }
  }

  auto flowArray = [&](CSolver** solvers, unsigned short iLevel) -> su2activematrix& {
    auto* nodes = solvers[FLOW_SOL]->GetNodes();
    return iLevel == 0 ? nodes->GetSolution() : nodes->GetSolution_time_n1();
  };
  auto turbArray = [&](CSolver** solvers, unsigned short iLevel) -> su2activematrix& {
    auto* nodes = solvers[TURB_SOL]->GetNodes();
    return iLevel == 0 ? nodes->GetSolution() : nodes->GetSolution_time_n1();
  };

  std::vector<passivedouble> donorValues(nPointDonor * nField);
  for (unsigned short iLevel = 0; iLevel < nLevel; ++iLevel) {
    const auto& flow = flowArray(donor.solver[MESH_0], iLevel);
    for (auto iPoint = 0ul; iPoint < nPointDonor; ++iPoint) {
      auto* values = &donorValues[iPoint * nField + iLevel * nPerLevel];
      for (unsigned short iVar = 0; iVar < nVarFlow; ++iVar) values[iVar] = SU2_TYPE::GetValue(flow(iPoint, iVar));
      if (nVarTurb == 0) continue;
      const auto& turb = turbArray(donor.solver[MESH_0], iLevel);
      const passivedouble factor = turbTimesDensity ? values[0] : 1.0;
      for (unsigned short iVar = 0; iVar < nVarTurb; ++iVar)
        values[nVarFlow + iVar] = factor * SU2_TYPE::GetValue(turb(iPoint, iVar));
    }
  }

  /*--- Projection. ---*/

  std::vector<std::string> newTags;
  for (unsigned short iMarker = 0; iMarker < newGeometry->GetnMarker(); ++iMarker)
    newTags.push_back(config->GetMarker_All_TagBound(iMarker));
  auto projectionOptions = options;
  /*--- Open boundaries (not walls or symmetry planes): far field, inlets, outlets, ... ---*/
  for (unsigned short iMarker = 0; iMarker < newGeometry->GetnMarker(); ++iMarker) {
    const auto kind = config->GetMarker_All_KindBC(iMarker);
    if (!config->GetSolid_Wall(iMarker) && kind != SYMMETRY_PLANE && kind != SEND_RECEIVE)
      projectionOptions.openMarkers.push_back(newTags[iMarker]);
  }
  projectionOptions.absoluteLimit =
      std::max(projectionOptions.absoluteLimit, 2.0 * SU2_TYPE::GetValue(config->GetAdap_Hausd()));
  CConservativeProjection projection(*donorGeometry, donor.markerTags, *newGeometry, newTags, projectionOptions);
  std::vector<passivedouble> newValues;
  projection.Project(nField, donorValues, newValues);

  /*--- No-slip walls: the projected momentum of a wall point is not zero (its control volume reaches into the moving
   *    fluid); the solver would set it to zero in its first iteration (keeping rho E), which is not conservative.
   *    Here it is set to zero (rho E kept, as the solver does) and the removed momentum is redistributed over the other
   *    points within the limiter bounds, so the momentum totals stay exact. ---*/

  std::vector<bool> wallPoint(nPoint, false);
  for (unsigned short iMarker = 0; iMarker < newGeometry->GetnMarker(); ++iMarker) {
    if (!config->GetViscous_Wall(iMarker)) continue;
    for (auto iVertex = 0ul; iVertex < newGeometry->GetnVertex(iMarker); ++iVertex)
      wallPoint[newGeometry->vertex[iMarker][iVertex]->GetNode()] = true;
  }
  summary.nWallPoints = std::count(wallPoint.begin(), wallPoint.end(), true);
  if (summary.nWallPoints > 0) {
    for (unsigned short iLevel = 0; iLevel < nLevel; ++iLevel) {
      const unsigned short offset = iLevel * nPerLevel;
      for (auto iPoint = 0ul; iPoint < nPoint; ++iPoint) {
        if (!wallPoint[iPoint]) continue;
        for (unsigned short iDim = 0; iDim < nDim; ++iDim) {
          auto& momentum = newValues[iPoint * nField + offset + 1 + iDim];
          summary.maxWallMomentum = std::max(summary.maxWallMomentum, fabs(momentum));
          momentum = 0.0;
        }
      }
      for (unsigned short iDim = 0; iDim < nDim; ++iDim)
        projection.Redistribute(offset + 1 + iDim, newValues, wallPoint);
    }
  }

  /*--- Admissibility of the complete states (flow and turbulence of a time level; the solver subtracts the SST k from
   *    the internal energy): control-volume mean of all fields of the level, else the flow and turbulence states of the
   *    admissible donor point of largest weight at the point; the integral change is redistributed over the other
   *    points. ---*/

  std::unique_ptr<CBarycentricLocator> locator;
  for (unsigned short iLevel = 0; iLevel < nLevel; ++iLevel) {
    const unsigned short offset = iLevel * nPerLevel;
    const auto& donorFlow = flowArray(donor.solver[MESH_0], iLevel);
    auto& nFixed = iLevel ? summary.nHistoryFixed : summary.nFlowFixed;
    std::vector<bool> fixed(nPoint, false);
    su2double state[8] = {};
    auto admissible = [&](unsigned long iPoint) {
      const auto* values = &newValues[iPoint * nField + offset];
      for (unsigned short iVar = 0; iVar < nVarFlow; ++iVar) state[iVar] = values[iVar];
      const su2double k = (turbTimesDensity && values[0] > 0.0) ? su2double(values[nVarFlow] / values[0]) : su2double(0.0);
      return CBarycentricTransfer::AdmissibleState(*fluidModel, nDim, state, k);
    };
    for (auto iPoint = 0ul; iPoint < nPoint; ++iPoint) {
      if (admissible(iPoint)) continue;
      nFixed++;
      fixed[iPoint] = true;
      for (unsigned short iVar = 0; iVar < nPerLevel; ++iVar)
        newValues[iPoint * nField + offset + iVar] = projection.GetMean(iPoint, offset + iVar);
      if (admissible(iPoint)) continue;
      if (!locator) locator = std::make_unique<CBarycentricLocator>(*donorGeometry, donor.markerTags, 1e300);
      const auto stencil = locator->Locate(newGeometry->nodes->GetCoord(iPoint));
      const su2activematrix* donorTurb = nVarTurb ? &turbArray(donor.solver[MESH_0], iLevel) : nullptr;
      int best = -1;
      for (unsigned short k = 0; k < stencil.nPoint; ++k) {
        if (best >= 0 && stencil.weight[k] <= stencil.weight[best]) continue;
        const auto jPoint = stencil.point[k];
        const su2double k0 = turbTimesDensity ? (*donorTurb)(jPoint, 0) : su2double(0.0);
        if (CBarycentricTransfer::AdmissibleState(*fluidModel, nDim, donorFlow[jPoint], k0)) best = k;
      }
      if (best < 0) {
        SU2_MPI::Error("No admissible flow state for the point " + std::to_string(iPoint) + " of the new mesh.",
                       CURRENT_FUNCTION);
      }
      /*--- Paired: flow and turbulence of the same donor point. ---*/
      const auto jPoint = stencil.point[best];
      for (unsigned short iVar = 0; iVar < nVarFlow; ++iVar)
        newValues[iPoint * nField + offset + iVar] = SU2_TYPE::GetValue(donorFlow(jPoint, iVar));
      for (unsigned short iVar = 0; iVar < nVarTurb; ++iVar) {
        const passivedouble factor = turbTimesDensity ? SU2_TYPE::GetValue(donorFlow(jPoint, 0)) : 1.0;
        newValues[iPoint * nField + offset + nVarFlow + iVar] = factor * SU2_TYPE::GetValue((*donorTurb)(jPoint, iVar));
      }
    }
    if (std::find(fixed.begin(), fixed.end(), true) != fixed.end()) {
      for (unsigned short iVar = 0; iVar < nPerLevel; ++iVar) projection.Redistribute(offset + iVar, newValues, fixed);
      for (auto iPoint = 0ul; iPoint < nPoint; ++iPoint) {
        if (fixed[iPoint] || admissible(iPoint)) continue;
        /*--- Made inadmissible by the redistribution (not expected): its mean, the total is then not exact. ---*/
        nFixed++;
        for (unsigned short iVar = 0; iVar < nPerLevel; ++iVar)
          newValues[iPoint * nField + offset + iVar] = projection.GetMean(iPoint, offset + iVar);
      }
    }
  }

  /*--- Set the arrays of the new mesh: flow, turbulence (divided by the projected density for SST, then limited to the
   *    bounds of the solver). ---*/

  for (unsigned short iLevel = 0; iLevel < nLevel; ++iLevel) {
    const unsigned short offset = iLevel * nPerLevel;
    auto& flow = flowArray(solver[MESH_0], iLevel);
    for (auto iPoint = 0ul; iPoint < nPoint; ++iPoint)
      for (unsigned short iVar = 0; iVar < nVarFlow; ++iVar)
        flow(iPoint, iVar) = newValues[iPoint * nField + offset + iVar];
    if (nVarTurb == 0) continue;
    auto& turb = turbArray(solver[MESH_0], iLevel);
    for (auto iPoint = 0ul; iPoint < nPoint; ++iPoint) {
      const passivedouble density = newValues[iPoint * nField + offset];
      for (unsigned short iVar = 0; iVar < nVarTurb; ++iVar) {
        su2double value = newValues[iPoint * nField + offset + nVarFlow + iVar] / (turbTimesDensity ? density : 1.0);
        const su2double lowerLimit = turbSolver->GetLowerLimit(iVar), upperLimit = turbSolver->GetUpperLimit(iVar);
        if (value < lowerLimit || value > upperLimit) {
          const su2double limit = (value < lowerLimit) ? lowerLimit : upperLimit;
          if (fabs(value - limit) > 1e-10 * fabs(limit)) summary.nTurbLimited++;
          value = limit;
        }
        turb(iPoint, iVar) = value;
      }
    }
  }
  for (const auto iSol : arrays.solverIndices) {
    auto* nodes = solver[MESH_0][iSol]->GetNodes();
    if (arrays.hasTimeN) nodes->GetSolution_time_n() = nodes->GetSolution();
    if (arrays.hasTimeN1 && !arrays.interpolateTimeN1) nodes->GetSolution_time_n1() = nodes->GetSolution_time_n();
  }

  /*--- Integrals of the final state (as the solver sums them: value x control volume), per field. ---*/

  summary.donorIntegral.assign(nField, 0.0);
  summary.newIntegral.assign(nField, 0.0);
  summary.relativeDefect.assign(nField, 0.0);
  for (unsigned short iLevel = 0; iLevel < nLevel; ++iLevel) {
    const unsigned short offset = iLevel * nPerLevel;
    auto sums = [&](CGeometry* geo, CSolver** solvers, std::vector<passivedouble>& integral) {
      const auto& flow = flowArray(solvers, iLevel);
      for (auto iPoint = 0ul; iPoint < geo->GetnPointDomain(); ++iPoint) {
        const passivedouble volume = SU2_TYPE::GetValue(geo->nodes->GetVolume(iPoint));
        for (unsigned short iVar = 0; iVar < nVarFlow; ++iVar)
          integral[offset + iVar] += SU2_TYPE::GetValue(flow(iPoint, iVar)) * volume;
        if (nVarTurb == 0) continue;
        const auto& turb = turbArray(solvers, iLevel);
        const passivedouble factor = turbTimesDensity ? SU2_TYPE::GetValue(flow(iPoint, 0)) : 1.0;
        for (unsigned short iVar = 0; iVar < nVarTurb; ++iVar)
          integral[offset + nVarFlow + iVar] += factor * SU2_TYPE::GetValue(turb(iPoint, iVar)) * volume;
      }
    };
    sums(donorGeometry, donor.solver[MESH_0], summary.donorIntegral);
    sums(newGeometry, solver[MESH_0], summary.newIntegral);
    passivedouble momentumNorm = 0.0;
    for (unsigned short iDim = 0; iDim < nDim; ++iDim) momentumNorm += pow(summary.donorIntegral[offset + 1 + iDim], 2);
    momentumNorm = sqrt(momentumNorm);
    for (unsigned short f = offset; f < offset + nPerLevel; ++f) {
      const bool momentum = f > offset && f <= offset + nDim;
      const passivedouble scale = momentum ? momentumNorm : fabs(summary.donorIntegral[f]);
      summary.relativeDefect[f] =
          (scale > 0.0) ? (summary.newIntegral[f] - summary.donorIntegral[f]) / scale : passivedouble(0.0);
    }
  }
  summary.projection = projection.GetSummary();

  FinishTransfer(config, geometry, solver, arrays);
  summary.time = SU2_TYPE::GetValue(SU2_MPI::Wtime() - start);

  /*--- Log. ---*/

  if (rank != MASTER_NODE) return;
  const auto& p = summary.projection;
  cout << endl << "------------------- Solution Transfer (conservative P1) -------------------" << endl;
  cout << std::scientific << std::setprecision(3);
  cout << "Supermesh of " << p.nTargetElem << " new and " << p.nDonorElem << " donor elements: " << p.nPairs
       << " overlapping pairs (" << p.nTested << " clipped), measure " << p.overlapVolume << " of " << p.targetVolume
       << " (new) and " << p.donorVolume << " (donor)." << endl;
  cout << "New domain outside the donor (S_n): " << p.nFillPieces << " parts of control volumes at " << p.nFillNodes
       << " points (" << p.nInteriorFill << " of points on no marker), measure " << p.fillVolume
       << ", filled with the closest donor values (max distance " << p.maxFillDistance << ", " << p.maxFillRelDistance
       << " of the face); new elements outside the donor: " << p.nElemOutside << "." << endl;
  cout << "Donor domain outside the new one (S_d): " << p.nSliverElems << " donor elements, measure " << p.sliverVolume
       << (options.sliverRule == CConservativeProjection::SliverRule::BOUNDARY
               ? ", content added to the nearest new boundary points of the same marker (max distance " +
                     std::to_string(p.maxSliverDistance) + ")."
               : options.sliverRule == CConservativeProjection::SliverRule::GLOBAL
                     ? ", content kept: the totals are the donor's, the difference to the right-hand side is spread "
                       "over the new domain by volume."
                     : options.sliverRule == CConservativeProjection::SliverRule::CLOSED
                           ? ", at walls and symmetry planes content kept (the difference to the S_n content there "
                             "spread "
                             "over the new domain by volume), at open boundaries the totals follow the domain."
                           : ", content dropped: the totals change by the S_n minus the S_d content.")
       << endl;
  cout << "Largest uncovered fraction of a control-volume piece: " << p.maxUncovered << "." << endl;
  cout
      << "Per field: S_n content and S_d content (relative to sum |u| V), correction spread over the domain, supermesh "
         "defect (round-off), CG iterations and residual, values limited:"
      << endl;
  for (unsigned short f = 0; f < nField; ++f) {
    const passivedouble scale = std::max(p.scale[f], passivedouble(1e-300));
    cout << "  " << std::setw(18) << summary.names[f] << ": " << std::setw(10) << p.fill[f] / scale << std::setw(11)
         << p.sliver[f] / scale << std::setw(11) << p.correction[f] / scale << std::setw(11) << p.supermeshDefect[f]
         << std::setw(5) << p.iterations[f] << std::setw(11) << p.residual[f] << std::setw(8) << p.nLimited[f]
         << (p.infeasible[f] ? "  bounds infeasible, rest spread by volume" : "") << endl;
  }
  if (summary.nWallPoints > 0) {
    cout << "No-slip wall points: " << summary.nWallPoints << ", projected momentum set to zero (largest "
         << summary.maxWallMomentum << "), redistributed over the other points." << endl;
  }
  cout << "Flow states not admissible after the projection (fixed): " << summary.nFlowFixed;
  if (summary.interpolateTimeN1) cout << ", U^(n-1): " << summary.nHistoryFixed;
  cout << "." << endl;
  if (nVarTurb > 0)
    cout << "Turbulence values limited to the bounds of the solver: " << summary.nTurbLimited << "." << endl;
  if (summary.nTimeLevels > 0) {
    cout << "Time history: Solution_time_n = the solution (U^n, projected once)";
    if (summary.interpolateTimeN1) {
      cout << ", Solution_time_n1 = U^(n-1) projected on the same supermesh";
    } else if (summary.nTimeLevels > 1) {
      cout << ", Solution_time_n1 = U^n (not used by this time marching)";
    }
    cout << "." << endl;
  }
  cout << "Domain volume: donor " << p.donorCV << ", new " << p.targetCV << ", relative change "
       << (p.targetCV - p.donorCV) / p.donorCV << "." << endl;
  cout << "Change of the integrals (sum of value x control volume), relative to the donor integral (momentum: to the "
          "norm "
          "of the donor momentum integral):"
       << endl;
  for (unsigned short f = 0; f < nField; ++f)
    cout << "  " << std::setw(18) << summary.names[f] << ": " << summary.relativeDefect[f] << endl;
  cout.unsetf(std::ios_base::floatfield);
  cout << std::setprecision(4) << "Time: setup " << p.timeSetup << " s, supermesh " << p.timeSupermesh << " s, slivers "
       << p.timeSlivers << " s, solve " << p.timeSolve << " s, limiter " << p.timeLimiter << " s, total "
       << summary.time << " s." << endl;
  cout << std::setprecision(6);
}

CSolutionTransfer::Report CConservativeTransfer::GetReport() const {
  Report report;
  report.nOutside = summary.projection.nFillNodes;
  report.maxDistance = summary.projection.maxFillDistance;
  report.maxRelDistance = summary.projection.maxFillRelDistance;
  report.conservationDefect = 0.0;
  for (auto f = 0ul; f < summary.nVarFlow && f < summary.relativeDefect.size(); ++f)
    report.conservationDefect = std::max(report.conservationDefect, fabs(summary.relativeDefect[f]));
  return report;
}
