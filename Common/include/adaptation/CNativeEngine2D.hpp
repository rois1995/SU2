/*!
 * \file CNativeEngine2D.hpp
 * \brief Synchronous owned-cell transactions against a frozen nodal target and immutable geometry.
 * \version 8.5.0 "Harrier"
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md)
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */

#pragma once

#include "CNativeField2D.hpp"

namespace SU2NativeBoundary2D {

struct Choice {
  Operation op;
  double score = -1;
  int rank = 0;
  template <class S>
  void Fields(S& s) {
    s(op, score, rank);
  }
};
struct EraseCell {
  Id id = 0;
  uint64_t version = 0;
  int caller = 0;
  template <class S>
  void Fields(S& s) {
    s(id, version, caller);
  }
};
struct NewCell {
  Cell cell;
  int caller = 0;
  template <class S>
  void Fields(S& s) {
    s(cell, caller);
  }
};
struct Perimeter {
  Node a, b, apex;
  int marker = 0, caller = 0, old = 0;
  template <class S>
  void Fields(S& s) {
    s(a, b, apex, marker, caller, old);
  }
};
struct Decision {
  int caller = 0, approve = 0, cross_rank = 0;
  template <class S>
  void Fields(S& s) {
    s(caller, approve, cross_rank);
  }
};
struct EngineOptions {
  size_t dependency_bytes = 2 * 1024 * 1024;
  double geometry_tolerance = 0;
  int phase_rounds = 300, sweeps = 6;
  bool ordered = false, fixed_boundary = false;
  MetricComposition metric_composition;
};
struct EngineStats {
  int rounds = 0, commits = 0, cross_rank = 0, conflicts = 0, size_rejected = 0, memory_rejected = 0,
      stale_rejected = 0;
  size_t max_patch = 0, max_donors = 0;
  int joint_commits = 0;
  double joint_seconds = 0;
  std::array<int, 8> accepted{};
  std::array<double, 8> phase_seconds{};
  std::array<double, 8> choice_seconds{};
  std::array<uint64_t, 8> selection_scans{};
  std::map<std::string, int> rejected;
};

/*--- Input consists of uniquely owned positive cells with physical component labels and original nodal tensors.
 *    Import validation/reference persistence belong to the SU2 adapter. No complete volume mesh is gathered.
 *    Each rank may propose one transaction per round; node reservations serialize overlapping write/read stars. ---*/
class Engine {
 public:
  World& world;
  Directory directory;
  Reference reference;
  EngineOptions options;
  std::map<Id, Cell> owned;
  DonorField donor;
  EngineStats stats;
  bool last_deferred = false, last_committed = false;
  Choice last_selected;

  Engine(World& w, std::map<Id, Cell> input, Reference policy, EngineOptions control)
      : world(w),
        directory(w),
        reference(std::move(policy)),
        options(control),
        owned(std::move(input)),
        donor(w, owned, options.metric_composition) {
    CLocalFailure failure;
    if (!(std::isfinite(options.geometry_tolerance) && options.geometry_tolerance > 0) || !options.dependency_bytes ||
        options.phase_rounds < 1 || options.sweeps < 1)
      failure.Set(1, 0, "Invalid native transaction controls.");
    Id largestPoint = 0, largestCell = 0;
    std::vector<Cell> added;
    added.reserve(owned.size());
    try {
      for (auto& entry : owned) {
        auto& cell = entry.second;
        if (entry.first != cell.t.id) throw std::runtime_error("Native owner map/cell identity mismatch.");
        FieldPatch patch;
        patch.composition = donor.composition;
        patch.cells.push_back(donor.owned.at(entry.first));
        for (const auto marker : cell.marker)
          if (marker) patch.extension_components.insert(marker);
        CacheTarget(cell, checked([&](Point p) { return patch.evaluate(p); }), epoch);
        for (const auto& v : cell.t.v) largestPoint = std::max(largestPoint, v.id);
        largestCell = std::max(largestCell, cell.t.id);
        added.push_back(cell);
      }
    } catch (const std::exception& error) {
      failure.Set(1, 0, error.what());
    }
    CollectiveFailure(failure, CURRENT_FUNCTION);
    largestPoint = CPassiveComm::Allreduce(largestPoint, CPassiveComm::Op::MAX);
    largestCell = CPassiveComm::Allreduce(largestCell, CPassiveComm::Op::MAX);
    if (largestPoint == UINT64_MAX || largestCell == UINT64_MAX) failure.Set(1, 0, "Native identity range exhausted.");
    CollectiveFailure(failure, CURRENT_FUNCTION);
    nextPoint = largestPoint + 1;
    nextCell = largestCell + 1;
    directory.update({}, added);
  }

