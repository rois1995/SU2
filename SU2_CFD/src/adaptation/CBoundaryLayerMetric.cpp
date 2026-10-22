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
#include "../../../Common/include/adaptation/CNativeSupport3D.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <chrono>
#include <cstring>
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
#include "../../../Common/include/adaptation/CNativeChordEnvelope.hpp"
#include "../../include/adaptation/ConvexClipping.hpp"
#include "../../../Common/include/geometry/CGeometry.hpp"
#include "../../../Common/include/linear_algebra/blas_structure.hpp"
#include "../../include/solvers/CSolver.hpp"

namespace {

using Vec3 = std::array<su2double, 3>;
using Mat3 = su2double[3][3];
constexpr double NormalFeatureThreshold3D = 1e-10;

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

/*--- Evaluate the closest-feature displacement locally: subtracting a rounded global foot corrupts thin BL normals. ---*/
Vec3 TriangleDisplacement(const su2double* a,const su2double* b,const su2double* c,const su2double* p,
                          const su2double* weight) {
  Vec3 result{};
#if defined(CODI_FORWARD_TYPE) || defined(CODI_REVERSE_TYPE)
  for(unsigned j=0;j<3;++j)result[j]=p[j]-(weight[0]*a[j]+weight[1]*b[j]+weight[2]*c[j]);
#else
  using V=std::array<long double,3>;
  auto difference=[](const su2double* x,const su2double* y){return V{(long double)x[0]-y[0],(long double)x[1]-y[1],(long double)x[2]-y[2]};};
  auto dot=[](const V& x,const V& y){return x[0]*y[0]+x[1]*y[1]+x[2]*y[2];};
  const su2double* vertex[3]={a,b,c};unsigned indices[3],count=0;
  for(unsigned k=0;k<3;++k)if(weight[k]>0)indices[count++]=k;
  V displacement{};
  if(count==3) {
    const auto ab=difference(b,a),ac=difference(c,a),ap=difference(p,a);
    const V n{ab[1]*ac[2]-ab[2]*ac[1],ab[2]*ac[0]-ab[0]*ac[2],ab[0]*ac[1]-ab[1]*ac[0]};
    const auto t=dot(ap,n)/dot(n,n);
    for(unsigned j=0;j<3;++j)displacement[j]=t*n[j];
  }else {
    if(!count)throw std::runtime_error("Empty native3D closest triangle feature.");
    displacement=difference(p,vertex[indices[0]]);
    if(count==2) {
      const auto e=difference(vertex[indices[1]],vertex[indices[0]]);
      const auto t=std::clamp(dot(displacement,e)/dot(e,e),0.L,1.L);
      for(unsigned j=0;j<3;++j)displacement[j]=std::fma(-t,e[j],displacement[j]);
    }
  }
  for(unsigned j=0;j<3;++j)result[j]=double(displacement[j]);
#endif
  return result;
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
              Bytes(d.faceNormal),Bytes(d.edgeNormal),Bytes(d.vertexLog));
    for(const auto& row:d.segments)bytes=Add(bytes,Bytes(row));
    if(d.adt)bytes=Add(bytes,d.adt->GetAllocatedBytes(),d.adt->RetainedWorkspaceBound(1));
  }
  // D1 feature set: retained base/canonical features, carrier planes and the candidate grid.
  bytes=Add(bytes,Bytes(baseFeatures),Bytes(features),Bytes(featurePlanes),Bytes(featureHmax));
  for(const auto* list:{&baseFeatures,&features})for(const auto& f:*list)bytes=Add(bytes,Bytes(f.walls));
  bytes=Add(bytes,Mul(featureGrid.size(),sizeof(decltype(featureGrid)::value_type)+4*sizeof(void*)));
  for(const auto& row:featureGrid)bytes=Add(bytes,Bytes(row.second));
  return bytes;
}

CBoundaryLayerMetric::CBoundaryLayerMetric(unsigned short nDim, std::vector<Wall> walls_, su2double cornerAngle,
                                           FeatureOptions features_)
    : nDim(nDim), cornerAngle(cornerAngle), walls(std::move(walls_)) {
  Build();
  BuildFeatures(std::move(features_));
}

