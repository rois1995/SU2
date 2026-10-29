/*!
 * \file CNativeMetric3D.cpp
 * \brief Actual-query metric shape, unique edges and frozen-target private progress.
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md).
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */
#include "../../include/adaptation/CNativeMetric3D.hpp"
#include "../../include/adaptation/CNativeFma.hpp"
#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <limits>
#include <set>
#include <sstream>
#include <stdexcept>
namespace SU2Native3D {
long double FusedMultiplyAdd(long double x,long double y,long double z) noexcept {
#if (defined(__x86_64__) || defined(__i386__)) && defined(__LDBL_MANT_DIG__) && __LDBL_MANT_DIG__==64
  // x87 extended format only (other long-double formats, e.g. AArch64 binary128, use std::fma below).
  static_assert(std::numeric_limits<long double>::digits==64 && std::numeric_limits<long double>::radix==2,
                "Native fused multiply-add assumes the x87 extended format.");
  struct Bits{uint64_t mantissa;uint16_t sign_exponent;};
  auto bits=[](long double value){Bits b{};std::memcpy(&b,&value,10);return b;};
  // Normal (explicit integer bit) operand with a biased exponent in [256, 0x7fff-256].
  auto inside=[](const Bits& b){const int e=b.sign_exponent&0x7fff;return e>=256 && e<=0x7fff-256 && (b.mantissa>>63);};
  const auto bx=bits(x),by=bits(y),bz=bits(z);
  constexpr int bias=16383;
  const int product=(bx.sign_exponent&0x7fff)+(by.sign_exponent&0x7fff);
  // Product and sum stay normal and finite: Dekker's split/product and Knuth's sum are exact, every nonzero exact
  // result is a multiple of an ulp far above the subnormal range, and the odd-rounded part below is normal.
  if(!inside(bx) || !inside(by) || !inside(bz) || product<bias+256 || product>0x7fff+bias-256)return std::fma(x,y,z);
  constexpr long double split=4294967297.0L;  // 2^32+1: Veltkamp halves of a 64-bit significand
  long double x1=x*split,y1=y*split;const long double m1=x*y;
  x1=(x-x1)+x1;y1=(y-y1)+y1;
  const long double x2=x-x1,y2=y-y1;
  const long double m2=(((x1*y1-m1)+x1*y2)+x2*y1)+x2*y2;  // m1+m2 = x*y exactly
  const long double a1=z+m1;
  long double t1=a1-z,t2=a1-t1;t1=m1-t1;t2=z-t2;
  const long double a2=t1+t2;  // a1+a2 = z+m1 exactly
  if(a1==0 && m2==0)return z+m1;
  // Round to odd of a2+m2: RN neighbour u and its exact error; an inexact even u moves one ulp toward the exact sum.
  const long double u=a2+m2;
  long double s1=u-a2,s2=u-s1;s1=m2-s1;s2=a2-s2;
  const long double error=s1+s2;
  long double odd=u;
  if(error!=0) {
    auto b=bits(u);
    if(!(b.mantissa&1)) {
      if((b.sign_exponent&0x7fff)<=1 || (b.sign_exponent&0x7fff)==0x7fff || !(b.mantissa>>63))return std::fma(x,y,z);
      if((error>0)==(u>0))++b.mantissa;  // even significand <= 2^64-2: stays in its binade
      else if(b.mantissa==(uint64_t(1)<<63)){b.mantissa=~uint64_t(0);--b.sign_exponent;}  // below a power of two
      else --b.mantissa;
      std::memcpy(&odd,&b,10);
    }
  }
  return a1+odd;
#else
  return std::fma(x,y,z);
#endif
}
long double CompensatedCross2D(long double ax,long double ay,long double bx,long double by) {
  const auto p=ax*by,q=ay*bx;
  return (p-q)+(FusedMultiplyAdd(ax,by,-p)-FusedMultiplyAdd(ay,bx,-q));
}
Point MetricMeanPoint(Point origin,const std::vector<std::pair<Point,Tensor>>& neighbors,size_t maximum_neighbors) {
  if(neighbors.empty() || neighbors.size()>maximum_neighbors)
    throw std::invalid_argument("Native3D movement neighbor count exceeds bounds.");
  long double scale=0;
  for(const auto& row:neighbors) {
    ValidateTensor(row.second);
    const auto& m=row.second;scale=std::max({scale,std::abs((long double)m.xx),std::abs((long double)m.xy),
        std::abs((long double)m.xz),std::abs((long double)m.yy),std::abs((long double)m.yz),std::abs((long double)m.zz)});
  }
  long double a[3][3]{},rhs[3]{},l[3][3]{},work[3]{},delta[3]{};
  for(const auto& row:neighbors) {
    const auto& m=row.second;const long double matrix[3][3]={{m.xx,m.xy,m.xz},{m.xy,m.yy,m.yz},{m.xz,m.yz,m.zz}};
    const long double d[3]={(long double)row.first.x-origin.x,(long double)row.first.y-origin.y,(long double)row.first.z-origin.z};
    for(unsigned i=0;i<3;++i)for(unsigned j=0;j<3;++j){a[i][j]+=matrix[i][j]/scale;rhs[i]+=matrix[i][j]/scale*d[j];}
  }
  for(unsigned i=0;i<3;++i)for(unsigned j=0;j<=i;++j) {
    auto value=a[i][j];for(unsigned k=0;k<j;++k)value-=l[i][k]*l[j][k];
    if(i==j){if(!(value>0))throw std::runtime_error("Unrepresentable native3D movement frame.");l[i][j]=std::sqrt(value);}
    else l[i][j]=value/l[j][j];
  }
  for(unsigned i=0;i<3;++i){auto value=rhs[i];for(unsigned j=0;j<i;++j)value-=l[i][j]*work[j];work[i]=value/l[i][i];}
  for(int i=2;i>=0;--i){auto value=work[i];for(unsigned j=i+1;j<3;++j)value-=l[j][i]*delta[j];delta[i]=value/l[i][i];}
  const Point result{double(origin.x+delta[0]),double(origin.y+delta[1]),double(origin.z+delta[2])};
  if(!std::isfinite(result.x) || !std::isfinite(result.y) || !std::isfinite(result.z))
    throw std::runtime_error("Nonfinite native3D movement proposal.");
  return result;
}


MetricShapeLinearization LinearizeMetricShape(const Tetrahedron& tet,const Tensor& metric,
                                            const std::array<Tensor,3>* spatial) {
  const auto measured=Measure(tet,metric);
  using V=std::array<long double,3>;using M=std::array<V,3>;
  auto dot=[](V a,V b){long double q=0;for(unsigned j=0;j<3;++j)q+=a[j]*b[j];return q;};
  auto sub=[](V a,V b){for(unsigned j=0;j<3;++j)a[j]-=b[j];return a;};
  auto cross=[](V a,V b){return V{a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]};};
  auto matrix=[](Tensor m,long double scale){return M{{{{m.xx/scale,m.xy/scale,m.xz/scale}},{{m.xy/scale,m.yy/scale,m.yz/scale}},{{m.xz/scale,m.yz/scale,m.zz/scale}}}};};
  auto apply=[&](const M& a,V v){return V{dot(a[0],v),dot(a[1],v),dot(a[2],v)};};
  const long double scale=std::max({std::abs(metric.xx),std::abs(metric.xy),std::abs(metric.xz),std::abs(metric.yy),std::abs(metric.yz),std::abs(metric.zz)});
  const auto m=matrix(metric,scale);M l{},inverse{};
  for(unsigned i=0;i<3;++i)for(unsigned j=0;j<=i;++j) {
    auto q=m[i][j];for(unsigned k=0;k<j;++k)q=FusedMultiplyAdd(-l[i][k],l[j][k],q);
    if(i==j){if(!(q>0))throw std::invalid_argument("Unresolved metric shape Jacobian frame.");l[i][j]=std::sqrt(q);}else l[i][j]=q/l[j][j];
  }
  for(unsigned column=0;column<3;++column) {
    V y{},x{};
    for(unsigned i=0;i<3;++i){long double q=i==column?1:0;for(unsigned k=0;k<i;++k)q-=l[i][k]*y[k];y[i]=q/l[i][i];}
    for(int i=2;i>=0;--i){long double q=y[i];for(unsigned k=i+1;k<3;++k)q-=l[k][i]*x[k];x[i]=q/l[i][i];}
    for(unsigned i=0;i<3;++i)inverse[i][column]=x[i];
  }
  long double size=0;std::array<V,4> x{};
  for(unsigned i=0;i<4;++i) {
    x[i]={static_cast<long double>(tet[i].x)-tet[0].x,static_cast<long double>(tet[i].y)-tet[0].y,static_cast<long double>(tet[i].z)-tet[0].z};
    for(auto value:x[i])size=std::max(size,std::abs(value));
  }
  if(!(size>0))throw std::invalid_argument("Degenerate metric shape Jacobian.");
  for(auto& v:x)for(auto& value:v)value/=size;
  const auto det=((Orientation(tet[0],tet[1],tet[2],tet[3])/size)/size)/size;
  if(!(det>0))throw std::invalid_argument("Metric shape Jacobian needs positive geometry.");
  std::array<V,4> volume{{{},cross(x[2],x[3]),cross(x[3],x[1]),cross(x[1],x[2])}};
  for(unsigned j=0;j<3;++j)volume[0][j]=-volume[1][j]-volume[2][j]-volume[3][j];
  long double squared=0;std::array<std::array<long double,4>,4> length{};unsigned corner=0;long double smallest=INFINITY;
  for(unsigned i=0;i<4;++i)for(unsigned j=i+1;j<4;++j) {
    const auto e=sub(x[i],x[j]);const auto value=dot(e,apply(m,e));
    if(!(value>0))throw std::invalid_argument("Unresolved metric shape Jacobian edge.");
    length[i][j]=length[j][i]=value;squared+=value;
  }
  for(unsigned i=0;i<4;++i) {
    long double product=1;for(unsigned j=0;j<4;++j)if(i!=j)product*=std::sqrt(length[i][j]);
    const auto value=1/product;if(value<smallest){smallest=value;corner=i;}
  }
  V q_spatial{},j_spatial{};
  if(spatial)for(unsigned axis=0;axis<3;++axis) {
    const auto dm=matrix((*spatial)[axis],scale);long double trace=0,ds=0,dl=0;
    for(unsigned i=0;i<3;++i)for(unsigned j=0;j<3;++j)trace+=inverse[i][j]*dm[j][i];
    for(unsigned i=0;i<4;++i)for(unsigned j=i+1;j<4;++j){const auto e=sub(x[i],x[j]);ds+=dot(e,apply(dm,e));}
    for(unsigned j=0;j<4;++j)if(j!=corner){const auto e=sub(x[corner],x[j]);dl+=dot(e,apply(dm,e))/length[corner][j];}
    q_spatial[axis]=(trace/3-ds/squared)/4;j_spatial[axis]=(trace-dl)/8;
  }
  MetricShapeLinearization result;const auto q_goal=1.01*MinimumMeanRatio,j_goal=1.01*MinimumScaledJacobian;
  result.residual={std::max(0.,1-measured.mean_ratio/q_goal),std::max(0.,1-measured.minimum_scaled_jacobian/j_goal)};
  for(unsigned i=0;i<4;++i) {
    V ds{},dl{};
    for(unsigned j=0;j<4;++j)if(j!=i){const auto e=apply(m,sub(x[i],x[j]));for(unsigned a=0;a<3;++a)ds[a]+=2*e[a]/(size*squared);}
    if(i==corner){for(unsigned j=0;j<4;++j)if(j!=corner){const auto e=apply(m,sub(x[corner],x[j]));for(unsigned a=0;a<3;++a)dl[a]+=e[a]/(size*length[corner][j]);}}
    else {const auto e=apply(m,sub(x[i],x[corner]));for(unsigned a=0;a<3;++a)dl[a]=e[a]/(size*length[corner][i]);}
    for(unsigned a=0;a<3;++a) {
      const auto dv=volume[i][a]/(size*det);
      if(result.residual[0]>0)result.derivative[0][i][a]=double(-measured.mean_ratio/q_goal*((2.L/3)*dv-ds[a]+q_spatial[a]));
      if(result.residual[1]>0)result.derivative[1][i][a]=double(-measured.minimum_scaled_jacobian/j_goal*(dv-dl[a]+j_spatial[a]));
      for(unsigned r=0;r<2;++r)if(!std::isfinite(result.derivative[r][i][a]))throw std::invalid_argument("Nonfinite metric shape Jacobian.");
    }
  }
  return result;
}