  static bool Better(const Choice& a, const Choice& b) {
    return a.score > b.score || (a.score == b.score && std::tie(a.op.a, a.op.b) < std::tie(b.op.a, b.op.b));
  }

  Choice CellChoice(const Cell& cell, Action action, const std::set<Edge>& attempted, bool coordinated = false) const {
    Choice best;
    best.rank = world.rank;
    best.op.action = action;
    auto add = [&](Id a, Id b, double score) {
      const Edge key{a, b};
      if (!(score >= 0)) return;
      if (score > best.score || (score == best.score && key < Edge{best.op.a, best.op.b})) {
        // A losing candidate cannot replace best, regardless of whether it was
        // attempted. Avoid the profiled tree lookup without changing priority.
        if (attempted.count(key)) return;
        best = {{action, a, b}, score, world.rank};
      }
    };
    for (int k = 0; k < 3; ++k) {
      const auto a = cell.t.v[k], b = cell.t.v[(k + 1) % 3];
      const auto key = edge(a.id, b.id);
      const auto length = cell.target_cache[k + 1];
      if (int(action) >= 4) {
        if (cell.t.protected_cell) continue;
        if (coordinated) {
          if (action == Action::BULK_SPLIT && !cell.marker[k] && length > 1.8) add(key.first, key.second, length - 1.8);
          continue;
        }
        if (action == Action::BULK_REMOVE && (length < .65 || cell.target_cache[0] < .18)) {
          const double demand = std::max(.65 - length, .18 - cell.target_cache[0]);
          if (!a.fixed) add(a.id, 0, demand);
          if (!b.fixed) add(b.id, 0, demand);
        }
        if (action == Action::BULK_SPLIT && length > 1.6) add(key.first, key.second, length - 1.6);
        // Shape repair can require a different cavity even when all edges
        // fit. SplitPatch admits this insertion only if it improves shape.
        if (action == Action::BULK_SPLIT && cell.target_cache[0] < .18 && !cell.marker[k])
          add(key.first, key.second, .18 - cell.target_cache[0]);
        if (action == Action::BULK_FLIP) add(key.first, key.second, std::max(1 - cell.target_cache[0], length - 1.8));
        if (action == Action::BULK_MOVE && !a.fixed) add(a.id, 0, std::max(1 - cell.target_cache[0], length - 1.8));
        continue;
      }
      if (!cell.marker[k]) continue;
      const Physical face{a, b, cell.marker[k]};
      const auto h = reference.height(face.marker);
      if (action == Action::HEIGHT && h > 0) {
        const auto actual = static_cast<double>(2 * area(cell.t)) / norm(b.p - a.p);
        if (std::abs(actual / h - 1) > 1e-8) add(key.first, key.second, std::abs(actual / h - 1));
      }
      if (options.fixed_boundary) continue;
      if (coordinated) {
        if (action == Action::SPLIT && h == 0 && !cell.t.protected_cell) {
          const double demand = std::max(.18 / cell.target_cache[0],
              *std::max_element(cell.target_cache.begin() + 1, cell.target_cache.end()) / 1.8);
          if (demand > 1) add(key.first, key.second, demand - 1);
        }
        continue;
      }
      if (action == Action::SPLIT) {
        auto demand = std::max(length / 1.6, reference.deviation(face) / options.geometry_tolerance);
        // A wall base can already fit while a side edge or wall-cell shape
        // violates the target. Only the coupled surface operator can repair
        // this protected cell while reconstructing its prescribed altitude.
        if (cell.t.protected_cell) {
          demand = std::max(demand, .18 / cell.target_cache[0]);
          for (int j = 1; j < 4; ++j) demand = std::max(demand, cell.target_cache[j] / 1.8);
        }
        const auto score = demand - 1;
        if (score > 0) add(key.first, key.second, score);
      }
      if (action == Action::REMOVE && length < .65) {
        if (!(a.fixed & FEATURE)) add(a.id, 0, .65 - length);
        if (!(b.fixed & FEATURE)) add(b.id, 0, .65 - length);
      }
      if (action == Action::REDISTRIBUTE) {
        if (!(a.fixed & FEATURE)) add(a.id, 0, .1);
        if (!(b.fixed & FEATURE)) add(b.id, 0, .1);
      }
    }
    return best;
  }

  Choice choose(Action action, const std::set<Edge>& attempted, bool coordinated = false) const {
    Choice best;
    best.rank = world.rank;
    best.op.action = action;
    for (const auto& entry : owned) {
      const auto choice = CellChoice(entry.second, action, attempted, coordinated);
      if (Better(choice, best)) best = choice;
    }
    return best;
  }

