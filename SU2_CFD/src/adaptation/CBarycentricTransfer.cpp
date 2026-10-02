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

/*--- Volume of the domain (control volumes of the domain points) and integral of each solution variable. ---*/
void Integrals(CGeometry& geometry, CSolver& solver, su2double& volume, std::vector<su2double>& integral) {
  const auto nVar = solver.GetnVar();
  const auto& solution = solver.GetNodes()->GetSolution();
  volume = 0.0;
  integral.assign(nVar, 0.0);
  for (auto iPoint = 0ul; iPoint < geometry.GetnPointDomain(); ++iPoint) {
    const su2double vol = geometry.nodes->GetVolume(iPoint);
    volume += vol;
    for (unsigned short iVar = 0; iVar < nVar; ++iVar) integral[iVar] += vol * solution(iPoint, iVar);
  }
}

/*--- Markers of each point of a geometry (indices into the marker names). ---*/
std::vector<std::vector<unsigned short>> PointMarkers(const CGeometry& geometry, unsigned long nNames) {
  std::vector<std::vector<unsigned short>> markers(geometry.GetnPoint());
  for (unsigned short iMarker = 0; iMarker < geometry.GetnMarker() && iMarker < nNames; ++iMarker)
    for (auto iVertex = 0ul; iVertex < geometry.GetnVertex(iMarker); ++iVertex)
      markers[geometry.vertex[iMarker][iVertex]->GetNode()].push_back(iMarker);
  return markers;
}

/*--- Stencil of a point: the closest point of the donor boundary of its marker(s) if it is on a marker that the donor
 *    has, else the containing element or the closest point of the nearest donor boundary face. ---*/
CBarycentricLocator::Stencil LocatePoint(CBarycentricLocator& locator, const su2double* x,
                                         const std::vector<unsigned short>& markers,
                                         const std::vector<std::string>& names) {
  std::vector<std::string> known;
  for (const auto iMarker : markers)
    if (locator.HasMarker(names[iMarker])) known.push_back(names[iMarker]);
  return known.empty() ? locator.Locate(x) : locator.LocateOnBoundary(x, known);
}

}  // namespace

