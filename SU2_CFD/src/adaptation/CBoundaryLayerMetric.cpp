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
#include "../../../Common/include/adaptation/CNativeImport3D.hpp"
#include "../../include/adaptation/ConvexClipping.hpp"
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

/*--- Invert the tangent block independently: a 3D eigensolve can lose h0^2
 *     through large spectral shifts even on a flat wall. Keep its logarithm for the fade. ---*/
void WallMetric3D(const Mat3& size, CBoundaryLayerMetric::Sample& sample) {
#if defined(CODI_FORWARD_TYPE) || defined(CODI_REVERSE_TYPE)
  // Native3D is primal-only; retain the existing active wall-metric path for other callers.
  su2double combined[3][3];
  for(unsigned i=0;i<3;++i)for(unsigned j=0;j<3;++j)
    combined[i][j]=size[i][j]+pow(sample.hn,2)*sample.normal[i]*sample.normal[j];
  Spectral(3,combined,sample.full.m,[](su2double value){return 1/value;});
  Spectral(3,sample.full.m,sample.logarithm.m,[](su2double value){return log(value);});
#else
  using Vector = std::array<long double,3>;
  Vector n{SU2_TYPE::GetValue(sample.normal[0]),SU2_TYPE::GetValue(sample.normal[1]),SU2_TYPE::GetValue(sample.normal[2])};
  const unsigned axis=std::min_element(n.begin(),n.end(),[](auto a,auto b){return std::abs(a)<std::abs(b);})-n.begin();
  Vector u{},v{};
  u[(axis+1)%3]=n[(axis+2)%3];u[(axis+2)%3]=-n[(axis+1)%3];
  const auto length=std::sqrt(u[0]*u[0]+u[1]*u[1]+u[2]*u[2]);
  for(auto& x:u) x/=length;
  v={n[1]*u[2]-n[2]*u[1],n[2]*u[0]-n[0]*u[2],n[0]*u[1]-n[1]*u[0]};
  long double aa=0,bb=0,cc=0;
  for(unsigned i=0;i<3;++i)for(unsigned j=0;j<3;++j) {
    const auto s=SU2_TYPE::GetValue(size[i][j]);
    aa+=u[i]*s*u[j];bb+=u[i]*s*v[j];cc+=v[i]*s*v[j];
  }
  const auto scale=std::max({std::abs(aa),std::abs(bb),std::abs(cc)});
  if (!(scale>0) || !std::isfinite(scale))
    throw std::runtime_error("Invalid native 3D wall tangent size.");
  aa/=scale;bb/=scale;cc/=scale;
  const auto largest=.5L*(aa+cc+std::hypot(aa-cc,2*bb));
  const auto smallest=std::fma(aa,cc,-bb*bb)/largest;
  if (!(smallest>0)) throw std::runtime_error("Singular native 3D wall tangent size.");
  const auto angle=.5L*std::atan2(2*bb,aa-cc), cosine=std::cos(angle), sine=std::sin(angle);
  Vector q{},r{};
  for(unsigned i=0;i<3;++i) {q[i]=cosine*u[i]+sine*v[i];r[i]=-sine*u[i]+cosine*v[i];}
  const auto l0=std::log(scale)+std::log(largest),l1=std::log(scale)+std::log(smallest);
  const auto ln=-2*std::log(SU2_TYPE::GetValue(sample.hn));
  for(unsigned i=0;i<3;++i)for(unsigned j=0;j<3;++j) {
    sample.full.m[i][j]=std::exp(-l0)*q[i]*q[j]+std::exp(-l1)*r[i]*r[j]+std::exp(ln)*n[i]*n[j];
    sample.logarithm.m[i][j]=-l0*q[i]*q[j]-l1*r[i]*r[j]+ln*n[i]*n[j];
  }
#endif
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
    ht = length;
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
    WallMetric3D(tangentSize, sample);
  }

  /*--- Fade: eigenvalues interpolated in logarithm towards the isotropic core metric. ---*/
  if (sample.weight < 1.0) {
    const su2double w = sample.weight;
    if (nDim == 3)
      Spectral(3, sample.logarithm.m, sample.metric.m, [&](su2double v) { return exp(w*v + (1-w)*log(coreEigenvalue)); });
    else
      Spectral(2, M, sample.metric.m, [&](su2double v) { return exp(w*log(v) + (1-w)*log(coreEigenvalue)); });
  } else {
    sample.metric = sample.full;
  }
  return sample;
}

std::vector<CBoundaryLayerMetric::WallReport> CBoundaryLayerMetric::Apply(const std::vector<su2double>& coord,
                                                                          std::vector<Tensor>& metric,
                                                                          su2double coreEigenvalue) {
  std::vector<WallReport> reports(walls.size());
  const auto nPoint = metric.size();

  for (unsigned short iWall = 0; iWall < walls.size(); ++iWall) reports[iWall].name = walls[iWall].layer.marker;
  for (auto iPoint = 0ul; iPoint < nPoint; ++iPoint)
    ApplyPoint(&coord[iPoint * nDim], metric[iPoint], coreEigenvalue, &reports);
  return reports;
}

double CBoundaryLayerMetric::CoreEigenvalue2D(double xx, double xy, double yy) {
  const double scale=std::max({std::abs(xx),std::abs(xy),std::abs(yy)});
  const double largest=.5*(xx/scale+yy/scale+std::hypot((xx-yy)/scale,2*xy/scale));
  return scale*static_cast<double>(SU2Native2D::NormalizedDeterminant(xx,xy,yy))/largest;
}

std::shared_ptr<CBoundaryLayerMetric> CBoundaryLayerMetric::FromNativeReference(
    const SU2NativeBoundary2D::ReferenceState& reference, const CConfig& config) {
  if (!reference.original) throw std::invalid_argument("Native BL composition needs original geometry.");
  std::vector<Wall> walls;
  for (unsigned short b = 0; b < config.GetnAdap_BL(); ++b) {
    Wall wall;
    wall.layer = config.GetAdap_BL(b);
    std::map<uint64_t, unsigned long> points;
    for (const auto& component : reference.original->Components()) {
      if (reference.marker_names.at(component.marker) != wall.layer.marker) continue;
      for (size_t k = 1; k < component.nodes.size(); ++k)
        for (const auto node : {component.nodes[k - 1], component.nodes[k]}) {
          auto insertion = points.emplace(node.id, points.size());
          if (insertion.second) { wall.coord.push_back(node.p.x); wall.coord.push_back(node.p.y); }
          wall.conn.push_back(insertion.first->second);
        }
    }
    walls.push_back(std::move(wall));
  }
  return std::make_shared<CBoundaryLayerMetric>(2, std::move(walls), config.GetAdap_Angle());
}