  // Phase-local storage only. A discarded choice bounds every unvisited cell:
  // attempted keys grow monotonically, and round() reports every published new
  // cell. Refill whenever that bound could beat the cached best. This preserves
  // exhaustive selection, including score/key ties, without a global index.
  static constexpr size_t CANDIDATE_BATCH = 64;
  struct SelectionCache {
    struct Entry {
      Id cell = 0;
      uint64_t version = 0;
      Choice choice;
    };
    std::array<Entry, CANDIDATE_BATCH> entries{};
    size_t size = 0;
    Choice discarded;
    uint64_t stamp = 0;
    bool valid = false;
  };
  static_assert(sizeof(SelectionCache) <= 8192, "Native local selection cache exceeds its fixed storage bound.");

  Choice chooseCached(Action action, const std::set<Edge>& attempted, SelectionCache& cache, bool coordinated = false) {
    auto insert = [&](const Cell& cell) {
      const auto choice = CellChoice(cell, action, attempted, coordinated);
      if (choice.score < 0) return;
      size_t position = 0;
      while (position < cache.size && !Better(choice, cache.entries[position].choice)) ++position;
      if (position == CANDIDATE_BATCH) {
        if (Better(choice, cache.discarded)) cache.discarded = choice;
        return;
      }
      if (cache.size == CANDIDATE_BATCH) {
        if (Better(cache.entries.back().choice, cache.discarded)) cache.discarded = cache.entries.back().choice;
      } else {
        ++cache.size;
      }
      for (size_t k = cache.size - 1; k > position; --k) cache.entries[k] = cache.entries[k - 1];
      cache.entries[position] = {cell.t.id, cell.version, choice};
    };
    auto refill = [&]() {
      cache.size = 0;
      cache.discarded = {};
      cache.stamp = epoch;
      cache.valid = true;
      for (const auto& entry : owned) insert(entry.second);
      ++stats.selection_scans[int(action)];
    };
    if (!cache.valid || (cache.stamp != epoch && (epoch - cache.stamp != 1 || changedOverflow))) {
      refill();
    } else {
      size_t kept = 0;
      for (size_t k = 0; k < cache.size; ++k) {
        auto entry = cache.entries[k];
        const auto found = owned.find(entry.cell);
        if (found == owned.end() || found->second.version != entry.version) continue;
        entry.choice = CellChoice(found->second, action, attempted, coordinated);
        if (entry.choice.score >= 0) cache.entries[kept++] = entry;
      }
      cache.size = kept;
      std::sort(cache.entries.begin(), cache.entries.begin() + cache.size,
                [](const auto& a, const auto& b) { return Better(a.choice, b.choice); });
      if (cache.stamp != epoch)
        for (size_t k = 0; k < changedCount; ++k) insert(owned.at(changedCells[k]));
      cache.stamp = epoch;
      if (!cache.size || Better(cache.discarded, cache.entries[0].choice)) refill();
    }
    Choice result;
    result.rank = world.rank;
    result.op.action = action;
    return cache.size ? cache.entries[0].choice : result;
  }