CBarycentricLocator::CBarycentricLocator(const CGeometry& geometry, const std::vector<std::string>& markerTags,
                                         su2double absoluteLimit)
    : nDim(geometry.GetnDim()), absoluteLimit(absoluteLimit) {
  const unsigned short elemType = (nDim == 2) ? TRIANGLE : TETRAHEDRON;
  const unsigned short faceType = (nDim == 2) ? LINE : TRIANGLE;

  coord.resize(geometry.GetnPoint() * nDim);
  su2double xMin[3] = {}, xMax[3] = {};
  for (unsigned short iDim = 0; iDim < nDim; ++iDim) {
    xMin[iDim] = std::numeric_limits<passivedouble>::max();
    xMax[iDim] = std::numeric_limits<passivedouble>::lowest();
  }
  for (auto iPoint = 0ul; iPoint < geometry.GetnPoint(); ++iPoint) {
    for (unsigned short iDim = 0; iDim < nDim; ++iDim) {
      const su2double x = geometry.nodes->GetCoord(iPoint, iDim);
      coord[iPoint * nDim + iDim] = x;
      xMin[iDim] = min(xMin[iDim], x);
      xMax[iDim] = max(xMax[iDim], x);
    }
  }
  for (unsigned short iDim = 0; iDim < nDim; ++iDim) domainSize += pow(xMax[iDim] - xMin[iDim], 2);
  domainSize = sqrt(domainSize);

  /*--- Elements. The weights of the ADT are in the order of these nodes. ---*/

  for (auto iElem = 0ul; iElem < geometry.GetnElem(); ++iElem) {
    const auto* elem = geometry.elem[iElem];
    if (elem->GetVTK_Type() != elemType) {
      SU2_MPI::Error("The solution transfer needs a mesh of triangles (2D) or tetrahedra (3D).", CURRENT_FUNCTION);
    }
    for (unsigned short iNode = 0; iNode <= nDim; ++iNode) elemConn.push_back(elem->GetNode(iNode));
  }
  if (elemConn.empty()) SU2_MPI::Error("The mesh has no elements.", CURRENT_FUNCTION);

  /*--- Faces of all markers, and of each marker name (send/receive markers have vertex elements, they are not
   *    boundaries). ---*/

  for (unsigned short iMarker = 0; iMarker < geometry.GetnMarker(); ++iMarker) {
    std::vector<unsigned long>* named = nullptr;
    if (iMarker < markerTags.size()) {
      const auto it = markerIndex.emplace(markerTags[iMarker], markerFaceConn.size()).first;
      if (it->second == markerFaceConn.size()) markerFaceConn.emplace_back();
      named = &markerFaceConn[it->second];
    }
    for (auto iElem = 0ul; iElem < geometry.GetnElem_Bound(iMarker); ++iElem) {
      const auto* face = geometry.bound[iMarker][iElem];
      if (face->GetVTK_Type() == VERTEX) continue;
      if (face->GetVTK_Type() != faceType) {
        SU2_MPI::Error("The solution transfer needs boundary lines (2D) or triangles (3D).", CURRENT_FUNCTION);
      }
      for (unsigned short iNode = 0; iNode < nDim; ++iNode) {
        faceConn.push_back(face->GetNode(iNode));
        if (named) named->push_back(face->GetNode(iNode));
      }
    }
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

  /*--- Marker names without faces (e.g. send/receive) are not boundaries. ---*/
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

bool CBarycentricTransfer::AdmissibleState(CFluidModel& fluidModel, unsigned short nDim, const su2double* solution) {
  for (unsigned short iVar = 0; iVar < nDim + 2; ++iVar) {
    if (!IsFinite(solution[iVar])) return false;
  }
  const su2double density = solution[0];
  if (!(density > 0.0)) return false;

  su2double velocity2 = 0.0;
  for (unsigned short iDim = 0; iDim < nDim; ++iDim) velocity2 += pow(solution[iDim + 1] / density, 2);
  const su2double staticEnergy = solution[nDim + 1] / density - 0.5 * velocity2;

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
  const auto nPoint = newGeometry->GetnPoint();

  /*--- Donor points and weights of each new point. Marker names: the config describes the new mesh, the donor carries
   *    its own names. Distances are accepted up to the limit of the locator, with 2 ADAP_HAUSD as the tolerance of
   *    the remesher's boundary approximation. ---*/

  std::vector<std::string> newTags;
  for (unsigned short iMarker = 0; iMarker < newGeometry->GetnMarker(); ++iMarker)
    newTags.push_back(config->GetMarker_All_TagBound(iMarker));
  const auto newPointMarkers = PointMarkers(*newGeometry, newTags.size());

  std::vector<CBarycentricLocator::Stencil> stencils(nPoint);
  {
    CBarycentricLocator locator(*donorGeometry, donor.markerTags, 2.0 * config->GetAdap_Hausd());
    summary.distanceLimit = locator.GetDistanceLimit(0.0);

    /*--- Statistics per marker of the new mesh (send/receive markers excluded), then the points on no marker. ---*/
    std::vector<int> entry(newTags.size(), -1);
    for (unsigned short iMarker = 0; iMarker < newTags.size(); ++iMarker) {
      if (config->GetMarker_All_KindBC(iMarker) == SEND_RECEIVE) continue;
      entry[iMarker] = summary.markers.size();
      summary.markers.emplace_back();
      summary.markers.back().name = newTags[iMarker];
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
      stencils[iPoint] = LocatePoint(locator, newGeometry->nodes->GetCoord(iPoint), newPointMarkers[iPoint], newTags);
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
      for (const auto iMarker : newPointMarkers[iPoint])
        if (entry[iMarker] >= 0) entries.push_back(entry[iMarker]);
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
      SU2_MPI::Error(std::to_string(nBeyond) + " points of the new mesh are farther from the donor mesh than accepted " +
                         "(max(face size, 2 ADAP_HAUSD, 1e-3 x domain size)), the farthest is " +
                         PointText(nDim, newGeometry->nodes->GetCoord(worstPoint)) + " at " +
                         std::to_string(SU2_TYPE::GetValue(worst.distance)) + " from the donor boundary (nearest face " +
                         std::to_string(SU2_TYPE::GetValue(worst.faceSize)) + " long, domain size " +
                         std::to_string(SU2_TYPE::GetValue(locator.GetDomainSize())) +
                         "). The two meshes do not describe the same domain (wrong mesh or units, or a boundary that "
                         "was moved).",
                     CURRENT_FUNCTION);
    }
  }
  summary.nPoint = nPoint;

  auto* flowSolver = solver[MESH_0][FLOW_SOL];
  auto* donorFlowSolver = donor.solver[MESH_0][FLOW_SOL];
  const bool hasTimeN = arrays.hasTimeN, hasTimeN1 = arrays.hasTimeN1;
  summary.interpolateTimeN1 = arrays.interpolateTimeN1;
  summary.nTimeLevels = hasTimeN + hasTimeN1;

  /*--- Flow: interpolated conservative variables, a donor state where they are not admissible. ---*/

  const auto nVarFlow = flowSolver->GetnVar();
  auto* fluidModel = flowSolver->GetFluidModel();
  if (fluidModel == nullptr || nVarFlow != nDim + 2) {
    SU2_MPI::Error("The flow solver is not a compressible flow solver.", CURRENT_FUNCTION);
  }
  auto InterpolateFlow = [&](const su2activematrix& donorSolution, su2activematrix& newSolution,
                             unsigned long& nFixed) {
    std::vector<su2double> state(nVarFlow);
    for (auto iPoint = 0ul; iPoint < nPoint; ++iPoint) {
      const auto& stencil = stencils[iPoint];
      std::fill(state.begin(), state.end(), 0.0);
      for (unsigned short k = 0; k < stencil.nPoint; ++k)
        for (unsigned short iVar = 0; iVar < nVarFlow; ++iVar)
          state[iVar] += stencil.weight[k] * donorSolution(stencil.point[k], iVar);

      if (!AdmissibleState(*fluidModel, nDim, state.data())) {
        nFixed++;
        int best = -1;
        for (unsigned short k = 0; k < stencil.nPoint; ++k) {
          if ((best < 0 || stencil.weight[k] > stencil.weight[best]) &&
              AdmissibleState(*fluidModel, nDim, donorSolution[stencil.point[k]])) {
            best = k;
          }
        }
        if (best < 0) {
          SU2_MPI::Error("No admissible donor flow state near the point " +
                             PointText(nDim, newGeometry->nodes->GetCoord(iPoint)) + ".",
                         CURRENT_FUNCTION);
        }
        for (unsigned short iVar = 0; iVar < nVarFlow; ++iVar) state[iVar] = donorSolution(stencil.point[best], iVar);
      }
      for (unsigned short iVar = 0; iVar < nVarFlow; ++iVar) newSolution(iPoint, iVar) = state[iVar];
    }
  };

  /*--- Turbulence: interpolated solution variables within the bounds of the solver. ---*/

  const auto* turbSolver = dynamic_cast<const CTurbSolver*>(solver[MESH_0][TURB_SOL]);
  if (solver[MESH_0][TURB_SOL] != nullptr && turbSolver == nullptr) {
    SU2_MPI::Error("Unexpected turbulence solver.", CURRENT_FUNCTION);
  }
  auto InterpolateTurb = [&](const su2activematrix& donorSolution, su2activematrix& newSolution) {
    const auto nVarTurb = turbSolver->GetnVar();
    for (auto iPoint = 0ul; iPoint < nPoint; ++iPoint) {
      const auto& stencil = stencils[iPoint];
      for (unsigned short iVar = 0; iVar < nVarTurb; ++iVar) {
        su2double value = 0.0;
        for (unsigned short k = 0; k < stencil.nPoint; ++k)
          value += stencil.weight[k] * donorSolution(stencil.point[k], iVar);

        /*--- Count only the values that are out of bounds by more than round-off. ---*/
        const su2double lower = turbSolver->GetLowerLimit(iVar), upper = turbSolver->GetUpperLimit(iVar);
        if (value < lower || value > upper) {
          const su2double limit = (value < lower) ? lower : upper;
          if (fabs(value - limit) > 1e-10 * fabs(limit)) summary.nTurbLimited++;
          value = limit;
        }
        newSolution(iPoint, iVar) = value;
      }
    }
  };

  for (const auto iSol : solverIndices) {
    auto* donorNodes = donor.solver[MESH_0][iSol]->GetNodes();
    auto* nodes = solver[MESH_0][iSol]->GetNodes();
    const bool flow = (iSol == FLOW_SOL);

    /*--- U^n, once. ---*/
    if (flow) {
      InterpolateFlow(donorNodes->GetSolution(), nodes->GetSolution(), summary.nFlowFixed);
    } else {
      InterpolateTurb(donorNodes->GetSolution(), nodes->GetSolution());
    }
    if (hasTimeN) nodes->GetSolution_time_n() = nodes->GetSolution();

    /*--- U^(n-1): 2nd order only, else U^n. ---*/
    if (summary.interpolateTimeN1) {
      if (flow) {
        InterpolateFlow(donorNodes->GetSolution_time_n1(), nodes->GetSolution_time_n1(), summary.nHistoryFixed);
      } else {
        InterpolateTurb(donorNodes->GetSolution_time_n1(), nodes->GetSolution_time_n1());
      }
    } else if (hasTimeN1) {
      nodes->GetSolution_time_n1() = nodes->GetSolution_time_n();
    }
  }

  /*--- Conservation defects and round-trip difference (donor -> new -> donor) of the flow variables. ---*/

  Integrals(*donorGeometry, *donorFlowSolver, summary.donorVolume, summary.donorIntegral);
  Integrals(*newGeometry, *flowSolver, summary.newVolume, summary.newIntegral);

  if (roundTripCheck) {
    const auto& donorSolution = donorFlowSolver->GetNodes()->GetSolution();
    const auto& newSolution = flowSolver->GetNodes()->GetSolution();
    const auto nPointDonor = donorGeometry->GetnPointDomain();

    std::vector<su2double> minValue(nVarFlow, std::numeric_limits<passivedouble>::max());
    std::vector<su2double> maxValue(nVarFlow, std::numeric_limits<passivedouble>::lowest());
    for (auto iPoint = 0ul; iPoint < nPointDonor; ++iPoint) {
      for (unsigned short iVar = 0; iVar < nVarFlow; ++iVar) {
        minValue[iVar] = min(minValue[iVar], donorSolution(iPoint, iVar));
        maxValue[iVar] = max(maxValue[iVar], donorSolution(iPoint, iVar));
      }
    }

    summary.roundTripL2.assign(nVarFlow, 0.0);
    summary.roundTripLinf.assign(nVarFlow, 0.0);
    CBarycentricLocator locator(*newGeometry, newTags);
    const auto donorPointMarkers = PointMarkers(*donorGeometry, donor.markerTags.size());
    for (auto iPoint = 0ul; iPoint < nPointDonor; ++iPoint) {
      const auto stencil =
          LocatePoint(locator, donorGeometry->nodes->GetCoord(iPoint), donorPointMarkers[iPoint], donor.markerTags);
      for (unsigned short iVar = 0; iVar < nVarFlow; ++iVar) {
        su2double value = 0.0;
        for (unsigned short k = 0; k < stencil.nPoint; ++k) value += stencil.weight[k] * newSolution(stencil.point[k], iVar);
        const su2double diff = fabs(value - donorSolution(iPoint, iVar));
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
  cout << "Flow states not admissible after the interpolation (donor state used): " << s.nFlowFixed << "." << endl;
  if (s.nTimeLevels > 0) {
    cout << "Time history: Solution_time_n = the solution (U^n, interpolated once)";
    if (s.interpolateTimeN1) {
      cout << ", Solution_time_n1 = U^(n-1) interpolated with the same stencils (flow states not admissible: "
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
