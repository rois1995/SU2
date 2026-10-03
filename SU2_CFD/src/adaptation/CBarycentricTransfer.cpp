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
#include "../../../Common/include/adaptation/CMeshGather.hpp"
#include "../../../Common/include/geometry/CGeometry.hpp"
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

/*--- Weights of the closest point of the segment [a,b] to p. ---*/
void ClosestPointSegment(unsigned short nDim, const su2double* a, const su2double* b, const su2double* p,
                         su2double* weight) {
  su2double length2 = 0.0, projection = 0.0;
  for (unsigned short iDim = 0; iDim < nDim; ++iDim) {
    length2 += pow(b[iDim] - a[iDim], 2);
    projection += (p[iDim] - a[iDim]) * (b[iDim] - a[iDim]);
  }
  su2double t = (length2 > 0.0) ? projection / length2 : su2double(0.0);
  t = min(max(t, su2double(0.0)), su2double(1.0));
  weight[0] = 1.0 - t;
  weight[1] = t;
}

/*--- Weights of the closest point of the triangle (a,b,c) to p in 3D, by the Voronoi regions of the triangle
 *    (Ericson, Real-Time Collision Detection, 5.1.5). The weights are in [0,1]. ---*/
void ClosestPointTriangle(const su2double* a, const su2double* b, const su2double* c, const su2double* p,
                          su2double* weight) {
  auto dot = [](const su2double* u, const su2double* v) { return u[0] * v[0] + u[1] * v[1] + u[2] * v[2]; };
  su2double ab[3], ac[3], ap[3], bp[3], cp[3];
  for (int i = 0; i < 3; ++i) {
    ab[i] = b[i] - a[i];
    ac[i] = c[i] - a[i];
    ap[i] = p[i] - a[i];
    bp[i] = p[i] - b[i];
    cp[i] = p[i] - c[i];
  }
  auto set = [weight](su2double wa, su2double wb, su2double wc) {
    weight[0] = wa;
    weight[1] = wb;
    weight[2] = wc;
  };

  const su2double d1 = dot(ab, ap), d2 = dot(ac, ap);
  if (d1 <= 0.0 && d2 <= 0.0) return set(1.0, 0.0, 0.0);

  const su2double d3 = dot(ab, bp), d4 = dot(ac, bp);
  if (d3 >= 0.0 && d4 <= d3) return set(0.0, 1.0, 0.0);

  const su2double vc = d1 * d4 - d3 * d2;
  if (vc <= 0.0 && d1 >= 0.0 && d3 <= 0.0) {
    const su2double v = d1 / (d1 - d3);
    return set(1.0 - v, v, 0.0);
  }

  const su2double d5 = dot(ab, cp), d6 = dot(ac, cp);
  if (d6 >= 0.0 && d5 <= d6) return set(0.0, 0.0, 1.0);

  const su2double vb = d5 * d2 - d1 * d6;
  if (vb <= 0.0 && d2 >= 0.0 && d6 <= 0.0) {
    const su2double w = d2 / (d2 - d6);
    return set(1.0 - w, 0.0, w);
  }

  const su2double va = d3 * d6 - d5 * d4;
  if (va <= 0.0 && (d4 - d3) >= 0.0 && (d5 - d6) >= 0.0) {
    const su2double w = (d4 - d3) / ((d4 - d3) + (d5 - d6));
    return set(0.0, 1.0 - w, w);
  }

  /*--- Inside the face region: weights proportional to va, vb, vc (positive up to round-off). A degenerate (zero-area)
   *    triangle has no such region: closest point of its edges. ---*/
  const su2double wa = max(su2double(0.0), va), wb = max(su2double(0.0), vb), wc = max(su2double(0.0), vc);
  const su2double sum = wa + wb + wc;
  if (!(sum > 0.0)) {
    const su2double* xNode[] = {a, b, c};
    su2double best = std::numeric_limits<passivedouble>::max();
    for (int i = 0; i < 3; ++i) {
      const int j = (i + 1) % 3;
      su2double w2[2] = {}, d2 = 0.0;
      ClosestPointSegment(3, xNode[i], xNode[j], p, w2);
      for (int k = 0; k < 3; ++k) d2 += pow(p[k] - w2[0] * xNode[i][k] - w2[1] * xNode[j][k], 2);
      if (d2 < best) {
        best = d2;
        weight[0] = weight[1] = weight[2] = 0.0;
        weight[i] = w2[0];
        weight[j] = w2[1];
      }
    }
    return;
  }
  set(wa / sum, wb / sum, wc / sum);
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
  const unsigned short faceType = (nDim == 2) ? LINE : TRIANGLE;
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

  /*--- Elements. The weights of the ADT are in the order of these nodes. ---*/

  elemConn = mesh.elem;
  if (elemConn.empty()) SU2_MPI::Error("The mesh has no elements.", CURRENT_FUNCTION);

  /*--- Faces of all markers, and of each marker name. ---*/

  for (const auto& marker : mesh.markers) {
    std::vector<unsigned long>* named = nullptr;
    if (!marker.name.empty()) {
      const auto it = markerIndex.emplace(marker.name, markerFaceConn.size()).first;
      if (it->second == markerFaceConn.size()) markerFaceConn.emplace_back();
      named = &markerFaceConn[it->second];
    }
    faceConn.insert(faceConn.end(), marker.elem.begin(), marker.elem.end());
    if (named) named->insert(named->end(), marker.elem.begin(), marker.elem.end());
  }
  if (faceConn.empty()) SU2_MPI::Error("The mesh has no boundary faces.", CURRENT_FUNCTION);

  /*--- Local ADTs (the global mode would gather the meshes of all ranks with collective calls). ---*/

  auto MakeADT = [this](const std::vector<unsigned long>& conn, unsigned short vtkType, unsigned short nNode) {
    const auto nItem = conn.size() / nNode;
    std::vector<su2double> coordCopy = coord;
    std::vector<unsigned long> connCopy = conn;
    std::vector<unsigned short> types(nItem, vtkType), markers(nItem, 0);
    std::vector<unsigned long> ids(nItem);
    std::iota(ids.begin(), ids.end(), 0ul);
    return std::unique_ptr<CADTElemClass>(new CADTElemClass(nDim, coordCopy, connCopy, types, markers, ids, false));
  };
  elemADT = MakeADT(elemConn, elemType, nDim + 1);
  faceADT = MakeADT(faceConn, faceType, nDim);

  /*--- Marker names without faces are not boundaries. ---*/
  for (auto it = markerIndex.begin(); it != markerIndex.end();) {
    if (markerFaceConn[it->second].empty()) {
      it = markerIndex.erase(it);
    } else {
      ++it;
    }
  }
  markerADT.resize(markerFaceConn.size());
  for (const auto& entry : markerIndex)
    markerADT[entry.second] = MakeADT(markerFaceConn[entry.second], faceType, nDim);
}

