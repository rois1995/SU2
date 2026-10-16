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
#include <iterator>
#include <map>
#include <numeric>
#include <queue>
#include <set>
#include <functional>

#include "../../../Common/include/CConfig.hpp"
#include "../../../Common/include/parallelization/CPassiveComm.hpp"
#include "../../../Common/include/adaptation/CNativeImport2D.hpp"
#include "../../../Common/include/adaptation/CNativeImport3D.hpp"
#include "../../../Common/include/adaptation/CNativeMetric3D.hpp"
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
void WallMetric3D(const Mat3& size, CBoundaryLayerMetric::Sample& sample,double curvature=0,
                  const std::array<double,3>* curvedDirection=nullptr) {
#if defined(CODI_FORWARD_TYPE) || defined(CODI_REVERSE_TYPE)
  // Native3D is primal-only; retain the existing active wall-metric path for other callers.
  if(curvature>0)throw std::runtime_error("Native3D analytic BL metric is primal-only.");
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
  auto l0=std::log(scale)+std::log(largest),l1=std::log(scale)+std::log(smallest);
  if(curvature>0) {
    if(!curvedDirection) {
      // Sphere: cap both tangent sizes, preserving every already finer direction.
      l0=-std::log(std::max(std::exp(-l0),(long double)curvature));
      l1=-std::log(std::max(std::exp(-l1),(long double)curvature));
    } else {
      long double d0=0,d1=0;for(unsigned i=0;i<3;++i){d0+=(*curvedDirection)[i]*u[i];d1+=(*curvedDirection)[i]*v[i];}
      const auto norm=std::hypot(d0,d1);if(!(norm>1e-12L))throw std::runtime_error("Unresolved native3D curvature tangent.");
      d0/=norm;d1/=norm;
      const auto covariance=scale*(aa*d0*d0+2*bb*d0*d1+cc*d1*d1);
      if(!(covariance>0))throw std::runtime_error("Invalid native3D curvature tangent size.");
      const auto alpha=std::max(0.L,(long double)curvature-1/covariance);
      if(alpha>0) {
        // Rank-one addition enforces M >= curvature*d*d^T without relaxing the old metric.
        const auto m0=std::exp(-l0),m1=std::exp(-l1);
        long double a=m0*cosine*cosine+m1*sine*sine+alpha*d0*d0;
        long double b=(m0-m1)*cosine*sine+alpha*d0*d1;
        long double c=m0*sine*sine+m1*cosine*cosine+alpha*d1*d1;
        const auto ms=std::max({std::abs(a),std::abs(b),std::abs(c)});a/=ms;b/=ms;c/=ms;
        const auto large=.5L*(a+c+std::hypot(a-c,2*b)),small=std::fma(a,c,-b*b)/large;
        if(!(small>0))throw std::runtime_error("Singular native3D curvature tangent metric.");
        const auto angle=.5L*std::atan2(2*b,a-c),co=std::cos(angle),si=std::sin(angle);
        for(unsigned i=0;i<3;++i){q[i]=co*u[i]+si*v[i];r[i]=-si*u[i]+co*v[i];}
        l0=-std::log(ms)-std::log(large);l1=-std::log(ms)-std::log(small);
      }
    }
  }
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
  double maximum_wall_eigenvalue=0;
  double proxy_error=0; /*!< Broad-phase envelope only; never a query distance or metric. */
  std::vector<CBoundaryLayerMetric::Tensor> vertexLog; /*!< \brief Logarithm of the size tensor of each vertex. */
};

