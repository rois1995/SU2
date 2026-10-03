/*!
 * \file CBarycentricTransfer.cpp
 * \brief Transfer of the solution to a new mesh by barycentric (P1) interpolation (mesh adaptation).
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

#include "../../include/adaptation/CBarycentricTransfer.hpp"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <limits>
#include <numeric>
#include <sstream>

#include "../../../Common/include/CConfig.hpp"
#include "../../../Common/include/adaptation/CAccurateSum.hpp"
#include "../../../Common/include/adaptation/CMeshGather.hpp"
#include "../../../Common/include/geometry/CGeometry.hpp"
#include "../../../Common/include/parallelization/CPassiveComm.hpp"
#include "../../include/adaptation/CTransferAdmissibility.hpp"
#include "../../include/fluid/CFluidModel.hpp"
#include "../../include/solvers/CSolver.hpp"
#include "../../include/solvers/CTurbSolver.hpp"

namespace {

/*--- Coordinates of a point as text, for the error messages. ---*/
std::string PointText(unsigned short nDim, const su2double* coord) {
  std::ostringstream text;
  text << std::setprecision(10) << "(";
  for (unsigned short iDim = 0; iDim < nDim; ++iDim) text << (iDim ? ", " : "") << coord[iDim];
  text << ")";
  return text.str();
}

bool IsFinite(su2double value) { return std::isfinite(SU2_TYPE::GetValue(value)); }

/*--- Volume of a mesh (sum of its control volumes) and integral of the first nVar fields (point-major values with
 *    nField per point). ---*/
void Integrals(const CSimplexMesh& mesh, const std::vector<su2double>& values, unsigned short nField,
               unsigned short nVar, su2double& volume, std::vector<su2double>& integral) {
  volume = 0.0;
  integral.assign(nVar, 0.0);
  for (auto iPoint = 0ul; iPoint < mesh.GetnPoint(); ++iPoint) {
    const su2double vol = mesh.volume[iPoint];
    volume += vol;
    for (unsigned short iVar = 0; iVar < nVar; ++iVar) integral[iVar] += vol * values[iPoint * nField + iVar];
  }
}

/*--- Marker names of each point of a mesh. ---*/
std::vector<std::vector<std::string>> PointMarkers(const CSimplexMesh& mesh) {
  std::vector<std::vector<std::string>> markers(mesh.GetnPoint());
  for (const auto& marker : mesh.markers) {
    for (const auto iPoint : marker.elem) {
      auto& names = markers[iPoint];
      if (std::find(names.begin(), names.end(), marker.name) == names.end()) names.push_back(marker.name);
    }
  }
  return markers;
}

/*--- Stencil of a point: the closest point of the donor boundary of its marker(s) if it is on a marker that the donor
 *    has, else the containing element or the closest point of the nearest donor boundary face. ---*/
CBarycentricLocator::Stencil LocatePoint(CBarycentricLocator& locator, const su2double* x,
                                         const std::vector<std::string>& names) {
  std::vector<std::string> known;
  for (const auto& name : names)
    if (locator.HasMarker(name)) known.push_back(name);
  return known.empty() ? locator.Locate(x) : locator.LocateOnBoundary(x, known);
}

}  // namespace

CBarycentricLocator::CBarycentricLocator(const CGeometry& geometry, const std::vector<std::string>& markerTags,
                                         su2double absoluteLimit)
    : nDim(geometry.GetnDim()), absoluteLimit(absoluteLimit) {
  /*--- The mesh of this rank (all its points); markers without a name only take part in Locate. ---*/
  auto tags = markerTags;
  tags.resize(std::max<size_t>(tags.size(), geometry.GetnMarker()), "");
  Build(CMeshGather::LocalMesh(geometry, tags, false));
}

CBarycentricLocator::CBarycentricLocator(const CSimplexMesh& mesh, su2double absoluteLimit)
    : nDim(mesh.nDim), absoluteLimit(absoluteLimit) {
  Build(mesh);
}

void CBarycentricLocator::Build(const CSimplexMesh& mesh) {
  if (nDim != 2 && nDim != 3) SU2_MPI::Error("The solution transfer needs a 2D or 3D mesh.", CURRENT_FUNCTION);
  const unsigned short elemType = (nDim == 2) ? TRIANGLE : TETRAHEDRON;
  const unsigned short nNode = nDim + 1;
  const auto nPoint = mesh.GetnPoint();

  coord.resize(nPoint * nDim);
  su2double xMin[3] = {}, xMax[3] = {};
  for (unsigned short iDim = 0; iDim < nDim; ++iDim) {
    xMin[iDim] = std::numeric_limits<passivedouble>::max();
    xMax[iDim] = std::numeric_limits<passivedouble>::lowest();
  }
  for (auto iPoint = 0ul; iPoint < nPoint; ++iPoint) {
    for (unsigned short iDim = 0; iDim < nDim; ++iDim) {
      const su2double x = mesh.coord[iPoint * nDim + iDim];
      coord[iPoint * nDim + iDim] = x;
      xMin[iDim] = min(xMin[iDim], x);
      xMax[iDim] = max(xMax[iDim], x);
    }
  }
  for (unsigned short iDim = 0; iDim < nDim; ++iDim) domainSize += pow(xMax[iDim] - xMin[iDim], 2);
  domainSize = sqrt(domainSize);

  /*--- Elements and their keys. The weights of the ADT are in the order of these nodes. ---*/

  elemConn = mesh.elem;
  if (elemConn.empty()) SU2_MPI::Error("The mesh has no elements.", CURRENT_FUNCTION);
  const auto nElem = elemConn.size() / nNode;
  elemKeys.resize(nElem);
  for (auto iElem = 0ul; iElem < nElem; ++iElem) {
    uint64_t nodes[4] = {};
    for (unsigned short k = 0; k < nNode; ++k) nodes[k] = elemConn[iElem * nNode + k];
    elemKeys[iElem] = MakeSimplexKey(nodes, nNode);
  }
  {
    std::vector<su2double> coordCopy = coord;
    std::vector<unsigned long> connCopy = elemConn;
    std::vector<unsigned short> types(nElem, elemType), markers(nElem, 0);
    std::vector<unsigned long> ids(nElem);
    std::iota(ids.begin(), ids.end(), 0ul);
    elemADT = std::make_unique<CADTElemClass>(nDim, coordCopy, connCopy, types, markers, ids, false);
  }

  /*--- Faces of all markers: positions of the named markers in the order of the mesh, the unnamed ones after them
   *    (the config order for a gathered mesh, as the config positions of the distributed search). ---*/

  std::vector<std::string> names;
  for (const auto& marker : mesh.markers) {
    if (marker.name.empty() || marker.elem.empty()) continue;
    if (std::find(names.begin(), names.end(), marker.name) == names.end()) names.push_back(marker.name);
  }
  const uint32_t unnamed = names.size();
  markerName = names;
  markerName.push_back("");
  CBoundaryFaces faces;
  faces.nDim = nDim;
  std::vector<bool> used(nPoint, false);
  for (const auto& marker : mesh.markers) {
    const uint32_t id = marker.name.empty()
                            ? unnamed
                            : static_cast<uint32_t>(std::find(names.begin(), names.end(), marker.name) - names.begin());
    for (auto iFace = 0ul; iFace < marker.elem.size() / nDim; ++iFace) {
      faces.marker.push_back(id);
      for (unsigned short k = 0; k < nDim; ++k) {
        const auto iPoint = marker.elem[iFace * nDim + k];
        faces.faceGid.push_back(iPoint);
        used[iPoint] = true;
      }
    }
    if (!marker.name.empty() && !marker.elem.empty()) markerIndex[marker.name] = id;
  }
  if (faces.marker.empty()) SU2_MPI::Error("The mesh has no boundary faces.", CURRENT_FUNCTION);
  for (auto iPoint = 0ul; iPoint < nPoint; ++iPoint) {
    if (!used[iPoint]) continue;
    faces.nodeGid.push_back(iPoint);
    for (unsigned short iDim = 0; iDim < nDim; ++iDim) faces.nodeCoord.push_back(mesh.coord[iPoint * nDim + iDim]);
  }
  boundary = std::make_unique<CCanonicalBoundary>(std::move(faces), SU2_TYPE::GetValue(domainSize));
  allMarkers = boundary->GetMarkers();
}

