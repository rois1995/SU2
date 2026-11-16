/*!
 * \file CNativeLaneCensus3D.hpp
 * \brief Print-only attribution of existing candidate calls and published edits.
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#pragma once
#include "CNativeDistributed3D.hpp"
#include <array>
#include <chrono>
#include <iomanip>
#include <iostream>

namespace SU2NativeLaneCensus3D {
using Clock = std::chrono::steady_clock;

struct Slot {
  unsigned category = 0;
  bool reached = false;
  bool no_parameter_edits = true;
};

struct Record {
  uint64_t trials = 0, approved = 0, commits = 0, work = 0;
  uint64_t same_skin = 0, no_parameter_edits = 0, no_coverage = 0;
  double seconds = 0;
};

inline unsigned Classify(const SU2Native3D::Dependencies& dependencies, const SU2Native3D::Imported& imported,
                         int rank) {
  bool local = true, wall = false;
  for (const auto& cell : dependencies.cells) local = local && cell.owner == rank;
  for (const auto& cell : imported.cells)
    for (const auto& facet : cell.physical) wall = wall || facet.present;
  return (local ? 0u : 2u) + unsigned(wall);
}

class Counters {
 public:
  explicit Counters(bool enabled) : enabled(enabled) {}

  Record* Begin(bool quality, Slot& slot, const SU2Native3D::Dependencies& dependencies,
                const SU2Native3D::Imported& imported, int rank) {
    if (!enabled) return nullptr;
    slot.category = Classify(dependencies, imported, rank);
    slot.reached = true;
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
    out.precision(precision);
  }

  std::array<std::array<Record, 4>, 2> records{};

 private:
  bool enabled;
};

class Trial {
 public:
  Trial(Counters& counters, bool quality, Slot& slot, const SU2Native3D::Dependencies& dependencies,
        const SU2Native3D::Imported& imported, int rank, const size_t& work)
      : row(counters.Begin(quality, slot, dependencies, imported, rank)) {
    if (!row) return;
    used = &work;
    before = work;
    started = Clock::now();
  }

  ~Trial() {
    if (!row) return;
    row->seconds += std::chrono::duration<double>(Clock::now() - started).count();
    row->work += *used - before;
  }

  Trial(const Trial&) = delete;
  Trial& operator=(const Trial&) = delete;

 private:
  Record* row = nullptr;
  const size_t* used = nullptr;
  size_t before = 0;
  Clock::time_point started{};
};
}  // namespace SU2NativeLaneCensus3D
