/*!
 * \file CNativeBalanceProfile2D.hpp
 * \brief Bounded, opt-in observations of private reconstruction; never controls adaptation.
 * \version 8.5.0 "Harrier"
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md)
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */
#pragma once
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <ctime>

namespace SU2NativeBoundary2D {
struct BalanceSample {
  uint64_t round = 0, seed_a = 0, seed_b = 0, cells = 0, boundary_cells = 0;
  uint64_t requests = 0, evaluations = 0, evictions = 0;
  int action = 0, coordinated = 0, reconstructed = 0, committed = 0;
  double wall_seconds = 0, cpu_seconds = -1;
  double xmin = 0, ymin = 0, xmax = 0, ymax = 0;
};
struct BalanceOperation {
  uint64_t selected = 0, attempts = 0, reconstructed = 0, committed = 0;
  uint64_t cpu_samples = 0, requests = 0, evaluations = 0, evictions = 0, cells = 0, max_cells = 0;
  double wall_seconds = 0, cpu_seconds = 0, longest_seconds = 0;
};
struct BalanceProfile {
  static constexpr size_t HOTSPOTS = 32;
  std::array<BalanceOperation, 8> operations{};
  std::array<BalanceSample, HOTSPOTS> hotspots{};
  size_t hotspot_count = 0;

  static double CpuSeconds(std::clock_t start, std::clock_t end) noexcept {
    return start == std::clock_t(-1) || end == std::clock_t(-1) || end < start
               ? -1 : double(end - start) / CLOCKS_PER_SEC;
  }
  void Record(const BalanceSample& sample) noexcept {
    auto& op = operations[sample.action];
    ++op.attempts;
    op.reconstructed += sample.reconstructed;
    op.committed += sample.committed;
    op.wall_seconds += sample.wall_seconds;
    if (sample.cpu_seconds >= 0) {
      ++op.cpu_samples;
      op.cpu_seconds += sample.cpu_seconds;
    }
    op.longest_seconds = std::max(op.longest_seconds, sample.wall_seconds);
    op.requests += sample.requests;
    op.evaluations += sample.evaluations;
    op.evictions += sample.evictions;
    op.cells += sample.cells;
    op.max_cells = std::max(op.max_cells, sample.cells);
    // No allocation or metric queries after the transaction's publication vote.
    size_t slot = hotspot_count;
    if (slot == HOTSPOTS) {
      slot = 0;
      for (size_t i = 1; i < HOTSPOTS; ++i)
        if (hotspots[i].wall_seconds < hotspots[slot].wall_seconds) slot = i;
      if (sample.wall_seconds <= hotspots[slot].wall_seconds) return;
    } else {
      ++hotspot_count;
    }
    hotspots[slot] = sample;
  }
};
}  // namespace SU2NativeBoundary2D