CBoundaryLayerMetric::CBoundaryLayerMetric(unsigned short nDim, std::vector<Wall> walls_, su2double cornerAngle)
    : CBoundaryLayerMetric(nDim, std::move(walls_), cornerAngle, FeatureOptions()) {}

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

  /*--- D1 feature detection needs the COMPLETE original walls and their neighbouring physical faces on every rank
   *    (the restricted wall subsets above are not enough: a feature can act farther than their padding). Gather every
   *    physical boundary face once, fluid-oriented (3D normal out of the fluid, 2D fluid on the left), deduplicated
   *    by global point indices; only the detected feature set is retained. ---*/
  FeatureOptions options;
  options.hgrad = SU2_TYPE::GetValue(config.GetAdap_Hgrad());
  options.wallFacesIncluded = true;
  {
    std::vector<unsigned long> localIds;  // per face: nNode, marker (config-file index), global point indices
    std::vector<passivedouble> localCoord;
    for (unsigned short iMarker = 0; iMarker < geometry.GetnMarker(); ++iMarker) {
      if (config.GetMarker_All_KindBC(iMarker) == SEND_RECEIVE) continue;
      const auto tag = config.GetMarker_All_TagBound(iMarker);
      unsigned long cfg = config.GetnMarker_CfgFile();
      for (unsigned short k = 0; k < config.GetnMarker_CfgFile(); ++k)
        if (config.GetMarker_CfgFile_TagBound(k) == tag) cfg = k;
      if (cfg == config.GetnMarker_CfgFile()) continue;
      for (auto iElem = 0ul; iElem < geometry.GetnElem_Bound(iMarker); ++iElem) {
        const auto* face = geometry.bound[iMarker][iElem];
        const auto type = face->GetVTK_Type();
        if (type == VERTEX) continue;
        if ((nDim == 2) ? (type != LINE) : (type != TRIANGLE && type != QUADRILATERAL)) continue;
        const unsigned short nNode = face->GetnNodes();
        unsigned long node[4];
        for (unsigned short iNode = 0; iNode < nNode; ++iNode) node[iNode] = face->GetNode(iNode);
        const auto* elem = geometry.elem[face->GetDomainElement()];
        su2double faceCenter[3] = {0.0}, elemCenter[3] = {0.0};
        for (unsigned short iNode = 0; iNode < nNode; ++iNode)
          for (unsigned short iDim = 0; iDim < nDim; ++iDim)
            faceCenter[iDim] += geometry.nodes->GetCoord(node[iNode], iDim) / nNode;
        for (unsigned short iNode = 0; iNode < elem->GetnNodes(); ++iNode)
          for (unsigned short iDim = 0; iDim < nDim; ++iDim)
            elemCenter[iDim] += geometry.nodes->GetCoord(elem->GetNode(iNode), iDim) / elem->GetnNodes();
        auto X = [&](unsigned short k, unsigned short iDim) { return geometry.nodes->GetCoord(node[k], iDim); };
        if (nDim == 2) {
          /*--- Fluid (the element centre) on the left of a -> b. ---*/
          const su2double side = (X(1, 0) - X(0, 0)) * (elemCenter[1] - X(0, 1)) - (X(1, 1) - X(0, 1)) * (elemCenter[0] - X(0, 0));
          if (side < 0.0) std::swap(node[0], node[1]);
        } else {
          su2double a[3], b[3], c[3];
          for (unsigned short iDim = 0; iDim < 3; ++iDim) {
            a[iDim] = X(1, iDim) - X(0, iDim);
            b[iDim] = X(2, iDim) - X(0, iDim);
            c[iDim] = X(nNode - 1, iDim) - X(0, iDim);
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
        localIds.push_back(cfg);
        for (unsigned short iNode = 0; iNode < nNode; ++iNode) {
          localIds.push_back(geometry.nodes->GetGlobalIndex(node[iNode]));
          for (unsigned short iDim = 0; iDim < nDim; ++iDim)
            localCoord.push_back(SU2_TYPE::GetValue(geometry.nodes->GetCoord(node[iNode], iDim)));
        }
      }
    }
    const auto allIds = CPassiveComm::Allgatherv(localIds, nullptr);
    const auto allCoord = CPassiveComm::Allgatherv(localCoord, nullptr);
    std::set<std::vector<unsigned long>> seen;
    for (size_t iId = 0, iCoord = 0; iId < allIds.size();) {
      const auto nNode = allIds[iId], cfg = allIds[iId + 1];
      std::vector<unsigned long> key(allIds.begin() + iId + 2, allIds.begin() + iId + 2 + nNode);
      FeatureFace face;
      face.coord.assign(allCoord.begin() + iCoord, allCoord.begin() + iCoord + nNode * nDim);
      iId += 2 + nNode;
      iCoord += nNode * nDim;
      std::sort(key.begin(), key.end());
      if (!seen.insert(key).second) continue;
      const auto name = config.GetMarker_CfgFile_TagBound(static_cast<unsigned short>(cfg));
      for (unsigned short iBL = 0; iBL < config.GetnAdap_BL(); ++iBL)
        if (config.GetAdap_BL(iBL).marker == name) face.wall = name;
      face.reflective = face.wall.empty() && config.GetMarker_CfgFile_KindBC(name) == SYMMETRY_PLANE;
      options.faces.push_back(std::move(face));
    }
  }
  BuildFeatures(std::move(options));
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
      for (auto iSeg = 0ul; iSeg < nSeg; ++iSeg) {
        const auto a = d->elem[2 * iSeg], b = d->elem[2 * iSeg + 1];
        const su2double v[2] = {x(b)[0] - x(a)[0], x(b)[1] - x(a)[1]};
        d->length[iSeg] = sqrt(v[0] * v[0] + v[1] * v[1]);
        if (!(d->length[iSeg] > 0.0)) SU2_MPI::Error("Wall " + name + " has an edge of zero length.", CURRENT_FUNCTION);
        rays[a].push_back({v[0] / d->length[iSeg], v[1] / d->length[iSeg], 0.0});
        rays[b].push_back({-v[0] / d->length[iSeg], -v[1] / d->length[iSeg], 0.0});
        d->segments[a].push_back(iSeg);
        d->segments[b].push_back(iSeg);
      }

      /*--- Vertex normal: bisector of the two rays where the wall turns, else normal of the ray. (Convex vertices
       *    turning by more than ADAP_ANGLE are D1 features, BuildFeatures; there is no separate corner limit.) ---*/
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
        } else {
          d->vertexNormal[iPoint] = {-r[0][1], r[0][0], 0.0};
        }
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

namespace {
/*--- Symmetric 3x3 eigen-decomposition by cyclic Jacobi in long double (columns of V are the eigenvectors). ---*/
bool JacobiEigen3(long double (&A)[3][3], long double (&V)[3][3], long double (&w)[3]) {
  long double norm = 0;
  for (unsigned i = 0; i < 3; ++i)
    for (unsigned j = 0; j < 3; ++j) {
      V[i][j] = (i == j) ? 1 : 0;
      norm += A[i][j] * A[i][j];
    }
  for (unsigned sweep = 0; sweep < 64; ++sweep) {
    const long double off = A[0][1] * A[0][1] + A[0][2] * A[0][2] + A[1][2] * A[1][2];
    if (!(off > 1e-36L * norm)) {
      for (unsigned k = 0; k < 3; ++k) w[k] = A[k][k];
      return true;
    }
    for (unsigned p = 0; p < 2; ++p)
      for (unsigned q = p + 1; q < 3; ++q) {
        if (A[p][q] == 0) continue;
        const long double theta = (A[q][q] - A[p][p]) / (2 * A[p][q]);
        const long double t = (theta >= 0 ? 1 : -1) / (std::abs(theta) + std::sqrt(theta * theta + 1));
        const long double c = 1 / std::sqrt(t * t + 1), s = t * c;
        for (unsigned k = 0; k < 3; ++k) {
          const long double akp = A[k][p], akq = A[k][q];
          A[k][p] = c * akp - s * akq;
          A[k][q] = s * akp + c * akq;
        }
        for (unsigned k = 0; k < 3; ++k) {
          const long double apk = A[p][k], aqk = A[q][k];
          A[p][k] = c * apk - s * aqk;
          A[q][k] = s * apk + c * aqk;
        }
        for (unsigned k = 0; k < 3; ++k) {
          const long double vkp = V[k][p], vkq = V[k][q];
          V[k][p] = c * vkp - s * vkq;
          V[k][q] = s * vkp + c * vkq;
        }
      }
  }
  return false;
}

/*--- Logarithm of a D1-composed 3D wall tensor: long-double Jacobi with an a-posteriori residual and orthogonality
 *    check and a positive spectrum, never the analytic frame route (the composed tensor has no wall frame). ---*/
void LogarithmSPD3(const su2double (&X)[3][3], su2double (&L)[3][3]) {
  long double scale = 0;
  for (unsigned i = 0; i < 3; ++i)
    for (unsigned j = 0; j < 3; ++j) scale = std::max(scale, std::abs((long double)SU2_TYPE::GetValue(X[i][j])));
  if (!(scale > 0) || !std::isfinite(scale)) throw std::runtime_error("Native BL D1 tensor is not finite.");
  long double A[3][3], B[3][3], V[3][3], w[3];
  for (unsigned i = 0; i < 3; ++i)
    for (unsigned j = 0; j < 3; ++j) A[i][j] = B[i][j] = SU2_TYPE::GetValue(X[i][j]) / scale;
  if (!JacobiEigen3(A, V, w)) throw std::runtime_error("Native BL D1 logarithm: Jacobi iteration did not converge.");
  long double residual = 0, orthogonality = 0, norm = 0;
  for (unsigned i = 0; i < 3; ++i)
    for (unsigned j = 0; j < 3; ++j) {
      long double r = -V[i][j] * w[j], o = (i == j) ? -1 : 0;
      for (unsigned k = 0; k < 3; ++k) {
        r += B[i][k] * V[k][j];
        o += V[k][i] * V[k][j];
      }
      residual += r * r;
      orthogonality += o * o;
      norm += B[i][j] * B[i][j];
    }
  if (!(std::sqrt(residual) <= 1e-15L * std::sqrt(norm)) || !(std::sqrt(orthogonality) <= 1e-15L))
    throw std::runtime_error("Native BL D1 logarithm fails its residual/orthogonality check.");
  long double logarithm[3];
  for (unsigned k = 0; k < 3; ++k) {
    if (!(w[k] > 0)) throw std::runtime_error("Native BL D1 tensor is not positive definite.");
    logarithm[k] = std::log(w[k]) + std::log(scale);
  }
  for (unsigned i = 0; i < 3; ++i)
    for (unsigned j = 0; j <= i; ++j) {
      long double value = 0;
      for (unsigned k = 0; k < 3; ++k) value += V[i][k] * logarithm[k] * V[j][k];
      L[i][j] = L[j][i] = double(value);
    }
}

/*--- D1 intersection A ∩ F in long double, the same S = A + F whitening as CSolver::IntersectMetrics (in that frame A
 *    has eigenvalues l in [0,1] and the intersection max(l, 1 - l)). Binary64 whitening loses ~1e-7 of the largest
 *    eigenvalue at BL anisotropy (1e11), so the D1 fold is evaluated with the long-double Jacobi route. ---*/
void IntersectLD(unsigned short n, const su2double (&A)[3][3], const su2double (&B)[3][3], su2double (&C)[3][3]) {
  long double scale = 0;
  for (unsigned i = 0; i < n; ++i)
    for (unsigned j = 0; j < n; ++j)
      scale = std::max({scale, std::abs((long double)SU2_TYPE::GetValue(A[i][j])), std::abs((long double)SU2_TYPE::GetValue(B[i][j]))});
  if (!(scale > 0) || !std::isfinite(scale)) throw std::runtime_error("Native BL D1 intersection needs finite SPD tensors.");
  long double a[3][3] = {{0}}, s[3][3] = {{0}}, V[3][3], w[3];
  for (unsigned i = 0; i < 3; ++i)
    for (unsigned j = 0; j < 3; ++j) {
      a[i][j] = (i < n && j < n) ? SU2_TYPE::GetValue(A[i][j]) / scale : (i == j ? 0.5L : 0.0L);
      s[i][j] = (i < n && j < n) ? a[i][j] + SU2_TYPE::GetValue(B[i][j]) / scale : (i == j ? 1.0L : 0.0L);
    }
  if (!JacobiEigen3(s, V, w)) throw std::runtime_error("Native BL D1 intersection: Jacobi iteration did not converge.");
  long double root[3][3], inverse[3][3];
  for (unsigned k = 0; k < 3; ++k)
    if (!(w[k] > 0)) throw std::runtime_error("Native BL D1 intersection sum is not positive definite.");
  for (unsigned i = 0; i < 3; ++i)
    for (unsigned j = 0; j < 3; ++j) {
      root[i][j] = inverse[i][j] = 0;
      for (unsigned k = 0; k < 3; ++k) {
        root[i][j] += V[i][k] * std::sqrt(w[k]) * V[j][k];
        inverse[i][j] += V[i][k] / std::sqrt(w[k]) * V[j][k];
      }
    }
  long double t[3][3], tmp[3][3], U[3][3], l[3];
  for (unsigned i = 0; i < 3; ++i)
    for (unsigned j = 0; j < 3; ++j) {
      tmp[i][j] = 0;
      for (unsigned k = 0; k < 3; ++k) tmp[i][j] += inverse[i][k] * a[k][j];
    }
  for (unsigned i = 0; i < 3; ++i)
    for (unsigned j = 0; j < 3; ++j) {
      t[i][j] = 0;
      for (unsigned k = 0; k < 3; ++k) t[i][j] += tmp[i][k] * inverse[k][j];
    }
  for (unsigned i = 0; i < 3; ++i)
    for (unsigned j = 0; j < i; ++j) t[i][j] = t[j][i] = 0.5L * (t[i][j] + t[j][i]);
  if (!JacobiEigen3(t, U, l)) throw std::runtime_error("Native BL D1 intersection: Jacobi iteration did not converge.");
  for (auto& x : l) x = std::max(x, 1 - x);
  for (unsigned i = 0; i < 3; ++i)
    for (unsigned j = 0; j < 3; ++j) {
      t[i][j] = 0;
      for (unsigned k = 0; k < 3; ++k) t[i][j] += U[i][k] * l[k] * U[j][k];
    }
  for (unsigned i = 0; i < 3; ++i)
    for (unsigned j = 0; j < 3; ++j) {
      tmp[i][j] = 0;
      for (unsigned k = 0; k < 3; ++k) tmp[i][j] += root[i][k] * t[k][j];
    }
  long double r[3][3];
  for (unsigned i = 0; i < 3; ++i)
    for (unsigned j = 0; j < 3; ++j) {
      r[i][j] = 0;
      for (unsigned k = 0; k < 3; ++k) r[i][j] += tmp[i][k] * root[k][j];
    }
  for (unsigned i = 0; i < n; ++i)
    for (unsigned j = 0; j < n; ++j) C[i][j] = double(scale * 0.5L * (r[i][j] + r[j][i]));
}

using FeaturePoint = std::array<double, 3>;
std::array<double, 7> FeatureOrder(int rank, const FeaturePoint& a, const FeaturePoint& b) {
  return {double(rank), a[0], a[1], a[2], b[0], b[1], b[2]};
}
}  // namespace

double CBoundaryLayerMetric::FeatureRadius(unsigned short wall, double h0) const {
  /*--- Largest normal size inside the band (d <= T; the outer-normal profile at its growth limit) and the largest
   *    tangential size: the pre-fade wall tensor A >= hmax^-2 I, so F <= hn_f(rho)^-2 I <= A beyond this radius. ---*/
  const auto& layer = walls[wall].layer;
  const double h0w = SU2_TYPE::GetValue(layer.firstHeight), g = SU2_TYPE::GetValue(layer.growth),
               T = SU2_TYPE::GetValue(layer.thickness);
  double hn = std::max(h0w, 2 * (h0w + (g - 1) * T) / (g + 1));
  if (outerNormal.version && T > outerNormal.protected_distance) {
    const double p = outerNormal.protected_distance;
    const double anchor = std::max(h0w, 2 * (h0w + (g - 1) * p) / (g + 1));
    const double limit = 2 * (1 - 2 / (outerNormal.maximum_growth + 1));
    hn = std::max(hn, anchor + limit * (T - p));
  }
  const double hmax = std::max(featureHmax.at(wall), hn) * (1 + 1e-12);
  return std::max(0.0, (hmax - h0) / featureSlope);
}

void CBoundaryLayerMetric::BuildFeatures(FeatureOptions options) {
  baseFeatures.clear();
  features.clear();
  featurePlanes.clear();
  featureGrid.clear();
  featureSummary = FeatureSummary();
  featureSlope = 0;
  featureCell = 0;
  featureHmax.assign(walls.size(), 0.0);
  if (!(options.hgrad > 1.0)) return;  // D1 disabled (test constructor default)
  const double angle = SU2_TYPE::GetValue(cornerAngle);
  if (!std::isfinite(options.hgrad) || !(angle >= 0 && angle < 180))
    throw std::invalid_argument("Invalid D1 feature angle or gradation.");
  featureSlope = std::log(options.hgrad);
  featureSummary.angle = angle;
  featureSummary.slope = featureSlope;
  const double pi = std::acos(-1.0);
  auto degrees = [&](long double cosine) { return double(std::acos(std::clamp(cosine, -1.0L, 1.0L))) * 180 / pi; };

  /*--- Original faces of all BL walls and the partner faces; vertex identity by exact coordinates. ---*/
  std::map<std::string, int> wallIndex;
  for (unsigned short w = 0; w < walls.size(); ++w) wallIndex[walls[w].layer.marker] = w;
  std::map<FeaturePoint, unsigned> index;
  std::vector<FeaturePoint> P;
  auto vertex = [&](const su2double* x) {
    const FeaturePoint p{SU2_TYPE::GetValue(x[0]), SU2_TYPE::GetValue(x[1]), nDim == 3 ? SU2_TYPE::GetValue(x[2]) : 0.0};
    const auto it = index.emplace(p, unsigned(P.size()));
    if (it.second) P.push_back(p);
    return it.first->second;
  };
  struct Face { std::array<unsigned, 3> v; int wall; bool reflective; };
  std::vector<Face> faces;
  /*--- Largest tangential wall size from the COMPLETE original wall faces (rank independent even where a rank holds
   *    a wall subset): 2D the longest segment; 3D the largest eigenvalue of the face size tensors 2/n sum e e^T, which
   *    bounds the vertex log-means, their projection and the curvature caps of every sample. ---*/
  auto wallSize = [&](const std::vector<unsigned>& ids, int wall) {
    if (wall < 0) return;
    const size_t n = ids.size();
    su2double C[3][3] = {{0.0}}, vec[3][3], val[3], work[3];
    for (size_t k = 0; k < n; ++k) {
      double e[3] = {0, 0, 0};
      for (unsigned j = 0; j < nDim; ++j) e[j] = P[ids[(k + 1) % n]][j] - P[ids[k]][j];
      if (nDim == 2) {
        if (k == 0) featureHmax[wall] = std::max(featureHmax[wall], std::hypot(e[0], e[1]) * (1 + 1e-12));
        continue;
      }
      for (unsigned i = 0; i < 3; ++i)
        for (unsigned j = 0; j < 3; ++j) C[i][j] += 2.0 / n * e[i] * e[j];
    }
    if (nDim == 2) return;
    CBlasStructure::EigenDecomposition(C, vec, val, 3, work);
    featureHmax[wall] = std::max(featureHmax[wall], std::sqrt(SU2_TYPE::GetValue(*std::max_element(val, val + 3))) * (1 + 1e-12));
  };
  auto add = [&](const std::vector<unsigned>& ids, int wall, bool reflective) {
    wallSize(ids, wall);
    if (nDim == 2) {
      if (ids[0] == ids[1]) throw std::invalid_argument("Degenerate D1 boundary segment.");
      faces.push_back({{ids[0], ids[1], ids[1]}, wall, reflective});
      return;
    }
    faces.push_back({{ids[0], ids[1], ids[2]}, wall, reflective});
    if (ids.size() == 4) faces.push_back({{ids[0], ids[2], ids[3]}, wall, reflective});
  };
  if (!options.wallFacesIncluded)
    for (unsigned short w = 0; w < walls.size(); ++w) {
      const auto& wall = walls[w];
      const unsigned stride = (nDim == 2) ? 2 : 4;
      for (size_t k = 0; k + stride <= wall.conn.size(); k += stride) {
        const unsigned n = (nDim == 2) ? 2 : (wall.conn[k + 3] == wall.conn[k + 2] ? 3 : 4);
        std::vector<unsigned> ids;
        for (unsigned j = 0; j < n; ++j) ids.push_back(vertex(&wall.coord[nDim * wall.conn[k + j]]));
        add(ids, w, false);
      }
    }
  for (const auto& face : options.faces) {
    int w = -1;
    if (!face.wall.empty()) {
      const auto it = wallIndex.find(face.wall);
      if (!options.wallFacesIncluded || it == wallIndex.end() || face.reflective)
        throw std::invalid_argument("Invalid D1 wall face of marker " + face.wall + ".");
      w = it->second;
    }
    const size_t n = face.coord.size() / nDim;
    if (face.coord.size() % nDim || (nDim == 2 ? n != 2 : (n != 3 && n != 4)))
      throw std::invalid_argument("Invalid D1 feature face.");
    std::vector<unsigned> ids;
    for (size_t j = 0; j < n; ++j) ids.push_back(vertex(&face.coord[nDim * j]));
    add(ids, w, face.reflective);
  }
  options.faces.clear();
  options.faces.shrink_to_fit();

  using LD3 = std::array<long double, 3>;
  auto point = [&](const FeaturePoint& p) { return SU2Native3D::Point{p[0], p[1], p[2]}; };
  auto unitNormal = [&](const Face& f) {
    LD3 a{}, b{};
    for (unsigned j = 0; j < 3; ++j) {
      a[j] = (long double)P[f.v[1]][j] - P[f.v[0]][j];
      b[j] = (long double)P[f.v[2]][j] - P[f.v[0]][j];
    }
    LD3 n{a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]};
    const auto length = std::sqrt(n[0] * n[0] + n[1] * n[1] + n[2] * n[2]);
    if (!(length > 0)) throw std::invalid_argument("Degenerate D1 boundary face.");
    for (auto& x : n) x /= length;
    return n;
  };
  auto reflect = [&](const FeaturePlane& plane, const FeaturePoint& x) {
    long double distance = 0;
    for (unsigned j = 0; j < 3; ++j) distance += ((long double)x[j] - plane.origin[j]) * plane.normal[j];
    FeaturePoint y{};
    for (unsigned j = 0; j < 3; ++j) y[j] = double(x[j] - 2 * distance * plane.normal[j]);
    return y;
  };
  /*--- A symmetry marker is a plane by declaration, but a reference that was not adopted to an exact plane carries
   *    rounding-level offsets (M6 legacy reference: |y| <= 4.6e-12). Carriers are therefore recognized up to
   *    1e-9 of the coordinate scale with parallel normals; exact-plane references group identically. ---*/
  long double scale = 0;
  for (const auto& p : P)
    for (const auto x : p) scale = std::max(scale, std::abs((long double)x));
  const long double carrierTolerance = 1e-9L * std::max(scale, (long double)std::numeric_limits<double>::min());
  auto onPlane = [&](const FeaturePlane& plane, const FeaturePoint& x) {
    long double d = 0;
    for (unsigned j = 0; j < 3; ++j) d += ((long double)x[j] - plane.origin[j]) * plane.normal[j];
    return std::abs(d) <= plane.tolerance;
  };
  auto direction2D = [&](const FeaturePoint& a, const FeaturePoint& b) {
    const long double dx = (long double)b[0] - a[0], dy = (long double)b[1] - a[1];
    const auto length = std::hypot(dx, dy);
    if (!(length > 0)) throw std::invalid_argument("Degenerate D1 boundary segment.");
    return std::array<long double, 3>{-dy / length, dx / length, 0};
  };
  auto parallel = [&](const FeaturePlane& plane, const Face& f) {
    const auto n = nDim == 3 ? unitNormal(f) : direction2D(P[f.v[0]], P[f.v[1]]);
    return std::abs(n[0] * plane.normal[0] + n[1] * plane.normal[1] + n[2] * plane.normal[2]) >= 1 - 1e-12L;
  };

  /*--- Reflective carriers (symmetry planes/lines) touching the BL walls, grouped by exact coplanarity. ---*/
  std::vector<char> wallVertex(P.size(), 0);
  for (const auto& f : faces)
    if (f.wall >= 0)
      for (unsigned k = 0; k < nDim; ++k) wallVertex[f.v[k]] = 1;
  std::vector<int> facePlane(faces.size(), -1);
  for (size_t i = 0; i < faces.size(); ++i) {
    const auto& f = faces[i];
    if (f.wall >= 0 || !f.reflective) continue;
    bool touches = false;
    for (unsigned k = 0; k < nDim; ++k) touches = touches || wallVertex[f.v[k]];
    if (!touches) continue;
    for (size_t p = 0; p < featurePlanes.size() && facePlane[i] < 0; ++p) {
      bool same = parallel(featurePlanes[p], f);
      for (unsigned k = 0; k < nDim; ++k) same = same && onPlane(featurePlanes[p], P[f.v[k]]);
      if (same) facePlane[i] = int(p);
    }
    if (facePlane[i] >= 0) continue;
    FeaturePlane plane;
    for (unsigned k = 0; k < 3; ++k) plane.face[k] = P[f.v[k]];
    for (unsigned j = 0; j < 3; ++j) plane.origin[j] = P[f.v[0]][j];
    plane.normal = nDim == 3 ? unitNormal(f) : direction2D(P[f.v[0]], P[f.v[1]]);
    plane.tolerance = carrierTolerance;
    featurePlanes.push_back(plane);
    facePlane[i] = int(featurePlanes.size() - 1);
  }
  /*--- Canonical carrier transforms, independent of the face order: each plane is defined by its member face with
   *    the lexicographically smallest sorted vertices (taken in sorted order); planes are ordered by that face. ---*/
  {
    using FaceKey = std::array<FeaturePoint, 3>;
    std::vector<FaceKey> canonical(featurePlanes.size());
    std::vector<char> set(featurePlanes.size(), 0);
    for (size_t i = 0; i < faces.size(); ++i) {
      if (facePlane[i] < 0) continue;
      FaceKey key{P[faces[i].v[0]], P[faces[i].v[1]], P[faces[i].v[2]]};
      std::sort(key.begin(), key.begin() + nDim);
      if (nDim == 2) key[2] = key[1];
      auto& best = canonical[facePlane[i]];
      if (!set[facePlane[i]] || key < best) best = key;
      set[facePlane[i]] = 1;
    }
    std::vector<unsigned> order(featurePlanes.size()), rank(featurePlanes.size());
    std::iota(order.begin(), order.end(), 0u);
    std::sort(order.begin(), order.end(), [&](unsigned x, unsigned y) { return canonical[x] < canonical[y]; });
    std::vector<FeaturePlane> planes;
    for (unsigned r = 0; r < order.size(); ++r) {
      rank[order[r]] = r;
      const auto& key = canonical[order[r]];
      FeaturePlane plane;
      plane.face = key;
      plane.tolerance = carrierTolerance;
      for (unsigned j = 0; j < 3; ++j) plane.origin[j] = key[0][j];
      plane.normal = nDim == 3 ? unitNormal({{index.at(key[0]), index.at(key[1]), index.at(key[2])}, -1, true})
                               : direction2D(key[0], key[1]);
      planes.push_back(plane);
    }
    featurePlanes = std::move(planes);
    for (auto& p : facePlane)
      if (p >= 0) p = int(rank[p]);
  }
  featureSummary.planes = featurePlanes.size();

  auto h0Of = [&](int w) { return SU2_TYPE::GetValue(walls[w].layer.firstHeight); };
  auto merge = [](std::vector<unsigned short>& list, int w) {
    if (w < 0) return;
    if (!std::binary_search(list.begin(), list.end(), (unsigned short)w))
      list.insert(std::upper_bound(list.begin(), list.end(), (unsigned short)w), (unsigned short)w);
  };
  auto makeSegment = [&](const FeaturePoint& a, const FeaturePoint& b) {
    Feature f;
    f.a = std::min(a, b);
    f.b = std::max(a, b);
    LD3 t{};
    long double length = 0;
    for (unsigned j = 0; j < 3; ++j) {
      t[j] = (long double)f.b[j] - f.a[j];
      length += t[j] * t[j];
    }
    length = std::sqrt(length);
    if (!(length > 0)) throw std::invalid_argument("Degenerate D1 feature segment.");
    for (unsigned j = 0; j < 3; ++j) f.t[j] = double(t[j] / length);
    f.length = double(length);
    return f;
  };

  if (nDim == 3) {
    /*--- Crease edges: a manifold edge of a BL face whose other side (a BL face, a non-reflective partner, or the
     *    mirror image of the BL face in a reflective partner's plane) makes it convex and turn by more than the
     *    angle. Convexity: each opposite vertex strictly on the solid side of the other face (exact predicate). ---*/
    struct Segment { unsigned a, b; std::vector<unsigned short> walls; double h0; };
    std::vector<Segment> segments;
    std::map<std::pair<unsigned, unsigned>, std::vector<unsigned>> edges;
    auto edgeKey = [](unsigned x, unsigned y) { return std::make_pair(std::min(x, y), std::max(x, y)); };
    for (size_t i = 0; i < faces.size(); ++i)
      if (faces[i].wall >= 0)
        for (unsigned k = 0; k < 3; ++k) edges[edgeKey(faces[i].v[k], faces[i].v[(k + 1) % 3])].push_back(unsigned(i));
    for (size_t i = 0; i < faces.size(); ++i)
      if (faces[i].wall < 0)
        for (unsigned k = 0; k < 3; ++k) {
          const auto it = edges.find(edgeKey(faces[i].v[k], faces[i].v[(k + 1) % 3]));
          if (it != edges.end()) it->second.push_back(unsigned(i));
        }
    for (const auto& row : edges) {
      if (row.second.size() == 1) continue;  // border of the supplied surface: its end vertices are classified below
      if (row.second.size() > 2) throw std::invalid_argument("Non-manifold D1 boundary edge.");
      const auto& f1 = faces[row.second[0]];
      const auto& f2 = faces[row.second[1]];
      unsigned k = 0;
      while (edgeKey(f1.v[k], f1.v[(k + 1) % 3]) != row.first) ++k;
      const unsigned a = f1.v[k], b = f1.v[(k + 1) % 3], o1 = f1.v[(k + 2) % 3];
      const auto n1 = unitNormal(f1);
      LD3 n2{};
      bool convex = false;
      if (f2.wall < 0 && f2.reflective) {
        const auto& plane = featurePlanes.at(facePlane[row.second[1]]);
        const auto image = reflect(plane, P[o1]);
        long double dot = 0;
        for (unsigned j = 0; j < 3; ++j) dot += n1[j] * plane.normal[j];
        for (unsigned j = 0; j < 3; ++j) n2[j] = n1[j] - 2 * dot * plane.normal[j];
        convex = SU2Native3D::Orientation(point(P[a]), point(P[b]), point(P[o1]), point(image)) > 0;
      } else {
        unsigned o2 = f2.v[0];
        for (const auto v : f2.v)
          if (v != a && v != b) o2 = v;
        n2 = unitNormal(f2);
        const auto s1 = SU2Native3D::Orientation(point(P[a]), point(P[b]), point(P[o1]), point(P[o2]));
        const auto s2 = SU2Native3D::Orientation(point(P[f2.v[0]]), point(P[f2.v[1]]), point(P[f2.v[2]]), point(P[o1]));
        if ((s1 > 0 && s2 < 0) || (s1 < 0 && s2 > 0))
          throw std::invalid_argument("Inconsistently oriented D1 boundary faces (3D faces need normals out of the fluid).");
        convex = s1 > 0 && s2 > 0;
      }
      if (!convex) continue;
      if (!(degrees(n1[0] * n2[0] + n1[1] * n2[1] + n1[2] * n2[2]) > angle)) continue;
      Segment segment{a, b, {}, h0Of(f1.wall)};
      merge(segment.walls, f1.wall);
      if (f2.wall >= 0) {
        merge(segment.walls, f2.wall);
        segment.h0 = std::min(segment.h0, h0Of(f2.wall));
      }
      segments.push_back(std::move(segment));
    }

    /*--- Lines and corners: >= 3 feature edges, a line turning by more than the angle, or a line end off a
     *    reflective carrier. A carrier end continues into its mirror image unless segment + image turn by more. ---*/
    std::map<unsigned, std::vector<std::pair<unsigned, unsigned>>> incident;
    for (unsigned s = 0; s < segments.size(); ++s) {
      incident[segments[s].a].push_back({segments[s].b, s});
      incident[segments[s].b].push_back({segments[s].a, s});
    }
    std::map<unsigned, std::vector<unsigned>> partners;
    for (size_t i = 0; i < faces.size(); ++i)
      if (faces[i].wall < 0)
        for (const auto v : faces[i].v)
          if (incident.count(v)) {
            auto& list = partners[v];
            if (list.empty() || list.back() != i) list.push_back(unsigned(i));
          }
    /*--- A corner belongs to every BL wall incident to its vertex (also a finer marker that meets it without a
     *    crease of its own) and takes the smallest of their first heights. ---*/
    std::map<unsigned, std::vector<unsigned short>> vertexWalls;
    for (const auto& f : faces)
      if (f.wall >= 0)
        for (const auto v : f.v)
          if (incident.count(v)) merge(vertexWalls[v], f.wall);
    std::map<unsigned, Feature> corners;
    auto addCorner = [&](unsigned v) {
      auto& corner = corners[v];
      corner.corner = true;
      corner.a = corner.b = P[v];
      corner.walls = vertexWalls.at(v);
      corner.h0 = std::numeric_limits<double>::max();
      for (const auto w : corner.walls) corner.h0 = std::min(corner.h0, h0Of(w));
    };
    auto direction = [&](const FeaturePoint& from, const FeaturePoint& to) {
      LD3 d{};
      long double length = 0;
      for (unsigned j = 0; j < 3; ++j) {
        d[j] = (long double)to[j] - from[j];
        length += d[j] * d[j];
      }
      length = std::sqrt(length);
      for (auto& x : d) x /= length;
      return d;
    };
    for (const auto& row : incident) {
      const unsigned v = row.first;
      const auto& nb = row.second;
      if (nb.size() >= 3) {
        addCorner(v);
        continue;
      }
      if (nb.size() == 2) {
        const auto u = direction(P[nb[0].first], P[v]), w = direction(P[v], P[nb[1].first]);
        if (degrees(u[0] * w[0] + u[1] * w[1] + u[2] * w[2]) > angle) addCorner(v);
        continue;
      }
      const auto it = partners.find(v);
      int plane = -1;
      bool carrier = it != partners.end();
      if (carrier)
        for (const auto i : it->second) {
          if (!faces[i].reflective || facePlane[i] < 0 || (plane >= 0 && plane != facePlane[i])) {
            carrier = false;
            break;
          }
          plane = facePlane[i];
        }
      if (!carrier) {
        addCorner(v);
        ++featureSummary.freeCorners;
        continue;
      }
      ++featureSummary.carrierEnds;
      const auto image = reflect(featurePlanes[plane], P[nb[0].first]);
      const auto u = direction(P[nb[0].first], P[v]), w = direction(P[v], image);
      if (degrees(u[0] * w[0] + u[1] * w[1] + u[2] * w[2]) > angle) {
        addCorner(v);
        ++featureSummary.carrierCorners;
      }
    }
    for (const auto& segment : segments) {
      auto f = makeSegment(P[segment.a], P[segment.b]);
      f.walls = segment.walls;
      f.h0 = segment.h0;
      baseFeatures.push_back(std::move(f));
    }
    for (auto& row : corners) baseFeatures.push_back(std::move(row.second));
  } else {
    /*--- 2D feature vertices: the wall turns right (fluid on the left) by more than the angle; the other side of a
     *    BL segment is a BL segment, a non-reflective partner, or the mirror image of the BL segment. ---*/
    std::map<unsigned, std::vector<unsigned>> in, out;
    for (size_t i = 0; i < faces.size(); ++i) {
      out[faces[i].v[0]].push_back(unsigned(i));
      in[faces[i].v[1]].push_back(unsigned(i));
    }
    for (unsigned v = 0; v < P.size(); ++v) {
      if (!wallVertex[v]) continue;
      const auto ii = in.find(v), oo = out.find(v);
      if (ii == in.end() || oo == out.end() || ii->second.size() != 1 || oo->second.size() != 1) continue;
      const auto& fi = faces[ii->second[0]];
      const auto& fo = faces[oo->second[0]];
      auto p = P[fi.v[0]], q = P[fo.v[1]];
      if (fi.wall < 0 && fi.reflective) p = reflect(featurePlanes.at(facePlane[ii->second[0]]), q);
      if (fo.wall < 0 && fo.reflective) q = reflect(featurePlanes.at(facePlane[oo->second[0]]), p);
      if (!(SU2Native2D::Orientation(p[0], p[1], P[v][0], P[v][1], q[0], q[1]) < 0)) continue;
      const long double ux = (long double)P[v][0] - p[0], uy = (long double)P[v][1] - p[1];
      const long double wx = (long double)q[0] - P[v][0], wy = (long double)q[1] - P[v][1];
      if (!(degrees((ux * wx + uy * wy) / (std::hypot(ux, uy) * std::hypot(wx, wy))) > angle)) continue;
      Feature f;
      f.corner = true;
      f.a = f.b = P[v];
      f.h0 = std::numeric_limits<double>::max();
      for (const auto* side : {&fi, &fo})
        if (side->wall >= 0) {
          merge(f.walls, side->wall);
          f.h0 = std::min(f.h0, h0Of(side->wall));
        }
      baseFeatures.push_back(std::move(f));
    }
  }
  std::sort(baseFeatures.begin(), baseFeatures.end(), [](const Feature& x, const Feature& y) {
    return FeatureOrder(x.corner ? 2 : 0, x.a, x.b) < FeatureOrder(y.corner ? 2 : 0, y.a, y.b);
  });
  IndexFeatures();
}