su2double CBarycentricLocator::GetDistanceLimit(su2double faceSize) const {
  return max(faceSize, max(absoluteLimit, su2double(domainFraction) * domainSize));
}

CBarycentricLocator::Stencil CBarycentricLocator::ClosestFace(const su2double* x, CADTElemClass& adt,
                                                              const std::vector<unsigned long>& conn) const {
  Stencil stencil;
  unsigned short markerID = 0;
  unsigned long id = 0;
  int rankID = 0;
  su2double dist = 0.0;
  adt.DetermineNearestElement(x, dist, markerID, id, rankID);

  stencil.inside = false;
  stencil.onFace = true;
  stencil.nPoint = nDim;
  const su2double* xFace[3] = {};
  for (unsigned short iNode = 0; iNode < nDim; ++iNode) {
    stencil.point[iNode] = conn[id * nDim + iNode];
    xFace[iNode] = &coord[stencil.point[iNode] * nDim];
  }
  if (nDim == 2) {
    ClosestPointSegment(nDim, xFace[0], xFace[1], x, stencil.weight);
  } else {
    ClosestPointTriangle(xFace[0], xFace[1], xFace[2], x, stencil.weight);
  }

  su2double closest[3] = {}, dist2 = 0.0;
  for (unsigned short iNode = 0; iNode < nDim; ++iNode)
    for (unsigned short iDim = 0; iDim < nDim; ++iDim) closest[iDim] += stencil.weight[iNode] * xFace[iNode][iDim];
  for (unsigned short iDim = 0; iDim < nDim; ++iDim) dist2 += pow(x[iDim] - closest[iDim], 2);
  stencil.distance = sqrt(dist2);

  for (unsigned short iNode = 0; iNode < nDim; ++iNode) {
    for (unsigned short jNode = iNode + 1; jNode < nDim; ++jNode) {
      su2double length2 = 0.0;
      for (unsigned short iDim = 0; iDim < nDim; ++iDim) length2 += pow(xFace[iNode][iDim] - xFace[jNode][iDim], 2);
      stencil.faceSize = max(stencil.faceSize, sqrt(length2));
    }
  }
  stencil.beyondLimit = !(stencil.distance <= GetDistanceLimit(stencil.faceSize));
  return stencil;
}

