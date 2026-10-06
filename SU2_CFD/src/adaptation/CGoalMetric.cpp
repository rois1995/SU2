/*!
 * \file CGoalMetric.cpp
 * \brief Point functions of the goal-oriented estimator (ADAP_SENSOR= GOAL, stage G).
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

#include "../../include/adaptation/CGoalMetric.hpp"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <limits>

#include "../../../Common/include/CConfig.hpp"
#include "../../../Common/include/geometry/CGeometry.hpp"
#include "../../../Common/include/linear_algebra/blas_structure.hpp"
#include "../../../Common/include/parallelization/CPassiveComm.hpp"
#include "../../include/solvers/CSolver.hpp"
#include "../../include/gradients/computeGradientsGreenGauss.hpp"
#include "../../include/gradients/computeGradientsLeastSquares.hpp"

namespace GoalMetric {

void FluxFields(unsigned short nDim, su2double gamma, const su2double* U, su2double* F) {
  const auto nVar = NumVar(nDim);
  const su2double rho = U[0], rhoE = U[nDim + 1];
  su2double u[MAXDIM] = {0.0}, m2 = 0.0;
  for (auto i = 0u; i < nDim; ++i) {
    u[i] = U[1 + i] / rho;
    m2 += U[1 + i] * U[1 + i];
  }
  const su2double p = (gamma - 1.0) * (rhoE - 0.5 * m2 / rho);
  for (auto d = 0u; d < nDim; ++d) {
    su2double* Fd = F + d * nVar;
    Fd[0] = U[1 + d];
    for (auto i = 0u; i < nDim; ++i) Fd[1 + i] = U[1 + d] * u[i] + (i == d ? p : 0.0);
    Fd[nDim + 1] = (rhoE + p) * u[d];
  }
}

void FluxJacobians(unsigned short nDim, su2double gamma, const su2double* U, su2double (&A)[MAXDIM][MAXVAR][MAXVAR]) {
  const auto nVar = NumVar(nDim);
  const su2double rho = U[0], rhoE = U[nDim + 1], gm1 = gamma - 1.0;
  su2double u[MAXDIM] = {0.0}, q2 = 0.0;
  for (auto i = 0u; i < nDim; ++i) {
    u[i] = U[1 + i] / rho;
    q2 += u[i] * u[i];
  }
  const su2double p = gm1 * (rhoE - 0.5 * rho * q2);
  const su2double H = (rhoE + p) / rho;
  const su2double dpdrho = 0.5 * gm1 * q2;

  for (auto d = 0u; d < nDim; ++d) {
    auto& Ad = A[d];
    for (auto i = 0u; i < nVar; ++i)
      for (auto j = 0u; j < nVar; ++j) Ad[i][j] = 0.0;

    /*--- Mass flux rho u_d. ---*/
    Ad[0][1 + d] = 1.0;

    /*--- Momentum fluxes rho u_d u_i + p delta_di. ---*/
    for (auto i = 0u; i < nDim; ++i) {
      Ad[1 + i][0] = -u[d] * u[i] + (i == d ? dpdrho : 0.0);
      for (auto k = 0u; k < nDim; ++k) {
        Ad[1 + i][1 + k] = (d == k ? u[i] : 0.0) + (i == k ? u[d] : 0.0) - (i == d ? su2double(gm1 * u[k]) : su2double(0.0));
      }
      Ad[1 + i][nDim + 1] = (i == d) ? gm1 : 0.0;
    }

    /*--- Energy flux (rho E + p) u_d. ---*/
    Ad[nDim + 1][0] = u[d] * (dpdrho - H);
    for (auto k = 0u; k < nDim; ++k) Ad[nDim + 1][1 + k] = (d == k ? H : 0.0) - gm1 * u[k] * u[d];
    Ad[nDim + 1][nDim + 1] = gamma * u[d];
  }
}

void OddPart(unsigned short nDim, const su2double* n, const su2double* lambda, const su2double* phi,
             su2double* lambdaOdd, su2double* phiOdd) {
  const auto nVar = NumVar(nDim);

  if (lambda != nullptr) {
    su2double proj = 0.0;
    for (auto i = 0u; i < nDim; ++i) proj += n[i] * lambda[1 + i];
    for (auto j = 0u; j < nVar; ++j) lambdaOdd[j] = 0.0;
    for (auto i = 0u; i < nDim; ++i) lambdaOdd[1 + i] = proj * n[i];
  }

  /*--- Phi as a matrix (row j = variable, column d = direction), phi[d * nVar + j]. A = Tn Phi (the momentum rows
   *    projected on the normal), B = Phi - A = Te Phi; odd = A Pt + B Pn = A - (A n) n^T + (B n) n^T. ---*/
  su2double Amat[MAXVAR][MAXDIM] = {{0.0}}, Bmat[MAXVAR][MAXDIM] = {{0.0}};
  for (auto d = 0u; d < nDim; ++d) {
    su2double proj = 0.0;
    for (auto i = 0u; i < nDim; ++i) proj += n[i] * phi[d * nVar + 1 + i];
    for (auto j = 0u; j < nVar; ++j) Amat[j][d] = 0.0;
    for (auto i = 0u; i < nDim; ++i) Amat[1 + i][d] = n[i] * proj;
    for (auto j = 0u; j < nVar; ++j) Bmat[j][d] = phi[d * nVar + j] - Amat[j][d];
  }
  for (auto j = 0u; j < nVar; ++j) {
    su2double An = 0.0, Bn = 0.0;
    for (auto d = 0u; d < nDim; ++d) {
      An += Amat[j][d] * n[d];
      Bn += Bmat[j][d] * n[d];
    }
    for (auto d = 0u; d < nDim; ++d) phiOdd[d * nVar + j] = Amat[j][d] + (Bn - An) * n[d];
  }
}

void MirrorValues(unsigned short nDim, const su2double* n, su2double* values) {
  const auto nVar = NumVar(nDim), nFlux = NumFlux(nDim);
  su2double lambdaOdd[MAXVAR], phiOdd[MAXFLUX];
  OddPart(nDim, n, values, values + nVar, lambdaOdd, phiOdd);
  for (auto j = 0u; j < nVar; ++j) values[j] -= lambdaOdd[j];
  for (auto f = 0u; f < nFlux; ++f) values[nVar + f] -= phiOdd[f];
}

