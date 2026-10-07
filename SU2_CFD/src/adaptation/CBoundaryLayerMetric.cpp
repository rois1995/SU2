/*!
 * \file CBoundaryLayerMetric.cpp
 * \brief Boundary-layer metric of wall markers for mesh adaptation (wall-normal growth, tangential wall sizes).
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

#include "../../include/adaptation/CBoundaryLayerMetric.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <map>
#include <numeric>

#include "../../../Common/include/CConfig.hpp"
#include "../../../Common/include/parallelization/CPassiveComm.hpp"
#include "../../../Common/include/adaptation/CNativeImport2D.hpp"
#include "../../../Common/include/geometry/CGeometry.hpp"
#include "../../../Common/include/linear_algebra/blas_structure.hpp"
#include "../../include/solvers/CSolver.hpp"

namespace {

using Vec3 = std::array<su2double, 3>;
using Mat3 = su2double[3][3];

su2double Dot(unsigned short nDim, const su2double* a, const su2double* b) {
  su2double s = 0.0;
  for (unsigned short i = 0; i < nDim; ++i) s += a[i] * b[i];
  return s;
}

Vec3 Cross(const su2double* a, const su2double* b) {
  return {a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]};
}

/*--- B = f(A) for a symmetric A (eigenvalues mapped, eigenvectors kept), symmetrized. ---*/
template <class F>
void Spectral(unsigned short n, const Mat3& A, Mat3& B, F f) {
  su2double vec[3][3], val[3], work[3];
  CBlasStructure::EigenDecomposition(A, vec, val, n, work);
  for (unsigned short i = 0; i < n; ++i) val[i] = f(val[i]);
  su2double C[3][3];
  CBlasStructure::EigenRecomposition(C, vec, val, n);
  for (unsigned short i = 0; i < n; ++i)
    for (unsigned short j = 0; j < n; ++j) B[i][j] = 0.5 * (C[i][j] + C[j][i]);
}

/*--- Rotation that takes the unsigned axis "source" to "target" by the shortest rotation (sign of the source chosen
 *    nearer to the target): R = I + [v]x + [v]x^2 / (1 + c), v = a x target, c = a . target. ---*/
void RotateNormal(const su2double* source, const su2double* target, Mat3& R) {
  const su2double sign = (Dot(3, source, target) < 0.0) ? -1.0 : 1.0;
  const su2double a[3] = {sign * source[0], sign * source[1], sign * source[2]};
  const auto v = Cross(a, target);
  const su2double c = Dot(3, a, target);
  const su2double K[3][3] = {{0.0, -v[2], v[1]}, {v[2], 0.0, -v[0]}, {-v[1], v[0], 0.0}};
  for (int i = 0; i < 3; ++i) {
    for (int j = 0; j < 3; ++j) {
      su2double K2 = 0.0;
      for (int k = 0; k < 3; ++k) K2 += K[i][k] * K[k][j];
      R[i][j] = (i == j ? 1.0 : 0.0) + K[i][j] + K2 / (1.0 + c);
    }
  }
}

/*--- B = R A R^T. ---*/
void Rotate(const Mat3& R, const Mat3& A, Mat3& B) {
  su2double T[3][3];
  for (int i = 0; i < 3; ++i)
    for (int j = 0; j < 3; ++j) {
      T[i][j] = 0.0;
      for (int k = 0; k < 3; ++k) T[i][j] += R[i][k] * A[k][j];
    }
  for (int i = 0; i < 3; ++i)
    for (int j = 0; j < 3; ++j) {
      B[i][j] = 0.0;
      for (int k = 0; k < 3; ++k) B[i][j] += T[i][k] * R[j][k];
    }
}

/*--- Weights of the closest point of the segment [a,b] to p (clipped parameter along the segment). ---*/
su2double SegmentParameter(unsigned short nDim, const su2double* a, const su2double* b, const su2double* p) {
  su2double length2 = 0.0, projection = 0.0;
  for (unsigned short iDim = 0; iDim < nDim; ++iDim) {
    length2 += pow(b[iDim] - a[iDim], 2);
    projection += (p[iDim] - a[iDim]) * (b[iDim] - a[iDim]);
  }
  const su2double t = projection / length2;
  return min(max(t, su2double(0.0)), su2double(1.0));
}

/*--- Weights of the closest point of the triangle (a,b,c) to p in 3D, by the Voronoi regions of the triangle (Ericson,
 *    Real-Time Collision Detection, 5.1.5), in [0,1]. ---*/
void ClosestPointTriangle(const su2double* a, const su2double* b, const su2double* c, const su2double* p,
                          su2double* weight) {
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
  const su2double d1 = Dot(3, ab, ap), d2 = Dot(3, ac, ap);
  if (d1 <= 0.0 && d2 <= 0.0) return set(1.0, 0.0, 0.0);
  const su2double d3 = Dot(3, ab, bp), d4 = Dot(3, ac, bp);
  if (d3 >= 0.0 && d4 <= d3) return set(0.0, 1.0, 0.0);
  const su2double vc = d1 * d4 - d3 * d2;
  if (vc <= 0.0 && d1 >= 0.0 && d3 <= 0.0) {
    const su2double v = d1 / (d1 - d3);
    return set(1.0 - v, v, 0.0);
  }
  const su2double d5 = Dot(3, ab, cp), d6 = Dot(3, ac, cp);
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
  const su2double wa = max(su2double(0.0), va), wb = max(su2double(0.0), vb), wc = max(su2double(0.0), vc);
  const su2double sum = wa + wb + wc;
  set(wa / sum, wb / sum, wc / sum);
}

/*--- Smoothstep weight of the outer fade: 1 up to "full", 0 from full + width on. ---*/
su2double FadeWeight(su2double distance, su2double full, su2double width) {
  if (width == 0.0) return (distance > full) ? 0.0 : 1.0;
  const su2double s = min(max((distance - full) / width, su2double(0.0)), su2double(1.0));
  return 1.0 - s * s * (3.0 - 2.0 * s);
}

}  // namespace

/*--- Search structures and per-vertex data of one wall. ---*/
struct CBoundaryLayerMetric::WallData {
  su2double h0 = 0.0, growth = 1.0, full = 0.0, width = 0.0, thickness = 0.0;
  su2double low[3] = {0.0}, high[3] = {0.0}; /*!< \brief Bounding box of the wall grown by the thickness. */
  su2double tolerance = 0.0;                 /*!< \brief Distance below which a point is on the wall (3D). */
  std::vector<unsigned long> elem;           /*!< \brief Segments (2D) or triangles (3D) of the ADT. */
  std::unique_ptr<CADTElemClass> adt;

  /*--- 2D ---*/
  std::vector<su2double> referenceSize;
  std::vector<su2double> length;                     /*!< \brief Length of each segment. */
  std::vector<std::vector<unsigned long>> segments;  /*!< \brief Segments of each vertex. */
  std::vector<Vec3> vertexNormal;                    /*!< \brief Normal of each vertex (2D bisector, 3D angle-weighted). */
  std::vector<unsigned long> corner;                 /*!< \brief Vertices where the wall turns. */
  std::vector<su2double> turn;                       /*!< \brief cos of half the angle between the two edges. */
  su2double cell = 0.0;                              /*!< \brief Cell size of the corner grid. */
  std::map<std::pair<long, long>, std::vector<unsigned long>> grid; /*!< \brief Corners (positions) per grid cell. */

  /*--- 3D ---*/
  std::vector<Vec3> faceNormal;            /*!< \brief Unit normal of each triangle. */
  std::vector<std::array<Vec3, 3>> edgeNormal; /*!< \brief Normal of the edge opposite each vertex of a triangle. */
  std::vector<CBoundaryLayerMetric::Tensor> vertexLog; /*!< \brief Logarithm of the size tensor of each vertex. */
};