  bool round(Choice choice, bool coordinated = false) {
    ++stats.rounds;
    changedCount = 0;
    changedOverflow = false;
    last_deferred = false;
    last_committed = false;
    const int action = static_cast<int>(choice.op.action);
    const int protocol = action + (coordinated ? 8 : 0);
    const int minimum = CPassiveComm::Allreduce(protocol, CPassiveComm::Op::MIN);
    const int maximum = CPassiveComm::Allreduce(protocol, CPassiveComm::Op::MAX);
    CLocalFailure protocolFailure;
    if (action < 0 || action > 7 || minimum != maximum || (coordinated && choice.op.action != Action::BULK_SPLIT && choice.op.action != Action::SPLIT))
      protocolFailure.Set(1, 0, "Native transaction actions or reconstruction modes differ or are unsupported.");
    CollectiveFailure(protocolFailure, CURRENT_FUNCTION);
    if (options.ordered) {
      const auto choices = world.metadata(std::vector<Choice>{choice});
      Choice best;
      for (const auto& c : choices)
        if (c.score > best.score ||
            (c.score == best.score && std::tie(c.op.a, c.op.b, c.rank) < std::tie(best.op.a, best.op.b, best.rank)))
          best = c;
      last_selected = best;
      if (best.rank != world.rank) {
        last_deferred = choice.score >= 0;
        choice.score = -1;
      }
    }
    bool active = choice.score >= 0, overflow = false, stale = false;
    const bool surface = int(choice.op.action) < 4;
    std::set<Id> seeds;
    if (active) {
      seeds.insert(choice.op.a);
      if (choice.op.action == Action::SPLIT || choice.op.action == Action::HEIGHT ||
          choice.op.action == Action::BULK_SPLIT || choice.op.action == Action::BULK_FLIP)
        seeds.insert(choice.op.b);
    }
    std::vector<Incidence> refs;
    std::vector<Cell> old, fresh;
    for (int growth = 0; growth < (surface ? 3 : coordinated ? 2 : 1); ++growth) {
      // A split imports the union of both endpoint stars for bounded private
      // cavity growth; a flip still needs only the two incident cells.
      refs = directory.lookup(seeds, overflow, choice.op.action == Action::BULK_FLIP);
      stats.max_patch = std::max(stats.max_patch, refs.size());
      const auto required =
          transfer_memory::Mul(refs.size(), 4 * sizeof(Cell) + 6 * sizeof(Incidence) + 8 * sizeof(Claim));
      if (active && overflow) {
        ++stats.size_rejected;
        active = false;
      }
      if (active && required > options.dependency_bytes) {
        ++stats.memory_rejected;
        active = false;
      }
      bool payloadAdmitted = false;
      auto imported = directory.fetch(refs, owned, active, stale, &payloadAdmitted, options.dependency_bytes,
                                      transfer_memory::Add(transfer_memory::Bytes(old), transfer_memory::Bytes(refs)));
      if (active && !payloadAdmitted) {
        ++stats.memory_rejected;
        active = false;
      }
      old = std::move(imported);
      if (stale) {
        ++stats.stale_rejected;
        active = false;
      }
      if (surface && active) Enlarge(old, seeds);
      if (coordinated && active)
        for (const auto& cell : old)
          for (const auto& vertex : cell.t.v) seeds.insert(vertex.id);
      if (!active) seeds.clear();
    }
    const bool reserved = directory.reserve(old, active, stats.conflicts);
    last_deferred |= active && !reserved;
    active = reserved;
    auto field = donor.import(old, active, options.dependency_bytes, options.geometry_tolerance, stats.max_donors,
                              stats.memory_rejected);
    const Metric target = checked([field](Point p) { return field->evaluate(p); });
    const Id pointBase = Allocate(nextPoint, active ? 3 * PATCH_LIMIT + 1 : 0);
    const Id cellBase = Allocate(nextCell, active ? 3 * PATCH_LIMIT : 0);
    bool approved = false;
    std::string reason;
    if (active) {
      try {
        // Unchanged vertices already have an authoritative sample of this
        // immutable target. Different rounded donor-edge projections may
        // differ by an ulp; keep that sample identical across incident cells.
        // New/moved points still query the imported original donor field.
        for (const auto& cell : old)
          for (int k = 0; k < 3; ++k) {
            const auto p = cell.t.v[k].p;
            if (field->cache.size() < QUERY_CACHE_LIMIT)
              field->cache.emplace(std::array<double, 2>{p.x, p.y}, cell.nodal_target[k]);
          }
        approved = reconstruct(choice.op, old, reference, target, pointBase, fresh, reason, options.geometry_tolerance,
                               coordinated);
        if (approved && fresh.size() > 3 * PATCH_LIMIT) {
          approved = false;
          reason = "replacement cell cap";
        }
        if (approved)
          for (size_t i = 0; i < fresh.size(); ++i) {
            auto& cell = fresh[i];
            cell.t.id = cellBase + i;
            cell.version = epoch;
            CacheTarget(cell, target, epoch);
            for (const auto& v : cell.t.v)
              if (v.id >= pointBase + 3 * PATCH_LIMIT + 1)
                throw std::runtime_error("Native replacement exceeded allocated point identities.");
          }
      } catch (const std::exception& error) {
        approved = false;
        reason = error.what();
      }
      if (!approved) ++stats.rejected[reason];
    }
    std::vector<Perimeter> certificate;
    if (approved)
      for (const int oldFlag : {1, 0}) {
        const auto& cells = oldFlag ? old : fresh;
        const auto exposed = boundary(triangles(cells));
        for (const auto& cell : cells)
          for (int k = 0; k < 3; ++k)
            if (exposed.count(edge(cell.t.v[k].id, cell.t.v[(k + 1) % 3].id)))
              certificate.push_back(
                  {cell.t.v[k], cell.t.v[(k + 1) % 3], cell.t.v[(k + 2) % 3], cell.marker[k], world.rank, oldFlag});
      }
    if (choice.op.fault == 4) {
      auto row =
          std::find_if(certificate.begin(), certificate.end(), [](const auto& r) { return !r.old && !r.marker; });
      if (row == certificate.end())
        row = std::find_if(certificate.begin(), certificate.end(), [](const auto& r) { return !r.old; });
      if (row != certificate.end()) row->a.fixed ^= 1;
    }
    if (surface) {
      bool collisionAdmitted = false;
      const auto all = world.exchange(std::vector<std::vector<Perimeter>>(world.size, certificate), &collisionAdmitted,
                                      options.dependency_bytes,
                                      transfer_memory::Add(transfer_memory::Bytes(old), transfer_memory::Bytes(fresh),
                                                           transfer_memory::Bytes(certificate)));
      if (!collisionAdmitted) {
        ++stats.memory_rejected;
        approved = false;
      }
      if (!CollisionVeto(all).empty()) {
        approved = false;
        ++stats.rejected["physical collision against current snapshot"];
      }
    }
    std::vector<std::vector<EraseCell>> eraseTo(world.size);
    std::vector<std::vector<NewCell>> freshTo(world.size);
    std::vector<std::vector<Perimeter>> perimeterTo(world.size);
    std::set<int> participants;
    if (approved) {
      for (const auto& r : refs) {
        eraseTo[r.owner].push_back({r.cell, r.version, world.rank});
        participants.insert(r.owner);
      }
      for (const auto& cell : fresh) {
        const auto owner = Owner(cell, old, refs);
        freshTo[owner].push_back({cell, world.rank});
        participants.insert(owner);
      }
      for (const auto rank : participants) perimeterTo[rank] = certificate;
    }
    bool admitted = true;
    const auto baseline =
        transfer_memory::Add(transfer_memory::Bytes(old), transfer_memory::Bytes(fresh), transfer_memory::Bytes(refs),
                             transfer_memory::Bytes(certificate), transfer_memory::Bytes(field->cells),
                             transfer_memory::Mul(field->cache.size(), 96));
    const auto pendingErase = world.exchange(
        eraseTo, &admitted, options.dependency_bytes,
        transfer_memory::Add(baseline, transfer_memory::Bytes(freshTo), transfer_memory::Bytes(perimeterTo)));
    bool freshAdmitted = true, perimeterAdmitted = true;
    if (!admitted)
      for (auto& bucket : freshTo) bucket.clear();
    const auto pendingFresh =
        world.exchange(freshTo, &freshAdmitted, options.dependency_bytes,
                       transfer_memory::Add(baseline, transfer_memory::Bytes(pendingErase),
                                            transfer_memory::Bytes(eraseTo), transfer_memory::Bytes(perimeterTo)));
    if (!admitted || !freshAdmitted)
      for (auto& bucket : perimeterTo) bucket.clear();
    const auto perimeter = world.exchange(
        perimeterTo, &perimeterAdmitted, options.dependency_bytes,
        transfer_memory::Add(baseline, transfer_memory::Bytes(pendingErase), transfer_memory::Bytes(pendingFresh),
                             transfer_memory::Bytes(eraseTo), transfer_memory::Bytes(freshTo)));
    if (!admitted || !freshAdmitted || !perimeterAdmitted) {
      approved = false;
      ++stats.memory_rejected;
    }
    const auto veto = ValidateStaging(pendingErase, pendingFresh, perimeter);
    if (!veto.empty()) {
      approved = false;
      ++stats.stale_rejected;
      ++stats.rejected["participant geometry/version certificate"];
    }
    const auto decisions = world.metadata(std::vector<Decision>{{world.rank, approved, participants.size() > 1}});
    std::set<int> commit;
    for (const auto& decision : decisions)
      if (decision.approve) commit.insert(decision.caller);
    std::vector<Cell> removed, added;
    std::map<Id, Cell> staged;
    CLocalFailure failure;
    try {
      for (const auto& erase : pendingErase)
        if (commit.count(erase.caller)) removed.push_back(owned.at(erase.id));
      for (const auto& cell : pendingFresh)
        if (commit.count(cell.caller)) {
          if (!staged.emplace(cell.cell.t.id, cell.cell).second)
            throw std::runtime_error("Duplicate prepared native cell.");
          added.push_back(cell.cell);
        }
    } catch (const std::exception& error) {
      failure.Set(1, 0, error.what());
    }
    const auto failed = ElectFailure(failure);
    if (failed.any) {
      approved = false;
      ++stats.rejected[failed.message];
    }
    const auto stagingBytes = transfer_memory::Add(
        baseline, transfer_memory::Bytes(pendingErase), transfer_memory::Bytes(pendingFresh),
        transfer_memory::Bytes(perimeter), transfer_memory::Bytes(removed), transfer_memory::Bytes(added),
        transfer_memory::Mul(staged.size(), sizeof(std::map<Id, Cell>::value_type) + 4 * sizeof(void*)));
    auto prepared = directory.prepare(failed.any ? std::vector<Cell>{} : removed,
                                      failed.any ? std::vector<Cell>{} : added, options.dependency_bytes, stagingBytes);
    if (!prepared.valid) {
      approved = false;
      ++stats.rejected[prepared.reason];
    }
    // Final common vote is after all publication storage, participant checks and directory nodes are prepared.
    const bool publish = !failed.any && prepared.valid;
    if (publish) {
      // Fixed storage: tracking cannot allocate or fail after the final vote.
      // Oversized publications simply force the next cache lookup to rescan.
      changedOverflow = staged.size() > changedCells.size();
      if (!changedOverflow)
        for (const auto& entry : staged) changedCells[changedCount++] = entry.first;
      for (const auto& cell : removed) owned.erase(cell.t.id);
      owned.merge(staged);
      directory.commit(std::move(prepared));
      last_committed = !commit.empty();
      if (approved) {
        ++stats.commits;
        stats.joint_commits += coordinated;
        ++stats.accepted[int(choice.op.action)];
        stats.cross_rank += participants.size() > 1;
      }
    }
    ++epoch;
    return publish && approved;
  }

