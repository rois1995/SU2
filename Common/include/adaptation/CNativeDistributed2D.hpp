/*!
 * \file CNativeDistributed2D.hpp
 * \brief Scalar serialization and incidence discovery for synchronous native 2D transactions.
 * \version 8.5.0 "Harrier"
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md)
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */

#pragma once

#include "CNativeBoundary2D.hpp"
#include "CDistributedSearch.hpp"
#include "CTransferMemory.hpp"
#include "../parallelization/CPassiveComm.hpp"

namespace SU2NativeBoundary2D {

template <class T>
struct IsArray : std::false_type {};
template <class T, size_t N>
struct IsArray<std::array<T, N>> : std::true_type {};

/*--- The same field walk measures, encodes and decodes a record. Numeric fields use scalar bytes, never struct
 *    padding. Endianness follows existing CPassiveComm packing conventions. MPI ranks run matching protocols. ---*/
class RecordStream {
 public:
  RecordStream() = default;  // Measurement only.
  explicit RecordStream(std::vector<char>& bytes) : output(&bytes) {}
  explicit RecordStream(const std::vector<char>& bytes) : input(&bytes) {}
  size_t Size() const { return position; }
  bool End() const { return input && position == input->size(); }
  template <class... T>
  void operator()(T&... value) {
    (Field(value), ...);
  }

 private:
  std::vector<char>* output = nullptr;
  const std::vector<char>* input = nullptr;
  size_t position = 0;
  template <class T>
  void Field(T& value) {
    if constexpr (IsArray<T>::value) {
      for (auto& entry : value) Field(entry);
    } else if constexpr (std::is_arithmetic<T>::value) {
      static_assert(std::is_integral<T>::value || std::is_same<T, double>::value,
                    "Native wire scalars are integers/doubles.");
      static_assert(sizeof(T) <= 8, "Unsupported native integer width.");
      uint64_t bits = 0;
      if (!input) {
        if constexpr (std::is_same<T, double>::value)
          std::memcpy(&bits, &value, 8);
        else
          bits = static_cast<uint64_t>(value);
      }
      if (output) {
        const auto first = output->size();
        output->resize(first + 8);
        std::memcpy(output->data() + first, &bits, 8);
      }
      if (input) {
        if (position > input->size() || input->size() - position < 8)
          throw std::runtime_error("Truncated native record.");
        std::memcpy(&bits, input->data() + position, 8);
        if constexpr (std::is_same<T, double>::value)
          std::memcpy(&value, &bits, 8);
        else if constexpr (std::is_signed<T>::value) {
          const int64_t decoded =
              bits <= uint64_t(INT64_MAX) ? static_cast<int64_t>(bits) : -1 - static_cast<int64_t>(UINT64_MAX - bits);
          if (decoded < std::numeric_limits<T>::min() || decoded > std::numeric_limits<T>::max())
            throw std::runtime_error("Native signed field out of range.");
          value = static_cast<T>(decoded);
        } else {
          if (bits > std::numeric_limits<T>::max()) throw std::runtime_error("Native unsigned field out of range.");
          value = static_cast<T>(bits);
        }
      }
      position += 8;
    } else
      value.Fields(*this);
  }
};

class World {
 public:
  const CPassiveComm::Communicator comm;
  const int rank, size;
  explicit World(CPassiveComm::Communicator c = SU2_MPI::GetComm())
      : comm(c), rank(CPassiveComm::Rank(c)), size(CPassiveComm::Size(c)) {}
  CElectedFailure elect(const CLocalFailure& local) const { return ElectFailure(local, comm); }
  void Fail(const CLocalFailure& local, const std::string& function) const {
    const auto elected = elect(local);
    if (!elected.any) return;
    if (comm == SU2_MPI::GetComm()) SU2_MPI::Error(elected.message, function);
    // Only an elected group failure can unwind collectively back to the CFD
    // ranks. Never enter SU2's global fatal-error protocol inside this subset.
    throw elected;
  }
  size_t bytes_sent = 0, max_exchange_bytes = 0;
  size_t max_exchange_work_bytes = 0;
  double collective_seconds = 0;
  uint64_t collective_calls = 0;
  int sum(int value) { return Reduce(value, CPassiveComm::Op::SUM); }
  int maximum(int value) { return Reduce(value, CPassiveComm::Op::MAX); }
  double seconds() const { return SU2_MPI::Wtime(); }

