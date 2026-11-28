/*!
 * \file CNativeGeometryCertificates3D.hpp
 * \brief Event-private surface certificates for the mode S construction census.
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#pragma once

#include "CNativeBoundaryParameters3D.hpp"
#include "CNativeDistributed3D.hpp"
#include "CNativeMarkerTolerance3D.hpp"
#include "CNativeTimerTree3D.hpp"
#include "CTransferMemory.hpp"
#include <chrono>
#include <cmath>
#include <cstring>
#include <limits>
#include <memory>
#include <type_traits>

namespace SU2Native3D::geometry_certificates {

using Binding = SU2NativeGeometry::SurfaceBinding;
using Budget = SU2NativeGeometry::AtlasBudget;
using Clock = std::chrono::steady_clock;
using Operation = SU2NativeTiming3D::Operation;
using Scope = SU2NativeTiming3D::Scope;
enum class Origin : uint64_t { StepC, Proof, Commit };

inline uint64_t Bits(double value) {
  uint64_t bits;
  static_assert(sizeof(bits) == sizeof(value));
  std::memcpy(&bits, &value, sizeof(value));
  return bits;
}

inline void AddDigestWord(uint64_t& digest, uint64_t word) {
  for (unsigned byte = 0; byte < sizeof(word); ++byte) {
    digest ^= (word >> (8 * byte)) & 255;
    digest *= 1099511628211ULL;
  }
}

using Vertex = std::array<uint64_t, 8>;
struct Key {
  std::array<uint64_t, 6> face{};
  std::array<Vertex, 3> vertices{};
  bool operator<(const Key& other) const {
    return std::tie(face, vertices) < std::tie(other.face, other.vertices);
  }
  bool operator==(const Key& other) const { return face == other.face && vertices == other.vertices; }
};

inline Key FaceKey(const BoundaryParameterState& state, Id facet, const std::array<Binding, 3>& bindings,
                   const std::array<Point, 3>& points) {
  const auto [body, sheet, marker] = state.Entity(facet);
  Key key;
  key.face = {state.Geometry().Canonical(facet), facet, body, sheet, uint64_t(marker),
              Bits(MarkerTolerance(marker, state.GeometryTolerance()))};
  for (size_t i = 0; i < points.size(); ++i) {
    const auto& p = points[i];
    const auto& b = bindings[i];
    key.vertices[i] = {Bits(p.x), Bits(p.y), Bits(p.z), b.feature, b.version, b.dimension, Bits(b.u), Bits(b.v)};
  }
  std::sort(key.vertices.begin(), key.vertices.end());
  return key;
}

struct Record {
  Key key;
  Origin origin;
};
static_assert(std::is_trivially_copyable<Record>::value);

inline bool ByKey(const Record& a, const Record& b) { return a.key < b.key; }

inline void SortProof(std::vector<Record>& records) {
  std::sort(records.begin(), records.end(), ByKey);
  records.erase(std::unique(records.begin(), records.end(),
                            [](const auto& a, const auto& b) { return a.key == b.key; }), records.end());
}

struct Snapshot {
  std::shared_ptr<const SU2NativeGeometry::MappedFacetReference> authority;
  std::vector<char> policy;
  uint64_t policy_hint = 0;
  std::vector<Record> records;

  size_t Bytes() const {
    return transfer_memory::Add(sizeof(Snapshot) + 4 * sizeof(void*), transfer_memory::Bytes(policy),
                                transfer_memory::Bytes(records));
  }
  const Record* Find(const Key& key) const {
    const auto found = std::lower_bound(records.begin(), records.end(), key,
                                        [](const Record& row, const Key& value) { return row.key < value; });
    return found != records.end() && found->key == key ? &*found : nullptr;
  }
};

struct Owner {
  std::shared_ptr<Snapshot> accepted;
};

struct Counts {
  std::array<uint64_t, 3> origins{};
  uint64_t calls = 0, hit = 0, miss = 0, finite = 0, nonfinite = 0, skips = 0;
  double skip_seconds = 0, gather_seconds = 0;
};

struct Event {
  const int probe, rank;
  const bool timings;
  std::shared_ptr<Snapshot> old, pending;
  std::vector<Record> core, trial, committed, proof;
  Counts counts;
  size_t stage = 0, eligible_stages = 0, maximum_bytes = 0, atlas_work_limit = 0;
  size_t observer_bytes = 0;
  size_t core_charged_bytes = 0, core_live_extra = 0;
  size_t engine_charged_bytes = 0, atlas_charged_bytes = 0;
  bool use_previous = false, eligible = false;
  const SU2NativeGeometry::MappedFacetReference* stage_authority = nullptr;
  uint64_t policy_hint = 0;
  // Writer states share the immutable policy established at stage entry.
  std::vector<char> stage_policy;

  Event(int probe, int rank, bool timings) : probe(probe), rank(rank), timings(timings) {}

  size_t Bytes() const {
    return transfer_memory::Add(sizeof(Event), observer_bytes, sizeof(Owner) + 4 * sizeof(void*),
        old ? old->Bytes() : 0, pending ? pending->Bytes() : 0,
        transfer_memory::Bytes(core), transfer_memory::Bytes(trial), transfer_memory::Bytes(committed),
        transfer_memory::Bytes(proof), transfer_memory::Bytes(stage_policy));
  }

  void Check(size_t bytes, size_t limit) {
    if (bytes == SIZE_MAX || bytes > limit)
      throw std::length_error("Native3D certificate storage exceeds working-storage admission.");
    maximum_bytes = std::max(maximum_bytes, bytes);
  }

  // Account for both the old allocation and its replacement during vector growth.
  void Reserve(std::vector<Record>& rows, size_t count, size_t live, size_t ceiling) {
    if (count <= rows.capacity()) return;
    Check(transfer_memory::Add(live, transfer_memory::Mul(count, sizeof(Record))), ceiling);
    rows.reserve(count);
  }

  void Push(std::vector<Record>& rows, Record record, size_t live, size_t ceiling) {
    if (rows.size() == rows.capacity()) {
      const auto count = std::max<size_t>(8, transfer_memory::Add(rows.size(), rows.size() / 2 + 1));
      Reserve(rows, count, live, ceiling);
    }
    rows.push_back(std::move(record));
  }

  bool SameStage(const BoundaryParameterState& state) const {
    return stage_authority == &state.Geometry() && policy_hint == state.Mobility().identity;
  }

  void StartStage(const BoundaryParameterState& state, size_t number, size_t live, size_t ceiling) {
    stage = number;
    stage_authority = &state.Geometry();
    policy_hint = state.Mobility().identity;
    const auto length = BoundaryAtlasBytes({}, state.Mobility());
    Check(transfer_memory::Add(live, length), ceiling);
    stage_policy = EncodeBoundaryAtlas({}, state.Mobility(), ceiling, live);
    use_previous = old && old->authority.get() == stage_authority &&
                   old->policy_hint == policy_hint && old->policy == stage_policy;
  }

  void CheckStage(const BoundaryParameterState& state, size_t live, size_t ceiling) {
    const auto length = BoundaryAtlasBytes({}, state.Mobility());
    Check(transfer_memory::Add(live, length), ceiling);
    if (!SameStage(state) || stage_policy != EncodeBoundaryAtlas({}, state.Mobility(), ceiling, live))
      throw std::invalid_argument("Native3D certificate stage authority or policy changed.");
  }

  void FinishCore(bool closed) {
    eligible = closed && stage_authority;
    if (!eligible) {
      std::vector<Record>().swap(core);
      return;
    }
    ++eligible_stages;
    SortProof(core);
  }

  void DiscardStage() {
    std::vector<Record>().swap(core);
    std::vector<Record>().swap(trial);
    std::vector<Record>().swap(committed);
    std::vector<Record>().swap(proof);
    std::vector<char>().swap(stage_policy);
    core_charged_bytes = core_live_extra = 0;
    engine_charged_bytes = atlas_charged_bytes = 0;
    use_previous = eligible = false;
    stage_authority = nullptr;
  }

  std::string Gather(World& world, const BoundaryParameterState& state, size_t live, size_t ceiling) {
    Scope timer(Operation::Certification);
    const auto started = timings ? Clock::now() : Clock::time_point{};
    CLocalFailure failure;
    std::vector<uint64_t> lengths;
    std::vector<size_t> offsets;
    std::vector<int> counts, displacements;
    size_t metadata_bytes = 0;
    try {
      CheckStage(state, live, ceiling);
      metadata_bytes = transfer_memory::Mul(world.size + 1, sizeof(uint64_t) + sizeof(size_t) + 2 * sizeof(int));
      Check(transfer_memory::Add(live, metadata_bytes), ceiling);
      lengths.resize(world.size);
      offsets.resize(world.size + 1);
      counts.resize(world.size);
      displacements.resize(world.size);
    } catch (const std::exception& error) {
      failure.Set(3, 0, error.what());
    }
    if (const auto elected = world.elect(failure); elected.any) return elected.message;
    const uint64_t local = proof.size();
#ifdef HAVE_MPI
    MPI_Allgather(&local, sizeof(local), MPI_BYTE, lengths.data(), sizeof(local), MPI_BYTE, world.comm);
#else
    lengths[0] = local;
#endif
    try {
      for (int r = 0; r < world.size; ++r)
        offsets[r + 1] = transfer_memory::Add(offsets[r], lengths[r]);
      Check(transfer_memory::Add(live, metadata_bytes, sizeof(Snapshot) + 4 * sizeof(void*),
                                transfer_memory::Mul(offsets.back(), sizeof(Record))), ceiling);
      pending = std::make_shared<Snapshot>();
      pending->records.resize(offsets.back());
    } catch (const std::exception& error) {
      failure.Set(3, 0, error.what());
    }
    if (const auto elected = world.elect(failure); elected.any) return elected.message;
#ifdef HAVE_MPI
    // Receive directly into the admitted snapshot; only MPI's integer domain splits a logical gather.
    constexpr size_t records_per_round = std::numeric_limits<int>::max() / sizeof(Record);
    for (size_t start = 0; start < offsets.back();) {
      const size_t end = start + std::min(records_per_round, offsets.back() - start);
      for (int r = 0; r < world.size; ++r) {
        const auto first = std::max(start, offsets[r]);
        const auto last = std::min(end, offsets[r + 1]);
        counts[r] = first < last ? int((last - first) * sizeof(Record)) : 0;
        displacements[r] = counts[r] ? int((first - start) * sizeof(Record)) : 0;
      }
      const auto* source = counts[world.rank]
          ? proof.data() + (std::max(start, offsets[world.rank]) - offsets[world.rank]) : nullptr;
      MPI_Allgatherv(source, counts[world.rank], MPI_BYTE, pending->records.data() + start,
                     counts.data(), displacements.data(), MPI_BYTE, world.comm);
      start = end;
    }
#else
    std::copy(proof.begin(), proof.end(), pending->records.begin());
#endif
    SortProof(pending->records);
    pending->authority = state.Authority();
    pending->policy = std::move(stage_policy);
    pending->policy_hint = policy_hint;
    if (timings) this->counts.gather_seconds = std::chrono::duration<double>(Clock::now() - started).count();
    return {};
  }

  const Record* Find(const Key& key) const {
    if (use_previous)
      if (const auto* match = old->Find(key)) return match;
    for (const auto* records : {&core, &committed}) {
      const auto found = std::lower_bound(records->begin(), records->end(), key,
                                          [](const Record& row, const Key& value) { return row.key < value; });
      if (found != records->end() && found->key == key) return &*found;
    }
    return nullptr;
  }

  void Disagree(const Key& key, double value, Origin origin) {
    if (std::isfinite(value)) ++counts.finite;
    else ++counts.nonfinite;
    Scope output(Operation::Output);
    std::cout << "NATIVE3D_GEOMETRY_CERTIFICATE_DISAGREE rank=" << rank << " stage=" << stage
              << " facet=" << key.face[1] << " entity_body=" << key.face[2] << " entity_sheet=" << key.face[3]
              << " marker=" << key.face[4] << " kind=" << (std::isfinite(value) ? "finite" : "nonfinite")
              << " legacy=" << std::setprecision(17) << value << " origin=" << uint64_t(origin);
    for (size_t i = 0; i < key.vertices.size(); ++i) {
      std::cout << " v" << i << '=';
      for (size_t j = 0; j < key.vertices[i].size(); ++j)
        std::cout << (j ? "," : "") << key.vertices[i][j];
    }
    std::cout << '\n';
  }

  void Report(int workers, size_t selected, size_t rank_eligible_stages) const {
    if (!timings) return;
    Scope output(Operation::Output);
    std::cout << "NATIVE3D_GEOMETRY_CERTIFICATES rank=" << rank << " workers=" << workers
              << " probe=" << probe << " stage=" << selected << " eligible_stages=" << rank_eligible_stages
              << " reader_calls=" << counts.calls << " cert_hit=" << counts.hit << " cert_miss=" << counts.miss
              << " hit_step_c=" << counts.origins[0] << " hit_proof=" << counts.origins[1]
              << " hit_commit=" << counts.origins[2] << " records=" << (pending ? pending->records.size() : 0)
              << " bytes=" << (pending ? pending->Bytes() : 0)
              << " atlas_work_limit=" << atlas_work_limit
              << " gather_seconds=" << std::setprecision(17) << counts.gather_seconds;
    if (probe == 6)
      std::cout << " cert_disagree=" << counts.finite + counts.nonfinite << " disagree_finite=" << counts.finite
                << " disagree_nonfinite=" << counts.nonfinite << " skip_calls=" << counts.skips
                << " skip_seconds=" << counts.skip_seconds;
    std::cout << '\n' << std::flush;
  }
};

inline thread_local Event* active = nullptr;
inline thread_local const double* reader_tolerance = nullptr;
inline thread_local bool collect_commit = false;

template <class T>
class ScopedValue {
 public:
  ScopedValue(T& slot, T value) : slot(slot), saved(slot) { slot = value; }
  ~ScopedValue() { slot = saved; }
  ScopedValue(const ScopedValue&) = delete;
  ScopedValue& operator=(const ScopedValue&) = delete;

 private:
  T& slot;
  const T saved;
};

inline bool Qualified(double bound, double tolerance) {
  return std::isfinite(bound) && bound >= 0 && bound <= tolerance;
}

}  // namespace SU2Native3D::geometry_certificates