CBoundaryLayerMetric::~CBoundaryLayerMetric() = default;

CBoundaryLayerMetric::CBoundaryLayerMetric(unsigned short nDim, std::vector<Wall> walls_, su2double cornerAngle)
    : nDim(nDim), cornerAngle(cornerAngle), walls(std::move(walls_)) {
  Build();
}

CBoundaryLayerMetric::CBoundaryLayerMetric(const CGeometry& geometry, const CConfig& config)
    : nDim(geometry.GetnDim()), cornerAngle(config.GetAdap_Angle()) {
  const int nRank = SU2_MPI::GetSize();
  struct Box {
    passivedouble low[3], high[3];
  };
  const bool restricted = config.GetKind_Adap_Remesher() == ADAP_REMESHER::NATIVE_CAVITY;
  std::vector<Box> localBoxes;
  if (restricted && geometry.GetnPointDomain()) {
    /*--- A bounded set of occupied spatial boxes avoids the empty space inside a whole-rank box.
     *    Every owned query is covered, including ranks with no local wall. The padding below includes
     *    incident-face support, so filtering is conservative rather than a nearest-wall approximation. ---*/
    std::vector<unsigned long> points(geometry.GetnPointDomain());
    std::iota(points.begin(), points.end(), 0);
    struct Range { size_t begin, end; Box box; };
    auto bounds = [&](size_t begin, size_t end) {
      Box box{};
      for (unsigned short i = 0; i < nDim; ++i) {
        box.low[i] = std::numeric_limits<passivedouble>::max();
        box.high[i] = std::numeric_limits<passivedouble>::lowest();
        for (size_t k = begin; k < end; ++k) {
          const auto x = SU2_TYPE::GetValue(geometry.nodes->GetCoord(points[k], i));
          box.low[i] = std::min(box.low[i], x);
          box.high[i] = std::max(box.high[i], x);
        }
      }
      return Range{begin, end, box};
    };
    std::vector<Range> ranges{bounds(0, points.size())};
    // ponytail: at most 64 boxes per rank; an indexed distributed query is needed at very large rank counts.
    while (ranges.size() < (nRank > 1 ? 64u : 1u)) {
      auto it = std::max_element(ranges.begin(), ranges.end(), [](const Range& a, const Range& b) {
        return a.end - a.begin < b.end - b.begin;
      });
      if (it->end - it->begin < 128) break;
      const auto old = *it;
      unsigned short axis = 0;
      for (unsigned short i = 1; i < nDim; ++i)
        if (old.box.high[i] - old.box.low[i] > old.box.high[axis] - old.box.low[axis]) axis = i;
      const auto middle = old.begin + (old.end - old.begin) / 2;
      std::nth_element(points.begin() + old.begin, points.begin() + middle, points.begin() + old.end,
                       [&](unsigned long a, unsigned long b) {
                         const auto x = SU2_TYPE::GetValue(geometry.nodes->GetCoord(a, axis));
                         const auto y = SU2_TYPE::GetValue(geometry.nodes->GetCoord(b, axis));
                         return x < y || (x == y && geometry.nodes->GetGlobalIndex(a) < geometry.nodes->GetGlobalIndex(b));
                       });
      *it = bounds(old.begin, middle);
      ranges.push_back(bounds(middle, old.end));
    }
    for (const auto& range : ranges) localBoxes.push_back(range.box);
  }
  std::vector<size_t> boxCounts;
  const auto boxes = restricted ? CPassiveComm::Allgatherv(localBoxes, &boxCounts) : std::vector<Box>{};
  std::vector<size_t> boxOffsets(nRank + 1);
  if (restricted)
    for (int peer = 0; peer < nRank; ++peer) boxOffsets[peer + 1] = boxOffsets[peer] + boxCounts[peer];

  for (unsigned short iBL = 0; iBL < config.GetnAdap_BL(); ++iBL) {
    Wall wall;
    wall.layer = config.GetAdap_BL(iBL);

    /*--- Faces of the marker on this rank: number of nodes, global indices and coordinates of the nodes; 3D faces
     *    oriented with their normal away from their volume element. ---*/
    std::vector<unsigned long> localIds;
    std::vector<su2double> localCoord;
    for (unsigned short iMarker = 0; iMarker < geometry.GetnMarker(); ++iMarker) {
      if (config.GetMarker_All_TagBound(iMarker) != wall.layer.marker) continue;
      if (!config.GetSolid_Wall(iMarker)) {
        SU2_MPI::Error("ADAP_BL_MARKER: " + wall.layer.marker + " is not a wall marker.", CURRENT_FUNCTION);
      }
      for (auto iElem = 0ul; iElem < geometry.GetnElem_Bound(iMarker); ++iElem) {
        const auto* face = geometry.bound[iMarker][iElem];
        const auto type = face->GetVTK_Type();
        if (type == VERTEX) continue;
        const bool valid = (nDim == 2) ? (type == LINE) : (type == TRIANGLE || type == QUADRILATERAL);
        if (!valid)
          SU2_MPI::Error("The boundary-layer metric needs wall lines (2D) or triangles/quadrilaterals (3D).",
                         CURRENT_FUNCTION);
        const unsigned short nNode = face->GetnNodes();
        unsigned long node[4];
        for (unsigned short iNode = 0; iNode < nNode; ++iNode) node[iNode] = face->GetNode(iNode);

        if (nDim == 3) {
          const auto* elem = geometry.elem[face->GetDomainElement()];
          su2double x[4][3] = {{0.0}}, faceCenter[3] = {0.0}, elemCenter[3] = {0.0};
          for (unsigned short iNode = 0; iNode < nNode; ++iNode)
            for (unsigned short iDim = 0; iDim < 3; ++iDim) {
              x[iNode][iDim] = geometry.nodes->GetCoord(node[iNode], iDim);
              faceCenter[iDim] += x[iNode][iDim] / nNode;
            }
          for (unsigned short iNode = 0; iNode < elem->GetnNodes(); ++iNode)
            for (unsigned short iDim = 0; iDim < 3; ++iDim)
              elemCenter[iDim] += geometry.nodes->GetCoord(elem->GetNode(iNode), iDim) / elem->GetnNodes();
          su2double a[3], b[3], c[3];
          for (unsigned short iDim = 0; iDim < 3; ++iDim) {
            a[iDim] = x[1][iDim] - x[0][iDim];
            b[iDim] = x[2][iDim] - x[0][iDim];
            c[iDim] = x[nNode - 1][iDim] - x[0][iDim];
          }
          auto normal = Cross(a, b);
          if (nNode == 4) {
            const auto second = Cross(b, c);
            for (unsigned short iDim = 0; iDim < 3; ++iDim) normal[iDim] += second[iDim];
          }
          su2double outward[3];
          for (unsigned short iDim = 0; iDim < 3; ++iDim) outward[iDim] = faceCenter[iDim] - elemCenter[iDim];
          if (Dot(3, normal.data(), outward) < 0.0) std::reverse(node + 1, node + nNode);
        }
        localIds.push_back(nNode);
        for (unsigned short iNode = 0; iNode < nNode; ++iNode) {
          localIds.push_back(geometry.nodes->GetGlobalIndex(node[iNode]));
          for (unsigned short iDim = 0; iDim < nDim; ++iDim)
            localCoord.push_back(geometry.nodes->GetCoord(node[iNode], iDim));
        }
      }
    }

    unsigned long globalFaces = 0;
    std::vector<unsigned long> allIds;
    std::vector<su2double> allCoord;
    if (restricted) {
      unsigned long localFaces = 0;
      passivedouble localDiameter = 0, diameter = 0;
      size_t coordOffset = 0;
      for (size_t idOffset = 0; idOffset < localIds.size();) {
        const auto count = localIds[idOffset++];
        ++localFaces;
        for (size_t a = 0; a < count; ++a)
          for (size_t b = a + 1; b < count; ++b) {
            passivedouble squared = 0;
            for (unsigned short i = 0; i < nDim; ++i) {
              const auto delta =
                  SU2_TYPE::GetValue(localCoord[coordOffset + a * nDim + i] - localCoord[coordOffset + b * nDim + i]);
              squared += delta * delta;
            }
            localDiameter = std::max(localDiameter, sqrt(squared));
          }
        idOffset += count;
        coordOffset += count * nDim;
      }
      CPassiveComm::Allreduce(&localFaces, &globalFaces, 1, CPassiveComm::Op::SUM);
      CPassiveComm::Allreduce(&localDiameter, &diameter, 1, CPassiveComm::Op::MAX);
      if (!globalFaces)
        SU2_MPI::Error("ADAP_BL_MARKER: the marker " + wall.layer.marker + " has no faces.", CURRENT_FUNCTION);
      /*--- Two face diameters beyond the BL band include all incident faces of vertices used by a closest face,
       *    including 3D angle-weighted normals/tensors. This keeps active BL geometry local; the immutable native
       *    reference remains replicated by the native backend's existing contract. ---*/
      const auto padding = SU2_TYPE::GetValue(wall.layer.thickness) + 2 * diameter;
      std::vector<unsigned long> sendIds;
      std::vector<passivedouble> sendCoord;
      std::vector<size_t> idCounts(nRank), coordCounts(nRank), receivedCounts;
      for (int peer = 0; peer < nRank; ++peer) {
        const auto beforeId = sendIds.size(), beforeCoord = sendCoord.size();
        coordOffset = 0;
        for (size_t idOffset = 0; idOffset < localIds.size();) {
          const auto count = localIds[idOffset];
          Box face{};
          for (unsigned short i = 0; i < nDim; ++i) {
            face.low[i] = std::numeric_limits<passivedouble>::max();
            face.high[i] = std::numeric_limits<passivedouble>::lowest();
            for (size_t node = 0; node < count; ++node) {
              const auto x = SU2_TYPE::GetValue(localCoord[coordOffset + node * nDim + i]);
              face.low[i] = std::min(face.low[i], x);
              face.high[i] = std::max(face.high[i], x);
            }
          }
          bool intersects = false;
          for (size_t k = boxOffsets[peer]; k < boxOffsets[peer + 1] && !intersects; ++k) {
            bool overlap = true;
            for (unsigned short i = 0; i < nDim; ++i) {
              const auto roundoff = 128 * std::numeric_limits<passivedouble>::epsilon() *
                                    std::max({fabs(face.low[i]), fabs(face.high[i]), padding});
              overlap &= face.low[i] <= boxes[k].high[i] + padding + roundoff &&
                         face.high[i] >= boxes[k].low[i] - padding - roundoff;
            }
            intersects = overlap;
          }
          if (intersects) {
            sendIds.insert(sendIds.end(), localIds.begin() + idOffset, localIds.begin() + idOffset + count + 1);
            for (size_t k = 0; k < count * nDim; ++k)
              sendCoord.push_back(SU2_TYPE::GetValue(localCoord[coordOffset + k]));
          }
          idOffset += count + 1;
          coordOffset += count * nDim;
        }
        idCounts[peer] = sendIds.size() - beforeId;
        coordCounts[peer] = sendCoord.size() - beforeCoord;
      }
      allIds = CPassiveComm::Alltoallv(sendIds, idCounts, receivedCounts);
      const auto passiveCoord = CPassiveComm::Alltoallv(sendCoord, coordCounts, receivedCounts);
      allCoord.assign(passiveCoord.begin(), passiveCoord.end());
    } else {
      /*--- Gather the faces of all ranks. ---*/
      int nIdLocal = localIds.size(), nCoordLocal = localCoord.size();
      std::vector<int> nId(nRank), nCoord(nRank), dispId(nRank, 0), dispCoord(nRank, 0);
      SU2_MPI::Allgather(&nIdLocal, 1, MPI_INT, nId.data(), 1, MPI_INT, SU2_MPI::GetComm());
      SU2_MPI::Allgather(&nCoordLocal, 1, MPI_INT, nCoord.data(), 1, MPI_INT, SU2_MPI::GetComm());
      for (int iRank = 1; iRank < nRank; ++iRank) {
        dispId[iRank] = dispId[iRank - 1] + nId[iRank - 1];
        dispCoord[iRank] = dispCoord[iRank - 1] + nCoord[iRank - 1];
      }
      allIds.resize(dispId.back() + nId.back());
      allCoord.resize(dispCoord.back() + nCoord.back());
      SU2_MPI::Allgatherv(localIds.data(), nIdLocal, MPI_UNSIGNED_LONG, allIds.data(), nId.data(), dispId.data(),
                          MPI_UNSIGNED_LONG, SU2_MPI::GetComm());
      SU2_MPI::Allgatherv(localCoord.data(), nCoordLocal, MPI_DOUBLE, allCoord.data(), nCoord.data(), dispCoord.data(),
                          MPI_DOUBLE, SU2_MPI::GetComm());
    }
    /*--- Points numbered in the order of their global index, each face once. ---*/
    std::map<unsigned long, unsigned long> pointIndex;
    std::map<unsigned long, std::vector<su2double>> pointCoord;
    std::vector<std::vector<unsigned long>> faces;
    std::map<std::vector<unsigned long>, bool> seen;
    for (size_t iId = 0, iCoord = 0; iId < allIds.size();) {
      const auto nNode = allIds[iId++];
      std::vector<unsigned long> ids(allIds.begin() + iId, allIds.begin() + iId + nNode);
      for (unsigned long iNode = 0; iNode < nNode; ++iNode) {
        pointCoord.emplace(ids[iNode], std::vector<su2double>(allCoord.begin() + iCoord + iNode * nDim,
                                                              allCoord.begin() + iCoord + (iNode + 1) * nDim));
      }
      iId += nNode;
      iCoord += nNode * nDim;
      auto key = ids;
      std::sort(key.begin(), key.end());
      if (seen.emplace(key, true).second) faces.push_back(ids);
    }
    if (restricted) {
      const unsigned long local = faces.size();
      unsigned long maximum = 0;
      CPassiveComm::Allreduce(&local, &maximum, 1, CPassiveComm::Op::MAX);
      if (SU2_MPI::GetRank() == MASTER_NODE)
        std::cout << "Native BL geometry " << wall.layer.marker << ": largest local subset " << maximum
                  << " of " << globalFaces << " supplied wall faces (before duplicate removal)." << std::endl;
    }
    for (const auto& entry : pointCoord) {
      const unsigned long index = pointIndex.size();
      pointIndex[entry.first] = index;
      wall.coord.insert(wall.coord.end(), entry.second.begin(), entry.second.end());
    }
    for (const auto& ids : faces) {
      unsigned long conn[4];
      for (size_t iNode = 0; iNode < ids.size(); ++iNode) conn[iNode] = pointIndex[ids[iNode]];
      if (nDim == 2) {
        wall.conn.insert(wall.conn.end(), conn, conn + 2);
      } else {
        if (ids.size() == 3) conn[3] = conn[2];
        wall.conn.insert(wall.conn.end(), conn, conn + 4);
      }
    }
    walls.push_back(std::move(wall));
  }
  Build();
}