void CBoundaryLayerMetric::IndexFeatures() {
  features.clear();
  featureGrid.clear();
  featureCell = 0;
  if (!(featureSlope > 0)) return;
  auto radius = [&](const Feature& f) {
    double r = 0;
    for (const auto w : f.walls) r = std::max(r, FeatureRadius(w, f.h0));
    return r;
  };
  auto rank = [](const Feature& f) { return (f.corner ? 2 : 0) + (f.image ? 1 : 0); };

  /*--- Mirror images (one reflection) of every feature within its support radius of a reflective carrier, so the
   *    composed field is reflection symmetric on the carrier; features lying in the carrier are their own image. ---*/
  features = baseFeatures;
  std::set<std::array<double, 7>> seen;
  for (const auto& f : features) seen.insert(FeatureOrder(rank(f), f.a, f.b));
  for (const auto& plane : featurePlanes)
    for (const auto& f : baseFeatures) {
      auto signedDistance = [&](const FeaturePoint& x) {
        long double d = 0;
        for (unsigned j = 0; j < 3; ++j) d += ((long double)x[j] - plane.origin[j]) * plane.normal[j];
        return d;
      };
      const auto da = signedDistance(f.a), db = signedDistance(f.b);
      const long double distance = ((da > 0 && db < 0) || (da < 0 && db > 0)) ? 0 : std::min(std::abs(da), std::abs(db));
      if (distance > radius(f)) continue;
      auto onPlane = [&](const FeaturePoint& x) { return std::abs(signedDistance(x)) <= plane.tolerance; };
      auto reflect = [&](const FeaturePoint& x) {
        if (onPlane(x)) return x;
        const auto d = signedDistance(x);
        FeaturePoint y{};
        for (unsigned j = 0; j < 3; ++j) y[j] = double(x[j] - 2 * d * plane.normal[j]);
        return y;
      };
      if (onPlane(f.a) && (f.corner || onPlane(f.b))) continue;
      Feature g = f;
      g.image = true;
      const auto a = reflect(f.a), b = f.corner ? a : reflect(f.b);
      g.a = std::min(a, b);
      g.b = std::max(a, b);
      if (!f.corner) {
        long double length = 0;
        std::array<long double, 3> t{};
        for (unsigned j = 0; j < 3; ++j) {
          t[j] = (long double)g.b[j] - g.a[j];
          length += t[j] * t[j];
        }
        length = std::sqrt(length);
        if (!(length > 0)) continue;
        for (unsigned j = 0; j < 3; ++j) g.t[j] = double(t[j] / length);
        g.length = double(length);
      }
      if (seen.insert(FeatureOrder(rank(g), g.a, g.b)).second) features.push_back(std::move(g));
    }
  std::sort(features.begin(), features.end(), [&](const Feature& x, const Feature& y) {
    return FeatureOrder(rank(x), x.a, x.b) < FeatureOrder(rank(y), y.a, y.b);
  });

  /*--- Summary and identity (FNV-1a over the parameters and the canonical feature list). ---*/
  auto& s = featureSummary;
  s.segments = s.images = s.corners = s.imageCorners = 0;
  s.maximumRadius = 0;
  uint64_t hash = 1469598103934665603ULL;
  auto mix = [&](const void* data, size_t bytes) {
    const auto* c = static_cast<const unsigned char*>(data);
    for (size_t i = 0; i < bytes; ++i) hash = (hash ^ c[i]) * 1099511628211ULL;
  };
  const uint64_t dimension = nDim;
  mix(&dimension, sizeof(dimension));
  mix(&s.angle, sizeof(s.angle));
  mix(&s.slope, sizeof(s.slope));
  double longest = 0;
  for (const auto& f : features) {
    const auto r = rank(f);
    (r == 0 ? s.segments : r == 1 ? s.images : r == 2 ? s.corners : s.imageCorners)++;
    s.maximumRadius = std::max(s.maximumRadius, radius(f));
    longest = std::max(longest, f.length);
    const int32_t kind = r;
    mix(&kind, sizeof(kind));
    for (const auto* point : {&f.a, &f.b})
      for (const double x : *point) {
        uint64_t bits;
        std::memcpy(&bits, &x, sizeof(bits));
        if (!(bits & 0x7FFFFFFFFFFFFFFFULL)) bits = 0;  // +0 and -0 are one coordinate (integer test: fast-math safe)
        mix(&bits, sizeof(bits));
      }
    mix(&f.h0, sizeof(f.h0));
    for (const auto w : f.walls) mix(&w, sizeof(w));
  }
  s.identity = hash;
  if (features.empty()) return;

  /*--- Candidate grid: a point within the support radius of a feature lies in a cell where it was inserted. ---*/
  featureCell = std::max({s.maximumRadius, longest, std::numeric_limits<double>::min()});
  for (unsigned i = 0; i < features.size(); ++i) {
    const auto& f = features[i];
    const double r = radius(f);
    std::array<long long, 3> lo{}, hi{};
    for (unsigned j = 0; j < nDim; ++j) {
      const double low = std::min(f.a[j], f.b[j]), high = std::max(f.a[j], f.b[j]);
      const double pad = r + 1e-12 * (std::max(std::abs(low), std::abs(high)) + r);
      lo[j] = static_cast<long long>(std::floor((low - pad) / featureCell));
      hi[j] = static_cast<long long>(std::floor((high + pad) / featureCell));
    }
    std::array<long long, 3> key{};
    for (key[0] = lo[0]; key[0] <= hi[0]; ++key[0])
      for (key[1] = lo[1]; key[1] <= hi[1]; ++key[1])
        for (key[2] = lo[2]; key[2] <= hi[2]; ++key[2]) featureGrid[key].push_back(i);
  }
}

