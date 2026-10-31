/*!
 * \file CNativeDistributed3D.cpp
 * \brief Routed complete stars and staged tetrahedral ownership publication.
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md).
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */
#include "../../include/adaptation/CNativeDistributed3D.hpp"
#include "../../include/adt/CADTElemClass.hpp"
#include "../../include/option_structure.hpp"
#include <chrono>
#include <climits>
#include <cmath>
#include <set>
#include <stdexcept>
#include <tuple>
namespace SU2Native3D {
namespace {
class Timer {
 public:
  explicit Timer(double& seconds) : seconds(seconds), start(std::chrono::steady_clock::now()) {}
  ~Timer() { seconds += std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count(); }
 private:
  double& seconds;
  std::chrono::steady_clock::time_point start;
};
template <class Map>
size_t MapBytes(size_t count) {
  // Requested-byte estimate; allocator-inclusive verification is a separate admission test.
  return transfer_memory::Mul(count, sizeof(typename Map::value_type) + 4 * sizeof(void*));
}
bool Same(Point a, Point b) { return a.x == b.x && a.y == b.y && a.z == b.z; }
std::array<Id, 4> Nodes(const OwnedCell& row) {
  std::array<Id, 4> result;
  for (size_t i = 0; i < 4; ++i) result[i] = row.cell.v[i].id;
  std::sort(result.begin(), result.end());
  return result;
}
void Validate(const OwnedCell& row) {
  const auto ids = Nodes(row);
  if (std::adjacent_find(ids.begin(), ids.end()) != ids.end())
    throw std::invalid_argument("Native 3D owned tetrahedron repeats a vertex.");
  const auto& v = row.cell.v;
  if (!(Orientation(v[0].p, v[1].p, v[2].p, v[3].p) > 0))
    throw std::invalid_argument("Native 3D ownership requires positive tetrahedra.");
}
class ParameterAtlasScope {
 public:
  explicit ParameterAtlasScope(SU2NativeGeometry::AtlasBudget* value):budget(value) {
    if(budget){caller=budget->caller_bytes;maximum=budget->maximum_bytes;}
  }
  ~ParameterAtlasScope(){if(budget){budget->caller_bytes=caller;budget->maximum_bytes=maximum;}}
  void Refresh(size_t live,size_t ceiling) {
    if(!budget)return;if(caller>maximum)throw std::invalid_argument("Invalid atlas publication reservation.");const auto peak=transfer_memory::Add(live,maximum-caller);
    if(live>ceiling||peak==SIZE_MAX)throw std::length_error("Atlas publication scratch reservation overflows admission.");
    budget->caller_bytes=live;budget->maximum_bytes=std::min(ceiling,peak);
  }
 private:
  SU2NativeGeometry::AtlasBudget* budget;size_t caller=0,maximum=0;
};
struct AtlasPage {
  uint64_t source=0,revision=0,identity=0,sequence=0,total=0,used=0;
  std::array<uint64_t,64> words{};
  template<class S>void Fields(S& stream){stream(source,revision,identity,sequence,total,used,words);}
};
struct Query {
  Feature feature;
  int caller = 0;
  unsigned ticket = 0;
  bool footprint = false, coordinate_star = false;
  template <class S> void Fields(S& s) { s(feature, caller, ticket, footprint, coordinate_star); }
};
struct Reply {
  Dependency dependency;
  unsigned ticket = 0;
  int status = 0;  // 0 dependency, 1 complete footer, 2 missing, 3 over budget.
  template <class S> void Fields(S& s) { s(dependency, ticket, status); }
};
struct FootprintReply {
  Dependency dependency;
  unsigned ticket = 0;
  int status = 0;  // Same footers; status4 carries the complete cell's vertices.
  std::array<Id,4> nodes{};
  template <class S> void Fields(S& s) { s(dependency,ticket,status,nodes); }
};
struct FetchQuery {
  Dependency dependency;
  int caller = 0;
  template <class S> void Fields(S& s) { s(dependency, caller); }
};
struct Payload {
  OwnedCell cell;
  bool present = false;
  template <class S> void Fields(S& s) { s(cell, present); }
};
struct BatchPayload {
  Id requested = 0;  // A missing/stale payload must still identify its dependent candidates.
  OwnedCell cell;
  bool present = false;
  template <class S> void Fields(S& s) { s(requested, cell, present); }
};
struct BatchClaim {
  Id vertex = 0;
  int caller = 0;
  unsigned ticket = 0;
  unsigned priority = 0;
  template <class S> void Fields(S& s) { s(vertex, caller, ticket, priority); }
};
struct BatchVeto {
  unsigned ticket = 0;bool pass_deferred=false;
  template <class S> void Fields(S& s) { s(ticket,pass_deferred); }
};
struct Claim {
  Id vertex = 0;
  int caller = 0;
  unsigned priority = 0;
  template <class S> void Fields(S& s) { s(vertex, caller, priority); }
};
struct Veto {
  int caller = 0;bool pass_deferred=false;
  template <class S> void Fields(S& s) { s(caller,pass_deferred); }
};
}  // namespace
int Directory::Rendezvous(Key key) const {
  auto id = key.second ^ (key.first ? uint64_t(0x9e3779b97f4a7c15ULL) : 0);
  id ^= id >> 30;
  id *= 0xbf58476d1ce4e5b9ULL;
  id ^= id >> 27;
  id *= 0x94d049bb133111ebULL;
  id ^= id >> 31;
  return id % world.size;
}
bool Directory::Vote(const CLocalFailure& failure, std::string& reason) {
  Timer timer(stats.election_seconds);
  ++stats.elections;
  const auto elected = world.elect(failure);
  reason = elected.message;
  return !elected.any;
}
struct Directory::CollisionIndex {
  std::unique_ptr<CADTElemClass> tree;
  std::vector<std::pair<Id,uint64_t>> rows;
  std::set<Id> added;
  bool initialized=false;
  size_t Bytes()const {
    return transfer_memory::Add(transfer_memory::Bytes(rows),MapBytes<std::set<Id>>(added.size()),
        tree?transfer_memory::Add(tree->GetAllocatedBytes(),tree->RetainedWorkspaceBound(1)):0);
  }
};
Directory::Directory(World& world,bool track_changes,BoundaryParameterState* parameters)
    :world(world),track_changes(track_changes),parameters(parameters) {}
Directory::~Directory()=default;
size_t Directory::ResidentBytes() const {
  return transfer_memory::Add(MapBytes<Stars>(stars.size()), resident_payload_bytes, transfer_memory::Bytes(changed_vertices),MapBytes<std::set<Id>>(pass_vertices.size()),
      collision?transfer_memory::Add(sizeof(CollisionIndex),collision->Bytes()):0);
}
std::vector<char> Directory::Snapshot() const {
  std::vector<char> bytes;
  RecordStream stream(bytes);bool active=construction_pass;uint64_t held=pass_vertices.size();stream(active,held);
  for(auto vertex:pass_vertices)stream(vertex);
  for (const auto& star : stars)
    for (auto row : star.second) stream(row.second);
  return bytes;
}
Directory::Prepared Directory::Prepare(const std::vector<OwnedCell>& removed, const std::vector<OwnedCell>& added,
                                      size_t ceiling, size_t baseline) {
  Prepared result;
  CLocalFailure failure;
  std::vector<std::vector<Entry>> to;
  const auto input_bytes = transfer_memory::Add(baseline, ResidentBytes(), transfer_memory::Bytes(removed),
      transfer_memory::Bytes(added), transfer_memory::Mul(world.size, sizeof(std::vector<Entry>)));
  auto peak = transfer_memory::Add(input_bytes, transfer_memory::GrowthBound(
      transfer_memory::Mul(5, transfer_memory::Add(removed.size(), added.size())), sizeof(Entry)));
  stats.maximum_work_bytes = std::max(stats.maximum_work_bytes, peak);
  if (peak == SIZE_MAX || peak > ceiling) failure.Set(3, 0, "Native 3D directory packing exceeds budget.");
  if (!Vote(failure, result.reason)) return result;
  try {
    to.resize(world.size);
    for (bool erase : {true, false})
      for (const auto& row : erase ? removed : added) {
        Validate(row);
        const auto nodes = Nodes(row);
        Entry entry{row.cell.id, true, {row.cell.id, row.version, world.rank}, nodes, {}, erase};
        to[Rendezvous({true, row.cell.id})].push_back(entry);
        entry.identity = false;
        for (auto node : row.cell.v) {
          entry.key = node.id;
          entry.point = node.p;
          to[Rendezvous({false, node.id})].push_back(entry);
        }
      }
  } catch (const std::exception& e) { failure.Set(2, 0, e.what()); }
  if (!Vote(failure, result.reason)) return result;
  bool admitted = false;
  const auto records = world.exchange(to, &admitted, ceiling, input_bytes);
  if (!admitted) failure.Set(3, 0, "Native 3D directory transport exceeds budget.");
  const auto changed_peak = transfer_memory::Add(input_bytes, transfer_memory::Bytes(to),
      transfer_memory::Bytes(records), MapBytes<std::set<Key>>(records.size()), MapBytes<Stars>(records.size()),construction_pass?MapBytes<std::set<Id>>(records.size()):0);
  stats.maximum_work_bytes = std::max(stats.maximum_work_bytes, changed_peak);
  if (changed_peak == SIZE_MAX || changed_peak > ceiling) failure.Set(3, 0, "Native 3D changed-key staging exceeds budget.");
  if (!Vote(failure, result.reason)) return result;
  std::set<Key> changed;
  size_t copied = 0;
  try {
    for (const auto& row : records) changed.insert({row.identity, row.key});
    for (auto key : changed) {
      const auto old = stars.find(key);
      if (old != stars.end()) copied = transfer_memory::Add(copied, MapBytes<Star>(old->second.size()));
    }
    peak = transfer_memory::Add(input_bytes, transfer_memory::Bytes(to), transfer_memory::Bytes(records),
        MapBytes<decltype(changed)>(changed.size()), MapBytes<Stars>(changed.size()), copied,
        MapBytes<Star>(records.size()),construction_pass?MapBytes<std::set<Id>>(records.size()):0);
    stats.maximum_work_bytes = std::max(stats.maximum_work_bytes, peak);
    if (peak == SIZE_MAX || peak > ceiling) failure.Set(3, 0, "Native 3D directory staging exceeds budget.");
  } catch (const std::exception& e) { failure.Set(2, 0, e.what()); }
  if (!Vote(failure, result.reason)) return result;
  try {
    for (auto key : changed) {
      const auto old = stars.find(key);
      result.changed.emplace(key, old == stars.end() ? Star{} : old->second);
      if(construction_pass && !key.first)result.pass_vertices.insert(key.second);
    }
    for (const auto& row : records)
      if (row.erase) {
        auto& star = result.changed.at({row.identity, row.key});
        const auto old = star.find(row.dependency.cell);
        if (old == star.end() || old->second.dependency.version != row.dependency.version ||
            old->second.dependency.owner != row.dependency.owner || old->second.nodes != row.nodes ||
            !Same(old->second.point, row.point))
          throw std::runtime_error("Native 3D stale removed dependency at cell " + std::to_string(row.dependency.cell));
        star.erase(old);
      }
    for (const auto& row : records)
      if (!row.erase) {
        auto& star = result.changed.at({row.identity, row.key});
        if (!star.emplace(row.dependency.cell, row).second)
          throw std::runtime_error("Native 3D duplicate owned dependency at cell " + std::to_string(row.dependency.cell));
        if (row.identity) {
          const auto old = stars.find({true, row.key});
          if (old != stars.end() && !old->second.empty() && row.dependency.version <= old->second.begin()->second.dependency.version)
            throw std::runtime_error("Native 3D reused cell identity must advance its version.");
        }
      }
    for (auto& star : result.changed)
      if (!star.first.first && !star.second.empty()) {
        const auto p = star.second.begin()->second.point;
        for (auto& row : star.second)
          if (!Same(p, row.second.point)) throw std::runtime_error("Native 3D inconsistent shared vertex " + std::to_string(star.first.second));
      }
  } catch (const std::exception& e) { failure.Set(2, 0, e.what()); }
  result.valid = Vote(failure, result.reason);
  if (!result.valid) result.changed.clear();
  return result;
}
void Directory::Commit(Prepared&& prepared) noexcept {
  while(!prepared.pass_vertices.empty())pass_vertices.insert(prepared.pass_vertices.extract(prepared.pass_vertices.begin()));
  for (auto it = prepared.changed.begin(); it != prepared.changed.end();) {
    auto old = stars.find(it->first);
    if (old != stars.end()) resident_payload_bytes -= MapBytes<Star>(old->second.size());
    resident_payload_bytes += MapBytes<Star>(it->second.size());
    if (it->second.empty()) {
      if (old != stars.end()) stars.erase(old);
      ++it;
    } else if (old != stars.end()) {
      old->second.swap(it->second);
      ++it;
    } else {
      auto current = it++;
      stars.insert(prepared.changed.extract(current));
    }
  }
}
namespace {
struct CollisionPage {
  int owner=0;
  unsigned priority=0;
  size_t page=0,pages=0,total=0,total_removed=0,count=0,removed_count=0;
  Feature coordinate_star{};
  std::array<Cell,MaximumReplacementCells> cells{};
  std::array<Id,MaximumCavityCells> removed{};
  template<class S>void Fields(S& s){s(owner,priority,page,pages,total,total_removed,count,removed_count,coordinate_star,cells,removed);}
};
struct CollisionProposal {
  int owner=0;
  unsigned priority=0;
  size_t count=0,removed_count=0;
  std::vector<Cell> cells;
  std::vector<Id> removed;
};
bool CoordinateReplacement(const Patch& old,const Patch& fresh,Feature feature) {
  if(feature.count!=1 || old.cells.empty() || old.cells.size()!=fresh.cells.size() ||
      old.cells.size()>MaximumDirectoryScan)return false;
  std::map<Id,const Cell*> originals;
  std::map<Id,Point> nodes;
  for(const auto& cell:old.cells) {
    if(!originals.emplace(cell.id,&cell).second)return false;
    unsigned incidence=0;
    for(auto n:cell.v) {
      incidence+=n.id==feature.vertices[0];
      const auto found=nodes.emplace(n.id,n.p);
      if(!found.second && !Same(found.first->second,n.p))return false;
    }
    if(incidence!=1)return false;
  }
  Node moved{};bool have_moved=false;
  for(const auto& cell:fresh.cells) {
    const auto found=originals.find(cell.id);if(found==originals.end())return false;
    for(size_t k=0;k<4;++k) {
      const auto before=found->second->v[k],after=cell.v[k];
      if(before.id==feature.vertices[0]) {
        if(nodes.count(after.id) || (have_moved && (moved.id!=after.id || !Same(moved.p,after.p))))return false;
        moved=after;have_moved=true;
      } else if(before.id!=after.id || !Same(before.p,after.p))return false;
    }
    originals.erase(found);
  }
  return originals.empty() && have_moved;
}
std::array<su2double,6> CollisionBounds(const Cell& cell) {
  std::array<su2double,6> box{INFINITY,INFINITY,INFINITY,-INFINITY,-INFINITY,-INFINITY};
  for(auto n:cell.v) {
    const double p[3]={n.p.x,n.p.y,n.p.z};
    for(unsigned k=0;k<3;++k) {
      if(!std::isfinite(p[k]))throw std::invalid_argument("Nonfinite collision cell.");
      box[k]=std::min(box[k],su2double(p[k]));box[k+3]=std::max(box[k+3],su2double(p[k]));
    }
  }
  return box;
}
}
bool Directory::Clearance(const Owned& owned,const Patch& old,const Patch& fresh,bool& candidate,
                          std::string& reason,size_t ceiling,size_t caller_bytes,bool* deferred,unsigned priority,Feature coordinate_star) {
  Timer timer(stats.collision_seconds);CLocalFailure failure;
  if(deferred)*deferred=false;
  auto admit=[&](size_t extra) {
    const size_t peak=transfer_memory::Add(caller_bytes,ParameterBytes(),ResidentBytes(),MapBytes<Owned>(owned.size()),extra);
    stats.maximum_work_bytes=std::max(stats.maximum_work_bytes,peak);
    if(peak==SIZE_MAX || peak>ceiling)failure.Set(3,0,"Native3D current-mesh collision search exceeds storage budget.");
    return Vote(failure,reason);
  };
  if(!initialized)failure.Set(1,0,"Native3D collision search needs a published directory.");
  if(candidate && (coordinate_star.count ? (coordinate_star.count!=1 || old.cells.empty() ||
      old.cells.size()!=fresh.cells.size() || old.cells.size()>MaximumDirectoryScan) :
      (old.cells.size()>MaximumCavityCells || fresh.cells.size()>MaximumReplacementCells)))
    failure.Set(1,0,"Native3D collision proposal exceeds its topology/coordinate admission.");
  if(!admit(transfer_memory::Add(sizeof(CollisionIndex),
      coordinate_star.count?transfer_memory::Mul(old.cells.size(),1024):0)))return false;
  try {if(candidate && coordinate_star.count && !CoordinateReplacement(old,fresh,coordinate_star))
    failure.Set(1,0,"Native3D coordinate collision proposal changes its admitted connectivity.");}
  catch(const std::exception& e){failure.Set(2,0,e.what());}
  if(!Vote(failure,reason))return false;
  try {if(!collision)collision=std::make_unique<CollisionIndex>();}
  catch(const std::exception& e){failure.Set(2,0,e.what());}
  if(!Vote(failure,reason))return false;
  // Balance full rebuild cost against scanning the live additions; this threshold changes no contact gate.
  const bool rebuild=!collision->initialized ||
      collision->added.size()>std::max<size_t>(64,size_t(std::sqrt(double(collision->rows.size()+1)))) ||
      collision->rows.size()>transfer_memory::Add(transfer_memory::Mul(owned.size(),2),64);
  const size_t n=rebuild?owned.size():0;
  if(!admit(transfer_memory::Mul(transfer_memory::Mul(4096,transfer_memory::Add(n,1)),
      size_t(omp_get_max_threads()))))return false;
  size_t retained=0,peak=0;if(n)CADTElemClass::PredictBytes(3,2*n,n,2*n,&retained,&peak);
  if(!admit(transfer_memory::Add(peak,transfer_memory::Mul(n,6*sizeof(su2double)+3*sizeof(unsigned long)+
      2*sizeof(unsigned short)+sizeof(std::pair<Id,uint64_t>)))))return false;
  try {if(rebuild) {
    auto next=std::make_unique<CollisionIndex>();next->rows.reserve(n);
    std::vector<su2double> coordinates;coordinates.reserve(6*n);
    std::vector<unsigned long> connectivity(2*n),ids(n);
    std::vector<unsigned short> types(n,LINE),markers(n,0);
    for(const auto& row:owned) {
      const auto box=CollisionBounds(row.second.cell);coordinates.insert(coordinates.end(),box.begin(),box.end());
      next->rows.emplace_back(row.first,row.second.version);
    }
    for(size_t i=0;i<2*n;++i)connectivity[i]=i;for(size_t i=0;i<n;++i)ids[i]=i;
    if(n)next->tree=std::make_unique<CADTElemClass>(3,coordinates,connectivity,types,markers,ids,false);
    next->initialized=true;collision.swap(next);++stats.collision_builds;
  }}catch(const std::exception& e){failure.Set(2,0,e.what());}
  if(!Vote(failure,reason))return false;
  const size_t local_pages=candidate?std::max((fresh.cells.size()+MaximumReplacementCells-1)/MaximumReplacementCells,
      (old.cells.size()+MaximumCavityCells-1)/MaximumCavityCells):0;
  const size_t all_pages=CPassiveComm::Allreduce(local_pages,CPassiveComm::Op::SUM,world.comm);
  const size_t query_scratch=transfer_memory::Add(
      transfer_memory::Mul(transfer_memory::Add(transfer_memory::Mul(local_pages,world.size),all_pages),
          4*sizeof(CollisionPage)),transfer_memory::Mul(collision->rows.size(),sizeof(unsigned long)),
      transfer_memory::Mul(world.size,256));
  if(!admit(query_scratch))return false;
  std::vector<CollisionPage> local;std::string rejected_reason;
  try {if(candidate) {
    for(size_t i=0;i<fresh.cells.size();++i) {
      const auto& v=fresh.cells[i].v;
      if(!(Orientation(v[0].p,v[1].p,v[2].p,v[3].p)>0))throw std::invalid_argument("Nonpositive collision candidate.");
      for(size_t j=0;j<i;++j)if(!ConformingDisjoint(fresh.cells[i],fresh.cells[j]))candidate=false;
    }
    if(candidate)for(size_t page=0;page<local_pages;++page) {
      CollisionPage p;p.owner=world.rank;p.priority=priority;p.page=page;p.pages=local_pages;
      p.total=fresh.cells.size();p.total_removed=old.cells.size();p.coordinate_star=coordinate_star;
      const size_t first_cell=page*MaximumReplacementCells,first_removed=page*MaximumCavityCells;
      for(size_t k=first_cell;k<std::min(first_cell+MaximumReplacementCells,fresh.cells.size());++k)p.cells[p.count++]=fresh.cells[k];
      for(size_t k=first_removed;k<std::min(first_removed+MaximumCavityCells,old.cells.size());++k)p.removed[p.removed_count++]=old.cells[k].id;
      local.push_back(std::move(p));
    }else rejected_reason="Native3D curved replacement overlaps or has nonconforming contact.";
  }}catch(const std::exception& e){failure.Set(2,0,e.what());}
  if(!Vote(failure,reason))return false;
  const size_t held=transfer_memory::Add(caller_bytes,ParameterBytes(),ResidentBytes(),MapBytes<Owned>(owned.size()),query_scratch);
  std::vector<std::vector<CollisionPage>> outgoing;std::vector<std::vector<Id>> veto;
  try {outgoing.assign(world.size,local);veto.resize(world.size);}
  catch(const std::exception& e){failure.Set(2,0,e.what());}
  if(!Vote(failure,reason))return false;
  bool transported=false;
  const auto pages=world.exchange(outgoing,&transported,ceiling,held);
  if(!transported){reason="Native3D collision proposal exchange exceeds storage budget.";return false;}
  std::vector<CollisionProposal> proposals;
  try {
    std::map<int,std::vector<const CollisionPage*>> grouped;
    for(const auto& page:pages) {
      const size_t limit=page.coordinate_star.count==1?MaximumDirectoryScan:MaximumCavityCells;
      if(page.owner<0 || page.owner>=world.size || page.coordinate_star.count>1 ||
          !page.pages || page.page>=page.pages || !page.total ||
          (page.coordinate_star.count && !page.total_removed) ||
          page.total_removed>limit || page.total>(page.coordinate_star.count?limit:MaximumReplacementCells) ||
          page.count>MaximumReplacementCells || page.removed_count>MaximumCavityCells ||
          page.pages!=std::max((page.total+MaximumReplacementCells-1)/MaximumReplacementCells,
              (page.total_removed+MaximumCavityCells-1)/MaximumCavityCells))
        throw std::runtime_error("Invalid current collision page.");
      grouped[page.owner].push_back(&page);
    }
    for(auto& owner:grouped) {
      auto& rows=owner.second;
      std::sort(rows.begin(),rows.end(),[](const auto* a,const auto* b){return a->page<b->page;});
      const auto& first=*rows.front();
      if(rows.size()!=first.pages)throw std::runtime_error("Incomplete current collision pages.");
      CollisionProposal proposal;proposal.owner=first.owner;proposal.priority=first.priority;
      proposal.count=first.total;proposal.removed_count=first.total_removed;
      for(size_t k=0;k<rows.size();++k) {
        const auto& page=*rows[k];
        const size_t cells_at=k*MaximumReplacementCells,removed_at=k*MaximumCavityCells;
        if(page.page!=k || page.pages!=first.pages || page.total!=first.total || page.total_removed!=first.total_removed ||
            page.priority!=first.priority || page.coordinate_star.count!=first.coordinate_star.count ||
            page.coordinate_star.vertices!=first.coordinate_star.vertices ||
            page.count!=(cells_at<first.total?std::min(MaximumReplacementCells,first.total-cells_at):0) ||
            page.removed_count!=(removed_at<first.total_removed?std::min(MaximumCavityCells,first.total_removed-removed_at):0))
          throw std::runtime_error("Inconsistent current collision pages.");
        proposal.cells.insert(proposal.cells.end(),page.cells.begin(),page.cells.begin()+page.count);
        proposal.removed.insert(proposal.removed.end(),page.removed.begin(),page.removed.begin()+page.removed_count);
      }
      std::sort(proposal.removed.begin(),proposal.removed.end());
      if(std::adjacent_find(proposal.removed.begin(),proposal.removed.end())!=proposal.removed.end())
        throw std::runtime_error("Repeated current collision dependency.");
      proposals.push_back(std::move(proposal));
    }
    std::vector<unsigned long> hits;hits.reserve(collision->rows.size());
    for(const auto& proposal:proposals) {
      Id obstruction=0;bool blocked=false;
      for(size_t k=0;k<proposal.count;++k) {
        const auto& cell=proposal.cells[k];const auto box=CollisionBounds(cell);hits.clear();
        if(collision->tree)collision->tree->DetermineIntersectingElements(box.data(),box.data()+3,hits);
        auto check=[&](Id id) {
          if(blocked || std::binary_search(proposal.removed.begin(),proposal.removed.begin()+proposal.removed_count,id))return;
          const auto current=owned.find(id);if(current==owned.end())return;
          const auto held=CollisionBounds(current->second.cell);
          for(unsigned k=0;k<3;++k)if(box[k]>held[k+3] || held[k]>box[k+3])return;
          ++stats.collision_checks;
          if(!ConformingDisjoint(cell,current->second.cell)){obstruction=id;blocked=true;}
        };
        stats.collision_candidates+=hits.size()+collision->added.size();
        for(auto hit:hits) {
          const auto row=collision->rows.at(hit);const auto current=owned.find(row.first);
          if(current!=owned.end() && current->second.version==row.second)check(row.first);
        }
        for(Id id:collision->added)check(id);
      }
      if(blocked)veto.at(proposal.owner).push_back(obstruction);
    }
  }catch(const std::exception& e){failure.Set(2,0,e.what());}
  if(!Vote(failure,reason))return false;
  const auto rejected=world.exchange(veto,&transported,ceiling,held);
  if(!transported){reason="Native3D collision veto exchange exceeds storage budget.";return false;}
  if(!rejected_reason.empty())reason=rejected_reason;
  if(!rejected.empty()) {
    candidate=false;reason="Native3D projected candidate collides with current cell "+
        std::to_string(*std::min_element(rejected.begin(),rejected.end()))+".";
  }
  if(proposals.size()>1) {
    // A physically vetoed proposal must not exclude an otherwise admissible peer.
    std::vector<std::vector<int>> outgoing_eligible;
    try {outgoing_eligible.assign(world.size,candidate?std::vector<int>{world.rank}:std::vector<int>{});}
    catch(const std::exception& e){failure.Set(2,0,e.what());}
    if(!Vote(failure,reason))return false;
    auto eligible=world.exchange(outgoing_eligible,&transported,ceiling,held);
    if(!transported){reason="Native3D collision eligibility exchange exceeds storage budget.";return false;}
    try {
      std::sort(eligible.begin(),eligible.end());
      if(std::adjacent_find(eligible.begin(),eligible.end())!=eligible.end())
        throw std::runtime_error("Repeated current collision owner.");
      std::vector<const CollisionProposal*> ordered,selected;ordered.reserve(eligible.size());selected.reserve(eligible.size());
      for(int owner:eligible) {
        const auto proposal=std::find_if(proposals.begin(),proposals.end(),[&](const auto& p){return p.owner==owner;});
        if(proposal==proposals.end())throw std::runtime_error("Missing eligible collision proposal.");
        ordered.push_back(&*proposal);
      }
      std::sort(ordered.begin(),ordered.end(),[](const auto* a,const auto* b) {
        return std::tie(a->priority,a->owner)<std::tie(b->priority,b->owner);
      });
      for(const auto* proposal:ordered) {
        bool compatible=true;
        for(const auto* peer:selected) {
          for(size_t i=0;i<proposal->count && compatible;++i)for(size_t j=0;j<peer->count && compatible;++j) {
            const auto a=CollisionBounds(proposal->cells[i]),b=CollisionBounds(peer->cells[j]);bool separated=false;
            for(unsigned k=0;k<3;++k)separated=separated || a[k]>b[k+3] || b[k]>a[k+3];
            if(separated)continue;
            ++stats.collision_checks;
            compatible=ConformingDisjoint(proposal->cells[i],peer->cells[j]);
          }
          if(!compatible)break;
        }
        if(compatible)selected.push_back(proposal);
        else if(proposal->owner==world.rank) {
          candidate=false;if(deferred)*deferred=true;
          reason="Native3D projected candidate deferred by a conflicting peer.";
        }
      }
    }catch(const std::exception& e){failure.Set(2,0,e.what());}
    if(!Vote(failure,reason))return false;
  }
  return true;
}
bool Directory::Build(const Owned& owned, std::string& reason, size_t ceiling, size_t caller_bytes) {
  Timer timer(stats.build_seconds);
  ++stats.builds;
  CLocalFailure failure;
  if (initialized) failure.Set(2, 0, "Native 3D directory is already initialized; rebuild after migration in a new directory.");
  const unsigned mode=(track_changes?1u:0u)|(parameters?2u:0u);
  const auto mode_low=CPassiveComm::Allreduce(mode,CPassiveComm::Op::MIN,world.comm),mode_high=CPassiveComm::Allreduce(mode,CPassiveComm::Op::MAX,world.comm);
  if((mode_low&1u)!=(mode_high&1u))failure.Set(1,0,"Native 3D change-notification mode differs across workers.");
  if((mode_low&2u)!=(mode_high&2u))failure.Set(1,0,"Native 3D boundary parameter mode differs across workers.");
  if(parameters){parameter_geometry=&parameters->Geometry();parameter_geometry_bytes=transfer_memory::Add(parameters->Geometry().ResidentBytes(),parameters->Geometry().Reference().ResidentBytes());}
  std::vector<OwnedCell> added;
  const auto baseline = transfer_memory::Add(caller_bytes,ParameterBytes(), MapBytes<Owned>(owned.size()));
  const auto initial_peak = transfer_memory::Add(baseline, ResidentBytes(), transfer_memory::Mul(owned.size(), sizeof(OwnedCell)));
  stats.maximum_work_bytes = std::max(stats.maximum_work_bytes, initial_peak);
  if (initial_peak == SIZE_MAX || initial_peak > ceiling)
    failure.Set(3, 0, "Native 3D initial ownership exceeds directory budget.");
  if (!Vote(failure, reason)) { ++stats.rejected; return false; }
  try {
    added.reserve(owned.size());
    for (const auto& row : owned) {
      if (row.first != row.second.cell.id) throw std::runtime_error("Native 3D owned map key mismatch.");
      added.push_back(row.second);
    }
  } catch (const std::exception& e) { failure.Set(2, 0, e.what()); }
  if (!Vote(failure, reason)) { ++stats.rejected; return false; }
  if(parameters && !AgreeParameters(reason,ceiling,baseline)){++stats.rejected;return false;}
  auto prepared = Prepare({}, added, ceiling, baseline);
  reason = prepared.reason;
  if (!prepared.valid) { ++stats.rejected; return false; }
  Commit(std::move(prepared));
  initialized = true;
  return true;
}
Dependencies Directory::Lookup(const std::vector<Feature>& features, size_t ceiling, size_t caller_bytes) {
  Dependencies result;
  result.valid = LookupImpl(features, &result, nullptr, ceiling, caller_bytes, result.reason);
  if (!result.valid) result.cells.clear();
  return result;
}
DependencyBatch Directory::LookupBatch(const std::vector<Feature>& features, size_t ceiling, size_t caller_bytes, bool coordinate_stars) {
  DependencyBatch result;
  result.valid = LookupImpl(features, nullptr, &result, ceiling, caller_bytes, result.reason, nullptr, coordinate_stars);
  if (!result.valid) result.candidates.clear();
  return result;
}
bool Directory::LookupImpl(const std::vector<Feature>& features, Dependencies* merged, DependencyBatch* batch,
                           size_t ceiling, size_t caller_bytes, std::string& reason,
                           std::vector<std::vector<Id>>* footprints, bool coordinate_stars) {
  Timer timer(stats.lookup_seconds);
  ++stats.lookups;
  auto lookup=[&](auto example) {
  using LookupReply=decltype(example);
  const size_t cell_bound=coordinate_stars ? MaximumDirectoryScan : MaximumCavityCells;
  CLocalFailure failure;
  std::vector<std::vector<Query>> to;
  const auto baseline = transfer_memory::Add(caller_bytes, ResidentBytes(), transfer_memory::Bytes(features));
  const auto request_peak = transfer_memory::Add(baseline, transfer_memory::Mul(world.size, sizeof(std::vector<Query>)),
      transfer_memory::GrowthBound(features.size(), sizeof(Query)));
  stats.maximum_work_bytes = std::max(stats.maximum_work_bytes, request_peak);
  if (request_peak == SIZE_MAX || request_peak > ceiling) failure.Set(3, 0, "Native 3D feature packing exceeds budget.");
  if (!Vote(failure, reason)) { ++stats.rejected; return false; }
  try {
    if (features.size() > MaximumDirectoryFeatures) throw std::runtime_error("Native 3D feature query exceeds budget.");
    to.resize(world.size);
    for (unsigned i = 0; i < features.size(); ++i) {
      auto f = features[i];
      if (f.count < 1 || f.count > 3) throw std::runtime_error("Native 3D invalid dependency feature.");
      std::sort(f.vertices.begin(), f.vertices.begin() + f.count);
      if (std::adjacent_find(f.vertices.begin(), f.vertices.begin() + f.count) != f.vertices.begin() + f.count)
        throw std::runtime_error("Native 3D dependency repeats a vertex.");
      to[Rendezvous({false, f.vertices[0]})].push_back({f, world.rank, i, footprints!=nullptr, coordinate_stars && f.count==1});
    }
  } catch (const std::exception& e) { failure.Set(2, 0, e.what()); }
  if (!Vote(failure, reason)) { ++stats.rejected; return false; }
  bool admitted = false;
  const auto queries = world.exchange(to, &admitted, ceiling, baseline);
  if (!admitted) failure.Set(3, 0, "Native 3D feature transport exceeds budget.");
  std::vector<std::vector<LookupReply>> reply;
  const auto reply_peak = transfer_memory::Add(baseline, transfer_memory::Bytes(to), transfer_memory::Bytes(queries),
      transfer_memory::Mul(world.size, sizeof(std::vector<LookupReply>)),
      transfer_memory::GrowthBound(transfer_memory::Mul(queries.size(), cell_bound + 1), sizeof(LookupReply)),
      transfer_memory::GrowthBound(cell_bound + 1, sizeof(LookupReply)));
  stats.maximum_work_bytes = std::max(stats.maximum_work_bytes, reply_peak);
  if (reply_peak == SIZE_MAX || reply_peak > ceiling) failure.Set(3, 0, "Native 3D dependency replies exceed budget.");
  if (!Vote(failure, reason)) { ++stats.rejected; return false; }
  try {
    reply.resize(world.size);
    for (const auto& q : queries) {
      if(q.footprint!=std::is_same<LookupReply,FootprintReply>::value)
        throw std::runtime_error("Native 3D dependency footprint mode differs.");
      const auto found = stars.find({false, q.feature.vertices[0]});
      int status = found == stars.end() ? 2 : found->second.size() > MaximumDirectoryScan ? 3 : 1;
      std::vector<LookupReply> matches;
      if (status == 1)
        for (const auto& row : found->second) {
          ++stats.metadata_scanned;
          bool contains = true;
          for (unsigned i = 0; i < q.feature.count; ++i)
            contains = contains && std::binary_search(row.second.nodes.begin(), row.second.nodes.end(), q.feature.vertices[i]);
          if (contains) {
            LookupReply match{row.second.dependency,q.ticket,q.footprint?4:0};
            if constexpr(std::is_same<LookupReply,FootprintReply>::value)match.nodes=row.second.nodes;
            matches.push_back(match);
            if (matches.size() > (q.coordinate_star ? MaximumDirectoryScan : MaximumCavityCells)) { status = 3; break; }
          }
        }
      if (status == 1 && matches.empty()) status = 2;
      if (status == 1) reply[q.caller].insert(reply[q.caller].end(), matches.begin(), matches.end());
      reply[q.caller].push_back({{}, q.ticket, status});
    }
  } catch (const std::exception& e) { failure.Set(2, 0, e.what()); }
  if (!Vote(failure, reason)) { ++stats.rejected; return false; }
  const auto received = world.exchange(reply, &admitted, ceiling,
      transfer_memory::Add(baseline, transfer_memory::Bytes(to), transfer_memory::Bytes(queries)));
  if (!admitted) failure.Set(3, 0, "Native 3D dependency import exceeds budget.");
  const auto groups = batch ? features.size() : size_t(1);
  const auto dependency_bound = transfer_memory::Mul(groups, cell_bound);
  const auto import_peak = transfer_memory::Add(baseline, transfer_memory::Bytes(to), transfer_memory::Bytes(queries),
      transfer_memory::Bytes(reply), transfer_memory::Bytes(received), MapBytes<std::map<Id, Dependency>>(transfer_memory::Add(dependency_bound,groups)),
      batch ? MapBytes<std::map<Id,Dependency>>(dependency_bound) : 0,
      transfer_memory::GrowthBound(dependency_bound, sizeof(Dependency)),
      transfer_memory::Mul(groups, sizeof(std::map<Id,Dependency>) + sizeof(Dependencies)),
      transfer_memory::Mul(features.size(), sizeof(int)),
      footprints ? transfer_memory::Add(transfer_memory::Mul(groups,sizeof(std::vector<Id>)),
          transfer_memory::GrowthBound(transfer_memory::Mul(4,dependency_bound),sizeof(Id)),
          MapBytes<std::map<Id,std::array<Id,4>>>(dependency_bound)) : 0);
  stats.maximum_work_bytes = std::max(stats.maximum_work_bytes, import_peak);
  if (import_peak == SIZE_MAX || import_peak > ceiling) failure.Set(3, 0, "Native 3D dependency validation exceeds budget.");
  if (!Vote(failure, reason)) { ++stats.rejected; return false; }
  try {
    std::map<Id,Dependency> merged_unique, authority;
    std::vector<std::map<Id,Dependency>> unique;
    if (batch) unique.resize(groups);
    std::vector<int> footer(features.size(), 0);
    std::map<Id,std::array<Id,4>> authority_nodes;
    if(footprints)footprints->resize(groups);
    for (const auto& row : received) {
      if (row.ticket >= features.size()) throw std::runtime_error("Native 3D invalid dependency reply ticket.");
      if (row.status!=0 && row.status!=4) {
        if (row.status < 1 || row.status > 3 || footer[row.ticket])
          throw std::runtime_error("Native 3D invalid or duplicate dependency footer.");
        if (merged && row.status != 1) throw std::runtime_error(row.status == 3 ?
            "Native 3D dependency star exceeds budget." : "Native 3D missing dependency feature.");
        footer[row.ticket] = row.status;
      } else {
        if(bool(footprints)!=(row.status==4))throw std::runtime_error("Native 3D dependency footprint mode differs.");
        if constexpr(std::is_same<LookupReply,FootprintReply>::value) {
          if(!std::is_sorted(row.nodes.begin(),row.nodes.end()) ||
              std::adjacent_find(row.nodes.begin(),row.nodes.end())!=row.nodes.end())
            throw std::runtime_error("Native 3D dependency footprint repeats or disorders vertices.");
          for(unsigned k=0;k<features[row.ticket].count;++k)
            if(!std::binary_search(row.nodes.begin(),row.nodes.end(),features[row.ticket].vertices[k]))
              throw std::runtime_error("Native 3D dependency footprint lacks its requested feature.");
          const auto old=authority_nodes.emplace(row.dependency.cell,row.nodes);
          if(!old.second && old.first->second!=row.nodes)throw std::runtime_error("Native 3D inconsistent dependency footprint.");
          auto& vertices=footprints->at(row.ticket);vertices.insert(vertices.end(),row.nodes.begin(),row.nodes.end());
        }
        if (batch) {
          const auto old = authority.emplace(row.dependency.cell,row.dependency);
          if (!old.second && (old.first->second.version != row.dependency.version || old.first->second.owner != row.dependency.owner))
            throw std::runtime_error("Native 3D inconsistent dependency ownership across candidates.");
        }
        auto& group = batch ? unique[row.ticket] : merged_unique;
        const auto old = group.emplace(row.dependency.cell, row.dependency);
        if (!old.second && (old.first->second.version != row.dependency.version || old.first->second.owner != row.dependency.owner))
          throw std::runtime_error("Native 3D inconsistent dependency ownership.");
        if (group.size() > (batch && coordinate_stars && features[row.ticket].count==1 ? MaximumDirectoryScan : MaximumCavityCells)) throw std::runtime_error("Native 3D dependency union exceeds budget.");
      }
    }
    if (std::find(footer.begin(), footer.end(), 0) != footer.end()) throw std::runtime_error("Native 3D incomplete dependency footer.");
    if (batch) batch->candidates.resize(features.size());
    for (size_t i = 0; i < groups; ++i) {
      auto& candidate = batch ? batch->candidates[i] : *merged;
      const auto& group = batch ? unique[i] : merged_unique;
      candidate.valid = !batch || footer[i] == 1;
      if(batch && coordinate_stars && features[i].count==1)candidate.coordinate_star=features[i];
      candidate.oversized = batch && footer[i] == 3;
      if (!candidate.valid) {
        if (!group.empty()) throw std::runtime_error("Native 3D refused star has unexpected dependencies.");
        candidate.reason = footer[i] == 3 ? "Native 3D dependency star exceeds budget." : "Native 3D missing dependency feature.";
      } else {
        if (batch && group.empty()) throw std::runtime_error("Native 3D complete star has no dependencies.");
        for (const auto& row : group) candidate.cells.push_back(row.second);
      }
      if(footprints) {
        auto& vertices=footprints->at(i);std::sort(vertices.begin(),vertices.end());
        vertices.erase(std::unique(vertices.begin(),vertices.end()),vertices.end());
      }
      stats.maximum_dependencies = std::max(stats.maximum_dependencies, candidate.cells.size());
    }
  } catch (const std::exception& e) { failure.Set(2, 0, e.what()); }
  const bool valid = Vote(failure, reason);
  if (!valid) ++stats.rejected;
  return valid;
  };
  return footprints?lookup(FootprintReply{}):lookup(Reply{});
}
Imported Directory::Fetch(const Dependencies& dependencies, const Owned& owned, size_t ceiling, size_t caller_bytes) {
  Timer timer(stats.fetch_seconds);
  ++stats.fetches;
  Imported result;
  CLocalFailure failure;
  std::vector<std::vector<FetchQuery>> to;
  const auto baseline = transfer_memory::Add(caller_bytes, ResidentBytes(), MapBytes<Owned>(owned.size()),
      transfer_memory::Bytes(dependencies.cells));
  const auto request_peak = transfer_memory::Add(baseline, transfer_memory::Mul(world.size, sizeof(std::vector<FetchQuery>)),
      transfer_memory::GrowthBound(dependencies.cells.size(), sizeof(FetchQuery)), MapBytes<std::set<Id>>(dependencies.cells.size()));
  stats.maximum_work_bytes = std::max(stats.maximum_work_bytes, request_peak);
  if (request_peak == SIZE_MAX || request_peak > ceiling) failure.Set(3, 0, "Native 3D payload request packing exceeds budget.");
  if (!Vote(failure, result.reason)) { ++stats.rejected; return result; }
  try {
    if (!dependencies.valid || dependencies.coordinate_star.count>1 ||
        dependencies.cells.size() > (dependencies.coordinate_star.count ? MaximumDirectoryScan : MaximumCavityCells)) throw std::runtime_error("Native 3D dependencies were not admitted.");
    to.resize(world.size);
    std::set<Id> unique;
    for (const auto& row : dependencies.cells) {
      if (row.owner < 0 || row.owner >= world.size || !unique.insert(row.cell).second) throw std::runtime_error("Native 3D invalid/duplicate dependency owner.");
      to[row.owner].push_back({row, world.rank});
    }
  } catch (const std::exception& e) { failure.Set(2, 0, e.what()); }
  if (!Vote(failure, result.reason)) { ++stats.rejected; return result; }
  bool admitted = false;
  const auto queries = world.exchange(to, &admitted, ceiling, baseline);
  if (!admitted) failure.Set(3, 0, "Native 3D payload requests exceed budget.");
  auto peak = transfer_memory::Add(baseline, transfer_memory::Bytes(to), transfer_memory::Bytes(queries),
      transfer_memory::Mul(world.size, sizeof(std::vector<Payload>)), transfer_memory::GrowthBound(queries.size(), sizeof(Payload)));
  stats.maximum_work_bytes = std::max(stats.maximum_work_bytes, peak);
  if (peak == SIZE_MAX || peak > ceiling) failure.Set(3, 0, "Native 3D payload packing exceeds budget.");
  if (!Vote(failure, result.reason)) { ++stats.rejected; return result; }
  std::vector<std::vector<Payload>> reply;
  try {
    reply.resize(world.size);
    for (const auto& q : queries) {
      const auto row = owned.find(q.dependency.cell);
      const bool present = row != owned.end() && row->first == row->second.cell.id && row->second.version == q.dependency.version;
      reply[q.caller].push_back({present ? row->second : OwnedCell{}, present});
    }
  } catch (const std::exception& e) { failure.Set(2, 0, e.what()); }
  if (!Vote(failure, result.reason)) { ++stats.rejected; return result; }
  const auto received = world.exchange(reply, &admitted, ceiling,
      transfer_memory::Add(baseline, transfer_memory::Bytes(to), transfer_memory::Bytes(queries)));
  if (!admitted) failure.Set(3, 0, "Native 3D payload import exceeds budget.");
  peak = transfer_memory::Add(baseline, transfer_memory::Bytes(to), transfer_memory::Bytes(queries),
      transfer_memory::Bytes(reply), transfer_memory::Bytes(received), MapBytes<std::map<Id, uint64_t>>(dependencies.cells.size()),
      transfer_memory::GrowthBound(dependencies.cells.size(), sizeof(OwnedCell)));
  stats.maximum_work_bytes = std::max(stats.maximum_work_bytes, peak);
  if (peak == SIZE_MAX || peak > ceiling) failure.Set(3, 0, "Native 3D payload validation exceeds budget.");
  if (!Vote(failure, result.reason)) { ++stats.rejected; return result; }
  try {
    std::map<Id, uint64_t> expected;
    for (const auto& row : dependencies.cells) expected.emplace(row.cell, row.version);
    if (received.size() != expected.size()) throw std::runtime_error("Native 3D missing dependency payload.");
    for (const auto& row : received) {
      const auto found = expected.find(row.cell.cell.id);
      if (!row.present || found == expected.end() || found->second != row.cell.version) throw std::runtime_error("Native 3D stale dependency payload.");
      if(dependencies.coordinate_star.count &&
          std::none_of(row.cell.cell.v.begin(),row.cell.cell.v.end(),[&](Node n){return n.id==dependencies.coordinate_star.vertices[0];}))
        throw std::runtime_error("Native 3D coordinate dependency is not incident to its vertex.");
      expected.erase(found);
      result.cells.push_back(row.cell);
    }
    std::sort(result.cells.begin(), result.cells.end(), [](const auto& a, const auto& b) { return a.cell.id < b.cell.id; });
    result.coordinate_star=dependencies.coordinate_star;
    stats.payloads += result.cells.size();
  } catch (const std::exception& e) { failure.Set(2, 0, e.what()); }
  result.valid = Vote(failure, result.reason);
  if (!result.valid) { result.cells.clear(); ++stats.rejected; }
  return result;
}
ImportedBatch Directory::FetchBatch(const DependencyBatch& dependencies, const Owned& owned,
                                     size_t ceiling, size_t caller_bytes) {
  Timer timer(stats.fetch_seconds);
  ++stats.fetches;
  ImportedBatch result;
  CLocalFailure failure;
  size_t count=0,max_candidate_cells=0, dependency_bytes=transfer_memory::Bytes(dependencies.candidates);
  for (const auto& candidate:dependencies.candidates) {
    count=transfer_memory::Add(count,candidate.cells.size());
    dependency_bytes=transfer_memory::Add(dependency_bytes,transfer_memory::Bytes(candidate.cells),
        transfer_memory::Mul(2,transfer_memory::Bytes(candidate.reason)));
    if(candidate.valid)max_candidate_cells=std::max(max_candidate_cells,candidate.cells.size());
  }
  for(const auto& candidate:dependencies.candidates)if(candidate.coordinate_star.count>1)
    failure.Set(2,0,"Native 3D invalid batch coordinate-star identity.");
  const auto baseline=transfer_memory::Add(caller_bytes,ResidentBytes(),MapBytes<Owned>(owned.size()),
      dependency_bytes,transfer_memory::Mul(dependencies.candidates.size(),sizeof(Imported)+128));
  auto peak=transfer_memory::Add(baseline,MapBytes<std::map<Id,Dependency>>(count),
      MapBytes<std::set<Id>>(max_candidate_cells),transfer_memory::Mul(world.size,sizeof(std::vector<FetchQuery>)),
      transfer_memory::GrowthBound(count,sizeof(FetchQuery)));
  stats.maximum_work_bytes=std::max(stats.maximum_work_bytes,peak);
  if (peak==SIZE_MAX || peak>ceiling) failure.Set(3,0,"Native 3D batch payload packing exceeds budget.");
  if (!Vote(failure,result.reason)) { ++stats.rejected; return result; }
  std::map<Id,Dependency> expected;
  std::vector<std::vector<FetchQuery>> to;
  try {
    if (!dependencies.valid || dependencies.candidates.size()>MaximumDirectoryFeatures)
      throw std::runtime_error("Native 3D batch dependencies were not admitted.");
    result.candidates.resize(dependencies.candidates.size());
    to.resize(world.size);
    for (size_t i=0;i<dependencies.candidates.size();++i) {
      const auto& candidate=dependencies.candidates[i];
      result.candidates[i].reason=candidate.reason;
      if (!candidate.valid) continue;
      const size_t limit=candidate.coordinate_star.count?MaximumDirectoryScan:MaximumCavityCells;
      if (candidate.cells.empty() || candidate.cells.size()>limit)
        throw std::runtime_error("Native 3D batch candidate lacks a bounded complete star.");
      std::set<Id> unique;
      for (const auto& row:candidate.cells) {
        if (row.owner<0 || row.owner>=world.size || !unique.insert(row.cell).second)
          throw std::runtime_error("Native 3D invalid/duplicate batch dependency.");
        const auto inserted=expected.emplace(row.cell,row);
        if (!inserted.second && (inserted.first->second.owner!=row.owner || inserted.first->second.version!=row.version))
          throw std::runtime_error("Native 3D inconsistent shared batch dependency.");
        if (inserted.second) to[row.owner].push_back({row,world.rank});
      }
    }
  } catch (const std::exception& e) { failure.Set(2,0,e.what()); }
  if (!Vote(failure,result.reason)) { result.candidates.clear(); ++stats.rejected; return result; }
  bool admitted=false;
  const auto retained=transfer_memory::Add(baseline,MapBytes<std::map<Id,Dependency>>(expected.size()));
  const auto queries=world.exchange(to,&admitted,ceiling,retained);
  if (!admitted) failure.Set(3,0,"Native 3D batch payload requests exceed budget.");
  peak=transfer_memory::Add(retained,transfer_memory::Bytes(to),transfer_memory::Bytes(queries),
      transfer_memory::Mul(world.size,sizeof(std::vector<BatchPayload>)),
      transfer_memory::GrowthBound(queries.size(),sizeof(BatchPayload)));
  stats.maximum_work_bytes=std::max(stats.maximum_work_bytes,peak);
  if (peak==SIZE_MAX || peak>ceiling) failure.Set(3,0,"Native 3D batch payload replies exceed budget.");
  if (!Vote(failure,result.reason)) { result.candidates.clear(); ++stats.rejected; return result; }
  std::vector<std::vector<BatchPayload>> reply;
  try {
    reply.resize(world.size);
    for (const auto& q:queries) {
      if (q.caller<0 || q.caller>=world.size || q.dependency.owner!=world.rank)
        throw std::runtime_error("Native 3D invalid batch payload routing.");
      const auto found=owned.find(q.dependency.cell);
      const bool present=found!=owned.end() && found->first==found->second.cell.id && found->second.version==q.dependency.version;
      reply[q.caller].push_back({q.dependency.cell,present ? found->second : OwnedCell{},present});
    }
  } catch (const std::exception& e) { failure.Set(2,0,e.what()); }
  if (!Vote(failure,result.reason)) { result.candidates.clear(); ++stats.rejected; return result; }
  const auto received=world.exchange(reply,&admitted,ceiling,
      transfer_memory::Add(retained,transfer_memory::Bytes(to),transfer_memory::Bytes(queries)));
  if (!admitted) failure.Set(3,0,"Native 3D batch payload import exceeds budget.");
  peak=transfer_memory::Add(retained,transfer_memory::Bytes(to),transfer_memory::Bytes(queries),
      transfer_memory::Bytes(reply),transfer_memory::Bytes(received),MapBytes<std::map<Id,size_t>>(expected.size()),
      transfer_memory::GrowthBound(count,sizeof(OwnedCell)));
  stats.maximum_work_bytes=std::max(stats.maximum_work_bytes,peak);
  if (peak==SIZE_MAX || peak>ceiling) failure.Set(3,0,"Native 3D batch payload validation exceeds budget.");
  if (!Vote(failure,result.reason)) { result.candidates.clear(); ++stats.rejected; return result; }
  try {
    if (received.size()!=expected.size()) throw std::runtime_error("Native 3D missing batch dependency payload.");
    std::map<Id,size_t> positions;
    for (size_t i=0;i<received.size();++i) {
      const auto& row=received[i];
      const auto found=expected.find(row.requested);
      if (found==expected.end() || !positions.emplace(row.requested,i).second)
        throw std::runtime_error("Native 3D unexpected/duplicate batch payload.");
      if (row.present) {
        if (row.cell.cell.id!=row.requested || row.cell.version!=found->second.version)
          throw std::runtime_error("Native 3D inconsistent batch payload identity.");
        Validate(row.cell); ++stats.payloads;
      }
    }
    for (size_t i=0;i<dependencies.candidates.size();++i) {
      const auto& candidate=dependencies.candidates[i];
      auto& imported=result.candidates[i];
      if (!candidate.valid) continue;
      imported.valid=true;
      imported.coordinate_star=candidate.coordinate_star;
      for (const auto& dependency:candidate.cells) {
        const auto& row=received[positions.at(dependency.cell)];
        if (!row.present) { imported.valid=false; imported.reason="Native 3D stale batch dependency payload."; break; }
        imported.cells.push_back(row.cell);
      }
      if (!imported.valid) imported.cells.clear();
      else {
        std::sort(imported.cells.begin(),imported.cells.end(),[](const auto& a,const auto& b){return a.cell.id<b.cell.id;});
        if(imported.coordinate_star.count && std::any_of(imported.cells.begin(),imported.cells.end(),[&](const OwnedCell& row){
            return std::none_of(row.cell.v.begin(),row.cell.v.end(),[&](Node node){return node.id==imported.coordinate_star.vertices[0];});}))
          throw std::runtime_error("Native 3D batch coordinate payload is not incident to its vertex.");
      }
    }
  } catch (const std::exception& e) { failure.Set(2,0,e.what()); }
  result.valid=Vote(failure,result.reason);
  if (!result.valid) { result.candidates.clear(); ++stats.rejected; }
  return result;
}
DependencyBatch Directory::PlanBatch(const std::vector<Feature>& features, std::vector<size_t>& slots,
                                     size_t ceiling, size_t caller_bytes) {
  DependencyBatch result;std::vector<std::vector<Id>> footprints;slots.clear();
  result.valid=LookupImpl(features,nullptr,&result,ceiling,caller_bytes,result.reason,&footprints);
  if(!result.valid){result.candidates.clear();return result;}
  CLocalFailure failure;size_t dependencies=transfer_memory::Bytes(result.candidates);
  for(const auto& candidate:result.candidates) {
    dependencies=transfer_memory::Add(dependencies,transfer_memory::Bytes(candidate.cells),transfer_memory::Bytes(candidate.reason));
    // An oversized complete star has an empty footprint, is never elected and is reported for blocking.
    if(!candidate.valid && !candidate.oversized)failure.Set(1,0,candidate.reason);
  }
  const auto compact=transfer_memory::Mul(features.size(),3*sizeof(size_t)+sizeof(Dependencies));
  const auto baseline=transfer_memory::Add(caller_bytes,transfer_memory::Bytes(features),dependencies,compact);
  const auto peak=transfer_memory::Add(baseline,ResidentBytes());
  stats.maximum_work_bytes=std::max(stats.maximum_work_bytes,peak);
  if(peak==SIZE_MAX || peak>ceiling)failure.Set(3,0,"Native 3D planned dependencies exceed budget.");
  if(!Vote(failure,result.reason)){result.valid=false;result.candidates.clear();++stats.rejected;return result;}
  const auto election=ChooseBatch(footprints,ceiling,baseline);
  if(!election.valid){result.valid=false;result.reason=election.reason;result.candidates.clear();return result;}
  try {
    std::vector<Dependencies> chosen;chosen.reserve(features.size());slots.reserve(features.size());
    result.refused.reserve(features.size());result.pass_deferred.reserve(features.size());
    for(size_t i=0;i<election.chosen.size();++i) {
      if(result.candidates[i].oversized){result.refused.push_back(i);continue;}
      if(election.pass_deferred[i]){result.pass_deferred.push_back(i);continue;}
      if(election.chosen[i]){slots.push_back(i);chosen.push_back(std::move(result.candidates[i]));}
    }
    result.candidates.swap(chosen);
  }catch(const std::exception& e){failure.Set(2,0,e.what());}
  result.valid=Vote(failure,result.reason);
  if(!result.valid){slots.clear();result.candidates.clear();result.refused.clear();result.pass_deferred.clear();++stats.rejected;}
  return result;
}
ReservationBatch Directory::ReserveBatch(const ImportedBatch& imported, size_t ceiling, size_t caller_bytes,
                                          const std::vector<unsigned>& priorities) {
  return ReserveBatchImpl(imported,ceiling,caller_bytes,false,priorities);
}
ReservationBatch Directory::ReserveInteriors(const ImportedBatch& imported,size_t ceiling,size_t caller_bytes) {
  return ReserveBatchImpl(imported,ceiling,caller_bytes,true);
}
ReservationBatch Directory::ReserveBatchImpl(const ImportedBatch& imported,size_t ceiling,size_t caller_bytes,bool interiors,
                                               const std::vector<unsigned>& priorities) {
  Timer timer(stats.reservation_seconds);++stats.reservations;
  CLocalFailure failure;size_t cells=0,imported_bytes=transfer_memory::Add(transfer_memory::Bytes(imported.candidates),
      transfer_memory::Bytes(priorities));bool coordinate_stars=false;
  for(const auto& candidate:imported.candidates) {
    cells=transfer_memory::Add(cells,candidate.cells.size());
    imported_bytes=transfer_memory::Add(imported_bytes,transfer_memory::Bytes(candidate.cells),transfer_memory::Bytes(candidate.reason));
    if(candidate.coordinate_star.count>1 || (interiors && candidate.coordinate_star.count))
      failure.Set(2,0,"Native 3D batch reservation has an invalid coordinate-star scope.");
    coordinate_stars=coordinate_stars||candidate.coordinate_star.count==1;
  }
  if(!priorities.empty()&&priorities.size()!=imported.candidates.size())
    failure.Set(2,0,"Native 3D batch reservation priorities do not match the candidate table.");
  const auto scratch=transfer_memory::Add(transfer_memory::Mul(imported.candidates.size(),sizeof(std::vector<Id>)),
      transfer_memory::GrowthBound(transfer_memory::Mul(4,cells),sizeof(Id)));
  const auto baseline=transfer_memory::Add(caller_bytes,imported_bytes);
  auto peak=transfer_memory::Add(baseline,ResidentBytes(),scratch);stats.maximum_work_bytes=std::max(stats.maximum_work_bytes,peak);
  if(peak==SIZE_MAX || peak>ceiling)failure.Set(3,0,"Native 3D batch reservation packing exceeds budget.");
  ReservationBatch result;
  if(!Vote(failure,result.reason)){++stats.rejected;return result;}
  std::vector<std::vector<Id>> footprints;
  try {
    if(!imported.valid || imported.candidates.size()>MaximumDirectoryFeatures)
      throw std::runtime_error("Native 3D batch reservation lacks admitted dependencies.");
    footprints.resize(imported.candidates.size());
    for(size_t i=0;i<imported.candidates.size();++i) {
      const auto& candidate=imported.candidates[i];if(!candidate.valid)continue;
      const size_t limit=candidate.coordinate_star.count?MaximumDirectoryScan:MaximumCavityCells;
      if(candidate.cells.empty() || candidate.cells.size()>limit)
        throw std::runtime_error("Native 3D batch reservation lacks a bounded complete star.");
      auto& vertices=footprints[i];vertices.reserve(4*candidate.cells.size());
      if(interiors) {
        if(candidate.cells.size()!=1)throw std::runtime_error("Native 3D interior reservation requires one complete cell.");
        vertices.push_back(candidate.cells.front().cell.id);
      } else for(const auto& row:candidate.cells) {
        if(candidate.coordinate_star.count && std::none_of(row.cell.v.begin(),row.cell.v.end(),[&](Node node){
            return node.id==candidate.coordinate_star.vertices[0];}))
          throw std::runtime_error("Native 3D coordinate reservation contains a foreign cell.");
        for(auto node:row.cell.v)vertices.push_back(node.id);
      }
      std::sort(vertices.begin(),vertices.end());vertices.erase(std::unique(vertices.begin(),vertices.end()),vertices.end());
    }
  }catch(const std::exception& e){failure.Set(2,0,e.what());}
  if(!Vote(failure,result.reason)){++stats.rejected;return result;}
  return ChooseBatch(footprints,ceiling,baseline,!interiors,priorities,coordinate_stars);
}
ReservationBatch Directory::ChooseBatch(const std::vector<std::vector<Id>>& footprints, size_t ceiling, size_t caller_bytes,
                                          bool vertex_keys,const std::vector<unsigned>& priorities,bool coordinate_stars) {
  ReservationBatch result;CLocalFailure failure;size_t vertices=0,footprint_bytes=transfer_memory::Bytes(footprints);
  if(!priorities.empty()&&priorities.size()!=footprints.size())failure.Set(2,0,"Native 3D batch priority table does not match candidate footprints.");
  for(const auto& row:footprints){vertices=transfer_memory::Add(vertices,row.size());footprint_bytes=transfer_memory::Add(footprint_bytes,transfer_memory::Bytes(row));}
  const auto baseline=transfer_memory::Add(caller_bytes,ResidentBytes(),footprint_bytes,transfer_memory::Mul(transfer_memory::Add(footprints.size(),64),2*sizeof(bool)));
  auto peak=transfer_memory::Add(baseline,transfer_memory::Mul(world.size,sizeof(std::vector<BatchClaim>)),
      transfer_memory::GrowthBound(vertices,sizeof(BatchClaim)));stats.maximum_work_bytes=std::max(stats.maximum_work_bytes,peak);
  if(peak==SIZE_MAX || peak>ceiling)failure.Set(3,0,"Native 3D candidate footprint claims exceed budget.");
  if(!Vote(failure,result.reason)){result.chosen.clear();result.pass_deferred.clear();++stats.rejected;return result;}
  std::vector<std::vector<BatchClaim>> to;
  try {
    if(footprints.size()>MaximumDirectoryFeatures)throw std::runtime_error("Native 3D candidate footprint count exceeds budget.");
    to.resize(world.size);result.chosen.resize(footprints.size(),false);result.pass_deferred.resize(footprints.size(),false);
    for(size_t i=0;i<footprints.size();++i) {
      const auto& row=footprints[i];if(row.empty())continue;
      const auto limit=coordinate_stars?4*MaximumDirectoryScan:4*MaximumCavityCells;
      if(row.size()>limit || !std::is_sorted(row.begin(),row.end()) || std::adjacent_find(row.begin(),row.end())!=row.end())
        throw std::runtime_error("Native 3D candidate footprint is not bounded and unique.");
      const unsigned priority=priorities.empty()?0:priorities[i];
      for(Id vertex:row)to[Rendezvous({false,vertex})].push_back({vertex,world.rank,unsigned(i),priority});
      result.chosen[i]=true;
    }
  }catch(const std::exception& e){failure.Set(2,0,e.what());}
  if(!Vote(failure,result.reason)){result.chosen.clear();result.pass_deferred.clear();++stats.rejected;return result;}
  bool admitted=false;
  const auto claims=world.exchange(to,&admitted,ceiling,baseline);
  if (!admitted) failure.Set(3,0,"Native 3D batch reservation transport exceeds budget.");
  using Winner=std::tuple<unsigned,int,unsigned>;
  using Winners=std::map<Id,Winner>;
  peak=transfer_memory::Add(baseline,transfer_memory::Bytes(to),transfer_memory::Bytes(claims),
      MapBytes<Winners>(claims.size()),transfer_memory::Mul(world.size,sizeof(std::vector<BatchVeto>)),
      transfer_memory::GrowthBound(claims.size(),sizeof(BatchVeto)));
  stats.maximum_work_bytes=std::max(stats.maximum_work_bytes,peak);
  if (peak==SIZE_MAX || peak>ceiling) failure.Set(3,0,"Native 3D batch reservation replies exceed budget.");
  if (!Vote(failure,result.reason)) { result.chosen.clear();result.pass_deferred.clear(); ++stats.rejected; return result; }
  std::vector<std::vector<BatchVeto>> reply;
  try {
    reply.resize(world.size);
    Winners winner;
    for (const auto& row:claims) {
      if (row.caller<0 || row.caller>=world.size || row.ticket>=MaximumDirectoryFeatures)
        throw std::runtime_error("Native 3D invalid batch reservation claim.");
      const auto priority=std::make_tuple(row.priority,row.caller,row.ticket);
      const auto inserted=winner.emplace(row.vertex,priority);
      if (!inserted.second) inserted.first->second=std::min(inserted.first->second,priority);
    }
    for (const auto& row:claims) {
      const bool held=construction_pass && vertex_keys && pass_vertices.count(row.vertex);
      if(held || winner.at(row.vertex)!=std::make_tuple(row.priority,row.caller,row.ticket))reply[row.caller].push_back({row.ticket,held});
    }
  } catch (const std::exception& e) { failure.Set(2,0,e.what()); }
  if (!Vote(failure,result.reason)) { result.chosen.clear();result.pass_deferred.clear(); ++stats.rejected; return result; }
  const auto vetoes=world.exchange(reply,&admitted,ceiling,
      transfer_memory::Add(baseline,transfer_memory::Bytes(to),transfer_memory::Bytes(claims)));
  if (!admitted) failure.Set(3,0,"Native 3D batch reservation replies exceed budget.");
  try {
    for (const auto& veto:vetoes) {
      if (veto.ticket>=result.chosen.size()) throw std::runtime_error("Native 3D invalid batch reservation veto.");
      result.chosen[veto.ticket]=false;result.pass_deferred[veto.ticket]=result.pass_deferred[veto.ticket]||veto.pass_deferred;
    }
  } catch (const std::exception& e) { failure.Set(2,0,e.what()); }
  result.valid=Vote(failure,result.reason);
  if (!result.valid) { result.chosen.clear();result.pass_deferred.clear(); ++stats.rejected; }
  return result;
}
bool Directory::Reserve(const Imported& imported, bool active, std::string& reason, size_t ceiling, size_t caller_bytes,unsigned priority) {
  Timer timer(stats.reservation_seconds);
  ++stats.reservations;last_pass_deferred=false;
  CLocalFailure failure;
  std::vector<std::vector<Claim>> to;
  const auto baseline = transfer_memory::Add(caller_bytes, ResidentBytes(), transfer_memory::Bytes(imported.cells));
  const auto vertices = active ? transfer_memory::Mul(4, imported.cells.size()) : 0;
  auto peak = transfer_memory::Add(baseline, transfer_memory::Mul(world.size, sizeof(std::vector<Claim>)),
      MapBytes<std::set<Id>>(vertices), transfer_memory::GrowthBound(vertices, sizeof(Claim)));
  stats.maximum_work_bytes = std::max(stats.maximum_work_bytes, peak);
  if (peak == SIZE_MAX || peak > ceiling) failure.Set(3, 0, "Native 3D reservation packing exceeds budget.");
  if (!Vote(failure, reason)) { ++stats.rejected; return false; }
  try {
    if (active && (!imported.valid || imported.cells.empty() || imported.coordinate_star.count>1 ||
        imported.cells.size() > (imported.coordinate_star.count ? MaximumDirectoryScan : MaximumCavityCells)))
      throw std::runtime_error("Native 3D reservation lacks admitted dependencies.");
    to.resize(world.size);
    if (active) {
      std::set<Id> vertices;
      for (const auto& row : imported.cells) {
        if(imported.coordinate_star.count &&
            std::none_of(row.cell.v.begin(),row.cell.v.end(),[&](Node n){return n.id==imported.coordinate_star.vertices[0];}))
          throw std::runtime_error("Native 3D coordinate reservation contains a foreign cell.");
        for (auto node : row.cell.v) vertices.insert(node.id);
      }
      for (auto id : vertices) to[Rendezvous({false, id})].push_back({id, world.rank,priority});
    }
  } catch (const std::exception& e) { failure.Set(2, 0, e.what()); }
  if (!Vote(failure, reason)) { ++stats.rejected; return false; }
  bool admitted = false;
  const auto claims = world.exchange(to, &admitted, ceiling, baseline);
  if (!admitted) failure.Set(3, 0, "Native 3D reservation transport exceeds budget.");
  std::vector<std::vector<Veto>> reply;
  peak = transfer_memory::Add(baseline, transfer_memory::Bytes(to), transfer_memory::Bytes(claims),
      transfer_memory::Mul(world.size, sizeof(std::vector<Veto>)), MapBytes<std::map<Id, std::pair<unsigned,int>>>(claims.size()),
      transfer_memory::GrowthBound(claims.size(), sizeof(Veto)));
  stats.maximum_work_bytes = std::max(stats.maximum_work_bytes, peak);
  if (peak == SIZE_MAX || peak > ceiling) failure.Set(3, 0, "Native 3D reservation replies exceed budget.");
  if (!Vote(failure, reason)) { ++stats.rejected; return false; }
  try {
    reply.resize(world.size);
    std::map<Id, std::pair<unsigned,int>> winner;
    for (const auto& row : claims) {
      const auto old = winner.find(row.vertex);
      const auto order=std::make_pair(row.priority,row.caller);
      if (old == winner.end()) winner.emplace(row.vertex, order);
      else old->second = std::min(old->second, order);
    }
    for (const auto& row : claims) {
      const bool held=construction_pass && pass_vertices.count(row.vertex);
      if(held || winner.at(row.vertex).second!=row.caller)reply[row.caller].push_back({row.caller,held});
    }
  } catch (const std::exception& e) { failure.Set(2, 0, e.what()); }
  if (!Vote(failure, reason)) { ++stats.rejected; return false; }
  const auto veto = world.exchange(reply, &admitted, ceiling,
      transfer_memory::Add(baseline, transfer_memory::Bytes(to), transfer_memory::Bytes(claims)));
  if (!admitted) { reason = "Native 3D reservation replies exceed budget."; ++stats.rejected; return false; }
  for(const auto& row:veto)last_pass_deferred=last_pass_deferred||row.pass_deferred;
  if (!veto.empty()) { reason = last_pass_deferred?"Native 3D cavity deferred to the next construction pass.":"Native 3D conflicting dependency reservation."; ++stats.rejected; return false; }
  reason.clear();
  return active;
}
size_t Directory::PreparedBytes(const Prepared& prepared) const {
  size_t bytes=transfer_memory::Add(MapBytes<Stars>(prepared.changed.size()),
      MapBytes<std::set<Id>>(prepared.pass_vertices.size()));
  for(const auto& star:prepared.changed)bytes=transfer_memory::Add(bytes,MapBytes<Star>(star.second.size()));
  return bytes;
}
bool Directory::NotifyChanges(const Prepared& prepared, std::vector<Id>& changed, std::string& reason,
                              size_t ceiling, size_t baseline) {
  Timer timer(stats.change_seconds);
  CLocalFailure failure;
  const size_t staged_bytes=PreparedBytes(prepared);
  size_t records=0;
  for (const auto& star:prepared.changed) {
    if (star.first.first) continue;
    const auto prior=stars.find(star.first);
    const size_t old_size=prior==stars.end() ? 0 : prior->second.size();
    if (old_size>MaximumDirectoryScan || star.second.size()>MaximumDirectoryScan)
      failure.Set(3,star.first.second,"Native 3D changed-star notification exceeds scan budget.");
    records=transfer_memory::Add(records,old_size,star.second.size());
  }
  const auto retained=transfer_memory::Add(baseline,ResidentBytes(),staged_bytes);
  const auto peak=transfer_memory::Add(retained,transfer_memory::Mul(world.size,sizeof(std::vector<Id>)),
      transfer_memory::GrowthBound(records,sizeof(Id)));
  stats.maximum_work_bytes=std::max(stats.maximum_work_bytes,peak);
  if (peak==SIZE_MAX || peak>ceiling) failure.Set(3,0,"Native 3D change-notification packing exceeds budget.");
  if (!Vote(failure,reason)) return false;
  std::vector<std::vector<Id>> to;
  try {
    to.resize(world.size);
    for (const auto& star:prepared.changed) {
      if (star.first.first) continue;
      const auto prior=stars.find(star.first);
      if (prior!=stars.end()) for (const auto& entry:prior->second) {
        to[entry.second.dependency.owner].push_back(star.first.second); ++stats.change_scanned;
      }
      for (const auto& entry:star.second) {
        to[entry.second.dependency.owner].push_back(star.first.second); ++stats.change_scanned;
      }
    }
    for (auto& bucket:to) {
      std::sort(bucket.begin(),bucket.end()); bucket.erase(std::unique(bucket.begin(),bucket.end()),bucket.end());
      stats.change_sent+=bucket.size();
    }
  } catch (const std::exception& e) { failure.Set(2,0,e.what()); }
  if (!Vote(failure,reason)) return false;
  bool admitted=false;
  changed=world.exchange(to,&admitted,ceiling,retained);
  stats.maximum_work_bytes=std::max(stats.maximum_work_bytes,world.max_exchange_work_bytes);
  if (!admitted) failure.Set(3,0,"Native 3D change-notification transport exceeds budget.");
  if (!Vote(failure,reason)) return false;
  stats.change_received+=changed.size();
  std::sort(changed.begin(),changed.end()); changed.erase(std::unique(changed.begin(),changed.end()),changed.end());
  return true;
}
bool Directory::AgreeParameters(std::string& reason,size_t ceiling,size_t caller) {
  Timer timer(stats.parameter_agreement_seconds);CLocalFailure failure;
  const size_t count=parameters->SnapshotBytes();
  const size_t bound=transfer_memory::Add(caller,ResidentBytes(),transfer_memory::Mul(count,4),transfer_memory::TransportBytes(world.size));
  stats.maximum_work_bytes=std::max(stats.maximum_work_bytes,bound);
  if(bound==SIZE_MAX || bound>ceiling)failure.Set(3,0,"Boundary parameter agreement exceeds storage admission.");
  if(!Vote(failure,reason))return false;
  std::vector<char> local,root;
  try{local=parameters->Snapshot();if(!world.rank)root=local;}
  catch(const std::exception& e){failure.Set(2,0,e.what());}
  if(!Vote(failure,reason))return false;
  CPassiveComm::BcastRounds(root,0,world.comm);
  if(local!=root)failure.Set(1,0,"Boundary parameter authority/records differ across workers.");
  stats.parameter_payload_bytes+=root.size();return Vote(failure,reason);
}
bool Directory::PrepareParameters(const std::vector<OwnedCell>& old,const std::vector<OwnedCell>& added,
    const std::vector<BoundaryParameterEdit>& edits,BoundaryParameterState::Publication& staged,
    std::string& reason,size_t ceiling,size_t caller,const SU2NativeGeometry::BoundaryCoverage& coverage,SU2NativeGeometry::AtlasBudget* reservation) {
  Timer timer(stats.parameter_publication_seconds);CLocalFailure failure;
  const size_t local_bound=transfer_memory::Add(caller,transfer_memory::Mul(old.size()+added.size(),2048),transfer_memory::Mul(edits.size(),128),BoundaryCoverageBytes(coverage));
  stats.maximum_work_bytes=std::max(stats.maximum_work_bytes,local_bound);
  if(local_bound==SIZE_MAX || local_bound>ceiling)failure.Set(3,0,"Boundary parameter face preparation exceeds storage admission.");
  if(!Vote(failure,reason))return false;
  ParameterAtlasScope atlas_scope(reservation);
  try{atlas_scope.Refresh(local_bound,ceiling);}catch(const std::exception& e){failure.Set(3,0,e.what());}
  if(!Vote(failure,reason))return false;
  std::vector<BoundaryFaceChange> local_faces;
  try {
    std::map<std::pair<Id,Face>,int> difference;
    auto collect=[&](const std::vector<OwnedCell>& cells,int sign) {
      for(const auto& row:cells) {
        const auto& v=row.cell.v;const std::array<Face,4> faces{{{v[1].id,v[2].id,v[3].id},{v[0].id,v[3].id,v[2].id},{v[0].id,v[1].id,v[3].id},{v[0].id,v[2].id,v[1].id}}};
        for(unsigned k=0;k<4;++k)if(row.physical[k].present) {
          Face face=faces[k];const auto first=std::min_element(face.begin(),face.end());std::rotate(face.begin(),first,face.end());
          difference[{row.physical[k].facet,face}]+=sign;
        }
      }
    };
    collect(old,-1);collect(added,1);
    for(const auto& row:difference)if(row.second) {
      if(row.second!=1 && row.second!=-1)throw std::invalid_argument("Duplicate physical face in local geometry publication.");
      local_faces.push_back({row.first.second,row.first.first,row.second>0});
    }
  }catch(const std::exception& e){failure.Set(2,0,e.what());}
  if(!Vote(failure,reason))return false;
  const size_t local_encoded=coverage.faces.empty()&&coverage.traces.empty()?0:BoundaryAtlasBytes(coverage,{});
  const size_t local_pages=local_encoded?transfer_memory::Add(local_encoded/8,63)/64:0;
  const size_t global_pages=CPassiveComm::Allreduce(local_pages,CPassiveComm::Op::SUM,world.comm);
  const auto atlas_peak=transfer_memory::Add(local_bound,transfer_memory::Mul(local_encoded,4),
      transfer_memory::Mul(global_pages,transfer_memory::Mul(sizeof(AtlasPage),world.size+20)));
  stats.maximum_work_bytes=std::max(stats.maximum_work_bytes,atlas_peak);
  if(atlas_peak==SIZE_MAX||atlas_peak>ceiling)failure.Set(3,0,"Atlas descriptor page transport exceeds storage admission.");
  if(global_pages&&!reservation)failure.Set(1,0,"Atlas descriptor transport lacks admitted geometry work.");
  if(!Vote(failure,reason))return false;
  SU2NativeGeometry::BoundaryCoverage global_coverage;
  if(global_pages) {
    std::vector<std::vector<AtlasPage>> to(world.size);
    try {
      const auto bytes=local_encoded?EncodeBoundaryAtlas(coverage,{},ceiling,local_bound):std::vector<char>{};
      SU2NativeBoundary2D::RecordStream input(bytes);
      std::vector<AtlasPage> pages;pages.reserve(local_pages);
      for(size_t i=0;i<local_pages;++i){AtlasPage page;page.source=world.rank;page.revision=parameters->Revision();page.identity=parameters->Mobility().identity;page.sequence=i;page.total=local_pages;
        page.used=std::min<size_t>(64,bytes.size()/8-i*64);for(size_t j=0;j<page.used;++j)input(page.words[j]);pages.push_back(page);}
      for(auto& bucket:to)bucket=pages;
    }catch(const std::exception& e){failure.Set(2,0,e.what());}
    if(!Vote(failure,reason))return false;
    bool admitted=false;auto pages=world.exchange(to,&admitted,ceiling,atlas_peak);
    if(!admitted){reason="Atlas descriptor fixed-page exchange refused storage.";return false;}
    try {
      std::sort(pages.begin(),pages.end(),[](const AtlasPage& a,const AtlasPage& b){return std::tie(a.source,a.sequence)<std::tie(b.source,b.sequence);});
      for(size_t first=0;first<pages.size();) {
        const auto source=pages[first].source,total=pages[first].total;size_t end=first;
        while(end<pages.size()&&pages[end].source==source)++end;
        if(source>=unsigned(world.size)||!total||end-first!=total)throw std::invalid_argument("Atlas descriptor source has missing/duplicate pages.");
        std::vector<char> bytes;bytes.reserve(transfer_memory::Mul(total,512));SU2NativeBoundary2D::RecordStream output(bytes);
        for(size_t i=first;i<end;++i){const auto& page=pages[i];
          if(page.sequence!=i-first||page.total!=total||page.revision!=parameters->Revision()||page.identity!=parameters->Mobility().identity||!page.used||page.used>64||(i+1<end&&page.used!=64))
            throw std::invalid_argument("Stale/inconsistent atlas descriptor page sequence.");
          for(size_t j=0;j<page.used;++j){auto word=page.words[j];output(word);}}
        SU2NativeGeometry::BoundaryCoverage decoded;SU2NativeGeometry::MobilityPolicy policy;
        DecodeBoundaryAtlas(bytes,decoded,policy,ceiling,atlas_peak);
        if(policy.identity||!policy.roles.empty()||!policy.features.empty()||!policy.corners.empty()||!policy.bodies.empty()||!policy.sheets.empty())throw std::invalid_argument("Atlas publication attempts a mobility-policy edit.");
        for(auto& row:decoded.faces){const auto old=global_coverage.faces.find(row.first);
          if(old!=global_coverage.faces.end()) {
            SU2NativeGeometry::BoundaryCoverage a,b;a.faces.emplace(*old);b.faces.emplace(row.first,row.second);
            if(EncodeBoundaryAtlas(a,{})!=EncodeBoundaryAtlas(b,{}))throw std::invalid_argument("Conflicting distributed atlas covers.");
          } else global_coverage.faces.emplace(row.first,std::move(row.second));}
        for(auto& row:decoded.traces){const auto old=global_coverage.traces.find(row.first);std::string detail;
          if(old!=global_coverage.traces.end()){if(!parameters->Geometry().EqualTraces(old->second,row.second,*reservation,detail))throw std::invalid_argument("Conflicting distributed atlas traces: "+detail);}
          else global_coverage.traces.emplace(row.first,std::move(row.second));}
        first=end;
      }
    }catch(const std::exception& e){failure.Set(2,0,e.what());}
    if(!Vote(failure,reason))return false;
    stats.parameter_payload_bytes+=transfer_memory::Mul(global_pages,560);
  }
  // Association refresh changes curved cell envelopes, so its owners must advance cell versions too.
  if(!global_coverage.faces.empty()) {
    const auto closure_peak=transfer_memory::Add(atlas_peak,BoundaryCoverageBytes(global_coverage),
        transfer_memory::Mul(global_coverage.faces.size(),256));
    stats.maximum_work_bytes=std::max(stats.maximum_work_bytes,closure_peak);
    if(closure_peak==SIZE_MAX||closure_peak>ceiling||global_coverage.faces.size()>size_t(INT_MAX)/2)
      failure.Set(3,0,"Atlas owner closure exceeds storage/transport admission.");
    if(!Vote(failure,reason))return false;
    std::map<Face,size_t> indices;std::vector<size_t> counts,total;
    try {
      counts.assign(2*global_coverage.faces.size(),0);total.resize(counts.size());
      for(const auto& row:global_coverage.faces)indices.emplace(row.first,indices.size());
      auto count=[&](const std::vector<OwnedCell>& cells,size_t side) {
        for(const auto& row:cells){const auto& v=row.cell.v;
          const std::array<Face,4> faces{{{v[1].id,v[2].id,v[3].id},{v[0].id,v[3].id,v[2].id},
                                        {v[0].id,v[1].id,v[3].id},{v[0].id,v[2].id,v[1].id}}};
          for(unsigned k=0;k<4;++k)if(row.physical[k].present){auto face=faces[k];
            std::rotate(face.begin(),std::min_element(face.begin(),face.end()),face.end());
            const auto found=indices.find(face);if(found!=indices.end())++counts[2*found->second+side];}
        }
      };
      count(old,0);count(added,1);
    }catch(const std::exception& e){failure.Set(2,0,e.what());}
    if(!Vote(failure,reason))return false;
    CPassiveComm::Allreduce(counts.data(),total.data(),counts.size(),CPassiveComm::Op::SUM,world.comm);
    for(const auto& row:indices){auto key=row.first;std::sort(key.begin(),key.end());
      if(total[2*row.second+1]!=1 || (parameters->Surface().count(key)&&total[2*row.second]!=1))
        failure.Set(1,0,"Atlas cover update requires replacement of every owning volume cell.");
    }
    if(!Vote(failure,reason))return false;
  }
  const size_t changes=CPassiveComm::Allreduce(transfer_memory::Add(local_faces.size(),edits.size()),CPassiveComm::Op::SUM,world.comm);
  if(!changes && !global_pages){staged.before=staged.after=parameters->Revision();staged.valid=true;return true;}
  const size_t peak=transfer_memory::Add(atlas_peak,BoundaryCoverageBytes(global_coverage),transfer_memory::Mul(changes,32768),
      transfer_memory::Mul(world.size,transfer_memory::Add(transfer_memory::Bytes(local_faces),transfer_memory::Bytes(edits))),
      transfer_memory::TransportBytes(world.size));
  stats.maximum_work_bytes=std::max(stats.maximum_work_bytes,peak);
  if(peak==SIZE_MAX || peak>ceiling)failure.Set(3,0,"Boundary parameter collective publication exceeds storage admission.");
  if(!Vote(failure,reason))return false;
  bool admitted=false;const auto common=transfer_memory::Add(local_bound,transfer_memory::Mul(changes,256));
  const auto faces=world.exchange(std::vector<std::vector<BoundaryFaceChange>>(world.size,local_faces),&admitted,ceiling,common);
  if(!admitted){reason="Boundary parameter face transport exceeds storage admission.";return false;}
  const auto global_edits=world.exchange(std::vector<std::vector<BoundaryParameterEdit>>(world.size,edits),&admitted,ceiling,transfer_memory::Add(common,transfer_memory::Bytes(faces)));
  if(!admitted){reason="Boundary parameter edit transport exceeds storage admission.";return false;}
  try{atlas_scope.Refresh(peak,ceiling);}catch(const std::exception& e){failure.Set(3,0,e.what());}
  if(!Vote(failure,reason))return false;
  std::string detail;
  staged=parameters->Prepare(global_edits,faces,detail,ceiling,transfer_memory::Add(local_bound,transfer_memory::Bytes(faces),transfer_memory::Bytes(global_edits),BoundaryCoverageBytes(global_coverage)),global_coverage,reservation);
  if(!staged.valid)failure.Set(1,0,detail);
  stats.parameter_edits+=global_edits.size();stats.parameter_faces+=faces.size();
  stats.parameter_payload_bytes+=global_edits.size()*88+faces.size()*40;
  return Vote(failure,reason);
}
bool Directory::Publish(Owned& owned, const std::vector<Dependency>& removed, const std::vector<OwnedCell>& added,
                        bool prevalidated, std::string& reason, size_t ceiling, size_t caller_bytes,
                        const std::vector<BoundaryParameterEdit>& parameter_edits,const SU2NativeGeometry::BoundaryCoverage& coverage,SU2NativeGeometry::AtlasBudget* reservation) {
  Timer timer(stats.publication_seconds);
  CLocalFailure failure;
  Owned staged;
  std::set<Id> collision_added;
  std::vector<OwnedCell> old;
  if (!prevalidated) failure.Set(1, 0, "Native 3D candidate veto before publication.");
  if(!parameters && (!parameter_edits.empty()||!coverage.faces.empty()||!coverage.traces.empty()))failure.Set(1,0,"Boundary parameter edits have no accepted parameter store.");
  if(parameters && parameter_geometry!=&parameters->Geometry())failure.Set(1,0,"Boundary geometry changed without rebuilding the directory.");
  const auto baseline = transfer_memory::Add(caller_bytes,ParameterBytes(),BoundaryCoverageBytes(coverage),transfer_memory::Bytes(parameter_edits), MapBytes<Owned>(owned.size()), transfer_memory::Bytes(removed),
      transfer_memory::Bytes(added), MapBytes<Owned>(added.size()), transfer_memory::Mul(removed.size(), sizeof(OwnedCell)),
      MapBytes<std::set<Id>>(removed.size()),collision?MapBytes<std::set<Id>>(added.size()):0);
  auto peak = transfer_memory::Add(baseline, ResidentBytes());
  stats.maximum_work_bytes = std::max(stats.maximum_work_bytes, peak);
  // Routed rows aggregate independent caller-validated cavities; bound their staging by bytes, not one cavity.
  if (peak == SIZE_MAX || peak > ceiling)
    failure.Set(3, 0, "Native 3D publication staging exceeds budget.");
  if (!Vote(failure, reason)) { ++stats.rejected; return false; }
  try {
    std::set<Id> erasures;
    old.reserve(removed.size());
    for (const auto& row : removed) {
      const auto found = owned.find(row.cell);
      if (row.owner != world.rank || found == owned.end() || found->second.version != row.version ||
          !erasures.insert(row.cell).second) throw std::runtime_error("Native 3D stale/duplicate publication removal.");
      old.push_back(found->second);
    }
    for (const auto& row : added) {
      if(collision)collision_added.insert(row.cell.id);
      Validate(row);
      if ((owned.count(row.cell.id) && !erasures.count(row.cell.id)) || !staged.emplace(row.cell.id, row).second)
        throw std::runtime_error("Native 3D duplicate publication cell identity.");
    }
  } catch (const std::exception& e) { failure.Set(2, 0, e.what()); }
  if (!Vote(failure, reason)) { ++stats.rejected; return false; }
  auto prepared = Prepare(old, added, ceiling, baseline);
  reason = prepared.reason;
  if (!prepared.valid) { ++stats.rejected; return false; }
  BoundaryParameterState::Publication parameter_stage;
  if(parameters && !PrepareParameters(old,added,parameter_edits,parameter_stage,reason,ceiling,transfer_memory::Add(baseline,ResidentBytes(),PreparedBytes(prepared)),coverage,reservation)) {++stats.rejected;return false;}
  std::vector<Id> changed;
  if (track_changes && !NotifyChanges(prepared,changed,reason,ceiling,transfer_memory::Add(baseline,parameter_stage.ResidentBytes()))) { ++stats.rejected; return false; }
  // All stores and notifications are staged before the final common vote; publication allocates nothing.
  for (const auto& row : old) {
    owned.erase(row.cell.id);if(collision)collision->added.erase(row.cell.id);
  }
  if(collision)while(!collision_added.empty())collision->added.insert(collision_added.extract(collision_added.begin()));
  while (!staged.empty()) owned.insert(staged.extract(staged.begin()));
  Commit(std::move(prepared));
  if(parameters)parameters->Commit(std::move(parameter_stage));
  changed_vertices.swap(changed);
  ++stats.publications;
  reason.clear();
  return true;
}
}  // namespace SU2Native3D