  template <class T>
  std::vector<T> exchange(const std::vector<std::vector<T>>& to, bool* admitted = nullptr, size_t ceiling = SIZE_MAX,
                          size_t baseline = 0) {
    CLocalFailure failure;
    if (to.size() != static_cast<size_t>(size)) failure.Set(1, 0, "Invalid native peer buckets.");
    Fail(failure, CURRENT_FUNCTION);
    T example{};
    RecordStream measure;
    measure(example);
    const auto recordBytes = measure.Size();
    std::vector<size_t> sendBytes(size), recvBytes;
    size_t total = 0;
    for (int r = 0; r < size; ++r) {
      if (to[r].size() > SIZE_MAX / recordBytes || total > SIZE_MAX - to[r].size() * recordBytes)
        failure.Set(1, r, "Native message size overflow.");
      else {
        sendBytes[r] = to[r].size() * recordBytes;
        total += sendBytes[r];
      }
    }
    Fail(failure, CURRENT_FUNCTION);
    // Exchange lengths before packing/import. The bound includes bucket capacities, result records,
    // packed bytes, all transport temporaries and live round buffers. Other caller storage belongs in
    // baseline. Reuse the transfer's requested-byte model; allocator bookkeeping/RSS is not this quantity.
    std::vector<uint64_t> counts(sendBytes.begin(), sendBytes.end()), incoming(size);
    const auto countStarted = seconds();
    CPassiveComm::Alltoall(counts.data(), incoming.data(), sizeof(uint64_t), comm);
    collective_seconds += seconds() - countStarted;
    ++collective_calls;
    size_t receivedTotal = 0;
    for (const auto count : incoming) {
      if (count > SIZE_MAX || receivedTotal > SIZE_MAX - static_cast<size_t>(count))
        failure.Set(1, 0, "Native receive size overflow.");
      else
        receivedTotal += static_cast<size_t>(count);
    }
    Fail(failure, CURRENT_FUNCTION);
    const auto retained = transfer_memory::Add(baseline, transfer_memory::Bytes(to), transfer_memory::Mul(32, size));
    const auto peak = transfer_memory::Add(
        retained, total, receivedTotal,
        std::max(transfer_memory::Mul(receivedTotal / recordBytes, sizeof(T)),
                 transfer_memory::Add(transfer_memory::TransportBytes(size),
                                      transfer_memory::Staging(CPassiveComm::GetRoundBytes(), total - sendBytes[rank],
                                                               receivedTotal - incoming[rank]))));
    max_exchange_work_bytes = std::max(max_exchange_work_bytes, peak);
    const bool accepted = !maximum(peak > ceiling);
    if (admitted) *admitted = accepted;
    if (!accepted) {
      if (!admitted) failure.Set(1, 0, "Native exchange exceeds its working-buffer ceiling.");
      Fail(failure, CURRENT_FUNCTION);
      return {};
    }
    std::vector<char> bytes;
    bytes.reserve(total);
    RecordStream encoder(bytes);
    for (const auto& bucket : to)
      for (auto value : bucket) encoder(value);
    const auto started = seconds();
    const auto received = CPassiveComm::AlltoallvRounds(bytes.data(), sendBytes, recvBytes, comm);
    collective_seconds += seconds() - started;
    ++collective_calls;
    bytes_sent += bytes.size();
    max_exchange_bytes = std::max(max_exchange_bytes, bytes.size() + received.size());
    if (received.size() % recordBytes) failure.Set(1, 0, "Partial native record received.");
    Fail(failure, CURRENT_FUNCTION);
    std::vector<T> result;
    result.reserve(received.size() / recordBytes);
    RecordStream decoder(received);
    while (!decoder.End()) {
      T value{};
      decoder(value);
      result.push_back(value);
    }
    return result;
  }
  template <class T>
  std::vector<T> metadata(const std::vector<T>& local) {
    // ponytail: bounded round proposals are replicated; hierarchical routing follows measured scaling limits.
    return exchange(std::vector<std::vector<T>>(size, local));
  }