void CBoundaryLayerMetric::ApplyFeatures(unsigned short wall, const su2double* X, Sample& sample) const {
  if (featureGrid.empty()) return;
  std::array<long long, 3> key{};
  for (unsigned j = 0; j < nDim; ++j)
    key[j] = static_cast<long long>(std::floor(SU2_TYPE::GetValue(X[j]) / featureCell));
  const auto cell = featureGrid.find(key);
  if (cell == featureGrid.end()) return;
  su2double M[3][3] = {{0.0}};
  for (unsigned i = 0; i < nDim; ++i)
    for (unsigned j = 0; j < nDim; ++j) M[i][j] = sample.full.m[i][j];
  auto smallest = [&](const su2double (&A)[3][3]) {
    su2double vec[3][3], val[3], work[3];
    CBlasStructure::EigenDecomposition(A, vec, val, nDim, work);
    return SU2_TYPE::GetValue(*std::min_element(val, val + nDim));
  };
  double lmin = -1;
  bool changed = false;
  for (const auto index : cell->second) {
    const auto& f = features[index];
    if (!std::binary_search(f.walls.begin(), f.walls.end(), wall)) continue;
    std::array<long double, 3> delta{};
    for (unsigned j = 0; j < nDim; ++j) delta[j] = (long double)SU2_TYPE::GetValue(X[j]) - f.a[j];
    if (!f.corner) {
      long double s = 0;
      for (unsigned j = 0; j < 3; ++j) s += delta[j] * f.t[j];
      s = std::clamp(s, 0.0L, (long double)f.length);
      for (unsigned j = 0; j < 3; ++j) delta[j] -= s * f.t[j];
    }
    const double rho = double(std::sqrt(delta[0] * delta[0] + delta[1] * delta[1] + delta[2] * delta[2]));
    if (!(rho <= FeatureRadius(wall, f.h0))) continue;
    const double h = f.h0 + featureSlope * rho, lambda = 1 / (h * h);
    if (lmin < 0) lmin = smallest(M);
    if (lambda <= lmin) continue;  // F <= lambda I <= M: exactly redundant
    su2double F[3][3] = {{0.0}}, D[3][3] = {{0.0}};
    for (unsigned i = 0; i < nDim; ++i)
      for (unsigned j = 0; j < nDim; ++j) {
        F[i][j] = (i == j ? lambda : 0.0) - (f.corner ? 0.0 : lambda * f.t[i] * f.t[j]);
        D[i][j] = M[i][j] - F[i][j];
      }
    if (smallest(D) >= 0) continue;  // M >= F: M ∩ F = M
    su2double C[3][3] = {{0.0}};
    IntersectLD(nDim, M, F, C);
    for (unsigned i = 0; i < nDim; ++i)
      for (unsigned j = 0; j < nDim; ++j) M[i][j] = C[i][j];
    lmin = smallest(M);
    changed = true;
  }
  if (!changed) return;
  for (unsigned i = 0; i < nDim; ++i)
    for (unsigned j = 0; j < nDim; ++j) sample.full.m[i][j] = M[i][j];
  if (nDim == 3) LogarithmSPD3(M, sample.logarithm.m);
}

bool CBoundaryLayerMetric::FeatureRefinement2D(const std::array<std::array<double, 2>, 3>& triangle,
                                               double bandRatio) const {
  if (nDim != 2) return false;
  for (const auto& f : features) {
    double radius = 0;
    for (const auto w : f.walls) radius = std::max(radius, FeatureRadius(w, f.h0));
    double rmax = 0, rmin = std::numeric_limits<double>::max();
    long double low = 0, high = 0;
    for (unsigned k = 0; k < 3; ++k) {
      const auto& u = triangle[k];
      const auto& v = triangle[(k + 1) % 3];
      rmax = std::max(rmax, std::hypot(u[0] - f.a[0], u[1] - f.a[1]));
      const auto side = SU2Native2D::Orientation(u[0], u[1], v[0], v[1], f.a[0], f.a[1]);
      low = std::min(low, side);
      high = std::max(high, side);
      const double ex = v[0] - u[0], ey = v[1] - u[1], length2 = ex * ex + ey * ey;
      const double t = length2 > 0 ? std::clamp(((f.a[0] - u[0]) * ex + (f.a[1] - u[1]) * ey) / length2, 0., 1.) : 0.;
      rmin = std::min(rmin, std::hypot(f.a[0] - u[0] - t * ex, f.a[1] - u[1] - t * ey));
    }
    if (low == 0 || high == 0) rmin = 0;  // the vertex is inside or on the triangle
    if (rmin > radius) continue;
    rmax = std::min(rmax, radius);
    if (f.h0 + featureSlope * rmax > bandRatio * (f.h0 + featureSlope * rmin)) return true;
  }
  return false;
}

void CBoundaryLayerMetric::ConfigureOuterNormal(double protectedDistance,double maximumGrowth) {
  if(nDim!=3 || outerNormal.frozen || outerNormal.version || !(protectedDistance>0) || !std::isfinite(protectedDistance) ||
      !(maximumGrowth>=1) || !std::isfinite(maximumGrowth))
    throw std::invalid_argument("Invalid or already-frozen outer-normal BL limits.");
  for(const auto& wall:walls)if(protectedDistance<wall.layer.firstHeight || maximumGrowth<wall.layer.growth)
    throw std::invalid_argument("Outer-normal BL limits must preserve original first height and growth.");
  outerNormal.protected_distance=protectedDistance;outerNormal.maximum_growth=maximumGrowth;outerNormal.version=1;
  // The outer growth limit raises the largest normal size inside the band, hence the exact D1 support radius.
  IndexFeatures();
}
void CBoundaryLayerMetric::SelectOuterNormal(double parameter) {
  if(outerNormal.frozen || !outerNormal.version || !(parameter>=0 && parameter<=1) || !std::isfinite(parameter))
    throw std::invalid_argument("Invalid or frozen outer-normal BL selection.");
  if(parameter==outerNormal.parameter)return;
  if(outerNormal.version==UINT64_MAX)throw std::runtime_error("BL profile identity exhausted.");
  outerNormal.parameter=parameter;++outerNormal.version;
}