su2double CBarycentricLocator::GetDistanceLimit(su2double faceSize) const {
  return max(faceSize, max(absoluteLimit, su2double(domainFraction) * domainSize));
}

CBarycentricLocator::Stencil CBarycentricLocator::FaceStencil(const CFaceHit& hit) const {
  Stencil stencil;
  stencil.inside = false;
  stencil.onFace = true;
  stencil.nPoint = nDim;
  for (unsigned short k = 0; k < nDim; ++k) {
    stencil.point[k] = boundary->FaceNodeGid(hit.face, k);
    stencil.weight[k] = hit.weight[k];
  }
  stencil.distance = hit.distance;
  stencil.faceSize = hit.faceSize;
  stencil.beyondLimit = !(stencil.distance <= GetDistanceLimit(stencil.faceSize));
  stencil.marker = hit.marker;
  stencil.key = hit.key;
  return stencil;
}

CElementHit CBarycentricLocator::ContainingHit(const su2double* x, long* element) {
  const unsigned short nNode = nDim + 1;
  std::vector<unsigned long> ids;
  std::vector<su2double> weights;
  elemADT->DetermineContainingElements(x, ids, weights);
  CElementHit best;
  long bestElem = -1;
  for (auto i = 0ul; i < ids.size(); ++i) {
    CElementHit hit;
    hit.found = true;
    hit.minWeight = std::numeric_limits<passivedouble>::max();
    for (unsigned short k = 0; k < nNode; ++k) {
      hit.weight[k] = SU2_TYPE::GetValue(weights[8 * i + k]);
      hit.minWeight = std::min(hit.minWeight, hit.weight[k]);
      hit.gid[k] = elemConn[ids[i] * nNode + k];
    }
    hit.key = elemKeys[ids[i]];
    if (BetterElement(hit, best)) {
      best = hit;
      bestElem = ids[i];
    }
  }
  if (element != nullptr) *element = bestElem;
  return best;
}

CBarycentricLocator::Stencil CBarycentricLocator::Locate(const su2double* x) {
  /*--- Canonical element that contains the point (tolerance of the ADT, weights >= -5e-11, set to >= 0 here so that
   *    the interpolation is a convex combination). ---*/

  const auto hit = ContainingHit(x);
  if (hit.found) {
    const auto element = CDistributedLocator::ElementStencil(hit, nDim);
    Stencil stencil;
    stencil.nPoint = element.nPoint;
    for (unsigned short k = 0; k < element.nPoint; ++k) {
      stencil.point[k] = element.gid[k];
      stencil.weight[k] = element.weight[k];
    }
    stencil.key = element.key;
    return stencil;
  }

  /*--- Outside the mesh: closest point of the canonical nearest boundary face (any marker). ---*/

  passivedouble xp[3] = {0.0, 0.0, 0.0};
  for (unsigned short iDim = 0; iDim < nDim; ++iDim) xp[iDim] = SU2_TYPE::GetValue(x[iDim]);
  return FaceStencil(boundary->Nearest(xp, allMarkers));
}

long CBarycentricLocator::ContainingElement(const su2double* x) {
  long element = -1;
  ContainingHit(x, &element);
  return element;
}

CBarycentricLocator::Stencil CBarycentricLocator::LocateOnBoundary(const su2double* x,
                                                                   const std::vector<std::string>& names,
                                                                   std::string* nameFound) {
  std::vector<uint32_t> ids;
  for (const auto& name : names) {
    const auto it = markerIndex.find(name);
    if (it != markerIndex.end()) ids.push_back(it->second);
  }
  if (ids.empty()) {
    if (nameFound != nullptr) nameFound->clear();
    return Locate(x);
  }
  passivedouble xp[3] = {0.0, 0.0, 0.0};
  for (unsigned short iDim = 0; iDim < nDim; ++iDim) xp[iDim] = SU2_TYPE::GetValue(x[iDim]);
  const auto hit = boundary->Nearest(xp, ids);
  auto best = FaceStencil(hit);
  if (nameFound != nullptr) *nameFound = markerName[hit.marker];

  unsigned short markerID = 0;
  unsigned long id = 0;
  int rankID = 0;
  su2double parCoor[3] = {}, weights[8] = {};
  best.inside = elemADT->DetermineContainingElement(x, markerID, id, rankID, parCoor, weights);
  return best;
}

bool CBarycentricTransfer::AdmissibleState(CFluidModel& fluidModel, unsigned short nDim, const su2double* solution,
                                           su2double turbKineticEnergy) {
  for (unsigned short iVar = 0; iVar < nDim + 2; ++iVar) {
    if (!IsFinite(solution[iVar])) return false;
  }
  if (!IsFinite(turbKineticEnergy)) return false;
  const su2double density = solution[0];
  if (!(density > 0.0)) return false;

  /*--- As CNSVariable::SetPrimVar: the turbulent kinetic energy (SST) is not part of the internal energy. ---*/
  su2double velocity2 = 0.0;
  for (unsigned short iDim = 0; iDim < nDim; ++iDim) velocity2 += pow(solution[iDim + 1] / density, 2);
  const su2double staticEnergy = solution[nDim + 1] / density - 0.5 * velocity2 - turbKineticEnergy;

  fluidModel.SetTDState_rhoe(density, staticEnergy);
  const su2double pressure = fluidModel.GetPressure();
  const su2double temperature = fluidModel.GetTemperature();
  const su2double soundSpeed2 = fluidModel.GetSoundSpeed2();

  return pressure > 0.0 && temperature > 0.0 && soundSpeed2 > 0.0 && IsFinite(pressure) && IsFinite(temperature) &&
         IsFinite(soundSpeed2);
}

bool CBarycentricTransfer::PointKernel(const CTransferAdmissibility& admissibility, unsigned short nPoint,
                                       const passivedouble* weight, const su2double* const* donorLevel,
                                       su2double* out, bool& fixed, unsigned long& nTurbLimited) {
  constexpr unsigned short kMaxVar = 8;
  const unsigned short nVarFlow = admissibility.GetnVarFlow(), nVarTurb = admissibility.GetnVarTurb();
  const bool sst = admissibility.IsSST();
  su2double state[kMaxVar] = {}, turb[kMaxVar] = {};
  for (unsigned short iVar = 0; iVar < nVarFlow; ++iVar) {
    state[iVar] = 0.0;
    for (unsigned short k = 0; k < nPoint; ++k) state[iVar] += weight[k] * donorLevel[k][iVar];
  }
  for (unsigned short iVar = 0; iVar < nVarTurb; ++iVar) {
    su2double value = 0.0;
    for (unsigned short k = 0; k < nPoint; ++k)
      value += weight[k] * (sst ? donorLevel[k][0] : su2double(1.0)) * donorLevel[k][nVarFlow + iVar];
    if (sst) value = (state[0] > 0.0) ? value / state[0] : su2double(0.0);
    turb[iVar] = admissibility.Bounded(iVar, value, &nTurbLimited);
  }
  fixed = false;
  if (!admissibility.Admissible(state, turb)) {
    /*--- Paired fallback: flow and (bounded) turbulence states of the same donor point. ---*/
    fixed = true;
    int best = -1;
    su2double bounded[kMaxVar] = {};
    for (unsigned short k = 0; k < nPoint; ++k) {
      if (best >= 0 && weight[k] <= weight[best]) continue;
      for (unsigned short iVar = 0; iVar < nVarTurb; ++iVar)
        bounded[iVar] = admissibility.Bounded(iVar, donorLevel[k][nVarFlow + iVar]);
      if (admissibility.Admissible(donorLevel[k], bounded)) best = k;
    }
    if (best < 0) return false;
    for (unsigned short iVar = 0; iVar < nVarFlow; ++iVar) state[iVar] = donorLevel[best][iVar];
    for (unsigned short iVar = 0; iVar < nVarTurb; ++iVar)
      turb[iVar] = admissibility.Bounded(iVar, donorLevel[best][nVarFlow + iVar]);
  }
  for (unsigned short iVar = 0; iVar < nVarFlow; ++iVar) out[iVar] = state[iVar];
  for (unsigned short iVar = 0; iVar < nVarTurb; ++iVar) out[nVarFlow + iVar] = turb[iVar];
  return true;
}

