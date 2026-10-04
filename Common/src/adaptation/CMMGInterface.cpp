/*!
 * \file CMMGInterface.cpp
 * \brief Conversion of the SU2 mesh and adaptation metric to MMG (MMG2D/MMG3D) and remeshing in memory.
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

#include "../../include/adaptation/CMMGInterface.hpp"
#include "../../include/adaptation/CMeshGather.hpp"

#include <algorithm>
#include <array>
#include <climits>
#include <cmath>
#include <iostream>
#include <set>

#include "../../include/CConfig.hpp"
#include "../../include/geometry/CGeometry.hpp"
#include "../../include/linear_algebra/blas_structure.hpp"

#ifdef HAVE_MMG
#include "mmg/libmmg.h"
#endif

namespace {

/*--- Signed volume (area in 2D) of a simplex, positive for SU2's and MMG's orientation. ---*/
passivedouble SignedVolume(unsigned short nDim, const std::vector<passivedouble>& coord, const unsigned long* nodes) {
  const auto* x0 = &coord[nodes[0] * nDim];
  const auto* x1 = &coord[nodes[1] * nDim];
  const auto* x2 = &coord[nodes[2] * nDim];
  if (nDim == 2) {
    return 0.5 * ((x1[0] - x0[0]) * (x2[1] - x0[1]) - (x1[1] - x0[1]) * (x2[0] - x0[0]));
  }
  const auto* x3 = &coord[nodes[3] * nDim];
  const passivedouble a[] = {x1[0] - x0[0], x1[1] - x0[1], x1[2] - x0[2]};
  const passivedouble b[] = {x2[0] - x0[0], x2[1] - x0[1], x2[2] - x0[2]};
  const passivedouble c[] = {x3[0] - x0[0], x3[1] - x0[1], x3[2] - x0[2]};
  return (c[0] * (a[1] * b[2] - a[2] * b[1]) + c[1] * (a[2] * b[0] - a[0] * b[2]) + c[2] * (a[0] * b[1] - a[1] * b[0])) /
         6.0;
}

using Face = std::array<unsigned long, 3>;

/*--- Sorted face nodes, the unused entry of 2D faces (edges) is ULONG_MAX. ---*/
Face MakeFace(unsigned short nDim, const unsigned long* nodes) {
  Face face = {ULONG_MAX, ULONG_MAX, ULONG_MAX};
  for (unsigned short i = 0; i < nDim; ++i) face[i] = nodes[i];
  std::sort(face.begin(), face.begin() + nDim);
  return face;
}

string PointInfo(unsigned short nDim, const std::vector<passivedouble>& coord, unsigned long iPoint) {
  string info = "point " + std::to_string(iPoint) + " (";
  for (unsigned short iDim = 0; iDim < nDim; ++iDim)
    info += std::to_string(coord[iPoint * nDim + iDim]) + (iDim + 1 < nDim ? ", " : ")");
  return info;
}

}  // namespace

bool CMMGInterface::IsFinitePositiveDefinite(unsigned short nDim, const passivedouble* metric) {
  const unsigned short nMetric = CSimplexMesh::GetnMetric(nDim);
  for (unsigned short iMet = 0; iMet < nMetric; ++iMet)
    if (!std::isfinite(metric[iMet])) return false;

  /*--- Full matrix from the upper triangle, (xx,xy,yy) or (xx,xy,xz,yy,yz,zz). ---*/
  passivedouble M[3][3] = {{0.0}};
  for (unsigned short iDim = 0, iMet = 0; iDim < nDim; ++iDim)
    for (unsigned short jDim = iDim; jDim < nDim; ++jDim, ++iMet) M[iDim][jDim] = M[jDim][iDim] = metric[iMet];

  /*--- Scaled to a unit diagonal, D^-1/2 M D^-1/2, whose eigenvalues are computed accurately also for strongly
   *    anisotropic metrics (determinants and Sylvester's criterion lose them to cancellation). The smallest one,
   *    relative to the largest, must stand clear of round-off: for a metric of aspect ratio AR it is at least
   *    about 1 / AR^2, for a singular one it is round-off (about 1e-15). ---*/
  passivedouble scale[3];
  for (unsigned short iDim = 0; iDim < nDim; ++iDim) {
    if (!(M[iDim][iDim] > 0.0)) return false;
    scale[iDim] = 1.0 / sqrt(M[iDim][iDim]);
  }
  for (unsigned short iDim = 0; iDim < nDim; ++iDim)
    for (unsigned short jDim = 0; jDim < nDim; ++jDim) M[iDim][jDim] *= scale[iDim] * scale[jDim];

  passivedouble vec[3][3], val[3], work[3];
  CBlasStructure::EigenDecomposition(M, vec, val, nDim, work);

  constexpr passivedouble minEigenRatio = 1e-14;
  return val[0] > minEigenRatio * val[nDim - 1];
}

void CMMGInterface::FloorFixedBoundaryMetric(CSimplexMesh& mesh) {
  const auto nDim = mesh.nDim;
  const auto nMetric = CSimplexMesh::GetnMetric(nDim);
  using Mat = passivedouble[3][3];

  /*--- f(A) for a symmetric A, eigenvectors kept. ---*/
  auto spectral = [nDim](const Mat& A, Mat& B, passivedouble (*f)(passivedouble)) {
    passivedouble vec[3][3], val[3], work[3];
    CBlasStructure::EigenDecomposition(A, vec, val, nDim, work);
    for (unsigned short i = 0; i < nDim; ++i) val[i] = f(val[i]);
    CBlasStructure::EigenRecomposition(B, vec, val, nDim);
  };
  auto product = [nDim](const Mat& A, const Mat& B, Mat& C) {
    for (unsigned short i = 0; i < nDim; ++i)
      for (unsigned short j = 0; j < nDim; ++j) {
        C[i][j] = 0.0;
        for (unsigned short k = 0; k < nDim; ++k) C[i][j] += A[i][k] * B[k][j];
      }
  };

  /*--- Edges of the boundary faces at each point. ---*/
  std::vector<std::vector<std::array<passivedouble, 3>>> edges(mesh.GetnPoint());
  for (const auto& marker : mesh.markers) {
    for (unsigned long iFace = 0; iFace < marker.GetnElem(nDim); ++iFace) {
      const auto* face = &marker.elem[iFace * nDim];
      for (unsigned short a = 0; a < nDim; ++a) {
        for (unsigned short b = 0; b < nDim; ++b) {
          if (a == b) continue;
          std::array<passivedouble, 3> e = {0.0, 0.0, 0.0};
          for (unsigned short iDim = 0; iDim < nDim; ++iDim)
            e[iDim] = mesh.coord[face[b] * nDim + iDim] - mesh.coord[face[a] * nDim + iDim];
          edges[face[a]].push_back(e);
        }
      }
    }
  }

  /*--- At each boundary point, for each boundary edge e at it that is longer than 1 in the metric: the union of the
   *    metric with the size |e| along e. In terms of size tensors S = M^-1 (squared sizes): S' = S^1/2 max(1,
   *    S^-1/2 e e^T S^-1/2) S^1/2, the smallest size tensor that contains S and the segment e (the intersection of
   *    metrics by simultaneous reduction, applied to the inverses); the sizes across e are kept. On a curved
   *    boundary the faces are chords inclined to the tangent of their points by half the turn of the boundary, so a
   *    much smaller normal size (a boundary layer) is raised to about |e| sin(turn / 2) at the points: the fixed
   *    faces cannot carry a thinner first cell. ---*/
  for (unsigned long iPoint = 0; iPoint < mesh.GetnPoint(); ++iPoint) {
    if (edges[iPoint].empty()) continue;
    auto* metric = &mesh.metric[iPoint * nMetric];
    Mat M = {{0.0}};
    for (unsigned short iDim = 0, iMet = 0; iDim < nDim; ++iDim)
      for (unsigned short jDim = iDim; jDim < nDim; ++jDim, ++iMet) M[iDim][jDim] = M[jDim][iDim] = metric[iMet];

    bool changed = false;
    for (const auto& e : edges[iPoint]) {
      passivedouble Me = 0.0;
      for (unsigned short i = 0; i < nDim; ++i)
        for (unsigned short j = 0; j < nDim; ++j) Me += e[i] * M[i][j] * e[j];
      if (Me <= 1.0) continue;  // not longer than 1 in the metric

      Mat S, half, invHalf, E = {{0.0}}, T, tmp, Snew;
      spectral(M, S, [](passivedouble v) -> passivedouble { return 1.0 / v; });
      spectral(S, half, [](passivedouble v) -> passivedouble { return sqrt(v); });
      spectral(S, invHalf, [](passivedouble v) -> passivedouble { return 1.0 / sqrt(v); });
      for (unsigned short i = 0; i < nDim; ++i)
        for (unsigned short j = 0; j < nDim; ++j) E[i][j] = e[i] * e[j];
      product(invHalf, E, tmp);
      product(tmp, invHalf, T);
      spectral(T, T, [](passivedouble v) -> passivedouble { return std::max(v, passivedouble(1.0)); });
      product(half, T, tmp);
      product(tmp, half, Snew);
      for (unsigned short i = 0; i < nDim; ++i)
        for (unsigned short j = 0; j < i; ++j) Snew[i][j] = Snew[j][i] = 0.5 * (Snew[i][j] + Snew[j][i]);
      spectral(Snew, M, [](passivedouble v) -> passivedouble { return 1.0 / v; });
      changed = true;
    }
    if (!changed) continue;
    for (unsigned short iDim = 0, iMet = 0; iDim < nDim; ++iDim)
      for (unsigned short jDim = iDim; jDim < nDim; ++jDim, ++iMet) metric[iMet] = 0.5 * (M[iDim][jDim] + M[jDim][iDim]);
  }
}