void CBoundaryLayerMetric::Build() {
  if (nDim != 2 && nDim != 3) SU2_MPI::Error("The boundary-layer metric needs a 2D or 3D mesh.", CURRENT_FUNCTION);

  /*--- Walls in the order of their names (the order of the intersections). ---*/
  std::sort(walls.begin(), walls.end(), [](const Wall& a, const Wall& b) { return a.layer.marker < b.layer.marker; });

  for (const auto& wall : walls) {
    const auto& name = wall.layer.marker;
    const auto nPoint = wall.coord.size() / nDim;
    auto d = std::unique_ptr<WallData>(new WallData);
    if (wall.conn.empty()) { data.push_back(std::move(d)); continue; }
    d->h0 = wall.layer.firstHeight;
    d->growth = wall.layer.growth;
    d->thickness = wall.layer.thickness;
    if (!(d->h0 > 0.0) || !(d->growth >= 1.0) || !(d->thickness >= d->h0)) {
      SU2_MPI::Error("Wall " + name + ": the boundary-layer metric needs first height > 0, growth >= 1 and thickness "
                     ">= first height.", CURRENT_FUNCTION);
    }
    d->full = max(d->h0, 0.9 * d->thickness);
    d->width = d->thickness - d->full;

    /*--- Bounding box grown by the thickness (points outside cannot be within the thickness), and the distance below
     *    which a point is on the wall (round-off of the coordinates). ---*/
    su2double maxAbs = 0.0, extent = 0.0;
    for (unsigned short iDim = 0; iDim < nDim; ++iDim) {
      d->low[iDim] = std::numeric_limits<passivedouble>::max();
      d->high[iDim] = std::numeric_limits<passivedouble>::lowest();
    }
    for (auto iPoint = 0ul; iPoint < nPoint; ++iPoint)
      for (unsigned short iDim = 0; iDim < nDim; ++iDim) {
        const su2double x = wall.coord[iPoint * nDim + iDim];
        d->low[iDim] = min(d->low[iDim], x);
        d->high[iDim] = max(d->high[iDim], x);
        maxAbs = max(maxAbs, fabs(x));
      }
    for (unsigned short iDim = 0; iDim < nDim; ++iDim) extent = max(extent, d->high[iDim] - d->low[iDim]);
    const su2double eps = std::numeric_limits<passivedouble>::epsilon();
    const su2double padding = 64 * eps * max(maxAbs, d->thickness);
    for (unsigned short iDim = 0; iDim < nDim; ++iDim) {
      d->low[iDim] -= d->thickness + padding;
      d->high[iDim] += d->thickness + padding;
    }
    d->tolerance = 64 * eps * max(maxAbs, extent);

    auto x = [&](unsigned long iPoint) { return &wall.coord[iPoint * nDim]; };

    if (nDim == 2) {
      /*--- Segments, their lengths, and the rays (unit vectors along the incident edges) of each vertex. ---*/
      const auto nSeg = wall.conn.size() / 2;
      d->elem = wall.conn;
      d->length.resize(nSeg);
      d->segments.assign(nPoint, {});
      std::vector<std::vector<Vec3>> rays(nPoint);
      su2double maxLength = 0.0;
      for (auto iSeg = 0ul; iSeg < nSeg; ++iSeg) {
        const auto a = d->elem[2 * iSeg], b = d->elem[2 * iSeg + 1];
        const su2double v[2] = {x(b)[0] - x(a)[0], x(b)[1] - x(a)[1]};
        d->length[iSeg] = sqrt(v[0] * v[0] + v[1] * v[1]);
        if (!(d->length[iSeg] > 0.0)) SU2_MPI::Error("Wall " + name + " has an edge of zero length.", CURRENT_FUNCTION);
        maxLength = max(maxLength, d->length[iSeg]);
        rays[a].push_back({v[0] / d->length[iSeg], v[1] / d->length[iSeg], 0.0});
        rays[b].push_back({-v[0] / d->length[iSeg], -v[1] / d->length[iSeg], 0.0});
        d->segments[a].push_back(iSeg);
        d->segments[b].push_back(iSeg);
      }

      /*--- Vertex normal: bisector of the two rays where the wall turns, else normal of the ray. The vertex limits the
       *    tangential size around it if the wall turns there by more than cornerAngle: |r1 + r2| = 2 sin(turn / 2).
       *    ---*/
      const su2double cornerMagnitude = max(su2double(1e-10), 2.0 * sin(0.5 * cornerAngle * PI_NUMBER / 180.0));
      d->vertexNormal.resize(nPoint);
      for (auto iPoint = 0ul; iPoint < nPoint; ++iPoint) {
        const auto& r = rays[iPoint];
        if (r.size() > 2) SU2_MPI::Error("Wall " + name + " branches at a vertex.", CURRENT_FUNCTION);
        if (r.empty()) continue;
        su2double bisector[2] = {r[0][0], r[0][1]};
        if (r.size() == 2) {
          bisector[0] += r[1][0];
          bisector[1] += r[1][1];
        }
        const su2double magnitude = sqrt(bisector[0] * bisector[0] + bisector[1] * bisector[1]);
        if (r.size() == 2 && magnitude > 1e-10) {
          d->vertexNormal[iPoint] = {bisector[0] / magnitude, bisector[1] / magnitude, 0.0};
          if (magnitude > cornerMagnitude) {
            d->corner.push_back(iPoint);
            d->turn.push_back(0.5 * magnitude);
          }
        } else {
          d->vertexNormal[iPoint] = {-r[0][1], r[0][0], 0.0};
        }
      }

      /*--- Grid of the corners, cells of the largest edge length (the corner search radius is half an edge). ---*/
      d->cell = maxLength;
      for (size_t iCorner = 0; iCorner < d->corner.size(); ++iCorner) {
        const auto* c = x(d->corner[iCorner]);
        const std::pair<long, long> key(static_cast<long>(floor(SU2_TYPE::GetValue(c[0] / d->cell))),
                                        static_cast<long>(floor(SU2_TYPE::GetValue(c[1] / d->cell))));
        d->grid[key].push_back(iCorner);
      }
    } else {
      /*--- Faces: unit normal, size tensor (2/n sum of e e^T over the n edges, projected on the face, plus its
       *    largest eigenvalue in the normal direction so that its logarithm exists), angles at the vertices. ---*/
      const auto nFace = wall.conn.size() / 4;
      d->vertexNormal.assign(nPoint, {0.0, 0.0, 0.0});
      std::vector<Vec3> normal(nFace);
      std::vector<Tensor> covariance(nFace);
      std::vector<std::array<su2double, 4>> angle(nFace);
      std::map<std::pair<unsigned long, unsigned long>, std::vector<unsigned long>> edgeFaces;

      for (auto iFace = 0ul; iFace < nFace; ++iFace) {
        const auto* node = &wall.conn[4 * iFace];
        const unsigned short nNode = (node[3] == node[2]) ? 3 : 4;
        su2double e[4][3], len[4];
        for (unsigned short k = 0; k < nNode; ++k) {
          const auto* p = x(node[k]);
          const auto* q = x(node[(k + 1) % nNode]);
          for (unsigned short iDim = 0; iDim < 3; ++iDim) e[k][iDim] = q[iDim] - p[iDim];
          len[k] = sqrt(Dot(3, e[k], e[k]));
          if (!(len[k] > 0.0)) SU2_MPI::Error("Wall " + name + " has a degenerate face.", CURRENT_FUNCTION);
          auto key = std::make_pair(min(node[k], node[(k + 1) % nNode]), max(node[k], node[(k + 1) % nNode]));
          edgeFaces[key].push_back(iFace);
        }
        su2double a[3], b[3], c[3];
        for (unsigned short iDim = 0; iDim < 3; ++iDim) {
          a[iDim] = x(node[1])[iDim] - x(node[0])[iDim];
          b[iDim] = x(node[2])[iDim] - x(node[0])[iDim];
          c[iDim] = x(node[nNode - 1])[iDim] - x(node[0])[iDim];
        }
        normal[iFace] = Cross(a, b);
        if (nNode == 4) {
          const auto second = Cross(b, c);
          for (unsigned short iDim = 0; iDim < 3; ++iDim) normal[iFace][iDim] += second[iDim];
        }
        const su2double magnitude = sqrt(Dot(3, normal[iFace].data(), normal[iFace].data()));
        if (!(magnitude > 0.0)) SU2_MPI::Error("Wall " + name + " has a degenerate face.", CURRENT_FUNCTION);
        for (auto& v : normal[iFace]) v /= magnitude;
        const auto* n = normal[iFace].data();

        su2double C[3][3] = {{0.0}}, P[3][3], PC[3][3];
        for (unsigned short k = 0; k < nNode; ++k)
          for (int i = 0; i < 3; ++i)
            for (int j = 0; j < 3; ++j) C[i][j] += 2.0 / nNode * e[k][i] * e[k][j];
        for (int i = 0; i < 3; ++i)
          for (int j = 0; j < 3; ++j) P[i][j] = (i == j ? 1.0 : 0.0) - n[i] * n[j];
        for (int i = 0; i < 3; ++i)
          for (int j = 0; j < 3; ++j) {
            PC[i][j] = 0.0;
            for (int k = 0; k < 3; ++k) PC[i][j] += P[i][k] * C[k][j];
          }
        for (int i = 0; i < 3; ++i)
          for (int j = 0; j < 3; ++j) {
            C[i][j] = 0.0;
            for (int k = 0; k < 3; ++k) C[i][j] += PC[i][k] * P[k][j];
          }
        su2double vec[3][3], val[3], work[3];
        CBlasStructure::EigenDecomposition(C, vec, val, 3, work);
        const su2double largest = *std::max_element(val, val + 3);
        for (int i = 0; i < 3; ++i)
          for (int j = 0; j < 3; ++j) covariance[iFace].m[i][j] = C[i][j] + largest * n[i] * n[j];

        /*--- Angle at vertex k between the edges to k-1 and k+1. ---*/
        for (unsigned short k = 0; k < nNode; ++k) {
          const unsigned short km = (k + nNode - 1) % nNode;
          su2double cosine = 0.0;
          for (unsigned short iDim = 0; iDim < 3; ++iDim) cosine += -e[km][iDim] / len[km] * e[k][iDim] / len[k];
          angle[iFace][k] = acos(min(max(cosine, su2double(-1.0)), su2double(1.0)));
          for (unsigned short iDim = 0; iDim < 3; ++iDim) d->vertexNormal[node[k]][iDim] += angle[iFace][k] * n[iDim];
        }
      }
      for (const auto& entry : edgeFaces) {
        if (entry.second.size() > 2) SU2_MPI::Error("Wall " + name + " has a non-manifold edge.", CURRENT_FUNCTION);
      }
      for (auto& vn : d->vertexNormal) {
        const su2double magnitude = sqrt(Dot(3, vn.data(), vn.data()));
        if (!(magnitude > 1e-12)) SU2_MPI::Error("Wall " + name + " has an ambiguous vertex normal.", CURRENT_FUNCTION);
        for (auto& v : vn) v /= magnitude;
      }

      /*--- Size tensor of each vertex: angle-weighted mean of the logarithms of the face tensors, each rotated so that
       *    its face normal becomes the vertex normal. ---*/
      d->vertexLog.assign(nPoint, Tensor());
      std::vector<su2double> weightSum(nPoint, 0.0);
      for (auto iFace = 0ul; iFace < nFace; ++iFace) {
        const auto* node = &wall.conn[4 * iFace];
        const unsigned short nNode = (node[3] == node[2]) ? 3 : 4;
        for (unsigned short k = 0; k < nNode; ++k) {
          su2double R[3][3], rotated[3][3], logC[3][3];
          RotateNormal(normal[iFace].data(), d->vertexNormal[node[k]].data(), R);
          Rotate(R, covariance[iFace].m, rotated);
          Spectral(3, rotated, logC, [](su2double v) { return log(v); });
          for (int i = 0; i < 3; ++i)
            for (int j = 0; j < 3; ++j) d->vertexLog[node[k]].m[i][j] += angle[iFace][k] * logC[i][j];
          weightSum[node[k]] += angle[iFace][k];
        }
      }
      for (auto iPoint = 0ul; iPoint < nPoint; ++iPoint)
        for (int i = 0; i < 3; ++i)
          for (int j = 0; j < 3; ++j) d->vertexLog[iPoint].m[i][j] /= weightSum[iPoint];

      /*--- Triangles of the search (quadrilaterals split by the diagonal through their smallest point index), their
       *    unit normals, and the normals of their edges (sum of the normals of the faces of the edge). ---*/
      for (auto iFace = 0ul; iFace < nFace; ++iFace) {
        const auto* g = &wall.conn[4 * iFace];
        if (g[3] == g[2]) {
          d->elem.insert(d->elem.end(), g, g + 3);
        } else if (min(g[0], g[2]) <= min(g[1], g[3])) {
          const unsigned long t[6] = {g[0], g[1], g[2], g[0], g[2], g[3]};
          d->elem.insert(d->elem.end(), t, t + 6);
        } else {
          const unsigned long t[6] = {g[1], g[2], g[3], g[1], g[3], g[0]};
          d->elem.insert(d->elem.end(), t, t + 6);
        }
      }
      const auto nTria = d->elem.size() / 3;
      d->faceNormal.resize(nTria);
      d->edgeNormal.resize(nTria);
      for (auto iTria = 0ul; iTria < nTria; ++iTria) {
        const auto* t = &d->elem[3 * iTria];
        su2double a[3], b[3];
        for (unsigned short iDim = 0; iDim < 3; ++iDim) {
          a[iDim] = x(t[1])[iDim] - x(t[0])[iDim];
          b[iDim] = x(t[2])[iDim] - x(t[0])[iDim];
        }
        auto n = Cross(a, b);
        const su2double magnitude = sqrt(Dot(3, n.data(), n.data()));
        if (!(magnitude > 0.0)) SU2_MPI::Error("Wall " + name + " has a degenerate face.", CURRENT_FUNCTION);
        for (auto& v : n) v /= magnitude;
        d->faceNormal[iTria] = n;
        for (unsigned short k = 0; k < 3; ++k) {
          const auto p = t[(k + 1) % 3], q = t[(k + 2) % 3];
          const auto it = edgeFaces.find(std::make_pair(min(p, q), max(p, q)));
          Vec3 sum = {0.0, 0.0, 0.0};
          if (it == edgeFaces.end()) {
            sum = n;
          } else {
            for (const auto iFace : it->second)
              for (unsigned short iDim = 0; iDim < 3; ++iDim) sum[iDim] += normal[iFace][iDim];
          }
          const su2double length = sqrt(Dot(3, sum.data(), sum.data()));
          if (!(length > 1e-12)) SU2_MPI::Error("Wall " + name + " has an ambiguous edge normal.", CURRENT_FUNCTION);
          for (auto& v : sum) v /= length;
          d->edgeNormal[iTria][k] = sum;
        }
      }
    }

    /*--- Local ADT of the segments / triangles. ---*/
    const unsigned short nNode = nDim;
    const auto nItem = d->elem.size() / nNode;
    std::vector<su2double> coordCopy = wall.coord;
    std::vector<unsigned long> connCopy = d->elem;
    std::vector<unsigned short> types(nItem, (nDim == 2) ? LINE : TRIANGLE), markers(nItem, 0);
    std::vector<unsigned long> ids(nItem);
    std::iota(ids.begin(), ids.end(), 0ul);
    d->adt.reset(new CADTElemClass(nDim, coordCopy, connCopy, types, markers, ids, false));

    data.push_back(std::move(d));
  }
}

