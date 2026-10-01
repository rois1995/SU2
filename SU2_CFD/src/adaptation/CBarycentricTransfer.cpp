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

  const su2double denom = 1.0 / (va + vb + vc);
  const su2double v = vb * denom, w = vc * denom;
  set(1.0 - v - w, v, w);
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

}  // namespace

CBarycentricLocator::CBarycentricLocator(const CGeometry& geometry) : nDim(geometry.GetnDim()) {
  const unsigned short elemType = (nDim == 2) ? TRIANGLE : TETRAHEDRON;
  const unsigned short faceType = (nDim == 2) ? LINE : TRIANGLE;

  coord.resize(geometry.GetnPoint() * nDim);
  for (auto iPoint = 0ul; iPoint < geometry.GetnPoint(); ++iPoint)
    for (unsigned short iDim = 0; iDim < nDim; ++iDim) coord[iPoint * nDim + iDim] = geometry.nodes->GetCoord(iPoint, iDim);

  /*--- Elements. The weights of the ADT are in the order of these nodes. ---*/

  for (auto iElem = 0ul; iElem < geometry.GetnElem(); ++iElem) {
    const auto* elem = geometry.elem[iElem];
    if (elem->GetVTK_Type() != elemType) {
      SU2_MPI::Error("The solution transfer needs a mesh of triangles (2D) or tetrahedra (3D).", CURRENT_FUNCTION);
    }
    for (unsigned short iNode = 0; iNode <= nDim; ++iNode) elemConn.push_back(elem->GetNode(iNode));
  }
  if (elemConn.empty()) SU2_MPI::Error("The mesh has no elements.", CURRENT_FUNCTION);

  /*--- Faces of all markers (send/receive markers have vertex elements, they are not boundaries). ---*/

  for (unsigned short iMarker = 0; iMarker < geometry.GetnMarker(); ++iMarker) {
    for (auto iElem = 0ul; iElem < geometry.GetnElem_Bound(iMarker); ++iElem) {
      const auto* face = geometry.bound[iMarker][iElem];
      if (face->GetVTK_Type() == VERTEX) continue;
      if (face->GetVTK_Type() != faceType) {
        SU2_MPI::Error("The solution transfer needs boundary lines (2D) or triangles (3D).", CURRENT_FUNCTION);
      }
      for (unsigned short iNode = 0; iNode < nDim; ++iNode) faceConn.push_back(face->GetNode(iNode));
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
}

CBarycentricLocator::Stencil CBarycentricLocator::Locate(const su2double* x) {
  Stencil stencil;
  unsigned short markerID = 0;
  unsigned long id = 0;
  int rankID = 0;

  /*--- Element that contains the point (tolerance of the ADT, weights >= -5e-11). ---*/

  su2double parCoor[3] = {}, weights[8] = {};
  if (elemADT->DetermineContainingElement(x, markerID, id, rankID, parCoor, weights)) {
    stencil.nPoint = nDim + 1;
    for (unsigned short iNode = 0; iNode <= nDim; ++iNode) {
      stencil.point[iNode] = elemConn[id * (nDim + 1) + iNode];
      stencil.weight[iNode] = weights[iNode];
    }
    return stencil;
  }

  /*--- Outside the mesh: closest point of the nearest boundary face. ---*/

  su2double dist = 0.0;
  faceADT->DetermineNearestElement(x, dist, markerID, id, rankID);

  stencil.inside = false;
  stencil.nPoint = nDim;
  const su2double* xFace[3] = {};
  for (unsigned short iNode = 0; iNode < nDim; ++iNode) {
    stencil.point[iNode] = faceConn[id * nDim + iNode];
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

  if (!(stencil.distance <= stencil.faceSize)) {
    SU2_MPI::Error("The point " + PointText(nDim, x) + " is not near the donor mesh: distance " +
                       std::to_string(SU2_TYPE::GetValue(stencil.distance)) + " to its boundary, nearest face size " +
                       std::to_string(SU2_TYPE::GetValue(stencil.faceSize)) + ".",
                   CURRENT_FUNCTION);
  }
  return stencil;
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

  /*--- Supported problems: compressible flow, SA or SST, one rank. ---*/

  if (SU2_MPI::GetSize() > 1) {
    SU2_MPI::Error("The barycentric solution transfer is only available on one rank for now.", CURRENT_FUNCTION);
  }
  const auto kindSolver = config->GetKind_Solver();
  if (kindSolver != MAIN_SOLVER::EULER && kindSolver != MAIN_SOLVER::NAVIER_STOKES &&
      kindSolver != MAIN_SOLVER::RANS) {
    SU2_MPI::Error("The barycentric solution transfer is only available for compressible EULER, NAVIER_STOKES or RANS.",
                   CURRENT_FUNCTION);
  }
  if (config->GetTime_Domain()) {
    SU2_MPI::Error("The barycentric solution transfer is only available for steady problems for now.",
                   CURRENT_FUNCTION);
  }
  const auto turbFamily = TurbModelFamily(config->GetKind_Turb_Model());
  if (config->GetKind_Turb_Model() != TURB_MODEL::NONE && turbFamily != TURB_FAMILY::SA &&
      turbFamily != TURB_FAMILY::KW) {
    SU2_MPI::Error("The barycentric solution transfer is only available for the SA and SST turbulence models.",
                   CURRENT_FUNCTION);
  }
  if (config->GetKind_Trans_Model() != TURB_TRANS_MODEL::NONE ||
      config->GetKind_Species_Model() != SPECIES_MODEL::NONE) {
    SU2_MPI::Error("The barycentric solution transfer is not available with transition or species models.",
                   CURRENT_FUNCTION);
  }

  std::vector<unsigned short> solverIndices;
  for (unsigned short iSol = 0; iSol < MAX_SOLS; ++iSol) {
    const auto* newSolver = solver[MESH_0][iSol];
    const auto* donorSolver = donor.solver[MESH_0][iSol];
    if (newSolver == nullptr && donorSolver == nullptr) continue;
    if (iSol != FLOW_SOL && iSol != TURB_SOL) {
      SU2_MPI::Error("The barycentric solution transfer is not available for solver " + std::to_string(iSol) + ".",
                     CURRENT_FUNCTION);
    }
    if (newSolver == nullptr || donorSolver == nullptr || newSolver->GetnVar() != donorSolver->GetnVar()) {
      SU2_MPI::Error("The solvers of the donor and of the new mesh differ.", CURRENT_FUNCTION);
    }
    solverIndices.push_back(iSol);
  }
  if (solver[MESH_0][FLOW_SOL] == nullptr) SU2_MPI::Error("No flow solver.", CURRENT_FUNCTION);

  auto* donorGeometry = donor.geometry[MESH_0];
  auto* newGeometry = geometry[MESH_0];
  const auto nDim = newGeometry->GetnDim();
  if (donorGeometry->GetnDim() != nDim) {
    SU2_MPI::Error("The donor and the new mesh have different dimensions.", CURRENT_FUNCTION);
  }
  const auto nPoint = newGeometry->GetnPoint();

  /*--- Donor points and weights of each new point. ---*/

  std::vector<CBarycentricLocator::Stencil> stencils(nPoint);
  {
    CBarycentricLocator locator(*donorGeometry);
    for (auto iPoint = 0ul; iPoint < nPoint; ++iPoint) {
      stencils[iPoint] = locator.Locate(newGeometry->nodes->GetCoord(iPoint));
      const auto& stencil = stencils[iPoint];
      if (!stencil.inside) {
        summary.nOutside++;
        summary.maxDistance = max(summary.maxDistance, stencil.distance);
        summary.maxRelDistance = max(summary.maxRelDistance, stencil.distance / stencil.faceSize);
      }
    }
  }
  summary.nPoint = nPoint;

  /*--- Flow: interpolated conservative variables, a donor state where they are not admissible. ---*/

  auto* flowSolver = solver[MESH_0][FLOW_SOL];
  auto* donorFlowSolver = donor.solver[MESH_0][FLOW_SOL];
  const auto nVarFlow = flowSolver->GetnVar();
  auto* fluidModel = flowSolver->GetFluidModel();
  if (fluidModel == nullptr || nVarFlow != nDim + 2) {
    SU2_MPI::Error("The flow solver is not a compressible flow solver.", CURRENT_FUNCTION);
  }
  {
    const auto& donorSolution = donorFlowSolver->GetNodes()->GetSolution();
    auto* nodes = flowSolver->GetNodes();
    std::vector<su2double> state(nVarFlow);

    for (auto iPoint = 0ul; iPoint < nPoint; ++iPoint) {
      const auto& stencil = stencils[iPoint];
      std::fill(state.begin(), state.end(), 0.0);
      for (unsigned short k = 0; k < stencil.nPoint; ++k)
        for (unsigned short iVar = 0; iVar < nVarFlow; ++iVar)
          state[iVar] += stencil.weight[k] * donorSolution(stencil.point[k], iVar);

      if (!AdmissibleState(*fluidModel, nDim, state.data())) {
        summary.nFlowFixed++;
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
      for (unsigned short iVar = 0; iVar < nVarFlow; ++iVar) nodes->SetSolution(iPoint, iVar, state[iVar]);
    }
  }

  /*--- Turbulence: interpolated solution variables within the bounds of the solver. ---*/

  if (solver[MESH_0][TURB_SOL] != nullptr) {
    const auto* turbSolver = dynamic_cast<const CTurbSolver*>(solver[MESH_0][TURB_SOL]);
    if (turbSolver == nullptr) SU2_MPI::Error("Unexpected turbulence solver.", CURRENT_FUNCTION);

    const auto nVarTurb = turbSolver->GetnVar();
    const auto& donorSolution = donor.solver[MESH_0][TURB_SOL]->GetNodes()->GetSolution();
    auto* nodes = solver[MESH_0][TURB_SOL]->GetNodes();

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
        nodes->SetSolution(iPoint, iVar, value);
      }
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
    CBarycentricLocator locator(*newGeometry);
    for (auto iPoint = 0ul; iPoint < nPointDonor; ++iPoint) {
      const auto stencil = locator.Locate(donorGeometry->nodes->GetCoord(iPoint));
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

  /*--- As after loading a restart file: communication, primitive variables, eddy viscosity, coarse levels (in the
   *    order of the restart, the turbulence solver updates the flow primitives). The old solution is the new one, the
   *    flow preprocessing would otherwise reset non-physical points to the free-stream state of the constructor. ---*/

  for (const auto iSol : solverIndices) solver[MESH_0][iSol]->Set_OldSolution();

  for (const auto iSol : solverIndices) {
    auto* sol = solver[MESH_0][iSol];
    SU2_OMP_PARALLEL_(if (sol->GetHasHybridParallel()))
    sol->UpdateLoadedSolution(geometry, solver, config);
    END_SU2_OMP_PARALLEL
  }

  for (unsigned short iMesh = 1; iMesh <= config->GetnMGLevels(); ++iMesh)
    for (const auto iSol : solverIndices) solver[iMesh][iSol]->Set_OldSolution();

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
  cout << "Points outside the donor mesh (closest donor boundary point used): " << s.nOutside;
  if (s.nOutside) cout << ", max distance " << s.maxDistance << " (" << s.maxRelDistance << " of the face size)";
  cout << "." << endl;
  cout << "Flow states not admissible after the interpolation (donor state used): " << s.nFlowFixed << "." << endl;
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