passivedouble CMMGInterface::CoarseningRatio(unsigned short nDim, const passivedouble* metricIn,
                                             const passivedouble* metricOut) {
  using Mat = passivedouble[3][3];
  Mat A = {{0.0}}, B = {{0.0}};
  for (unsigned short iDim = 0, iMet = 0; iDim < nDim; ++iDim)
    for (unsigned short jDim = iDim; jDim < nDim; ++jDim, ++iMet) {
      A[iDim][jDim] = A[jDim][iDim] = metricIn[iMet];
      B[iDim][jDim] = B[jDim][iDim] = metricOut[iMet];
    }
  /*--- A^-1/2 (eigenvalues of a valid metric are positive). ---*/
  passivedouble vec[3][3], val[3], work[3];
  Mat invHalf;
  CBlasStructure::EigenDecomposition(A, vec, val, nDim, work);
  for (unsigned short i = 0; i < nDim; ++i) val[i] = 1.0 / sqrt(val[i]);
  CBlasStructure::EigenRecomposition(invHalf, vec, val, nDim);
  Mat tmp = {{0.0}}, C = {{0.0}};
  for (unsigned short i = 0; i < nDim; ++i)
    for (unsigned short j = 0; j < nDim; ++j)
      for (unsigned short k = 0; k < nDim; ++k) tmp[i][j] += invHalf[i][k] * B[k][j];
  for (unsigned short i = 0; i < nDim; ++i)
    for (unsigned short j = 0; j < nDim; ++j)
      for (unsigned short k = 0; k < nDim; ++k) C[i][j] += tmp[i][k] * invHalf[k][j];
  for (unsigned short i = 0; i < nDim; ++i)
    for (unsigned short j = 0; j < i; ++j) C[i][j] = C[j][i] = 0.5 * (C[i][j] + C[j][i]);
  CBlasStructure::EigenDecomposition(C, vec, val, nDim, work);
  passivedouble minVal = val[0];
  for (unsigned short i = 1; i < nDim; ++i) minVal = std::min(minVal, val[i]);
  return minVal;
}

std::vector<CMMGInterface::LocalParameter> CMMGInterface::WallLocalParameters(const CSimplexMesh& mesh) const {
  std::vector<LocalParameter> local;
  const auto nDim = mesh.nDim;
  for (const auto& marker : mesh.markers) {
    if (std::find(params.boundaryLayerMarkers.begin(), params.boundaryLayerMarkers.end(), marker.name) ==
        params.boundaryLayerMarkers.end())
      continue;
    passivedouble longest = 0.0;
    for (auto iFace = 0ul; iFace < marker.GetnElem(nDim); ++iFace) {
      const auto* face = &marker.elem[iFace * nDim];
      for (unsigned short a = 0; a < nDim; ++a) {
        const auto b = (a + 1) % nDim;
        passivedouble len2 = 0.0;
        for (unsigned short iDim = 0; iDim < nDim; ++iDim)
          len2 += pow(mesh.coord[face[b] * nDim + iDim] - mesh.coord[face[a] * nDim + iDim], 2);
        longest = std::max(longest, sqrt(len2));
      }
    }
    if (longest <= 0.0) continue;
    LocalParameter param;
    param.ref = marker.ref;
    param.hmin = params.hmin;
    param.hmax = std::max(params.hmin, std::min(params.hmax, 2.0 * longest));
    param.hausd = params.hausd;
    local.push_back(param);
  }
  return local;
}

void CMMGInterface::CheckSupport(const CConfig& config, const CGeometry& geometry) {
  if (config.GetMultizone_Problem() || config.GetnZone() > 1 || config.GetnMarker_ZoneInterface() > 0) {
    SU2_MPI::Error("Mesh adaptation does not support multizone problems or sliding meshes.", CURRENT_FUNCTION);
  }
  if (config.GetnMarker_Periodic() > 0) {
    SU2_MPI::Error("Mesh adaptation does not support periodic boundaries yet.", CURRENT_FUNCTION);
  }
  if (config.GetGrid_Movement() || config.GetDynamic_Grid() || config.GetDeform_Mesh()) {
    SU2_MPI::Error("Mesh adaptation does not support moving or deforming meshes.", CURRENT_FUNCTION);
  }
  if (config.GetFEMSolver()) {
    SU2_MPI::Error("Mesh adaptation does not support the FEM solvers.", CURRENT_FUNCTION);
  }
  if (!config.GetMarkerCreateCopy().empty()) {
    SU2_MPI::Error("Mesh adaptation does not support MARKER_CREATE_COPY yet.", CURRENT_FUNCTION);
  }
  const auto nDim = geometry.GetnDim();
  if (nDim != 2 && nDim != 3) {
    SU2_MPI::Error("Mesh adaptation needs a 2D or 3D mesh.", CURRENT_FUNCTION);
  }
  /*--- Halo points come from the MPI partition only (periodic boundaries are rejected above); the mesh is gathered
   *    on the master rank for MMG (ExtractMesh). ---*/
  for (unsigned short iMarker = 0; iMarker < geometry.GetnMarker(); ++iMarker) {
    const auto kind = config.GetMarker_All_KindBC(iMarker);
    const auto tag = config.GetMarker_All_TagBound(iMarker);
    switch (kind) {
      case PERIODIC_BOUNDARY:
        SU2_MPI::Error("Mesh adaptation does not support periodic boundaries yet (marker " + tag + ").",
                       CURRENT_FUNCTION);
        break;
      case ACTDISK_INLET:
      case ACTDISK_OUTLET:
      case NEARFIELD_BOUNDARY:
      case FLUID_INTERFACE:
        SU2_MPI::Error("Mesh adaptation does not support paired boundaries (actuator disk, near field, fluid "
                       "interface): marker " + tag + ".", CURRENT_FUNCTION);
        break;
      default:
        break;
    }
  }
}