CBoundaryLayerMetric::Sample CBoundaryLayerMetric::Evaluate(unsigned short iWall, const su2double* X,
                                                            su2double coreEigenvalue) {
  const auto& wall = walls[iWall];
  auto& d = *data[iWall];
  Sample sample;
  if (!d.adt) return sample;
  auto x = [&](unsigned long iPoint) { return &wall.coord[iPoint * nDim]; };

  unsigned short markerID = 0;
  unsigned long id = 0;
  int rankID = 0;
  su2double adtDistance = 0.0;
  d.adt->DetermineNearestElement(X, adtDistance, markerID, id, rankID);

  su2double tangentSize[3][3] = {{0.0}};  // 3D: size tensor (squared sizes) on the tangent plane
  su2double ht = 0.0, tangent[3] = {0.0};  // 2D

  if (nDim == 2) {
    /*--- Exact distance to a segment, parameter of the closest point. ---*/
    auto segmentDistance = [&](unsigned long iSeg, su2double& along) {
      const auto* a = x(d.elem[2 * iSeg]);
      const auto* b = x(d.elem[2 * iSeg + 1]);
      along = SegmentParameter(2, a, b, X);
      const su2double r[2] = {X[0] - a[0] - along * (b[0] - a[0]), X[1] - a[1] - along * (b[1] - a[1])};
      return sqrt(r[0] * r[0] + r[1] * r[1]);
    };
    su2double along = 0.0;
    su2double best = segmentDistance(id, along);

    /*--- At a vertex the closest point is shared by its two segments: the segment whose midpoint is nearer is taken
     *    (the rule of the Python version in exact arithmetic, whose search visits the segments by the distance of their
     *    midpoints); the two distances are compared with a relative tolerance of 1e-12, so the choice does not depend
     *    on round-off. ---*/
    if (along == 0.0 || along == 1.0) {
      const auto vertex = d.elem[2 * id + (along == 1.0 ? 1 : 0)];
      auto midDistance = [&](unsigned long iSeg) {
        const auto* a = x(d.elem[2 * iSeg]);
        const auto* b = x(d.elem[2 * iSeg + 1]);
        return pow(X[0] - 0.5 * (a[0] + b[0]), 2) + pow(X[1] - 0.5 * (a[1] + b[1]), 2);
      };
      for (const auto other : d.segments[vertex]) {
        if (other == id) continue;
        su2double alongOther = 0.0;
        const su2double dist = segmentDistance(other, alongOther);
        const bool tie = fabs(dist - best) <= 1e-12 * best;
        if ((dist < best && !tie) || (tie && midDistance(other) < midDistance(id))) {
          id = other;
          best = min(best, dist);
          along = alongOther;
        }
      }
    }
    sample.distance = best;

    const auto* a = x(d.elem[2 * id]);
    const auto* b = x(d.elem[2 * id + 1]);
    const su2double length = d.length[id];
    const su2double t[2] = {(b[0] - a[0]) / length, (b[1] - a[1]) / length};
    su2double n[2] = {-t[1], t[0]};
    const su2double radial[2] = {X[0] - a[0] - along * (b[0] - a[0]), X[1] - a[1] - along * (b[1] - a[1])};
    const su2double radius = sqrt(radial[0] * radial[0] + radial[1] * radial[1]);
    if (along == 0.0 || along == 1.0) {
      if (radius > 1e-12 * length) {
        n[0] = radial[0] / radius;
        n[1] = radial[1] / radius;
      } else {
        const auto& vn = d.vertexNormal[d.elem[2 * id + (along > 0.5 ? 1 : 0)]];
        n[0] = vn[0];
        n[1] = vn[1];
      }
    }
    sample.normal[0] = n[0];
    sample.normal[1] = n[1];
    tangent[0] = -n[1];
    tangent[1] = n[0];

    /*--- Tangential size: length of the closest segment, limited near the vertices where the wall turns. ---*/
    ht = d.referenceSize.empty() ? length : d.referenceSize[id];
    /*--- Corner searches use the existing face radius: the reference chord already avoids crossing features. ---*/
    const su2double radiusSearch = 0.5 * length;
    const long ci = static_cast<long>(floor(SU2_TYPE::GetValue(X[0] / d.cell)));
    const long cj = static_cast<long>(floor(SU2_TYPE::GetValue(X[1] / d.cell)));
    su2double limited = ht;
    for (long i = ci - 1; i <= ci + 1; ++i) {
      for (long j = cj - 1; j <= cj + 1; ++j) {
        const auto it = d.grid.find(std::make_pair(i, j));
        if (it == d.grid.end()) continue;
        for (const auto iCorner : it->second) {
          const auto* c = x(d.corner[iCorner]);
          const su2double r = sqrt(pow(X[0] - c[0], 2) + pow(X[1] - c[1], 2));
          if (r <= radiusSearch) limited = min(limited, max(d.h0, 2.0 * r / d.turn[iCorner]));
        }
      }
    }
    ht = limited;
  } else {
    /*--- Closest point of the nearest triangle and its barycentric weights. ---*/
    const auto* t = &d.elem[3 * id];
    su2double w[3];
    ClosestPointTriangle(x(t[0]), x(t[1]), x(t[2]), X, w);
    su2double projected[3] = {0.0};
    for (unsigned short k = 0; k < 3; ++k)
      for (unsigned short iDim = 0; iDim < 3; ++iDim) projected[iDim] += w[k] * x(t[k])[iDim];
    su2double dist2 = 0.0;
    for (unsigned short iDim = 0; iDim < 3; ++iDim) dist2 += pow(X[iDim] - projected[iDim], 2);
    sample.distance = sqrt(dist2);

    /*--- Normal: of the face, of an edge, of a vertex, or towards the point if it is off the wall. ---*/
    Vec3 n = d.faceNormal[id];
    unsigned short nZero = 0, kMin = 0, kMax = 0;
    for (unsigned short k = 0; k < 3; ++k) {
      if (w[k] <= 1e-10) ++nZero;
      if (w[k] < w[kMin]) kMin = k;
      if (w[k] > w[kMax]) kMax = k;
    }
    if (nZero == 1) n = d.edgeNormal[id][kMin];
    if (nZero >= 2) n = d.vertexNormal[t[kMax]];
    if (sample.distance > d.tolerance) {
      for (unsigned short iDim = 0; iDim < 3; ++iDim) n[iDim] = (X[iDim] - projected[iDim]) / sample.distance;
    }
    for (unsigned short iDim = 0; iDim < 3; ++iDim) sample.normal[iDim] = n[iDim];

    /*--- Size tensor: barycentric mean of the logarithms of the vertex tensors, rotated to this normal; then its
     *    projection on the tangent plane. ---*/
    su2double logC[3][3] = {{0.0}};
    for (unsigned short k = 0; k < 3; ++k) {
      su2double R[3][3], rotated[3][3];
      RotateNormal(d.vertexNormal[t[k]].data(), n.data(), R);
      Rotate(R, d.vertexLog[t[k]].m, rotated);
      for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j) logC[i][j] += w[k] * rotated[i][j];
    }
    su2double C[3][3], P[3][3], PC[3][3];
    Spectral(3, logC, C, [](su2double v) { return exp(v); });
    for (int i = 0; i < 3; ++i)
      for (int j = 0; j < 3; ++j) P[i][j] = (i == j ? 1.0 : 0.0) - n[i] * n[j];
    for (int i = 0; i < 3; ++i)
      for (int j = 0; j < 3; ++j) {
        PC[i][j] = 0.0;
        for (int k = 0; k < 3; ++k) PC[i][j] += P[i][k] * C[k][j];
      }
    for (int i = 0; i < 3; ++i)
      for (int j = 0; j < 3; ++j) {
        tangentSize[i][j] = 0.0;
        for (int k = 0; k < 3; ++k) tangentSize[i][j] += PC[i][k] * P[k][j];
      }
  }

  sample.weight = FadeWeight(sample.distance, d.full, d.width);
  if (!(sample.weight > 0.0)) return sample;

  /*--- Wall-normal size of geometric rows: harmonic mean of the two rows around the point, at least h0. ---*/
  sample.hn = max(d.h0, 2.0 * (d.h0 + (d.growth - 1.0) * sample.distance) / (d.growth + 1.0));

  auto& M = sample.full.m;
  if (nDim == 2) {
    for (int i = 0; i < 2; ++i)
      for (int j = 0; j < 2; ++j)
        M[i][j] = tangent[i] * tangent[j] / (ht * ht) + sample.normal[i] * sample.normal[j] / pow(sample.hn, 2);
  } else {
    su2double S[3][3];
    for (int i = 0; i < 3; ++i)
      for (int j = 0; j < 3; ++j) S[i][j] = tangentSize[i][j] + pow(sample.hn, 2) * sample.normal[i] * sample.normal[j];
    Spectral(3, S, M, [](su2double v) { return 1.0 / v; });
  }

  /*--- Fade: eigenvalues interpolated in logarithm towards the isotropic core metric. ---*/
  if (sample.weight < 1.0) {
    const su2double w = sample.weight;
    Spectral(nDim, M, sample.metric.m, [&](su2double v) { return exp(w * log(v) + (1.0 - w) * log(coreEigenvalue)); });
  } else {
    sample.metric = sample.full;
  }
  return sample;
}