namespace {

/*--- The text of the distance-limit error (the same for the distributed and the gathered transfer). ---*/
std::string BeyondLimitText(unsigned long nBeyond, unsigned short nDim, const su2double* x, su2double distance,
                            su2double faceSize, su2double domainSize) {
  return std::to_string(nBeyond) +
         " points of the new mesh are farther from the donor mesh than accepted (max(face size, "
         "2 ADAP_HAUSD, 1e-3 x domain size)), the farthest is " +
         PointText(nDim, x) + " at " + std::to_string(SU2_TYPE::GetValue(distance)) +
         " from the donor boundary (nearest face " + std::to_string(SU2_TYPE::GetValue(faceSize)) +
         " long, domain size " + std::to_string(SU2_TYPE::GetValue(domainSize)) +
         "). The two meshes do not describe the same domain (wrong mesh or units, or a boundary that was moved).";
}

/*--- Fields of a time level of a solver: the solution (U^n) or Solution_time_n1 (U^(n-1)). ---*/
su2activematrix& LevelArray(CVariable* nodes, unsigned short iLevel) {
  return iLevel == 0 ? nodes->GetSolution() : nodes->GetSolution_time_n1();
}

}  // namespace

void CBarycentricTransfer::Transfer(CConfig* config, const CMeshDonor& donor, CGeometry** geometry,
                                    CSolver*** solver) {
  if (gathered) {
    TransferGathered(config, donor, geometry, solver);
    return;
  }
  SU2_ZONE_SCOPED

  const auto startTime = SU2_MPI::Wtime();
  const int rank = SU2_MPI::GetRank();
  summary = Summary();
  stencilRecords.clear();
  CTransferRoundScope rounds;

  const auto arrays = CheckProblem("barycentric", config, donor, geometry, solver);
  const auto& solverIndices = arrays.solverIndices;
  auto* donorGeometry = donor.geometry[MESH_0];
  auto* newGeometry = geometry[MESH_0];
  const auto nDim = arrays.nDim;

  auto* flowSolver = solver[MESH_0][FLOW_SOL];
  summary.interpolateTimeN1 = arrays.interpolateTimeN1;
  summary.nTimeLevels = arrays.hasTimeN + arrays.hasTimeN1;
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
  constexpr unsigned short kMaxVar = 8;
  if (nVarFlow > kMaxVar || nVarTurb > kMaxVar) SU2_MPI::Error("Too many variables.", CURRENT_FUNCTION);
  const CTransferAdmissibility admissibility(*fluidModel, nDim, turbSolver, arrays.sst);

  /*--- Fields of each point: per time level (U^n, and U^(n-1) for 2nd order) the flow and turbulence variables (raw
   *    solver arrays). ---*/
  const unsigned short nLevel = arrays.nLevel, nPerLevel = arrays.nPerLevel(), nField = arrays.nField();
  const size_t recordBytes = nField * FieldValueBytes();

  /*--- Donor: the values of the owned points in the rendezvous directory (only owned values are read: the halo
   *    state of the donor does not matter). ---*/
  double phaseTime[6] = {};
  auto phase = SU2_MPI::Wtime();
  CPointDirectory directory;
  {
    const auto nOwned = donorGeometry->GetnPointDomain();
    std::vector<uint64_t> gids(nOwned);
    std::vector<char> records(nOwned * recordBytes);
    su2double values[2 * 2 * kMaxVar] = {};
    for (auto iPoint = 0ul; iPoint < nOwned; ++iPoint) {
      gids[iPoint] = donorGeometry->nodes->GetGlobalIndex(iPoint);
      for (unsigned short iLevel = 0; iLevel < nLevel; ++iLevel) {
        for (const auto iSol : solverIndices) {
          const auto& array = LevelArray(donor.solver[MESH_0][iSol]->GetNodes(), iLevel);
          const unsigned short offset = iLevel * nPerLevel + (iSol == FLOW_SOL ? 0 : nVarFlow);
          for (unsigned long iVar = 0; iVar < array.cols(); ++iVar) values[offset + iVar] = array(iPoint, iVar);
        }
      }
      PackFieldValues(&records[iPoint * recordBytes], values, nField);
    }
    directory.Build(gids, records, recordBytes, "donor");
  }

  /*--- Search structures of the donor; the owned points of the new mesh with their markers. ---*/
  std::vector<std::string> newTags;
  for (unsigned short iMarker = 0; iMarker < newGeometry->GetnMarker(); ++iMarker)
    newTags.push_back(config->GetMarker_All_TagBound(iMarker));
  const auto donorTags = DonorMarkerTags("barycentric", donor);
  const auto nPoint = newGeometry->GetnPointDomain();
  std::vector<passivedouble> coord(nPoint * nDim);
  std::vector<std::vector<uint32_t>> pointMarkers;
  {
    auto all = PointMarkerIds(*newGeometry, newTags, *config);
    all.resize(nPoint);
    pointMarkers = std::move(all);
  }
  for (auto iPoint = 0ul; iPoint < nPoint; ++iPoint)
    for (unsigned short iDim = 0; iDim < nDim; ++iDim)
      coord[iPoint * nDim + iDim] = SU2_TYPE::GetValue(newGeometry->nodes->GetCoord(iPoint, iDim));
  phaseTime[0] = SU2_MPI::Wtime() - phase;
  phase = SU2_MPI::Wtime();

  std::vector<CDistributedLocator::Stencil> stencils;
  passivedouble domainSize = 0.0;
  unsigned long nQueriesSent = 0, nQueriesReceived = 0, nChunk = 0;
  {
    CDistributedLocator locator(*donorGeometry, donorTags, *config, 2.0 * SU2_TYPE::GetValue(config->GetAdap_Hausd()));
    summary.distanceLimit = locator.GetDistanceLimit(0.0);
    domainSize = locator.GetDomainSize();
    stencils = locator.Locate(coord, pointMarkers);
    nQueriesSent = locator.GetLastQueriesSent();
    nQueriesReceived = locator.GetLastQueriesReceived();
    nChunk = locator.GetLastChunks();

    /*--- Statistics per marker of the new mesh (names in config order on every rank), then the points on no marker;
     *    the farthest point beyond the distance limit (ties by global index) stops the transfer on all ranks. ---*/
    const auto names = CMeshGather::GatherMarkerNames(*config, newTags, *newGeometry);
    std::vector<uint32_t> nameIds;
    for (const auto& name : names) {
      summary.markers.emplace_back();
      summary.markers.back().name = name;
      nameIds.push_back(MarkerConfigId(*config, name));
    }
    summary.markers.emplace_back();
    summary.markers.back().name = "(interior)";
    const auto interior = summary.markers.size() - 1;
    const auto nEntry = summary.markers.size();
    const passivedouble offTolerance = 1e-12 * domainSize;

    std::vector<unsigned long> counts(4 * nEntry + 2, 0);  // per entry nPoint, nOutside, nOff, nBeyondFace; nOutside, nBeyond
    std::vector<double> maxima(2 * nEntry + 2, 0.0);       // per entry maxDistance, maxRelDistance; global both
    std::vector<std::vector<double>> sums(2 * nEntry);     // per entry distances, relative distances
    passivedouble worstRatio = 0.0;
    unsigned long worstPoint = 0;
    uint64_t worstGid = UINT64_MAX;
    std::vector<unsigned long> entries;
    for (auto iPoint = 0ul; iPoint < nPoint; ++iPoint) {
      const auto& stencil = stencils[iPoint];
      const passivedouble relDistance = stencil.onFace ? stencil.distance / stencil.faceSize : 0.0;
      if (!stencil.inside) counts[4 * nEntry]++;
      maxima[2 * nEntry] = std::max(maxima[2 * nEntry], stencil.distance);
      maxima[2 * nEntry + 1] = std::max(maxima[2 * nEntry + 1], relDistance);
      if (stencil.beyondLimit) {
        counts[4 * nEntry + 1]++;
        passivedouble ratio = stencil.distance / locator.GetDistanceLimit(stencil.faceSize);
        if (!std::isfinite(ratio)) ratio = std::numeric_limits<passivedouble>::infinity();
        const uint64_t gid = newGeometry->nodes->GetGlobalIndex(iPoint);
        /*--- Ties by the global index (partition independent). ---*/
        if (ratio > worstRatio || (ratio == worstRatio && gid < worstGid)) {
          worstRatio = ratio;
          worstPoint = iPoint;
          worstGid = gid;
        }
      }
      entries.clear();
      for (auto iEntry = 0ul; iEntry < interior; ++iEntry)
        if (std::binary_search(pointMarkers[iPoint].begin(), pointMarkers[iPoint].end(), nameIds[iEntry]))
          entries.push_back(iEntry);
      if (entries.empty()) entries.push_back(interior);
      for (const auto iEntry : entries) {
        counts[4 * iEntry]++;
        if (!stencil.inside) counts[4 * iEntry + 1]++;
        if (stencil.onFace && stencil.distance > offTolerance) {
          counts[4 * iEntry + 2]++;
          maxima[2 * iEntry] = std::max(maxima[2 * iEntry], stencil.distance);
          maxima[2 * iEntry + 1] = std::max(maxima[2 * iEntry + 1], relDistance);
          sums[2 * iEntry].push_back(stencil.distance);
          sums[2 * iEntry + 1].push_back(relDistance);
          if (stencil.distance > stencil.faceSize) counts[4 * iEntry + 3]++;
        }
      }
    }
    std::vector<unsigned long> globalCounts(counts.size());
    std::vector<double> globalMaxima(maxima.size());
    CPassiveComm::Allreduce(counts.data(), globalCounts.data(), counts.size(), CPassiveComm::Op::SUM);
    CPassiveComm::Allreduce(maxima.data(), globalMaxima.data(), maxima.size(), CPassiveComm::Op::MAX);
    CAccurateSumBatch batch;
    for (const auto& terms : sums) batch.Add(terms);
    batch.Reduce();
    for (auto iEntry = 0ul; iEntry < nEntry; ++iEntry) {
      auto& stats = summary.markers[iEntry];
      stats.nPoint = globalCounts[4 * iEntry];
      stats.nOutside = globalCounts[4 * iEntry + 1];
      stats.nOff = globalCounts[4 * iEntry + 2];
      stats.nBeyondFace = globalCounts[4 * iEntry + 3];
      stats.maxDistance = globalMaxima[2 * iEntry];
      stats.maxRelDistance = globalMaxima[2 * iEntry + 1];
      stats.sumDistance = batch.Get(2 * iEntry);
      stats.sumRelDistance = batch.Get(2 * iEntry + 1);
    }
    summary.nPoint = CPassiveComm::AllreduceSum(nPoint);
    summary.nOutside = globalCounts[4 * nEntry];
    summary.maxDistance = globalMaxima[2 * nEntry];
    summary.maxRelDistance = globalMaxima[2 * nEntry + 1];

    const unsigned long nBeyond = globalCounts[4 * nEntry + 1];
    if (nBeyond > 0) {
      const double globalWorst = CPassiveComm::Allreduce(static_cast<double>(worstRatio), CPassiveComm::Op::MAX);
      CLocalFailure failure;
      if (worstRatio == globalWorst && worstRatio > 0.0) {
        su2double x[3] = {0.0, 0.0, 0.0};
        for (unsigned short iDim = 0; iDim < nDim; ++iDim) x[iDim] = coord[worstPoint * nDim + iDim];
        const auto& worst = stencils[worstPoint];
        failure.Set(1, newGeometry->nodes->GetGlobalIndex(worstPoint),
                    BeyondLimitText(nBeyond, nDim, x, worst.distance, worst.faceSize, domainSize));
      }
      CollectiveFailure(failure, CURRENT_FUNCTION);
    }
  }
  phaseTime[1] = SU2_MPI::Wtime() - phase;
  phase = SU2_MPI::Wtime();

  /*--- Values of the stencil points from the directory. ---*/
  std::vector<uint64_t> unique;
  for (const auto& stencil : stencils) unique.insert(unique.end(), stencil.gid, stencil.gid + stencil.nPoint);
  std::sort(unique.begin(), unique.end());
  unique.erase(std::unique(unique.begin(), unique.end()), unique.end());
  std::vector<su2double> donorValues(unique.size() * nField);
  {
    const auto fetched = directory.Fetch(unique);
    for (auto i = 0ul; i < unique.size(); ++i)
      UnpackFieldValues(&fetched[i * recordBytes], &donorValues[i * nField], nField);
  }
  phaseTime[2] = SU2_MPI::Wtime() - phase;
  phase = SU2_MPI::Wtime();

  /*--- The point kernel per owned point and time level; the first point (by global index) without an admissible donor
   *    state stops the transfer on all ranks. ---*/
  std::vector<su2double> newValues(nPoint * nField);
  {
    CLocalFailure failure;
    unsigned long nFixed[2] = {0, 0}, nTurbLimited = 0;
    const su2double* levelValues[4] = {};
    for (auto iPoint = 0ul; iPoint < nPoint; ++iPoint) {
      const auto& stencil = stencils[iPoint];
      unsigned long index[4] = {};
      for (unsigned short k = 0; k < stencil.nPoint; ++k)
        index[k] = std::lower_bound(unique.begin(), unique.end(), stencil.gid[k]) - unique.begin();
      for (unsigned short iLevel = 0; iLevel < nLevel; ++iLevel) {
        for (unsigned short k = 0; k < stencil.nPoint; ++k)
          levelValues[k] = &donorValues[index[k] * nField + iLevel * nPerLevel];
        bool fixed = false;
        if (!PointKernel(admissibility, stencil.nPoint, stencil.weight, levelValues,
                         &newValues[iPoint * nField + iLevel * nPerLevel], fixed, nTurbLimited)) {
          su2double x[3] = {0.0, 0.0, 0.0};
          for (unsigned short iDim = 0; iDim < nDim; ++iDim) x[iDim] = coord[iPoint * nDim + iDim];
          failure.Set(1, newGeometry->nodes->GetGlobalIndex(iPoint),
                      "No admissible donor state (flow and turbulence) near the point " + PointText(nDim, x) + ".");
        }
        nFixed[iLevel] += fixed;
      }
    }
    CollectiveFailure(failure, CURRENT_FUNCTION);
    unsigned long local[3] = {nFixed[0], nFixed[1], nTurbLimited}, global[3];
    CPassiveComm::Allreduce(local, global, 3, CPassiveComm::Op::SUM);
    summary.nFlowFixed = global[0];
    summary.nHistoryFixed = global[1];
    summary.nTurbLimited = global[2];
  }

  if (keepStencils) {
    for (auto iPoint = 0ul; iPoint < nPoint; ++iPoint) {
      const auto& stencil = stencils[iPoint];
      StencilRecord record;
      record.gid = newGeometry->nodes->GetGlobalIndex(iPoint);
      record.onFace = stencil.onFace;
      record.inside = stencil.inside;
      record.beyondLimit = stencil.beyondLimit;
      record.marker = stencil.marker;
      record.key = stencil.key;
      for (unsigned short k = 0; k < stencil.nPoint; ++k) {
        record.point[k] = stencil.gid[k];
        record.weight[k] = stencil.weight[k];
      }
      stencilRecords.push_back(record);
    }
  }
  stencils = std::vector<CDistributedLocator::Stencil>();
  donorValues = std::vector<su2double>();

  /*--- The owned rows of the new arrays: U^n into the solution (and Solution_time_n), U^(n-1) or U^n into
   *    Solution_time_n1. ---*/
  for (unsigned short iLevel = 0; iLevel < nLevel; ++iLevel) {
    for (const auto iSol : solverIndices) {
      auto& values = LevelArray(solver[MESH_0][iSol]->GetNodes(), iLevel);
      const unsigned short offset = iLevel * nPerLevel + (iSol == FLOW_SOL ? 0 : nVarFlow);
      for (auto iPoint = 0ul; iPoint < nPoint; ++iPoint)
        for (unsigned long iVar = 0; iVar < values.cols(); ++iVar)
          values(iPoint, iVar) = newValues[iPoint * nField + offset + iVar];
    }
  }
  for (const auto iSol : solverIndices) {
    auto* nodes = solver[MESH_0][iSol]->GetNodes();
    if (arrays.hasTimeN) nodes->GetSolution_time_n() = nodes->GetSolution();
    if (arrays.hasTimeN1 && !arrays.interpolateTimeN1) nodes->GetSolution_time_n1() = nodes->GetSolution_time_n();
  }
  phaseTime[3] = SU2_MPI::Wtime() - phase;
  phase = SU2_MPI::Wtime();

  /*--- Volumes and integrals of the flow variables (accurate global sums over the owned points). ---*/
  {
    const auto nDonor = donorGeometry->GetnPointDomain();
    const auto& donorFlow = donor.solver[MESH_0][FLOW_SOL]->GetNodes()->GetSolution();
    std::vector<double> donorVolume(nDonor), newVolume(nPoint);
    std::vector<std::vector<su2double>> donorTerms(nVarFlow, std::vector<su2double>(nDonor)),
        newTerms(nVarFlow, std::vector<su2double>(nPoint));
    for (auto iPoint = 0ul; iPoint < nDonor; ++iPoint) {
      donorVolume[iPoint] = SU2_TYPE::GetValue(donorGeometry->nodes->GetVolume(iPoint));
      for (unsigned short iVar = 0; iVar < nVarFlow; ++iVar)
        donorTerms[iVar][iPoint] = donorVolume[iPoint] * donorFlow(iPoint, iVar);
    }
    for (auto iPoint = 0ul; iPoint < nPoint; ++iPoint) {
      newVolume[iPoint] = SU2_TYPE::GetValue(newGeometry->nodes->GetVolume(iPoint));
      for (unsigned short iVar = 0; iVar < nVarFlow; ++iVar)
        newTerms[iVar][iPoint] = newVolume[iPoint] * newValues[iPoint * nField + iVar];
    }
    CAccurateSumBatch batch;
    const auto qDonor = batch.Add(donorVolume), qNew = batch.Add(newVolume);
    std::vector<size_t> qDonorVar(nVarFlow), qNewVar(nVarFlow);
    for (unsigned short iVar = 0; iVar < nVarFlow; ++iVar) {
      qDonorVar[iVar] = batch.AddActive(donorTerms[iVar]);
      qNewVar[iVar] = batch.AddActive(newTerms[iVar]);
    }
    batch.Reduce();
    summary.donorVolume = batch.Get(qDonor);
    summary.newVolume = batch.Get(qNew);
    for (unsigned short iVar = 0; iVar < nVarFlow; ++iVar) {
      summary.donorIntegral.push_back(batch.GetActive(qDonorVar[iVar]));
      summary.newIntegral.push_back(batch.GetActive(qNewVar[iVar]));
    }
  }

  /*--- Round trip donor -> new -> donor of the flow variables (U^n), a read-only diagnostic: the canonical location
   *    of the owned donor points in the new mesh, the transferred values from a directory of the new mesh, raw P1
   *    interpolation; donor points beyond the distance limit of the new mesh are counted, never an error. ---*/
  if (roundTripCheck) {
    const size_t flowBytes = nVarFlow * FieldValueBytes();
    CPointDirectory newDirectory;
    {
      std::vector<uint64_t> gids(nPoint);
      std::vector<char> records(nPoint * flowBytes);
      for (auto iPoint = 0ul; iPoint < nPoint; ++iPoint) {
        gids[iPoint] = newGeometry->nodes->GetGlobalIndex(iPoint);
        PackFieldValues(&records[iPoint * flowBytes], &newValues[iPoint * nField], nVarFlow);
      }
      newDirectory.Build(gids, records, flowBytes, "new");
    }
    const auto nDonor = donorGeometry->GetnPointDomain();
    std::vector<passivedouble> donorCoord(nDonor * nDim);
    for (auto iPoint = 0ul; iPoint < nDonor; ++iPoint)
      for (unsigned short iDim = 0; iDim < nDim; ++iDim)
        donorCoord[iPoint * nDim + iDim] = SU2_TYPE::GetValue(donorGeometry->nodes->GetCoord(iPoint, iDim));
    auto donorMarkers = PointMarkerIds(*donorGeometry, donorTags, *config);
    donorMarkers.resize(nDonor);
    std::vector<CDistributedLocator::Stencil> inverse;
    passivedouble inverseLimit = 0.0;
    {
      CDistributedLocator locator(*newGeometry, newTags, *config, 0.0);
      inverse = locator.Locate(donorCoord, donorMarkers);
      unsigned long nBeyond = 0;
      passivedouble worst = 0.0;
      for (const auto& stencil : inverse) {
        if (!stencil.beyondLimit) continue;
        nBeyond++;
        worst = std::max(worst, stencil.distance / locator.GetDistanceLimit(stencil.faceSize));
      }
      summary.nRoundTripBeyond = CPassiveComm::AllreduceSum(nBeyond);
      summary.roundTripWorstRatio = CPassiveComm::Allreduce(static_cast<double>(worst), CPassiveComm::Op::MAX);
      inverseLimit = locator.GetDistanceLimit(0.0);
    }
    std::vector<uint64_t> gids;
    for (const auto& stencil : inverse) gids.insert(gids.end(), stencil.gid, stencil.gid + stencil.nPoint);
    std::sort(gids.begin(), gids.end());
    gids.erase(std::unique(gids.begin(), gids.end()), gids.end());
    const auto fetched = newDirectory.Fetch(gids);
    std::vector<su2double> values(gids.size() * nVarFlow);
    for (auto i = 0ul; i < gids.size(); ++i) UnpackFieldValues(&fetched[i * flowBytes], &values[i * nVarFlow], nVarFlow);

    const auto& donorFlow = donor.solver[MESH_0][FLOW_SOL]->GetNodes()->GetSolution();
    std::vector<double> minValue(nVarFlow, std::numeric_limits<double>::max()),
        maxValue(nVarFlow, std::numeric_limits<double>::lowest()), linf(nVarFlow, 0.0);
    std::vector<std::vector<double>> squares(nVarFlow, std::vector<double>(nDonor));
    for (auto iPoint = 0ul; iPoint < nDonor; ++iPoint) {
      const auto& stencil = inverse[iPoint];
      for (unsigned short iVar = 0; iVar < nVarFlow; ++iVar) {
        const double donorValue = SU2_TYPE::GetValue(donorFlow(iPoint, iVar));
        minValue[iVar] = std::min(minValue[iVar], donorValue);
        maxValue[iVar] = std::max(maxValue[iVar], donorValue);
        su2double value = 0.0;
        for (unsigned short k = 0; k < stencil.nPoint; ++k) {
          const auto j = std::lower_bound(gids.begin(), gids.end(), stencil.gid[k]) - gids.begin();
          value += stencil.weight[k] * values[j * nVarFlow + iVar];
        }
        const double diff = std::fabs(SU2_TYPE::GetValue(value) - donorValue);
        squares[iVar][iPoint] = diff * diff;
        linf[iVar] = std::max(linf[iVar], diff);
      }
    }
    std::vector<double> globalMin(nVarFlow), globalMax(nVarFlow), globalLinf(nVarFlow);
    CPassiveComm::Allreduce(minValue.data(), globalMin.data(), nVarFlow, CPassiveComm::Op::MIN);
    CPassiveComm::Allreduce(maxValue.data(), globalMax.data(), nVarFlow, CPassiveComm::Op::MAX);
    CPassiveComm::Allreduce(linf.data(), globalLinf.data(), nVarFlow, CPassiveComm::Op::MAX);
    CAccurateSumBatch batch;
    for (const auto& terms : squares) batch.Add(terms);
    batch.Reduce();
    const auto nDonorGlobal = CPassiveComm::AllreduceSum(nDonor);
    summary.roundTripL2.assign(nVarFlow, 0.0);
    summary.roundTripLinf.assign(nVarFlow, 0.0);
    for (unsigned short iVar = 0; iVar < nVarFlow; ++iVar) {
      double range = globalMax[iVar] - globalMin[iVar];
      if (!(range > 0.0)) range = std::max(std::fabs(globalMax[iVar]), 1.0);
      summary.roundTripL2[iVar] = std::sqrt(batch.Get(iVar) / std::max(nDonorGlobal, 1ul)) / range;
      summary.roundTripLinf[iVar] = globalLinf[iVar] / range;
    }
    (void)inverseLimit;
  }
  phaseTime[4] = SU2_MPI::Wtime() - phase;
  phase = SU2_MPI::Wtime();

  FinishTransfer(config, geometry, solver, arrays);
  phaseTime[5] = SU2_MPI::Wtime() - phase;
  summary.time = SU2_MPI::Wtime() - startTime;

  /*--- Timing per phase (largest over the ranks) and the query balance (largest / mean). ---*/
  double maxTime[6];
  CPassiveComm::Allreduce(phaseTime, maxTime, 6, CPassiveComm::Op::MAX);
  unsigned long queries[2] = {nQueriesSent, nQueriesReceived}, maxQueries[2], sumQueries[2];
  CPassiveComm::Allreduce(queries, maxQueries, 2, CPassiveComm::Op::MAX);
  CPassiveComm::Allreduce(queries, sumQueries, 2, CPassiveComm::Op::SUM);
  const double meanReceived = static_cast<double>(sumQueries[1]) / SU2_MPI::GetSize();
  std::ostringstream timing;
  timing << std::setprecision(3) << "Distributed transfer on " << SU2_MPI::GetSize() << " rank(s), " << summary.time
         << " s (largest over the ranks: directory " << maxTime[0] << " s, location " << maxTime[1] << " s, fetch "
         << maxTime[2] << " s, kernel " << maxTime[3] << " s, integrals and round trip " << maxTime[4]
         << " s, finish " << maxTime[5] << " s); queries " << sumQueries[0] << " in " << nChunk
         << " chunk(s), received per rank max/mean " << (meanReceived > 0 ? maxQueries[1] / meanReceived : 0.0)
         << (meanReceived > 0 && maxQueries[1] > 4 * meanReceived ? " (imbalance warning)" : "") << ".";
  if (rank == MASTER_NODE) PrintSummary(nDim, solver[MESH_0][TURB_SOL] != nullptr, timing.str());
}

