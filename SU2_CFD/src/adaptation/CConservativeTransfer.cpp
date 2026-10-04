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
#include "../../../Common/include/adaptation/CDistributedSearch.hpp"
#include "../../../Common/include/adaptation/CMeshGather.hpp"
#include "../../../Common/include/parallelization/CPassiveComm.hpp"
#include "../../include/adaptation/CDistributedProjection.hpp"
#include "../../include/adaptation/CTransferAdmissibility.hpp"
#include "../../include/fluid/CFluidModel.hpp"
#include "../../include/solvers/CSolver.hpp"
#include "../../include/solvers/CTurbSolver.hpp"

namespace {

using namespace conservative;

constexpr passivedouble kInf = std::numeric_limits<passivedouble>::infinity();

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

/*--- Accurate sum of local terms (one rank, no communication). ---*/
su2double LocalSum(const std::vector<su2double>& terms) {
  CAccurateSumBatch batch;
  batch.AddActive(terms);
  batch.ReduceLocal();
  return batch.GetActive(0);
}
passivedouble LocalSumPassive(const std::vector<passivedouble>& terms) {
  CAccurateSumBatch batch;
  batch.Add(terms);
  batch.ReduceLocal();
  return batch.Get(0);
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
      const passivedouble* nodes[4] = {};
      for (unsigned short k = 0; k < nNode; ++k) nodes[k] = &coord[elem[iElem * nNode + k] * nDim];
      volElem[iElem] = SimplexMeasure(nDim, nodes);
    }
  };

  /*--- Donor: points, elements and their keys, the locator (ADT of the elements, canonical boundary). ---*/

  readMesh(donor, coordD, elemD, volElemD, cvD);
  nPointD = donor.GetnPoint();
  nElemD = donor.GetnElem();
  keysD.resize(nElemD);
  for (auto iElem = 0ul; iElem < nElemD; ++iElem) {
    uint64_t gids[4] = {};
    for (unsigned short k = 0; k < nNode; ++k) gids[k] = elemD[iElem * nNode + k];
    keysD[iElem] = MakeSimplexKey(gids, nNode);
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

  /*--- Mass matrix (exact for simplices). ---*/

  mass.Assemble(nDim, nPointT, nPointT, elemT, volElemT);
  for (auto iPoint = 0ul; iPoint < nPointT; ++iPoint) {
    if (!(mass.diag[iPoint] > 0.0)) {
      SU2_MPI::Error("A point of the new mesh has no control volume (unused point or degenerate elements).",
                     CURRENT_FUNCTION);
    }
  }

  summary.nTargetElem = nElemT;
  summary.nDonorElem = nElemD;
  summary.timeSetup = SU2_TYPE::GetValue(SU2_MPI::Wtime() - start);
}

CConservativeProjection::~CConservativeProjection() = default;

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