void CBoundaryLayerMetric::SetNativeReference(const SU2NativeBoundary2D::ReferenceState& reference,
                                              const CConfig& config) {
  if (nDim != 2 || !reference.original || !config.GetAdap_Surface()) return;
  unsigned long localConflict = 0, globalConflict = 0;
  for (size_t wall = 0; wall < walls.size(); ++wall) {
    auto& d = *data[wall];
    const auto& geometry = walls[wall];
    d.referenceSize.resize(d.length.size());
    for (size_t k = 0; k < d.length.size(); ++k) {
      using namespace SU2NativeBoundary2D;
      const auto a = d.elem[2 * k], b = d.elem[2 * k + 1];
      const Point first{SU2_TYPE::GetValue(geometry.coord[2 * a]), SU2_TYPE::GetValue(geometry.coord[2 * a + 1])};
      const Point last{SU2_TYPE::GetValue(geometry.coord[2 * b]), SU2_TYPE::GetValue(geometry.coord[2 * b + 1])};
      const auto found = reference.accepted_edges.find(CoordinateKey(first, last));
      if (found == reference.accepted_edges.end())
        SU2_MPI::Error("BL metric face has no accepted native reference association.", CURRENT_FUNCTION);
      const auto component = found->second;
      const auto center = reference.original->Parameter(component, (first + last) * 0.5);
      const auto minimum =
          std::max(SU2_TYPE::GetValue(config.GetAdap_Hmin()), 2 * SU2_TYPE::GetValue(geometry.layer.firstHeight));
      bool conflict = false;
      d.referenceSize[k] = reference.original->TangentialSize(
          component, center, minimum, std::max(minimum, SU2_TYPE::GetValue(config.GetAdap_Hmax())),
          SU2_TYPE::GetValue(config.GetAdap_Hausd()), &conflict);
      localConflict += conflict || minimum > config.GetAdap_Hmax();
    }
  }
  CPassiveComm::Allreduce(&localConflict, &globalConflict, 1, CPassiveComm::Op::SUM);
  if (SU2_MPI::GetRank() == MASTER_NODE && globalConflict)
    std::cout << "WARNING: Native reference chord tolerance conflicts with the tangential minimum at " << globalConflict
              << " local wall-face samples (shared samples may be counted on several ranks)." << std::endl;
}