int CMMGInterface::GetMarkerReference(const CConfig& config, const string& name) {
  std::vector<string> names;
  for (unsigned short iMarker = 0; iMarker < config.GetnMarker_CfgFile(); ++iMarker)
    names.push_back(config.GetMarker_CfgFile_TagBound(iMarker));
  std::sort(names.begin(), names.end());
  names.erase(std::unique(names.begin(), names.end()), names.end());

  const auto it = std::lower_bound(names.begin(), names.end(), name);
  if (it == names.end() || *it != name) {
    SU2_MPI::Error("The configuration file doesn't have any definition for marker " + name, CURRENT_FUNCTION);
  }
  return 1 + static_cast<int>(it - names.begin());
}

CSimplexMesh CMMGInterface::ExtractMesh(const CConfig& config, const CGeometry& geometry,
                                        const su2activematrix& metric) {
  const auto nDim = geometry.GetnDim();
  if (nDim != 2 && nDim != 3) SU2_MPI::Error("Mesh adaptation needs a 2D or 3D mesh.", CURRENT_FUNCTION);
  const auto nPointDomain = geometry.GetnPointDomain();
  const auto nMetric = CSimplexMesh::GetnMetric(nDim);
  if (geometry.GetGlobal_nPointDomain() >= static_cast<unsigned long>(INT_MAX) ||
      geometry.GetGlobal_nElemDomain() >= static_cast<unsigned long>(INT_MAX)) {
    SU2_MPI::Error("The mesh is too large for MMG (32-bit indices).", CURRENT_FUNCTION);
  }
  if (metric.rows() < nPointDomain || metric.cols() != nMetric) {
    SU2_MPI::Error("The adaptation metric is not available (COMPUTE_METRIC = YES is required).", CURRENT_FUNCTION);
  }

  /*--- The metric of the points of this rank must be finite and positive definite. ---*/
  std::vector<su2double> localMetric(nPointDomain * nMetric);
  for (auto iPoint = 0ul; iPoint < nPointDomain; ++iPoint) {
    passivedouble value[6] = {0.0};
    for (unsigned short iMet = 0; iMet < nMetric; ++iMet) {
      localMetric[iPoint * nMetric + iMet] = metric(iPoint, iMet);
      value[iMet] = SU2_TYPE::GetValue(metric(iPoint, iMet));
    }
    if (!IsFinitePositiveDefinite(nDim, value)) {
      std::vector<passivedouble> x(nDim);
      for (unsigned short iDim = 0; iDim < nDim; ++iDim)
        x[iDim] = SU2_TYPE::GetValue(geometry.nodes->GetCoord(iPoint, iDim));
      const auto point = "point " + std::to_string(geometry.nodes->GetGlobalIndex(iPoint));
      SU2_MPI::Error("The adaptation metric is not finite and positive definite (or numerically singular) at the " +
                         PointInfo(nDim, x, 0).replace(0, 7, point) + ".",
                     CURRENT_FUNCTION);
    }
  }

  /*--- The whole mesh on the master rank, in the global numbering of the mesh (the same for any partition, see
   *    CMeshGather): points, simplices, boundary elements by marker in the order of the config (the order the mesh
   *    output writes them, so the geometry built from the adapted mesh in memory and the one read from its exported
   *    file have the same marker order; it matters where markers share points, e.g. a symmetry plane and an Euler
   *    wall applied one after the other). The config describes this geometry. ---*/
  std::vector<string> markerTags;
  for (unsigned short iMarker = 0; iMarker < geometry.GetnMarker(); ++iMarker)
    markerTags.push_back(config.GetMarker_All_TagBound(iMarker));
  const CMeshGather gather(geometry);
  auto mesh = gather.GatherMesh(config, markerTags, false);
  const auto globalMetric = gather.Gather(localMetric.data(), nMetric);
  if (!gather.IsRoot()) return mesh;

  mesh.metric.resize(globalMetric.size());
  for (auto i = 0ul; i < globalMetric.size(); ++i) mesh.metric[i] = SU2_TYPE::GetValue(globalMetric[i]);

  /*--- Volume elements positively oriented. ---*/
  const unsigned short nNode = nDim + 1;
  unsigned long nFlip = 0;
  for (auto iElem = 0ul; iElem < mesh.GetnElem(); ++iElem) {
    auto* nodes = &mesh.elem[iElem * nNode];
    const auto volume = SignedVolume(nDim, mesh.coord, nodes);
    if (volume < 0.0) {
      std::swap(nodes[nNode - 2], nodes[nNode - 1]);
      ++nFlip;
    } else if (!(volume > 0.0)) {
      SU2_MPI::Error("Element " + std::to_string(iElem) + " has zero volume.", CURRENT_FUNCTION);
    }
  }
  if (nFlip > 0) {
    cout << "Mesh adaptation: reoriented " << nFlip << " elements with negative volume." << endl;
  }
  return mesh;
}

