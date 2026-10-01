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

void CMMGInterface::CheckSupport(const CConfig& config, const CGeometry& geometry) {
  if (SU2_MPI::GetSize() > 1) {
    SU2_MPI::Error("Mesh adaptation with MMG runs on one MPI rank only for now.", CURRENT_FUNCTION);
  }
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
  if (geometry.GetnPoint() != geometry.GetnPointDomain()) {
    SU2_MPI::Error("Mesh adaptation does not support halo points (MPI or periodic).", CURRENT_FUNCTION);
  }
  for (unsigned short iMarker = 0; iMarker < geometry.GetnMarker(); ++iMarker) {
    const auto kind = config.GetMarker_All_KindBC(iMarker);
    const auto tag = config.GetMarker_All_TagBound(iMarker);
    switch (kind) {
      case SEND_RECEIVE:
        SU2_MPI::Error("Mesh adaptation does not support MPI send/receive markers.", CURRENT_FUNCTION);
        break;
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
  CSimplexMesh mesh;
  const auto nDim = geometry.GetnDim();
  if (nDim != 2 && nDim != 3) SU2_MPI::Error("Mesh adaptation needs a 2D or 3D mesh.", CURRENT_FUNCTION);
  mesh.nDim = nDim;

  const auto nPoint = geometry.GetnPoint();
  const auto nElem = geometry.GetnElem();
  const auto nMetric = CSimplexMesh::GetnMetric(nDim);
  if (nPoint != geometry.GetnPointDomain()) {
    SU2_MPI::Error("Mesh adaptation does not support halo points (MPI or periodic).", CURRENT_FUNCTION);
  }
  if (nPoint >= static_cast<unsigned long>(INT_MAX) || nElem >= static_cast<unsigned long>(INT_MAX)) {
    SU2_MPI::Error("The mesh is too large for MMG (32-bit indices).", CURRENT_FUNCTION);
  }
  if (metric.rows() < nPoint || metric.cols() != nMetric) {
    SU2_MPI::Error("The adaptation metric is not available (COMPUTE_METRIC = YES is required).", CURRENT_FUNCTION);
  }

  /*--- Points and metric. ---*/
  mesh.coord.resize(nPoint * nDim);
  mesh.metric.resize(nPoint * nMetric);
  for (auto iPoint = 0ul; iPoint < nPoint; ++iPoint) {
    for (unsigned short iDim = 0; iDim < nDim; ++iDim)
      mesh.coord[iPoint * nDim + iDim] = SU2_TYPE::GetValue(geometry.nodes->GetCoord(iPoint, iDim));
    for (unsigned short iMet = 0; iMet < nMetric; ++iMet)
      mesh.metric[iPoint * nMetric + iMet] = SU2_TYPE::GetValue(metric(iPoint, iMet));
  }
  for (auto iPoint = 0ul; iPoint < nPoint; ++iPoint) {
    if (!IsFinitePositiveDefinite(nDim, &mesh.metric[iPoint * nMetric])) {
      SU2_MPI::Error("The adaptation metric is not finite and positive definite (or numerically singular) at " +
                     PointInfo(nDim, mesh.coord, iPoint) + ".", CURRENT_FUNCTION);
    }
  }

  /*--- Volume elements, triangles or tetrahedra only, positively oriented. ---*/
  const unsigned short nNode = nDim + 1;
  const unsigned short simplex = (nDim == 2) ? TRIANGLE : TETRAHEDRON;
  mesh.elem.resize(nElem * nNode);
  mesh.elemRef.assign(nElem, 0);
  unsigned long nFlip = 0;
  for (auto iElem = 0ul; iElem < nElem; ++iElem) {
    const auto* element = geometry.elem[iElem];
    if (element->GetVTK_Type() != simplex) {
      SU2_MPI::Error(string("Mesh adaptation supports only ") + (nDim == 2 ? "triangles" : "tetrahedra") +
                     " (element " + std::to_string(iElem) + " has VTK type " +
                     std::to_string(element->GetVTK_Type()) + ").", CURRENT_FUNCTION);
    }
    auto* nodes = &mesh.elem[iElem * nNode];
    for (unsigned short iNode = 0; iNode < nNode; ++iNode) nodes[iNode] = element->GetNode(iNode);

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

  /*--- Boundary elements, grouped by physical marker. The reference does not depend on the partition. ---*/
  const unsigned short boundType = (nDim == 2) ? LINE : TRIANGLE;
  for (unsigned short iMarker = 0; iMarker < geometry.GetnMarker(); ++iMarker) {
    if (config.GetMarker_All_KindBC(iMarker) == SEND_RECEIVE) {
      SU2_MPI::Error("Mesh adaptation does not support MPI send/receive markers.", CURRENT_FUNCTION);
    }
    CSimplexMesh::Marker marker;
    marker.name = config.GetMarker_All_TagBound(iMarker);
    marker.ref = GetMarkerReference(config, marker.name);
    const auto nElemBound = geometry.GetnElem_Bound(iMarker);
    marker.elem.resize(nElemBound * nDim);
    for (auto iElem = 0ul; iElem < nElemBound; ++iElem) {
      const auto* element = geometry.bound[iMarker][iElem];
      if (element->GetVTK_Type() != boundType) {
        SU2_MPI::Error(string("Mesh adaptation supports only ") + (nDim == 2 ? "line" : "triangle") +
                       " boundary elements (marker " + marker.name + ").", CURRENT_FUNCTION);
      }
      for (unsigned short iNode = 0; iNode < nDim; ++iNode) marker.elem[iElem * nDim + iNode] = element->GetNode(iNode);
    }
    mesh.markers.push_back(std::move(marker));
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

  int ok = 1;
  if (nDim == 2) {
    MMG2D_Init_mesh(MMG5_ARG_start, MMG5_ARG_ppMesh, &mmg->mesh, MMG5_ARG_ppMet, &mmg->met, MMG5_ARG_end);
    ok &= MMG2D_Set_iparameter(mmg->mesh, mmg->met, MMG2D_IPARAM_verbose, params.verbosity);
    ok &= MMG2D_Set_meshSize(mmg->mesh, nPoint, nElem, 0, nBound);
    ok &= MMG2D_Set_vertices(mmg->mesh, coord.data(), pointRef.data());
    ok &= MMG2D_Set_triangles(mmg->mesh, elem.data(), elemRef.data());
    if (nBound > 0) ok &= MMG2D_Set_edges(mmg->mesh, bound.data(), boundRef.data());
    for (const auto k : corners) {
      ok &= MMG2D_Set_corner(mmg->mesh, k);
      ok &= MMG2D_Set_requiredVertex(mmg->mesh, k);
    }
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
    for (const auto k : corners) {
      ok &= MMG3D_Set_corner(mmg->mesh, k);
      ok &= MMG3D_Set_requiredVertex(mmg->mesh, k);
    }
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
    if (ok) ier = MMG2D_mmg2dlib(mmg->mesh, mmg->met);
  } else {
    ok &= MMG3D_Set_iparameter(mmg->mesh, mmg->met, MMG3D_IPARAM_verbose, params.verbosity);
    ok &= MMG3D_Set_iparameter(mmg->mesh, mmg->met, MMG3D_IPARAM_angle, 1);
    ok &= MMG3D_Set_dparameter(mmg->mesh, mmg->met, MMG3D_DPARAM_angleDetection, params.angle);
    ok &= MMG3D_Set_dparameter(mmg->mesh, mmg->met, MMG3D_DPARAM_hmin, params.hmin);
    ok &= MMG3D_Set_dparameter(mmg->mesh, mmg->met, MMG3D_DPARAM_hmax, params.hmax);
    ok &= MMG3D_Set_dparameter(mmg->mesh, mmg->met, MMG3D_DPARAM_hgrad, params.hgrad);
    ok &= MMG3D_Set_dparameter(mmg->mesh, mmg->met, MMG3D_DPARAM_hausd, params.hausd);
    if (ok) ier = MMG3D_mmg3dlib(mmg->mesh, mmg->met);
  }
  if (!ok) SU2_MPI::Error("Could not set the MMG parameters.", CURRENT_FUNCTION);

  switch (ier) {
    case MMG5_SUCCESS:
      return Status::SUCCESS;
    case MMG5_LOWFAILURE:
      cout << "WARNING: MMG could not fully adapt the mesh (MMG5_LOWFAILURE). The mesh it returned is "
              "accepted only if it passes validation, it may not follow the metric." << endl;
      return Status::LOWFAILURE;
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
  SetMesh(mesh);
  Remesh();
  auto adapted = GetMesh();
  ValidateMesh(adapted, &mesh, "Mesh returned by MMG");
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

CSimplexMesh CMMGRemesher::Remesh(const CConfig& config, const CGeometry& geometry, const su2activematrix& metric) {
  const auto mesh = CMMGInterface::ExtractMesh(config, geometry, metric);

  if (SU2_MPI::GetRank() == MASTER_NODE)
    cout << endl << "------------------------------ Remesh (MMG) -----------------------------" << endl;

  CMMGInterface mmg(config);
  auto adapted = mmg.Adapt(mesh);

  if (SU2_MPI::GetRank() == MASTER_NODE) {
    cout << "Remeshed " << mesh.GetnPoint() << " points, " << mesh.GetnElem() << " elements into "
         << adapted.GetnPoint() << " points, " << adapted.GetnElem() << " elements." << endl;
  }
  return adapted;
}