CBoundaryLayerMetric::Sample CBoundaryLayerMetric::Evaluate(unsigned short iWall, const su2double* X,
                                                            su2double coreEigenvalue) {
  return EvaluateImpl(iWall,X,coreEigenvalue,false);
}

CBoundaryLayerMetric::Sample CBoundaryLayerMetric::EvaluateImpl(unsigned short iWall, const su2double* X,
                                                               su2double coreEigenvalue, bool distanceOnly,
                                                               unsigned long pinned) {
  const auto& wall = walls[iWall];
  auto& d = *data[iWall];
  Sample sample;sample.profile_version=outerNormal.version;
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
    if(distanceOnly)return sample;

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

    /*--- Tangential size: length of the closest segment (convex feature vertices act through D1 below). ---*/
    ht = length;
  } else {
    /*--- The ADT's tolerant triangle test is a seed, not an authoritative closest facet. ---*/
    const auto* query=wall.authority?lookup:X;
    auto closest=[&](unsigned long face,su2double (&weights)[3],su2double (&point)[3]) {
      const auto* t=&d.elem[3*face];ClosestPointTriangle(x(t[0]),x(t[1]),x(t[2]),query,weights);
      const auto displacement=TriangleDisplacement(x(t[0]),x(t[1]),x(t[2]),query,weights);
      std::copy(displacement.begin(),displacement.end(),point);
      su2double distance2=0;for(unsigned j=0;j<3;++j)distance2+=pow(point[j],2);
      return distance2;
    };
    su2double w[3],radial[3];auto dist2=closest(id,w,radial);
    std::vector<unsigned long> candidates;
    d.adt->DetermineElementsWithinDistance(query,sqrt(dist2)+d.tolerance,candidates);
    std::sort(candidates.begin(),candidates.end());
    for(auto other:candidates)if(other!=id) {
      su2double weights[3],point[3];const auto distance2=closest(other,weights,point);
      if(distance2<dist2 || (distance2==dist2 && other<id)) {
        id=other;dist2=distance2;std::copy(weights,weights+3,w);std::copy(point,point+3,radial);
      }
    }
    if(pinned!=ChordOwners::NoFacet && !wall.authority) {
      // Chord ownership (EdgeBreakpoints envelope piece): deterministic among rounding-level distance ties. The
      // envelope (long double) is the authority; the double closest-point classifier is not re-compared against it.
      if(pinned>=d.faceNormal.size())throw std::invalid_argument("Native BL chord owner is not a wall facet.");
      id=pinned;dist2=closest(id,w,radial);
    }
    const auto* t = &d.elem[3 * id];
    sample.distance = sqrt(dist2);
    if(wall.authority) {
      sample.distance=smooth.distance;
      geometricNormal[0]=smooth.normal.x;geometricNormal[1]=smooth.normal.y;geometricNormal[2]=smooth.normal.z;
    }

    if(distanceOnly)return sample;
    /*--- Normal: of the face, of an edge, of a vertex, or towards the point if it is off the wall. ---*/
    Vec3 n = d.faceNormal[id];
    unsigned short nZero = 0, kMin = 0, kMax = 0;
    for (unsigned short k = 0; k < 3; ++k) {
      if (w[k] <= NormalFeatureThreshold3D) ++nZero;
      if (w[k] < w[kMin]) kMin = k;
      if (w[k] > w[kMax]) kMax = k;
    }
    if (nZero == 1) n = d.edgeNormal[id][kMin];
    if (nZero >= 2) n = d.vertexNormal[t[kMax]];
    if(wall.authority)n={smooth.normal.x,smooth.normal.y,smooth.normal.z};
    if (sample.distance > d.tolerance) {
      if(wall.authority)n={smooth.direction.x,smooth.direction.y,smooth.direction.z};
      else for (unsigned short iDim = 0; iDim < 3; ++iDim) n[iDim] = radial[iDim] / sample.distance;
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
  if(outerNormal.parameter>0 && sample.distance>outerNormal.protected_distance) {
    const auto p=outerNormal.protected_distance;
    const auto anchor=max(d.h0,2*(d.h0+(d.growth-1)*p)/(d.growth+1));
    const auto slope=2*(1-2/(d.growth+1));
    const auto limit=2*(1-2/(outerNormal.maximum_growth+1));
    sample.hn=max(sample.hn,anchor+(slope+outerNormal.parameter*(limit-slope))*(sample.distance-p));
  }

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

  /*--- D1: the full-strength wall sample intersected with its features (canonical order), before the fade. ---*/
  ApplyFeatures(iWall, X, sample);

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
  /*--- D1: the walls above are complete and fluid-oriented (fluid on the left); partners are the other markers. ---*/
  FeatureOptions options;
  options.hgrad = SU2_TYPE::GetValue(config.GetAdap_Hgrad());
  for (const auto& component : reference.original->Components()) {
    const auto& name = reference.marker_names.at(component.marker);
    bool bl = false;
    for (unsigned short b = 0; b < config.GetnAdap_BL(); ++b) bl = bl || config.GetAdap_BL(b).marker == name;
    if (bl) continue;
    const bool reflective = config.GetMarker_CfgFile_KindBC(name) == SYMMETRY_PLANE;
    for (size_t k = 1; k < component.nodes.size(); ++k)
      options.faces.push_back({{component.nodes[k - 1].p.x, component.nodes[k - 1].p.y, component.nodes[k].p.x,
                                component.nodes[k].p.y}, std::string(), reflective});
  }
  return std::make_shared<CBoundaryLayerMetric>(2, std::move(walls), config.GetAdap_Angle(), std::move(options));
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
      if(reference.original->Affine().count(face.id))
        throw std::invalid_argument("Native3D BL on declared affine wall authority is unsupported; no faceted substitute is frozen.");
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
  /*--- D1: complete original walls (facets oriented out of the fluid) and the other physical facets as partners. ---*/
  FeatureOptions options;
  options.hgrad=SU2_TYPE::GetValue(config.GetAdap_Hgrad());
  for(const auto& face:reference.facets) {
    const auto& name=reference.marker_names.at(face.marker);
    bool bl=false;for(unsigned short b=0;b<config.GetnAdap_BL();++b)bl=bl || config.GetAdap_BL(b).marker==name;
    if(bl)continue;
    FeatureFace partner;partner.reflective=config.GetMarker_CfgFile_KindBC(name)==SYMMETRY_PLANE;
    for(const auto& node:face.v){partner.coord.push_back(node.p.x);partner.coord.push_back(node.p.y);partner.coord.push_back(node.p.z);}
    options.faces.push_back(std::move(partner));
  }
  return std::make_shared<CBoundaryLayerMetric>(3,std::move(walls),config.GetAdap_Angle(),std::move(options));
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
                      std::array<double,4>& parent,BLPoint3D& position,
                      double inwardScale=std::numeric_limits<double>::epsilon()) {
  for(auto w:parent)if(!std::isfinite(w) || w < -64*std::numeric_limits<double>::epsilon())
    throw std::runtime_error("Native3D BL integration has invalid original barycentrics.");
  const auto requested=parent;
  for(unsigned attempt=0;attempt<8;++attempt) {
    const double inward=attempt?std::ldexp(inwardScale,attempt+3):0;
    if(inward>=.5)break;
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
  return IntegrationRule3DImpl(tetrahedron,bandRatio,std::move(density),sensorDependent,nullptr,0);
}

struct CBoundaryLayerMetric::ComplexityWorkspace3D {
  const CBoundaryLayerMetric* owner;
  uint64_t profile_version=0;
  BLTetrahedron cell;
  size_t ceiling,bytes=sizeof(ComplexityWorkspace3D);
  bool prepared=false,hidden=false;
  int opposite=-1;
  std::vector<SupportFace3D> support;
  std::map<std::array<double,8>,std::vector<double>> chords;
};

std::shared_ptr<CBoundaryLayerMetric::ComplexityWorkspace3D> CBoundaryLayerMetric::PrepareComplexity3D(
    const BLTetrahedron& tetrahedron,size_t maximumBytes) {
  if(nDim!=3 || maximumBytes<sizeof(ComplexityWorkspace3D))
    throw std::invalid_argument("Invalid native3D geometry workspace budget.");
  for(const auto& p:tetrahedron)for(auto x:p)if(!std::isfinite(x))
    throw std::invalid_argument("Nonfinite native3D geometry workspace cell.");
  if(BLDeterminant(tetrahedron[0],tetrahedron[1],tetrahedron[2],tetrahedron[3])==0)
    throw std::invalid_argument("Degenerate native3D geometry workspace cell.");
  auto result=std::make_shared<ComplexityWorkspace3D>();
  result->owner=this;result->profile_version=outerNormal.version;result->cell=tetrahedron;result->ceiling=maximumBytes;
  return result;
}

CBoundaryLayerMetric::ComplexityEstimate3D CBoundaryLayerMetric::EstimateComplexity3D(
    const std::array<std::array<double,3>,4>& tetrahedron,IntegrationDensity3D density,size_t maximumEvaluations,
    double absoluteTolerance,ComplexityWorkspace3D* workspace) {
  if(!density || maximumEvaluations<64 || maximumEvaluations>65536 ||
      (!std::isfinite(absoluteTolerance) || (absoluteTolerance<0 && absoluteTolerance!=-1)) ||
      (workspace && (workspace->owner!=this || workspace->profile_version!=outerNormal.version || workspace->cell!=tetrahedron)))
    throw std::invalid_argument("Invalid bounded native3D complexity control.");
  ComplexityEstimate3D estimate;
  const auto start=std::chrono::steady_clock::now();
  auto measuredDensity=[&](const IntegrationPoint3D& q) {
    const auto begin=std::chrono::steady_clock::now();const auto value=density(q);
    estimate.densitySeconds+=std::chrono::duration<double>(std::chrono::steady_clock::now()-begin).count();
    return value;
  };
  IntegrationRule3DImpl(tetrahedron,1.5,measuredDensity,nullptr,&estimate,maximumEvaluations,absoluteTolerance,workspace);
  estimate.seconds=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();
  estimate.cachedBytes=workspace?workspace->bytes:0;
  if(!(estimate.value>0) || !std::isfinite(estimate.value) || std::isnan(estimate.indicator) || estimate.indicator<0)
    throw std::runtime_error("Invalid bounded native3D complexity result.");
  return estimate;
}

std::vector<CBoundaryLayerMetric::IntegrationPoint3D> CBoundaryLayerMetric::IntegrationRule3DImpl(
    const std::array<std::array<double,3>,4>& tetrahedron,double bandRatio,IntegrationDensity3D density,
    bool* sensorDependent,ComplexityEstimate3D* estimate,size_t maximumEvaluations,
    double absoluteTolerance,ComplexityWorkspace3D* workspace) {
  if(nDim!=3 || !std::isfinite(bandRatio) || !(bandRatio>1))
    throw std::invalid_argument("Invalid native3D BL volume quadrature.");
  for(const auto& p:tetrahedron)for(auto x:p)if(!std::isfinite(x))
    throw std::invalid_argument("Nonfinite native3D BL integration cell.");
  if(BLDeterminant(tetrahedron[0],tetrahedron[1],tetrahedron[2],tetrahedron[3])==0)
    throw std::invalid_argument("Degenerate native3D BL integration cell.");
  const std::array<std::array<double,4>,4> bary{{{{1,0,0,0}},{{0,1,0,0}},{{0,0,1,0}},{{0,0,0,1}}}};
  using SupportFace=SupportFace3D;
  std::vector<SupportFace> freshSupport;
  bool hiddenSupport=workspace && workspace->prepared && workspace->hidden;
  const auto preparationStart=std::chrono::steady_clock::now();
  if(!workspace || !workspace->prepared)for(unsigned short wall=0;wall<walls.size();++wall) {
    const auto& d=*data[wall];if(!d.adt)continue;
    const double thickness=SU2_TYPE::GetValue(d.thickness)+d.proxy_error;
    if(walls[wall].authority && !MayIntersectSmoothBand(tetrahedron,*walls[wall].authority,walls[wall].authority_facet,SU2_TYPE::GetValue(d.thickness)))continue;
    bool hidden=true;
    if(!walls[wall].authority)for(const auto& p:tetrahedron){const su2double x[3]={p[0],p[1],p[2]};hidden=hidden && EvaluateImpl(wall,x,1.,true).distance>=thickness;}
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
      freshSupport.push_back(face);hiddenSupport=hiddenSupport || hidden;
      if(walls[wall].authority)break; // One frame seed; smooth support is certified by the whole component.
    }
  }
  if(workspace && !workspace->prepared) {
    const size_t bytes=freshSupport.capacity()*sizeof(SupportFace);
    if(bytes<=workspace->ceiling-workspace->bytes) {
      workspace->support=std::move(freshSupport);workspace->bytes+=bytes;
      workspace->hidden=hiddenSupport;workspace->prepared=true;
    }
  }
  const auto& support=workspace && workspace->prepared?workspace->support:freshSupport;
  if(estimate)estimate->preparationSeconds+=std::chrono::duration<double>(std::chrono::steady_clock::now()-preparationStart).count();
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
  if(estimate && support.empty()) {
    SU2Native3D::Tetrahedron source;for(unsigned k=0;k<4;++k)source[k]={tetrahedron[k][0],tetrahedron[k][1],tetrahedron[k][2]};
    const bool reversed=SU2Native3D::Orientation(source[0],source[1],source[2],source[3])<0;
    if(reversed)std::swap(source[1],source[2]);
    const double volume=std::abs(BLDeterminant(tetrahedron[0],tetrahedron[1],tetrahedron[2],tetrahedron[3]))/6;
    auto value=[&](std::array<double,4> b) {
      BLPoint3D p{};SetBLSourceQuery(tetrahedron,source,reversed,b,p);
      IntegrationPoint3D q{b,1,{}};++estimate->evaluations;return probe(q);
    };
    const double a=(5+3*std::sqrt(5.))/20,b=(5-std::sqrt(5.))/20;
    double lower=0;for(unsigned i=0;i<4;++i){std::array<double,4> beta{{b,b,b,b}};beta[i]=a;lower+=volume*.25*value(beta);}
    const double x3[3]={.5*(1-.77459666924148337704),.5,.5*(1+.77459666924148337704)},w3[3]={5./18,4./9,5./18};
    const double x2[2]={.5*(1-.57735026918962576451),.5*(1+.57735026918962576451)};
    for(unsigned i=0;i<3;++i)for(unsigned j=0;j<3;++j)for(unsigned k=0;k<2;++k) {
      const auto u=x3[i],v=x3[j],w=x2[k];
      estimate->value+=volume*3*u*u*v*w3[i]*w3[j]*value({1-u,u*(1-v),u*v*(1-w),u*v*w});
    }
    estimate->indicator=std::abs(estimate->value-lower);estimate->panels=1;return {};
  }
  // Scalar complexity needs a positive covering, not the strict projected-chart closure.
  if(estimate)return IntegrationRule3DVisible(tetrahedron,support,bandRatio,probe,estimate,maximumEvaluations,absoluteTolerance,workspace);
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
    return IntegrationRule3DVisible(tetrahedron,support,bandRatio,probe,estimate,maximumEvaluations);
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
  return IntegrationColumns3D(tetrahedron,columns,support,bandRatio,probe,estimate,maximumEvaluations);
}

std::vector<CBoundaryLayerMetric::IntegrationPoint3D> CBoundaryLayerMetric::IntegrationRule3DVisible(
    const BLTetrahedron& tetrahedron,const std::vector<SupportFace3D>& support,double bandRatio,
    const IntegrationDensity3D& density,ComplexityEstimate3D* estimate,size_t maximumEvaluations,
    double absoluteTolerance,ComplexityWorkspace3D* workspace) {
  unsigned opposite=workspace && workspace->opposite>=0?unsigned(workspace->opposite):0;
  double best=std::numeric_limits<double>::infinity();
  if(!workspace || workspace->opposite<0)for(unsigned short wall=0;wall<walls.size();++wall)if(data[wall]->adt) {
    std::array<double,4> distances;
    for(unsigned k=0;k<4;++k)distances[k]=SU2_TYPE::GetValue(EvaluateImpl(wall,tetrahedron[k].data(),1.,true).distance)/SU2_TYPE::GetValue(data[wall]->thickness);
    for(unsigned candidate=0;candidate<4;++candidate) {
      double score=0;for(unsigned k=0;k<4;++k)if(k!=candidate)score+=distances[k];
      if(score<best){best=score;opposite=candidate;}
    }
  }
  if(workspace)workspace->opposite=int(opposite);
  Column3D column;column.collapsed=true;
  column.area=std::abs(BLDeterminant(tetrahedron[0],tetrahedron[1],tetrahedron[2],tetrahedron[3]))/2;
  unsigned next=0;
  for(unsigned k=0;k<4;++k)if(k!=opposite) {
    column.entry[next][k]=1;column.exit[next][opposite]=1;column.axial[next]=1;
    for(unsigned j=0;j<3;++j)column.projection[next][j]=tetrahedron[k][j]-tetrahedron[opposite][j];
    ++next;
  }
  return IntegrationColumns3D(tetrahedron,{column},support,bandRatio,density,estimate,maximumEvaluations,absoluteTolerance,workspace);
}

std::vector<CBoundaryLayerMetric::IntegrationPoint3D> CBoundaryLayerMetric::IntegrationColumns3D(
    const BLTetrahedron& tetrahedron,const std::vector<Column3D>& charts,const std::vector<SupportFace3D>& support,
    double bandRatio,const IntegrationDensity3D& probe,ComplexityEstimate3D* estimate,size_t maximumEvaluations,
    double absoluteTolerance,ComplexityWorkspace3D* workspace) {
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
  if(estimate) {
    // Only complete coverings are published; a failed child transaction leaves its parent intact.
    struct WorkLimit {};
    auto evaluate=[&](IntegrationPoint3D& point) {
      if(estimate->evaluations==maximumEvaluations)throw WorkLimit{};
      ++estimate->evaluations;return probe(point);
    };
    long double coordinateScale=0,minimumAltitude=std::numeric_limits<long double>::infinity();
    const auto determinant=std::abs(BLDeterminant(tetrahedron[0],tetrahedron[1],tetrahedron[2],tetrahedron[3]));
    for(const auto& p:tetrahedron)for(auto x:p)coordinateScale=std::max(coordinateScale,std::abs((long double)x));
    for(unsigned f=0;f<4;++f) {
      long double u[3],v[3];for(unsigned j=0;j<3;++j) {
        u[j]=(long double)tetrahedron[(f+2)%4][j]-tetrahedron[(f+1)%4][j];
        v[j]=(long double)tetrahedron[(f+3)%4][j]-tetrahedron[(f+1)%4][j];
      }
      const auto area=std::hypot(u[1]*v[2]-u[2]*v[1],u[2]*v[0]-u[0]*v[2],u[0]*v[1]-u[1]*v[0]);
      minimumAltitude=std::min(minimumAltitude,determinant/area);
    }
    const double inwardScale=double(std::numeric_limits<double>::epsilon()*std::max(1.L,coordinateScale/minimumAltitude));
    auto position=[&](std::array<double,4>& b) {
      BLPoint3D p{};SetBLSourceQuery(tetrahedron,originalSource,reversed,b,p,inwardScale);return p;
    };
    const double volume=std::abs(BLDeterminant(tetrahedron[0],tetrahedron[1],tetrahedron[2],tetrahedron[3]))/6;
    // A complete positive fallback is retained only with infinite uncertainty if band seeding cannot finish.
    const double qa=(5+3*std::sqrt(5.))/20,qb=(5-std::sqrt(5.))/20;
    double seed=0;
    for(unsigned i=0;i<4;++i) {
      std::array<double,4> b{{qb,qb,qb,qb}};b[i]=qa;const auto p=position(b);
      IntegrationPoint3D q{b,volume/4,SamplePoints({p[0],p[1],p[2]})};seed+=q.weight*evaluate(q);
    }
    struct ScalarPanel {
      size_t chart;Triangle triangle;double fraction,value=0,error=0;unsigned depth;
      bool unresolved=false;std::array<double,3> vertexValues{};
    };
    auto sample=[&](ScalarPanel& panel,bool fine,std::vector<bool>& sampled,double& radialError) {
      const auto& chart=charts[panel.chart];double value=0;
      for(unsigned at=0;at<(fine?7u:12u);++at) {
        std::array<double,3> angular{};
        for(unsigned j=0;j<3;++j)for(unsigned k=0;k<3;++k)
          angular[j]+=panel.triangle[k][j]*(fine?faceBary[at][k]:comparisonBary[at][k]);
        std::array<double,4> entry{},exit{};
        for(unsigned k=0;k<4;++k)for(unsigned j=0;j<3;++j) {
          entry[k]+=angular[j]*chart.entry[j][k];exit[k]+=angular[j]*chart.exit[j][k];
        }
        BLPoint3D start{},end{};
        for(unsigned j=0;j<3;++j) {
          long double a=tetrahedron[0][j],b=a;
          for(unsigned k=1;k<4;++k) {
            const long double delta=(long double)tetrahedron[k][j]-tetrahedron[0][j];
            a+=(long double)entry[k]*delta;b+=(long double)exit[k]*delta;
          }
          start[j]=double(a);end[j]=double(b);
        }
        long double length=0;for(unsigned j=0;j<3;++j)length+=angular[j]*chart.axial[j];
        if(!(length>0))throw std::runtime_error("Nonpositive bounded-complexity chord.");
        const auto jacobian=chart.area*panel.fraction*length*(fine?faceWeights[at]:comparisonWeights[at]);
        std::array<double,8> key;for(unsigned k=0;k<4;++k){key[k]=entry[k];key[k+4]=exit[k];}
        const std::vector<double>* retained=nullptr;
        std::vector<double> fresh;
        if(workspace) {
          const auto found=workspace->chords.find(key);
          if(found!=workspace->chords.end()){retained=&found->second;++estimate->reusedChords;}
        }
        if(!retained) {
          const auto begin=std::chrono::steady_clock::now();
          fresh=start==end?std::vector<double>{0,1}:EdgeBreakpointsImpl(start,end,2.,4,false);
          estimate->preparationSeconds+=std::chrono::duration<double>(std::chrono::steady_clock::now()-begin).count();
          ++estimate->preparedChords;
          const auto bytes=sizeof(decltype(workspace->chords)::value_type)+4*sizeof(void*)+fresh.capacity()*sizeof(double);
          if(workspace && bytes<=workspace->ceiling-workspace->bytes) {
            const auto inserted=workspace->chords.emplace(key,std::move(fresh));
            workspace->bytes+=bytes;retained=&inserted.first->second;
          }else retained=&fresh;
        }
        const auto& cuts=*retained;
        double ray=0;
        for(size_t k=1;k<cuts.size();++k) {
          double sums[2]={};const double half=.5*(cuts[k]-cuts[k-1]),center=.5*(cuts[k]+cuts[k-1]);
          for(unsigned order:{3u,5u})for(unsigned g=0;g<order;++g) {
            const double t=center+half*(order==3?gauss3[g]:gauss[g]);std::array<double,4> parent{};
            for(unsigned j=0;j<4;++j)parent[j]=(1-t)*entry[j]+t*exit[j];
            const auto p=position(parent);
            const double weight=double(jacobian*half*(chart.collapsed?(1-t)*(1-t):1)*(order==3?weights3[g]:weights[g]));
            if(!(weight>0) || !std::isfinite(weight))throw std::runtime_error("Invalid bounded-complexity weight.");
            IntegrationPoint3D q{parent,weight,SamplePoints({p[0],p[1],p[2]})};
            for(const auto& s:q.samples)sampled[s.wall]=true;
            sums[order==5]+=weight*evaluate(q);
          }
          radialError+=std::abs(sums[1]-sums[0]);ray+=sums[1];
        }
        value+=ray;
        if(fine && at>=4)panel.vertexValues[at-4]=ray/double(chart.area*panel.fraction*faceWeights[at]);
      }
      return value;
    };
    auto build=[&](size_t chart,const Triangle& triangle,double fraction,unsigned depth) {
      ScalarPanel p{chart,triangle,fraction,0,0,depth};std::vector<bool> sampled(walls.size(),false);
      double radialError=0;p.value=sample(p,true,sampled,radialError);
      const auto comparison=sample(p,false,sampled,radialError);p.error=std::abs(p.value-comparison)+radialError;
      // Preserve the existing geometric guard against agreement of two rules that miss a thin footprint.
      std::array<BLPoint3D,6> boundary{};
      for(unsigned side=0;side<2;++side)for(unsigned v=0;v<3;++v)for(unsigned k=0;k<4;++k)for(unsigned j=0;j<3;++j)
        for(unsigned t=0;t<3;++t)boundary[3*side+v][j]+=triangle[v][t]*(side?charts[chart].exit[t][k]:charts[chart].entry[t][k])*tetrahedron[k][j];
      const std::array<BLTetrahedron,3> regions{{{boundary[0],boundary[1],boundary[2],boundary[5]},
        {boundary[0],boundary[1],boundary[4],boundary[5]},{boundary[0],boundary[3],boundary[4],boundary[5]}}};
      for(const auto& f:support)if(!sampled[f.wall])for(const auto& r:regions) {
        if(BLDeterminant(r[0],r[1],r[2],r[3])==0)continue;
        const auto& w=walls[f.wall];
        p.unresolved=p.unresolved || (w.authority?MayIntersectSmoothBand(r,*w.authority,w.authority_facet,SU2_TYPE::GetValue(data[f.wall]->thickness)):
            MayIntersectWallBand(r,f.vertices,f.thickness*(1-1e-12)));
      }
      return p;
    };
    std::vector<ScalarPanel> panels;
    const Triangle initial{{{{1,0,0,0}},{{0,1,0,0}},{{0,0,1,0}}}};
    auto seedExhausted=[&]() {
      estimate->value=seed;estimate->indicator=std::numeric_limits<double>::infinity();
      estimate->limited=estimate->unresolvedSupport=true;
    };
    try {for(size_t c=0;c<charts.size();++c)panels.push_back(build(c,initial,1.,0));}
    catch(const SU2Native3D::ChordEnvelope::WorkLimit&) {seedExhausted();return {};}
    catch(const WorkLimit&) {
      estimate->value=seed;estimate->indicator=std::numeric_limits<double>::infinity();
      estimate->limited=estimate->unresolvedSupport=true;return {};
    }
    bool geometryLimited=false;
    auto publish=[&]() {
      estimate->value=estimate->indicator=0;estimate->unresolvedSupport=false;estimate->panels=panels.size();
      for(const auto& p:panels) {
        estimate->value+=p.value;estimate->indicator+=p.error;estimate->unresolvedSupport|=p.unresolved;
      }
      estimate->unresolvedSupport|=geometryLimited;
      if(estimate->unresolvedSupport)estimate->indicator=std::numeric_limits<double>::infinity();
    };
    for(;;) {
      publish();
      if(!estimate->unresolvedSupport && estimate->indicator<=
          (absoluteTolerance<0?1e-3*estimate->value:absoluteTolerance))break;
      size_t selected=0;double worst=-1;
      for(size_t i=0;i<panels.size();++i) {
        const auto error=panels[i].unresolved?std::numeric_limits<double>::infinity():panels[i].error;
        if(error>worst){worst=error;selected=i;}
      }
      const auto parent=panels[selected];
      if(parent.depth==32){estimate->limited=true;break;}
      unsigned first=0,last=1;double difference=0;
      for(unsigned i=0;i<3;++i)for(unsigned j=i+1;j<3;++j) {
        double score=std::abs(parent.vertexValues[i]-parent.vertexValues[j]);
        if(parent.unresolved) {
          score=0;for(unsigned d=0;d<3;++d) {
            long double delta=0;for(unsigned k=0;k<3;++k)delta+=((long double)parent.triangle[i][k]-parent.triangle[j][k])*charts[parent.chart].projection[k][d];
            score+=double(delta*delta);
          }
        }
        if(score>difference){difference=score;first=i;last=j;}
      }
      auto a=parent.triangle,b=a;std::array<double,4> middle{};
      for(unsigned k=0;k<4;++k)middle[k]=.5*(a[first][k]+a[last][k]);
      if(middle==a[first] || middle==a[last]){estimate->limited=true;break;}
      a[first]=middle;b[last]=middle;
      try {
        auto left=build(parent.chart,a,.5*parent.fraction,parent.depth+1);
        auto right=build(parent.chart,b,.5*parent.fraction,parent.depth+1);
        panels[selected]=std::move(left);panels.push_back(std::move(right));
      }catch(const SU2Native3D::ChordEnvelope::WorkLimit&){estimate->limited=geometryLimited=true;break;}
       catch(const WorkLimit&){estimate->limited=true;break;}
    }
    publish();return {};
  }
  size_t retained=rule.size(),nextPanel=0;
  double totalValue=0,totalError=0;
  auto integrate=[&](const Column3D& chart,const Triangle& triangle,double fraction,bool fine,std::vector<IntegrationPoint3D>& points,double& radialError,std::array<double,3>* vertexValues=nullptr) {
      double integral=0;
      for(unsigned corner=0;corner<(fine?7u:12u);++corner) {
        const double before=integral;
        std::array<double,3> angular{};
        for(unsigned j=0;j<3;++j)for(unsigned k=0;k<3;++k)
          angular[j]+=triangle[k][j]*(fine?faceBary[corner][k]:comparisonBary[corner][k]);
        std::array<double,4> entry{},exit{};BLPoint3D start{},end{};
        for(unsigned k=0;k<4;++k) {
          for(unsigned j=0;j<3;++j){entry[k]+=angular[j]*chart.entry[j][k];exit[k]+=angular[j]*chart.exit[j][k];}
          for(unsigned j=0;j<3;++j){start[j]+=entry[k]*tetrahedron[k][j];end[j]+=exit[k]*tetrahedron[k][j];}
        }
        long double length=0;for(unsigned j=0;j<3;++j)length+=angular[j]*chart.axial[j];
        if(!(length>0))throw std::runtime_error("Native3D parallel integration chord has nonpositive length: length="+std::to_string(double(length))+" chart_area="+std::to_string(double(chart.area)));
        const auto jacobian=chart.area*fraction*length*(fine?faceWeights[corner]:comparisonWeights[corner]);
        // Sub-ulp endpoints can round to one point; retain their positive barycentric measure.
        const auto cuts=start==end?std::vector<double>{0,1}:EdgeBreakpoints(start,end,std::max(2.,bandRatio),4);
        auto radial=[&](double left,double right,unsigned order,std::vector<IntegrationPoint3D>& samples) {
          double value=0;const double half=.5*(right-left),center=.5*(right+left);
          for(unsigned g=0;g<order;++g) {
            const double t=center+half*(order==7?gauss7[g]:(order==5?gauss[g]:gauss3[g]));
            std::array<double,4> parent{};BLPoint3D position{};
            for(unsigned k=0;k<4;++k) {
              parent[k]=(1-t)*entry[k]+t*exit[k];
              for(unsigned j=0;j<3;++j)position[j]+=parent[k]*tetrahedron[k][j];
            }
            SetBLSourceQuery(tetrahedron,originalSource,reversed,parent,position);
            const double collapse=chart.collapsed?(1-t)*(1-t):1.;
            IntegrationPoint3D point{parent,double(jacobian*half*collapse*(order==7?weights7[g]:(order==5?weights[g]:weights3[g]))),
                SamplePoints({position[0],position[1],position[2]})};
            value+=point.weight*probe(point);samples.push_back(std::move(point));
          }
          return value;
        };
        struct RadialPanel {
          double left,right,value,error,comparison;
          unsigned depth;
          bool upgraded=false;
          std::vector<IntegrationPoint3D> points,comparisonPoints;
        };
        std::map<size_t,RadialPanel> radialPanels;
        std::priority_queue<std::pair<double,size_t>> radialPriority;
        size_t nextRadial=0,rayPoints=0;double rayValue=0,rayError=0;
        auto retain=[&](RadialPanel panel) {
          rayPoints+=panel.points.size();
          if((fine?retained:0)+points.size()+rayPoints>65536)
            throw IntegrationLimit("Native3D BL column point budget exhausted: retained="+std::to_string(retained)+
                " panel="+std::to_string(nextPanel)+" ray="+std::to_string(corner));
          rayValue+=panel.value;rayError+=panel.error;
          radialPriority.emplace(panel.error,nextRadial);radialPanels.emplace(nextRadial++,std::move(panel));
        };
        auto addRadial=[&](double left,double right,unsigned depth) {
          RadialPanel panel{left,right,0,0,0,depth};
          panel.value=radial(left,right,3,panel.points);
          panel.comparison=radial(left,right,5,panel.comparisonPoints);
          panel.error=std::abs(panel.comparison-panel.value);retain(std::move(panel));
        };
        for(size_t band=1;band<cuts.size();++band)addRadial(cuts[band-1],cuts[band],0);
        // A nearest-feature jump needs a ray-integral error budget, not relative accuracy on an infinitesimal interval.
        while(rayError>1e-5*rayValue) {
          const auto selected=radialPriority.top().second;radialPriority.pop();
          auto panel=std::move(radialPanels.at(selected));radialPanels.erase(selected);
          rayValue-=panel.value;rayError=std::max(0.,rayError-panel.error);rayPoints-=panel.points.size();
          if(!panel.upgraded) {
            std::vector<IntegrationPoint3D> higherPoints;
            const auto higher=radial(panel.left,panel.right,7,higherPoints);
            panel.value=panel.comparison;panel.error=std::abs(higher-panel.comparison);
            panel.points=std::move(panel.comparisonPoints);panel.upgraded=true;retain(std::move(panel));continue;
          }
          if(panel.depth>=32)throw IntegrationLimit("Native3D BL ray density remains unresolved after32 subdivisions: value="+
              std::to_string(rayValue)+" error="+std::to_string(rayError+panel.error));
          if(++visited>16384)throw IntegrationLimit("Native3D BL radial quadrature work budget exhausted: retained="+std::to_string(retained)+
              " panels="+std::to_string(nextPanel)+" cones="+std::to_string(charts.size())+" ray="+std::to_string(corner));
          const double middle=.5*(panel.left+panel.right);
          if(middle==panel.left || middle==panel.right)throw std::runtime_error("Unrepresentable native3D radial subdivision.");
          addRadial(panel.left,middle,panel.depth+1);addRadial(middle,panel.right,panel.depth+1);
        }
        radialError+=rayError;integral+=rayValue;
        for(auto& panel:radialPanels)points.insert(points.end(),std::make_move_iterator(panel.second.points.begin()),std::make_move_iterator(panel.second.points.end()));
        if(vertexValues && corner>=4)(*vertexValues)[corner-4]=(integral-before)/double(chart.area*fraction*faceWeights[corner]);
      }
      return integral;
    };
  struct Panel {
    size_t cone;
    Triangle triangle;
    double fraction;
    unsigned depth;
    std::vector<IntegrationPoint3D> fine;
    double value,error;
    bool unresolved=false;
    std::array<double,3> vertexValues{};
  };
  size_t unresolved=0;
  std::map<size_t,Panel> panels;
  std::priority_queue<std::pair<double,size_t>> priority;
  for(const auto& point:rule)totalValue+=point.weight*probe(point);
  auto add=[&](size_t cone,const Triangle& triangle,double fraction,unsigned depth) {
    if(++visited>16384)throw IntegrationLimit("Native3D BL angular quadrature work budget exhausted: retained="+std::to_string(retained)+
        " panels="+std::to_string(nextPanel)+" cones="+std::to_string(charts.size())+" estimated_error="+std::to_string(totalError)+" density="+std::to_string(totalValue));
    Panel panel{cone,triangle,fraction,depth,{},0,0};std::vector<IntegrationPoint3D> coarse;
    double highRadialError=0,lowRadialError=0;
    panel.value=integrate(charts[cone],triangle,fraction,true,panel.fine,highRadialError,&panel.vertexValues);
    const auto low=integrate(charts[cone],triangle,fraction,false,coarse,lowRadialError);
    panel.error=std::abs(panel.value-low)+highRadialError+lowRadialError;
    std::vector<bool> sampled(walls.size(),false);
    for(const auto* points:{&panel.fine,&coarse})for(const auto& point:*points)for(const auto& s:point.samples)sampled[s.wall]=true;
    if(std::any_of(support.begin(),support.end(),[&](const auto& f){return !sampled[f.wall];})) {
      std::array<BLPoint3D,6> boundary{};
      for(unsigned side=0;side<2;++side)for(unsigned p=0;p<3;++p)for(unsigned j=0;j<3;++j)for(unsigned k=0;k<4;++k)for(unsigned v=0;v<3;++v)
        boundary[3*side+p][j]+=triangle[p][v]*(side?charts[cone].exit[v][k]:charts[cone].entry[v][k])*tetrahedron[k][j];
      const std::array<BLTetrahedron,3> regions{{{boundary[0],boundary[1],boundary[2],boundary[5]},
          {boundary[0],boundary[1],boundary[4],boundary[5]},{boundary[0],boundary[3],boundary[4],boundary[5]}}};
      for(const auto& face:support)if(!sampled[face.wall])for(const auto& region:regions) {
        if(BLDeterminant(region[0],region[1],region[2],region[3])==0)continue;
        const auto& wall=walls[face.wall];
        if(wall.authority ? MayIntersectSmoothBand(region,*wall.authority,wall.authority_facet,SU2_TYPE::GetValue(data[face.wall]->thickness)) :
            MayIntersectWallBand(region,face.vertices,face.thickness*(1-1e-12)))panel.unresolved=true;
      }
    }
    unresolved+=panel.unresolved;
    totalValue+=panel.value;totalError+=panel.error;
    retained+=panel.fine.size();priority.emplace(panel.unresolved?std::numeric_limits<double>::infinity():panel.error,nextPanel);
    panels.emplace(nextPanel++,std::move(panel));
  };
  const Triangle initial{{{{1,0,0,0}},{{0,1,0,0}},{{0,0,1,0}}}};
  for(size_t k=0;k<charts.size();++k)add(k,initial,1.,0);
  if(panels.empty() || !(totalValue>0) || !std::isfinite(totalValue))throw std::runtime_error("Invalid native3D BL column integral.");
  while(unresolved || totalError>1e-4*totalValue) {
    if(priority.empty())throw std::runtime_error("Native3D BL angular error queue is unresolved.");
    const auto selected=priority.top().second;priority.pop();
    auto panel=std::move(panels.at(selected));panels.erase(selected);
    if(panel.depth>=32)throw IntegrationLimit("Native3D BL angular density remains unresolved after 32 subdivisions.");
    unresolved-=panel.unresolved;
    totalValue-=panel.value;totalError=std::max(0.,totalError-panel.error);retained-=panel.fine.size();
    panel.fine.clear();panel.fine.shrink_to_fit();
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
    for(const auto vertex:{a,b}) {
      auto child=panel.triangle;child[vertex]=midpoint;add(panel.cone,child,.5*panel.fraction,panel.depth+1);
    }
  }
  for(auto& entry:panels)rule.insert(rule.end(),std::make_move_iterator(entry.second.fine.begin()),std::make_move_iterator(entry.second.fine.end()));
  return rule;
}

std::vector<double> CBoundaryLayerMetric::EdgeBreakpoints(
    const std::array<double, 3>& a, const std::array<double, 3>& b, double bandRatio, unsigned fadeBands,
    ChordOwners* owners) {
  return EdgeBreakpointsImpl(a,b,bandRatio,fadeBands,true,owners);
}

std::vector<double> CBoundaryLayerMetric::EdgeBreakpointsImpl(
    const std::array<double,3>& a,const std::array<double,3>& b,double bandRatio,unsigned fadeBands,bool featureTransitions,
    ChordOwners* owners) {
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
  if (owners) owners->walls.assign(walls.size(), {});
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
    if(outerNormal.version && outerNormal.protected_distance<thickness)levels.push_back(outerNormal.protected_distance);
    // Strict edges also integrate the existing on-wall face/edge/vertex normal classification band.
    if(nDim==3 && featureTransitions && !walls[wall].authority)levels.push_back(SU2_TYPE::GetValue(d.tolerance));
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
          {SU2_TYPE::GetValue(x[0]),SU2_TYPE::GetValue(x[1]),SU2_TYPE::GetValue(x[2])}).distance : SU2_TYPE::GetValue(EvaluateImpl(wall,x,1.,true).distance);
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
    if(nDim==3) {
      if(faces.empty())continue;
      double coordinateScale=thickness;for(unsigned j=0;j<3;++j)coordinateScale=std::max({coordinateScale,std::abs(a[j]),std::abs(b[j])});
      // The wall metric is inactive beyond its fade; retain roundoff padding and the on-wall classification band.
      const auto support=std::nextafter(std::max(thickness,SU2_TYPE::GetValue(d.tolerance))+
          128*std::numeric_limits<double>::epsilon()*coordinateScale+4*SU2_TYPE::GetValue(d.tolerance),INFINITY);
      SU2Native3D::ChordEnvelope envelope(a,b,1u<<20,support);
      std::set<unsigned long> vertices;
      std::set<std::pair<unsigned long,unsigned long>> edges;
      auto vertex=[&](unsigned long id) {
        std::array<double,3> p;for(unsigned j=0;j<3;++j)p[j]=SU2_TYPE::GetValue(walls[wall].coord[3*id+j]);return p;
      };
      // Envelope feature -> owning facet: candidate facets are ascending, so a shared vertex/segment belongs to the
      // lowest candidate facet that introduced it. The support cap (feature 0) has no owner.
      std::vector<unsigned long> featureFacet(owners ? envelope.Features() : 0, ChordOwners::NoFacet);
      auto record=[&](unsigned long face){if(owners)featureFacet.resize(envelope.Features(),face);};
      for(const auto face:faces) {
        const auto* ids=&d.elem[3*face];std::array<std::array<double,3>,3> triangle;
        for(unsigned k=0;k<3;++k) {
          triangle[k]=vertex(ids[k]);
          if(vertices.insert(ids[k]).second){envelope.Vertex(triangle[k]);record(face);}
          const auto edge=std::minmax(ids[k],ids[(k+1)%3]);
          if(edges.insert(edge).second){envelope.Segment(vertex(edge.first),vertex(edge.second));record(face);}
        }
        envelope.Face(triangle);record(face);
      }
      if(owners) {
        auto& ownership=owners->walls[wall];
        const auto pieces=envelope.Ownership();
        if(owners->Pieces()+pieces.size()>65536)throw std::runtime_error("Native BL chord ownership exceeds work budget.");
        ownership.reserve(pieces.size());
        for(const auto& piece:pieces) {
          const auto facet=piece.second<featureFacet.size() ? featureFacet[piece.second] : ChordOwners::NoFacet;
          if(!ownership.empty() && ownership.back().facet==facet)ownership.back().hi=piece.first;
          else ownership.push_back({piece.first,facet});
        }
      }
      // Envelope boundaries also resolve nearest-feature changes, not only distance levels.
      for(auto t:envelope.Cuts(levels,featureTransitions))if(t>0 && t<1 && retained.insert(t).second) {
        if(cuts.size()>=65536)throw std::runtime_error("Native BL edge breakpoints exceed work budget.");
        cuts.push_back(t);
      }
      if(featureTransitions) {
        const auto cross=[](const Vector& a,const Vector& b){return Vector{a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]};};
        const auto delta=[](const std::array<double,3>& a,const std::array<double,3>& b){return Vector{(long double)a[0]-b[0],(long double)a[1]-b[1],(long double)a[2]-b[2]};};
        long double magnitude=0;for(unsigned j=0;j<3;++j)magnitude=std::max({magnitude,std::abs((long double)a[j]),std::abs((long double)b[j])});
        const auto padding=64*std::numeric_limits<double>::epsilon()*magnitude;
        size_t classified=0;
        auto classificationCut=[&](long double t,long double distance) {
          if(!(t>0 && t<1) || retained.count(double(t)))return;
          if(++classified>65536)throw SU2Native3D::ChordEnvelope::WorkLimit("Native3D wall-normal classification root budget exhausted.");
          const auto nearest=envelope.Distance(t);
          if(nearest>SU2_TYPE::GetValue(d.tolerance)+padding || distance>nearest+padding)return;
          if(cuts.size()>=65536)throw std::runtime_error("Native BL edge breakpoints exceed work budget.");
          retained.insert(double(t));cuts.push_back(double(t));
        };
        // The on-wall policy also changes where closest-point barycentric weights cross its feature threshold.
        for(const auto face:faces) {
          const auto* ids=&d.elem[3*face];const auto p=vertex(ids[0]);
          const auto u=delta(vertex(ids[1]),p),v=delta(vertex(ids[2]),p),r=delta(a,p),n=cross(u,v);
          const auto square=dot(n,n);
          const long double first[3]={0,dot(cross(r,v),n)/square,dot(cross(u,r),n)/square};
          const long double slope[3]={0,dot(cross(chord,v),n)/square,dot(cross(u,chord),n)/square};
          const long double w[3]={1-first[1]-first[2],first[1],first[2]},dw[3]={-slope[1]-slope[2],slope[1],slope[2]};
          for(unsigned k=0;k<3;++k)if(dw[k]!=0) {
            const auto t=(NormalFeatureThreshold3D-w[k])/dw[k];if(!(t>0 && t<1))continue;
            long double low=1,scale=1;for(unsigned j=0;j<3;++j){low=std::min(low,std::fma(t,dw[j],w[j]));scale=std::max({scale,std::abs(w[j]),std::abs(dw[j])});}
            if(low < -256*std::numeric_limits<long double>::epsilon()*scale)continue;
            const auto distance=std::abs(dot(r,n)+t*dot(chord,n))/std::sqrt(square);
            classificationCut(t,distance);
          }
        }
        for(const auto& edge:edges) {
          const auto p=vertex(edge.first);const auto e=delta(vertex(edge.second),p),r=delta(a,p);
          const auto square=dot(e,e),slope=dot(chord,e)/square;if(slope==0)continue;
          const auto first=dot(r,e)/square;
          for(long double fraction:{(long double)NormalFeatureThreshold3D,1.L-NormalFeatureThreshold3D}) {
            const auto t=(fraction-first)/slope;if(!(t>0 && t<1))continue;
            Vector residual{};for(unsigned j=0;j<3;++j)residual[j]=std::fma(t,chord[j],r[j])-fraction*e[j];
            classificationCut(t,std::sqrt(dot(residual,residual)));
          }
        }
      }
      continue;
    }
    for (const auto face : faces) {
      constexpr unsigned count=2;
      Vector x[3]{};
      for(unsigned k=0;k<count;++k)for(unsigned j=0;j<nDim;++j)
        x[k][j]=SU2_TYPE::GetValue(walls[wall].coord[nDim*d.elem[count*face+k]+j]);
      for (unsigned k = 0; k < count; ++k) {
        Vector r{};
        for (unsigned j = 0; j < nDim; ++j) r[j] = a[j]-x[k][j];
        for (const auto level : levels) {
          roots(dot(chord,chord),2*dot(r,chord),dot(r,r)-static_cast<long double>(level)*level);
        }
        if (nDim == 2 && k == 1) continue;
        Vector edge{};
        for (unsigned j = 0; j < nDim; ++j) edge[j] = x[(k+1)%count][j]-x[k][j];
        const auto ee = dot(edge,edge), re = dot(r,edge)/ee, ce = dot(chord,edge)/ee;
        Vector rp{}, cp{};
        for (unsigned j = 0; j < nDim; ++j) { rp[j]=r[j]-re*edge[j]; cp[j]=chord[j]-ce*edge[j]; }
        for (const auto level : levels) {
          roots(dot(cp,cp),2*dot(rp,cp),dot(rp,rp)-static_cast<long double>(level)*level);
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
    for(unsigned k=0;k<3;++k) {const su2double x[2]={triangle[k][0],triangle[k][1]};d[k]=SU2_TYPE::GetValue(EvaluateImpl(wall,x,1.,true).distance);}
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
        const double exact=SU2_TYPE::GetValue(EvaluateImpl(wall,x,1.,true).distance);
        subdivide=subdivide || (std::abs(exact-.5*(d[k]+d[next]))>tolerance);
      }
    }
  }
  // D1 feature vertices: refine while hn_f varies by more than the band ratio over the triangle (best effort up to
  // the existing depth limit; only an unresolved wall band is fatal).
  if (!subdivide && depth < 16 && FeatureRefinement2D(triangle, bandRatio)) subdivide = true;
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
  if(sample.profile_version!=outerNormal.version)throw std::runtime_error("Stale BL sample from another profile.");
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