void CMMGInterface::ValidateMesh(const CSimplexMesh& mesh, const CSimplexMesh* reference, const string& what) {
  const auto nDim = mesh.nDim;
  if (nDim != 2 && nDim != 3) SU2_MPI::Error(what + ": invalid dimension.", CURRENT_FUNCTION);
  const unsigned short nNode = nDim + 1;
  const auto nPoint = mesh.GetnPoint();
  const auto nElem = mesh.GetnElem();

  if (nPoint == 0 || nElem == 0 || mesh.coord.size() != nPoint * nDim || mesh.elem.size() != nElem * nNode ||
      mesh.elemRef.size() != nElem ||
      (!mesh.metric.empty() && mesh.metric.size() != nPoint * CSimplexMesh::GetnMetric(nDim))) {
    SU2_MPI::Error(what + ": empty mesh or inconsistent array sizes.", CURRENT_FUNCTION);
  }
  for (const auto x : mesh.coord) {
    if (!std::isfinite(x)) SU2_MPI::Error(what + ": non-finite coordinates.", CURRENT_FUNCTION);
  }

  /*--- Connectivity in range, all points used, positive volumes. ---*/
  std::vector<bool> used(nPoint, false);
  unsigned long nBadVolume = 0;
  for (auto iElem = 0ul; iElem < nElem; ++iElem) {
    const auto* nodes = &mesh.elem[iElem * nNode];
    for (unsigned short iNode = 0; iNode < nNode; ++iNode) {
      if (nodes[iNode] >= nPoint) SU2_MPI::Error(what + ": element node out of range.", CURRENT_FUNCTION);
      used[nodes[iNode]] = true;
    }
    if (!(SignedVolume(nDim, mesh.coord, nodes) > 0.0)) ++nBadVolume;
  }
  if (nBadVolume > 0) {
    SU2_MPI::Error(what + ": " + std::to_string(nBadVolume) + " elements with zero or negative volume.",
                   CURRENT_FUNCTION);
  }
  const auto nUnused = std::count(used.begin(), used.end(), false);
  if (nUnused > 0) {
    SU2_MPI::Error(what + ": " + std::to_string(nUnused) + " points are not used by any element.", CURRENT_FUNCTION);
  }

  /*--- Faces of the volume elements, the faces of only one element are the boundary of the mesh. ---*/
  std::vector<Face> faces;
  faces.reserve(nElem * nNode);
  for (auto iElem = 0ul; iElem < nElem; ++iElem) {
    const auto* nodes = &mesh.elem[iElem * nNode];
    for (unsigned short iFace = 0; iFace < nNode; ++iFace) {
      unsigned long faceNodes[3] = {0, 0, 0};
      for (unsigned short iNode = 0, k = 0; iNode < nNode; ++iNode)
        if (iNode != iFace) faceNodes[k++] = nodes[iNode];
      faces.push_back(MakeFace(nDim, faceNodes));
    }
  }
  std::sort(faces.begin(), faces.end());
  std::vector<Face> uniqueFaces;
  std::vector<unsigned short> nFaceElem;
  for (const auto& face : faces) {
    if (uniqueFaces.empty() || uniqueFaces.back() != face) {
      uniqueFaces.push_back(face);
      nFaceElem.push_back(0);
    }
    ++nFaceElem.back();
  }
  const auto nNonManifold = std::count_if(nFaceElem.begin(), nFaceElem.end(), [](unsigned short n) { return n > 2; });
  if (nNonManifold > 0) {
    SU2_MPI::Error(what + ": " + std::to_string(nNonManifold) + " faces shared by more than two elements.",
                   CURRENT_FUNCTION);
  }

  /*--- Every marker element is a face of the mesh, every boundary face belongs to exactly one marker. ---*/
  std::vector<unsigned short> nMarkerElem(uniqueFaces.size(), 0);
  std::set<string> names;
  for (const auto& marker : mesh.markers) {
    if (marker.ref <= 0 || !names.insert(marker.name).second) {
      SU2_MPI::Error(what + ": invalid or repeated marker " + marker.name + ".", CURRENT_FUNCTION);
    }
    if (marker.elem.size() % nDim != 0) SU2_MPI::Error(what + ": inconsistent marker array.", CURRENT_FUNCTION);
    for (auto iElem = 0ul; iElem < marker.GetnElem(nDim); ++iElem) {
      const auto* nodes = &marker.elem[iElem * nDim];
      for (unsigned short iNode = 0; iNode < nDim; ++iNode) {
        if (nodes[iNode] >= nPoint) SU2_MPI::Error(what + ": boundary node out of range.", CURRENT_FUNCTION);
      }
      const auto face = MakeFace(nDim, nodes);
      const auto it = std::lower_bound(uniqueFaces.begin(), uniqueFaces.end(), face);
      if (it == uniqueFaces.end() || *it != face) {
        SU2_MPI::Error(what + ": an element of marker " + marker.name + " is not a face of the mesh.",
                       CURRENT_FUNCTION);
      }
      ++nMarkerElem[it - uniqueFaces.begin()];
    }
  }
  unsigned long nUncovered = 0, nRepeated = 0;
  for (auto iFace = 0ul; iFace < uniqueFaces.size(); ++iFace) {
    if (nFaceElem[iFace] == 1 && nMarkerElem[iFace] == 0) ++nUncovered;
    if (nMarkerElem[iFace] > 1) ++nRepeated;
  }
  if (nUncovered > 0) {
    SU2_MPI::Error(what + ": " + std::to_string(nUncovered) + " boundary faces do not belong to any marker.",
                   CURRENT_FUNCTION);
  }
  if (nRepeated > 0) {
    SU2_MPI::Error(what + ": " + std::to_string(nRepeated) + " faces belong to more than one marker element.",
                   CURRENT_FUNCTION);
  }

  /*--- No marker is lost. ---*/
  if (reference != nullptr) {
    for (const auto& refMarker : reference->markers) {
      if (refMarker.elem.empty()) continue;
      const auto* marker = mesh.FindMarker(refMarker.name);
      if (marker == nullptr || marker->elem.empty()) {
        SU2_MPI::Error(what + ": marker " + refMarker.name + " has no elements left.", CURRENT_FUNCTION);
      }
    }
  }
}

void CMMGInterface::CheckSameBoundary(const CSimplexMesh& mesh, const CSimplexMesh& reference, const string& what) {
  const auto nDim = mesh.nDim;
  if (reference.nDim != nDim) SU2_MPI::Error(what + ": the dimension changed.", CURRENT_FUNCTION);

  /*--- Boundary points of a mesh, sorted by their coordinates (exact comparison). ---*/
  using Coord = std::array<passivedouble, 3>;
  auto boundaryPoints = [nDim](const CSimplexMesh& m) {
    std::vector<std::pair<Coord, unsigned long>> points;
    std::vector<bool> isBoundary(m.GetnPoint(), false);
    for (const auto& marker : m.markers)
      for (const auto iPoint : marker.elem) isBoundary[iPoint] = true;
    for (auto iPoint = 0ul; iPoint < m.GetnPoint(); ++iPoint) {
      if (!isBoundary[iPoint]) continue;
      Coord x = {0.0, 0.0, 0.0};
      for (unsigned short iDim = 0; iDim < nDim; ++iDim) x[iDim] = m.coord[iPoint * nDim + iDim];
      points.emplace_back(x, iPoint);
    }
    std::sort(points.begin(), points.end());
    return points;
  };
  const auto points = boundaryPoints(mesh), refPoints = boundaryPoints(reference);
  if (points.size() != refPoints.size()) {
    SU2_MPI::Error(what + ": " + std::to_string(points.size()) + " boundary points instead of " +
                       std::to_string(refPoints.size()) + ".",
                   CURRENT_FUNCTION);
  }
  std::vector<unsigned long> toReference(mesh.GetnPoint(), ULONG_MAX);
  for (auto i = 0ul; i < points.size(); ++i) {
    if (points[i].first != refPoints[i].first) {
      SU2_MPI::Error(what + ": the boundary point " + PointInfo(nDim, reference.coord, refPoints[i].second) +
                         " was moved or removed.",
                     CURRENT_FUNCTION);
    }
    toReference[points[i].second] = refPoints[i].second;
  }

  /*--- Faces of each marker, as sorted reference point indices. ---*/
  for (const auto& refMarker : reference.markers) {
    const auto* marker = mesh.FindMarker(refMarker.name);
    std::vector<Face> faces, refFaces;
    if (marker != nullptr) {
      for (auto iElem = 0ul; iElem < marker->GetnElem(nDim); ++iElem) {
        unsigned long nodes[3] = {0, 0, 0};
        for (unsigned short iNode = 0; iNode < nDim; ++iNode) nodes[iNode] = toReference[marker->elem[iElem * nDim + iNode]];
        faces.push_back(MakeFace(nDim, nodes));
      }
    }
    for (auto iElem = 0ul; iElem < refMarker.GetnElem(nDim); ++iElem) refFaces.push_back(MakeFace(nDim, &refMarker.elem[iElem * nDim]));
    std::sort(faces.begin(), faces.end());
    std::sort(refFaces.begin(), refFaces.end());
    if (faces != refFaces) {
      SU2_MPI::Error(what + ": the boundary faces of marker " + refMarker.name + " changed (" +
                         std::to_string(faces.size()) + " faces, " + std::to_string(refFaces.size()) + " before).",
                     CURRENT_FUNCTION);
    }
  }
  for (const auto& marker : mesh.markers) {
    if (!marker.elem.empty() && reference.FindMarker(marker.name) == nullptr) {
      SU2_MPI::Error(what + ": new marker " + marker.name + ".", CURRENT_FUNCTION);
    }
  }
}

