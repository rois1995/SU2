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
#include <functional>
#include <iomanip>
#include <iostream>
#include <limits>
#include <numeric>
#include <sstream>

#include "../../../Common/include/CConfig.hpp"
#include "../../../Common/include/geometry/CGeometry.hpp"
#include "../../../Common/include/adaptation/CMeshGather.hpp"
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

/*!
 * \brief Coupled, conservative recovery of admissible states on the new mesh (CConservativeTransfer).
 * \note For a point whose state is not admissible: the patch of the point and its neighbours (graph rings, grown
 *       until it works, up to the whole connected mesh) gets the volume-weighted mean of the patch as the reference
 *       state a (all fields of the time level together: flow and turbulence; the momentum mean over the points that
 *       are not on no-slip walls, a has zero momentum on them), and every state of the patch is blended towards it:
 *       u_j <- a_j + theta (u_j - a_j), one theta in [0, 1] for the patch: 0.9 of the largest for which every state of
 *       the patch is admissible (bisection per point; the margin keeps the states off zero pressure; then checked,
 *       halved if a state is still not admissible). Since
 *       sum_j |C_j| (u_j - a_j) = 0 the integrals of the patch are kept exactly, the wall momentum stays zero, and every
 *       new value lies between the values of the patch (no new extrema). The mean of admissible ideal-gas states is
 *       admissible (rho e is concave in (rho, rho u, rho E, rho k)), so a patch mean fails only next to strongly
 *       inadmissible states: the patch grows. If the mean of the whole mesh is not admissible, the recovery fails.
 */
template <class T>
class CAdmissibilityRecovery {
 public:
  using Predicate = std::function<bool(const T* state)>;

  CAdmissibilityRecovery(const CSimplexMesh& mesh, unsigned short nVar, const std::vector<bool>& wallPoint,
                         Predicate admissible)
      : controlVolume(mesh.volume),
        nDim(mesh.nDim),
        nVar(nVar),
        wallPoint(wallPoint),
        admissible(std::move(admissible)) {
    /*--- Neighbours of each point: the other points of its elements (the edges of a simplex mesh). ---*/
    const auto nPoint = mesh.GetnPoint();
    const unsigned short nNode = nDim + 1;
    std::vector<std::vector<unsigned long>> adjacency(nPoint);
    for (auto iElem = 0ul; iElem < mesh.GetnElem(); ++iElem)
      for (unsigned short k = 0; k < nNode; ++k)
        for (unsigned short l = 0; l < nNode; ++l)
          if (k != l) adjacency[mesh.elem[iElem * nNode + k]].push_back(mesh.elem[iElem * nNode + l]);
    neighborPtr.assign(nPoint + 1, 0);
    for (auto iPoint = 0ul; iPoint < nPoint; ++iPoint) {
      auto& row = adjacency[iPoint];
      std::sort(row.begin(), row.end());
      row.erase(std::unique(row.begin(), row.end()), row.end());
      neighborPtr[iPoint + 1] = neighborPtr[iPoint] + row.size();
    }
    neighbors.reserve(neighborPtr[nPoint]);
    for (const auto& row : adjacency) neighbors.insert(neighbors.end(), row.begin(), row.end());
    stamp.assign(nPoint, 0);
  }

  unsigned long maxRing = 0; /*!< \brief Largest patch radius (graph rings) needed. */
  unsigned long nPatches = 0;