  void phase(Action action, bool coordinated = false) {
    const double start = world.seconds();
    std::set<Edge> attempted;
    SelectionCache cache;
    for (int iteration = 0; iteration < (coordinated ? std::min(128, options.phase_rounds) : options.phase_rounds);
         ++iteration) {
      const double choosing = world.seconds();
      const auto choice = int(action) >= 4 ? chooseCached(action, attempted, cache, coordinated)
                                           : choose(action, attempted, coordinated);
      stats.choice_seconds[int(action)] += world.seconds() - choosing;
      if (!world.sum(choice.score >= 0)) break;
      const bool accepted = round(choice, coordinated);
      if (options.ordered) {
        if (!last_committed || action == Action::REDISTRIBUTE)
          attempted.insert({last_selected.op.a, last_selected.op.b});
      } else if (choice.score >= 0 && ((!accepted && !last_deferred) || action == Action::REDISTRIBUTE))
        attempted.insert({choice.op.a, choice.op.b});
    }
    const double elapsed = world.seconds() - start;
    stats.phase_seconds[int(action)] += elapsed;
    if (coordinated) stats.joint_seconds += elapsed;
  }
  bool satisfied() const {
    uint64_t missed = 0;
    CLocalFailure failure;
    try {
      for (const auto& entry : owned) {
        const auto& cell = entry.second;
        missed += cell.target_cache[0] < .18 ||
                  *std::max_element(cell.target_cache.begin() + 1, cell.target_cache.end()) > 1.8;
        for (int k = 0; k < 3; ++k)
          if (cell.marker[k]) {
            const auto a = cell.t.v[k], b = cell.t.v[(k + 1) % 3];
            missed += reference.deviation({a, b, cell.marker[k]}) > options.geometry_tolerance;
            const double height = reference.height(cell.marker[k]);
            if (height > 0) missed += std::abs(double(2 * area(cell.t)) / norm(b.p - a.p) / height - 1) > 1e-8;
          }
      }
    } catch (const std::exception& error) {
      failure.Set(1, 0, error.what());
    }
    CollectiveFailure(failure, CURRENT_FUNCTION);
    return CPassiveComm::Allreduce(missed, CPassiveComm::Op::SUM) == 0;
  }
  void repair() {
    // Reconsider failed edges after neighboring commits, with a finite work
    // allowance. Original target, wall constraints and final gates are unchanged.
    for (int pass = 0; pass < options.sweeps; ++pass) {
      if (satisfied()) break;
      const int before = stats.commits;
      phase(Action::BULK_SPLIT, true);
      if (!satisfied()) phase(Action::SPLIT, true);
      if (!world.sum(stats.commits - before)) break;
    }
  }
  void adapt() {
    for (int sweep = 0; sweep < options.sweeps; ++sweep) {
      for (const auto action : {Action::HEIGHT, Action::SPLIT, Action::REMOVE, Action::REDISTRIBUTE,
                                Action::BULK_REMOVE, Action::BULK_SPLIT, Action::BULK_FLIP, Action::BULK_MOVE})
        phase(action);
      if (satisfied()) return;
    }
    repair();
  }