#ifdef HAVE_MMG

struct CMMGInterface::MMGData {
  MMG5_pMesh mesh = nullptr;
  MMG5_pSol met = nullptr;
  unsigned short nDim = 0;

  void Free() {
    if (mesh == nullptr) return;
    if (nDim == 2) {
      MMG2D_Free_all(MMG5_ARG_start, MMG5_ARG_ppMesh, &mesh, MMG5_ARG_ppMet, &met, MMG5_ARG_end);
    } else {
      MMG3D_Free_all(MMG5_ARG_start, MMG5_ARG_ppMesh, &mesh, MMG5_ARG_ppMet, &met, MMG5_ARG_end);
    }
    mesh = nullptr;
    met = nullptr;
  }
  ~MMGData() { Free(); }
};

CMMGInterface::CMMGInterface(const CConfig& config) : mmg(new MMGData) {
  params.hmin = SU2_TYPE::GetValue(config.GetAdap_Hmin());
  params.hmax = SU2_TYPE::GetValue(config.GetAdap_Hmax());
  params.hgrad = SU2_TYPE::GetValue(config.GetAdap_Hgrad());
  params.hausd = SU2_TYPE::GetValue(config.GetAdap_Hausd());
  params.angle = SU2_TYPE::GetValue(config.GetAdap_Angle());
  params.surface = config.GetAdap_Surface();
  params.boundaryLayer = (config.GetnAdap_BL() > 0);
  for (unsigned short iBL = 0; iBL < config.GetnAdap_BL(); ++iBL)
    params.boundaryLayerMarkers.push_back(config.GetAdap_BL(iBL).marker);
  params.localWallHmax = config.GetAdap_BL_LocalHmax();
}

CMMGInterface::~CMMGInterface() = default;

void CMMGInterface::SetMesh(const CSimplexMesh& mesh) {
  const auto nDim = mesh.nDim;
  if (nDim != 2 && nDim != 3) SU2_MPI::Error("MMG needs a 2D or 3D mesh.", CURRENT_FUNCTION);
  const unsigned short nNode = nDim + 1;
  const auto nMetric = CSimplexMesh::GetnMetric(nDim);
  const auto nPoint = mesh.GetnPoint();
  const auto nElem = mesh.GetnElem();
  if (mesh.metric.size() != nPoint * nMetric) SU2_MPI::Error("The mesh has no metric.", CURRENT_FUNCTION);

  unsigned long nBound = 0;
  for (const auto& marker : mesh.markers) nBound += marker.GetnElem(nDim);
  if (nPoint >= static_cast<unsigned long>(INT_MAX) || nElem >= static_cast<unsigned long>(INT_MAX) ||
      nBound >= static_cast<unsigned long>(INT_MAX)) {
    SU2_MPI::Error("The mesh is too large for MMG (32-bit indices).", CURRENT_FUNCTION);
  }

  mmg->Free();
  mmg->nDim = nDim;
  this->nDim = nDim;
  markerInfo.clear();
  for (const auto& marker : mesh.markers) {
    if (marker.ref <= 0) SU2_MPI::Error("Marker references must be positive.", CURRENT_FUNCTION);
    CSimplexMesh::Marker info;
    info.name = marker.name;
    info.ref = marker.ref;
    markerInfo.push_back(info);
  }
  localParams.clear();
  if (!localOverride.empty()) localParams = localOverride;
  else if (params.localWallHmax) localParams = WallLocalParameters(mesh);

  /*--- MMG arrays: 1-based indices. ---*/
  std::vector<double> coord(mesh.coord.begin(), mesh.coord.end());
  std::vector<int> pointRef(nPoint, 0);
  std::vector<int> elem(nElem * nNode);
  for (auto i = 0ul; i < elem.size(); ++i) elem[i] = static_cast<int>(mesh.elem[i]) + 1;
  std::vector<int> elemRef(mesh.elemRef.begin(), mesh.elemRef.end());
  std::vector<int> bound, boundRef;
  bound.reserve(nBound * nDim);
  boundRef.reserve(nBound);
  for (const auto& marker : mesh.markers) {
    for (auto iElem = 0ul; iElem < marker.GetnElem(nDim); ++iElem) {
      for (unsigned short iNode = 0; iNode < nDim; ++iNode)
        bound.push_back(static_cast<int>(marker.elem[iElem * nDim + iNode]) + 1);
      boundRef.push_back(marker.ref);
    }
  }
  std::vector<double> metric(mesh.metric.begin(), mesh.metric.end());

  /*--- Points shared by 2 (2D) or 3 (3D) markers are corners of the boundary partition, keep them. In 3D the
   * lines between two markers are reference edges, which MMG keeps by itself. ---*/
  std::vector<std::vector<int>> pointMarkers(nPoint);
  for (const auto& marker : mesh.markers) {
    for (const auto iPoint : marker.elem) {
      auto& refs = pointMarkers[iPoint];
      if (std::find(refs.begin(), refs.end(), marker.ref) == refs.end()) refs.push_back(marker.ref);
    }
  }
  std::vector<int> corners;
  for (auto iPoint = 0ul; iPoint < nPoint; ++iPoint)
    if (pointMarkers[iPoint].size() >= nDim) corners.push_back(static_cast<int>(iPoint) + 1);
  for (const auto iPoint : params.requiredPoints) {
    if (iPoint >= nPoint) SU2_MPI::Error("Required point out of range.", CURRENT_FUNCTION);
    const int k = static_cast<int>(iPoint) + 1;
    if (std::find(corners.begin(), corners.end(), k) == corners.end()) corners.push_back(k);
  }
  std::vector<int> required;
  for (const auto iPoint : params.requiredVertices) {
    if (iPoint >= nPoint) SU2_MPI::Error("Required vertex out of range.", CURRENT_FUNCTION);
    required.push_back(static_cast<int>(iPoint) + 1);
  }

  /*--- Boundary elements of the required markers (surface adapted elsewhere), 1-based in the order of bound. ---*/
  std::vector<int> requiredBound;
  {
    int k = 0;
    for (const auto& marker : mesh.markers) {
      const bool required = std::find(params.requiredMarkers.begin(), params.requiredMarkers.end(), marker.name) !=
                            params.requiredMarkers.end();
      for (auto iElem = 0ul; iElem < marker.GetnElem(nDim); ++iElem) {
        ++k;
        if (required) requiredBound.push_back(k);
      }
    }
  }

  /*--- Fixed surface: MMG keeps the boundary points but scales the coordinates to a unit box and back, which changes
   *    them by round-off. Their reference is their index + 1, so GetMesh can restore the exact input coordinates. ---*/
  fixedBoundary.clear();
  if (!params.surface) {
    fixedBoundary.resize(nPoint, false);
    for (auto iPoint = 0ul; iPoint < nPoint; ++iPoint) {
      if (pointMarkers[iPoint].empty()) continue;
      fixedBoundary[iPoint] = true;
      pointRef[iPoint] = static_cast<int>(iPoint) + 1;
    }
    fixedCoord = mesh.coord;
  }

  int ok = 1;
  if (nDim == 2) {
    MMG2D_Init_mesh(MMG5_ARG_start, MMG5_ARG_ppMesh, &mmg->mesh, MMG5_ARG_ppMet, &mmg->met, MMG5_ARG_end);
    ok &= MMG2D_Set_iparameter(mmg->mesh, mmg->met, MMG2D_IPARAM_verbose, params.verbosity);
    ok &= MMG2D_Set_meshSize(mmg->mesh, nPoint, nElem, 0, nBound);
    ok &= MMG2D_Set_vertices(mmg->mesh, coord.data(), pointRef.data());
    ok &= MMG2D_Set_triangles(mmg->mesh, elem.data(), elemRef.data());
    if (nBound > 0) ok &= MMG2D_Set_edges(mmg->mesh, bound.data(), boundRef.data());
    for (const auto k : requiredBound) ok &= MMG2D_Set_requiredEdge(mmg->mesh, k);
    for (const auto k : corners) {
      ok &= MMG2D_Set_corner(mmg->mesh, k);
      ok &= MMG2D_Set_requiredVertex(mmg->mesh, k);
    }
    for (const auto k : required) ok &= MMG2D_Set_requiredVertex(mmg->mesh, k);
    ok &= MMG2D_Set_solSize(mmg->mesh, mmg->met, MMG5_Vertex, nPoint, MMG5_Tensor);
    ok &= MMG2D_Set_tensorSols(mmg->met, metric.data());
    if (ok) ok &= MMG2D_Chk_meshData(mmg->mesh, mmg->met);
  } else {
    MMG3D_Init_mesh(MMG5_ARG_start, MMG5_ARG_ppMesh, &mmg->mesh, MMG5_ARG_ppMet, &mmg->met, MMG5_ARG_end);
    ok &= MMG3D_Set_iparameter(mmg->mesh, mmg->met, MMG3D_IPARAM_verbose, params.verbosity);
    ok &= MMG3D_Set_meshSize(mmg->mesh, nPoint, nElem, 0, nBound, 0, 0);
    ok &= MMG3D_Set_vertices(mmg->mesh, coord.data(), pointRef.data());
    ok &= MMG3D_Set_tetrahedra(mmg->mesh, elem.data(), elemRef.data());
    if (nBound > 0) ok &= MMG3D_Set_triangles(mmg->mesh, bound.data(), boundRef.data());
    for (const auto k : requiredBound) ok &= MMG3D_Set_requiredTriangle(mmg->mesh, k);
    for (const auto k : corners) {
      ok &= MMG3D_Set_corner(mmg->mesh, k);
      ok &= MMG3D_Set_requiredVertex(mmg->mesh, k);
    }
    for (const auto k : required) ok &= MMG3D_Set_requiredVertex(mmg->mesh, k);
    ok &= MMG3D_Set_solSize(mmg->mesh, mmg->met, MMG5_Vertex, nPoint, MMG5_Tensor);
    ok &= MMG3D_Set_tensorSols(mmg->met, metric.data());
    if (ok) ok &= MMG3D_Chk_meshData(mmg->mesh, mmg->met);
  }
  if (!ok) SU2_MPI::Error("Could not load the mesh and metric into MMG.", CURRENT_FUNCTION);
}