CBoundaryLayerMetric::~CBoundaryLayerMetric() = default;
size_t CBoundaryLayerMetric::StorageBytes() const {
  using namespace transfer_memory;
  size_t bytes=Add(sizeof(*this),Bytes(walls),Bytes(data));
  for(const auto& wall:walls)bytes=Add(bytes,wall.layer.marker.capacity()+1,Bytes(wall.coord),Bytes(wall.conn));
  for(const auto& ptr:data)if(ptr) {
    const auto& d=*ptr;
    bytes=Add(bytes,sizeof(d),Bytes(d.elem),Bytes(d.length),Bytes(d.segments),Bytes(d.vertexNormal),
              Bytes(d.corner),Bytes(d.turn),Bytes(d.faceNormal),Bytes(d.edgeNormal),Bytes(d.vertexLog));
    for(const auto& row:d.segments)bytes=Add(bytes,Bytes(row));
    bytes=Add(bytes,Mul(d.grid.size(),sizeof(decltype(d.grid)::value_type)+4*sizeof(void*)));
    for(const auto& row:d.grid)bytes=Add(bytes,Bytes(row.second));
    if(d.adt)bytes=Add(bytes,d.adt->GetAllocatedBytes(),d.adt->RetainedWorkspaceBound(1));
  }
  return bytes;
}

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
    if(wall.authority && (nDim!=3 || !wall.authority->CompleteAnalytic(wall.authority_facet) ||
        !(wall.surface_tolerance>0) || !std::isfinite(wall.surface_tolerance)))
      throw std::invalid_argument("Invalid native3D analytic BL authority/geometry tolerance.");
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

    if(wall.authority) {
      const auto root=wall.authority->Canonical(wall.authority_facet);
      using TriangleKey=std::array<std::array<double,3>,3>;
      std::set<TriangleKey> expected,actual;
      for(const auto& entry:wall.authority->Facets())if(wall.authority->Canonical(entry.first)==root) {
        TriangleKey key;
        for(unsigned k=0;k<3;++k)key[k]={entry.second.v[k].p.x,entry.second.v[k].p.y,entry.second.v[k].p.z};
        std::sort(key.begin(),key.end());expected.insert(key);
      }
      const auto& g=wall.authority->Analytic().at(root);
      for(size_t at=0;at<d->elem.size();at+=3) {
        SU2Native3D::Tetrahedron points{};TriangleKey key;
        for(unsigned k=0;k<3;++k) {
          const auto* p=x(d->elem[at+k]);key[k]={SU2_TYPE::GetValue(p[0]),SU2_TYPE::GetValue(p[1]),SU2_TYPE::GetValue(p[2])};
          points[k]={key[k][0],key[k][1],key[k][2]};
        }
        std::sort(key.begin(),key.end());actual.insert(key);
        d->proxy_error=std::max(d->proxy_error,wall.authority->SurfaceError(root,{points[0],points[1],points[2]}));
        if(g.kind==SU2Native3D::AnalyticSurface::Kind::Disk)for(unsigned k=0;k<3;++k) {
          const auto a=points[k],b=points[(k+1)%3];
          const double ra=std::hypot(a.x-g.origin.x,a.y-g.origin.y,a.z-g.origin.z),rb=std::hypot(b.x-g.origin.x,b.y-g.origin.y,b.z-g.origin.z);
          if(std::abs(ra-g.extent.x)<=g.tolerance && std::abs(rb-g.extent.x)<=g.tolerance) {
            const double chord=std::hypot(a.x-b.x,a.y-b.y,a.z-b.z);
            const long double half=.5L*chord;
            const long double sagitta=half*half/(g.extent.x+std::sqrt(std::max(0.L,(long double)g.extent.x*g.extent.x-half*half)));
            d->proxy_error=std::max(d->proxy_error,double(sagitta));
          }
        }
      }
      if(actual!=expected || actual.size()!=d->elem.size()/3)throw std::invalid_argument("Native3D analytic BL proxy must cover exactly its complete original component.");
      for(unsigned j=0;j<3;++j){d->low[j]-=d->proxy_error;d->high[j]+=d->proxy_error;}
      double bound=1/std::pow(SU2_TYPE::GetValue(d->h0),2);
      for(const auto& logarithm:d->vertexLog) {
        su2double vectors[3][3],values[3],workspace[3];CBlasStructure::EigenDecomposition(logarithm.m,vectors,values,3,workspace);
        for(auto value:values)bound=std::max(bound,std::exp(-SU2_TYPE::GetValue(value)));
      }
      if(g.kind!=SU2Native3D::AnalyticSurface::Kind::Disk) {
        const long double epsilon=std::min(wall.surface_tolerance,.5*SU2_TYPE::GetValue(d->h0));
        const auto e=std::min(epsilon,(long double)g.extent.x);
        const auto chord=2*std::sqrt(e*(2*g.extent.x-e));
        bound=std::max(bound,double(std::pow(SU2Native3D::MaximumMetricEdge/chord,2)));
      }
      d->maximum_wall_eigenvalue=4*bound;
      if(!std::isfinite(d->maximum_wall_eigenvalue))throw std::runtime_error("Unrepresentable native3D curved BL metric bound.");
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
  SU2Native3D::PlanarReference::ClosestSurface smooth;
  su2double lookup[3]={X[0],X[1],nDim==3?X[2]:su2double(0.)};
  if(wall.authority) {
    smooth=wall.authority->ClosestAnalytic(wall.authority_facet,
        {SU2_TYPE::GetValue(X[0]),SU2_TYPE::GetValue(X[1]),SU2_TYPE::GetValue(X[2])});
    lookup[0]=smooth.point.x;lookup[1]=smooth.point.y;lookup[2]=smooth.point.z;
  }
  // Tangential interpolation belongs to the physical foot, not an interior proxy Voronoi branch.
  d.adt->DetermineNearestElement(lookup, adtDistance, markerID, id, rankID);

  su2double tangentSize[3][3] = {{0.0}};  // 3D: size tensor (squared sizes) on the tangent plane
  su2double ht = 0.0, tangent[3] = {0.0};  // 2D
  su2double geometricNormal[3] = {0.0};

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
    /*--- The ADT's tolerant triangle test is a seed, not an authoritative closest facet. ---*/
    const auto* query=wall.authority?lookup:X;
    auto closest=[&](unsigned long face,su2double (&weights)[3],su2double (&point)[3]) {
      const auto* t=&d.elem[3*face];ClosestPointTriangle(x(t[0]),x(t[1]),x(t[2]),query,weights);
      for(unsigned j=0;j<3;++j){point[j]=0;for(unsigned k=0;k<3;++k)point[j]+=weights[k]*x(t[k])[j];}
      su2double distance2=0;for(unsigned j=0;j<3;++j)distance2+=pow(query[j]-point[j],2);
      return distance2;
    };
    su2double w[3],projected[3];auto dist2=closest(id,w,projected);
    std::vector<unsigned long> candidates;
    d.adt->DetermineElementsWithinDistance(query,sqrt(dist2)+d.tolerance,candidates);
    std::sort(candidates.begin(),candidates.end());
    for(auto other:candidates)if(other!=id) {
      su2double weights[3],point[3];const auto distance2=closest(other,weights,point);
      if(distance2<dist2 || (distance2==dist2 && other<id)) {
        id=other;dist2=distance2;std::copy(weights,weights+3,w);std::copy(point,point+3,projected);
      }
    }
    const auto* t = &d.elem[3 * id];
    sample.distance = sqrt(dist2);
    if(wall.authority) {
      sample.distance=smooth.distance;
      projected[0]=smooth.point.x;projected[1]=smooth.point.y;projected[2]=smooth.point.z;
      geometricNormal[0]=smooth.normal.x;geometricNormal[1]=smooth.normal.y;geometricNormal[2]=smooth.normal.z;
    }

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
    if(wall.authority)n={smooth.normal.x,smooth.normal.y,smooth.normal.z};
    if (sample.distance > d.tolerance) {
      if(wall.authority)n={smooth.direction.x,smooth.direction.y,smooth.direction.z};
      else for (unsigned short iDim = 0; iDim < 3; ++iDim) n[iDim] = (X[iDim] - projected[iDim]) / sample.distance;
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
    double curvature=0;std::array<double,3> direction{};const std::array<double,3>* curvedDirection=nullptr;
    if(wall.authority) {
      const auto& g=wall.authority->Analytic().at(wall.authority_facet);
      if(g.kind!=SU2Native3D::AnalyticSurface::Kind::Disk) {
        const long double epsilon=std::min(wall.surface_tolerance,.5*SU2_TYPE::GetValue(d.h0));
        const long double radius=g.extent.x,e=std::min(epsilon,radius);
        const long double chord=2*std::sqrt(e*(2*radius-e));
        curvature=double(std::pow(SU2Native3D::MaximumMetricEdge/chord,2));
        if(g.kind==SU2Native3D::AnalyticSurface::Kind::Cylinder) {
          const double norm=std::hypot(g.axis.x,g.axis.y,g.axis.z);
          const double a[3]={g.axis.x/norm,g.axis.y/norm,g.axis.z/norm};
          direction[0]=a[1]*SU2_TYPE::GetValue(geometricNormal[2])-a[2]*SU2_TYPE::GetValue(geometricNormal[1]);
          direction[1]=a[2]*SU2_TYPE::GetValue(geometricNormal[0])-a[0]*SU2_TYPE::GetValue(geometricNormal[2]);
          direction[2]=a[0]*SU2_TYPE::GetValue(geometricNormal[1])-a[1]*SU2_TYPE::GetValue(geometricNormal[0]);
          curvedDirection=&direction;
        }
      }
    }
    WallMetric3D(tangentSize,sample,curvature,curvedDirection);
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
    bool faceted=false;std::set<uint64_t> components;
    for(const auto& face:reference.facets) {
      if(reference.marker_names.at(face.marker)!=wall.layer.marker) continue;
      if(reference.original->Analytic().count(face.id))components.insert(reference.original->Canonical(face.id));
      else faceted=true;
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
    if(!components.empty()) {
      if(faceted || components.size()!=1)throw std::invalid_argument("Native3D analytic BL marker needs one complete component; split physical components into separate markers.");
      wall.authority=reference.original;wall.authority_facet=*components.begin();
      if(!wall.authority->CompleteAnalytic(wall.authority_facet))throw std::invalid_argument("Native3D analytic BL partial/graph component is unsupported.");
      wall.surface_tolerance=SU2_TYPE::GetValue(config.GetAdap_Hausd());
    }
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
bool MayIntersectSmoothBand(const BLTetrahedron& tet,const SU2Native3D::PlanarReference& authority,
                            uint64_t facet,double thickness) {
  if(BLDeterminant(tet[0],tet[1],tet[2],tet[3])==0)return false;
  const auto& g=authority.Analytic().at(facet);
  using V=std::array<long double,3>;
  auto dot=[](const V& a,const V& b){return a[0]*b[0]+a[1]*b[1]+a[2]*b[2];};
  auto cross=[](const V& a,const V& b){return V{a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]};};
  auto norm=[&](const V& a){return std::sqrt(dot(a,a));};
  V axis{g.axis.x,g.axis.y,g.axis.z};const auto scale=norm(axis);for(auto& x:axis)x/=scale;
  std::array<V,4> radial;long double zlo=INFINITY,zhi=-INFINITY,rmin=INFINITY,rmax=0,magnitude=g.extent.x;
  for(unsigned k=0;k<4;++k) {
    radial[k]={(long double)tet[k][0]-g.origin.x,(long double)tet[k][1]-g.origin.y,(long double)tet[k][2]-g.origin.z};
    for(unsigned j=0;j<3;++j)magnitude=std::max(magnitude,std::abs((long double)tet[k][j]));
    if(g.kind!=SU2Native3D::AnalyticSurface::Kind::Sphere) {
      const auto z=dot(radial[k],axis);zlo=std::min(zlo,z);zhi=std::max(zhi,z);
      for(unsigned j=0;j<3;++j)radial[k][j]-=z*axis[j];
    }
    rmin=std::min(rmin,norm(radial[k]));rmax=std::max(rmax,norm(radial[k]));
  }
  if(g.kind==SU2Native3D::AnalyticSurface::Kind::Sphere) {
    SU2Native3D::Tetrahedron source;for(unsigned k=0;k<4;++k)source[k]={tet[k][0],tet[k][1],tet[k][2]};
    if(SU2Native3D::Orientation(source[0],source[1],source[2],source[3])<0)std::swap(source[0],source[1]);
    std::array<long double,4> weights;
    if(SU2Native3D::Barycentric(source,g.origin,weights))rmin=0;
    else for(unsigned skip=0;skip<4;++skip) {
      unsigned v[3],at=0;for(unsigned k=0;k<4;++k)if(k!=skip)v[at++]=k;
      su2double a[3],b[3],c[3],p[3]={g.origin.x,g.origin.y,g.origin.z},w[3];
      for(unsigned j=0;j<3;++j){a[j]=tet[v[0]][j];b[j]=tet[v[1]][j];c[j]=tet[v[2]][j];}
      ClosestPointTriangle(a,b,c,p,w);V d{};
      for(unsigned j=0;j<3;++j)d[j]=SU2_TYPE::GetValue(w[0]*a[j]+w[1]*b[j]+w[2]*c[j])-p[j];
      rmin=std::min(rmin,norm(d));
    }
  } else {
    for(unsigned i=0;i<4;++i)for(unsigned j=i+1;j<4;++j) {
      V edge{};for(unsigned k=0;k<3;++k)edge[k]=radial[j][k]-radial[i][k];
      const auto square=dot(edge,edge);const auto t=square>0?std::clamp(-dot(radial[i],edge)/square,0.L,1.L):0.L;
      V closest{};for(unsigned k=0;k<3;++k)closest[k]=radial[i][k]+t*edge[k];rmin=std::min(rmin,norm(closest));
      for(unsigned third=j+1;third<4;++third) {
        const auto a=dot(cross(radial[i],radial[j]),axis),b=dot(cross(radial[j],radial[third]),axis),c=dot(cross(radial[third],radial[i]),axis);
        const auto pad=128*std::numeric_limits<long double>::epsilon()*magnitude*magnitude;
        // A projected triangle containing the axis puts zero in the convex radial range.
        if((a>=-pad && b>=-pad && c>=-pad)||(a<=pad && b<=pad && c<=pad))rmin=0;
      }
    }
  }
  const long double padding=256*std::numeric_limits<double>::epsilon()*magnitude;
  rmin=std::max(0.L,rmin-padding);rmax+=padding;zlo-=padding;zhi+=padding;
  const auto radialGap=g.kind==SU2Native3D::AnalyticSurface::Kind::Disk ?
      std::max(0.L,rmin-g.extent.x) : std::max({0.L,rmin-g.extent.x,(long double)g.extent.x-rmax});
  const auto axialGap=g.kind==SU2Native3D::AnalyticSurface::Kind::Sphere ? 0.L :
      std::max({0.L,zlo-(g.kind==SU2Native3D::AnalyticSurface::Kind::Cylinder?g.extent.y:0.),-zhi});
  return std::hypot(radialGap,axialGap)<thickness;
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


}

std::vector<CBoundaryLayerMetric::IntegrationPoint3D> CBoundaryLayerMetric::IntegrationRule3D(
    const std::array<std::array<double,3>,4>& tetrahedron,double bandRatio,IntegrationDensity3D density,bool* sensorDependent) {
  if(nDim!=3 || !std::isfinite(bandRatio) || !(bandRatio>1))
    throw std::invalid_argument("Invalid native3D BL volume quadrature.");
  for(const auto& p:tetrahedron)for(auto x:p)if(!std::isfinite(x))
    throw std::invalid_argument("Nonfinite native3D BL integration cell.");
  if(BLDeterminant(tetrahedron[0],tetrahedron[1],tetrahedron[2],tetrahedron[3])==0)
    throw std::invalid_argument("Degenerate native3D BL integration cell.");
  const std::array<std::array<double,4>,4> bary{{{{1,0,0,0}},{{0,1,0,0}},{{0,0,1,0}},{{0,0,0,1}}}};
  using SupportFace=SupportFace3D;
  std::vector<SupportFace> support;
  bool hiddenSupport=false;
  for(unsigned short wall=0;wall<walls.size();++wall) {
    const auto& d=*data[wall];if(!d.adt)continue;
    const double thickness=SU2_TYPE::GetValue(d.thickness)+d.proxy_error;
    if(walls[wall].authority && !MayIntersectSmoothBand(tetrahedron,*walls[wall].authority,walls[wall].authority_facet,SU2_TYPE::GetValue(d.thickness)))continue;
    bool hidden=true;
    if(!walls[wall].authority)for(const auto& p:tetrahedron){const su2double x[3]={p[0],p[1],p[2]};hidden=hidden && Evaluate(wall,x,1.).distance>=thickness;}
    su2double low[3],high[3];
    for(unsigned j=0;j<3;++j) {
      double a=tetrahedron[0][j],b=a;for(const auto& p:tetrahedron){a=std::min(a,p[j]);b=std::max(b,p[j]);}
      low[j]=a-thickness;high[j]=b+thickness;
    }
    std::vector<unsigned long> faces;d.adt->DetermineIntersectingElements(low,high,faces);
    std::sort(faces.begin(),faces.end());
    for(auto f:faces) {
      SupportFace face;face.thickness=thickness;face.wall=wall;
      for(unsigned k=0;k<3;++k)for(unsigned j=0;j<3;++j)
        face.vertices[k][j]=SU2_TYPE::GetValue(walls[wall].coord[3*d.elem[3*f+k]+j]);
      if(!MayIntersectWallBand(tetrahedron,face.vertices,thickness*(1-1e-12)))continue;
      support.push_back(face);hiddenSupport=hiddenSupport || hidden;
      if(walls[wall].authority)break; // One frame seed; smooth support is certified by the whole component.
    }
  }
  if(sensorDependent)*sensorDependent=hiddenSupport;
  auto probe=[&](const IntegrationPoint3D& point) {
    double value=0;
    if(density)value=density(point);
    else {
      Tensor metric;for(unsigned j=0;j<3;++j)metric.m[j][j]=1;
      for(const auto& sample:point.samples)ApplySample(sample.wall,sample.sample,metric,1.,nullptr,false);
      su2double vectors[3][3],eigenvalues[3],workspace[3];
      CBlasStructure::EigenDecomposition(metric.m,vectors,eigenvalues,3,workspace);
      value=SU2_TYPE::GetValue(sqrt(eigenvalues[0]*eigenvalues[1]*eigenvalues[2]));
    }
    if(!(value>0) || !std::isfinite(value))throw std::runtime_error("Native3D BL quadrature density is nonfinite/nonpositive.");
    return value;
  };
  // Positive radial fans avoid a spherical silhouette in parallel-chord coordinates.
  SU2Native3D::Tetrahedron fanSource;
  for(unsigned k=0;k<4;++k)fanSource[k]={tetrahedron[k][0],tetrahedron[k][1],tetrahedron[k][2]};
  const bool fanReversed=SU2Native3D::Orientation(fanSource[0],fanSource[1],fanSource[2],fanSource[3])<0;
  if(fanReversed)std::swap(fanSource[1],fanSource[2]);
  for(const auto& wall:walls)if(wall.authority) {
    const auto& g=wall.authority->Analytic().at(wall.authority_facet);
    std::array<long double,4> centerWeights;
    if(g.kind!=SU2Native3D::AnalyticSurface::Kind::Sphere || !SU2Native3D::Barycentric(fanSource,g.origin,centerWeights))continue;
    if(fanReversed)std::swap(centerWeights[1],centerWeights[2]);
    const BLPoint3D center{g.origin.x,g.origin.y,g.origin.z};
    const double radialNodes[5]={-.90617984593866399280,-.53846931010568309104,0,.53846931010568309104,.90617984593866399280};
    const double radialWeights[5]={.23692688505618908751,.47862867049936646804,128./225,.47862867049936646804,.23692688505618908751};
    const double lowNodes[3]={-.77459666924148337704,0,.77459666924148337704},lowWeights[3]={5./9,8./9,5./9};
    const double angular[7][3]={{1./3,1./3,1./3},{.059715871789770,.470142064105115,.470142064105115},
      {.470142064105115,.059715871789770,.470142064105115},{.470142064105115,.470142064105115,.059715871789770},
      {.797426985353087,.101286507323456,.101286507323456},{.101286507323456,.797426985353087,.101286507323456},
      {.101286507323456,.101286507323456,.797426985353087}};
    const double angularWeights[7]={.225,.132394152788506,.132394152788506,.132394152788506,.125939180544827,.125939180544827,.125939180544827};
    using Panel=std::array<std::array<double,4>,3>;
    std::vector<IntegrationPoint3D> radialRule;size_t radialWork=0;
    auto position=[&](const std::array<double,4>& b){BLPoint3D p{};for(unsigned k=0;k<4;++k)for(unsigned j=0;j<3;++j)p[j]+=b[k]*tetrahedron[k][j];return p;};
    std::function<void(const Panel&,unsigned)> refineAngular;
    refineAngular=[&](const Panel& panel,unsigned depth) {
      if(++radialWork>16384)throw std::runtime_error("Native3D spherical-fan work budget exhausted.");
      const BLTetrahedron region{center,position(panel[0]),position(panel[1]),position(panel[2])};
      const double volume=std::abs(BLDeterminant(region[0],region[1],region[2],region[3]))/6;if(!(volume>0))return;
      auto integrate=[&](bool high,std::vector<IntegrationPoint3D>& points,double& error) {
        double value=0;
        for(unsigned at=0;at<(high?7u:3u);++at) {
          std::array<double,4> end{};
          for(unsigned k=0;k<4;++k)for(unsigned j=0;j<3;++j)
            end[k]+=panel[j][k]*(high?angular[at][j]:(at==j?2./3:1./6));
          const auto target=position(end);const auto cuts=EdgeBreakpoints(center,target,std::max(2.,bandRatio),4);
          const double jacobian=3*volume*(high?angularWeights[at]:1./3);
          auto gauss=[&](double left,double right,unsigned order,std::vector<IntegrationPoint3D>& samples) {
            double sum=0;const double half=.5*(right-left),middle=.5*(right+left);
            for(unsigned p=0;p<order;++p) {
              const double t=middle+half*(order==5?radialNodes[p]:lowNodes[p]);std::array<double,4> parent{};
              for(unsigned k=0;k<4;++k)parent[k]=double((1-t)*centerWeights[k]+t*end[k]);
              auto x=position(parent);SetBLSourceQuery(tetrahedron,fanSource,fanReversed,parent,x);
              IntegrationPoint3D point{parent,jacobian*half*(order==5?radialWeights[p]:lowWeights[p])*t*t,SamplePoints({x[0],x[1],x[2]})};
              sum+=point.weight*probe(point);samples.push_back(std::move(point));
            }
            return sum;
          };
          std::function<void(double,double,unsigned)> refineRadius;
          refineRadius=[&](double left,double right,unsigned level) {
            std::vector<IntegrationPoint3D> lower,upper;const auto a=gauss(left,right,3,lower),b=gauss(left,right,5,upper);
            if(std::abs(a-b)<=1e-5*b) {
              if(radialRule.size()+points.size()+lower.size()>65536)throw std::runtime_error("Native3D spherical-fan point budget exhausted.");
              error+=std::abs(a-b);value+=a;points.insert(points.end(),std::make_move_iterator(lower.begin()),std::make_move_iterator(lower.end()));return;
            }
            if(++radialWork>16384)throw std::runtime_error("Native3D spherical radial work budget exhausted.");
            const double middle=.5*(left+right);
            if(level>=32 || middle==left || middle==right)throw std::runtime_error("Unresolved native3D spherical radial density.");
            refineRadius(left,middle,level+1);refineRadius(middle,right,level+1);
          };
          for(size_t k=1;k<cuts.size();++k)refineRadius(cuts[k-1],cuts[k],0);
        }
        return value;
      };
      std::vector<IntegrationPoint3D> low,high;double lowerError=0,upperError=0;
      const auto a=integrate(false,low,lowerError),b=integrate(true,high,upperError);
      std::vector<bool> sampled(walls.size(),false);
      for(const auto* points:{&low,&high})for(const auto& point:*points)for(const auto& s:point.samples)sampled[s.wall]=true;
      bool unresolved=false;
      for(unsigned w=0;w<walls.size();++w)if(!sampled[w] && data[w]->adt) {
        if(walls[w].authority)unresolved=unresolved || MayIntersectSmoothBand(region,*walls[w].authority,walls[w].authority_facet,SU2_TYPE::GetValue(data[w]->thickness));
        else {
          const double thickness=SU2_TYPE::GetValue(data[w]->thickness);su2double lo[3],hi[3];
          for(unsigned j=0;j<3;++j){double first=region[0][j],last=first;for(const auto& x:region){first=std::min(first,x[j]);last=std::max(last,x[j]);}lo[j]=first-thickness;hi[j]=last+thickness;}
          std::vector<unsigned long> faces;data[w]->adt->DetermineIntersectingElements(lo,hi,faces);
          for(auto f:faces){std::array<BLPoint3D,3> face;for(unsigned k=0;k<3;++k)for(unsigned j=0;j<3;++j)face[k][j]=SU2_TYPE::GetValue(walls[w].coord[3*data[w]->elem[3*f+k]+j]);unresolved=unresolved || MayIntersectWallBand(region,face,thickness*(1-1e-12));}
        }
      }
      if(!unresolved && std::abs(a-b)+lowerError+upperError<=1e-4*b) {
        if(radialRule.size()+high.size()>65536)throw std::runtime_error("Native3D spherical-fan point budget exhausted.");
        radialRule.insert(radialRule.end(),std::make_move_iterator(high.begin()),std::make_move_iterator(high.end()));return;
      }
      if(depth>=32)throw std::runtime_error("Unresolved native3D spherical angular density/support.");
      unsigned first=0,last=1;double longest=0;
      for(unsigned i=0;i<3;++i)for(unsigned j=i+1;j<3;++j){const auto x=position(panel[i]),y=position(panel[j]);double d=0;for(unsigned k=0;k<3;++k)d+=(x[k]-y[k])*(x[k]-y[k]);if(d>longest){longest=d;first=i;last=j;}}
      std::array<double,4> middle{};for(unsigned k=0;k<4;++k)middle[k]=.5*(panel[first][k]+panel[last][k]);
      if(middle==panel[first] || middle==panel[last])throw std::runtime_error("Unrepresentable native3D spherical angular split.");
      for(auto vertex:{first,last}){auto child=panel;child[vertex]=middle;refineAngular(child,depth+1);}
    };
    for(unsigned skip=0;skip<4;++skip) {
      unsigned ids[3],at=0;for(unsigned k=0;k<4;++k)if(k!=skip)ids[at++]=k;
      const double volume=std::abs(BLDeterminant(center,tetrahedron[ids[0]],tetrahedron[ids[1]],tetrahedron[ids[2]]))/6;
      if(!(volume>0))continue;
      su2double a[3],b[3],c[3],origin[3]={g.origin.x,g.origin.y,g.origin.z},weights[3];
      for(unsigned j=0;j<3;++j){a[j]=tetrahedron[ids[0]][j];b[j]=tetrahedron[ids[1]][j];c[j]=tetrahedron[ids[2]][j];}
      ClosestPointTriangle(a,b,c,origin,weights);std::array<double,4> anchor{};for(unsigned j=0;j<3;++j)anchor[ids[j]]=SU2_TYPE::GetValue(weights[j]);
      for(unsigned j=0;j<3;++j){Panel face{anchor};face[1][ids[j]]=1;face[2][ids[(j+1)%3]]=1;refineAngular(face,0);}
    }
    if(sensorDependent)*sensorDependent=true;
    return radialRule;
  }
  using Triangle=std::array<std::array<double,4>,3>;
  struct ChordRule {
    Triangle entry,exit;
    std::array<std::array<double,2>,3> projection;
    std::array<long double,3> axial{};
    long double area;
  };
  std::vector<ChordRule> charts;
  std::vector<IntegrationPoint3D> rule;
  if(!hiddenSupport) {
    if(sensorDependent)*sensorDependent=bool(density);
    return IntegrationRule3DVisible(tetrahedron,support,bandRatio,probe);
  }
  SU2Native3D::Tetrahedron originalSource;
  for(unsigned k=0;k<4;++k)originalSource[k]={tetrahedron[k][0],tetrahedron[k][1],tetrahedron[k][2]};
  const bool reversed=SU2Native3D::Orientation(originalSource[0],originalSource[1],originalSource[2],originalSource[3])<0;
  if(reversed)std::swap(originalSource[1],originalSource[2]);
  using V=std::array<long double,3>;
  auto cross=[](const V& a,const V& b){return V{a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]};};
  auto delta=[](const BLPoint3D& a,const BLPoint3D& b){return V{(long double)a[0]-b[0],(long double)a[1]-b[1],(long double)a[2]-b[2]};};
  auto unit=[](V a){const auto length=std::hypot(a[0],a[1],a[2]);if(!(length>0))throw std::runtime_error("Degenerate native3D integration frame.");for(auto& x:a)x/=length;return a;};
  const auto& wall=support.front().vertices;
  const auto normal=unit(cross(delta(wall[1],wall[0]),delta(wall[2],wall[0])));
  V axis{};axis[std::min_element(normal.begin(),normal.end(),[](auto a,auto b){return std::abs(a)<std::abs(b);})-normal.begin()]=1;
  const auto tangent=unit(cross(normal,axis)),second=cross(normal,tangent);
  const SU2Native3D::AnalyticSurface* cylinder=nullptr;
  if(walls.size()==1 && walls[0].authority && walls[0].authority->CompleteAnalytic(walls[0].authority_facet)) {
    const auto& g=walls[0].authority->Analytic().at(walls[0].authority_facet);
    if(g.kind==SU2Native3D::AnalyticSurface::Kind::Cylinder) {
      const auto axial=unit({g.axis.x,g.axis.y,g.axis.z});bool inside=true;
      for(const auto& p:tetrahedron){const auto z=((long double)p[0]-g.origin.x)*axial[0]+((long double)p[1]-g.origin.y)*axial[1]+((long double)p[2]-g.origin.z)*axial[2];inside=inside && z>=0 && z<=g.extent.y;}
      if(inside)cylinder=&g;
    }
  }
  V direction{};bool conditioned=false;
  if(cylinder){direction=unit({cylinder->axis.x,cylinder->axis.y,cylinder->axis.z});conditioned=true;
    axis={};axis[std::min_element(direction.begin(),direction.end(),[](auto a,auto b){return std::abs(a)<std::abs(b);})-direction.begin()]=1;}

  // Nearly vertical source faces make projected barycentrics ill-conditioned after rotation.
  if(!cylinder)for(const auto tilt:std::array<std::array<double,2>,5>{{{{0,0}},{{.031,.017}},{{-.017,.031}},{{-.031,-.017}},{{.017,-.031}}}}) {
    auto candidate=normal;for(unsigned j=0;j<3;++j)candidate[j]+=tilt[0]*tangent[j]+tilt[1]*second[j];candidate=unit(candidate);
    bool valid=true;
    for(unsigned opposite=0;opposite<4;++opposite) {
      unsigned vertices[3],next=0;for(unsigned k=0;k<4;++k)if(k!=opposite)vertices[next++]=k;
      const auto face=unit(cross(delta(tetrahedron[vertices[1]],tetrahedron[vertices[0]]),delta(tetrahedron[vertices[2]],tetrahedron[vertices[0]])));
      long double cosine=0;for(unsigned j=0;j<3;++j)cosine+=face[j]*candidate[j];
      valid=valid && (cosine==0 || std::abs(cosine)>=1e-3);
    }
    if(valid){direction=candidate;conditioned=true;break;}
  }
  if(!conditioned)throw std::runtime_error("Native3D integration cannot condition the projected source faces.");
  const auto u=unit(cross(direction,axis)),v=cross(direction,u);
  const BLPoint3D projectionOrigin=cylinder?BLPoint3D{cylinder->origin.x,cylinder->origin.y,cylinder->origin.z}:tetrahedron[0];
  std::array<std::array<double,2>,4> projected{};
  for(unsigned k=0;k<4;++k)for(unsigned j=0;j<3;++j) {
    projected[k][0]+=double(((long double)tetrahedron[k][j]-projectionOrigin[j])*u[j]);
    projected[k][1]+=double(((long double)tetrahedron[k][j]-projectionOrigin[j])*v[j]);
  }
  struct FaceProjection { std::array<unsigned,3> vertices; bool entry; };
  std::vector<FaceProjection> faces;
  for(unsigned opposite=0;opposite<4;++opposite) {
    FaceProjection f;unsigned next=0;for(unsigned k=0;k<4;++k)if(k!=opposite)f.vertices[next++]=k;
    const auto a=projected[f.vertices[0]],b=projected[f.vertices[1]],c=projected[f.vertices[2]];
    const auto area=SU2Native2D::Orientation(a[0],a[1],b[0],b[1],c[0],c[1]);
    if(!area)continue;
    const auto orientation=BLDeterminant(tetrahedron[f.vertices[0]],tetrahedron[f.vertices[1]],tetrahedron[f.vertices[2]],tetrahedron[opposite]);
    f.entry=(area>0)==(orientation>0);faces.push_back(f);
  }
  auto endpoint=[&](const FaceProjection& face,const std::array<double,2>& q) {
    for(unsigned j=0;j<3;++j)if(q==projected[face.vertices[j]]){std::array<double,4> result{};result[face.vertices[j]]=1;return result;}
    const auto a=projected[face.vertices[0]],b=projected[face.vertices[1]],c=projected[face.vertices[2]];
    const long double bx=(long double)b[0]-a[0],by=(long double)b[1]-a[1],cx=(long double)c[0]-a[0],cy=(long double)c[1]-a[1],
        qx=(long double)q[0]-a[0],qy=(long double)q[1]-a[1],det=bx*cy-by*cx;
    const auto first=(qx*cy-qy*cx)/det,second=(bx*qy-by*qx)/det;
    std::array<double,4> result{};result[face.vertices[0]]=double(1-first-second);
    result[face.vertices[1]]=double(first);result[face.vertices[2]]=double(second);return result;
  };
  // Unique entry/exit face pairs partition the orthogonal projection of a convex tetrahedron.
  for(const auto& entry:faces)if(entry.entry)for(const auto& exit:faces)if(!exit.entry) {
    unsigned common[2],count=0,entryOther=0,exitOther=0;
    for(auto k:entry.vertices) {
      if(std::find(exit.vertices.begin(),exit.vertices.end(),k)!=exit.vertices.end())common[count++]=k;
      else entryOther=k;
    }
    for(auto k:exit.vertices)if(std::find(entry.vertices.begin(),entry.vertices.end(),k)==entry.vertices.end())exitOther=k;
    auto a=projected[common[0]],b=projected[common[1]];const auto c=projected[entryOther],d=projected[exitOther];
    auto orient=[](const auto& a,const auto& b,const auto& c){return SU2Native2D::Orientation(a[0],a[1],b[0],b[1],c[0],c[1]);};
    const auto sideC=orient(a,b,c),sideD=orient(a,b,d);
    // Shared-base triangles on opposite half-planes overlap only on their edge.
    if((sideC>0)!=(sideD>0))continue;
    if(sideC<0)std::swap(a,b);
    auto contains=[&](const auto& top,const auto& point){return orient(b,top,point)>=0 && orient(top,a,point)>=0;};
    std::array<double,2> apex;
    if(contains(c,d))apex=d;
    else if(contains(d,c))apex=c;
    else {
      // The tighter ray from each base endpoint bounds the unique triangular intersection.
      const auto first=orient(a,c,d)>0?c:d,last=orient(b,c,d)<0?c:d;
      const long double ux=(long double)first[0]-a[0],uy=(long double)first[1]-a[1],vx=(long double)last[0]-b[0],vy=(long double)last[1]-b[1];
      const auto denominator=SU2Native3D::CompensatedCross2D(ux,uy,vx,vy);
      if(!(denominator>0))throw std::runtime_error("Unresolved native3D projected edge intersection.");
      const auto t=orient(a,b,last)/denominator;
      if(!(t>0 && t<=1))throw std::runtime_error("Native3D projected intersection leaves its source edge.");
      apex={double(a[0]+t*ux),double(a[1]+t*uy)};
    }
    // Preserve the common vertices exactly; floating half-plane self-clipping creates artificial slivers.
    convex_clip::Polygon overlap;overlap.InitTriangle(a.data(),b.data(),apex.data());
    // Separate small finite wall footprints before density probes; agreement of two rules can miss them.
    std::vector<convex_clip::Polygon> pieces{overlap};
    for(const auto& face:support) {
      if(walls[face.wall].authority)continue; // Facet footprints do not partition a smooth component.
      std::array<std::array<double,2>,3> footprint{};
      for(unsigned k=0;k<3;++k)for(unsigned j=0;j<3;++j) {
        footprint[k][0]+=double(((long double)face.vertices[k][j]-projectionOrigin[j])*u[j]);
        footprint[k][1]+=double(((long double)face.vertices[k][j]-projectionOrigin[j])*v[j]);
      }
      for(unsigned edge=0;edge<3;++edge) {
        const auto a=footprint[edge],b=footprint[(edge+1)%3];
        const auto length=std::hypot(b[0]-a[0],b[1]-a[1]);if(!length)continue;
        const double nx=(a[1]-b[1])/length,ny=(b[0]-a[0])/length,d=-nx*a[0]-ny*a[1];
        std::vector<std::array<double,3>> lines;
        for(const double offset:{-face.thickness,-.9*face.thickness,0.,.9*face.thickness,face.thickness})
          lines.push_back({nx,ny,d+offset});
        // Endpoint-normal planes separate edge and corner distance regimes in the projected footprint.
        const double tx=(b[0]-a[0])/length,ty=(b[1]-a[1])/length;
        lines.push_back({tx,ty,-tx*a[0]-ty*a[1]});lines.push_back({tx,ty,-tx*b[0]-ty*b[1]});
        for(const auto& line:lines) {
          std::vector<convex_clip::Polygon> next;
          for(const auto& piece:pieces) {
            bool positive=false,negative=false;
            for(int k=0;k<piece.n;++k) {
              const long double first=(long double)line[0]*piece.x[k][0],second=(long double)line[1]*piece.x[k][1];
              const auto side=first+second+line[2];
              // Omit only indistinguishable integration cuts; clipping noise must not create zero-length chords.
              const auto roundoff=8*std::numeric_limits<double>::epsilon()*(std::abs(first)+std::abs(second)+std::abs(line[2]));
              positive=positive || side>roundoff;negative=negative || side<-roundoff;
            }
            if(!positive || !negative){next.push_back(piece);continue;}
            auto left=piece,right=piece;
            if(left.Clip(line[0],line[1],line[2])<0 || right.Clip(-line[0],-line[1],-line[2])<0)
              throw std::runtime_error("Native3D projected wall partition exceeds polygon capacity.");
            if(left.n>=3)next.push_back(left);if(right.n>=3)next.push_back(right);
          }
          if(next.size()>4096)throw std::runtime_error("Native3D projected wall partition exceeds work budget.");
          pieces=std::move(next);
        }
      }
    }
    for(const auto& piece:pieces) {
      double area=0,center[2]{};piece.Moments(area,center);if(!(area>0))continue;
      int anchor=0;long double longest=0;
      // Anchor away from the shared source edge without adding centroid charts.
      for(int k=0;k<piece.n;++k) {
        const std::array<double,2> q{piece.x[k][0],piece.x[k][1]};
        const auto first=endpoint(entry,q),last=endpoint(exit,q);long double length=0;
        for(unsigned j=0;j<4;++j)for(unsigned d=0;d<3;++d)
          length+=((long double)last[j]-first[j])*((long double)tetrahedron[j][d]-tetrahedron[0][d])*direction[d];
        if(length>longest){longest=length;anchor=k;}
      }
      for(int k=1;k+1<piece.n;++k) {
      ChordRule chart;const int vertices[3]={anchor,(anchor+k)%piece.n,(anchor+k+1)%piece.n};
      for(unsigned j=0;j<3;++j)chart.projection[j]={piece.x[vertices[j]][0],piece.x[vertices[j]][1]};
      for(unsigned j=0;j<3;++j) {
        chart.entry[j]=endpoint(entry,chart.projection[j]);chart.exit[j]=endpoint(exit,chart.projection[j]);
      }
      const auto a=chart.projection[0],b=chart.projection[1],c=chart.projection[2];
      chart.area=.5*(((long double)b[0]-a[0])*((long double)c[1]-a[1])-((long double)b[1]-a[1])*((long double)c[0]-a[0]));
      if(cylinder)for(unsigned j=0;j<3;++j) {
        for(unsigned k=0;k<4;++k)for(unsigned d=0;d<3;++d)chart.axial[j]+=((long double)chart.exit[j][k]-chart.entry[j][k])*((long double)tetrahedron[k][d]-tetrahedron[0][d])*direction[d];
        if(chart.axial[j]<0)throw std::runtime_error("Negative native3D cylindrical chart height.");
      }
      if(chart.area>0)charts.push_back(chart);
      }
    }
  }
  if(charts.empty())throw std::runtime_error("Native3D parallel integration has no positive projected chart.");
  if(cylinder) {
    using P2=std::array<long double,2>;
    P2 origin{};
    const long double offset[3]={(long double)cylinder->origin.x-projectionOrigin[0],(long double)cylinder->origin.y-projectionOrigin[1],(long double)cylinder->origin.z-projectionOrigin[2]};
    for(unsigned j=0;j<3;++j){origin[0]+=offset[j]*u[j];origin[1]+=offset[j]*v[j];}
    auto wedge=[](P2 a,P2 b){return SU2Native3D::CompensatedCross2D(a[0],a[1],b[0],b[1]);};
    auto minus=[](P2 a,P2 b){return P2{a[0]-b[0],a[1]-b[1]};};
    long double radius=0;
    for(const auto& p:projected)radius=std::max(radius,std::hypot(p[0]-origin[0],p[1]-origin[1]));
    if(!(radius>0))throw std::runtime_error("Degenerate native3D cylindrical projection.");
    const auto axial=unit({cylinder->axis.x,cylinder->axis.y,cylinder->axis.z});
    BLPoint3D rayStart{},rayEnd{};
    const double center[3]={cylinder->origin.x,cylinder->origin.y,cylinder->origin.z};
    for(unsigned j=0;j<3;++j){rayStart[j]=double(center[j]+.5L*cylinder->extent.y*axial[j]);rayEnd[j]=double(rayStart[j]+radius*u[j]);}
    auto cuts=EdgeBreakpoints(rayStart,rayEnd,std::max(2.,bandRatio),4);
    std::vector<long double> levels;for(double t:cuts)levels.push_back(t*radius);
    levels.front()=0;levels.back()=radius;
    struct PolarPanel {
      size_t chart,band;
      P2 first,last;
      std::array<std::array<long double,2>,3> bounds;
      unsigned depth=0;
      std::vector<IntegrationPoint3D> points;
      long double value=0,volume=0,error=0,volumeError=0;
      std::array<long double,3> directionError{},directionVolumeError{};
    };
    const double x3[3]={-.77459666924148337704,0,.77459666924148337704},w3[3]={5./9,8./9,5./9};
    const double x5[5]={-.90617984593866399280,-.53846931010568309104,0,.53846931010568309104,.90617984593866399280};
    const double w5[5]={.23692688505618908751,.47862867049936646804,128./225,.47862867049936646804,.23692688505618908751};
    struct RadialInterval {
      long double lo=0,hi=std::numeric_limits<long double>::infinity(),norm=0;
      std::array<std::array<long double,3>,2> bary{};
      bool valid=false;
    };
    auto radialRange=[&](const ChordRule& chart,P2 first,P2 last,long double t) {
      RadialInterval range;const P2 ray{(1-t)*first[0]+t*last[0],(1-t)*first[1]+t*last[1]};range.norm=std::hypot(ray[0],ray[1]);
      auto against=[&](P2 edge){return (1-t)*wedge(edge,first)+t*wedge(edge,last);};
      int ends[2]={-1,-1};
      for(unsigned j=0;j<3;++j){const P2 a{chart.projection[j][0]-origin[0],chart.projection[j][1]-origin[1]};
        const P2 b{chart.projection[(j+1)%3][0]-origin[0],chart.projection[(j+1)%3][1]-origin[1]},edge=minus(b,a);
        const auto denominator=against(edge),numerator=wedge(edge,a);
        if(denominator>0){const auto hit=numerator*range.norm/denominator;if(hit>range.lo){range.lo=hit;ends[0]=j;}}
        else if(denominator<0){const auto hit=numerator*range.norm/denominator;if(hit<range.hi){range.hi=hit;ends[1]=j;}}
        else if(numerator>0)return range;
      }
      if(!(range.hi>range.lo) || !std::isfinite(range.hi))return range;
      for(unsigned end=0;end<2;++end){
        if(ends[end]<0) {
          for(unsigned j=0;j<3;++j){const auto a=chart.projection[(j+1)%3],b=chart.projection[(j+2)%3];
            range.bary[end][j]=wedge({a[0]-origin[0],a[1]-origin[1]},{b[0]-origin[0],b[1]-origin[1]})/(2*chart.area);}
        } else {
          const unsigned i=ends[end],j=(i+1)%3;
          const P2 a{chart.projection[i][0]-origin[0],chart.projection[i][1]-origin[1]},b{chart.projection[j][0]-origin[0],chart.projection[j][1]-origin[1]};
          const auto denominator=against(minus(b,a));range.bary[end][i]=against(b)/denominator;range.bary[end][j]=-against(a)/denominator;
        }
        long double sum=0;for(auto value:range.bary[end]){if(value<0 || !std::isfinite(value))throw std::runtime_error("Invalid native3D cylindrical ray-boundary coordinate.");sum+=value;}
        if(!(sum>0))throw std::runtime_error("Empty native3D cylindrical ray-boundary coordinate.");
        for(auto& value:range.bary[end])value/=sum;
      }
      range.valid=true;return range;
    };
    auto sample=[&](const PolarPanel& panel,const std::array<unsigned,3>& orders,bool retain) {
      std::array<long double,2> integral{};const auto& chart=charts[panel.chart];
      for(unsigned it=0;it<orders[0];++it){
        const auto half=.5L*(panel.bounds[0][1]-panel.bounds[0][0]),middle=.5L*(panel.bounds[0][1]+panel.bounds[0][0]);
        const auto fraction=middle+half*(orders[0]==3?x3[it]:x5[it]);const auto range=radialRange(chart,panel.first,panel.last,fraction);
        const auto left=std::max(range.lo,levels[panel.band]),right=std::min(range.hi,levels[panel.band+1]);
        if(!range.valid || !(right>left))throw std::runtime_error("Unresolved native3D cylindrical radial-band closure.");
        const auto angularJacobian=wedge(panel.first,panel.last)/(range.norm*range.norm);
        for(unsigned ir=0;ir<orders[1];++ir){
          const auto rh=.5L*(panel.bounds[1][1]-panel.bounds[1][0]),rm=.5L*(panel.bounds[1][1]+panel.bounds[1][0]);
          const auto fractionR=rm+rh*(orders[1]==3?x3[ir]:x5[ir]),r=left+(right-left)*fractionR;
          const auto low=((range.hi-right)+(1-fractionR)*(right-left))/(range.hi-range.lo);
          const auto high=((left-range.lo)+fractionR*(right-left))/(range.hi-range.lo);
          std::array<long double,3> angular{};
          for(unsigned j=0;j<3;++j)angular[j]=low*range.bary[0][j]+high*range.bary[1][j];
          std::array<long double,4> entry{},exit{};long double length=0;
          for(unsigned j=0;j<3;++j){length+=angular[j]*chart.axial[j];for(unsigned k=0;k<4;++k){entry[k]+=angular[j]*chart.entry[j][k];exit[k]+=angular[j]*chart.exit[j][k];}}
          if(!(length>0))throw std::runtime_error("Nonpositive native3D cylindrical axial chord.");
          for(unsigned iz=0;iz<orders[2];++iz){
            const auto zh=.5L*(panel.bounds[2][1]-panel.bounds[2][0]),zm=.5L*(panel.bounds[2][1]+panel.bounds[2][0]);
            const auto z=zm+zh*(orders[2]==3?x3[iz]:x5[iz]);std::array<double,4> parent{};
            for(unsigned k=0;k<4;++k)parent[k]=double((1-z)*entry[k]+z*exit[k]);
            BLPoint3D position{};SetBLSourceQuery(tetrahedron,originalSource,reversed,parent,position);
            const auto weight=half*angularJacobian*rh*zh*(right-left)*r*length*
                (orders[0]==3?w3[it]:w5[it])*(orders[1]==3?w3[ir]:w5[ir])*(orders[2]==3?w3[iz]:w5[iz]);
            const double pointWeight=double(weight);
            if(!(pointWeight>0) || !std::isfinite(pointWeight))throw std::runtime_error("Invalid native3D cylindrical quadrature weight.");
            IntegrationPoint3D point{parent,pointWeight,SamplePoints({position[0],position[1],position[2]})};
            integral[0]+=point.weight*probe(point);integral[1]+=point.weight;
            if(retain)rule.push_back(std::move(point));
          }
        }
      }
      return integral;
    };
    std::map<size_t,PolarPanel> panels;std::priority_queue<std::pair<long double,size_t>> priority;
    long double total=0,totalVolume=0,error=0,volumeError=0;size_t next=0,retained=0,work=0;
    const auto expectedVolume=std::abs(BLDeterminant(tetrahedron[0],tetrahedron[1],tetrahedron[2],tetrahedron[3]))/6;
    if(!(expectedVolume>0) || !std::isfinite(expectedVolume))throw std::runtime_error("Invalid native3D cylindrical source volume.");
    long double densityScale=1;
    auto score=[&](const PolarPanel& panel){return panel.error+1e6L*densityScale*panel.volumeError;};
    auto add=[&](PolarPanel panel) {
      if(++work>16384)throw std::runtime_error("Native3D cylindrical volume work budget exhausted.");
      rule.clear();const auto low=sample(panel,{3,3,3},true);panel.points.swap(rule);panel.value=low[0];panel.volume=low[1];
      for(unsigned axis=0;axis<3;++axis){std::array<unsigned,3> order{3,3,3};order[axis]=5;const auto high=sample(panel,order,false);
        panel.directionError[axis]=std::abs(high[0]-low[0]);panel.directionVolumeError[axis]=std::abs(high[1]-low[1]);
        panel.error+=panel.directionError[axis];panel.volumeError+=panel.directionVolumeError[axis];}
      const auto high=sample(panel,{5,5,5},false);panel.error=std::max(panel.error,std::abs(high[0]-low[0]));
      panel.volumeError=std::max(panel.volumeError,std::abs(high[1]-low[1]));
      if(retained+panel.points.size()>65536)throw std::runtime_error("Native3D cylindrical volume point budget exhausted.");
      retained+=panel.points.size();total+=panel.value;totalVolume+=panel.volume;error+=panel.error;volumeError+=panel.volumeError;
      priority.emplace(score(panel),next);panels.emplace(next++,std::move(panel));
    };
    for(size_t at=0;at<charts.size();++at){const auto& chart=charts[at];std::vector<P2> rays{{1,0},{0,1},{-1,0},{0,-1}};
      auto angle=[&](P2 p){if(p[0]!=0 || p[1]!=0)rays.push_back(p);};
      for(unsigned j=0;j<3;++j){const P2 a{chart.projection[j][0]-origin[0],chart.projection[j][1]-origin[1]};
        const P2 b{chart.projection[(j+1)%3][0]-origin[0],chart.projection[(j+1)%3][1]-origin[1]},edge=minus(b,a);angle(a);
        // A radial edge contributes only its endpoint rays; recomputing circle roots creates duplicate ulp-wide angles.
        if(wedge(a,b)==0)continue;
        const auto square=edge[0]*edge[0]+edge[1]*edge[1],linear=2*(a[0]*edge[0]+a[1]*edge[1]);
        if(!(square>0))throw std::runtime_error("Degenerate native3D cylindrical projected edge.");
        for(auto level:levels){const auto constant=a[0]*a[0]+a[1]*a[1]-level*level,discriminant=linear*linear-4*square*constant;
          if(discriminant<0)continue;
          const auto root=std::sqrt(discriminant),q=-.5L*(linear+std::copysign(root,linear));
          for(auto t:std::array<long double,2>{q?q/square:-linear/(2*square),q?constant/q:-linear/(2*square)})
            if(t>0 && t<1)angle({a[0]+t*edge[0],a[1]+t*edge[1]});
        }
      }
      // Sort rays directly; local sector angles preserve small offsets at the periodic seam.
      auto upper=[](P2 p){return p[1]>0 || (p[1]==0 && p[0]>=0);};
      // Powers of two condition the rays without changing their exact directions.
      for(auto& ray:rays){int exponent=0;std::frexp(std::max(std::abs(ray[0]),std::abs(ray[1])),&exponent);for(auto& value:ray)value=std::ldexp(value,-exponent);}
      std::sort(rays.begin(),rays.end(),[&](P2 a,P2 b){return upper(a)!=upper(b)?upper(a):wedge(a,b)>0;});
      rays.erase(std::unique(rays.begin(),rays.end(),[&](P2 a,P2 b){return upper(a)==upper(b) && wedge(a,b)==0;}),rays.end());
      for(size_t k=0;k<rays.size();++k){const auto first=rays[k],last=rays[(k+1)%rays.size()];
        if(!(wedge(first,last)>0))throw std::runtime_error("Unresolved native3D cylindrical angular sector.");
        const auto range=radialRange(chart,first,last,.5L);if(!range.valid)continue;
        for(size_t band=0;band+1<levels.size();++band)if(std::min(range.hi,levels[band+1])>std::max(range.lo,levels[band]))
          add(PolarPanel{at,band,first,last,{{{0,1},{0,1},{0,1}}}});
      }
    }
    if(!(total>0) || panels.empty())throw std::runtime_error("Empty native3D cylindrical integration partition.");
    densityScale=total/expectedVolume;priority={};for(const auto& row:panels)priority.emplace(score(row.second),row.first);
    while(error>1e-4L*total || volumeError>1e-10L*expectedVolume || std::abs(totalVolume-expectedVolume)>2e-10L*expectedVolume){
      if(priority.empty() || !(priority.top().first>0))throw std::runtime_error("Unresolved native3D cylindrical integration error.");
      const auto selected=priority.top().second;priority.pop();auto panel=std::move(panels.at(selected));panels.erase(selected);
      if(panel.depth>=32)throw std::runtime_error("Unresolved native3D cylindrical density after32 subdivisions.");
      retained-=panel.points.size();panel.points.clear();total-=panel.value;totalVolume-=panel.volume;
      error=std::max(0.L,error-panel.error);volumeError=std::max(0.L,volumeError-panel.volumeError);
      unsigned split=0;long double largest=-1;
      for(unsigned j=0;j<3;++j){const auto value=panel.directionError[j]+1e6L*densityScale*panel.directionVolumeError[j];if(value>largest){largest=value;split=j;}}
      const auto middle=.5L*(panel.bounds[split][0]+panel.bounds[split][1]);
      if(middle==panel.bounds[split][0] || middle==panel.bounds[split][1])throw std::runtime_error("Unrepresentable native3D cylindrical density split.");
      for(unsigned side=0;side<2;++side){auto bounds=panel.bounds;bounds[split][1-side]=middle;add(PolarPanel{panel.chart,panel.band,panel.first,panel.last,bounds,panel.depth+1});}
    }
    rule.clear();rule.reserve(retained);for(auto& panel:panels)rule.insert(rule.end(),std::make_move_iterator(panel.second.points.begin()),std::make_move_iterator(panel.second.points.end()));
    if(sensorDependent)*sensorDependent=true;
    return rule;
  }
  std::vector<Column3D> columns;
  for(const auto& chart:charts) {
    Column3D column;column.entry=chart.entry;column.exit=chart.exit;column.area=chart.area;
    for(unsigned j=0;j<3;++j) {
      column.projection[j]={chart.projection[j][0],chart.projection[j][1],0};
      for(unsigned k=0;k<4;++k)for(unsigned d=0;d<3;++d)
        column.axial[j]+=((long double)chart.exit[j][k]-chart.entry[j][k])*((long double)tetrahedron[k][d]-tetrahedron[0][d])*direction[d];
    }
    columns.push_back(std::move(column));
  }
  return IntegrationColumns3D(tetrahedron,columns,support,bandRatio,probe);
}

std::vector<CBoundaryLayerMetric::IntegrationPoint3D> CBoundaryLayerMetric::IntegrationRule3DVisible(
    const BLTetrahedron& tetrahedron,const std::vector<SupportFace3D>& support,double bandRatio,
    const IntegrationDensity3D& density) {
  unsigned opposite=0;double best=std::numeric_limits<double>::infinity();
  for(unsigned short wall=0;wall<walls.size();++wall)if(data[wall]->adt) {
    std::array<double,4> distances;
    for(unsigned k=0;k<4;++k)distances[k]=SU2_TYPE::GetValue(Evaluate(wall,tetrahedron[k].data(),1.).distance)/SU2_TYPE::GetValue(data[wall]->thickness);
    for(unsigned candidate=0;candidate<4;++candidate) {
      double score=0;for(unsigned k=0;k<4;++k)if(k!=candidate)score+=distances[k];
      if(score<best){best=score;opposite=candidate;}
    }
  }
  Column3D column;column.collapsed=true;
  column.area=std::abs(BLDeterminant(tetrahedron[0],tetrahedron[1],tetrahedron[2],tetrahedron[3]))/2;
  unsigned next=0;
  for(unsigned k=0;k<4;++k)if(k!=opposite) {
    column.entry[next][k]=1;column.exit[next][opposite]=1;column.axial[next]=1;
    for(unsigned j=0;j<3;++j)column.projection[next][j]=tetrahedron[k][j]-tetrahedron[opposite][j];
    ++next;
  }
  return IntegrationColumns3D(tetrahedron,{column},support,bandRatio,density);
}

std::vector<CBoundaryLayerMetric::IntegrationPoint3D> CBoundaryLayerMetric::IntegrationColumns3D(
    const BLTetrahedron& tetrahedron,const std::vector<Column3D>& charts,const std::vector<SupportFace3D>& support,
    double bandRatio,const IntegrationDensity3D& probe) {
  using Triangle=std::array<std::array<double,4>,3>;
  SU2Native3D::Tetrahedron originalSource;
  for(unsigned k=0;k<4;++k)originalSource[k]={tetrahedron[k][0],tetrahedron[k][1],tetrahedron[k][2]};
  const bool reversed=SU2Native3D::Orientation(originalSource[0],originalSource[1],originalSource[2],originalSource[3])<0;
  if(reversed)std::swap(originalSource[1],originalSource[2]);
  size_t visited=0;std::vector<IntegrationPoint3D> rule;
  const double gauss[5]={-.90617984593866399280,-.53846931010568309104,0,.53846931010568309104,.90617984593866399280};
  const double weights[5]={.23692688505618908751,.47862867049936646804,128./225,.47862867049936646804,.23692688505618908751};
  const double gauss3[3]={-.77459666924148337704,0,.77459666924148337704},weights3[3]={5./9,8./9,5./9};
  const double gauss7[7]={-.94910791234275852453,-.74153118559939443986,-.40584515137739716691,0,
      .40584515137739716691,.74153118559939443986,.94910791234275852453};
  const double weights7[7]={.12948496616886969327,.27970539148927666790,.38183005050511894495,.41795918367346938776,
      .38183005050511894495,.27970539148927666790,.12948496616886969327};
  const double faceBary[7][3]={
      {1./3,1./3,1./3},
      {.059715871789770,.470142064105115,.470142064105115},
      {.470142064105115,.059715871789770,.470142064105115},
      {.470142064105115,.470142064105115,.059715871789770},
      {.797426985353087,.101286507323456,.101286507323456},
      {.101286507323456,.797426985353087,.101286507323456},
      {.101286507323456,.101286507323456,.797426985353087}};
  const double faceWeights[7]={.225,.132394152788506,.132394152788506,.132394152788506,
      .125939180544827,.125939180544827,.125939180544827};
  // Compare the retained degree-five rule with a higher-degree probe without retaining its extra points.
  const double comparisonBary[12][3]={
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
  const double comparisonWeights[12]={.116786275726379,.116786275726379,.116786275726379,
      .050844906370207,.050844906370207,.050844906370207,
      .082851075618374,.082851075618374,.082851075618374,.082851075618374,.082851075618374,.082851075618374};
  struct RadialPanel {
    double left,right,value,error,comparison;
    unsigned depth;
    bool upgraded=false;
    unsigned count=3;
    std::vector<unsigned short> sampled,comparisonSampled;
    std::vector<IntegrationPoint3D> points,comparisonPoints;
  };
  struct Ray {
    std::array<double,4> entry{},exit{};
    long double jacobian=0;
    long double value=0,error=0;
    bool fine=false;
    size_t count=0,next=0;
    std::vector<size_t> sampled;
    std::map<size_t,RadialPanel> intervals;
    std::priority_queue<std::pair<double,size_t>> priority;
  };
  struct Panel {
    size_t cone;
    Triangle triangle;
    double fraction;
    unsigned depth;
    std::array<Ray,19> rays;
    long double value=0,comparison=0,angularError=0,radialError=0;
    size_t count=0,revision=0;
    bool unresolved=false,geometryChecked=false;
    std::vector<bool> possible;
    std::array<double,3> vertexValues{};
  };
  size_t retained=0,nextPanel=0,unresolved=0;
  long double totalValue=0,totalComparison=0,totalAngularError=0,totalRadialError=0;
  std::map<size_t,Panel> panels;
  using Priority=std::priority_queue<std::pair<double,std::pair<size_t,size_t>>>;
  Priority angularPriority,radialPriority;
  auto radial=[&](const Panel& panel,const Ray& ray,double left,double right,unsigned order,
                  std::vector<IntegrationPoint3D>& samples) {
    double value=0;const double half=.5*(right-left),center=.5*(right+left);
    for(unsigned g=0;g<order;++g) {
      const double t=center+half*(order==7?gauss7[g]:(order==5?gauss[g]:gauss3[g]));
      std::array<double,4> parent{};BLPoint3D position{};
      for(unsigned k=0;k<4;++k) {
        parent[k]=(1-t)*ray.entry[k]+t*ray.exit[k];
        for(unsigned j=0;j<3;++j)position[j]+=parent[k]*tetrahedron[k][j];
      }
      SetBLSourceQuery(tetrahedron,originalSource,reversed,parent,position);
      const double collapse=charts[panel.cone].collapsed?(1-t)*(1-t):1.;
      IntegrationPoint3D point{parent,double(ray.jacobian*half*collapse*(order==7?weights7[g]:(order==5?weights[g]:weights3[g]))),
          SamplePoints({position[0],position[1],position[2]})};
      if(!(point.weight>0) || !std::isfinite(point.weight))throw std::runtime_error("Invalid native3D BL quadrature weight.");
      value+=point.weight*probe(point);samples.push_back(std::move(point));
    }
    return value;
  };
  auto retainRadial=[&](Ray& ray,RadialPanel interval) {
    if(!(interval.value>0) || !std::isfinite(interval.value) || !std::isfinite(interval.error))
      throw std::runtime_error("Nonfinite/nonpositive native3D BL radial error estimate.");
    ray.count+=interval.count;
    if(ray.count>65536)throw std::runtime_error("Native3D BL column ray point budget exhausted.");
    for(auto wall:interval.sampled)++ray.sampled[wall];
    ray.value+=interval.value;ray.error+=interval.error;
    ray.priority.emplace(interval.error,ray.next);ray.intervals.emplace(ray.next++,std::move(interval));
  };
  auto addRadial=[&](Panel& panel,Ray& ray,double left,double right,unsigned depth) {
    RadialPanel interval{left,right,0,0,0,depth};
    interval.value=radial(panel,ray,left,right,3,interval.points);
    interval.comparison=radial(panel,ray,left,right,5,interval.comparisonPoints);
    interval.error=std::abs(interval.comparison-interval.value);
    for(const auto& point:interval.points)for(const auto& sample:point.samples)interval.sampled.push_back(sample.wall);
    for(const auto& point:interval.comparisonPoints)for(const auto& sample:point.samples)interval.comparisonSampled.push_back(sample.wall);
    if(!ray.fine) {
      std::vector<IntegrationPoint3D>().swap(interval.points);
      std::vector<IntegrationPoint3D>().swap(interval.comparisonPoints);
    }
    retainRadial(ray,std::move(interval));
  };
  auto removeTotals=[&](const Panel& panel) {
    retained-=panel.count;unresolved-=panel.unresolved;
    totalValue-=panel.value;totalComparison-=panel.comparison;
    totalAngularError=std::max(0.L,totalAngularError-panel.angularError);
    totalRadialError=std::max(0.L,totalRadialError-panel.radialError);
  };
  auto publish=[&](size_t id,Panel& panel) {
    panel.value=panel.comparison=panel.radialError=0;panel.count=0;
    size_t comparisonCount=0;double largestRadial=0;
    std::vector<bool> sampled(walls.size(),false);
    for(unsigned k=0;k<panel.rays.size();++k) {
      const auto& ray=panel.rays[k];panel.radialError+=ray.error;
      largestRadial=std::max(largestRadial,ray.priority.top().first);
      if(k<7){panel.value+=ray.value;panel.count+=ray.count;}
      else {panel.comparison+=ray.value;comparisonCount+=ray.count;}
      for(unsigned w=0;w<walls.size();++w)sampled[w]=sampled[w] || ray.sampled[w];
    }
    if(retained+panel.count>65536 || comparisonCount>65536)
      throw std::runtime_error("Native3D BL column point budget exhausted: retained="+std::to_string(retained)+
          " panel="+std::to_string(id)+" fine="+std::to_string(panel.count)+" comparison="+std::to_string(comparisonCount));
    panel.angularError=std::abs(panel.value-panel.comparison);
    if(!(panel.value>0) || !(panel.comparison>0) || !std::isfinite(panel.value) ||
        !std::isfinite(panel.comparison) || !std::isfinite(panel.angularError) || !std::isfinite(panel.radialError))
      throw std::runtime_error("Nonfinite/nonpositive native3D BL cell error estimate.");
    for(unsigned k=0;k<3;++k)panel.vertexValues[k]=panel.rays[k+4].value/
        double(charts[panel.cone].area*panel.fraction*faceWeights[k+4]);
    if(!panel.geometryChecked && std::any_of(support.begin(),support.end(),[&](const auto& f){return !sampled[f.wall];})) {
      std::array<BLPoint3D,6> boundary{};
      for(unsigned side=0;side<2;++side)for(unsigned p=0;p<3;++p)for(unsigned j=0;j<3;++j)for(unsigned k=0;k<4;++k)for(unsigned v=0;v<3;++v)
        boundary[3*side+p][j]+=panel.triangle[p][v]*(side?charts[panel.cone].exit[v][k]:charts[panel.cone].entry[v][k])*tetrahedron[k][j];
      const std::array<BLTetrahedron,3> regions{{{boundary[0],boundary[1],boundary[2],boundary[5]},
          {boundary[0],boundary[1],boundary[4],boundary[5]},{boundary[0],boundary[3],boundary[4],boundary[5]}}};
      panel.possible.assign(walls.size(),false);
      for(const auto& face:support)for(const auto& region:regions) {
        if(BLDeterminant(region[0],region[1],region[2],region[3])==0)continue;
        const auto& wall=walls[face.wall];
        if(wall.authority ? MayIntersectSmoothBand(region,*wall.authority,wall.authority_facet,SU2_TYPE::GetValue(data[face.wall]->thickness)) :
            MayIntersectWallBand(region,face.vertices,face.thickness*(1-1e-12)))panel.possible[face.wall]=true;
      }
      panel.geometryChecked=true;
    }
    panel.unresolved=false;
    if(panel.geometryChecked)for(unsigned w=0;w<walls.size();++w)panel.unresolved=panel.unresolved || (panel.possible[w] && !sampled[w]);
    totalValue+=panel.value;totalComparison+=panel.comparison;
    totalAngularError+=panel.angularError;totalRadialError+=panel.radialError;
    retained+=panel.count;unresolved+=panel.unresolved;++panel.revision;
    angularPriority.push({panel.unresolved?std::numeric_limits<double>::infinity():panel.angularError,{id,panel.revision}});
    radialPriority.push({largestRadial,{id,panel.revision}});
  };
  auto add=[&](size_t cone,const Triangle& triangle,double fraction,unsigned depth) {
    if(++visited>16384)throw std::runtime_error("Native3D BL angular quadrature work budget exhausted: retained="+std::to_string(retained)+
        " panels="+std::to_string(nextPanel)+" cones="+std::to_string(charts.size()));
    Panel panel{cone,triangle,fraction,depth,{}};
    const auto& chart=charts[cone];
    for(unsigned corner=0;corner<panel.rays.size();++corner) {
      auto& ray=panel.rays[corner];ray.sampled.assign(walls.size(),0);
      const bool fine=corner<7;ray.fine=fine;const auto index=fine?corner:corner-7;
      std::array<double,3> angular{};
      for(unsigned j=0;j<3;++j)for(unsigned k=0;k<3;++k)
        angular[j]+=triangle[k][j]*(fine?faceBary[index][k]:comparisonBary[index][k]);
      BLPoint3D start{},end{};
      for(unsigned k=0;k<4;++k) {
        for(unsigned j=0;j<3;++j){ray.entry[k]+=angular[j]*chart.entry[j][k];ray.exit[k]+=angular[j]*chart.exit[j][k];}
        for(unsigned j=0;j<3;++j){start[j]+=ray.entry[k]*tetrahedron[k][j];end[j]+=ray.exit[k]*tetrahedron[k][j];}
      }
      long double length=0;for(unsigned j=0;j<3;++j)length+=angular[j]*chart.axial[j];
      if(!(length>0))throw std::runtime_error("Native3D parallel integration chord has nonpositive length.");
      ray.jacobian=chart.area*fraction*length*(fine?faceWeights[index]:comparisonWeights[index]);
      // Sub-ulp endpoints can round to one point; retain their positive barycentric measure.
      const auto cuts=start==end?std::vector<double>{0,1}:EdgeBreakpoints(start,end,std::max(2.,bandRatio),4);
      if(cuts.size()<2)throw std::runtime_error("Native3D BL ray has no integration intervals.");
      for(size_t band=1;band<cuts.size();++band)addRadial(panel,ray,cuts[band-1],cuts[band],0);
    }
    publish(nextPanel,panel);panels.emplace(nextPanel++,std::move(panel));
  };
  auto front=[&](Priority& priority) {
    while(!priority.empty()) {
      const auto candidate=priority.top().second;const auto found=panels.find(candidate.first);
      if(found!=panels.end() && found->second.revision==candidate.second)return candidate.first;
      priority.pop();
    }
    throw std::runtime_error("Native3D BL cell error queue is unresolved.");
  };
  const Triangle initial{{{{1,0,0,0}},{{0,1,0,0}},{{0,0,1,0}}}};
  for(size_t k=0;k<charts.size();++k)add(k,initial,1.,0);
  if(panels.empty() || !(totalValue>0) || !std::isfinite(totalValue))throw std::runtime_error("Invalid native3D BL column integral.");
  // Allocate radial error across the complete cell, rather than oversolving each small weighted ray.
  while(unresolved || totalAngularError+totalRadialError>1e-4*totalValue ||
        totalRadialError>1e-5*(totalValue+totalComparison)) {
    const auto angularId=front(angularPriority),radialId=front(radialPriority);
    const bool radialOnly=!unresolved && totalAngularError+totalRadialError<=1e-4*totalValue;
    if(radialOnly || radialPriority.top().first>angularPriority.top().first) {
      radialPriority.pop();const auto selected=radialId;auto& panel=panels.at(selected);removeTotals(panel);
      unsigned worst=0;
      for(unsigned k=1;k<panel.rays.size();++k)
        if(panel.rays[k].priority.top().first>panel.rays[worst].priority.top().first)worst=k;
      auto& ray=panel.rays[worst];const auto intervalId=ray.priority.top().second;ray.priority.pop();
      auto interval=std::move(ray.intervals.at(intervalId));ray.intervals.erase(intervalId);
      ray.value-=interval.value;ray.error=std::max(0.L,ray.error-interval.error);ray.count-=interval.count;
      for(auto wall:interval.sampled)--ray.sampled[wall];
      if(!interval.upgraded) {
        std::vector<IntegrationPoint3D> higherPoints;
        const auto higher=radial(panel,ray,interval.left,interval.right,7,higherPoints);
        interval.value=interval.comparison;interval.error=std::abs(higher-interval.comparison);
        interval.points=std::move(interval.comparisonPoints);interval.sampled=std::move(interval.comparisonSampled);
        interval.count=5;interval.upgraded=true;retainRadial(ray,std::move(interval));
      } else {
        if(interval.depth>=32)throw std::runtime_error("Native3D BL radial density remains unresolved after32 subdivisions: cell="+
            std::to_string(totalValue+panel.value)+" radial_error="+std::to_string(totalRadialError+panel.radialError));
        if(++visited>16384)throw std::runtime_error("Native3D BL radial quadrature work budget exhausted: retained="+std::to_string(retained)+
            " panels="+std::to_string(nextPanel)+" cones="+std::to_string(charts.size())+" ray="+std::to_string(worst));
        const double middle=.5*(interval.left+interval.right);
        if(middle==interval.left || middle==interval.right)throw std::runtime_error("Unrepresentable native3D radial subdivision.");
        addRadial(panel,ray,interval.left,middle,interval.depth+1);addRadial(panel,ray,middle,interval.right,interval.depth+1);
      }
      publish(selected,panel);continue;
    }
    angularPriority.pop();const auto selected=angularId;auto panel=std::move(panels.at(selected));panels.erase(selected);
    if(panel.depth>=32)throw std::runtime_error("Native3D BL angular density remains unresolved after32 subdivisions.");
    removeTotals(panel);
    unsigned a=0,b=1;long double longest=0;
    for(unsigned i=0;i<3;++i)for(unsigned j=i+1;j<3;++j) {
      long double length=0;
      for(unsigned d=0;d<3;++d) {
        long double delta=0;for(unsigned k=0;k<3;++k)delta+=((long double)panel.triangle[i][k]-panel.triangle[j][k])*charts[panel.cone].projection[k][d];
        length+=delta*delta;
      }
      if(length>longest){longest=length;a=i;b=j;}
    }
    // Thin footprints need refinement across their density gradient, not along the longest wall edge.
    if(!panel.unresolved) {
      double difference=0,scale=0;for(auto value:panel.vertexValues)scale=std::max(scale,std::abs(value));
      for(unsigned i=0;i<3;++i)for(unsigned j=i+1;j<3;++j) {
        const auto change=std::abs(panel.vertexValues[i]-panel.vertexValues[j]);
        if(change>difference && change>1e-8*scale){difference=change;a=i;b=j;}
      }
    }
    std::array<double,4> midpoint{};for(unsigned k=0;k<4;++k)midpoint[k]=.5*panel.triangle[a][k]+.5*panel.triangle[b][k];
    if(midpoint==panel.triangle[a] || midpoint==panel.triangle[b])throw std::runtime_error("Unrepresentable native3D angular subdivision.");
    // Release discarded rays before constructing both children; no partial rule is returned on failure.
    panel.rays={};
    for(const auto vertex:{a,b}) {
      auto child=panel.triangle;child[vertex]=midpoint;add(panel.cone,child,.5*panel.fraction,panel.depth+1);
    }
  }
  for(auto& entry:panels)for(unsigned k=0;k<7;++k)for(auto& interval:entry.second.rays[k].intervals)
    rule.insert(rule.end(),std::make_move_iterator(interval.second.points.begin()),std::make_move_iterator(interval.second.points.end()));
  return rule;
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
  std::set<double> retained{0,1};
  std::function<bool(double)> validRoot;
  auto retain = [&](long double t) {
    if (t > 0 && t < 1) {
      const double rounded=static_cast<double>(t);
      if(retained.count(rounded) || !validRoot(rounded))return;
      if (cuts.size() >= 65536) throw std::runtime_error("Native BL edge breakpoints exceed work budget.");
      retained.insert(rounded);cuts.push_back(rounded);
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
    validRoot=[&](double t) {
      su2double x[3]{};for(unsigned j=0;j<nDim;++j)x[j]=(1-t)*a[j]+t*b[j];
      const double actual=walls[wall].authority ? walls[wall].authority->ClosestAnalytic(walls[wall].authority_facet,
          {SU2_TYPE::GetValue(x[0]),SU2_TYPE::GetValue(x[1]),SU2_TYPE::GetValue(x[2])}).distance : SU2_TYPE::GetValue(Evaluate(wall,x,1.).distance);
      double coordinateScale=0;
      for(unsigned j=0;j<nDim;++j)coordinateScale=std::max({coordinateScale,std::abs(a[j]),std::abs(b[j])});
      const double roundoff=64*std::numeric_limits<double>::epsilon()*coordinateScale;
      for(auto level:levels)
        if(std::abs(actual-level)<=std::max(roundoff,1e-10*std::max(h0,level)))return true;
      return false;
    };
    if(walls[wall].authority) {
      const auto& authority=*walls[wall].authority;const auto& g=authority.Analytic().at(walls[wall].authority_facet);
      Vector radial{},velocity=chord,axis{};
      const double center[3]={g.origin.x,g.origin.y,g.origin.z};
      for(unsigned j=0;j<3;++j)radial[j]=(long double)a[j]-center[j];
      long double z=0,slope=0;
      if(g.kind!=SU2Native3D::AnalyticSurface::Kind::Sphere) {
        axis={g.axis.x,g.axis.y,g.axis.z};const auto n=std::sqrt(dot(axis,axis));for(auto& x:axis)x/=n;
        z=dot(radial,axis);slope=dot(chord,axis);
        for(unsigned j=0;j<3;++j){radial[j]-=z*axis[j];velocity[j]-=slope*axis[j];}
      }
      if(g.kind!=SU2Native3D::AnalyticSurface::Kind::Disk)for(double level:levels)for(int side:{-1,1}) {
        const long double radius=(long double)g.extent.x+side*(long double)level;
        if(radius<0)continue;
        if(level && radius==g.extent.x)throw std::runtime_error("Unrepresentable native3D smooth distance band.");
        const auto speed=dot(velocity,velocity);
        if(speed>0) {
          const auto center=-dot(radial,velocity)/speed;Vector nearest{};
          for(unsigned j=0;j<3;++j)nearest[j]=std::fma(center,velocity[j],radial[j]);
          const auto minimum=std::sqrt(dot(nearest,nearest));const auto difference=(radius-minimum)*(radius+minimum);
          if(difference>=0){const auto width=std::sqrt(difference/speed);retain(center-width);retain(center+width);}
        }
      }
      if(g.kind==SU2Native3D::AnalyticSurface::Kind::Disk && slope)for(double level:levels){retain((level-z)/slope);retain((-level-z)/slope);}
      if(g.kind!=SU2Native3D::AnalyticSurface::Kind::Sphere && dot(velocity,velocity)==0) {
        const auto rho=std::sqrt(dot(radial,radial));
        const auto radialDistance=g.kind==SU2Native3D::AnalyticSurface::Kind::Cylinder?rho-g.extent.x:std::max(0.L,rho-g.extent.x);
        if(slope)for(double height:{0.,g.kind==SU2Native3D::AnalyticSurface::Kind::Cylinder?g.extent.y:0.})for(double level:levels) {
          const auto difference=((long double)level-radialDistance)*((long double)level+radialDistance);
          if(difference>=0) {
            const auto center=(height-z)/slope,width=std::sqrt(difference)/std::abs(slope);
            retain(center-width);retain(center+width);
          }
        }
      } else if(g.kind!=SU2Native3D::AnalyticSurface::Kind::Sphere) {
        auto mandatory=[&](double t){if(t>0 && t<1 && retained.insert(t).second){if(cuts.size()>=65536)throw std::runtime_error("Native3D curved edge cuts exceed work budget.");cuts.push_back(t);}};
        std::vector<double> pieces{0,1};
        if(slope)for(double height:{0.,g.kind==SU2Native3D::AnalyticSurface::Kind::Cylinder?g.extent.y:0.}) {
          const double t=double((height-z)/slope);if(t>0 && t<1){pieces.push_back(t);mandatory(t);}
        }
        std::sort(pieces.begin(),pieces.end());pieces.erase(std::unique(pieces.begin(),pieces.end()),pieces.end());
        const auto length=std::sqrt(dot(chord,chord));size_t work=0;double unresolvedError=0;
        const double allowance=1e-8/levels.size();
        std::function<void(double,double)> resolve;
        resolve=[&](double left,double right) {
          if(++work>16384)throw std::runtime_error("Native3D finite-rim edge resolution exceeds work budget.");
          const auto middle=.5*(left+right);SU2Native3D::Point p;
          p={double((1-middle)*a[0]+middle*b[0]),double((1-middle)*a[1]+middle*b[1]),double((1-middle)*a[2]+middle*b[2])};
          const auto closest=authority.ClosestAnalytic(walls[wall].authority_facet,p);const double distance=closest.distance;
          const auto half=(right-left)/2;auto radius=length*half;
          Vector here{};for(unsigned j=0;j<3;++j)here[j]=radial[j]+middle*velocity[j];
          const auto radialLower=std::sqrt(dot(here,here))-std::sqrt(dot(velocity,velocity))*half;
          const auto distanceLower=distance-radius;
          if(radialLower>0 && distanceLower>0) {
            const Vector gradient{closest.direction.x,closest.direction.y,closest.direction.z};
            // Distance to a finite circular rim has this Hessian bound away from its medial axis.
            const auto hessian=length*length*(1/distanceLower+1/radialLower);
            radius=std::min(radius,std::abs(dot(gradient,chord))*half+.5L*hessian*half*half);
          }
          const auto error=64*std::numeric_limits<double>::epsilon()*std::max({std::abs(p.x),std::abs(p.y),std::abs(p.z),g.extent.x});
          if(std::none_of(levels.begin(),levels.end(),[&](double level){return level>=distance-radius-error && level<=distance+radius+error;}))return;
          const double uncertainty=double(length*(right-left)*std::sqrt(d.maximum_wall_eigenvalue));
          if(uncertainty<=allowance) {
            unresolvedError+=uncertainty;if(unresolvedError>1e-8)throw std::runtime_error("Native3D finite-rim crossing uncertainty exceeds edge tolerance.");
            mandatory(left);mandatory(right);return;
          }
          if(middle==left || middle==right)throw std::runtime_error("Unresolved native3D finite-rim distance crossing.");
          resolve(left,middle);resolve(middle,right);
        };
        for(size_t k=1;k<pieces.size();++k) {
          const auto left=pieces[k-1],right=pieces[k],middle=.5*(left+right);
          const auto height=z+middle*slope;
          if(g.kind==SU2Native3D::AnalyticSurface::Kind::Cylinder && height>=0 && height<=g.extent.y)continue;
          if(g.kind==SU2Native3D::AnalyticSurface::Kind::Disk) {
            auto radius2=[&](double t){Vector p{};for(unsigned j=0;j<3;++j)p[j]=radial[j]+t*velocity[j];return dot(p,p);};
            if(std::max(radius2(left),radius2(right))<=(long double)g.extent.x*g.extent.x)continue;
          }
          resolve(left,right);
        }
      }
      continue;
    }
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
    if(depth>=16) throw std::runtime_error("Geometric BL quadrature failed to resolve a wall band after 16 subdivisions.");
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