CBarycentricLocator::Stencil CBarycentricLocator::Locate(const su2double* x) {
  unsigned short markerID = 0;
  unsigned long id = 0;
  int rankID = 0;

  /*--- Element that contains the point (tolerance of the ADT, weights >= -5e-11, set to >= 0 here so that the
   *    interpolation is a convex combination). ---*/

  su2double parCoor[3] = {}, weights[8] = {};
  if (elemADT->DetermineContainingElement(x, markerID, id, rankID, parCoor, weights)) {
    Stencil stencil;
    stencil.nPoint = nDim + 1;
    su2double sum = 0.0;
    for (unsigned short iNode = 0; iNode <= nDim; ++iNode) {
      stencil.point[iNode] = elemConn[id * (nDim + 1) + iNode];
      stencil.weight[iNode] = max(su2double(0.0), weights[iNode]);
      sum += stencil.weight[iNode];
    }
    for (unsigned short iNode = 0; iNode <= nDim; ++iNode) stencil.weight[iNode] /= sum;
    return stencil;
  }

  /*--- Outside the mesh: closest point of the nearest boundary face. ---*/

  return ClosestFace(x, *faceADT, faceConn);
}

long CBarycentricLocator::ContainingElement(const su2double* x) {
  unsigned short markerID = 0;
  unsigned long id = 0;
  int rankID = 0;
  su2double parCoor[3] = {}, weights[8] = {};
  return elemADT->DetermineContainingElement(x, markerID, id, rankID, parCoor, weights) ? static_cast<long>(id) : -1;
}

