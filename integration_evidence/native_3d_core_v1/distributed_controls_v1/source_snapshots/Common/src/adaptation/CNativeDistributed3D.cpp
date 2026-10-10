/*!
 * \file CNativeDistributed3D.cpp
 * \brief Routed complete stars and staged tetrahedral ownership publication.
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md).
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */
#include "../../include/adaptation/CNativeDistributed3D.hpp"
#include <chrono>
#include <set>
#include <stdexcept>
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
void Validate(const OwnedCell& row) { const Incidence checked({row.cell}); (void)checked; }
struct Query {
  Feature feature;
  int caller = 0;
  unsigned ticket = 0;
  template <class S> void Fields(S& s) { s(feature, caller, ticket); }
};
struct Reply {
  Dependency dependency;
  unsigned ticket = 0;
  int status = 0;  // 0 dependency, 1 complete footer, 2 missing, 3 over budget.
  template <class S> void Fields(S& s) { s(dependency, ticket, status); }
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
struct Claim {
  Id vertex = 0;
  int caller = 0;
  template <class S> void Fields(S& s) { s(vertex, caller); }
};
struct Veto {
  int caller = 0;
  template <class S> void Fields(S& s) { s(caller); }
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
size_t Directory::ResidentBytes() const { return transfer_memory::Add(MapBytes<Stars>(stars.size()), resident_payload_bytes); }
std::vector<char> Directory::Snapshot() const {
  std::vector<char> bytes;
  RecordStream stream(bytes);
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
      transfer_memory::Bytes(records), MapBytes<std::set<Key>>(records.size()), MapBytes<Stars>(records.size()));
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
        MapBytes<Star>(records.size()));
    stats.maximum_work_bytes = std::max(stats.maximum_work_bytes, peak);
    if (peak == SIZE_MAX || peak > ceiling) failure.Set(3, 0, "Native 3D directory staging exceeds budget.");
  } catch (const std::exception& e) { failure.Set(2, 0, e.what()); }
  if (!Vote(failure, result.reason)) return result;
  try {
    for (auto key : changed) {
      const auto old = stars.find(key);
      result.changed.emplace(key, old == stars.end() ? Star{} : old->second);
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
bool Directory::Build(const Owned& owned, std::string& reason, size_t ceiling, size_t caller_bytes) {
  Timer timer(stats.build_seconds);
  ++stats.builds;
  CLocalFailure failure;
  std::vector<OwnedCell> added;
  const auto baseline = transfer_memory::Add(caller_bytes, MapBytes<Owned>(owned.size()));
  if (transfer_memory::Add(baseline, ResidentBytes(), transfer_memory::Mul(owned.size(), sizeof(OwnedCell))) > ceiling)
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
  auto prepared = Prepare({}, added, ceiling, baseline);
  reason = prepared.reason;
  if (!prepared.valid) { ++stats.rejected; return false; }
  Commit(std::move(prepared));
  return true;
}
Dependencies Directory::Lookup(std::vector<Feature> features, size_t ceiling, size_t caller_bytes) {
  Timer timer(stats.lookup_seconds);
  ++stats.lookups;
  Dependencies result;
  CLocalFailure failure;
  std::vector<std::vector<Query>> to;
  const auto baseline = transfer_memory::Add(caller_bytes, ResidentBytes(), transfer_memory::Bytes(features));
  const auto request_peak = transfer_memory::Add(baseline, transfer_memory::Mul(world.size, sizeof(std::vector<Query>)),
      transfer_memory::GrowthBound(features.size(), sizeof(Query)));
  stats.maximum_work_bytes = std::max(stats.maximum_work_bytes, request_peak);
  if (request_peak == SIZE_MAX || request_peak > ceiling) failure.Set(3, 0, "Native 3D feature packing exceeds budget.");
  if (!Vote(failure, result.reason)) { ++stats.rejected; return result; }
  try {
    if (features.size() > MaximumDirectoryFeatures) throw std::runtime_error("Native 3D feature query exceeds budget.");
    to.resize(world.size);
    for (unsigned i = 0; i < features.size(); ++i) {
      auto& f = features[i];
      if (f.count < 1 || f.count > 3) throw std::runtime_error("Native 3D invalid dependency feature.");
      std::sort(f.vertices.begin(), f.vertices.begin() + f.count);
      if (std::adjacent_find(f.vertices.begin(), f.vertices.begin() + f.count) != f.vertices.begin() + f.count)
        throw std::runtime_error("Native 3D dependency repeats a vertex.");
      to[Rendezvous({false, f.vertices[0]})].push_back({f, world.rank, i});
    }
  } catch (const std::exception& e) { failure.Set(2, 0, e.what()); }
  if (!Vote(failure, result.reason)) { ++stats.rejected; return result; }
  bool admitted = false;
  const auto queries = world.exchange(to, &admitted, ceiling, baseline);
  if (!admitted) failure.Set(3, 0, "Native 3D feature transport exceeds budget.");
  std::vector<std::vector<Reply>> reply;
  const auto reply_peak = transfer_memory::Add(baseline, transfer_memory::Bytes(to), transfer_memory::Bytes(queries),
      transfer_memory::Mul(world.size, sizeof(std::vector<Reply>)),
      transfer_memory::GrowthBound(transfer_memory::Mul(queries.size(), MaximumCavityCells + 1), sizeof(Reply)),
      transfer_memory::GrowthBound(MaximumCavityCells + 1, sizeof(Reply)));
  stats.maximum_work_bytes = std::max(stats.maximum_work_bytes, reply_peak);
  if (reply_peak == SIZE_MAX || reply_peak > ceiling) failure.Set(3, 0, "Native 3D dependency replies exceed budget.");
  if (!Vote(failure, result.reason)) { ++stats.rejected; return result; }
  try {
    reply.resize(world.size);
    for (const auto& q : queries) {
      const auto found = stars.find({false, q.feature.vertices[0]});
      int status = found == stars.end() ? 2 : found->second.size() > MaximumDirectoryScan ? 3 : 1;
      std::vector<Reply> matches;
      if (status == 1)
        for (const auto& row : found->second) {
          ++stats.metadata_scanned;
          bool contains = true;
          for (unsigned i = 0; i < q.feature.count; ++i)
            contains = contains && std::binary_search(row.second.nodes.begin(), row.second.nodes.end(), q.feature.vertices[i]);
          if (contains) {
            matches.push_back({row.second.dependency, q.ticket, 0});
            if (matches.size() > MaximumCavityCells) { status = 3; break; }
          }
        }
      if (status == 1 && matches.empty()) status = 2;
      if (status == 1) reply[q.caller].insert(reply[q.caller].end(), matches.begin(), matches.end());
      reply[q.caller].push_back({{}, q.ticket, status});
    }
  } catch (const std::exception& e) { failure.Set(2, 0, e.what()); }
  if (!Vote(failure, result.reason)) { ++stats.rejected; return result; }
  const auto received = world.exchange(reply, &admitted, ceiling,
      transfer_memory::Add(baseline, transfer_memory::Bytes(to), transfer_memory::Bytes(queries)));
  if (!admitted) failure.Set(3, 0, "Native 3D dependency import exceeds budget.");
  const auto import_peak = transfer_memory::Add(baseline, transfer_memory::Bytes(to), transfer_memory::Bytes(queries),
      transfer_memory::Bytes(reply), transfer_memory::Bytes(received), MapBytes<std::map<Id, Dependency>>(MaximumCavityCells + 1),
      transfer_memory::GrowthBound(MaximumCavityCells, sizeof(Dependency)), transfer_memory::Mul(features.size(), sizeof(bool)));
  stats.maximum_work_bytes = std::max(stats.maximum_work_bytes, import_peak);
  if (import_peak == SIZE_MAX || import_peak > ceiling) failure.Set(3, 0, "Native 3D dependency validation exceeds budget.");
  if (!Vote(failure, result.reason)) { ++stats.rejected; return result; }
  try {
    std::map<Id, Dependency> unique;
    std::vector<bool> footer(features.size(), false);
    for (const auto& row : received) {
      if (row.ticket >= features.size()) throw std::runtime_error("Native 3D invalid dependency reply ticket.");
      if (row.status) {
        if (row.status != 1 || footer[row.ticket]) throw std::runtime_error(row.status == 3 ?
            "Native 3D dependency star exceeds budget." : "Native 3D missing or duplicate dependency feature.");
        footer[row.ticket] = true;
      } else {
        const auto old = unique.emplace(row.dependency.cell, row.dependency);
        if (!old.second && (old.first->second.version != row.dependency.version || old.first->second.owner != row.dependency.owner))
          throw std::runtime_error("Native 3D inconsistent dependency ownership.");
        if (unique.size() > MaximumCavityCells) throw std::runtime_error("Native 3D dependency union exceeds budget.");
      }
    }
    if (std::find(footer.begin(), footer.end(), false) != footer.end()) throw std::runtime_error("Native 3D incomplete dependency footer.");
    for (const auto& row : unique) result.cells.push_back(row.second);
    stats.maximum_dependencies = std::max(stats.maximum_dependencies, result.cells.size());
  } catch (const std::exception& e) { failure.Set(2, 0, e.what()); }
  result.valid = Vote(failure, result.reason);
  if (!result.valid) { result.cells.clear(); ++stats.rejected; }
  return result;
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
    if (!dependencies.valid || dependencies.cells.size() > MaximumCavityCells) throw std::runtime_error("Native 3D dependencies were not admitted.");
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
      expected.erase(found);
      result.cells.push_back(row.cell);
    }
    std::sort(result.cells.begin(), result.cells.end(), [](const auto& a, const auto& b) { return a.cell.id < b.cell.id; });
    stats.payloads += result.cells.size();
  } catch (const std::exception& e) { failure.Set(2, 0, e.what()); }
  result.valid = Vote(failure, result.reason);
  if (!result.valid) { result.cells.clear(); ++stats.rejected; }
  return result;
}
bool Directory::Reserve(const Imported& imported, bool active, std::string& reason, size_t ceiling, size_t caller_bytes) {
  Timer timer(stats.reservation_seconds);
  ++stats.reservations;
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
    if (active && (!imported.valid || imported.cells.empty() || imported.cells.size() > MaximumCavityCells))
      throw std::runtime_error("Native 3D reservation lacks admitted dependencies.");
    to.resize(world.size);
    if (active) {
      std::set<Id> vertices;
      for (const auto& row : imported.cells) for (auto node : row.cell.v) vertices.insert(node.id);
      for (auto id : vertices) to[Rendezvous({false, id})].push_back({id, world.rank});
    }
  } catch (const std::exception& e) { failure.Set(2, 0, e.what()); }
  if (!Vote(failure, reason)) { ++stats.rejected; return false; }
  bool admitted = false;
  const auto claims = world.exchange(to, &admitted, ceiling, baseline);
  if (!admitted) failure.Set(3, 0, "Native 3D reservation transport exceeds budget.");
  std::vector<std::vector<Veto>> reply;
  peak = transfer_memory::Add(baseline, transfer_memory::Bytes(to), transfer_memory::Bytes(claims),
      transfer_memory::Mul(world.size, sizeof(std::vector<Veto>)), MapBytes<std::map<Id, int>>(claims.size()),
      transfer_memory::GrowthBound(claims.size(), sizeof(Veto)));
  stats.maximum_work_bytes = std::max(stats.maximum_work_bytes, peak);
  if (peak == SIZE_MAX || peak > ceiling) failure.Set(3, 0, "Native 3D reservation replies exceed budget.");
  if (!Vote(failure, reason)) { ++stats.rejected; return false; }
  try {
    reply.resize(world.size);
    std::map<Id, int> winner;
    for (const auto& row : claims) {
      const auto old = winner.find(row.vertex);
      if (old == winner.end()) winner.emplace(row.vertex, row.caller);
      else old->second = std::min(old->second, row.caller);
    }
    for (const auto& row : claims) if (winner.at(row.vertex) != row.caller) reply[row.caller].push_back({row.caller});
  } catch (const std::exception& e) { failure.Set(2, 0, e.what()); }
  if (!Vote(failure, reason)) { ++stats.rejected; return false; }
  const auto veto = world.exchange(reply, &admitted, ceiling,
      transfer_memory::Add(baseline, transfer_memory::Bytes(to), transfer_memory::Bytes(claims)));
  if (!admitted) { reason = "Native 3D reservation replies exceed budget."; ++stats.rejected; return false; }
  if (!veto.empty()) { reason = "Native 3D conflicting dependency reservation."; ++stats.rejected; return false; }
  reason.clear();
  return active;
}
bool Directory::Publish(Owned& owned, const std::vector<Dependency>& removed, const std::vector<OwnedCell>& added,
                        bool prevalidated, std::string& reason, size_t ceiling, size_t caller_bytes) {
  Timer timer(stats.publication_seconds);
  CLocalFailure failure;
  Owned staged;
  std::vector<OwnedCell> old;
  if (!prevalidated) failure.Set(1, 0, "Native 3D candidate veto before publication.");
  const auto baseline = transfer_memory::Add(caller_bytes, MapBytes<Owned>(owned.size()), transfer_memory::Bytes(removed),
      transfer_memory::Bytes(added), MapBytes<Owned>(added.size()), transfer_memory::Mul(removed.size(), sizeof(OwnedCell)),
      MapBytes<std::set<Id>>(removed.size()));
  auto peak = transfer_memory::Add(baseline, ResidentBytes());
  stats.maximum_work_bytes = std::max(stats.maximum_work_bytes, peak);
  if (removed.size() > MaximumCavityCells || added.size() > MaximumReplacementCells || peak == SIZE_MAX || peak > ceiling)
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
      Validate(row);
      if ((owned.count(row.cell.id) && !erasures.count(row.cell.id)) || !staged.emplace(row.cell.id, row).second)
        throw std::runtime_error("Native 3D duplicate publication cell identity.");
    }
  } catch (const std::exception& e) { failure.Set(2, 0, e.what()); }
  if (!Vote(failure, reason)) { ++stats.rejected; return false; }
  auto prepared = Prepare(old, added, ceiling, baseline);
  reason = prepared.reason;
  if (!prepared.valid) { ++stats.rejected; return false; }
  // Prepare's common vote is final: both stores already own every node needed by publication.
  for (const auto& row : old) owned.erase(row.cell.id);
  while (!staged.empty()) owned.insert(staged.extract(staged.begin()));
  Commit(std::move(prepared));
  ++stats.publications;
  reason.clear();
  return true;
}
}  // namespace SU2Native3D