std::shared_ptr<CBoundaryLayerMetric> CBoundaryLayerMetric::FromNativeReference(
    const SU2Native3D::ReferenceState& reference, const CConfig& config) {
  if (!reference.original) throw std::invalid_argument("Native3D BL needs immutable original geometry.");
  std::vector<Wall> walls;
  for (unsigned short b=0;b<config.GetnAdap_BL();++b) {
    Wall wall;wall.layer=config.GetAdap_BL(b);
    std::map<uint64_t,unsigned long> points;
    for(const auto& face:reference.facets) {
      if(reference.marker_names.at(face.marker)!=wall.layer.marker) continue;
      if(reference.original->Analytic().count(face.id))
        throw std::runtime_error("Native3D analytic BL distance/normal evaluation is not implemented.");
      for(const auto& node:face.v) {
        auto insertion=points.emplace(node.id,points.size());
        if(insertion.second) {
          wall.coord.push_back(node.p.x);wall.coord.push_back(node.p.y);wall.coord.push_back(node.p.z);
        }
        wall.conn.push_back(insertion.first->second);
      }
      wall.conn.push_back(wall.conn.back());
    }
    if(wall.conn.empty()) throw std::invalid_argument("Native3D BL marker has no original wall faces: "+wall.layer.marker);
    walls.push_back(std::move(wall));
  }
  return std::make_shared<CBoundaryLayerMetric>(3,std::move(walls),config.GetAdap_Angle());
}

namespace {
using BLPoint3D=std::array<double,3>;
using BLTetrahedron=std::array<BLPoint3D,4>;
long double BLDeterminant(const BLPoint3D& a,const BLPoint3D& b,const BLPoint3D& c,const BLPoint3D& d) {
  return SU2Native3D::Orientation({a[0],a[1],a[2]},{b[0],b[1],b[2]},{c[0],c[1],c[2]},{d[0],d[1],d[2]});
}
// Separating projections only exclude support; uncertain geometry always receives subdivision.
bool MayIntersectWallBand(const BLTetrahedron& tet,const std::array<BLPoint3D,3>& face,double thickness) {
  using V=std::array<long double,3>;
  auto minus=[](const BLPoint3D& a,const BLPoint3D& b) {return V{(long double)a[0]-b[0],(long double)a[1]-b[1],(long double)a[2]-b[2]};};
  auto cross=[](const V& a,const V& b) {return V{a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]};};
  auto separated=[&](const V& axis) {
    const auto norm=std::sqrt(axis[0]*axis[0]+axis[1]*axis[1]+axis[2]*axis[2]);
    if(!norm) return false;
    auto project=[&](const BLPoint3D& p) {const auto x=minus(p,tet[0]);return axis[0]*x[0]+axis[1]*x[1]+axis[2]*x[2];};
    long double al=project(tet[0]),ah=al,bl=project(face[0]),bh=bl;
    for(const auto& p:tet) {const auto t=project(p);al=std::min(al,t);ah=std::max(ah,t);}
    for(const auto& p:face) {const auto t=project(p);bl=std::min(bl,t);bh=std::max(bh,t);}
    const auto margin=64*std::numeric_limits<long double>::epsilon()*std::max({std::abs(al),std::abs(ah),std::abs(bl),std::abs(bh),thickness*norm});
    return bl-ah>thickness*norm+margin || al-bh>thickness*norm+margin;
  };
  if(separated(cross(minus(face[1],face[0]),minus(face[2],face[0])))) return false;
  for(unsigned f=0;f<4;++f)
    if(separated(cross(minus(tet[(f+2)%4],tet[(f+1)%4]),minus(tet[(f+3)%4],tet[(f+1)%4])))) return false;
  for(unsigned i=0;i<4;++i)for(unsigned j=i+1;j<4;++j)for(unsigned k=0;k<3;++k)
    if(separated(cross(minus(tet[j],tet[i]),minus(face[(k+1)%3],face[k])))) return false;
  // Rounded offsets also have vertex/edge contact directions absent from polytope SAT normals.
  auto closestAxis=[&](const BLPoint3D& p,const std::array<BLPoint3D,3>& triangle) {
    su2double x[3]={p[0],p[1],p[2]},vertex[3][3],weights[3];
    for(unsigned k=0;k<3;++k)for(unsigned j=0;j<3;++j)vertex[k][j]=triangle[k][j];
    ClosestPointTriangle(vertex[0],vertex[1],vertex[2],x,weights);
    BLPoint3D closest{};
    for(unsigned j=0;j<3;++j)for(unsigned k=0;k<3;++k)
      closest[j]+=SU2_TYPE::GetValue(weights[k])*triangle[k][j];
    return minus(p,closest);
  };
  for(const auto& p:tet)if(separated(closestAxis(p,face)))return false;
  for(const auto& p:face)for(unsigned f=0;f<4;++f)
    if(separated(closestAxis(p,{tet[(f+1)%4],tet[(f+2)%4],tet[(f+3)%4]})))return false;
  return true;
}
void SetBLSourceQuery(const BLTetrahedron& source,const SU2Native3D::Tetrahedron& positive,bool reversed,
                      std::array<double,4>& parent,BLPoint3D& position) {
  for(auto w:parent)if(!std::isfinite(w) || w < -64*std::numeric_limits<double>::epsilon())
    throw std::runtime_error("Native3D BL integration has invalid original barycentrics.");
  const auto requested=parent;
  for(unsigned attempt=0;attempt<8;++attempt) {
    const double inward=attempt?std::ldexp(std::numeric_limits<double>::epsilon(),attempt+3):0;
    for(unsigned j=0;j<3;++j) {
      long double x=source[0][j];
      for(unsigned k=1;k<4;++k)x+=((1-inward)*requested[k]+inward*.25)*((long double)source[k][j]-source[0][j]);
      position[j]=double(x);
    }
    std::array<long double,4> actual;
    if(!SU2Native3D::Barycentric(positive,{position[0],position[1],position[2]},actual))continue;
    if(reversed)std::swap(actual[1],actual[2]);
    for(unsigned k=0;k<4;++k)parent[k]=double(actual[k]);
    return;
  }
  throw std::runtime_error("Native3D BL quadrature cannot place a representable original-cell sample.");
}