CMMGInterface::Status CMMGInterface::Remesh() {
  if (mmg->mesh == nullptr) SU2_MPI::Error("No mesh loaded into MMG.", CURRENT_FUNCTION);

  int ok = 1, ier = MMG5_STRONGFAILURE;
  if (nDim == 2) {
    ok &= MMG2D_Set_iparameter(mmg->mesh, mmg->met, MMG2D_IPARAM_verbose, params.verbosity);
    ok &= MMG2D_Set_iparameter(mmg->mesh, mmg->met, MMG2D_IPARAM_angle, 1);
    ok &= MMG2D_Set_dparameter(mmg->mesh, mmg->met, MMG2D_DPARAM_angleDetection, params.angle);
    ok &= MMG2D_Set_dparameter(mmg->mesh, mmg->met, MMG2D_DPARAM_hmin, params.hmin);
    ok &= MMG2D_Set_dparameter(mmg->mesh, mmg->met, MMG2D_DPARAM_hmax, params.hmax);
    ok &= MMG2D_Set_dparameter(mmg->mesh, mmg->met, MMG2D_DPARAM_hgrad, params.hgrad);
    ok &= MMG2D_Set_dparameter(mmg->mesh, mmg->met, MMG2D_DPARAM_hausd, params.hausd);
    ok &= MMG2D_Set_iparameter(mmg->mesh, mmg->met, MMG2D_IPARAM_nosurf, params.surface ? 0 : 1);
    ok &= MMG2D_Set_iparameter(mmg->mesh, mmg->met, MMG2D_IPARAM_nosizreq, params.surface ? 0 : 1);
    if (!params.surface) ok &= MMG2D_Set_dparameter(mmg->mesh, mmg->met, MMG2D_DPARAM_hgradreq, -1.0);
    const bool noswap = (params.swap < 0) ? params.boundaryLayer : (params.swap == 0);
    ok &= MMG2D_Set_iparameter(mmg->mesh, mmg->met, MMG2D_IPARAM_noswap, noswap ? 1 : 0);
    if (!localParams.empty()) {
      ok &= MMG2D_Set_iparameter(mmg->mesh, mmg->met, MMG2D_IPARAM_numberOfLocalParam,
                                 static_cast<int>(localParams.size()));
      for (const auto& local : localParams)
        ok &= MMG2D_Set_localParameter(mmg->mesh, mmg->met, MMG5_Edg, local.ref, local.hmin, local.hmax, local.hausd);
    }
    if (ok) ier = MMG2D_mmg2dlib(mmg->mesh, mmg->met);
  } else {
    ok &= MMG3D_Set_iparameter(mmg->mesh, mmg->met, MMG3D_IPARAM_verbose, params.verbosity);
    ok &= MMG3D_Set_iparameter(mmg->mesh, mmg->met, MMG3D_IPARAM_angle, 1);
    ok &= MMG3D_Set_dparameter(mmg->mesh, mmg->met, MMG3D_DPARAM_angleDetection, params.angle);
    ok &= MMG3D_Set_dparameter(mmg->mesh, mmg->met, MMG3D_DPARAM_hmin, params.hmin);
    ok &= MMG3D_Set_dparameter(mmg->mesh, mmg->met, MMG3D_DPARAM_hmax, params.hmax);
    ok &= MMG3D_Set_dparameter(mmg->mesh, mmg->met, MMG3D_DPARAM_hgrad, params.hgrad);
    ok &= MMG3D_Set_dparameter(mmg->mesh, mmg->met, MMG3D_DPARAM_hausd, params.hausd);
    ok &= MMG3D_Set_iparameter(mmg->mesh, mmg->met, MMG3D_IPARAM_nosurf, params.surface ? 0 : 1);
    ok &= MMG3D_Set_iparameter(mmg->mesh, mmg->met, MMG3D_IPARAM_nosizreq, params.surface ? 0 : 1);
    if (!params.surface) ok &= MMG3D_Set_dparameter(mmg->mesh, mmg->met, MMG3D_DPARAM_hgradreq, -1.0);
    if (!localParams.empty()) {
      ok &= MMG3D_Set_iparameter(mmg->mesh, mmg->met, MMG3D_IPARAM_numberOfLocalParam,
                                 static_cast<int>(localParams.size()));
      for (const auto& local : localParams)
        ok &= MMG3D_Set_localParameter(mmg->mesh, mmg->met, MMG5_Triangle, local.ref, local.hmin, local.hmax,
                                       local.hausd);
    }
    if (ok) ier = MMG3D_mmg3dlib(mmg->mesh, mmg->met);
  }
  if (!ok) SU2_MPI::Error("Could not set the MMG parameters.", CURRENT_FUNCTION);

  switch (ier) {
    case MMG5_SUCCESS:
      status = Status::SUCCESS;
      return status;
    case MMG5_LOWFAILURE:
      cout << "WARNING: MMG could not fully adapt the mesh (MMG5_LOWFAILURE). The mesh it returned is "
              "accepted only if it passes validation, it may not follow the metric." << endl;
      status = Status::LOWFAILURE;
      return status;
    default:
      SU2_MPI::Error("MMG failed (MMG5_STRONGFAILURE, status " + std::to_string(ier) + "), no valid mesh.",
                     CURRENT_FUNCTION);
      return Status::STRONGFAILURE;
  }
}