void MirrorGradients(unsigned short nDim, const su2double* n, su2double* grad) {
  const auto nVar = NumVar(nDim), nField = FieldCount(nDim);
  su2double odd[MAXFIELD][MAXDIM] = {{0.0}}, even[MAXFIELD][MAXDIM] = {{0.0}};

  /*--- Component split of each spatial derivative. ---*/
  for (auto a = 0u; a < nDim; ++a) {
    su2double comp[MAXFIELD], compOdd[MAXFIELD];
    for (auto k = 0u; k < nField; ++k) comp[k] = grad[k * nDim + a];
    OddPart(nDim, n, comp, comp + nVar, compOdd, compOdd + nVar);
    for (auto k = 0u; k < nField; ++k) {
      odd[k][a] = compOdd[k];
      even[k][a] = comp[k] - compOdd[k];
    }
  }

  /*--- Even: tangential gradient; odd: normal gradient. ---*/
  for (auto k = 0u; k < nField; ++k) {
    su2double evenN = 0.0, oddN = 0.0;
    for (auto a = 0u; a < nDim; ++a) {
      evenN += even[k][a] * n[a];
      oddN += odd[k][a] * n[a];
    }
    for (auto a = 0u; a < nDim; ++a) grad[k * nDim + a] = even[k][a] - evenN * n[a] + oddN * n[a];
  }
}

void MirrorHessians(unsigned short nDim, const su2double* n, su2double* hess) {
  const auto nFlux = NumFlux(nDim);
  su2double odd[MAXFLUX][MAXDIM][MAXDIM] = {{{0.0}}}, even[MAXFLUX][MAXDIM][MAXDIM] = {{{0.0}}};

  for (auto a = 0u; a < nDim; ++a) {
    for (auto b = 0u; b < nDim; ++b) {
      su2double comp[MAXFLUX], compOdd[MAXFLUX];
      for (auto f = 0u; f < nFlux; ++f) comp[f] = hess[(f * nDim + a) * nDim + b];
      OddPart(nDim, n, nullptr, comp, nullptr, compOdd);
      for (auto f = 0u; f < nFlux; ++f) {
        odd[f][a][b] = compOdd[f];
        even[f][a][b] = comp[f] - compOdd[f];
      }
    }
  }

  /*--- X = Pn H Pt + Pt H Pn = n h^T + h n^T - 2 s n n^T, h = H n, s = n.H n. Even: H - X; odd: X. ---*/
  auto mixedBlock = [&](const su2double (&H)[MAXDIM][MAXDIM], su2double (&X)[MAXDIM][MAXDIM]) {
    su2double h[MAXDIM] = {0.0}, s = 0.0;
    for (auto a = 0u; a < nDim; ++a) {
      for (auto b = 0u; b < nDim; ++b) h[a] += H[a][b] * n[b];
      s += n[a] * h[a];
    }
    for (auto a = 0u; a < nDim; ++a)
      for (auto b = 0u; b < nDim; ++b) X[a][b] = n[a] * h[b] + h[a] * n[b] - 2.0 * s * n[a] * n[b];
  };

  for (auto f = 0u; f < nFlux; ++f) {
    su2double Xeven[MAXDIM][MAXDIM], Xodd[MAXDIM][MAXDIM];
    mixedBlock(even[f], Xeven);
    mixedBlock(odd[f], Xodd);
    for (auto a = 0u; a < nDim; ++a)
      for (auto b = 0u; b < nDim; ++b) hess[(f * nDim + a) * nDim + b] = even[f][a][b] - Xeven[a][b] + Xodd[a][b];
  }
}

bool AccumulateField(unsigned short nDim, su2double weight, const su2double* H, su2double (&Hgo)[MAXDIM][MAXDIM],
                     su2double (&S)[MAXDIM][MAXDIM]) {
  su2double Hm[MAXDIM][MAXDIM] = {{0.0}};
  bool finite = true;
  for (auto a = 0u; a < nDim; ++a)
    for (auto b = 0u; b < nDim; ++b) {
      Hm[a][b] = H[a * nDim + b];
      finite = finite && std::isfinite(SU2_TYPE::GetValue(Hm[a][b]));
    }
  if (!finite) return false;

  /*--- |H| = V |Lambda| V^T. ---*/
  su2double vec[MAXDIM][MAXDIM], val[MAXDIM], work[MAXDIM], absH[MAXDIM][MAXDIM];
  CBlasStructure::EigenDecomposition(Hm, vec, val, nDim, work);
  for (auto a = 0u; a < nDim; ++a) val[a] = fabs(val[a]);
  CBlasStructure::EigenRecomposition(absH, vec, val, nDim);

  for (auto a = 0u; a < nDim; ++a) {
    for (auto b = 0u; b < nDim; ++b) {
      Hgo[a][b] += fabs(weight) * absH[a][b];
      S[a][b] += weight * Hm[a][b];
    }
  }
  return true;
}

su2double NuclearNorm(unsigned short nDim, const su2double (&S)[MAXDIM][MAXDIM]) {
  su2double vec[MAXDIM][MAXDIM], val[MAXDIM], work[MAXDIM];
  CBlasStructure::EigenDecomposition(S, vec, val, nDim, work);
  su2double norm = 0.0;
  for (auto a = 0u; a < nDim; ++a) norm += fabs(val[a]);
  return norm;
}

void Eq33Terms(unsigned short nDim, const su2double* gradLambda, const su2double (&A)[MAXDIM][MAXVAR][MAXVAR],
               su2double& eq33Signed, su2double& eq33Abs) {
  const auto nVar = NumVar(nDim);
  eq33Signed = 0.0;
  eq33Abs = 0.0;
  for (auto j = 0u; j < nVar; ++j) {
    su2double G = 0.0;
    for (auto d = 0u; d < nDim; ++d) {
      for (auto k = 0u; k < nVar; ++k) {
        const su2double term = A[d][k][j] * gradLambda[k * nDim + d];
        G += term;
        eq33Abs += fabs(term);
      }
    }
    eq33Signed += fabs(G);
  }
}

unsigned short PointEstimate(unsigned short nDim, const su2double* gradLambda, const su2double* hess,
                             const su2double (&A)[MAXDIM][MAXVAR][MAXVAR], su2double (&Hgo)[MAXDIM][MAXDIM],
                             su2double& signedNorm, su2double& eq33Signed, su2double& eq33Abs) {
  const auto nVar = NumVar(nDim);
  su2double S[MAXDIM][MAXDIM] = {{0.0}};
  for (auto a = 0u; a < MAXDIM; ++a)
    for (auto b = 0u; b < MAXDIM; ++b) Hgo[a][b] = 0.0;
  unsigned short nonFinite = 0;

  for (auto d = 0u; d < nDim; ++d)
    for (auto j = 0u; j < nVar; ++j)
      if (!AccumulateField(nDim, gradLambda[j * nDim + d], hess + (d * nVar + j) * nDim * nDim, Hgo, S)) ++nonFinite;

  signedNorm = NuclearNorm(nDim, S);
  Eq33Terms(nDim, gradLambda, A, eq33Signed, eq33Abs);
  return nonFinite;
}