template<class Append>
void VisitBLTetrahedra(const convex_clip::Polyhedron& piece,Append append) {
  if(!piece.n) return;
    if(piece.n==4) {
      // A tetrahedral piece already has the positive degree-two rule; avoid four redundant fans.
      BLTetrahedron t;
      for(unsigned k=0;k<4;++k)for(unsigned j=0;j<3;++j)t[k][j]=piece.x[k][j];
      append(t);return;
    }
    BLPoint3D center{};
    for(int v=0;v<piece.n;++v)for(unsigned j=0;j<3;++j)center[j]+=piece.x[v][j]/piece.n;
    bool marked[convex_clip::Polyhedron::MAXV][3]{};
    for(int vs=0;vs<piece.n;++vs)for(int ps=0;ps<3;++ps) {
      if(marked[vs][ps]) continue;
      std::vector<int> face;int current=vs,port=ps;
      do {
        if(face.size()>convex_clip::Polyhedron::MAXV) throw std::runtime_error("Invalid native3D BL polyhedron face graph.");
        marked[current][port]=true;face.push_back(current);
        const int next=piece.nbr[current][port];int back=0;
        while(back<3 && piece.nbr[next][back]!=current)++back;
        if(back==3) throw std::runtime_error("Invalid native3D BL polyhedron incidence.");
        current=next;port=(back+1)%3;
      }while(current!=vs);
      for(size_t f=1;f+1<face.size();++f) {
        BLTetrahedron t{center};
        const int vertices[3]={face[0],face[f],face[f+1]};
        for(unsigned k=0;k<3;++k)for(unsigned j=0;j<3;++j)t[k+1][j]=piece.x[vertices[k]][j];
        append(t);
      }
    }
}

}

std::vector<CBoundaryLayerMetric::IntegrationPoint3D> CBoundaryLayerMetric::IntegrationRule3D(
    const std::array<std::array<double,3>,4>& tetrahedron,double bandRatio) {
  if(nDim!=3 || !std::isfinite(bandRatio) || !(bandRatio>1))
    throw std::invalid_argument("Invalid native3D BL volume quadrature.");
  for(const auto& p:tetrahedron)for(auto x:p)if(!std::isfinite(x))
    throw std::invalid_argument("Nonfinite native3D BL integration cell.");
  if(BLDeterminant(tetrahedron[0],tetrahedron[1],tetrahedron[2],tetrahedron[3])==0)
    throw std::invalid_argument("Degenerate native3D BL integration cell.");
  const std::array<std::array<double,4>,4> bary{{{{1,0,0,0}},{{0,1,0,0}},{{0,0,1,0}},{{0,0,0,1}}}};
  using Poly=convex_clip::Polyhedron;
  const double vertices[4][3]={{0,0,0},{1,0,0},{0,1,0},{0,0,1}};
  const double* points[4]={vertices[0],vertices[1],vertices[2],vertices[3]};
  Poly initial;initial.InitTetrahedron(points);std::vector<Poly> pieces{initial};
  auto cut=[&](const std::array<long double,3>& normal,const BLPoint3D& origin,long double offset) {
    std::array<long double,4> values{};
    for(unsigned k=0;k<4;++k) {
      values[k]=offset;
      for(unsigned j=0;j<3;++j)values[k]+=normal[j]*((long double)tetrahedron[k][j]-origin[j]);
    }
    const auto scale=std::max({std::abs(values[0]),std::abs(values[1]),std::abs(values[2]),std::abs(values[3])});
    if(!scale)return;
    const double nx=(values[1]-values[0])/scale,ny=(values[2]-values[0])/scale,nz=(values[3]-values[0])/scale,d=values[0]/scale;
    std::vector<Poly> next;
    for(const auto& piece:pieces) {
      double low=std::numeric_limits<double>::max(),high=-low;
      for(int v=0;v<piece.n;++v){const auto value=nx*piece.x[v][0]+ny*piece.x[v][1]+nz*piece.x[v][2]+d;low=std::min(low,value);high=std::max(high,value);}
      if(low>=0 || high<=0)next.push_back(piece);
      else {
        auto lower=piece,upper=piece;
        if(lower.Clip(-nx,-ny,-nz,-d)<0 || upper.Clip(nx,ny,nz,d)<0)
          throw std::runtime_error("Native3D BL feature partition exceeds vertex capacity.");
        if(lower.n)next.push_back(lower);if(upper.n)next.push_back(upper);
      }
      if(next.size()>4096)throw std::runtime_error("Native3D BL feature partition exceeds piece budget.");
    }
    pieces=std::move(next);
  };
  for(unsigned short wall=0;wall<walls.size();++wall) {
    const auto& dataWall=*data[wall];if(!dataWall.adt)continue;
    const double thickness=SU2_TYPE::GetValue(dataWall.thickness);
    bool hidden=true;
    for(const auto& p:tetrahedron){const su2double x[3]={p[0],p[1],p[2]};hidden=hidden && Evaluate(wall,x,1.).distance>=thickness;}
    if(!hidden)continue;
    su2double low[3],high[3];
    for(unsigned j=0;j<3;++j) {
      double a=tetrahedron[0][j],b=a;for(const auto& p:tetrahedron){a=std::min(a,p[j]);b=std::max(b,p[j]);}
      low[j]=a-thickness;high[j]=b+thickness;
    }
    std::vector<unsigned long> faces;dataWall.adt->DetermineIntersectingElements(low,high,faces);
    std::sort(faces.begin(),faces.end());
    for(auto f:faces) {
      std::array<BLPoint3D,3> face;
      for(unsigned k=0;k<3;++k)for(unsigned j=0;j<3;++j)
        face[k][j]=SU2_TYPE::GetValue(walls[wall].coord[3*dataWall.elem[3*f+k]+j]);
      if(!MayIntersectWallBand(tetrahedron,face,thickness))continue;
      using V=std::array<long double,3>;
      auto minus=[](const BLPoint3D& a,const BLPoint3D& b){return V{(long double)a[0]-b[0],(long double)a[1]-b[1],(long double)a[2]-b[2]};};
      auto cross=[](const V& a,const V& b){return V{a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]};};
      const auto normal=cross(minus(face[1],face[0]),minus(face[2],face[0]));
      const auto length=std::sqrt(normal[0]*normal[0]+normal[1]*normal[1]+normal[2]*normal[2]);
      for(const double level:{-thickness,0.,thickness})cut(normal,face[0],level*length);
      for(unsigned k=0;k<3;++k) {
        const auto edge=minus(face[(k+1)%3],face[k]);
        const auto side=cross(normal,edge);
        // Face prism and edge/vertex Voronoi boundaries localize support before curved-distance refinement.
        cut(side,face[k],0);
        cut(edge,face[k],0);cut(edge,face[(k+1)%3],0);
      }
    }
  }
  size_t visited=0;std::vector<IntegrationPoint3D> rule;
  if(pieces.size()==1)IntegrationRule3DImpl(tetrahedron,bary,tetrahedron,bandRatio,0,visited,rule);
  else for(const auto& piece:pieces)VisitBLTetrahedra(piece,[&](const BLTetrahedron& local) {
    BLTetrahedron physical{};std::array<std::array<double,4>,4> mapping{};
    for(unsigned k=0;k<4;++k) {
      mapping[k]={1-local[k][0]-local[k][1]-local[k][2],local[k][0],local[k][1],local[k][2]};
      for(unsigned j=0;j<3;++j)for(unsigned v=0;v<4;++v)physical[k][j]+=mapping[k][v]*tetrahedron[v][j];
    }
    const auto determinant=std::abs(BLDeterminant(physical[0],physical[1],physical[2],physical[3]));
    if(!determinant)return;
    bool supported=false;
    for(unsigned short wall=0;wall<walls.size() && !supported;++wall) {
      const auto& wallData=*data[wall];if(!wallData.adt)continue;
      const double thickness=SU2_TYPE::GetValue(wallData.thickness);
      su2double low[3],high[3];
      for(unsigned j=0;j<3;++j) {
        double a=physical[0][j],b=a;for(const auto& p:physical){a=std::min(a,p[j]);b=std::max(b,p[j]);}
        low[j]=a-thickness;high[j]=b+thickness;
      }
      std::vector<unsigned long> faces;wallData.adt->DetermineIntersectingElements(low,high,faces);
      for(auto f:faces) {
        std::array<BLPoint3D,3> face;
        for(unsigned k=0;k<3;++k)for(unsigned j=0;j<3;++j)
          face[k][j]=SU2_TYPE::GetValue(walls[wall].coord[3*wallData.elem[3*f+k]+j]);
        supported=MayIntersectWallBand(physical,face,thickness*(1-1e-12));
        if(supported)break;
      }
    }
    if(!supported) {
      IntegrationRule3DImpl(physical,mapping,tetrahedron,bandRatio,0,visited,rule);
      return;
    }
    // Conical quadrature resolves distance bands along actual rays instead of bisecting curved support in3D.
    unsigned apex=0;double farthest=-1;
    for(unsigned k=0;k<4;++k) {
      double nearest=std::numeric_limits<double>::max();
      for(unsigned short wall=0;wall<walls.size();++wall) {
        const su2double x[3]={physical[k][0],physical[k][1],physical[k][2]};
        if(data[wall]->adt)nearest=std::min(nearest,SU2_TYPE::GetValue(Evaluate(wall,x,1.).distance));
      }
      if(nearest>farthest){farthest=nearest;apex=k;}
    }
    SU2Native3D::Tetrahedron originalSource;
    for(unsigned k=0;k<4;++k)originalSource[k]={tetrahedron[k][0],tetrahedron[k][1],tetrahedron[k][2]};
    const bool reversed=SU2Native3D::Orientation(originalSource[0],originalSource[1],originalSource[2],originalSource[3])<0;
    if(reversed)std::swap(originalSource[1],originalSource[2]);
    const double gauss[5]={-.90617984593866399280,-.53846931010568309104,0,.53846931010568309104,.90617984593866399280};
    const double weights[5]={.23692688505618908751,.47862867049936646804,128./225,.47862867049936646804,.23692688505618908751};
    std::array<unsigned,3> base{};unsigned next=0;for(unsigned k=0;k<4;++k)if(k!=apex)base[next++]=k;
    const double faceBary[12][3]={
        {.501426509658179,.249286745170910,.249286745170910},
        {.249286745170910,.501426509658179,.249286745170910},
        {.249286745170910,.249286745170910,.501426509658179},
        {.873821971016996,.063089014491502,.063089014491502},
        {.063089014491502,.873821971016996,.063089014491502},
        {.063089014491502,.063089014491502,.873821971016996},
        {.053145049844817,.310352451033784,.636502499121399},
        {.053145049844817,.636502499121399,.310352451033784},
        {.310352451033784,.053145049844817,.636502499121399},
        {.310352451033784,.636502499121399,.053145049844817},
        {.636502499121399,.053145049844817,.310352451033784},
        {.636502499121399,.310352451033784,.053145049844817}};
    const double faceWeights[12]={.116786275726379,.116786275726379,.116786275726379,
        .050844906370207,.050844906370207,.050844906370207,
        .082851075618374,.082851075618374,.082851075618374,.082851075618374,.082851075618374,.082851075618374};
    for(unsigned corner=0;corner<12;++corner) {
      std::array<double,4> baseBary{};
      for(unsigned j=0;j<4;++j)for(unsigned k=0;k<3;++k)
        baseBary[j]+=mapping[base[k]][j]*faceBary[corner][k];
      BLPoint3D basePoint{};
      for(unsigned j=0;j<3;++j)for(unsigned k=0;k<4;++k)basePoint[j]+=baseBary[k]*tetrahedron[k][j];
      const auto cuts=EdgeBreakpoints(physical[apex],basePoint,std::max(2.,bandRatio),4);
      for(size_t band=1;band<cuts.size();++band)for(unsigned g=0;g<5;++g) {
        if(rule.size()>=65536)throw std::runtime_error("Native3D BL volume quadrature point budget exhausted.");
        const double half=.5*(cuts[band]-cuts[band-1]),t=.5*(cuts[band]+cuts[band-1])+half*gauss[g];
        std::array<double,4> parent{};BLPoint3D position{};
        for(unsigned k=0;k<4;++k) {
          parent[k]=(1-t)*mapping[apex][k]+t*baseBary[k];
          for(unsigned j=0;j<3;++j)position[j]+=parent[k]*tetrahedron[k][j];
        }
        SetBLSourceQuery(tetrahedron,originalSource,reversed,parent,position);
        rule.push_back({parent,double(determinant/2*faceWeights[corner]*half*weights[g]*t*t),SamplePoints({position[0],position[1],position[2]})});
      }
    }
  });
  return rule;
}