std::vector<CBoundaryLayerMetric::PointSample> CBoundaryLayerMetric::SamplePoints(const std::vector<su2double>& coord) {
  std::vector<PointSample> result;
  for (unsigned short wall = 0; wall < walls.size(); ++wall) {
    const auto& d = *data[wall];
    if (!d.adt) continue;
    for (auto point = 0ul; point < coord.size() / nDim; ++point) {
      const auto* X = &coord[point * nDim];
      bool inside = true;
      for (unsigned short i = 0; i < nDim; ++i) inside = inside && X[i] >= d.low[i] && X[i] <= d.high[i];
      if (!inside) continue;
      const auto sample = Evaluate(wall, X, 1.0);  // Only full metric, normal and weight are cached.
      if (sample.weight > 0.0) result.push_back({point, wall, sample});
    }
  }
  return result;
}

std::vector<CBoundaryLayerMetric::WallReport> CBoundaryLayerMetric::Apply(const std::vector<su2double>& coord,
                                                                          std::vector<Tensor>& metric,
                                                                          su2double coreEigenvalue) {
  return Apply(SamplePoints(coord), metric, coreEigenvalue);
}

std::vector<CBoundaryLayerMetric::WallReport> CBoundaryLayerMetric::Apply(const std::vector<PointSample>& samples,
                                                                          std::vector<Tensor>& metric,
                                                                          su2double coreEigenvalue,
                                                                          bool prescribedNormal, bool fixedSurface,
                                                                          su2double arMax) {
  std::vector<WallReport> reports(walls.size());
  for (unsigned short wall = 0; wall < walls.size(); ++wall) reports[wall].name = walls[wall].layer.marker;

  /*--- A more distant wall must not overwrite the prescribed spacing of the closest wall in overlapping bands.
   *    Samples are ordered by marker name; equal distances therefore have a deterministic tie break. ---*/
  std::vector<size_t> nearest;
  if (prescribedNormal) {
    nearest.assign(metric.size(), samples.size());
    for (size_t k = 0; k < samples.size(); ++k) {
      const auto point = samples[k].point;
      if (nearest[point] == samples.size() || samples[k].sample.distance < samples[nearest[point]].sample.distance)
        nearest[point] = k;
    }
  }
  for (size_t k = 0; k < samples.size(); ++k) {
    const auto point = samples[k].point;
    if (prescribedNormal && nearest[point] != k) continue;
    const auto wall = samples[k].wall;
    const auto& sample = samples[k].sample;
    auto& report = reports[wall];
    ++report.nPoint;
    su2double vec[3][3], val[3], work[3];
    CBlasStructure::EigenDecomposition(sample.full.m, vec, val, nDim, work);
    report.maxAspectRatio =
        max(report.maxAspectRatio, sqrt(*std::max_element(val, val + nDim) / *std::min_element(val, val + nDim)));
    const Tensor previous = metric[point];
    if (prescribedNormal) {
      if (TangentialFloor(sample, val, data[wall]->h0, metric[point], true, fixedSurface, arMax)) ++report.nFloor;
    } else {
      Tensor faded = sample.full;
      if (sample.weight < 1.0) {
        const auto weight = sample.weight;
        Spectral(nDim, sample.full.m, faded.m,
                 [&](su2double value) { return exp(weight * log(value) + (1.0 - weight) * log(coreEigenvalue)); });
      }
      su2double intersection[3][3] = {{0.0}};
      CSolver::IntersectMetrics(nDim, metric[point].m, faded.m, intersection);
      for (unsigned short i = 0; i < nDim; ++i)
        for (unsigned short j = 0; j < nDim; ++j)
          metric[point].m[i][j] = 0.5 * (intersection[i][j] + intersection[j][i]);
      if (sample.weight == 1.0 && TangentialFloor(sample, val, data[wall]->h0, metric[point])) ++report.nFloor;
    }
    bool changed = false;
    for (unsigned short i = 0; i < nDim; ++i)
      for (unsigned short j = 0; j < nDim; ++j) changed = changed || metric[point].m[i][j] != previous.m[i][j];
    if (changed) ++report.nChanged;
  }
  return reports;
}

