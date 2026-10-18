/*!
 * \file CNativeDonor3D.cpp
 * \brief Indexed original sensor imports without composed-tensor transport or global candidate scans.
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md).
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */
#include "../../include/adaptation/CNativeDonor3D.hpp"
#include "../../include/adt/CADTElemClass.hpp"
#include "../../include/option_structure.hpp"
#include <chrono>
#include <cmath>
#include <limits>
#include <stdexcept>
namespace SU2Native3D {
namespace {
class Timer {
 public:
  Timer(double& value, bool enabled) : value(value), enabled(enabled) {
    if (enabled) start = std::chrono::steady_clock::now();
  }
  ~Timer() { if (enabled) value += std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count(); }
 private:
  double& value;
  bool enabled;
  std::chrono::steady_clock::time_point start;
};
std::array<double, 6> Components(Tensor m) { return {m.xx, m.xy, m.xz, m.yy, m.yz, m.zz}; }
struct Authority {
  unsigned kind = 0;  // 0 shared node, 1 cell ID, 2 canonical simplex.
  std::array<Id, 4> key{};
  Point point;
  Tensor sensor;
  template <class S> void Fields(S& s) { s(kind, key, point, sensor); }
};
int Server(const Authority& row, int size) {
  uint64_t hash = row.kind;
  for (auto id : row.key) hash ^= id + uint64_t(0x9e3779b97f4a7c15ULL) + (hash << 6) + (hash >> 2);
  return hash % size;
}
struct Request {
  DonorRegion region{};
  int caller = 0;
  bool tetrahedral = false, segment = false, positive_length = false;
  unsigned char query = 0;  // 0 full cell; bits0..6 select actual centroid and six closed edges.
  Tetrahedron tetra{};
  template <class S> void Fields(S& s) { s(region, caller, tetrahedral, segment, positive_length, query, tetra); }
};
struct OwnedDonor {
  DonorCell donor;int rank=0;
  template<class S>void Fields(S& s){s(donor,rank);}
};
struct PointRequest {
  int caller=0;unsigned kind=0;size_t ordinal=0;Point point;Id component=UINT64_MAX;
  Point end{};bool midpoint=false;
  template<class S>void Fields(S& s){s(caller,kind,ordinal,point,component,end,midpoint);}
};
struct SampleSupportRequest {
  int caller=0;std::array<Id,4> key{};
  template<class S>void Fields(S& s){s(caller,key);}
};
struct PointReply {
  size_t ordinal=0;DonorSample sample;
  template<class S>void Fields(S& s){s(ordinal,sample);}
};
constexpr std::array<std::array<size_t, 2>, 6> QueryEdges{{{0,1},{0,2},{0,3},{1,2},{1,3},{2,3}}};
DonorRegion Bounds(const Cell& cell) {
  const auto p = cell.v[0].p;
  DonorRegion result{p.x, p.y, p.z, p.x, p.y, p.z};
  for (const auto& n : cell.v) {
    const double v[3]{n.p.x, n.p.y, n.p.z};
    for (size_t a = 0; a < 3; ++a) {
      result[a] = std::min(result[a], v[a]); result[a + 3] = std::max(result[a + 3], v[a]);
    }
  }
  return result;
}
DonorRegion SegmentBounds(Point a, Point b) {
  return {std::min(a.x,b.x),std::min(a.y,b.y),std::min(a.z,b.z),
          std::max(a.x,b.x),std::max(a.y,b.y),std::max(a.z,b.z)};
}
DonorRegion PointBounds(const PointRequest& query) {
  if(!query.midpoint)return SegmentBounds(query.point,query.point);
  const Point p{double(((long double)query.point.x+query.end.x)/2),double(((long double)query.point.y+query.end.y)/2),double(((long double)query.point.z+query.end.z)/2)};
  if(!std::isfinite(p.x) || !std::isfinite(p.y) || !std::isfinite(p.z))throw std::invalid_argument("Unrepresentable original midpoint coordinate.");
  auto lower=[](double x){const auto y=std::nextafter(x,-INFINITY);return std::isfinite(y)?y:x;};
  auto upper=[](double x){const auto y=std::nextafter(x,INFINITY);return std::isfinite(y)?y:x;};
  return {lower(p.x),lower(p.y),lower(p.z),upper(p.x),upper(p.y),upper(p.z)};
}
bool SegmentMayIntersectBox(Point from, Point to, const double* box) {
  // Binary64 differences/products stay normal and finite only with this wider exponent range.
  if constexpr (std::numeric_limits<long double>::digits < std::numeric_limits<double>::digits ||
                std::numeric_limits<long double>::max_exponent < 2*std::numeric_limits<double>::max_exponent+4 ||
                std::numeric_limits<long double>::min_exponent >
                    2*(std::numeric_limits<double>::min_exponent-std::numeric_limits<double>::digits)-4) return true;
  const std::array<double,3> a{from.x,from.y,from.z}, b{to.x,to.y,to.z};
  if (a==b) return true;  // The existing point box is already authoritative for this broad phase.
  auto side = [&](double x, double y, size_t i, size_t j) {
    // Preserve rounding points under solver fast-math; 64eps bounds these seven normal operations.
    volatile long double u=static_cast<long double>(b[i])-a[i], v=static_cast<long double>(y)-a[j];
    volatile long double w=static_cast<long double>(b[j])-a[j], z=static_cast<long double>(x)-a[i];
    volatile long double p=u*v, q=w*z;
    const auto determinant=p-q, permanent=std::abs(p)+std::abs(q);
    const auto epsilon=std::numeric_limits<long double>::epsilon();
    if (std::isfinite(determinant) && std::isfinite(permanent) &&
        permanent>std::numeric_limits<long double>::min()/epsilon && std::abs(determinant)>64*epsilon*permanent)
      return determinant>0 ? 1 : -1;
    return 0;
  };
  for (size_t i=0;i<3;++i) for (size_t j=i+1;j<3;++j) {
    volatile long double di=static_cast<long double>(b[i])-a[i], dj=static_cast<long double>(b[j])-a[j];
    if (!std::isfinite(di) || !std::isfinite(dj)) return true;
    const auto xmin=box[i+(dj<=0 ? 0 : 3)], ymin=box[j+(di>=0 ? 0 : 3)];
    const auto xmax=box[i+(dj<=0 ? 3 : 0)], ymax=box[j+(di>=0 ? 3 : 0)];
    if (side(xmin,ymin,i,j)>0 || side(xmax,ymax,i,j)<0) return false;
  }
  return true;  // Touching/unresolved projections still require the exact tetrahedron predicate.
}
bool Intersects(const DonorRegion& region, const double* box) {
  for (size_t a = 0; a < 3; ++a) if (region[a] > box[a + 3] || region[a + 3] < box[a]) return false;
  return true;
}
bool Outside(const Tetrahedron& a, const Tetrahedron& b, KernelStats* stats) {
  for (size_t i = 0; i < 4; ++i) {
    bool separated = true;
    auto trial = a;
    for (auto p : b) {
      trial[i] = p;
      if (Orientation(trial[0], trial[1], trial[2], trial[3], stats) >= 0) { separated = false; break; }
    }
    if (separated) return true;
  }
  return false;
}
size_t MapBytes(size_t count, size_t value) { return transfer_memory::Mul(count, value + 4 * sizeof(void*)); }
}  // namespace
DonorImport::DonorImport(World& group, bool enabled) : world(group), timings(enabled) {}
DonorImport::~DonorImport() = default;
bool DonorImport::Vote(const CLocalFailure& failure, std::string& reason) {
  const auto elected = world.elect(failure);
  reason = elected.message;
  return !elected.any;
}
bool DonorImport::Admit(size_t bytes, size_t ceiling, std::string& reason) {
  stats.maximum_work_bytes = std::max(stats.maximum_work_bytes, bytes);
  CLocalFailure failure;
  if (bytes == SIZE_MAX || bytes > ceiling) failure.Set(3, 0, "Native 3D donor import exceeds working-storage budget.");
  return Vote(failure, reason);
}
size_t DonorImport::ResidentBytes() const {
  return transfer_memory::Add(transfer_memory::Bytes(donors), transfer_memory::Bytes(boxes),
      index ? transfer_memory::Add(index->GetAllocatedBytes(), index->RetainedWorkspaceBound(1)) : 0,
      ranks ? ranks->GetMemory() : 0);
}
bool DonorImport::Build(const std::vector<DonorCell>& original, std::string& reason,
                        size_t ceiling, size_t caller_bytes) {
  Timer timer(stats.build_seconds, timings);
  ++stats.builds;
  CLocalFailure failure;
  if (initialized) failure.Set(1, 0, "Native 3D donor source is already frozen.");
  if (!Vote(failure, reason)) { ++stats.rejected; return false; }
  const auto n = original.size();
  // Guard ordinary arithmetic inside the house predictor before calling it.
  const auto envelope = transfer_memory::Mul(transfer_memory::Mul(4096, transfer_memory::Add(n, 1)),
      size_t(omp_get_max_threads()));
  if (!Admit(transfer_memory::Add(caller_bytes, transfer_memory::Bytes(original), envelope), ceiling, reason)) {
    ++stats.rejected; return false;
  }
  size_t retained = 0, peak = 0;
  if (n) CADTElemClass::PredictBytes(3, 2 * n, n, 2 * n, &retained, &peak);
  const auto baseline = transfer_memory::Add(caller_bytes, transfer_memory::Bytes(original));
  const auto initial_peak = transfer_memory::Add(baseline, transfer_memory::Mul(n, sizeof(DonorCell)),
      transfer_memory::Mul(6 * n, sizeof(double)), transfer_memory::Mul(n, 256), peak,
      transfer_memory::Mul(world.size, 32768), transfer_memory::GrowthBound(6 * n, sizeof(Authority)));
  if (!Admit(initial_peak, ceiling, reason)) { ++stats.rejected; return false; }
  std::vector<DonorCell> staged;
  std::vector<double> staged_boxes;
  std::vector<std::vector<Authority>> to;
  std::unique_ptr<CADTElemClass> staged_index;
  std::unique_ptr<CRankBoxTree> staged_ranks;
  std::vector<double> owner_boxes;
  try {
    staged = original;
    std::sort(staged.begin(), staged.end(), [](const auto& a, const auto& b) { return a.Key() < b.Key(); });
    to.resize(world.size);
    staged_boxes.reserve(6 * n);
    std::vector<double> centroids;
    centroids.reserve(3 * n);
    for (const auto& donor : staged) {
      const auto key = donor.Key();
      if (std::adjacent_find(key.begin(), key.end()) != key.end()) throw std::invalid_argument("Repeated donor node.");
      const auto& v = donor.cell.v;
      if (!(Orientation(v[0].p, v[1].p, v[2].p, v[3].p) > 0)) throw std::invalid_argument("Nonpositive donor cell.");
      for (size_t i = 0; i < 4; ++i) {
        ValidateTensor(donor.sensor[i]);
        Authority row{0, {v[i].id, 0, 0, 0}, v[i].p, donor.sensor[i]};
        to[Server(row, world.size)].push_back(row);
      }
      for (unsigned kind : {1, 2}) {
        Authority row{kind, kind == 1 ? std::array<Id, 4>{donor.cell.id, 0, 0, 0} : key, {}, {}};
        to[Server(row, world.size)].push_back(row);
      }
      const auto box = Bounds(donor.cell);
      staged_boxes.insert(staged_boxes.end(), box.begin(), box.end());
      for (size_t a = 0; a < 3; ++a) centroids.push_back(.5 * box[a] + .5 * box[a + 3]);
    }
    owner_boxes = BisectionBoxes(3, staged_boxes, centroids, 8, nullptr);
    staged_ranks = std::make_unique<CRankBoxTree>();
    if (n) {
      std::vector<su2double> coordinates(staged_boxes.begin(), staged_boxes.end());
      std::vector<unsigned long> connectivity(2 * n), ids(n);
      for (size_t i = 0; i < connectivity.size(); ++i) connectivity[i] = i;
      for (size_t i = 0; i < n; ++i) ids[i] = i;
      std::vector<unsigned short> types(n, LINE), markers(n, 0);
      staged_index = std::make_unique<CADTElemClass>(3, coordinates, connectivity, types, markers, ids, false);
    }
  } catch (const std::exception& e) { failure.Set(2, 0, e.what()); }
  if (!Vote(failure, reason)) { ++stats.rejected; return false; }
  bool admitted = false;
  const auto records = world.exchange(to, &admitted, ceiling, initial_peak);
  if (!admitted) failure.Set(3, 0, "Original donor authority transport exceeds budget.");
  if (!Vote(failure, reason)) { ++stats.rejected; return false; }
  const auto check_peak = transfer_memory::Add(initial_peak, transfer_memory::Bytes(to), transfer_memory::Bytes(records),
      MapBytes(records.size(), sizeof(std::pair<const std::pair<unsigned, std::array<Id, 4>>, Authority>)));
  if (!Admit(check_peak, ceiling, reason)) { ++stats.rejected; return false; }
  try {
    std::map<std::pair<unsigned, std::array<Id, 4>>, Authority> authority;
    for (const auto& row : records) {
      const auto inserted = authority.emplace(std::make_pair(row.kind, row.key), row);
      if (inserted.second) continue;
      const auto& old = inserted.first->second;
      if (row.kind != 0) throw std::invalid_argument("Duplicate original donor cell ID or simplex across workers.");
      if (old.point.x != row.point.x || old.point.y != row.point.y || old.point.z != row.point.z ||
          Components(old.sensor) != Components(row.sensor))
        throw std::invalid_argument("Inconsistent original donor sensor/coordinate across workers.");
    }
  } catch (const std::exception& e) { failure.Set(2, 0, e.what()); }
  if (!Vote(failure, reason)) { ++stats.rejected; return false; }
  const auto total = CPassiveComm::Allreduce(uint64_t(std::min(n, MaximumDonors + 1)), CPassiveComm::Op::SUM, world.comm);
  if (!total) failure.Set(1, 0, "Empty original donor source.");
  if (!Vote(failure, reason)) { ++stats.rejected; return false; }
  staged_ranks->Build(3, owner_boxes, world.comm);
  const auto resident = transfer_memory::Add(baseline, transfer_memory::Bytes(staged), transfer_memory::Bytes(staged_boxes),
      staged_index ? transfer_memory::Add(staged_index->GetAllocatedBytes(), staged_index->RetainedWorkspaceBound(1)) : 0,
      staged_ranks->GetMemory());
  if (!Admit(transfer_memory::Add(check_peak, resident), ceiling, reason)) { ++stats.rejected; return false; }
  donors.swap(staged); boxes.swap(staged_boxes); index.swap(staged_index); ranks.swap(staged_ranks);
  filter_cells = total > MaximumDonors;
  initialized = true;built_from=&original;
  return true;
}
DonorPatch DonorImport::Import(const std::vector<DonorRegion>& regions, size_t ceiling, size_t caller_bytes) {
  return ImportRegions(regions, nullptr, ceiling, caller_bytes);
}
DonorPatch DonorImport::ImportCells(const std::vector<Cell>& cells, size_t ceiling, size_t caller_bytes) {
  return ImportRegions({}, &cells, ceiling, caller_bytes);
}
DonorPatch DonorImport::ImportMetricCells(const std::vector<Cell>& cells, size_t ceiling, size_t caller_bytes) {
  return ImportRegions({}, &cells, ceiling, caller_bytes, true);
}
DonorPatch DonorImport::ImportMetricSamples(const std::vector<Cell>& cells,const std::vector<Id>& extensions,
                                           size_t ceiling,size_t caller_bytes) {
  DonorPatch result;CLocalFailure failure;std::vector<Point> centers;std::vector<std::array<Point,2>> edges;
  const auto prepared=transfer_memory::Add(caller_bytes,transfer_memory::Bytes(cells),transfer_memory::Bytes(extensions),
      transfer_memory::GrowthBound(cells.size(),sizeof(Point)+6*sizeof(std::array<Point,2>)));
  if(cells.size()>MaximumCavityCells/6)failure.Set(1,0,"Metric sample cell batch exceeds query budget.");
  if(!Vote(failure,result.reason) || !Admit(transfer_memory::Add(prepared,ResidentBytes()),ceiling,result.reason))return result;
  try {
    centers.reserve(cells.size());edges.reserve(6*cells.size());
    for(const auto& cell:cells) {
      const auto& v=cell.v;
      if(!(Orientation(v[0].p,v[1].p,v[2].p,v[3].p)>0))throw std::invalid_argument("Nonpositive metric sample cell.");
      centers.push_back(Centroid(cell));for(auto edge:QueryEdges)edges.push_back({v[edge[0]].p,v[edge[1]].p});
    }
  }catch(const std::exception& e){failure.Set(2,0,e.what());}
  if(!Vote(failure,result.reason))return result;
  result=ImportEdgeSegments(edges,ceiling,prepared);if(!result.valid)return result;
  const auto held=transfer_memory::Add(prepared,transfer_memory::Bytes(result.cells),transfer_memory::Bytes(result.source_rank));
  const auto samples=SampleQueries(centers,extensions,ceiling,held,nullptr,true);
  if(!samples.valid)return {{},false,samples.reason};
  const auto route=transfer_memory::Add(held,ResidentBytes(),transfer_memory::Bytes(samples.samples),
      transfer_memory::Mul(world.size,sizeof(std::vector<SampleSupportRequest>)),
      transfer_memory::GrowthBound(samples.samples.size(),sizeof(SampleSupportRequest)));
  if(!Admit(route,ceiling,result.reason))return {{},false,result.reason};
  std::vector<std::vector<SampleSupportRequest>> send;
  size_t missing=0;
  try {send.resize(world.size);for(const auto& sample:samples.samples)
    if(std::none_of(result.cells.begin(),result.cells.end(),[&](const DonorCell& d){return d.Key()==sample.Key();})) {
      send.at(sample.source_rank).push_back({world.rank,sample.Key()});++missing;
    }}
  catch(const std::exception& e){failure.Set(2,0,e.what());}
  if(!Vote(failure,result.reason))return {{},false,result.reason};
  if(!world.maximum(missing!=0))return result;
  bool admitted=false;const auto requests=world.exchange(send,&admitted,ceiling,route);
  if(!admitted)failure.Set(3,0,"Metric point-support requests exceed storage.");
  if(!Vote(failure,result.reason))return {{},false,result.reason};
  const auto reply_bound=transfer_memory::Add(route,transfer_memory::Bytes(requests),
      transfer_memory::Mul(world.size,sizeof(std::vector<OwnedDonor>)),transfer_memory::GrowthBound(requests.size(),sizeof(OwnedDonor)),
      transfer_memory::GrowthBound(samples.samples.size(),sizeof(OwnedDonor)));
  if(!Admit(reply_bound,ceiling,result.reason))return {{},false,result.reason};
  std::vector<std::vector<OwnedDonor>> replies;
  try {
    replies.resize(world.size);
    for(const auto& request:requests) {
      if(request.caller<0 || request.caller>=world.size)throw std::runtime_error("Invalid metric point-support caller.");
      const auto it=std::lower_bound(donors.begin(),donors.end(),request.key,[](const DonorCell& d,const std::array<Id,4>& key){return d.Key()<key;});
      if(it==donors.end() || it->Key()!=request.key)throw std::runtime_error("Selected metric point support is absent from its original owner.");
      replies[request.caller].push_back({*it,world.rank});
    }
  }catch(const std::exception& e){failure.Set(2,0,e.what());}
  if(!Vote(failure,result.reason))return {{},false,result.reason};
  const auto received=world.exchange(replies,&admitted,ceiling,reply_bound);
  if(!admitted)failure.Set(3,0,"Metric point-support payload exceeds storage.");
  if(!Vote(failure,result.reason))return {{},false,result.reason};
  if(!Admit(transfer_memory::Add(reply_bound,transfer_memory::Bytes(received),
      transfer_memory::GrowthBound(transfer_memory::Add(result.cells.size(),received.size()),sizeof(DonorCell)+sizeof(int))),ceiling,result.reason))return {{},false,result.reason};
  try {
    if(received.size()!=missing)throw std::runtime_error("Incomplete metric point-support reply.");
    for(const auto& row:received) {
      if(row.rank<0 || row.rank>=world.size || std::none_of(samples.samples.begin(),samples.samples.end(),[&](const DonorSample& sample){
        return sample.source_rank==row.rank && sample.cell==row.donor.cell.id && sample.Key()==row.donor.Key();}))
        throw std::runtime_error("Invalid metric point-support owner/key.");
      if(std::any_of(result.cells.begin(),result.cells.end(),[&](const DonorCell& old){return old.Key()==row.donor.Key();}))continue;
      if(result.cells.size()==MaximumDonors)throw std::runtime_error("Original donor union exceeds patch budget.");
      result.cells.push_back(row.donor);result.source_rank.push_back(row.rank);
    }
  }catch(const std::exception& e){failure.Set(2,0,e.what());}
  if(!Vote(failure,result.reason))return {{},false,result.reason};
  stats.payloads+=received.size();stats.maximum_patch=std::max(stats.maximum_patch,result.cells.size());return result;
}
DonorSamples DonorImport::SamplePoints(const std::vector<Point>& points,const std::vector<Id>& extensions,
                                      size_t ceiling,size_t caller_bytes) {
  return SampleQueries(points,extensions,ceiling,caller_bytes);
}
DonorSamples DonorImport::SampleMidpoints(const std::vector<std::array<Point,2>>& endpoints,const std::vector<Id>& extensions,
                                         size_t ceiling,size_t caller_bytes) {
  return SampleQueries({},extensions,ceiling,caller_bytes,&endpoints);
}
DonorSamples DonorImport::SampleQueries(const std::vector<Point>& points,const std::vector<Id>& extensions,
                                       size_t ceiling,size_t caller_bytes,const std::vector<std::array<Point,2>>* midpoints,bool allow_missing) {
  const auto size=midpoints?midpoints->size():points.size();
  Timer total(stats.point_seconds,timings);++stats.point_batches;stats.point_queries+=size;
  DonorSamples result;CLocalFailure failure;
  if(!initialized || size>MaximumCavityCells)failure.Set(1,0,"Invalid/uninitialized original point sample batch.");
  auto finite=[](Point p){return std::isfinite(p.x)&&std::isfinite(p.y)&&std::isfinite(p.z);};
  for(auto p:points)if(!finite(p))failure.Set(1,0,"Nonfinite original point sample.");
  if(midpoints)for(const auto& ends:*midpoints)if(!finite(ends[0]) || !finite(ends[1]))
    failure.Set(1,0,"Nonfinite original midpoint endpoint.");
  if(std::find(extensions.begin(),extensions.end(),UINT64_MAX)!=extensions.end())
    failure.Set(1,0,"Physical source cannot be declared as an extension.");
  if(!Vote(failure,result.reason)){++stats.rejected;return result;}
  const auto baseline=transfer_memory::Add(caller_bytes,ResidentBytes(),midpoints?transfer_memory::Bytes(*midpoints):transfer_memory::Bytes(points),transfer_memory::Bytes(extensions));
  const auto route_peak=transfer_memory::Add(baseline,ranks->QueryBytes(),
      transfer_memory::Mul(world.size,sizeof(std::vector<PointRequest>)),
      transfer_memory::GrowthBound(transfer_memory::Mul(transfer_memory::Add(size,extensions.size()),world.size),sizeof(PointRequest)),
      transfer_memory::Bytes(extensions));
  if(!Admit(route_peak,ceiling,result.reason)){++stats.rejected;return result;}
  std::vector<std::vector<PointRequest>> send;
  try {
    send.resize(world.size);std::vector<int> owners;auto eligible=extensions;
    std::sort(eligible.begin(),eligible.end());eligible.erase(std::unique(eligible.begin(),eligible.end()),eligible.end());
    for(size_t i=0;i<size;++i) {
      PointRequest query{world.rank,0,i,midpoints?midpoints->at(i)[0]:points[i],UINT64_MAX};
      if(midpoints){query.end=midpoints->at(i)[1];query.midpoint=true;}
      const auto box=PointBounds(query);ranks->RanksIntersecting(box.data(),box.data()+3,owners);
      for(auto peer:owners)send.at(peer).push_back(query);
    }
    // Fixed-size eligibility records share the query transport; the house codec requires fixed records.
    for(auto& bucket:send)if(!bucket.empty())for(auto component:eligible)
      bucket.push_back({world.rank,1,0,{},component});
  }catch(const std::exception& error){failure.Set(2,0,error.what());}
  if(!Vote(failure,result.reason)){++stats.rejected;return result;}
  bool admitted=false;auto requests=world.exchange(send,&admitted,ceiling,route_peak);
  if(!admitted)failure.Set(3,0,"Original point request transport exceeds budget.");
  if(!Vote(failure,result.reason)){++stats.rejected;return result;}
  const auto incoming=transfer_memory::Bytes(requests);const auto count=requests.size();
  const auto search_peak=transfer_memory::Add(route_peak,incoming,transfer_memory::IntersectionQueryBound(donors.size()),
      transfer_memory::Mul(count,sizeof(Id)+64),
      transfer_memory::Mul(world.size,sizeof(std::vector<PointReply>)),transfer_memory::GrowthBound(count,sizeof(PointReply)),
      transfer_memory::GrowthBound(size,sizeof(DonorSample)+sizeof(unsigned char)),
      transfer_memory::GrowthBound(transfer_memory::Mul(size,world.size),sizeof(PointReply)));
  if(!Admit(search_peak,ceiling,result.reason)){++stats.rejected;return result;}
  std::vector<std::vector<PointReply>> replies;
  try {
    replies.resize(world.size);std::vector<unsigned long> candidates;std::map<int,std::vector<Id>> eligibility;
    for(const auto& request:requests) {
      if(request.caller<0 || request.caller>=world.size || request.kind>1)
        throw std::runtime_error("Invalid original point request protocol.");
      if(request.kind==1) {
        if(request.component==UINT64_MAX || request.midpoint)throw std::runtime_error("Invalid original point extension declaration.");
        eligibility[request.caller].push_back(request.component);
      }
    }
    for(auto& row:eligibility) {
      auto& ids=row.second;std::sort(ids.begin(),ids.end());
      if(std::adjacent_find(ids.begin(),ids.end())!=ids.end())throw std::runtime_error("Repeated original point extension declaration.");
    }
    for(const auto& query:requests)if(!query.kind) {
      if(query.ordinal>=MaximumCavityCells || !std::isfinite(query.point.x) || !std::isfinite(query.point.y) || !std::isfinite(query.point.z) || (query.midpoint && !finite(query.end)))
        throw std::runtime_error("Invalid original point request coordinate/index.");
      const auto& allowed=eligibility[query.caller];
        if(!index)continue;
        DonorSample sample;bool found=false;size_t selected=0;
        {
          Timer search(stats.point_search_seconds,timings);
          const auto point=PointBounds(query);index->DetermineIntersectingElements(point.data(),point.data()+3,candidates);
          std::sort(candidates.begin(),candidates.end());stats.point_candidates+=candidates.size();
          // Build sorts canonical keys. Search physical donors first, then declared extensions.
          for(unsigned tier=0;tier<2 && !found;++tier)for(auto i:candidates) {
            const auto& row=donors.at(i);const bool exterior=row.extension_component!=UINT64_MAX;
            if(exterior!=(tier==1) || (exterior && !std::binary_search(allowed.begin(),allowed.end(),row.extension_component)))continue;
            if(!Intersects(point,boxes.data()+6*i))continue;
            const auto& v=row.cell.v;++stats.point_containment_tests;
            const Tetrahedron tet{v[0].p,v[1].p,v[2].p,v[3].p};
            const bool contains=query.midpoint?BarycentricMidpoint(tet,query.point,query.end,sample.weights,&stats.filter_kernel)
                                               :Barycentric(tet,query.point,sample.weights,&stats.filter_kernel);
            if(contains) {
              selected=i;found=true;break;
            }
          }
        }
        if(found) {
          Timer interpolation(stats.point_interpolation_seconds,timings);const auto& row=donors[selected];
          sample.sensor=InterpolateSensor(row,sample.weights);ValidateTensor(sample.sensor,&stats.filter_kernel);
          for(unsigned j=0;j<4;++j)sample.nodes[j]=row.cell.v[j].id;
          sample.cell=row.cell.id;sample.extension_component=row.extension_component;sample.source_rank=world.rank;
          replies.at(query.caller).push_back({query.ordinal,sample});
        }
      }
  }catch(const std::exception& error){failure.Set(2,0,error.what());}
  if(!Vote(failure,result.reason)){++stats.rejected;return result;}
  auto received=world.exchange(replies,&admitted,ceiling,search_peak);
  if(!admitted)failure.Set(3,0,"Original point reply transport exceeds budget.");
  stats.point_payloads+=received.size();
  std::vector<DonorSample> selected;std::vector<unsigned char> found;
  try {
    if(admitted) {
      selected.resize(size);found.resize(size);
      for(const auto& reply:received) {
        const auto& sample=reply.sample;
        if(reply.ordinal>=size || sample.source_rank<0 || sample.source_rank>=world.size ||
           (sample.extension_component!=UINT64_MAX && std::find(extensions.begin(),extensions.end(),sample.extension_component)==extensions.end()))
          throw std::runtime_error("Invalid original point reply protocol.");
        const auto key=sample.Key();
        if(std::adjacent_find(key.begin(),key.end())!=key.end())throw std::runtime_error("Repeated original point donor node.");
        long double sum=0;for(auto w:sample.weights){if(!std::isfinite(w) || w<0 || w>1)throw std::runtime_error("Invalid original point barycentric weight.");sum+=w;}
        if(std::abs(sum-1)>64*std::numeric_limits<long double>::epsilon())throw std::runtime_error("Invalid original point weight sum.");
        ValidateTensor(sample.sensor,&stats.filter_kernel);
        auto& prior=selected[reply.ordinal];
        const auto priority=std::make_pair(sample.extension_component!=UINT64_MAX,key);
        if(!found[reply.ordinal] || priority<std::make_pair(prior.extension_component!=UINT64_MAX,prior.Key()))prior=sample;
        found[reply.ordinal]=1;
      }
      if(!allow_missing && std::find(found.begin(),found.end(),0)!=found.end())throw std::runtime_error("No original donor contains a requested point.");
      if(allow_missing) {
        size_t count=0;for(size_t i=0;i<selected.size();++i)if(found[i])selected[count++]=selected[i];
        selected.resize(count); // Only import actual supports; private metric queries still reject missing coverage.
      }
    }
  }catch(const std::exception& error){failure.Set(2,0,error.what());}
  if(!Vote(failure,result.reason)){++stats.rejected;return result;}
  result.samples=std::move(selected);result.valid=true;return result;
}
DonorPatch DonorImport::ImportSegments(const std::vector<std::array<Point,2>>& segments,size_t ceiling,size_t caller_bytes) {
  return ImportRegions({},nullptr,ceiling,caller_bytes,false,&segments);
}
DonorPatch DonorImport::ImportEdgeSegments(const std::vector<std::array<Point,2>>& segments,size_t ceiling,size_t caller_bytes) {
  return ImportRegions({},nullptr,ceiling,caller_bytes,false,&segments,nullptr,true);
}
DonorPatch DonorImport::ProbePoints(const std::vector<Point>& points,size_t ceiling,size_t caller_bytes) {
  return ImportRegions({},nullptr,ceiling,caller_bytes,false,nullptr,&points);
}
DonorPatch DonorImport::ImportRegions(const std::vector<DonorRegion>& regions, const std::vector<Cell>* cells,
                                    size_t ceiling, size_t caller_bytes, bool metric_queries,
                                    const std::vector<std::array<Point,2>>* segments, const std::vector<Point>* points, bool positive_segments) {
  Timer timer(stats.import_seconds, timings);
  ++stats.imports;
  DonorPatch result;
  CLocalFailure failure;
  if (!initialized) failure.Set(1, 0, "Original donor source has not been frozen.");
  const auto count = points ? points->size() : segments ? segments->size() : cells ? cells->size() : regions.size();
  if (count > MaximumCavityCells) failure.Set(1, 0, "Too many original donor import regions.");
  if (!Vote(failure, result.reason)) { ++stats.rejected; return result; }
  const auto baseline = transfer_memory::Add(caller_bytes, ResidentBytes(), points ? transfer_memory::Bytes(*points) : segments ? transfer_memory::Bytes(*segments) : cells ? transfer_memory::Bytes(*cells) : transfer_memory::Bytes(regions));
  const auto route_peak = transfer_memory::Add(baseline, ranks->QueryBytes(),
      transfer_memory::Mul(world.size, sizeof(std::vector<Request>) + sizeof(unsigned char)),
      transfer_memory::GrowthBound(transfer_memory::Mul(count, world.size), sizeof(Request)));
  if (!Admit(route_peak, ceiling, result.reason)) { ++stats.rejected; return result; }
  std::vector<std::vector<Request>> to;
  try {
    Timer routing(stats.routing_seconds, timings);
    to.resize(world.size);
    std::vector<int> owners;
    std::vector<unsigned char> mask(world.size);
    auto route = [&](const DonorRegion& region) {
      for (size_t a = 0; a < 3; ++a)
        if (!(std::isfinite(region[a]) && std::isfinite(region[a + 3]) && region[a] <= region[a + 3]))
          throw std::invalid_argument("Invalid original donor import region.");
      ranks->RanksIntersecting(region.data(), region.data() + 3, owners);
    };
    auto send = [&](const Request& request) {
      route(request.region);
      for (auto owner : owners) to[owner].push_back(request);
    };
    for (size_t i = 0; i < count; ++i) {
      Request request; request.caller = world.rank;
      if(points || segments) {
        const auto ends=points?std::array<Point,2>{points->at(i),points->at(i)}:segments->at(i);
        for(Point p:ends)for(double x:{p.x,p.y,p.z})if(!std::isfinite(x))throw std::invalid_argument("Nonfinite original segment query.");
        request.region=SegmentBounds(ends[0],ends[1]);request.tetra[0]=ends[0];request.tetra[1]=ends[1];
        if(positive_segments && ends[0].x==ends[1].x && ends[0].y==ends[1].y && ends[0].z==ends[1].z)
          throw std::invalid_argument("Metric donor segment must have nonzero length.");
        request.segment=true;request.tetrahedral=true;request.positive_length=positive_segments;
      } else request.region = cells ? Bounds(cells->at(i)) : regions[i];
      if (cells) {
        request.tetrahedral = filter_cells;
        for (size_t j = 0; j < 4; ++j) request.tetra[j] = cells->at(i).v[j].p;
        const auto& t = request.tetra;
        if (!(Orientation(t[0], t[1], t[2], t[3]) > 0)) throw std::invalid_argument("Nonpositive donor import cell.");
      }
      if (cells && filter_cells && metric_queries) {
        std::fill(mask.begin(), mask.end(), 0);
        const auto center = Centroid(cells->at(i));
        for (size_t query = 0; query <= QueryEdges.size(); ++query) {
          const Point a = query ? request.tetra[QueryEdges[query-1][0]] : center;
          const Point b = query ? request.tetra[QueryEdges[query-1][1]] : center;
          route(SegmentBounds(a,b));
          for (auto owner : owners) mask[owner] |= 1u << query;
        }
        // Carry the sender's rounded centroid in the existing point box, never recompute it remotely.
        request.region = SegmentBounds(center,center);
        for (int owner = 0; owner < world.size; ++owner) if (mask[owner]) {
          request.query = mask[owner]; to[owner].push_back(request);
        }
      } else send(request);
    }
  } catch (const std::exception& e) { failure.Set(2, 0, e.what()); }
  if (!Vote(failure, result.reason)) { ++stats.rejected; return result; }
  bool admitted = false;
  std::vector<Request> requests;
  {
    Timer exchange(stats.exchange_seconds, timings);
    requests = world.exchange(to, &admitted, ceiling, route_peak);
  }
  if (!admitted) failure.Set(3, 0, "Original donor request transport exceeds budget.");
  if (!Vote(failure, result.reason)) { ++stats.rejected; return result; }
  const auto search_peak = transfer_memory::Add(route_peak, transfer_memory::Bytes(to), transfer_memory::Bytes(requests),
      transfer_memory::IntersectionQueryBound(donors.size()), transfer_memory::Mul(world.size, sizeof(std::map<Id, size_t>)),
      MapBytes(transfer_memory::Mul(world.size, MaximumDonors + 1), sizeof(std::pair<const Id, size_t>)),
      transfer_memory::Mul(world.size, sizeof(std::vector<OwnedDonor>)),
      transfer_memory::GrowthBound(transfer_memory::Mul(world.size, MaximumDonors), sizeof(OwnedDonor)),
      transfer_memory::Mul(MaximumDonors,sizeof(DonorCell)+sizeof(int)));
  if (!Admit(search_peak, ceiling, result.reason)) { ++stats.rejected; return result; }
  std::vector<std::vector<OwnedDonor>> payload;
  try {
    Timer search_time(stats.search_seconds, timings);
    std::vector<std::map<Id, size_t>> selected(world.size);
    std::vector<unsigned long> candidates;
    for (const auto& request : requests) {
      ++stats.owner_requests;
      if (request.caller < 0 || request.caller >= world.size) throw std::invalid_argument("Invalid donor request owner.");
      if (request.query > 127 || (request.query && !request.tetrahedral) ||
          (request.segment && (!request.tetrahedral || request.query)) || (request.positive_length && !request.segment))
        throw std::invalid_argument("Invalid donor metric-query kind.");
      if (!index) continue;
      auto search = [&](const DonorRegion& b, unsigned char query) {
        const su2double lo[3]{b[0], b[1], b[2]}, hi[3]{b[3], b[4], b[5]};
        index->DetermineIntersectingElements(lo, hi, candidates);
        stats.index_candidates += candidates.size();
        for (auto i : candidates) {
          if (!Intersects(b, boxes.data() + 6 * i)) continue;
          auto& set = selected[request.caller];
          // One import returns a caller union: a previously admitted immutable donor needs no second filter.
          if (set.count(donors.at(i).cell.id)) { ++stats.reused_candidates; continue; }
          if (request.tetrahedral) {
            Timer filtering(stats.filter_seconds, timings);
            ++stats.filter_candidates;
            const auto& v = donors.at(i).cell.v;
            const Tetrahedron donor{v[0].p, v[1].p, v[2].p, v[3].p};
            bool needed = true;
            if (query) {
              const auto& point = request.region;
              const Point a = query == 1 ? Point{point[0],point[1],point[2]} : request.tetra[QueryEdges[query - 2][0]];
              const Point end = query == 1 ? a : request.tetra[QueryEdges[query - 2][1]];
              needed = SegmentMayIntersectBox(a, end, boxes.data()+6*i);
              if (!needed) ++stats.box_filtered;
              else needed = request.positive_length ? IntersectsSegmentPositiveLength(donor,a,end,&stats.filter_kernel)
                                                   : IntersectsSegment(donor,a,end,&stats.filter_kernel);
            } else needed = !(Outside(request.tetra, donor, &stats.filter_kernel) ||
                              Outside(donor, request.tetra, &stats.filter_kernel));
            if (!needed) { ++stats.filtered; continue; }
          }
          set.emplace(donors.at(i).cell.id, i);
          if (set.size() > MaximumDonors) throw std::runtime_error("Original donor import exceeds patch budget.");
        }
      };
      if (request.segment) search(request.region,2);
      else if (!request.query) search(request.region,0);
      else for (size_t query = 0; query <= QueryEdges.size(); ++query) if (request.query & (1u << query)) {
        const Point a = query ? request.tetra[QueryEdges[query-1][0]] : Point{request.region[0],request.region[1],request.region[2]};
        const Point b = query ? request.tetra[QueryEdges[query-1][1]] : a;
        search(SegmentBounds(a,b),query+1);
      }
    }
    payload.resize(world.size);
    for (int peer = 0; peer < world.size; ++peer)
      for (auto entry : selected[peer]) payload[peer].push_back({donors[entry.second],world.rank});
  } catch (const std::exception& e) { failure.Set(2, 0, e.what()); }
  if (!Vote(failure, result.reason)) { ++stats.rejected; return result; }
  std::vector<OwnedDonor> received;
  {
    Timer exchange(stats.exchange_seconds, timings);
    received = world.exchange(payload, &admitted, ceiling, search_peak);
  }
  if (!admitted) failure.Set(3, 0, "Original donor payload transport exceeds budget.");
  if (received.size() > MaximumDonors) failure.Set(1, 0, "Original donor union exceeds patch budget.");
  if (count && received.empty() && !points) failure.Set(1, 0, "No original donor intersects requested regions.");
  try {
    if(admitted && received.size()<=MaximumDonors) {
      std::sort(received.begin(), received.end(), [](const auto& a, const auto& b) { return a.donor.Key() < b.donor.Key(); });
      result.cells.reserve(received.size());result.source_rank.reserve(received.size());
      for(auto& row:received) {
        if(row.rank<0 || row.rank>=world.size)throw std::runtime_error("Invalid original donor source rank.");
        result.cells.push_back(std::move(row.donor));result.source_rank.push_back(row.rank);
      }
    }
  }catch(const std::exception& error){failure.Set(2,0,error.what());}
  if(!Vote(failure,result.reason)){++stats.rejected;return result;}
  stats.payloads += received.size();
  stats.maximum_patch = std::max(stats.maximum_patch, received.size());
  result.valid = true;
  return result;
}
}  // namespace SU2Native3D