void CBoundaryLayerMetric::ApplyPointOnChord(const su2double* X, Tensor& metric, su2double coreEigenvalue,
                                             const ChordOwners& owners, long double t, bool useTangentialFloor) {
  if (!owners.walls.empty() && owners.walls.size() != walls.size())
    throw std::invalid_argument("Native BL chord ownership does not match the wall set.");
  for (unsigned short wall = 0; wall < walls.size(); ++wall) {
    const auto& d = *data[wall];
    bool inside = true;
    for (unsigned short k = 0; k < nDim; ++k) inside = inside && X[k] >= d.low[k] && X[k] <= d.high[k];
    if (!inside || !d.adt) continue;
    const auto pinned = owners.walls.empty() ? ChordOwners::NoFacet : owners.Owner(wall, t);
    ApplySample(wall, EvaluateImpl(wall, X, 1.0, false, pinned), metric, coreEigenvalue, nullptr, useTangentialFloor);
  }
}

unsigned long CBoundaryLayerMetric::ChordOwners::Owner(unsigned short wall, long double t) const {
  if (wall >= walls.size() || !(t >= 0 && t <= 1)) throw std::invalid_argument("Invalid native BL chord owner query.");
  const auto& pieces = walls[wall];
  const auto p = std::lower_bound(pieces.begin(), pieces.end(), t,
                                  [](const Piece& piece, long double value) { return piece.hi < value; });
  return p == pieces.end() ? NoFacet : p->facet;
}