CSimplexMesh CMMGInterface::GetMesh() const {
  if (mmg->mesh == nullptr) SU2_MPI::Error("No mesh loaded into MMG.", CURRENT_FUNCTION);

  const unsigned short nNode = nDim + 1;
  const auto nMetric = CSimplexMesh::GetnMetric(nDim);
  int np = 0, ne = 0, nb = 0, nprism = 0, nquad = 0, na = 0, ok = 1;
  int solEntity = 0, solNp = 0, solType = 0;
  if (nDim == 2) {
    ok &= MMG2D_Get_meshSize(mmg->mesh, &np, &ne, &nquad, &nb);
    ok &= MMG2D_Get_solSize(mmg->mesh, mmg->met, &solEntity, &solNp, &solType);
  } else {
    ok &= MMG3D_Get_meshSize(mmg->mesh, &np, &ne, &nprism, &nb, &nquad, &na);
    ok &= MMG3D_Get_solSize(mmg->mesh, mmg->met, &solEntity, &solNp, &solType);
  }
  if (!ok || np <= 0 || ne <= 0) SU2_MPI::Error("Could not read the mesh size from MMG.", CURRENT_FUNCTION);
  if (nprism > 0 || nquad > 0) SU2_MPI::Error("MMG returned non-simplex elements.", CURRENT_FUNCTION);
  const bool hasMetric = (solNp == np && solType == MMG5_Tensor);

  std::vector<double> coord(np * nDim), metric(hasMetric ? np * nMetric : 0);
  std::vector<int> pointRef(np), corner(np), required(np);
  std::vector<int> elem(ne * nNode), elemRef(ne), elemRequired(ne);
  std::vector<int> bound(nb * nDim), boundRef(nb), boundRidge(nb), boundRequired(nb);
  if (nDim == 2) {
    ok &= MMG2D_Get_vertices(mmg->mesh, coord.data(), pointRef.data(), corner.data(), required.data());
    ok &= MMG2D_Get_triangles(mmg->mesh, elem.data(), elemRef.data(), elemRequired.data());
    if (nb > 0) ok &= MMG2D_Get_edges(mmg->mesh, bound.data(), boundRef.data(), boundRidge.data(), boundRequired.data());
    if (hasMetric) ok &= MMG2D_Get_tensorSols(mmg->met, metric.data());
  } else {
    ok &= MMG3D_Get_vertices(mmg->mesh, coord.data(), pointRef.data(), corner.data(), required.data());
    ok &= MMG3D_Get_tetrahedra(mmg->mesh, elem.data(), elemRef.data(), elemRequired.data());
    if (nb > 0) ok &= MMG3D_Get_triangles(mmg->mesh, bound.data(), boundRef.data(), boundRequired.data());
    if (hasMetric) ok &= MMG3D_Get_tensorSols(mmg->met, metric.data());
  }
  if (!ok) SU2_MPI::Error("Could not read the mesh from MMG.", CURRENT_FUNCTION);

  /*--- Back to 0-based indices. ---*/
  CSimplexMesh mesh;
  mesh.nDim = nDim;
  mesh.coord.assign(coord.begin(), coord.end());

  /*--- Fixed surface: the exact input coordinates of the kept boundary points (identified by their reference and
   *    checked to be within round-off of the input point). ---*/
  fixedSource.assign(np, -1);
  if (!fixedBoundary.empty()) {
    passivedouble size2 = 0.0;
    for (unsigned short iDim = 0; iDim < nDim; ++iDim) {
      passivedouble xMin = coord[iDim], xMax = coord[iDim];
      for (int iPoint = 0; iPoint < np; ++iPoint) {
        xMin = std::min(xMin, coord[iPoint * nDim + iDim]);
        xMax = std::max(xMax, coord[iPoint * nDim + iDim]);
      }
      size2 += pow(xMax - xMin, 2);
    }
    const passivedouble tol2 = 1e-20 * size2;  // 1e-10 of the domain size
    for (int iPoint = 0; iPoint < np; ++iPoint) {
      const auto ref = pointRef[iPoint];
      if (ref < 1 || ref > static_cast<int>(fixedBoundary.size()) || !fixedBoundary[ref - 1]) continue;
      passivedouble dist2 = 0.0;
      for (unsigned short iDim = 0; iDim < nDim; ++iDim)
        dist2 += pow(coord[iPoint * nDim + iDim] - fixedCoord[(ref - 1) * nDim + iDim], 2);
      if (dist2 > tol2) continue;
      fixedSource[iPoint] = ref - 1;
      for (unsigned short iDim = 0; iDim < nDim; ++iDim)
        mesh.coord[iPoint * nDim + iDim] = fixedCoord[(ref - 1) * nDim + iDim];
    }
  }
  mesh.metric.assign(metric.begin(), metric.end());
  mesh.elem.resize(elem.size());
  for (auto i = 0ul; i < elem.size(); ++i) {
    if (elem[i] < 1 || elem[i] > np) SU2_MPI::Error("MMG returned an invalid element.", CURRENT_FUNCTION);
    mesh.elem[i] = elem[i] - 1;
  }
  mesh.elemRef.assign(elemRef.begin(), elemRef.end());

  /*--- Boundary elements by marker reference; unknown references mean that MMG made a boundary without marker. ---*/
  mesh.markers = markerInfo;
  unsigned long nUnknown = 0;
  std::set<int> unknownRefs;
  for (int iBound = 0; iBound < nb; ++iBound) {
    const auto it = std::find_if(mesh.markers.begin(), mesh.markers.end(),
                                 [&](const CSimplexMesh::Marker& marker) { return marker.ref == boundRef[iBound]; });
    if (it == mesh.markers.end()) {
      ++nUnknown;
      unknownRefs.insert(boundRef[iBound]);
      continue;
    }
    for (unsigned short iNode = 0; iNode < nDim; ++iNode) {
      const auto iPoint = bound[iBound * nDim + iNode];
      if (iPoint < 1 || iPoint > np) SU2_MPI::Error("MMG returned an invalid boundary element.", CURRENT_FUNCTION);
      it->elem.push_back(iPoint - 1);
    }
  }
  if (nUnknown > 0) {
    string refs;
    for (const auto ref : unknownRefs) refs += " " + std::to_string(ref);
    SU2_MPI::Error("MMG returned " + std::to_string(nUnknown) +
                   " boundary elements whose reference is not an SU2 marker (references:" + refs + ").",
                   CURRENT_FUNCTION);
  }
  return mesh;
}