  /*!
   * \brief Recover the patch around seed. The fields of a point start at values[iPoint * stride + offset].
   * \return False if not even the mean of the whole connected mesh is admissible (nothing changed then).
   */
  bool Recover(unsigned long seed, std::vector<T>& values, size_t stride, size_t offset) {
    tag++;
    std::vector<unsigned long> patch = {seed}, frontier = {seed}, next;
    stamp[seed] = tag;
    std::vector<T> ref(nVar), refWall(nVar), state(nVar);

    for (unsigned long ring = 1;; ++ring) {
      next.clear();
      for (const auto iPoint : frontier) {
        for (auto iNeigh = neighborPtr[iPoint]; iNeigh < neighborPtr[iPoint + 1]; ++iNeigh) {
          const auto jPoint = neighbors[iNeigh];
          if (stamp[jPoint] == tag) continue;
          stamp[jPoint] = tag;
          patch.push_back(jPoint);
          next.push_back(jPoint);
        }
      }
      const bool grown = !next.empty();
      frontier.swap(next);

      /*--- Reference states: volume-weighted means (momentum over the points off no-slip walls). ---*/
      passivedouble volume = 0.0, volumeOffWall = 0.0;
      bool anyWall = false;
      std::fill(ref.begin(), ref.end(), T(0.0));
      for (const auto iPoint : patch) {
        const passivedouble cv = controlVolume[iPoint];
        const bool wall = wallPoint[iPoint];
        anyWall |= wall;
        volume += cv;
        if (!wall) volumeOffWall += cv;
        const T* u = &values[iPoint * stride + offset];
        for (unsigned short iVar = 0; iVar < nVar; ++iVar) {
          if (Momentum(iVar) && wall) continue;
          ref[iVar] += cv * u[iVar];
        }
      }
      for (unsigned short iVar = 0; iVar < nVar; ++iVar) {
        const passivedouble v = Momentum(iVar) ? volumeOffWall : volume;
        ref[iVar] = (v > 0.0) ? T(ref[iVar] / v) : T(0.0);
        refWall[iVar] = Momentum(iVar) ? T(0.0) : ref[iVar];
      }
      if (!admissible(ref.data()) || (anyWall && !admissible(refWall.data()))) {
        if (!grown) return false;
        continue;
      }

      /*--- Largest blending factor: bisection for each state that is not admissible. ---*/
      passivedouble theta = 1.0;
      auto blend = [&](unsigned long iPoint, passivedouble t) {
        const T* a = wallPoint[iPoint] ? refWall.data() : ref.data();
        const T* u = &values[iPoint * stride + offset];
        for (unsigned short iVar = 0; iVar < nVar; ++iVar) state[iVar] = a[iVar] + t * (u[iVar] - a[iVar]);
        return admissible(state.data());
      };
      for (const auto iPoint : patch) {
        if (blend(iPoint, 1.0)) continue;
        passivedouble lo = 0.0, hi = 1.0;
        for (int iter = 0; iter < 60; ++iter) {
          const passivedouble mid = 0.5 * (lo + hi);
          (blend(iPoint, mid) ? lo : hi) = mid;
        }
        /*--- 0.9 of the boundary value: the state stays strictly inside (with a concave rho e at least 10% of the
         *    reference's internal energy), not at zero pressure where round-off decides. ---*/
        theta = std::min(theta, 0.9 * lo);
      }
      for (int attempt = 0; attempt < 60; ++attempt) {
        bool all = true;
        for (const auto iPoint : patch) all = all && blend(iPoint, theta);
        if (all) break;
        theta = (attempt < 59) ? 0.5 * theta : 0.0;
      }

      for (const auto iPoint : patch) {
        const T* a = wallPoint[iPoint] ? refWall.data() : ref.data();
        T* u = &values[iPoint * stride + offset];
        for (unsigned short iVar = 0; iVar < nVar; ++iVar) u[iVar] = a[iVar] + theta * (u[iVar] - a[iVar]);
      }
      maxRing = std::max(maxRing, ring);
      nPatches++;
      return true;
    }
  }

 private:
  bool Momentum(unsigned short iVar) const { return iVar >= 1 && iVar <= nDim; }

  const std::vector<passivedouble>& controlVolume;
  std::vector<unsigned long> neighborPtr, neighbors;
  unsigned short nDim, nVar;
  const std::vector<bool>& wallPoint;
  Predicate admissible;
  std::vector<unsigned long> stamp;
  unsigned long tag = 0;
};

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

namespace {
/*--- The mesh of a geometry without halo points (one rank), with its control volumes. ---*/
CSimplexMesh SerialMesh(const CGeometry& geometry, const std::vector<std::string>& tags) {
  if (geometry.GetnPoint() != geometry.GetnPointDomain()) {
    SU2_MPI::Error("The conservative projection of a geometry needs a mesh without halo points (one rank); with MPI it "
                   "works on the meshes gathered on one rank.", CURRENT_FUNCTION);
  }
  auto names = tags;
  names.resize(std::max<size_t>(names.size(), geometry.GetnMarker()), "");
  return CMeshGather::LocalMesh(geometry, names, true);
}
}  // namespace

CConservativeProjection::CConservativeProjection(const CGeometry& donor, const std::vector<std::string>& donorTags,
                                                 const CGeometry& target, const std::vector<std::string>& targetTags,
                                                 const Options& options)
    : CConservativeProjection(SerialMesh(donor, donorTags), SerialMesh(target, targetTags), options) {}