std::string CheckSymmetry(const CGeometry& geometry, const CConfig& config,
                          std::vector<std::array<su2double, MAXDIM>>& normals) {
  const auto nDim = geometry.GetnDim();
  const auto nMarker = geometry.GetnMarker();
  normals.assign(nMarker, {0.0, 0.0, 0.0});

  /*--- The markers of a rank are a subset of those of the config file: reductions use the config-file index. ---*/
  const auto nCfg = config.GetnMarker_CfgFile();
  std::vector<int> cfgOf(nMarker, -1);
  for (auto iMarker = 0u; iMarker < nMarker; ++iMarker)
    for (auto k = 0u; k < nCfg; ++k)
      if (config.GetMarker_CfgFile_TagBound(k) == config.GetMarker_All_TagBound(iMarker)) cfgOf[iMarker] = k;
  std::vector<bool> cfgSym(nCfg, false);
  bool anySymmetry = false;
  for (auto k = 0u; k < nCfg; ++k) {
    cfgSym[k] = config.GetMarker_CfgFile_KindBC(config.GetMarker_CfgFile_TagBound(k)) == SYMMETRY_PLANE;
    anySymmetry = anySymmetry || cfgSym[k];
  }
  if (!anySymmetry) return "";
  for (auto iMarker = 0u; iMarker < nMarker; ++iMarker) {
    if (config.GetMarker_All_KindBC(iMarker) == SYMMETRY_PLANE && cfgOf[iMarker] < 0)
      return "symmetry marker " + config.GetMarker_All_TagBound(iMarker) + " is not in the config file.";
  }

  /*--- Global bounding box: the tolerance scales with the mesh extent, independently of its origin. ---*/
  passivedouble localMin[MAXDIM], localMax[MAXDIM], globalMin[MAXDIM], globalMax[MAXDIM];
  for (auto iDim = 0u; iDim < nDim; ++iDim) {
    localMin[iDim] = std::numeric_limits<passivedouble>::max();
    localMax[iDim] = std::numeric_limits<passivedouble>::lowest();
  }
  for (auto iPoint = 0ul; iPoint < geometry.GetnPointDomain(); ++iPoint)
    for (auto iDim = 0u; iDim < nDim; ++iDim) {
      const auto coord = SU2_TYPE::GetValue(geometry.nodes->GetCoord(iPoint, iDim));
      localMin[iDim] = std::min(localMin[iDim], coord);
      localMax[iDim] = std::max(localMax[iDim], coord);
    }
  CPassiveComm::Allreduce(localMin, globalMin, nDim, CPassiveComm::Op::MIN);
  CPassiveComm::Allreduce(localMax, globalMax, nDim, CPassiveComm::Op::MAX);
  passivedouble extent = 0.0;
  for (auto iDim = 0u; iDim < nDim; ++iDim) extent = std::max(extent, globalMax[iDim] - globalMin[iDim]);
  const passivedouble tolDot = 1e-12, tolOrigin = 1e-10 * extent;

  /*--- Area-weighted normal and centroid of each symmetry marker (owned vertices, summed over the ranks).
   *    Centroids relative to the bounding-box origin avoid cancellation on translated meshes. ---*/
  std::vector<passivedouble> local(nCfg * 7, 0.0), global(nCfg * 7, 0.0);
  for (auto iMarker = 0u; iMarker < nMarker; ++iMarker) {
    if (config.GetMarker_All_KindBC(iMarker) != SYMMETRY_PLANE) continue;
    const auto k = cfgOf[iMarker];
    for (auto iVertex = 0ul; iVertex < geometry.GetnVertex(iMarker); ++iVertex) {
      const auto iPoint = geometry.vertex[iMarker][iVertex]->GetNode();
      if (!geometry.nodes->GetDomain(iPoint)) continue;
      const auto* normal = geometry.vertex[iMarker][iVertex]->GetNormal();
      passivedouble area = 0.0;
      for (auto iDim = 0u; iDim < nDim; ++iDim) area += pow(SU2_TYPE::GetValue(normal[iDim]), 2);
      area = sqrt(area);
      for (auto iDim = 0u; iDim < nDim; ++iDim) {
        local[k * 7 + iDim] += SU2_TYPE::GetValue(normal[iDim]);
        local[k * 7 + 3 + iDim] +=
            area * (SU2_TYPE::GetValue(geometry.nodes->GetCoord(iPoint, iDim)) - globalMin[iDim]);
      }
      local[k * 7 + 6] += area;
    }
  }
  CPassiveComm::Allreduce(local.data(), global.data(), local.size(), CPassiveComm::Op::SUM);

  std::vector<std::array<passivedouble, MAXDIM>> cfgNormal(nCfg, {0.0, 0.0, 0.0}), cfgCentroid(nCfg, {0.0, 0.0, 0.0});
  for (auto k = 0u; k < nCfg; ++k) {
    if (!cfgSym[k]) continue;
    passivedouble norm = 0.0;
    for (auto iDim = 0u; iDim < nDim; ++iDim) norm += pow(global[k * 7 + iDim], 2);
    norm = sqrt(norm);
    if (!(norm > 0.0) || !(global[k * 7 + 6] > 0.0)) {
      return "symmetry marker " + config.GetMarker_CfgFile_TagBound(k) + " has no area.";
    }
    for (auto iDim = 0u; iDim < nDim; ++iDim) {
      cfgNormal[k][iDim] = global[k * 7 + iDim] / norm;
      cfgCentroid[k][iDim] = global[k * 7 + 3 + iDim] / global[k * 7 + 6];
    }
  }
  std::vector<std::array<passivedouble, MAXDIM>> centroid(nMarker, {0.0, 0.0, 0.0});
  for (auto iMarker = 0u; iMarker < nMarker; ++iMarker) {
    if (config.GetMarker_All_KindBC(iMarker) != SYMMETRY_PLANE) continue;
    for (auto iDim = 0u; iDim < nDim; ++iDim) {
      normals[iMarker][iDim] = cfgNormal[cfgOf[iMarker]][iDim];
      centroid[iMarker][iDim] = cfgCentroid[cfgOf[iMarker]][iDim];
    }
  }

  /*--- Planarity (normals and offsets), orthogonality where two symmetry markers meet, and where a symmetry marker
   *    meets an Euler wall (the mirror rule must not undo the wall reconstruction of the normal momentum lambda). ---*/
  unsigned long nBad[4] = {0, 0, 0, 0};
  for (auto iMarker = 0u; iMarker < nMarker; ++iMarker) {
    if (config.GetMarker_All_KindBC(iMarker) != SYMMETRY_PLANE) continue;
    for (auto iVertex = 0ul; iVertex < geometry.GetnVertex(iMarker); ++iVertex) {
      const auto iPoint = geometry.vertex[iMarker][iVertex]->GetNode();
      if (!geometry.nodes->GetDomain(iPoint)) continue;
      const auto* normal = geometry.vertex[iMarker][iVertex]->GetNormal();
      passivedouble area = 0.0, normalError = 0.0;
      for (auto iDim = 0u; iDim < nDim; ++iDim) area += pow(SU2_TYPE::GetValue(normal[iDim]), 2);
      area = sqrt(area);
      if (area > 0.0) {
        for (auto iDim = 0u; iDim < nDim; ++iDim)
          normalError += pow(SU2_TYPE::GetValue(normal[iDim]) / area - SU2_TYPE::GetValue(normals[iMarker][iDim]), 2);
      }
      if (!(area > 0.0) || normalError > 1e-16) ++nBad[0];
      passivedouble offset = 0.0;
      for (auto iDim = 0u; iDim < nDim; ++iDim)
        offset +=
            (SU2_TYPE::GetValue(geometry.nodes->GetCoord(iPoint, iDim)) - globalMin[iDim] - centroid[iMarker][iDim]) *
            SU2_TYPE::GetValue(normals[iMarker][iDim]);
      if (fabs(offset) > tolOrigin) ++nBad[1];
      for (auto jMarker = 0u; jMarker < nMarker; ++jMarker) {
        const auto kind = config.GetMarker_All_KindBC(jMarker);
        if (jMarker == iMarker || (kind != SYMMETRY_PLANE && kind != EULER_WALL)) continue;
        const auto jVertex = geometry.nodes->GetVertex(iPoint, jMarker);
        if (jVertex < 0) continue;
        passivedouble other[MAXDIM] = {0.0}, otherArea = 0.0, cross = 0.0;
        if (kind == SYMMETRY_PLANE) {
          for (auto iDim = 0u; iDim < nDim; ++iDim) other[iDim] = SU2_TYPE::GetValue(normals[jMarker][iDim]);
          otherArea = 1.0;
        } else {
          const auto* wall = geometry.vertex[jMarker][jVertex]->GetNormal();
          for (auto iDim = 0u; iDim < nDim; ++iDim) {
            other[iDim] = SU2_TYPE::GetValue(wall[iDim]);
            otherArea += other[iDim] * other[iDim];
          }
          otherArea = sqrt(otherArea);
        }
        for (auto iDim = 0u; iDim < nDim; ++iDim) cross += SU2_TYPE::GetValue(normals[iMarker][iDim]) * other[iDim];
        if (!(otherArea > 0.0)) continue;
        cross /= otherArea;
        if (kind == SYMMETRY_PLANE && fabs(cross) >= 1e-8) ++nBad[2];
        if (kind == EULER_WALL && fabs(cross) >= 1e-6) ++nBad[3];
      }
    }
  }
  unsigned long nBadGlobal[4] = {0, 0, 0, 0};
  for (int i = 0; i < 4; ++i) nBadGlobal[i] = CPassiveComm::AllreduceSum(nBad[i]);
  if (nBadGlobal[0] + nBadGlobal[1] > 0) {
    return "the symmetry markers must be planar (" + std::to_string(nBadGlobal[0] + nBadGlobal[1]) +
           " vertices off the plane).";
  }
  if (nBadGlobal[2] > 0) {
    return "symmetry markers that meet must be orthogonal (" + std::to_string(nBadGlobal[2]) + " points).";
  }
  if (nBadGlobal[3] > 0) {
    return "symmetry markers must meet Euler walls at right angles (" + std::to_string(nBadGlobal[3]) + " points).";
  }

  /*--- Mirror invariance of the freestream and of the objectives (force directions as in
   *    ComputeAeroCoeffsFromForceMoment). ---*/
  const passivedouble alpha = SU2_TYPE::GetValue(config.GetAoA()) * PI_NUMBER / 180.0;
  const passivedouble beta = SU2_TYPE::GetValue(config.GetAoS()) * PI_NUMBER / 180.0;
  passivedouble drag[3] = {0.0}, lift[3] = {0.0};
  if (nDim == 2) {
    drag[0] = cos(alpha); drag[1] = sin(alpha);
    lift[0] = -sin(alpha); lift[1] = cos(alpha);
  } else {
    drag[0] = cos(alpha) * cos(beta); drag[1] = sin(beta); drag[2] = sin(alpha) * cos(beta);
    lift[0] = -sin(alpha); lift[2] = cos(alpha);
  }
  for (auto k = 0u; k < nCfg; ++k) {
    if (!cfgSym[k]) continue;
    passivedouble n[3] = {0.0};
    for (auto iDim = 0u; iDim < nDim; ++iDim) n[iDim] = cfgNormal[k][iDim];
    auto dot = [&](const passivedouble* v) {
      passivedouble value = 0.0;
      for (auto iDim = 0u; iDim < nDim; ++iDim) value += v[iDim] * n[iDim];
      return value;
    };
    const std::string tag = config.GetMarker_CfgFile_TagBound(k);
    if (fabs(dot(drag)) > tolDot) return "the freestream crosses the symmetry plane " + tag + " (AoA/AoS).";

    for (auto iObj = 0u; iObj < config.GetnObj(); ++iObj) {
      const auto kind = config.GetKind_ObjFunc(iObj);
      passivedouble axis[3] = {0.0};
      bool force = true;
      switch (kind) {
        case DRAG_COEFFICIENT: continue;  // the freestream direction, checked above
        case LIFT_COEFFICIENT: std::copy(lift, lift + 3, axis); break;
        case FORCE_X_COEFFICIENT: axis[0] = 1.0; break;
        case FORCE_Y_COEFFICIENT: axis[1] = 1.0; break;
        case FORCE_Z_COEFFICIENT: axis[2] = 1.0; break;
        case MOMENT_X_COEFFICIENT: axis[0] = 1.0; force = false; break;
        case MOMENT_Y_COEFFICIENT: axis[1] = 1.0; force = false; break;
        case MOMENT_Z_COEFFICIENT: axis[2] = 1.0; force = false; break;
        default:
          return "with the symmetry plane " + tag + " the objective must be DRAG, LIFT, FORCE_* in the plane or "
                 "MOMENT_* about the plane normal (other objectives need an odd adjoint).";
      }
      if (force && fabs(dot(axis)) > tolDot) {
        return "the objective force is not parallel to the symmetry plane " + tag + ".";
      }
      if (!force) {
        if (fabs(fabs(dot(axis)) - 1.0) > tolDot) {
          return "the moment axis of the objective is not normal to the symmetry plane " + tag + ".";
        }
        for (auto iMon = 0u; iMon < std::max<unsigned short>(config.GetnMarker_Monitoring(), 1); ++iMon) {
          const auto origin = config.GetRefOriginMoment(iMon);
          passivedouble offset = 0.0;
          for (auto iDim = 0u; iDim < nDim; ++iDim)
            offset += (SU2_TYPE::GetValue(origin[iDim]) - globalMin[iDim] - cfgCentroid[k][iDim]) * n[iDim];
          if (fabs(offset) > tolOrigin) return "the moment origin is not on the symmetry plane " + tag + ".";
        }
      }
    }
  }
  return "";
}

}  // namespace GoalMetric