void CConservativeProjection::AccumulatePiece(void* context, unsigned short i, passivedouble volume,
                                              const passivedouble* centroid, const passivedouble* mu) {
  auto& self = *static_cast<CConservativeProjection*>(context);
  const auto nNode = self.nNode, nDim = self.nDim, nField = self.nField;
  const auto iElem = self.pairT, jElem = self.pairD;
  const auto* nodesD = &self.elemD[jElem * nNode];
  const auto iPoint = self.elemT[iElem * nNode + i];
  const auto& U = *self.donorField;
  for (unsigned short f = 0; f < nField; ++f) {
    su2double value = 0.0;
    for (unsigned short k = 0; k < nNode; ++k) value += mu[k] * U[nodesD[k] * nField + f];
    self.rhs[iPoint * nField + f] += volume * value;
  }
  const auto piece = iElem * nNode + i;
  self.covT[piece] += volume;
  self.covD[jElem] += volume;
  const auto* origin = &self.coordT[self.elemT[iElem * nNode] * nDim];
  for (unsigned short iDim = 0; iDim < nDim; ++iDim) {
    self.momT[piece * 3 + iDim] += volume * centroid[iDim];
    self.momD[jElem * 3 + iDim] += volume * (centroid[iDim] + origin[iDim] - self.coordD[nodesD[0] * nDim + iDim]);
  }
  self.UpdateBounds(iPoint, nodesD, nNode);
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

  /*--- Supermesh: every target element against every donor element whose inflated box intersects its box (ADT box
   *    query, exhaustive), in the order of the donor element keys. ---*/

  auto start = SU2_MPI::Wtime();
  {
    Frame frame;
    std::vector<unsigned long> candidates;
    for (auto iElem = 0ul; iElem < nElemT; ++iElem) {
      const passivedouble* nodes[4] = {};
      for (unsigned short k = 0; k < nNode; ++k) nodes[k] = &coordT[elemT[iElem * nNode + k] * nDim];
      SetFrame(nDim, nodes, frame);
      summary.targetVolume += frame.volume;
      if (!(frame.volume > 0.0)) {
        summary.nDegenerate++;
        continue;
      }
      su2double bbMin[3] = {}, bbMax[3] = {};
      for (unsigned short iDim = 0; iDim < nDim; ++iDim) {
        passivedouble lo = nodes[0][iDim], hi = nodes[0][iDim];
        for (unsigned short k = 1; k < nNode; ++k) {
          lo = std::min(lo, nodes[k][iDim]);
          hi = std::max(hi, nodes[k][iDim]);
        }
        bbMin[iDim] = lo;
        bbMax[iDim] = hi;
      }
      donorLocator->IntersectingElements(bbMin, bbMax, candidates);
      std::sort(candidates.begin(), candidates.end(),
                [this](unsigned long a, unsigned long b) { return keysD[a] < keysD[b]; });
      bool any = false;
      pairT = iElem;
      for (const auto jElem : candidates) {
        const passivedouble* donorNodes[4] = {};
        for (unsigned short k = 0; k < nNode; ++k) donorNodes[k] = &coordD[elemD[jElem * nNode + k] * nDim];
        pairD = jElem;
        bool clipped = false;
        const bool overlapped = Overlap(nDim, frame, donorNodes, volElemD[jElem], AccumulatePiece, this, &clipped);
        summary.nTested += clipped;
        summary.nPairs += overlapped;
        any |= overlapped;
      }
      if (!any) summary.nElemOutside++;
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
      const passivedouble* nodes[4] = {};
      for (unsigned short k = 0; k < nNode; ++k) nodes[k] = &coordT[elemT[iElem * nNode + k] * nDim];
      SetFrame(nDim, nodes, frame);
      if (!(frame.volume > 0.0)) continue;
      const passivedouble pieceVolume = frame.volume / nNode;
      for (unsigned short i = 0; i < nNode; ++i) {
        const auto piece = iElem * nNode + i;
        const passivedouble missing = pieceVolume - covT[piece];
        summary.maxUncovered = std::max(summary.maxUncovered, missing / pieceVolume);
        if (!(missing > kGapFraction * pieceVolume)) continue;

        /*--- Centroid of the piece (the element clipped by the piece planes), then of its missing part. ---*/
        passivedouble volume = 0.0, centroid[3] = {};
        PieceMoments(nDim, frame, i, volume, centroid);
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
    const passivedouble* donorNodes[4] = {};
    for (unsigned short k = 0; k < nNode; ++k) donorNodes[k] = &coordD[nodesD[k] * nDim];
    passivedouble xp[3] = {}, mu[4] = {};
    SliverCentroid(nDim, donorNodes, volume, missing, &momD[jElem * 3], xp, mu);
    su2double x[3] = {};
    for (unsigned short iDim = 0; iDim < nDim; ++iDim) x[iDim] = xp[iDim];
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

  /*--- Exact totals (accurate sums): the donor total minus the right-hand side (the S_n content, the S_d content left
   *    to it, and round-off) is spread over the new domain by volume (a uniform shift of the solution, M 1 = |C|). ---*/

  const passivedouble sumRowVolume = LocalSumPassive(mass.rowVolume);
  summary.targetCV = LocalSumPassive(cvT);
  summary.donorCV = LocalSumPassive(cvD);

  summary.donorTotal.assign(nField, 0.0);
  summary.targetTotal.assign(nField, 0.0);
  summary.scale.assign(nField, 0.0);
  summary.correction.assign(nField, 0.0);
  summary.supermeshDefect.assign(nField, 0.0);
  range.assign(nField, 0.0);
  std::vector<su2double> terms;
  std::vector<passivedouble> absTerms;
  for (unsigned short f = 0; f < nField; ++f) {
    summary.fill[f] = SU2_TYPE::GetValue(fillA[f]);
    summary.sliver[f] = SU2_TYPE::GetValue(sliverA[f]);
    summary.fillOpen[f] = SU2_TYPE::GetValue(fillOpenA[f]);
    summary.sliverOpen[f] = SU2_TYPE::GetValue(sliverOpenA[f]);
    su2double minValue = kInf, maxValue = -kInf;
    terms.resize(nPointD);
    absTerms.resize(nPointD);
    for (auto iPoint = 0ul; iPoint < nPointD; ++iPoint) {
      const su2double& value = U[iPoint * nField + f];
      terms[iPoint] = value * cvD[iPoint];
      absTerms[iPoint] = fabs(SU2_TYPE::GetValue(value)) * cvD[iPoint];
      if (value < minValue) minValue = value;
      if (value > maxValue) maxValue = value;
    }
    const su2double donorTotal = LocalSum(terms);
    summary.scale[f] = LocalSumPassive(absTerms);
    range[f] = maxValue - minValue;
    terms.resize(nPointT);
    for (auto iPoint = 0ul; iPoint < nPointT; ++iPoint) terms[iPoint] = rhs[iPoint * nField + f];
    const su2double rhsTotal = LocalSum(terms);
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
      rhs[iPoint * nField + f] += correction * mass.rowVolume[iPoint] / sumRowVolume;
  }
  summary.timeSlivers = SU2_TYPE::GetValue(SU2_MPI::Wtime() - start);

  /*--- Solve M u = S per field (guarded conjugate gradients), then limit with the exact total. ---*/

  newValues.assign(nPointT * nField, 0.0);
  summary.iterations.assign(nField, 0);
  summary.residual.assign(nField, 0.0);
  summary.nLimited.assign(nField, 0);
  summary.infeasible.assign(nField, false);
  summary.nViolations.assign(nField, 0);
  summary.maxViolation.assign(nField, 0.0);
  summary.newTotal.assign(nField, 0.0);
  const MassSolver solver(mass, nullptr, options.solverTolerance, options.maxSolverIter);
  std::vector<su2double> b(nPointT), x(nPointT), lo(nPointT), hi(nPointT);
  passivedouble solveTime = 0.0, limiterTime = 0.0;
  for (unsigned short f = 0; f < nField; ++f) {
    start = SU2_MPI::Wtime();
    for (auto iPoint = 0ul; iPoint < nPointT; ++iPoint) b[iPoint] = rhs[iPoint * nField + f];
    const auto solve = solver.SolveActive(b, x);
    summary.iterations[f] = solve.iterations;
    summary.residual[f] = solve.trueResidual;
    summary.nSolveWarnings += solve.warning;
    if (solve.failed) {
      SU2_MPI::Error("The mass-matrix solve of field " + std::to_string(f) + " failed (" +
                         (solve.nonfiniteRHS ? std::string("nonfinite right-hand side")
                                             : solve.breakdown ? std::string("breakdown")
                                                               : "true residual " + std::to_string(solve.trueResidual)) +
                         ", " + std::to_string(solve.iterations) + " iterations).",
                     CURRENT_FUNCTION);
    }
    solveTime += SU2_TYPE::GetValue(SU2_MPI::Wtime() - start);
    start = SU2_MPI::Wtime();

    SetBounds(f, lo, hi);
    const passivedouble typical = summary.scale[f] / std::max(summary.donorCV, passivedouble(1e-300));
    const passivedouble countTol =
        DiagnosticTol(1e-12) * std::max({SU2_TYPE::GetValue(range[f]), typical, passivedouble(1e-300)});
    const auto limited =
        BoundedRedistribute(x, cvT, targetTotalA[f], lo, hi, nullptr, countTol, SU2_TYPE::GetValue(range[f]), false);
    if (limited.error) SU2_MPI::Error("The limiter of field " + std::to_string(f) + ": " + limited.reason + ".", CURRENT_FUNCTION);
    summary.nLimited[f] = limited.nClipped;
    summary.infeasible[f] = limited.relaxed;
    summary.nViolations[f] = limited.nViolations;
    summary.maxViolation[f] = limited.maxViolation;
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
  std::vector<su2double> terms(nPointT);
  for (auto iPoint = 0ul; iPoint < nPointT; ++iPoint) terms[iPoint] = values[iPoint * nField + iField] * cvT[iPoint];
  return LocalSum(terms);
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
  const passivedouble typical = summary.scale[iField] / std::max(summary.donorCV, passivedouble(1e-300));
  const passivedouble countTol =
      DiagnosticTol(1e-12) * std::max({SU2_TYPE::GetValue(range[iField]), typical, passivedouble(1e-300)});
  const auto result = BoundedRedistribute(v, cvT, targetTotalA[iField], lo, hi, &frozen, countTol,
                                          SU2_TYPE::GetValue(range[iField]), false);
  if (result.error) SU2_MPI::Error("The redistribution of field " + std::to_string(iField) + ": " + result.reason + ".", CURRENT_FUNCTION);
  for (auto iPoint = 0ul; iPoint < nPointT; ++iPoint) newValues[iPoint * nField + iField] = v[iPoint];
  return !result.relaxed;
}

namespace {

/*--- Names of the projected fields (per time level: flow, then turbulence). ---*/
std::vector<std::string> FieldNames(unsigned short nDim, unsigned short nLevel, unsigned short nVarTurb, bool sst) {
  std::vector<std::string> names;
  for (unsigned short iLevel = 0; iLevel < nLevel; ++iLevel) {
    const string suffix = iLevel ? " (n-1)" : "";
    names.push_back("Density" + suffix);
    for (unsigned short iDim = 0; iDim < nDim; ++iDim) names.push_back(string("Momentum-") + "xyz"[iDim] + suffix);
    names.push_back("Energy" + suffix);
    for (unsigned short iVar = 0; iVar < nVarTurb; ++iVar)
      names.push_back((sst ? (iVar == 0 ? "rho*k" : "rho*omega") : "Nu_Tilde") + suffix);
  }
  return names;
}

/*--- Kinds of the markers by name (the config file defines them for every rank). ---*/
bool ViscousWall(const CConfig& config, const string& name) {
  const auto kind = config.GetMarker_CfgFile_KindBC(name);
  return kind == HEAT_FLUX || kind == ISOTHERMAL || kind == HEAT_TRANSFER || kind == SMOLUCHOWSKI_MAXWELL ||
         kind == CHT_WALL_INTERFACE;
}
bool OpenBoundary(const CConfig& config, const string& name) {
  const auto kind = config.GetMarker_CfgFile_KindBC(name);
  return !ViscousWall(config, name) && kind != EULER_WALL && kind != SYMMETRY_PLANE;
}

/*--- The turbulence values of the points of one level within the bounds of the solver (conservative representation:
 *    SST k = (rho k) / rho, rho k set back; points with rho <= 0 skipped), counted beyond round-off. ---*/
unsigned long BoundTurbulence(const CTransferAdmissibility& admissibility, std::vector<su2double>& values,
                              unsigned long nPoint, unsigned short nField, unsigned short offset) {
  unsigned long nLimited = 0;
  const auto nVarFlow = admissibility.GetnVarFlow(), nVarTurb = admissibility.GetnVarTurb();
  const bool sst = admissibility.IsSST();
  for (auto iPoint = 0ul; iPoint < nPoint; ++iPoint) {
    auto* fields = &values[iPoint * nField + offset];
    const su2double density = fields[0];
    if (sst && !(density > 0.0)) continue;
    for (unsigned short iVar = 0; iVar < nVarTurb; ++iVar) {
      const su2double factor = sst ? density : su2double(1.0);
      const su2double value = fields[nVarFlow + iVar] / factor;
      const su2double lower = admissibility.Lower(iVar), upper = admissibility.Upper(iVar);
      if (value < lower || value > upper) {
        const su2double limit = (value < lower) ? lower : upper;
        if (fabs(value - limit) > DiagnosticTol(1e-10) * fabs(limit)) nLimited++;
        fields[nVarFlow + iVar] = factor * limit;
      }
    }
  }
  return nLimited;
}

}  // namespace

void CConservativeTransfer::Transfer(CConfig* config, const CMeshDonor& donor, CGeometry** geometry,
                                     CSolver*** solver) {
  const int size = SU2_MPI::GetSize(), rank = SU2_MPI::GetRank();
  if (gathered || (options.sliverRule == CConservativeProjection::SliverRule::BOUNDARY && size == 1)) {
    TransferGathered(config, donor, geometry, solver);
    return;
  }
  if (options.sliverRule == CConservativeProjection::SliverRule::BOUNDARY) {
    SU2_MPI::Error("The BOUNDARY sliver rule of the conservative transfer needs one rank (developer option).",
                   CURRENT_FUNCTION);
  }
  SU2_ZONE_SCOPED

  const auto start = SU2_MPI::Wtime();
  summary = Summary();
  CTransferRoundScope rounds;

  const auto arrays = CheckProblem("conservative", config, donor, geometry, solver);
  auto* donorGeometry = donor.geometry[MESH_0];
  auto* newGeometry = geometry[MESH_0];
  const auto nDim = arrays.nDim;
  summary.nVarFlow = nDim + 2;
  summary.interpolateTimeN1 = arrays.interpolateTimeN1;
  summary.nTimeLevels = arrays.hasTimeN + arrays.hasTimeN1;

  auto* flowSolver = solver[MESH_0][FLOW_SOL];
  const auto nVarFlow = arrays.nVarFlow;
  auto* fluidModel = flowSolver->GetFluidModel();
  if (fluidModel == nullptr || nVarFlow != nDim + 2) {
    SU2_MPI::Error("The flow solver is not a compressible flow solver.", CURRENT_FUNCTION);
  }
  const auto* turbSolver = dynamic_cast<const CTurbSolver*>(solver[MESH_0][TURB_SOL]);
  if (solver[MESH_0][TURB_SOL] != nullptr && turbSolver == nullptr) {
    SU2_MPI::Error("Unexpected turbulence solver.", CURRENT_FUNCTION);
  }
  const unsigned short nVarTurb = arrays.nVarTurb;
  const bool turbTimesDensity = arrays.sst;
  const CTransferAdmissibility admissibility(*fluidModel, nDim, turbSolver, arrays.sst);
  const unsigned short nLevel = arrays.nLevel, nPerLevel = arrays.nPerLevel(), nField = arrays.nField();
  summary.names = FieldNames(nDim, nLevel, nVarTurb, turbTimesDensity);

  auto levelArray = [](CSolver** solvers, unsigned short iSol, unsigned short iLevel) -> su2activematrix& {
    auto* nodes = solvers[iSol]->GetNodes();
    return iLevel == 0 ? nodes->GetSolution() : nodes->GetSolution_time_n1();
  };

  /*--- Fields of the owned donor points: per time level the flow variables, then the turbulence variables in
   *    conservation form (SST: rho k, rho omega). ---*/
  const auto nOwnedD = donorGeometry->GetnPointDomain();
  std::vector<su2double> donorValues(nOwnedD * nField);
  for (unsigned short iLevel = 0; iLevel < nLevel; ++iLevel) {
    const auto& flow = levelArray(donor.solver[MESH_0], FLOW_SOL, iLevel);
    for (auto iPoint = 0ul; iPoint < nOwnedD; ++iPoint) {
      auto* values = &donorValues[iPoint * nField + iLevel * nPerLevel];
      for (unsigned short iVar = 0; iVar < nVarFlow; ++iVar) values[iVar] = flow(iPoint, iVar);
      if (nVarTurb == 0) continue;
      const auto& turb = levelArray(donor.solver[MESH_0], TURB_SOL, iLevel);
      for (unsigned short iVar = 0; iVar < nVarTurb; ++iVar)
        values[nVarFlow + iVar] = (turbTimesDensity ? values[0] : su2double(1.0)) * turb(iPoint, iVar);
    }
  }

  /*--- Projection on the owned rows of the new mesh. Open boundaries (not walls or symmetry planes) by name. ---*/
  std::vector<std::string> newTags;
  for (unsigned short iMarker = 0; iMarker < newGeometry->GetnMarker(); ++iMarker)
    newTags.push_back(config->GetMarker_All_TagBound(iMarker));
  const auto newNames = CMeshGather::GatherMarkerNames(*config, newTags, *newGeometry);
  auto projectionOptions = options;
  for (const auto& name : newNames)
    if (OpenBoundary(*config, name)) projectionOptions.openMarkers.push_back(name);
  projectionOptions.absoluteLimit =
      std::max(projectionOptions.absoluteLimit, 2.0 * SU2_TYPE::GetValue(config->GetAdap_Hausd()));
  CDistributedProjection projection(*donorGeometry, DonorMarkerTags("conservative", donor), *newGeometry, newTags,
                                    *config, projectionOptions);
  std::vector<su2double> newValues;
  {
    /*--- Live bytes of this transfer during the projection (memory ceiling, 5.17): names and a bound of the small
     *    bookkeeping objects (configuration arrays, options, summary scalars). ---*/
    using transfer_memory::Bytes;
    const size_t callerBytes = Bytes(newTags) + Bytes(newNames) + Bytes(projectionOptions.openMarkers) +
                               Bytes(summary.names) + (size_t(64) << 10);
    projection.Project(nField, donorValues, newValues, callerBytes);
  }
  const auto nPoint = newGeometry->GetnPointDomain();
  summary.nPoint = CPassiveComm::AllreduceSum(nPoint);

  /*--- No-slip walls: momentum of the owned wall points set to zero (rho E kept), redistributed over the other points
   *    within the limiter bounds (totals exact). ---*/
  std::vector<bool> wallPoint(nPoint, false);
  {
    const auto markerIds = PointMarkerIds(*newGeometry, newTags, *config);
    for (auto iPoint = 0ul; iPoint < nPoint; ++iPoint)
      for (const auto id : markerIds[iPoint])
        if (id < config->GetnMarker_CfgFile() && ViscousWall(*config, config->GetMarker_CfgFile_TagBound(id)))
          wallPoint[iPoint] = true;
  }
  summary.nWallPoints = CPassiveComm::AllreduceSum(std::count(wallPoint.begin(), wallPoint.end(), true));
  if (summary.nWallPoints > 0) {
    passivedouble maxMomentum = 0.0;
    for (unsigned short iLevel = 0; iLevel < nLevel; ++iLevel) {
      const unsigned short offset = iLevel * nPerLevel;
      for (auto iPoint = 0ul; iPoint < nPoint; ++iPoint) {
        if (!wallPoint[iPoint]) continue;
        for (unsigned short iDim = 0; iDim < nDim; ++iDim) {
          auto& momentum = newValues[iPoint * nField + offset + 1 + iDim];
          maxMomentum = std::max(maxMomentum, fabs(SU2_TYPE::GetValue(momentum)));
          momentum = 0.0;
        }
      }
      for (unsigned short iDim = 0; iDim < nDim; ++iDim)
        if (!projection.Redistribute(offset + 1 + iDim, newValues, wallPoint)) summary.nWallBoundsExceeded++;
    }
    summary.maxWallMomentum = CPassiveComm::Allreduce(maxMomentum, CPassiveComm::Op::MAX);
  }

  /*--- Turbulence integrals before the bounds steps (for the report). ---*/
  auto turbulenceTotals = [&]() {
    std::vector<su2double> totals;
    for (unsigned short iLevel = 0; iLevel < nLevel; ++iLevel)
      for (unsigned short iVar = 0; iVar < nVarTurb; ++iVar)
        totals.push_back(projection.NewTotal(iLevel * nPerLevel + nVarFlow + iVar, newValues));
    return totals;
  };
  const auto turbulenceBefore = turbulenceTotals();

  /*--- Turbulence within the bounds of the solver, before the admissibility (points with rho <= 0 skipped). ---*/
  {
    unsigned long nLimited = 0;
    for (unsigned short iLevel = 0; iLevel < nLevel && nVarTurb > 0; ++iLevel)
      nLimited += BoundTurbulence(admissibility, newValues, nPoint, nField, iLevel * nPerLevel);
    summary.nTurbLimited = CPassiveComm::AllreduceSum(nLimited);
  }

  /*--- Stage 1 of the admissibility recovery (convex predicate P1). Rare: until its distributed version (M3) the level
   *    is gathered on the master rank with the new mesh and the wall flags, the serial recovery runs there (sequential in
   *    global index order), the values go back. Stage 2: the bounds on every point. ---*/
  for (unsigned short iLevel = 0; iLevel < nLevel; ++iLevel) {
    const unsigned short offset = iLevel * nPerLevel;
    unsigned long nBad = 0;
    for (auto iPoint = 0ul; iPoint < nPoint; ++iPoint) nBad += !admissibility.AdmissibleStage1(&newValues[iPoint * nField + offset]);
    if (CPassiveComm::AllreduceSum(nBad) == 0) continue;
    summary.recoveryGathered = true;
    const CMeshGather newGather(*newGeometry);
    const auto newMesh = newGather.GatherMesh(*config, newTags, true);
    std::vector<su2double> local(nPoint * (nPerLevel + 1));
    for (auto iPoint = 0ul; iPoint < nPoint; ++iPoint) {
      for (unsigned short iVar = 0; iVar < nPerLevel; ++iVar)
        local[iPoint * (nPerLevel + 1) + iVar] = newValues[iPoint * nField + offset + iVar];
      local[iPoint * (nPerLevel + 1) + nPerLevel] = wallPoint[iPoint] ? 1.0 : 0.0;
    }
    auto global = newGather.Gather(local.data(), nPerLevel + 1);
    CLocalFailure failure;
    unsigned long nFixed = 0, nPatches = 0, maxRing = 0;
    if (newGather.IsRoot()) {
      const auto nGlobal = newMesh.GetnPoint();
      std::vector<bool> wall(nGlobal);
      for (auto i = 0ul; i < nGlobal; ++i) wall[i] = global[i * (nPerLevel + 1) + nPerLevel] > 0.5;
      auto predicate = [&](const su2double* fields) { return admissibility.AdmissibleStage1(fields); };
      CAdmissibilityRecovery<su2double> recovery(newMesh, nPerLevel, wall, predicate);
      for (auto i = 0ul; i < nGlobal && !failure.Failed(); ++i) {
        if (predicate(&global[i * (nPerLevel + 1)])) continue;
        nFixed++;
        if (!recovery.Recover(i, global, nPerLevel + 1, 0)) {
          failure.Set(1, i,
                      "The projected state of the point " + PointText(nDim, &newMesh.coord[i * nDim]) +
                          (iLevel ? " (U^(n-1))" : "") +
                          " is not admissible and cannot be recovered conservatively (not even the mean state of "
                          "the mesh is admissible).");
        }
      }
      nPatches = recovery.nPatches;
      maxRing = recovery.maxRing;
    }
    CollectiveFailure(failure, CURRENT_FUNCTION);
    newGather.Scatter(global, nPerLevel + 1, local.data());
    for (auto iPoint = 0ul; iPoint < nPoint; ++iPoint)
      for (unsigned short iVar = 0; iVar < nPerLevel; ++iVar)
        newValues[iPoint * nField + offset + iVar] = local[iPoint * (nPerLevel + 1) + iVar];
    unsigned long counts[2] = {nFixed, nPatches}, sums[2];
    CPassiveComm::Allreduce(counts, sums, 2, CPassiveComm::Op::SUM);
    (iLevel ? summary.nHistoryFixed : summary.nFlowFixed) += sums[0];
    summary.nRecoveryPatches += sums[1];
    summary.maxRecoveryRing = std::max(summary.maxRecoveryRing, CPassiveComm::AllreduceMax(maxRing));
  }
  {
    unsigned long nLimited = 0;
    for (unsigned short iLevel = 0; iLevel < nLevel && nVarTurb > 0; ++iLevel)
      nLimited += BoundTurbulence(admissibility, newValues, nPoint, nField, iLevel * nPerLevel);
    summary.nTurbLimitedStage2 = CPassiveComm::AllreduceSum(nLimited);
  }
  const auto& projectionSummary = projection.GetSummary();
  {
    const auto turbulenceAfter = turbulenceTotals();
    for (auto i = 0ul; i < turbulenceAfter.size(); ++i) {
      const auto f = (i / nVarTurb) * nPerLevel + nVarFlow + i % nVarTurb;
      summary.turbulenceChange.push_back(SU2_TYPE::GetValue(turbulenceAfter[i] - turbulenceBefore[i]) /
                                         std::max(projectionSummary.scale[f], passivedouble(1e-300)));
    }
  }

  /*--- Final checks (collective): every state of every level admissible with the full predicate, the wall momentum
   *    zero, the totals of the flow variables those of the projection. ---*/
  {
    CLocalFailure failure;
    for (unsigned short iLevel = 0; iLevel < nLevel; ++iLevel) {
      const unsigned short offset = iLevel * nPerLevel;
      for (auto iPoint = 0ul; iPoint < nPoint; ++iPoint) {
        const auto* values = &newValues[iPoint * nField + offset];
        bool ok = admissibility.AdmissibleConservative(values);
        if (wallPoint[iPoint])
          for (unsigned short iDim = 0; iDim < nDim; ++iDim) ok = ok && values[1 + iDim] == 0.0;
        if (!ok) {
          passivedouble x[3] = {};
          for (unsigned short iDim = 0; iDim < nDim; ++iDim)
            x[iDim] = SU2_TYPE::GetValue(newGeometry->nodes->GetCoord(iPoint, iDim));
          failure.Set(2, newGeometry->nodes->GetGlobalIndex(iPoint),
                      "The transferred state of the point " + PointText(nDim, x) + " is not admissible after the recovery.");
        }
      }
    }
    for (unsigned short iLevel = 0; iLevel < nLevel; ++iLevel) {
      for (unsigned short iVar = 0; iVar < nVarFlow; ++iVar) {
        const auto f = iLevel * nPerLevel + iVar;
        const passivedouble defect =
            SU2_TYPE::GetValue(projection.NewTotal(f, newValues)) - projectionSummary.targetTotal[f];
        if (fabs(defect) > TransferTol(1e-12, 1e-5) * std::max(projectionSummary.scale[f], passivedouble(1e-300))) {
          failure.Set(1, UINT64_MAX - f,
                      "The transfer did not keep the total of " + summary.names[f] + " (relative defect " +
                          std::to_string(defect / projectionSummary.scale[f]) + ").");
        }
      }
    }
    CollectiveFailure(failure, CURRENT_FUNCTION);
  }

  /*--- Values of the new arrays (SST: turbulence divided by the density), integrals of the final state. ---*/
  auto finalValues = newValues;
  for (unsigned short iLevel = 0; iLevel < nLevel && nVarTurb > 0 && turbTimesDensity; ++iLevel) {
    const unsigned short offset = iLevel * nPerLevel;
    for (auto iPoint = 0ul; iPoint < nPoint; ++iPoint) {
      const su2double density = newValues[iPoint * nField + offset];
      for (unsigned short iVar = 0; iVar < nVarTurb; ++iVar)
        finalValues[iPoint * nField + offset + nVarFlow + iVar] =
            newValues[iPoint * nField + offset + nVarFlow + iVar] / density;
    }
  }
  {
    CAccurateSumBatch batch;
    std::vector<double> terms;
    for (unsigned short f = 0; f < nField; ++f) {
      terms.resize(nOwnedD);
      for (auto iPoint = 0ul; iPoint < nOwnedD; ++iPoint)
        terms[iPoint] = SU2_TYPE::GetValue(donorValues[iPoint * nField + f]) *
                        SU2_TYPE::GetValue(donorGeometry->nodes->GetVolume(iPoint));
      batch.Add(terms);
      terms.resize(nPoint);
      for (auto iPoint = 0ul; iPoint < nPoint; ++iPoint)
        terms[iPoint] = SU2_TYPE::GetValue(newValues[iPoint * nField + f]) *
                        SU2_TYPE::GetValue(newGeometry->nodes->GetVolume(iPoint));
      batch.Add(terms);
    }
    batch.Reduce();
    summary.donorIntegral.assign(nField, 0.0);
    summary.newIntegral.assign(nField, 0.0);
    summary.relativeDefect.assign(nField, 0.0);
    for (unsigned short f = 0; f < nField; ++f) {
      summary.donorIntegral[f] = batch.Get(2 * f);
      summary.newIntegral[f] = batch.Get(2 * f + 1);
    }
    for (unsigned short iLevel = 0; iLevel < nLevel; ++iLevel) {
      const unsigned short offset = iLevel * nPerLevel;
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
  }
  summary.projection = projection.GetSummary();

  /*--- The owned rows of the new arrays; history; as after a restart. ---*/
  for (unsigned short iLevel = 0; iLevel < nLevel; ++iLevel) {
    const unsigned short offset = iLevel * nPerLevel;
    auto& flow = levelArray(solver[MESH_0], FLOW_SOL, iLevel);
    for (auto iPoint = 0ul; iPoint < nPoint; ++iPoint)
      for (unsigned short iVar = 0; iVar < nVarFlow; ++iVar) flow(iPoint, iVar) = finalValues[iPoint * nField + offset + iVar];
    if (nVarTurb == 0) continue;
    auto& turb = levelArray(solver[MESH_0], TURB_SOL, iLevel);
    for (auto iPoint = 0ul; iPoint < nPoint; ++iPoint)
      for (unsigned short iVar = 0; iVar < nVarTurb; ++iVar)
        turb(iPoint, iVar) = finalValues[iPoint * nField + offset + nVarFlow + iVar];
  }
  for (const auto iSol : arrays.solverIndices) {
    auto* nodes = solver[MESH_0][iSol]->GetNodes();
    if (arrays.hasTimeN) nodes->GetSolution_time_n() = nodes->GetSolution();
    if (arrays.hasTimeN1 && !arrays.interpolateTimeN1) nodes->GetSolution_time_n1() = nodes->GetSolution_time_n();
  }
  FinishTransfer(config, geometry, solver, arrays);
  summary.time = SU2_TYPE::GetValue(SU2_MPI::Wtime() - start);

  const unsigned long maxImport[1] = {summary.projection.maxImported};
  const double meanImport = static_cast<double>(summary.projection.nImported) / size;
  std::ostringstream timing;
  timing << std::setprecision(3) << "Distributed conservative transfer on " << size << " rank(s): import groups "
         << summary.projection.nImportGroups << ", imported donor elements " << summary.projection.nImported
         << " (largest per rank / mean " << (meanImport > 0 ? maxImport[0] / meanImport : 0.0)
         << (meanImport > 0 && maxImport[0] > 4 * meanImport ? ", imbalance warning" : "") << ")"
         << "; planned memory per rank (largest): resident and coverage " << (summary.projection.memoryResident >> 20)
         << " MB, import " << (summary.projection.memoryImport >> 20) << " MB (ceiling "
         << (GetTransferMemoryCeiling() >> 20) << " MB)"
         << (summary.recoveryGathered ? "; recovery on the gathered level (rank 0)" : "") << ".";
  if (rank == MASTER_NODE) PrintSummary(nDim, nField, nVarTurb > 0, timing.str());
}

void CConservativeTransfer::TransferGathered(CConfig* config, const CMeshDonor& donor, CGeometry** geometry,
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
  const bool turbTimesDensity = arrays.sst;
  const CTransferAdmissibility admissibility(*fluidModel, nDim, turbSolver, arrays.sst);

  /*--- Fields: per time level (U^n, and U^(n-1) for 2nd order) the flow variables and the turbulence variables. ---*/

  const unsigned short nLevel = arrays.interpolateTimeN1 ? 2 : 1;
  const unsigned short nPerLevel = nVarFlow + nVarTurb;
  const unsigned short nField = nLevel * nPerLevel;
  summary.names = FieldNames(nDim, nLevel, nVarTurb, turbTimesDensity);

  auto flowArray = [&](CSolver** solvers, unsigned short iLevel) -> su2activematrix& {
    auto* nodes = solvers[FLOW_SOL]->GetNodes();
    return iLevel == 0 ? nodes->GetSolution() : nodes->GetSolution_time_n1();
  };
  auto turbArray = [&](CSolver** solvers, unsigned short iLevel) -> su2activematrix& {
    auto* nodes = solvers[TURB_SOL]->GetNodes();
    return iLevel == 0 ? nodes->GetSolution() : nodes->GetSolution_time_n1();
  };

  /*--- Both meshes (with their control volumes) and the donor fields on the master rank, in the global numbering of
   *    the meshes (CMeshGather); the projection runs there on the complete meshes and the new values go back to the
   *    ranks that own the points. ---*/

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

    /*--- Projection. Open boundaries (not walls or symmetry planes): far field, inlets, outlets, ... ---*/

    auto projectionOptions = options;
    for (const auto& marker : newMesh.markers)
      if (OpenBoundary(*config, marker.name)) projectionOptions.openMarkers.push_back(marker.name);
    projectionOptions.absoluteLimit =
        std::max(projectionOptions.absoluteLimit, 2.0 * SU2_TYPE::GetValue(config->GetAdap_Hausd()));
    CConservativeProjection projection(donorMesh, newMesh, projectionOptions);
    std::vector<su2double> newValues;
    projection.Project(nField, donorValues, newValues);

    /*--- No-slip walls: the projected momentum of a wall point is set to zero (rho E kept, as the solver does) and the
     *    removed momentum is redistributed over the other points within the limiter bounds (totals exact). ---*/

    std::vector<bool> wallPoint(nPoint, false);
    for (const auto& marker : newMesh.markers) {
      if (!ViscousWall(*config, marker.name)) continue;
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
        for (unsigned short iDim = 0; iDim < nDim; ++iDim)
          if (!projection.Redistribute(offset + 1 + iDim, newValues, wallPoint)) summary.nWallBoundsExceeded++;
      }
    }

    /*--- Turbulence integrals before the bounds steps; bounds (rho <= 0 skipped); stage 1 recovery (P1, sequential
     *    in global index order); stage 2 bounds on every point. ---*/
    std::vector<su2double> turbulenceBefore;
    for (unsigned short iLevel = 0; iLevel < nLevel; ++iLevel)
      for (unsigned short iVar = 0; iVar < nVarTurb; ++iVar)
        turbulenceBefore.push_back(projection.NewTotal(iLevel * nPerLevel + nVarFlow + iVar, newValues));
    for (unsigned short iLevel = 0; iLevel < nLevel && nVarTurb > 0; ++iLevel)
      summary.nTurbLimited += BoundTurbulence(admissibility, newValues, nPoint, nField, iLevel * nPerLevel);

    auto pointText = [&](unsigned long iPoint) { return PointText(nDim, &newMesh.coord[iPoint * nDim]); };
    auto stage1 = [&](const su2double* values) { return admissibility.AdmissibleStage1(values); };
    CAdmissibilityRecovery<su2double> recovery(newMesh, nPerLevel, wallPoint, stage1);
    for (unsigned short iLevel = 0; iLevel < nLevel; ++iLevel) {
      const unsigned short offset = iLevel * nPerLevel;
      auto& nFixed = iLevel ? summary.nHistoryFixed : summary.nFlowFixed;
      for (auto iPoint = 0ul; iPoint < nPoint; ++iPoint) {
        if (stage1(&newValues[iPoint * nField + offset])) continue;
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
    for (unsigned short iLevel = 0; iLevel < nLevel && nVarTurb > 0; ++iLevel)
      summary.nTurbLimitedStage2 += BoundTurbulence(admissibility, newValues, nPoint, nField, iLevel * nPerLevel);
    const auto& projectionSummary = projection.GetSummary();
    for (unsigned short iLevel = 0, i = 0; iLevel < nLevel; ++iLevel)
      for (unsigned short iVar = 0; iVar < nVarTurb; ++iVar, ++i) {
        const auto f = iLevel * nPerLevel + nVarFlow + iVar;
        summary.turbulenceChange.push_back(SU2_TYPE::GetValue(projection.NewTotal(f, newValues) - turbulenceBefore[i]) /
                                           std::max(projectionSummary.scale[f], passivedouble(1e-300)));
      }

    /*--- Final checks before the arrays are set: every state admissible (full predicate), the wall momentum zero, the
     *    totals of the flow variables those of the projection. ---*/

    for (unsigned short iLevel = 0; iLevel < nLevel; ++iLevel) {
      const unsigned short offset = iLevel * nPerLevel;
      for (auto iPoint = 0ul; iPoint < nPoint; ++iPoint) {
        const auto* values = &newValues[iPoint * nField + offset];
        bool ok = admissibility.AdmissibleConservative(values);
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
        if (fabs(defect) > TransferTol(1e-12, 1e-5) * std::max(projectionSummary.scale[f], passivedouble(1e-300))) {
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

  if (rank != MASTER_NODE) return;
  std::ostringstream timing;
  timing << "Gathered conservative transfer (MPI-1 reference): donor and new mesh with the donor solution gathered "
            "on rank "
         << MASTER_NODE << " (" << gatherTime << " s), new values sent to the ranks of their points (" << scatterTime
         << " s).";
  PrintSummary(nDim, nField, nVarTurb > 0, timing.str());
}

void CConservativeTransfer::PrintSummary(unsigned short nDim, unsigned short nField, bool turbulence,
                                         const std::string& timing) const {
  const auto& p = summary.projection;
  cout << endl << "------------------- Solution Transfer (conservative P1) -------------------" << endl;
  cout << timing << endl;
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
                             "spread over the new domain by volume), at open boundaries the totals follow the domain."
                           : ", content dropped: the totals change by the S_n minus the S_d content.")
       << endl;
  cout << "Largest uncovered fraction of a control-volume piece: " << p.maxUncovered << "." << endl;
  cout << "Per field: S_n content and S_d content (relative to sum |u| V), correction spread over the domain, supermesh "
          "defect (round-off), CG iterations and true residual, values limited:"
       << endl;
  for (unsigned short f = 0; f < nField && f < p.scale.size(); ++f) {
    const passivedouble scale = std::max(p.scale[f], passivedouble(1e-300));
    cout << "  " << std::setw(18) << summary.names[f] << ": " << std::setw(10) << p.fill[f] / scale << std::setw(11)
         << p.sliver[f] / scale << std::setw(11) << p.correction[f] / scale << std::setw(11) << p.supermeshDefect[f]
         << std::setw(5) << p.iterations[f] << std::setw(11) << p.residual[f] << std::setw(8) << p.nLimited[f];
    if (p.infeasible[f]) {
      cout << "  bounds relaxed (" << p.nViolations[f] << " values, largest " << p.maxViolation[f]
           << " of the range), rest spread by volume";
    }
    cout << endl;
  }
  if (p.nSolveWarnings > 0)
    cout << "Warning: " << p.nSolveWarnings << " field(s) with a true residual above the CG tolerance (below 1e-10)."
         << endl;
  if (summary.nWallPoints > 0) {
    cout << "No-slip wall points: " << summary.nWallPoints << ", projected momentum set to zero (largest "
         << summary.maxWallMomentum << "), redistributed over the other points." << endl;
  }
  if (summary.nWallBoundsExceeded > 0) {
    cout << "The limiter bounds could not hold the removed wall momentum of " << summary.nWallBoundsExceeded
         << " momentum field(s): the rest was spread over the domain by volume (totals exact)." << endl;
  }
  cout << "States not admissible after the projection (stage 1 predicate, k_lo floor with SST): " << summary.nFlowFixed;
  if (summary.interpolateTimeN1) cout << ", U^(n-1): " << summary.nHistoryFixed;
  cout << "; recovered by blending " << summary.nRecoveryPatches << " patch(es) towards their mean state (up to "
       << summary.maxRecoveryRing << " neighbour rings), integrals kept." << endl;
  if (turbulence) {
    cout << "Turbulence values limited to the bounds of the solver: " << summary.nTurbLimited << " (after the recovery: "
         << summary.nTurbLimitedStage2 << "); turbulence integral change of the bounds steps:";
    for (const auto change : summary.turbulenceChange) cout << " " << change;
    cout << "." << endl;
  }
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
          "norm of the donor momentum integral):"
       << endl;
  for (unsigned short f = 0; f < nField && f < summary.relativeDefect.size(); ++f)
    cout << "  " << std::setw(18) << summary.names[f] << ": " << summary.relativeDefect[f] << endl;
  cout.unsetf(std::ios_base::floatfield);
  cout << std::setprecision(4) << "Time: setup " << p.timeSetup << " s, import " << p.timeImport << " s, supermesh "
       << p.timeSupermesh << " s, coverage and slivers " << p.timeCoverage + p.timeSlivers << " s, solve "
       << p.timeSolve << " s, limiter " << p.timeLimiter << " s, total " << summary.time << " s." << endl;
  cout << std::setprecision(6);
  (void)nDim;
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