void CBoundaryLayerMetric::IntegrationRule3DImpl(const BLTetrahedron& tet,
    const std::array<std::array<double,4>,4>& original,const BLTetrahedron& source,double bandRatio,unsigned depth,
    size_t& visited,std::vector<IntegrationPoint3D>& rule) {
  if(++visited>16384) throw std::runtime_error("Native3D BL geometry quadrature work budget exhausted.");
  std::vector<std::array<double,4>> distances(walls.size());
  bool subdivide=false;
  for(unsigned short wall=0;wall<walls.size();++wall) {
    const auto& d=*data[wall];if(!d.adt) continue;
    auto distance=[&](const BLPoint3D& p) {const su2double x[3]={p[0],p[1],p[2]};return SU2_TYPE::GetValue(Evaluate(wall,x,1.).distance);};
    for(unsigned k=0;k<4;++k) distances[wall][k]=distance(tet[k]);
    const auto minimum=*std::min_element(distances[wall].begin(),distances[wall].end());
    const double thickness=SU2_TYPE::GetValue(d.thickness);
    if(minimum>=thickness) {
      su2double low[3],high[3];
      for(unsigned j=0;j<3;++j) {
        double a=tet[0][j],b=a;for(const auto& p:tet){a=std::min(a,p[j]);b=std::max(b,p[j]);}
        low[j]=a-thickness;high[j]=b+thickness;
      }
      std::vector<unsigned long> faces;d.adt->DetermineIntersectingElements(low,high,faces);
      for(auto f:faces) {
        std::array<BLPoint3D,3> face;
        for(unsigned k=0;k<3;++k)for(unsigned j=0;j<3;++j)
          face[k][j]=SU2_TYPE::GetValue(walls[wall].coord[3*d.elem[3*f+k]+j]);
        subdivide=subdivide || MayIntersectWallBand(tet,face,thickness*(1-1e-12));
        if(subdivide) break;
      }
    } else {
      const double tolerance=.1*std::max(SU2_TYPE::GetValue(d.h0),minimum);
      // Edge, face and cell samples detect nearest-face changes before distance-band clipping.
      for(unsigned mask=1;mask<16 && !subdivide;++mask) {
        unsigned count=0;for(unsigned k=0;k<4;++k)count+=(mask>>k)&1u;
        if(count==1) continue;
        BLPoint3D p{};double linear=0;
        for(unsigned k=0;k<4;++k)if(mask&(1u<<k)) {
          linear+=distances[wall][k]/count;
          for(unsigned j=0;j<3;++j)p[j]+=tet[k][j]/count;
        }
        subdivide=std::abs(distance(p)-linear)>tolerance;
      }
    }
    if(subdivide) break;
  }
  if(subdivide) {
    if(depth>=32) throw std::runtime_error("Native3D BL geometry remains unresolved after 32 subdivisions.");
    unsigned a=0,b=1;long double largest=0;
    for(unsigned i=0;i<4;++i)for(unsigned j=i+1;j<4;++j) {
      long double length=0;for(unsigned k=0;k<3;++k){const auto x=(long double)tet[i][k]-tet[j][k];length+=x*x;}
      if(length>largest){largest=length;a=i;b=j;}
    }
    BLPoint3D midpoint;std::array<double,4> bary{};
    for(unsigned j=0;j<3;++j)midpoint[j]=.5*tet[a][j]+.5*tet[b][j];
    for(unsigned j=0;j<4;++j)bary[j]=.5*original[a][j]+.5*original[b][j];
    if(midpoint==tet[a] || midpoint==tet[b]) throw std::runtime_error("Unrepresentable native3D BL subdivision.");
    for(const auto vertex:{a,b}) {
      auto child=tet;auto mapping=original;child[vertex]=midpoint;mapping[vertex]=bary;
      IntegrationRule3DImpl(child,mapping,source,bandRatio,depth+1,visited,rule);
    }
    return;
  }
  using Poly=convex_clip::Polyhedron;
  const double nodes[4][3]={{0,0,0},{1,0,0},{0,1,0},{0,0,1}};
  const double* initial[4]={nodes[0],nodes[1],nodes[2],nodes[3]};
  Poly first;first.InitTetrahedron(initial);std::vector<Poly> pieces{first};
  for(unsigned short wall=0;wall<walls.size();++wall) {
    const auto& d=*data[wall];if(!d.adt) continue;
    const auto& distance=distances[wall];const double thickness=SU2_TYPE::GetValue(d.thickness);
    if(*std::min_element(distance.begin(),distance.end())>=thickness) continue;
    const double h0=SU2_TYPE::GetValue(d.h0),full=SU2_TYPE::GetValue(d.full);
    std::vector<double> levels{.5*h0};
    for(double level=h0;level<full;) {
      if(levels.size()>4096) throw std::runtime_error("Native3D BL volume bands exceed work budget.");
      levels.push_back(level);const double next=level*bandRatio;
      if(!(next>level)) throw std::runtime_error("Unresolved native3D BL volume bands.");
      level=next;
    }
    levels.push_back(full);for(unsigned k=1;k<=16;++k)levels.push_back(full+(thickness-full)*k/16);
    for(auto level:levels) {
      std::vector<Poly> next;
      for(const auto& piece:pieces) {
        double low=std::numeric_limits<double>::max(),high=-low;
        const double nx=distance[1]-distance[0],ny=distance[2]-distance[0],nz=distance[3]-distance[0],offset=distance[0]-level;
        for(int v=0;v<piece.n;++v){const auto value=nx*piece.x[v][0]+ny*piece.x[v][1]+nz*piece.x[v][2]+offset;low=std::min(low,value);high=std::max(high,value);}
        if(low>=0 || high<=0){
          next.push_back(piece);
          if(next.size()>4096) throw std::runtime_error("Native3D BL clipped volume exceeds piece budget.");
          continue;
        }
        auto lower=piece,upper=piece;
        if(lower.Clip(-nx,-ny,-nz,-offset)<0 || upper.Clip(nx,ny,nz,offset)<0)
          throw std::runtime_error("Native3D BL clipped polyhedron exceeds vertex capacity.");
        if(lower.n>0)next.push_back(lower);if(upper.n>0)next.push_back(upper);
        if(next.size()>4096) throw std::runtime_error("Native3D BL clipped volume exceeds piece budget.");
      }
      pieces=std::move(next);
    }
  }
  SU2Native3D::Tetrahedron originalSource;
  for(unsigned k=0;k<4;++k)originalSource[k]={source[k][0],source[k][1],source[k][2]};
  const bool reversed=SU2Native3D::Orientation(originalSource[0],originalSource[1],originalSource[2],originalSource[3])<0;
  if(reversed)std::swap(originalSource[1],originalSource[2]);
  const auto determinant=std::abs(BLDeterminant(tet[0],tet[1],tet[2],tet[3]));
  auto append=[&](const BLTetrahedron& t) {
    const auto fraction=std::abs(BLDeterminant(t[0],t[1],t[2],t[3]));
    if(!fraction)return;
    for(unsigned corner=0;corner<4;++corner) {
      if(rule.size()>=65536) throw std::runtime_error("Native3D BL volume quadrature point budget exhausted.");
      std::array<double,4> local{},parent{};
      for(unsigned k=0;k<4;++k) {
        const double w=k==corner?.58541019662496845446:.13819660112501051518;
        local[0]+=w*(1-t[k][0]-t[k][1]-t[k][2]);
        for(unsigned j=0;j<3;++j)local[j+1]+=w*t[k][j];
      }
      BLPoint3D position{};
      for(unsigned k=0;k<4;++k)for(unsigned j=0;j<4;++j)parent[j]+=local[k]*original[k][j];
      for(unsigned k=0;k<4;++k)for(unsigned j=0;j<3;++j)position[j]+=parent[k]*source[k][j];
      SetBLSourceQuery(source,originalSource,reversed,parent,position);
      rule.push_back({parent,double(determinant*fraction/24),SamplePoints({position[0],position[1],position[2]})});
    }
  };
  for(const auto& piece:pieces)VisitBLTetrahedra(piece,append);
}