void CSolver::ComputeGoalHessian(CGeometry* geometry, const CConfig* config, const vector<su2double>* state,
                                 vector<su2double>* fluxHessians) {
  SU2_ZONE_SCOPED

  using namespace GoalMetric;
  const unsigned short nDim = geometry->GetnDim();
  const unsigned short nVarG = NumVar(nDim), nFlux = NumFlux(nDim), nField = FieldCount(nDim);
  const unsigned short nMet = nDim * (nDim + 1) / 2;
  const unsigned long nPoint = geometry->GetnPoint(), nPointDomain = geometry->GetnPointDomain();
  const auto method = static_cast<ENUM_FLOW_GRADIENT>(config->GetKind_Hessian_Method());

  auto& field = base_nodes->GetAuxVar_Adapt();
  auto& gradient = base_nodes->GetGradient_Adapt();
  auto& hessian = base_nodes->GetHessian();
  auto& work = base_nodes->GetHessian_Field();
  auto& gradGrad = base_nodes->GetHessian_Grad();

  /*--- Storage and limits: the Green-Gauss kernel has static arrays of 20 fields, indexed by the field index. ---*/
  if (nField > MAXFIELD || field.rows() != nPoint || field.cols() != nField || gradient.length() != nPoint ||
      gradient.rows() != nField || gradient.cols() != nDim || hessian.length() != nPoint || hessian.rows() < 1 ||
      hessian.cols() != nMet || work.rows() != nPoint || work.cols() != nDim || gradGrad.length() != nPoint) {
    SU2_MPI::Error("The adaptation work arrays do not have the layout of the goal-oriented fields.", CURRENT_FUNCTION);
  }
  if (method != GREEN_GAUSS && method != WEIGHTED_LEAST_SQUARES) {
    SU2_MPI::Error("The goal-oriented metric needs NUM_METHOD_HESS= GREEN_GAUSS or WEIGHTED_LEAST_SQUARES.",
                   CURRENT_FUNCTION);
  }
  if (config->GetnMarker_Periodic() > 0) {
    SU2_MPI::Error("The goal-oriented metric does not support periodic markers.", CURRENT_FUNCTION);
  }
  if (state != nullptr && state->size() < nPointDomain * nVarG) {
    SU2_MPI::Error("The state for the goal-oriented diagnostics is too small.", CURRENT_FUNCTION);
  }

  /*--- Symmetry planes: supported cases and normals; owned points with the planes they lie on. ---*/
  vector<std::array<su2double, MAXDIM>> planeNormal;
  const auto symmetryError = CheckSymmetry(*geometry, *config, planeNormal);
  if (!symmetryError.empty()) {
    SU2_MPI::Error("Goal-oriented metric (ADAP_SENSOR= GOAL): " + symmetryError, CURRENT_FUNCTION);
  }
  vector<long> symIndex(nPointDomain, -1);
  vector<unsigned long> symPoints;
  vector<vector<unsigned short>> symPlanes;
  for (auto iMarker = 0u; iMarker < geometry->GetnMarker(); ++iMarker) {
    if (config->GetMarker_All_KindBC(iMarker) != SYMMETRY_PLANE) continue;
    for (auto iVertex = 0ul; iVertex < geometry->GetnVertex(iMarker); ++iVertex) {
      const auto iPoint = geometry->vertex[iMarker][iVertex]->GetNode();
      if (iPoint >= nPointDomain) continue;
      if (symIndex[iPoint] < 0) {
        symIndex[iPoint] = symPoints.size();
        symPoints.push_back(iPoint);
        symPlanes.emplace_back();
      }
      symPlanes[symIndex[iPoint]].push_back(iMarker);
    }
  }

  /*--- a. Values: halos; slip-wall reconstruction of the normal momentum lambda; symmetry rule; halos. ---*/

  InitiateComms(geometry, config, MPI_QUANTITIES::AUXVAR_ADAPT);
  CompleteComms(geometry, config, MPI_QUANTITIES::AUXVAR_ADAPT);

  unsigned long wallCounts[3] = {0, 0, 0};  // wall points, invalid donors, wall points not reconstructed

  if (config->GetAdap_Goal_Wall_Extrapolation()) {
    /*--- 1. Wall flags of the owners, to the halos. ---*/
    vector<char> isWall(nPoint, 0);
    vector<vector<std::array<passivedouble, MAXDIM>>> wallNormals(nPointDomain);
    for (auto iMarker = 0u; iMarker < geometry->GetnMarker(); ++iMarker) {
      if (config->GetMarker_All_KindBC(iMarker) != EULER_WALL) continue;
      for (auto iVertex = 0ul; iVertex < geometry->GetnVertex(iMarker); ++iVertex) {
        const auto iPoint = geometry->vertex[iMarker][iVertex]->GetNode();
        if (iPoint >= nPointDomain) continue;
        isWall[iPoint] = 1;
        const auto* normal = geometry->vertex[iMarker][iVertex]->GetNormal();
        passivedouble area = 0.0;
        for (auto iDim = 0u; iDim < nDim; ++iDim) area += pow(SU2_TYPE::GetValue(normal[iDim]), 2);
        area = sqrt(area);
        std::array<passivedouble, MAXDIM> unit = {0.0, 0.0, 0.0};
        for (auto iDim = 0u; iDim < nDim; ++iDim) unit[iDim] = SU2_TYPE::GetValue(normal[iDim]) / area;
        wallNormals[iPoint].push_back(unit);
      }
    }
    CPassiveComm::ExchangeHalo(*geometry, isWall.data(), 1);

    auto lambdaM = [&](unsigned long iPoint, unsigned short c) { return SU2_TYPE::GetValue(field(iPoint, 1 + c)); };
    auto coord = [&](unsigned long iPoint, unsigned short iDim) {
      return SU2_TYPE::GetValue(geometry->nodes->GetCoord(iPoint, iDim));
    };

    /*--- 2. Donors: owned non-wall points next to a wall, WLS gradient of the Cartesian momentum lambda over their
     *    non-wall neighbours; record = nDim x nDim gradient + validity. ---*/
    const size_t nRecord = nDim * nDim + 1;
    vector<passivedouble> donor(nPoint * nRecord, 0.0);
    for (auto jPoint = 0ul; jPoint < nPointDomain; ++jPoint) {
      if (isWall[jPoint]) continue;
      bool nextToWall = false;
      for (const auto kPoint : geometry->nodes->GetPoints(jPoint)) nextToWall = nextToWall || isWall[kPoint];
      if (!nextToWall) continue;

      passivedouble M[MAXDIM][MAXDIM] = {{0.0}}, D[MAXDIM][MAXDIM] = {{0.0}}, b[MAXDIM][MAXDIM] = {{0.0}};
      for (const auto kPoint : geometry->nodes->GetPoints(jPoint)) {
        if (isWall[kPoint]) continue;
        passivedouble d[MAXDIM] = {0.0}, d2 = 0.0;
        for (auto iDim = 0u; iDim < nDim; ++iDim) {
          d[iDim] = coord(kPoint, iDim) - coord(jPoint, iDim);
          d2 += d[iDim] * d[iDim];
        }
        if (!(d2 > 0.0)) continue;
        const passivedouble w = 1.0 / d2;
        for (auto a = 0u; a < nDim; ++a) {
          for (auto bDim = 0u; bDim < nDim; ++bDim) {
            M[a][bDim] += w * d[a] * d[bDim];
            D[a][bDim] += d[a] * d[bDim] / d2;
          }
          for (auto c = 0u; c < nDim; ++c) b[c][a] += w * d[a] * (lambdaM(kPoint, c) - lambdaM(jPoint, c));
        }
      }

      /*--- Scale-independent rank test on the unit directions, then g = M^-1 b by the eigen decomposition. ---*/
      passivedouble vec[MAXDIM][MAXDIM], val[MAXDIM], tmp[MAXDIM];
      CBlasStructure::EigenDecomposition(D, vec, val, nDim, tmp);
      const passivedouble dMax = *std::max_element(val, val + nDim), dMin = *std::min_element(val, val + nDim);
      if (!(dMax > 0.0) || dMin < 1e-6 * dMax) {
        ++wallCounts[1];
        continue;
      }
      CBlasStructure::EigenDecomposition(M, vec, val, nDim, tmp);
      auto* record = &donor[jPoint * nRecord];
      for (auto c = 0u; c < nDim; ++c) {
        for (auto a = 0u; a < nDim; ++a) {
          passivedouble g = 0.0;
          for (auto k = 0u; k < nDim; ++k) {
            passivedouble proj = 0.0;
            for (auto e = 0u; e < nDim; ++e) proj += vec[e][k] * b[c][e];
            g += vec[a][k] * proj / val[k];
          }
          record[c * nDim + a] = g;
        }
      }
      record[nDim * nDim] = 1.0;
    }
    CPassiveComm::ExchangeHalo(*geometry, donor.data(), nRecord * sizeof(passivedouble));

    /*--- 3. Wall owners: extrapolation from the non-wall neighbours, the normal part replaces the G1a value. ---*/
    for (auto iPoint = 0ul; iPoint < nPointDomain; ++iPoint) {
      if (!isWall[iPoint]) continue;
      ++wallCounts[0];
      passivedouble ext[MAXDIM] = {0.0}, wSum = 0.0;
      for (const auto jPoint : geometry->nodes->GetPoints(iPoint)) {
        if (isWall[jPoint]) continue;
        passivedouble d[MAXDIM] = {0.0}, dist = 0.0;
        for (auto iDim = 0u; iDim < nDim; ++iDim) {
          d[iDim] = coord(iPoint, iDim) - coord(jPoint, iDim);
          dist += d[iDim] * d[iDim];
        }
        dist = sqrt(dist);
        if (!(dist > 0.0)) continue;
        const passivedouble w = 1.0 / dist;
        const auto* record = &donor[jPoint * nRecord];
        for (auto c = 0u; c < nDim; ++c) {
          passivedouble value = lambdaM(jPoint, c);
          for (auto a = 0u; a < nDim; ++a) value += record[c * nDim + a] * d[a];
          ext[c] += w * value;
        }
        wSum += w;
      }
      if (!(wSum > 0.0)) {
        ++wallCounts[2];
        continue;
      }
      for (auto c = 0u; c < nDim; ++c) ext[c] /= wSum;

      /*--- Orthonormal basis of the wall normals of the point (nearly parallel normals merged). ---*/
      vector<std::array<passivedouble, MAXDIM>> basis;
      for (auto normal : wallNormals[iPoint]) {
        for (const auto& q : basis) {
          passivedouble proj = 0.0;
          for (auto iDim = 0u; iDim < nDim; ++iDim) proj += q[iDim] * normal[iDim];
          for (auto iDim = 0u; iDim < nDim; ++iDim) normal[iDim] -= proj * q[iDim];
        }
        passivedouble norm = 0.0;
        for (auto iDim = 0u; iDim < nDim; ++iDim) norm += normal[iDim] * normal[iDim];
        norm = sqrt(norm);
        if (norm < 1e-3) continue;
        for (auto iDim = 0u; iDim < nDim; ++iDim) normal[iDim] /= norm;
        basis.push_back(normal);
      }
      passivedouble lam[MAXDIM] = {0.0};
      for (auto c = 0u; c < nDim; ++c) lam[c] = lambdaM(iPoint, c);
      for (const auto& q : basis) {
        passivedouble projOld = 0.0, projExt = 0.0;
        for (auto iDim = 0u; iDim < nDim; ++iDim) {
          projOld += q[iDim] * lam[iDim];
          projExt += q[iDim] * ext[iDim];
        }
        for (auto iDim = 0u; iDim < nDim; ++iDim) lam[iDim] += (projExt - projOld) * q[iDim];
      }
      for (auto c = 0u; c < nDim; ++c) field(iPoint, 1 + c) = lam[c];
    }
  }

  for (size_t iSym = 0; iSym < symPoints.size(); ++iSym) {
    for (const auto iMarker : symPlanes[iSym]) MirrorValues(nDim, planeNormal[iMarker].data(), &field(symPoints[iSym], 0));
  }
  InitiateComms(geometry, config, MPI_QUANTITIES::AUXVAR_ADAPT);
  CompleteComms(geometry, config, MPI_QUANTITIES::AUXVAR_ADAPT);

  /*--- b. Gradients of all fields, no boundary correction, then the symmetry rule, then halos. ---*/

  auto rawGradient = [&](const auto& values, size_t nVarField, auto& result) {
    if (method == GREEN_GAUSS) {
      computeGradientsGreenGauss(nullptr, MPI_QUANTITIES::GRADIENT_ADAPT, PERIODIC_NONE, *geometry, *config, values, 0,
                                 nVarField, -1, result, false, false);
    } else {
      computeGradientsLeastSquares(nullptr, MPI_QUANTITIES::GRADIENT_ADAPT, PERIODIC_NONE, *geometry, *config, true,
                                   values, 0, nVarField, -1, result, base_nodes->GetRmatrix(), false, false);
    }
  };
  rawGradient(field, nField, gradient);

  for (size_t iSym = 0; iSym < symPoints.size(); ++iSym) {
    const auto iPoint = symPoints[iSym];
    su2double grad[MAXFIELD * MAXDIM];
    for (auto k = 0u; k < nField; ++k)
      for (auto a = 0u; a < nDim; ++a) grad[k * nDim + a] = gradient(iPoint, k, a);
    for (const auto iMarker : symPlanes[iSym]) MirrorGradients(nDim, planeNormal[iMarker].data(), grad);
    for (auto k = 0u; k < nField; ++k)
      for (auto a = 0u; a < nDim; ++a) gradient(iPoint, k, a) = grad[k * nDim + a];
  }
  InitiateComms(geometry, config, MPI_QUANTITIES::GRADIENT_ADAPT);
  CompleteComms(geometry, config, MPI_QUANTITIES::GRADIENT_ADAPT);

  /*--- c. Hessian of each flux field (gradient of its gradient), accumulated into the estimate; symmetry points are
   *    finished after the loop, the rule needs all the fields of a point. ---*/

  const unsigned short nFull = nDim * nDim;
  vector<su2double> Hgo(nPointDomain * nFull, 0.0), Ssum(nPointDomain * nFull, 0.0);
  vector<su2double> symHess(symPoints.size() * nFlux * nFull, 0.0);
  unsigned long nRejected = 0;
  if (fluxHessians != nullptr) fluxHessians->assign(nPointDomain * nFlux * nFull, 0.0);

  auto accumulate = [&](unsigned long iPoint, unsigned short f, const su2double* H) {
    const unsigned short d = f / nVarG, j = f % nVarG;
    su2double hgo[MAXDIM][MAXDIM] = {{0.0}}, sum[MAXDIM][MAXDIM] = {{0.0}};
    if (!AccumulateField(nDim, gradient(iPoint, j, d), H, hgo, sum)) {
      ++nRejected;
      return;
    }
    for (auto a = 0u; a < nDim; ++a)
      for (auto b = 0u; b < nDim; ++b) {
        Hgo[iPoint * nFull + a * nDim + b] += hgo[a][b];
        Ssum[iPoint * nFull + a * nDim + b] += sum[a][b];
      }
  };

  for (unsigned short f = 0; f < nFlux; ++f) {
    const unsigned short k = nVarG + f;
    for (auto iPoint = 0ul; iPoint < nPoint; ++iPoint)
      for (auto a = 0u; a < nDim; ++a) work(iPoint, a) = gradient(iPoint, k, a);
    rawGradient(work, nDim, gradGrad);

    for (auto iPoint = 0ul; iPoint < nPointDomain; ++iPoint) {
      su2double H[MAXDIM * MAXDIM];
      for (auto a = 0u; a < nDim; ++a)
        for (auto b = 0u; b < nDim; ++b) H[a * nDim + b] = 0.5 * (gradGrad(iPoint, a, b) + gradGrad(iPoint, b, a));
      if (symIndex[iPoint] >= 0) {
        std::copy(H, H + nFull, &symHess[(symIndex[iPoint] * nFlux + f) * nFull]);
      } else {
        accumulate(iPoint, f, H);
        if (fluxHessians != nullptr) std::copy(H, H + nFull, &(*fluxHessians)[(iPoint * nFlux + f) * nFull]);
      }
    }
  }
  for (size_t iSym = 0; iSym < symPoints.size(); ++iSym) {
    auto* hess = &symHess[iSym * nFlux * nFull];
    for (const auto iMarker : symPlanes[iSym]) MirrorHessians(nDim, planeNormal[iMarker].data(), hess);
    for (unsigned short f = 0; f < nFlux; ++f) accumulate(symPoints[iSym], f, hess + f * nFull);
    if (fluxHessians != nullptr) std::copy(hess, hess + nFlux * nFull, &(*fluxHessians)[symPoints[iSym] * nFlux * nFull]);
  }

  /*--- d. H_go into the Hessian slot 0, diagnostics, halos. ---*/

  GoalDiagnostics.resize(nPoint, 2 + nDim) = 0.0;
  constexpr int nBin = 100;
  vector<passivedouble> localStats(4 + 2 * nBin, 0.0);  // weighted sums (r_sign, r_33, weight), volume, histograms
  passivedouble localMinRatio = std::numeric_limits<passivedouble>::max();
  unsigned long nNonFinite = 0, nZero = 0;

  for (auto iPoint = 0ul; iPoint < nPointDomain; ++iPoint) {
    su2double M[MAXDIM][MAXDIM] = {{0.0}}, S[MAXDIM][MAXDIM] = {{0.0}};
    bool finite = true;
    for (auto a = 0u; a < nDim; ++a)
      for (auto b = 0u; b < nDim; ++b) {
        M[a][b] = Hgo[iPoint * nFull + a * nDim + b];
        S[a][b] = Ssum[iPoint * nFull + a * nDim + b];
        finite = finite && std::isfinite(SU2_TYPE::GetValue(M[a][b])) && std::isfinite(SU2_TYPE::GetValue(S[a][b]));
      }
    for (unsigned short iMet = 0, a = 0; a < nDim; ++a)
      for (auto b = a; b < nDim; ++b, ++iMet) hessian(iPoint, 0, iMet) = M[a][b];
    for (auto c = 0u; c < nDim; ++c) GoalDiagnostics(iPoint, 2 + c) = SU2_TYPE::GetValue(field(iPoint, 1 + c));
    if (!finite) {
      ++nNonFinite;
      continue;
    }

    passivedouble trace = 0.0;
    for (auto a = 0u; a < nDim; ++a) trace += SU2_TYPE::GetValue(M[a][a]);
    if (!std::isfinite(trace)) {
      ++nNonFinite;
      continue;
    }
    su2double vec[MAXDIM][MAXDIM], val[MAXDIM], tmp[MAXDIM];
    CBlasStructure::EigenDecomposition(M, vec, val, nDim, tmp);
    if (trace > 0.0) {
      localMinRatio = std::min(localMinRatio, SU2_TYPE::GetValue(*std::min_element(val, val + nDim)) / trace);
    } else {
      ++nZero;
    }

    passivedouble rSign = (trace > 0.0) ? SU2_TYPE::GetValue(NuclearNorm(nDim, S)) / trace : 0.0;
    passivedouble r33 = 0.0;
    if (state != nullptr) {
      su2double A[MAXDIM][MAXVAR][MAXVAR], gradLambda[MAXVAR * MAXDIM], num = 0.0, den = 0.0;
      FluxJacobians(nDim, config->GetGamma(), &(*state)[iPoint * nVarG], A);
      for (auto j = 0u; j < nVarG; ++j)
        for (auto d = 0u; d < nDim; ++d) gradLambda[j * nDim + d] = gradient(iPoint, j, d);
      Eq33Terms(nDim, gradLambda, A, num, den);
      finite = std::isfinite(SU2_TYPE::GetValue(num)) && std::isfinite(SU2_TYPE::GetValue(den));
      if (finite && trace > 0.0 && den > 0.0) r33 = SU2_TYPE::GetValue(num / den);
    }
    if (!finite || !std::isfinite(rSign) || !std::isfinite(r33)) {
      ++nNonFinite;
      continue;  // ratios remain 0; invalid diagnostics do not enter the statistics or histograms
    }
    /*--- Finite ratios are in [0,1], up to round-off. Clamp before converting to histogram bins. ---*/
    rSign = std::min(1.0, std::max(0.0, rSign));
    r33 = std::min(1.0, std::max(0.0, r33));
    GoalDiagnostics(iPoint, 0) = rSign;
    GoalDiagnostics(iPoint, 1) = r33;

    const passivedouble volume = SU2_TYPE::GetValue(geometry->nodes->GetVolume(iPoint));
    const passivedouble weight = trace * volume;
    localStats[0] += weight * rSign;
    localStats[1] += weight * r33;
    localStats[2] += weight;
    localStats[3] += volume;
    const int binSign = std::min(nBin - 1, static_cast<int>(rSign * nBin));
    const int bin33 = std::min(nBin - 1, static_cast<int>(r33 * nBin));
    localStats[4 + binSign] += volume;
    localStats[4 + nBin + bin33] += volume;
  }

  InitiateComms(geometry, config, MPI_QUANTITIES::HESSIAN);
  CompleteComms(geometry, config, MPI_QUANTITIES::HESSIAN);

  /*--- e. Global report. ---*/

  vector<passivedouble> stats(localStats.size(), 0.0);
  CPassiveComm::Allreduce(localStats.data(), stats.data(), stats.size(), CPassiveComm::Op::SUM);
  const passivedouble minRatio = CPassiveComm::Allreduce(localMinRatio, CPassiveComm::Op::MIN);
  unsigned long counts[6] = {nRejected, nNonFinite, nZero, wallCounts[0], wallCounts[1], wallCounts[2]};
  for (auto& count : counts) count = CPassiveComm::AllreduceSum(count);
  GoalRejected = counts[0];
  GoalNonFinite = counts[1];
  GoalMinRatio = (minRatio == std::numeric_limits<passivedouble>::max()) ? 0.0 : minRatio;
  for (int k = 0; k < 3; ++k) GoalWallCounts[k] = counts[3 + k];

  if (rank == MASTER_NODE) {
    auto quantile = [&](int offset, passivedouble q) {
      passivedouble cumulative = 0.0;
      for (int bin = 0; bin < nBin; ++bin) {
        cumulative += stats[offset + bin];
        if (cumulative >= q * stats[3]) return (bin + 1.0) / nBin;
      }
      return 1.0;
    };
    cout << "Goal-oriented Hessian (ADAP_SENSOR= GOAL): " << counts[0] << " rejected (non-finite) flux Hessians, "
         << counts[1] << " non-finite estimates or diagnostics, " << counts[2] << " points with H_go = 0." << endl;
    cout << "  min over points of lambda_min(H_go)/tr(H_go): " << std::scientific << std::setprecision(3)
         << ((minRatio == std::numeric_limits<passivedouble>::max()) ? 0.0 : minRatio) << std::defaultfloat
         << std::setprecision(6) << " (positive semidefinite: >= -1e-12)." << endl;
    if (stats[2] > 0.0) {
      cout << "  signed-sum ratio ||sum g H||_* / tr(H_go): weighted mean " << stats[0] / stats[2]
           << ", volume quantiles 10/50/90 %: " << quantile(4, 0.1) << " " << quantile(4, 0.5) << " "
           << quantile(4, 0.9) << "." << endl;
      cout << "  eq. (33) ratio |sum_d A_d^T grad_d lambda| / sum |.|: weighted mean " << stats[1] / stats[2]
           << ", volume quantiles 10/50/90 %: " << quantile(4 + nBin, 0.1) << " " << quantile(4 + nBin, 0.5) << " "
           << quantile(4 + nBin, 0.9) << (state == nullptr ? " (no state given)." : ".") << endl;
    } else {
      cout << "  H_go is zero everywhere, the ratios are 0." << endl;
    }
    if (config->GetAdap_Goal_Wall_Extrapolation() && counts[3] > 0) {
      cout << "  slip-wall lambda_n reconstructed on " << counts[3] - counts[5] << " of " << counts[3]
           << " wall points (" << counts[4] << " donors without a full-rank stencil)." << endl;
    }
  }
}