 private:
  Id nextPoint = 0, nextCell = 0;
  uint64_t epoch = 1;
  std::array<Id, CANDIDATE_BATCH> changedCells{};
  size_t changedCount = 0;
  bool changedOverflow = false;
  Id Allocate(Id& next, size_t amount) {
    const auto total = CPassiveComm::Allreduce(uint64_t(amount), CPassiveComm::Op::SUM);
    CLocalFailure failure;
    if (next > UINT64_MAX - total) failure.Set(1, 0, "Native identity allocation overflow.");
    if (epoch == UINT64_MAX) failure.Set(1, 0, "Native transaction version range exhausted.");
    CollectiveFailure(failure, CURRENT_FUNCTION);
    static_assert(sizeof(unsigned long) == sizeof(Id), "Native MPI prefix requires 64-bit SU2 indices.");
    const auto offset = CPassiveComm::ExscanSum(amount);
    const auto first = next + offset;
    next += total;
    return first;
  }
  void Enlarge(const std::vector<Cell>& old, std::set<Id>& seeds) const {
    std::set<Id> physicalNodes;
    for (const auto& cell : old)
      for (int k = 0; k < 3; ++k)
        if (cell.marker[k]) {
          physicalNodes.insert(cell.t.v[k].id);
          physicalNodes.insert(cell.t.v[(k + 1) % 3].id);
        }
    for (const auto& cell : old)
      for (int k = 0; k < 3; ++k)
        if (cell.marker[k] && reference.is_wall(cell.marker[k])) {
          const auto a = cell.t.v[k], b = cell.t.v[(k + 1) % 3];
          const auto e = b.p - a.p;
          const auto h = reference.height(cell.marker[k]);
          seeds.insert(cell.t.v[(k + 2) % 3].id);
          for (const auto& neighbor : old)
            for (const auto& v : neighbor.t.v)
              if (!physicalNodes.count(v.id)) {
                const auto projection = dot(v.p - a.p, e) / dot(e, e);
                const auto altitude = static_cast<double>(orient(a.p, b.p, v.p)) / norm(e);
                if (projection >= 0 && projection <= 1 && altitude >= 0 && altitude < 1.15 * h) seeds.insert(v.id);
              }
        }
  }
  int Owner(const Cell& fresh, const std::vector<Cell>& old, const std::vector<Incidence>& refs) const {
    const auto p = centroid(fresh.t);
    Id parent = UINT64_MAX;
    double distance = std::numeric_limits<double>::infinity();
    for (const auto& cell : old) {
      const bool contains = orient(cell.t.v[0].p, cell.t.v[1].p, p) >= 0 &&
                            orient(cell.t.v[1].p, cell.t.v[2].p, p) >= 0 &&
                            orient(cell.t.v[2].p, cell.t.v[0].p, p) >= 0;
      const auto d = contains ? 0. : norm(centroid(cell.t) - p);
      if (d < distance || (d == distance && cell.t.id < parent)) {
        distance = d;
        parent = cell.t.id;
      }
    }
    for (const auto& r : refs)
      if (r.cell == parent) return r.owner;
    throw std::logic_error("Native replacement has no parent owner.");
  }
  static bool Same(Node a, Node b) { return a.id == b.id && a.p.x == b.p.x && a.p.y == b.p.y && a.fixed == b.fixed; }
  static bool Contact(Perimeter a, Perimeter b) {
    if (!SegmentsIntersect(a.a.p, a.b.p, b.a.p, b.b.p)) return false;
    if (a.a.id != b.a.id && a.a.id != b.b.id && a.b.id != b.a.id && a.b.id != b.b.id) return true;
    // Shared endpoints may touch, but no nonshared endpoint may lie on the other segment.
    for (const auto& v : {a.a, a.b})
      if (v.id != b.a.id && v.id != b.b.id && OnSegment(b.a.p, b.b.p, v.p)) return true;
    for (const auto& v : {b.a, b.b})
      if (v.id != a.a.id && v.id != a.b.id && OnSegment(a.a.p, a.b.p, v.p)) return true;
    return edge(a.a.id, a.b.id) == edge(b.a.id, b.b.id);
  }
  std::vector<Veto> CollisionVeto(const std::vector<Perimeter>& rows) {
    std::map<int, std::array<std::map<Edge, Perimeter>, 2>> proposals;
    for (const auto& row : rows) proposals[row.caller][row.old].emplace(edge(row.a.id, row.b.id), row);
    std::vector<std::vector<Veto>> no(world.size);
    std::vector<Perimeter> changed;
    for (const auto& proposal : proposals) {
      const auto& before = proposal.second[1];
      const auto& after = proposal.second[0];
      std::vector<Perimeter> edited;
      for (const auto& entry : after)
        if (entry.second.marker && (!before.count(entry.first) || !Same(entry.second.a, before.at(entry.first).a) ||
                                    !Same(entry.second.b, before.at(entry.first).b)))
          edited.push_back(entry.second);
      if (edited.empty()) continue;
      std::map<Id, Node> successor;
      for (const auto& entry : after) successor.emplace(entry.second.a.id, entry.second.b);
      std::vector<Node> loop;
      Id current = successor.begin()->first;
      for (size_t k = 0; k < successor.size(); ++k) {
        if (!successor.count(current)) break;
        const auto next = successor.at(current);
        loop.push_back(next);
        current = next.id;
      }
      bool veto = loop.size() != after.size() || !simple(loop);
      for (const auto& cell : owned)
        for (int k = 0; k < 3; ++k)
          if (cell.second.marker[k]) {
            const Perimeter held{cell.second.t.v[k], cell.second.t.v[(k + 1) % 3], {}, cell.second.marker[k]};
            const auto key = edge(held.a.id, held.b.id);
            if (before.count(key) && before.at(key).marker) continue;
            for (const auto& face : edited) veto |= Contact(face, held);
            // Also reject swallowing an unrelated component without a segment crossing (e.g. a small hole).
            if (!veto) veto = inside(held.a.p, loop) || inside(held.b.p, loop);
          }
      if (veto) no[proposal.first].push_back({proposal.first});
      changed.insert(changed.end(), edited.begin(), edited.end());
    }
    if (world.rank == 0)
      for (size_t i = 0; i < changed.size(); ++i)
        for (size_t j = i + 1; j < changed.size(); ++j)
          if (changed[i].caller != changed[j].caller && Contact(changed[i], changed[j])) {
            const auto loser = std::max(changed[i].caller, changed[j].caller);
            no[loser].push_back({loser});
          }
    return world.exchange(no);
  }
  std::vector<Veto> ValidateStaging(const std::vector<EraseCell>& erased, const std::vector<NewCell>& fresh,
                                    const std::vector<Perimeter>& perimeter) {
    std::vector<std::vector<Veto>> no(world.size);
    std::set<Id> eraseIds, newIds;
    for (const auto& e : erased)
      if (!owned.count(e.id) || owned.at(e.id).version != e.version || !eraseIds.insert(e.id).second)
        no[e.caller].push_back({e.caller});
    for (const auto& n : fresh) {
      bool valid = !owned.count(n.cell.t.id) && newIds.insert(n.cell.t.id).second && area(n.cell.t) > 0;
      for (const auto& v : n.cell.t.v) valid = valid && std::isfinite(v.p.x) && std::isfinite(v.p.y);
      for (const auto& m : n.cell.nodal_target) valid = valid && NormalizedDeterminant(m.xx, m.xy, m.yy) > 1e-14L;
      if (!valid) no[n.caller].push_back({n.caller});
    }
    std::map<int, std::array<std::map<Edge, Perimeter>, 2>> certificates;
    for (const auto& row : perimeter)
      if (!certificates[row.caller][row.old].emplace(edge(row.a.id, row.b.id), row).second)
        no[row.caller].push_back({row.caller});
    for (const auto& certificate : certificates) {
      const auto caller = certificate.first;
      const auto& before = certificate.second[1];
      const auto& after = certificate.second[0];
      bool valid = !before.empty() && !after.empty();
      try {
        for (const auto& p : before)
          if (!p.second.marker) {
            const auto found = after.find(p.first);
            valid = valid && found != after.end();
            if (found != after.end())
              valid = valid && !found->second.marker && Same(p.second.a, found->second.a) &&
                      Same(p.second.b, found->second.b);
          }
        for (const auto& p : after) {
          const auto& row = p.second;
          if (!row.marker)
            valid = valid && before.count(p.first) && !before.at(p.first).marker;
          else {
            const Physical face{row.a, row.b, row.marker};
            const auto interval = reference.interval(row.marker, row.a.p, row.b.p);
            valid = valid && reference.deviation(face) <= options.geometry_tolerance;
            for (const auto point :
                 std::array<std::pair<Node, double>, 2>{{{row.a, interval.first}, {row.b, interval.second}}}) {
              const auto projected = reference.point(row.marker, point.second);
              const auto scale =
                  std::max({norm(row.b.p - row.a.p), std::abs(point.first.p.x), std::abs(point.first.p.y)});
              valid = valid && norm(projected - point.first.p) <= 32 * std::numeric_limits<double>::epsilon() * scale;
            }
            const auto h = reference.height(row.marker);
            if (h > 0)
              valid = valid &&
                      std::abs(static_cast<double>(orient(row.a.p, row.b.p, row.apex.p)) / norm(row.b.p - row.a.p) / h -
                               1) < 1e-8;
          }
        }
        auto bind = [&](const Cell& cell, const std::map<Edge, Perimeter>& rows) {
          for (int k = 0; k < 3; ++k) {
            const auto found = rows.find(edge(cell.t.v[k].id, cell.t.v[(k + 1) % 3].id));
            if (found != rows.end())
              valid = valid && Same(found->second.a, cell.t.v[k]) && Same(found->second.b, cell.t.v[(k + 1) % 3]) &&
                      Same(found->second.apex, cell.t.v[(k + 2) % 3]) && found->second.marker == cell.marker[k];
          }
        };
        for (const auto& e : erased)
          if (e.caller == caller && owned.count(e.id)) bind(owned.at(e.id), before);
        for (const auto& n : fresh)
          if (n.caller == caller) bind(n.cell, after);
      } catch (const std::exception&) {
        valid = false;
      }
      if (!valid) no[caller].push_back({caller});
    }
    return world.exchange(no);
  }
};

}  // namespace SU2NativeBoundary2D