CConservativeProjection::CConservativeProjection(const CSimplexMesh& donor, const CSimplexMesh& target,
                                                 const Options& options)
    : options(options), nDim(target.nDim), nNode(target.nDim + 1) {
  const auto start = SU2_MPI::Wtime();
  if (donor.nDim != nDim) SU2_MPI::Error("The two meshes have different dimensions.", CURRENT_FUNCTION);
  if (donor.volume.size() != donor.GetnPoint() || target.volume.size() != target.GetnPoint()) {
    SU2_MPI::Error("The conservative projection needs the control volumes of both meshes.", CURRENT_FUNCTION);
  }

  auto readMesh = [&](const CSimplexMesh& mesh, std::vector<passivedouble>& coord, std::vector<unsigned long>& elem,
                      std::vector<passivedouble>& volElem, std::vector<passivedouble>& cv) {
    coord = mesh.coord;
    cv = mesh.volume;
    elem = mesh.elem;
    volElem.resize(mesh.GetnElem());
    for (auto iElem = 0ul; iElem < mesh.GetnElem(); ++iElem) {
      passivedouble y[4][3] = {}, G[4][3], a[4];
      for (unsigned short k = 0; k < nNode; ++k)
        for (unsigned short iDim = 0; iDim < nDim; ++iDim)
          y[k][iDim] = coord[elem[iElem * nNode + k] * nDim + iDim] - coord[elem[iElem * nNode] * nDim + iDim];
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
  donorLocator = std::make_unique<CBarycentricLocator>(donor, options.absoluteLimit);
  donorNames = donorLocator->GetMarkerNames();

  /*--- Target: points, elements, markers of each point. ---*/

  readMesh(target, coordT, elemT, volElemT, cvT);
  nPointT = target.GetnPoint();
  nElemT = target.GetnElem();
  pointMarkers.resize(nPointT);
  for (const auto& marker : target.markers) {
    if (marker.name.empty()) continue;
    for (const auto iPoint : marker.elem) {
      auto& names = pointMarkers[iPoint];
      if (std::find(names.begin(), names.end(), marker.name) == names.end()) names.push_back(marker.name);
    }
  }
  if (options.sliverRule == SliverRule::BOUNDARY) targetLocator = std::make_unique<CBarycentricLocator>(target);

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
      const su2double& value = U[donorPoints[k] * nField + f];
      su2double& lo = lower[iPoint * nField + f];
      su2double& hi = upper[iPoint * nField + f];
      if (value < lo) lo = value;
      if (value > hi) hi = value;
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
      su2double value = 0.0;
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

void CConservativeProjection::SolveMassActive(const std::vector<su2double>& b, std::vector<su2double>& x,
                                              unsigned long& iterations, passivedouble& residual) const {
  /*--- M is passive (geometry), x = M^-1 b is linear in b: the values are solved as passive numbers, and in forward
   *    mode (DIRECT_DIFF) the derivatives as well, x' = M^-1 b', to the same tolerance (instead of differentiating the
   *    iterations of the conjugate gradients). The transfer is never recorded on a reverse-mode tape (the adaptation
   *    is rejected with adjoint problems), so there the result is a passive value. ---*/
  const auto n = nPointT;
  std::vector<passivedouble> bValue(n), xValue;
  for (auto i = 0ul; i < n; ++i) bValue[i] = SU2_TYPE::GetValue(b[i]);
  SolveMass(bValue, xValue, iterations, residual);
  x.resize(n);
  for (auto i = 0ul; i < n; ++i) x[i] = xValue[i];
#ifdef CODI_FORWARD_TYPE
  std::vector<passivedouble> bDerivative(n), xDerivative;
  bool seeded = false;
  for (auto i = 0ul; i < n; ++i) {
    bDerivative[i] = SU2_TYPE::GetDerivative(b[i]);
    seeded |= (bDerivative[i] != 0.0);
  }
  if (seeded) {
    unsigned long derivativeIterations = 0;
    passivedouble derivativeResidual = 0.0;
    SolveMass(bDerivative, xDerivative, derivativeIterations, derivativeResidual);
    for (auto i = 0ul; i < n; ++i) SU2_TYPE::SetDerivative(x[i], xDerivative[i]);
  }
#endif
}

bool CConservativeProjection::BoundedRedistribute(std::vector<su2double>& v, unsigned short iField, su2double total,
                                                  const std::vector<su2double>& lo, const std::vector<su2double>& hi,
                                                  const std::vector<bool>* frozen, unsigned long* nClipped) const {
  const auto n = nPointT;
  auto isFree = [&](unsigned long i) { return frozen == nullptr || !(*frozen)[i]; };

  /*--- Clip to the bounds (count beyond round-off). ---*/
  if (nClipped != nullptr) *nClipped = 0;
  const passivedouble typical = summary.scale[iField] / std::max(summary.donorCV, passivedouble(1e-300));
  const passivedouble countTol = 1e-12 * std::max({SU2_TYPE::GetValue(range[iField]), typical, passivedouble(1e-300)});
  auto infinite = [](const su2double& value) { return std::isinf(SU2_TYPE::GetValue(value)); };
  for (auto i = 0ul; i < n; ++i) {
    if (!isFree(i)) continue;
    if (v[i] < lo[i] || v[i] > hi[i]) {
      const su2double bound = (v[i] < lo[i]) ? lo[i] : hi[i];
      if (nClipped != nullptr && fabs(v[i] - bound) > countTol) (*nClipped)++;
      v[i] = bound;
    }
  }

  /*--- Redistribute the defect over the nodes with room, in proportion to the room (iterated). ---*/
  passivedouble scale = 0.0;
  for (auto i = 0ul; i < n; ++i) scale += fabs(SU2_TYPE::GetValue(v[i])) * cvT[i];
  scale = std::max(scale, fabs(SU2_TYPE::GetValue(total)));
  for (int iter = 0; iter < 100; ++iter) {
    su2double sum = 0.0;
    for (auto i = 0ul; i < n; ++i) sum += v[i] * cvT[i];
    const su2double r = total - sum;
    if (!(fabs(r) > 1e-15 * scale)) return true;

    su2double capacity = 0.0;
    passivedouble unboundedVolume = 0.0;
    for (auto i = 0ul; i < n; ++i) {
      if (!isFree(i)) continue;
      const su2double room = (r > 0.0) ? hi[i] - v[i] : v[i] - lo[i];
      if (infinite(room)) {
        unboundedVolume += cvT[i];
      } else if (room > 0.0) {
        capacity += room * cvT[i];
      }
    }
    if (unboundedVolume > 0.0) {
      for (auto i = 0ul; i < n; ++i) {
        if (!isFree(i)) continue;
        const su2double room = (r > 0.0) ? hi[i] - v[i] : v[i] - lo[i];
        if (infinite(room)) v[i] += r / unboundedVolume;
      }
      continue;
    }
    if (!(capacity > 0.0)) break;
    su2double fraction = fabs(r) / capacity;
    if (fraction > 1.0) fraction = 1.0;
    for (auto i = 0ul; i < n; ++i) {
      if (!isFree(i)) continue;
      su2double room = (r > 0.0) ? hi[i] - v[i] : v[i] - lo[i];
      if (!(room > 0.0)) room = 0.0;
      v[i] += (r > 0.0 ? fraction : su2double(-fraction)) * room;
    }
  }

  /*--- The bounds cannot hold the total (or round-off is left): spread the rest over the free nodes by volume. ---*/
  su2double sum = 0.0;
  passivedouble freeVolume = 0.0;
  for (auto i = 0ul; i < n; ++i) {
    sum += v[i] * cvT[i];
    if (isFree(i)) freeVolume += cvT[i];
  }
  const su2double r = total - sum;
  if (freeVolume > 0.0)
    for (auto i = 0ul; i < n; ++i)
      if (isFree(i)) v[i] += r / freeVolume;
  return !(fabs(r) > 1e-12 * scale);
}

void CConservativeProjection::Project(unsigned short nFieldIn, const std::vector<su2double>& donorValues,
                                      std::vector<su2double>& newValues) {
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
  fillA.assign(nField, 0.0);
  sliverA.assign(nField, 0.0);
  fillOpenA.assign(nField, 0.0);
  sliverOpenA.assign(nField, 0.0);
  targetTotalA.assign(nField, 0.0);
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
          su2double value = 0.0;
          for (unsigned short k = 0; k < stencil.nPoint; ++k)
            value += SU2_TYPE::GetValue(stencil.weight[k]) * U[stencil.point[k] * nField + f];
          rhs[iPoint * nField + f] += missing * value;
          fillA[f] += missing * value;
          if (open) fillOpenA[f] += missing * value;
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
    std::vector<su2double> content(nField, 0.0);
    for (unsigned short f = 0; f < nField; ++f) {
      for (unsigned short k = 0; k < nNode; ++k) content[f] += missing * mu[k] * U[nodesD[k] * nField + f];
      sliverA[f] += content[f];
      if (open) sliverOpenA[f] += content[f];
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
    summary.fill[f] = SU2_TYPE::GetValue(fillA[f]);
    summary.sliver[f] = SU2_TYPE::GetValue(sliverA[f]);
    summary.fillOpen[f] = SU2_TYPE::GetValue(fillOpenA[f]);
    summary.sliverOpen[f] = SU2_TYPE::GetValue(sliverOpenA[f]);
    su2double minValue = kInf, maxValue = -kInf, rhsTotal = 0.0, donorTotal = 0.0;
    for (auto iPoint = 0ul; iPoint < nPointD; ++iPoint) {
      const su2double& value = U[iPoint * nField + f];
      donorTotal += value * cvD[iPoint];
      summary.scale[f] += fabs(SU2_TYPE::GetValue(value)) * cvD[iPoint];
      if (value < minValue) minValue = value;
      if (value > maxValue) maxValue = value;
    }
    range[f] = maxValue - minValue;
    for (auto iPoint = 0ul; iPoint < nPointT; ++iPoint) rhsTotal += rhs[iPoint * nField + f];
    /*--- Total of the new field: the donor total, or with NONE the content of the common domain plus S_n. ---*/
    targetTotalA[f] = donorTotal;
    if (options.sliverRule == SliverRule::NONE) targetTotalA[f] += fillA[f] - sliverA[f];
    if (options.sliverRule == SliverRule::CLOSED) targetTotalA[f] += fillOpenA[f] - sliverOpenA[f];
    const su2double correction = targetTotalA[f] - rhsTotal;
    summary.donorTotal[f] = SU2_TYPE::GetValue(donorTotal);
    summary.targetTotal[f] = SU2_TYPE::GetValue(targetTotalA[f]);
    summary.correction[f] = SU2_TYPE::GetValue(correction);
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
      rhs[iPoint * nField + f] += correction * rowVolume[iPoint] / sumRowVolume;
  }
  summary.timeSlivers = SU2_TYPE::GetValue(SU2_MPI::Wtime() - start);

  /*--- Solve M u = S per field, then limit with the exact total. ---*/

  newValues.assign(nPointT * nField, 0.0);
  summary.iterations.assign(nField, 0);
  summary.residual.assign(nField, 0.0);
  summary.nLimited.assign(nField, 0);
  summary.infeasible.assign(nField, false);
  summary.newTotal.assign(nField, 0.0);
  std::vector<su2double> b(nPointT), x(nPointT), lo(nPointT), hi(nPointT);
  passivedouble solveTime = 0.0, limiterTime = 0.0;
  for (unsigned short f = 0; f < nField; ++f) {
    start = SU2_MPI::Wtime();
    for (auto iPoint = 0ul; iPoint < nPointT; ++iPoint) b[iPoint] = rhs[iPoint * nField + f];
    SolveMassActive(b, x, summary.iterations[f], summary.residual[f]);
    solveTime += SU2_TYPE::GetValue(SU2_MPI::Wtime() - start);
    start = SU2_MPI::Wtime();

    SetBounds(f, lo, hi);
    summary.infeasible[f] = !BoundedRedistribute(x, f, targetTotalA[f], lo, hi, nullptr, &summary.nLimited[f]);
    for (auto iPoint = 0ul; iPoint < nPointT; ++iPoint) newValues[iPoint * nField + f] = x[iPoint];
    summary.newTotal[f] = SU2_TYPE::GetValue(NewTotal(f, newValues));
    limiterTime += SU2_TYPE::GetValue(SU2_MPI::Wtime() - start);
  }
  summary.timeSolve = solveTime;
  summary.timeLimiter = limiterTime;
}

void CConservativeProjection::SetBounds(unsigned short iField, std::vector<su2double>& lo,
                                        std::vector<su2double>& hi) const {
  /*--- Limiter bounds of each node, widened by the tolerance times the donor range; none without the limiter or for a
   *    node without bounds. ---*/
  const su2double widen = options.limiterTolerance * range[iField];
  lo.resize(nPointT);
  hi.resize(nPointT);
  for (auto iPoint = 0ul; iPoint < nPointT; ++iPoint) {
    lo[iPoint] = options.limiter ? su2double(lower[iPoint * nField + iField] - widen) : su2double(-kInf);
    hi[iPoint] = options.limiter ? su2double(upper[iPoint * nField + iField] + widen) : su2double(kInf);
    if (std::isinf(SU2_TYPE::GetValue(lo[iPoint])) || std::isinf(SU2_TYPE::GetValue(hi[iPoint]))) {
      lo[iPoint] = -kInf;
      hi[iPoint] = kInf;
    }
  }
}

su2double CConservativeProjection::NewTotal(unsigned short iField, const std::vector<su2double>& values) const {
  su2double total = 0.0;
  for (auto iPoint = 0ul; iPoint < nPointT; ++iPoint) total += values[iPoint * nField + iField] * cvT[iPoint];
  return total;
}

bool CConservativeProjection::Redistribute(unsigned short iField, std::vector<su2double>& newValues,
                                           const std::vector<bool>& frozen) const {
  std::vector<su2double> v(nPointT), lo, hi;
  SetBounds(iField, lo, hi);
  for (auto iPoint = 0ul; iPoint < nPointT; ++iPoint) {
    v[iPoint] = newValues[iPoint * nField + iField];
    /*--- Values already outside their bounds are not pulled back here. ---*/
    if (v[iPoint] < lo[iPoint]) lo[iPoint] = v[iPoint];
    if (v[iPoint] > hi[iPoint]) hi[iPoint] = v[iPoint];
  }
  const bool ok = BoundedRedistribute(v, iField, targetTotalA[iField], lo, hi, &frozen, nullptr);
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

  /*--- Both meshes (with their control volumes) and the donor fields on the master rank, in the global numbering of
   *    the meshes (CMeshGather); the projection runs there on the complete meshes (one supermesh, one solve with
   *    exact totals) and the new values go back to the ranks that own the points. The fields keep their derivatives
   *    (forward mode, DIRECT_DIFF): the projection is active in the field values, passive in the geometry and the
   *    mass matrix. ---*/

  const auto gatherStart = SU2_MPI::Wtime();
  std::vector<std::string> newTags;
  for (unsigned short iMarker = 0; iMarker < newGeometry->GetnMarker(); ++iMarker)
    newTags.push_back(config->GetMarker_All_TagBound(iMarker));
  const CMeshGather donorGather(*donorGeometry), newGather(*newGeometry);
  const auto donorMesh = donorGather.GatherMesh(*config, DonorMarkerTags("conservative", donor), true);
  const auto newMesh = newGather.GatherMesh(*config, newTags, true);
  std::vector<su2double> donorValues;
  {
    const auto nPointDomain = donorGeometry->GetnPointDomain();
    std::vector<su2double> local(nPointDomain * nField);
    for (unsigned short iLevel = 0; iLevel < nLevel; ++iLevel) {
      const auto& flow = flowArray(donor.solver[MESH_0], iLevel);
      for (auto iPoint = 0ul; iPoint < nPointDomain; ++iPoint) {
        auto* values = &local[iPoint * nField + iLevel * nPerLevel];
        for (unsigned short iVar = 0; iVar < nVarFlow; ++iVar) values[iVar] = flow(iPoint, iVar);
        if (nVarTurb == 0) continue;
        const auto& turb = turbArray(donor.solver[MESH_0], iLevel);
        for (unsigned short iVar = 0; iVar < nVarTurb; ++iVar)
          values[nVarFlow + iVar] = (turbTimesDensity ? values[0] : su2double(1.0)) * turb(iPoint, iVar);
      }
    }
    donorValues = donorGather.Gather(local.data(), nField);
  }
  const auto gatherTime = SU2_MPI::Wtime() - gatherStart;

  /*--- Final values of the new points (flow, turbulence divided by the density for SST), per time level. ---*/
  std::vector<su2double> finalValues;

  if (donorGather.IsRoot()) {
    const auto nPoint = newMesh.GetnPoint();
    const auto nPointDonor = donorMesh.GetnPoint();
    summary.nPoint = nPoint;

    /*--- Kinds of the markers by name (the config file defines them for every rank). ---*/
    auto viscousWall = [&](const string& name) {
      const auto kind = config->GetMarker_CfgFile_KindBC(name);
      return kind == HEAT_FLUX || kind == ISOTHERMAL || kind == HEAT_TRANSFER || kind == SMOLUCHOWSKI_MAXWELL ||
             kind == CHT_WALL_INTERFACE;
    };
    auto solidWall = [&](const string& name) {
      return viscousWall(name) || config->GetMarker_CfgFile_KindBC(name) == EULER_WALL;
    };

    /*--- Projection. Open boundaries (not walls or symmetry planes): far field, inlets, outlets, ... ---*/

    auto projectionOptions = options;
    for (const auto& marker : newMesh.markers) {
      if (!solidWall(marker.name) && config->GetMarker_CfgFile_KindBC(marker.name) != SYMMETRY_PLANE)
        projectionOptions.openMarkers.push_back(marker.name);
    }
    projectionOptions.absoluteLimit =
        std::max(projectionOptions.absoluteLimit, 2.0 * SU2_TYPE::GetValue(config->GetAdap_Hausd()));
    CConservativeProjection projection(donorMesh, newMesh, projectionOptions);
    std::vector<su2double> newValues;
    projection.Project(nField, donorValues, newValues);

    /*--- No-slip walls: the projected momentum of a wall point is not zero (its control volume reaches into the
     *    moving fluid); the solver would set it to zero in its first iteration (keeping rho E), which is not
     *    conservative. Here it is set to zero (rho E kept, as the solver does) and the removed momentum is
     *    redistributed over the other points within the limiter bounds, so the momentum totals stay exact. ---*/

    std::vector<bool> wallPoint(nPoint, false);
    for (const auto& marker : newMesh.markers) {
      if (!viscousWall(marker.name)) continue;
      for (const auto iPoint : marker.elem) wallPoint[iPoint] = true;
    }
    summary.nWallPoints = std::count(wallPoint.begin(), wallPoint.end(), true);
    if (summary.nWallPoints > 0) {
      for (unsigned short iLevel = 0; iLevel < nLevel; ++iLevel) {
        const unsigned short offset = iLevel * nPerLevel;
        for (auto iPoint = 0ul; iPoint < nPoint; ++iPoint) {
          if (!wallPoint[iPoint]) continue;
          for (unsigned short iDim = 0; iDim < nDim; ++iDim) {
            auto& momentum = newValues[iPoint * nField + offset + 1 + iDim];
            summary.maxWallMomentum = std::max(summary.maxWallMomentum, fabs(SU2_TYPE::GetValue(momentum)));
            momentum = 0.0;
          }
        }
        /*--- If the limiter bounds cannot hold the removed momentum, the rest is spread over the other points by
         *    volume (the totals stay exact, the bounds are exceeded): counted and reported. ---*/
        for (unsigned short iDim = 0; iDim < nDim; ++iDim)
          if (!projection.Redistribute(offset + 1 + iDim, newValues, wallPoint)) summary.nWallBoundsExceeded++;
      }
    }

    /*--- Turbulence within the bounds of the solver (the bounds it applies after each update; SST: k = (rho k) / rho,
     *    rho k set back). This is the only step that can change a turbulence integral (counted). Done before the
     *    admissibility, which then sees the final k. ---*/

    for (unsigned short iLevel = 0; iLevel < nLevel && nVarTurb > 0; ++iLevel) {
      const unsigned short offset = iLevel * nPerLevel;
      for (auto iPoint = 0ul; iPoint < nPoint; ++iPoint) {
        auto* values = &newValues[iPoint * nField + offset];
        const su2double density = values[0];
        if (turbTimesDensity && !(density > 0.0)) continue;  // not admissible, recovered below with rho k
        for (unsigned short iVar = 0; iVar < nVarTurb; ++iVar) {
          const su2double factor = turbTimesDensity ? density : su2double(1.0);
          const su2double value = values[nVarFlow + iVar] / factor;
          const su2double lower = turbSolver->GetLowerLimit(iVar), upper = turbSolver->GetUpperLimit(iVar);
          if (value < lower || value > upper) {
            const su2double limit = (value < lower) ? lower : upper;
            if (fabs(value - limit) > 1e-10 * fabs(limit)) summary.nTurbLimited++;
            values[nVarFlow + iVar] = factor * limit;
          }
        }
      }
    }

    /*--- Admissibility of the complete state of every point (flow and turbulence of a time level; the solver
     *    subtracts the SST k = (rho k) / rho from the internal energy), recovered by the coupled conservative blending
     *    of CAdmissibilityRecovery (integrals and wall momentum kept, no new extrema). The transfer stops if a state
     *    cannot be recovered. ---*/

    auto pointText = [&](unsigned long iPoint) { return PointText(nDim, &newMesh.coord[iPoint * nDim]); };
    su2double state[8] = {};
    auto admissibleState = [&](const su2double* values) {
      for (unsigned short iVar = 0; iVar < nVarFlow; ++iVar) state[iVar] = values[iVar];
      const su2double k =
          (turbTimesDensity && values[0] > 0.0) ? su2double(values[nVarFlow] / values[0]) : su2double(0.0);
      return CBarycentricTransfer::AdmissibleState(*fluidModel, nDim, state, k);
    };
    CAdmissibilityRecovery<su2double> recovery(newMesh, nPerLevel, wallPoint, admissibleState);
    for (unsigned short iLevel = 0; iLevel < nLevel; ++iLevel) {
      const unsigned short offset = iLevel * nPerLevel;
      auto& nFixed = iLevel ? summary.nHistoryFixed : summary.nFlowFixed;
      for (auto iPoint = 0ul; iPoint < nPoint; ++iPoint) {
        if (admissibleState(&newValues[iPoint * nField + offset])) continue;
        nFixed++;
        if (!recovery.Recover(iPoint, newValues, nField, offset)) {
          SU2_MPI::Error("The projected state of the point " + pointText(iPoint) + (iLevel ? " (U^(n-1))" : "") +
                             " is not admissible and cannot be recovered conservatively (not even the mean state of "
                             "the mesh is admissible).",
                         CURRENT_FUNCTION);
        }
      }
    }
    summary.nRecoveryPatches = recovery.nPatches;
    summary.maxRecoveryRing = recovery.maxRing;

    /*--- Final checks before the arrays are set: every state admissible, the wall momentum zero, the totals of the
     *    flow variables those of the projection. ---*/

    const auto& projectionSummary = projection.GetSummary();
    for (unsigned short iLevel = 0; iLevel < nLevel; ++iLevel) {
      const unsigned short offset = iLevel * nPerLevel;
      for (auto iPoint = 0ul; iPoint < nPoint; ++iPoint) {
        const auto* values = &newValues[iPoint * nField + offset];
        bool ok = admissibleState(values);
        if (wallPoint[iPoint])
          for (unsigned short iDim = 0; iDim < nDim; ++iDim) ok = ok && values[1 + iDim] == 0.0;
        if (!ok) {
          SU2_MPI::Error("The transferred state of the point " + pointText(iPoint) +
                             " is not admissible after the recovery.",
                         CURRENT_FUNCTION);
        }
      }
      for (unsigned short iVar = 0; iVar < nVarFlow; ++iVar) {
        const auto f = offset + iVar;
        const passivedouble defect =
            SU2_TYPE::GetValue(projection.NewTotal(f, newValues)) - projectionSummary.targetTotal[f];
        if (fabs(defect) > 1e-12 * std::max(projectionSummary.scale[f], passivedouble(1e-300))) {
          SU2_MPI::Error("The transfer did not keep the total of " + summary.names[f] + " (relative defect " +
                             std::to_string(defect / projectionSummary.scale[f]) + ").",
                         CURRENT_FUNCTION);
        }
      }
    }

    /*--- Values of the new arrays: flow, turbulence (SST: divided by the density). ---*/

    finalValues = newValues;
    for (unsigned short iLevel = 0; iLevel < nLevel && nVarTurb > 0 && turbTimesDensity; ++iLevel) {
      const unsigned short offset = iLevel * nPerLevel;
      for (auto iPoint = 0ul; iPoint < nPoint; ++iPoint) {
        const su2double density = newValues[iPoint * nField + offset];
        for (unsigned short iVar = 0; iVar < nVarTurb; ++iVar)
          finalValues[iPoint * nField + offset + nVarFlow + iVar] =
              newValues[iPoint * nField + offset + nVarFlow + iVar] / density;
      }
    }

    /*--- Integrals of the final state (as the solver sums them: value x control volume), per field. ---*/

    summary.donorIntegral.assign(nField, 0.0);
    summary.newIntegral.assign(nField, 0.0);
    summary.relativeDefect.assign(nField, 0.0);
    for (unsigned short iLevel = 0; iLevel < nLevel; ++iLevel) {
      const unsigned short offset = iLevel * nPerLevel;
      for (auto iPoint = 0ul; iPoint < nPointDonor; ++iPoint) {
        const passivedouble volume = donorMesh.volume[iPoint];
        const auto* values = &donorValues[iPoint * nField + offset];
        for (unsigned short iVar = 0; iVar < nPerLevel; ++iVar)
          summary.donorIntegral[offset + iVar] += SU2_TYPE::GetValue(values[iVar]) * volume;
      }
      for (auto iPoint = 0ul; iPoint < nPoint; ++iPoint) {
        const passivedouble volume = newMesh.volume[iPoint];
        const auto* values = &finalValues[iPoint * nField + offset];
        for (unsigned short iVar = 0; iVar < nVarFlow; ++iVar)
          summary.newIntegral[offset + iVar] += SU2_TYPE::GetValue(values[iVar]) * volume;
        const passivedouble factor = turbTimesDensity ? SU2_TYPE::GetValue(values[0]) : 1.0;
        for (unsigned short iVar = 0; iVar < nVarTurb; ++iVar)
          summary.newIntegral[offset + nVarFlow + iVar] +=
              factor * SU2_TYPE::GetValue(values[nVarFlow + iVar]) * volume;
      }
      passivedouble momentumNorm = 0.0;
      for (unsigned short iDim = 0; iDim < nDim; ++iDim)
        momentumNorm += pow(summary.donorIntegral[offset + 1 + iDim], 2);
      momentumNorm = sqrt(momentumNorm);
      for (unsigned short f = offset; f < offset + nPerLevel; ++f) {
        const bool momentum = f > offset && f <= offset + nDim;
        const passivedouble scale = momentum ? momentumNorm : fabs(summary.donorIntegral[f]);
        summary.relativeDefect[f] =
            (scale > 0.0) ? (summary.newIntegral[f] - summary.donorIntegral[f]) / scale : passivedouble(0.0);
      }
    }
    summary.projection = projection.GetSummary();
  }

  /*--- The new values to the ranks that own the points. ---*/

  const auto scatterStart = SU2_MPI::Wtime();
  {
    const auto nPointDomain = newGeometry->GetnPointDomain();
    std::vector<su2double> local(nPointDomain * nField);
    newGather.Scatter(finalValues, nField, local.data());
    for (unsigned short iLevel = 0; iLevel < nLevel; ++iLevel) {
      const unsigned short offset = iLevel * nPerLevel;
      auto& flow = flowArray(solver[MESH_0], iLevel);
      for (auto iPoint = 0ul; iPoint < nPointDomain; ++iPoint)
        for (unsigned short iVar = 0; iVar < nVarFlow; ++iVar)
          flow(iPoint, iVar) = local[iPoint * nField + offset + iVar];
      if (nVarTurb == 0) continue;
      auto& turb = turbArray(solver[MESH_0], iLevel);
      for (auto iPoint = 0ul; iPoint < nPointDomain; ++iPoint)
        for (unsigned short iVar = 0; iVar < nVarTurb; ++iVar)
          turb(iPoint, iVar) = local[iPoint * nField + offset + nVarFlow + iVar];
    }
  }
  for (const auto iSol : arrays.solverIndices) {
    auto* nodes = solver[MESH_0][iSol]->GetNodes();
    if (arrays.hasTimeN) nodes->GetSolution_time_n() = nodes->GetSolution();
    if (arrays.hasTimeN1 && !arrays.interpolateTimeN1) nodes->GetSolution_time_n1() = nodes->GetSolution_time_n();
  }
  const auto scatterTime = SU2_MPI::Wtime() - scatterStart;

  BroadcastSummary();

  FinishTransfer(config, geometry, solver, arrays);
  summary.time = SU2_TYPE::GetValue(SU2_MPI::Wtime() - start);

  if (rank == MASTER_NODE && SU2_MPI::GetSize() > 1) {
    cout << endl << "Conservative transfer: donor and new mesh with the donor solution gathered on rank "
         << MASTER_NODE << " (" << gatherTime << " s), new values sent to the ranks of their points (" << scatterTime
         << " s)." << endl;
  }

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
  if (summary.nWallBoundsExceeded > 0) {
    cout << "The limiter bounds could not hold the removed wall momentum of " << summary.nWallBoundsExceeded
         << " momentum field(s): the rest was spread over the domain by volume (totals exact)." << endl;
  }
  cout << "States not admissible after the projection (k subtracted with SST): " << summary.nFlowFixed;
  if (summary.interpolateTimeN1) cout << ", U^(n-1): " << summary.nHistoryFixed;
  cout << "; recovered by blending " << summary.nRecoveryPatches << " patch(es) towards their mean state (up to "
       << summary.maxRecoveryRing << " neighbour rings), integrals kept." << endl;
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

void CConservativeTransfer::BroadcastSummary() {
  if (SU2_MPI::GetSize() == 1) return;
  auto& s = summary;
  auto& p = s.projection;
  unsigned long counts[] = {s.nPoint, s.nFlowFixed, s.nHistoryFixed, s.nTurbLimited, s.nRecoveryPatches,
                            s.maxRecoveryRing, s.nWallBoundsExceeded, s.nWallPoints, p.nFillNodes,
                            s.relativeDefect.size()};
  SU2_MPI::Bcast(counts, 10, MPI_UNSIGNED_LONG, MASTER_NODE, SU2_MPI::GetComm());
  s.nPoint = counts[0];
  s.nFlowFixed = counts[1];
  s.nHistoryFixed = counts[2];
  s.nTurbLimited = counts[3];
  s.nRecoveryPatches = counts[4];
  s.maxRecoveryRing = counts[5];
  s.nWallBoundsExceeded = counts[6];
  s.nWallPoints = counts[7];
  p.nFillNodes = counts[8];
  const auto nField = counts[9];

  /*--- Doubles as su2double (MPI_DOUBLE is the active type in the AD builds). ---*/
  std::vector<su2double> values = {s.maxWallMomentum, p.maxFillDistance, p.maxFillRelDistance};
  for (auto f = 0ul; f < nField && SU2_MPI::GetRank() == MASTER_NODE; ++f) {
    values.push_back(s.relativeDefect[f]);
    values.push_back(s.donorIntegral[f]);
    values.push_back(s.newIntegral[f]);
  }
  values.resize(3 + 3 * nField);
  SU2_MPI::Bcast(values.data(), static_cast<int>(values.size()), MPI_DOUBLE, MASTER_NODE, SU2_MPI::GetComm());
  s.maxWallMomentum = SU2_TYPE::GetValue(values[0]);
  p.maxFillDistance = SU2_TYPE::GetValue(values[1]);
  p.maxFillRelDistance = SU2_TYPE::GetValue(values[2]);
  s.relativeDefect.resize(nField);
  s.donorIntegral.resize(nField);
  s.newIntegral.resize(nField);
  for (auto f = 0ul; f < nField; ++f) {
    s.relativeDefect[f] = SU2_TYPE::GetValue(values[3 + 3 * f]);
    s.donorIntegral[f] = SU2_TYPE::GetValue(values[4 + 3 * f]);
    s.newIntegral[f] = SU2_TYPE::GetValue(values[5 + 3 * f]);
  }
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