namespace {
bool Same(Point a, Point b) { return a.x == b.x && a.y == b.y && a.z == b.z; }
void Shape(const PatchMetric& m);
// Per-operation (generation) shape gate under the current ShapeAdmission; the final gate is Shape().
std::atomic<int> shape_admission{int(ShapeAdmission::Absolute)};  // explicit legacy/diagnostic default; production scopes ValidityOnly
void GenerationShape(const PatchMetric& m) { Shape(m); }
}  // namespace
void SetShapeAdmission(ShapeAdmission admission) { shape_admission.store(int(admission)); }
ShapeAdmission GetShapeAdmission() { return ShapeAdmission(shape_admission.load()); }
bool EdgeMeasure::Reusable(Point from,Point to,const std::shared_ptr<const FrozenMetricIdentity>& field)const {
  return field && identity==field && Same(a,from) && Same(b,to) && std::isfinite(length) && length>0;
}
std::vector<unsigned char> MetricEdgeRequests(const std::vector<Cell>& cells,std::map<Edge,EdgeMeasure>& reuse,
    const std::shared_ptr<const FrozenMetricIdentity>& identity,const std::map<Edge,EdgeMeasure>* campaign) {
  if(cells.size()>MaximumReplacementCells || !identity)throw std::invalid_argument("Invalid frozen metric query plan.");
  std::vector<unsigned char> masks(cells.size(),0);
  for(size_t n=0;n<cells.size();++n) {
    unsigned bit=0;
    for(unsigned i=0;i<4;++i)for(unsigned j=i+1;j<4;++j,++bit) {
      Node a=cells[n].v[i],b=cells[n].v[j];if(a.id>b.id)std::swap(a,b);
      const Edge key{a.id,b.id};auto found=reuse.find(key);
      if(found!=reuse.end() && found->second.Reusable(a.p,b.p,identity))continue;
      if(campaign) {
        const auto prior=campaign->find(key);
        if(prior!=campaign->end() && prior->second.Reusable(a.p,b.p,identity)) {
          reuse.insert_or_assign(key,prior->second);continue;
        }
      }
      masks[n]|=1u<<bit;
    }
  }
  return masks;
}
static PatchMetric MeasureMetricCellsImpl(const std::vector<Cell>& cells, FrozenField& target, MetricStats* stats,
                               const std::map<Edge, EdgeMeasure>* reuse, bool require_shape, MetricDiagnosis* diagnosis,bool shapes_only,
                               std::map<Edge,EdgeMeasure>* campaign, bool measure_shape=true) {
  if (diagnosis) { *diagnosis={}; diagnosis->failure=MetricFailure::Measurement; }
  if (cells.empty() || cells.size() > MaximumReplacementCells)
    throw std::invalid_argument("Native 3D metric patch exceeds budget or is empty.");
  if (stats) {
    ++stats->patches;
    stats->maximum_cells = std::max(stats->maximum_cells, cells.size());
  }
  PatchMetric result;
  if(measure_shape) result.weakest_cell = result.sliver_cell = cells.front().id;
  std::map<Id, Node> nodes;
  std::set<Id> cell_ids;
  std::set<std::array<Id, 4>> simplices;
  for (const auto& cell : cells) {
    if (!cell_ids.insert(cell.id).second) throw std::invalid_argument("Duplicate native 3D metric cell ID.");
    std::array<Id, 4> key;
    for (size_t i = 0; i < 4; ++i) {
      auto n = cell.v[i];
      key[i] = n.id;
      const auto old = nodes.emplace(n.id, n);
      if (!old.second && !Same(old.first->second.p, n.p))
        throw std::invalid_argument("Inconsistent native 3D metric node coordinates.");
    }
    std::sort(key.begin(), key.end());
    if (std::adjacent_find(key.begin(), key.end()) != key.end() || !simplices.insert(key).second)
      throw std::invalid_argument("Repeated native 3D metric simplex/node.");
    if (stats) ++stats->cells;
    const auto orientation=Orientation(cell.v[0].p,cell.v[1].p,cell.v[2].p,cell.v[3].p);
    if(!(orientation>0)) throw std::invalid_argument("Native 3D metric cell must have positive exact orientation.");
    if(measure_shape) {
      MetricMeasures m;
      try {
        m = Measure({cell.v[0].p, cell.v[1].p, cell.v[2].p, cell.v[3].p}, target.Query(Centroid(cell)).target);
      } catch (const std::exception& e) {
        throw std::runtime_error(std::string(e.what()) + " at native 3D metric cell " + std::to_string(cell.id) + ".");
      }
      if (!result.shape_measured || m.mean_ratio < result.minimum_mean_ratio) {
        result.minimum_mean_ratio = m.mean_ratio;
        result.weakest_cell = cell.id;
      }
      if (!result.shape_measured || m.minimum_scaled_jacobian < result.minimum_scaled_jacobian) {
        result.minimum_scaled_jacobian = m.minimum_scaled_jacobian;
        result.sliver_cell = cell.id;
      }
      result.shape_measured=true;
    }
    if(!shapes_only)for (size_t i = 0; i < 4; ++i)
      for (size_t j = i + 1; j < 4; ++j) result.edges.emplace(Edge{key[i], key[j]}, EdgeMeasure{});
  }
  if(shapes_only)return result; // Only the scalar ranking wrapper can consume this incomplete measurement.
  // Optional diagnostic shape gate; edge-only measurements never query centroids.
  if (diagnosis && result.shape_measured) {
    diagnosis->shape_measured=true;
    diagnosis->minimum_mean_ratio=result.minimum_mean_ratio;
    diagnosis->minimum_scaled_jacobian=result.minimum_scaled_jacobian;
    diagnosis->weakest_cell=result.weakest_cell; diagnosis->sliver_cell=result.sliver_cell;
    if (require_shape && GetShapeAdmission()==ShapeAdmission::Absolute &&
        (result.minimum_mean_ratio < MinimumMeanRatio || result.minimum_scaled_jacobian < MinimumScaledJacobian))
      diagnosis->failure=MetricFailure::Shape;
  }
  if (require_shape && GetShapeAdmission()==ShapeAdmission::Absolute) GenerationShape(result);
  if (stats) {
    stats->maximum_nodes = std::max(stats->maximum_nodes, nodes.size());
    stats->maximum_edges = std::max(stats->maximum_edges, result.edges.size());
  }
  target.RequireEdgePolicy();
  for (auto& edge : result.edges) {
    auto& m = edge.second;
    m.a = nodes.at(edge.first[0]).p;
    m.b = nodes.at(edge.first[1]).p;
    if (stats) ++stats->edges;
    bool reused = false;
    if (reuse) {
      const auto prior = reuse->find(edge.first);
      if (prior != reuse->end() && prior->second.Reusable(m.a,m.b,target.Identity())) {
        m.length = prior->second.length;
        reused = true;
        if (stats) ++stats->edge_reused;
      }
    }
    if(!reused && campaign) {
      const auto prior=campaign->find(edge.first);
      if(prior!=campaign->end() && prior->second.Reusable(m.a,m.b,target.Identity())) {
        m.length=prior->second.length;reused=true;
        if(stats){++stats->edge_reused;++stats->edge_campaign_reused;}
      }
    }
    if (!reused) {
      if (stats) ++stats->edge_evaluations;
      try {
        m.length = target.TargetEdgeLength(m.a, m.b);
      } catch (const std::exception& e) {
        throw std::runtime_error(std::string(e.what()) + " at native 3D metric edge " + std::to_string(edge.first[0]) +
                                 "," + std::to_string(edge.first[1]) + ".");
      }
    }
    m.identity = target.Identity();
    if(campaign && !reused) {
      // shortcut: bounded edge-key eviction; change policy only if measured capacity misses justify it.
      if(campaign->size()>=MaximumCampaignEdges && !campaign->count(edge.first))campaign->erase(campaign->begin());
      campaign->insert_or_assign(edge.first,m);
    }
    if (m.length > result.maximum_length) {
      result.maximum_length = m.length;
      result.longest_edge = edge.first;
    }
  }
  if (diagnosis) diagnosis->failure=MetricFailure::None;
  return result;
}
double MetricShapeQuality(const std::vector<Cell>& cells,FrozenField& target) {
  const auto shape=MeasureMetricCellsImpl(cells,target,nullptr,nullptr,false,nullptr,true,nullptr);
  return std::min(shape.minimum_mean_ratio/MinimumMeanRatio,shape.minimum_scaled_jacobian/MinimumScaledJacobian);
}
PatchMetric MeasureMetricEdges(const std::vector<Cell>& cells,FrozenField& target,MetricStats* stats,
                               const std::map<Edge,EdgeMeasure>* reuse,std::map<Edge,EdgeMeasure>* campaign) {
  return MeasureMetricCellsImpl(cells,target,stats,reuse,false,nullptr,false,campaign,false);
}
PatchMetric MeasureMetricCells(const std::vector<Cell>& cells,FrozenField& target,MetricStats* stats,
                               const std::map<Edge,EdgeMeasure>* reuse,bool require_shape,MetricDiagnosis* diagnosis,
                               std::map<Edge,EdgeMeasure>* campaign) {
  return MeasureMetricCellsImpl(cells,target,stats,reuse,require_shape,diagnosis,false,campaign,true);
}
namespace {
void Shape(const PatchMetric& m) {
  if (!m.shape_measured) throw std::invalid_argument("Native 3D final shape was not measured.");
  if (!std::isfinite(m.minimum_mean_ratio) || !std::isfinite(m.minimum_scaled_jacobian) ||
      m.minimum_mean_ratio < MinimumMeanRatio || m.minimum_scaled_jacobian < MinimumScaledJacobian) {
    std::ostringstream reason;
    reason << std::setprecision(17) << "Native 3D metric shape/sliver failure: q=" << m.minimum_mean_ratio
           << " at cell " << m.weakest_cell << ", J=" << m.minimum_scaled_jacobian << " at cell " << m.sliver_cell
           << '.';
    throw std::invalid_argument(reason.str());
  }
}
long double Excess(long double length) {
  const auto e = std::max(0.L, length - MaximumMetricEdge);
  return e * e;
}
bool Shared(const std::map<Edge, EdgeMeasure>& others, const std::pair<const Edge, EdgeMeasure>& edge) {
  const auto old = others.find(edge.first);
  return old != others.end() && Same(old->second.a, edge.second.a) && Same(old->second.b, edge.second.b);
}
}  // namespace
bool ValidateMetricPatch(const std::vector<Cell>& cells, FrozenField& target, PatchMetric& measured,
                         std::string& reason, MetricStats* stats, const std::map<Edge, EdgeMeasure>* frozen_reuse) {
  try {
    measured = MeasureMetricCells(cells, target, stats, frozen_reuse);
    Shape(measured);
    if (measured.maximum_length > MaximumMetricEdge) {
      std::ostringstream message;
      message << std::setprecision(21) << "Native 3D oversized metric edge " << measured.longest_edge[0] << ','
              << measured.longest_edge[1] << ": length=" << measured.maximum_length << '.';
      throw std::invalid_argument(message.str());
    }
    reason.clear();
    return true;
  } catch (const std::exception& e) {
    if (stats) ++stats->failures;
    reason = e.what();
    return false;
  }
}
namespace {
std::atomic<int> refine_acceptance{int(RefineAcceptance::SplitInsertion)};  // production default (user decision 2026-10-23)
}  // namespace
void SetRefineAcceptance(RefineAcceptance acceptance) { refine_acceptance.store(int(acceptance)); }
RefineAcceptance GetRefineAcceptance() { return RefineAcceptance(refine_acceptance.load()); }
bool ValidateMetricChange(size_t old_count, const PatchMetric& before, size_t fresh_count,
                          const PatchMetric& measured, MetricChange kind, std::string& reason, MetricDiagnosis* diagnosis,
                          size_t relocated, bool coordinate_move) {
  if (diagnosis) { *diagnosis={}; diagnosis->failure=MetricFailure::Measurement; }
  try {
    if (kind != MetricChange::Refine && kind != MetricChange::Coarsen && kind != MetricChange::Repair && kind != MetricChange::GeometryRefine && kind != MetricChange::GeometryRepair)
      throw std::invalid_argument("Unknown native 3D metric-change intent.");
    const bool coordinate_repair = coordinate_move &&
        (kind == MetricChange::Repair || kind == MetricChange::GeometryRepair) && old_count == fresh_count && old_count > 0;
    if (coordinate_move && !coordinate_repair)
      throw std::invalid_argument("Coordinate-move metric exception requires a same-count Repair or GeometryRepair.");
    if (!old_count || !fresh_count ||
        (old_count > MaximumReplacementCells && !coordinate_repair) ||
        (fresh_count > MaximumReplacementCells && !coordinate_repair))
      throw std::invalid_argument("Native 3D metric-change cell count exceeds budget or is empty.");
    if (coordinate_repair && old_count > std::numeric_limits<size_t>::max() / 6)
      throw std::invalid_argument("Coordinate-move metric edge cap overflows size_t.");
    const size_t edge_limit = coordinate_repair ? 6 * old_count : 6 * MaximumReplacementCells;
    for (const auto* metric : {&before, &measured}) {
      if ((metric->shape_measured && (!std::isfinite(metric->minimum_mean_ratio) || !std::isfinite(metric->minimum_scaled_jacobian) ||
          metric->minimum_mean_ratio < 0 || metric->minimum_scaled_jacobian < 0)) || metric->edges.empty() ||
          metric->edges.size() > edge_limit)
        throw std::invalid_argument("Invalid native 3D metric-change measurement.");
      for (const auto& edge : metric->edges)
        if (!std::isfinite(edge.second.length) || edge.second.length <= 0)
          throw std::invalid_argument("Invalid native 3D metric-change edge measurement.");
    }
    if(GetShapeAdmission()==ShapeAdmission::Absolute && (!before.shape_measured || !measured.shape_measured))
      throw std::invalid_argument("Native 3D metric change lacks measured shape under absolute admission.");
    if (diagnosis) {
      diagnosis->shape_measured=measured.shape_measured;
      if(measured.shape_measured) {
        diagnosis->minimum_mean_ratio=measured.minimum_mean_ratio;
        diagnosis->minimum_scaled_jacobian=measured.minimum_scaled_jacobian;
        diagnosis->weakest_cell=measured.weakest_cell; diagnosis->sliver_cell=measured.sliver_cell;
      }
      diagnosis->failure=MetricFailure::Shape;
    }
    if (GetShapeAdmission()==ShapeAdmission::Absolute) GenerationShape(measured);
    long double removed = 0, added = 0, longest_removed = 0, longest_added = 0;
    for (const auto& edge : before.edges)
      if (!Shared(measured.edges, edge)) {
        removed += Excess(edge.second.length);
        longest_removed = std::max(longest_removed, edge.second.length);
      }
    for (const auto& edge : measured.edges)
      if (!Shared(before.edges, edge)) {
        added += Excess(edge.second.length);
        longest_added = std::max(longest_added, edge.second.length);
      }
    const bool aggregate_size_progress = removed > 0 && added < removed * (1 - 1e-12L);
    // Both measurements hold every edge of their cells, so the edge keys give the vertex sets. An old vertex absent
    // from the fresh cells is removed (collapse/absorb) unless the caller certifies it as relocated (a refit replaces a
    // moved apex one for one by a new identity). A refinement must also gain a vertex net.
    std::set<Id> old_vertices, fresh_vertices;
    for (const auto& edge : before.edges) old_vertices.insert(edge.first.begin(), edge.first.end());
    for (const auto& edge : measured.edges) fresh_vertices.insert(edge.first.begin(), edge.first.end());
    size_t gone = 0;
    for (const auto id : old_vertices) gone += !fresh_vertices.count(id);
    const bool vertex_removal = gone > relocated || fresh_vertices.size() < old_vertices.size() + 1;
    const bool size_insertion = !vertex_removal && longest_removed > SplitInsertionLength;
    const bool frontier_progress = longest_removed > 0 && longest_added < longest_removed * (1 - 1e-12L);
    const bool size_progress = aggregate_size_progress ||
        (kind == MetricChange::Refine && GetRefineAcceptance() == RefineAcceptance::SplitInsertion &&
         size_insertion && frontier_progress);
    const auto score=[](const PatchMetric& m){return std::min(m.minimum_mean_ratio/MinimumMeanRatio,
                                                           m.minimum_scaled_jacobian/MinimumScaledJacobian);};
    const bool quality_progress = before.shape_measured && measured.shape_measured &&
        (GetShapeAdmission()==ShapeAdmission::ValidityOnly ? score(measured)>score(before)+1e-7 :
        ((measured.minimum_mean_ratio > before.minimum_mean_ratio + 1e-7 &&
         measured.minimum_scaled_jacobian >= before.minimum_scaled_jacobian * (1 - 1e-12)) ||
        (measured.minimum_scaled_jacobian > before.minimum_scaled_jacobian + 1e-7 &&
         measured.minimum_mean_ratio >= before.minimum_mean_ratio * (1 - 1e-12))));
    if (diagnosis) {
      diagnosis->failure=MetricFailure::Progress;
      diagnosis->removed_excess=removed; diagnosis->added_excess=added;
      diagnosis->size_progress=size_progress;
      diagnosis->quality_progress=quality_progress && (GetShapeAdmission()==ShapeAdmission::ValidityOnly || added<=removed);
      diagnosis->cell_removal=fresh_count<old_count;
      diagnosis->size_insertion=size_insertion; diagnosis->vertex_removal=vertex_removal;
      diagnosis->longest_removed=longest_removed;
    }
    if (kind == MetricChange::Refine && GetRefineAcceptance() == RefineAcceptance::SplitInsertion) {
      // A size insertion may add total excess, but its longest replacement edge must shrink the changed-edge frontier.
      if (vertex_removal && !aggregate_size_progress)
        throw std::invalid_argument("Native 3D refinement removes a vertex without reducing frozen changed-edge excess.");
      if (!vertex_removal && size_insertion && !frontier_progress)
        throw std::invalid_argument("Native 3D size insertion does not shorten the longest changed-edge frontier.");
      // 2D (CNativeMesh2D.hpp SplitPatch): the minimum quality must rise. Production has no skin here, so it cannot
      // test retained interior edges; production Refine requests are edges > MaximumMetricEdge (size insertions).
      const bool short_split_allowed = GetShapeAdmission()==ShapeAdmission::ValidityOnly ? added==0 :
          (added==0 && measured.shape_measured && before.shape_measured && measured.minimum_mean_ratio>before.minimum_mean_ratio+1e-7);
      if (!vertex_removal && !size_insertion && !short_split_allowed)
        throw std::invalid_argument("Native 3D refinement below the split length adds an oversized edge or no resolution.");
    } else if (kind == MetricChange::Refine) {
      if (fresh_count <= old_count || !(removed > 0 && added < removed * (1 - 1e-12L)))
        throw std::invalid_argument("Native 3D refinement does not reduce frozen changed-edge excess.");
    } else if (kind == MetricChange::GeometryRefine) {
      // Validity-only construction may accrue metric debt; its caller separately certifies strict geometry progress.
      if (fresh_count <= old_count || (added > removed && GetShapeAdmission()!=ShapeAdmission::ValidityOnly))
        throw std::invalid_argument("Native3D geometric refinement adds frozen metric excess or removes no geometric freedom.");
    } else if (kind == MetricChange::GeometryRepair) {
      if (added>removed && GetShapeAdmission()!=ShapeAdmission::ValidityOnly)
        throw std::invalid_argument("Native3D geometric repair adds frozen metric excess.");
    } else if (kind == MetricChange::Coarsen) {
      if (fresh_count >= old_count || (added > 0 &&
          (GetShapeAdmission()!=ShapeAdmission::ValidityOnly || !aggregate_size_progress)))
        throw std::invalid_argument("Native 3D coarsening removes no cells or fails changed-edge size admission.");
    } else {
      const bool repair_progress=size_progress || (quality_progress &&
          (GetShapeAdmission()==ShapeAdmission::ValidityOnly || added<=removed));
      if (!repair_progress)
        throw std::invalid_argument("Native 3D repair improves neither frozen size excess nor quality/sliver bounds.");
    }
    if (diagnosis) diagnosis->failure=MetricFailure::None;
    reason.clear();
    return true;
  } catch (const std::exception& e) {
    reason = e.what();
    return false;
  }
}
bool ValidateMetricChange(const std::vector<Cell>& old, const std::vector<Cell>& fresh, MetricChange kind,
                          FrozenField& target, PatchMetric& measured, std::string& reason, MetricStats* stats, const std::map<Edge, EdgeMeasure>* frozen_reuse, MetricDiagnosis* diagnosis,
                          size_t relocated) {
  if (diagnosis) { *diagnosis={}; diagnosis->failure=MetricFailure::Measurement; }
  try {
    if (stats) ++stats->changes;
    if (kind != MetricChange::Refine && kind != MetricChange::Coarsen && kind != MetricChange::Repair && kind != MetricChange::GeometryRefine && kind != MetricChange::GeometryRepair)
      throw std::invalid_argument("Unknown native 3D metric-change intent.");
    const bool measure_shape = GetShapeAdmission()==ShapeAdmission::Absolute || kind==MetricChange::Repair || kind==MetricChange::GeometryRepair;
    measured = measure_shape ? MeasureMetricCells(fresh,target,stats,frozen_reuse,
                                                   GetShapeAdmission()==ShapeAdmission::Absolute,diagnosis) :
                               MeasureMetricEdges(fresh,target,stats,frozen_reuse);
    if(diagnosis && measured.shape_measured) {
      diagnosis->shape_measured=true; diagnosis->minimum_mean_ratio=measured.minimum_mean_ratio;
      diagnosis->minimum_scaled_jacobian=measured.minimum_scaled_jacobian;
      diagnosis->weakest_cell=measured.weakest_cell; diagnosis->sliver_cell=measured.sliver_cell;
    }
    if (diagnosis) diagnosis->failure=MetricFailure::Measurement;
    const auto before = measure_shape ? MeasureMetricCells(old,target,stats,&measured.edges) : MeasureMetricEdges(old,target,stats,&measured.edges);
    const bool valid = ValidateMetricChange(old.size(), before, fresh.size(), measured, kind, reason, diagnosis, relocated);
    if (!valid && stats) ++stats->failures;
    return valid;
  } catch (const std::exception& e) {
    if (stats) ++stats->failures;
    reason = e.what();
    return false;
  }
}
}  // namespace SU2Native3D