void CBarycentricTransfer::TransferGathered(CConfig* config, const CMeshDonor& donor, CGeometry** geometry,
                                            CSolver*** solver) {
  SU2_ZONE_SCOPED

  const auto startTime = SU2_MPI::Wtime();
  const int rank = SU2_MPI::GetRank();
  summary = Summary();
  stencilRecords.clear();

  const auto arrays = CheckProblem("barycentric", config, donor, geometry, solver);
  const auto& solverIndices = arrays.solverIndices;

  auto* donorGeometry = donor.geometry[MESH_0];
  auto* newGeometry = geometry[MESH_0];
  const auto nDim = newGeometry->GetnDim();

  auto* flowSolver = solver[MESH_0][FLOW_SOL];
  const bool hasTimeN = arrays.hasTimeN, hasTimeN1 = arrays.hasTimeN1;
  summary.interpolateTimeN1 = arrays.interpolateTimeN1;
  summary.nTimeLevels = hasTimeN + hasTimeN1;

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
  constexpr unsigned short kMaxVar = 8;
  if (nVarFlow > kMaxVar || nVarTurb > kMaxVar) SU2_MPI::Error("Too many variables.", CURRENT_FUNCTION);
  const CTransferAdmissibility admissibility(*fluidModel, nDim, turbSolver, arrays.sst);

  /*--- Fields of each point: per time level (U^n, and U^(n-1) for 2nd order) the flow and turbulence variables. ---*/
  const unsigned short nLevel = summary.interpolateTimeN1 ? 2 : 1;
  const unsigned short nPerLevel = nVarFlow + nVarTurb;
  const unsigned short nField = nLevel * nPerLevel;

  /*--- Both meshes and the donor solution on the master rank, in the global numbering of the meshes (CMeshGather),
   *    where the serial interpolation below runs; the new values go back to the ranks that own the points. The
   *    marker names: the config describes the new mesh, the donor carries its own names. ---*/

  const auto gatherStart = SU2_MPI::Wtime();
  std::vector<std::string> newTags;
  for (unsigned short iMarker = 0; iMarker < newGeometry->GetnMarker(); ++iMarker)
    newTags.push_back(config->GetMarker_All_TagBound(iMarker));
  const CMeshGather donorGather(*donorGeometry), newGather(*newGeometry);
  const auto donorMesh = donorGather.GatherMesh(*config, DonorMarkerTags("barycentric", donor), true);
  const auto newMesh = newGather.GatherMesh(*config, newTags, true);
  std::vector<su2double> donorValues;
  {
    const auto nPointDomain = donorGeometry->GetnPointDomain();
    std::vector<su2double> local(nPointDomain * nField);
    for (unsigned short iLevel = 0; iLevel < nLevel; ++iLevel) {
      for (const auto iSol : solverIndices) {
        const auto& values = LevelArray(donor.solver[MESH_0][iSol]->GetNodes(), iLevel);
        const unsigned short offset = iLevel * nPerLevel + (iSol == FLOW_SOL ? 0 : nVarFlow);
        for (auto iPoint = 0ul; iPoint < nPointDomain; ++iPoint)
          for (unsigned long iVar = 0; iVar < values.cols(); ++iVar)
            local[iPoint * nField + offset + iVar] = values(iPoint, iVar);
      }
    }
    donorValues = donorGather.Gather(local.data(), nField);
  }
  const auto gatherTime = SU2_MPI::Wtime() - gatherStart;

  std::vector<su2double> newValues;
  if (donorGather.IsRoot()) {
    const auto nPoint = newMesh.GetnPoint();
    const auto nPointDonor = donorMesh.GetnPoint();
    newValues.assign(nPoint * nField, 0.0);
    summary.nPoint = nPoint;

    /*--- Donor points and weights of each new point. Distances are accepted up to the limit of the locator, with
     *    2 ADAP_HAUSD as the tolerance of the remesher's boundary approximation. ---*/

    const auto newPointMarkers = PointMarkers(newMesh);
    std::vector<CBarycentricLocator::Stencil> stencils(nPoint);
    {
      CBarycentricLocator locator(donorMesh, 2.0 * config->GetAdap_Hausd());
      summary.distanceLimit = locator.GetDistanceLimit(0.0);

      /*--- Statistics per marker of the new mesh, then the points on no marker. ---*/
      for (const auto& marker : newMesh.markers) {
        summary.markers.emplace_back();
        summary.markers.back().name = marker.name;
      }
      summary.markers.emplace_back();
      summary.markers.back().name = "(interior)";
      const auto interior = summary.markers.size() - 1;
      const su2double offTolerance = 1e-12 * locator.GetDomainSize();

      unsigned long nBeyond = 0;
      su2double worstRatio = 0.0;
      CBarycentricLocator::Stencil worst;
      unsigned long worstPoint = 0;

      for (auto iPoint = 0ul; iPoint < nPoint; ++iPoint) {
        su2double x[3] = {0.0};
        for (unsigned short iDim = 0; iDim < nDim; ++iDim) x[iDim] = newMesh.coord[iPoint * nDim + iDim];
        stencils[iPoint] = LocatePoint(locator, x, newPointMarkers[iPoint]);
        const auto& stencil = stencils[iPoint];
        const su2double relDistance = stencil.onFace ? su2double(stencil.distance / stencil.faceSize) : su2double(0.0);
        if (!stencil.inside) summary.nOutside++;
        summary.maxDistance = max(summary.maxDistance, stencil.distance);
        summary.maxRelDistance = max(summary.maxRelDistance, relDistance);
        if (stencil.beyondLimit) {
          nBeyond++;
          const su2double ratio = stencil.distance / locator.GetDistanceLimit(stencil.faceSize);
          if (ratio > worstRatio) {
            worstRatio = ratio;
            worst = stencil;
            worstPoint = iPoint;
          }
        }

        std::vector<unsigned long> entries;
        for (const auto& name : newPointMarkers[iPoint]) {
          for (auto iEntry = 0ul; iEntry < interior; ++iEntry)
            if (summary.markers[iEntry].name == name) entries.push_back(iEntry);
        }
        if (entries.empty()) entries.push_back(interior);
        for (const auto iEntry : entries) {
          auto& stats = summary.markers[iEntry];
          stats.nPoint++;
          if (!stencil.inside) stats.nOutside++;
          if (stencil.onFace && stencil.distance > offTolerance) {
            stats.nOff++;
            stats.maxDistance = max(stats.maxDistance, stencil.distance);
            stats.sumDistance += stencil.distance;
            stats.maxRelDistance = max(stats.maxRelDistance, relDistance);
            stats.sumRelDistance += relDistance;
            if (stencil.distance > stencil.faceSize) stats.nBeyondFace++;
          }
        }
      }

      if (nBeyond > 0) {
        su2double x[3] = {0.0};
        for (unsigned short iDim = 0; iDim < nDim; ++iDim) x[iDim] = newMesh.coord[worstPoint * nDim + iDim];
        SU2_MPI::Error(BeyondLimitText(nBeyond, nDim, x, worst.distance, worst.faceSize, locator.GetDomainSize()),
                       CURRENT_FUNCTION);
      }
    }

    /*--- Flow and turbulence of the time levels, point by point (PointKernel). ---*/

    auto InterpolateLevel = [&](unsigned short iLevel, unsigned long& nFixed) {
      const unsigned short offset = iLevel * nPerLevel;
      const su2double* levelValues[4] = {};
      passivedouble weight[4] = {};
      for (auto iPoint = 0ul; iPoint < nPoint; ++iPoint) {
        const auto& stencil = stencils[iPoint];
        for (unsigned short k = 0; k < stencil.nPoint; ++k) {
          levelValues[k] = &donorValues[stencil.point[k] * nField + offset];
          weight[k] = SU2_TYPE::GetValue(stencil.weight[k]);
        }
        bool fixed = false;
        if (!PointKernel(admissibility, stencil.nPoint, weight, levelValues, &newValues[iPoint * nField + offset], fixed,
                         summary.nTurbLimited)) {
          su2double x[3] = {0.0};
          for (unsigned short iDim = 0; iDim < nDim; ++iDim) x[iDim] = newMesh.coord[iPoint * nDim + iDim];
          SU2_MPI::Error("No admissible donor state (flow and turbulence) near the point " + PointText(nDim, x) + ".",
                         CURRENT_FUNCTION);
        }
        nFixed += fixed;
      }
    };

    /*--- U^n, once; U^(n-1): 2nd order only (else U^n, below). ---*/
    InterpolateLevel(0, summary.nFlowFixed);
    if (summary.interpolateTimeN1) InterpolateLevel(1, summary.nHistoryFixed);

    if (keepStencils) {
      for (auto iPoint = 0ul; iPoint < nPoint; ++iPoint) {
        const auto& stencil = stencils[iPoint];
        StencilRecord record;
        record.gid = iPoint;
        record.onFace = stencil.onFace;
        record.inside = stencil.inside;
        record.beyondLimit = stencil.beyondLimit;
        record.marker = stencil.marker;
        record.key = stencil.key;
        for (unsigned short k = 0; k < stencil.nPoint; ++k) {
          record.point[k] = stencil.point[k];
          record.weight[k] = SU2_TYPE::GetValue(stencil.weight[k]);
        }
        stencilRecords.push_back(record);
      }
    }

    /*--- Conservation defects and round-trip difference (donor -> new -> donor) of the flow variables. ---*/

    Integrals(donorMesh, donorValues, nField, nVarFlow, summary.donorVolume, summary.donorIntegral);
    Integrals(newMesh, newValues, nField, nVarFlow, summary.newVolume, summary.newIntegral);

    if (roundTripCheck) {
      std::vector<su2double> minValue(nVarFlow, std::numeric_limits<passivedouble>::max());
      std::vector<su2double> maxValue(nVarFlow, std::numeric_limits<passivedouble>::lowest());
      for (auto iPoint = 0ul; iPoint < nPointDonor; ++iPoint) {
        for (unsigned short iVar = 0; iVar < nVarFlow; ++iVar) {
          minValue[iVar] = min(minValue[iVar], donorValues[iPoint * nField + iVar]);
          maxValue[iVar] = max(maxValue[iVar], donorValues[iPoint * nField + iVar]);
        }
      }

      summary.roundTripL2.assign(nVarFlow, 0.0);
      summary.roundTripLinf.assign(nVarFlow, 0.0);
      CBarycentricLocator locator(newMesh);
      const auto donorPointMarkers = PointMarkers(donorMesh);
      for (auto iPoint = 0ul; iPoint < nPointDonor; ++iPoint) {
        su2double x[3] = {0.0};
        for (unsigned short iDim = 0; iDim < nDim; ++iDim) x[iDim] = donorMesh.coord[iPoint * nDim + iDim];
        const auto stencil = LocatePoint(locator, x, donorPointMarkers[iPoint]);
        if (stencil.beyondLimit) {
          summary.nRoundTripBeyond++;
          summary.roundTripWorstRatio = std::max(
              summary.roundTripWorstRatio,
              SU2_TYPE::GetValue(stencil.distance / locator.GetDistanceLimit(stencil.faceSize)));
        }
        for (unsigned short iVar = 0; iVar < nVarFlow; ++iVar) {
          su2double value = 0.0;
          for (unsigned short k = 0; k < stencil.nPoint; ++k)
            value += stencil.weight[k] * newValues[stencil.point[k] * nField + iVar];
          const su2double diff = fabs(value - donorValues[iPoint * nField + iVar]);
          summary.roundTripL2[iVar] += diff * diff;
          summary.roundTripLinf[iVar] = max(summary.roundTripLinf[iVar], diff);
        }
      }
      for (unsigned short iVar = 0; iVar < nVarFlow; ++iVar) {
        su2double range = maxValue[iVar] - minValue[iVar];
        if (!(range > 0.0)) range = max(fabs(maxValue[iVar]), su2double(1.0));
        summary.roundTripL2[iVar] = sqrt(summary.roundTripL2[iVar] / max(nPointDonor, 1ul)) / range;
        summary.roundTripLinf[iVar] /= range;
      }
    }
  }

  /*--- The new values to the ranks that own the points: U^n into the solution (and Solution_time_n), U^(n-1) or U^n
   *    into Solution_time_n1. ---*/

  const auto scatterStart = SU2_MPI::Wtime();
  {
    const auto nPointDomain = newGeometry->GetnPointDomain();
    std::vector<su2double> local(nPointDomain * nField);
    newGather.Scatter(newValues, nField, local.data());
    for (unsigned short iLevel = 0; iLevel < nLevel; ++iLevel) {
      for (const auto iSol : solverIndices) {
        auto& values = LevelArray(solver[MESH_0][iSol]->GetNodes(), iLevel);
        const unsigned short offset = iLevel * nPerLevel + (iSol == FLOW_SOL ? 0 : nVarFlow);
        for (auto iPoint = 0ul; iPoint < nPointDomain; ++iPoint)
          for (unsigned long iVar = 0; iVar < values.cols(); ++iVar)
            values(iPoint, iVar) = local[iPoint * nField + offset + iVar];
      }
    }
  }
  for (const auto iSol : solverIndices) {
    auto* nodes = solver[MESH_0][iSol]->GetNodes();
    if (hasTimeN) nodes->GetSolution_time_n() = nodes->GetSolution();
    if (hasTimeN1 && !summary.interpolateTimeN1) nodes->GetSolution_time_n1() = nodes->GetSolution_time_n();
  }
  const auto scatterTime = SU2_MPI::Wtime() - scatterStart;

  BroadcastSummary();

  FinishTransfer(config, geometry, solver, arrays);
  summary.time = SU2_MPI::Wtime() - startTime;

  if (rank != MASTER_NODE) return;
  std::ostringstream timing;
  timing << "Gathered transfer (MPI-1 reference): donor and new mesh with the donor solution gathered on rank "
         << MASTER_NODE << " (" << gatherTime << " s), new values sent to the ranks of their points (" << scatterTime
         << " s), total " << summary.time << " s.";
  PrintSummary(nDim, solver[MESH_0][TURB_SOL] != nullptr, timing.str());
}