std::vector<double> CBoundaryLayerMetric::EdgeBreakpoints(
    const std::array<double, 3>& a, const std::array<double, 3>& b, double bandRatio, unsigned fadeBands) {
  if (!std::isfinite(bandRatio) || !(bandRatio > 1.0) || !fadeBands || fadeBands>256)
    throw std::invalid_argument("Invalid native BL edge band ratio/fade partition.");
  using Vector = std::array<long double, 3>;
  Vector chord{};
  for (unsigned j = 0; j < nDim; ++j) {
    if (!std::isfinite(a[j]) || !std::isfinite(b[j]))
      throw std::invalid_argument("Nonfinite native BL chord.");
    chord[j] = static_cast<long double>(b[j]) - a[j];
  }
  auto dot = [](const Vector& u, const Vector& v) {
    return u[0]*v[0] + u[1]*v[1] + u[2]*v[2];
  };
  if (!(dot(chord,chord) > 0)) throw std::invalid_argument("Zero native BL chord.");
  std::vector<double> cuts{0,1};
  auto retain = [&](long double t) {
    if (t > 0 && t < 1) {
      if (cuts.size() >= 65536) throw std::runtime_error("Native BL edge breakpoints exceed work budget.");
      cuts.push_back(static_cast<double>(t));
    }
  };
  auto roots = [&](long double aa, long double bb, long double cc) {
    const auto scale = std::max({std::abs(aa),std::abs(bb),std::abs(cc)});
    if (!scale) return;
    aa /= scale; bb /= scale; cc /= scale;
    if (!aa) { if (bb) retain(-cc/bb); return; }
    const auto discriminant = std::fma(bb,bb,-4*aa*cc);
    if (discriminant < 0) return;
    const auto q = -.5L*(bb + std::copysign(std::sqrt(discriminant),bb));
    if (!q) retain(-bb/(2*aa));
    else { retain(q/aa); retain(cc/q); }
  };
  for (unsigned short wall = 0; wall < walls.size(); ++wall) {
    const size_t wallCutsBegin=cuts.size();
    const auto& d = *data[wall];
    if (!d.adt) continue;
    const double h0 = SU2_TYPE::GetValue(d.h0), full = SU2_TYPE::GetValue(d.full);
    const double thickness = SU2_TYPE::GetValue(d.thickness);
    std::vector<double> levels{0,.5*h0};
    for (double level = h0; level < full;) {
      if (levels.size() >= 4096) throw std::runtime_error("Native BL distance bands exceed work budget.");
      levels.push_back(level);
      const double next = level*bandRatio;
      if (!(next > level)) throw std::runtime_error("Unresolved native BL distance bands.");
      level = next;
    }
    levels.push_back(full);
    for (unsigned k = 1; k <= fadeBands; ++k) levels.push_back(full + (thickness-full)*k/fadeBands);
    su2double low[3]{}, high[3]{};
    for (unsigned j = 0; j < nDim; ++j) {
      low[j] = std::min(a[j],b[j])-thickness;
      high[j] = std::max(a[j],b[j])+thickness;
    }
    std::vector<unsigned long> faces;
    d.adt->DetermineIntersectingElements(low,high,faces);
    std::sort(faces.begin(),faces.end());
    for (const auto face : faces) {
      const unsigned count = nDim == 2 ? 2 : 3;
      Vector x[3]{};
      for (unsigned k = 0; k < count; ++k)
        for (unsigned j = 0; j < nDim; ++j)
          x[k][j] = SU2_TYPE::GetValue(walls[wall].coord[nDim*d.elem[count*face+k]+j]);
      // All feature roots are retained; extra cuts are harmless and avoid fragile Voronoi predicates.
      for (unsigned k = 0; k < count; ++k) {
        Vector r{};
        for (unsigned j = 0; j < nDim; ++j) r[j] = a[j]-x[k][j];
        for (const auto level : levels)
          roots(dot(chord,chord),2*dot(r,chord),dot(r,r)-static_cast<long double>(level)*level);
        if (nDim == 2 && k == 1) continue;
        Vector edge{};
        for (unsigned j = 0; j < nDim; ++j) edge[j] = x[(k+1)%count][j]-x[k][j];
        const auto ee = dot(edge,edge), re = dot(r,edge)/ee, ce = dot(chord,edge)/ee;
        Vector rp{}, cp{};
        for (unsigned j = 0; j < nDim; ++j) { rp[j]=r[j]-re*edge[j]; cp[j]=chord[j]-ce*edge[j]; }
        for (const auto level : levels)
          roots(dot(cp,cp),2*dot(rp,cp),dot(rp,rp)-static_cast<long double>(level)*level);
      }
      if (nDim == 3) {
        Vector u{},v{},normal{},r{};
        for (unsigned j = 0; j < 3; ++j) { u[j]=x[1][j]-x[0][j]; v[j]=x[2][j]-x[0][j]; r[j]=a[j]-x[0][j]; }
        normal={u[1]*v[2]-u[2]*v[1],u[2]*v[0]-u[0]*v[2],u[0]*v[1]-u[1]*v[0]};
        const auto nn=std::sqrt(dot(normal,normal)), slope=dot(chord,normal), offset=dot(r,normal);
        if (slope) for (const auto level : levels) {
          retain((level*nn-offset)/slope); retain((-level*nn-offset)/slope);
        }
      }
    }
    auto validRoot=[&](double t) {
      su2double x[3]{};for(unsigned j=0;j<nDim;++j)x[j]=(1-t)*a[j]+t*b[j];
      const double actual=SU2_TYPE::GetValue(Evaluate(wall,x,1.).distance);
      double coordinateScale=0;
      for(unsigned j=0;j<nDim;++j)coordinateScale=std::max({coordinateScale,std::abs(a[j]),std::abs(b[j])});
      const double roundoff=64*std::numeric_limits<double>::epsilon()*coordinateScale;
      for(auto level:levels)
        if(std::abs(actual-level)<=std::max(roundoff,1e-10*std::max(h0,level)))return true;
      return false;
    };
    cuts.erase(std::remove_if(cuts.begin()+wallCutsBegin,cuts.end(),[&](double t){return !validRoot(t);}),cuts.end());
  }
  std::sort(cuts.begin(),cuts.end());
  cuts.erase(std::unique(cuts.begin(),cuts.end()),cuts.end());
  return cuts;
}