CBarycentricLocator::Stencil CBarycentricLocator::LocateOnBoundary(const su2double* x,
                                                                   const std::vector<std::string>& names,
                                                                   std::string* nameFound) {
  Stencil best;
  bool found = false;
  for (const auto& name : names) {
    const auto it = markerIndex.find(name);
    if (it == markerIndex.end()) continue;
    const auto stencil = ClosestFace(x, *markerADT[it->second], markerFaceConn[it->second]);
    if (!found || stencil.distance < best.distance) {
      best = stencil;
      if (nameFound != nullptr) *nameFound = name;
    }
    found = true;
  }
  if (!found) {
    if (nameFound != nullptr) nameFound->clear();
    return Locate(x);
  }

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

void CBarycentricTransfer::Transfer(CConfig* config, const CMeshDonor& donor, CGeometry** geometry,
                                    CSolver*** solver) {
  SU2_ZONE_SCOPED

  const int rank = SU2_MPI::GetRank();
  summary = Summary();

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
  /*--- SST: CTurbSSTSolver is "Conservative" (rho k, rho omega) and its k enters the internal energy. ---*/
  const bool sst = turbSolver && TurbModelFamily(config->GetKind_Turb_Model()) == TURB_FAMILY::KW;

  /*--- Fields of each point: per time level (U^n, and U^(n-1) for 2nd order) the flow and turbulence variables. ---*/
  const unsigned short nLevel = summary.interpolateTimeN1 ? 2 : 1;
  const unsigned short nPerLevel = nVarFlow + nVarTurb;
  const unsigned short nField = nLevel * nPerLevel;
  auto levelArray = [](CVariable* nodes, unsigned short iLevel) -> su2activematrix& {
    return iLevel == 0 ? nodes->GetSolution() : nodes->GetSolution_time_n1();
  };

  /*--- Both meshes and the donor solution on the master rank, in the global numbering of the meshes (CMeshGather),
   *    where the serial interpolation below runs; the new values go back to the ranks that own the points. The
   *    marker names: the config describes the new mesh, the donor carries its own names. ---*/

  const auto gatherStart = SU2_MPI::Wtime();
  std::vector<std::string> newTags;
  for (unsigned short iMarker = 0; iMarker < newGeometry->GetnMarker(); ++iMarker)
    newTags.push_back(config->GetMarker_All_TagBound(iMarker));
  const CMeshGather donorGather(*donorGeometry), newGather(*newGeometry);
  const auto donorMesh = donorGather.GatherMesh(*config, donor.markerTags, true);
  const auto newMesh = newGather.GatherMesh(*config, newTags, true);
  std::vector<su2double> donorValues;
  {
    const auto nPointDomain = donorGeometry->GetnPointDomain();
    std::vector<su2double> local(nPointDomain * nField);
    for (unsigned short iLevel = 0; iLevel < nLevel; ++iLevel) {
      for (const auto iSol : solverIndices) {
        const auto& values = levelArray(donor.solver[MESH_0][iSol]->GetNodes(), iLevel);
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
        SU2_MPI::Error(std::to_string(nBeyond) +
                           " points of the new mesh are farther from the donor mesh than accepted (max(face size, "
                           "2 ADAP_HAUSD, 1e-3 x domain size)), the farthest is " +
                           PointText(nDim, x) + " at " + std::to_string(SU2_TYPE::GetValue(worst.distance)) +
                           " from the donor boundary (nearest face " +
                           std::to_string(SU2_TYPE::GetValue(worst.faceSize)) + " long, domain size " +
                           std::to_string(SU2_TYPE::GetValue(locator.GetDomainSize())) +
                           "). The two meshes do not describe the same domain (wrong mesh or units, or a boundary "
                           "that was moved).",
                       CURRENT_FUNCTION);
      }
    }

    /*--- Turbulence value within the bounds of the solver; counted only beyond round-off. ---*/
    auto Bounded = [&](unsigned short iVar, su2double value, bool count) {
      const su2double lower = turbSolver->GetLowerLimit(iVar), upper = turbSolver->GetUpperLimit(iVar);
      if (value < lower || value > upper) {
        const su2double limit = (value < lower) ? lower : upper;
        if (count && fabs(value - limit) > 1e-10 * fabs(limit)) summary.nTurbLimited++;
        value = limit;
      }
      return value;
    };

    /*--- Flow and turbulence of one time level, point by point: interpolated conservative flow variables; turbulence
     *    variables within the bounds of the solver (SST: rho k and rho omega interpolated, divided by the
     *    interpolated density). The complete state of the point must be admissible with the internal energy of the
     *    solver (SST subtracts k); if not, the point takes the flow and turbulence states of one donor point. ---*/

    auto InterpolateLevel = [&](unsigned short iLevel, unsigned long& nFixed) {
      const unsigned short offset = iLevel * nPerLevel;
      auto donorAt = [&](unsigned long jPoint, unsigned short iVar) {
        return donorValues[jPoint * nField + offset + iVar];
      };
      su2double state[kMaxVar] = {}, turb[kMaxVar] = {};
      for (auto iPoint = 0ul; iPoint < nPoint; ++iPoint) {
        const auto& stencil = stencils[iPoint];
        for (unsigned short iVar = 0; iVar < nVarFlow; ++iVar) {
          state[iVar] = 0.0;
          for (unsigned short k = 0; k < stencil.nPoint; ++k)
            state[iVar] += stencil.weight[k] * donorAt(stencil.point[k], iVar);
        }
        for (unsigned short iVar = 0; iVar < nVarTurb; ++iVar) {
          su2double value = 0.0;
          for (unsigned short k = 0; k < stencil.nPoint; ++k) {
            const auto jPoint = stencil.point[k];
            value += stencil.weight[k] * (sst ? donorAt(jPoint, 0) : su2double(1.0)) * donorAt(jPoint, nVarFlow + iVar);
          }
          if (sst) value = (state[0] > 0.0) ? value / state[0] : su2double(0.0);
          turb[iVar] = Bounded(iVar, value, true);
        }

        if (!AdmissibleState(*fluidModel, nDim, state, sst ? turb[0] : su2double(0.0))) {
          /*--- Paired fallback: flow and turbulence states of the same donor point. ---*/
          nFixed++;
          int best = -1;
          for (unsigned short k = 0; k < stencil.nPoint; ++k) {
            if (best >= 0 && stencil.weight[k] <= stencil.weight[best]) continue;
            const auto jPoint = stencil.point[k];
            const su2double k0 = sst ? Bounded(0, donorAt(jPoint, nVarFlow), false) : su2double(0.0);
            if (AdmissibleState(*fluidModel, nDim, &donorValues[jPoint * nField + offset], k0)) best = k;
          }
          if (best < 0) {
            su2double x[3] = {0.0};
            for (unsigned short iDim = 0; iDim < nDim; ++iDim) x[iDim] = newMesh.coord[iPoint * nDim + iDim];
            SU2_MPI::Error("No admissible donor state (flow and turbulence) near the point " + PointText(nDim, x) + ".",
                           CURRENT_FUNCTION);
          }
          const auto jPoint = stencil.point[best];
          for (unsigned short iVar = 0; iVar < nVarFlow; ++iVar) state[iVar] = donorAt(jPoint, iVar);
          for (unsigned short iVar = 0; iVar < nVarTurb; ++iVar)
            turb[iVar] = Bounded(iVar, donorAt(jPoint, nVarFlow + iVar), false);
        }
        for (unsigned short iVar = 0; iVar < nVarFlow; ++iVar) newValues[iPoint * nField + offset + iVar] = state[iVar];
        for (unsigned short iVar = 0; iVar < nVarTurb; ++iVar)
          newValues[iPoint * nField + offset + nVarFlow + iVar] = turb[iVar];
      }
    };

    /*--- U^n, once; U^(n-1): 2nd order only (else U^n, below). ---*/
    InterpolateLevel(0, summary.nFlowFixed);
    if (summary.interpolateTimeN1) InterpolateLevel(1, summary.nHistoryFixed);

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
        auto& values = levelArray(solver[MESH_0][iSol]->GetNodes(), iLevel);
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

  /*--- Summary. ---*/

  if (rank != MASTER_NODE) return;

  const auto& s = summary;
  std::vector<string> names = {"Density"};
  for (unsigned short iDim = 0; iDim < nDim; ++iDim) names.push_back(string("Momentum-") + "xyz"[iDim]);
  names.push_back("Energy");

  su2double momentumNorm = 0.0;
  for (unsigned short iDim = 0; iDim < nDim; ++iDim) momentumNorm += pow(s.donorIntegral[iDim + 1], 2);
  momentumNorm = sqrt(momentumNorm);

  cout << endl << "------------------------ Solution Transfer (P1) -------------------------" << endl;
  if (SU2_MPI::GetSize() > 1) {
    cout << "Donor and new mesh with the donor solution gathered on rank " << MASTER_NODE << " (" << gatherTime
         << " s), new values sent to the ranks of their points (" << scatterTime << " s)." << endl;
  }
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
  if (solver[MESH_0][TURB_SOL] != nullptr)
    cout << "Turbulence values limited to the bounds of the solver: " << s.nTurbLimited << "." << endl;
  cout << "Domain volume: donor " << s.donorVolume << ", new " << s.newVolume
       << ", relative change " << (s.newVolume - s.donorVolume) / s.donorVolume << "." << endl;
  cout << "Change of the integrals (not conserved by the interpolation), relative to the donor integral"
       << " (momentum: to the norm of the donor momentum integral):" << endl;
  for (unsigned short iVar = 0; iVar < nVarFlow; ++iVar) {
    const bool momentum = (iVar > 0 && iVar <= nDim);
    const su2double scale = momentum ? momentumNorm : fabs(s.donorIntegral[iVar]);
    cout << "  " << std::setw(10) << names[iVar] << ": " << (s.newIntegral[iVar] - s.donorIntegral[iVar]) / scale
         << endl;
  }
  if (roundTripCheck) {
    cout << "Round trip donor -> new -> donor, difference relative to the donor range (RMS, max):" << endl;
    for (unsigned short iVar = 0; iVar < nVarFlow; ++iVar) {
      cout << "  " << std::setw(10) << names[iVar] << ": " << s.roundTripL2[iVar] << ", " << s.roundTripLinf[iVar]
           << endl;
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
