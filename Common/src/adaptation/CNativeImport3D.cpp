/*!
 * \file CNativeImport3D.cpp
 * \brief CFD-owned tetrahedral import and original-P1 distributed reader return.
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md).
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */
#include "../../include/adaptation/CNativeImport3D.hpp"
#include "../../include/adaptation/CDistributedSearch.hpp"
#include "../../include/adaptation/CMeshGather.hpp"
#include "../../include/CConfig.hpp"
#include "../../include/geometry/CGeometry.hpp"
#include <cmath>
#include <type_traits>
#include <tuple>
#include <limits>
namespace SU2Native3D {
size_t OwnedStorageBytes(const Owned& cells) {
  return transfer_memory::Mul(cells.size(), sizeof(Owned::value_type) + 4 * sizeof(void*));
}
size_t ReferenceStorageBytes(const ReferenceState& reference, bool authority) {
  return transfer_memory::Add(transfer_memory::Bytes(reference.facets), transfer_memory::Bytes(reference.marker_names),
                              authority && reference.original ? reference.original->ResidentBytes() : 0);
}
namespace {
std::array<Face, 4> Faces(const Cell& c) {
  const auto& v = c.v;
  return {{{v[1].id, v[2].id, v[3].id},
           {v[0].id, v[3].id, v[2].id},
           {v[0].id, v[1].id, v[3].id},
           {v[0].id, v[2].id, v[1].id}}};
}
Face Key(Face f) {
  std::sort(f.begin(), f.end());
  return f;
}
int Bucket(Face f, int ranks) {
  uint64_t hash = 14695981039346656037ULL;
  for (auto id : Key(f)) {
    hash ^= id;
    hash *= 1099511628211ULL;
  }
  return hash % ranks;
}
int Parity(Face f) {
  unsigned count = 0;
  for (size_t i = 0; i < 3; ++i)
    for (size_t j = i + 1; j < 3; ++j) count += f[i] > f[j];
  return count % 2 ? -1 : 1;
}
size_t OwnedBytes(const Owned& cells) { return OwnedStorageBytes(cells); }
struct Envelope {
  World& world;
  BridgeStats& stats;
  double start, collective_start;
  uint64_t bytes_start, calls_start;
  Envelope(World& world, BridgeStats& stats)
      : world(world), stats(stats), start(world.seconds()), collective_start(world.collective_seconds),
        bytes_start(world.bytes_sent), calls_start(world.collective_calls) { stats = {}; }
  ~Envelope() {
    stats.tracked_bytes = world.bytes_sent - bytes_start;
    stats.tracked_calls = world.collective_calls - calls_start;
    stats.tracked_collective_seconds = world.collective_seconds - collective_start;
    stats.total_seconds = world.seconds() - start;
    stats.other_seconds =
        std::max(0., stats.total_seconds - stats.points_seconds - stats.cells_seconds - stats.faces_seconds -
                         stats.source_seconds - stats.query_seconds - stats.reader_seconds);
  }
};
bool Vote(World& world, const CLocalFailure& failure, std::string& reason) {
  const auto elected = world.elect(failure);
  reason = elected.message;
  return !elected.any;
}
bool Admit(World& world, size_t bytes, size_t ceiling, BridgeStats& stats, std::string& reason) {
  stats.maximum_native_bytes = std::max(stats.maximum_native_bytes, bytes);
  CLocalFailure failure;
  if (bytes == SIZE_MAX || bytes > ceiling) failure.Set(3, 0, "Native 3D bridge storage budget exceeded.");
  return Vote(world, failure, reason);
}
void Add(BridgeStats& stats, const FieldStats& field) {
  stats.requests += field.requests;
  stats.hits += field.hits;
  stats.misses += field.misses;
  stats.invalid_queries += field.invalid_queries;
  stats.failures += field.failures;
  stats.evictions += field.evictions;
  stats.compositions += field.compositions;
  stats.field_build_seconds += field.build_seconds;
  stats.field_search_seconds += field.search_seconds;
  stats.interpolation_seconds += field.interpolation_seconds;
  stats.composition_seconds += field.composition_seconds;
}
struct FieldAccounting {
  BridgeStats& stats;
  const FrozenField& field;
  // A rejected batch still incurred every query made before its exception.
  ~FieldAccounting() { Add(stats, field.Statistics()); }
};
struct FaceRecord {
  Face face{};
  Id cell = 0;
  int owner = 0, slot = 0, marker = -1, parity = 0;
  bool label = false;
  template <class S>
  void Fields(S& s) {
    s(face, cell, owner, slot, marker, parity, label);
  }
};
struct Binding {
  Id cell = 0;
  int slot = 0, marker = 0;
  template <class S>
  void Fields(S& s) {
    s(cell, slot, marker);
  }
};

using PrimitiveKey=std::tuple<unsigned,double,double,double,double,double,double,double,double,double,double>;
PrimitiveKey Primitive(const AnalyticSurface& g) {
  return {unsigned(g.kind),g.origin.x,g.origin.y,g.origin.z,g.extent.x,g.extent.y,g.extent.z,g.tolerance,g.axis.x,g.axis.y,g.axis.z};
}
/*! Freeze a continuous piecewise-affine exterior sensor target from original boundary nodes.
 * Bump columns copy nodal tensors vertically; spherical frusta copy them radially. Original P1 has priority.
 * Capped cylinders use one shared radial/axial collar map, including disk faces and rim identities.
 * Sorted source IDs choose compatible prism diagonals; component tags still control later association. */
bool Coverage(World& world,const Owned& cells,std::vector<DonorCell>& source,const PlanarReference& reference,
              double geometry_tolerance,size_t live,size_t ceiling,BridgeStats& stats,std::string& reason) {
  if(reference.Analytic().empty())return true;
  CLocalFailure failure;
  const size_t added=transfer_memory::Mul(cells.size(),12);
  const size_t requested=transfer_memory::Add(live,transfer_memory::GrowthBound(transfer_memory::Add(source.size(),added),sizeof(DonorCell)),
      transfer_memory::Mul(reference.Analytic().size(),1024));
  if(!Admit(world,requested,ceiling,stats,reason))return false;
  struct Rule {AnalyticSurface geometry;int lo=2,hi=-2;double height=std::numeric_limits<double>::infinity(),factor=0,pad=0;
    std::array<int,2> cap_lo{{2,2}},cap_hi{{-2,-2}};};
  std::map<PrimitiveKey,Rule> rules;
  std::map<Id,std::pair<PrimitiveKey,unsigned>> caps;
  Id maximum_node=0,maximum_cell=0;
  try {
    if(!(std::isfinite(geometry_tolerance)&&geometry_tolerance>0))throw std::invalid_argument("Invalid exterior source geometry bound.");
    for(const auto& row:reference.Analytic())if(row.second.kind!=AnalyticSurface::Kind::Disk)rules.emplace(Primitive(row.second),Rule{row.second});
    // Match declared end disks to a unique cylinder; their shared nodes use the same affine collar map.
    for(const auto& disk:reference.Analytic())if(disk.second.kind==AnalyticSurface::Kind::Disk) {
      const auto& d=disk.second;unsigned matches=0;
      for(const auto& row:rules)if(row.second.geometry.kind==AnalyticSurface::Kind::Cylinder) {
        const auto& g=row.second.geometry;
        const long double length=std::hypot(g.axis.x,g.axis.y,g.axis.z),dl=std::hypot(d.axis.x,d.axis.y,d.axis.z);
        const std::array<long double,3> a{{g.axis.x/length,g.axis.y/length,g.axis.z/length}},b{{d.axis.x/dl,d.axis.y/dl,d.axis.z/dl}};
        const auto cross=std::hypot(a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]);
        if(cross>64*std::numeric_limits<double>::epsilon() || std::abs(g.extent.x-d.extent.x)>g.tolerance+d.tolerance)continue;
        for(unsigned end=0;end<2;++end) {
          const long double t=end ? g.extent.y : 0;
          const auto distance=std::hypot(static_cast<long double>(d.origin.x)-g.origin.x-t*a[0],
                                        static_cast<long double>(d.origin.y)-g.origin.y-t*a[1],
                                        static_cast<long double>(d.origin.z)-g.origin.z-t*a[2]);
          if(distance<=g.tolerance+d.tolerance){caps[disk.first]={row.first,end};++matches;}
        }
      }
      if(matches>1)throw std::invalid_argument("Ambiguous cylinder/end-disk source collar association.");
    }
    for(const auto& d:source) {
      if(d.extension_component!=UINT64_MAX)throw std::invalid_argument("Exterior source already generated.");
      maximum_cell=std::max(maximum_cell,d.cell.id);for(auto n:d.cell.v)maximum_node=std::max(maximum_node,n.id);
      const auto& owned=cells.at(d.cell.id);
      for(size_t slot=0;slot<4;++slot)if(owned.physical[slot].present) {
        const auto id=owned.physical[slot].facet;
        const auto entry=reference.Analytic().find(id);
        if(entry==reference.Analytic().end())continue;
        const auto cap=caps.find(id);
        if(entry->second.kind==AnalyticSurface::Kind::Disk && cap==caps.end())continue;
        auto& rule=rules.at(cap==caps.end() ? Primitive(entry->second) : cap->second.first);const auto& g=rule.geometry;
        const auto ids=Faces(d.cell)[slot];std::array<Point,3> p{};
        for(size_t i=0;i<3;++i)for(auto n:d.cell.v)if(n.id==ids[i])p[i]=n.p;
        if(cap!=caps.end()) {
          const auto length=std::hypot(g.axis.x,g.axis.y,g.axis.z);
          const Point tip{p[0].x+g.axis.x/length,p[0].y+g.axis.y/length,p[0].z+g.axis.z/length};
          const auto area=Orientation(p[0],p[1],p[2],tip);const int sign=area>0?1:area<0?-1:0;
          if(!sign)throw std::invalid_argument("Degenerate cylinder end-disk source face.");
          const auto end=cap->second.second;rule.cap_lo[end]=std::min(rule.cap_lo[end],sign);rule.cap_hi[end]=std::max(rule.cap_hi[end],sign);
          continue;
        }
        int direction=0;
        if(g.kind==AnalyticSurface::Kind::Bump) {
          const auto area=Orientation({p[0].x,p[0].y,0},{p[1].x,p[1].y,0},{p[2].x,p[2].y,0},{0,0,1});
          direction=area>0?1:area<0?-1:0;
        } else if(g.kind==AnalyticSurface::Kind::Cylinder) {
          const auto orientation=CylinderFacing(g,p);direction=orientation>0?1:orientation<0?-1:0;
          rule.height=std::min(rule.height,CylinderRadialFloor(g,p));
        } else {
          const auto volume=Orientation(g.origin,p[0],p[1],p[2]);direction=volume>0?1:volume<0?-1:0;
          std::array<long double,3> a{},b{},normal{};
          for(size_t k=0;k<3;++k){const auto x=[](Point p,size_t k){return k==0?p.x:k==1?p.y:p.z;};
            a[k]=static_cast<long double>(x(p[1],k))-x(p[0],k);b[k]=static_cast<long double>(x(p[2],k))-x(p[0],k);}
          normal={a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]};
          const auto length=std::hypot(normal[0],normal[1],normal[2]);
          const auto height=std::abs(volume)/length;
          if(!(height>0&&std::isfinite(height)))throw std::invalid_argument("Unusable radial exterior source face.");
          rule.height=std::min(rule.height,static_cast<double>(height));
        }
        if(!direction)throw std::invalid_argument("Degenerate exterior source chart.");
        rule.lo=std::min(rule.lo,direction);rule.hi=std::max(rule.hi,direction);
      }
    }
  }catch(const std::exception& e){failure.Set(1,0,e.what());}
  if(!Vote(world,failure,reason))return false;
  const auto count_lo=CPassiveComm::Allreduce(uint64_t(rules.size()),CPassiveComm::Op::MIN,world.comm);
  const auto count_hi=CPassiveComm::Allreduce(uint64_t(rules.size()),CPassiveComm::Op::MAX,world.comm);
  if(count_lo!=count_hi)failure.Set(1,0,"Exterior primitive count differs across ranks.");
  if(!Vote(world,failure,reason))return false;
  maximum_node=CPassiveComm::Allreduce(maximum_node,CPassiveComm::Op::MAX,world.comm);
  maximum_cell=CPassiveComm::Allreduce(maximum_cell,CPassiveComm::Op::MAX,world.comm);
  for(auto& row:rules) {
    auto& rule=row.second;const auto& g=rule.geometry;
    for(double value:{double(unsigned(g.kind)),g.origin.x,g.origin.y,g.origin.z,g.extent.x,g.extent.y,g.extent.z,g.tolerance,g.axis.x,g.axis.y,g.axis.z}) {
      const auto low=CPassiveComm::Allreduce(value,CPassiveComm::Op::MIN,world.comm);
      const auto high=CPassiveComm::Allreduce(value,CPassiveComm::Op::MAX,world.comm);
      if(low!=high)failure.Set(1,0,"Exterior primitive parameters differ across ranks.");
    }
    rule.lo=CPassiveComm::Allreduce(rule.lo,CPassiveComm::Op::MIN,world.comm);
    rule.hi=CPassiveComm::Allreduce(rule.hi,CPassiveComm::Op::MAX,world.comm);
    rule.height=CPassiveComm::Allreduce(rule.height,CPassiveComm::Op::MIN,world.comm);
    for(unsigned end=0;end<2;++end) {
      rule.cap_lo[end]=CPassiveComm::Allreduce(rule.cap_lo[end],CPassiveComm::Op::MIN,world.comm);
      rule.cap_hi[end]=CPassiveComm::Allreduce(rule.cap_hi[end],CPassiveComm::Op::MAX,world.comm);
    }
    if(rule.lo==2&&rule.hi==-2)continue; // Authority may include a chart absent from this accepted surface.
    if(rule.lo!=rule.hi)failure.Set(1,0,"Inconsistent exterior source orientation for one analytic primitive.");
    if(g.kind==AnalyticSurface::Kind::Cylinder && rule.cap_lo[0]!=2 && rule.cap_lo[1]!=2) {
      for(unsigned end=0;end<2;++end)if(rule.cap_lo[end]!=rule.cap_hi[end] || rule.cap_lo[end]!=(end?rule.lo:-rule.lo))
        failure.Set(1,0,"Cylinder/end-disk source collar orientations disagree.");
      const auto scale=std::max({1.,std::abs(g.origin.x),std::abs(g.origin.y),std::abs(g.origin.z),g.extent.x,g.extent.y});
      const auto pad=std::max(4*g.tolerance,128*std::numeric_limits<double>::epsilon()*scale);
      if(!(pad>0 && pad<g.extent.y/4))failure.Set(1,0,"Cylinder end-disk source envelope is unrepresentable.");
      rule.pad=rule.lo*pad;
    }
    if(g.kind==AnalyticSurface::Kind::Bump)
      rule.factor=rule.lo*std::nextafter(std::abs(g.extent.z)/16+2*g.tolerance,INFINITY);
    else if(rule.lo>0)
      rule.factor=std::nextafter(double((static_cast<long double>(g.extent.x)+geometry_tolerance+2*g.tolerance)/rule.height),INFINITY);
    else {
      const long double radius=static_cast<long double>(g.extent.x)-geometry_tolerance-2*g.tolerance;
      if(!(radius>0))failure.Set(1,0,"Inward source envelope reaches the sphere center.");
      rule.factor=std::nextafter(double(radius/(static_cast<long double>(g.extent.x)+g.tolerance)),0.);
    }
    if(!std::isfinite(rule.factor) || (g.kind!=AnalyticSurface::Kind::Bump && !(rule.factor>0 && rule.factor!=1)))
      failure.Set(1,0,"Unrepresentable exterior source envelope.");
  }
  // Reserve disjoint transient source identities; accepted CFD identities are never changed.
  const auto node_span=transfer_memory::Add(maximum_node,1),node_end=transfer_memory::Mul(node_span,transfer_memory::Add(rules.size(),1));
  const auto cell_end=transfer_memory::Add(maximum_cell,1,transfer_memory::Mul(12,transfer_memory::Add(maximum_cell,1)));
  if(node_end==SIZE_MAX || cell_end==SIZE_MAX)failure.Set(1,0,"Exterior source identity range exhausted.");
  if(!Vote(world,failure,reason))return false;
  const size_t count=source.size();
  try {
    source.reserve(transfer_memory::Add(count,added));
    for(size_t at=0;at<count;++at) {
      const auto original=source[at];const auto& owned=cells.at(original.cell.id);
      for(size_t slot=0;slot<4;++slot)if(owned.physical[slot].present) {
        const Id component=owned.physical[slot].facet;
        const auto entry=reference.Analytic().find(component);if(entry==reference.Analytic().end())continue;
        const auto cap=caps.find(component);
        if(entry->second.kind==AnalyticSurface::Kind::Disk && cap==caps.end())continue;
        const auto key=cap==caps.end() ? Primitive(entry->second) : cap->second.first;
        const auto found=rules.find(key);const auto& rule=found->second;const auto& g=rule.geometry;
        if(cap!=caps.end() && !rule.pad)continue;
        const size_t index=std::distance(rules.begin(),found);
        const auto ids=Faces(original.cell)[slot];std::array<std::pair<Node,Tensor>,3> old;
        for(size_t i=0;i<3;++i)for(size_t j=0;j<4;++j)if(original.cell.v[j].id==ids[i])old[i]={original.cell.v[j],original.sensor[j]};
        std::sort(old.begin(),old.end(),[](const auto& a,const auto& b){return a.first.id<b.first.id;});
        auto next=old;
        for(auto& point:next) {
          auto& n=point.first;n.id=node_span*(index+1)+n.id;
          if(g.kind==AnalyticSurface::Kind::Bump)n.p.z=std::nextafter(n.p.z+rule.factor,rule.factor>0?INFINITY:-INFINITY);
          else if(g.kind==AnalyticSurface::Kind::Cylinder) {
            const auto original_point=n.p;n.p=ScaleCylinder(g,n.p,rule.factor);
            if(rule.pad) {
              const long double length=std::hypot(g.axis.x,g.axis.y,g.axis.z);
              const std::array<long double,3> a{{g.axis.x/length,g.axis.y/length,g.axis.z/length}};
              const long double t=(static_cast<long double>(original_point.x)-g.origin.x)*a[0]+
                  (static_cast<long double>(original_point.y)-g.origin.y)*a[1]+(static_cast<long double>(original_point.z)-g.origin.z)*a[2];
              const auto shift=rule.pad*(2*t/g.extent.y-1);
              n.p={double(n.p.x+shift*a[0]),double(n.p.y+shift*a[1]),double(n.p.z+shift*a[2])};
            }
          }
          else n.p={double(g.origin.x+(static_cast<long double>(n.p.x)-g.origin.x)*rule.factor),
                    double(g.origin.y+(static_cast<long double>(n.p.y)-g.origin.y)*rule.factor),
                    double(g.origin.z+(static_cast<long double>(n.p.z)-g.origin.z)*rule.factor)};
        }
        const std::array<std::array<std::pair<Node,Tensor>,4>,3> tets{{
          {old[0],old[1],old[2],next[2]},{old[0],old[1],next[1],next[2]},{old[0],next[0],next[1],next[2]}}};
        for(size_t i=0;i<3;++i) {
          auto v=tets[i];const auto sign=Orientation(v[0].first.p,v[1].first.p,v[2].first.p,v[3].first.p);
          if(!sign)throw std::invalid_argument("Unrepresentable exterior source tetrahedron.");
          if(sign<0)std::swap(v[0],v[1]);
          DonorCell donor;donor.cell.id=maximum_cell+1+12*original.cell.id+3*slot+i;donor.extension_component=component;donor.parent_cell=original.cell.id;
          for(size_t j=0;j<4;++j){donor.cell.v[j]=v[j].first;donor.sensor[j]=v[j].second;}
          source.push_back(donor);
        }
      }
    }
  }catch(const std::exception& e){failure.Set(1,0,e.what());}
  return Vote(world,failure,reason);
}
}  // namespace
bool ImportGeometry(const CConfig& config, const CGeometry& geometry, const su2activematrix* metric,
                    ReferenceState& state, ImportedGeometry& output, std::string& reason, BridgeStats& stats,
                    size_t ceiling, size_t caller_bytes) {
  World parent;
  ImportedGeometry result;
  ReferenceState pending;
  {
    Envelope envelope(parent, stats);
    CLocalFailure failure;
    if (geometry.GetnDim() != 3 || !std::is_same<su2double, double>::value ||
        !std::is_same<passivedouble, double>::value)
      failure.Set(1, 0, "Native tetrahedral import requires a primal double-precision 3D geometry.");
    if (metric && (metric->cols() != 6 || metric->rows() < geometry.GetnPointDomain()))
      failure.Set(1, 0, "Native tetrahedral import needs six sensor components at every CFD-owned point.");
    CollectiveFailure(failure, CURRENT_FUNCTION);
    const size_t np = geometry.GetnPoint(), owned_points = geometry.GetnPointDomain(), ne = geometry.GetnElem();
    size_t nf = 0, tag_chars = 0;
    for (unsigned short m = 0; m < geometry.GetnMarker(); ++m) {
      nf = transfer_memory::Add(nf, geometry.GetnElem_Bound(m));
      tag_chars = transfer_memory::Add(tag_chars, config.GetMarker_All_TagBound(m).size() + 1);
    }
    const size_t global_tag_chars = CPassiveComm::Allreduce(uint64_t(tag_chars), CPassiveComm::Op::SUM, parent.comm);
    // OwnedSimplices/OwnedBoundaryFaces grow from empty; marker gather retains local and replicated text.
    const size_t helper_bound = transfer_memory::Add(
        caller_bytes, ReferenceStorageBytes(state), OwnedBytes(output.cells), transfer_memory::Bytes(output.original),
        transfer_memory::GrowthBound(ne, 5 * sizeof(unsigned long) + sizeof(CSimplexKey)),
        transfer_memory::GrowthBound(nf, 3 * sizeof(unsigned long) + sizeof(CSimplexKey) + sizeof(uint32_t)),
        transfer_memory::Mul(8, transfer_memory::Add(tag_chars, global_tag_chars)),
        transfer_memory::GrowthBound(config.GetnMarker_CfgFile() + geometry.GetnMarker(), sizeof(std::string)),
        transfer_memory::TransportBytes(parent.size), transfer_memory::Mul(parent.size, 16 * sizeof(uint64_t)));
    if (!Admit(parent, helper_bound, ceiling, stats, reason)) return false;
    const auto simplices = OwnedSimplices(geometry);
    std::vector<std::string> tags;
    for (unsigned short m = 0; m < geometry.GetnMarker(); ++m) tags.push_back(config.GetMarker_All_TagBound(m));
    const auto names = CMeshGather::GatherMarkerNames(config, tags, geometry);
    const auto physical = OwnedBoundaryFaces(geometry, tags, config);
    const double point_start = parent.seconds();
    const uint64_t global_points = CPassiveComm::Allreduce(uint64_t(owned_points), CPassiveComm::Op::SUM, parent.comm);
    const uint64_t block = std::max<uint64_t>(1, transfer_memory::CeilDiv(global_points, parent.size));
    const uint64_t first = std::min<uint64_t>(global_points, uint64_t(parent.rank) * block),
                   last = std::min<uint64_t>(global_points, uint64_t(parent.rank + 1) * block);
    std::vector<uint64_t> build_to(parent.size), fetch_to(parent.size), build_from(parent.size),
        fetch_from(parent.size);
    for (unsigned long p = 0; p < np; ++p) {
      const auto id = geometry.nodes->GetGlobalIndex(p);
      if (id >= global_points) failure.Set(1, id, "Native3D point indices are outside the original owner numbering.");
      const int owner = std::min<uint64_t>(parent.size - 1, id / block);
      if (p < owned_points) ++build_to[owner];
      ++fetch_to[owner];
    }
    CollectiveFailure(failure, CURRENT_FUNCTION);
    // Counts alone bound the house directory before it allocates payloads; halo requests may repeat across ranks.
    CPassiveComm::Alltoall(build_to.data(), build_from.data(), sizeof(uint64_t), parent.comm);
    CPassiveComm::Alltoall(fetch_to.data(), fetch_from.data(), sizeof(uint64_t), parent.comm);
    size_t build_in = 0, fetch_in = 0;
    for (int q = 0; q < parent.size; ++q) {
      build_in = transfer_memory::Add(build_in, build_from[q]);
      fetch_in = transfer_memory::Add(fetch_in, fetch_from[q]);
    }
    const size_t point_resident = transfer_memory::Add(helper_bound, transfer_memory::Mul(np, sizeof(uint64_t)),
                                                       transfer_memory::Mul(owned_points, 72));
    const size_t directory_build = transfer_memory::Add(
        point_resident, transfer_memory::Mul(owned_points, sizeof(int) + 80), transfer_memory::Mul(build_in, 80),
        transfer_memory::Mul(last - first, 73),
        transfer_memory::Staging(CPassiveComm::GetRoundBytes(), transfer_memory::Mul(owned_points, 80),
                                 transfer_memory::Mul(build_in, 80)));
    const size_t directory_fetch =
        transfer_memory::Add(point_resident, transfer_memory::Mul(last - first, 72),
                             transfer_memory::Mul(np, 8 + 72 + 72), transfer_memory::Mul(fetch_in, 8 + 72),
                             transfer_memory::Staging(CPassiveComm::GetRoundBytes(), transfer_memory::Mul(np, 8),
                                                      transfer_memory::Mul(fetch_in, 8)),
                             transfer_memory::Staging(CPassiveComm::GetRoundBytes(), transfer_memory::Mul(fetch_in, 72),
                                                      transfer_memory::Mul(np, 72)));
    if (!Admit(parent, std::max(directory_build, directory_fetch), ceiling, stats, reason)) return false;
    const size_t native_cells =
        transfer_memory::Mul(simplices.size(), sizeof(Owned::value_type) + 4 * sizeof(void*) + sizeof(DonorCell));
    const size_t point_and_cells = transfer_memory::Add(point_resident, transfer_memory::Mul(last - first, 72),
                                                        transfer_memory::Mul(np, 72), native_cells);
    if (!Admit(parent, point_and_cells, ceiling, stats, reason)) return false;
    {
      CPointDirectory points;
      std::vector<uint64_t> gids;
      std::vector<char> bytes;
      gids.reserve(np);
      bytes.reserve(transfer_memory::Mul(owned_points, 72));
      for (unsigned long p = 0; p < geometry.GetnPointDomain(); ++p) {
        gids.push_back(geometry.nodes->GetGlobalIndex(p));
        std::array<double, 9> values{};
        for (size_t d = 0; d < 3; ++d) values[d] = SU2_TYPE::GetValue(geometry.nodes->GetCoord(p, d));
        for (size_t k = 0; k < 6; ++k)
          values[k + 3] = metric ? SU2_TYPE::GetValue((*metric)(p, k)) : (k == 0 || k == 3 || k == 5 ? 1. : 0.);
        try {
          for (size_t d = 0; d < 3; ++d)
            if (!std::isfinite(values[d])) throw std::invalid_argument("Nonfinite CFD-owned native3D coordinate.");
          ValidateTensor({values[3], values[4], values[5], values[6], values[7], values[8]});
          RecordStream encode(bytes);
          encode(values);
        } catch (const std::exception& e) {
          failure.Set(1, gids.back(), e.what());
        }
      }
      CollectiveFailure(failure, CURRENT_FUNCTION);
      points.Build(gids, bytes, 72, "native tetrahedral original sensor points");
      gids.clear();
      for (unsigned long p = 0; p < geometry.GetnPoint(); ++p) gids.push_back(geometry.nodes->GetGlobalIndex(p));
      const auto fetched = points.Fetch(gids);
      stats.points_seconds = parent.seconds() - point_start;
      const double cell_start = parent.seconds();
      const auto prefix = CPassiveComm::ExscanSum(simplices.size());
      try {
        result.original.reserve(simplices.size());
        for (unsigned long e = 0; e < simplices.size(); ++e) {
          DonorCell donor;
          donor.cell.id = prefix + e;
          for (size_t k = 0; k < 4; ++k) {
            const auto p = simplices.nodes[4 * e + k];
            const char* data = fetched.data() + 72 * p;
            const auto x = GetBytes<double>(data), y = GetBytes<double>(data), z = GetBytes<double>(data);
            donor.cell.v[k] = {gids[p], {x, y, z}};
            donor.sensor[k] = {GetBytes<double>(data), GetBytes<double>(data), GetBytes<double>(data),
                               GetBytes<double>(data), GetBytes<double>(data), GetBytes<double>(data)};
            if (x != SU2_TYPE::GetValue(geometry.nodes->GetCoord(p, 0)) ||
                y != SU2_TYPE::GetValue(geometry.nodes->GetCoord(p, 1)) ||
                z != SU2_TYPE::GetValue(geometry.nodes->GetCoord(p, 2)))
              throw std::invalid_argument("Inconsistent native3D CFD halo coordinates.");
          }
          const auto& v = donor.cell.v;
          const auto sign = Orientation(v[0].p, v[1].p, v[2].p, v[3].p);
          if (!sign) throw std::invalid_argument("Degenerate native3D CFD tetrahedron.");
          if (sign < 0) {
            std::swap(donor.cell.v[0], donor.cell.v[1]);
            std::swap(donor.sensor[0], donor.sensor[1]);
          }
          result.cells.emplace(donor.cell.id, OwnedCell{donor.cell});
          result.original.push_back(donor);
        }
      } catch (const std::exception& e) {
        failure.Set(2, 0, e.what());
      }
      CollectiveFailure(failure, CURRENT_FUNCTION);
      stats.cells_seconds = parent.seconds() - cell_start;
    }  // Release point payloads/directory before constructing the face rendezvous.
    const double face_start = parent.seconds();
    const size_t face_count = transfer_memory::Add(physical.size(), transfer_memory::Mul(result.cells.size(), 4));
    const size_t face_base =
        transfer_memory::Add(helper_bound, OwnedBytes(result.cells), transfer_memory::Bytes(result.original));
    if (!Admit(parent,
               transfer_memory::Add(face_base, transfer_memory::GrowthBound(face_count, sizeof(FaceRecord)),
                                    transfer_memory::Mul(parent.size, sizeof(std::vector<FaceRecord>))),
               ceiling, stats, reason))
      return false;
    std::vector<std::vector<FaceRecord>> to(parent.size);
    for (unsigned long i = 0; i < physical.size(); ++i) {
      Face f{};
      for (size_t k = 0; k < 3; ++k) f[k] = geometry.nodes->GetGlobalIndex(physical.nodes[3 * i + k]);
      const auto name = config.GetMarker_CfgFile_TagBound(physical.markerId[i]);
      const auto found = std::find(names.begin(), names.end(), name);
      if (found == names.end()) failure.Set(1, f[0], "Native3D physical marker is missing from the global name list.");
      to[Bucket(f, parent.size)].push_back({Key(f), 0, parent.rank, 0, int(found - names.begin()), 0, true});
    }
    for (const auto& r : result.cells) {
      const auto faces = Faces(r.second.cell);
      for (size_t k = 0; k < 4; ++k)
        to[Bucket(faces[k], parent.size)].push_back(
            {Key(faces[k]), r.first, parent.rank, int(k), -1, Parity(faces[k]), false});
    }
    CollectiveFailure(failure, CURRENT_FUNCTION);
    bool admitted = false;
    auto records = parent.exchange(to, &admitted, ceiling, face_base);
    stats.maximum_native_bytes = std::max(stats.maximum_native_bytes, parent.max_exchange_work_bytes);
    if (!admitted) {
      reason = "Native3D face rendezvous exceeds storage budget.";
      return false;
    }
    std::sort(records.begin(), records.end(), [](const auto& a, const auto& b) {
      return std::tie(a.face, a.label, a.owner, a.cell) < std::tie(b.face, b.label, b.owner, b.cell);
    });
    const size_t reply_base =
        transfer_memory::Add(face_base, transfer_memory::Bytes(to), transfer_memory::Bytes(records));
    if (!Admit(parent,
               transfer_memory::Add(reply_base, transfer_memory::GrowthBound(records.size(), sizeof(Binding)),
                                    transfer_memory::Mul(parent.size, sizeof(std::vector<Binding>))),
               ceiling, stats, reason))
      return false;
    std::vector<std::vector<Binding>> reply(parent.size);
    for (size_t first = 0; first < records.size();) {
      size_t last = first + 1;
      while (last < records.size() && records[last].face == records[first].face) ++last;
      size_t volumes = 0, labels = 0;
      int marker = -1, orientation = 0;
      for (size_t i = first; i < last; ++i)
        if (records[i].label) {
          ++labels;
          marker = records[i].marker;
        } else {
          ++volumes;
          orientation += records[i].parity;
        }
      if ((volumes == 1 && labels == 1) || (volumes == 2 && labels == 0 && orientation == 0)) {
        if (labels)
          for (size_t i = first; i < last; ++i)
            if (!records[i].label) reply[records[i].owner].push_back({records[i].cell, records[i].slot, marker});
      } else
        failure.Set(1, records[first].face[0],
                    "Native3D face/physical-label incidence is not an oriented conforming manifold.");
      first = last;
    }
    CollectiveFailure(failure, CURRENT_FUNCTION);
    const auto bindings = parent.exchange(reply, &admitted, ceiling, reply_base);
    stats.maximum_native_bytes = std::max(stats.maximum_native_bytes, parent.max_exchange_work_bytes);
    if (!admitted) {
      reason = "Native3D physical binding exchange exceeds storage budget.";
      return false;
    }
    const size_t reference_base =
        transfer_memory::Add(reply_base, transfer_memory::Bytes(reply), transfer_memory::Bytes(bindings));
    if (!state.original) {
      const auto count = CPassiveComm::Allreduce(uint64_t(bindings.size()), CPassiveComm::Op::SUM, parent.comm);
      if (!count || transfer_memory::Mul(count, sizeof(Facet)) == SIZE_MAX)
        failure.Set(1, 0, "Native3D immutable facet authority is empty or exceeds addressable storage.");
      CollectiveFailure(failure, CURRENT_FUNCTION);
      // Admit reference maps/index/component scratch and replicated wire copies before construction.
      const size_t reference_scratch = PlanarReference::ConstructionBytes(count);
      if (!Admit(parent,
                 transfer_memory::Add(reference_base, reference_scratch,
                                      transfer_memory::Add(transfer_memory::Mul(count, 3 * sizeof(Facet)),
                                                           transfer_memory::TransportBytes(parent.size))),
                 ceiling, stats, reason))
        return false;
      std::vector<Facet> local;
      const auto offset = CPassiveComm::ExscanSum(bindings.size());
      for (size_t i = 0; i < bindings.size(); ++i) {
        const auto b = bindings[i];
        const auto& cell = result.cells.at(b.cell).cell;
        const auto face = Faces(cell)[b.slot];
        Facet f;
        f.id = offset + i;
        f.marker = b.marker;
        for (size_t k = 0; k < 3; ++k)
          for (auto n : cell.v)
            if (n.id == face[k]) f.v[k] = n;
        local.push_back(f);
      }
      pending.facets = CPassiveComm::Allgatherv(local, nullptr, parent.comm);
      pending.marker_names = names;
      try {
        std::vector<Face> skin;
        for (auto f : pending.facets) skin.push_back({f.v[0].id, f.v[1].id, f.v[2].id});
        ValidateDomainSurfaceTopology(skin);
        pending.original = std::make_shared<PlanarReference>(pending.facets, reference_scratch);
      } catch (const std::exception& e) {
        failure.Set(1, 0, e.what());
      }
    }
    const auto& authority = state.original ? state : pending;
    if (authority.marker_names != names) failure.Set(1, 0, "Native3D immutable physical marker names changed.");
    CollectiveFailure(failure, CURRENT_FUNCTION);
    if (!Admit(parent, transfer_memory::Add(reference_base,
                   state.original ? 0 : ReferenceStorageBytes(pending), authority.original->SearchWorkspaceBytes()),
               ceiling, stats, reason)) return false;
    try {
      for (auto b : bindings) {
        auto& cell = result.cells.at(b.cell);
        const auto face = Faces(cell.cell)[b.slot];
        std::array<Point, 3> p{};
        for (size_t k = 0; k < 3; ++k)
          for (auto n : cell.cell.v)
            if (n.id == face[k]) p[k] = n.p;
        Id chosen = 0;
        if (!authority.original->FindFace(b.marker, p, chosen))
          throw std::invalid_argument("Native3D physical triangle leaves its immutable oriented marker facets.");
        cell.physical[b.slot] = {true, chosen};
      }
    } catch (const std::exception& e) {
      failure.Set(1, 0, e.what());
    }
    CollectiveFailure(failure, CURRENT_FUNCTION);
    if(!Coverage(parent,result.cells,result.original,*authority.original,SU2_TYPE::GetValue(config.GetAdap_Hausd()),
                 transfer_memory::Add(reference_base,state.original ? 0 : ReferenceStorageBytes(pending)),ceiling,stats,reason))
      return false;
    stats.faces_seconds = parent.seconds() - face_start;
  }
  static_assert(std::is_nothrow_move_assignable<ImportedGeometry>::value,
                "Native3D import publication must not allocate.");
  result.stats = stats;
  output = std::move(result);
  if (!state.original) std::swap(state, pending);
  reason.clear();
  return true;
}
ImportedGeometry ImportGeometry(const CConfig& config, const CGeometry& geometry, const su2activematrix* metric,
                                ReferenceState& state) {
  ImportedGeometry result;
  BridgeStats stats;
  std::string reason;
  if (!ImportGeometry(config, geometry, metric, state, result, reason, stats, SIZE_MAX))
    SU2_MPI::Error(reason, CURRENT_FUNCTION);
  return result;
}
namespace {
/*! Shared collective batch schedule. Only the original sensor is imported; composition remains query-local. */
template <class Make, class Use>
bool Samples(World& parent, size_t count, DonorImport& source, MetricComposition composition, Make make, Use use,
             BridgeStats& stats, std::string& reason, size_t ceiling, size_t live, size_t initial, bool query_timings, const PlanarReference* reference=nullptr) {
  // shortcut: conservative bounded field/cache scratch; calibrate against the actual index/cache allocations.
  const size_t scratch =
      transfer_memory::Add(MaximumFrozenFieldBytes,
                           transfer_memory::Mul(MaximumSensorSamples, 32 * sizeof(double)),
                           transfer_memory::Mul(MaximumCavityCells, sizeof(DonorRegion)));
  live = transfer_memory::Add(live, scratch);
  if (!Admit(parent, live, ceiling, stats, reason)) return false;
  size_t next = 0, batch = initial;
  while (parent.maximum(next < count)) {
    const double start = parent.seconds();
    CLocalFailure failure;
    std::vector<DonorRegion> regions;
    try {
      for (size_t i = next; i < std::min(count, next + batch); ++i) make(i, regions);
    } catch (const std::exception& e) {
      failure.Set(2, 0, e.what());
    }
    if (!Vote(parent, failure, reason)) {
      stats.query_seconds += parent.seconds() - start;
      return false;
    }
    auto imported = source.Import(regions, ceiling, live);
    ++stats.batches;
    if (!imported.valid) {
      reason = imported.reason;
      stats.query_seconds += parent.seconds() - start;
      if (batch == 1) return false;
      batch = std::max(size_t(1), batch / 2);
      ++stats.batch_retries;
      continue;
    }
    try {
      if (!regions.empty()) {
        const auto components=reference ? AssociatedExtensions(imported.cells,*reference) : std::vector<Id>{};
        FrozenField field(std::move(imported.cells), composition, query_timings,components);
        FieldAccounting accounting{stats,field};
        ++stats.field_builds;
        for (size_t i = next; i < std::min(count, next + batch); ++i) use(i, field);
      }
    } catch (const std::exception& e) {
      failure.Set(2, 0, e.what());
    }
    if (!Vote(parent, failure, reason)) {
      stats.query_seconds += parent.seconds() - start;
      return false;
    }
    next += std::min(batch, count - next);
    stats.query_seconds += parent.seconds() - start;
  }
  stats.maximum_native_bytes = std::max(stats.maximum_native_bytes, source.Statistics().maximum_work_bytes);
  return true;
}
void PointRegion(Point p, std::vector<DonorRegion>& regions) { regions.push_back({p.x, p.y, p.z, p.x, p.y, p.z}); }
}  // namespace
bool WorkingWeights(World& parent, const Owned& cells, DonorImport& source, std::vector<uint32_t>& output,
                    std::string& reason, BridgeStats& stats, MetricComposition composition, size_t ceiling,
                    size_t caller_bytes, bool query_timings) {
  Envelope envelope(parent, stats);
  const size_t live = transfer_memory::Add(caller_bytes, OwnedBytes(cells), source.ResidentBytes(),
                                           transfer_memory::Mul(cells.size(), sizeof(const Cell*) + sizeof(uint32_t)));
  if (!Admit(parent, live, ceiling, stats, reason)) return false;
  std::vector<const Cell*> ordered;
  std::vector<uint32_t> weights;
  CLocalFailure failure;
  try {
    weights.resize(cells.size());
    ordered.reserve(cells.size());
    for (auto& r : cells) ordered.push_back(&r.second.cell);
  } catch (const std::exception& e) {
    failure.Set(2, 0, e.what());
  }
  if (!Vote(parent, failure, reason)) return false;
  auto make = [&](size_t i, std::vector<DonorRegion>& regions) {
    const auto& cell = *ordered[i];
    PointRegion(Centroid(cell), regions);
    for (auto n : cell.v) PointRegion(n.p, regions);
  };
  auto use = [&](size_t i, FrozenField& field) { weights[i] = WorkWeights({*ordered[i]}, field).front(); };
  if (!Samples(parent, cells.size(), source, std::move(composition), make, use, stats, reason, ceiling, live, 8,
               query_timings))
    return false;
  output.swap(weights);
  reason.clear();
  return true;
}
bool ReaderSlices(World& parent, const Owned& cells, const std::vector<DonorCell>& original,
                  const ReferenceState& state, CReaderSlices& output, std::string& reason, BridgeStats& stats,
                  MetricComposition composition, size_t ceiling, size_t caller_bytes, bool query_timings) {
  Envelope envelope(parent, stats);
  CLocalFailure failure;
#ifdef HAVE_MPI
  int relation = MPI_UNEQUAL;
  MPI_Comm_compare(parent.comm, SU2_MPI::GetComm(), &relation);
  if (relation != MPI_IDENT) failure.Set(1, 0, "Native3D reader return requires the actual CFD communicator.");
#endif
  if (!state.original || state.marker_names.empty()) failure.Set(1, 0, "Uninitialized native3D reader reference.");
  if (!Vote(parent, failure, reason)) return false;
  using transfer_memory::Add;
  using transfer_memory::Bytes;
  using transfer_memory::GrowthBound;
  using transfer_memory::Mul;
  const auto vertices = Mul(cells.size(), 4);
  size_t physical_faces = 0, name_bytes = 0;
  for (const auto& row : cells)
    for (const auto& face : row.second.physical) physical_faces += face.present;
  for (const auto& name : state.marker_names) name_bytes = Add(name_bytes, name.size(), 1);
  const size_t live = Add(caller_bytes, OwnedBytes(cells), Bytes(original), output.ResidentBytes(),
                         Mul(vertices, sizeof(std::pair<const Id, Node>) + sizeof(std::pair<const Id, unsigned long>) +
                                           8 * sizeof(void*)),
                         GrowthBound(vertices, sizeof(Node) + sizeof(uint64_t) + 3 * sizeof(passivedouble)),
                         Mul(vertices, 6 * sizeof(passivedouble)),
                         GrowthBound(cells.size(), 4 * sizeof(unsigned long) + sizeof(int)),
                         GrowthBound(physical_faces, 3 * sizeof(unsigned long)),
                         GrowthBound(state.marker_names.size(), sizeof(CSimplexMesh::Marker)), name_bytes);
  if (!Admit(parent, live, ceiling, stats, reason)) return false;
  DonorImport source(parent, query_timings);
  const double build = parent.seconds();
  if (!source.Build(original, reason, ceiling, live)) {
    stats.source_seconds = parent.seconds() - build;
    return false;
  }
  stats.source_seconds = parent.seconds() - build;
  std::map<Id, Node> nodes;
  std::vector<Node> ordered;
  CSimplexMesh mesh;
  mesh.nDim = 3;
  std::vector<uint64_t> keys;
  std::map<Id, unsigned long> index;
  try {
    for (const auto& r : cells)
      for (auto n : r.second.cell.v) {
        const auto old = nodes.emplace(n.id, n);
        if (!old.second &&
            (old.first->second.p.x != n.p.x || old.first->second.p.y != n.p.y || old.first->second.p.z != n.p.z))
          throw std::invalid_argument("Inconsistent native3D output coordinates.");
      }
    for (auto r : nodes) {
      index.emplace(r.first, ordered.size());
      ordered.push_back(r.second);
      keys.push_back(r.first);
      mesh.coord.insert(mesh.coord.end(), {r.second.p.x, r.second.p.y, r.second.p.z});
    }
    mesh.metric.resize(6 * ordered.size());
    for (size_t m = 0; m < state.marker_names.size(); ++m)
      mesh.markers.push_back({state.marker_names[m], int(m + 1), {}});
    for (const auto& r : cells) {
      const auto& c = r.second.cell;
      for (auto n : c.v) mesh.elem.push_back(index.at(n.id));
      mesh.elemRef.push_back(1);
      const auto faces = Faces(c);
      for (size_t k = 0; k < 4; ++k)
        if (r.second.physical[k].present) {
          const auto marker = state.original->Get(r.second.physical[k].facet).marker;
          for (auto id : faces[k]) mesh.markers.at(marker).elem.push_back(index.at(id));
        }
    }
  } catch (const std::exception& e) {
    failure.Set(2, 0, e.what());
  }
  if (!Vote(parent, failure, reason)) return false;
  auto make = [&](size_t i, std::vector<DonorRegion>& regions) { PointRegion(ordered[i].p, regions); };
  auto use = [&](size_t i, FrozenField& field) {
    const auto m = field.Query(ordered[i].p).target;
    const std::array<double, 6> values{m.xx, m.xy, m.xz, m.yy, m.yz, m.zz};
    std::copy(values.begin(), values.end(), mesh.metric.begin() + 6 * i);
  };
  if (!Samples(parent, ordered.size(), source, std::move(composition), make, use, stats, reason, ceiling,
               transfer_memory::Add(live, source.ResidentBytes()), 16, query_timings,state.original.get()))
    return false;
  const double reader_start = parent.seconds();
  CReaderSliceStats reader_stats;
  const size_t reader_caller = Add(caller_bytes, OwnedBytes(cells), Bytes(original), source.ResidentBytes(),
                                  Mul(nodes.size(), sizeof(decltype(nodes)::value_type) + 4 * sizeof(void*)),
                                  Mul(index.size(), sizeof(decltype(index)::value_type) + 4 * sizeof(void*)), Bytes(ordered));
  const bool accepted = CReaderSlices::FromDistributed(mesh, keys, output, reason, reader_stats, ceiling, reader_caller);
  stats.reader_seconds = parent.seconds() - reader_start;
  stats.maximum_native_bytes = std::max(stats.maximum_native_bytes, reader_stats.maximum_bytes);
  if (!accepted) return false;
  reason.clear();
  return true;
}
}  // namespace SU2Native3D