std::vector<CBoundaryLayerMetric::PointSample> CBoundaryLayerMetric::SamplePoints(const std::vector<su2double>& coord) {
  std::vector<PointSample> result;
  for (auto point = 0ul; point < coord.size() / nDim; ++point) {
    const auto* X = &coord[point * nDim];
    for (unsigned short wall = 0; wall < walls.size(); ++wall) {
      const auto& d = *data[wall];
      bool inside = true;
      for (unsigned short k = 0; k < nDim; ++k) inside = inside && X[k] >= d.low[k] && X[k] <= d.high[k];
      if (!inside || !d.adt) continue;
      const auto sample = Evaluate(wall, X, 1.0);
      if (sample.weight > 0.0) result.push_back({point, wall, sample});
    }
  }
  return result;
}

std::vector<CBoundaryLayerMetric::IntegrationPoint2D> CBoundaryLayerMetric::IntegrationRule2D(
    const std::array<std::array<double, 2>, 3>& triangle, double bandRatio) {
  return IntegrationRule2DImpl(triangle, bandRatio, 0);
}

std::vector<CBoundaryLayerMetric::IntegrationPoint2D> CBoundaryLayerMetric::IntegrationRule2DImpl(
    const std::array<std::array<double, 2>, 3>& triangle, double bandRatio, unsigned depth) {
  if (nDim != 2 || !(bandRatio > 1.0)) throw std::invalid_argument("Invalid native BL quadrature.");
  using Bary = std::array<double, 3>;
  using Polygon = std::vector<Bary>;
  auto position = [&](const Bary& b) {
    std::array<su2double, 2> p{};
    for (unsigned k = 0; k < 3; ++k)
      for (unsigned d = 0; d < 2; ++d) p[d] += b[k] * triangle[k][d];
    return p;
  };
  bool subdivide = false;
  auto cross = [](const auto& a, const auto& b, const auto& c) {
    return (b[0]-a[0])*(c[1]-a[1])-(b[1]-a[1])*(c[0]-a[0]);
  };
  auto pointSegment = [](const auto& p, const auto& a, const auto& b) {
    const double xx=b[0]-a[0], yy=b[1]-a[1], length=xx*xx+yy*yy;
    const auto t=std::clamp(((p[0]-a[0])*xx+(p[1]-a[1])*yy)/length,0.,1.);
    return std::hypot(p[0]-a[0]-t*xx,p[1]-a[1]-t*yy);
  };
  auto inside = [&](const auto& p) {
    double low=0, high=0;
    for(unsigned k=0;k<3;++k) {const auto x=cross(triangle[k],triangle[(k+1)%3],p);low=std::min(low,x);high=std::max(high,x);}
    return low==0 || high==0;
  };
  for (unsigned short wall=0;wall<walls.size() && !subdivide;++wall) {
    if (!data[wall]->adt) continue;
    const double thickness=SU2_TYPE::GetValue(walls[wall].layer.thickness);
    std::array<double,3> d;
    for(unsigned k=0;k<3;++k) {const su2double x[2]={triangle[k][0],triangle[k][1]};d[k]=SU2_TYPE::GetValue(Evaluate(wall,x,1.).distance);}
    const double minimum=*std::min_element(d.begin(),d.end());
    if (minimum >= thickness) {
      // A curved wall can cross the integration band between donor vertices. Check segments, not nodal weights.
      const auto& wallGeometry=walls[wall];
      for(size_t face=0;face<wallGeometry.conn.size()/2 && !subdivide;++face) {
        std::array<double,2> a,b;
        for(unsigned j=0;j<2;++j) {a[j]=SU2_TYPE::GetValue(wallGeometry.coord[2*wallGeometry.conn[2*face]+j]);b[j]=SU2_TYPE::GetValue(wallGeometry.coord[2*wallGeometry.conn[2*face+1]+j]);}
        bool separated=false;
        for(unsigned j=0;j<2;++j) {
          double low=triangle[0][j],high=low;
          for(unsigned k=1;k<3;++k) {low=std::min(low,triangle[k][j]);high=std::max(high,triangle[k][j]);}
          separated=separated || std::max(a[j],b[j])+thickness<low || std::min(a[j],b[j])-thickness>high;
        }
        if(separated) continue;
        double gap=inside(a)||inside(b)?0:std::numeric_limits<double>::max();
        for(unsigned k=0;k<3;++k) {
          const auto& u=triangle[k];const auto& v=triangle[(k+1)%3];
          gap=std::min({gap,pointSegment(u,a,b),pointSegment(a,u,v),pointSegment(b,u,v)});
          if(cross(a,b,u)*cross(a,b,v)<0 && cross(u,v,a)*cross(u,v,b)<0) gap=0;
        }
        subdivide=gap < thickness*(1-1e-12);
      }
    } else {
      // Refine changing nearest-face geometry before constructing distance bands. Even an outside midpoint
      // can reveal a wrong near-wall distance slope from a far vertex closest to another face.
      const double tolerance=.1*std::max(SU2_TYPE::GetValue(walls[wall].layer.firstHeight),minimum);
      for(unsigned k=0;k<3;++k) {
        const unsigned next=(k+1)%3;
        const su2double x[2]={.5*(triangle[k][0]+triangle[next][0]),.5*(triangle[k][1]+triangle[next][1])};
        const double exact=SU2_TYPE::GetValue(Evaluate(wall,x,1.).distance);
        subdivide=subdivide || (std::abs(exact-.5*(d[k]+d[next]))>tolerance);
      }
    }
  }
  if (subdivide) {
    if(depth>=16) SU2_MPI::Error("Geometric BL quadrature failed to resolve a wall band after 16 subdivisions.",CURRENT_FUNCTION);
    const Bary vertices[6] = {{{1,0,0}},{{0,1,0}},{{0,0,1}},{{.5,.5,0}},{{0,.5,.5}},{{.5,0,.5}}};
    const unsigned children[4][3]={{0,3,5},{3,1,4},{5,4,2},{3,4,5}};
    std::vector<IntegrationPoint2D> refined;
    for(const auto& child:children) {
      std::array<std::array<double,2>,3> coords;
      for(unsigned k=0;k<3;++k) {const auto x=position(vertices[child[k]]);for(unsigned j=0;j<2;++j)coords[k][j]=SU2_TYPE::GetValue(x[j]);}
      auto rule=IntegrationRule2DImpl(coords,bandRatio,depth+1);
      for(auto& point:rule) {
        Bary parent{};
        for(unsigned k=0;k<3;++k)for(unsigned j=0;j<3;++j)parent[j]+=point.barycentric[k]*vertices[child[k]][j];
        point.barycentric=parent;refined.push_back(std::move(point));
      }
    }
    return refined;
  }
  std::vector<Polygon> pieces{{{{1,0,0}}, {{0,1,0}}, {{0,0,1}}}};
  for (unsigned short wall = 0; wall < walls.size(); ++wall) {
    if (!data[wall]->adt) continue;
    std::array<double, 3> distance{};
    for (unsigned k = 0; k < 3; ++k) {
      const su2double x[2] = {triangle[k][0], triangle[k][1]};
      distance[k] = SU2_TYPE::GetValue(Evaluate(wall, x, 1.0).distance);
    }
    const auto& layer = walls[wall].layer;
    const double h0 = SU2_TYPE::GetValue(layer.firstHeight), thickness = SU2_TYPE::GetValue(layer.thickness);
    if (*std::min_element(distance.begin(), distance.end()) >= thickness) continue;
    auto interpolatedDistance = [&](const Bary& b) {
      return b[0]*distance[0] + b[1]*distance[1] + b[2]*distance[2];
    };
    std::vector<double> levels;
    // Geometric bands resolve h0 even when no interior donor vertex lies inside the layer.
    for (double level = h0; level < .9*thickness; level *= bandRatio) levels.push_back(level);
    levels.push_back(.9*thickness);
    for (unsigned k = 1; k <= 16; ++k) levels.push_back((.9+.1*k/16)*thickness);
    for (const auto level : levels) {
      std::vector<Polygon> next;
      for (const auto& piece : pieces) {
        Polygon low, high;
        for (size_t k = 0; k < piece.size(); ++k) {
          const auto& a = piece[k]; const auto& b = piece[(k+1)%piece.size()];
          const auto da = interpolatedDistance(a), db = interpolatedDistance(b);
          (da <= level ? low : high).push_back(a);
          if ((da < level && db > level) || (da > level && db < level)) {
            const auto t = (level-da)/(db-da);
            Bary cut{};
            for (unsigned j = 0; j < 3; ++j) cut[j] = a[j] + t*(b[j]-a[j]);
            low.push_back(cut); high.push_back(cut);
          }
        }
        if (low.size() >= 3) next.push_back(std::move(low));
        if (high.size() >= 3) next.push_back(std::move(high));
      }
      pieces = std::move(next);
    }
  }
  std::vector<IntegrationPoint2D> rule;
  for (const auto& piece : pieces) {
    for (size_t k = 1; k + 1 < piece.size(); ++k) {
      const Bary vertices[3] = {piece[0], piece[k], piece[k+1]};
      const auto a = position(vertices[0]), b = position(vertices[1]), c = position(vertices[2]);
      const double area = .5 * std::abs(SU2_TYPE::GetValue((b[0]-a[0])*(c[1]-a[1])-(b[1]-a[1])*(c[0]-a[0])));
      if (!(area > 0)) continue;
      for (unsigned corner = 0; corner < 3; ++corner) {
        Bary bary{};
        for (unsigned j = 0; j < 3; ++j)
          for (unsigned v = 0; v < 3; ++v) bary[j] += vertices[v][j] * (v == corner ? 2./3 : 1./6);
        const auto x = position(bary);
        rule.push_back({bary, area/3, SamplePoints({x[0],x[1]})});
      }
    }
  }
  return rule;
}