size_t CBoundaryLayerMetric::ChordOwners::Pieces() const {
  size_t count = 0;
  for (const auto& wall : walls) count += wall.size();
  return count;
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


bool CBoundaryLayerMetric::PlanarWalls3D() const {
  if(nDim!=3 || walls.empty())return false;
  for(size_t wall=0;wall<walls.size();++wall) {
    const auto& coord=walls[wall].coord;const auto& triangles=data[wall]->elem;
    if(walls[wall].authority || triangles.size()<3)return false;
    std::array<long double,3> a{},u{},v{},normal{};long double scale=0;
    for(auto x:coord)scale=std::max(scale,std::abs((long double)SU2_TYPE::GetValue(x)));
    for(unsigned j=0;j<3;++j) {
      a[j]=SU2_TYPE::GetValue(coord[3*triangles[0]+j]);
      u[j]=(long double)SU2_TYPE::GetValue(coord[3*triangles[1]+j])-a[j];
      v[j]=(long double)SU2_TYPE::GetValue(coord[3*triangles[2]+j])-a[j];
    }
    normal={u[1]*v[2]-u[2]*v[1],u[2]*v[0]-u[0]*v[2],u[0]*v[1]-u[1]*v[0]};
    const auto length=std::hypot(normal[0],normal[1],normal[2]);
    if(!(length>0))return false;
    for(auto& x:normal)x/=length;
    const auto tolerance=128*std::numeric_limits<double>::epsilon()*scale;
    for(size_t point=0;point<coord.size()/3;++point) {
      long double distance=0;
      for(unsigned j=0;j<3;++j)distance+=normal[j]*((long double)SU2_TYPE::GetValue(coord[3*point+j])-a[j]);
      if(std::abs(distance)>tolerance)return false;
    }
  }
  return true;
}

bool CBoundaryLayerMetric::SupportsCorrectionSupport3D() const {
  if(nDim!=3 || walls.empty())return false;
  for(const auto& wall:walls)if(wall.authority || wall.layer.growth<1 || wall.layer.thickness<wall.layer.firstHeight)return false;
  return true;
}

std::shared_ptr<SU2Native3D::SupportSampler3D> CBoundaryLayerMetric::PrepareCorrectionSupport3D(
    size_t maximumBytes,uint64_t maximumWork) const {
  if(nDim!=3)throw std::invalid_argument("Correction support requires3D faceted walls.");
  std::vector<SU2Native3D::SupportWall3D> support;
  for(size_t wall=0;wall<walls.size();++wall) {
    if(walls[wall].authority)throw std::invalid_argument("Analytic wall correction support is not qualified; retain analytic quadrature.");
    SU2Native3D::SupportWall3D row;const auto& d=*data[wall];
    row.first_height=SU2_TYPE::GetValue(d.h0);row.growth=SU2_TYPE::GetValue(d.growth);row.thickness=SU2_TYPE::GetValue(d.thickness);
    for(size_t p=0;p<walls[wall].coord.size()/3;++p)row.vertices.push_back({SU2_TYPE::GetValue(walls[wall].coord[3*p]),SU2_TYPE::GetValue(walls[wall].coord[3*p+1]),SU2_TYPE::GetValue(walls[wall].coord[3*p+2])});
    for(size_t f=0;f<d.elem.size()/3;++f)row.triangles.push_back({d.elem[3*f],d.elem[3*f+1],d.elem[3*f+2]});
    support.push_back(std::move(row));
  }
  return std::make_shared<SU2Native3D::SupportSampler3D>(support,maximumBytes,maximumWork);
}
