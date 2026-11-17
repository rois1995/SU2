/*
 * \file CNativeLaneCensus3D.hpp
 * \brief Print-only attribution of existing candidate calls and published edits.
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#pragma once
#include "CNativeDistributed3D.hpp"
#include <algorithm>
#include <array>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <set>

namespace SU2NativeLaneCensus3D {
using Clock = std::chrono::steady_clock;

enum class WallClass : unsigned { S, W1, W2M, W2O };

struct WallSlot {
  WallClass classification = WallClass::W2O;
  unsigned erased = 0, inserted = 0, ladder = 3;
  bool described = false, link_local = false, first_proof = false, same_skin = false;
};

struct Slot {
  unsigned category = 0;
  bool reached = false;
  bool no_parameter_edits = true;
  double seconds = 0;
  bool required_primary_split = false, checked_primary_split = false;
  WallSlot wall;
};

struct Record {
  uint64_t trials = 0, approved = 0, commits = 0, work = 0;
  uint64_t same_skin = 0, no_parameter_edits = 0, no_coverage = 0;
  double seconds = 0;
};

struct WallRecord {
  uint64_t commits = 0, class_s = 0, class_w1 = 0, class_w2m = 0, class_w2o = 0;
  uint64_t w1_kind0 = 0, w1_kind1 = 0, w1_kind2 = 0, w2m_kind1 = 0;
  uint64_t w1_link_local = 0, w2m_link_local = 0, w1_first_proof = 0;
  uint64_t w1_link_local_first_proof = 0;
  double seconds_s = 0, seconds_w1 = 0, seconds_w2m = 0, seconds_w2o = 0;
  double seconds_w1_link_local = 0, seconds_w1_link_local_first_proof = 0, seconds_w2m_link_local = 0;
};

inline unsigned Classify(const SU2Native3D::Dependencies& dependencies, const SU2Native3D::Imported& imported,
                         int rank) {
  bool local = true, wall = false;
  for (const auto& cell : dependencies.cells) local = local && cell.owner == rank;
  for (const auto& cell : imported.cells)
    for (const auto& facet : cell.physical) wall = wall || facet.present;
  return (local ? 0u : 2u) + unsigned(wall);
}

inline std::pair<SU2Native3D::Id, SU2Native3D::Face> FaceKey(const SU2Native3D::SurfaceFace& face) {
  auto vertices = face.v;
  std::sort(vertices.begin(), vertices.end());
  return {face.facet, vertices};
}

inline bool HasWallFace(const SU2Native3D::Patch& patch, const SU2Native3D::SurfaceFace& face) {
  const auto key = FaceKey(face);
  return std::any_of(patch.surface.begin(), patch.surface.end(),
                     [&](const auto& candidate) { return FaceKey(candidate) == key; });
}

inline std::pair<unsigned, unsigned> WallEditCounts(const SU2Native3D::Patch& old_patch,
                                                    const SU2Native3D::Patch& fresh_patch,
                                                    const std::vector<SU2Native3D::BoundaryParameterEdit>& edits,
                                                    const SU2Native3D::BoundaryParameterEdit*& inserted_wall_point) {
  std::set<SU2Native3D::Id> old_vertices, fresh_vertices;
  for (const auto& face : old_patch.surface) old_vertices.insert(face.v.begin(), face.v.end());
  for (const auto& face : fresh_patch.surface) fresh_vertices.insert(face.v.begin(), face.v.end());
  unsigned erased = 0, inserted = 0;
  inserted_wall_point = nullptr;
  for (const auto& edit : edits) {
    if (edit.insert && fresh_vertices.count(edit.point.node.id)) {
      ++inserted;
      inserted_wall_point = &edit;
    } else if (!edit.insert && old_vertices.count(edit.point.node.id)) {
      ++erased;
    }
  }
  return {erased, inserted};
}

inline bool FirstProof(const SU2Native3D::Patch& old_patch, const SU2Native3D::Patch& fresh_patch,
                       const SU2Native3D::BoundaryParameterEdit* inserted_wall_point) {
  if (!inserted_wall_point) return false;
  for (const auto& face : old_patch.surface) {
    if (HasWallFace(fresh_patch, face)) continue;
    std::array<SU2Native3D::Point, 3> points{};
    for (unsigned i = 0; i < 3; ++i) {
      const auto found = old_patch.parameters.find(face.v[i]);
      if (found == old_patch.parameters.end()) return false;
      points[i] = found->second.node.p;
    }
    if (SU2Native3D::Orientation(points[0], points[1], points[2], inserted_wall_point->point.node.p) < 0) return false;
  }
  return true;
}

inline void DescribeWall(Slot& slot, const SU2Native3D::Patch& old_patch, const SU2Native3D::Patch& fresh_patch,
                         const std::vector<SU2Native3D::BoundaryParameterEdit>& edits, bool same_skin, bool no_coverage,
                         bool link_local, unsigned ladder) {
  if (!slot.reached || slot.category != 1) return;
  const SU2Native3D::BoundaryParameterEdit* inserted_wall_point = nullptr;
  const auto counts = WallEditCounts(old_patch, fresh_patch, edits, inserted_wall_point);
  slot.wall.erased = counts.first;
  slot.wall.inserted = counts.second;
  slot.wall.same_skin = same_skin;
  slot.wall.link_local = link_local;
  slot.wall.ladder = ladder;
  const bool no_edits = edits.empty();
  if (same_skin && no_edits && no_coverage) {
    slot.wall.classification = WallClass::S;
  } else if (counts.first <= 1 && counts.second <= 1) {
    slot.wall.classification = WallClass::W1;
  } else if (counts.first == counts.second && counts.first >= 2 &&
             old_patch.surface.size() == fresh_patch.surface.size()) {
    slot.wall.classification = WallClass::W2M;
  } else {
    slot.wall.classification = WallClass::W2O;
  }
  slot.wall.first_proof = slot.wall.classification == WallClass::W1 && counts.second == 1 &&
                          FirstProof(old_patch, fresh_patch, inserted_wall_point);
  slot.wall.described = true;
}

class Counters {
 public:
  explicit Counters(bool enabled) : enabled(enabled) {}

  Record* Begin(bool quality, Slot& slot, const SU2Native3D::Dependencies& dependencies,
                const SU2Native3D::Imported& imported, int rank) {
    if (!enabled) return nullptr;
    slot.category = Classify(dependencies, imported, rank);
    slot.reached = true;
    slot.seconds = 0;
    slot.required_primary_split = false;
    slot.checked_primary_split = false;
    slot.wall = WallSlot{};
    auto& row = records[quality][slot.category];
    ++row.trials;
    return &row;
  }

  void Approved(bool quality, const Slot& slot, bool approved) {
    if (enabled && slot.reached && approved) ++records[quality][slot.category].approved;
  }

  void Commit(bool quality, const Slot& slot, bool same_skin, bool no_parameters, bool no_coverage) {
    if (!enabled || !slot.reached) return;
    auto& row = records[quality][slot.category];
    ++row.commits;
    row.same_skin += same_skin;
    row.no_parameter_edits += no_parameters;
    row.no_coverage += no_coverage;
  }

  void CommitWall(bool quality, const Slot& slot) {
    if (!enabled || !quality || !slot.reached || slot.category != 1 || !slot.wall.described) return;
    auto& row = wall_record;
    ++row.commits;
    switch (slot.wall.classification) {
      case WallClass::S:
        ++row.class_s;
        row.seconds_s += slot.seconds;
        break;
      case WallClass::W1:
        ++row.class_w1;
        row.seconds_w1 += slot.seconds;
        row.w1_kind0 += slot.wall.ladder == 0;
        row.w1_kind1 += slot.wall.ladder == 1;
        row.w1_kind2 += slot.wall.ladder == 2;
        row.w1_link_local += slot.wall.link_local;
        row.w1_first_proof += slot.wall.first_proof;
        row.w1_link_local_first_proof += slot.wall.link_local && slot.wall.first_proof;
        if (slot.wall.link_local) row.seconds_w1_link_local += slot.seconds;
        if (slot.wall.link_local && slot.wall.first_proof) row.seconds_w1_link_local_first_proof += slot.seconds;
        break;
      case WallClass::W2M:
        ++row.class_w2m;
        row.seconds_w2m += slot.seconds;
        row.w2m_kind1 += slot.wall.ladder == 1;
        row.w2m_link_local += slot.wall.link_local;
        if (slot.wall.link_local) row.seconds_w2m_link_local += slot.seconds;
        break;
      case WallClass::W2O:
        ++row.class_w2o;
        row.seconds_w2o += slot.seconds;
        break;
    }
  }

  void Print(int rank, std::ostream& out = std::cout) const {
    if (!enabled) return;
    const auto precision = out.precision();
    out << std::setprecision(17);
    const char* names[] = {"local_interior", "local_wall", "shared_interior", "shared_wall"};
    for (unsigned stage = 0; stage < 2; ++stage)
      for (unsigned category = 0; category < 4; ++category) {
        const auto& row = records[stage][category];
        out << "NATIVE3D_LANE_CENSUS rank=" << rank << " stage=" << (stage ? "quality" : "construction")
            << " class=" << names[category] << " trials=" << row.trials << " approved=" << row.approved
            << " commits=" << row.commits << " seconds=" << row.seconds << " work=" << row.work
            << " same_skin=" << row.same_skin << " no_parameter_edits=" << row.no_parameter_edits
            << " no_coverage=" << row.no_coverage << '\n';
      }
    PrintWall(rank, out);
    out.precision(precision);
  }

  std::array<std::array<Record, 4>, 2> records{};
  WallRecord wall_record{};

 private:
  void PrintWall(int rank, std::ostream& out) const {
    const auto& row = wall_record;
    out << "NATIVE3D_WALL_LANE_CENSUS rank=" << rank << " commits=" << row.commits << " class_s=" << row.class_s
        << " class_w1=" << row.class_w1 << " class_w2m=" << row.class_w2m << " class_w2o=" << row.class_w2o
        << " w1_kind0=" << row.w1_kind0 << " w1_kind1=" << row.w1_kind1 << " w1_kind2=" << row.w1_kind2
        << " w2m_kind1=" << row.w2m_kind1 << " w1_link_local=" << row.w1_link_local
        << " w2m_link_local=" << row.w2m_link_local << " w1_first_proof=" << row.w1_first_proof
        << " w1_link_local_first_proof=" << row.w1_link_local_first_proof << " seconds_s=" << row.seconds_s
        << " seconds_w1=" << row.seconds_w1 << " seconds_w2m=" << row.seconds_w2m << " seconds_w2o=" << row.seconds_w2o
        << " seconds_w1_link_local=" << row.seconds_w1_link_local
        << " seconds_w1_link_local_first_proof=" << row.seconds_w1_link_local_first_proof
        << " seconds_w2m_link_local=" << row.seconds_w2m_link_local << '\n';
  }

  bool enabled;
};

class Trial {
 public:
  Trial(Counters& counters, bool quality, Slot& slot, const SU2Native3D::Dependencies& dependencies,
        const SU2Native3D::Imported& imported, int rank, const size_t& work)
      : row(counters.Begin(quality, slot, dependencies, imported, rank)), slot(&slot), quality(quality) {
    if (!row) return;
    used = &work;
    before = work;
    started = Clock::now();
  }

  ~Trial() {
    if (!row) return;
    const double elapsed = std::chrono::duration<double>(Clock::now() - started).count();
    row->seconds += elapsed;
    if (quality && slot->category == 1) slot->seconds += elapsed;
    row->work += *used - before;
  }

  Trial(const Trial&) = delete;
  Trial& operator=(const Trial&) = delete;

 private:
  Record* row = nullptr;
  Slot* slot = nullptr;
  bool quality = false;
  const size_t* used = nullptr;
  size_t before = 0;
  Clock::time_point started{};
};
}  // namespace SU2NativeLaneCensus3D