void CBarycentricTransfer::PrintSummary(unsigned short nDim, bool turbulence, const std::string& timing) const {
  const auto& s = summary;
  std::vector<string> names = {"Density"};
  for (unsigned short iDim = 0; iDim < nDim; ++iDim) names.push_back(string("Momentum-") + "xyz"[iDim]);
  names.push_back("Energy");
  const auto nVarFlow = s.donorIntegral.size();

  su2double momentumNorm = 0.0;
  for (unsigned short iDim = 0; iDim < nDim && iDim + 1 < nVarFlow; ++iDim) momentumNorm += pow(s.donorIntegral[iDim + 1], 2);
  momentumNorm = sqrt(momentumNorm);

  cout << endl << "------------------------ Solution Transfer (P1) -------------------------" << endl;
  cout << timing << endl;
  cout << std::scientific << std::setprecision(3);
  cout << "Barycentric interpolation of the donor solution at " << s.nPoint << " points." << endl;
  cout << "Points outside the donor mesh: " << s.nOutside << ". Points on a marker take the closest point of the "
       << "donor boundary of the same marker, other points outside the donor the closest donor boundary point." << endl;
  cout << "Distance to the closest donor point (mean over the points off the donor boundary), absolute and relative "
       << "to the longest edge of the donor face:" << endl;
  cout << "  " << std::setw(16) << "Marker" << std::setw(9) << "Points" << std::setw(9) << "Outside" << std::setw(9)
       << "Off" << std::setw(11) << "Max dist" << std::setw(11) << "Mean dist" << std::setw(11) << "Max rel"
       << std::setw(11) << "Mean rel" << std::setw(9) << "> face" << endl;
  for (const auto& m : s.markers) {
    const su2double nOff = max(m.nOff, 1ul);
    cout << "  " << std::setw(16) << m.name.substr(0, 15) << std::setw(9) << m.nPoint << std::setw(9) << m.nOutside
         << std::setw(9) << m.nOff << std::setw(11) << m.maxDistance << std::setw(11) << m.sumDistance / nOff
         << std::setw(11) << m.maxRelDistance << std::setw(11) << m.sumRelDistance / nOff << std::setw(9)
         << m.nBeyondFace << endl;
  }
  cout << "Distance accepted: the face size, at least " << s.distanceLimit
       << " (2 ADAP_HAUSD or 1e-3 x the domain size)." << endl;
  cout << "States not admissible after the interpolation (flow and turbulence of one donor point used): " << s.nFlowFixed
       << "." << endl;
  if (s.nTimeLevels > 0) {
    cout << "Time history: Solution_time_n = the solution (U^n, interpolated once)";
    if (s.interpolateTimeN1) {
      cout << ", Solution_time_n1 = U^(n-1) interpolated with the same stencils (states not admissible: "
           << s.nHistoryFixed << ")";
    } else if (s.nTimeLevels > 1) {
      cout << ", Solution_time_n1 = U^n (not used by this time marching)";
    }
    cout << "." << endl;
  }
  if (turbulence) cout << "Turbulence values limited to the bounds of the solver: " << s.nTurbLimited << "." << endl;
  cout << "Domain volume: donor " << s.donorVolume << ", new " << s.newVolume << ", relative change "
       << (s.newVolume - s.donorVolume) / s.donorVolume << "." << endl;
  cout << "Change of the integrals (not conserved by the interpolation), relative to the donor integral"
       << " (momentum: to the norm of the donor momentum integral):" << endl;
  for (unsigned short iVar = 0; iVar < nVarFlow; ++iVar) {
    const bool momentum = (iVar > 0 && iVar <= nDim);
    const su2double scale = momentum ? momentumNorm : fabs(s.donorIntegral[iVar]);
    cout << "  " << std::setw(10) << names[iVar] << ": " << (s.newIntegral[iVar] - s.donorIntegral[iVar]) / scale
         << endl;
  }
  if (roundTripCheck && s.roundTripL2.size() == nVarFlow) {
    cout << "Round trip donor -> new -> donor, difference relative to the donor range (RMS, max):" << endl;
    for (unsigned short iVar = 0; iVar < nVarFlow; ++iVar) {
      cout << "  " << std::setw(10) << names[iVar] << ": " << s.roundTripL2[iVar] << ", " << s.roundTripLinf[iVar]
           << endl;
    }
    if (s.nRoundTripBeyond > 0) {
      cout << "  (" << s.nRoundTripBeyond << " donor points beyond the distance limit of the new mesh, largest ratio "
           << s.roundTripWorstRatio << "; diagnostic only)" << endl;
    }
  }
  cout.unsetf(std::ios_base::floatfield);
  cout << std::setprecision(6);
}