void CBoundaryLayerMetric::ApplySample(unsigned short wall, const Sample& sample, Tensor& metric,
                                      su2double coreEigenvalue, std::vector<WallReport>* reports,
                                      bool useTangentialFloor) {
  if (!(sample.weight > 0.0)) return;
  Tensor faded = sample.full;
  if (sample.weight < 1.0) {
    if (nDim == 3)
      Spectral(3, sample.logarithm.m, faded.m, [&](su2double value) {
        return exp(sample.weight*value + (1-sample.weight)*log(coreEigenvalue));
      });
    else
      Spectral(2, sample.full.m, faded.m, [&](su2double value) {
        return exp(sample.weight*log(value) + (1-sample.weight)*log(coreEigenvalue));
      });
  }
  su2double C[3][3] = {{0.0}};
  CSolver::IntersectMetrics(nDim, metric.m, faded.m, C);
  bool changed = false;
  for (unsigned short i = 0; i < nDim; ++i)
    for (unsigned short j = 0; j < nDim; ++j) {
      const auto value = 0.5 * (C[i][j] + C[j][i]);
      changed = changed || (value != metric.m[i][j]);
      metric.m[i][j] = value;
    }
  su2double vec[3][3], val[3], work[3];
  bool floor = false;
  if (reports || (useTangentialFloor && sample.weight == 1.0)) {
    CBlasStructure::EigenDecomposition(sample.full.m, vec, val, nDim, work);
    floor = useTangentialFloor && sample.weight == 1.0 && TangentialFloor(sample, val, data[wall]->h0, metric);
  }
  if (reports) {
    auto& report = reports->at(wall);
    ++report.nPoint; report.nChanged += changed; report.nFloor += floor;
    report.maxAspectRatio = max(report.maxAspectRatio,
        sqrt(*std::max_element(val, val + nDim) / *std::min_element(val, val + nDim)));
  }
}