 private:
  int Reduce(int value, CPassiveComm::Op op) {
    const auto started = seconds();
    const auto result = CPassiveComm::Allreduce(value, op, comm);
    collective_seconds += seconds() - started;
    ++collective_calls;
    return result;
  }
};

struct Incidence {
  Id node = 0, cell = 0;
  uint64_t version = 0;
  int owner = 0, erase = 0;
  template <class S>
  void Fields(S& s) {
    s(node, cell, version, owner, erase);
  }
};
struct Lookup {
  Id node = 0;
  int caller = 0;
  template <class S>
  void Fields(S& s) {
    s(node, caller);
  }
};
struct CellQuery {
  Id cell = 0;
  uint64_t version = 0;
  int caller = 0;
  template <class S>
  void Fields(S& s) {
    s(cell, version, caller);
  }
};
struct Payload {
  Cell cell;
  int present = 0;
  template <class S>
  void Fields(S& s) {
    s(cell, present);
  }
};
struct Claim {
  Id node = 0;
  int caller = 0;
  template <class S>
  void Fields(S& s) {
    s(node, caller);
  }
};
struct Veto {
  int caller = 0;
  template <class S>
  void Fields(S& s) {
    s(caller);
  }
};

/*--- Changed stars are built privately before the common vote. Publication transfers preallocated map nodes;
 *    neither rejection nor preparation changes the accepted directory. ---*/
class Directory {
 public:
  using Stars = std::map<Id, std::map<Id, Incidence>>;
  struct Prepared {
    Stars stars;
    bool valid = true;
    std::string reason;
  };
  World& world;
  Stars incident;
  explicit Directory(World& w) : world(w) {}
  int rendezvous(Id id) const {
    id ^= id >> 30;
    id *= 0xbf58476d1ce4e5b9ULL;
    id ^= id >> 27;
    id *= 0x94d049bb133111ebULL;
    id ^= id >> 31;
    return id % world.size;
  }
  Prepared prepare(const std::vector<Cell>& removed, const std::vector<Cell>& added, size_t ceiling = SIZE_MAX,
                   size_t baseline = 0) {
    std::vector<std::vector<Incidence>> to(world.size);
    for (int erase : {1, 0})
      for (const auto& c : erase ? removed : added)
        for (const auto& v : c.t.v) to[rendezvous(v.id)].push_back({v.id, c.t.id, c.version, world.rank, erase});
    CLocalFailure failure;
    Prepared staged;
    bool admitted = true;
    auto records = world.exchange(to, &admitted, ceiling, baseline);
    if (!admitted) failure.Set(3, 0, "Native incidence transport exceeds the dependency budget.");
    // Include every changed star, including unselected artificial-perimeter vertices. Selected-cavity caps
    // alone do not bound the private copy of a high-valence perimeter star. Count before cloning any map.
    std::sort(records.begin(), records.end(), [](const Incidence& a, const Incidence& b) {
      return std::tie(a.node, a.erase, a.cell) < std::tie(b.node, b.erase, b.cell);
    });
    size_t starBytes = 0;
    for (size_t first = 0; first < records.size();) {
      size_t last = first, additions = 0, erasures = 0;
      while (last < records.size() && records[last].node == records[first].node) {
        additions += !records[last].erase;
        erasures += records[last].erase != 0;
        ++last;
      }
      const auto accepted = incident.find(records[first].node);
      const auto count = accepted == incident.end() ? 0 : accepted->second.size();
      if (ceiling != SIZE_MAX &&
          (count > 128 || additions > 128 || erasures > count || count - erasures + additions > 128))
        failure.Set(3, records[first].node, "Native changed incidence star exceeds the staging cap.");
      // Requested-byte model: map payload plus tree links/padding. Allocator-inclusive verification is separate.
      starBytes = transfer_memory::Add(
          starBytes, sizeof(Stars::value_type) + 4 * sizeof(void*),
          transfer_memory::Mul(count + additions, sizeof(Stars::mapped_type::value_type) + 4 * sizeof(void*)));
      first = last;
    }
    const auto peak =
        transfer_memory::Add(baseline, transfer_memory::Bytes(to), transfer_memory::Bytes(records), starBytes);
    if (peak > ceiling) failure.Set(3, 0, "Native incidence staging exceeds the dependency budget.");
    auto elected = world.elect(failure);
    if (elected.any) {
      staged.valid = false;
      staged.reason = elected.message;
      return staged;
    }
    try {
      for (const auto& r : records)
        if (!staged.stars.count(r.node)) {
          const auto accepted = incident.find(r.node);
          staged.stars.emplace(r.node, accepted == incident.end() ? Stars::mapped_type{} : accepted->second);
        }
      for (const auto& r : records)
        if (r.erase) {
          auto& star = staged.stars.at(r.node);
          const auto cell = star.find(r.cell);
          if (cell == star.end() || cell->second.version != r.version || cell->second.owner != r.owner)
            failure.Set(1, r.node, "Missing or stale removed native incidence.");
          else
            star.erase(cell);
        }
      for (const auto& r : records)
        if (!r.erase && !staged.stars.at(r.node).emplace(r.cell, r).second)
          failure.Set(1, r.node, "Duplicate native incident owner.");
    } catch (const std::bad_alloc&) {
      failure.Set(2, 0, "Native incidence staging allocation failed.");
    }
    elected = world.elect(failure);
    staged.valid = !elected.any;
    staged.reason = elected.message;
    if (!staged.valid) staged.stars.clear();
    return staged;
  }
  void commit(Prepared&& staged) {
    if (!staged.valid) throw std::logic_error("Cannot publish rejected native incidence staging.");
    for (auto it = staged.stars.begin(); it != staged.stars.end();) {
      const auto accepted = incident.find(it->first);
      if (it->second.empty()) {
        if (accepted != incident.end()) incident.erase(accepted);
        ++it;
      } else if (accepted != incident.end()) {
        accepted->second.swap(it->second);
        ++it;
      } else {
        auto transfer = it++;
        incident.insert(staged.stars.extract(transfer));
      }
    }
  }
  void update(const std::vector<Cell>& removed, const std::vector<Cell>& added) {
    auto staged = prepare(removed, added);
    CLocalFailure failure;
    if (!staged.valid) failure.Set(1, 0, staged.reason);
    world.Fail(failure, CURRENT_FUNCTION);
    commit(std::move(staged));
  }
  std::vector<Incidence> lookup(const std::set<Id>& ids, bool& overflow, bool intersection = false) {
    std::vector<std::vector<Lookup>> to(world.size);
    for (const auto id : ids) to[rendezvous(id)].push_back({id, world.rank});
    const auto requests = world.exchange(to);
    std::vector<std::vector<Incidence>> reply(world.size);
    std::vector<std::map<Id, Incidence>> bounded(world.size);
    std::set<int> tooLarge;
    for (const auto& q : requests) {
      if (!intersection && tooLarge.count(q.caller)) continue;
      const auto node = incident.find(q.node);
      if (node == incident.end()) continue;
      if (node->second.size() > PATCH_LIMIT) {
        if (intersection)
          reply[q.caller].push_back({q.node, 0, 0, -1, 0});
        else
          tooLarge.insert(q.caller);
        continue;
      }
      for (const auto& r : node->second) {
        if (intersection)
          reply[q.caller].push_back(r.second);
        else
          bounded[q.caller].emplace(r.first, r.second);
      }
      if (!intersection && bounded[q.caller].size() > PATCH_LIMIT) {
        bounded[q.caller].clear();
        tooLarge.insert(q.caller);
      }
    }
    if (!intersection)
      for (int rank = 0; rank < world.size; ++rank) {
        if (tooLarge.count(rank))
          reply[rank].push_back({0, 0, 0, -1, 0});
        else
          for (const auto& record : bounded[rank]) reply[rank].push_back(record.second);
      }
    const auto found = world.exchange(reply);
    std::map<Id, Incidence> unique;
    std::map<Id, std::set<Id>> coverage;
    CLocalFailure failure;
    for (const auto& r : found) {
      if (r.owner < 0)
        overflow = true;
      else if (unique.count(r.cell) && (unique[r.cell].owner != r.owner || unique[r.cell].version != r.version))
        failure.Set(1, r.cell, "Inconsistent native incident record.");
      else {
        unique[r.cell] = r;
        coverage[r.cell].insert(r.node);
      }
    }
    world.Fail(failure, CURRENT_FUNCTION);
    if (intersection)
      for (auto it = unique.begin(); it != unique.end();) {
        if (coverage[it->first].size() != ids.size())
          it = unique.erase(it);
        else
          ++it;
      }
    if (unique.size() > PATCH_LIMIT) overflow = true;
    std::vector<Incidence> result;
    for (const auto& r : unique) result.push_back(r.second);
    return result;
  }
  std::vector<Cell> fetch(const std::vector<Incidence>& refs, const std::map<Id, Cell>& owned, bool admitted,
                          bool& stale, bool* payloadAdmitted = nullptr, size_t ceiling = SIZE_MAX,
                          size_t baseline = 0) {
    std::vector<std::vector<CellQuery>> to(world.size);
    if (admitted)
      for (const auto& r : refs) to[r.owner].push_back({r.cell, r.version, world.rank});
    const auto requests = world.exchange(to);
    const auto packing = transfer_memory::Add(baseline, transfer_memory::Bytes(to), transfer_memory::Bytes(requests),
                                              transfer_memory::GrowthBound(requests.size(), sizeof(Payload)));
    if (world.maximum(packing > ceiling)) {
      if (payloadAdmitted) *payloadAdmitted = false;
      return {};
    }
    std::vector<std::vector<Payload>> reply(world.size);
    for (const auto& q : requests) {
      const auto found = owned.find(q.cell);
      const bool valid = found != owned.end() && found->second.version == q.version;
      reply[q.caller].push_back({valid ? found->second : Cell{}, valid});
    }
    const auto records =
        world.exchange(reply, payloadAdmitted, ceiling,
                       transfer_memory::Add(baseline, transfer_memory::Bytes(to), transfer_memory::Bytes(requests)));
    std::vector<Cell> result;
    result.reserve(records.size());
    for (const auto& r : records) {
      if (!r.present)
        stale = true;
      else
        result.push_back(r.cell);
    }
    std::sort(result.begin(), result.end(), [](const auto& a, const auto& b) { return a.t.id < b.t.id; });
    return result;
  }
  bool reserve(const std::vector<Cell>& old, bool active, int& conflicting) {
    std::set<Id> ids;
    for (const auto& c : old)
      for (const auto& v : c.t.v) ids.insert(v.id);
    std::vector<std::vector<Claim>> to(world.size);
    if (active)
      for (const auto id : ids) to[rendezvous(id)].push_back({id, world.rank});
    const auto claims = world.exchange(to);
    std::map<Id, int> winner;
    std::map<Id, std::set<int>> participants;
    for (const auto& c : claims) {
      participants[c.node].insert(c.caller);
      if (!winner.count(c.node) || c.caller < winner[c.node]) winner[c.node] = c.caller;
    }
    std::vector<std::vector<Veto>> reply(world.size);
    for (const auto& entry : participants)
      for (const int caller : entry.second)
        if (caller != winner[entry.first]) reply[caller].push_back({caller});
    const auto veto = world.exchange(reply);
    conflicting += !veto.empty();
    return active && veto.empty();
  }
  size_t resident_record_bytes() const {
    size_t count = 0;
    for (const auto& node : incident) count += node.second.size();
    return count * sizeof(Incidence);
  }
};

}  // namespace SU2NativeBoundary2D