bool CBoundaryLayerMetric::TangentialFloor(const Sample& sample, const su2double* wallEigenvalues, su2double h0,
                                           Tensor& M, bool prescribedNormal, bool fixedSurface, su2double arMax) const {
  /*--- Band: up to floorFraction x the largest tangential size of the wall metric, at least h0. The smallest
   *    eigenvalue of the wall metric is a tangential one (the normal one is 1 / hn^2, the largest). ---*/
  const su2double htMax = 1.0 / sqrt(*std::min_element(wallEigenvalues, wallEigenvalues + nDim));
  const bool floor = fixedSurface && sample.distance <= max(h0, floorFraction * htMax);
  if (!prescribedNormal && !floor) return false;
  const Tensor previous = M;

  /*--- Orthonormal frame: tangents first, the wall normal last. ---*/
  const su2double* n = sample.normal;
  su2double Q[3][3] = {{0.0}};
  if (nDim == 2) {
    Q[0][0] = -n[1];
    Q[1][0] = n[0];
    Q[0][1] = n[0];
    Q[1][1] = n[1];
  } else {
    unsigned short axis = 0;
    for (unsigned short i = 1; i < 3; ++i)
      if (fabs(n[i]) < fabs(n[axis])) axis = i;
    su2double e[3] = {0.0, 0.0, 0.0};
    e[axis] = 1.0;
    auto t1 = Cross(n, e);
    const su2double norm = sqrt(Dot(3, t1.data(), t1.data()));
    for (auto& v : t1) v /= norm;
    const auto t2 = Cross(n, t1.data());
    for (unsigned short i = 0; i < 3; ++i) {
      Q[i][0] = t1[i];
      Q[i][1] = t2[i];
      Q[i][2] = n[i];
    }
  }

  /*--- Metrics in the frame: Q^T M Q. ---*/
  auto toFrame = [&](const su2double(&A)[3][3], su2double(&B)[3][3]) {
    for (unsigned short i = 0; i < nDim; ++i)
      for (unsigned short j = 0; j < nDim; ++j) {
        B[i][j] = 0.0;
        for (unsigned short k = 0; k < nDim; ++k)
          for (unsigned short l = 0; l < nDim; ++l) B[i][j] += Q[k][i] * A[k][l] * Q[l][j];
      }
  };
  su2double A[3][3] = {{0.0}}, W[3][3] = {{0.0}};
  toFrame(M.m, A);
  toFrame(sample.full.m, W);

  /*--- Tangential block: the union of the two metrics (in each direction the larger size), i.e. the inverse of the
   *    intersection of the inverses; the coupling with the normal is dropped, the normal entry is kept. ---*/
  const unsigned short nt = nDim - 1;
  su2double At[3][3] = {{0.0}}, Wt[3][3] = {{0.0}}, U[3][3] = {{0.0}};
  for (unsigned short i = 0; i < nt; ++i)
    for (unsigned short j = 0; j < nt; ++j) {
      At[i][j] = A[i][j];
      Wt[i][j] = W[i][j];
    }
  if (nt == 1) {
    U[0][0] = floor ? min(At[0][0], Wt[0][0]) : max(At[0][0], Wt[0][0]);
  } else if (!floor) {
    CSolver::IntersectMetrics(2, At, Wt, U);
  } else {
    su2double iA[3][3] = {{0.0}}, iW[3][3] = {{0.0}}, I[3][3] = {{0.0}};
    Spectral(2, At, iA, [](su2double v) { return 1.0 / v; });
    Spectral(2, Wt, iW, [](su2double v) { return 1.0 / v; });
    CSolver::IntersectMetrics(2, iA, iW, I);
    Spectral(2, I, U, [](su2double v) { return 1.0 / v; });
  }
  /*--- Bounds act on the tangential block; changing hn would violate the native first-height contract.
   *    A fixed near-wall surface cannot be refined to satisfy a smaller aspect ratio: retain its geometry floor. ---*/
  if (prescribedNormal && !floor) {
    const auto normal = W[nt][nt], ratio = arMax * arMax;
    if (nt == 1)
      U[0][0] = min(max(U[0][0], normal / ratio), normal * ratio);
    else {
      su2double vec[3][3], val[3], work[3];
      CBlasStructure::EigenDecomposition(U, vec, val, nt, work);
      for (unsigned short i = 0; i < nt; ++i) val[i] = min(val[i], normal * ratio);
      const auto lower = max(normal, *std::max_element(val, val + nt)) / ratio;
      for (unsigned short i = 0; i < nt; ++i) val[i] = max(val[i], lower);
      su2double bounded[3][3];
      CBlasStructure::EigenRecomposition(bounded, vec, val, nt);
      for (unsigned short i = 0; i < nt; ++i)
        for (unsigned short j = 0; j < nt; ++j) U[i][j] = bounded[i][j];
    }
  }
  su2double B[3][3] = {{0.0}};
  for (unsigned short i = 0; i < nt; ++i)
    for (unsigned short j = 0; j < nt; ++j) B[i][j] = U[i][j];
  B[nt][nt] = prescribedNormal ? W[nt][nt] : A[nt][nt];

  /*--- Back: M = Q B Q^T. ---*/
  for (unsigned short i = 0; i < nDim; ++i)
    for (unsigned short j = 0; j < nDim; ++j) {
      su2double value = 0.0;
      for (unsigned short k = 0; k < nDim; ++k)
        for (unsigned short l = 0; l < nDim; ++l) value += Q[i][k] * B[k][l] * Q[j][l];
      M.m[i][j] = value;
    }
  for (unsigned short i = 0; i < nDim; ++i)
    for (unsigned short j = 0; j < i; ++j) M.m[i][j] = M.m[j][i] = 0.5 * (M.m[i][j] + M.m[j][i]);
  /*--- Smooth SPD transition back to the sensor metric in the outer band, including its coupling. ---*/
  if (prescribedNormal && sample.weight < 1.0) {
    su2double logOld[3][3], logLayer[3][3], blended[3][3];
    Spectral(nDim, previous.m, logOld, [](su2double value) { return log(value); });
    Spectral(nDim, M.m, logLayer, [](su2double value) { return log(value); });
    for (unsigned short i = 0; i < nDim; ++i)
      for (unsigned short j = 0; j < nDim; ++j)
        blended[i][j] = (1.0 - sample.weight) * logOld[i][j] + sample.weight * logLayer[i][j];
    Spectral(nDim, blended, M.m, [](su2double value) { return exp(value); });
  }
  return floor;
}
