/*!
 * \file CNativeRuntimeProfile3D.hpp
 * \brief Opt-in per-rank native tetrahedral lifecycle scopes; nested records never add to parents.
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md).
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */
#pragma once
#include "../parallelization/CPassiveComm.hpp"
#include <array>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <sstream>
#if defined(__unix__)
#include <sys/resource.h>
#endif
namespace SU2Native3D {
inline size_t ProcessPeakRSS3D() {
#if defined(__unix__)
  struct rusage usage {};
  if (!getrusage(RUSAGE_SELF, &usage)) {
#if defined(__APPLE__)
    return usage.ru_maxrss;
#else
    return size_t(usage.ru_maxrss) * 1024;
#endif
  }
#endif
  return 0;
}
inline double ProfileRow3D(const std::string& row, SU2_Comm comm = SU2_MPI::GetComm()) {
  const auto begin = SU2_MPI::Wtime();
  const auto all = CPassiveComm::Allgatherv(std::vector<char>(row.begin(), row.end()), nullptr, comm);
  if (!SU2_MPI::GetRank()) std::cout.write(all.data(), all.size());
  return SU2_MPI::Wtime() - begin;
}
/*! Every rank calls Report together. OFF performs no clocks, allocations or profile collectives.
 * total excludes this record's emission; a parent scope includes nested records' emission.
 * CPU is process CPU (including MPI progress), never inferred private CPU or MPI waiting.
 * Process RSS is a lifetime high-water mark, not an event-local allocation bound. */
template <size_t N>
class RuntimeProfile3D {
  static_assert(N <= 16, "Lifecycle scopes must remain bounded.");
  bool enabled;
  const char* stage;
  double begin = 0, now = 0;
  std::clock_t cpu = 0, current_cpu = 0;
  std::array<double, N> phases{}, cpus{};
  static double CPU(std::clock_t a, std::clock_t b) {
    return a == std::clock_t(-1) || b == std::clock_t(-1) || b < a ? -1. : double(b - a) / CLOCKS_PER_SEC;
  }

 public:
  RuntimeProfile3D(bool active, const char* name) : enabled(active), stage(name) {
    if (enabled) {
      begin = now = SU2_MPI::Wtime();
      cpu = current_cpu = std::clock();
    }
  }
  void Mark(size_t phase) {
    if (!enabled) return;
    const auto end = SU2_MPI::Wtime();
    const auto next_cpu = std::clock();
    phases.at(phase) += end - now;
    cpus.at(phase) += CPU(current_cpu, next_cpu);
    now = end;
    current_cpu = next_cpu;
  }
  double Report(unsigned long time_iter, const std::array<const char*, N>& names, const std::string& extra = {}) const {
    if (!enabled) return 0;
    const auto total = SU2_MPI::Wtime() - begin;
    const auto total_cpu = CPU(cpu, std::clock());
    std::ostringstream row;
    row << std::setprecision(17) << "NATIVE3D_STAGE stage=" << stage << " rank=" << SU2_MPI::GetRank()
        << " ranks=" << SU2_MPI::GetSize() << " time_iter=" << time_iter << " total=" << total
        << " other=" << total - std::accumulate(phases.begin(), phases.end(), 0.) << " cpu=" << total_cpu
        << " cpu_other=" << total_cpu - std::accumulate(cpus.begin(), cpus.end(), 0.)
        << " process_peak_rss=" << ProcessPeakRSS3D();
    for (size_t i = 0; i < N; ++i) row << ' ' << names[i] << '=' << phases[i] << ' ' << names[i] << "_cpu=" << cpus[i];
    row << extra << '\n';
    return ProfileRow3D(row.str());
  }
};
}  // namespace SU2Native3D