void CBoundaryLayerMetric::ApplyPoint(const su2double* X, Tensor& metric, su2double coreEigenvalue,
                                      std::vector<WallReport>* reports, bool useTangentialFloor) {
  for (unsigned short wall = 0; wall < walls.size(); ++wall) {
    const auto& d = *data[wall];
    bool inside = true;
    for (unsigned short k = 0; k < nDim; ++k) inside = inside && X[k] >= d.low[k] && X[k] <= d.high[k];
    if (!inside || !d.adt) continue;
    ApplySample(wall, Evaluate(wall, X, 1.0), metric, coreEigenvalue, reports, useTangentialFloor);
  }
}

bool CBoundaryLayerMetric::TangentialFloor(const Sample& sample, const su2double* wallEigenvalues, su2double h0,
                                           Tensor& M) const {
  /*--- Band: up to floorFraction x the largest tangential size of the wall metric, at least h0. The smallest
   *    eigenvalue of the wall metric is a tangential one (the normal one is 1 / hn^2, the largest). ---*/
  const su2double htMax = 1.0 / sqrt(*std::min_element(wallEigenvalues, wallEigenvalues + nDim));
  if (sample.distance > max(h0, floorFraction * htMax)) return false;

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
  auto toFrame = [&](const su2double (&A)[3][3], su2double (&B)[3][3]) {
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
    U[0][0] = min(At[0][0], Wt[0][0]);
  } else {
    su2double iA[3][3] = {{0.0}}, iW[3][3] = {{0.0}}, I[3][3] = {{0.0}};
    Spectral(2, At, iA, [](su2double v) { return 1.0 / v; });
    Spectral(2, Wt, iW, [](su2double v) { return 1.0 / v; });
    CSolver::IntersectMetrics(2, iA, iW, I);
    Spectral(2, I, U, [](su2double v) { return 1.0 / v; });
  }
  su2double B[3][3] = {{0.0}};
  for (unsigned short i = 0; i < nt; ++i)
    for (unsigned short j = 0; j < nt; ++j) B[i][j] = U[i][j];
  B[nt][nt] = A[nt][nt];

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
  return true;
}