void CBarycentricTransfer::BroadcastSummary() {
  if (SU2_MPI::GetSize() == 1) return;
  auto& s = summary;
  unsigned long counts[] = {s.nPoint, s.nOutside, s.nFlowFixed, s.nHistoryFixed, s.nTurbLimited,
                            s.donorIntegral.size()};
  SU2_MPI::Bcast(counts, 6, MPI_UNSIGNED_LONG, MASTER_NODE, SU2_MPI::GetComm());
  s.nPoint = counts[0];
  s.nOutside = counts[1];
  s.nFlowFixed = counts[2];
  s.nHistoryFixed = counts[3];
  s.nTurbLimited = counts[4];
  su2double values[] = {s.maxDistance, s.maxRelDistance, s.distanceLimit, s.donorVolume, s.newVolume};
  SU2_MPI::Bcast(values, 5, MPI_DOUBLE, MASTER_NODE, SU2_MPI::GetComm());
  s.maxDistance = values[0];
  s.maxRelDistance = values[1];
  s.distanceLimit = values[2];
  s.donorVolume = values[3];
  s.newVolume = values[4];
  s.donorIntegral.resize(counts[5]);
  s.newIntegral.resize(counts[5]);
  if (counts[5] > 0) {
    const int n = static_cast<int>(counts[5]);
    SU2_MPI::Bcast(s.donorIntegral.data(), n, MPI_DOUBLE, MASTER_NODE, SU2_MPI::GetComm());
    SU2_MPI::Bcast(s.newIntegral.data(), n, MPI_DOUBLE, MASTER_NODE, SU2_MPI::GetComm());
  }
}