CSimplexMesh CMMGInterface::Adapt(const CSimplexMesh& mesh) {
  ValidateMesh(mesh, nullptr, "Mesh given to MMG");
  metricCheck = MetricCheck();
  CSimplexMesh floored;
  if (params.surface) {
    SetMesh(mesh);
  } else {
    /*--- Fixed boundary faces (MMG keeps the metric at their points, nosizreq): no boundary edge may be longer than
     *    1 in the metric of its points, else MMG fills the cells on it with nodes very close to it. ---*/
    floored = mesh;
    FloorFixedBoundaryMetric(floored);
    SetMesh(floored);
  }
  Remesh();
  auto adapted = GetMesh();
  ValidateMesh(adapted, &mesh, "Mesh returned by MMG");
  if (!params.surface) {
    CheckSameBoundary(adapted, mesh, "Mesh returned by MMG with ADAP_SURFACE= NO");

    /*--- MetricCheck: MMG's metric at the kept boundary points must not be coarser than the (floored) one given. ---*/
    const auto nMetric = CSimplexMesh::GetnMetric(nDim);
    if (adapted.metric.size() == adapted.GetnPoint() * nMetric) {
      std::vector<int> nMarkerOf(mesh.GetnPoint(), 0);
      for (const auto& marker : mesh.markers) {
        std::set<unsigned long> points(marker.elem.begin(), marker.elem.end());
        for (const auto iPoint : points) nMarkerOf[iPoint]++;
      }
      for (auto iPoint = 0ul; iPoint < adapted.GetnPoint(); ++iPoint) {
        const auto source = fixedSource[iPoint];
        if (source < 0) continue;
        ++metricCheck.nChecked;
        const auto ratio = CoarseningRatio(nDim, &floored.metric[source * nMetric], &adapted.metric[iPoint * nMetric]);
        metricCheck.worstRatio = std::min(metricCheck.worstRatio, ratio);
        if (ratio >= 1.0 - metricCheckTolerance) continue;
        ++metricCheck.nViolations;
        if (nMarkerOf[source] >= 2) ++metricCheck.nCornerViolations;
        metricCheck.points.push_back(source);
      }
      if (metricCheck.nViolations > 0) {
        cout << "WARNING: MMG made the metric coarser at " << metricCheck.nViolations << " of " << metricCheck.nChecked
             << " fixed boundary points (" << metricCheck.nCornerViolations << " corners; worst: smallest size ratio "
             << 1.0 / sqrt(std::max(metricCheck.worstRatio, 1e-300)) << "x coarser). MMG's own boundary metric "
             << "replaced the given one there (MMG issue #331: use an MMG with PR #332, or ADAP_BL_LOCAL_HMAX= YES); "
             << "the target sizes, e.g. a boundary-layer first height, are not met at those points:";
        const auto nShow = std::min<std::size_t>(metricCheck.points.size(), 10);
        for (std::size_t i = 0; i < nShow; ++i) {
          cout << " (";
          for (unsigned short iDim = 0; iDim < nDim; ++iDim)
            cout << (iDim ? ", " : "") << mesh.coord[metricCheck.points[i] * nDim + iDim];
          cout << ")";
        }
        cout << (metricCheck.points.size() > nShow ? " ..." : "") << endl;
      }
    }
  }
  return adapted;
}

void CMMGInterface::SaveMesh(const string& filename) const {
  if (mmg->mesh == nullptr) SU2_MPI::Error("No mesh loaded into MMG.", CURRENT_FUNCTION);
  const string meshFile = filename + ".mesh", solFile = filename + ".sol";
  int ok = 1;
  if (nDim == 2) {
    ok &= MMG2D_saveMesh(mmg->mesh, meshFile.c_str());
    ok &= MMG2D_saveSol(mmg->mesh, mmg->met, solFile.c_str());
  } else {
    ok &= MMG3D_saveMesh(mmg->mesh, meshFile.c_str());
    ok &= MMG3D_saveSol(mmg->mesh, mmg->met, solFile.c_str());
  }
  if (!ok) SU2_MPI::Error("Could not write " + meshFile + ".", CURRENT_FUNCTION);
}

#else

struct CMMGInterface::MMGData {};

CMMGInterface::CMMGInterface(const CConfig&) {
  SU2_MPI::Error("SU2 was built without MMG; reconfigure with -Denable-mmg=true for mesh adaptation.",
                 CURRENT_FUNCTION);
}

CMMGInterface::~CMMGInterface() = default;

void CMMGInterface::SetMesh(const CSimplexMesh&) { SU2_MPI::Error("SU2 was built without MMG.", CURRENT_FUNCTION); }

CMMGInterface::Status CMMGInterface::Remesh() {
  SU2_MPI::Error("SU2 was built without MMG.", CURRENT_FUNCTION);
  return Status::STRONGFAILURE;
}

CSimplexMesh CMMGInterface::GetMesh() const {
  SU2_MPI::Error("SU2 was built without MMG.", CURRENT_FUNCTION);
  return {};
}

CSimplexMesh CMMGInterface::Adapt(const CSimplexMesh&) {
  SU2_MPI::Error("SU2 was built without MMG.", CURRENT_FUNCTION);
  return {};
}

void CMMGInterface::SaveMesh(const string&) const { SU2_MPI::Error("SU2 was built without MMG.", CURRENT_FUNCTION); }

#endif

CMMGRemesher::CMMGRemesher() {
#ifndef HAVE_MMG
  SU2_MPI::Error("Mesh adaptation needs MMG: SU2 was built without it, reconfigure with -Denable-mmg=true.",
                 CURRENT_FUNCTION);
#endif
}

CRemeshResult CMMGRemesher::Remesh(const CConfig& config, const CGeometry& geometry, const su2activematrix& metric) {
  const int rank = SU2_MPI::GetRank(), size = SU2_MPI::GetSize();
  const auto startTime = SU2_MPI::Wtime();

  /*--- The whole mesh and metric on the master rank (all ranks take part in the gather). ---*/
  const auto mesh = CMMGInterface::ExtractMesh(config, geometry, metric);
  const auto extractTime = SU2_MPI::Wtime();

  if (rank == MASTER_NODE)
    cout << endl << "------------------------------ Remesh (MMG) -----------------------------" << endl;

  /*--- Serial MMG on the master rank; the other ranks wait for their part of its mesh. An error on the master rank
   *    (MMG failure, invalid mesh) stops all ranks (SU2_MPI::Error aborts the communicator). ---*/
  CSimplexMesh adapted;
  bool success = true;
  if (rank == MASTER_NODE) {
    CMMGInterface mmg(config);
    adapted = mmg.Adapt(mesh);
    success = mmg.GetStatus() == CMMGInterface::Status::SUCCESS;
  }
  const auto mmgTime = SU2_MPI::Wtime();

  /*--- Every rank gets the part of the new mesh it reads (CReaderSlices: its linear slice of the points, the elements
   *    touching it, the boundary elements on the master rank), sent by the master rank, which alone holds the
   *    complete mesh. ---*/
  CRemeshResult result;
  result.slices = CReaderSlices::FromComplete(adapted, MASTER_NODE);
  result.markers = result.slices.markersWithElements;
  result.status = CRemeshResult::Status::COMPLETE;
  const auto endTime = SU2_MPI::Wtime();

  if (rank == MASTER_NODE) {
    cout << "Remeshed " << mesh.GetnPoint() << " points, " << mesh.GetnElem() << " elements into "
         << adapted.GetnPoint() << " points, " << adapted.GetnElem() << " elements." << endl;
    cout << "MMG" << mesh.nDim << "D status " << (success ? "SUCCESS" : "LOWFAILURE") << ", " << mmgTime - extractTime
         << " s (with the validation of input and output), extraction " << extractTime - startTime << " s";
    if (size > 1) cout << " (gathered from " << size << " ranks), slices sent " << endTime - mmgTime << " s";
    cout << "." << endl;
    if (!config.GetAdap_Surface())
      cout << "Volume only (ADAP_SURFACE= NO): boundary points and faces kept (checked)." << endl;
  }
  return result;
}