CSolutionTransfer::Report CBarycentricTransfer::GetReport() const {
  Report report;
  report.nOutside = summary.nOutside;
  report.maxDistance = SU2_TYPE::GetValue(summary.maxDistance);
  report.maxRelDistance = SU2_TYPE::GetValue(summary.maxRelDistance);

  /*--- Change of the integrals, relative as in the log (momentum: to the norm of the donor momentum integral). ---*/
  const auto nVar = summary.donorIntegral.size();
  if (nVar < 3 || summary.newIntegral.size() != nVar) return report;
  const auto nDim = nVar - 2;
  su2double momentumNorm = 0.0;
  for (auto iDim = 0ul; iDim < nDim; ++iDim) momentumNorm += pow(summary.donorIntegral[iDim + 1], 2);
  momentumNorm = sqrt(momentumNorm);
  report.conservationDefect = 0.0;
  for (auto iVar = 0ul; iVar < nVar; ++iVar) {
    const bool momentum = (iVar > 0 && iVar <= nDim);
    const su2double scale = momentum ? momentumNorm : fabs(summary.donorIntegral[iVar]);
    if (!(scale > 0.0)) continue;
    report.conservationDefect = max(report.conservationDefect, SU2_TYPE::GetValue(
                                        fabs(summary.newIntegral[iVar] - summary.donorIntegral[iVar]) / scale));
  }
  return report;
}
